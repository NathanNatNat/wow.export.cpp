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

/**
 * Reactive instance data.
 */

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

static bool isSelected(const std::vector<int>& selection, int itemId) {
	return std::find(selection.begin(), selection.end(), itemId) != selection.end();
}

static int selectionIndexOf(const std::vector<int>& selection, int itemId) {
	auto it = std::find(selection.begin(), selection.end(), itemId);
	if (it != selection.end())
		return static_cast<int>(std::distance(selection.begin(), it));
	return -1;
}

static int indexOfItemById(const std::vector<ItemEntry>& items, int itemId) {
	for (size_t i = 0; i < items.size(); ++i) {
		if (items[i].id == itemId)
			return static_cast<int>(i);
	}
	return -1;
}

static std::string toLower(const std::string& s) {
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(),
	               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

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

static ImVec4 getQualityColor(int quality) {
	switch (quality) {
		case 0: return ImVec4(157/255.0f, 157/255.0f, 157/255.0f, 1.0f);
		case 1: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		case 2: return ImVec4(30/255.0f, 1.0f, 0.0f, 1.0f);
		case 3: return ImVec4(0.0f, 112/255.0f, 221/255.0f, 1.0f);
		case 4: return ImVec4(163/255.0f, 53/255.0f, 238/255.0f, 1.0f);
		case 5: return ImVec4(1.0f, 128/255.0f, 0.0f, 1.0f);
		case 6: return ImVec4(230/255.0f, 204/255.0f, 128/255.0f, 1.0f);
		case 7: // fallthrough
		case 8: return ImVec4(0.0f, 204/255.0f, 1.0f, 1.0f);
		default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

/**
 * HTML mark-up to render for this component.
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
            const std::function<void(const ItemEntry&)>& onOptions) {
	ImGui::PushID(id);

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

	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const float containerHeight = availSize.y;

	const float itemHeightVal = 46.0f;
	const int totalItems = static_cast<int>(filteredItems.size());
	const float scrollerHeight = (totalItems > 0)
		? std::max(20.0f, containerHeight * (static_cast<float>(state.slotCount) / static_cast<float>(totalItems)))
		: containerHeight;

	resize(containerHeight, scrollerHeight, state);

	const int idx = scrollIndex(filteredItems, state);
	const int startIdx = std::max(0, idx);
	const int endIdx = std::min(totalItems, startIdx + state.slotCount);

	ImGui::BeginChild("##itemlistbox_container", availSize, ImGuiChildFlags_Borders,
	                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None)) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			wheelMouse(-wheel, containerHeight, scrollerHeight, itemHeightVal, filteredItems, state);
		}
	}

	const ImGuiIO& io = ImGui::GetIO();
	if (state.isScrolling) {
		moveMouse(io.MousePos.y, containerHeight, scrollerHeight, state);
		if (!io.MouseDown[0]) {
			stopMouse(state);
		}
	}

	if (keyinput) {
		handleKey(filteredItems, selection, single, state, onSelectionChanged,
		          containerHeight, scrollerHeight);
	}

	{
		const ImVec2 winPos = ImGui::GetWindowPos();
		const float scrollbarWidth = 8.0f;
		const float scrollbarX = winPos.x + availSize.x - scrollbarWidth;
		const float thumbY = winPos.y + scrollOffset(state);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const ImVec2 thumbMin(scrollbarX, thumbY);
		const ImVec2 thumbMax(scrollbarX + scrollbarWidth, thumbY + scrollerHeight);

		const bool thumbHovered = ImGui::IsMouseHoveringRect(thumbMin, thumbMax) || state.isScrolling;
		const ImU32 thumbColor = thumbHovered
			? ImGui::GetColorU32(ImGuiCol_Text)
			: ImGui::GetColorU32(ImGuiCol_TextDisabled);

		drawList->AddRectFilled(thumbMin, thumbMax, thumbColor, 4.0f);

		if (ImGui::IsMouseHoveringRect(thumbMin, thumbMax) && ImGui::IsMouseClicked(0)) {
			startMouse(io.MousePos.y, state);
		}
	}

	const ImVec2 framePad = ImGui::GetStyle().FramePadding;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float equipBtnW = ImGui::CalcTextSize("Equip").x + framePad.x * 2.0f;
	const float optionsBtnW = ImGui::CalcTextSize("Options").x + framePad.x * 2.0f;
	const float idTextW = 60.0f;
	const float reservedRight = equipBtnW + optionsBtnW + idTextW + spacing * 4.0f;

	for (int i = startIdx; i < endIdx; ++i) {
		const ItemEntry& item = filteredItems[static_cast<size_t>(i)];
		const bool itemSelected = isSelected(selection, item.id);

		icon_render::loadIcon(item.icon);


		ImGui::PushID(i);

		const uint32_t iconTex = icon_render::getIconTexture(item.icon);
		if (iconTex != 0) {
			const ImVec2 iconPos = ImGui::GetCursorScreenPos();
			constexpr float ICON_SIZE = 32.0f;

			ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(iconTex)),
			             ImVec2(ICON_SIZE, ICON_SIZE));

			const ImVec2 borderMin(iconPos.x - 1.0f, iconPos.y - 1.0f);
			const ImVec2 borderMax(iconPos.x + ICON_SIZE + 1.0f, iconPos.y + ICON_SIZE + 1.0f);
			ImGui::GetWindowDrawList()->AddRect(borderMin, borderMax, IM_COL32(138, 138, 138, 255));

			ImGui::SameLine(0.0f, 10.0f);
		}

		const ImVec4 qualColor = getQualityColor(item.quality);
		ImGui::PushStyleColor(ImGuiCol_Text, qualColor);

		if (ImGui::Selectable("##item_sel", itemSelected, ImGuiSelectableFlags_None,
		                      ImVec2(availSize.x - reservedRight, 0.0f))) {
			selectItem(i, io.KeyCtrl, io.KeyShift, filteredItems, selection, single, state, onSelectionChanged);
		}
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - (availSize.x - reservedRight));
		ImGui::Text("%s", item.name.c_str());
		ImGui::PopStyleColor();

		ImGui::SameLine(0.0f, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(128/255.0f, 128/255.0f, 128/255.0f, 1.0f));
		ImGui::Text("(%d)", item.id);
		ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::SmallButton("Equip")) {
			if (onEquip)
				onEquip(item);
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Options")) {
			if (onOptions)
				onOptions(item);
		}

		ImGui::PopID();
	}

	ImGui::EndChild();

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