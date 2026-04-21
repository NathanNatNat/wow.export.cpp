/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

class M2LegacyRendererGL;
class WMOLegacyRendererGL;
class MDXRendererGL;

/**
 * Legacy Models tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_listbox_context, copy_file_paths, copy_export_paths,
 * open_export_directory, export_model, toggle_animation_pause,
 * step_animation, seek_animation, start_scrub, end_scrub),
 * and mounted().
 *
 * Also exposes: getActiveRenderer().
 *
 * Provides: Models nav button (MPQ only), model listbox with
 * context menu (copy paths, open export dir), 3D model preview
 * with texture ribbon, animation controls (disabled in JS),
 * sidebar with preview checkboxes, skins, geosets,
 * WMO groups and doodad sets.
 * Export in OBJ/STL/RAW/PNG/CLIPBOARD format.
 */
namespace tab_models_legacy {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Models', 'cube.svg', InstallType.MPQ) }
 */
void registerTab();

/**
 * Initialize the legacy models tab (build file list, creature data, viewer context, watches).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the legacy models tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export legacy model files.
 * JS equivalent: export_files(core, files, export_id)
 * @param files       List of file entry strings.
 * @param export_id   Export ID for tracking (default -1).
 */
void export_files(const std::vector<nlohmann::json>& files, int export_id = -1);

/**
 * Get the active renderer (if any).
 * JS equivalent: getActiveRenderer: () => active_renderer
 * @returns Pointer to active M2LegacyRendererGL, or nullptr.
 *
 * Note: The active renderer may also be WMOLegacy or MDX.
 * This returns a generic void pointer; callers need
 * legacyModelViewerActiveType to determine actual type.
 */
std::variant<std::monostate, M2LegacyRendererGL*, MDXRendererGL*, WMOLegacyRendererGL*> getActiveRenderer();

} // namespace tab_models_legacy
