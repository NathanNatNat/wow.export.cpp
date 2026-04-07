/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Text file tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_listbox_context, copy_file_paths, copy_listfile_format,
 * copy_file_data_ids, copy_export_paths, open_export_directory,
 * copy_text, export_text), and mounted().
 *
 * Provides: Text nav button (CASC only), text file listbox with
 * context menu, text preview, copy to clipboard, export.
 */
namespace tab_text {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Text', ...) }
 */
void registerTab();

/**
 * Initialize the text tab (set up selection watch for preview).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the text tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Copy the selected text file content to clipboard.
 * JS equivalent: methods.copy_text()
 */
void copy_text();

/**
 * Export selected text files.
 * JS equivalent: methods.export_text()
 */
void export_text();

} // namespace tab_text
