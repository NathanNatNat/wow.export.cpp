/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

#include <nlohmann/json.hpp>

/**
 * Cache collector worker module.
 *
 * JS equivalent: Worker thread (worker_threads) that:
 *   - Reads .build.info and .flavor.info to identify installed WoW products
 *   - Scans for binary executables and hashes them (MD5)
 *   - Scans WDB/ADB cache directories for cache files
 *   - Uploads changed cache files in chunks to a remote endpoint
 *   - Maintains persistent state to avoid re-uploading unchanged files
 *
 * C++ equivalent: Runs on a std::jthread, communicates via a message callback
 * instead of parentPort.postMessage.
 */
namespace cache_collector {

/// Minimum size for WDB files to be considered valid.
inline constexpr int64_t WDB_MIN_SIZE = 32;

/// Chunk size for multipart uploads (5 MB).
inline constexpr int64_t CHUNK_SIZE = 5 * 1024 * 1024;

/// Maximum age of cache files to upload (30 days in milliseconds).
inline constexpr int64_t MAX_AGE_MS = 30LL * 24 * 60 * 60 * 1000;

/// Pattern for binary executable files.
inline constexpr std::string_view BINARY_EXE_PATTERN_SUFFIX = ".exe";

/// macOS app bundle directory suffix.
inline constexpr std::string_view BINARY_APP_DIR = ".app";

/**
 * A single cache file entry (WDB or ADB).
 */
struct CacheFileEntry {
	std::string name;
	std::string locale;
	int64_t size = 0;
	std::filesystem::path path;
	int64_t modified_at = 0; // milliseconds since epoch
};

/**
 * Result of scanning a single flavor directory.
 */
struct FlavorResult {
	std::string product;
	std::string patch;
	std::string build_number;
	std::string build_key;
	std::string cdn_key;
	std::unordered_map<std::string, std::string> binary_hashes;
	std::vector<CacheFileEntry> cache_files;
};

/**
 * HTTP response (simplified).
 */
struct HttpResponse {
	int status = 0;
	bool ok = false;
	std::string data;
};

/**
 * JSON POST response.
 */
struct JsonPostResponse {
	int status = 0;
	bool ok = false;
	nlohmann::json response_json;
};

/**
 * Worker configuration (equivalent to JS workerData).
 */
struct WorkerConfig {
	std::filesystem::path install_path;
	std::filesystem::path state_path;
	std::string machine_id;
	std::string submit_url;
	std::string finalize_url;
	std::string user_agent;
};

/// Message callback type (replaces parentPort.postMessage).
using LogCallback = std::function<void(const std::string&)>;

/**
 * Perform an HTTPS request.
 * JS equivalent: https_request(url, options, body)
 */
HttpResponse https_request(const std::string& url,
                           const std::string& method,
                           const std::unordered_map<std::string, std::string>& headers,
                           const std::string& body = "");

/**
 * POST JSON to a URL and parse the response.
 * JS equivalent: json_post(url, payload, user_agent)
 */
JsonPostResponse json_post(const std::string& url, const nlohmann::json& payload, const std::string& user_agent);

/**
 * Build a multipart/form-data body for chunk upload.
 * JS equivalent: build_multipart(boundary, file_buf, offset)
 */
std::vector<uint8_t> build_multipart(const std::string& boundary, const std::vector<uint8_t>& file_buf, int64_t offset);

/**
 * Upload a buffer in chunks via multipart POST.
 * JS equivalent: upload_chunks(url, buffer)
 */
void upload_chunks(const std::string& url, const std::vector<uint8_t>& buffer);

/**
 * Parse .build.info text into rows of key-value pairs.
 * JS equivalent: parse_build_info(text)
 */
std::vector<std::unordered_map<std::string, std::string>> parse_build_info(const std::string& text);

/**
 * Compute MD5 hash of a file.
 * JS equivalent: hash_file(file_path)
 */
std::string hash_file(const std::filesystem::path& file_path);

/**
 * Find binary executables in a flavor directory.
 * JS equivalent: find_binaries(flavor_dir)
 */
std::vector<std::filesystem::path> find_binaries(const std::filesystem::path& flavor_dir);

/**
 * Scan a macOS .app bundle for binaries.
 * JS equivalent: scan_app_bundle(app_dir)
 */
std::vector<std::filesystem::path> scan_app_bundle(const std::filesystem::path& app_dir);

/**
 * Scan for WDB cache files in a flavor directory.
 * JS equivalent: scan_wdb(flavor_dir)
 */
std::vector<CacheFileEntry> scan_wdb(const std::filesystem::path& flavor_dir);

/**
 * Scan for ADB cache files in a flavor directory.
 * JS equivalent: scan_adb(flavor_dir)
 */
std::vector<CacheFileEntry> scan_adb(const std::filesystem::path& flavor_dir);

/**
 * Load persistent state from disk.
 * JS equivalent: load_state(state_path)
 */
nlohmann::json load_state(const std::filesystem::path& state_path);

/**
 * Save persistent state to disk.
 * JS equivalent: save_state(state_path, state)
 */
void save_state(const std::filesystem::path& state_path, const nlohmann::json& state);

/**
 * Upload cache files for a single flavor.
 * JS equivalent: upload_flavor(result, state)
 */
void upload_flavor(const FlavorResult& result, nlohmann::json& state, const WorkerConfig& config, const LogCallback& log);

/**
 * Main collection entry point.
 * Scans the install path, identifies flavors, collects caches, and uploads.
 * JS equivalent: collect()
 */
void collect(const WorkerConfig& config, const LogCallback& log);

} // namespace cache_collector
