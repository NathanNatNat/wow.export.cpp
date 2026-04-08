/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Legacy data (DBC) tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), data(), template, methods
 * (handle_context_menu, copy_rows_csv, copy_rows_sql, copy_cell,
 * export_data, export_csv, export_sql, export_dbc),
 * and mounted().
 *
 * Provides: Data nav button (MPQ only), DBC table listbox, data table
 * viewer with context menu (CSV/SQL copy), export in CSV/SQL/DBC format.
 */
namespace legacy_tab_data {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Data', 'database.svg', InstallType.MPQ) }
 */
void registerTab();

/**
 * Initialize the legacy data tab (scan DBC files, set up selection watch).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the legacy data tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export data in the currently selected format (CSV/SQL/DBC).
 * JS equivalent: methods.export_data()
 */
void export_data();

} // namespace legacy_tab_data
