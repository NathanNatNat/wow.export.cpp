/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <functional>

/**
 * Slider component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['modelValue'],
 * emits: ['update:modelValue']. Renders a horizontal slider with a
 * draggable handle and clickable track, value between 0 and 1.
 */
namespace slider {

/**
 * Persistent state for a single Slider widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct SliderState {
	bool isScrolling = false;

	// Mouse drag tracking (equivalent to JS instance vars set in startMouse).
	float scrollStartX = 0.0f;
	float scrollStart = 0.0f;
};

/**
 * Render a horizontal slider using ImGui.
 *
 * @param id       Unique ImGui ID string for this widget instance.
 * @param value    Current slider value (0-1).
 * @param state    Persistent state across frames.
 * @param onChange Callback invoked when value changes; receives new value (clamped 0-1).
 */
void render(const char* id, float value, SliderState& state,
            const std::function<void(float)>& onChange);

} // namespace slider
