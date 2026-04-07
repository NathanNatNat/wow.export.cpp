/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Fonts tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_listbox_context, copy_file_paths, copy_listfile_format,
 * copy_file_data_ids, copy_export_paths, open_export_directory,
 * export_fonts), and mounted().
 *
 * Provides: Fonts nav button (CASC only), font listbox with context menu,
 * font preview (glyph grid + text input), export.
 */
namespace tab_fonts {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Fonts', ...) }
 */
void registerTab();

/**
 * Initialize the fonts tab (set up preview, selection watch).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the fonts tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export selected font files.
 * JS equivalent: methods.export_fonts()
 */
void export_fonts();

} // namespace tab_fonts
