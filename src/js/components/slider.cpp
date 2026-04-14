/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "slider.h"

#include <imgui.h>
#include "../../app.h"
#include <algorithm>
#include <cmath>

namespace slider {

/**
 * value: Slider value between 0 and 1.
 */
// props: ['modelValue']
// emits: ['update:modelValue']

// data: isScrolling — stored in SliderState

/**
 * Invoked when the component is mounted.
 * Used to register global mouse listeners.
 */

/**
 * Invoked when the component is destroyed.
 * Used to unregister global mouse listeners.
 */

/**
 * Set the current value of this slider.
 * @param {number} value
 */
static float setValue(float value) {
	return std::min(1.0f, std::max(0.0f, value));
}

/**
 * Invoked when a mouse-down event is captured on the slider handle.
 * @param {MouseEvent} e
 */
static void startMouse(float mouseX, float currentValue, SliderState& state) {
	state.scrollStartX = mouseX;
	state.scrollStart = currentValue;
	state.isScrolling = true;
}

/**
 * Invoked when a mouse-move event is captured globally.
 * @param {MouseEvent} e
 */
static float moveMouse(float mouseX, float containerWidth, SliderState& state) {
	if (state.isScrolling) {
		const float delta = mouseX - state.scrollStartX;
		return setValue(state.scrollStart + (delta / containerWidth));
	}
	return -1.0f; // No change
}

/**
 * Invoked when a mouse-up event is captured globally.
 */
static void stopMouse(SliderState& state) {
	state.isScrolling = false;
}

/**
 * Invoked when the user clicks somewhere on the slider.
 * @param {MouseEvent} e
 */
static float handleClick(float clickOffsetX, float containerWidth) {
	return setValue(clickOffsetX / containerWidth);
}

/**
 * HTML mark-up to render for this component.
 */
// template: converted to ImGui immediate-mode rendering below.

void render(const char* id, float value, SliderState& state,
            const std::function<void(float)>& onChange) {
	ImGui::PushID(id);

	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const float sliderHeight = 20.0f;   // CSS: .ui-slider { height: 20px; }
	const float handleHeight = 28.0f;   // CSS: .handle { height: 28px; }
	const float sliderWidth = availSize.x;
	const float handleWidth = 10.0f;    // CSS: .handle { width: 10px; }
	const float verticalOverhang = (handleHeight - sliderHeight) * 0.5f; // Handle extends beyond track

	// CSS: .ui-slider { margin: 4px 0; } — top margin
	ImGui::Dummy(ImVec2(0.0f, 4.0f));

	// Total height includes the handle overhang on both sides.
	const float totalHeight = handleHeight;
	const float trackOffsetY = verticalOverhang; // Track is vertically centered within total area

	// Begin a child region for the slider (sized to fit handle overhang).
	ImGui::BeginChild("##slider_container", ImVec2(sliderWidth, totalHeight), ImGuiChildFlags_None,
	                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	const ImVec2 winPos = ImGui::GetWindowPos();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	const ImGuiIO& io = ImGui::GetIO();

	// Handle global mouse move/up for handle dragging.
	if (state.isScrolling) {
		const float newValue = moveMouse(io.MousePos.x, sliderWidth, state);
		if (newValue >= 0.0f && onChange)
			onChange(newValue);
		if (!io.MouseDown[0])
			stopMouse(state);
	}

	// --- Track (CSS: .ui-slider) ---
	// Track background: full 20px height, vertically centered within the child region.
	const ImVec2 trackMin(winPos.x, winPos.y + trackOffsetY);
	const ImVec2 trackMax(winPos.x + sliderWidth, winPos.y + trackOffsetY + sliderHeight);

	// CSS: box-shadow: black 0 0 1px — approximate with a slightly larger dark rect behind the track
	drawList->AddRectFilled(
		ImVec2(trackMin.x - 1.0f, trackMin.y - 1.0f),
		ImVec2(trackMax.x + 1.0f, trackMax.y + 1.0f),
		IM_COL32(0, 0, 0, 128));

	// Track background fill
	drawList->AddRectFilled(trackMin, trackMax, app::theme::SLIDER_TRACK_U32);

	// CSS: border: 1px solid var(--border)
	drawList->AddRect(trackMin, trackMax, app::theme::BORDER_U32, 0.0f, 0, 1.0f);

	// --- Fill bar (CSS: .ui-slider .fill) ---
	// CSS: position: absolute; top: 0; left: 0; bottom: 0; — fills full height of the track
	const float fillWidth = sliderWidth * value;
	if (fillWidth > 0.0f) {
		const ImVec2 fillMin(winPos.x, winPos.y + trackOffsetY);
		const ImVec2 fillMax(winPos.x + fillWidth, winPos.y + trackOffsetY + sliderHeight);
		drawList->AddRectFilled(fillMin, fillMax, app::theme::SLIDER_FILL_U32);
	}

	// --- Handle (CSS: .ui-slider .handle) ---
	// CSS: left: (modelValue * 100) + '%' — left edge at value position (no translateX centering)
	// CSS: top: 50%; transform: translateY(-50%) — vertically centered
	const float handleX = winPos.x + fillWidth;
	const ImVec2 handleMin(handleX, winPos.y);
	const ImVec2 handleMax(handleX + handleWidth, winPos.y + handleHeight);

	// CSS: .handle:hover — hover only when actually hovering, NOT during drag (TODO 420)
	const bool handleActuallyHovered = ImGui::IsMouseHoveringRect(handleMin, handleMax);

	// CSS: box-shadow: black 0 0 8px — approximate shadow behind the handle
	drawList->AddRectFilled(
		ImVec2(handleMin.x - 2.0f, handleMin.y - 2.0f),
		ImVec2(handleMax.x + 2.0f, handleMax.y + 2.0f),
		IM_COL32(0, 0, 0, 80));

	// Handle color: default = var(--border), hover = var(--font-alt)
	const ImU32 handleColor = handleActuallyHovered
		? app::theme::SLIDER_THUMB_ACTIVE_U32
		: app::theme::SLIDER_THUMB_U32;
	drawList->AddRectFilled(handleMin, handleMax, handleColor);

	// CSS: cursor: pointer — show hand cursor on handle hover (TODO 421)
	if (handleActuallyHovered)
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

	// Handle mouse-down on the handle — start dragging.
	if (handleActuallyHovered && ImGui::IsMouseClicked(0)) {
		startMouse(io.MousePos.x, value, state);
	}

	// Click on the track (not the handle) — jump to position.
	// JS: @click="handleClick" — fires on click (mousedown + mouseup on same element),
	// so we use IsMouseReleased instead of IsMouseClicked (TODO 419).
	// Don't handle click events on the draggable handle.
	const ImVec2 sliderAreaMin(winPos.x, winPos.y + trackOffsetY);
	const ImVec2 sliderAreaMax(winPos.x + sliderWidth, winPos.y + trackOffsetY + sliderHeight);
	if (!state.isScrolling && !handleActuallyHovered &&
	    ImGui::IsMouseHoveringRect(sliderAreaMin, sliderAreaMax) &&
	    ImGui::IsMouseReleased(0)) {
		const float clickOffsetX = io.MousePos.x - winPos.x;
		const float newValue = handleClick(clickOffsetX, sliderWidth);
		if (onChange)
			onChange(newValue);
	}

	ImGui::EndChild();

	// CSS: .ui-slider { margin: 4px 0; } — bottom margin
	ImGui::Dummy(ImVec2(0.0f, 4.0f));

	ImGui::PopID();
}

} // namespace slider