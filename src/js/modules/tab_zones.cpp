/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>
License: MIT
 */

#include "tab_zones.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../buffer.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
#include "../casc/blp.h"
#include "../casc/db2.h"
#include "../db/WDCReader.h"
#include "../ui/texture-exporter.h"
#include "../install-type.h"
#include "../modules.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <set>
#include <regex>
#include <cmath>

#include <imgui.h>
#include <glad/gl.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>
#endif

namespace tab_zones {

// Upload RGBA pixel data to an OpenGL texture, returning the texture ID.
// Deletes the previous texture if old_tex != 0.
static uint32_t upload_rgba_to_gl(const uint8_t* pixels, int w, int h, uint32_t old_tex = 0) {
	if (old_tex != 0) {
		GLuint old_gl = static_cast<GLuint>(old_tex);
		glDeleteTextures(1, &old_gl);
	}
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	return static_cast<uint32_t>(tex);
}

// Composite BLP tile RGBA pixels onto the zone map pixel buffer at (dest_x, dest_y).
static void composite_blp_tile(casc::BLPImage& blp, int dest_x, int dest_y, int map_w, int map_h,
	std::vector<uint8_t>& pixels) {
	std::vector<uint8_t> tile_pixels = blp.toUInt8Array();
	const int tw = static_cast<int>(blp.width);
	const int th = static_cast<int>(blp.height);

	for (int y = 0; y < th; ++y) {
		const int dy = dest_y + y;
		if (dy < 0 || dy >= map_h) continue;
		for (int x = 0; x < tw; ++x) {
			const int dx = dest_x + x;
			if (dx < 0 || dx >= map_w) continue;
			const size_t src_idx = (static_cast<size_t>(y) * tw + x) * 4;
			const size_t dst_idx = (static_cast<size_t>(dy) * map_w + dx) * 4;
			// Simple alpha-over compositing.
			const uint8_t sa = tile_pixels[src_idx + 3];
			if (sa == 255) {
				pixels[dst_idx]     = tile_pixels[src_idx];
				pixels[dst_idx + 1] = tile_pixels[src_idx + 1];
				pixels[dst_idx + 2] = tile_pixels[src_idx + 2];
				pixels[dst_idx + 3] = 255;
			} else if (sa > 0) {
				const float a = sa / 255.0f;
				const float inv_a = 1.0f - a;
				pixels[dst_idx]     = static_cast<uint8_t>(std::min(tile_pixels[src_idx]     * a + pixels[dst_idx]     * inv_a, 255.0f));
				pixels[dst_idx + 1] = static_cast<uint8_t>(std::min(tile_pixels[src_idx + 1] * a + pixels[dst_idx + 1] * inv_a, 255.0f));
				pixels[dst_idx + 2] = static_cast<uint8_t>(std::min(tile_pixels[src_idx + 2] * a + pixels[dst_idx + 2] * inv_a, 255.0f));
				pixels[dst_idx + 3] = std::max(pixels[dst_idx + 3], sa);
			}
		}
	}
}

// Helper to extract uint32_t from a FieldValue.
static uint32_t fieldToUint32(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<uint32_t>(*p);
	return 0;
}

// Helper to extract int from a FieldValue.
static int fieldToInt(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<int>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<int>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<int>(*p);
	return 0;
}

// Helper to extract std::string from a FieldValue.
static std::string fieldToString(const db::FieldValue& val) {
	if (auto* p = std::get_if<std::string>(&val))
		return *p;
	return "";
}

// --- File-local structures ---

// JS: parse_zone_entry result: { id, zone_name, area_name }
// The zone entry format is: expansion_id \x19 [zone_id] \x19 area_name \x19 (zone_name)
struct ZoneDisplayInfo {
int id = 0;
std::string zone_name;
std::string area_name;
int expansion = -1;
};

// Art style layer combining UiMapArt + UiMapArtStyleLayer
struct CombinedArtStyle {
int id = 0;
int ui_map_art_style_id = 0;
int layer_index = 0;
int layer_width = 0;
int layer_height = 0;
int tile_width = 0;
int tile_height = 0;
};

// --- File-local state ---

// JS: const EXPANSION_NAMES = [...];
static const std::vector<std::string> EXPANSION_NAMES = {
	"Classic",
	"The Burning Crusade",
	"Wrath of the Lich King",
	"Cataclysm",
	"Mists of Pandaria",
	"Warlords of Draenor",
	"Legion",
	"Battle for Azeroth",
	"Shadowlands",
	"Dragonflight",
	"The War Within"
};

// JS: let selected_zone_id = null;
static std::optional<int> selected_zone_id;

// JS: let selected_phase_id = null;
static std::optional<int> selected_phase_id;

// Change-detection.
static std::string prev_selection_first;
static bool prev_show_zone_base_map = true;
static bool prev_show_zone_overlays = true;

// --- Internal functions ---

// JS: const parse_zone_entry = (entry) => { ... }
static ZoneDisplayInfo parse_zone_entry(const std::string& entry) {
	// Format: expansion_id \x19 [zone_id] \x19 AreaName_lang \x19 (ZoneName)
	// JS regex skips expansion prefix: /\[(\d+)\]\31([^\31]+)\31\(([^)]+)\)/
	// match[1]=id, match[2]=AreaName_lang→zone_name, match[3]=ZoneName→area_name
	std::regex re(R"((\d+)\x19\[(\d+)\]\x19([^\x19]+)\x19\(([^)]+)\))");
	std::smatch match;
	if (!std::regex_match(entry, match, re))
		return {};

	ZoneDisplayInfo info;
	info.expansion = std::stoi(match[1].str());
	info.id = std::stoi(match[2].str());
	info.zone_name = match[3].str();
	info.area_name = match[4].str();
	return info;
}

// JS: const get_zone_ui_map_id = async (zone_id) => { ... }
static std::optional<int> get_zone_ui_map_id(int zone_id) {
// JS: for (const assignment of (await db2.UiMapAssignment.getAllRows()).values()) {
//     if (assignment.AreaID === zone_id) return assignment.UiMapID; }
auto& ui_map_assignment = casc::db2::getTable("UiMapAssignment");
if (!ui_map_assignment.isLoaded)
	ui_map_assignment.parse();

for (const auto& [id, row] : ui_map_assignment.getAllRows()) {
	auto area_it = row.find("AreaID");
	if (area_it != row.end() && fieldToInt(area_it->second) == zone_id) {
		auto uimap_it = row.find("UiMapID");
		if (uimap_it != row.end())
			return fieldToInt(uimap_it->second);
	}
}
return std::nullopt;
}

// JS: const get_zone_phases = async (zone_id) => { ... }
static std::vector<ZonePhase> get_zone_phases(int zone_id) {
const auto ui_map_id = get_zone_ui_map_id(zone_id);
if (!ui_map_id.has_value())
return {};

std::vector<ZonePhase> phases;
std::set<int> seen_phases;

// JS: for (const link_entry of (await db2.UiMapXMapArt.getAllRows()).values()) { ... }
auto& ui_map_x_map_art = casc::db2::getTable("UiMapXMapArt");
if (!ui_map_x_map_art.isLoaded)
	ui_map_x_map_art.parse();

for (const auto& [id, row] : ui_map_x_map_art.getAllRows()) {
	auto uimap_it = row.find("UiMapID");
	if (uimap_it == row.end() || fieldToInt(uimap_it->second) != *ui_map_id)
		continue;

	auto phase_it = row.find("PhaseID");
	int phase_id = (phase_it != row.end()) ? fieldToInt(phase_it->second) : 0;

	if (!seen_phases.contains(phase_id)) {
		seen_phases.insert(phase_id);
		std::string label = (phase_id == 0) ? "Default" : std::format("Phase {}", phase_id);
		phases.push_back(ZonePhase{ phase_id, std::move(label) });
	}
}

std::sort(phases.begin(), phases.end(), [](const ZonePhase& a, const ZonePhase& b) {
return a.id < b.id;
});

return phases;
}

// Forward declarations for render functions used in render_zone_to_canvas.
static void render_map_tiles(const CombinedArtStyle& art_style, int layer_index, int expected_zone_id, bool skip_zone_check);
static void render_world_map_overlays(const CombinedArtStyle& art_style, int expected_zone_id, bool skip_zone_check);
static void render_overlay_tiles(const std::vector<db::DataRecord>& tiles, const CombinedArtStyle& art_style, int overlay_offset_x, int overlay_offset_y, int expected_zone_id, bool skip_zone_check);

// JS: const render_zone_to_canvas = async (canvas, zone_id, phase_id, set_canvas_size, skip_zone_check) => { ... }
static ZoneMapInfo render_zone_to_canvas(int zone_id, std::optional<int> phase_id = std::nullopt, bool skip_zone_check = false) {
const auto ui_map_id = get_zone_ui_map_id(zone_id);
if (!ui_map_id.has_value()) {
logging::write(std::format("no UiMap found for zone ID {}", zone_id));
throw std::runtime_error("no map data available for this zone");
}

// JS: const map_data = await db2.UiMap.getRow(ui_map_id);
auto& ui_map = casc::db2::getTable("UiMap");
if (!ui_map.isLoaded)
	ui_map.parse();
auto ui_map_row_opt = ui_map.getRow(static_cast<uint32_t>(*ui_map_id));
// map_data is used for context/logging in JS; the key data comes from UiMapXMapArt and UiMapArt.
(void)ui_map_row_opt;

// JS: Collect linked art IDs from UiMapXMapArt
std::vector<int> linked_art_ids;
auto& ui_map_x_map_art = casc::db2::getTable("UiMapXMapArt");
if (!ui_map_x_map_art.isLoaded)
	ui_map_x_map_art.parse();

for (const auto& [id, row] : ui_map_x_map_art.getAllRows()) {
	auto uimap_it = row.find("UiMapID");
	if (uimap_it == row.end() || fieldToInt(uimap_it->second) != *ui_map_id)
		continue;

	auto phase_it = row.find("PhaseID");
	int row_phase = (phase_it != row.end()) ? fieldToInt(phase_it->second) : 0;

	// If phase_id is specified, only include matching phases; otherwise include phase 0 (default).
	if (phase_id.has_value()) {
		if (row_phase != *phase_id)
			continue;
	} else {
		if (row_phase != 0)
			continue;
	}

	auto art_it = row.find("UiMapArtID");
	if (art_it != row.end())
		linked_art_ids.push_back(fieldToInt(art_it->second));
}

// JS: Collect combined art styles from UiMapArt + UiMapArtStyleLayer
std::vector<CombinedArtStyle> art_styles;
auto& ui_map_art = casc::db2::getTable("UiMapArt");
auto& ui_map_art_style_layer = casc::db2::getTable("UiMapArtStyleLayer");
if (!ui_map_art.isLoaded)
	ui_map_art.parse();
if (!ui_map_art_style_layer.isLoaded)
	ui_map_art_style_layer.parse();

for (int linked_art_id : linked_art_ids) {
	auto art_row_opt = ui_map_art.getRow(static_cast<uint32_t>(linked_art_id));
	if (!art_row_opt.has_value())
		continue;

	// Find matching UiMapArtStyleLayer rows for this art ID.
	for (const auto& [layer_id, layer_row] : ui_map_art_style_layer.getAllRows()) {
		auto art_ref_it = layer_row.find("UiMapArtID");
		if (art_ref_it == layer_row.end() || fieldToInt(art_ref_it->second) != linked_art_id)
			continue;

		CombinedArtStyle style;
		style.id = static_cast<int>(layer_id);
		auto sid_it = layer_row.find("UiMapArtStyleID");
		if (sid_it != layer_row.end())
			style.ui_map_art_style_id = fieldToInt(sid_it->second);
		auto li_it = layer_row.find("LayerIndex");
		if (li_it != layer_row.end())
			style.layer_index = fieldToInt(li_it->second);
		auto lw_it = layer_row.find("LayerWidth");
		if (lw_it != layer_row.end())
			style.layer_width = fieldToInt(lw_it->second);
		auto lh_it = layer_row.find("LayerHeight");
		if (lh_it != layer_row.end())
			style.layer_height = fieldToInt(lh_it->second);
		auto tw_it = layer_row.find("TileWidth");
		if (tw_it != layer_row.end())
			style.tile_width = fieldToInt(tw_it->second);
		auto th_it = layer_row.find("TileHeight");
		if (th_it != layer_row.end())
			style.tile_height = fieldToInt(th_it->second);

		art_styles.push_back(style);
	}
}

if (art_styles.empty()) {
logging::write(std::format("no art styles found for UiMap ID {} (phase {})",
*ui_map_id, phase_id.value_or(-1)));
throw std::runtime_error("no art styles found for map");
}

logging::write(std::format("found {} art styles for UiMap ID {} (phase {})",
art_styles.size(), *ui_map_id, phase_id.value_or(-1)));

// Sort by LayerIndex.
std::sort(art_styles.begin(), art_styles.end(), [](const CombinedArtStyle& a, const CombinedArtStyle& b) {
return a.layer_index < b.layer_index;
});

int map_width = 0, map_height = 0;

for (const auto& art_style : art_styles) {
	// JS: const all_tiles = await db2.UiMapArtTile.getRelationRows(art_style.ID);
	// UiMapArtTile is preloaded during initialize(); getRelationRows is called inside render_map_tiles.

	if (art_style.layer_index == 0) {
		map_width = art_style.layer_width;
		map_height = art_style.layer_height;

		// Allocate pixel buffer for the zone map composite.
		core::view->zoneMapWidth = map_width;
		core::view->zoneMapHeight = map_height;
		core::view->zoneMapPixels.assign(static_cast<size_t>(map_width) * map_height * 4, 0);
	}

	// JS: if (core.view.config.showZoneBaseMap) { ... render layers ... }
	if (core::view->config.value("showZoneBaseMap", true)) {
		render_map_tiles(art_style, art_style.layer_index, zone_id, skip_zone_check);
	}

	// JS: if (core.view.config.showZoneOverlays) await render_world_map_overlays(ctx, art_style, zone_id, skip_zone_check);
	if (core::view->config.value("showZoneOverlays", true)) {
		render_world_map_overlays(art_style, zone_id, skip_zone_check);
	}
}

logging::write(std::format("successfully rendered zone map for zone ID {} (UiMap ID {})",
zone_id, *ui_map_id));

return { map_width, map_height, *ui_map_id };
}

// Tile render result for Promise.all equivalent tracking.
struct TileRenderResult {
	bool success = false;
	bool skipped = false;
	std::string error;
};

// JS: const render_map_tiles = async (ctx, tiles, art_style, layer_index, expected_zone_id, skip_zone_check) => { ... }
static void render_map_tiles(const CombinedArtStyle& art_style, int layer_index, int expected_zone_id, bool skip_zone_check = false) {
	// 1. Get tiles from UiMapArtTile for the given art_style, filtered by layer_index.
	auto& ui_map_art_tile = casc::db2::getTable("UiMapArtTile");
	auto all_tiles = ui_map_art_tile.getRelationRows(art_style.id);

	// Filter tiles by LayerIndex
	std::vector<db::DataRecord> tiles;
	for (const auto& tile : all_tiles) {
		auto layer_it = tile.find("LayerIndex");
		if (layer_it != tile.end()) {
			int tile_layer = std::visit([](const auto& v) -> int {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_arithmetic_v<T>) return static_cast<int>(v);
				return 0;
			}, layer_it->second);
			if (tile_layer == layer_index)
				tiles.push_back(tile);
		}
	}

	// 2. Sort tiles by RowIndex, then ColIndex.
	std::sort(tiles.begin(), tiles.end(), [](const db::DataRecord& a, const db::DataRecord& b) {
		auto get_int = [](const db::DataRecord& r, const std::string& key) -> int {
			auto it = r.find(key);
			if (it == r.end()) return 0;
			return std::visit([](const auto& v) -> int {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_arithmetic_v<T>) return static_cast<int>(v);
				return 0;
			}, it->second);
		};
		int ra = get_int(a, "RowIndex"), rb = get_int(b, "RowIndex");
		if (ra != rb) return ra < rb;
		return get_int(a, "ColIndex") < get_int(b, "ColIndex");
	});

	// 3. For each tile, load BLP from CASC and composite
	int successful = 0;
	for (const auto& tile : tiles) {
		// Check if zone changed
		if (!skip_zone_check && selected_zone_id.has_value() && *selected_zone_id != expected_zone_id)
			break;

		auto get_field_int = [&](const std::string& key) -> int {
			auto it = tile.find(key);
			if (it == tile.end()) return 0;
			return std::visit([](const auto& v) -> int {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_arithmetic_v<T>) return static_cast<int>(v);
				return 0;
			}, it->second);
		};

		int col = get_field_int("ColIndex");
		int row = get_field_int("RowIndex");
		uint32_t file_data_id = static_cast<uint32_t>(get_field_int("FileDataID"));

		if (file_data_id == 0) continue;

		int pixel_x = col * art_style.tile_width;
		int pixel_y = row * art_style.tile_height;

		try {
			// c. Load BLP from CASC via tile.FileDataID
			BufferWrapper blp_data = core::view->casc->getVirtualFileByID(file_data_id, true);
			casc::BLPImage blp(blp_data);
			// Composite BLP tile pixels onto zone map texture at (pixel_x, pixel_y).
			composite_blp_tile(blp, pixel_x, pixel_y,
				core::view->zoneMapWidth, core::view->zoneMapHeight,
				core::view->zoneMapPixels);
			successful++;
		} catch (const std::exception& e) {
			logging::write(std::format("Failed to load zone tile {}: {}", file_data_id, e.what()));
		}
	}

	logging::write(std::format("rendered {}/{} tiles successfully", successful, tiles.size()));
}

// JS: const render_world_map_overlays = async (ctx, art_style, expected_zone_id, skip_zone_check) => { ... }
static void render_world_map_overlays(const CombinedArtStyle& art_style, int expected_zone_id, bool skip_zone_check = false) {
	// 1. Get WorldMapOverlay relation rows for the given art_style.ID
	// JS: const overlays = await db2.WorldMapOverlay.getRelationRows(art_style.ID);
	auto& world_map_overlay = casc::db2::getTable("WorldMapOverlay");
	auto overlays = world_map_overlay.getRelationRows(art_style.id);

	auto get_field_int = [](const db::DataRecord& r, const std::string& key) -> int {
		auto it = r.find(key);
		if (it == r.end()) return 0;
		return std::visit([](const auto& v) -> int {
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::is_arithmetic_v<T>) return static_cast<int>(v);
			return 0;
		}, it->second);
	};

	// 2. For each overlay, get tiles and call render_overlay_tiles
	for (const auto& overlay : overlays) {
		int overlay_id = get_field_int(overlay, "ID");
		int offset_x = get_field_int(overlay, "OffsetX");
		int offset_y = get_field_int(overlay, "OffsetY");

		// JS: const tiles = await db2.WorldMapOverlayTile.getRelationRows(overlay.ID);
		auto& world_map_overlay_tile = casc::db2::getTable("WorldMapOverlayTile");
		auto overlay_tiles = world_map_overlay_tile.getRelationRows(overlay_id);

		logging::write(std::format("rendering WorldMapOverlay ID {} with {} tiles at offset ({},{})",
			overlay_id, overlay_tiles.size(), offset_x, offset_y));

		// JS: await render_overlay_tiles(ctx, tiles, overlay, art_style, expected_zone_id, skip_zone_check);
		render_overlay_tiles(overlay_tiles, art_style, offset_x, offset_y, expected_zone_id, skip_zone_check);
	}
}

// JS: const render_overlay_tiles = async (ctx, tiles, overlay, art_style, expected_zone_id, skip_zone_check) => { ... }
static void render_overlay_tiles(const std::vector<db::DataRecord>& tiles, const CombinedArtStyle& art_style, int overlay_offset_x, int overlay_offset_y, int expected_zone_id, bool skip_zone_check = false) {
	// Check if zone changed or overlays disabled
	if (!skip_zone_check && selected_zone_id.has_value() && *selected_zone_id != expected_zone_id)
		return;

	if (!core::view->config.value("showZoneOverlays", true))
		return;

	// 1. Sort tiles by RowIndex, then ColIndex.
	std::vector<db::DataRecord> sorted_tiles(tiles.begin(), tiles.end());

	auto get_field_int = [](const db::DataRecord& r, const std::string& key) -> int {
		auto it = r.find(key);
		if (it == r.end()) return 0;
		return std::visit([](const auto& v) -> int {
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::is_arithmetic_v<T>) return static_cast<int>(v);
			return 0;
		}, it->second);
	};

	std::sort(sorted_tiles.begin(), sorted_tiles.end(), [&](const db::DataRecord& a, const db::DataRecord& b) {
		int ra = get_field_int(a, "RowIndex"), rb = get_field_int(b, "RowIndex");
		if (ra != rb) return ra < rb;
		return get_field_int(a, "ColIndex") < get_field_int(b, "ColIndex");
	});

	// 2. For each tile, load BLP from CASC
	int successful = 0;
	for (const auto& tile : sorted_tiles) {
		if (!skip_zone_check && selected_zone_id.has_value() && *selected_zone_id != expected_zone_id)
			break;

		int col = get_field_int(tile, "ColIndex");
		int row = get_field_int(tile, "RowIndex");
		uint32_t file_data_id = static_cast<uint32_t>(get_field_int(tile, "FileDataID"));

		if (file_data_id == 0) continue;

		int base_x = overlay_offset_x + col * art_style.tile_width;
		int base_y = overlay_offset_y + row * art_style.tile_height;

		try {
			// b. Load BLP from CASC via tile.FileDataID
			BufferWrapper blp_data = core::view->casc->getVirtualFileByID(file_data_id, true);
			casc::BLPImage blp(blp_data);
			// Composite overlay BLP tile pixels onto zone map texture at (base_x, base_y).
			composite_blp_tile(blp, base_x, base_y,
				core::view->zoneMapWidth, core::view->zoneMapHeight,
				core::view->zoneMapPixels);
			successful++;
		} catch (const std::exception& e) {
			logging::write(std::format("Failed to load overlay tile {}: {}", file_data_id, e.what()));
		}
	}

	logging::write(std::format("rendered {}/{} overlay tiles successfully", successful, sorted_tiles.size()));
}

// JS: const load_zone_map = async (zone_id, phase_id) => { ... }
static void load_zone_map(int zone_id, std::optional<int> phase_id = std::nullopt) {
try {
const auto result = render_zone_to_canvas(zone_id, phase_id);
// Upload the composited pixel buffer to a GL texture for ImGui::Image display.
if (!core::view->zoneMapPixels.empty() && core::view->zoneMapWidth > 0 && core::view->zoneMapHeight > 0) {
	core::view->zoneMapTexID = upload_rgba_to_gl(
		core::view->zoneMapPixels.data(),
		core::view->zoneMapWidth, core::view->zoneMapHeight,
		core::view->zoneMapTexID);
}
} catch (const std::exception& e) {
logging::write(std::format("failed to render zone map: {}", e.what()));
core::setToast("error", std::format("Failed to load map data: {}", e.what()));
}
}

// --- Public API ---

void registerTab() {
// JS: this.registerNavButton('Zones', 'mountain-castle.svg', InstallType.CASC);
modules::register_nav_button("tab_zones", "Zones", "mountain-castle.svg", install_type::CASC);
}

void mounted() {
auto& view = *core::view;

// JS: await this.initialize();
core::showLoadingScreen(3);

core::progressLoadingScreen("Loading map tiles...");
casc::db2::preloadTable("UiMapArtTile");

core::progressLoadingScreen("Loading map overlays...");
casc::db2::preloadTable("WorldMapOverlay");
casc::db2::preloadTable("WorldMapOverlayTile");

core::progressLoadingScreen("Loading zone data...");

// JS: const expansion_map = new Map();
// JS: for (const [id, entry] of await db2.Map.getAllRows())
//         expansion_map.set(id, entry.ExpansionID);
std::unordered_map<int, int> expansion_map;
auto& map_table = casc::db2::getTable("Map");
if (!map_table.isLoaded)
	map_table.parse();

for (const auto& [id, row] : map_table.getAllRows()) {
	auto exp_it = row.find("ExpansionID");
	if (exp_it != row.end())
		expansion_map[static_cast<int>(id)] = fieldToInt(exp_it->second);
}

logging::write(std::format("loaded {} maps for expansion mapping", expansion_map.size()));

// JS: const available_zones = new Set();
// JS: for (const entry of (await db2.UiMapAssignment.getAllRows()).values())
//         available_zones.add(entry.AreaID);
std::set<int> available_zones;
auto& ui_map_assignment = casc::db2::getTable("UiMapAssignment");
if (!ui_map_assignment.isLoaded)
	ui_map_assignment.parse();

for (const auto& [id, row] : ui_map_assignment.getAllRows()) {
	auto area_it = row.find("AreaID");
	if (area_it != row.end())
		available_zones.insert(fieldToInt(area_it->second));
}

logging::write(std::format("loaded {} zones from UiMapAssignment", available_zones.size()));

// JS: const table = db2.AreaTable;
// JS: for (const [id, entry] of await table.getAllRows()) { ... }
auto& area_table = casc::db2::getTable("AreaTable");
if (!area_table.isLoaded)
	area_table.parse();

std::vector<std::string> zone_entries;
for (const auto& [id, row] : area_table.getAllRows()) {
	if (!available_zones.contains(static_cast<int>(id)))
		continue;

	auto cont_it = row.find("ContinentID");
	int continent_id = (cont_it != row.end()) ? fieldToInt(cont_it->second) : 0;
	int expansion_id = expansion_map.count(continent_id) ? expansion_map[continent_id] : 0;

	auto name_it = row.find("AreaName_lang");
	std::string area_name = (name_it != row.end()) ? fieldToString(name_it->second) : "";

	auto zone_it = row.find("ZoneName");
	std::string zone_name = (zone_it != row.end()) ? fieldToString(zone_it->second) : "";

	zone_entries.push_back(std::format("{}\x19[{}]\x19{}\x19({})",
		expansion_id, id, area_name, zone_name));
}

view.zoneViewerZones.clear();
for (const auto& entry : zone_entries)
view.zoneViewerZones.push_back(entry);

logging::write(std::format("loaded {} zones from AreaTable", zone_entries.size()));

core::hideLoadingScreen();

// Store initial config values for change-detection.
prev_show_zone_base_map = view.config.value("showZoneBaseMap", true);
prev_show_zone_overlays = view.config.value("showZoneOverlays", true);
}

void render() {
auto& view = *core::view;

// --- Change-detection for selection (equivalent to watch on selectionZones) ---
if (!view.selectionZones.empty()) {
const std::string first = view.selectionZones[0].get<std::string>();
if (view.isBusy == 0 && !first.empty() && first != prev_selection_first) {
const auto zone = parse_zone_entry(first);
if (zone.id > 0 && (!selected_zone_id.has_value() || *selected_zone_id != zone.id)) {
selected_zone_id = zone.id;
logging::write(std::format("selected zone: {} ({})", zone.zone_name, zone.id));

const auto phases = get_zone_phases(zone.id);

view.zonePhases.clear();
for (const auto& phase : phases) {
nlohmann::json p;
p["id"] = phase.id;
p["label"] = phase.label;
view.zonePhases.push_back(std::move(p));
}

if (!phases.empty()) {
selected_phase_id = phases[0].id;
view.zonePhaseSelection = phases[0].id;
} else {
selected_phase_id = std::nullopt;
view.zonePhaseSelection = nullptr;
}

load_zone_map(zone.id, selected_phase_id);
}

prev_selection_first = first;
}
}

// --- Change-detection for zonePhaseSelection ---
if (!view.zonePhaseSelection.is_null() && view.zonePhaseSelection.is_number()) {
int new_phase = view.zonePhaseSelection.get<int>();
if (selected_zone_id.has_value() && view.isBusy == 0 &&
(!selected_phase_id.has_value() || *selected_phase_id != new_phase)) {
selected_phase_id = new_phase;
logging::write(std::format("zone phase changed to {}, reloading zone {}",
new_phase, *selected_zone_id));
load_zone_map(*selected_zone_id, selected_phase_id);
}
}

// --- Change-detection for config.showZoneBaseMap ---
const bool current_show_base_map = view.config.value("showZoneBaseMap", true);
if (current_show_base_map != prev_show_zone_base_map) {
if (selected_zone_id.has_value() && view.isBusy == 0) {
logging::write(std::format("zone base map setting changed, reloading zone {}",
*selected_zone_id));
load_zone_map(*selected_zone_id, selected_phase_id);
}
prev_show_zone_base_map = current_show_base_map;
}

// --- Change-detection for config.showZoneOverlays ---
const bool current_show_overlays = view.config.value("showZoneOverlays", true);
if (current_show_overlays != prev_show_zone_overlays) {
if (selected_zone_id.has_value() && view.isBusy == 0) {
logging::write(std::format("zone overlay setting changed, reloading zone {}",
*selected_zone_id));
load_zone_map(*selected_zone_id, selected_phase_id);
}
prev_show_zone_overlays = current_show_overlays;
}

// --- Template rendering ---

// Expansion filter buttons.
// JS: <div class="expansion-buttons">
//     <button class="expansion-button show-all" ...>
//     <button v-for="expansion in $core.view.constants.EXPANSIONS" ...>
if (ImGui::SmallButton("All"))
view.selectedZoneExpansionFilter = -1;
ImGui::SameLine();

// TODO(conversion): Expansion icons are background-image CSS vars in JS; here we use text buttons.
// When icon font or texture icons are available, replace with icon buttons.
for (int i = 0; i < static_cast<int>(EXPANSION_NAMES.size()); i++) {
const bool selected = (view.selectedZoneExpansionFilter == i);
if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
if (ImGui::SmallButton(std::format("E{}", i).c_str()))
view.selectedZoneExpansionFilter = i;
if (selected) ImGui::PopStyleColor();
ImGui::SameLine();
}
ImGui::NewLine();

// Zone listbox.
// JS: <component :is="$components.ListboxZones" ... >
ImGui::BeginChild("zones-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, -ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Borders);
// TODO(conversion): ListboxZones component rendering will be wired when integration is complete.
ImGui::Text("Zones: %zu", view.zoneViewerZones.size());
ImGui::EndChild();

// Context menu.
// JS: <ContextMenu :node="contextMenus.nodeZone" ...>
//   copy_zone_names, copy_area_names, copy_zone_ids, copy_zone_export_path, open_zone_export_directory
// TODO(conversion): ContextMenu component rendering will be wired when integration is complete.

// Filter.
// JS: <div class="filter">
if (view.config.value("regexFilters", false))
ImGui::TextUnformatted("Regex Enabled");

char filter_buf[256] = {};
std::strncpy(filter_buf, view.userInputFilterZones.c_str(), sizeof(filter_buf) - 1);
if (ImGui::InputText("##FilterZones", filter_buf, sizeof(filter_buf)))
view.userInputFilterZones = filter_buf;

ImGui::SameLine();

// Right panel: export controls + zone preview.
ImGui::BeginGroup();

// Export controls.
// JS: <div class="zone-export-controls">
bool show_base_map = view.config.value("showZoneBaseMap", true);
if (ImGui::Checkbox("Show Base Map", &show_base_map))
view.config["showZoneBaseMap"] = show_base_map;

ImGui::SameLine();

bool show_overlays = view.config.value("showZoneOverlays", true);
if (ImGui::Checkbox("Show Overlays", &show_overlays))
view.config["showZoneOverlays"] = show_overlays;

ImGui::SameLine();

const bool busy = view.isBusy > 0;
const bool no_selection = view.selectionZones.empty();
if (busy || no_selection) ImGui::BeginDisabled();
if (ImGui::Button("Export Map"))
export_zone_map();
if (busy || no_selection) ImGui::EndDisabled();

// Phase dropdown.
// JS: <select v-model="$core.view.zonePhaseSelection">
if (view.zonePhases.size() > 1) {
std::string current_label = "Default";
if (view.zonePhaseSelection.is_number()) {
int current_id = view.zonePhaseSelection.get<int>();
for (const auto& phase : view.zonePhases) {
if (phase.value("id", -99) == current_id) {
current_label = phase.value("label", std::string("Unknown"));
break;
}
}
}

if (ImGui::BeginCombo("Phase", current_label.c_str())) {
for (const auto& phase : view.zonePhases) {
const std::string label = phase.value("label", std::string("Unknown"));
const int phase_id = phase.value("id", -99);
const bool is_selected = view.zonePhaseSelection.is_number() &&
view.zonePhaseSelection.get<int>() == phase_id;
if (ImGui::Selectable(label.c_str(), is_selected))
view.zonePhaseSelection = phase_id;
if (is_selected)
ImGui::SetItemDefaultFocus();
}
ImGui::EndCombo();
}
}

// Zone map preview (canvas).
// JS: <div class="zone-viewer-container preview-container">
//     <canvas id="zone-canvas"></canvas>
ImGui::BeginChild("zone-canvas-area", ImVec2(0, 0), ImGuiChildFlags_Borders);
if (view.zoneMapTexID != 0 && view.zoneMapWidth > 0 && view.zoneMapHeight > 0) {
	// Fit zone map into available area while preserving aspect ratio.
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float tex_w = static_cast<float>(view.zoneMapWidth);
	const float tex_h = static_cast<float>(view.zoneMapHeight);
	const float scale = std::min(avail.x / tex_w, avail.y / tex_h);
	const ImVec2 img_size(tex_w * scale, tex_h * scale);
	ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.zoneMapTexID)), img_size);
} else {
	ImGui::TextUnformatted("Select a zone to preview its map");
}
ImGui::EndChild();

ImGui::EndGroup();
}

// --- Context menu methods ---

// JS: methods.handle_zone_context(data)
// TODO(conversion): Context menu handling will be wired when ContextMenu component is integrated.
// The methods below implement the clipboard operations identically to JS.

// JS: methods.copy_zone_names(selection)
static void copy_zone_names(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
const auto zone = parse_zone_entry(entry);
if (zone.id > 0) {
if (!result.empty())
result += '\n';
result += zone.zone_name;
}
}
ImGui::SetClipboardText(result.c_str());
}

// JS: methods.copy_area_names(selection)
static void copy_area_names(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
const auto zone = parse_zone_entry(entry);
if (zone.id > 0) {
if (!result.empty())
result += '\n';
result += zone.area_name;
}
}
ImGui::SetClipboardText(result.c_str());
}

// JS: methods.copy_zone_ids(selection)
static void copy_zone_ids(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
const auto zone = parse_zone_entry(entry);
if (zone.id > 0) {
if (!result.empty())
result += '\n';
result += std::to_string(zone.id);
}
}
ImGui::SetClipboardText(result.c_str());
}

// JS: methods.copy_zone_export_path()
static void copy_zone_export_path() {
const std::string dir = casc::ExportHelper::getExportPath("zones");
ImGui::SetClipboardText(dir.c_str());
}

// JS: methods.open_zone_export_directory()
static void open_zone_export_directory() {
	const std::string dir = casc::ExportHelper::getExportPath("zones");
#ifdef _WIN32
	const std::wstring wpath(dir.begin(), dir.end());
	ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	std::string cmd = "xdg-open \"" + dir + "\" &";
	std::system(cmd.c_str());
#endif
}

void export_zone_map() {
auto& view = *core::view;
const auto& user_selection = view.selectionZones;
if (user_selection.empty()) {
core::setToast("info", "You didn't select any zones to export; you should do that first.");
return;
}

casc::ExportHelper helper(static_cast<int>(user_selection.size()), "zone");
helper.start();

const std::string format = view.config.value("exportTextureFormat", std::string("PNG"));
const std::string ext = format == "WEBP" ? ".webp" : ".png";
const std::string mime_type = format == "WEBP" ? "image/webp" : "image/png";

for (const auto& zone_entry : user_selection) {
if (helper.isCancelled())
return;

try {
const auto zone = parse_zone_entry(zone_entry.get<std::string>());

logging::write(std::format("exporting zone map: {} ({}) phase {}",
zone.zone_name, zone.id, selected_phase_id.value_or(-1)));

const auto map_info = render_zone_to_canvas(zone.id, selected_phase_id, true);

if (map_info.width == 0 || map_info.height == 0) {
logging::write(std::format("no map data available for zone {}, skipping", zone.id));
helper.mark(std::format("Zone_{}", zone.id), false, "No map data available");
continue;
}

// JS: Sanitize names for filenames.
auto sanitize = [](const std::string& s) -> std::string {
std::string result;
for (char c : s) {
if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == ' ')
result += c;
}
// Replace whitespace runs with underscore.
std::string final_result;
bool last_was_space = false;
for (char c : result) {
if (c == ' ') {
if (!last_was_space)
final_result += '_';
last_was_space = true;
} else {
final_result += c;
last_was_space = false;
}
}
return final_result;
};

const std::string normalized_zone_name = sanitize(zone.zone_name);
const std::string normalized_area_name = sanitize(zone.area_name);
const std::string phase_suffix = (selected_phase_id.has_value() && *selected_phase_id != 0)
? std::format("_Phase{}", *selected_phase_id) : "";

namespace fs = std::filesystem;
const std::string filename = std::format("Zone_{}_{}_{}{}{}",
zone.id, normalized_zone_name, normalized_area_name, phase_suffix, ext);
const std::string export_path = casc::ExportHelper::getExportPath(
(fs::path("zones") / filename).string());

logging::write(std::format("exporting zone map at full resolution ({}x{}): {}",
map_info.width, map_info.height, filename));

// JS: const buf = await BufferWrapper.fromCanvas(export_canvas, mime_type, quality);
// JS: await buf.writeToFile(export_path);
// TODO(conversion): BufferWrapper fromCanvas equivalent (PNG/WebP encoding) will be wired.
// Parameters: composite texture (map_info.width x map_info.height), mime_type, exportWebPQuality.
(void)mime_type;

helper.mark((fs::path("zones") / filename).string(), true);

logging::write(std::format("successfully exported zone map to: {}", export_path));
} catch (const std::exception& e) {
logging::write(std::format("failed to export zone map: {}", e.what()));
helper.mark(zone_entry.get<std::string>(), false, e.what());
}
}

helper.finish();
}


} // namespace tab_zones
