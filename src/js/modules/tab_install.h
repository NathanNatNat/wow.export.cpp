/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Install manifest tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods (export_install,
 * view_strings, export_strings, back_to_manifest), and mounted().
 *
 * Provides: Browse Install Manifest context menu option, install file listbox
 * with tag-based filtering sidebar, string viewer for binary files, string export.
 */
namespace tab_install {

/**
 * Register the tab (context menu option).
 * JS equivalent: register() { this.registerContextMenuOption(...) }
 */
void registerTab();

/**
 * Initialize the install tab (load manifest, set up tag watches).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the install tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export selected install files.
 * JS equivalent: methods.export_install() → export_install_files(core)
 */
void export_install();

/**
 * View strings extracted from a selected binary file.
 * JS equivalent: methods.view_strings() → view_strings(core)
 */
void view_strings();

/**
 * Export extracted strings to a text file.
 * JS equivalent: methods.export_strings() → export_strings(core)
 */
void export_strings();

/**
 * Return from string view to manifest view.
 * JS equivalent: methods.back_to_manifest() → back_to_manifest(core)
 */
void back_to_manifest();

} // namespace tab_install
