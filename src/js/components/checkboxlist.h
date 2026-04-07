/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <nlohmann/json.hpp>

/**
 * Scrollable checkbox list component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['items'], virtual scrolling,
 * custom scrollbar drag, and checkbox toggling.
 *
 * Items are nlohmann::json objects with "checked" (bool) and "label" (string) fields.
 */
namespace checkboxlist {

/**
 * Persistent state for a single CheckboxList widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct CheckboxListState {
	float scroll = 0.0f;
	float scrollRel = 0.0f;
	bool isScrolling = false;
	int slotCount = 1;

	// Mouse drag tracking (equivalent to JS instance vars set in startMouse).
	float scrollStartY = 0.0f;
	float scrollStart = 0.0f;
};

/**
 * Render a scrollable checkbox list using ImGui.
 *
 * @param id       Unique ImGui ID string for this widget instance.
 * @param items    Reference to the item array; each element must have "checked" (bool) and "label" (string).
 * @param state    Persistent state across frames.
 */
void render(const char* id, std::vector<nlohmann::json>& items, CheckboxListState& state);

} // namespace checkboxlist
