/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Home tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with template:
 *   <div class="tab" id="tab-home">
 *     <HomeShowcase />
 *     <div id="home-changes"><div v-html="$core.view.whatsNewHTML"></div></div>
 *     <div id="home-help-buttons"> ... </div>
 *   </div>
 *
 * Layout: 2-column grid (1fr 1fr), 4 rows (auto 1fr auto auto), padding 50px.
 *   Left column: HomeShowcase header + showcase area + links
 *   Right column: #home-changes (What's New changelog with background image)
 *   Bottom full width: #home-help-buttons (Discord, GitHub, Patreon)
 *
 * HomeShowcase in JS loads external web images from showcase.json which is
 * not practical in C++/ImGui. Replaced with navigation card shortcuts per
 * CONVERSION_TRACKER.md section 9.14.
 */
namespace tab_home {

/**
 * Render the home tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Render the shared home tab layout (used by both tab_home and legacy_tab_home).
 * CSS: #tab-home, #legacy-tab-home share the same grid layout.
 */
void renderHomeLayout();

/**
 * Navigate to a named module.
 * JS equivalent: methods.navigate(module_name)
 * @param module_name Name of the module to activate.
 */
void navigate(const char* module_name);

} // namespace tab_home
