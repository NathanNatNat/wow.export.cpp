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
	const float sliderHeight = 20.0f;
	const float sliderWidth = availSize.x;
	const float handleWidth = 10.0f;

	// Begin a child region for the slider.
	ImGui::BeginChild("##slider_container", ImVec2(sliderWidth, sliderHeight), ImGuiChildFlags_None,
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

	// <div class="ui-slider" @click="handleClick">
	// Track background.
	const ImVec2 trackMin(winPos.x, winPos.y + sliderHeight * 0.3f);
	const ImVec2 trackMax(winPos.x + sliderWidth, winPos.y + sliderHeight * 0.7f);
	drawList->AddRectFilled(trackMin, trackMax, app::theme::SLIDER_TRACK_U32, 3.0f);

	// <div class="fill" :style="{ width: (modelValue * 100) + '%' }"></div>
	// Fill bar.
	const float fillWidth = sliderWidth * value;
	if (fillWidth > 0.0f) {
		const ImVec2 fillMin(winPos.x, winPos.y + sliderHeight * 0.3f);
		const ImVec2 fillMax(winPos.x + fillWidth, winPos.y + sliderHeight * 0.7f);
		drawList->AddRectFilled(fillMin, fillMax, app::theme::BUTTON_BASE_U32, 3.0f);
	}

	// <div class="handle" ref="handle" @mousedown="startMouse" :style="{ left: (modelValue * 100) + '%' }"></div>
	// Handle.
	const float handleX = winPos.x + fillWidth - handleWidth * 0.5f;
	const ImVec2 handleMin(handleX, winPos.y);
	const ImVec2 handleMax(handleX + handleWidth, winPos.y + sliderHeight);
	const bool handleHovered = ImGui::IsMouseHoveringRect(handleMin, handleMax) || state.isScrolling;
	const ImU32 handleColor = handleHovered
		? app::theme::SLIDER_THUMB_ACTIVE_U32
		: app::theme::SLIDER_THUMB_U32;
	drawList->AddRectFilled(handleMin, handleMax, handleColor, 3.0f);

	// Handle mouse-down on the handle.
	if (ImGui::IsMouseHoveringRect(handleMin, handleMax) && ImGui::IsMouseClicked(0)) {
		startMouse(io.MousePos.x, value, state);
	}

	// Click on the track (not the handle) — jump to position.
	// Don't handle click events on the draggable handle.
	if (!state.isScrolling && !ImGui::IsMouseHoveringRect(handleMin, handleMax) &&
	    ImGui::IsMouseHoveringRect(ImVec2(winPos.x, winPos.y), ImVec2(winPos.x + sliderWidth, winPos.y + sliderHeight)) &&
	    ImGui::IsMouseClicked(0)) {
		const float clickOffsetX = io.MousePos.x - winPos.x;
		const float newValue = handleClick(clickOffsetX, sliderWidth);
		if (onChange)
			onChange(newValue);
	}

	ImGui::EndChild();
	ImGui::PopID();
}

} // namespace slider