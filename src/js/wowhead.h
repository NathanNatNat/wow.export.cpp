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
#include <optional>
#include <stdexcept>

struct WowheadResult {
	int version = 0;
	int race = 0;
	int gender = 0;
	int clazz = 0;
	int spec = 0;
	int level = 0;
	int npc_options = 0;
	int pepe = 0;
	int mount = 0;
	std::vector<int> customizations;
	std::unordered_map<int, int> equipment;
};

WowheadResult wowhead_parse(std::string_view url);
