/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "generics.h"
#include "buffer.h"
#include "constants.h"
#include "log.h"

#include <cmath>
#include <cstring>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <format>
#include <thread>
#include <future>
#include <functional>

#include <nlohmann/json.hpp>
#include <httplib.h>
#include <zlib.h>

namespace generics {

namespace {

// ── JEDEC file size units ────────────────────────────────────────

constexpr const char* JEDEC[] = {
	"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"
};

/**
 * Parse a URL into scheme, host, port and path components.
 */
struct ParsedURL {
	std::string scheme;
	std::string host;
	int port;
	std::string path;
};

ParsedURL parseURL(const std::string& url) {
	ParsedURL result;

	size_t schemeEnd = url.find("://");
	if (schemeEnd == std::string::npos)
		throw std::runtime_error("Invalid URL: " + url);

	result.scheme = url.substr(0, schemeEnd);
	size_t hostStart = schemeEnd + 3;
	size_t pathStart = url.find('/', hostStart);
	std::string hostPort;

	if (pathStart == std::string::npos) {
		hostPort = url.substr(hostStart);
		result.path = "/";
	} else {
		hostPort = url.substr(hostStart, pathStart - hostStart);
		result.path = url.substr(pathStart);
	}

	size_t colonPos = hostPort.find(':');
	if (colonPos != std::string::npos) {
		result.host = hostPort.substr(0, colonPos);
		result.port = std::stoi(hostPort.substr(colonPos + 1));
	} else {
		result.host = hostPort;
		result.port = (result.scheme == "https") ? 443 : 80;
	}

	return result;
}

/**
 * Perform a single HTTP GET request and return the response body as bytes.
 * Handles HTTPS vs HTTP via cpp-httplib.
 */
std::vector<uint8_t> doHttpGet(const std::string& url,
                               int64_t partialOfs = -1,
                               int64_t partialLen = -1) {
	auto parsed = parseURL(url);

	httplib::Headers headers;
	headers.emplace("User-Agent", std::string(constants::USER_AGENT()));

	if (partialOfs > -1 && partialLen > -1) {
		headers.emplace("Range", std::format("bytes={}-{}", partialOfs, partialOfs + partialLen - 1));
	}

	httplib::Result res;

	if (parsed.scheme == "https") {
		httplib::SSLClient cli(parsed.host, parsed.port);
		cli.set_connection_timeout(30);
		cli.set_read_timeout(60);
		cli.set_follow_location(true);
		res = cli.Get(parsed.path, headers);
	} else {
		httplib::Client cli(parsed.host, parsed.port);
		cli.set_connection_timeout(30);
		cli.set_read_timeout(60);
		cli.set_follow_location(true);
		res = cli.Get(parsed.path, headers);
	}

	if (!res)
		throw std::runtime_error(std::format("HTTP request failed for {}: {}", url, httplib::to_string(res.error())));

	if (res->status < 200 || res->status > 302)
		throw std::runtime_error(std::format("Status Code: {}", res->status));

	const auto& body = res->body;
	return std::vector<uint8_t>(body.begin(), body.end());
}

/**
 * Inflate (decompress) zlib data.
 */
std::vector<uint8_t> inflateData(const std::vector<uint8_t>& input) {
	z_stream strm{};
	strm.next_in = const_cast<uint8_t*>(input.data());
	strm.avail_in = static_cast<uInt>(input.size());

	if (inflateInit(&strm) != Z_OK)
		throw std::runtime_error("Failed to initialize zlib inflate");

	std::vector<uint8_t> output;
	uint8_t chunk[16384];

	int ret;
	do {
		strm.next_out = chunk;
		strm.avail_out = sizeof(chunk);
		ret = inflate(&strm, Z_NO_FLUSH);

		if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
			inflateEnd(&strm);
			throw std::runtime_error("zlib inflate error");
		}

		size_t have = sizeof(chunk) - strm.avail_out;
		output.insert(output.end(), chunk, chunk + have);
	} while (ret != Z_STREAM_END);

	inflateEnd(&strm);
	return output;
}

/**
 * Compute MD5 hash of a file. Reuses the approach from buffer.cpp.
 * Returns the hash in the specified encoding.
 */
std::string computeFileHash(const std::filesystem::path& file,
                            std::string_view method,
                            std::string_view encoding) {
	// Read the entire file into a BufferWrapper and use its calculateHash method
	auto buf = BufferWrapper::readFile(file);
	return buf.calculateHash(method, encoding);
}

} // anonymous namespace

/**
 * Async wrapper for HTTP GET.
 * Supports URL fallback chains (tries each URL in order).
 * @param url Single URL.
 * @returns Raw response body as a vector of bytes.
 */
std::vector<uint8_t> get(const std::string& url) {
	return get(std::vector<std::string>{url});
}

/**
 * Async wrapper for HTTP GET.
 * Supports URL fallback chains (tries each URL in order).
 * @param urls List of fallback URLs.
 * @returns Raw response body as a vector of bytes.
 */
std::vector<uint8_t> get(const std::vector<std::string>& urls) {
	size_t index = 1;
	std::vector<uint8_t> result;
	bool success = false;

	for (const auto& url : urls) {
		logging::write(std::format("get -> [{}/{}]: {}", index, urls.size(), url));

		try {
			result = doHttpGet(url);
			logging::write(std::format("get -> [{}][200] {}", index, url));
			success = true;
			break;
		} catch (const std::exception& error) {
			logging::write(std::format("fetch failed {}: {}", url, error.what()));
			index++;
			if (index > urls.size())
				throw;
		}
	}

	if (!success)
		throw std::runtime_error("All URLs failed");

	return result;
}

/**
 * Dispatch a handler for an array of items with a limit to how
 * many can be resolving at once.
 * @param items    Each one is passed to the handler.
 * @param handler  Called for each item.
 * @param limit    This many will be resolving at any given time.
 */
template <typename T>
void queue(const std::vector<T>& items,
           const std::function<void(const T&)>& handler,
           size_t limit) {
	size_t index = 0;
	size_t complete = 0;
	std::vector<std::future<void>> futures;

	while (complete < items.size()) {
		// Launch up to 'limit' concurrent tasks
		while (futures.size() < limit && index < items.size()) {
			const T& item = items[index];
			futures.push_back(std::async(std::launch::async, [&handler, &item]() {
				handler(item);
			}));
			index++;
		}

		// Wait for any one to complete
		if (!futures.empty()) {
			futures.front().get();
			futures.erase(futures.begin());
			complete++;
		}
	}
}

/**
 * Ping a URL and measure the response time in milliseconds.
 * Not perfectly accurate, but good enough for our purposes.
 * Throws on error or HTTP code other than 200.
 * @param url URL to ping.
 */
int64_t ping(const std::string& url) {
	auto pingStart = std::chrono::steady_clock::now();

	get(url);

	auto elapsed = std::chrono::steady_clock::now() - pingStart;
	return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

/**
 * Attempt to parse JSON, returning std::nullopt on failure.
 * Inline function to keep code paths clean of unnecessary try/catch blocks.
 * @param data JSON string.
 */
std::optional<nlohmann::json> parseJSON(std::string_view data) {
	try {
		return nlohmann::json::parse(data);
	} catch (const nlohmann::json::exception&) {
		return std::nullopt;
	}
}

/**
 * Obtain JSON from a remote end-point.
 * @param url URL to fetch JSON from.
 */
nlohmann::json getJSON(const std::string& url) {
	auto data = get(url);
	std::string_view sv(reinterpret_cast<const char*>(data.data()), data.size());
	return nlohmann::json::parse(sv);
}

/**
 * Read a JSON file from disk, returning std::nullopt on error.
 * Provides basic pruning for comments (lines starting with //) with ignoreComments.
 * @param file           Path to the JSON file.
 * @param ignoreComments If true, strips comment lines before parsing.
 */
std::optional<nlohmann::json> readJSON(const std::filesystem::path& file, bool ignoreComments) {
	try {
		std::ifstream ifs(file);
		if (!ifs.is_open()) {
			// Check for EPERM equivalent — rethrow access errors
			auto ec = std::error_code{};
			auto perms = std::filesystem::status(file, ec).permissions();
			if (!ec && (perms & std::filesystem::perms::owner_read) == std::filesystem::perms::none)
				throw std::runtime_error("Permission denied: " + file.string());
			return std::nullopt;
		}

		std::string raw((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		if (ignoreComments) {
			std::istringstream stream(raw);
			std::string line;
			std::string filtered;

			while (std::getline(stream, line)) {
				// Remove trailing \r if present
				if (!line.empty() && line.back() == '\r')
					line.pop_back();

				if (!line.starts_with("//"))
					filtered += line + '\n';
			}

			return nlohmann::json::parse(filtered);
		}

		return nlohmann::json::parse(raw);
	} catch (const std::runtime_error&) {
		throw; // Re-throw permission errors
	} catch (const std::exception&) {
		return std::nullopt;
	}
}

/**
 * Request data from a URL with optional partial content (Range header).
 * Handles redirects (301/302) and logs download progress.
 * @param url        URL to request data from.
 * @param partialOfs Partial content start offset (-1 to disable).
 * @param partialLen Partial content length (-1 to disable).
 * @returns Raw response body.
 */
std::vector<uint8_t> requestData(const std::string& url, int64_t partialOfs, int64_t partialLen) {
	logging::write(std::format("Requesting data from {} (offset: {}, length: {})", url, partialOfs, partialLen));
	auto data = doHttpGet(url, partialOfs, partialLen);
	logging::write(std::format("Download complete: {} bytes received", data.size()));
	return data;
}

/**
 * Download a file (optionally to a local file).
 * Zlib inflation will be applied if deflate is true.
 * Data is always returned even if `out` is provided.
 * @param url        Remote URL.
 * @param out        Optional file path to save downloaded data.
 * @param partialOfs Partial content start offset (-1 to disable).
 * @param partialLen Partial content length (-1 to disable).
 * @param deflate    If true, will inflate (decompress) the data.
 */
BufferWrapper downloadFile(const std::string& url, const std::string& out,
                           int64_t partialOfs, int64_t partialLen, bool doDeflate) {
	return downloadFile(std::vector<std::string>{url}, out, partialOfs, partialLen, doDeflate);
}

/**
 * Download a file (optionally to a local file).
 * Supports URL fallback chains.
 * @param urls       List of fallback URLs.
 * @param out        Optional file path to save downloaded data.
 * @param partialOfs Partial content start offset (-1 to disable).
 * @param partialLen Partial content length (-1 to disable).
 * @param deflate    If true, will inflate (decompress) the data.
 */
BufferWrapper downloadFile(const std::vector<std::string>& urls, const std::string& out,
                           int64_t partialOfs, int64_t partialLen, bool doDeflate) {
	for (const auto& currentUrl : urls) {
		try {
			logging::write(std::format("downloadFile -> {}", currentUrl));

			auto data = requestData(currentUrl, partialOfs, partialLen);

			if (doDeflate)
				data = inflateData(data);

			BufferWrapper wrapped(std::move(data));

			if (!out.empty()) {
				createDirectory(std::filesystem::path(out).parent_path());
				wrapped.writeToFile(out);
			}

			return wrapped;
		} catch (const std::exception& error) {
			logging::write(std::format("Failed to download from {}: {}", currentUrl, error.what()));
		}
	}

	throw std::runtime_error("All download attempts failed.");
}

/**
 * Create all directories in a given path if they do not exist.
 * @param dir Directory path.
 */
void createDirectory(const std::filesystem::path& dir) {
	std::filesystem::create_directories(dir);
}

/**
 * Returns after a redraw.
 * In C++ (ImGui), this is a no-op since ImGui redraws every frame.
 */
void redraw() {
	// In ImGui, the main loop redraws every frame.
	// This is a no-op placeholder for JS compatibility.
}

/**
 * Format a number (bytes) to a displayable file size.
 * Simplified version of filesize.js using JEDEC units.
 * @param input Number of bytes.
 */
std::string filesize(double input) {
	if (std::isnan(input))
		return "NaN";

	const bool isNegative = input < 0;

	// Flipping a negative number to determine the size.
	if (isNegative)
		input = -input;

	// Determining the exponent.
	int exponent = (input > 0) ? static_cast<int>(std::floor(std::log(input) / std::log(1024.0))) : 0;
	if (exponent < 0)
		exponent = 0;

	// Exceeding supported length, time to reduce & multiply.
	if (exponent > 8)
		exponent = 8;

	std::string valueStr;
	std::string unit;

	// Zero is now a special case because bytes divide by 1.
	if (input == 0) {
		valueStr = "0";
		unit = JEDEC[exponent];
	} else {
		double val = input / std::pow(2.0, exponent * 10);

		if (exponent > 0) {
			// Format with 2 decimal places, then strip trailing zeros
			// to match JS: Number(val.toFixed(2)) — e.g. 1.00 → "1", 1.50 → "1.5"
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(2) << val;
			valueStr = oss.str();

			// Strip trailing zeros after decimal point
			if (valueStr.find('.') != std::string::npos) {
				size_t lastNonZero = valueStr.find_last_not_of('0');
				if (lastNonZero != std::string::npos && valueStr[lastNonZero] == '.')
					valueStr.erase(lastNonZero); // Remove decimal point too
				else
					valueStr.erase(lastNonZero + 1);
			}
		} else {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << val;
			valueStr = oss.str();
		}

		// Check if rounded value equals 1024 and bump exponent
		double parsed = std::stod(valueStr);
		if (parsed == 1024.0 && exponent < 8) {
			valueStr = "1";
			exponent++;
		}

		unit = JEDEC[exponent];
	}

	// Decorating a 'diff'.
	if (isNegative)
		valueStr = "-" + valueStr;

	return valueStr + " " + unit;
}

/**
 * Calculate the hash of a file.
 * @param file     Path to the file to hash.
 * @param method   Hashing method (e.g. "md5").
 * @param encoding Output encoding (e.g. "hex").
 */
std::string getFileHash(const std::filesystem::path& file, std::string_view method, std::string_view encoding) {
	return computeFileHash(file, method, encoding);
}

/**
 * Wrapper for checking if a file exists.
 * @param file Path to the file.
 */
bool fileExists(const std::filesystem::path& file) {
	try {
		return std::filesystem::exists(file);
	} catch (const std::exception&) {
		return false;
	}
}

/**
 * Check if a directory exists and is writable.
 * @param dir Path to the directory.
 */
bool directoryIsWritable(const std::filesystem::path& dir) {
	try {
		auto status = std::filesystem::status(dir);
		if (!std::filesystem::is_directory(status))
			return false;

		auto perms = status.permissions();
		return (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
	} catch (const std::exception&) {
		return false;
	}
}

/**
 * Read a portion of a file.
 * @param file   Path of the file.
 * @param offset Offset to start reading from.
 * @param length Total bytes to read.
 */
BufferWrapper readFile(const std::filesystem::path& file, size_t offset, size_t length) {
	std::ifstream fd(file, std::ios::binary);
	if (!fd.is_open())
		throw std::runtime_error("Failed to open file: " + file.string());

	BufferWrapper buf = BufferWrapper::alloc(length);

	fd.seekg(static_cast<std::streamoff>(offset));
	fd.read(reinterpret_cast<char*>(buf.raw().data()), static_cast<std::streamsize>(length));
	fd.close();

	return buf;
}

/**
 * Recursively delete a directory and everything inside of it.
 * Returns the total size of all files deleted.
 * @param dir Directory to delete.
 */
uintmax_t deleteDirectory(const std::filesystem::path& dir) {
	uintmax_t deleteSize = 0;
	try {
		for (const auto& entry : std::filesystem::directory_iterator(dir)) {
			if (entry.is_directory()) {
				deleteSize += deleteDirectory(entry.path());
			} else {
				deleteSize += entry.file_size();
				std::filesystem::remove(entry.path());
			}
		}

		std::filesystem::remove(dir);
	} catch (const std::exception&) {
		// Something failed to delete.
	}

	return deleteSize;
}

/**
 * Process work in non-blocking batches.
 * Allows large amounts of work to be done without freezing the UI.
 *
 * @param name      Name for logging purposes.
 * @param work      Array of items to process.
 * @param processor Function called for each item: (item, index) => void
 * @param batchSize Items to process per batch.
 */
template <typename T>
void batchWork(std::string_view name,
               const std::vector<T>& work,
               const std::function<void(const T&, size_t)>& processor,
               size_t batchSize) {
	size_t index = 0;
	size_t total = work.size();
	auto startTime = std::chrono::steady_clock::now();
	int lastProgressPercent = 0;

	logging::write(std::format("Starting batch work \"{}\" with {} items...", name, total));

	while (index < total) {
		size_t endIndex = std::min(index + batchSize, total);

		for (size_t i = index; i < endIndex; i++)
			processor(work[i], i);

		index = endIndex;

		int progressPercent = static_cast<int>((static_cast<double>(index) / total) * 100);
		if (progressPercent >= lastProgressPercent + 10 && progressPercent < 100) {
			logging::write(std::format("Batch work \"{}\" progress: {}% ({}/{})", name, progressPercent, index, total));
			lastProgressPercent = progressPercent;
		}
	}

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - startTime).count();
	logging::write(std::format("Batch work \"{}\" completed in {}ms ({} items)", name, duration, total));
}

/**
 * Return a formatted representation of seconds.
 * Example: 26 will return "00:26"
 * @param seconds Number of seconds.
 */
std::string formatPlaybackSeconds(double seconds) {
	if (std::isnan(seconds))
		return "00:00";

	int mins = static_cast<int>(std::floor(seconds / 60.0));
	int secs = static_cast<int>(std::round(std::fmod(seconds, 60.0)));

	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(2) << mins
	    << ':' << std::setfill('0') << std::setw(2) << secs;
	return oss.str();
}

// Explicit template instantiations for common types
// These allow the templates to be used from other translation units.
// Additional instantiations can be added as needed.

template void batchWork<std::string>(std::string_view, const std::vector<std::string>&,
	const std::function<void(const std::string&, size_t)>&, size_t);

template void batchWork<std::pair<uint32_t, std::string>>(std::string_view,
	const std::vector<std::pair<uint32_t, std::string>>&,
	const std::function<void(const std::pair<uint32_t, std::string>&, size_t)>&, size_t);

} // namespace generics
