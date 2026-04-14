/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Changelog tab module — displays the application changelog.
 *
 * JS equivalent: Vue component that loads CHANGELOG.md and renders it
 * with MarkdownContent component. Registered as a context menu option
 * with icon 'list.svg' and label 'View Recent Changes'.
 */
namespace tab_changelog {

/**
 * Register the tab (context menu option).
 * JS equivalent: register() { this.registerContextMenuOption('View Recent Changes', 'list.svg') }
 */
void registerTab();

/**
 * Initialize / mounted callback — loads changelog text.
 * JS equivalent: async mounted()
 */
void mounted();

/**
 * Render the changelog tab widget using ImGui.
 */
void render();

} // namespace tab_changelog
