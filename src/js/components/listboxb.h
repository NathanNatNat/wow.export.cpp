/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <functional>

/**
 * Listbox-B component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['items', 'selection', 'single', 'keyinput', 'disable'],
 * emits: ['update:selection']. A simplified listbox with virtual scrolling,
 * custom scrollbar drag, single/multi-select, keyboard navigation, and
 * clipboard copy support.
 *
 * Items are objects with a "label" string field.
 */
namespace listboxb {

/**
 * A single item in the listbox.
 */
struct ListboxBItem {
	std::string label;
};

/**
 * Persistent state for a single ListboxB widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct ListboxBState {
	float scroll = 0.0f;
	float scrollRel = 0.0f;
	bool isScrolling = false;
	int slotCount = 1;
	int lastSelectItem = -1;  // Index into items; -1 = null

	// Mouse drag tracking (equivalent to JS instance vars set in startMouse).
	float scrollStartY = 0.0f;
	float scrollStart = 0.0f;
};

/**
 * Render a listbox-B using ImGui.
 *
 * @param id              Unique ImGui ID string for this widget instance.
 * @param items           Array of item entries displayed in the list.
 * @param selection       Currently selected item indices (into items array).
 * @param single          If true, only one entry can be selected.
 * @param keyinput        If true, listbox registers for keyboard input.
 * @param disable         If true, disables selection changes.
 * @param state           Persistent state across frames.
 * @param onSelectionChanged  Callback when selection changes; receives new selection indices.
 */
void render(const char* id, const std::vector<ListboxBItem>& items,
            const std::vector<int>& selection, bool single, bool keyinput, bool disable,
            ListboxBState& state,
            const std::function<void(const std::vector<int>&)>& onSelectionChanged);

} // namespace listboxb
