/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "realmlist.h"

#include "../core.h"
#include "../log.h"
#include "../constants.h"
#include "../generics.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <format>
#include <string>
#include <stdexcept>

namespace {

/**
 * Parse a realm list JSON object and populate core::view->realmList.
 * @param data JSON object keyed by region tag, each containing a "realms" array.
 */
void parseRealmList(const nlohmann::json& data) {
	nlohmann::json realms = nlohmann::json::object();

	int realmCount = 0;
	int regionCount = 0;

	for (const auto& [regionTag, region] : data.items()) {
		regionCount++;
		nlohmann::json realmArray = nlohmann::json::array();
		for (const auto& realm : region["realms"]) {
			realmCount++;
			realmArray.push_back({
				{ "name", realm["name"] },
				{ "id", realm["id"] },
				{ "slug", realm["slug"] }
			});
		}
		realms[regionTag] = std::move(realmArray);
	}

	logging::write(std::format("Loaded {} realms in {} regions.", realmCount, regionCount));

	core::view->realmList = std::move(realms);
}

} // anonymous namespace

namespace casc {
namespace realmlist {

void load() {
	logging::write("Loading realmlist...");

	// TODO 194: Match JS behavior — JS does `String(core.view.config.realmListURL)`
	// which converts any value (including undefined) to a string. We match by
	// accepting any JSON type and converting to string.
	std::string url;
	if (core::view->config.contains("realmListURL")) {
		const auto& val = core::view->config["realmListURL"];
		if (val.is_string())
			url = val.get<std::string>();
		else if (!val.is_null())
			url = val.dump(); // converts number/bool/etc. to string like JS String()
	}

	if (url.empty())
		throw std::runtime_error("Missing/malformed realmListURL in configuration!");

	// Try loading cached realmlist from disk.
	try {
		std::ifstream ifs(constants::CACHE::REALMLIST());
		if (ifs.is_open()) {
			std::string content((std::istreambuf_iterator<char>(ifs)),
			                     std::istreambuf_iterator<char>());
			nlohmann::json realmList = nlohmann::json::parse(content);
			parseRealmList(realmList);
		} else {
			logging::write("Failed to load realmlist from disk (not cached)");
		}
	} catch (const std::exception&) {
		logging::write("Failed to load realmlist from disk (not cached)");
	}

	// Fetch latest realmlist from remote URL.
	try {
		auto res = generics::get(url);
		if (res.ok) {
			std::string json_text = res.text();
			nlohmann::json json = nlohmann::json::parse(json_text);
			parseRealmList(json);

			std::ofstream ofs(constants::CACHE::REALMLIST(), std::ios::trunc);
			if (ofs.is_open())
				ofs << json_text;
		} else {
			logging::write(std::format("Failed to retrieve realmlist from {} ({})", url, res.status));
		}
	} catch (const std::exception& e) {
		logging::write(std::format("Failed to retrieve realmlist from {}: {}", url, e.what()));
	}
}

} // namespace realmlist
} // namespace casc
