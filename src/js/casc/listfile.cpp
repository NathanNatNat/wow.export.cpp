/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#include "listfile.h"
#include "export-helper.h"
#include "../generics.h"
#include "../constants.h"
#include "../core.h"
#include "../log.h"
#include "../buffer.h"
#include "../mmap.h"
#include "../hashing/xxhash64.h"

#include "../db/caches/DBTextureFileData.h"
#include "../db/caches/DBModelFileData.h"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <future>
#include <mutex>
#include <regex>
#include <chrono>
#include <cstring>
#include <format>
#include <array>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace casc {
namespace listfile {

namespace BIN_LF_COMPONENTS {
	static constexpr std::string_view ID_INDEX = "listfile-id-index.dat";
	static constexpr std::string_view STRINGS = "listfile-strings.dat";
	static constexpr std::string_view TREE_NODES = "listfile-tree-nodes.dat";
	static constexpr std::string_view PF_MODELS = "listfile-pf-models.dat";
	static constexpr std::string_view PF_TEXTURES = "listfile-pf-textures.dat";
	static constexpr std::string_view PF_SOUNDS = "listfile-pf-sounds.dat";
	static constexpr std::string_view PF_VIDEOS = "listfile-pf-videos.dat";
	static constexpr std::string_view PF_TEXT = "listfile-pf-text.dat";
	static constexpr std::string_view PF_FONTS = "listfile-pf-fonts.dat";
}

// All component file names in order (for iteration)
static constexpr std::array<std::string_view, 9> BIN_LF_COMPONENT_VALUES = {
	BIN_LF_COMPONENTS::ID_INDEX,
	BIN_LF_COMPONENTS::STRINGS,
	BIN_LF_COMPONENTS::TREE_NODES,
	BIN_LF_COMPONENTS::PF_MODELS,
	BIN_LF_COMPONENTS::PF_TEXTURES,
	BIN_LF_COMPONENTS::PF_SOUNDS,
	BIN_LF_COMPONENTS::PF_VIDEOS,
	BIN_LF_COMPONENTS::PF_TEXT,
	BIN_LF_COMPONENTS::PF_FONTS
};

static std::unordered_map<std::string, uint32_t> legacy_name_lookup;
static std::unordered_map<uint32_t, std::string> legacy_id_lookup;

static bool loaded = false;

static std::unordered_map<uint32_t, std::string> preloadedIdLookup;
static std::unordered_map<std::string, uint32_t> preloadedNameLookup;

static std::unordered_map<uint32_t, uint32_t> binary_id_to_offset;
static std::unordered_map<uint32_t, uint8_t> binary_id_to_pf_index;

static std::array<mmap_util::MmapObject*, 7> binary_strings_mmap = {};
static mmap_util::MmapObject* binary_tree_nodes_mmap = nullptr;

static bool is_binary_mode = false;

// In binary mode: Map<uint32_t, string>. In legacy mode: vector<uint32_t>.
static std::unordered_map<uint32_t, std::string> preload_textures_map;
static std::unordered_map<uint32_t, std::string> preload_sounds_map;
static std::unordered_map<uint32_t, std::string> preload_text_map;
static std::unordered_map<uint32_t, std::string> preload_fonts_map;
static std::unordered_map<uint32_t, std::string> preload_models_map;
static std::vector<uint32_t> preload_textures_order;
static std::vector<uint32_t> preload_sounds_order;
static std::vector<uint32_t> preload_text_order;
static std::vector<uint32_t> preload_fonts_order;
static std::vector<uint32_t> preload_models_order;

static std::vector<uint32_t> preload_textures_ids;
static std::vector<uint32_t> preload_sounds_ids;
static std::vector<uint32_t> preload_text_ids;
static std::vector<uint32_t> preload_fonts_ids;
static std::vector<uint32_t> preload_models_ids;

static bool is_preloaded = false;
static std::optional<std::shared_future<bool>> preload_future;
static std::mutex preload_mutex;

// --- Internal helper: getFileDataIDsByExtension (legacy mode) ---
static std::vector<uint32_t> getFileDataIDsByExtension(const std::vector<ExtFilter>& exts, std::string_view name) {
	std::vector<uint32_t> entries;

	std::vector<std::pair<uint32_t, std::string>> entriesArray(preloadedIdLookup.begin(), preloadedIdLookup.end());

	generics::batchWork<std::pair<uint32_t, std::string>>(name, entriesArray,
		[&](const std::pair<uint32_t, std::string>& item, size_t) {
			const auto& [fileDataID, filename] = item;
			for (const auto& ext : exts) {
				if (ext.has_exclusion && ext.exclusion_regex) {
					if (filename.ends_with(ext.ext) && !std::regex_search(filename, *ext.exclusion_regex)) {
						entries.push_back(fileDataID);
						break;
					}
				} else {
					if (filename.ends_with(ext.ext)) {
						entries.push_back(fileDataID);
						break;
					}
				}
			}
		}, 1000);

	return entries;
}

static std::shared_future<bool> makeReadySharedFuture(bool value) {
	std::promise<bool> promise;
	promise.set_value(value);
	return promise.get_future().share();
}

// --- Internal: listfile_check_cache_expiry ---
static bool listfile_check_cache_expiry(int64_t last_modified) {
	if (last_modified > 0) {
		int64_t ttl = 0;
		if (core::view->config.contains("listfileCacheRefresh"))
			ttl = core::view->config["listfileCacheRefresh"].get<int64_t>();

		ttl *= 24LL * 60 * 60 * 1000;

		if (ttl == 0) {
			logging::write(std::format("Cached listfile is out-of-date (> {}).", ttl));
			return true;
		}

		auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();

		if ((now - last_modified) > ttl) {
			logging::write(std::format("Cached listfile is out-of-date (> {}).", ttl));
			return true;
		}

		logging::write("Listfile is cached locally.");
		return false;
	}

	logging::write("Listfile is not cached, downloading fresh.");
	return true;
}

// --- Helper: get file last modified time in milliseconds ---
static int64_t getFileLastModifiedMs(const fs::path& file_path) {
	try {
		auto ftime = fs::last_write_time(file_path);
		// Convert file_time to system_clock then to epoch ms
		auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			sctp.time_since_epoch()).count();
	} catch (...) {
		return 0;
	}
}

// --- Internal: binary_read_string_at_offset ---
static std::string binary_read_string_at_offset(uint8_t pf_index, uint32_t offset) {
	const auto* mmap_obj = binary_strings_mmap[pf_index];
	const auto* data = static_cast<const uint8_t*>(mmap_obj->data);
	size_t end = offset;

	while (end < mmap_obj->size && data[end] != 0)
		end++;

	return std::string(reinterpret_cast<const char*>(data + offset), end - offset);
}

// --- Internal: listfile_binary_find_component_child ---
static int32_t listfile_binary_find_component_child(uint32_t node_ofs, std::string_view component_name) {
	const uint64_t target_hash = hashing::XXH64::hash(component_name);

	const auto* node_data = static_cast<const uint8_t*>(binary_tree_nodes_mmap->data);
	const auto* base = node_data + node_ofs;

	auto readU32BE = [](const uint8_t* p) -> uint32_t {
		return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
		       (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
	};
	auto readU64BE = [](const uint8_t* p) -> uint64_t {
		return (static_cast<uint64_t>(p[0]) << 56) | (static_cast<uint64_t>(p[1]) << 48) |
		       (static_cast<uint64_t>(p[2]) << 40) | (static_cast<uint64_t>(p[3]) << 32) |
		       (static_cast<uint64_t>(p[4]) << 24) | (static_cast<uint64_t>(p[5]) << 16) |
		       (static_cast<uint64_t>(p[6]) << 8) | static_cast<uint64_t>(p[7]);
	};

	const uint32_t child_count = readU32BE(base);
	constexpr uint32_t child_entries_ofs = 9;

	int32_t left = 0;
	int32_t right = static_cast<int32_t>(child_count) - 1;

	while (left <= right) {
		const int32_t mid = (left + right) / 2;
		const uint32_t entry_ofs = child_entries_ofs + (mid * 12);

		const uint64_t child_hash = readU64BE(base + entry_ofs);

		if (child_hash == target_hash)
			return static_cast<int32_t>(readU32BE(base + entry_ofs + 8));

		if (child_hash < target_hash)
			left = mid + 1;
		else
			right = mid - 1;
	}

	return -1;
}

// --- Internal: listfile_binary_find_file ---
static std::optional<uint32_t> listfile_binary_find_file(uint32_t node_ofs, std::string_view filename) {
	const auto* node_data = static_cast<const uint8_t*>(binary_tree_nodes_mmap->data);
	const auto* base = node_data + node_ofs;

	auto readU32BE = [](const uint8_t* p) -> uint32_t {
		return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
		       (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
	};
	auto readU64BE = [](const uint8_t* p) -> uint64_t {
		return (static_cast<uint64_t>(p[0]) << 56) | (static_cast<uint64_t>(p[1]) << 48) |
		       (static_cast<uint64_t>(p[2]) << 40) | (static_cast<uint64_t>(p[3]) << 32) |
		       (static_cast<uint64_t>(p[4]) << 24) | (static_cast<uint64_t>(p[5]) << 16) |
		       (static_cast<uint64_t>(p[6]) << 8) | static_cast<uint64_t>(p[7]);
	};
	auto readU16BE = [](const uint8_t* p) -> uint16_t {
		return (static_cast<uint16_t>(p[0]) << 8) | static_cast<uint16_t>(p[1]);
	};

	const uint32_t child_count = readU32BE(base);
	const uint32_t file_count = readU32BE(base + 4);
	const bool is_large_dir = base[8] == 1;

	if (file_count == 0)
		return std::nullopt;

	const uint32_t file_entries_start = 9 + (child_count * 12);

	if (is_large_dir) {
		const uint64_t target_hash = hashing::XXH64::hash(filename);

		int32_t left = 0;
		int32_t right = static_cast<int32_t>(file_count) - 1;

		while (left <= right) {
			const int32_t mid = (left + right) / 2;
			const uint32_t entry_ofs = file_entries_start + (mid * 12);

			const uint64_t file_hash = readU64BE(base + entry_ofs);

			if (file_hash == target_hash)
				return readU32BE(base + entry_ofs + 8);

			if (file_hash < target_hash)
				left = mid + 1;
			else
				right = mid - 1;
		}
	} else {
		uint32_t pos = file_entries_start;
		for (uint32_t i = 0; i < file_count; i++) {
			const uint16_t filename_len = readU16BE(base + pos);
			pos += 2;

			std::string_view entry_filename(reinterpret_cast<const char*>(node_data + node_ofs + pos), filename_len);
			pos += filename_len;

			const uint32_t file_id = readU32BE(base + pos);
			pos += 4;

			if (entry_filename == filename)
				return file_id;
		}
	}

	return std::nullopt;
}

// --- Internal: listfile_binary_lookup_filename ---
static std::optional<uint32_t> listfile_binary_lookup_filename(const std::string& filename) {
	auto it = legacy_name_lookup.find(filename);
	if (it != legacy_name_lookup.end())
		return it->second;

	std::vector<std::string_view> components;
	std::string_view sv(filename);
	size_t start = 0;
	while (start < sv.size()) {
		size_t pos = sv.find('/', start);
		if (pos == std::string_view::npos) {
			components.push_back(sv.substr(start));
			break;
		}
		components.push_back(sv.substr(start, pos - start));
		start = pos + 1;
	}

	uint32_t node_ofs = 0;

	for (size_t i = 0; i + 1 < components.size(); i++) {
		int32_t result = listfile_binary_find_component_child(node_ofs, components[i]);
		if (result == -1)
			return std::nullopt;
		node_ofs = static_cast<uint32_t>(result);
	}

	return listfile_binary_find_file(node_ofs, components.back());
}

// --- Internal: listfile_preload_binary ---
static bool listfile_preload_binary() {
	try {
		logging::write("Downloading binary listfile format...");

		std::string bin_url;
		if (core::view->config.contains("listfileBinarySource"))
			bin_url = core::view->config["listfileBinarySource"].get<std::string>();

		if (bin_url.empty())
			throw std::runtime_error("Missing/malformed listfileBinarySource in configuration!");

		fs::create_directories(constants::CACHE::DIR_LISTFILE());

		int64_t last_modified = 0;

		try {
			auto idx_file = constants::CACHE::DIR_LISTFILE() / std::string(BIN_LF_COMPONENTS::ID_INDEX);
			last_modified = getFileLastModifiedMs(idx_file);
		} catch (...) {
			// No cached files.
		}

		if (listfile_check_cache_expiry(last_modified)) {
			bool download_failed = false;

			for (const auto& file : BIN_LF_COMPONENT_VALUES) {
				// Replace first %s with the file name
				std::string file_url = bin_url;
				auto pct_pos = file_url.find("%s");
				if (pct_pos != std::string::npos)
					file_url.replace(pct_pos, 2, file);

				auto cache_file = constants::CACHE::DIR_LISTFILE() / std::string(file);

				try {
					logging::write(std::format("Downloading binary listfile component: {}", std::string(file)));
					auto data = generics::downloadFile({file_url});
					data.writeToFile(cache_file);
				} catch (const std::exception& e) {
					logging::write(std::format("Failed to download binary listfile component ({}): {})", std::string(file), e.what()));
					download_failed = true;
					break;
				}
			}

			if (download_failed) {
				logging::write("Partial binary listfile download detected, cleaning up...");
				for (const auto& file : BIN_LF_COMPONENT_VALUES) {
					try {
						fs::remove(constants::CACHE::DIR_LISTFILE() / std::string(file));
					} catch (...) {}
				}
				throw std::runtime_error("Partial binary listfile download, falling back to legacy");
			}
		}

		logging::write("Loading binary listfile ID index into memory...");
		auto idx_file = constants::CACHE::DIR_LISTFILE() / std::string(BIN_LF_COMPONENTS::ID_INDEX);
		auto idx_buffer = BufferWrapper::readFile(idx_file);

		binary_id_to_offset.clear();
		binary_id_to_pf_index.clear();

		const size_t entry_count = idx_buffer.byteLength() / 9;
		for (size_t i = 0; i < entry_count; i++) {
			const uint32_t id = idx_buffer.readUInt32BE();
			const uint32_t string_offset = idx_buffer.readUInt32BE();
			const uint8_t pf_idx = idx_buffer.readUInt8();

			binary_id_to_offset.emplace(id, string_offset);
			binary_id_to_pf_index.emplace(id, pf_idx);
		}

		logging::write(std::format("Loaded {} binary listfile entries", binary_id_to_offset.size()));

		preload_models_map.clear();
		preload_textures_map.clear();
		preload_sounds_map.clear();
		preload_text_map.clear();
		preload_fonts_map.clear();
		preload_models_order.clear();
		preload_textures_order.clear();
		preload_sounds_order.clear();
		preload_text_order.clear();
		preload_fonts_order.clear();

		// null = skip preloading (videos loaded dynamically from MovieVariation)
		// Index: 0=null(strings), 1=models, 2=textures, 3=sounds, 4=null(videos), 5=text, 6=fonts
		struct PreloadContainer {
			std::unordered_map<uint32_t, std::string>* map;
			std::vector<uint32_t>* order;
		};
		std::array<PreloadContainer, 7> pf_preload_map = {{
			{nullptr, nullptr},                            // 0: STRINGS (main)
			{&preload_models_map, &preload_models_order}, // 1: PF_MODELS
			{&preload_textures_map, &preload_textures_order}, // 2: PF_TEXTURES
			{&preload_sounds_map, &preload_sounds_order}, // 3: PF_SOUNDS
			{nullptr, nullptr},                            // 4: PF_VIDEOS (skip)
			{&preload_text_map, &preload_text_order},     // 5: PF_TEXT
			{&preload_fonts_map, &preload_fonts_order}    // 6: PF_FONTS
		}};

		constexpr std::array<std::string_view, 7> pf_files = {
			BIN_LF_COMPONENTS::STRINGS,
			BIN_LF_COMPONENTS::PF_MODELS,
			BIN_LF_COMPONENTS::PF_TEXTURES,
			BIN_LF_COMPONENTS::PF_SOUNDS,
			BIN_LF_COMPONENTS::PF_VIDEOS,
			BIN_LF_COMPONENTS::PF_TEXT,
			BIN_LF_COMPONENTS::PF_FONTS
		};

		binary_strings_mmap = {};

		for (size_t i = 0; i < pf_files.size(); i++) {
			try {
				auto* mmap_obj = mmap_util::create_virtual_file();
				auto file_path = constants::CACHE::DIR_LISTFILE() / std::string(pf_files[i]);
				logging::write(std::format("Mapping pf file {}: {}", i, file_path.string()));

				if (!mmap_obj->map(file_path))
					throw std::runtime_error("Failed to map pf file: " + file_path.string());

				binary_strings_mmap[i] = mmap_obj;
			} catch (const std::exception& e) {
				logging::write(std::format("Error mapping pf file {}: {}", i, e.what()));
				throw;
			}
		}

		for (size_t i = 1; i < pf_files.size(); i++) {
			auto& preload_container = pf_preload_map[i];
			auto* preload_map = preload_container.map;
			auto* preload_order = preload_container.order;
			if (preload_map == nullptr || preload_order == nullptr)
				continue;

			auto file_path = constants::CACHE::DIR_LISTFILE() / std::string(pf_files[i]);
			logging::write(std::format("Preloading pf file {}: {}", i, file_path.string()));

			auto file_buffer = BufferWrapper::readFile(file_path);
			const uint32_t pf_entry_count = file_buffer.readUInt32BE();

			for (uint32_t j = 0; j < pf_entry_count; j++) {
				const uint32_t file_data_id = file_buffer.readUInt32BE();
				const std::string fn = file_buffer.readNullTerminatedString();
				auto [it, inserted] = preload_map->emplace(file_data_id, fn);
				if (inserted) {
					preload_order->push_back(file_data_id);
				} else {
					it->second = fn;
				}
			}

			logging::write(std::format("Preloaded {} entries from pf file {}", preload_map->size(), i));
		}

		try {
			binary_tree_nodes_mmap = mmap_util::create_virtual_file();
			auto tree_nodes_file = constants::CACHE::DIR_LISTFILE() / std::string(BIN_LF_COMPONENTS::TREE_NODES);
			logging::write(std::format("Mapping tree nodes file: {}", tree_nodes_file.string()));

			if (!binary_tree_nodes_mmap->map(tree_nodes_file))
				throw std::runtime_error("Failed to map tree nodes file: " + tree_nodes_file.string());
		} catch (const std::exception& e) {
			logging::write(std::format("Error mapping tree nodes file: {}", e.what()));
			throw;
		}

		logging::write("Binary listfile preload complete");
		is_binary_mode = true;
		return true;
	} catch (const std::exception& e) {
		logging::write(std::format("Error downloading binary listfile: {}", e.what()));
		return false;
	}
}

// --- Internal: listfile_preload_legacy ---
static bool listfile_preload_legacy() {
	try {
		std::string url;
		if (core::view->config.contains("listfileURL"))
			url = core::view->config["listfileURL"].get<std::string>();

		if (url.empty())
			throw std::runtime_error("Missing/malformed listfileURL in configuration!");

		fs::create_directories(constants::CACHE::DIR_LISTFILE());

		auto cache_file = constants::CACHE::DIR_LISTFILE() / std::string(constants::CACHE::LISTFILE_DATA);

		preloadedIdLookup.clear();
		preloadedNameLookup.clear();

		BufferWrapper data;
		bool have_data = false;

		if (url.starts_with("http")) {
			BufferWrapper cached;
			bool have_cached = false;
			int64_t last_modified = 0;

			try {
				cached = BufferWrapper::readFile(cache_file);
				have_cached = true;
				last_modified = getFileLastModifiedMs(cache_file);
			} catch (...) {
				// No cached file
			}

			if (listfile_check_cache_expiry(last_modified)) {
				try {
					std::string fallback_url;
					if (core::view->config.contains("listfileFallbackURL"))
						fallback_url = core::view->config["listfileFallbackURL"].get<std::string>();

					auto pct_pos = fallback_url.find("%s");
					if (pct_pos != std::string::npos)
						fallback_url.erase(pct_pos, 2);

					data = generics::downloadFile({url, fallback_url});
					have_data = true;

					data.writeToFile(cache_file);
				} catch (const std::exception& e) {
					if (!have_cached) {
						logging::write(std::format("Failed to download listfile during preload, no cached version for fallback: {}", e.what()));
						return false;
					}

					logging::write(std::format("Failed to download listfile during preload, using cached version: {}", e.what()));
					data = std::move(cached);
					have_data = true;
				}
			} else {
				data = std::move(cached);
				have_data = true;
			}
		} else {
			logging::write(std::format("Preloading user-defined local listfile: {}", url));
			data = BufferWrapper::readFile(url);
			have_data = true;
		}

		if (!have_data)
			return false;

		auto lines = data.readLines();
		logging::write(std::format("Processing {} listfile lines in chunks...", lines.size()));

		generics::batchWork<std::string>(std::string_view("listfile parsing"), lines,
			[&](const std::string& line, size_t) {
				if (line.empty())
					return;

				auto semicolon_pos = line.find(';');
				if (semicolon_pos == std::string::npos) {
					logging::write("Invalid listfile line (token count): " + line);
					return;
				}

				// Ensure exactly 2 tokens (no more semicolons)
				if (line.find(';', semicolon_pos + 1) != std::string::npos) {
					logging::write("Invalid listfile line (token count): " + line);
					return;
				}

				std::string id_str = line.substr(0, semicolon_pos);
				uint32_t fileDataID;
				try {
					fileDataID = static_cast<uint32_t>(std::stoul(id_str));
				} catch (...) {
					logging::write("Invalid listfile line (non-numerical ID): " + line);
					return;
				}

				std::string fileName = line.substr(semicolon_pos + 1);
				std::transform(fileName.begin(), fileName.end(), fileName.begin(),
					[](unsigned char c) { return std::tolower(c); });

				// TODO 197: Use operator[] instead of emplace() to match JS Map.set()
				// which overwrites existing entries with the same key.
				preloadedIdLookup[fileDataID] = fileName;
				preloadedNameLookup[fileName] = fileDataID;
			}, 1000);

		if (preloadedIdLookup.empty()) {
			logging::write("No entries found in preloaded listfile");
			return false;
		}

		preload_textures_ids = getFileDataIDsByExtension({ExtFilter(".blp")}, "filtering textures");

		preload_sounds_ids = getFileDataIDsByExtension(
			{ExtFilter(".ogg"), ExtFilter(".mp3"), ExtFilter(".unk_sound")}, "filtering sounds");

		preload_text_ids = getFileDataIDsByExtension(
			{ExtFilter(".txt"), ExtFilter(".lua"), ExtFilter(".xml"), ExtFilter(".sbt"),
			 ExtFilter(".wtf"), ExtFilter(".htm"), ExtFilter(".toc"), ExtFilter(".xsd")},
			"filtering text files");

		preload_fonts_ids = getFileDataIDsByExtension({ExtFilter(".ttf")}, "filtering fonts");

		preload_models_ids = getFileDataIDsByExtension(
			{ExtFilter(".m2"), ExtFilter(".m3"), ExtFilter(".wmo", constants::LISTFILE_MODEL_FILTER())},
			"filtering models");

		is_preloaded = true;
		logging::write(std::format("Preloaded {} listfile entries and filtered by extensions", preloadedIdLookup.size()));
		return true;
	} catch (const std::exception& e) {
		logging::write(std::format("Error during listfile preload: {}", e.what()));
		is_preloaded = false;
		return false;
	}
}

// --- Internal: listfile_preload ---
static bool listfile_preload_impl() {
	is_preloaded = false;
	logging::write("Preloading master listfile...");

	bool enable_binary = false;
	if (core::view->config.contains("enableBinaryListfile"))
		enable_binary = core::view->config["enableBinaryListfile"].get<bool>();

	if (enable_binary) {
		if (listfile_preload_binary()) {
			logging::write("Binary listfile loaded successfully"); // todo: some info?
			is_preloaded = true;
			return true;
		}

		logging::write("Failed to download binary listfile, falling back to legacy format");
	}

	return listfile_preload_legacy();
}

std::shared_future<bool> preloadAsync() {
	std::lock_guard<std::mutex> guard(preload_mutex);

	if (preload_future.has_value())
		return *preload_future;

	if (is_preloaded)
		return makeReadySharedFuture(true);

	preload_future = std::async(std::launch::async, []() {
		const bool result = listfile_preload_impl();
		std::lock_guard<std::mutex> lock(preload_mutex);
		preload_future.reset();
		return result;
	}).share();

	return *preload_future;
}

bool preload() {
	return preloadAsync().get();
}

std::shared_future<bool> prepareListfileAsync() {
	if (is_preloaded)
		return makeReadySharedFuture(true);

	{
		std::lock_guard<std::mutex> guard(preload_mutex);
		if (preload_future.has_value()) {
			logging::write("Waiting for listfile preload to complete...");
			return *preload_future;
		}
	}

	logging::write("Starting listfile preload...");
	return preloadAsync();
}

bool prepareListfile() {
	return prepareListfileAsync().get();
}

// --- Internal: loadIDTable ---
static size_t loadIDTable(const std::unordered_set<uint32_t>& ids, const std::string& ext) {
	size_t loadCount = 0;

	for (uint32_t fileDataID : ids) {
		if (!existsByID(fileDataID)) {
			std::string fileName = "unknown/" + std::to_string(fileDataID) + ext;
			// TODO 197: Use operator[] to match JS Map.set() overwrite semantics.
			legacy_id_lookup[fileDataID] = fileName;
			legacy_name_lookup[fileName] = fileDataID;
			loadCount++;
		}
	}

	return loadCount;
}

size_t loadUnknownTextures() {
	db::caches::DBTextureFileData::ensureInitialized();
	const auto& ids = db::caches::DBTextureFileData::getFileDataIDs();
	size_t unkBlp = loadIDTable(ids, ".blp");
	logging::write(std::format("Added {} unknown BLP textures from TextureFileData to listfile", unkBlp));
	return unkBlp;
}

std::future<size_t> loadUnknownTexturesAsync() {
	return std::async(std::launch::async, []() {
		return loadUnknownTextures();
	});
}

size_t loadUnknownModels() {
	// TODO 198: JS calls DBModelFileData.getFileDataIDs() directly without explicit
	// initialization. Removed the extra initializeModelFileData() call that had no
	// JS equivalent.
	const auto& ids = db::caches::DBModelFileData::getFileDataIDs();
	size_t unkM2 = loadIDTable(ids, ".m2");
	logging::write(std::format("Added {} unknown M2 models from ModelFileData to listfile", unkM2));
	return unkM2;
}

std::future<size_t> loadUnknownModelsAsync() {
	return std::async(std::launch::async, []() {
		return loadUnknownModels();
	});
}

void loadUnknowns() {
	loadUnknownModels();
}

std::future<void> loadUnknownsAsync() {
	return std::async(std::launch::async, []() {
		loadUnknowns();
	});
}

bool existsByID(uint32_t id) {
	if (legacy_id_lookup.contains(id))
		return true;

	if (is_binary_mode && binary_id_to_offset.contains(id))
		return true;

	return false;
}

std::optional<std::string> getByID(uint32_t id) {
	if (is_binary_mode) {
		auto it = legacy_id_lookup.find(id);
		if (it != legacy_id_lookup.end())
			return it->second;

		auto ofs_it = binary_id_to_offset.find(id);
		if (ofs_it == binary_id_to_offset.end())
			return std::nullopt;

		auto pf_it = binary_id_to_pf_index.find(id);
		if (pf_it == binary_id_to_pf_index.end())
			return std::nullopt;
		return binary_read_string_at_offset(pf_it->second, ofs_it->second);
	}

	auto it = legacy_id_lookup.find(id);
	if (it != legacy_id_lookup.end())
		return it->second;

	return std::nullopt;
}

std::string getByIDOrUnknown(uint32_t id, const std::string& ext) {
	auto result = getByID(id);
	if (result.has_value())
		return *result;

	return formatUnknownFile(id, ext);
}

std::optional<uint32_t> getByFilename(const std::string& filename) {
	std::string lower = filename;
	std::transform(lower.begin(), lower.end(), lower.begin(),
		[](unsigned char c) { return std::tolower(c); });
	std::replace(lower.begin(), lower.end(), '\\', '/');

	std::optional<uint32_t> lookup;

	if (is_binary_mode) {
		lookup = listfile_binary_lookup_filename(lower);
	} else {
		auto it = legacy_name_lookup.find(lower);
		if (it != legacy_name_lookup.end())
			lookup = it->second;
	}

	//     return getByFilename(ExportHelper.replaceExtension(filename, '.m2'));
	// Note: JS checks 'mdx' without dot (matches any suffix ending in 'mdx').
	if (!lookup.has_value() && (lower.ends_with(".mdl") || lower.ends_with("mdx")))
		return getByFilename(ExportHelper::replaceExtension(lower, ".m2"));

	return lookup;
}

std::vector<std::string> getFilenamesByExtension(const std::vector<ExtFilter>& exts) {
	std::vector<uint32_t> entries;

	if (is_binary_mode) {
		for (const auto& [fileDataID, offset] : binary_id_to_offset) {
			auto pf_it = binary_id_to_pf_index.find(fileDataID);
			const std::string fn = binary_read_string_at_offset(pf_it->second, offset);
			for (const auto& ext : exts) {
				if (ext.has_exclusion && ext.exclusion_regex) {
					if (fn.ends_with(ext.ext) && !std::regex_search(fn, *ext.exclusion_regex)) {
						entries.push_back(fileDataID);
						break;
					}
				} else {
					if (fn.ends_with(ext.ext)) {
						entries.push_back(fileDataID);
						break;
					}
				}
			}
		}
	} else {
		for (const auto& [fileDataID, fn] : legacy_id_lookup) {
			for (const auto& ext : exts) {
				if (ext.has_exclusion && ext.exclusion_regex) {
					if (fn.ends_with(ext.ext) && !std::regex_search(fn, *ext.exclusion_regex)) {
						entries.push_back(fileDataID);
						break;
					}
				} else {
					if (fn.ends_with(ext.ext)) {
						entries.push_back(fileDataID);
						break;
					}
				}
			}
		}
	}

	return formatEntries(entries);
}

std::vector<std::string> formatEntries(std::vector<uint32_t>& file_data_ids) {
	bool sort_by_id = false;
	if (core::view->config.contains("listfileSortByID"))
		sort_by_id = core::view->config["listfileSortByID"].get<bool>();

	if (sort_by_id)
		std::sort(file_data_ids.begin(), file_data_ids.end());

	const size_t n_entries = file_data_ids.size();
	std::vector<std::string> entries(n_entries);

	for (size_t i = 0; i < n_entries; i++) {
		const uint32_t fid = file_data_ids[i];
		entries[i] = getByIDOrUnknown(fid) + " [" + std::to_string(fid) + "]";
	}

	if (!sort_by_id)
		std::sort(entries.begin(), entries.end());

	return entries;
}

std::optional<int> applyPreload(const std::unordered_set<uint32_t>& rootEntries) {
	if (!is_preloaded) {
		logging::write("No preloaded listfile available, falling back to normal loading");
		return 0;
	}

	try {
		logging::write("Applying preloaded listfile data...");

		size_t valid_entries = 0;
		if (!is_binary_mode) {
			for (const auto& [fileDataID, fileName] : preloadedIdLookup) {
				if (rootEntries.contains(fileDataID)) {
					// TODO 197: Use operator[] to match JS Map.set() overwrite semantics.
					legacy_id_lookup[fileDataID] = fileName;
					legacy_name_lookup[fileName] = fileDataID;
					valid_entries++;
				}
			}

			core::view->listfileTextures.clear();
			auto tex = formatEntries(preload_textures_ids);
			for (auto& s : tex) core::view->listfileTextures.emplace_back(std::move(s));

			auto snd = formatEntries(preload_sounds_ids);
			core::view->listfileSounds.clear();
			for (auto& s : snd) core::view->listfileSounds.emplace_back(std::move(s));

			auto txt = formatEntries(preload_text_ids);
			core::view->listfileText.clear();
			for (auto& s : txt) core::view->listfileText.emplace_back(std::move(s));

			auto fnt = formatEntries(preload_fonts_ids);
			core::view->listfileFonts.clear();
			for (auto& s : fnt) core::view->listfileFonts.emplace_back(std::move(s));

			auto mdl = formatEntries(preload_models_ids);
			core::view->listfileModels.clear();
			for (auto& s : mdl) core::view->listfileModels.emplace_back(std::move(s));
		} else {
			for (auto it = binary_id_to_offset.begin(); it != binary_id_to_offset.end(); ) {
				if (!rootEntries.contains(it->first)) {
					binary_id_to_pf_index.erase(it->first);
					it = binary_id_to_offset.erase(it);
				} else {
					valid_entries++;
					++it;
				}
			}

			auto filter_and_format = [&rootEntries](std::unordered_map<uint32_t, std::string>& preload_map,
				const std::vector<uint32_t>& preload_order)
				-> std::vector<nlohmann::json>
			{
				std::vector<nlohmann::json> formatted;
				formatted.reserve(preload_order.size());

				for (const uint32_t fid : preload_order) {
					if (!rootEntries.contains(fid))
						continue;

					const auto it = preload_map.find(fid);
					if (it == preload_map.end())
						continue;

					formatted.emplace_back(it->second + " [" + std::to_string(fid) + "]");
				}

				preload_map.clear();
				return formatted;
			};

			core::view->listfileTextures = filter_and_format(preload_textures_map, preload_textures_order);
			core::view->listfileSounds = filter_and_format(preload_sounds_map, preload_sounds_order);
			core::view->listfileText = filter_and_format(preload_text_map, preload_text_order);
			core::view->listfileFonts = filter_and_format(preload_fonts_map, preload_fonts_order);
			core::view->listfileModels = filter_and_format(preload_models_map, preload_models_order);
			preload_textures_order.clear();
			preload_sounds_order.clear();
			preload_text_order.clear();
			preload_fonts_order.clear();
			preload_models_order.clear();
		}

		if (valid_entries == 0) {
			logging::write("No preloaded entries matched rootEntries");
			return 0;
		}

		loaded = true;
		logging::write(std::format("Applied {} preloaded listfile entries", valid_entries));
	} catch (const std::exception& e) {
		logging::write(std::format("Error applying preloaded listfile: {}", e.what()));
	}

	return std::nullopt;
}

static std::vector<FilteredEntry> getFilteredEntriesImpl(const std::string* search, const std::regex* re) {
	std::vector<FilteredEntry> results;

	auto matches = [&](const std::string& fileName) -> bool {
		if (re != nullptr)
			return std::regex_search(fileName, *re);
		return fileName.find(*search) != std::string::npos;
	};

	if (is_binary_mode) {
		for (const auto& [fileDataID, offset] : binary_id_to_offset) {
			auto pf_it = binary_id_to_pf_index.find(fileDataID);
			std::string fileName = binary_read_string_at_offset(pf_it->second, offset);
			if (matches(fileName))
				results.push_back({fileDataID, std::move(fileName)});
		}

		for (const auto& [fileDataID, fileName] : legacy_id_lookup) {
			if (matches(fileName))
				results.push_back({fileDataID, fileName});
		}
	} else {
		for (const auto& [fileDataID, fileName] : legacy_id_lookup) {
			if (matches(fileName))
				results.push_back({fileDataID, fileName});
		}
	}

	return results;
}

std::vector<FilteredEntry> getFilteredEntries(const std::string& search) {
	return getFilteredEntriesImpl(&search, nullptr);
}

std::vector<FilteredEntry> getFilteredEntries(const std::regex& search) {
	return getFilteredEntriesImpl(nullptr, &search);
}

// TODO 200: Use std::optional to distinguish between "no filter" (nullopt = include everything)
// and "empty filter" (empty vector = match nothing), matching JS's undefined vs [].
std::vector<std::string> renderListfile(const std::optional<std::vector<uint32_t>>& file_data_ids,
                                         bool include_main_index) {
	std::vector<std::string> result;
	// has_id_filter is true when file_data_ids is provided (even if empty)
	const bool has_id_filter = file_data_ids.has_value();

	if (is_binary_mode) {
		constexpr std::array<std::string_view, 7> pf_files = {
			BIN_LF_COMPONENTS::STRINGS,
			BIN_LF_COMPONENTS::PF_MODELS,
			BIN_LF_COMPONENTS::PF_TEXTURES,
			BIN_LF_COMPONENTS::PF_SOUNDS,
			BIN_LF_COMPONENTS::PF_VIDEOS,
			BIN_LF_COMPONENTS::PF_TEXT,
			BIN_LF_COMPONENTS::PF_FONTS
		};

		const size_t start_index = include_main_index ? 0 : 1;
		std::unordered_set<uint32_t> id_set;
		if (has_id_filter)
			id_set.insert(file_data_ids->begin(), file_data_ids->end());

		for (size_t i = start_index; i < pf_files.size(); i++) {
			auto file_path = constants::CACHE::DIR_LISTFILE() / std::string(pf_files[i]);
			auto file_buffer = BufferWrapper::readFile(file_path);
			const uint32_t entry_count = file_buffer.readUInt32BE();

			for (uint32_t j = 0; j < entry_count; j++) {
				const uint32_t file_data_id = file_buffer.readUInt32BE();
				const std::string fn = file_buffer.readNullTerminatedString();

				if (!has_id_filter || id_set.contains(file_data_id))
					result.push_back(fn + " [" + std::to_string(file_data_id) + "]");
			}
		}
	}

	// JS: `file_data_ids === undefined` means no filter — include all legacy entries.
	// JS: `file_data_ids = []` means empty filter — include nothing from legacy lookups.
	if (!has_id_filter) {
		for (const auto& [file_data_id, fn] : legacy_id_lookup) {
			result.push_back(fn + " [" + std::to_string(file_data_id) + "]");
		}
	} else {
		std::unordered_set<uint32_t> id_set(file_data_ids->begin(), file_data_ids->end());
		for (const auto& [file_data_id, fn] : legacy_id_lookup) {
			if (id_set.contains(file_data_id))
				result.push_back(fn + " [" + std::to_string(file_data_id) + "]");
		}
	}

	return result;
}

std::future<std::vector<std::string>> renderListfileAsync(const std::optional<std::vector<uint32_t>>& file_data_ids,
                                                          bool include_main_index) {
	return std::async(std::launch::async, [file_data_ids, include_main_index]() {
		return renderListfile(file_data_ids, include_main_index);
	});
}

std::string stripFileEntry(const std::string& entry) {
	auto pos = entry.rfind(" [");
	if (pos != std::string::npos)
		return entry.substr(0, pos);

	return entry;
}

ParsedEntry parseFileEntry(const std::string& entry) {
	ParsedEntry result;
	result.file_path = stripFileEntry(entry);

	// TODO 204: Make regex static const to avoid recompiling on every call.
	static const std::regex fid_regex(R"(\[(\d+)\]$)");
	std::smatch match;
	if (std::regex_search(entry, match, fid_regex))
		result.file_data_id = static_cast<uint32_t>(std::stoul(match[1].str()));

	return result;
}

std::string formatUnknownFile(uint32_t fileDataID, const std::string& ext) {
	return "unknown/" + std::to_string(fileDataID) + ext;
}

bool isLoaded() {
	return loaded;
}

void ingestIdentifiedFiles(const std::vector<std::pair<uint32_t, std::string>>& entries) {
	for (const auto& [fileDataID, ext] : entries) {
		std::string fileName = "unknown/" + std::to_string(fileDataID) + ext;
		// TODO 197: Use operator[] to match JS Map.set() overwrite semantics.
		legacy_id_lookup[fileDataID] = fileName;
		legacy_name_lookup[fileName] = fileDataID;
	}
}

void addEntry(uint32_t fileDataID, const std::string& fileName,
              std::vector<std::string>* listfile_out) {
	std::string lower = fileName;
	std::transform(lower.begin(), lower.end(), lower.begin(),
		[](unsigned char c) { return std::tolower(c); });

	legacy_id_lookup[fileDataID] = lower;
	legacy_name_lookup[lower] = fileDataID;

	if (listfile_out)
		listfile_out->push_back(lower + " [" + std::to_string(fileDataID) + "]");
}

} // namespace listfile
} // namespace casc
