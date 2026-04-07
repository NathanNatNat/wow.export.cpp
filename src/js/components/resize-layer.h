/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <functional>
#include <imgui.h>

/**
 * Resize layer component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component that wraps child content and emits a 'resize'
 * event with the element's clientWidth whenever a ResizeObserver fires.
 *
 * In ImGui, we track the content region width each frame and invoke the
 * callback when it changes.
 */
namespace resize_layer {

/**
 * Persistent state for a single ResizeLayer widget instance.
 * Tracks the previously observed width to detect changes.
 */
struct ResizeLayerState {
	float prevWidth = 0.0f;
};

/**
 * Render the resize layer container using ImGui.
 *
 * The contentCallback renders the child content (equivalent of <slot>).
 * The onResize callback is invoked whenever the available width changes,
 * receiving the new width in pixels.
 *
 * @param id              Unique ImGui ID string for this widget instance.
 * @param state           Persistent state across frames.
 * @param onResize        Callback invoked when width changes; receives new width.
 * @param contentCallback Callback to render child content (equivalent of <slot></slot>).
 */
void render(const char* id, ResizeLayerState& state,
            const std::function<void(float)>& onResize,
            const std::function<void()>& contentCallback);

} // namespace resize_layer
