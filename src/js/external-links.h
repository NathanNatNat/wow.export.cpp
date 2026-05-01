/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <unordered_map>
#include <imgui.h>

namespace ExternalLinks {

extern const std::unordered_map<std::string, std::string> STATIC_LINKS;

inline constexpr const char* WOWHEAD_ITEM = "https://www.wowhead.com/item={}";

std::string resolve(const std::string& link);

void open(const std::string& link);

void wowHead_viewItem(int itemID);

void renderLink(const char* link, const char* label, const ImVec4* color = nullptr);

} // namespace ExternalLinks
