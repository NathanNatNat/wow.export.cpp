/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include <nlohmann/json_fwd.hpp>

/**
 * Listbox context menu helper.
 *
 * Provides parsing, clipboard, file-explorer, and context menu
 * operations for selection entries in the file listbox.
 *
 * JS equivalent: module.exports = { parse_entry, get_file_paths,
 *     get_listfile_entries, get_file_data_ids, get_export_paths,
 *     get_export_directory, has_file_data_ids, copy_file_paths,
 *     copy_listfile_format, copy_file_data_ids, copy_export_paths,
 *     open_export_directory, handle_context_menu, close_context_menu }
 */
namespace listbox_context {

/**
 * Parsed entry result.
 * JS equivalent: { filePath: string, fileDataID: number|null }
 */
struct ParsedEntry {
	std::string filePath;
	std::optional<uint32_t> fileDataID;
};

/**
 * Parse a file entry to extract file path and file data ID.
 * JS equivalent: parse_entry(entry)
 * @param entry File entry in format "path/to/file [123]" or just "path/to/file"
 * @returns ParsedEntry with filePath and optional fileDataID.
 */
ParsedEntry parse_entry(const std::string& entry);

/**
 * Get file paths from selection entries.
 * JS equivalent: get_file_paths(selection)
 * @param selection List of entry strings.
 * @returns List of stripped file paths.
 */
std::vector<std::string> get_file_paths(const std::vector<std::string>& selection);

/**
 * Get file entries in listfile format (path;fileDataID).
 * JS equivalent: get_listfile_entries(selection)
 * @param selection List of entry strings.
 * @returns List of listfile-formatted strings.
 */
std::vector<std::string> get_listfile_entries(const std::vector<std::string>& selection);

/**
 * Get file data IDs from selection entries.
 * JS equivalent: get_file_data_ids(selection)
 * @param selection List of entry strings.
 * @returns List of file data IDs (entries without IDs are omitted).
 */
std::vector<uint32_t> get_file_data_ids(const std::vector<std::string>& selection);

/**
 * Get export paths for selection entries.
 * JS equivalent: get_export_paths(selection)
 * @param selection List of entry strings.
 * @returns List of export paths.
 */
std::vector<std::string> get_export_paths(const std::vector<std::string>& selection);

/**
 * Get export directory for the first selected entry.
 * JS equivalent: get_export_directory(selection)
 * @param selection List of entry strings.
 * @returns Export directory path, or nullopt if no selection.
 */
std::optional<std::string> get_export_directory(const std::vector<std::string>& selection);

/**
 * Check if selection has file data IDs.
 * JS equivalent: has_file_data_ids(selection)
 * @param selection List of entry strings.
 * @returns True if the first entry has a file data ID.
 */
bool has_file_data_ids(const std::vector<std::string>& selection);

/**
 * Copy file paths to clipboard.
 * JS equivalent: copy_file_paths(selection)
 * @param selection List of entry strings.
 */
void copy_file_paths(const std::vector<std::string>& selection);

/**
 * Copy file entries in listfile format to clipboard.
 * JS equivalent: copy_listfile_format(selection)
 * @param selection List of entry strings.
 */
void copy_listfile_format(const std::vector<std::string>& selection);

/**
 * Copy file data IDs to clipboard.
 * JS equivalent: copy_file_data_ids(selection)
 * @param selection List of entry strings.
 */
void copy_file_data_ids(const std::vector<std::string>& selection);

/**
 * Copy export paths to clipboard.
 * JS equivalent: copy_export_paths(selection)
 * @param selection List of entry strings.
 */
void copy_export_paths(const std::vector<std::string>& selection);

/**
 * Open export directory in file explorer.
 * JS equivalent: open_export_directory(selection)
 * @param selection List of entry strings.
 */
void open_export_directory(const std::vector<std::string>& selection);

/**
 * Handle context menu event from listbox.
 * JS equivalent: handle_context_menu(data, isLegacy)
 * @param data Context menu event data { item, selection, event }.
 * @param isLegacy If true, this is a legacy (MPQ) tab without file data IDs.
 */
void handle_context_menu(const nlohmann::json& data, bool isLegacy = false);

/**
 * Compatibility overload for existing C++ call-sites that pass a selection list.
 * Internally adapts to the JS-style data object contract.
 */
void handle_context_menu(const std::vector<std::string>& selection, bool isLegacy = false);

/**
 * Close the context menu.
 * JS equivalent: close_context_menu()
 */
void close_context_menu();

} // namespace listbox_context
