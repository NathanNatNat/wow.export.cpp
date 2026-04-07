/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "listbox-context.h"
#include "../core.h"
#include "../casc/listfile.h"
#include "../casc/export-helper.h"

#include <filesystem>
#include <regex>
#include <sstream>

#include <nlohmann/json.hpp>
#include <imgui.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#endif

namespace fs = std::filesystem;

namespace listbox_context {

/**
 * Parse a file entry to extract file path and file data ID.
 * @param entry - File entry in format "path/to/file [123]" or just "path/to/file"
 * @returns ParsedEntry with filePath and optional fileDataID.
 */
ParsedEntry parse_entry(const std::string& entry) {
	const std::string file_path = casc::listfile::stripFileEntry(entry);

	std::optional<uint32_t> file_data_id;

	// Match [digits] at end of entry.
	static const std::regex fid_regex(R"(\[(\d+)\]$)");
	std::smatch match;
	if (std::regex_search(entry, match, fid_regex)) {
		file_data_id = static_cast<uint32_t>(std::stoul(match[1].str()));
	} else {
		file_data_id = casc::listfile::getByFilename(file_path);
	}

	return { file_path, file_data_id };
}

/**
 * Get file paths from selection entries.
 * @param selection
 * @returns List of stripped file paths.
 */
std::vector<std::string> get_file_paths(const std::vector<std::string>& selection) {
	std::vector<std::string> result;
	result.reserve(selection.size());
	for (const auto& entry : selection)
		result.push_back(casc::listfile::stripFileEntry(entry));
	return result;
}

/**
 * Get file entries in listfile format (path;fileDataID).
 * @param selection
 * @returns List of listfile-formatted strings.
 */
std::vector<std::string> get_listfile_entries(const std::vector<std::string>& selection) {
	std::vector<std::string> result;
	result.reserve(selection.size());
	for (const auto& entry : selection) {
		auto parsed = parse_entry(entry);
		if (parsed.fileDataID.has_value())
			result.push_back(parsed.filePath + ";" + std::to_string(parsed.fileDataID.value()));
		else
			result.push_back(parsed.filePath);
	}
	return result;
}

/**
 * Get file data IDs from selection entries.
 * @param selection
 * @returns List of file data IDs (entries without IDs are omitted).
 */
std::vector<uint32_t> get_file_data_ids(const std::vector<std::string>& selection) {
	std::vector<uint32_t> result;
	result.reserve(selection.size());
	for (const auto& entry : selection) {
		auto parsed = parse_entry(entry);
		if (parsed.fileDataID.has_value())
			result.push_back(parsed.fileDataID.value());
	}
	return result;
}

/**
 * Get export paths for selection entries.
 * @param selection
 * @returns List of export paths.
 */
std::vector<std::string> get_export_paths(const std::vector<std::string>& selection) {
	std::vector<std::string> result;
	result.reserve(selection.size());
	for (const auto& entry : selection) {
		const std::string file_path = casc::listfile::stripFileEntry(entry);
		result.push_back(casc::ExportHelper::getExportPath(file_path));
	}
	return result;
}

/**
 * Get export directory for the first selected entry.
 * @param selection
 * @returns Export directory path, or empty string if no selection.
 */
std::string get_export_directory(const std::vector<std::string>& selection) {
	if (selection.empty())
		return "";

	const std::string file_path = casc::listfile::stripFileEntry(selection[0]);
	const std::string export_path = casc::ExportHelper::getExportPath(file_path);
	return fs::path(export_path).parent_path().string();
}

/**
 * Copy file paths to clipboard.
 * @param selection
 */
void copy_file_paths(const std::vector<std::string>& selection) {
	const auto paths = get_file_paths(selection);
	std::ostringstream oss;
	for (size_t i = 0; i < paths.size(); ++i) {
		if (i > 0) oss << '\n';
		oss << paths[i];
	}
	ImGui::SetClipboardText(oss.str().c_str());
}

/**
 * Copy file entries in listfile format to clipboard.
 * @param selection
 */
void copy_listfile_format(const std::vector<std::string>& selection) {
	const auto entries = get_listfile_entries(selection);
	std::ostringstream oss;
	for (size_t i = 0; i < entries.size(); ++i) {
		if (i > 0) oss << '\n';
		oss << entries[i];
	}
	ImGui::SetClipboardText(oss.str().c_str());
}

/**
 * Copy file data IDs to clipboard.
 * @param selection
 */
void copy_file_data_ids(const std::vector<std::string>& selection) {
	const auto ids = get_file_data_ids(selection);
	std::ostringstream oss;
	for (size_t i = 0; i < ids.size(); ++i) {
		if (i > 0) oss << '\n';
		oss << ids[i];
	}
	ImGui::SetClipboardText(oss.str().c_str());
}

/**
 * Copy export paths to clipboard.
 * @param selection
 */
void copy_export_paths(const std::vector<std::string>& selection) {
	const auto paths = get_export_paths(selection);
	std::ostringstream oss;
	for (size_t i = 0; i < paths.size(); ++i) {
		if (i > 0) oss << '\n';
		oss << paths[i];
	}
	ImGui::SetClipboardText(oss.str().c_str());
}

/**
 * Open export directory in file explorer.
 * @param selection
 */
void open_export_directory(const std::vector<std::string>& selection) {
	const std::string dir = get_export_directory(selection);
	if (dir.empty())
		return;

#ifdef _WIN32
	const std::wstring wpath(dir.begin(), dir.end());
	ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	std::string cmd = "xdg-open \"" + dir + "\" &";
	std::system(cmd.c_str());
#endif
}

/**
 * Check if selection has file data IDs.
 * @param selection
 * @returns True if the first entry has a file data ID.
 */
bool has_file_data_ids(const std::vector<std::string>& selection) {
	if (selection.empty())
		return false;

	auto parsed = parse_entry(selection[0]);
	return parsed.fileDataID.has_value();
}

/**
 * Handle context menu event from listbox.
 * @param selection - List of selected entry strings.
 * @param isLegacy - If true, this is a legacy (MPQ) tab without file data IDs.
 */
void handle_context_menu(const std::vector<std::string>& selection, bool isLegacy) {
	nlohmann::json node;
	node["selection"] = selection;
	node["count"] = selection.size();
	node["hasFileDataIDs"] = !isLegacy && has_file_data_ids(selection);

	core::view->contextMenus.nodeListbox = std::move(node);
}

/**
 * Close the context menu.
 */
void close_context_menu() {
	core::view->contextMenus.nodeListbox = nullptr;
}

} // namespace listbox_context
