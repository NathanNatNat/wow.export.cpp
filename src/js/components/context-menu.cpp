/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "context-menu.h"

#include <imgui.h>
#include <string>

namespace context_menu {

// Keep a global track of the client mouse position.
// each frame, so we read it directly instead of tracking via a global listener.
// window.addEventListener('mousemove', event => { ... });

/**
 * node: Object which this context menu represents.
 */
// props: ['node']

// data: positionX, positionY, isLow, isLeft — stored in ContextMenuState

void reposition(ContextMenuState& state) {
	const ImGuiIO& io = ImGui::GetIO();
	state.positionX = io.MousePos.x;
	state.positionY = io.MousePos.y;
	state.isLow = state.positionY > io.DisplaySize.y / 2.0f;
	state.isLeft = state.positionX > io.DisplaySize.x / 2.0f;
}

// watch: node — change detection handled in render() via prevNodeActive.

// mounted: Initial position in case the menu renders immediately, but primary
// positioning occurs when `node` flips truthy (on open).

// template: converted to ImGui immediate-mode rendering below.
// <div class="context-menu" v-if="node !== null && node !== false"
//      :class="{ low: isLow, left: isLeft }"
//      :style="{ top: positionY + 'px', left: positionX + 'px' }"
//      @mouseleave="$emit('close')" @click="$emit('close')">
//     <div class="context-menu-zone"></div>
//     <slot v-bind:node="node"></slot>
// </div>

void render(const char* id, const nlohmann::json& node, ContextMenuState& state,
            const std::function<void()>& onClose,
            const std::function<void(const nlohmann::json&)>& contentCallback) {
	// Determine if node is "active" (not null and not false).
	const bool nodeActive = !node.is_null() && !(node.is_boolean() && !node.get<bool>());

	// Watch: when node transitions from inactive to active, reposition.
	if (nodeActive && !state.prevNodeActive) {
		reposition(state);
	}
	state.prevNodeActive = nodeActive;

	// v-if="node !== null && node !== false"
	if (!nodeActive)
		return;

	ImGui::PushID(id);

	// Position the popup window.
	// :class="{ low: isLow, left: isLeft }" — adjust anchor point.
	// Default: top-left corner at mouse position.
	// low: anchor from bottom instead of top.
	// left: anchor from right instead of left.
	ImVec2 windowPos(state.positionX, state.positionY);
	ImVec2 windowPivot(0.0f, 0.0f);

	if (state.isLeft)
		windowPivot.x = 1.0f;
	if (state.isLow)
		windowPivot.y = 1.0f;

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);
	ImGui::SetNextWindowBgAlpha(0.95f);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	                                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
	                                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
	                                ImGuiWindowFlags_NoMove;

	// Use unique window name per id to avoid collision between multiple instances.
	std::string windowName = std::string("##context_menu_") + id;
	if (ImGui::Begin(windowName.c_str(), nullptr, windowFlags)) {
		// <div class="context-menu-zone"></div>
		// The context-menu-zone in JS is an invisible div that extends the hover area between
		// the trigger and menu to prevent premature close. In ImGui, the window itself acts
		// as the hover region, and we handle close on mouse-leave below, making a separate
		// zone unnecessary.

		// Render menu content via the callback (equivalent of <slot v-bind:node="node">).
		if (contentCallback) {
			contentCallback(node);
		}

		// @click="$emit('close')" — close on any click within the menu.
		// This catches clicks on non-Selectable elements (text, separators, etc.)
		// which individual Selectable items would not handle.
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
		    ImGui::IsMouseClicked(0)) {
			if (onClose)
				onClose();
		}

		// @mouseleave="$emit('close')" — close when mouse leaves the window.
		// JS closes immediately on mouseleave. We replicate this by checking if the
		// mouse is outside the window bounds.
		if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RectOnly)) {
			if (onClose)
				onClose();
		}
	}
	ImGui::End();

	ImGui::PopID();
}

} // namespace context_menu