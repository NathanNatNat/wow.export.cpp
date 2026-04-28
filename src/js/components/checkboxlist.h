/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <vector>
#include <nlohmann/json.hpp>

/**
 * Scrollable checkbox list component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['items'], custom drag-scroll
 * thumb, and checkbox toggling.
 *
 * Items are nlohmann::json objects with "checked" (bool) and "label" (string) fields.
 */
namespace checkboxlist {

/**
 * Persistent state for a single CheckboxList widget instance.
 *
 * The JS component tracks `scroll`, `scrollRel`, `isScrolling`, and `slotCount`
 * to drive a hand-rolled drag scroller. The C++ port renders the list inside a
 * native ImGui child window with a built-in scrollbar, so ImGui owns the scroll
 * state internally. The struct is kept for API compatibility with callers.
 */
struct CheckboxListState {};

/**
 * Render a scrollable checkbox list using ImGui.
 *
 * @param id       Unique ImGui ID string for this widget instance.
 * @param items    Reference to the item array; each element must have "checked" (bool) and "label" (string).
 * @param state    Persistent state across frames (currently unused — kept for API stability).
 */
void render(const char* id, std::vector<nlohmann::json>& items, CheckboxListState& state);

} // namespace checkboxlist
