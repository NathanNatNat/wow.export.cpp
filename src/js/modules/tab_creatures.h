/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Forward declarations.
class M2RendererGL;

/**
 * Creatures tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (initialize, handle_listbox_context, copy_creature_names,
 * copy_creature_ids, preview_texture, export_ribbon_texture,
 * toggle_uv_layer, export_creatures, toggle_animation_pause,
 * step_animation, seek_animation, start_scrub, end_scrub),
 * and mounted().
 *
 * Also exposes: getActiveRenderer(), preview_creature(), export_files().
 *
 * Provides: Creatures nav button (CASC only), creature listbox with
 * context menu (copy names/IDs), 3D model preview with texture ribbon,
 * texture preview with UV layers, animation controls with scrubber,
 * sidebar with preview/export checkboxes, equipment checklist, geoset list,
 * skin list, WMO groups and doodad sets.
 * Export in OBJ/STL/GLTF/GLB/RAW/PNG/CLIPBOARD format.
 */
namespace tab_creatures {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Creatures', 'nessy.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the creatures tab (load DB caches, build creature list, set up watches).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the creatures tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Get the active model renderer (if any).
 * JS equivalent: getActiveRenderer: () => active_renderer
 * @returns Pointer to active M2RendererGL, or nullptr.
 */
M2RendererGL* getActiveRenderer();

} // namespace tab_creatures
