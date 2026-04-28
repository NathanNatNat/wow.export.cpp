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
 * Render a horizontal slider using ImGui.
 *
 * @param id       Unique ImGui ID string for this widget instance.
 * @param value    Current slider value (0-1).
 * @param onChange Callback invoked when value changes; receives new value (clamped 0-1).
 */
void render(const char* id, float value,
            const std::function<void(float)>& onChange);

} // namespace slider
