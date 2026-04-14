/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "menu-button.h"

#include <imgui.h>
#include "../../app.h"

namespace menu_button {

/**
 * options: An array of objects with label/value properties.
 * default: The default value from the options array.
 * disabled: Controls disabled state of the component.
 * dropdown: If true, the full button prompts the context menu, not just the arrow.
 */
// props: ['options', 'default', 'disabled', 'dropdown']
// emits: ['change', 'click']

// data: selectedObj, open — stored in MenuButtonState

/**
 * Returns the option with the same value as the provided default or
 * falls back to returning the first available option.
 * @returns {object}
 */
static int defaultObj(const std::vector<MenuOption>& options, const std::string& defaultVal) {
	for (size_t i = 0; i < options.size(); ++i) {
		if (options[i].value == defaultVal)
			return static_cast<int>(i);
	}
	return options.empty() ? -1 : 0;
}

/**
 * Returns the currently selected option or falls back to the default.
 * @returns {object}
 */
static int selected(const MenuButtonState& state, const std::vector<MenuOption>& options, const std::string& defaultVal) {
	// JS: this.selectedObj ?? this.defaultObj — stores object reference.
	// C++: we store the selected value string and find the matching option.
	if (!state.selectedValue.empty()) {
		for (size_t i = 0; i < options.size(); ++i) {
			if (options[i].value == state.selectedValue)
				return static_cast<int>(i);
		}
	}
	return defaultObj(options, defaultVal);
}

/**
 * Set the selected option for this menu button.
 * @param {object} option
 */
static void select(int optionIndex, MenuButtonState& state,
                    const std::vector<MenuOption>& options,
                    const std::function<void(const std::string&)>& onChange) {
	state.open = false;
	if (optionIndex >= 0 && optionIndex < static_cast<int>(options.size())) {
		state.selectedValue = options[static_cast<size_t>(optionIndex)].value;
		if (onChange)
			onChange(options[static_cast<size_t>(optionIndex)].value);
	}
}

/**
 * Attempt to open the menu.
 * Respects component disabled state.
 */
static void openMenu(bool disabled, MenuButtonState& state) {
	state.open = !state.open && !disabled;
}

/**
 * Handle clicks onto the button node.
 */
static void handleClick(bool dropdownMode, bool disabled, MenuButtonState& state,
                         const std::function<void()>& onClick) {
	if (dropdownMode)
		openMenu(disabled, state);
	else if (onClick)
		onClick();
}

/**
 * HTML mark-up to render for this component.
 */
// template: converted to ImGui immediate-mode rendering below.

void render(const char* id, const std::vector<MenuOption>& options,
            const std::string& defaultVal, bool disabled, bool dropdown,
            MenuButtonState& state,
            const std::function<void(const std::string&)>& onChange,
            const std::function<void()>& onClick) {
	if (options.empty())
		return;

	ImGui::PushID(id);

	// <div class="ui-menu-button" :class="{ disabled, dropdown, open }">
	const int selectedIdx = selected(state, options, defaultVal);
	const MenuOption& selectedOpt = options[static_cast<size_t>(selectedIdx)];

	// Determine the display label: this.selected.label ?? this.selected.value
	const std::string& displayLabel = selectedOpt.label.empty() ? selectedOpt.value : selectedOpt.label;

	// Disable visual style if disabled.
	if (disabled) {
		ImGui::BeginDisabled(true);
	}

	// CSS class states: dropdown mode and open state both apply hover background
	// to the entire button+arrow area. In ImGui, we push a highlight button color
	// when the menu is open or when hovered in dropdown mode.
	bool pushed_style = false;
	if (!disabled && state.open) {
		// .open state: both button and arrow get hover background
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
		pushed_style = true;
	}

	// <input type="button" :value="this.selected.label ?? this.selected.value" @click="handleClick"/>
	const float arrowWidth = 20.0f;
	const float totalWidth = ImGui::GetContentRegionAvail().x;
	const float buttonWidth = totalWidth - arrowWidth;

	if (ImGui::Button(displayLabel.c_str(), ImVec2(buttonWidth, 0.0f))) {
		handleClick(dropdown, disabled, state, onClick);
	}

	// <div class="arrow" @click.stop="openMenu"></div>
	// CSS uses caret-down.svg icon centered in a 29px-wide area.
	ImGui::SameLine(0.0f, 0.0f);
	if (ImGui::Button(ICON_FA_CARET_DOWN, ImVec2(arrowWidth, 0.0f))) {
		openMenu(disabled, state);
	}

	if (pushed_style)
		ImGui::PopStyleColor();

	if (disabled) {
		ImGui::EndDisabled();
	}

	// <context-menu :node="open" @close="open = false">
	//     <span v-for="option in options" @click="select(option)">{{ option.label ?? option.value }}</span>
	// </context-menu>
	// JS uses the context-menu child component which positions based on mouse cursor
	// and closes on mouse-leave. In ImGui, we use a positioned window below the button
	// with click-outside-to-close behavior, which provides equivalent UX.
	if (state.open) {
		ImGui::SetNextWindowBgAlpha(0.95f);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		                                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		                                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
		                                ImGuiWindowFlags_NoMove;

		// Position the popup below the button.
		const ImVec2 buttonMin = ImGui::GetItemRectMin();
		const ImVec2 buttonMax = ImGui::GetItemRectMax();
		ImGui::SetNextWindowPos(ImVec2(buttonMin.x, buttonMax.y), ImGuiCond_Always);

		if (ImGui::Begin((std::string("##menu_button_popup_") + id).c_str(), nullptr, windowFlags)) {
			for (size_t i = 0; i < options.size(); ++i) {
				const std::string& optLabel = options[i].label.empty() ? options[i].value : options[i].label;
				if (ImGui::Selectable(optLabel.c_str())) {
					select(static_cast<int>(i), state, options, onChange);
				}
			}

			// Close when clicking outside the menu.
			if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RectOnly) &&
			    ImGui::IsMouseClicked(0)) {
				state.open = false;
			}
		}
		ImGui::End();
	}

	ImGui::PopID();
}

} // namespace menu_button