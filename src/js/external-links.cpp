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

const std::unordered_map<std::string, std::string> STATIC_LINKS = {
	{"::WEBSITE",       "https://www.kruithne.net/wow.export/"},
	{"::DISCORD",       "https://discord.gg/kC3EzAYBtf"},
	{"::PATREON",       "https://patreon.com/Kruithne"},
	{"::GITHUB",        "https://github.com/Kruithne/wow.export"},
	{"::ISSUE_TRACKER", "https://github.com/Kruithne/wow.export/issues"}
};

std::string resolve(const std::string& link) {
	if (link.size() >= 2 && link[0] == ':' && link[1] == ':') {
		auto it = STATIC_LINKS.find(link);
		if (it != STATIC_LINKS.end())
			return it->second;
	}
	return link;
}

void open(const std::string& link) {
	std::string url = resolve(link);
#ifdef _WIN32
	int required = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
	if (required <= 0)
		return;
	std::wstring wurl(static_cast<size_t>(required), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, wurl.data(), required);
	ShellExecuteW(nullptr, L"open", wurl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	// Use fork+execlp to pass the URL as a direct argument to xdg-open, avoiding
	// shell expansion entirely. std::system() with string concatenation is unsafe
	// because a URL containing `"`, backticks, or `$()` can inject shell commands.
	// JS equivalent: nw.Shell.openExternal(link) — which is also injection-safe.
	pid_t pid = fork();
	if (pid == 0) {
		execlp("xdg-open", "xdg-open", url.c_str(), nullptr);
		_exit(127); // exec failed
	}
	// Parent: child runs independently, no need to waitpid
#endif
}

void wowHead_viewItem(int itemID) {
	open(std::format(WOWHEAD_ITEM, itemID));
}

// renderLink() is an intentional ImGui-specific addition with no JS equivalent.
// In the JS app, clickable links are handled by a global DOM click handler in
// app.js that opens any element with a `data-external` attribute. Dear ImGui has
// no DOM, so each call site must invoke this function explicitly instead.
void renderLink(const char* link, const char* label, const ImVec4* color) {
	if (color)
		ImGui::PushStyleColor(ImGuiCol_Text, *color);
	ImGui::TextUnformatted(label);
	if (color)
		ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

	if (ImGui::IsItemClicked())
		open(link);
}

} // namespace ExternalLinks
