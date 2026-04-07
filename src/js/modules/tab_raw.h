/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>

/**
 * Raw client files tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_listbox_context, copy_file_paths, copy_listfile_format,
 * copy_file_data_ids, copy_export_paths, open_export_directory,
 * detect_raw, export_raw), and mounted().
 *
 * Provides: Browse Raw Client Files context menu option, file listbox
 * with context menu, auto-detect selected files, export selected files.
 */
namespace tab_raw {

/**
 * Register the tab (context menu option).
 * JS equivalent: register() { this.registerContextMenuOption(...) }
 */
void registerTab();

/**
 * Initialize the raw tab (compute raw files, set up config watch).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the raw tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Auto-detect file types for selected unknown files.
 * JS equivalent: methods.detect_raw() → detect_raw_files(core)
 */
void detect_raw();

/**
 * Export selected raw files.
 * JS equivalent: methods.export_raw() → export_raw_files(core)
 */
void export_raw();

} // namespace tab_raw
