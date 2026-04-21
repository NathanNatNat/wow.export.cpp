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

static void drawArrowButton(float width, bool hoveredOrOpen, bool disabled) {
	ImGui::PushStyleColor(ImGuiCol_Button,
		hoveredOrOpen ? app::theme::BUTTON_HOVER : ImGui::GetStyleColorVec4(ImGuiCol_Button));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, app::theme::BUTTON_HOVER);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, app::theme::BUTTON_HOVER);
	ImGui::Button("##arrow", ImVec2(width, 0.0f));
	ImGui::PopStyleColor(3);

	const ImVec2 arrowMin = ImGui::GetItemRectMin();
	const ImVec2 arrowMax = ImGui::GetItemRectMax();
	const ImVec2 arrowCenter((arrowMin.x + arrowMax.x) * 0.5f, (arrowMin.y + arrowMax.y) * 0.5f);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddLine(
		ImVec2(arrowMin.x, arrowMin.y),
		ImVec2(arrowMin.x, arrowMax.y),
		IM_COL32(255, 255, 255, 82),
		1.0f
	);

	const float halfWidth = 5.0f;
	const float halfHeight = 2.5f;
	const ImU32 arrowColor = disabled ? app::theme::FONT_DISABLED_U32 : app::theme::FONT_HIGHLIGHT_U32;
	drawList->AddTriangleFilled(
		ImVec2(arrowCenter.x - halfWidth, arrowCenter.y - halfHeight),
		ImVec2(arrowCenter.x + halfWidth, arrowCenter.y - halfHeight),
		ImVec2(arrowCenter.x, arrowCenter.y + halfHeight),
		arrowColor
	);
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

	// <div class="ui-menu-button" :class="{ disabled, dropdown, open }">
	const int selectedIdx = selected(state, options, defaultVal);
	const MenuOption& selectedOpt = options[static_cast<size_t>(selectedIdx)];

	// Determine the display label: this.selected.label ?? this.selected.value
	const std::string& displayLabel = selectedOpt.label.empty() ? selectedOpt.value : selectedOpt.label;

	// Disable visual style if disabled.
	if (disabled) {
		ImGui::BeginDisabled(true);
	}

	// <input type="button" :value="this.selected.label ?? this.selected.value" @click="handleClick"/>
	const float arrowWidth = 29.0f;
	const float totalWidth = ImGui::GetContentRegionAvail().x;
	const float buttonWidth = totalWidth - arrowWidth;
	const ImVec2 buttonGroupMin = ImGui::GetCursorScreenPos();
	const ImVec2 buttonGroupMax(buttonGroupMin.x + totalWidth, buttonGroupMin.y + ImGui::GetFrameHeight());
	const bool hoveredButtonGroup = !disabled && (state.open || (dropdown && ImGui::IsMouseHoveringRect(buttonGroupMin, buttonGroupMax)));

	ImGui::PushStyleColor(ImGuiCol_Button,
		hoveredButtonGroup ? app::theme::BUTTON_HOVER : ImGui::GetStyleColorVec4(ImGuiCol_Button));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, app::theme::BUTTON_HOVER);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, app::theme::BUTTON_HOVER);

	if (ImGui::Button(displayLabel.c_str(), ImVec2(buttonWidth, 0.0f))) {
		handleClick(dropdown, disabled, state, onClick);
	}

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::PopStyleColor(3);
	drawArrowButton(arrowWidth, hoveredButtonGroup, disabled);
	if (!disabled && ImGui::IsItemHovered())
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	if (ImGui::IsItemClicked()) {
		openMenu(disabled, state);
	}

	if (disabled) {
		ImGui::EndDisabled();
	}

	// <context-menu :node="open" @close="open = false">
	//     <span v-for="option in options" @click="select(option)">{{ option.label ?? option.value }}</span>
	// </context-menu>
	if (state.open) {
		const ImVec2 buttonMin = buttonGroupMin;
		const ImVec2 buttonMax = ImVec2(buttonGroupMin.x + totalWidth, buttonGroupMin.y + ImGui::GetFrameHeight());
		const float popupY = upward ? buttonMin.y : buttonMax.y;

		ImGui::SetNextWindowPos(ImVec2(buttonMin.x, popupY), ImGuiCond_Always, upward ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.137f, 0.137f, 0.137f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, app::theme::BORDER);
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.208f, 0.208f, 0.208f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.208f, 0.208f, 0.208f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		                               ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
		                               ImGuiWindowFlags_NoMove;

		if (ImGui::Begin((std::string("##menu_button_popup_") + id).c_str(), nullptr, windowFlags)) {
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			for (size_t i = 0; i < options.size(); ++i) {
				const std::string& optLabel = options[i].label.empty() ? options[i].value : options[i].label;
				if (ImGui::Selectable(optLabel.c_str(), false, ImGuiSelectableFlags_None,
				                      ImVec2(ImGui::CalcTextSize(optLabel.c_str()).x + 16.0f, 0.0f))) {
					select(static_cast<int>(i), state, options, onChange);
				}

				if (i + 1 < options.size()) {
					const ImVec2 separatorPos = ImGui::GetCursorScreenPos();
					drawList->AddLine(
						separatorPos,
						ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, separatorPos.y),
						app::theme::BORDER_U32
					);
				}
			}

			constexpr float hoverZonePadding = 20.0f;
			const ImVec2 mousePos = ImGui::GetIO().MousePos;
			ImVec2 zoneMin = ImGui::GetWindowPos();
			ImVec2 zoneMax(zoneMin.x + ImGui::GetWindowSize().x, zoneMin.y + ImGui::GetWindowSize().y);
			zoneMin.x -= hoverZonePadding;
			zoneMin.y -= hoverZonePadding;
			zoneMax.x += hoverZonePadding;
			zoneMax.y += hoverZonePadding;
			const bool insideHoverZone = mousePos.x >= zoneMin.x && mousePos.x <= zoneMax.x &&
			                             mousePos.y >= zoneMin.y && mousePos.y <= zoneMax.y;

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
			    ImGui::IsMouseClicked(0)) {
				state.open = false;
			} else if (!insideHoverZone) {
				state.open = false;
			}

			ImGui::PopStyleVar();
		}
		ImGui::End();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(5);
	}

	ImGui::PopID();
}

} // namespace menu_button
