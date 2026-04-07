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

/**
 * Combo box / autocomplete dropdown component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['value', 'source', 'placeholder', 'maxheight'],
 * emits: ['update:value']. Renders a text input with a filtered dropdown list.
 *
 * Source items are nlohmann::json objects with "value" and "label" (string) fields.
 */
namespace combobox {

/**
 * Persistent state for a single ComboBox widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct ComboBoxState {
	std::string currentText;
	bool isActive = false;

	// Change-detection for the 'value' prop watch.
	nlohmann::json prevValue;
};

/**
 * Render a combo box (text input with filtered dropdown) using ImGui.
 *
 * @param id          Unique ImGui ID string for this widget instance.
 * @param value       Current selected value (json object with "value"/"label", or null).
 * @param source      Array of available options (each with "value" and "label" strings).
 * @param placeholder Placeholder text shown when input is empty.
 * @param maxheight   Maximum number of dropdown items to display (0 = unlimited).
 * @param state       Persistent state across frames.
 * @param onChange    Callback invoked when selection changes; receives the selected option (or null).
 */
void render(const char* id, const nlohmann::json& value, const std::vector<nlohmann::json>& source,
            const char* placeholder, int maxheight, ComboBoxState& state,
            const std::function<void(const nlohmann::json&)>& onChange);

} // namespace combobox
