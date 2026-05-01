/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>

namespace combobox {

struct ComboBoxState {
	std::string currentText;
	bool isActive = false;
	nlohmann::json prevValue;
	bool initialized = false;
	bool blurPending = false;
	double blurDeadline = 0.0;
};

void render(const char* id, const nlohmann::json& value, const std::vector<nlohmann::json>& source,
            const char* placeholder, int maxheight, ComboBoxState& state,
            const std::function<void(const nlohmann::json&)>& onChange);

} // namespace combobox
