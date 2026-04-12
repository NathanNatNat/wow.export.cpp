/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "listboxb.h"

#include <imgui.h>
#include "../../app.h"
#include <cmath>
#include <algorithm>
#include <string>

namespace listboxb {

/**
 * items: Item entries displayed in the list.
 * selection: Reactive selection controller.
 * single: If set, only one entry can be selected.
 * keyinput: If true, listbox registers for keyboard input.
 * disable: If provided, used as reactive disable flag.
 */
// props: ['items', 'selection', 'single', 'keyinput', 'disable']
// emits: ['update:selection']

/**
 * Reactive instance data.
 */
// data: scroll, scrollRel, isScrolling, slotCount, lastSelectItem — stored in ListboxBState

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
static float scrollOffset(const ListboxBState& state) {
	return state.scroll;
}

/**
 * Index which array reading should start at, based on the current
 * relative scroll and the overal item count. Value is dynamically
 * capped based on slot count to prevent empty slots appearing.
 */
static int scrollIndex(const std::vector<ListboxBItem>& items, const ListboxBState& state) {
	return static_cast<int>(std::round((static_cast<float>(items.size()) - static_cast<float>(state.slotCount)) * state.scrollRel));
}

/**
 * Weight (0-1) of a single item.
 */
static float itemWeight(const std::vector<ListboxBItem>& items) {
	if (items.empty())
		return 0.0f;
	return 1.0f / static_cast<float>(items.size());
}

/**
 * Invoked by a ResizeObserver when the main component node
 * is resized due to layout changes.
 */
static void resize(float containerHeight, float scrollerHeight, ListboxBState& state) {
	state.scroll = (containerHeight - scrollerHeight) * state.scrollRel;
	state.slotCount = static_cast<int>(std::floor(containerHeight / 26.0f));
}

/**
 * Restricts the scroll offset to prevent overflowing and
 * calculates the relative (0-1) offset based on the scroll.
 */
static void recalculateBounds(float containerHeight, float scrollerHeight, ListboxBState& state) {
	const float max = containerHeight - scrollerHeight;
	state.scroll = std::min(max, std::max(0.0f, state.scroll));
	state.scrollRel = (max > 0.0f) ? (state.scroll / max) : 0.0f;
}

/**
 * Invoked when a mouse-down event is captured on the scroll widget.
 * @param {MouseEvent} e
 */
static void startMouse(float mouseY, ListboxBState& state) {
	state.scrollStartY = mouseY;
	state.scrollStart = state.scroll;
	state.isScrolling = true;
}

/**
 * Invoked when a mouse-move event is captured globally.
 * @param {MouseEvent} e
 */
static void moveMouse(float mouseY, float containerHeight, float scrollerHeight, ListboxBState& state) {
	if (state.isScrolling) {
		state.scroll = state.scrollStart + (mouseY - state.scrollStartY);
		recalculateBounds(containerHeight, scrollerHeight, state);
	}
}

/**
 * Invoked when a mouse-up event is captured globally.
 */
static void stopMouse(ListboxBState& state) {
	state.isScrolling = false;
}

/**
 * Invoked when a mouse-wheel event is captured on the component node.
 * @param {WheelEvent} e
 */
static void wheelMouse(float wheelDelta, float containerHeight, float scrollerHeight, float itemHeight,
                        const std::vector<ListboxBItem>& items, ListboxBState& state) {
	const float weight = containerHeight - scrollerHeight;

	if (itemHeight > 0.0f) {
		const int scrollCount = static_cast<int>(std::floor(containerHeight / itemHeight));
		const float direction = wheelDelta > 0.0f ? 1.0f : -1.0f;
		state.scroll += ((static_cast<float>(scrollCount) * itemWeight(items)) * weight) * direction;
		recalculateBounds(containerHeight, scrollerHeight, state);
	}
}

/**
 * Helper: check if an index is in the selection vector.
 */
static bool isSelected(const std::vector<int>& selection, int index) {
	return std::find(selection.begin(), selection.end(), index) != selection.end();
}

/**
 * Helper: find position of an index in the selection vector, or -1.
 */
static int selectionIndexOf(const std::vector<int>& selection, int index) {
	auto it = std::find(selection.begin(), selection.end(), index);
	if (it != selection.end())
		return static_cast<int>(std::distance(selection.begin(), it));
	return -1;
}

/**
 * Invoked when a keydown event is fired.
 * @param {KeyboardEvent} e
 */
static void handleKey(const std::vector<ListboxBItem>& items, const std::vector<int>& selection,
                       bool single, bool disable, ListboxBState& state,
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
		for (size_t i = 0; i < selection.size(); ++i) {
			if (i > 0) clipText += '\n';
			const int idx = selection[i];
			if (idx >= 0 && idx < static_cast<int>(items.size()))
				clipText += items[static_cast<size_t>(idx)].label;
		}
		ImGui::SetClipboardText(clipText.c_str());
	} else {
		if (disable)
			return;

		// Arrow keys.
		const bool isArrowUp = ImGui::IsKeyPressed(ImGuiKey_UpArrow);
		const bool isArrowDown = ImGui::IsKeyPressed(ImGuiKey_DownArrow);
		if (isArrowUp || isArrowDown) {
			const int delta = isArrowUp ? -1 : 1;

			// Move/expand selection one.
			const int lastSelectIndex = state.lastSelectItem;
			const int nextIndex = lastSelectIndex + delta;
			if (nextIndex >= 0 && nextIndex < static_cast<int>(items.size())) {
				const int currentScrollIdx = scrollIndex(items, state);
				const int lastViewIndex = isArrowUp ? currentScrollIdx : currentScrollIdx + state.slotCount;
				int diff = std::abs(nextIndex - lastViewIndex);
				if (isArrowDown)
					diff += 1;

				if ((isArrowUp && nextIndex < lastViewIndex) || (isArrowDown && nextIndex >= lastViewIndex)) {
					const float weight = containerHeight - scrollerHeight;
					state.scroll += ((static_cast<float>(diff) * itemWeight(items)) * weight) * static_cast<float>(delta);
					recalculateBounds(containerHeight, scrollerHeight, state);
				}

				std::vector<int> newSelection = selection;

				if (!io.KeyShift || single)
					newSelection.clear();

				newSelection.push_back(nextIndex);
				state.lastSelectItem = nextIndex;
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
                         const std::vector<ListboxBItem>& items,
                         const std::vector<int>& selection,
                         bool single, bool disable, ListboxBState& state,
                         const std::function<void(const std::vector<int>&)>& onSelectionChanged) {
	if (disable)
		return;

	const int checkIndex = selectionIndexOf(selection, itemIndex);
	std::vector<int> newSelection = selection;

	if (single) {
		// Listbox is in single-entry mode, replace selection.
		if (checkIndex == -1) {
			newSelection.clear();
			newSelection.push_back(itemIndex);
		}

		state.lastSelectItem = itemIndex;
	} else {
		if (ctrlKey) {
			// Ctrl-key held, so allow multiple selections.
			if (checkIndex > -1)
				newSelection.erase(newSelection.begin() + checkIndex);
			else
				newSelection.push_back(itemIndex);
		} else if (shiftKey) {
			// Shift-key held, select a range.
			if (state.lastSelectItem >= 0 && state.lastSelectItem != itemIndex) {
				const int lastSelectIndex = state.lastSelectItem;
				const int thisSelectIndex = itemIndex;

				const int rangeLen = std::abs(lastSelectIndex - thisSelectIndex);
				const int lowest = std::min(lastSelectIndex, thisSelectIndex);

				for (int i = lowest; i <= lowest + rangeLen; ++i) {
					if (!isSelected(newSelection, i))
						newSelection.push_back(i);
				}
			}
		} else if (checkIndex == -1 || (checkIndex > -1 && static_cast<int>(newSelection.size()) > 1)) {
			// Normal click, replace entire selection.
			newSelection.clear();
			newSelection.push_back(itemIndex);
		}

		state.lastSelectItem = itemIndex;
	}

	if (onSelectionChanged)
		onSelectionChanged(newSelection);
}

/**
 * HTML mark-up to render for this component.
 */
// template: converted to ImGui immediate-mode rendering below.

void render(const char* id, const std::vector<ListboxBItem>& items,
            const std::vector<int>& selection, bool single, bool keyinput, bool disable,
            ListboxBState& state,
            const std::function<void(const std::vector<int>&)>& onSelectionChanged) {
	ImGui::PushID(id);

	// Get the available content region as the container dimensions.
	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const float containerHeight = availSize.y;

	// The scroller thumb height is proportional to visible vs total items.
	const float itemHeight = 26.0f;
	const int totalItems = static_cast<int>(items.size());
	const float scrollerHeight = (totalItems > 0)
		? std::max(20.0f, containerHeight * (static_cast<float>(state.slotCount) / static_cast<float>(totalItems)))
		: containerHeight;

	resize(containerHeight, scrollerHeight, state);

	// Compute display range.
	const int idx = scrollIndex(items, state);
	const int startIdx = std::max(0, idx);
	const int endIdx = std::min(totalItems, startIdx + state.slotCount);

	// Begin a child region to contain the list.
	// <div class="ui-listbox" @wheel="wheelMouse">
	ImGui::BeginChild("##listboxb_container", availSize, ImGuiChildFlags_None,
	                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Handle mouse wheel on the container.
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None)) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			wheelMouse(-wheel, containerHeight, scrollerHeight, itemHeight, items, state);
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
		handleKey(items, selection, single, disable, state, onSelectionChanged,
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
	//     <span class="sub sub-0">{{ item.label }}</span>
	// </div>
	for (int i = startIdx; i < endIdx; ++i) {
		const ListboxBItem& item = items[static_cast<size_t>(i)];
		const bool itemSelected = isSelected(selection, i);

		// Alternating row background + selected highlight.
		//      .ui-listbox .item:nth-child(even) { background: var(--background-alt); }
		//      .ui-listbox .item.selected { background: var(--font-alt); }
		{
			const ImVec2 rowMin = ImGui::GetCursorScreenPos();
			const ImVec2 rowMax(rowMin.x + availSize.x - 10.0f, rowMin.y + itemHeight);
			const ImU32 rowBg = ((i - startIdx) % 2 == 0)
				? app::theme::BG_ALT_U32
				: app::theme::BG_DARK_U32;
			ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, rowBg);

			if (itemSelected)
				ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, app::theme::ROW_SELECTED_U32);
		}

		ImGui::PushID(i);

		// <span class="sub sub-0">{{ item.label }}</span>
		// Clicking the row selects the item.
		if (ImGui::Selectable(item.label.c_str(), itemSelected, ImGuiSelectableFlags_None,
		                      ImVec2(availSize.x - 10.0f, 0.0f))) {
			selectItem(i, io.KeyCtrl, io.KeyShift, items, selection, single, disable, state, onSelectionChanged);
		}

		ImGui::PopID();
	}

	ImGui::EndChild();
	ImGui::PopID();
}

} // namespace listboxb