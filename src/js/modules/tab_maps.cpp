/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>
License: MIT
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "tab_maps.h"
#include "../log.h"
#include "../core.h"
#include "../constants.h"
#include "../install-type.h"
#include "../modules.h"
#include "../casc/listfile.h"
#include "../casc/db2.h"
#include "../casc/blp.h"
#include "../casc/export-helper.h"
#include "../casc/casc-source.h"
#include "../3D/loaders/WDTLoader.h"
#include "../3D/loaders/ADTLoader.h"
#include "../3D/loaders/WMOLoader.h"
#include "../3D/exporters/ADTExporter.h"
#include "../3D/exporters/WMOExporter.h"
#include "../tiled-png-writer.h"
#include "../png-writer.h"
#include "../file-writer.h"
#include "../buffer.h"
#include "../db/WDCReader.h"
#include "../components/listbox-maps.h"
#include "../components/context-menu.h"
#include "../components/map-viewer.h"
#include "../components/menu-button.h"
#include "../../app.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <future>
#include <limits>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

#include <openssl/evp.h>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

namespace tab_maps {

static std::optional<std::string> build_stack_trace(const char* function_name, const std::exception& e) {
	return std::format("{}: {}", function_name, e.what());
}

static std::string md5_hex(const std::string& input) {
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	if (!ctx)
		return {};
	EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
	EVP_DigestUpdate(ctx, input.data(), input.size());
	uint8_t digest[EVP_MAX_MD_SIZE];
	unsigned int digest_len = 0;
	EVP_DigestFinal_ex(ctx, digest, &digest_len);
	EVP_MD_CTX_free(ctx);
	static constexpr char hex_chars[] = "0123456789abcdef";
	std::string result;
	result.reserve(digest_len * 2);
	for (unsigned int i = 0; i < digest_len; i++) {
		result.push_back(hex_chars[(digest[i] >> 4) & 0x0F]);
		result.push_back(hex_chars[digest[i] & 0x0F]);
	}
	return result;
}

static constexpr double TILE_SIZE = constants::GAME::TILE_SIZE;

static constexpr int MAP_OFFSET = constants::GAME::MAP_OFFSET;

static std::optional<int> selected_map_id;

static std::string selected_map_dir;
static bool has_selected_map_dir = false;

static std::unique_ptr<WDTLoader> selected_wdt;
static BufferWrapper selected_wdt_data;

static bool game_objects_db2_loaded = false;
static std::unordered_map<uint32_t, std::vector<db::DataRecord>> game_objects_db2;

static bool wmo_minimap_textures_loaded = false;
static std::unordered_map<uint32_t, std::vector<db::DataRecord>> wmo_minimap_textures;

static std::optional<WMOMinimapData> current_wmo_minimap;

static std::string prev_selection_str;
static bool tab_initialized = false;
static bool tab_initializing = false;

static listbox_maps::ListboxMapsState listbox_state;
static context_menu::ContextMenuState context_menu_state;
static map_viewer::MapViewerState map_viewer_state;
static menu_button::MenuButtonState menu_button_export_state;
static menu_button::MenuButtonState menu_button_quality_state;
static menu_button::MenuButtonState menu_button_heightmap_res_state;
static menu_button::MenuButtonState menu_button_heightmap_depth_state;

static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

static uint32_t fieldToUint32(const db::FieldValue& val) {
return std::visit([](const auto& v) -> uint32_t {
using T = std::decay_t<decltype(v)>;
if constexpr (std::is_arithmetic_v<T>)
return static_cast<uint32_t>(v);
return 0;
}, val);
}

static int32_t fieldToInt32(const db::FieldValue& val) {
return std::visit([](const auto& v) -> int32_t {
using T = std::decay_t<decltype(v)>;
if constexpr (std::is_arithmetic_v<T>)
return static_cast<int32_t>(v);
return 0;
}, val);
}

static float fieldToFloat(const db::FieldValue& val) {
return std::visit([](const auto& v) -> float {
using T = std::decay_t<decltype(v)>;
if constexpr (std::is_arithmetic_v<T>)
return static_cast<float>(v);
return 0.0f;
}, val);
}

static std::string fieldToString(const db::FieldValue& val) {
return std::visit([](const auto& v) -> std::string {
using T = std::decay_t<decltype(v)>;
if constexpr (std::is_same_v<T, std::string>)
return v;
return "";
}, val);
}

static std::vector<float> fieldToFloatVec(const db::FieldValue& val) {
return std::visit([](const auto& v) -> std::vector<float> {
using T = std::decay_t<decltype(v)>;
if constexpr (std::is_same_v<T, std::vector<float>>)
return v;
if constexpr (std::is_same_v<T, std::vector<uint32_t>>) {
std::vector<float> result;
result.reserve(v.size());
for (auto x : v)
result.push_back(static_cast<float>(x));
return result;
}
return {};
}, val);
}

/**
 * Parse a map entry from the listbox.
 * @param entry
 */
static MapEntry parse_map_entry(const std::string& entry) {
std::regex re(R"(\[(\d+)\]\x19([^\x19]+)\x19\(([^)]+)\))");
std::smatch match;
if (!std::regex_search(entry, match, re))
throw std::runtime_error("unexpected map entry");

MapEntry result;
result.id = std::stoi(match[1].str());
result.name = match[2].str();
result.dir = match[3].str();
return result;
}

/**
 * Load a map tile.
 *
 * Returns RGBA pixel data (size * size * 4 bytes), or empty if not available.
 */
static std::vector<uint8_t> load_map_tile(int x, int y, int size) {
if (!has_selected_map_dir)
return {};

try {
const std::string padded_x = std::format("{:02d}", x);
const std::string padded_y = std::format("{:02d}", y);
const std::string tile_path = std::format("world/minimaps/{}/map{}_{}.blp", selected_map_dir, padded_x, padded_y);

BufferWrapper data = core::view->casc->getVirtualFileByName(tile_path, true);
casc::BLPImage blp(data);
auto rgba = blp.toUInt8Array(0, 0b0111);

uint32_t blp_width = blp.getScaledWidth();
uint32_t blp_height = blp.getScaledHeight();

if (static_cast<int>(blp_width) == size && static_cast<int>(blp_height) == size)
return rgba;

std::vector<uint8_t> scaled(size * size * 4, 0);
const float scale_x = static_cast<float>(blp_width) / static_cast<float>(size);
const float scale_y = static_cast<float>(blp_height) / static_cast<float>(size);

for (int py = 0; py < size; py++) {
for (int px = 0; px < size; px++) {
const float src_xf = (px + 0.5f) * scale_x - 0.5f;
const float src_yf = (py + 0.5f) * scale_y - 0.5f;
const int x0 = (std::max)(0, static_cast<int>(std::floor(src_xf)));
const int y0 = (std::max)(0, static_cast<int>(std::floor(src_yf)));
const int x1 = (std::min)(static_cast<int>(blp_width) - 1, x0 + 1);
const int y1 = (std::min)(static_cast<int>(blp_height) - 1, y0 + 1);
const float fx = src_xf - x0;
const float fy = src_yf - y0;
const int dst_idx = (py * size + px) * 4;
for (int c = 0; c < 4; c++) {
const float top = rgba[(y0 * blp_width + x0) * 4 + c] * (1.0f - fx)
                + rgba[(y0 * blp_width + x1) * 4 + c] * fx;
const float bot = rgba[(y1 * blp_width + x0) * 4 + c] * (1.0f - fx)
                + rgba[(y1 * blp_width + x1) * 4 + c] * fx;
scaled[dst_idx + c] = static_cast<uint8_t>(std::round(top * (1.0f - fy) + bot * fy));
}
}
}

return scaled;
} catch (...) {
return {};
}
}

/**
 * Load a WMO minimap tile.
 * Composites multiple tiles at the same position using alpha blending.
 */
static std::vector<uint8_t> load_wmo_minimap_tile(int x, int y, int size) {
if (!current_wmo_minimap)
return {};

const std::string key = std::format("{},{}", x, y);
auto it = current_wmo_minimap->tiles_by_coord.find(key);
if (it == current_wmo_minimap->tiles_by_coord.end() || it->second.empty())
return {};

try {
std::vector<uint8_t> composite(size * size * 4, 0);
float output_scale = static_cast<float>(size) / static_cast<float>(current_wmo_minimap->output_tile_size);

const auto& tile_list = it->second;
for (const auto& tile : tile_list) {
BufferWrapper data = core::view->casc->getVirtualFileByID(tile.fileDataID, true);
casc::BLPImage blp(data);
auto rgba = blp.toUInt8Array(0, 0b1111);

uint32_t blp_w = blp.getScaledWidth();
uint32_t blp_h = blp.getScaledHeight();

const float draw_x = tile.drawX * output_scale;
const float draw_y = tile.drawY * output_scale;
const float draw_w = blp_w * tile.scaleX * output_scale;
const float draw_h = blp_h * tile.scaleY * output_scale;

const int dst_x0 = (std::max)(0, static_cast<int>(std::floor(draw_x)));
const int dst_y0 = (std::max)(0, static_cast<int>(std::floor(draw_y)));
const int dst_x1 = (std::min)(size, static_cast<int>(std::ceil(draw_x + draw_w)));
const int dst_y1 = (std::min)(size, static_cast<int>(std::ceil(draw_y + draw_h)));

if (draw_w <= 0.0f || draw_h <= 0.0f)
continue;

const float inv_scale_x = static_cast<float>(blp_w) / draw_w;
const float inv_scale_y = static_cast<float>(blp_h) / draw_h;

for (int py = dst_y0; py < dst_y1; py++) {
for (int px = dst_x0; px < dst_x1; px++) {
int src_x = (std::min)(static_cast<int>((px - draw_x) * inv_scale_x), static_cast<int>(blp_w) - 1);
int src_y = (std::min)(static_cast<int>((py - draw_y) * inv_scale_y), static_cast<int>(blp_h) - 1);
if (src_x < 0 || src_y < 0)
continue;
int src_idx = (src_y * blp_w + src_x) * 4;
int dst_idx = (py * size + px) * 4;

uint8_t src_a = rgba[src_idx + 3];
if (src_a == 0)
continue;

if (src_a == 255) {
composite[dst_idx + 0] = rgba[src_idx + 0];
composite[dst_idx + 1] = rgba[src_idx + 1];
composite[dst_idx + 2] = rgba[src_idx + 2];
composite[dst_idx + 3] = 255;
} else {
uint8_t dst_a = composite[dst_idx + 3];
uint8_t out_a = src_a + dst_a * (255 - src_a) / 255;
if (out_a > 0) {
composite[dst_idx + 0] = static_cast<uint8_t>((rgba[src_idx + 0] * src_a + composite[dst_idx + 0] * dst_a * (255 - src_a) / 255) / out_a);
composite[dst_idx + 1] = static_cast<uint8_t>((rgba[src_idx + 1] * src_a + composite[dst_idx + 1] * dst_a * (255 - src_a) / 255) / out_a);
composite[dst_idx + 2] = static_cast<uint8_t>((rgba[src_idx + 2] * src_a + composite[dst_idx + 2] * dst_a * (255 - src_a) / 255) / out_a);
}
composite[dst_idx + 3] = out_a;
}
}
}
}

return composite;
} catch (...) {
return {};
}
}

/**
 * Collect game objects from GameObjects.db2 for export.
 * @param mapID
 * @param filter
 */
static std::vector<ADTGameObject> collect_game_objects(uint32_t mapID,
const std::function<bool(const db::DataRecord&)>& filter)
{
if (!game_objects_db2_loaded) {
game_objects_db2_loaded = true;
game_objects_db2.clear();

auto& go_table = casc::db2::getTable("GameObjects");
auto all_rows = go_table.getAllRows();

auto& go_display_table = casc::db2::getTable("GameObjectDisplayInfo");

for (auto& [id, row] : all_rows) {
auto display_it = row.find("DisplayID");
if (display_it == row.end())
continue;

uint32_t display_id = fieldToUint32(display_it->second);
auto fid_row = go_display_table.getRow(display_id);
if (fid_row.has_value()) {
auto fid_it = fid_row->find("FileDataID");
if (fid_it != fid_row->end()) {
uint32_t file_data_id = fieldToUint32(fid_it->second);
row["FileDataID"] = static_cast<uint64_t>(file_data_id);

auto owner_it = row.find("OwnerID");
if (owner_it != row.end()) {
uint32_t owner_id = fieldToUint32(owner_it->second);
game_objects_db2[owner_id].push_back(row);
}
}
}
}
}

std::vector<ADTGameObject> result;
auto it = game_objects_db2.find(mapID);
if (it != game_objects_db2.end()) {
for (const auto& obj : it->second) {
if (filter && filter(obj)) {
ADTGameObject go;
auto fid_it = obj.find("FileDataID");
auto pos_it = obj.find("Pos");
auto rot_it = obj.find("Rot");
auto scale_it = obj.find("Scale");

if (fid_it != obj.end())
go.FileDataID = fieldToUint32(fid_it->second);
if (pos_it != obj.end())
go.Position = fieldToFloatVec(pos_it->second);
if (rot_it != obj.end())
go.Rotation = fieldToFloatVec(rot_it->second);
if (scale_it != obj.end())
go.scale = fieldToFloat(scale_it->second);

result.push_back(go);
}
}
}

return result;
}

/**
 * Sample height at a specific position within a chunk using bilinear interpolation.
 * @param chunk
 * @param localX
 * @param localY
 * @returns interpolated height
 */
static int get_vert_idx(int x, int y) {
int index = 0;
for (int row = 0; row < y * 2; row++)
index += (row % 2) ? 8 : 9;

bool is_short = !!((y * 2) % 2);
index += is_short ? (std::min)(x, 7) : (std::min)(x, 8);
return index;
}

static float sample_chunk_height(const ::ADTChunk& chunk, float localX, float localY) {
float vx = localX * 8.0f;
float vy = localY * 8.0f;

int x0 = static_cast<int>(std::floor(vx));
int y0 = static_cast<int>(std::floor(vy));
int x1 = (std::min)(8, x0 + 1);
int y1 = (std::min)(8, y0 + 1);

float base_z = (chunk.position.size() > 2) ? chunk.position[2] : 0.0f;

auto get_height = [&](int cx, int cy) -> float {
int idx = get_vert_idx(cx, cy);
if (idx >= 0 && static_cast<size_t>(idx) < chunk.vertices.size())
return chunk.vertices[idx] + base_z;
return base_z;
};

float h00 = get_height(x0, y0);
float h10 = get_height(x1, y0);
float h01 = get_height(x0, y1);
float h11 = get_height(x1, y1);

float fx = vx - x0;
float fy = vy - y0;

float h0 = h00 * (1.0f - fx) + h10 * fx;
float h1 = h01 * (1.0f - fx) + h11 * fx;

return h0 * (1.0f - fy) + h1 * fy;
}

/**
 * Extract height data from a terrain tile.
 * @param map_dir
 * @param tile_x
 * @param tile_y
 * @param resolution
 * @returns optional height data
 */
static std::optional<HeightData> extract_height_data_from_tile(
const std::string& map_dir, uint32_t tile_x, uint32_t tile_y, int resolution)
{
const std::string prefix = std::format("world/maps/{}/{}", map_dir, map_dir);
const std::string tile_prefix = prefix + '_' + std::to_string(tile_x) + '_' + std::to_string(tile_y);

try {
auto root_fid = casc::listfile::getByFilename(tile_prefix + ".adt");
if (!root_fid) {
logging::write(std::format("cannot find fileDataID for {}.adt", tile_prefix));
return std::nullopt;
}

BufferWrapper root_file = core::view->casc->getVirtualFileByID(*root_fid);
ADTLoader root_adt(root_file);
root_adt.loadRoot();

if (root_adt.chunks.empty()) {
logging::write(std::format("no chunks found in ADT file {}", tile_prefix));
return std::nullopt;
}

std::vector<float> heights(resolution * resolution, 0.0f);
for (int py = 0; py < resolution; py++) {
for (int px = 0; px < resolution; px++) {
int chunk_x = static_cast<int>(std::floor(px * 16.0 / resolution));
int chunk_y = static_cast<int>(std::floor(py * 16.0 / resolution));
int chunk_index = chunk_y * 16 + chunk_x;

if (chunk_index >= static_cast<int>(root_adt.chunks.size()))
	continue;

const auto& chunk = root_adt.chunks[chunk_index];
if (chunk.vertices.empty())
	continue;

float local_x = static_cast<float>(px * 16.0 / resolution) - chunk_x;
float local_y = static_cast<float>(py * 16.0 / resolution) - chunk_y;

float height = sample_chunk_height(chunk, local_x, local_y);

// rotate 90° CW to align with terrain mesh coordinate system
int height_idx = (resolution - 1 - px) * resolution + py;
heights[height_idx] = height;
}
}

HeightData result;
result.heights = std::move(heights);
result.resolution = resolution;
result.tileX = tile_x;
result.tileY = tile_y;
return result;

} catch (const std::exception& e) {
logging::write(std::format("error extracting height data from tile {}: {}", tile_prefix, e.what()));
return std::nullopt;
}
}

static void load_map(int mapID, const std::string& mapDir);
static void setup_wmo_minimap(WDTLoader& wdt);
static void initialize();
static void export_selected_map();
static void export_selected_map_as_raw();
static void export_selected_map_as_png();
static void export_selected_map_as_heightmaps();

static void handle_map_context(const listbox::ContextMenuEvent& data) {
core::view->contextMenus.nodeMap = nlohmann::json{
{"selection", data.selection},
{"count", static_cast<int>(data.selection.size())}
};
}

static void copy_map_names(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
try {
auto map = parse_map_entry(entry);
if (!result.empty()) result += '\n';
result += map.name;
} catch (...) {}
}
ImGui::SetClipboardText(result.c_str());
}

static void copy_map_internal_names(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
try {
auto map = parse_map_entry(entry);
if (!result.empty()) result += '\n';
result += map.dir;
} catch (...) {}
}
ImGui::SetClipboardText(result.c_str());
}

static void copy_map_ids(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
try {
auto map = parse_map_entry(entry);
if (!result.empty()) result += '\n';
result += std::to_string(map.id);
} catch (...) {}
}
ImGui::SetClipboardText(result.c_str());
}

static void copy_map_export_paths(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
try {
auto map = parse_map_entry(entry);
auto path = casc::ExportHelper::getExportPath(
(std::filesystem::path("maps") / map.dir).string());
if (!result.empty()) result += '\n';
result += path;
} catch (...) {}
}
ImGui::SetClipboardText(result.c_str());
}

static void open_map_export_directory(const std::vector<std::string>& selection) {
if (selection.empty())
return;

try {
auto map = parse_map_entry(selection[0]);
auto dir = casc::ExportHelper::getExportPath(
(std::filesystem::path("maps") / map.dir).string());

#ifdef _WIN32
ShellExecuteW(nullptr, L"open", std::filesystem::path(dir).wstring().c_str(), {}, nullptr, SW_SHOWNORMAL);
#else
std::string cmd = "xdg-open \"" + dir + "\" &";
(void)std::system(cmd.c_str());
#endif
} catch (...) {}
}

static void setup_wmo_minimap(WDTLoader& wdt) {
try {
const auto& placement = wdt.worldModelPlacement;
uint32_t file_data_id = 0;

if (!wdt.worldModel.empty()) {
auto fid = casc::listfile::getByFilename(wdt.worldModel);
if (!fid)
return;
file_data_id = *fid;
} else {
if (!placement.id)
return;
file_data_id = placement.id;
}

BufferWrapper wmo_data = core::view->casc->getVirtualFileByID(file_data_id);
WMOLoader wmo(wmo_data, file_data_id);
wmo.load();

uint32_t wmo_id = wmo.wmoID;
if (!wmo_id)
return;

auto tex_it = wmo_minimap_textures.find(wmo_id);
if (tex_it == wmo_minimap_textures.end() || tex_it->second.empty())
return;

const auto& group_info = wmo.groupInfo;
if (group_info.empty())
return;

// group tiles by groupNum
std::map<int, std::vector<db::DataRecord*>> groups_tiles;
for (auto& tile_row : tex_it->second) {
int group_num = static_cast<int>(fieldToUint32(tile_row.at("GroupNum")));
groups_tiles[group_num].push_back(&tile_row);
}

// calculate absolute pixel position for each tile
struct TilePos {
uint32_t fileDataID;
int groupNum;
int blockX;
int blockY;
float absX;
float absY;
float zOrder;
};
std::vector<TilePos> tile_positions;

for (auto& [group_num, group_tiles] : groups_tiles) {
if (group_num >= static_cast<int>(group_info.size()))
	continue;

const auto& group = group_info[group_num];
float g_min_x = (std::min)(group.boundingBox1[0], group.boundingBox2[0]) * 2.0f;
float g_min_y = (std::min)(group.boundingBox1[1], group.boundingBox2[1]) * 2.0f;
float g_min_z = (std::min)(group.boundingBox1[2], group.boundingBox2[2]);

for (auto* tile_row : group_tiles) {
	int bx = static_cast<int>(fieldToUint32(tile_row->at("BlockX")));
	int by = static_cast<int>(fieldToUint32(tile_row->at("BlockY")));
	uint32_t fdid = fieldToUint32(tile_row->at("FileDataID"));
	tile_positions.push_back({
		fdid, group_num, bx, by,
		g_min_x + (bx * 256.0f),
		g_min_y + (by * 256.0f),
		g_min_z
	});
}
}

// find bounds of all tiles
float min_x = std::numeric_limits<float>::infinity();
float max_x = -std::numeric_limits<float>::infinity();
float min_y = std::numeric_limits<float>::infinity();
float max_y = -std::numeric_limits<float>::infinity();

for (const auto& t : tile_positions) {
min_x = (std::min)(min_x, t.absX);
max_x = (std::max)(max_x, t.absX + 256.0f);
min_y = (std::min)(min_y, t.absY);
max_y = (std::max)(max_y, t.absY + 256.0f);
}

int canvas_width = static_cast<int>(std::ceil(max_x - min_x));
int canvas_height = static_cast<int>(std::ceil(max_y - min_y));

// convert to canvas coords and build positioned tiles
std::vector<WMOMinimapTileInfo> positioned_tiles;
for (const auto& t : tile_positions) {
float canvas_x = t.absX - min_x;
float canvas_y = (max_y - 256.0f) - t.absY; // flip Y for canvas

WMOMinimapTileInfo info;
info.fileDataID = t.fileDataID;
info.groupNum = t.groupNum;
info.blockX = t.blockX;
info.blockY = t.blockY;
info.pixelX = canvas_x;
info.pixelY = canvas_y;
info.scaleX = 1.0f;
info.scaleY = 1.0f;
info.srcWidth = 256;
info.srcHeight = 256;
info.zOrder = t.zOrder;
positioned_tiles.push_back(info);
}

if (positioned_tiles.empty())
return;

// use 256px output tile size, calculate grid
constexpr int OUTPUT_TILE_SIZE = 256;
int grid_width = static_cast<int>(std::ceil(static_cast<float>(canvas_width) / OUTPUT_TILE_SIZE));
int grid_height = static_cast<int>(std::ceil(static_cast<float>(canvas_height) / OUTPUT_TILE_SIZE));
int grid_size = (std::max)(grid_width, grid_height);
std::vector<int> mask(grid_size * grid_size, 0);
std::map<std::string, std::vector<WMOMinimapTileInfo>> tiles_by_coord;

// assign tiles to grid cells based on their pixel position
for (auto& tile : positioned_tiles) {
int gx_start = static_cast<int>(std::floor(tile.pixelX / OUTPUT_TILE_SIZE));
int gy_start = static_cast<int>(std::floor(tile.pixelY / OUTPUT_TILE_SIZE));
float tile_width = tile.srcWidth * tile.scaleX;
float tile_height = tile.srcHeight * tile.scaleY;
int gx_end = static_cast<int>(std::floor((tile.pixelX + tile_width - 1) / OUTPUT_TILE_SIZE));
int gy_end = static_cast<int>(std::floor((tile.pixelY + tile_height - 1) / OUTPUT_TILE_SIZE));

for (int gx = gx_start; gx <= gx_end; gx++) {
	for (int gy = gy_start; gy <= gy_end; gy++) {
		if (gx < 0 || gx >= grid_size || gy < 0 || gy >= grid_size)
			continue;

		int index = gx * grid_size + gy;
		mask[index] = 1;

		std::string key = std::format("{},{}", gx, gy);
		WMOMinimapTileInfo draw_tile = tile;
		draw_tile.drawX = tile.pixelX - (gx * OUTPUT_TILE_SIZE);
		draw_tile.drawY = tile.pixelY - (gy * OUTPUT_TILE_SIZE);
		tiles_by_coord[key].push_back(draw_tile);
	}
}
}

// sort tiles in each grid cell by Z order
for (auto& [key, tile_list] : tiles_by_coord)
std::sort(tile_list.begin(), tile_list.end(),
	[](const WMOMinimapTileInfo& a, const WMOMinimapTileInfo& b) { return a.zOrder < b.zOrder; });

WMOMinimapData data;
data.wmo_id = wmo_id;
data.tiles = std::move(positioned_tiles);
data.canvas_width = canvas_width;
data.canvas_height = canvas_height;
data.grid_width = grid_width;
data.grid_height = grid_height;
data.grid_size = grid_size;
data.mask = std::move(mask);
data.tiles_by_coord = std::move(tiles_by_coord);
data.output_tile_size = OUTPUT_TILE_SIZE;
current_wmo_minimap = std::move(data);

logging::write(std::format("loaded WMO minimap: {} tiles, {}x{} canvas, {}x{} grid",
current_wmo_minimap->tiles.size(), current_wmo_minimap->canvas_width, current_wmo_minimap->canvas_height,
current_wmo_minimap->grid_width, current_wmo_minimap->grid_height));
logging::write(std::format("WMO minimap unique grid cells: {}", current_wmo_minimap->tiles_by_coord.size()));
} catch (const std::exception& e) {
logging::write(std::format("failed to setup WMO minimap: {}", e.what()));
current_wmo_minimap = std::nullopt;
}
}

static void load_map(int mapID, const std::string& mapDir) {
std::string map_dir_lower = mapDir;
std::transform(map_dir_lower.begin(), map_dir_lower.end(), map_dir_lower.begin(), ::tolower);

core::hideToast();

selected_map_id = mapID;
selected_map_dir = map_dir_lower;
has_selected_map_dir = true;

selected_wdt.reset();
current_wmo_minimap = std::nullopt;
core::view->mapViewerHasWorldModel = false;
core::view->mapViewerIsWMOMinimap = false;
core::view->mapViewerGridSize = nullptr;
core::view->mapViewerSelection.clear();

const std::string wdt_path = std::format("world/maps/{}/{}.wdt", map_dir_lower, map_dir_lower);
logging::write(std::format("loading map preview for {} ({})", map_dir_lower, mapID));

try {
BufferWrapper wdt_data = core::view->casc->getVirtualFileByName(wdt_path);
selected_wdt_data = std::move(wdt_data);
selected_wdt = std::make_unique<WDTLoader>(selected_wdt_data);
selected_wdt->load();

if (selected_wdt->hasWorldModelPlacement)
	core::view->mapViewerHasWorldModel = true;

const bool has_terrain = !selected_wdt->tiles.empty() &&
	std::any_of(selected_wdt->tiles.begin(), selected_wdt->tiles.end(),
		[](uint32_t t) { return t == 1; });

const bool has_global_wmo = selected_wdt->hasWorldModelPlacement;

if (!has_terrain && has_global_wmo) {
	setup_wmo_minimap(*selected_wdt);

	if (current_wmo_minimap) {
		core::view->mapViewerTileLoader = "wmo_minimap";
		core::view->mapViewerChunkMask = nlohmann::json::array();
		for (int m : current_wmo_minimap->mask)
			core::view->mapViewerChunkMask.push_back(m);
		core::view->mapViewerGridSize = current_wmo_minimap->grid_size;
		core::view->mapViewerIsWMOMinimap = true;
		core::view->mapViewerSelectedMap = mapID;
		core::view->mapViewerSelectedDir = mapDir;
		core::setToast("info", "Showing WMO minimap. Use \"Export Global WMO\" to export the world model.", {}, 6000);
		return;
	}

	core::setToast("info", "This map has no terrain tiles. Use \"Export Global WMO\" to export the world model.", {}, 6000);
}

core::view->mapViewerTileLoader = "terrain";
core::view->mapViewerChunkMask = nlohmann::json::array();
for (uint32_t t : selected_wdt->tiles)
	core::view->mapViewerChunkMask.push_back(static_cast<int>(t));
core::view->mapViewerSelectedMap = mapID;
core::view->mapViewerSelectedDir = mapDir;
} catch (const std::exception& e) {
logging::write(std::format("cannot load {}, defaulting to all chunks enabled", wdt_path));
selected_wdt_data = {};
core::view->mapViewerTileLoader = "terrain";
core::view->mapViewerChunkMask = nullptr;
core::view->mapViewerSelectedMap = mapID;
core::view->mapViewerSelectedDir = mapDir;
}
}

void export_map_wmo() {
casc::ExportHelper helper(1, "WMO");
helper.start();

try {
if (!selected_wdt || !selected_wdt->hasWorldModelPlacement)
throw std::runtime_error("map does not contain a world model.");

const auto& placement = selected_wdt->worldModelPlacement;
uint32_t file_data_id = 0;
std::string file_name;

if (!selected_wdt->worldModel.empty()) {
file_name = selected_wdt->worldModel;
auto fid = casc::listfile::getByFilename(file_name);

if (!fid)
throw std::runtime_error("invalid world model path: " + file_name);

file_data_id = *fid;
} else {
if (placement.id == 0)
throw std::runtime_error("map does not define a valid world model.");

file_data_id = placement.id;
auto name = casc::listfile::getByID(file_data_id).value_or("");
file_name = !name.empty() ? name : ("unknown_" + std::to_string(file_data_id) + ".wmo");
}

const std::string mark_file_name = casc::ExportHelper::replaceExtension(file_name, ".obj");
const std::string export_path = casc::ExportHelper::getExportPath(mark_file_name);

BufferWrapper data = core::view->casc->getVirtualFileByID(file_data_id);
WMOExporter wmo(std::move(data), file_data_id, core::view->casc);
std::vector<WMOExportDoodadSetMask> doodad_mask(placement.doodadSetIndex + 1);
doodad_mask[placement.doodadSetIndex].checked = true;
wmo.setDoodadSetMask(std::move(doodad_mask));
wmo.exportAsOBJ(export_path, &helper, nullptr);

if (helper.isCancelled())
return;

helper.mark(mark_file_name, true);
} catch (const std::exception& e) {
helper.mark("world model", false, e.what(), build_stack_trace("export_map_wmo", e));
}

WMOExporter::clearCache();
helper.finish();
}

void export_map_wmo_minimap() {
casc::ExportHelper helper(1, "minimap");
helper.start();

try {
if (!current_wmo_minimap) {
if (!selected_wdt || !selected_wdt->hasWorldModelPlacement)
throw std::runtime_error("map does not contain a world model.");

setup_wmo_minimap(*selected_wdt);

if (!current_wmo_minimap)
throw std::runtime_error("no minimap textures found for this WMO.");
}

const auto& minimap_data = *current_wmo_minimap;
const auto& tiles_by_coord = minimap_data.tiles_by_coord;
int canvas_width = minimap_data.canvas_width;
int canvas_height = minimap_data.canvas_height;
int output_tile_size = minimap_data.output_tile_size;

logging::write(std::format("WMO minimap export: {} tile positions, {}x{} pixels",
tiles_by_coord.size(), canvas_width, canvas_height));

TiledPNGWriter writer(canvas_width, canvas_height, output_tile_size);

for (const auto& [coord_key, tile_list] : tiles_by_coord) {
	if (helper.isCancelled()) break;
	auto comma = coord_key.find(',');
	if (comma == std::string::npos) continue;
	int tx = std::stoi(coord_key.substr(0, comma));
	int ty = std::stoi(coord_key.substr(comma + 1));

	std::vector<uint8_t> composite(output_tile_size * output_tile_size * 4, 0);
	for (const auto& tile : tile_list) {
		try {
			BufferWrapper tile_data = core::view->casc->getVirtualFileByID(tile.fileDataID, true);
			casc::BLPImage blp(tile_data);
			auto rgba = blp.toUInt8Array(0, 0b1111);
			uint32_t tw = blp.getScaledWidth();
			uint32_t th = blp.getScaledHeight();

			const float draw_x = tile.drawX;
			const float draw_y = tile.drawY;
			const float draw_w = static_cast<float>(tw) * tile.scaleX;
			const float draw_h = static_cast<float>(th) * tile.scaleY;

			const int dst_x0 = (std::max)(0, static_cast<int>(std::floor(draw_x)));
			const int dst_y0 = (std::max)(0, static_cast<int>(std::floor(draw_y)));
			const int dst_x1 = (std::min)(output_tile_size, static_cast<int>(std::ceil(draw_x + draw_w)));
			const int dst_y1 = (std::min)(output_tile_size, static_cast<int>(std::ceil(draw_y + draw_h)));

			for (int py = dst_y0; py < dst_y1; py++) {
				for (int px = dst_x0; px < dst_x1; px++) {
					int src_x = (std::min)(static_cast<int>((px - draw_x) * tw / draw_w), static_cast<int>(tw) - 1);
					int src_y = (std::min)(static_cast<int>((py - draw_y) * th / draw_h), static_cast<int>(th) - 1);
					int src_idx = (src_y * tw + src_x) * 4;
					int dst_idx = (py * output_tile_size + px) * 4;
					uint8_t src_a = rgba[src_idx + 3];
					if (src_a == 0) continue;
					if (src_a == 255) {
						composite[dst_idx + 0] = rgba[src_idx + 0];
						composite[dst_idx + 1] = rgba[src_idx + 1];
						composite[dst_idx + 2] = rgba[src_idx + 2];
						composite[dst_idx + 3] = 255;
					} else {
						uint8_t dst_a = composite[dst_idx + 3];
						uint8_t out_a = static_cast<uint8_t>(src_a + dst_a * (255 - src_a) / 255);
						if (out_a > 0) {
							composite[dst_idx + 0] = static_cast<uint8_t>((rgba[src_idx + 0] * src_a + composite[dst_idx + 0] * dst_a * (255 - src_a) / 255) / out_a);
							composite[dst_idx + 1] = static_cast<uint8_t>((rgba[src_idx + 1] * src_a + composite[dst_idx + 1] * dst_a * (255 - src_a) / 255) / out_a);
							composite[dst_idx + 2] = static_cast<uint8_t>((rgba[src_idx + 2] * src_a + composite[dst_idx + 2] * dst_a * (255 - src_a) / 255) / out_a);
						}
						composite[dst_idx + 3] = out_a;
					}
				}
			}
		} catch (...) {}
	}

	writer.addTile(static_cast<uint32_t>(tx), static_cast<uint32_t>(ty),
		TiledPNGWriter::ImageData{ std::move(composite), static_cast<uint32_t>(output_tile_size), static_cast<uint32_t>(output_tile_size) });
}

const std::string filename = selected_map_dir + "_wmo_minimap.png";
const std::string relative_path = (std::filesystem::path("maps") / selected_map_dir / filename).string();
const std::string out_path = casc::ExportHelper::getExportPath(relative_path);

writer.write(out_path).get();

auto export_paths = core::openLastExportStream();
if (export_paths.isOpen()) {
export_paths.writeLine("png:" + out_path);
export_paths.close();
}

helper.mark(relative_path, true);
logging::write(std::format("WMO minimap exported: {}", out_path));

} catch (const std::exception& e) {
helper.mark("WMO minimap", false, e.what(), build_stack_trace("export_map_wmo_minimap", e));
}

helper.finish();
}

void export_map() {
auto& view = *core::view;
std::string format = view.config.value("exportMapFormat", "OBJ");
if (format == "OBJ")
export_selected_map();
else if (format == "PNG")
export_selected_map_as_png();
else if (format == "RAW")
export_selected_map_as_raw();
else if (format == "HEIGHTMAPS")
export_selected_map_as_heightmaps();
}

enum class MapExportFormat { OBJ, RAW };

struct PendingMapTileExport {
	std::vector<int> tile_indices;
	size_t next_index = 0;
	MapExportFormat format;
	int export_quality = 512;
	std::string dir;
	std::string mark_path;
	std::optional<FileWriter> export_paths;
	std::optional<casc::ExportHelper> helper;
	bool helper_started = false;
};

static std::optional<PendingMapTileExport> pending_map_export;

static void pump_map_export() {
	if (!pending_map_export.has_value())
		return;

	auto& task = *pending_map_export;
	auto& helper = task.helper.value();
	auto& view = *core::view;

	if (!task.helper_started) {
		helper.start();
		task.helper_started = true;
	}

	if (helper.isCancelled()) {
		if (task.export_paths) task.export_paths->close();
		ADTExporter::clearCache();
		pending_map_export.reset();
		return;
	}

	if (task.next_index >= task.tile_indices.size()) {
		if (task.export_paths) task.export_paths->close();
		ADTExporter::clearCache();
		helper.finish();
		pending_map_export.reset();
		return;
	}

	int index = task.tile_indices[task.next_index++];

	ADTExporter adt(selected_map_id.value_or(0), selected_map_dir, static_cast<uint32_t>(index));

	if (task.format == MapExportFormat::OBJ) {
		uint32_t tile_x = static_cast<uint32_t>(index) % constants::GAME::MAP_SIZE;
		uint32_t tile_y = static_cast<uint32_t>(index) / constants::GAME::MAP_SIZE;

		std::vector<ADTGameObject> game_objects_vec;

		if (view.config.value("mapsIncludeGameObjects", false)) {
			double start_x = MAP_OFFSET - (tile_x * TILE_SIZE) - TILE_SIZE;
			double start_y = MAP_OFFSET - (tile_y * TILE_SIZE) - TILE_SIZE;
			double end_x = start_x + TILE_SIZE;
			double end_y = start_y + TILE_SIZE;

			game_objects_vec = collect_game_objects(selected_map_id.value_or(0), [&](const db::DataRecord& obj) -> bool {
				auto pos_it = obj.find("Pos");
				if (pos_it == obj.end())
					return false;
				auto pos = fieldToFloatVec(pos_it->second);
				if (pos.size() < 2)
					return false;
				float posX = pos[0];
				float posY = pos[1];
				return posX > start_x && posX < end_x && posY > start_y && posY < end_y;
			});
		}

		try {
			std::vector<::ADTGameObject> export_game_objects;
			for (const auto& go : game_objects_vec) {
				::ADTGameObject ego;
				ego.FileDataID = go.FileDataID;
				ego.Position = go.Position;
				ego.Rotation = go.Rotation;
				ego.scale = go.scale;
				export_game_objects.push_back(std::move(ego));
			}
			auto out = adt.exportTile(task.dir, task.export_quality,
				export_game_objects.empty() ? nullptr : &export_game_objects,
				&helper, core::view->casc);
			if (task.export_paths) task.export_paths->writeLine(out.type + ":" + out.path.string());
			helper.mark(task.mark_path, true);
		} catch (const std::exception& e) {
			helper.mark(task.mark_path, false, e.what(), build_stack_trace("pump_map_export", e));
		}
	} else {
		try {
			auto out = adt.exportTile(task.dir, 0, nullptr, &helper, core::view->casc);
			if (task.export_paths) task.export_paths->writeLine(out.type + ":" + out.path.string());
			helper.mark(task.mark_path, true);
		} catch (const std::exception& e) {
			helper.mark(task.mark_path, false, e.what(), build_stack_trace("pump_map_export", e));
		}
	}
}

static void export_selected_map() {
	if (pending_map_export.has_value())
		return;

	auto& view = *core::view;
	const auto& export_tiles = view.mapViewerSelection;
	int export_quality = view.config.value("exportMapQuality", 512);

	if (export_tiles.empty()) {
		core::setToast("error", "You haven't selected any tiles; hold shift and click on a map tile to select it.", {}, -1);
		return;
	}

	std::vector<int> tile_indices;
	for (const auto& t : export_tiles)
		tile_indices.push_back(t.get<int>());

	PendingMapTileExport task;
	task.tile_indices = std::move(tile_indices);
	task.format = MapExportFormat::OBJ;
	task.export_quality = export_quality;
	task.dir = casc::ExportHelper::getExportPath(
		(std::filesystem::path("maps") / selected_map_dir).string());
	task.export_paths.emplace(core::openLastExportStream());
	task.mark_path = (std::filesystem::path("maps") / selected_map_dir / selected_map_dir).string();
	task.helper.emplace(static_cast<int>(task.tile_indices.size()), "tile");
	pending_map_export = std::move(task);
}

static void export_selected_map_as_raw() {
	if (pending_map_export.has_value())
		return;

	auto& view = *core::view;
	const auto& export_tiles = view.mapViewerSelection;

	if (export_tiles.empty()) {
		core::setToast("error", "You haven't selected any tiles; hold shift and click on a map tile to select it.", {}, -1);
		return;
	}

	std::vector<int> tile_indices;
	for (const auto& t : export_tiles)
		tile_indices.push_back(t.get<int>());

	PendingMapTileExport task;
	task.tile_indices = std::move(tile_indices);
	task.format = MapExportFormat::RAW;
	task.dir = casc::ExportHelper::getExportPath(
		(std::filesystem::path("maps") / selected_map_dir).string());
	task.export_paths.emplace(core::openLastExportStream());
	task.mark_path = (std::filesystem::path("maps") / selected_map_dir / selected_map_dir).string();
	task.helper.emplace(static_cast<int>(task.tile_indices.size()), "tile");
	pending_map_export = std::move(task);
}

static void export_selected_map_as_png() {
auto& view = *core::view;
const auto& export_tiles = view.mapViewerSelection;

if (export_tiles.empty()) {
core::setToast("error", "You haven't selected any tiles; hold shift and click on a map tile to select it.", {}, -1);
return;
}

std::vector<int> tile_indices;
for (const auto& t : export_tiles)
tile_indices.push_back(t.get<int>());

casc::ExportHelper helper(static_cast<int>(tile_indices.size()) + 1, "tile");
helper.start();

try {
struct TileCoord {
int index;
int x;
int y;
};

std::vector<TileCoord> tile_coords;
for (int index : tile_indices) {
TileCoord tc;
tc.index = index;
tc.x = index / constants::GAME::MAP_SIZE;
tc.y = index % constants::GAME::MAP_SIZE;
tile_coords.push_back(tc);
}

int min_x = (std::numeric_limits<int>::max)();
int max_x = (std::numeric_limits<int>::min)();
int min_y = (std::numeric_limits<int>::max)();
int max_y = (std::numeric_limits<int>::min)();

for (const auto& t : tile_coords) {
min_x = (std::min)(min_x, t.x);
max_x = (std::max)(max_x, t.x);
min_y = (std::min)(min_y, t.y);
max_y = (std::max)(max_y, t.y);
}

auto first_tile = load_map_tile(tile_coords[0].x, tile_coords[0].y, 512);
if (first_tile.empty())
throw std::runtime_error("unable to load first tile to determine tile size");

int tile_size = 512;
logging::write(std::format("detected tile size: {}x{} pixels", tile_size, tile_size));

int tiles_wide = (max_x - min_x) + 1;
int tiles_high = (max_y - min_y) + 1;
int final_width = tiles_wide * tile_size;
int final_height = tiles_high * tile_size;

logging::write(std::format("PNG canvas {}x{} pixels ({} x {} tiles)", final_width, final_height, tiles_wide, tiles_high));

TiledPNGWriter writer(final_width, final_height, tile_size);

for (const auto& tile_coord : tile_coords) {
if (helper.isCancelled())
break;

auto tile_data = load_map_tile(tile_coord.x, tile_coord.y, tile_size);

if (!tile_data.empty()) {
int rel_x = tile_coord.x - min_x;
int rel_y = tile_coord.y - min_y;

TiledPNGWriter::ImageData img_data;
img_data.data = std::move(tile_data);
img_data.width = tile_size;
img_data.height = tile_size;
writer.addTile(rel_x, rel_y, std::move(img_data));

logging::write(std::format("added tile {},{} at position {},{}", tile_coord.x, tile_coord.y, rel_x, rel_y));
helper.mark(std::format("Tile {} {}", tile_coord.x, tile_coord.y), true);
} else {
logging::write(std::format("failed to load tile {},{}, leaving gap", tile_coord.x, tile_coord.y));
helper.mark(std::format("Tile {} {}", tile_coord.x, tile_coord.y), false, "Tile not available");
}
}

auto sorted_tiles = tile_indices;
std::sort(sorted_tiles.begin(), sorted_tiles.end());

std::string tiles_str;
for (size_t i = 0; i < sorted_tiles.size(); i++) {
if (i > 0) tiles_str += ',';
tiles_str += std::to_string(sorted_tiles[i]);
}

std::string tile_hash = md5_hex(tiles_str).substr(0, 8);

const std::string filename = selected_map_dir + "_" + tile_hash + ".png";
const std::string out_path = casc::ExportHelper::getExportPath(
(std::filesystem::path("maps") / selected_map_dir / filename).string());

writer.write(out_path).get();

auto stats = writer.getStats();
logging::write(std::format("map export complete: {} ({} tiles)", out_path, stats.totalTiles));

auto export_paths = core::openLastExportStream();
if (export_paths.isOpen()) {
export_paths.writeLine("png:" + out_path);
export_paths.close();
}

helper.mark((std::filesystem::path("maps") / selected_map_dir / filename).string(), true);

} catch (const std::exception& e) {
helper.mark("PNG export", false, e.what(), build_stack_trace("export_selected_map_as_png", e));
logging::write(std::format("PNG export failed: {}", e.what()));
}

helper.finish();
}

static void export_selected_map_as_heightmaps() {
auto& view = *core::view;
const auto& export_tiles = view.mapViewerSelection;
int export_resolution = view.config.value("heightmapResolution", 256);

if (export_tiles.empty()) {
core::setToast("error", "You haven't selected any tiles; hold shift and click on a map tile to select it.", {}, -1);
return;
}

if (export_resolution == -1)
export_resolution = view.config.value("heightmapCustomResolution", 256);

if (export_resolution <= 0) {
core::setToast("error", "Invalid heightmap resolution selected.", {}, -1);
return;
}

std::vector<int> tile_indices;
for (const auto& t : export_tiles)
tile_indices.push_back(t.get<int>());

const std::string dir = casc::ExportHelper::getExportPath(
(std::filesystem::path("maps") / selected_map_dir / "heightmaps").string());
auto export_paths = core::openLastExportStream();

core::setToast("progress", "Calculating height range across all tiles...", {}, -1, false);
float global_min_height = (std::numeric_limits<float>::infinity)();
float global_max_height = -(std::numeric_limits<float>::infinity)();

for (size_t i = 0; i < tile_indices.size(); i++) {
int tile_index = tile_indices[i];
uint32_t tile_x = static_cast<uint32_t>(tile_index) / constants::GAME::MAP_SIZE;
uint32_t tile_y = static_cast<uint32_t>(tile_index) % constants::GAME::MAP_SIZE;

try {
auto height_data = extract_height_data_from_tile(selected_map_dir, tile_x, tile_y, export_resolution);

if (height_data && !height_data->heights.empty()) {
float tile_min = (std::numeric_limits<float>::infinity)();
float tile_max = -(std::numeric_limits<float>::infinity)();

for (size_t j = 0; j < height_data->heights.size(); j++) {
float height = height_data->heights[j];
if (height < tile_min)
tile_min = height;
if (height > tile_max)
tile_max = height;
}

global_min_height = (std::min)(global_min_height, tile_min);
global_max_height = (std::max)(global_max_height, tile_max);

logging::write(std::format("tile {}: height range [{}, {}]", tile_index, tile_min, tile_max));
}
} catch (const std::exception& e) {
logging::write(std::format("failed to extract height data from tile {}: {}", tile_index, e.what()));
}
}

if (global_min_height == (std::numeric_limits<float>::infinity)() ||
global_max_height == -(std::numeric_limits<float>::infinity)()) {
core::hideToast();
core::setToast("error", "No valid height data found in selected tiles", {}, -1);
return;
}

float height_range = global_max_height - global_min_height;
logging::write(std::format("global height range: [{}, {}] (range: {})", global_min_height, global_max_height, height_range));

core::hideToast();

casc::ExportHelper helper(static_cast<int>(tile_indices.size()), "heightmap");
helper.start();

for (size_t i = 0; i < tile_indices.size(); i++) {
int tile_index = tile_indices[i];

if (helper.isCancelled())
break;

uint32_t tile_x = static_cast<uint32_t>(tile_index) / constants::GAME::MAP_SIZE;
uint32_t tile_y = static_cast<uint32_t>(tile_index) % constants::GAME::MAP_SIZE;

const std::string tile_id = std::to_string(tile_x) + "_" + std::to_string(tile_y);
const std::string filename = "heightmap_" + tile_id + ".png";

try {
auto height_data = extract_height_data_from_tile(selected_map_dir, tile_x, tile_y, export_resolution);

if (!height_data || height_data->heights.empty()) {
helper.mark("heightmap_" + tile_id + ".png", false, "no height data available");
continue;
}

const std::string out_path = (std::filesystem::path(dir) / filename).string();

PNGWriter writer(export_resolution, export_resolution);
int bit_depth = view.config.value("heightmapBitDepth", 8);

if (bit_depth == 32) {
writer.bytesPerPixel = 4;
writer.bitDepth = 8;
writer.colorType = 6;

auto& pixel_data = writer.getPixelData();
pixel_data.resize(export_resolution * export_resolution * 4);

for (size_t j = 0; j < height_data->heights.size(); j++) {
float normalized_height = (height_data->heights[j] - global_min_height) / height_range;

uint8_t byte_view[4];
std::memcpy(byte_view, &normalized_height, 4);

size_t pixel_offset = j * 4;
pixel_data[pixel_offset + 0] = byte_view[0];
pixel_data[pixel_offset + 1] = byte_view[1];
pixel_data[pixel_offset + 2] = byte_view[2];
pixel_data[pixel_offset + 3] = byte_view[3];
}
} else if (bit_depth == 16) {
writer.bytesPerPixel = 2;
writer.bitDepth = 16;
writer.colorType = 0;

auto& pixel_data = writer.getPixelData();
pixel_data.resize(export_resolution * export_resolution * 2);

for (size_t j = 0; j < height_data->heights.size(); j++) {
float normalized_height = (height_data->heights[j] - global_min_height) / height_range;
uint16_t gray_value = static_cast<uint16_t>(std::floor(normalized_height * 65535));
size_t pixel_offset = j * 2;

pixel_data[pixel_offset + 0] = (gray_value >> 8) & 0xFF;
pixel_data[pixel_offset + 1] = gray_value & 0xFF;
}
} else {
writer.bytesPerPixel = 1;
writer.bitDepth = 8;
writer.colorType = 0;

auto& pixel_data = writer.getPixelData();
pixel_data.resize(export_resolution * export_resolution);

for (size_t j = 0; j < height_data->heights.size(); j++) {
float normalized_height = (height_data->heights[j] - global_min_height) / height_range;
pixel_data[j] = static_cast<uint8_t>(std::floor(normalized_height * 255));
}
}

writer.write(out_path).get();

if (export_paths.isOpen())
export_paths.writeLine("png:" + out_path);

helper.mark((std::filesystem::path("maps") / selected_map_dir / "heightmaps" / filename).string(), true);
logging::write(std::format("exported heightmap: {}", out_path));

} catch (const std::exception& e) {
helper.mark(filename, false, e.what(), build_stack_trace("export_selected_map_as_heightmaps", e));
logging::write(std::format("failed to export heightmap for tile {}: {}", tile_index, e.what()));
}
}

if (export_paths.isOpen())
export_paths.close();
helper.finish();
}

void registerTab() {
modules::register_nav_button("tab_maps", "Maps", "map.svg", install_type::CASC);
}

static void initialize() {
core::showLoadingScreen(3);

core::progressLoadingScreen("Loading WMO minimap textures...");

wmo_minimap_textures_loaded = true;
wmo_minimap_textures.clear();

auto& wmo_mm_table = casc::db2::getTable("WMOMinimapTexture");
auto all_wmo_rows = wmo_mm_table.getAllRows();

for (auto& [id, row] : all_wmo_rows) {
auto wmoid_it = row.find("WMOID");
if (wmoid_it == row.end())
continue;
uint32_t wmo_id = fieldToUint32(wmoid_it->second);

wmo_minimap_textures[wmo_id].push_back(row);
}

logging::write(std::format("loaded {} WMO minimap entries", wmo_minimap_textures.size()));

core::progressLoadingScreen("Loading maps...");

auto& map_table = casc::db2::getTable("Map");
auto map_rows = map_table.getAllRows();

std::vector<std::string> maps;
for (auto& [id, entry] : map_rows) {
auto dir_it = entry.find("Directory");
auto name_it = entry.find("MapName_lang");
auto wdt_fid_it = entry.find("WdtFileDataID");
auto exp_it = entry.find("ExpansionID");

if (dir_it == entry.end() || name_it == entry.end())
continue;

std::string directory = fieldToString(dir_it->second);
std::string map_name = fieldToString(name_it->second);
uint32_t wdt_fid = (wdt_fid_it != entry.end()) ? fieldToUint32(wdt_fid_it->second) : 0;
uint32_t expansion_id = (exp_it != entry.end()) ? fieldToUint32(exp_it->second) : 0;

std::string wdt_path = std::format("world/maps/{}/{}.wdt", directory, directory);

if (wdt_fid) {
if (!casc::listfile::existsByID(wdt_fid))
casc::listfile::addEntry(wdt_fid, wdt_path);

maps.push_back(std::format("{}\x19[{}]\x19{}\x19({})", expansion_id, id, map_name, directory));
} else if (casc::listfile::getByFilename(wdt_path)) {
maps.push_back(std::format("{}\x19[{}]\x19{}\x19({})", expansion_id, id, map_name, directory));
}
}

core::postToMainThread([maps = std::move(maps)]() mutable {
	core::view->mapViewerMaps.clear();
	for (const auto& m : maps)
		core::view->mapViewerMaps.push_back(m);
	tab_initialized = true;
	tab_initializing = false;
	core::hideLoadingScreen();
});
}

void mounted() {
core::view->mapViewerTileLoader = "terrain";

tab_initialized = false;
prev_selection_str.clear();

if (!tab_initializing) {
tab_initializing = true;
std::thread(initialize).detach();
}
}

void render() {
auto& view = *core::view;

pump_map_export();

if (!tab_initialized)
return;

std::string current_selection_str;
for (const auto& s : view.selectionMaps) {
current_selection_str += s.get<std::string>();
current_selection_str += '\0';
}

if (current_selection_str != prev_selection_str) {
prev_selection_str = current_selection_str;
const std::string first = view.selectionMaps.empty()
    ? std::string{}
    : view.selectionMaps[0].get<std::string>();
if (!view.isBusy && !first.empty()) {
try {
auto map = parse_map_entry(first);
if (!selected_map_id || *selected_map_id != map.id)
load_map(map.id, map.dir);
} catch (...) {}
}
}

if (app::layout::BeginTab("tab-maps")) {

const ImVec2 tabAvail = ImGui::GetContentRegionAvail();
const ImVec2 tabOrigin = ImGui::GetCursorPos();

{
	ImGui::SetCursorPos(tabOrigin);
	app::theme::renderExpansionFilterButtons(
		view.selectedExpansionFilter,
		static_cast<int>(constants::EXPANSIONS.size()));
}
float expansionRowH = ImGui::GetCursorPosY() - tabOrigin.y;

constexpr float FILTER_BAR_H = app::layout::FILTER_BAR_HEIGHT;
const float contentH = tabAvail.y - expansionRowH;

constexpr float SIDEBAR_W = app::layout::SIDEBAR_WIDTH;
const float gridW = tabAvail.x - SIDEBAR_W;
const float leftColW = gridW * 0.5f;
const float rightColW = gridW - leftColW;
const float topRowH = contentH - FILTER_BAR_H;

const float rowYStart = tabOrigin.y + expansionRowH;

{
	ImGui::SetCursorPos(ImVec2(tabOrigin.x + app::layout::LIST_MARGIN_LEFT,
	                           rowYStart + app::layout::LIST_MARGIN_TOP));
	ImGui::BeginChild("maps-list-container",
		ImVec2(leftColW - app::layout::LIST_MARGIN_LEFT - app::layout::LIST_MARGIN_RIGHT,
		       topRowH - app::layout::LIST_MARGIN_TOP));

	const auto& map_items = core::cached_json_strings(view.mapViewerMaps, s_items_cache, s_items_cache_size);

	std::vector<std::string> selection_strs;
	for (const auto& s : view.selectionMaps)
		selection_strs.push_back(s.get<std::string>());

	listbox::CopyMode copy_mode = listbox::CopyMode::Default;
	std::string copy_mode_str = view.config.value("copyMode", "Default");
	if (copy_mode_str == "DIR") copy_mode = listbox::CopyMode::DIR;
	else if (copy_mode_str == "FID") copy_mode = listbox::CopyMode::FID;

	listbox_maps::render(
		"listbox-maps",
		map_items,
		view.userInputFilterMaps,
		selection_strs,
		true,
		true,
		view.config.value("regexFilters", false),
		copy_mode,
		view.config.value("pasteSelection", false),
		view.config.value("removePathSpacesCopy", false),
		"map",
		nullptr,
		false,
		"maps",
		{},
		false,
		view.selectedExpansionFilter,
		listbox_state,
		[&](const std::vector<std::string>& new_sel) {
			view.selectionMaps.clear();
			for (const auto& s : new_sel)
				view.selectionMaps.push_back(s);
		},
		[](const listbox::ContextMenuEvent& ev) {
			handle_map_context(ev);
		}
	);

	ImGui::EndChild();
}

{
	ImGui::SetCursorPos(ImVec2(tabOrigin.x + leftColW + app::layout::PREVIEW_MARGIN_LEFT,
	                           tabOrigin.y + app::layout::PREVIEW_MARGIN_TOP));
	ImGui::BeginChild("maps-preview-container",
		ImVec2(rightColW - app::layout::PREVIEW_MARGIN_LEFT - app::layout::PREVIEW_MARGIN_RIGHT,
		       tabAvail.y - FILTER_BAR_H - app::layout::PREVIEW_MARGIN_TOP));

	auto tile_loader = [](int x, int y, int size) -> std::vector<uint8_t> {
		std::string loader_type = "terrain";
		if (core::view->mapViewerTileLoader.is_string())
			loader_type = core::view->mapViewerTileLoader.get<std::string>();

		if (loader_type == "wmo_minimap")
			return load_wmo_minimap_tile(x, y, size);
		else
			return load_map_tile(x, y, size);
	};

	std::vector<int> mask;
	if (view.mapViewerChunkMask.is_array()) {
		for (const auto& v : view.mapViewerChunkMask)
			mask.push_back(v.get<int>());
	}

	std::vector<int> selection;
	for (const auto& v : view.mapViewerSelection)
		selection.push_back(v.get<int>());

	int map_id = view.mapViewerSelectedMap.is_number() ? view.mapViewerSelectedMap.get<int>() : -1;
	int grid_size = view.mapViewerGridSize.is_number() ? view.mapViewerGridSize.get<int>() : 0;
	bool selectable = !view.mapViewerIsWMOMinimap;

	map_viewer::renderWidget(
		"map-viewer",
		map_viewer_state,
		tile_loader,
		512,
		map_id,
		12,
		mask,
		selection,
		selectable,
		grid_size,
		[&](const std::vector<int>& new_sel) {
			view.mapViewerSelection.clear();
			for (int idx : new_sel)
				view.mapViewerSelection.push_back(idx);
		}
	);

	ImGui::EndChild();
}

{
	ImGui::SetCursorPos(ImVec2(tabOrigin.x, rowYStart + topRowH));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 0.0f));
	ImGui::BeginChild("maps-filter", ImVec2(leftColW, FILTER_BAR_H), ImGuiChildFlags_None,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	{
		float padY = (FILTER_BAR_H - ImGui::GetFrameHeight()) * 0.5f;
		if (padY > 0.0f) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);

		bool regex_enabled = view.config.value("regexFilters", false);
		if (regex_enabled) {
			ImGui::TextDisabled("Regex Enabled");
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", view.regexTooltip.c_str());
			ImGui::SameLine();
		}

		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterMaps.c_str(), sizeof(filter_buf) - 1);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::InputTextWithHint("##filter-maps", "Filter maps...", filter_buf, sizeof(filter_buf)))
			view.userInputFilterMaps = filter_buf;
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

{
	const auto& node = view.contextMenus.nodeMap;
	context_menu::render(
		"ctx-maps",
		node,
		context_menu_state,
		[&]() { view.contextMenus.nodeMap = nullptr; },
		[&](const nlohmann::json& ctx_node) {
			std::vector<std::string> ctx_selection;
			if (ctx_node.contains("selection") && ctx_node["selection"].is_array()) {
				for (const auto& s : ctx_node["selection"])
					ctx_selection.push_back(s.get<std::string>());
			}
			int count = ctx_node.value("count", 0);
			std::string plural = count > 1 ? "s" : "";

			if (ImGui::Selectable(std::format("Copy map name{}", plural).c_str()))
				copy_map_names(ctx_selection);
			if (ImGui::Selectable(std::format("Copy internal name{}", plural).c_str()))
				copy_map_internal_names(ctx_selection);
			if (ImGui::Selectable(std::format("Copy map ID{}", plural).c_str()))
				copy_map_ids(ctx_selection);
			if (ImGui::Selectable(std::format("Copy export path{}", plural).c_str()))
				copy_map_export_paths(ctx_selection);
			if (ImGui::Selectable("Open export directory"))
				open_map_export_directory(ctx_selection);
		}
	);
}

{
	ImGui::SetCursorPos(ImVec2(tabOrigin.x + leftColW, rowYStart + topRowH));
	ImGui::BeginChild("maps-preview-controls", ImVec2(rightColW, FILTER_BAR_H), ImGuiChildFlags_None,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	{
		float padY = (FILTER_BAR_H - ImGui::GetFrameHeight()) * 0.5f;
		if (padY > 0.0f) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);

		if (view.mapViewerHasWorldModel) {
			bool disabled = view.isBusy;
			if (disabled) app::theme::BeginDisabledButton();
			if (ImGui::Button("Export Global WMO"))
				export_map_wmo();
			ImGui::SameLine();
			if (ImGui::Button("Export WMO Minimap"))
				export_map_wmo_minimap();
			if (disabled) app::theme::EndDisabledButton();
			ImGui::SameLine();
		}

		if (!view.mapViewerIsWMOMinimap) {
			std::vector<menu_button::MenuOption> export_options;
			for (const auto& opt : view.menuButtonMapExport)
				export_options.push_back({opt.label, opt.value});

			std::string default_format = view.config.value("exportMapFormat", "OBJ");
			bool export_disabled = view.isBusy || view.mapViewerSelection.empty();

			if (export_disabled) app::theme::BeginDisabledButton();
			menu_button::render(
				"map-export-btn",
				export_options,
				default_format,
				false,
				false,
				menu_button_export_state,
				[&](const std::string& val) {
					view.config["exportMapFormat"] = val;
				},
				[]() {
					export_map();
				}
			);
			if (export_disabled) app::theme::EndDisabledButton();
		}
	}
	ImGui::EndChild();
}

{
	ImGui::SetCursorPos(ImVec2(tabOrigin.x + gridW,
	                           tabOrigin.y + app::layout::SIDEBAR_MARGIN_TOP));
	ImGui::BeginChild("maps-sidebar",
		ImVec2(SIDEBAR_W - app::layout::SIDEBAR_PADDING_RIGHT,
		       tabAvail.y - app::layout::SIDEBAR_MARGIN_TOP));

	ImGui::SeparatorText("Export Options");

	bool maps_include_wmo = view.config.value("mapsIncludeWMO", true);
	if (ImGui::Checkbox("Export WMO##maps_wmo", &maps_include_wmo))
		view.config["mapsIncludeWMO"] = maps_include_wmo;
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Include WMO objects (large objects such as buildings)");

	if (maps_include_wmo) {
		bool maps_include_wmo_sets = view.config.value("mapsIncludeWMOSets", true);
		if (ImGui::Checkbox("Export WMO Sets##maps_wmo_sets", &maps_include_wmo_sets))
			view.config["mapsIncludeWMOSets"] = maps_include_wmo_sets;
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Include objects inside WMOs (interior decorations)");
	}

	bool maps_include_m2 = view.config.value("mapsIncludeM2", true);
	if (ImGui::Checkbox("Export M2##maps_m2", &maps_include_m2))
		view.config["mapsIncludeM2"] = maps_include_m2;
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Export M2 objects on this tile (smaller objects such as trees)");

	bool maps_include_foliage = view.config.value("mapsIncludeFoliage", false);
	if (ImGui::Checkbox("Export Foliage##maps_foliage", &maps_include_foliage))
		view.config["mapsIncludeFoliage"] = maps_include_foliage;
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Export foliage used on this tile (grass, etc)");

	bool maps_export_raw = view.config.value("mapsExportRaw", false);

	if (!maps_export_raw) {
		bool maps_include_liquid = view.config.value("mapsIncludeLiquid", true);
		if (ImGui::Checkbox("Export Liquids##maps_liquid", &maps_include_liquid))
			view.config["mapsIncludeLiquid"] = maps_include_liquid;
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Export raw liquid data (water, lava, etc)");
	}

	bool maps_include_game_objects = view.config.value("mapsIncludeGameObjects", false);
	if (ImGui::Checkbox("Export G-Objects##maps_gobjects", &maps_include_game_objects))
		view.config["mapsIncludeGameObjects"] = maps_include_game_objects;
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Export client-side interactable objects (signs, banners, etc)");

	if (!maps_export_raw) {
		bool maps_include_holes = view.config.value("mapsIncludeHoles", true);
		if (ImGui::Checkbox("Include Holes##maps_holes", &maps_include_holes))
			view.config["mapsIncludeHoles"] = maps_include_holes;
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Include terrain holes for WMOs");
	}

	ImGui::SeparatorText("Model Textures");

	bool models_export_textures = view.config.value("modelsExportTextures", true);
	if (ImGui::Checkbox("Textures##maps_textures", &models_export_textures))
		view.config["modelsExportTextures"] = models_export_textures;
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Include textures when exporting models");

	if (models_export_textures) {
		bool models_export_alpha = view.config.value("modelsExportAlpha", true);
		if (ImGui::Checkbox("Texture Alpha##maps_alpha", &models_export_alpha))
			view.config["modelsExportAlpha"] = models_export_alpha;
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Include alpha channel in exported model textures");
	}

	if (!maps_export_raw) {
		ImGui::SeparatorText("Terrain Texture Quality");

		std::vector<menu_button::MenuOption> quality_options;
		for (const auto& opt : view.menuButtonTextureQuality)
			quality_options.push_back({opt.label, std::to_string(opt.value)});

		std::string default_quality = std::to_string(view.config.value("exportMapQuality", 512));

		menu_button::render(
			"map-quality-btn",
			quality_options,
			default_quality,
			false,
			true,
			menu_button_quality_state,
			[&](const std::string& val) {
				view.config["exportMapQuality"] = std::stoi(val);
			},
			nullptr
		);

		ImGui::SeparatorText("Heightmaps");

		std::vector<menu_button::MenuOption> hm_res_options;
		for (const auto& opt : view.menuButtonHeightmapResolution)
			hm_res_options.push_back({opt.label, std::to_string(opt.value)});

		std::string default_hm_res = std::to_string(view.config.value("heightmapResolution", 256));

		menu_button::render(
			"hm-resolution-btn",
			hm_res_options,
			default_hm_res,
			false,
			true,
			menu_button_heightmap_res_state,
			[&](const std::string& val) {
				view.config["heightmapResolution"] = std::stoi(val);
			},
			nullptr
		);

		std::vector<menu_button::MenuOption> hm_depth_options;
		for (const auto& opt : view.menuButtonHeightmapBitDepth)
			hm_depth_options.push_back({opt.label, std::to_string(opt.value)});

		std::string default_hm_depth = std::to_string(view.config.value("heightmapBitDepth", 8));

		ImGui::Spacing();
		menu_button::render(
			"hm-bitdepth-btn",
			hm_depth_options,
			default_hm_depth,
			false,
			true,
			menu_button_heightmap_depth_state,
			[&](const std::string& val) {
				view.config["heightmapBitDepth"] = std::stoi(val);
			},
			nullptr
		);

		if (view.config.value("heightmapResolution", 256) == -1) {
			ImGui::Spacing();
			ImGui::Text("Heightmap Resolution");
			int custom_res = view.config.value("heightmapCustomResolution", 256);
			if (ImGui::InputInt("##hm_custom_res", &custom_res, 1, 10)) {
				if (custom_res < 1) custom_res = 1;
				view.config["heightmapCustomResolution"] = custom_res;
			}
		}
	}

	ImGui::EndChild();
}

}
app::layout::EndTab();
}

}
