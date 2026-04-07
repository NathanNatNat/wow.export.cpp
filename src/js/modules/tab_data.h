/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Data (DB2) tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), data(), template, methods
 * (handle_context_menu, copy_rows_csv, copy_rows_sql, copy_cell,
 * initialize, export_data, export_csv, export_sql, export_db2),
 * and mounted().
 *
 * Provides: Data nav button (CASC only), DB2 table listbox, data table
 * viewer with context menu (CSV/SQL copy), export in CSV/SQL/DB2 format.
 */
namespace tab_data {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Data', 'database.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the data tab (load dbd manifest, set up selection watch).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the data tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Export data in the currently selected format (CSV/SQL/DB2).
 * JS equivalent: methods.export_data()
 */
void export_data();

} // namespace tab_data
