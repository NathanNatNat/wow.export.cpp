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
#include <functional>
#include <filesystem>
#include <optional>
#include <unordered_map>

#include <nlohmann/json_fwd.hpp>

class BufferWrapper;

/**
 * Generic utility functions.
 *
 * JS equivalent: module.exports = { getJSON, readJSON, parseJSON, filesize,
 *     getFileHash, createDirectory, downloadFile, ping, get, queue,
 *     redraw, fileExists, directoryIsWritable, readFile, deleteDirectory,
 *     formatPlaybackSeconds, batchWork }
 */
namespace generics {

/**
 * HTTP response structure — C++ equivalent of the JS fetch Response object.
 */
struct HttpResponse {
	int status = 0;
	std::string statusText;
	std::vector<uint8_t> body;
	std::unordered_map<std::string, std::string> headers;
	bool ok = false; // true if status is 200–299

	std::string text() const;
	nlohmann::json json() const;
};

/**
 * Async wrapper for HTTP GET.
 * Supports URL fallback chains (tries each URL in order).
 * @param url Single URL or list of fallback URLs.
 * @returns Fetch-style response object.
 */
HttpResponse get(const std::string& url);
HttpResponse get(const std::vector<std::string>& urls);

/**
 * Dispatch an async handler for an array of items with a limit to how
 * many can be resolving at once.
 * @param items    Each one is passed to the handler.
 * @param handler  Called for each item.
 * @param limit    This many will be resolving at any given time.
 */
template <typename T>
void queue(const std::vector<T>& items,
           const std::function<void(const T&)>& handler,
           size_t limit);

/**
 * Ping a URL and measure the response time in milliseconds.
 * @param url URL to ping.
 * @returns Response time in milliseconds.
 */
int64_t ping(const std::string& url);

/**
 * Attempt to parse JSON, returning std::nullopt on failure.
 * @param data JSON string.
 */
std::optional<nlohmann::json> parseJSON(std::string_view data);

/**
 * Obtain JSON from a remote end-point.
 * @param url URL to fetch JSON from.
 */
nlohmann::json getJSON(const std::string& url);

/**
 * Read a JSON file from disk, returning std::nullopt on error.
 * Provides basic pruning for comments (lines starting with //) with ignoreComments.
 * @param file           Path to the JSON file.
 * @param ignoreComments If true, strips comment lines before parsing.
 */
std::optional<nlohmann::json> readJSON(const std::filesystem::path& file, bool ignoreComments = false);

/**
 * Download a file (optionally to a local file).
 * Zlib inflation will be applied if deflate is true.
 * Data is always returned even if `out` is provided.
 * @param url        Remote URL (or list of fallback URLs).
 * @param out        Optional file path to save downloaded data.
 * @param partialOfs Partial content start offset (-1 to disable).
 * @param partialLen Partial content length (-1 to disable).
 * @param deflate    If true, will inflate (decompress) the data.
 */
BufferWrapper downloadFile(const std::string& url, const std::string& out = "",
                           int64_t partialOfs = -1, int64_t partialLen = -1, bool deflate = false);
BufferWrapper downloadFile(const std::vector<std::string>& urls, const std::string& out = "",
                           int64_t partialOfs = -1, int64_t partialLen = -1, bool deflate = false);

/**
 * Create all directories in a given path if they do not exist.
 * @param dir Directory path.
 */
void createDirectory(const std::filesystem::path& dir);

/**
 * Format a number (bytes) to a displayable file size (JEDEC format).
 * @param input Number of bytes.
 */
std::string filesize(double input);

/**
 * Calculate the hash of a file.
 * @param file     Path to the file to hash.
 * @param method   Hashing method (e.g. "md5").
 * @param encoding Output encoding (e.g. "hex").
 */
std::string getFileHash(const std::filesystem::path& file, std::string_view method, std::string_view encoding);

/**
 * Check if a file exists.
 * @param file Path to the file.
 */
bool fileExists(const std::filesystem::path& file);

/**
 * Check if a directory exists and is writable.
 * @param dir Path to the directory.
 */
bool directoryIsWritable(const std::filesystem::path& dir);

/**
 * Read a portion of a file.
 * @param file   Path of the file.
 * @param offset Offset to start reading from.
 * @param length Total bytes to read.
 */
BufferWrapper readFile(const std::filesystem::path& file, size_t offset, size_t length);

/**
 * Recursively delete a directory and everything inside of it.
 * Returns the total size of all files deleted.
 * @param dir Directory to delete.
 */
uintmax_t deleteDirectory(const std::filesystem::path& dir);

/**
 * Process work in non-blocking batches.
 * Allows large amounts of work to be done without freezing the UI.
 * @param name      Name for logging purposes.
 * @param work      Array of items to process.
 * @param processor Function called for each item: (item, index) => void
 * @param batchSize Items to process per batch.
 */
template <typename T>
void batchWork(std::string_view name,
               const std::vector<T>& work,
               const std::function<void(const T&, size_t)>& processor,
               size_t batchSize = 1000);

/**
 * Return a formatted representation of seconds.
 * Example: 26 will return "00:26"
 * @param seconds Number of seconds.
 */
std::string formatPlaybackSeconds(double seconds);

/**
 * Returns after a redraw.
 * Uses two cooperative scheduling yields, matching JS's double
 * requestAnimationFrame ordering guarantee.
 */
void redraw();

} // namespace generics
