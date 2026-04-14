/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "resize-layer.h"

#include <imgui.h>

namespace resize_layer {

/**
 * Invoked when this component is mounted.
 * @see https://vuejs.org/v2/guide/instance.html
 */

/**
 * Invoked before this component is destroyed.
 * @see https://vuejs.org/v2/guide/instance.html
 */

// template: `<div><slot></slot></div>` — converted to ImGui below.

void render(const char* id, ResizeLayerState& state,
            const std::function<void(float)>& onResize,
            const std::function<void()>& contentCallback) {
	ImGui::PushID(id);

	// <div><slot></slot></div> — wrapping container equivalent.
	// JS uses a <div> wrapper observed by ResizeObserver. In ImGui, we use
	// BeginGroup/EndGroup as the wrapper to measure content region width.
	ImGui::BeginGroup();

	// Get the available content region width (equivalent of this.$el.clientWidth).
	// JS ResizeObserver uses integer clientWidth, so cast to int for safe comparison.
	const float currentWidth = ImGui::GetContentRegionAvail().x;
	const int currentWidthInt = static_cast<int>(currentWidth);
	const int prevWidthInt = static_cast<int>(state.prevWidth);

	// Emit 'resize' event when width changes (equivalent of ResizeObserver callback).
	if (currentWidthInt != prevWidthInt) {
		state.prevWidth = currentWidth;
		if (onResize)
			onResize(currentWidth);
	}

	// <slot></slot> — render child content.
	if (contentCallback)
		contentCallback();

	ImGui::EndGroup();

	ImGui::PopID();
}

} // namespace resize_layer