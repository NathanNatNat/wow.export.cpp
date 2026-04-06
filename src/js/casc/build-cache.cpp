/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "build-cache.h"

#include "../log.h"
#include "../constants.h"
#include "../generics.h"
#include "../core.h"
#include "../buffer.h"
#include "../mmap.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <format>
#include <string>
#include <stdexcept>

namespace fs = std::filesystem;

namespace casc {

// Module-level state: cache integrity hash map.
static nlohmann::json cacheIntegrity;
static bool cacheIntegrityLoaded = false;

/**
 * Returns the current timestamp in milliseconds since epoch (Date.now() equivalent).
 */
static int64_t dateNow() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()
	).count();
}

// -- BuildCache --

BuildCache::BuildCache(const std::string& key)
	: key(key)
	, meta(nlohmann::json::object())
	, cacheDir(fs::path(constants::CACHE::DIR_BUILDS()) / key)
	, manifestPath(fs::path(constants::CACHE::DIR_BUILDS()) / key / std::string(constants::CACHE::BUILD_MANIFEST))
{
}

void BuildCache::init() {
	// Create cache directory if needed.
	fs::create_directories(cacheDir);

	// Load manifest values.
	try {
		std::ifstream ifs(manifestPath);
		if (ifs.is_open()) {
			nlohmann::json manifest = nlohmann::json::parse(ifs);
			meta.update(manifest);
		} else {
			logging::write(std::format("No cache manifest found for {}", key));
		}
	} catch (const std::exception&) {
		logging::write(std::format("No cache manifest found for {}", key));
	}

	// Save access update without blocking.
	meta["lastAccess"] = dateNow();
	saveManifest();
}

std::optional<BufferWrapper> BuildCache::getFile(const std::string& file, const std::string& dir) {
	try {
		const fs::path filePath = getFilePath(file, dir);
		const std::string filePathStr = filePath.string();

		// Cache integrity is not loaded yet, reject.
		if (!cacheIntegrityLoaded) {
			logging::write(std::format("Cannot verify integrity of file, cache integrity not loaded ({})", filePathStr));
			return std::nullopt;
		}

		// File integrity cannot be verified, reject.
		if (!cacheIntegrity.contains(filePathStr) || !cacheIntegrity[filePathStr].is_string()) {
			logging::write(std::format("Cannot verify integrity of file, rejecting cache ({})", filePathStr));
			return std::nullopt;
		}

		const std::string integrityHash = cacheIntegrity[filePathStr].get<std::string>();

		BufferWrapper data = BufferWrapper::readFile(filePath);
		const std::string dataHash = data.calculateHash("sha1", "hex");

		// Reject cache if hash does not match.
		if (dataHash != integrityHash) {
			logging::write(std::format("Bad integrity for file {}, rejecting cache ({} != {})", filePathStr, dataHash, integrityHash));
			return std::nullopt;
		}

		return data;
	} catch (const std::exception&) {
		return std::nullopt;
	}
}

std::filesystem::path BuildCache::getFilePath(const std::string& file, const std::string& dir) const {
	if (!dir.empty())
		return fs::path(dir) / file;

	return cacheDir / file;
}

void BuildCache::storeFile(const std::string& file, BufferWrapper& data, const std::string& dir) {
	const fs::path filePath = getFilePath(file, dir);

	if (!dir.empty())
		generics::createDirectory(filePath.parent_path());

	// Cache integrity is not loaded yet, initialize an empty map.
	if (!cacheIntegrityLoaded) {
		logging::write("Cache integrity not loaded, initializing empty integrity map.");
		cacheIntegrity = nlohmann::json::object();
		cacheIntegrityLoaded = true;
	}

	// Integrity checking.
	const std::string hash = data.calculateHash("sha1", "hex");
	cacheIntegrity[filePath.string()] = hash;

	data.writeToFile(filePath);
	core::view->cacheSize += static_cast<int64_t>(data.byteLength());

	saveCacheIntegrity();
}

void BuildCache::saveCacheIntegrity() {
	std::ofstream ofs(constants::CACHE::INTEGRITY_FILE());
	if (ofs.is_open())
		ofs << cacheIntegrity.dump();
}

void BuildCache::saveManifest() {
	std::ofstream ofs(manifestPath);
	if (ofs.is_open())
		ofs << meta.dump();
}

// -- Module initialization --

void initBuildCacheSystem() {
	try {
		auto integrity = generics::readJSON(constants::CACHE::INTEGRITY_FILE(), false);
		if (!integrity.has_value())
			throw std::runtime_error("File cannot be accessed or contains malformed JSON: " + constants::CACHE::INTEGRITY_FILE().string());

		cacheIntegrity = integrity.value();
	} catch (const std::exception& e) {
		logging::write("Unable to load cache integrity file; entire cache will be invalidated.");
		logging::write(e.what());

		cacheIntegrity = nlohmann::json::object();
	}

	cacheIntegrityLoaded = true;
	core::events.emit("cache-integrity-ready");
}

// -- Event handlers --

void registerBuildCacheEvents() {
	// Invoked when the user requests a cache purge.
	core::events.on("click-cache-clear", []() {
		auto _lock = core::create_busy_lock();
		core::setToast("progress", "Clearing cache, please wait...", nullptr, -1, false);
		logging::write(std::format("Manual cache purge requested by user! (Cache size: {})", generics::filesize(static_cast<double>(core::view->cacheSize))));

		try {
			mmap_util::release_virtual_files();

			std::error_code ec;
			fs::remove_all(constants::CACHE::DIR(), ec);
			fs::create_directories(constants::CACHE::DIR());

			core::view->cacheSize = 0;
			logging::write("Purge complete, awaiting mandatory restart.");

			// TODO(conversion): Restart action: emit event since restartApplication() is not yet available.
			nlohmann::json restartOptions = nlohmann::json::object();
			restartOptions["Restart"] = "restart-application";
			core::setToast("success", "Cache has been successfully cleared, a restart is required.", restartOptions, -1, false);

			core::events.emit("cache-cleared");
		} catch (const std::exception& e) {
			logging::write(std::format("Error clearing cache: {}", e.what()));
			core::setToast("error", std::format("Failed to clear cache: {}", e.what()), nullptr, -1, false);
		}
	});

	// Run cache clean-up once a CASC source has been selected.
	// We delay this until here so that we don't potentially mark
	// a build as stale and delete it right before the user requests it.
	core::events.once("casc-source-changed", []() {
		int64_t cacheExpire = 0;
		if (core::view->config.contains("cacheExpiry") && core::view->config["cacheExpiry"].is_number()) {
			cacheExpire = core::view->config["cacheExpiry"].get<int64_t>();
		} else if (core::view->config.contains("cacheExpiry") && core::view->config["cacheExpiry"].is_string()) {
			try {
				cacheExpire = std::stoll(core::view->config["cacheExpiry"].get<std::string>());
			} catch (...) {
				cacheExpire = 0;
			}
		}

		cacheExpire *= 24 * 60 * 60 * 1000;

		// If user sets cacheExpiry to 0 in the configuration, we completely
		// skip the clean-up process. This is generally considered a bad idea.
		if (cacheExpire == 0) {
			logging::write(std::format("WARNING: Cache clean-up has been skipped due to cacheExpiry being {}", cacheExpire));
			return;
		}

		logging::write("Running clean-up for stale build caches...");

		std::error_code ec;
		if (!fs::exists(constants::CACHE::DIR_BUILDS(), ec))
			return;

		const int64_t ts = dateNow();

		for (const auto& entry : fs::directory_iterator(constants::CACHE::DIR_BUILDS(), ec)) {
			// We only care about directories with MD5 names.
			// There shouldn't be anything else in there anyway.
			if (!entry.is_directory())
				continue;

			const std::string entryName = entry.path().filename().string();
			if (entryName.length() != 32)
				continue;

			bool deleteEntry = false;
			uintmax_t manifestSize = 0;
			const fs::path entryDir = constants::CACHE::DIR_BUILDS() / entryName;
			const fs::path entryManifest = entryDir / std::string(constants::CACHE::BUILD_MANIFEST);

			try {
				std::ifstream ifs(entryManifest);
				if (!ifs.is_open())
					throw std::runtime_error("Cannot open manifest");

				std::string manifestRaw((std::istreambuf_iterator<char>(ifs)),
				                         std::istreambuf_iterator<char>());
				nlohmann::json manifest = nlohmann::json::parse(manifestRaw);
				manifestSize = manifestRaw.size();

				if (manifest.contains("lastAccess") && manifest["lastAccess"].is_number()) {
					const int64_t delta = ts - manifest["lastAccess"].get<int64_t>();
					if (delta > cacheExpire) {
						deleteEntry = true;
						logging::write(std::format("Build cache {} has expired ({}), marking for deletion.", entryName, delta));
					}
				} else {
					// lastAccess property missing from manifest?
					deleteEntry = true;
					logging::write(std::format("Unable to read lastAccess from {}, marking for deletion.", entryName));
				}
			} catch (const std::exception&) {
				// Manifest is missing or malformed.
				deleteEntry = true;
				logging::write(std::format("Unable to read manifest for {}, marking for deletion.", entryName));
			}

			if (deleteEntry) {
				uintmax_t deleteSize = generics::deleteDirectory(entryDir);

				// We don't include manifests in the cache size, so we need to make
				// sure we don't subtract the size of it from our total to maintain accuracy.
				if (deleteSize >= manifestSize)
					deleteSize -= manifestSize;

				core::view->cacheSize -= static_cast<int64_t>(deleteSize);
			}
		}
	});
}

} // namespace casc