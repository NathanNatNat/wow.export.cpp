/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "slider.h"

#include <imgui.h>

namespace slider {

/**
 * value: Slider value between 0 and 1.
 */
// props: ['modelValue']
// emits: ['update:modelValue']

/**
 * HTML mark-up to render for this component.
 * Replaced custom DrawList rendering with a native ImGui::SliderFloat widget.
 */
void render(const char* id, float value, SliderState& /*state*/,
            const std::function<void(float)>& onChange) {
	ImGui::PushID(id);
	ImGui::SetNextItemWidth(-FLT_MIN);
	float v = value;
	if (ImGui::SliderFloat("##slider", &v, 0.0f, 1.0f, "")) {
		if (onChange)
			onChange(v);
	}
	ImGui::PopID();
}

} // namespace slider
