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
 * Items tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (initialize, view_models, view_textures, copy_to_clipboard,
 * view_on_wowhead, toggle_checklist_item, equip_item),
 * and mounted().
 *
 * Provides: Items nav button (CASC only), item listbox with
 * context menu (view models/textures, copy name/ID, wowhead link),
 * sidebar with item type and quality filter checklists.
 */
namespace tab_items {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Items', 'sword.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the items tab (load items, build filter masks, set up watches).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the items tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Set the items tab as the active tab.
 * JS equivalent: this.setActive() called from other modules.
 */
void setActive();

/**
 * Mark all item types to the given state.
 * JS equivalent: setAllItemTypes(state) on the Vue app.
 */
void setAllItemTypes(bool state);

/**
 * Mark all item qualities to the given state.
 * JS equivalent: setAllItemQualities(state) on the Vue app.
 */
void setAllItemQualities(bool state);

/**
 * Item data accessible to the item picker modal.
 * Mirrors the fields used by item-picker-modal.js's filtered_items computation.
 */
struct ItemData {
	uint32_t id = 0;
	std::string name;
	std::string displayName;
	uint32_t icon = 0;
	int quality = 0;
	int inventoryType = 0;
};

void applySlotFilter(const std::string& filter);

/**
 * Returns all loaded items (unfiltered). Used by item-picker-modal.
 * JS equivalent: DBItemList.getItems() in item-picker-modal.js.
 * Returns nullptr if items have not been loaded yet.
 */
const std::vector<ItemData>* getAllItems();

} // namespace tab_items
