/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string_view>
#include <nlohmann/json.hpp>

/**
 * Parse an XML string into a JSON-like object hierarchy.
 *
 * Tags become object keys. Attributes are stored with an "@_" prefix.
 * Multiple same-named children are grouped into arrays; single children
 * are stored directly. Self-closing tags and processing instructions
 * (<?...?>) are handled.
 *
 * @param xml The XML string to parse.
 * @return A nlohmann::json object representing the parsed XML.
 */
nlohmann::json parse_xml(std::string_view xml);
