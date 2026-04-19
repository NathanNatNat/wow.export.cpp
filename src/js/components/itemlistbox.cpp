/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "itemlistbox.h"

#include <imgui.h>
#include "../../app.h"
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

/**
 * Invoked when the component is destroyed.
 * Used to unregister global mouse listeners and resize observer.
 */

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
	// CSS: #tab-items #listbox-items .item { height: 46px; }
	state.slotCount = static_cast<int>(std::floor(containerHeight / 46.0f));
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
	// In JS, selection stores item objects and checks by reference identity.
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
	if (ImGui::IsAnyItemActive())
		return;

	// User hasn't selected anything in the listbox yet.
	if (state.lastSelectItem < 0)
		return;

	if (ImGui::IsKeyPressed(ImGuiKey_C) && io.KeyCtrl) {
		// Copy selection to clipboard.
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
			const int lastSelectIndex = indexOfItemById(filteredItems, state.lastSelectItem);
			if (lastSelectIndex < 0)
				return;

			const int nextIndex = lastSelectIndex + delta;
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

				if (!io.KeyShift || single)
					newSelection.clear();

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
		if (checkIndex == -1) {
			newSelection.clear();
			newSelection.push_back(itemId);
		}

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
			if (state.lastSelectItem >= 0 && state.lastSelectItem != itemId) {
				const int lastSelectIndex = indexOfItemById(filteredItems, state.lastSelectItem);
				const int thisSelectIndex = itemIndex;

				if (lastSelectIndex >= 0) {
					const int rangeLen = std::abs(lastSelectIndex - thisSelectIndex);
					const int lowest = std::min(lastSelectIndex, thisSelectIndex);

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

		state.lastSelectItem = itemId;
	}

	if (onSelectionChanged)
		onSelectionChanged(newSelection);
}

/**
 * Get ImGui color for item quality.
 * Maps WoW item quality values to standard color scheme.
 */
static ImVec4 getQualityColor(int quality) {
	// Exact CSS hex values from app.css .item-quality-N classes.
	switch (quality) {
		case 0: return ImVec4(157/255.0f, 157/255.0f, 157/255.0f, 1.0f); // Poor (#9d9d9d)
		case 1: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);                   // Common (#ffffff)
		case 2: return ImVec4(30/255.0f, 1.0f, 0.0f, 1.0f);              // Uncommon (#1eff00)
		case 3: return ImVec4(0.0f, 112/255.0f, 221/255.0f, 1.0f);       // Rare (#0070dd)
		case 4: return ImVec4(163/255.0f, 53/255.0f, 238/255.0f, 1.0f);  // Epic (#a335ee)
		case 5: return ImVec4(1.0f, 128/255.0f, 0.0f, 1.0f);             // Legendary (#ff8000)
		case 6: return ImVec4(230/255.0f, 204/255.0f, 128/255.0f, 1.0f); // Artifact (#e6cc80)
		case 7: // fallthrough
		case 8: return ImVec4(0.0f, 204/255.0f, 1.0f, 1.0f);             // Heirloom/Mythic (#00ccff)
		default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);                  // Default (white)
	}
}

/**
 * watch: displayItems — load icons for visible items.
 */

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

	// Compute filtered items — only when inputs have actually changed.
	{
		const ItemEntry* curData = items.empty() ? nullptr : items.data();
		const bool needRecompute = !state.filteredItemsCacheValid
			|| curData         != state.cachedItemsData
			|| items.size()    != state.cachedItemsSize
			|| filter          != state.cachedFilter
			|| regex           != state.cachedRegexMode;

		if (needRecompute) {
			state.cachedFilteredItems  = computeFilteredItems(items, filter, regex, selection, onSelectionChanged);
			state.cachedItemsData      = curData;
			state.cachedItemsSize      = items.size();
			state.cachedFilter         = filter;
			state.cachedRegexMode      = regex;
			state.filteredItemsCacheValid = true;
		}
	}
	const std::vector<ItemEntry>& filteredItems = state.cachedFilteredItems;

	// Get the available content region as the container dimensions.
	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const float containerHeight = availSize.y;

	// The scroller thumb height is proportional to visible vs total items.
	// CSS: #tab-items #listbox-items .item { height: 46px; font-size: 1.2em; }
	const float itemHeightVal = 46.0f;
	const int totalItems = static_cast<int>(filteredItems.size());
	const float scrollerHeight = (totalItems > 0)
		? std::max(20.0f, containerHeight * (static_cast<float>(state.slotCount) / static_cast<float>(totalItems)))
		: containerHeight;

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
			? app::theme::TEXT_ACTIVE_U32
			: app::theme::TEXT_IDLE_U32;

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

		// Alternating row background + selected highlight.
		//      .ui-listbox .item:nth-child(even) { background: var(--background-alt); }
		//      .ui-listbox .item.selected { background: var(--font-alt); }
		{
			const ImVec2 rowMin = ImGui::GetCursorScreenPos();
			const ImVec2 rowMax(rowMin.x + availSize.x - 10.0f, rowMin.y + itemHeightVal);
			const ImU32 rowBg = ((i - startIdx) % 2 == 0)
				? app::theme::BG_ALT_U32
				: app::theme::BG_DARK_U32;
			ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, rowBg);

			if (itemSelected)
				ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, app::theme::ROW_SELECTED_U32);
		}

		ImGui::PushID(i);

		// Item icon (<div :class="['item-icon', 'icon-' + item.icon ]"></div>).
		const uint32_t iconTex = icon_render::getIconTexture(item.icon);
		if (iconTex != 0) {
			const ImVec2 iconPos = ImGui::GetCursorScreenPos();
			constexpr float ICON_SIZE = 32.0f;

			ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(iconTex)),
			             ImVec2(ICON_SIZE, ICON_SIZE));

			// Draw 1px border on top of the icon matching CSS border: 1px solid #8a8a8a.
			const ImVec2 borderMin(iconPos.x - 1.0f, iconPos.y - 1.0f);
			const ImVec2 borderMax(iconPos.x + ICON_SIZE + 1.0f, iconPos.y + ICON_SIZE + 1.0f);
			ImGui::GetWindowDrawList()->AddRect(borderMin, borderMax, IM_COL32(138, 138, 138, 255));

			ImGui::SameLine(0.0f, 10.0f);
		}

		// Item name with quality color (<div :class="['item-name', 'item-quality-' + item.quality]">).
		const ImVec4 qualColor = getQualityColor(item.quality);
		ImGui::PushStyleColor(ImGuiCol_Text, qualColor);

		// Clicking the row selects the item.
		// Build display text: "Name" (quality colored) + " (ID)" (grey, smaller — CSS .item-id { color: grey; font-size: 0.8em; })
		if (ImGui::Selectable("##item_sel", itemSelected, ImGuiSelectableFlags_None,
		                      ImVec2(availSize.x - 120.0f, 0.0f))) {
			selectItem(i, io.KeyCtrl, io.KeyShift, filteredItems, selection, single, state, onSelectionChanged);
		}
		// Render name and ID on top of the selectable at the same line.
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - (availSize.x - 120.0f));
		ImGui::Text("%s", item.name.c_str());
		ImGui::PopStyleColor();

		// Item ID: <span class="item-id">({{ item.id }})</span> — grey, font-size: 0.8em
		ImGui::SameLine(0.0f, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(128/255.0f, 128/255.0f, 128/255.0f, 1.0f)); // grey
		{
			const float origScale = ImGui::GetFont()->Scale;
			ImGui::GetFont()->Scale *= 0.8f;
			ImGui::PushFont(ImGui::GetFont());
			ImGui::Text("(%d)", item.id);
			ImGui::GetFont()->Scale = origScale;
			ImGui::PopFont();
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