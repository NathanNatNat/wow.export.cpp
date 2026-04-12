/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Legacy home tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with template:
 *   <div class="tab" id="legacy-tab-home">
 *     <HomeShowcase />
 *     <div id="home-changes"><div v-html="$core.view.whatsNewHTML"></div></div>
 *     <div id="home-help-buttons"> ... </div>
 *   </div>
 *
 * Identical to tab_home but with the "legacy-tab-home" CSS ID.
 */
namespace legacy_tab_home {

/**
 * Render the legacy home tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Navigate to a named module.
 * JS equivalent: methods.navigate(module_name)
 * @param module_name Name of the module to activate.
 */
void navigate(const char* module_name);

} // namespace legacy_tab_home
