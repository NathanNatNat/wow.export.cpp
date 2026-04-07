/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Audio tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_listbox_context, copy_file_paths, copy_listfile_format,
 * copy_file_data_ids, copy_export_paths, open_export_directory,
 * toggle_playback, handle_seek, initialize, export_selected),
 * and mounted().
 *
 * Provides: Audio nav button (CASC only), sound file listbox with
 * context menu, audio player with seek/volume/loop, export.
 */
namespace tab_audio {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Audio', 'music.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the audio tab (player init, unknown sound files, selection watch).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the audio tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Toggle audio playback (play/pause).
 * JS equivalent: methods.toggle_playback()
 */
void toggle_playback();

/**
 * Export selected sound files.
 * JS equivalent: methods.export_selected() → export_sounds(core)
 */
void export_selected();

} // namespace tab_audio
