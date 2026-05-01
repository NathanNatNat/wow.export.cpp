/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <vector>
#include <nlohmann/json.hpp>

namespace checkboxlist {

struct CheckboxListState {};

void render(const char* id, std::vector<nlohmann::json>& items, CheckboxListState& state);

} // namespace checkboxlist
