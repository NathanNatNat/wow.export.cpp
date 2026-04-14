/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "combobox.h"

#include <imgui.h>
#include "../../app.h"
#include <algorithm>
#include <cctype>

namespace combobox {

// props: ['value', 'source', 'placeholder', 'maxheight']
// emits: ['update:value']

/**
 * Equivalent to the JS computed property filteredSource.
 */
static std::vector<const nlohmann::json*> filteredSource(const std::string& currentText,
                                                          const std::vector<nlohmann::json>& source,
                                                          int maxheight) {
	std::string currentTextLower = currentText;
	std::transform(currentTextLower.begin(), currentTextLower.end(), currentTextLower.begin(),
	               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	std::vector<const nlohmann::json*> items;
	for (const auto& item : source) {
		std::string labelLower = item.value("label", std::string(""));
		std::transform(labelLower.begin(), labelLower.end(), labelLower.begin(),
		               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (labelLower.find(currentTextLower) == 0) {
			items.push_back(&item);
		}
	}

	if (maxheight > 0 && static_cast<int>(items.size()) > maxheight)
		items.resize(static_cast<size_t>(maxheight));

	return items;
}

/**
 * Select an option from the dropdown.
 * Equivalent to the JS selectOption() method.
 */
static void selectOption(const nlohmann::json* option, const nlohmann::json& currentValue,
                          ComboBoxState& state,
                          const std::function<void(const nlohmann::json&)>& onChange) {
	if (!option) {
		state.currentText = "";
		onChange(nlohmann::json(nullptr));
		return;
	}

	state.currentText = option->value("label", std::string(""));
	state.isActive = false;

	if (!currentValue.contains("value") || !option->contains("value") ||
	    currentValue["value"] != (*option)["value"])
		onChange(*option);
}

/**
 * Equivalent to the JS onEnter() method.
 */
static void onEnter(ComboBoxState& state, const nlohmann::json& currentValue,
                     const std::vector<nlohmann::json>& source,
                     int maxheight, const std::function<void(const nlohmann::json&)>& onChange) {
	state.isActive = false;
	auto matches = filteredSource(state.currentText, source, maxheight);
	if (!matches.empty()) {
		selectOption(matches[0], currentValue, state, onChange);
	} else {
		state.currentText = "";
		onChange(nlohmann::json(nullptr));
	}
}

/**
 * Watch handler for value prop changes.
 * Equivalent to the JS watch: { value: function(newValue) { ... } }.
 *
 * JS calls this.selectOption(this.source.find(item => item.value === newValue.value))
 * which may emit update:value if no matching option is found (emits null via selectOption).
 */
static void watchValue(const nlohmann::json& value, const std::vector<nlohmann::json>& source,
                        ComboBoxState& state, const std::function<void(const nlohmann::json&)>& onChange) {
	if (value != state.prevValue) {
		state.prevValue = value;

		if (!value.is_null()) {
			// Find the matching option in source.
			const nlohmann::json* found = nullptr;
			for (const auto& item : source) {
				if (item.contains("value") && value.contains("value") && item["value"] == value["value"]) {
					found = &item;
					break;
				}
			}
			// Call selectOption to match JS behavior (emits null if not found).
			selectOption(found, value, state, onChange);
		} else {
			state.currentText = "";
		}
	}
}

/** HTML mark-up to render for this component. */
// template: converted to ImGui immediate-mode rendering below.

void render(const char* id, const nlohmann::json& value, const std::vector<nlohmann::json>& source,
            const char* placeholder, int maxheight, ComboBoxState& state,
            const std::function<void(const nlohmann::json&)>& onChange) {
	ImGui::PushID(id);

	// mounted() equivalent: initialize on first render.
	// JS: if (this.value !== null) this.selectOption(this.source.find(...)); else this.currentText = '';
	if (!state.initialized) {
		state.initialized = true;
		if (!value.is_null()) {
			const nlohmann::json* found = nullptr;
			for (const auto& item : source) {
				if (item.contains("value") && value.contains("value") && item["value"] == value["value"]) {
					found = &item;
					break;
				}
			}
			selectOption(found, value, state, onChange);
		} else {
			state.currentText = "";
		}
		state.prevValue = value;
	}

	// Watch: value prop change detection.
	watchValue(value, source, state, onChange);

	// <div class="ui-combobox">
	// <input type="text" :placeholder="placeholder" v-model="currentText" @blur="onBlur" @focus="onFocus" ref="field" @keyup.enter="onEnter"/>

	ImGui::SetNextItemWidth(-FLT_MIN);
	bool inputActive = false;

	// Ensure currentText has capacity for InputText to write into.
	// Reserve at least some space to avoid UB when string is empty.
	if (state.currentText.capacity() < 256)
		state.currentText.reserve(256);

	if (ImGui::InputText("##input", state.currentText.data(), state.currentText.capacity() + 1,
	                     ImGuiInputTextFlags_CallbackResize,
	                     [](ImGuiInputTextCallbackData* data) -> int {
	                         if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
	                             auto* str = static_cast<std::string*>(data->UserData);
	                             str->resize(static_cast<size_t>(data->BufTextLen));
	                             data->Buf = str->data();
	                         }
	                         return 0;
	                     },
	                     &state.currentText)) {
		// Text changed — mark active.
		state.isActive = true;
	}

	// Show placeholder when empty and not focused.
	// Use ImGui item rect for accurate positioning (not hardcoded offsets).
	if (state.currentText.empty() && !ImGui::IsItemActive()) {
		const ImVec2 itemMin = ImGui::GetItemRectMin();
		const ImVec2 itemMax = ImGui::GetItemRectMax();
		const float textY = itemMin.y + (itemMax.y - itemMin.y - ImGui::GetTextLineHeight()) * 0.5f;
		ImGui::GetWindowDrawList()->AddText(
			ImVec2(itemMin.x + ImGui::GetStyle().FramePadding.x, textY),
			app::theme::FIELD_PLACEHOLDER_U32,
			placeholder
		);
	}

	// onFocus equivalent.
	if (ImGui::IsItemActivated()) {
		state.isActive = true;
	}

	// Handle Enter key.
	if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
		onEnter(state, value, source, maxheight, onChange);
	}

	inputActive = ImGui::IsItemActive();

	// onBlur equivalent — JS uses setTimeout(() => { this.isActive = false; }, 200)
	// to delay deactivation by 200ms, allowing dropdown clicks to register.
	// We use a frame counter (~12 frames at 60fps ≈ 200ms).
	if (ImGui::IsItemDeactivated() && !inputActive) {
		state.blurDelayFrames = 12;
	}

	// <ul v-if="isActive && currentText.length > 0">
	//     <li v-for="item in filteredSource" @click="selectOption(item)">{{ item.label }}</li>
	// </ul>
	if (state.isActive && !state.currentText.empty()) {
		auto matches = filteredSource(state.currentText, source, maxheight);
		if (!matches.empty()) {
			// CSS: <ul> has no explicit max-height in template; controlled by CSS.
			// Use calculated height based on item count (no arbitrary cap).
			const float dropdownHeight = static_cast<float>(matches.size()) * ImGui::GetTextLineHeightWithSpacing();

			ImGui::BeginChild("##dropdown", ImVec2(0.0f, dropdownHeight),
			                  ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);

			for (const auto* item : matches) {
				const std::string label = item->value("label", std::string(""));
				if (ImGui::Selectable(label.c_str())) {
					selectOption(item, value, state, onChange);
					state.blurDelayFrames = 0; // Cancel pending blur
				}
			}

			// Keep active while hovering dropdown.
			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None)) {
				inputActive = true;
				state.blurDelayFrames = 0; // Cancel pending blur while hovering dropdown
			}

			ImGui::EndChild();
		}
	}

	// Deactivate after blur delay expires (200ms equivalent).
	if (state.blurDelayFrames > 0) {
		state.blurDelayFrames--;
		if (state.blurDelayFrames == 0 && !inputActive) {
			state.isActive = false;
		}
	}

	ImGui::PopID();
}

} // namespace combobox