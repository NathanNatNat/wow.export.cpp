/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <functional>

/**
 * Menu button component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['options', 'default', 'disabled', 'dropdown'],
 * emits: ['change', 'click']. Renders a button with a dropdown arrow that opens
 * a context menu of selectable options.
 *
 * Options are objects with label and value fields.
 */
namespace menu_button {

/**
 * A single option in the menu button dropdown.
 */
struct MenuOption {
	std::string label;
	std::string value;
};

/**
 * Persistent state for a single MenuButton widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct MenuButtonState {
	std::string selectedValue;  // Stores the selected option value (empty = use default).
	                            // JS stores the option object reference (this.selectedObj),
	                            // making it immune to array reordering. Using the value string
	                            // achieves the same effect.
	bool open = false;          // If the menu is open or not.
};

/**
 * Render a menu button using ImGui.
 *
 * @param id          Unique ImGui ID string for this widget instance.
 * @param options     Array of available options (each with label and value).
 * @param defaultVal  The default value from the options array.
 * @param disabled    Controls disabled state of the component.
 * @param dropdown    If true, the full button prompts the context menu, not just the arrow.
 * @param state       Persistent state across frames.
 * @param onChange    Callback invoked when selection changes; receives the selected option value.
 * @param onClick     Callback invoked when button is clicked (only when dropdown is false).
 */
void render(const char* id, const std::vector<MenuOption>& options,
            const std::string& defaultVal, bool disabled, bool dropdown,
            MenuButtonState& state,
            const std::function<void(const std::string&)>& onChange,
            const std::function<void()>& onClick);

} // namespace menu_button
