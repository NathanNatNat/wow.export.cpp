/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace casc {

/**
 * Parse a CASC version/CDN config file.
 *
 * The first line contains pipe-separated field definitions (e.g.,
 * "Name!STRING:0|Path!STRING:0|..."). Subsequent non-empty, non-comment
 * lines are pipe-separated entries whose values are mapped to the field
 * names. Spaces in field names are stripped (e.g., "Install Key" → "InstallKey").
 *
 * @param data Raw config file content.
 * @return Vector of maps, one per entry, keyed by field name.
 */
std::vector<std::unordered_map<std::string, std::string>> parseVersionConfig(std::string_view data);

} // namespace casc
