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
 * Open a native file open dialog.
 * Returns the selected file path, or empty string if cancelled.
 *
 * @param filter_desc  Description for the file filter (e.g., "JSON Files").
 * @param filter_ext   Extension filter (e.g., "*.json"). Empty for all files.
 * @param default_dir  Default directory to open. Empty for system default.
 */
std::string openFileDialog(const std::string& filter_desc = "", const std::string& filter_ext = "",
                           const std::string& default_dir = "");

/**
 * Open a native file save dialog.
 * Returns the selected file path, or empty string if cancelled.
 *
 * @param default_name Default file name (e.g., "character.json").
 * @param filter_desc  Description for the file filter (e.g., "JSON Files").
 * @param filter_ext   Extension filter (e.g., "*.json"). Empty for all files.
 * @param default_dir  Default directory to open. Empty for system default.
 */
std::string saveFileDialog(const std::string& default_name = "", const std::string& filter_desc = "",
                           const std::string& filter_ext = "", const std::string& default_dir = "");

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
