/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "wmv.h"
#include "xml.h"
#include "wow/EquipmentSlots.h"

#include <algorithm>
#include <stdexcept>
#include <format>
#include <string>
#include <sstream>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace wmv {

namespace {

struct RaceGender {
	int race;
	int gender;
};

/**
 * Get a nested JSON string value using optional-chaining-like behavior.
 * Returns std::nullopt if any key in the chain is missing.
 */
std::optional<std::string> get_nested_string(const nlohmann::json& j,
                                              std::initializer_list<std::string> keys) {
	const nlohmann::json* current = &j;
	for (const auto& key : keys) {
		if (!current->is_object() || !current->contains(key))
			return std::nullopt;
		current = &(*current)[key];
	}
	if (current->is_string())
		return current->get<std::string>();
	return std::nullopt;
}

/**
 * Parse an integer from a JSON element's @_value attribute.
 * Returns std::nullopt if the element or attribute is missing, or parsing fails.
 * JS equivalent: parseInt(item.slot?.['@_value'])
 */
std::optional<int> get_element_int(const nlohmann::json& parent, const std::string& key) {
	if (!parent.contains(key))
		return std::nullopt;

	const auto& elem = parent[key];
	if (!elem.is_object() || !elem.contains("@_value"))
		return std::nullopt;

	const auto& val = elem["@_value"];
	if (val.is_number_integer())
		return val.get<int>();
	if (val.is_string()) {
		try { return std::stoi(val.get<std::string>()); }
		catch (...) { return std::nullopt; }
	}
	return std::nullopt;
}

/**
 * Parse an integer from a JSON element's @_value attribute with a default.
 * JS equivalent: parseInt(char_details?.skinColor?.['@_value'] ?? '0')
 */
int get_attr_int(const nlohmann::json& parent, const std::string& key, int default_val = 0) {
	auto result = get_element_int(parent, key);
	return result.value_or(default_val);
}

/**
 * Parse an integer from a JSON attribute (e.g. @_id, @_value).
 * JS equivalent: parseInt(cust['@_id'])
 */
std::optional<int> get_attr_value(const nlohmann::json& obj, const std::string& attr_name) {
	if (!obj.contains(attr_name))
		return std::nullopt;

	const auto& val = obj[attr_name];
	if (val.is_number_integer())
		return val.get<int>();
	if (val.is_string()) {
		try { return std::stoi(val.get<std::string>()); }
		catch (...) { return std::nullopt; }
	}
	return std::nullopt;
}

RaceGender extract_race_gender_from_path(const std::string& model_path) {
	static const std::unordered_map<std::string, int> race_map = {
		{"human", 1},
		{"orc", 2},
		{"dwarf", 3},
		{"nightelf", 4},
		{"scourge", 5},
		{"tauren", 6},
		{"gnome", 7},
		{"troll", 8},
		{"goblin", 9},
		{"bloodelf", 10},
		{"draenei", 11},
		{"worgen", 22},
		{"pandaren", 24},
		{"nightborne", 27},
		{"highmountaintauren", 28},
		{"voidelf", 29},
		{"lightforgeddraenei", 30},
		{"zandalaritroll", 31},
		{"kultiran", 32},
		{"darkirondwarf", 34},
		{"vulpera", 35},
		{"mechagnome", 37},
		{"dracthyr", 52},
		{"earthen", 84},

		// todo: 36, MagharOrc
	};

	// JS: const path_lower = model_path.toLowerCase().replace(/\\/g, '/');
	std::string path_lower = model_path;
	std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	std::replace(path_lower.begin(), path_lower.end(), '\\', '/');

	// JS: const parts = path_lower.split('/');
	std::vector<std::string> parts;
	std::istringstream iss(path_lower);
	std::string part;
	while (std::getline(iss, part, '/'))
		parts.push_back(part);

	std::optional<int> race;
	std::optional<int> gender;

	for (const auto& p : parts) {
		auto it = race_map.find(p);
		if (it != race_map.end())
			race = it->second;

		// JS: if (part === 'male' || part.includes('male') && !part.includes('female'))
		// JS operator precedence: || lower than &&, so: part === 'male' || (includes('male') && !includes('female'))
		if (p == "male" || (p.find("male") != std::string::npos && p.find("female") == std::string::npos))
			gender = 0;
		else if (p == "female" || p.find("female") != std::string::npos)
			gender = 1;
	}

	if (!race.has_value() || !gender.has_value())
		throw std::runtime_error(std::format("unable to determine race/gender from model path: {}", model_path));

	return { race.value(), gender.value() };
}

/**
 * Parse equipment from a .chr file's equipment node.
 * Common to both v1 and v2 formats.
 */
std::unordered_map<int, int> parse_equipment(const nlohmann::json& data) {
	std::unordered_map<int, int> equipment;

	if (!data.contains("equipment") || !data["equipment"].is_object())
		return equipment;

	const auto& equip_node = data["equipment"];
	if (!equip_node.contains("item"))
		return equipment;

	// JS: const items = Array.isArray(data.equipment.item) ? ... : [data.equipment.item];
	nlohmann::json items;
	if (equip_node["item"].is_array())
		items = equip_node["item"];
	else
		items = nlohmann::json::array({equip_node["item"]});

	for (const auto& item : items) {
		auto wmv_slot = get_element_int(item, "slot");
		auto item_id = get_element_int(item, "id");

		// JS: if (isNaN(wmv_slot) || isNaN(item_id) || item_id === 0) continue;
		if (!wmv_slot || !item_id || item_id.value() == 0)
			continue;

		auto slot_id = get_slot_id_for_wmv_slot(wmv_slot.value());
		if (slot_id.has_value())
			equipment[slot_id.value()] = item_id.value();
	}

	return equipment;
}

WmvParseResult wmv_parse_v2(const nlohmann::json& data) {
	// JS: const model_path = data.model?.file?.['@_name'];
	auto model_path_opt = get_nested_string(data, {"model", "file", "@_name"});
	if (!model_path_opt.has_value())
		throw std::runtime_error("invalid .chr file: missing model path");

	const std::string& model_path = model_path_opt.value();
	auto [race, gender] = extract_race_gender_from_path(model_path);

	// JS: const customizations = [];
	std::vector<WmvCustomization> customizations;

	// JS: const char_details = data.model?.CharDetails;
	if (data.contains("model") && data["model"].is_object() &&
	    data["model"].contains("CharDetails")) {
		const auto& char_details = data["model"]["CharDetails"];

		// JS: if (char_details?.customization) { ... }
		if (char_details.contains("customization")) {
			// JS: const cust_array = Array.isArray(...) ? ... : [...]
			nlohmann::json cust_array;
			if (char_details["customization"].is_array())
				cust_array = char_details["customization"];
			else
				cust_array = nlohmann::json::array({char_details["customization"]});

			for (const auto& cust : cust_array) {
				auto option_id = get_attr_value(cust, "@_id");
				auto choice_id = get_attr_value(cust, "@_value");

				// JS: if (!isNaN(option_id) && !isNaN(choice_id))
				if (option_id.has_value() && choice_id.has_value())
					customizations.push_back({option_id.value(), choice_id.value()});
			}
		}
	}

	// parse equipment
	auto equipment = parse_equipment(data);

	WmvParseResult result;
	result.race = race;
	result.gender = gender;
	result.customizations = std::move(customizations);
	result.equipment = std::move(equipment);
	result.model_path = model_path;
	return result;
}

WmvParseResult wmv_parse_v1(const nlohmann::json& data) {
	// JS: const model_path = data.model?.file?.['@_name'];
	auto model_path_opt = get_nested_string(data, {"model", "file", "@_name"});
	if (!model_path_opt.has_value())
		throw std::runtime_error("invalid .chr file: missing model path");

	const std::string& model_path = model_path_opt.value();
	auto [race, gender] = extract_race_gender_from_path(model_path);

	// JS: const char_details = data.model?.CharDetails;
	WmvLegacyValues legacy_values;
	if (data.contains("model") && data["model"].is_object() &&
	    data["model"].contains("CharDetails")) {
		const auto& char_details = data["model"]["CharDetails"];
		legacy_values.skin_color = get_attr_int(char_details, "skinColor");
		legacy_values.face_type = get_attr_int(char_details, "faceType");
		legacy_values.hair_color = get_attr_int(char_details, "hairColor");
		legacy_values.hair_style = get_attr_int(char_details, "hairStyle");
		legacy_values.facial_hair = get_attr_int(char_details, "facialHair");
	}

	// parse equipment (v1 also has equipment node)
	auto equipment = parse_equipment(data);

	WmvParseResult result;
	result.race = race;
	result.gender = gender;
	result.legacy_values = legacy_values;
	result.equipment = std::move(equipment);
	result.model_path = model_path;
	return result;
}

} // anonymous namespace

WmvParseResult wmv_parse(std::string_view xml_str) {
	auto parsed = parse_xml(xml_str);

	if (!parsed.contains("SavedCharacter"))
		throw std::runtime_error("invalid .chr file: missing SavedCharacter root");

	const auto& saved_char = parsed["SavedCharacter"];

	// JS: const version = parsed.SavedCharacter['@_version'];
	std::string version;
	if (saved_char.contains("@_version"))
		version = saved_char["@_version"].get<std::string>();

	if (version == "2.0")
		return wmv_parse_v2(saved_char);
	else if (version == "1.0")
		return wmv_parse_v1(saved_char);
	else
		throw std::runtime_error(std::format("unsupported .chr version: {}", version));
}

} // namespace wmv
