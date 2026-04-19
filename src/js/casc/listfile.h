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
#include <regex>
#include <utility>
#include <future>
#include <variant>

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
 *
 * TODO 201: The exclusion regex is now stored in the struct, allowing
 * different exclusion patterns per extension (matching JS behavior).
 */
struct ExtFilter {
	std::string ext;
	bool has_exclusion = false;
	std::optional<std::regex> exclusion_regex;

	ExtFilter(const std::string& e) : ext(e), has_exclusion(false) {}
	ExtFilter(const std::string& e, const std::regex& regex)
		: ext(e), has_exclusion(true), exclusion_regex(regex) {}
};

// --- Preloading ---

/**
 * Begin preloading the master listfile (binary or legacy format).
 */
bool preload();

/**
 * Async-equivalent API mirroring JS Promise-based preload().
 */
std::shared_future<bool> preloadAsync();

/**
 * Prepare the listfile, waiting for any in-progress preload.
 */
bool prepareListfile();

/**
 * Async-equivalent API mirroring JS Promise-based prepareListfile().
 */
std::shared_future<bool> prepareListfileAsync();

/**
 * Apply preloaded listfile data, filtering against the root entries.
 * @param rootEntries Set of valid file data IDs from the CASC root.
 */
std::optional<int> applyPreload(const std::unordered_set<uint32_t>& rootEntries);

// --- Unknown file loading ---

/**
 * Load unknown textures from TextureFileData.db2.
 * @returns Number of unknown BLP textures added.
 */
size_t loadUnknownTextures();
std::future<size_t> loadUnknownTexturesAsync();

/**
 * Load unknown models from ModelFileData.db2.
 * @returns Number of unknown M2 models added.
 */
size_t loadUnknownModels();
std::future<size_t> loadUnknownModelsAsync();

/**
 * Load all unknown file types.
 */
void loadUnknowns();
std::future<void> loadUnknownsAsync();

// --- Lookup ---

/**
 * Check if a filename exists for a given file data ID.
 */
bool existsByID(uint32_t id);

/**
 * Get a filename from a given file data ID.
 * @returns The filename or empty string if not found.
 */
std::optional<std::string> getByID(uint32_t id);

/**
 * Get a filename from a given file data ID or format it as an unknown file.
 */
std::string getByIDOrUnknown(uint32_t id, const std::string& ext = "");

/**
 * Get a file data ID by filename.
 * @returns The file data ID or std::nullopt if not found.
 */
std::optional<uint32_t> getByFilename(const std::string& filename);

// --- Filtering ---

/**
 * Get filenames filtered by extension(s).
 * @param exts Single extension or list of extensions.
 * @returns Formatted listfile entries.
 */
std::vector<std::string> getFilenamesByExtension(const std::vector<ExtFilter>& exts);

/**
 * Get filtered entries matching a search term.
 * @param search Search substring.
 * @param is_regex Whether to treat search as a regex.
 * @returns Vector of matching entries.
 */
std::vector<FilteredEntry> getFilteredEntries(const std::string& search);
std::vector<FilteredEntry> getFilteredEntries(const std::regex& search);

// --- Formatting ---

/**
 * Format file data IDs into display strings.
 */
std::vector<std::string> formatEntries(std::vector<uint32_t>& file_data_ids);

/**
 * Returns a file path for an unknown fileDataID.
 */
std::string formatUnknownFile(uint32_t fileDataID, const std::string& ext = "");

/**
 * Strip a prefixed file ID from a listfile entry.
 */
std::string stripFileEntry(const std::string& entry);

/**
 * Parse a listfile entry into path and file data ID.
 */
ParsedEntry parseFileEntry(const std::string& entry);

// --- Mutation ---

/**
 * Ingest identified files into the legacy lookup.
 * @param entries Vector of (fileDataID, extension) pairs.
 */
void ingestIdentifiedFiles(const std::vector<std::pair<uint32_t, std::string>>& entries);

/**
 * Add a single entry to the listfile lookup tables.
 * @param fileDataID File data ID.
 * @param fileName File name.
 * @param listfile Optional runtime listfile to append the formatted entry to.
 */
void addEntry(uint32_t fileDataID, const std::string& fileName,
              std::vector<std::string>* listfile = nullptr);

/**
 * Render listfile entries for display.
 * @param file_data_ids Optional set of IDs to include (nullopt = all, empty = match nothing).
 * @param include_main_index Whether to include the main string index (binary mode).
 * @returns Formatted listfile strings.
 */
std::vector<std::string> renderListfile(const std::optional<std::vector<uint32_t>>& file_data_ids = std::nullopt,
                                         bool include_main_index = false);
std::future<std::vector<std::string>> renderListfileAsync(const std::optional<std::vector<uint32_t>>& file_data_ids = std::nullopt,
                                                          bool include_main_index = false);

// --- State ---

/**
 * Returns true if a listfile has been loaded.
 */
bool isLoaded();

} // namespace listfile
} // namespace casc
