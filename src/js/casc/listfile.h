/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <utility>

namespace casc {

/**
 * Listfile module — master listfile management.
 *
 * JS equivalent: module.exports = { loadUnknowns, loadUnknownTextures,
 *   loadUnknownModels, preload, prepareListfile, applyPreload,
 *   existsByID, getByID, getByFilename, getFilenamesByExtension,
 *   getFilteredEntries, getByIDOrUnknown, stripFileEntry, parseFileEntry,
 *   formatEntries, formatUnknownFile, ingestIdentifiedFiles, isLoaded,
 *   addEntry, renderListfile }
 */
namespace listfile {

/**
 * Result struct for getFilteredEntries.
 * JS equivalent: { fileDataID, fileName }
 */
struct FilteredEntry {
	uint32_t fileDataID;
	std::string fileName;
};

/**
 * Result struct for parseFileEntry.
 * JS equivalent: { file_path, file_data_id }
 */
struct ParsedEntry {
	std::string file_path;
	std::optional<uint32_t> file_data_id;
};

/**
 * Extension filter that can optionally include an exclusion regex.
 * JS equivalent: string | [string, RegExp]
 * In JS, extension filters can be a plain string like ".blp" or a
 * tuple [".wmo", LISTFILE_MODEL_FILTER] where the regex excludes matches.
 */
struct ExtFilter {
	std::string ext;
	bool has_exclusion = false;

	ExtFilter(const std::string& e) : ext(e), has_exclusion(false) {}
	ExtFilter(const std::string& e, bool exclude) : ext(e), has_exclusion(exclude) {}
};

// --- Preloading ---

/**
 * Begin preloading the master listfile (binary or legacy format).
 * JS: preload()
 */
void preload();

/**
 * Prepare the listfile, waiting for any in-progress preload.
 * JS: prepareListfile()
 */
void prepareListfile();

/**
 * Apply preloaded listfile data, filtering against the root entries.
 * JS: applyPreload(rootEntries)
 * @param rootEntries Set of valid file data IDs from the CASC root.
 */
void applyPreload(const std::unordered_set<uint32_t>& rootEntries);

// --- Unknown file loading ---

/**
 * Load unknown textures from TextureFileData.db2.
 * JS: loadUnknownTextures()
 * @returns Number of unknown BLP textures added.
 */
size_t loadUnknownTextures();

/**
 * Load unknown models from ModelFileData.db2.
 * JS: loadUnknownModels()
 * @returns Number of unknown M2 models added.
 */
size_t loadUnknownModels();

/**
 * Load all unknown file types.
 * JS: loadUnknowns()
 */
void loadUnknowns();

// --- Lookup ---

/**
 * Check if a filename exists for a given file data ID.
 * JS: existsByID(id)
 */
bool existsByID(uint32_t id);

/**
 * Get a filename from a given file data ID.
 * JS: getByID(id)
 * @returns The filename or empty string if not found.
 */
std::string getByID(uint32_t id);

/**
 * Get a filename from a given file data ID or format it as an unknown file.
 * JS: getByIDOrUnknown(id, ext)
 */
std::string getByIDOrUnknown(uint32_t id, const std::string& ext = "");

/**
 * Get a file data ID by filename.
 * JS: getByFilename(filename)
 * @returns The file data ID or std::nullopt if not found.
 */
std::optional<uint32_t> getByFilename(const std::string& filename);

// --- Filtering ---

/**
 * Get filenames filtered by extension(s).
 * JS: getFilenamesByExtension(exts)
 * @param exts Single extension or list of extensions.
 * @returns Formatted listfile entries.
 */
std::vector<std::string> getFilenamesByExtension(const std::vector<ExtFilter>& exts);

/**
 * Get filtered entries matching a search term.
 * JS: getFilteredEntries(search)
 * @param search Search substring.
 * @param is_regex Whether to treat search as a regex.
 * @returns Vector of matching entries.
 */
std::vector<FilteredEntry> getFilteredEntries(const std::string& search, bool is_regex = false);

// --- Formatting ---

/**
 * Format file data IDs into display strings.
 * JS: formatEntries(file_data_ids)
 */
std::vector<std::string> formatEntries(std::vector<uint32_t>& file_data_ids);

/**
 * Returns a file path for an unknown fileDataID.
 * JS: formatUnknownFile(fileDataID, ext)
 */
std::string formatUnknownFile(uint32_t fileDataID, const std::string& ext = "");

/**
 * Strip a prefixed file ID from a listfile entry.
 * JS: stripFileEntry(entry)
 */
std::string stripFileEntry(const std::string& entry);

/**
 * Parse a listfile entry into path and file data ID.
 * JS: parseFileEntry(entry)
 */
ParsedEntry parseFileEntry(const std::string& entry);

// --- Mutation ---

/**
 * Ingest identified files into the legacy lookup.
 * JS: ingestIdentifiedFiles(entries)
 * @param entries Vector of (fileDataID, extension) pairs.
 */
void ingestIdentifiedFiles(const std::vector<std::pair<uint32_t, std::string>>& entries);

/**
 * Add a single entry to the listfile lookup tables.
 * JS: addEntry(fileDataID, fileName, listfile)
 * @param fileDataID File data ID.
 * @param fileName File name.
 * @param listfile Optional runtime listfile to append the formatted entry to.
 */
void addEntry(uint32_t fileDataID, const std::string& fileName,
              std::vector<std::string>* listfile = nullptr);

/**
 * Render listfile entries for display.
 * JS: renderListfile(file_data_ids, include_main_index)
 * @param file_data_ids Optional set of IDs to include (empty = all).
 * @param include_main_index Whether to include the main string index (binary mode).
 * @returns Formatted listfile strings.
 */
std::vector<std::string> renderListfile(const std::vector<uint32_t>& file_data_ids = {},
                                         bool include_main_index = false);

// --- State ---

/**
 * Returns true if a listfile has been loaded.
 * JS: isLoaded()
 */
bool isLoaded();

} // namespace listfile
} // namespace casc
