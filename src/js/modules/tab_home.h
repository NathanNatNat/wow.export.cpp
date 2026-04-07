/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Home tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with template showing whatsNewHTML,
 * Discord/GitHub/Patreon help buttons, and a navigate() method.
 *
 * The template renders:
 *   - #home-changes: the "What's New" HTML content from core::view->whatsNewHTML
 *   - #home-help-buttons: three data-external links (Discord, GitHub, Patreon)
 */
namespace tab_home {

/**
 * Render the home tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Navigate to a named module.
 * JS equivalent: methods.navigate(module_name)
 * @param module_name Name of the module to activate.
 */
void navigate(const char* module_name);

} // namespace tab_home
