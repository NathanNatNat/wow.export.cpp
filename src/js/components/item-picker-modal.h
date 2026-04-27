/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Item picker modal component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: src/js/components/item-picker-modal.js
 *
 * A modal dialog that lets the user search and pick an item to equip.
 * Filters the item list by slot type, supports text search, and calls
 * wow::equip_item() on selection.
 *
 * Usage:
 *   Call item_picker_modal::open(slot_id, slot_filter) to open.
 *   Call item_picker_modal::render() once per frame inside the ImGui frame.
 *   The modal closes itself on item selection or Cancel/Escape.
 */
namespace item_picker_modal {

/**
 * Open the item picker modal for a given equipment slot.
 *
 * @param slot_id     Equipment slot ID (JS: slot_id prop).
 * @param slot_filter Label for type pre-filtering (JS: slot_filter prop, e.g. "Shoulder").
 */
void open(int slot_id, const std::string& slot_filter = "");

/**
 * Render the item picker modal. Must be called every frame.
 * Emits @close internally and returns true if the modal is still open.
 */
void render();

/**
 * Returns true if the modal is currently open.
 */
bool is_open();

} // namespace item_picker_modal
