/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

namespace ExternalLinks {

inline constexpr const char* WOWHEAD_ITEM = "https://www.wowhead.com/item={}";

void open(const std::string& link);

void wowHead_viewItem(int itemID);

} // namespace ExternalLinks
