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
void render(const char* id, float value,
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
