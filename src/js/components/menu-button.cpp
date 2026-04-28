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
	if (optionIndex >= 0 && optionIndex < static_cast<int>(options.size())) {
		state.selectedValue = options[static_cast<size_t>(optionIndex)].value;
		if (onChange)
			onChange(options[static_cast<size_t>(optionIndex)].value);
	}
}

static void drawArrowButton(float width, bool hoveredOrOpen, bool disabled) {
	if (hoveredOrOpen)
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
	// Use a "v" glyph as the button label so the down-arrow is rendered by
	// the native ImGui text path rather than a raw ImDrawList::AddTriangleFilled
	// call (CLAUDE.md: prefer native widgets over raw ImDrawList).
	ImGui::Button("v##arrow", ImVec2(width, 0.0f));
	if (hoveredOrOpen)
		ImGui::PopStyleColor();

	const ImVec2 arrowMin = ImGui::GetItemRectMin();
	const ImVec2 arrowMax = ImGui::GetItemRectMax();

	// 1px vertical divider between the main button and the arrow button.
	// This is a custom decorative effect with no native equivalent, so it
	// remains a direct ImDrawList::AddLine call (allowed per CLAUDE.md).
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddLine(
		ImVec2(arrowMin.x, arrowMin.y),
		ImVec2(arrowMin.x, arrowMax.y),
		IM_COL32(255, 255, 255, 82),
		1.0f
	);

	(void)disabled; // disabled tinting is handled by ImGui::BeginDisabled in the caller.
}

/**
 * HTML mark-up to render for this component.
 */
// template: converted to ImGui immediate-mode rendering below.

void render(const char* id, const std::vector<MenuOption>& options,
            const std::string& defaultVal, bool disabled, bool dropdown,
            bool upward,
            MenuButtonState& state,
            const std::function<void(const std::string&)>& onChange,
            const std::function<void()>& onClick) {
	if (options.empty())
		return;

	ImGui::PushID(id);

	const std::string popupId = std::string("##mbp_") + id;
	const bool popupOpen = ImGui::IsPopupOpen(popupId.c_str());

	// <div class="ui-menu-button" :class="{ disabled, dropdown, open }">
	const int selectedIdx = selected(state, options, defaultVal);
	const MenuOption& selectedOpt = options[static_cast<size_t>(selectedIdx)];

	// Determine the display label: this.selected.label ?? this.selected.value
	const std::string& displayLabel = selectedOpt.label.empty() ? selectedOpt.value : selectedOpt.label;

	if (disabled)
		ImGui::BeginDisabled(true);

	// <input type="button" :value="this.selected.label ?? this.selected.value" @click="handleClick"/>
	const float arrowWidth = 29.0f;
	const float totalWidth = ImGui::GetContentRegionAvail().x;
	const float buttonWidth = totalWidth - arrowWidth;
	const ImVec2 buttonGroupMin = ImGui::GetCursorScreenPos();
	const ImVec2 buttonGroupMax(buttonGroupMin.x + totalWidth, buttonGroupMin.y + ImGui::GetFrameHeight());
	const bool hoveredButtonGroup = !disabled && (popupOpen || (dropdown && ImGui::IsMouseHoveringRect(buttonGroupMin, buttonGroupMax)));

	if (hoveredButtonGroup)
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));

	if (ImGui::Button(displayLabel.c_str(), ImVec2(buttonWidth, 0.0f))) {
		if (dropdown) {
			// JS openMenu (lines 37-40): this.open = !this.open && !this.disabled
			// — toggles the menu closed if already open. ImGui's click-outside
			// handling auto-closes the popup before this click is delivered, so
			// the toggle behavior collapses to: only re-open if it wasn't open.
			if (!disabled && !popupOpen)
				ImGui::OpenPopup(popupId.c_str());
		} else if (onClick) {
			onClick();
		}
	}

	ImGui::SameLine(0.0f, 0.0f);
	if (hoveredButtonGroup)
		ImGui::PopStyleColor();

	// <div class="arrow" @click.stop="openMenu"></div>
	drawArrowButton(arrowWidth, hoveredButtonGroup, disabled);
	if (!disabled && ImGui::IsItemHovered())
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	// JS openMenu toggle semantics — only open if not already open. The popup
	// is auto-closed by ImGui when the user clicks outside it (i.e., on this
	// arrow), so omitting OpenPopup when popupOpen is true preserves the JS
	// `this.open = !this.open` toggle-close behavior.
	if (!disabled && !popupOpen && ImGui::IsItemClicked())
		ImGui::OpenPopup(popupId.c_str());

	if (disabled)
		ImGui::EndDisabled();

	// <context-menu :node="open" @close="open = false">
	//     <span v-for="option in options" @click="select(option)">{{ option.label ?? option.value }}</span>
	// </context-menu>
	//
	// Use ImGui's proper popup API so input is always routed to the popup window.
	const float popupY = upward ? buttonGroupMin.y : buttonGroupMax.y;
	const ImVec2 pivot = upward ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f);
	ImGui::SetNextWindowPos(ImVec2(buttonGroupMin.x, popupY), ImGuiCond_Always, pivot);

	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.208f, 0.208f, 0.208f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.208f, 0.208f, 0.208f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	if (ImGui::BeginPopup(popupId.c_str(), ImGuiWindowFlags_NoNav)) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));

		for (size_t i = 0; i < options.size(); ++i) {
			const std::string& optLabel = options[i].label.empty() ? options[i].value : options[i].label;
			if (ImGui::Selectable(optLabel.c_str(), false, ImGuiSelectableFlags_None, ImVec2(0.0f, 0.0f))) {
				select(static_cast<int>(i), state, options, onChange);
				ImGui::CloseCurrentPopup();
			}

			// Use the native ImGui separator widget between options
			// (CLAUDE.md: prefer native widgets over raw ImDrawList).
			if (i + 1 < options.size())
				ImGui::Separator();
		}

		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(3);

	state.open = popupOpen;

	ImGui::PopID();
}

} // namespace menu_button
