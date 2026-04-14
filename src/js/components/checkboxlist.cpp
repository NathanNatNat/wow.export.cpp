/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "checkboxlist.h"

#include <imgui.h>
#include "../../app.h"
#include <cmath>
#include <algorithm>

namespace checkboxlist {

/**
 * items: Item entries displayed in the list.
 */
// props: ['items']

/**
 * Reactive instance data.
 */
// data: scroll, scrollRel, isScrolling, slotCount — stored in CheckboxListState

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
static float scrollOffset(const CheckboxListState& state) {
	return state.scroll;
}

/**
 * Index which array reading should start at, based on the current
 * relative scroll and the overall item count. Value is dynamically
 * capped based on slot count to prevent empty slots appearing.
 */
static int scrollIndex(const std::vector<nlohmann::json>& items, const CheckboxListState& state) {
	return static_cast<int>(std::round((static_cast<float>(items.size()) - static_cast<float>(state.slotCount)) * state.scrollRel));
}

/**
 * Weight (0-1) of a single item.
 * JS: `return 1 / this.items.length;` — yields Infinity when items is empty.
 */
static float itemWeight(const std::vector<nlohmann::json>& items) {
	// Match JS: 1 / 0 === Infinity
	return 1.0f / static_cast<float>(items.size());
}

/**
 * Invoked by a ResizeObserver when the main component node
 * is resized due to layout changes.
 */
static void resize(float containerHeight, float scrollerHeight, CheckboxListState& state) {
	state.scroll = (containerHeight - scrollerHeight) * state.scrollRel;
	state.slotCount = static_cast<int>(std::floor(containerHeight / 26.0f));
}

/**
 * Restricts the scroll offset to prevent overflowing and
 * calculates the relative (0-1) offset based on the scroll.
 *
 * Note: JS does `this.scrollRel = this.scroll / max` which produces Infinity/NaN
 * when max === 0. We guard against division by zero here because NaN/Infinity
 * would break downstream arithmetic in C++ (unlike JS where it silently propagates).
 * When max === 0, all items fit in view, so scrollRel = 0 is correct.
 */
static void recalculateBounds(float containerHeight, float scrollerHeight, CheckboxListState& state) {
	const float max = containerHeight - scrollerHeight;
	state.scroll = std::min(max, std::max(0.0f, state.scroll));
	state.scrollRel = (max > 0.0f) ? (state.scroll / max) : 0.0f;
}

/**
 * Invoked when a mouse-down event is captured on the scroll widget.
 * @param {MouseEvent} e
 */
static void startMouse(float mouseY, CheckboxListState& state) {
	state.scrollStartY = mouseY;
	state.scrollStart = state.scroll;
	state.isScrolling = true;
}

/**
 * Invoked when a mouse-move event is captured globally.
 * @param {MouseEvent} e
 */
static void moveMouse(float mouseY, float containerHeight, float scrollerHeight, CheckboxListState& state) {
	if (state.isScrolling) {
		state.scroll = state.scrollStart + (mouseY - state.scrollStartY);
		recalculateBounds(containerHeight, scrollerHeight, state);
	}
}

/**
 * Invoked when a mouse-up event is captured globally.
 */
static void stopMouse(CheckboxListState& state) {
	state.isScrolling = false;
}

/**
 * Invoked when a mouse-wheel event is captured on the component node.
 * @param {WheelEvent} e
 */
static void wheelMouse(float wheelDelta, float containerHeight, float scrollerHeight, float itemHeight,
                        const std::vector<nlohmann::json>& items, CheckboxListState& state) {
	const float weight = containerHeight - scrollerHeight;

	if (itemHeight > 0.0f) {
		const int scrollCount = static_cast<int>(std::floor(containerHeight / itemHeight));
		const float direction = wheelDelta > 0.0f ? 1.0f : -1.0f;
		state.scroll += ((static_cast<float>(scrollCount) * itemWeight(items)) * weight) * direction;
		recalculateBounds(containerHeight, scrollerHeight, state);
	}
}

/**
 * Propagate entry clicks to the child checkbox.
 * @param {MouseEvent} event
 */
// loop below — clicking the row toggles the checkbox directly.

/**
 * HTML mark-up to render for this component.
 */
// template: converted to ImGui immediate-mode rendering below.

void render(const char* id, std::vector<nlohmann::json>& items, CheckboxListState& state) {
	ImGui::PushID(id);

	// Get the available content region as the container dimensions.
	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const float containerHeight = availSize.y;

	// JS dynamically queries child.clientHeight; CSS sets height: 26px on .item.
	// Use the ImGui equivalent: checkbox frame height with spacing.
	const float itemHeight = std::max(ImGui::GetFrameHeightWithSpacing(), 26.0f);
	const int totalItems = static_cast<int>(items.size());

	// The scroller thumb height is proportional to visible vs total items.
	const float scrollerHeight = (totalItems > 0)
		? std::max(20.0f, containerHeight * (static_cast<float>(state.slotCount) / static_cast<float>(totalItems)))
		: containerHeight;

	// Only call resize() when container/scroller dimensions actually change (ResizeObserver equivalent).
	if (state.prevContainerHeight != containerHeight || state.prevScrollerHeight != scrollerHeight || !state.initialized) {
		state.prevContainerHeight = containerHeight;
		state.prevScrollerHeight = scrollerHeight;
		state.initialized = true;
		resize(containerHeight, scrollerHeight, state);
	}

	// Compute display range.
	const int idx = scrollIndex(items, state);
	const int startIdx = std::max(0, idx);
	const int endIdx = std::min(totalItems, startIdx + state.slotCount);

	// Begin a child region to contain the list.
	ImGui::BeginChild("##checkboxlist_container", availSize, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

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

	// Draw the scrollbar on the right side.
	// CSS: .scroller { right: 3px; width: 8px; height: 45px; opacity: 0.7; }
	// CSS: .scroller > div { top: 3px; bottom: 3px; background: var(--font-primary); border-radius: 5px; }
	// CSS: .scroller:hover > div, .scroller.using > div { background: var(--font-highlight); }
	{
		const ImVec2 winPos = ImGui::GetWindowPos();
		const float scrollbarWidth = 8.0f;
		const float scrollbarX = winPos.x + availSize.x - scrollbarWidth - 3.0f; // right: 3px
		const float thumbY = winPos.y + scrollOffset(state);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Scroller outer bounds (for hit-testing).
		const ImVec2 thumbMin(scrollbarX, thumbY);
		const ImVec2 thumbMax(scrollbarX + scrollbarWidth, thumbY + scrollerHeight);

		// Inner div: inset top: 3px, bottom: 3px from outer scroller.
		const ImVec2 innerMin(scrollbarX, thumbY + 3.0f);
		const ImVec2 innerMax(scrollbarX + scrollbarWidth, thumbY + scrollerHeight - 3.0f);

		// Determine if mouse is over the thumb for hover effect.
		const bool thumbHovered = ImGui::IsMouseHoveringRect(thumbMin, thumbMax) || state.isScrolling;

		// CSS: default background: var(--font-primary) at 0.7 opacity
		// CSS: hover/using: background: var(--font-highlight) at 0.7 opacity
		const ImU32 baseColor = thumbHovered
			? app::theme::FONT_HIGHLIGHT_U32
			: app::theme::FONT_PRIMARY_U32;
		// Apply 0.7 opacity
		const ImU32 thumbColor = (baseColor & 0x00FFFFFF) |
			(static_cast<ImU32>(static_cast<float>((baseColor >> 24) & 0xFF) * 0.7f) << 24);

		drawList->AddRectFilled(innerMin, innerMax, thumbColor, app::theme::SCROLLBAR_ROUNDING);

		// Handle mouse-down on the scroller thumb.
		if (ImGui::IsMouseHoveringRect(thumbMin, thumbMax) && ImGui::IsMouseClicked(0)) {
			startMouse(io.MousePos.y, state);
		}
	}

	// Render visible items.
	// <div v-for="(item, i) in displayItems" class="item" @click="propagateClick($event)" :class="{ selected: item.checked }">
	//     <input type="checkbox" v-model="item.checked"/>
	//     <span>{{ item.label }}</span>
	// </div>
	for (int i = startIdx; i < endIdx; ++i) {
		auto& item = items[static_cast<size_t>(i)];
		bool checked = item.value("checked", false);
		const std::string label = item.value("label", std::string(""));

		// CSS: .item:nth-child(even) { background: var(--background-alt); }
		if ((i - startIdx) % 2 == 1) {
			const ImVec2 rowMin = ImGui::GetCursorScreenPos();
			const ImVec2 rowMax(rowMin.x + availSize.x - 10.0f, rowMin.y + itemHeight);
			ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, app::theme::BG_ALT_U32);
		}

		// CSS: .item.selected { background: var(--font-alt); }
		if (checked) {
			const ImVec2 rowMin = ImGui::GetCursorScreenPos();
			const ImVec2 rowMax(rowMin.x + availSize.x - 10.0f, rowMin.y + itemHeight);
			ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, app::theme::FONT_ALT_U32);
		}

		ImGui::PushID(i);

		// Checkbox + label — clicking anywhere on the row toggles the checkbox.
		if (ImGui::Checkbox(("##cb" + std::to_string(i)).c_str(), &checked)) {
			item["checked"] = checked;
		}
		ImGui::SameLine();
		// Clicking the label text also toggles the checkbox (propagateClick equivalent).
		if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(availSize.x - 30.0f, 0.0f))) {
			checked = !checked;
			item["checked"] = checked;
		}

		ImGui::PopID();
	}

	ImGui::EndChild();
	ImGui::PopID();
}

} // namespace checkboxlist