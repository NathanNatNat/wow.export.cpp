/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <functional>

/**
 * File field component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['modelValue', 'placeholder'],
 * emits: ['update:modelValue']. Renders a text input that opens a native
 * directory picker dialog when focused/clicked.
 *
 * In C++, the directory picker uses platform-specific APIs:
 * - Windows: IFileOpenDialog (COM)
 * - Linux: zenity or kdialog via popen
 */
namespace file_field {

/**
 * Persistent state for a single FileField widget instance.
 */
struct FileFieldState {
	// Internal buffer for text input editing.
	std::string inputBuffer;
	bool bufferInitialized = false;
};

/**
 * Open a native directory picker dialog.
 * Returns the selected directory path, or empty string if cancelled.
 *
 * Equivalent to the JS file input with nwdirectory attribute.
 */
std::string openDirectoryDialog();

/**
 * Render a file/directory field using ImGui.
 *
 * @param id          Unique ImGui ID string for this widget instance.
 * @param modelValue  Current directory path value.
 * @param placeholder Placeholder text shown when empty.
 * @param state       Persistent state across frames.
 * @param onChange    Callback invoked when the value changes; receives new path.
 */
void render(const char* id, const std::string& modelValue, const char* placeholder,
            FileFieldState& state, const std::function<void(const std::string&)>& onChange);

} // namespace file_field
