/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * Item Sets tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (initialize, equip_set), and mounted().
 *
 * Provides: Item Sets nav button (CASC only), item set listbox
 * with equip action, set filtering.
 */
namespace tab_item_sets {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Item Sets', 'armour.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the item sets tab (load item sets, apply filter).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the item sets tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

} // namespace tab_item_sets
