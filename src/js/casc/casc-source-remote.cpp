/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "casc-source-remote.h"
#include "../constants.h"
#include "../generics.h"
#include "../core.h"
#include "../log.h"
#include "casc-source.h"
#include "version-config.h"
#include "cdn-config.h"
#include "build-cache.h"
#include "listfile.h"
#include "blte-reader.h"
#include "blte-stream-reader.h"
#include "cdn-resolver.h"

#include <stdexcept>
#include <string>
#include <format>
#include <regex>
#include <algorithm>
#include <numeric>

namespace casc {

// EMPTY_HASH is already defined in blte-reader.h as casc::EMPTY_HASH

CASCRemote::CASCRemote(const std::string& region)
	: CASC(true), region(region)
{
}

/**
 * Initialize remote CASC source.
 */
void CASCRemote::init() {
	logging::write("Initializing remote CASC source (" + region + ")");

	host = std::format(constants::PATCH::HOST, region);
	if (region == "cn")
		host = std::string(constants::PATCH::HOST_CHINA);

	builds.clear();

	// Collect version configs for all products.
	// JS: const promises = constants.PRODUCTS.map(p => this.getVersionConfig(p.product));
	// JS: const results = await Promise.allSettled(promises);
	// In C++, we try each product sequentially and catch failures.

	// Iterate through successful requests and extract product config for our region.
	for (const auto& p : constants::PRODUCTS) {
		try {
			auto config = getVersionConfig(std::string(p.product));

			// JS: result.value.find(e => e.Region === this.region)
			// .find() returns undefined if no match — we push empty map as equivalent.
			bool regionFound = false;
			for (auto& entry : config) {
				if (entry.count("Region") && entry["Region"] == region) {
					builds.push_back(entry);
					regionFound = true;
					break;
				}
			}
			if (!regionFound)
				builds.push_back({}); // equivalent to JS undefined from .find()
		} catch (const std::exception& e) {
			// JS Promise.allSettled: rejected promises are NOT pushed to builds.
			// Only fulfilled results contribute to the builds array.
		}
	}

	logging::write("Remote builds loaded: " + std::to_string(builds.size()));
}

/**
 * Download the remote version config for a specific product.
 * @param product
 */
std::vector<std::unordered_map<std::string, std::string>> CASCRemote::getVersionConfig(const std::string& product) {
	auto config = getConfig(product, std::string(constants::PATCH::VERSION_CONFIG));
	for (auto& entry : config)
		entry["Product"] = product;
	return config;
}

/**
 * Download and parse a version config file.
 * @param product
 * @param file
 */
std::vector<std::unordered_map<std::string, std::string>> CASCRemote::getConfig(const std::string& product, const std::string& file) {
	const std::string url = host + product + file;
	auto res = generics::get(url);

	if (res.empty())
		throw std::runtime_error(std::format("HTTP error from remote CASC endpoint: {}", url));

	std::string text(res.begin(), res.end());
	return parseVersionConfig(text);
}

/**
 * Download and parse a CDN config file.
 * Attempts multiple CDN hosts in order of ping speed if one fails.
 * @param key
 * @param cdnHosts Optional array of CDN hosts to try (in priority order)
 */
std::unordered_map<std::string, std::string> CASCRemote::getCDNConfig(const std::string& key,
	const std::vector<std::string>& cdnHosts)
{
	// If no hosts provided, use the current host
	std::vector<std::string> hostsToTry = cdnHosts.empty() ? std::vector<std::string>{host} : cdnHosts;

	std::string lastErrorMsg;
	for (const auto& h : hostsToTry) {
		try {
			const std::string url = h + "config/" + formatCDNKey(key);
			logging::write("Attempting to retrieve CDN config from: " + url);
			auto res = generics::get(url);

			if (res.empty())
				throw std::runtime_error("HTTP error from CDN config endpoint");

			std::string configText(res.begin(), res.end());
			auto config = parseCDNConfig(configText);

			if (h != host) {
				logging::write("Successfully retrieved config from fallback host: " + h);
				host = h;
			}

			return config;
		} catch (const std::exception& error) {
			logging::write("Failed to retrieve CDN config from " + h + ": " + error.what());
			lastErrorMsg = error.what();

			cdn_resolver::markHostFailed(h);
			continue;
		}
	}

	throw std::runtime_error(std::format("Unable to retrieve CDN config file {} from any CDN host. Last error: {}",
		key, lastErrorMsg.empty() ? "Unknown error" : lastErrorMsg));
}

/**
 * Obtain a file by it's fileDataID.
 * @param fileDataID
 * @param partialDecrypt
 * @param suppressLog
 * @param supportFallback
 * @param forceFallback
 * @param contentKey
 */
// TODO: This could do with being an interface.
BLTEReader CASCRemote::getFileAsBLTE(uint32_t fileDataID, bool partialDecrypt,
	bool suppressLog, bool supportFallback, bool forceFallback, const std::string& contentKey)
{
	if (!suppressLog)
		logging::write(std::format("Loading remote CASC file {} ({})", fileDataID, listfile::getByID(fileDataID)));

	const std::string encodingKey = !contentKey.empty() ? CASC::getEncodingKeyForContentKey(contentKey) : CASC::getFile(fileDataID);
	auto cachedData = cache->getFile(encodingKey, constants::CACHE::DIR_DATA().string());

	BufferWrapper data;
	if (!cachedData.has_value()) {
		auto archIt = archives.find(encodingKey);
		if (archIt != archives.end()) {
			data = getDataFilePartial(formatCDNKey(archIt->second.key), archIt->second.offset, archIt->second.size);

			if (!suppressLog)
				logging::write(std::format("Downloading CASC file {} from archive {}", fileDataID, archIt->second.key));
		} else {
			data = getDataFile(formatCDNKey(encodingKey));

			if (!suppressLog)
				logging::write(std::format("Downloading unarchived CASC file {}", fileDataID));

			if (data.byteLength() == 0)
				throw std::runtime_error("No remote unarchived/archive indexed for encoding key: " + encodingKey);
		}

		cache->storeFile(encodingKey, data, constants::CACHE::DIR_DATA().string());
	} else {
		data = std::move(cachedData.value());
		if (!suppressLog)
			logging::write(std::format("Loaded CASC file {} from cache", fileDataID));
	}

	return BLTEReader(std::move(data), encodingKey, partialDecrypt);
}

/**
 * Get a streaming reader for a file by its fileDataID.
 * @param fileDataID
 * @param partialDecrypt
 * @param suppressLog
 * @param contentKey
 * @returns BLTEStreamReader
 */
BLTEStreamReader CASCRemote::getFileStream(uint32_t fileDataID, bool partialDecrypt,
	bool suppressLog, const std::string& contentKey)
{
	if (!suppressLog)
		logging::write(std::format("Creating stream for remote CASC file {} ({})", fileDataID, listfile::getByID(fileDataID)));

	const std::string encodingKey = !contentKey.empty() ? CASC::getEncodingKeyForContentKey(contentKey) : CASC::getFile(fileDataID);
	auto archIt = archives.find(encodingKey);

	// download blte header to parse metadata
	BufferWrapper headerData;
	int64_t baseOffset = 0;

	if (archIt != archives.end()) {
		// archived file - download first 4kb to get header
		headerData = getDataFilePartial(
			formatCDNKey(archIt->second.key),
			archIt->second.offset,
			std::min(4096, archIt->second.size)
		);
		baseOffset = archIt->second.offset;

		if (!suppressLog)
			logging::write(std::format("Streaming remote CASC file {} from archive {}", fileDataID, archIt->second.key));
	} else {
		// unarchived file
		headerData = getDataFilePartial(
			formatCDNKey(encodingKey),
			0,
			4096
		);

		if (!suppressLog)
			logging::write(std::format("Streaming unarchived remote CASC file {}", fileDataID));
	}

	auto metadata = BLTEReader::parseBLTEHeader(headerData, encodingKey, false);

	// create block fetcher function
	// Capture needed data by value for the lambda
	const bool hasArchive = (archIt != archives.end());
	const std::string archiveKey = hasArchive ? archIt->second.key : "";

	auto blockFetcher = [this, metadata, encodingKey, hasArchive, archiveKey, baseOffset](size_t blockIndex) -> BufferWrapper {
		const auto& block = metadata.blocks[blockIndex];
		const int64_t blockOffset = static_cast<int64_t>(metadata.dataStart) + block.fileOffset;

		if (hasArchive) {
			return this->getDataFilePartial(
				this->formatCDNKey(archiveKey),
				baseOffset + blockOffset,
				block.CompSize
			);
		} else {
			return this->getDataFilePartial(
				this->formatCDNKey(encodingKey),
				blockOffset,
				block.CompSize
			);
		}
	};

	return BLTEStreamReader(encodingKey, metadata, blockFetcher, partialDecrypt);
}

/**
 * Returns a list of available products on the remote CDN.
 * Format example: "PTR: World of Warcraft 8.3.0.32272"
 */
std::vector<ProductEntry> CASCRemote::getProductList() {
	std::vector<ProductEntry> products;
	for (size_t i = 0; i < builds.size(); i++) {
		const auto& entry = builds[i];

		// This check exists because some regions (e.g. China) may not have all products.
		if (entry.empty())
			continue;

		auto versionIt = entry.find("VersionsName");
		auto productIt = entry.find("Product");
		if (versionIt == entry.end() || productIt == entry.end())
			continue;

		// Find matching product definition.
		std::string_view title;
		for (const auto& p : constants::PRODUCTS) {
			if (p.product == productIt->second) {
				title = p.title;
				break;
			}
		}

		const std::string label = std::format("{} {}", title, versionIt->second);

		// Extract expansion ID from version string.
		std::regex versionRegex("^(\\d+)\\.");
		std::smatch match;
		const std::string& versionName = versionIt->second;
		int expansionId = 0;
		if (std::regex_search(versionName, match, versionRegex))
			expansionId = std::min(std::stoi(match[1].str()) - 1, 12);

		products.push_back({ label, expansionId, static_cast<int>(i) });
	}

	return products;
}

/**
 * Preload requirements for reading remote files without initializing the
 * entire instance. Used by local CASC install for CDN fallback.
 * @param buildIndex
 * @param cache Optional shared cache.
 */
void CASCRemote::preload(int buildIndex, BuildCache* sharedCache) {
	build = builds[buildIndex];
	logging::write("Preloading remote CASC build");

	if (sharedCache) {
		cache = sharedCache;
	} else {
		ownedCache = std::make_unique<BuildCache>(build["BuildConfig"]);
		ownedCache->init();
		cache = ownedCache.get();
	}

	loadServerConfig();
	resolveCDNHost();
	loadConfigs();
	loadArchives();
}

/**
 * Load the CASC interface with the given build.
 * @param buildIndex
 */
void CASCRemote::load(int buildIndex) {
	core::showLoadingScreen(12);
	preload(buildIndex);

	loadEncoding();
	loadRoot();

	core::view->casc = this;

	prepareListfile();
	prepareDBDManifest();
	loadListfile(build["BuildConfig"]);

	core::hideLoadingScreen();
}

/**
 * Download and parse the encoding file.
 */
void CASCRemote::loadEncoding() {
	// Split encoding keys by space — second key is the encoding key.
	const std::string& encStr = buildConfig["encoding"];
	std::string encKey;
	{
		auto spacePos = encStr.find(' ');
		if (spacePos != std::string::npos)
			encKey = encStr.substr(spacePos + 1);
		else
			encKey = encStr;
	}

	logging::timeLog();

	core::progressLoadingScreen("Loading encoding table");
	auto encRaw = cache->getFile(std::string(constants::CACHE::BUILD_ENCODING));
	BufferWrapper encData;
	if (!encRaw.has_value()) {
		// Encoding file not cached, download it.
		logging::write("Encoding for build " + build["BuildConfig"] + " not cached, downloading.");
		encData = getDataFile(formatCDNKey(encKey));

		// Store back into cache (no need to block).
		cache->storeFile(std::string(constants::CACHE::BUILD_ENCODING), encData);
	} else {
		logging::write("Encoding for build " + build["BuildConfig"] + " cached locally.");
		encData = std::move(encRaw.value());
	}

	logging::timeEnd("Loaded encoding table (" + generics::filesize(static_cast<double>(encData.byteLength())) + ")");

	// Parse encoding file.
	logging::timeLog();
	core::progressLoadingScreen("Parsing encoding table");
	parseEncodingFile(std::move(encData), encKey);
	logging::timeEnd("Parsed encoding table (" + std::to_string(encodingKeys.size()) + " entries)");
}

/**
 * Download and parse the root file.
 */
void CASCRemote::loadRoot() {
	// Get root key from encoding table.
	auto rootKeyIt = encodingKeys.find(buildConfig["root"]);
	if (rootKeyIt == encodingKeys.end())
		throw std::runtime_error("No encoding entry found for root key");

	const std::string& rootKey = rootKeyIt->second;

	logging::timeLog();
	core::progressLoadingScreen("Loading root table");

	auto rootCached = cache->getFile(std::string(constants::CACHE::BUILD_ROOT));
	BufferWrapper rootData;
	if (!rootCached.has_value()) {
		// Root file not cached, download.
		logging::write("Root file for build " + build["BuildConfig"] + " not cached, downloading.");

		rootData = getDataFile(formatCDNKey(rootKey));
		cache->storeFile(std::string(constants::CACHE::BUILD_ROOT), rootData);
	} else {
		rootData = std::move(rootCached.value());
	}

	logging::timeEnd("Loaded root file (" + generics::filesize(static_cast<double>(rootData.byteLength())) + ")");

	// Parse root file.
	logging::timeLog();
	core::progressLoadingScreen("Parsing root file");
	const size_t rootEntryCount = parseRootFile(std::move(rootData), rootKey);
	logging::timeEnd("Parsed root file (" + std::to_string(rootEntryCount) + " entries, " + std::to_string(rootTypes.size()) + " types)");
}

/**
 * Download and parse archive files.
 */
void CASCRemote::loadArchives() {
	// Download archive indexes.
	// Split archive keys by space.
	const std::string& archivesStr = cdnConfig["archives"];
	std::vector<std::string> archiveKeys;
	{
		std::istringstream iss(archivesStr);
		std::string token;
		while (iss >> token)
			archiveKeys.push_back(token);
	}
	const size_t archiveCount = archiveKeys.size();

	logging::timeLog();

	core::progressLoadingScreen("Loading archives");

	generics::queue<std::string>(archiveKeys, [this](const std::string& key) {
		parseArchiveIndex(key);
	}, 50);

	// Quick and dirty way to get the total archive size using config.
	int64_t archiveTotalSize = 0;
	if (cdnConfig.count("archivesIndexSize")) {
		std::istringstream iss(cdnConfig["archivesIndexSize"]);
		std::string token;
		while (iss >> token)
			archiveTotalSize += std::stoll(token);
	}

	logging::timeEnd(std::format("Loaded {} archives ({} entries, {})",
		archiveCount, archives.size(), generics::filesize(static_cast<double>(archiveTotalSize))));
}

/**
 * Download the CDN configuration and store the entry for our
 * selected region.
 */
void CASCRemote::loadServerConfig() {
	core::progressLoadingScreen("Fetching CDN configuration");

	// Download CDN server list.
	auto serverConfigs = getConfig(build["Product"], std::string(constants::PATCH::SERVER_CONFIG));
	logging::write("Server configs loaded: " + std::to_string(serverConfigs.size()));

	// Locate the CDN entry for our selected region.
	bool found = false;
	for (auto& e : serverConfigs) {
		if (e.count("Name") && e["Name"] == region) {
			serverConfig = e;
			found = true;
			break;
		}
	}
	if (!found)
		throw std::runtime_error("CDN config does not contain entry for region " + region);
}

/**
 * Load and parse the contents of an archive index.
 * Will use global cache and download if missing.
 * @param key
 */
void CASCRemote::parseArchiveIndex(const std::string& key) {
	const std::string fileName = key + ".index";

	auto cachedData = cache->getFile(fileName, constants::CACHE::DIR_INDEXES().string());
	BufferWrapper data;
	if (!cachedData.has_value()) {
		const std::string cdnKey = formatCDNKey(key) + ".index";
		data = getDataFile(cdnKey);
		cache->storeFile(fileName, data, constants::CACHE::DIR_INDEXES().string());
	} else {
		data = std::move(cachedData.value());
	}

	// Skip to the end of the archive to find the count.
	data.seek(-12);
	const int32_t count = data.readInt32LE();

	if (static_cast<int64_t>(count) * 24 > static_cast<int64_t>(data.byteLength()))
		throw std::runtime_error("Unable to parse archive, unexpected size: " + std::to_string(data.byteLength()));

	data.seek(0); // Reset position.

	for (int32_t i = 0; i < count; i++) {
		std::string hash = data.readHexString(16);

		// Skip zero hashes.
		if (hash == EMPTY_HASH)
			hash = data.readHexString(16);

		archives[hash] = { key, data.readInt32BE(), data.readInt32BE() };
	}
}

/**
 * Download a data file from the CDN.
 * @param file
 * @returns BufferWrapper
 */
BufferWrapper CASCRemote::getDataFile(const std::string& file) {
	return generics::downloadFile(host + "data/" + file);
}

/**
 * Download a partial chunk of a data file from the CDN.
 * @param file
 * @param ofs
 * @param len
 * @returns BufferWrapper
 */
BufferWrapper CASCRemote::getDataFilePartial(const std::string& file, int64_t ofs, int64_t len) {
	return generics::downloadFile(host + "data/" + file, "", ofs, len);
}

/**
 * Download the CDNConfig and BuildConfig.
 */
void CASCRemote::loadConfigs() {
	// Download CDNConfig and BuildConfig.
	core::progressLoadingScreen("Fetching build configurations");

	auto cdnHosts = cdn_resolver::getRankedHosts(region, serverConfig);

	cdnConfig = getCDNConfig(build["CDNConfig"], cdnHosts);
	buildConfig = getCDNConfig(build["BuildConfig"], cdnHosts);

	logging::write("CDNConfig loaded");
	logging::write("BuildConfig loaded");
}

/**
 * Resolve the fastest CDN host for this region and server configuration.
 */
void CASCRemote::resolveCDNHost() {
	core::progressLoadingScreen("Locating fastest CDN server");

	host = cdn_resolver::getBestHost(region, serverConfig);
}

/**
 * Format a CDN key for use in CDN requests.
 * 49299eae4e3a195953764bb4adb3c91f -> 49/29/49299eae4e3a195953764bb4adb3c91f
 * @param key
 */
std::string CASCRemote::formatCDNKey(const std::string& key) {
	return key.substr(0, 2) + "/" + key.substr(2, 2) + "/" + key;
}

/**
 * ensure file is in cache (unwrapped from BLTE) and return path.
 * @param encodingKey
 * @param fileDataID
 * @param suppressLog
 * @returns path to cached file
 */
std::string CASCRemote::_ensureFileInCache(const std::string& encodingKey, uint32_t fileDataID, bool suppressLog) {
	const std::string cacheFileName = encodingKey + ".data";
	const auto cachedPath = cache->getFilePath(cacheFileName, constants::CACHE::DIR_DATA().string());

	// check if already in cache
	auto cached = cache->getFile(cacheFileName, constants::CACHE::DIR_DATA().string());
	if (cached.has_value())
		return cachedPath.string();

	// retrieve and unwrap from BLTE
	if (!suppressLog)
		logging::write(std::format("caching file {} ({}) for mmap", fileDataID, listfile::getByID(fileDataID)));

	auto archIt = archives.find(encodingKey);
	BufferWrapper data;
	if (archIt != archives.end())
		data = getDataFilePartial(formatCDNKey(archIt->second.key), archIt->second.offset, archIt->second.size);
	else
		data = getDataFile(formatCDNKey(encodingKey));

	BLTEReader blte(std::move(data), encodingKey, true);
	blte.processAllBlocks();

	// write to cache
	BufferWrapper blteWrapper = std::move(static_cast<BufferWrapper&>(blte));
	cache->storeFile(cacheFileName, blteWrapper, constants::CACHE::DIR_DATA().string());

	return cachedPath.string();
}

/**
 * Get encoding info for a file by fileDataID for CDN streaming.
 * Returns FileEncodingInfo or std::nullopt.
 * @param fileDataID
 */
std::optional<FileEncodingInfo> CASCRemote::getFileEncodingInfo(uint32_t fileDataID) {
	try {
		const std::string encodingKey = CASC::getFile(fileDataID);
		auto archIt = archives.find(encodingKey);

		if (archIt != archives.end())
			return FileEncodingInfo{ encodingKey, ArchiveInfo{ archIt->second.key, archIt->second.offset, archIt->second.size } };

		return FileEncodingInfo{ encodingKey, std::nullopt };
	} catch (...) {
		return std::nullopt;
	}
}

/**
 * Get the current build ID.
 * @returns build name string
 */
std::string CASCRemote::getBuildName() {
	auto it = build.find("VersionsName");
	return (it != build.end()) ? it->second : "";
}

/**
 * Returns the build configuration key.
 * @returns build key string
 */
std::string CASCRemote::getBuildKey() {
	auto it = build.find("BuildConfig");
	return (it != build.end()) ? it->second : "";
}

} // namespace casc