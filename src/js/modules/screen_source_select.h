/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Source select screen module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with template, data(), methods
 * (get_product_tag, set_selected_cdn, load_install, open_local_install,
 * open_legacy_install, init_cdn_pings, click_source_local,
 * click_source_local_recent, click_source_remote, click_source_legacy,
 * click_source_legacy_recent, click_source_build,
 * click_return_to_source_select), and mounted().
 *
 * Provides: Local/Remote/Legacy installation source selection,
 * CDN region picker with ping, build selector, recent installation
 * history, file/directory dialogs for install paths.
 */
namespace screen_source_select {

/**
 * Initialize the source select screen (init config arrays, file selectors, CDN pings).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the source select screen widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Open a local CASC installation.
 * JS equivalent: methods.open_local_install(install_path, product)
 * @param install_path  Path to the WoW installation directory.
 * @param product       Optional product identifier for auto-selecting build.
 */
void open_local_install(const std::string& install_path, const std::string& product = "");

/**
 * Open a legacy MPQ installation.
 * JS equivalent: methods.open_legacy_install(install_path)
 * @param install_path  Path to the legacy WoW installation directory.
 */
void open_legacy_install(const std::string& install_path);

/**
 * Load a selected build from the active CASC source.
 * JS equivalent: methods.load_install(index)
 * @param index Build index to load.
 */
void load_install(int index);

} // namespace screen_source_select
