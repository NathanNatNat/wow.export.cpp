/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace wowhead {

struct ParseResult {
	int version = 0;
	int race = 0;
	int gender = 0;
	int class_ = 0;
	int spec = 0;
	int level = 0;
	int npc_options = 0;
	int pepe = 0;
	int mount = 0;
	std::vector<int> customizations;
	std::unordered_map<int, int> equipment;
};

ParseResult wowhead_parse(const std::string& url);

} // namespace wowhead
