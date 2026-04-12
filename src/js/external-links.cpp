/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "external-links.h"

#include <format>
#include <string>
#include <unordered_map>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#endif

namespace ExternalLinks {

/**
 * Defines static links which can be referenced via the data-external HTML attribute.
 */
static const std::unordered_map<std::string, std::string> STATIC_LINKS = {
	{ "::WEBSITE", "https://www.kruithne.net/wow.export/" },
	{ "::DISCORD", "https://discord.gg/kC3EzAYBtf" },
	{ "::PATREON", "https://patreon.com/Kruithne" },
	{ "::GITHUB", "https://github.com/Kruithne/wow.export" },
	{ "::ISSUE_TRACKER", "https://github.com/Kruithne/wow.export/issues" }
};

/**
 * Defines the URL pattern for locating a specific item on Wowhead.
 */
static constexpr const char* WOWHEAD_ITEM = "https://www.wowhead.com/item={}";

void open(const std::string& link) {
	std::string resolved = link;

	if (resolved.starts_with("::")) {
		auto it = STATIC_LINKS.find(resolved);
		if (it != STATIC_LINKS.end())
			resolved = it->second;
	}

#ifdef _WIN32
	// Use MultiByteToWideChar for correct UTF-8 to UTF-16 conversion.
	int wlen = MultiByteToWideChar(CP_UTF8, 0, resolved.c_str(),
		static_cast<int>(resolved.size()), nullptr, 0);
	std::wstring wlink(static_cast<size_t>(wlen), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, resolved.c_str(),
		static_cast<int>(resolved.size()), wlink.data(), wlen);
	ShellExecuteW(nullptr, L"open", wlink.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	// Fork and exec to avoid shell injection from untrusted URLs.
	pid_t pid = fork();
	if (pid == 0) {
		execlp("xdg-open", "xdg-open", resolved.c_str(), nullptr);
		_exit(127);
	}
#endif
}

void wowHead_viewItem(int itemID) {
	open(std::format(WOWHEAD_ITEM, itemID));
}

} // namespace ExternalLinks
