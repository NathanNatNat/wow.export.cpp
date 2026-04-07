/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "file-field.h"

#include <imgui.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#include <comdef.h>
#else
#include <cstdio>
#include <cstring>
#endif

namespace file_field {

// props: ['modelValue', 'placeholder']
// emits: ['update:modelValue']

/**
 * Invoked when the component is mounted.
 * Used to create an internal file node.
 */
// TODO(conversion): In C++, we use platform-native directory dialogs instead of
// DOM input elements. No mount/unmount lifecycle needed.

/**
 * Invoked when this component is unmounted.
 * Used to remove internal references to file node.
 */
// TODO(conversion): No explicit unmount needed — no DOM resources to clean up.

/**
 * Open a native directory picker dialog.
 * Returns the selected directory path, or empty string if cancelled.
 *
 * Equivalent to the JS file input with nwdirectory attribute.
 */
std::string openDirectoryDialog() {
#ifdef _WIN32
	std::string result;

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		IFileOpenDialog* pFileOpen = nullptr;
		hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
		                     IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
		if (SUCCEEDED(hr)) {
			// Set the dialog to pick folders.
			DWORD dwOptions;
			pFileOpen->GetOptions(&dwOptions);
			pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

			hr = pFileOpen->Show(nullptr);
			if (SUCCEEDED(hr)) {
				IShellItem* pItem = nullptr;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr)) {
					PWSTR pszFilePath = nullptr;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					if (SUCCEEDED(hr)) {
						// Convert wide string to UTF-8.
						int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, nullptr, 0, nullptr, nullptr);
						if (size_needed > 0) {
							result.resize(static_cast<size_t>(size_needed - 1));
							WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &result[0], size_needed, nullptr, nullptr);
						}
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}

	return result;
#else
	// Linux: Use zenity for directory selection.
	std::string result;
	FILE* pipe = popen("zenity --file-selection --directory 2>/dev/null", "r");
	if (pipe) {
		char buffer[4096];
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
			result += buffer;
		}
		pclose(pipe);

		// Remove trailing newline.
		while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
			result.pop_back();
	}

	return result;
#endif
}

/**
 * openDialog method.
 * Wipe the value here so that it fires after user interaction
 * even if they pick the "same" directory.
 */
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
			IM_COL32(255, 255, 255, 100),
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