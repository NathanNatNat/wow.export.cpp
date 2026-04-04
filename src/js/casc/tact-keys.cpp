/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "tact-keys.h"

#include "../log.h"
#include "../generics.h"
#include "../constants.h"
#include "../core.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <format>
#include <stdexcept>
#include <unordered_map>
#include <mutex>

namespace {

std::unordered_map<std::string, std::string> KEY_RING;
bool isSaving = false;
std::mutex keyRingMutex;

/**
 * Convert a string to lowercase in-place.
 */
std::string toLower(std::string_view sv) {
	std::string s(sv);
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

/**
 * Validate a keyName/key pair.
 * @param keyName
 * @param key
 */
bool validateKeyPair(std::string_view keyName, std::string_view key) {
	if (keyName.length() != 16)
		return false;

	if (key.length() != 32)
		return false;

	return true;
}

/**
 * Saves the tact keys to disk.
 */
void doSave() {
	nlohmann::json j = nlohmann::json::object();
	{
		std::lock_guard<std::mutex> lock(keyRingMutex);
		for (const auto& [name, value] : KEY_RING)
			j[name] = value;
	}

	std::ofstream ofs(constants::CACHE::TACT_KEYS());
	if (ofs.is_open()) {
		ofs << j.dump(1, '\t');
		ofs.close();
	}
	isSaving = false;
}

/**
 * Request for tact keys to be saved on the next tick.
 * Multiple calls can be chained in the same tick.
 */
void save() {
	if (!isSaving) {
		isSaving = true;
		// In JS this was setImmediate(doSave). In C++ we save immediately.
		doSave();
	}
}

/**
 * Split a string by a delimiter into parts.
 */
std::vector<std::string_view> splitLines(std::string_view data) {
	std::vector<std::string_view> lines;
	std::size_t start = 0;
	for (std::size_t i = 0; i < data.size(); i++) {
		if (data[i] == '\n') {
			std::string_view line = data.substr(start, i - start);
			if (!line.empty() && line.back() == '\r')
				line.remove_suffix(1);
			lines.push_back(line);
			start = i + 1;
		}
	}
	if (start <= data.size()) {
		std::string_view line = data.substr(start);
		if (!line.empty() && line.back() == '\r')
			line.remove_suffix(1);
		lines.push_back(line);
	}
	return lines;
}

/**
 * Trim whitespace from a string_view.
 */
std::string_view trim(std::string_view sv) {
	while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
		sv.remove_prefix(1);
	while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))
		sv.remove_suffix(1);
	return sv;
}

} // anonymous namespace

namespace casc {
namespace tact_keys {

std::string getKey(std::string_view keyName) {
	std::string lower = toLower(keyName);
	std::lock_guard<std::mutex> lock(keyRingMutex);
	auto it = KEY_RING.find(lower);
	if (it != KEY_RING.end())
		return it->second;
	return {};
}

bool addKey(std::string_view keyName, std::string_view key) {
	if (!validateKeyPair(keyName, key))
		return false;

	std::string lowerName = toLower(keyName);
	std::string lowerKey = toLower(key);

	{
		std::lock_guard<std::mutex> lock(keyRingMutex);
		auto it = KEY_RING.find(lowerName);
		if (it == KEY_RING.end() || it->second != lowerKey) {
			KEY_RING[lowerName] = lowerKey;
			logging::write(std::format("Registered new decryption key {} -> {}", lowerName, lowerKey));
			save();
		}
	}

	return true;
}

void load() {
	// Load from local cache.
	try {
		std::ifstream ifs(constants::CACHE::TACT_KEYS());
		if (ifs.is_open()) {
			nlohmann::json tactKeys = nlohmann::json::parse(ifs);

			// Validate/add our cached keys manually rather than passing to addKey()
			// to skip over redundant logging/saving calls.
			int added = 0;
			for (auto it = tactKeys.begin(); it != tactKeys.end(); ++it) {
				const std::string& keyName = it.key();
				const std::string key = it.value().get<std::string>();

				if (validateKeyPair(keyName, key)) {
					std::lock_guard<std::mutex> lock(keyRingMutex);
					KEY_RING[toLower(keyName)] = toLower(key);
					added++;
				} else {
					logging::write(std::format("Skipping invalid tact key from cache: {} -> {}", keyName, key));
				}
			}

			logging::write(std::format("Loaded {} tact keys from local cache.", added));
		}
	} catch (...) {
		// No tactKeys cached locally, doesn't matter.
	}

	const std::string tact_url = core::view->config.value("tactKeysURL", "");
	const std::string tact_url_fallback = core::view->config.value("tactKeysFallbackURL", "");

	std::vector<uint8_t> resData;
	try {
		resData = generics::get({tact_url, tact_url_fallback});
	} catch (...) {
		throw std::runtime_error("Unable to update tactKeys");
	}

	if (resData.empty())
		throw std::runtime_error("Unable to update tactKeys, empty response");

	std::string_view dataView(reinterpret_cast<const char*>(resData.data()), resData.size());
	auto lines = splitLines(dataView);
	int remoteAdded = 0;

	for (const auto& line : lines) {
		// Split by space.
		auto spacePos = line.find(' ');
		if (spacePos == std::string_view::npos)
			continue;

		// Check there are exactly 2 parts (no additional spaces in value).
		std::string_view keyName = trim(line.substr(0, spacePos));
		std::string_view rest = line.substr(spacePos + 1);
		// Trim leading spaces on value side.
		std::string_view key = trim(rest);

		// JS splits by ' ' and checks parts.length !== 2, meaning exactly 2 parts.
		// If there's another space in the key value, skip.
		if (key.find(' ') != std::string_view::npos)
			continue;

		if (validateKeyPair(keyName, key)) {
			std::lock_guard<std::mutex> lock(keyRingMutex);
			KEY_RING[toLower(keyName)] = toLower(key);
			remoteAdded++;
		} else {
			logging::write(std::format("Skipping invalid remote tact key: {} -> {}", keyName, key));
		}
	}

	if (remoteAdded > 0)
		logging::write(std::format("Added {} tact keys from {}", remoteAdded, tact_url));
}

} // namespace tact_keys
} // namespace casc