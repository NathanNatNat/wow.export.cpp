/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>

/**
 * WoW Model Viewer (.chr) file parser.
 *
 * JS equivalent: wmv.js — module.exports = { wmv_parse }
 */
namespace wmv {

/**
 * Customization option (v2 format).
 */
struct WmvCustomization {
	int option_id;
	int choice_id;
};

/**
 * Legacy character detail values (v1 format).
 */
struct WmvLegacyValues {
	int skin_color = 0;
	int face_type = 0;
	int hair_color = 0;
	int hair_style = 0;
	int facial_hair = 0;
};

/**
 * Result of parsing a .chr file.
 */
struct WmvParseResult {
	int race = 0;
	int gender = 0;
	std::vector<WmvCustomization> customizations;    // v2 only
	std::optional<WmvLegacyValues> legacy_values;    // v1 only
	std::unordered_map<int, int> equipment;           // slot_id -> item_id
	std::string model_path;
};

/**
 * Parse a WoW Model Viewer .chr file.
 * @param xml_str The XML content of the .chr file.
 * @returns Parsed character data.
 * @throws std::runtime_error if the file is invalid or unsupported.
 */
WmvParseResult wmv_parse(std::string_view xml_str);

} // namespace wmv
