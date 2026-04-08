/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Settings screen module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, data(),
 * computed (is_edit_export_path_concerning, default_character_path,
 * cache_size_formatted, available_locale_keys, selected_locale_key),
 * methods (handle_cache_clear, handle_tact_key, go_home, handle_discard,
 * handle_apply, handle_reset), and mounted().
 *
 * Provides: Settings context menu option ("Manage Settings", gear.svg),
 * configuration editing with apply/discard/reset, export directory,
 * character save directory, all boolean/number/text settings,
 * locale selector, encryption key management, cache clearing.
 */
namespace screen_settings {

/**
 * Register the settings screen (context menu option).
 * JS equivalent: register() { this.registerContextMenuOption('Manage Settings', 'gear.svg') }
 */
void registerScreen();

/**
 * Initialize the settings screen (copy config to configEdit).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the settings screen widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Apply current settings and return home.
 * JS equivalent: methods.handle_apply()
 */
void handle_apply();

/**
 * Discard changes and return home.
 * JS equivalent: methods.handle_discard()
 */
void handle_discard();

/**
 * Reset settings to defaults.
 * JS equivalent: methods.handle_reset()
 */
void handle_reset();

} // namespace screen_settings
