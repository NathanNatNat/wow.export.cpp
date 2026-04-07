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
// TODO(conversion): In ImGui, ResizeObserver is not needed. We check
// the content region width each frame and fire the callback on change.

/**
 * Invoked before this component is destroyed.
 * @see https://vuejs.org/v2/guide/instance.html
 */
// TODO(conversion): No explicit unmount needed in ImGui immediate mode.

// template: `<div><slot></slot></div>` — converted to ImGui below.

void render(const char* id, ResizeLayerState& state,
            const std::function<void(float)>& onResize,
            const std::function<void()>& contentCallback) {
	ImGui::PushID(id);

	// Get the available content region width (equivalent of this.$el.clientWidth).
	const float currentWidth = ImGui::GetContentRegionAvail().x;

	// Emit 'resize' event when width changes (equivalent of ResizeObserver callback).
	if (currentWidth != state.prevWidth) {
		state.prevWidth = currentWidth;
		if (onResize)
			onResize(currentWidth);
	}

	// <div><slot></slot></div> — render child content.
	if (contentCallback)
		contentCallback();

	ImGui::PopID();
}

} // namespace resize_layer