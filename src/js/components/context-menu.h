/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <functional>
#include <nlohmann/json.hpp>

/**
 * Context menu component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['node'], emits: ['close'].
 * Displays a positioned popup near the mouse cursor that auto-positions
 * based on screen quadrant (low/left offsets).
 *
 * The node prop controls visibility: when non-null/non-false the menu is shown.
 */
namespace context_menu {

/**
 * Persistent state for a single ContextMenu widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct ContextMenuState {
	float positionX = 0.0f;
	float positionY = 0.0f;
	bool isLow = false;
	bool isLeft = false;

	// Change-detection for the 'node' prop watch.
	bool prevNodeActive = false;
};

/**
 * Reposition the context menu to the current mouse cursor location.
 * Adjusts isLow/isLeft flags based on screen quadrant.
 * Equivalent to the JS reposition() method.
 *
 * @param state  Persistent state to update with new position.
 */
void reposition(ContextMenuState& state);

/**
 * Render a context menu popup using ImGui.
 *
 * The contentCallback should emit the menu items (e.g. ImGui::Selectable calls).
 * The node parameter controls visibility: menu is shown when node is not null and not false.
 *
 * @param id              Unique ImGui ID string for this widget instance.
 * @param node            The node data associated with this context menu (null = hidden).
 * @param state           Persistent state across frames.
 * @param onClose         Callback invoked when the menu should close (mouse leave or item click).
 * @param contentCallback Callback to render the menu contents; receives the node.
 */
void render(const char* id, const nlohmann::json& node, ContextMenuState& state,
            const std::function<void()>& onClose,
            const std::function<void(const nlohmann::json&)>& contentCallback);

} // namespace context_menu
