/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

/**
 * Maps tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_map_context, copy_map_names, copy_map_internal_names,
 * copy_map_ids, copy_map_export_paths, open_map_export_directory,
 * load_map, setup_wmo_minimap, export_map_wmo, export_map_wmo_minimap,
 * export_map, export_selected_map, export_selected_map_as_raw,
 * export_selected_map_as_png, export_selected_map_as_heightmaps,
 * initialize), and mounted().
 *
 * Provides: Maps nav button (CASC only), map listbox with expansion
 * filter buttons, map viewer tile grid with selection, WMO minimap
 * support, export in OBJ/PNG/RAW/HEIGHTMAPS format, sidebar with
 * export options (WMO, M2, foliage, liquid, game objects, holes,
 * terrain texture quality, heightmap resolution/bit-depth).
 */
namespace tab_maps {

/**
 * Parsed map entry result.
 * JS equivalent: { id, name, dir }
 */
struct MapEntry {
	int id = 0;
	std::string name;
	std::string dir;
};

/**
 * Height data extracted from a terrain tile.
 * JS: return { heights, resolution, tileX, tileY };
 */
struct HeightData {
	std::vector<float> heights;
	int resolution = 0;
	uint32_t tileX = 0;
	uint32_t tileY = 0;
};

/**
 * WMO minimap tile info (positioned tile for compositing).
 */
struct WMOMinimapTileInfo {
	uint32_t fileDataID = 0;
	int groupNum = 0;
	int blockX = 0;
	int blockY = 0;
	float drawX = 0.0f;
	float drawY = 0.0f;
	float scaleX = 1.0f;
	float scaleY = 1.0f;
	float zOrder = 0.0f;
	float pixelX = 0.0f;
	float pixelY = 0.0f;
	int srcWidth = 256;
	int srcHeight = 256;
};

/**
 * WMO minimap data for the currently loaded map.
 * JS: current_wmo_minimap object.
 */
struct WMOMinimapData {
	uint32_t wmo_id = 0;
	std::vector<WMOMinimapTileInfo> tiles;
	int canvas_width = 0;
	int canvas_height = 0;
	int grid_width = 0;
	int grid_height = 0;
	int grid_size = 0;
	std::vector<int> mask;
	std::map<std::string, std::vector<WMOMinimapTileInfo>> tiles_by_coord;
	int output_tile_size = 256;
};

/**
 * Game object for ADT export.
 * Matches the subset of fields from GameObjects DB2 used in export.
 */
struct ADTGameObject {
	uint32_t FileDataID = 0;
	std::vector<float> Position;
	std::vector<float> Rotation;
	float scale = 1.0f;
};

/**
 * ADT chunk data (used by sample_chunk_height).
 * Maps to the ADTLoader chunk structure.
 */
struct ADTChunk {
	std::vector<float> vertices;
	std::vector<float> position; // [x, y, z]
};

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Maps', 'map.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the maps tab (load DB2 tables, build map list, set up watches).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the maps tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export the global WMO for the selected map.
 * JS equivalent: methods.export_map_wmo()
 */
void export_map_wmo();

/**
 * Export the WMO minimap as a PNG.
 * JS equivalent: methods.export_map_wmo_minimap()
 */
void export_map_wmo_minimap();

/**
 * Export selected map tiles in the current format.
 * JS equivalent: methods.export_map()
 */
void export_map();

} // namespace tab_maps
