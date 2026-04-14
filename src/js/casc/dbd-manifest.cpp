/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "dbd-manifest.h"

#include "../core.h"
#include "../log.h"
#include "../generics.h"
#include "../buffer.h"

#include <algorithm>
#include <atomic>
#include <format>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

// TODO 191: is_preloaded must be atomic since it is written from the async
// worker thread and read from the calling thread (prepareManifest).
std::atomic<bool> is_preloaded{false};
std::optional<std::shared_future<void>> preload_promise;
std::unordered_map<std::string, int> table_to_id;
std::unordered_map<int, std::string> id_to_table;
std::mutex manifest_mutex;
// TODO 192: Protect preload() from concurrent calls with std::once_flag.
std::once_flag preload_once_flag;

} // anonymous namespace

namespace casc {
namespace dbd_manifest {

/**
 * preload the dbd manifest from configured urls
 */
void preload() {
	// TODO 192: Use std::call_once to prevent concurrent preloads.
	// In JS this was safe because JS is single-threaded; in C++ we need
	// explicit protection against concurrent calls.
	std::call_once(preload_once_flag, []() {
		preload_promise = std::async(std::launch::async, []() {
			try {
				const std::string dbd_filename_url = core::view->config["dbdFilenameURL"].get<std::string>();
				const std::string dbd_filename_fallback_url = core::view->config["dbdFilenameFallbackURL"].get<std::string>();

				auto raw = generics::downloadFile(std::vector<std::string>{ dbd_filename_url, dbd_filename_fallback_url });
				const nlohmann::json manifest_data = raw.readJSON();

				{
					std::lock_guard<std::mutex> lock(manifest_mutex);
					for (const auto& entry : manifest_data) {
						// TODO 190: Match JS truthiness: `if (entry.tableName && entry.db2FileDataID)`.
						// JS truthiness accepts any truthy value. We check that the fields exist
						// and are non-null/non-false/non-zero/non-empty, matching JS semantics.
						if (!entry.contains("tableName") || !entry.contains("db2FileDataID"))
							continue;

						const auto& tn = entry["tableName"];
						const auto& fid = entry["db2FileDataID"];

						// JS: any truthy value for tableName (non-empty string, number, etc.)
						// JS: any truthy value for db2FileDataID (non-zero number, non-empty string, etc.)
						bool tn_truthy = false;
						if (tn.is_string())
							tn_truthy = !tn.get<std::string>().empty();
						else if (tn.is_number())
							tn_truthy = tn.get<double>() != 0.0;
						else if (tn.is_boolean())
							tn_truthy = tn.get<bool>();
						// null/object/array are falsy in JS

						bool fid_truthy = false;
						if (fid.is_number())
							fid_truthy = fid.get<double>() != 0.0;
						else if (fid.is_string())
							fid_truthy = !fid.get<std::string>().empty();
						else if (fid.is_boolean())
							fid_truthy = fid.get<bool>();

						if (tn_truthy && fid_truthy) {
							const std::string table_name = tn.is_string() ? tn.get<std::string>() : std::to_string(tn.get<int>());
							const int file_data_id = fid.is_number() ? fid.get<int>() : std::stoi(fid.get<std::string>());
							table_to_id[table_name] = file_data_id;
							id_to_table[file_data_id] = table_name;
						}
					}
				}

				logging::write(std::format("preloaded dbd manifest with {} entries", table_to_id.size()));
				is_preloaded.store(true);
			} catch (const std::exception& e) {
				logging::write(std::format("failed to preload dbd manifest: {}", e.what()));
				is_preloaded.store(true);
			}
		}).share();
	});
}

/**
 * prepare the manifest for use, awaiting preload if necessary
 * @returns true always, matching JS's Promise<boolean>
 */
bool prepareManifest() {
	if (is_preloaded.load())
		return true;

	if (preload_promise.has_value())
		preload_promise->wait();

	return true;
}

/**
 * get table name by filedataid
 * @param id
 * @returns table name or std::nullopt
 */
std::optional<std::string> getByID(int id) {
	std::lock_guard<std::mutex> lock(manifest_mutex);
	auto it = id_to_table.find(id);
	if (it != id_to_table.end())
		return it->second;
	return std::nullopt;
}

/**
 * get filedataid by table name
 * @param table_name
 * @returns filedataid or std::nullopt
 */
std::optional<int> getByTableName(const std::string& table_name) {
	std::lock_guard<std::mutex> lock(manifest_mutex);
	auto it = table_to_id.find(table_name);
	if (it != table_to_id.end())
		return it->second;
	return std::nullopt;
}

/**
 * get all table names
 * @returns sorted vector of table names
 */
std::vector<std::string> getAllTableNames() {
	std::lock_guard<std::mutex> lock(manifest_mutex);
	std::vector<std::string> names;
	names.reserve(table_to_id.size());
	for (const auto& [key, value] : table_to_id)
		names.push_back(key);
	std::sort(names.begin(), names.end());
	return names;
}

} // namespace dbd_manifest
} // namespace casc
