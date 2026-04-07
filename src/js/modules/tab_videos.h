/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Videos tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (handle_listbox_context, copy_file_paths, copy_listfile_format,
 * copy_file_data_ids, copy_export_paths, open_export_directory,
 * preview_video, export_selected, export_mp4, export_avi,
 * export_mp3, export_subtitles, initialize),
 * and mounted().
 *
 * Also exposes file-local functions: stop_video, build_payload,
 * stream_video, play_streaming_video, load_video_listfile,
 * get_movie_data, get_mp4_url, trigger_kino_processing.
 *
 * Provides: Videos nav button (CASC only), video file listbox with
 * context menu, video streaming via Kino server, subtitle support,
 * export in MP4/AVI/MP3/SUBTITLES format.
 */
namespace tab_videos {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Videos', 'film.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the videos tab and set up watches.
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the videos tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Preview/stream the selected video.
 * JS equivalent: methods.preview_video()
 */
void preview_video();

/**
 * Export selected video files (dispatches based on exportVideoFormat).
 * JS equivalent: methods.export_selected()
 */
void export_selected();

} // namespace tab_videos
