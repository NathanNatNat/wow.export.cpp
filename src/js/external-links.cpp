/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "external-links.h"

#include <format>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

namespace ExternalLinks {

void open(const std::string& link) {
#ifdef _WIN32
	int required = MultiByteToWideChar(CP_UTF8, 0, link.c_str(), -1, nullptr, 0);
	if (required <= 0)
		return;
	std::wstring wurl(static_cast<size_t>(required), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, link.c_str(), -1, wurl.data(), required);
	ShellExecuteW(nullptr, L"open", wurl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	pid_t pid = fork();
	if (pid == 0) {
		execlp("xdg-open", "xdg-open", link.c_str(), nullptr);
		_exit(127);
	}
#endif
}

void wowHead_viewItem(int itemID) {
	open(std::format(WOWHEAD_ITEM, itemID));
}

} // namespace ExternalLinks
