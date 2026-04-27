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

class M2RendererGL;
class M3RendererGL;
class WMORendererGL;

/**
 * Decor tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (initialize, handle_listbox_context, copy_decor_names,
 * copy_file_data_ids, preview_texture, export_ribbon_texture,
 * toggle_uv_layer, export_decor, toggle_checklist_item,
 * toggle_category_group, toggle_animation_pause, step_animation,
 * seek_animation, start_scrub, end_scrub),
 * and mounted().
 *
 * Also exposes: getActiveRenderer(), preview_decor(), export_files().
 *
 * Provides: Decor nav button (CASC only), decor listbox with
 * context menu (copy names/IDs), 3D model preview with texture ribbon,
 * animation controls, category sidebar with grouped checklist,
 * export in OBJ/STL/GLTF/GLB/PNG/CLIPBOARD format.
 */
namespace tab_decor {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Decor', 'house.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the decor tab (load decor data, build category masks, set up watches).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the decor tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Get the active model renderer (if any).
 * JS equivalent: getActiveRenderer: () => active_renderer
 * @returns Active renderer pointer variant (M2/M3/WMO), or std::monostate when none.
 */
std::variant<std::monostate, M2RendererGL*, M3RendererGL*, WMORendererGL*> getActiveRenderer();

} // namespace tab_decor
