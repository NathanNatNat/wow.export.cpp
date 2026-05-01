/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "context-menu.h"

#include <imgui.h>
#include <string>

#include "../../app.h"

namespace context_menu {

// Keep a global track of the client mouse position.

/**
 * node: Object which this context menu represents.
 */

void reposition(ContextMenuState& state) {
	const ImGuiIO& io = ImGui::GetIO();
	state.positionX = io.MousePos.x;
	state.positionY = io.MousePos.y;
	state.isLow = state.positionY > io.DisplaySize.y / 2.0f;
	state.isLeft = state.positionX > io.DisplaySize.x / 2.0f;
}

// Initial position in case the menu renders immediately, but primary
// positioning occurs when `node` flips truthy (on open).

void render(const char* id, const nlohmann::json& node, ContextMenuState& state,
            const std::function<void()>& onClose,
            const std::function<void(const nlohmann::json&)>& contentCallback) {
	const bool nodeActive = !node.is_null() && !(node.is_boolean() && !node.get<bool>());

	if (nodeActive && !state.prevNodeActive) {
		reposition(state);
	}
	state.prevNodeActive = nodeActive;

	if (!nodeActive)
		return;

	ImGui::PushID(id);

	ImVec2 windowPos(state.positionX, state.positionY);
	ImVec2 windowPivot(0.0f, 0.0f);

	if (state.isLeft)
		windowPivot.x = 1.0f;
	if (state.isLow)
		windowPivot.y = 1.0f;

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	                                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
	                                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
	                                ImGuiWindowFlags_NoMove;

	std::string windowName = std::string("##context_menu_") + id;
	if (ImGui::Begin(windowName.c_str(), nullptr, windowFlags)) {
		constexpr float contextMenuZonePaddingLeft = 20.0f;
		constexpr float contextMenuZonePaddingTop = 20.0f;
		constexpr float contextMenuZonePaddingRight = 20.0f;
		constexpr float contextMenuZonePaddingBottom = 20.0f;

		if (contentCallback) {
			contentCallback(node);
		}

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
		    ImGui::IsMouseClicked(0)) {
			if (onClose)
				onClose();
		}

		const ImVec2 mousePos = ImGui::GetIO().MousePos;
		ImVec2 zoneMin = ImGui::GetWindowPos();
		ImVec2 zoneMax = ImVec2(zoneMin.x + ImGui::GetWindowSize().x, zoneMin.y + ImGui::GetWindowSize().y);
		zoneMin.x -= contextMenuZonePaddingLeft;
		zoneMin.y -= contextMenuZonePaddingTop;
		zoneMax.x += contextMenuZonePaddingRight;
		zoneMax.y += contextMenuZonePaddingBottom;
		const bool isInHoverZone = mousePos.x >= zoneMin.x && mousePos.x <= zoneMax.x &&
		                           mousePos.y >= zoneMin.y && mousePos.y <= zoneMax.y;
		if (!isInHoverZone) {
			if (onClose)
				onClose();
		}
	}
	ImGui::End();

	ImGui::PopStyleVar(2);

	ImGui::PopID();
}

} // namespace context_menu
