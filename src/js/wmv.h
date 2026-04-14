/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace wmv {

struct Customization {
	int option_id;
	int choice_id;
};

struct LegacyValues {
	int skin_color = 0;
	int face_type = 0;
	int hair_color = 0;
	int hair_style = 0;
	int facial_hair = 0;
};

struct ParseResultV2 {
	int race;
	int gender;
	std::vector<Customization> customizations;
	std::unordered_map<int, int> equipment;
	std::string model_path;
};

struct ParseResultV1 {
	int race;
	int gender;
	LegacyValues legacy_values;
	std::unordered_map<int, int> equipment;
	std::string model_path;
};

using ParseResult = std::variant<ParseResultV1, ParseResultV2>;

/**
 * Parse a WoWModelViewer .chr XML string into a structured result.
 *
 * Supports version 1.0 (returns ParseResultV1) and version 2.0 (returns ParseResultV2).
 * Throws std::runtime_error on invalid or unsupported input.
 */
ParseResult wmv_parse(std::string_view xml_str);

} // namespace wmv
