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

void render(const char* id, ResizeLayerState& state,
            const std::function<void(float)>& onResize,
            const std::function<void()>& contentCallback) {
	ImGui::PushID(id);

	ImGui::BeginGroup();

	const float currentWidth = ImGui::GetContentRegionAvail().x;
	const int currentWidthInt = static_cast<int>(currentWidth);
	const int prevWidthInt = static_cast<int>(state.prevWidth);

	if (currentWidthInt != prevWidthInt) {
		state.prevWidth = currentWidth;
		if (onResize)
			onResize(currentWidth);
	}

	if (contentCallback)
		contentCallback();

	ImGui::EndGroup();

	ImGui::PopID();
}

} // namespace resize_layer