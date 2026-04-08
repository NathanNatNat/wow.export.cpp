/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Legacy fonts tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_listbox_context, copy_file_paths, copy_export_paths,
 * open_export_directory, export_fonts),
 * and mounted().
 *
 * Provides: Fonts nav button (MPQ only), font listbox with
 * context menu, font preview (glyph grid + text input), export.
 */
namespace legacy_tab_fonts {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Fonts', 'font.svg', InstallType.MPQ) }
 */
void registerTab();

/**
 * Initialize the legacy fonts tab (load font list, set up preview).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the legacy fonts tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export selected font files.
 * JS equivalent: methods.export_fonts()
 */
void export_fonts();

} // namespace legacy_tab_fonts
