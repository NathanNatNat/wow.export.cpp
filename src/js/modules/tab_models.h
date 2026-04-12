/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class M2RendererGL;
class M3RendererGL;
class WMORendererGL;

/**
 * Models tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (initialize, handle_listbox_context, copy_file_paths,
 * copy_listfile_format, copy_file_data_ids, copy_export_paths,
 * open_export_directory, preview_texture, export_ribbon_texture,
 * toggle_uv_layer, export_model, toggle_animation_pause,
 * step_animation, seek_animation, start_scrub, end_scrub),
 * and mounted().
 *
 * Also exposes: getActiveRenderer(), preview_model(), export_files().
 *
 * Provides: Models nav button (CASC only), model listbox with
 * context menu (copy paths/IDs), 3D model preview with texture ribbon,
 * texture preview with UV layers, animation controls with scrubber,
 * sidebar with preview/export checkboxes, geoset list, skin list,
 * WMO groups and doodad sets.
 * Export in OBJ/STL/GLTF/GLB/RAW/PNG/CLIPBOARD format.
 */
namespace tab_models {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Models', 'cube.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the models tab (load model file data, DB caches, set up watches).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the models tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export model files.
 * JS equivalent: export_files(core, files, is_local, export_id)
 * @param files       List of file entries (JSON strings or numbers).
 * @param is_local    If true, files are local filesystem paths.
 * @param export_id   Export ID for tracking (default -1).
 */
void export_files(const std::vector<nlohmann::json>& files, bool is_local = false, int export_id = -1);

/**
 * Get the active model renderer (if any).
 * JS equivalent: getActiveRenderer: () => active_renderer
 * @returns Pointer to active M2RendererGL, or nullptr.
 *
 * Note: The active renderer may also be M3 or WMO.
 * This returns the M2 renderer specifically; use getActiveRendererType()
 * to determine the actual type.
 */
M2RendererGL* getActiveRenderer();

} // namespace tab_models
