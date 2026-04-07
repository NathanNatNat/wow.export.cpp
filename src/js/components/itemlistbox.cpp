/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "itemlistbox.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <regex>
#include <format>

#include "../icon-render.h"

namespace itemlistbox {

/**
 * items: Item entries displayed in the list.
 * filter: Optional reactive filter for items.
 * selection: Reactive selection controller.
 * single: If set, only one entry can be selected.
 * keyinput: If true, listbox registers for keyboard input.
 * regex: If true, filter will be treated as a regular expression.
 * includefilecount: If true, includes a file counter on the component.
 * unittype: Unit name for what the listbox contains. Used with includefilecount.
 */
// props: ['items', 'filter', 'selection', 'single', 'keyinput', 'regex', 'includefilecount', 'unittype']
// emits: ['update:selection', 'equip', 'options']

/**
 * Reactive instance data.
 */
// data: scroll, scrollRel, isScrolling, slotCount, lastSelectItem — stored in ItemListboxState

/**
 * Invoked when the component is mounted.
 * Used to register global listeners and resize observer.
 */
// TODO(conversion): In ImGui, global mouse listeners and ResizeObserver are not needed.
// ImGui provides mouse state via ImGui::GetIO() and resizing is handled by layout each frame.

/**
 * Invoked when the component is destroyed.
 * Used to unregister global mouse listeners and resize observer.
 */
// TODO(conversion): No explicit unmount needed in ImGui immediate mode.

/**
 * Offset of the scroll widget in pixels.
 * Between 0 and the height of the component.
 */
static float scrollOffset(const ItemListboxState& state) {
	return state.scroll;
}

/**
 * Index which array reading should start at, based on the current
 * relative scroll and the overal item count. Value is dynamically
 * capped based on slot count to prevent empty slots appearing.
 */
static int scrollIndex(const std::vector<ItemEntry>& filteredItems, const ItemListboxState& state) {
	return static_cast<int>(std::round((static_cast<float>(filteredItems.size()) - static_cast<float>(state.slotCount)) * state.scrollRel));
}

/**
 * Weight (0-1) of a single item.
 */
static float itemWeight(const std::vector<ItemEntry>& filteredItems) {
	if (filteredItems.empty())
		return 0.0f;
	return 1.0f / static_cast<float>(filteredItems.size());
}

/**
 * Invoked by a ResizeObserver when the main component node
 * is resized due to layout changes.
 */
static void resize(float containerHeight, float scrollerHeight, ItemListboxState& state) {
	state.scroll = (containerHeight - scrollerHeight) * state.scrollRel;
	state.slotCount = static_cast<int>(std::floor(containerHeight / 26.0f));
}

/**
 * Restricts the scroll offset to prevent overflowing and
 * calculates the relative (0-1) offset based on the scroll.
 */
static void recalculateBounds(float containerHeight, float scrollerHeight, ItemListboxState& state) {
	const float max = containerHeight - scrollerHeight;
	state.scroll = std::min(max, std::max(0.0f, state.scroll));
	state.scrollRel = (max > 0.0f) ? (state.scroll / max) : 0.0f;
}

/**
 * Invoked when a mouse-down event is captured on the scroll widget.
 * @param {MouseEvent} e
 */
static void startMouse(float mouseY, ItemListboxState& state) {
	state.scrollStartY = mouseY;
	state.scrollStart = state.scroll;
	state.isScrolling = true;
}

/**
 * Invoked when a mouse-move event is captured globally.
 * @param {MouseEvent} e
 */
static void moveMouse(float mouseY, float containerHeight, float scrollerHeight, ItemListboxState& state) {
	if (state.isScrolling) {
		state.scroll = state.scrollStart + (mouseY - state.scrollStartY);
		recalculateBounds(containerHeight, scrollerHeight, state);
	}
}

/**
 * Invoked when a mouse-up event is captured globally.
 */
static void stopMouse(ItemListboxState& state) {
	state.isScrolling = false;
}

/**
 * Invoked when a mouse-wheel event is captured on the component node.
 * @param {WheelEvent} e
 */
static void wheelMouse(float wheelDelta, float containerHeight, float scrollerHeight, float itemHeightVal,
                        const std::vector<ItemEntry>& filteredItems, ItemListboxState& state) {
	const float weight = containerHeight - scrollerHeight;

	if (itemHeightVal > 0.0f) {
		const int scrollCount = static_cast<int>(std::floor(containerHeight / itemHeightVal));
		const float direction = wheelDelta > 0.0f ? 1.0f : -1.0f;
		state.scroll += ((static_cast<float>(scrollCount) * itemWeight(filteredItems)) * weight) * direction;
		recalculateBounds(containerHeight, scrollerHeight, state);
	}
}

/**
 * Helper: check if an item ID is in the selection vector.
 * JS equivalent: this.selection.includes(item) — uses object identity.
 */
static bool isSelected(const std::vector<int>& selection, int itemId) {
	return std::find(selection.begin(), selection.end(), itemId) != selection.end();
}

/**
 * Helper: find position of an item ID in the selection vector, or -1.
 * JS equivalent: this.selection.indexOf(item) — uses object identity.
 */
static int selectionIndexOf(const std::vector<int>& selection, int itemId) {
	auto it = std::find(selection.begin(), selection.end(), itemId);
	if (it != selection.end())
		return static_cast<int>(std::distance(selection.begin(), it));
	return -1;
}

/**
 * Helper: find the index of an item with a given ID in the items vector, or -1.
 * JS equivalent: this.filteredItems.indexOf(item) — finds position by identity.
 */
static int indexOfItemById(const std::vector<ItemEntry>& items, int itemId) {
	for (size_t i = 0; i < items.size(); ++i) {
		if (items[i].id == itemId)
			return static_cast<int>(i);
	}
	return -1;
}

/**
 * Convert a string to lowercase.
 */
static std::string toLower(const std::string& s) {
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(),
	               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

/**
 * Trim whitespace from both ends of a string.
 */
static std::string trim(const std::string& s) {
	auto start = s.find_first_not_of(" \t\n\r\f\v");
	if (start == std::string::npos)
		return "";
	auto end = s.find_last_not_of(" \t\n\r\f\v");
	return s.substr(start, end - start + 1);
}

/**
 * Reactively filtered version of the underlying data array.
 * Automatically refilters when the filter input is changed.
 */
static std::vector<ItemEntry> computeFilteredItems(const std::vector<ItemEntry>& items,
                                                     const std::string& filter,
                                                     bool useRegex,
                                                     const std::vector<int>& selection,
                                                     const std::function<void(const std::vector<int>&)>& onSelectionChanged) {
	// Skip filtering if no filter is set.
	if (filter.empty())
		return items;

	std::vector<ItemEntry> res;

	if (useRegex) {
		try {
			std::regex regexFilter(trim(filter), std::regex_constants::icase);
			for (const auto& e : items) {
				if (std::regex_search(e.displayName, regexFilter))
					res.push_back(e);
			}
		} catch (const std::regex_error&) {
			// Regular expression did not compile, skip filtering.
			res = items;
		}
	} else {
		const std::string filterLower = toLower(trim(filter));
		if (filterLower.empty()) {
			res = items;
		} else {
			for (const auto& e : items) {
				if (toLower(e.displayName).find(filterLower) != std::string::npos)
					res.push_back(e);
			}
		}
	}

	// Prune selection: remove any selected item IDs that are no longer in the filtered result.
	// JS equivalent: this.selection.filter(item => res.includes(item))
	// In JS, selection stores item objects and checks by reference identity.
	// In C++, selection stores item IDs and checks by ID equality.
	bool hasChanges = false;
	std::vector<int> newSelection;
	for (int selectedId : selection) {
		bool found = false;
		for (const auto& entry : res) {
			if (entry.id == selectedId) {
				found = true;
				break;
			}
		}
		if (found) {
			newSelection.push_back(selectedId);
		} else {
			hasChanges = true;
		}
	}

	if (hasChanges && onSelectionChanged)
		onSelectionChanged(newSelection);

	return res;
}

/**
 * Invoked when a keydown event is fired.
 * @param {KeyboardEvent} e
 */
static void handleKey(const std::vector<ItemEntry>& filteredItems, const std::vector<int>& selection,
                       bool single, ItemListboxState& state,
                       const std::function<void(const std::vector<int>&)>& onSelectionChanged,
                       float containerHeight, float scrollerHeight) {
	const ImGuiIO& io = ImGui::GetIO();

	// If document.activeElement is the document body, then we can safely assume
	// the user is not focusing anything, and can intercept keyboard input.
	// TODO(conversion): In ImGui, we check if no item is active (no text input focused, etc.).
	if (ImGui::IsAnyItemActive())
		return;

	// User hasn't selected anything in the listbox yet.
	// JS: if (!this.lastSelectItem) return; — checks null
	if (state.lastSelectItem < 0)
		return;

	if (ImGui::IsKeyPressed(ImGuiKey_C) && io.KeyCtrl) {
		// Copy selection to clipboard.
		// JS: nw.Clipboard.get().set(this.selection.map(e => e.displayName).join('\n'), 'text');
		std::string clipText;
		bool first = true;
		for (int selectedId : selection) {
			int idx = indexOfItemById(filteredItems, selectedId);
			if (idx >= 0) {
				if (!first) clipText += '\n';
				clipText += filteredItems[static_cast<size_t>(idx)].displayName;
				first = false;
			}
		}
		ImGui::SetClipboardText(clipText.c_str());
	} else {
		// Arrow keys.
		const bool isArrowUp = ImGui::IsKeyPressed(ImGuiKey_UpArrow);
		const bool isArrowDown = ImGui::IsKeyPressed(ImGuiKey_DownArrow);
		if (isArrowUp || isArrowDown) {
			const int delta = isArrowUp ? -1 : 1;

			// Move/expand selection one.
			// JS: const lastSelectIndex = this.filteredItems.indexOf(this.lastSelectItem);
			const int lastSelectIndex = indexOfItemById(filteredItems, state.lastSelectItem);
			if (lastSelectIndex < 0)
				return;

			const int nextIndex = lastSelectIndex + delta;
			// JS: const next = this.filteredItems[nextIndex]; if (next) { ... }
			if (nextIndex >= 0 && nextIndex < static_cast<int>(filteredItems.size())) {
				const int currentScrollIdx = scrollIndex(filteredItems, state);
				const int lastViewIndex = isArrowUp ? currentScrollIdx : currentScrollIdx + state.slotCount;
				int diff = std::abs(nextIndex - lastViewIndex);
				if (isArrowDown)
					diff += 1;

				if ((isArrowUp && nextIndex < lastViewIndex) || (isArrowDown && nextIndex >= lastViewIndex)) {
					const float weight = containerHeight - scrollerHeight;
					state.scroll += ((static_cast<float>(diff) * itemWeight(filteredItems)) * weight) * static_cast<float>(delta);
					recalculateBounds(containerHeight, scrollerHeight, state);
				}

				std::vector<int> newSelection = selection;

				// JS: if (!e.shiftKey || this.single) newSelection.splice(0);
				if (!io.KeyShift || single)
					newSelection.clear();

				// JS: newSelection.push(next); — pushes item object
				// C++: push item ID
				const int nextItemId = filteredItems[static_cast<size_t>(nextIndex)].id;
				newSelection.push_back(nextItemId);
				state.lastSelectItem = nextItemId;

				if (onSelectionChanged)
					onSelectionChanged(newSelection);
			}
		}
	}
}

/**
 * Invoked when a user selects an item in the list.
 * @param {string} item
 * @param {MouseEvent} e
 */
static void selectItem(int itemIndex, bool ctrlKey, bool shiftKey,
                         const std::vector<ItemEntry>& filteredItems,
                         const std::vector<int>& selection,
                         bool single, ItemListboxState& state,
                         const std::function<void(const std::vector<int>&)>& onSelectionChanged) {
	const int itemId = filteredItems[static_cast<size_t>(itemIndex)].id;
	const int checkIndex = selectionIndexOf(selection, itemId);
	std::vector<int> newSelection = selection;

	if (single) {
		// Listbox is in single-entry mode, replace selection.
		// JS: if (checkIndex === -1) { newSelection.splice(0); newSelection.push(item); }
		if (checkIndex == -1) {
			newSelection.clear();
			newSelection.push_back(itemId);
		}

		// JS: this.lastSelectItem = item;
		state.lastSelectItem = itemId;
	} else {
		if (ctrlKey) {
			// Ctrl-key held, so allow multiple selections.
			if (checkIndex > -1)
				newSelection.erase(newSelection.begin() + checkIndex);
			else
				newSelection.push_back(itemId);
		} else if (shiftKey) {
			// Shift-key held, select a range.
			// JS: if (this.lastSelectItem && this.lastSelectItem !== item)
			if (state.lastSelectItem >= 0 && state.lastSelectItem != itemId) {
				// JS: const lastSelectIndex = this.filteredItems.indexOf(this.lastSelectItem);
				const int lastSelectIndex = indexOfItemById(filteredItems, state.lastSelectItem);
				const int thisSelectIndex = itemIndex;

				if (lastSelectIndex >= 0) {
					const int rangeLen = std::abs(lastSelectIndex - thisSelectIndex);
					const int lowest = std::min(lastSelectIndex, thisSelectIndex);

					// JS: const range = this.filteredItems.slice(lowest, lowest + delta + 1);
					// JS: for (const select of range) { if (newSelection.indexOf(select) === -1) newSelection.push(select); }
					for (int i = lowest; i <= lowest + rangeLen; ++i) {
						const int rangeItemId = filteredItems[static_cast<size_t>(i)].id;
						if (!isSelected(newSelection, rangeItemId))
							newSelection.push_back(rangeItemId);
					}
				}
			}
		} else if (checkIndex == -1 || (checkIndex > -1 && static_cast<int>(newSelection.size()) > 1)) {
			// Normal click, replace entire selection.
			newSelection.clear();
			newSelection.push_back(itemId);
		}

		// JS: this.lastSelectItem = item;
		state.lastSelectItem = itemId;
	}

	// JS: this.$emit('update:selection', newSelection);
	if (onSelectionChanged)
		onSelectionChanged(newSelection);
}

/**
 * Get ImGui color for item quality.
 * Maps WoW item quality values to standard color scheme.
 */
static ImVec4 getQualityColor(int quality) {
	switch (quality) {
		case 0: return ImVec4(0.62f, 0.62f, 0.62f, 1.0f); // Poor (gray)
		case 1: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);    // Common (white)
		case 2: return ImVec4(0.12f, 1.0f, 0.0f, 1.0f);    // Uncommon (green)
		case 3: return ImVec4(0.0f, 0.44f, 0.87f, 1.0f);   // Rare (blue)
		case 4: return ImVec4(0.64f, 0.21f, 0.93f, 1.0f);  // Epic (purple)
		case 5: return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);     // Legendary (orange)
		case 6: return ImVec4(0.9f, 0.8f, 0.5f, 1.0f);     // Artifact (light gold)
		case 7: return ImVec4(0.0f, 0.8f, 1.0f, 1.0f);     // Heirloom (light blue)
		default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);    // Default (white)
	}
}

/**
 * watch: displayItems — load icons for visible items.
 */
// NOTE: Icon loading is done inline during rendering below.

/**
 * HTML mark-up to render for this component.
 */
// template: converted to ImGui immediate-mode rendering below.

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
            const std::function<void(const ItemEntry&)>& onOptions) {
	ImGui::PushID(id);

	// Compute filtered items.
	const std::vector<ItemEntry> filteredItems = computeFilteredItems(
		items, filter, regex, selection, onSelectionChanged);

	// Get the available content region as the container dimensions.
	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const float containerHeight = availSize.y;

	// The scroller thumb height is proportional to visible vs total items.
	const float itemHeightVal = 26.0f;
	const int totalItems = static_cast<int>(filteredItems.size());
	const float scrollerHeight = (totalItems > 0)
		? std::max(20.0f, containerHeight * (static_cast<float>(state.slotCount) / static_cast<float>(totalItems)))
		: containerHeight;

	// Equivalent of resize() — recalculate slot count and scroll each frame.
	resize(containerHeight, scrollerHeight, state);

	// Compute display range.
	const int idx = scrollIndex(filteredItems, state);
	const int startIdx = std::max(0, idx);
	const int endIdx = std::min(totalItems, startIdx + state.slotCount);

	// Begin a child region to contain the list.
	// <div ref="root" class="ui-listbox" @wheel="wheelMouse">
	ImGui::BeginChild("##itemlistbox_container", availSize, ImGuiChildFlags_None,
	                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Handle mouse wheel on the container.
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None)) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			wheelMouse(-wheel, containerHeight, scrollerHeight, itemHeightVal, filteredItems, state);
		}
	}

	// Handle global mouse move/up for scrollbar dragging.
	const ImGuiIO& io = ImGui::GetIO();
	if (state.isScrolling) {
		moveMouse(io.MousePos.y, containerHeight, scrollerHeight, state);
		if (!io.MouseDown[0]) {
			stopMouse(state);
		}
	}

	// Handle keyboard input.
	if (keyinput) {
		handleKey(filteredItems, selection, single, state, onSelectionChanged,
		          containerHeight, scrollerHeight);
	}

	// Draw the scrollbar on the right side.
	// <div class="scroller" ref="scroller" @mousedown="startMouse" :class="{ using: isScrolling }" :style="{ top: scrollOffset }"><div></div></div>
	{
		const ImVec2 winPos = ImGui::GetWindowPos();
		const float scrollbarWidth = 8.0f;
		const float scrollbarX = winPos.x + availSize.x - scrollbarWidth;
		const float thumbY = winPos.y + scrollOffset(state);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Scroller thumb.
		const ImVec2 thumbMin(scrollbarX, thumbY);
		const ImVec2 thumbMax(scrollbarX + scrollbarWidth, thumbY + scrollerHeight);

		// Determine if mouse is over the thumb for hover effect.
		const bool thumbHovered = ImGui::IsMouseHoveringRect(thumbMin, thumbMax) || state.isScrolling;
		const ImU32 thumbColor = thumbHovered
			? IM_COL32(255, 255, 255, 180)
			: IM_COL32(255, 255, 255, 80);

		drawList->AddRectFilled(thumbMin, thumbMax, thumbColor, 4.0f);

		// Handle mouse-down on the scroller thumb.
		if (ImGui::IsMouseHoveringRect(thumbMin, thumbMax) && ImGui::IsMouseClicked(0)) {
			startMouse(io.MousePos.y, state);
		}
	}

	// Render visible items.
	// <div v-for="(item, i) in displayItems" class="item" @click="selectItem(item, $event)" :class="{ selected: selection.includes(item) }">
	//     <div :class="['item-icon', 'icon-' + item.icon ]"></div>
	//     <div :class="['item-name', 'item-quality-' + item.quality]">{{ item.name }} <span class="item-id">({{ item.id }})</span></div>
	//     <ul class="item-buttons">
	//         <li @click.self="$emit('equip', item)">Equip</li>
	//         <li @click.self="$emit('options', item)">Options</li>
	//     </ul>
	// </div>
	for (int i = startIdx; i < endIdx; ++i) {
		const ItemEntry& item = filteredItems[static_cast<size_t>(i)];
		const bool itemSelected = isSelected(selection, item.id);

		// watch: displayItems — load icons for visible items.
		icon_render::loadIcon(item.icon);

		// Selected highlight (:class="{ selected: selection.includes(item) }").
		if (itemSelected) {
			const ImVec2 rowMin = ImGui::GetCursorScreenPos();
			const ImVec2 rowMax(rowMin.x + availSize.x - 10.0f, rowMin.y + itemHeightVal);
			ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, IM_COL32(34, 181, 73, 40));
		}

		ImGui::PushID(i);

		// Item icon (<div :class="['item-icon', 'icon-' + item.icon ]"></div>).
		const uint32_t iconTex = icon_render::getIconTexture(item.icon);
		if (iconTex != 0) {
			ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(iconTex)),
			             ImVec2(20.0f, 20.0f));
			ImGui::SameLine();
		}

		// Item name with quality color (<div :class="['item-name', 'item-quality-' + item.quality]">).
		const ImVec4 qualColor = getQualityColor(item.quality);
		ImGui::PushStyleColor(ImGuiCol_Text, qualColor);

		// Build display text: "Name (ID)"
		std::string displayText = item.name + " (" + std::to_string(item.id) + ")";

		// Clicking the row selects the item.
		if (ImGui::Selectable(displayText.c_str(), itemSelected, ImGuiSelectableFlags_None,
		                      ImVec2(availSize.x - 120.0f, 0.0f))) {
			selectItem(i, io.KeyCtrl, io.KeyShift, filteredItems, selection, single, state, onSelectionChanged);
		}

		ImGui::PopStyleColor();

		// Item buttons (<ul class="item-buttons">).
		ImGui::SameLine();
		// <li @click.self="$emit('equip', item)">Equip</li>
		if (ImGui::SmallButton("Equip")) {
			if (onEquip)
				onEquip(item);
		}
		ImGui::SameLine();
		// <li @click.self="$emit('options', item)">Options</li>
		if (ImGui::SmallButton("Options")) {
			if (onOptions)
				onOptions(item);
		}

		ImGui::PopID();
	}

	ImGui::EndChild();

	// <div class="list-status" v-if="unittype">{{ filteredItems.length }} {{ unittype + (filteredItems.length != 1 ? 's' : '') }} found. {{ selection.length > 0 ? ' (' + selection.length + ' selected)' : '' }}</div>
	if (!unittype.empty()) {
		std::string statusText = std::to_string(filteredItems.size()) + " " + unittype;
		if (filteredItems.size() != 1)
			statusText += "s";
		statusText += " found.";
		if (!selection.empty())
			statusText += " (" + std::to_string(selection.size()) + " selected)";

		ImGui::Text("%s", statusText.c_str());
	}

	ImGui::PopID();
}

} // namespace itemlistbox