/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "external-links.h"

#include <format>
#include <cstdlib>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
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
	std::string cmd = "xdg-open \"" + url + "\" &";
	std::system(cmd.c_str());
#endif
}

void wowHead_viewItem(int itemID) {
	open(std::format(WOWHEAD_ITEM, itemID));
}

void renderLink(const char* link, const char* label, const ImVec4* color) {
	if (color)
		ImGui::PushStyleColor(ImGuiCol_Text, *color);
	ImGui::TextUnformatted(label);
	if (color)
		ImGui::PopStyleColor();

	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImVec2 item_min = ImGui::GetItemRectMin();
		ImVec2 item_max = ImGui::GetItemRectMax();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddText(item_min, IM_COL32(255, 255, 255, 255), label);
		draw->AddLine(ImVec2(item_min.x, item_max.y), ImVec2(item_max.x, item_max.y), IM_COL32(255, 255, 255, 255), 1.0f);
	}

	if (ImGui::IsItemClicked())
		open(link);
}

} // namespace ExternalLinks
