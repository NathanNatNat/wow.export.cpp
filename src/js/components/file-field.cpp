/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "file-field.h"

#include <imgui.h>
#include "../../app.h"
#include <string>

#include <portable-file-dialogs.h>

namespace file_field {

// props: ['modelValue', 'placeholder']
// emits: ['update:modelValue']

/**
 * Invoked when the component is mounted.
 * Used to create an internal file node.
 */

/**
 * Invoked when this component is unmounted.
 * Used to remove internal references to file node.
 */

static void openDialog(FileFieldState& state, const std::function<void(const std::string&)>& onChange) {
	std::string selected = pfd::select_folder("Select Directory").result();
	if (!selected.empty()) {
		state.inputBuffer = selected;
		if (onChange)
			onChange(selected);
	}
}

/**
 * HTML mark-up to render for this component.
 */
// template: `<input type="text" :value="modelValue" :placeholder="placeholder"
//            @focus="openDialog" @input="$emit('update:modelValue', $event.target.value)"/>`
// Converted to ImGui below.

void render(const char* id, const std::string& modelValue, const char* placeholder,
            FileFieldState& state, const std::function<void(const std::string&)>& onChange) {
	ImGui::PushID(id);

	// Sync internal buffer with external value when not actively editing.
	if (!state.bufferInitialized || state.inputBuffer != modelValue) {
		state.inputBuffer = modelValue;
		state.bufferInitialized = true;
	}

	ImGui::SetNextItemWidth(-1.0f);

	// Text input — @input="$emit('update:modelValue', $event.target.value)"
	if (ImGui::InputText("##filefield", &state.inputBuffer[0], state.inputBuffer.capacity() + 1,
	                     ImGuiInputTextFlags_CallbackResize,
	                     [](ImGuiInputTextCallbackData* data) -> int {
	                         if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
	                             auto* str = static_cast<std::string*>(data->UserData);
	                             str->resize(static_cast<size_t>(data->BufTextLen));
	                             data->Buf = &(*str)[0];
	                         }
	                         return 0;
	                     },
	                     &state.inputBuffer)) {
		if (onChange)
			onChange(state.inputBuffer);
	}

	// @focus="openDialog" — fire on the rising-edge frame the input becomes active.
	if (ImGui::IsItemActivated())
		openDialog(state, onChange);

	// Show placeholder when empty and not focused.
	if (state.inputBuffer.empty() && !ImGui::IsItemActive() && placeholder) {
		const ImVec2 textPos = ImGui::GetItemRectMin();
		ImGui::GetWindowDrawList()->AddText(
			ImVec2(textPos.x + 4.0f, textPos.y + 2.0f),
			ImGui::GetColorU32(ImGuiCol_TextDisabled),
			placeholder
		);
	}

	ImGui::PopID();
}

} // namespace file_field
