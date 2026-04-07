/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

/**
 * Zones tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_zone_context, copy_zone_names, copy_area_names, copy_zone_ids,
 * copy_zone_export_path, open_zone_export_directory, initialize,
 * export_zone_map), and mounted().
 *
 * Provides: Zones nav button (CASC only), zone listbox with expansion
 * filter buttons, zone map preview (canvas → OpenGL texture), phase
 * selection dropdown, zone map export in PNG/WEBP format.
 */
namespace tab_zones {

/**
 * Parsed zone entry result.
 * JS equivalent: { id, zone_name, area_name }
 */
struct ZoneEntry {
	int id = 0;
	std::string zone_name;
	std::string area_name;
};

/**
 * Zone map render result.
 * JS equivalent: { width, height, ui_map_id }
 */
struct ZoneMapInfo {
	int width = 0;
	int height = 0;
	int ui_map_id = 0;
};

/**
 * Zone phase entry.
 * JS equivalent: { id, label }
 */
struct ZonePhase {
	int id = 0;
	std::string label;
};

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Zones', 'mountain-castle.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the zones tab (preload DB2 tables, build zone list, set up watches).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the zones tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export zone map for selected zone(s).
 * JS equivalent: methods.export_zone_map()
 */
void export_zone_map();

} // namespace tab_zones
