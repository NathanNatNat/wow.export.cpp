/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "wmv.h"
#include "xml.h"
#include "wow/EquipmentSlots.h"

#include <algorithm>
#include <format>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace wmv {

namespace {

// Safely parse an int from a JSON value (string or number).
// Returns std::nullopt if the value is null, missing, or not parseable (equivalent to JS isNaN).
std::optional<int> safe_parse_int(const nlohmann::json& val) {
	if (val.is_null())
		return std::nullopt;

	if (val.is_number_integer())
		return val.get<int>();

	if (val.is_string()) {
		const auto& s = val.get_ref<const std::string&>();
		try {
			size_t pos = 0;
			int result = std::stoi(s, &pos);
			if (pos > 0)
				return result;
		} catch (...) {}
	}

	return std::nullopt;
}

// Safely navigate a JSON path, returning null json if any key is missing.
const nlohmann::json& safe_at(const nlohmann::json& j, const std::string& key) {
	static const nlohmann::json null_json;
	if (j.is_object() && j.contains(key))
		return j[key];
	return null_json;
}

// Extract a nested JSON value using chained keys, returning null json if any key is missing.
const nlohmann::json& safe_navigate(const nlohmann::json& j, std::initializer_list<std::string> keys) {
	static const nlohmann::json null_json;
	const nlohmann::json* current = &j;
	for (const auto& key : keys) {
		if (!current->is_object() || !current->contains(key))
			return null_json;
		current = &(*current)[key];
	}
	return *current;
}

struct RaceGender {
	int race;
	int gender;
};

RaceGender extract_race_gender_from_path(const std::string& model_path) {
	static const std::unordered_map<std::string, int> race_map = {
		{ "human", 1 },
		{ "orc", 2 },
		{ "dwarf", 3 },
		{ "nightelf", 4 },
		{ "scourge", 5 },
		{ "tauren", 6 },
		{ "gnome", 7 },
		{ "troll", 8 },
		{ "goblin", 9 },
		{ "bloodelf", 10 },
		{ "draenei", 11 },
		{ "worgen", 22 },
		{ "pandaren", 24 },
		{ "nightborne", 27 },
		{ "highmountaintauren", 28 },
		{ "voidelf", 29 },
		{ "lightforgeddraenei", 30 },
		{ "zandalaritroll", 31 },
		{ "kultiran", 32 },
		{ "darkirondwarf", 34 },
		{ "vulpera", 35 },
		{ "mechagnome", 37 },
		{ "dracthyr", 52 },
		{ "earthen", 84 },

		// todo: 36, MagharOrc
	};

	// lowercase and replace backslashes with forward slashes
	std::string path_lower = model_path;
	std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	std::replace(path_lower.begin(), path_lower.end(), '\\', '/');

	// split by '/'
	std::vector<std::string> parts;
	std::istringstream stream(path_lower);
	std::string part;
	while (std::getline(stream, part, '/'))
		parts.push_back(part);

	std::optional<int> race;
	std::optional<int> gender;

	for (const auto& p : parts) {
		auto it = race_map.find(p);
		if (it != race_map.end())
			race = it->second;

		if (p == "male" || (p.find("male") != std::string::npos && p.find("female") == std::string::npos))
			gender = 0;
		else if (p == "female" || p.find("female") != std::string::npos)
			gender = 1;
	}

	if (!race.has_value() || !gender.has_value())
		throw std::runtime_error(std::format("unable to determine race/gender from model path: {}", model_path));

	return { race.value(), gender.value() };
}

// Normalize an item list from JSON: if it's an array, return it; if it's a single object, wrap it.
std::vector<nlohmann::json> normalize_array(const nlohmann::json& val) {
	if (val.is_array())
		return val.get<std::vector<nlohmann::json>>();
	else
		return { val };
}

// Parse the equipment node common to both v1 and v2.
std::unordered_map<int, int> parse_equipment(const nlohmann::json& data) {
	std::unordered_map<int, int> equipment;

	const auto& item_node = safe_navigate(data, { "equipment", "item" });
	if (item_node.is_null())
		return equipment;

	auto items = normalize_array(item_node);

	for (const auto& item : items) {
		auto wmv_slot = safe_parse_int(safe_navigate(item, { "slot", "@_value" }));
		auto item_id = safe_parse_int(safe_navigate(item, { "id", "@_value" }));

		if (!wmv_slot.has_value() || !item_id.has_value() || item_id.value() == 0)
			continue;

		auto slot_id = wow::get_slot_id_for_wmv_slot(wmv_slot.value());
		if (slot_id.has_value())
			equipment[slot_id.value()] = item_id.value();
	}

	return equipment;
}

ParseResultV2 wmv_parse_v2(const nlohmann::json& data) {
	const auto& model_path_val = safe_navigate(data, { "model", "file", "@_name" });
	if (model_path_val.is_null() || !model_path_val.is_string())
		throw std::runtime_error("invalid .chr file: missing model path");

	std::string model_path = model_path_val.get<std::string>();
	auto [race, gender] = extract_race_gender_from_path(model_path);

	std::vector<Customization> customizations;
	const auto& char_details = safe_navigate(data, { "model", "CharDetails" });

	if (!char_details.is_null() && char_details.contains("customization")) {
		auto cust_array = normalize_array(char_details["customization"]);

		for (const auto& cust : cust_array) {
			auto option_id = safe_parse_int(safe_at(cust, "@_id"));
			auto choice_id = safe_parse_int(safe_at(cust, "@_value"));

			if (option_id.has_value() && choice_id.has_value())
				customizations.push_back({ option_id.value(), choice_id.value() });
		}
	}

	// parse equipment
	auto equipment = parse_equipment(data);

	return {
		race,
		gender,
		std::move(customizations),
		std::move(equipment),
		std::move(model_path)
	};
}

ParseResultV1 wmv_parse_v1(const nlohmann::json& data) {
	const auto& model_path_val = safe_navigate(data, { "model", "file", "@_name" });
	if (model_path_val.is_null() || !model_path_val.is_string())
		throw std::runtime_error("invalid .chr file: missing model path");

	std::string model_path = model_path_val.get<std::string>();
	auto [race, gender] = extract_race_gender_from_path(model_path);

	const auto& char_details = safe_navigate(data, { "model", "CharDetails" });

	auto get_legacy_value = [&](const std::string& key) -> int {
		const auto& val = safe_navigate(char_details, { key, "@_value" });
		if (val.is_null())
			return 0; // JS: parseInt(undefined ?? '0') => 0
		auto parsed = safe_parse_int(val);
		return parsed.value_or(-1); // JS NaN sentinel for non-numeric strings
	};

	LegacyValues legacy_values;
	legacy_values.skin_color = get_legacy_value("skinColor");
	legacy_values.face_type = get_legacy_value("faceType");
	legacy_values.hair_color = get_legacy_value("hairColor");
	legacy_values.hair_style = get_legacy_value("hairStyle");
	legacy_values.facial_hair = get_legacy_value("facialHair");

	// parse equipment (v1 also has equipment node)
	auto equipment = parse_equipment(data);

	return {
		race,
		gender,
		legacy_values,
		std::move(equipment),
		std::move(model_path)
	};
}

} // anonymous namespace

ParseResult wmv_parse(std::string_view xml_str) {
	auto parsed = parse_xml(xml_str);

	if (!parsed.contains("SavedCharacter"))
		throw std::runtime_error("invalid .chr file: missing SavedCharacter root");

	const auto& saved_char = parsed["SavedCharacter"];
	std::string version = saved_char.value("@_version", "");

	if (version == "2.0")
		return wmv_parse_v2(saved_char);
	else if (version == "1.0")
		return wmv_parse_v1(saved_char);
	else
		throw std::runtime_error(std::format("unsupported .chr version: {}", version));
}

} // namespace wmv
