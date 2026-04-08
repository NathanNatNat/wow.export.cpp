/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Legacy files tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_listbox_context, copy_file_paths, copy_export_paths,
 * open_export_directory, export_selected),
 * and mounted().
 *
 * Provides: Files nav button (MPQ only), file listbox with
 * context menu, raw file export.
 */
namespace legacy_tab_files {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Files', 'file-lines.svg', InstallType.MPQ) }
 */
void registerTab();

/**
 * Initialize the legacy files tab (load file list from MPQ).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the legacy files tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export selected raw files.
 * JS equivalent: methods.export_selected() → export_files(core)
 */
void export_selected();

} // namespace legacy_tab_files
