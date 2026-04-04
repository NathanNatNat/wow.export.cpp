/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "external-links.h"

#include <format>
#include <string>
#include <string_view>
#include <unordered_map>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#endif

namespace {

/**
 * Defines static links which can be referenced via the data-external HTML attribute.
 */
const std::unordered_map<std::string, std::string> STATIC_LINKS = {
	{"::WEBSITE", "https://www.kruithne.net/wow.export/"},
	{"::DISCORD", "https://discord.gg/kC3EzAYBtf"},
	{"::PATREON", "https://patreon.com/Kruithne"},
	{"::GITHUB", "https://github.com/Kruithne/wow.export"},
	{"::ISSUE_TRACKER", "https://github.com/Kruithne/wow.export/issues"}
};

/**
 * Defines the URL pattern for locating a specific item on Wowhead.
 */
constexpr std::string_view WOWHEAD_ITEM = "https://www.wowhead.com/item={}";

} // anonymous namespace

namespace external_links {

void open(std::string_view link) {
	std::string resolved;

	if (link.starts_with("::")) {
		auto it = STATIC_LINKS.find(std::string(link));
		if (it != STATIC_LINKS.end())
			resolved = it->second;
		else
			return;
	} else {
		resolved = std::string(link);
	}

#ifdef _WIN32
	ShellExecuteA(nullptr, "open", resolved.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	std::string cmd = "xdg-open \"" + resolved + "\" &";
	std::system(cmd.c_str());
#endif
}

void wowHead_viewItem(int itemID) {
	open(std::format("https://www.wowhead.com/item={}", itemID));
}

} // namespace external_links