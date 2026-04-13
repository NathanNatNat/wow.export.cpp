/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <format>
#include <unordered_map>
#include <cstdlib>
#include <imgui.h>

/**
 * ExternalLinks — centralized external link resolution and opening.
 *
 * JS equivalent: src/js/external-links.js (ExternalLinks class).
 *
 * Defines static links referenced via data-external HTML attributes in the
 * original JS, and provides open() / wowHead_viewItem() static methods.
 */
namespace ExternalLinks {

/**
 * Static links which can be referenced via the ::NAME pattern.
 * JS equivalent: const STATIC_LINKS = { ... }
 */
inline const std::unordered_map<std::string, std::string> STATIC_LINKS = {
	{"::WEBSITE",       "https://www.kruithne.net/wow.export/"},
	{"::DISCORD",       "https://discord.gg/kC3EzAYBtf"},
	{"::PATREON",       "https://patreon.com/Kruithne"},
	{"::GITHUB",        "https://github.com/Kruithne/wow.export"},
	{"::ISSUE_TRACKER", "https://github.com/Kruithne/wow.export/issues"}
};

/**
 * URL pattern for locating a specific item on Wowhead.
 * JS equivalent: const WOWHEAD_ITEM = 'https://www.wowhead.com/item=%d'
 */
inline constexpr const char* WOWHEAD_ITEM = "https://www.wowhead.com/item={}";

/**
 * Resolve a link identifier to its URL.
 * If the link starts with "::", it is looked up in STATIC_LINKS.
 * Otherwise, it is returned as-is.
 * @param link Link identifier or URL.
 * @return Resolved URL.
 */
inline std::string resolve(const std::string& link) {
	if (link.size() >= 2 && link[0] == ':' && link[1] == ':') {
		auto it = STATIC_LINKS.find(link);
		if (it != STATIC_LINKS.end())
			return it->second;
	}
	return link;
}

/**
 * Open an external link on the system.
 * If the link starts with "::", it is resolved via STATIC_LINKS first.
 * JS equivalent: ExternalLinks.open(link)
 * @param link Link identifier (e.g., "::DISCORD") or direct URL.
 */
inline void open(const std::string& link) {
	std::string url = resolve(link);
#ifdef _WIN32
	// ShellExecuteW is available via windows.h (included by the caller).
	std::wstring wurl(url.begin(), url.end());
	ShellExecuteW(nullptr, L"open", wurl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	std::string cmd = "xdg-open \"" + url + "\" &";
	std::system(cmd.c_str());
#endif
}

/**
 * Open a specific item on Wowhead.
 * JS equivalent: ExternalLinks.wowHead_viewItem(itemID)
 * @param itemID Item ID to view on Wowhead.
 */
inline void wowHead_viewItem(int itemID) {
	open(std::format("https://www.wowhead.com/item={}", itemID));
}

/**
 * Render an ImGui clickable text link that opens an external URL when clicked.
 *
 * This provides the C++ equivalent of the JS global `data-external` click
 * handler (app.js lines 115–131). In the JS, any element with a
 * `data-external` attribute automatically opens the URL on click. In Dear
 * ImGui there is no DOM, so each link must call this function explicitly.
 *
 * The link text is rendered with the specified color (defaults to current text
 * color) and shows a hand cursor on hover.
 *
 * @param link   Link identifier (e.g. "::DISCORD") or direct URL.
 * @param label  Visible text for the link.
 * @param color  Optional text color. Pass nullptr to use default ImGui text color.
 */
inline void renderLink(const char* link, const char* label, const ImVec4* color = nullptr) {
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
