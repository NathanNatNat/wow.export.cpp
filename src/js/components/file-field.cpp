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

/**
 * Open a native directory picker dialog.
 * Returns the selected directory path, or empty string if cancelled.
 * Uses portable-file-dialogs — no system dependency on zenity or COM/IFileDialog.
 * Equivalent to the JS file input with nwdirectory attribute.
 */
std::string openDirectoryDialog() {
	return pfd::select_folder("Select Directory").result();
}

/**
 * Open a native file open dialog.
 * Returns the selected file path, or empty string if cancelled.
 * Uses portable-file-dialogs — no system dependency on zenity or COM/IFileDialog.
 */
std::string openFileDialog(const std::string& filter_desc, const std::string& filter_ext,
                           const std::string& default_dir) {
	std::vector<std::string> filters;
	if (!filter_desc.empty() && !filter_ext.empty()) {
		filters.push_back(filter_desc);
		filters.push_back(filter_ext);
	}
	auto result = pfd::open_file("Open File", default_dir, filters).result();
	return result.empty() ? "" : result[0];
}

/**
 * Open a native file save dialog.
 * Returns the selected file path, or empty string if cancelled.
 * Uses portable-file-dialogs — no system dependency on zenity or COM/IFileDialog.
 */
std::string saveFileDialog(const std::string& default_name, const std::string& filter_desc,
                           const std::string& filter_ext, const std::string& default_dir) {
	std::vector<std::string> filters;
	if (!filter_desc.empty() && !filter_ext.empty()) {
		filters.push_back(filter_desc);
		filters.push_back(filter_ext);
	}
	std::string default_path = default_dir;
	if (!default_path.empty() && !default_name.empty())
		default_path += "/" + default_name;
	else if (!default_name.empty())
		default_path = default_name;
	return pfd::save_file("Save File", default_path, filters).result();
}
static void openDialog(FileFieldState& state, const std::function<void(const std::string&)>& onChange) {
	std::string selected = openDirectoryDialog();
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

	ImGui::SetNextItemWidth(-40.0f);

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

	// Show placeholder when empty and not focused.
	if (state.inputBuffer.empty() && !ImGui::IsItemActive() && placeholder) {
		const ImVec2 textPos = ImGui::GetItemRectMin();
		ImGui::GetWindowDrawList()->AddText(
			ImVec2(textPos.x + 4.0f, textPos.y + 2.0f),
			app::theme::FIELD_PLACEHOLDER_U32,
			placeholder
		);
	}

	// Browse button — @focus="openDialog"
	ImGui::SameLine();
	if (ImGui::Button("...", ImVec2(30.0f, 0.0f))) {
		openDialog(state, onChange);
	}

	ImGui::PopID();
}

} // namespace file_field