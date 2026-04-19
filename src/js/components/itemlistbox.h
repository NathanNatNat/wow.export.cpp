/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

/**
 * Item listbox component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['items', 'filter', 'selection', 'single',
 * 'keyinput', 'regex', 'includefilecount', 'unittype'],
 * emits: ['update:selection', 'equip', 'options'].
 *
 * A listbox for item entries with icon rendering, quality coloring,
 * equip/options buttons, virtual scrolling, custom scrollbar drag,
 * single/multi-select, and keyboard navigation.
 *
 * Selection model: The JS source stores item object references in the selection
 * array. The C++ equivalent stores item IDs (ItemEntry::id) which provide
 * the same stable identity semantics across filter changes.
 */
namespace itemlistbox {

/**
 * A single item entry in the item listbox.
 * JS equivalent: items array entries with { id, name, displayName, icon, quality }.
 */
struct ItemEntry {
	int id = 0;
	std::string name;
	std::string displayName;
	uint32_t icon = 0;       // File data ID for the icon BLP.
	int quality = 0;         // Item quality (0-7+ for color coding).
};

/**
 * Persistent state for a single ItemListbox widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct ItemListboxState {
	float scroll = 0.0f;
	float scrollRel = 0.0f;
	bool isScrolling = false;
	int slotCount = 1;
	int lastSelectItem = -1;  // Item ID of last selected item; -1 = null (JS: lastSelectItem object)

	// Mouse drag tracking (equivalent to JS instance vars set in startMouse).
	float scrollStartY = 0.0f;
	float scrollStart = 0.0f;

	// Cached filtered items — rebuilt only when inputs change, not every frame.
	std::vector<ItemEntry> cachedFilteredItems;
	const ItemEntry* cachedItemsData      = nullptr;
	size_t           cachedItemsSize      = ~size_t(0);
	std::string      cachedFilter;
	bool             cachedRegexMode      = false;
	bool             filteredItemsCacheValid = false;
};

/**
 * Render an item listbox using ImGui.
 *
 * @param id                   Unique ImGui ID string for this widget instance.
 * @param items                Array of item entries displayed in the list.
 * @param filter               Current text filter value (empty for no filter).
 * @param selection            Currently selected item IDs (ItemEntry::id values).
 *                             JS equivalent: array of item object references.
 * @param single               If true, only one entry can be selected.
 * @param keyinput             If true, listbox registers for keyboard input.
 * @param regex                If true, filter is treated as a regular expression.
 * @param unittype             Unit name for file counter (empty to hide counter).
 * @param state                Persistent state across frames.
 * @param onSelectionChanged   Callback when selection changes; receives new selected item IDs.
 * @param onEquip              Callback when "Equip" button is clicked; receives the item.
 * @param onOptions            Callback when "Options" button is clicked; receives the item.
 */
void render(const char* id,
            const std::vector<ItemEntry>& items,
            const std::string& filter,
            const std::vector<int>& selection,
            bool single,
            bool keyinput,
            bool regex,
            const std::string& unittype,
            ItemListboxState& state,
            const std::function<void(const std::vector<int>&)>& onSelectionChanged,
            const std::function<void(const ItemEntry&)>& onEquip,
            const std::function<void(const ItemEntry&)>& onOptions);

} // namespace itemlistbox
