/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Help tab module — displays searchable help articles from markdown files.
 *
 * JS equivalent: Vue component that loads .md files from help_docs directory,
 * provides search/filtering, and renders articles with MarkdownContent component.
 * Registered as a context menu option with icon 'help.svg'.
 */
namespace tab_help {

/**
 * Register the tab (context menu option).
 * JS equivalent: register() { this.registerContextMenuOption('Help', 'help.svg') }
 */
void registerTab();

/**
 * Initialize / mounted callback — loads help documents.
 * JS equivalent: async mounted()
 */
void mounted();

/**
 * Render the help tab widget using ImGui.
 */
void render();

} // namespace tab_help
