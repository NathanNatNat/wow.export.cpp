/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Blender add-on installation tab.
 *
 * JS equivalent: Vue component that detects Blender installations, allows
 * automatic or manual installation of the wow.export Blender add-on.
 * Registered as a context menu option with icon 'blender.png'.
 */
namespace tab_blender {

/**
 * Register the tab (context menu option).
 * JS equivalent: register() { this.registerContextMenuOption('Install Blender Add-on', '../images/blender.png') }
 */
void registerTab();

/**
 * Render the Blender tab widget using ImGui.
 */
void render();

/**
 * Check local Blender add-on version and prompt user to update if newer version available.
 * JS equivalent: checkLocalVersion() in tab_blender.js lines 127-170.
 */
void checkLocalVersion();

} // namespace tab_blender
