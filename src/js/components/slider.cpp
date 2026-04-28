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

// JS deviation: JS implements a fully custom slider (lines 41-99 of slider.js)
// with `mousedown`/`mousemove`/`mouseup` document listeners, a fill bar, and a
// draggable handle. C++ replaces all of that with native `ImGui::SliderFloat`,
// which handles drag/click/keyboard input internally. Per CLAUDE.md Visual
// Fidelity rules, exact appearance need not match — only the value-in-[0,1]
// semantic is preserved.
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
