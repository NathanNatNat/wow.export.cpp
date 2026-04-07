/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Legacy home tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with template showing whatsNewHTML,
 * Discord/GitHub/Patreon help buttons, and a navigate() method.
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
