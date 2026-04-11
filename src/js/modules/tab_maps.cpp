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
#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

namespace tab_maps {

// ── MD5 helper (matches JS crypto.createHash('md5')) ────────────────────
// Same RFC 1321 implementation used in cache-collector.cpp.
namespace md5_detail {

struct MD5Context {
	uint32_t state[4];
	uint64_t count;
	uint8_t buffer[64];
};

static constexpr uint32_t S[64] = {
	7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
	5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
	4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
	6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

static constexpr uint32_t K[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static uint32_t left_rotate(uint32_t x, uint32_t c) {
	return (x << c) | (x >> (32 - c));
}

static void md5_transform(uint32_t state[4], const uint8_t block[64]) {
	uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
	uint32_t M[16];
	for (int i = 0; i < 16; i++)
		M[i] = static_cast<uint32_t>(block[i * 4]) |
		       (static_cast<uint32_t>(block[i * 4 + 1]) << 8) |
		       (static_cast<uint32_t>(block[i * 4 + 2]) << 16) |
		       (static_cast<uint32_t>(block[i * 4 + 3]) << 24);
	for (uint32_t i = 0; i < 64; i++) {
		uint32_t f, g;
		if (i < 16) { f = (b & c) | (~b & d); g = i; }
		else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) % 16; }
		else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) % 16; }
		else { f = c ^ (b | ~d); g = (7 * i) % 16; }
		uint32_t temp = d; d = c; c = b;
		b = b + left_rotate(a + f + K[i] + M[g], S[i]); a = temp;
	}
	state[0] += a; state[1] += b; state[2] += c; state[3] += d;
}

static void md5_init(MD5Context& ctx) {
	ctx.state[0] = 0x67452301; ctx.state[1] = 0xefcdab89;
	ctx.state[2] = 0x98badcfe; ctx.state[3] = 0x10325476;
	ctx.count = 0; std::memset(ctx.buffer, 0, sizeof(ctx.buffer));
}

static void md5_update(MD5Context& ctx, const uint8_t* data, size_t len) {
	size_t index = static_cast<size_t>(ctx.count % 64);
	ctx.count += len;
	size_t i = 0;
	if (index) {
		size_t part_len = 64 - index;
		if (len >= part_len) {
			std::memcpy(ctx.buffer + index, data, part_len);
			md5_transform(ctx.state, ctx.buffer); i = part_len;
		} else { std::memcpy(ctx.buffer + index, data, len); return; }
	}
	for (; i + 63 < len; i += 64) md5_transform(ctx.state, data + i);
	if (i < len) std::memcpy(ctx.buffer, data + i, len - i);
}

static std::string md5_final_hex(MD5Context& ctx) {
	uint8_t padding[64] = { 0x80 };
	uint64_t bits = ctx.count * 8;
	size_t index = static_cast<size_t>(ctx.count % 64);
	size_t pad_len = (index < 56) ? (56 - index) : (120 - index);
	md5_update(ctx, padding, pad_len);
	uint8_t bits_buf[8];
	for (int i = 0; i < 8; i++) bits_buf[i] = static_cast<uint8_t>(bits >> (i * 8));
	md5_update(ctx, bits_buf, 8);
	static constexpr char hex_chars[] = "0123456789abcdef";
	std::string result;
	result.reserve(32);
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			uint8_t byte = static_cast<uint8_t>(ctx.state[i] >> (j * 8));
			result.push_back(hex_chars[(byte >> 4) & 0x0F]);
			result.push_back(hex_chars[byte & 0x0F]);
		}
	}
	return result;
}

} // namespace md5_detail

/// Compute MD5 hex digest of a string. Matches JS crypto.createHash('md5').update(str).digest('hex').
static std::string md5_hex(const std::string& input) {
	md5_detail::MD5Context ctx;
	md5_detail::md5_init(ctx);
	md5_detail::md5_update(ctx, reinterpret_cast<const uint8_t*>(input.data()), input.size());
	return md5_detail::md5_final_hex(ctx);
}

// --- Constants ---

// JS: const TILE_SIZE = constants.GAME.TILE_SIZE;
static constexpr double TILE_SIZE = constants::GAME::TILE_SIZE;

// JS: const MAP_OFFSET = constants.GAME.MAP_OFFSET;
static constexpr int MAP_OFFSET = constants::GAME::MAP_OFFSET;

// --- File-local state ---

// JS: let selected_map_id = null;
static std::optional<int> selected_map_id;

// JS: let selected_map_dir = null;
static std::string selected_map_dir;
static bool has_selected_map_dir = false;

// JS: let selected_wdt = null;
static std::unique_ptr<WDTLoader> selected_wdt;
static BufferWrapper selected_wdt_data; // Keeps the buffer alive for WDTLoader

// JS: let game_objects_db2 = null;
static bool game_objects_db2_loaded = false;
static std::unordered_map<uint32_t, std::vector<db::DataRecord>> game_objects_db2;

// JS: let wmo_minimap_textures = null;
static bool wmo_minimap_textures_loaded = false;
static std::unordered_map<uint32_t, std::vector<db::DataRecord>> wmo_minimap_textures;

// JS: let current_wmo_minimap = null;
static std::optional<WMOMinimapData> current_wmo_minimap;

// Change-detection for watches
static std::string prev_selection_first;
static bool tab_initialized = false;

// Component states
static listbox_maps::ListboxMapsState listbox_state;
static context_menu::ContextMenuState context_menu_state;
static map_viewer::MapViewerState map_viewer_state;
static menu_button::MenuButtonState menu_button_export_state;
static menu_button::MenuButtonState menu_button_quality_state;
static menu_button::MenuButtonState menu_button_heightmap_res_state;
static menu_button::MenuButtonState menu_button_heightmap_depth_state;

// --- Field value helpers (same as other tab modules) ---

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

// --- Internal functions ---

/**
 * Parse a map entry from the listbox.
 * JS: const parse_map_entry = (entry) => { ... }
 * @param entry
 */
static MapEntry parse_map_entry(const std::string& entry) {
// Format: expansion_id \x19 [map_id] \x19 MapName_lang \x19 (Directory)
// JS regex: /\[(\d+)\]\31([^\31]+)\31\(([^)]+)\)/
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
 * JS: const load_map_tile = async (x, y, size) => { ... }
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

// JS: const data = await core.view.casc.getFileByName(tile_path, false, true);
BufferWrapper data = core::view->casc->getVirtualFileByName(tile_path, true);
casc::BLPImage blp(data);
auto rgba = blp.toUInt8Array(0, 0b0111);

uint32_t blp_width = blp.width;
uint32_t blp_height = blp.height;

if (static_cast<int>(blp_width) == size && static_cast<int>(blp_height) == size)
return rgba;

// Scale the image to the requested size
std::vector<uint8_t> scaled(size * size * 4, 0);
float scale_x = static_cast<float>(blp_width) / static_cast<float>(size);
float scale_y = static_cast<float>(blp_height) / static_cast<float>(size);

for (int py = 0; py < size; py++) {
for (int px = 0; px < size; px++) {
int src_x = (std::min)(static_cast<int>(px * scale_x), static_cast<int>(blp_width) - 1);
int src_y = (std::min)(static_cast<int>(py * scale_y), static_cast<int>(blp_height) - 1);
int src_idx = (src_y * blp_width + src_x) * 4;
int dst_idx = (py * size + px) * 4;
scaled[dst_idx + 0] = rgba[src_idx + 0];
scaled[dst_idx + 1] = rgba[src_idx + 1];
scaled[dst_idx + 2] = rgba[src_idx + 2];
scaled[dst_idx + 3] = rgba[src_idx + 3];
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
 * JS: const load_wmo_minimap_tile = async (x, y, size) => { ... }
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

// JS: for (const tile of tile_list) { const data = await core.view.casc.getFile(tile.fileDataID); ... }
const auto& tile_list = it->second;
for (const auto& tile : tile_list) {
BufferWrapper data = core::view->casc->getVirtualFileByID(tile.fileDataID, true);
casc::BLPImage blp(data);
auto rgba = blp.toUInt8Array(0, 0b1111);

uint32_t blp_w = blp.width;
uint32_t blp_h = blp.height;

// Alpha-composite tile onto the output buffer
for (int py = 0; py < size; py++) {
for (int px = 0; px < size; px++) {
int src_x = (std::min)(static_cast<int>(px * output_scale), static_cast<int>(blp_w) - 1);
int src_y = (std::min)(static_cast<int>(py * output_scale), static_cast<int>(blp_h) - 1);
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
 * JS: const collect_game_objects = async (mapID, filter) => { ... }
 * @param mapID
 * @param filter
 */
static std::vector<ADTGameObject> collect_game_objects(uint32_t mapID,
const std::function<bool(const db::DataRecord&)>& filter)
{
if (!game_objects_db2_loaded) {
game_objects_db2_loaded = true;
game_objects_db2.clear();

// JS: for (const row of (await db2.GameObjects.getAllRows()).values()) { ... }
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
// JS: row.FileDataID = fid_row.FileDataID;
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
 * JS: const sample_chunk_height = (chunk, localX, localY) => { ... }
 * @param chunk
 * @param localX
 * @param localY
 * @returns interpolated height
 */
// JS: const get_vert_idx = (x, y) => { ... }
// Extracted as a static helper to avoid MSVC lambda capture issues.
static int get_vert_idx(int x, int y) {
int index = 0;
for (int row = 0; row < y * 2; row++)
index += (row % 2) ? 8 : 9;

bool is_short = !!((y * 2) % 2);
index += is_short ? (std::min)(x, 7) : (std::min)(x, 8);
return index;
}

static float sample_chunk_height(const ADTChunk& chunk, float localX, float localY) {
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

// JS: const h00 = chunk.vertices[get_vert_idx(x0, y0)] + chunk.position[2];
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
 * JS: const extract_height_data_from_tile = async (adt, resolution) => { ... }
 * @param map_dir
 * @param tile_x
 * @param tile_y
 * @param resolution
 * @returns optional height data
 */
static std::optional<HeightData> extract_height_data_from_tile(
const std::string& map_dir, uint32_t tile_x, uint32_t tile_y, int resolution)
{
// JS: const prefix = util.format('world/maps/%s/%s', map_dir, map_dir);
const std::string prefix = std::format("world/maps/{}/{}", map_dir, map_dir);
// JS: const tile_prefix = prefix + '_' + adt.tileY + '_' + adt.tileX;
const std::string tile_prefix = prefix + '_' + std::to_string(tile_y) + '_' + std::to_string(tile_x);

try {
// JS: const root_fid = listfile.getByFilename(tile_prefix + '.adt');
auto root_fid = casc::listfile::getByFilename(tile_prefix + ".adt");
if (!root_fid) {
logging::write(std::format("cannot find fileDataID for {}.adt", tile_prefix));
return std::nullopt;
}

// JS: const root_file = await core.view.casc.getFile(root_fid);
BufferWrapper root_file = core::view->casc->getVirtualFileByID(*root_fid);
// JS: const root_adt = new ADTLoader(root_file);
// JS: root_adt.loadRoot();
// TODO(conversion): ADTLoader processing will be wired when ADTLoader is fully converted.
// The height values are stored in a resolution x resolution grid with
// a 90-degree CW rotation: heights[height_idx] where
// height_idx = (resolution - 1 - px) * resolution + py.
(void)root_file;
return std::nullopt;

} catch (const std::exception& e) {
logging::write(std::format("error extracting height data from tile {}: {}", tile_prefix, e.what()));
return std::nullopt;
}
}

// Forward declarations for methods
static void load_map(int mapID, const std::string& mapDir);
static void setup_wmo_minimap(WDTLoader& wdt);
static void initialize();
static void export_selected_map();
static void export_selected_map_as_raw();
static void export_selected_map_as_png();
static void export_selected_map_as_heightmaps();

// --- Context menu methods ---

/**
 * JS: handle_map_context(data)
 */
static void handle_map_context(const listbox::ContextMenuEvent& data) {
// JS: this.$core.view.contextMenus.nodeMap = { selection: data.selection, count: data.selection.length };
core::view->contextMenus.nodeMap = nlohmann::json{
{"selection", data.selection},
{"count", static_cast<int>(data.selection.size())}
};
}

/**
 * JS: copy_map_names(selection)
 */
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

/**
 * JS: copy_map_internal_names(selection)
 */
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

/**
 * JS: copy_map_ids(selection)
 */
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

/**
 * JS: copy_map_export_paths(selection)
 */
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

/**
 * JS: open_map_export_directory(selection)
 */
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

// --- Map loading ---

/**
 * JS: methods.setup_wmo_minimap(wdt)
 */
static void setup_wmo_minimap(WDTLoader& wdt) {
try {
const auto& placement = wdt.worldModelPlacement;
uint32_t file_data_id = 0;

// JS: if (wdt.worldModel) { file_data_id = listfile.getByFilename(wdt.worldModel); }
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

// JS: const wmo_data = await this.$core.view.casc.getFile(file_data_id);
BufferWrapper wmo_data = core::view->casc->getVirtualFileByID(file_data_id);
// JS: const wmo = new WMOLoader(wmo_data, file_data_id);
// JS: await wmo.load();
// TODO(conversion): WMOLoader processing will be wired when WMOLoader integration is complete.
// When wired, this will load the WMO, find minimap textures by wmoID,
// compute group bounding boxes, position tiles absolutely, bin into a grid,
// and populate current_wmo_minimap with mask/tiles_by_coord/grid_size.
(void)wmo_data;
} catch (const std::exception& e) {
logging::write(std::format("failed to setup WMO minimap: {}", e.what()));
current_wmo_minimap = std::nullopt;
}
}

/**
 * JS: methods.load_map(mapID, mapDir)
 */
static void load_map(int mapID, const std::string& mapDir) {
// JS: const map_dir_lower = mapDir.toLowerCase();
std::string map_dir_lower = mapDir;
std::transform(map_dir_lower.begin(), map_dir_lower.end(), map_dir_lower.begin(), ::tolower);

// JS: this.$core.hideToast();
core::hideToast();

selected_map_id = mapID;
selected_map_dir = map_dir_lower;
has_selected_map_dir = true;

// JS: selected_wdt = null; current_wmo_minimap = null;
selected_wdt.reset();
current_wmo_minimap = std::nullopt;
core::view->mapViewerHasWorldModel = false;
core::view->mapViewerIsWMOMinimap = false;
core::view->mapViewerGridSize = nullptr;
// JS: this.$core.view.mapViewerSelection.splice(0);
core::view->mapViewerSelection.clear();

const std::string wdt_path = std::format("world/maps/{}/{}.wdt", map_dir_lower, map_dir_lower);
logging::write(std::format("loading map preview for {} ({})", map_dir_lower, mapID));

// JS: const data = await this.$core.view.casc.getFileByName(wdt_path);
BufferWrapper wdt_data = core::view->casc->getVirtualFileByName(wdt_path);
// JS: const wdt = selected_wdt = new WDTLoader(data);
// JS: wdt.load();
// TODO(conversion): WDTLoader processing will be wired when WDTLoader is fully converted.
// When wired, the WDT will be loaded and checked for terrain/WMO.
// If no terrain but has WMO, setup_wmo_minimap is called.
// Otherwise terrain minimap loader is used.
selected_wdt_data = std::move(wdt_data);

// Fallback until CASC is wired:
core::view->mapViewerTileLoader = "terrain";
core::view->mapViewerChunkMask = nullptr;
core::view->mapViewerSelectedMap = mapID;
core::view->mapViewerSelectedDir = mapDir;
}

// --- Export functions ---

/**
 * JS: methods.export_map_wmo()
 */
void export_map_wmo() {
casc::ExportHelper helper(1, "WMO");
helper.start();

try {
// JS: if (!selected_wdt || !selected_wdt.worldModelPlacement)
if (!selected_wdt || (selected_wdt->worldModelPlacement.id == 0 && selected_wdt->worldModel.empty()))
throw std::runtime_error("map does not contain a world model.");

const auto& placement = selected_wdt->worldModelPlacement;
uint32_t file_data_id = 0;
std::string file_name;

// JS: if (selected_wdt.worldModel) { ... } else { ... }
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
auto name = casc::listfile::getByID(file_data_id);
file_name = !name.empty() ? name : ("unknown_" + std::to_string(file_data_id) + ".wmo");
}

const std::string mark_file_name = casc::ExportHelper::replaceExtension(file_name, ".obj");
const std::string export_path = casc::ExportHelper::getExportPath(mark_file_name);

// JS: const data = await this.$core.view.casc.getFile(file_data_id);
BufferWrapper data = core::view->casc->getVirtualFileByID(file_data_id);
// JS: const wmo = new WMOExporter(data, file_data_id);
WMOExporter wmo(std::move(data), file_data_id, core::view->casc);
// JS: wmo.setDoodadSetMask({ [placement.doodadSetIndex]: { checked: true } });
std::vector<WMOExportDoodadSetMask> doodad_mask(placement.doodadSetIndex + 1);
doodad_mask[placement.doodadSetIndex].checked = true;
wmo.setDoodadSetMask(std::move(doodad_mask));
// JS: await wmo.exportAsOBJ(export_path, helper);
wmo.exportAsOBJ(export_path, &helper, nullptr);

if (helper.isCancelled())
return;

helper.mark(mark_file_name, true);
} catch (const std::exception& e) {
helper.mark("world model", false, e.what());
}

WMOExporter::clearCache();
helper.finish();
}

/**
 * JS: methods.export_map_wmo_minimap()
 */
void export_map_wmo_minimap() {
casc::ExportHelper helper(1, "minimap");
helper.start();

try {
// JS: let minimap_data = current_wmo_minimap;
if (!current_wmo_minimap) {
if (!selected_wdt || (selected_wdt->worldModelPlacement.id == 0 && selected_wdt->worldModel.empty()))
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

// JS: for (const [key, tile_list] of tiles_by_coord) { ... casc.getFile, BLPFile, canvas compositing ... }
for (const auto& [coord_key, tile_list] : tiles_by_coord) {
	// Parse coord key "x,y"
	auto comma = coord_key.find(',');
	if (comma == std::string::npos) continue;
	int tx = std::stoi(coord_key.substr(0, comma));
	int ty = std::stoi(coord_key.substr(comma + 1));

	// Composite all tiles at this position
	std::vector<uint8_t> composite(output_tile_size * output_tile_size * 4, 0);
	for (const auto& tile : tile_list) {
		try {
			BufferWrapper tile_data = core::view->casc->getVirtualFileByID(tile.fileDataID, true);
			casc::BLPImage blp(tile_data);
			auto rgba = blp.toUInt8Array(0, 0b1111);
			uint32_t tw = blp.width;
			uint32_t th = blp.height;

			for (int py = 0; py < output_tile_size; py++) {
				for (int px = 0; px < output_tile_size; px++) {
					int src_x = (std::min)(static_cast<int>(px * tw / output_tile_size), static_cast<int>(tw) - 1);
					int src_y = (std::min)(static_cast<int>(py * th / output_tile_size), static_cast<int>(th) - 1);
					int src_idx = (src_y * tw + src_x) * 4;
					int dst_idx = (py * output_tile_size + px) * 4;
					uint8_t a = rgba[src_idx + 3];
					if (a == 0) continue;
					if (a == 255) {
						composite[dst_idx + 0] = rgba[src_idx + 0];
						composite[dst_idx + 1] = rgba[src_idx + 1];
						composite[dst_idx + 2] = rgba[src_idx + 2];
						composite[dst_idx + 3] = 255;
					} else {
						float af = a / 255.0f;
						composite[dst_idx + 0] = static_cast<uint8_t>(composite[dst_idx + 0] * (1 - af) + rgba[src_idx + 0] * af);
						composite[dst_idx + 1] = static_cast<uint8_t>(composite[dst_idx + 1] * (1 - af) + rgba[src_idx + 1] * af);
						composite[dst_idx + 2] = static_cast<uint8_t>(composite[dst_idx + 2] * (1 - af) + rgba[src_idx + 2] * af);
						composite[dst_idx + 3] = (std::max)(composite[dst_idx + 3], a);
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

writer.write(out_path);

// JS: const export_paths = this.$core.openLastExportStream();
auto export_paths = core::openLastExportStream();
export_paths.writeLine("png:" + out_path);
export_paths.close();

helper.mark(relative_path, true);
logging::write(std::format("WMO minimap exported: {}", out_path));

} catch (const std::exception& e) {
helper.mark("WMO minimap", false, e.what());
}

helper.finish();
}

/**
 * JS: methods.export_map()
 */
void export_map() {
// JS: const format = this.$core.view.config.exportMapFormat;
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

/**
 * JS: methods.export_selected_map()
 */
static void export_selected_map() {
auto& view = *core::view;
const auto& export_tiles = view.mapViewerSelection;
int export_quality = view.config.value("exportMapQuality", 512);

// JS: if (export_tiles.length === 0) return this.$core.setToast('error', ...);
if (export_tiles.empty()) {
core::setToast("error", "You haven't selected any tiles; hold shift and click on a map tile to select it.", {}, -1);
return;
}

// Convert JSON selection to int vector
std::vector<int> tile_indices;
for (const auto& t : export_tiles)
tile_indices.push_back(t.get<int>());

casc::ExportHelper helper(static_cast<int>(tile_indices.size()), "tile");
helper.start();

const std::string dir = casc::ExportHelper::getExportPath(
(std::filesystem::path("maps") / selected_map_dir).string());
auto export_paths = core::openLastExportStream();
const std::string mark_path = (std::filesystem::path("maps") / selected_map_dir / selected_map_dir).string();

for (int index : tile_indices) {
if (helper.isCancelled())
break;

ADTExporter adt(selected_map_id.value_or(0), selected_map_dir, static_cast<uint32_t>(index));

uint32_t tile_x = static_cast<uint32_t>(index) / constants::GAME::MAP_SIZE;
uint32_t tile_y = static_cast<uint32_t>(index) % constants::GAME::MAP_SIZE;

std::vector<ADTGameObject> game_objects_vec;

// JS: if (this.$core.view.config.mapsIncludeGameObjects === true) { ... }
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
// JS: const out = await adt.export(dir, export_quality, game_objects, helper);
// JS: await export_paths?.writeLine(out.type + ':' + out.path);
// Convert tab_maps::ADTGameObject to ::ADTGameObject for ADTExporter API
std::vector<::ADTGameObject> export_game_objects;
for (const auto& go : game_objects_vec) {
	::ADTGameObject ego;
	ego.FileDataID = go.FileDataID;
	ego.Position = go.Position;
	ego.Rotation = go.Rotation;
	ego.scale = go.scale;
	export_game_objects.push_back(std::move(ego));
}
auto out = adt.exportTile(dir, export_quality,
	export_game_objects.empty() ? nullptr : &export_game_objects,
	&helper, core::view->casc);
export_paths.writeLine(out.type + ":" + out.path.string());
helper.mark(mark_path, true);
} catch (const std::exception& e) {
helper.mark(mark_path, false, e.what());
}
}

export_paths.close();
ADTExporter::clearCache();
helper.finish();
}

/**
 * JS: methods.export_selected_map_as_raw()
 */
static void export_selected_map_as_raw() {
auto& view = *core::view;
const auto& export_tiles = view.mapViewerSelection;

if (export_tiles.empty()) {
core::setToast("error", "You haven't selected any tiles; hold shift and click on a map tile to select it.", {}, -1);
return;
}

std::vector<int> tile_indices;
for (const auto& t : export_tiles)
tile_indices.push_back(t.get<int>());

casc::ExportHelper helper(static_cast<int>(tile_indices.size()), "tile");
helper.start();

const std::string dir = casc::ExportHelper::getExportPath(
(std::filesystem::path("maps") / selected_map_dir).string());
auto export_paths = core::openLastExportStream();
const std::string mark_path = (std::filesystem::path("maps") / selected_map_dir / selected_map_dir).string();

for (int index : tile_indices) {
if (helper.isCancelled())
break;

ADTExporter adt(selected_map_id.value_or(0), selected_map_dir, static_cast<uint32_t>(index));

try {
// JS: const out = await adt.export(dir, 0, undefined, helper);
auto out = adt.exportTile(dir, 0, nullptr, &helper, core::view->casc);
export_paths.writeLine(out.type + ":" + out.path.string());
helper.mark(mark_path, true);
} catch (const std::exception& e) {
helper.mark(mark_path, false, e.what());
}
}

export_paths.close();
ADTExporter::clearCache();
helper.finish();
}

/**
 * JS: methods.export_selected_map_as_png()
 */
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
// JS: const tile_coords = export_tiles.map(index => ({ index, x: Math.floor(index / MAP_SIZE), y: index % MAP_SIZE }));
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

// JS: const first_tile = await load_map_tile(tile_coords[0].x, tile_coords[0].y, 512);
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

// JS: const sorted_tiles = [...export_tiles].sort((a, b) => a - b);
// JS: const tile_hash = crypto.createHash('md5').update(sorted_tiles.join(',')).digest('hex').substring(0, 8);
auto sorted_tiles = tile_indices;
std::sort(sorted_tiles.begin(), sorted_tiles.end());

std::string tiles_str;
for (size_t i = 0; i < sorted_tiles.size(); i++) {
if (i > 0) tiles_str += ',';
tiles_str += std::to_string(sorted_tiles[i]);
}

// MD5 hash matching JS crypto.createHash('md5')
std::string tile_hash = md5_hex(tiles_str).substr(0, 8);

const std::string filename = selected_map_dir + "_" + tile_hash + ".png";
const std::string out_path = casc::ExportHelper::getExportPath(
(std::filesystem::path("maps") / selected_map_dir / filename).string());

writer.write(out_path);

auto stats = writer.getStats();
logging::write(std::format("map export complete: {} ({} tiles)", out_path, stats.totalTiles));

auto export_paths = core::openLastExportStream();
export_paths.writeLine("png:" + out_path);
export_paths.close();

helper.mark((std::filesystem::path("maps") / selected_map_dir / filename).string(), true);

} catch (const std::exception& e) {
helper.mark("PNG export", false, e.what());
logging::write(std::format("PNG export failed: {}", e.what()));
}

helper.finish();
}

/**
 * JS: methods.export_selected_map_as_heightmaps()
 */
static void export_selected_map_as_heightmaps() {
auto& view = *core::view;
const auto& export_tiles = view.mapViewerSelection;
int export_resolution = view.config.value("heightmapResolution", 256);

if (export_tiles.empty()) {
core::setToast("error", "You haven't selected any tiles; hold shift and click on a map tile to select it.", {}, -1);
return;
}

// JS: if (export_resolution === -1) export_resolution = this.$core.view.config.heightmapCustomResolution;
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

// JS: this.$core.setToast('progress', 'Calculating height range across all tiles...', null, -1, false);
core::setToast("progress", "Calculating height range across all tiles...", {}, -1, false);
float global_min_height = (std::numeric_limits<float>::infinity)();
float global_max_height = -(std::numeric_limits<float>::infinity)();

// First pass: determine global height range
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

// Second pass: generate heightmap PNGs
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

// JS: const writer = new PNGWriter(export_resolution, export_resolution);
PNGWriter writer(export_resolution, export_resolution);
int bit_depth = view.config.value("heightmapBitDepth", 8);

if (bit_depth == 32) {
// JS: writer.bytesPerPixel = 4; writer.bitDepth = 8; writer.colorType = 6;
writer.bytesPerPixel = 4;
writer.bitDepth = 8;
writer.colorType = 6;

auto& pixel_data = writer.getPixelData();
pixel_data.resize(export_resolution * export_resolution * 4);

for (size_t j = 0; j < height_data->heights.size(); j++) {
float normalized_height = (height_data->heights[j] - global_min_height) / height_range;

// JS: float_view[0] = normalized_height; pixel_data = byte_view bytes
uint8_t byte_view[4];
std::memcpy(byte_view, &normalized_height, 4);

size_t pixel_offset = j * 4;
pixel_data[pixel_offset + 0] = byte_view[0];
pixel_data[pixel_offset + 1] = byte_view[1];
pixel_data[pixel_offset + 2] = byte_view[2];
pixel_data[pixel_offset + 3] = byte_view[3];
}
} else if (bit_depth == 16) {
// JS: writer.bytesPerPixel = 2; writer.bitDepth = 16; writer.colorType = 0;
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
// 8-bit grayscale (default)
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

writer.write(out_path);

export_paths.writeLine("png:" + out_path);

helper.mark((std::filesystem::path("maps") / selected_map_dir / "heightmaps" / filename).string(), true);
logging::write(std::format("exported heightmap: {}", out_path));

} catch (const std::exception& e) {
helper.mark(filename, false, e.what());
logging::write(std::format("failed to export heightmap for tile {}: {}", tile_index, e.what()));
}
}

export_paths.close();
helper.finish();
}

// --- Tab registration ---

/**
 * JS: register() { this.registerNavButton('Maps', 'map.svg', InstallType.CASC); }
 */
void registerTab() {
// JS: this.registerNavButton('Maps', 'map.svg', InstallType.CASC);
modules::register_nav_button("tab_maps", "Maps", "map.svg", install_type::CASC);
}

/**
 * JS: methods.initialize()
 */
static void initialize() {
// JS: this.$core.showLoadingScreen(3);
core::showLoadingScreen(3);

// JS: await this.$core.progressLoadingScreen('Loading WMO minimap textures...');
core::progressLoadingScreen("Loading WMO minimap textures...");

// JS: wmo_minimap_textures = new Map();
wmo_minimap_textures_loaded = true;
wmo_minimap_textures.clear();

// JS: for (const row of (await db2.WMOMinimapTexture.getAllRows()).values()) { ... }
auto& wmo_mm_table = casc::db2::getTable("WMOMinimapTexture");
auto all_wmo_rows = wmo_mm_table.getAllRows();

for (auto& [id, row] : all_wmo_rows) {
auto wmoid_it = row.find("WMOID");
if (wmoid_it == row.end())
continue;
uint32_t wmo_id = fieldToUint32(wmoid_it->second);

// JS: tiles.push({ groupNum: row.GroupNum, blockX: row.BlockX, blockY: row.BlockY, fileDataID: row.FileDataID });
wmo_minimap_textures[wmo_id].push_back(row);
}

logging::write(std::format("loaded {} WMO minimap entries", wmo_minimap_textures.size()));

// JS: await this.$core.progressLoadingScreen('Loading maps...');
core::progressLoadingScreen("Loading maps...");

// JS: const maps = [];
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

// JS: if (entry.WdtFileDataID) { ... } else if (listfile.getByFilename(wdt_path)) { ... }
if (wdt_fid) {
if (!casc::listfile::existsByID(wdt_fid))
casc::listfile::addEntry(wdt_fid, wdt_path);

maps.push_back(std::format("{}\x19[{}]\x19{}\x19({})", expansion_id, id, map_name, directory));
} else if (casc::listfile::getByFilename(wdt_path)) {
maps.push_back(std::format("{}\x19[{}]\x19{}\x19({})", expansion_id, id, map_name, directory));
}
}

// JS: this.$core.view.mapViewerMaps = maps;
core::view->mapViewerMaps.clear();
for (const auto& m : maps)
core::view->mapViewerMaps.push_back(m);

// JS: this.$core.hideLoadingScreen();
core::hideLoadingScreen();
}

/**
 * JS: mounted()
 */
void mounted() {
// JS: this.$core.view.mapViewerTileLoader = load_map_tile;
core::view->mapViewerTileLoader = "terrain";

// Initialize will be called after tab is shown (lazy init in render)
tab_initialized = false;
prev_selection_first.clear();
}

// --- ImGui render ---

/**
 * Render the maps tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render() {
auto& view = *core::view;

// Lazy initialization (equivalent of mounted() calling initialize())
if (!tab_initialized) {
tab_initialized = true;
initialize();
}

// --- Watch: selectionMaps ---
// JS: this.$core.view.$watch('selectionMaps', async selection => { ... })
std::string current_selection_first;
if (!view.selectionMaps.empty()) {
current_selection_first = view.selectionMaps[0].get<std::string>();
}

if (current_selection_first != prev_selection_first) {
prev_selection_first = current_selection_first;
if (!view.isBusy && !current_selection_first.empty()) {
try {
auto map = parse_map_entry(current_selection_first);
if (!selected_map_id || *selected_map_id != map.id)
load_map(map.id, map.dir);
} catch (...) {}
}
}

// --- UI Layout ---
// JS: <div class="tab list-tab" id="tab-maps">

ImGui::PushID("tab-maps");

// --- Expansion filter buttons ---
// JS: <div class="expansion-buttons"> ... </div>
{
ImGui::BeginGroup();

// "Show All" button
bool all_active = (view.selectedExpansionFilter == -1);
if (all_active)
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.133f, 0.71f, 0.286f, 1.0f));
if (ImGui::Button("All##exp_all"))
view.selectedExpansionFilter = -1;
if (all_active)
ImGui::PopStyleColor();

// Per-expansion buttons
for (const auto& exp : constants::EXPANSIONS) {
ImGui::SameLine();
bool active = (view.selectedExpansionFilter == exp.id);
if (active)
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.133f, 0.71f, 0.286f, 1.0f));
if (ImGui::Button(std::format("{}##exp_{}", exp.shortName, exp.id).c_str()))
view.selectedExpansionFilter = exp.id;
if (active)
ImGui::PopStyleColor();
}

ImGui::EndGroup();
}

// --- Map list + filter + context menu ---
{
// Convert JSON items to string vector for listbox
std::vector<std::string> map_items;
map_items.reserve(view.mapViewerMaps.size());
for (const auto& item : view.mapViewerMaps)
map_items.push_back(item.get<std::string>());

// Convert JSON selection to string vector
std::vector<std::string> selection_strs;
for (const auto& s : view.selectionMaps)
selection_strs.push_back(s.get<std::string>());

// Determine copy mode
listbox::CopyMode copy_mode = listbox::CopyMode::Default;
std::string copy_mode_str = view.config.value("copyMode", "Default");
if (copy_mode_str == "DIR") copy_mode = listbox::CopyMode::DIR;
else if (copy_mode_str == "FID") copy_mode = listbox::CopyMode::FID;

listbox_maps::render(
"listbox-maps",
map_items,
view.userInputFilterMaps,
selection_strs,
true,   // single
true,   // keyinput
view.config.value("regexFilters", false),
copy_mode,
view.config.value("pasteSelection", false),
view.config.value("removePathSpacesCopy", false),
"map",  // unittype
nullptr, // overrideItems
false,   // disable
"maps",  // persistscrollkey
{},      // quickfilters
false,   // nocopy
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

// Context menu
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

// Filter input
{
bool regex_enabled = view.config.value("regexFilters", false);
if (regex_enabled)
ImGui::TextDisabled("Regex Enabled");

char filter_buf[256] = {};
std::strncpy(filter_buf, view.userInputFilterMaps.c_str(), sizeof(filter_buf) - 1);
if (ImGui::InputText("##filter-maps", filter_buf, sizeof(filter_buf)))
view.userInputFilterMaps = filter_buf;
}
}

// --- Map Viewer ---
{
auto tile_loader = [](int x, int y, int size) -> std::vector<uint8_t> {
std::string loader_type = "terrain";
if (core::view->mapViewerTileLoader.is_string())
loader_type = core::view->mapViewerTileLoader.get<std::string>();

if (loader_type == "wmo_minimap")
return load_wmo_minimap_tile(x, y, size);
else
return load_map_tile(x, y, size);
};

// Convert mapViewerChunkMask (JSON) to std::vector<int>
std::vector<int> mask;
if (view.mapViewerChunkMask.is_array()) {
for (const auto& v : view.mapViewerChunkMask)
mask.push_back(v.get<int>());
}

// Convert mapViewerSelection to std::vector<int>
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
512,   // tileSize
map_id,
12,    // zoom
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
}

// --- Export buttons ---
{
if (view.mapViewerHasWorldModel) {
bool disabled = view.isBusy;
if (disabled) app::theme::BeginDisabledButton();
if (ImGui::Button("Export Global WMO"))
export_map_wmo();
ImGui::SameLine();
if (ImGui::Button("Export WMO Minimap"))
export_map_wmo_minimap();
if (disabled) app::theme::EndDisabledButton();
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

// --- Sidebar ---
{
ImGui::Separator();
ImGui::Text("Export Options");

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

ImGui::Separator();
ImGui::Text("Model Textures");

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
ImGui::Separator();
ImGui::Text("Terrain Texture Quality");

// Texture quality dropdown
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

ImGui::Separator();
ImGui::Text("Heightmaps");

// Heightmap resolution dropdown
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

// Heightmap bit depth dropdown
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

// Custom resolution input
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
}

ImGui::PopID();
}

} // namespace tab_maps
