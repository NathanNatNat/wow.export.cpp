/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Legacy textures tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_listbox_context, copy_file_paths, copy_export_paths,
 * open_export_directory, export_textures), and mounted().
 *
 * Provides: Textures nav button (MPQ only), texture file listbox with
 * context menu, texture preview with channel mask toggle (BLP/PNG/JPG),
 * export in configured format.
 */
namespace legacy_tab_textures {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Textures', 'image.svg', InstallType.MPQ) }
 */
void registerTab();

/**
 * Initialize the legacy textures tab (load file list, selection watch, channel mask watch).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the legacy textures tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export selected textures.
 * JS equivalent: methods.export_textures()
 */
void export_textures();

} // namespace legacy_tab_textures
