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
void render(const char* id, const std::string& modelValue, const char* placeholder,
            FileFieldState& state, const std::function<void(const std::string&)>& onChange) {
	ImGui::PushID(id);

	if (!state.bufferInitialized || state.inputBuffer != modelValue) {
		state.inputBuffer = modelValue;
		state.bufferInitialized = true;
	}

	ImGui::SetNextItemWidth(-1.0f);

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

	if (ImGui::IsItemActivated())
		openDialog(state, onChange);

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
