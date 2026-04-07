/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_videos.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../constants.h"
#include "../subtitles.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
#include "../casc/db2.h"
#include "../ui/listbox-context.h"
#include "../install-type.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <fstream>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <httplib.h>

namespace tab_videos {

// --- File-local state ---

// JS: let movie_variation_map = null;
static std::optional<std::unordered_map<uint32_t, uint32_t>> movie_variation_map;

// JS: let video_file_data_ids = null;
static std::optional<std::vector<uint32_t>> video_file_data_ids;

// JS: let selected_file = null;
static std::string selected_file;

// JS: let current_video_element = null;
// TODO(conversion): Video playback uses Kino server streaming; no local video element in C++.
// The video element state is tracked via videoPlayerState in AppState.

// JS: let current_subtitle_track = null;
// TODO(conversion): Subtitle track is handled via Kino server and subtitles module; no DOM track in C++.
static bool has_subtitle_track = false;

// JS: let current_subtitle_blob_url = null;
// TODO(conversion): Blob URLs are a browser concept; subtitle data managed directly in C++.
static std::string current_subtitle_vtt;

// JS: let is_streaming = false;
static bool is_streaming = false;

// JS: let poll_timer = null;
// TODO(conversion): Poll timer replaced with poll state flags; polling handled in main loop or async thread.
static bool poll_active = false;

// JS: let poll_cancelled = false;
static bool poll_cancelled = false;

// JS: let kino_processing_cancelled = false;
static bool kino_processing_cancelled = false;

// Change-detection for selection and config watches.
static std::string prev_selection_first;
static bool prev_video_player_show_subtitles = false;

// --- Subtitle info struct ---

struct SubtitleInfo {
	uint32_t file_data_id = 0;
	int format = 0;
};

// --- Movie data struct ---

struct MovieData {
	uint32_t AudioFileDataID = 0;
	uint32_t SubtitleFileDataID = 0;
	int SubtitleFileFormat = 0;
};

// --- Build payload result ---

struct BuildPayloadResult {
	nlohmann::json payload;
	std::optional<SubtitleInfo> subtitle;
};

// --- HTTP helper: POST JSON to Kino API ---
// Returns (status_code, response_body_json). Throws on connection error.
static std::pair<int, nlohmann::json> kino_post(const nlohmann::json& payload) {
	// TODO(conversion): Kino HTTP POST will be wired when full HTTP integration is complete.
	// The JS uses fetch() with POST; C++ uses cpp-httplib.
	// Parse the Kino API URL.
	const std::string api_url(constants::KINO::API_URL);

	// Use generics HTTP infrastructure pattern from generics.cpp.
	httplib::SSLClient cli("www.kruithne.net");
	cli.set_connection_timeout(30);
	cli.set_read_timeout(60);
	cli.set_follow_location(true);

	httplib::Headers headers;
	headers.emplace("Content-Type", "application/json");
	headers.emplace("User-Agent", std::string(constants::USER_AGENT()));

	const std::string body = payload.dump();
	auto res = cli.Post("/wow.export/v2/get_video", headers, body, "application/json");

	if (!res)
		throw std::runtime_error(std::format("HTTP request failed for {}: {}", api_url, httplib::to_string(res.error())));

	nlohmann::json response_json;
	if (!res->body.empty()) {
		try {
			response_json = nlohmann::json::parse(res->body);
		} catch (...) {
			// Body is not JSON, leave as null
		}
	}

	return { res->status, response_json };
}

// --- Internal functions ---

// JS: const stop_video = async (core_ref) => { ... }
static void stop_video() {
	poll_cancelled = true;

	// JS: if (poll_timer) { clearTimeout(poll_timer); poll_timer = null; }
	poll_active = false;

	// JS: if (current_video_element) { ... }
	// TODO(conversion): Video element pause/unload is a browser API; in C++ we just reset state.
	// The actual video playback is handled by the Kino streaming server.
	has_subtitle_track = false;
	current_subtitle_vtt.clear();

	// JS: if (current_subtitle_blob_url) { URLPolyfill.revokeObjectURL(current_subtitle_blob_url); ... }
	// TODO(conversion): Blob URLs are a browser concept; no cleanup needed in C++.

	is_streaming = false;
	core::view->videoPlayerState = false;
}

// JS: const build_payload = async (core_ref, file_data_id) => { ... }
static std::optional<BuildPayloadResult> build_payload(uint32_t file_data_id) {
	// JS: const casc = core_ref.view.casc;

	// get video encoding info
	// JS: const vid_info = await casc.getFileEncodingInfo(file_data_id);
	// TODO(conversion): CASC getFileEncodingInfo will be wired when CASC integration is complete.
	nlohmann::json vid_info;
	// Placeholder: vid_info should contain encoding info from CASC.
	// if (!vid_info) { ... }
	if (vid_info.is_null()) {
		logging::write(std::format("failed to get encoding info for video file {}", file_data_id));
		return std::nullopt;
	}

	nlohmann::json payload;
	payload["vid"] = vid_info;
	std::optional<SubtitleInfo> subtitle_info;

	// check if we have movie mapping
	if (movie_variation_map.has_value()) {
		auto it = movie_variation_map->find(file_data_id);
		if (it != movie_variation_map->end()) {
			uint32_t movie_id = it->second;
			try {
				// JS: const movie_row = await db2.Movie.getRow(movie_id);
				// TODO(conversion): DB2 Movie.getRow will be wired when DB2 system is fully integrated.
				auto& movie_table = casc::db2::getTable("Movie");
				(void)movie_table;
				nlohmann::json movie_row; // placeholder
				if (!movie_row.is_null()) {
					// get audio file encoding info
					// JS: if (movie_row.AudioFileDataID && movie_row.AudioFileDataID !== 0) { ... }
					uint32_t audio_fdid = movie_row.value("AudioFileDataID", 0u);
					if (audio_fdid != 0) {
						// JS: const aud_info = await casc.getFileEncodingInfo(movie_row.AudioFileDataID);
						// TODO(conversion): CASC getFileEncodingInfo will be wired when CASC integration is complete.
						nlohmann::json aud_info;
						if (!aud_info.is_null())
							payload["aud"] = aud_info;
					}

					// get subtitle file encoding info for server + store for local loading
					// JS: if (movie_row.SubtitleFileDataID && movie_row.SubtitleFileDataID !== 0) { ... }
					uint32_t sub_fdid = movie_row.value("SubtitleFileDataID", 0u);
					if (sub_fdid != 0) {
						// JS: const srt_info = await casc.getFileEncodingInfo(movie_row.SubtitleFileDataID);
						// TODO(conversion): CASC getFileEncodingInfo will be wired when CASC integration is complete.
						nlohmann::json srt_info;
						if (!srt_info.is_null()) {
							srt_info["type"] = movie_row.value("SubtitleFileFormat", 0);
							payload["srt"] = srt_info;
						}

						subtitle_info = SubtitleInfo{
							sub_fdid,
							movie_row.value("SubtitleFileFormat", 0)
						};
					}
				}
			} catch (const std::exception& e) {
				logging::write(std::format("failed to lookup movie data for movie_id {}: {}", movie_id, e.what()));
			}
		}
	}

	return BuildPayloadResult{ payload, subtitle_info };
}

// JS: const play_streaming_video = async (core_ref, url, video, subtitle_info) => { ... }
static void play_streaming_video(const std::string& url, const std::optional<SubtitleInfo>& subtitle_info) {
	// JS: current_video_element = video;
	// JS: video.src = url;
	// TODO(conversion): Video playback via URL will be wired when media playback is integrated.
	// The Kino server provides a direct MP4 URL for browser playback; in C++ this would need
	// a media player backend or external process.

	// always load subtitles if available, toggle visibility based on config
	if (subtitle_info.has_value()) {
		try {
			// JS: const vtt = await subtitles.get_subtitles_vtt(
			//     core_ref.view.casc, subtitle_info.file_data_id, subtitle_info.format);
			// TODO(conversion): Loading subtitle file from CASC will be wired when CASC integration is complete.
			// For now, we prepare the subtitle infrastructure:
			// 1. Load raw subtitle data from CASC via subtitle_info.file_data_id
			// 2. Convert to VTT using subtitles::get_subtitles_vtt(text, format)
			std::string raw_subtitle_text; // placeholder: loaded from CASC
			subtitles::SubtitleFormat fmt = static_cast<subtitles::SubtitleFormat>(subtitle_info->format);
			current_subtitle_vtt = subtitles::get_subtitles_vtt(raw_subtitle_text, fmt);
			has_subtitle_track = true;

			// JS: const blob = new BlobPolyfill([vtt], { type: 'text/vtt' });
			// JS: current_subtitle_blob_url = URLPolyfill.createObjectURL(blob);
			// TODO(conversion): Blob URL creation is a browser concept; VTT data stored directly.

			// JS: const track = document.createElement('track');
			// JS: track.kind = 'subtitles'; track.label = 'Subtitles'; track.srclang = 'en';
			// JS: track.src = current_subtitle_blob_url;
			// JS: video.appendChild(track); current_subtitle_track = track;
			// TODO(conversion): DOM track element is browser-specific; subtitle rendering will use ImGui overlay.

			// set initial visibility after track loads
			// JS: track.addEventListener('load', () => {
			//     track.track.mode = core_ref.view.config.videoPlayerShowSubtitles ? 'showing' : 'hidden';
			// });
			// TODO(conversion): Subtitle visibility controlled via config watch in render().

			logging::write(std::format("loaded subtitles for video (fdid: {}, format: {})",
				subtitle_info->file_data_id, subtitle_info->format));
		} catch (const std::exception& e) {
			logging::write(std::format("failed to load subtitles: {}", e.what()));
		}
	}

	// JS: video.load(); video.play().catch(e => { ... });
	// TODO(conversion): Media playback will be wired when video player integration is complete.

	// JS: video.onended = () => { ... };
	// TODO(conversion): Video end callback will be wired when media playback is integrated.
	// When video ends: is_streaming = false; core::view->videoPlayerState = false;

	// JS: video.onerror = () => { ... };
	// TODO(conversion): Video error callback will be wired when media playback is integrated.
}

// JS: const stream_video = async (core_ref, file_name, video) => { ... }
static void stream_video(const std::string& file_name) {
	const auto file_data_id_opt = casc::listfile::getByFilename(file_name);

	if (!file_data_id_opt.has_value()) {
		core::setToast("error", "Unable to find file in listfile");
		return;
	}

	const uint32_t file_data_id = *file_data_id_opt;
	logging::write(std::format("stream_video called for: {} (fdid: {})", file_name, file_data_id));

	try {
		stop_video();
		poll_cancelled = false;
		is_streaming = true;
		core::view->videoPlayerState = true;

		const auto build_result = build_payload(file_data_id);
		if (!build_result.has_value()) {
			core::setToast("error", "Failed to get video encoding info");
			is_streaming = false;
			core::view->videoPlayerState = false;
			return;
		}

		const auto& payload = build_result->payload;
		const auto& subtitle = build_result->subtitle;
		logging::write(std::format("sending kino request: {}", payload.dump()));

		// JS: const send_request = async () => { ... fetch(constants.KINO.API_URL, { method: 'POST', ... }) ... };
		auto [status, data] = kino_post(payload);

		// JS: const handle_response = async (res) => { ... };
		if (poll_cancelled)
			return;

		if (status == 200) {
			// JS: if (data.url) { ... }
			if (data.contains("url") && data["url"].is_string()) {
				std::string video_url = data["url"].get<std::string>();
				logging::write(std::format("received video url: {}", video_url));
				core::hideToast();
				play_streaming_video(video_url, subtitle);
			} else {
				throw std::runtime_error("server returned 200 but no url");
			}
		} else if (status == 202) {
			logging::write(std::format("video is queued for processing, polling in {}ms",
				constants::KINO::POLL_INTERVAL));

			core::setToast("progress", "Video is being processed, please wait...", nullptr, -1, true);

			// listen for toast cancellation
			// JS: const cancel_handler = () => { ... };
			// JS: core_ref.events.once('toast-cancelled', cancel_handler);
			size_t cancel_listener_id = core::events.once("toast-cancelled", []() {
				poll_cancelled = true;
				poll_active = false;
				is_streaming = false;
				core::view->videoPlayerState = false;
				logging::write("video processing cancelled by user");
			});

			// JS: poll_timer = setTimeout(async () => { ... }, constants.KINO.POLL_INTERVAL);
			// JS uses recursive setTimeout for indefinite polling; C++ uses a loop.
			// TODO(conversion): Polling is synchronous here. In a real async implementation,
			// this would be deferred to a background thread.
			poll_active = true;

			bool poll_done = false;
			while (!poll_done && !poll_cancelled) {
				// Sleep for poll interval, then retry
				std::this_thread::sleep_for(std::chrono::milliseconds(constants::KINO::POLL_INTERVAL));

				if (poll_cancelled) {
					core::events.off("toast-cancelled", cancel_listener_id);
					return;
				}

				try {
					auto [poll_status, poll_data] = kino_post(payload);

					if (poll_cancelled) {
						core::events.off("toast-cancelled", cancel_listener_id);
						return;
					}

					if (poll_status == 200) {
						if (poll_data.contains("url") && poll_data["url"].is_string()) {
							std::string video_url = poll_data["url"].get<std::string>();
							logging::write(std::format("received video url: {}", video_url));
							core::events.off("toast-cancelled", cancel_listener_id);
							core::hideToast();
							play_streaming_video(video_url, subtitle);
							poll_done = true;
						} else {
							core::events.off("toast-cancelled", cancel_listener_id);
							throw std::runtime_error("server returned 200 but no url");
						}
					} else if (poll_status == 202) {
						// Still processing — loop continues (matches JS recursive setTimeout).
						logging::write(std::format("video still processing, polling again in {}ms",
							constants::KINO::POLL_INTERVAL));
					} else {
						core::events.off("toast-cancelled", cancel_listener_id);
						throw std::runtime_error(std::format("server returned {}", poll_status));
					}
				} catch (const std::exception& e) {
					core::events.off("toast-cancelled", cancel_listener_id);
					if (!poll_cancelled) {
						logging::write(std::format("poll request failed: {}", e.what()));
						core::setToast("error", std::string("Failed to check video status: ") + e.what(), nullptr, -1);
						// TODO(conversion): 'view log' toast action will be wired when toast action callbacks are integrated.
						is_streaming = false;
						core::view->videoPlayerState = false;
					}
					poll_done = true;
				}
			}

			poll_active = false;
		} else {
			throw std::runtime_error(std::format("server returned {}", status));
		}

	} catch (const std::exception& e) {
		is_streaming = false;
		core::view->videoPlayerState = false;

		logging::write(std::format("failed to stream video {}: {}", file_name, e.what()));
		core::setToast("error", std::string("Failed to stream video: ") + e.what(), nullptr, -1);
		// TODO(conversion): 'view log' toast action will be wired when toast action callbacks are integrated.
	}
}

// JS: const load_video_listfile = async () => { ... }
static void load_video_listfile() {
	logging::write("loading MovieVariation table...");
	// JS: const movie_variation = await db2.preload.MovieVariation();
	// TODO(conversion): DB2 MovieVariation preload will be wired when DB2 system is fully integrated.
	auto& movie_variation = casc::db2::preloadTable("MovieVariation");

	movie_variation_map.emplace();
	std::unordered_set<uint32_t> seen_ids;
	std::vector<uint32_t> file_data_ids_vec;

	// JS: const rows = await movie_variation.getAllRows();
	// JS: for (const [id, row] of rows) { ... }
	// TODO(conversion): WDCReader::getAllRows iteration will be wired when full DB2 support is available.
	// Placeholder: iterate all rows and build mapping.
	// for (const auto& [id, row] : movie_variation.getAllRows()) {
	//     uint32_t fdid = row.at("FileDataID");
	//     uint32_t mid = row.at("MovieID");
	//     if (fdid && mid) {
	//         movie_variation_map->emplace(fdid, mid);
	//         if (!seen_ids.contains(fdid)) {
	//             seen_ids.insert(fdid);
	//             file_data_ids_vec.push_back(fdid);
	//         }
	//     }
	// }
	(void)movie_variation;
	(void)seen_ids;

	video_file_data_ids = std::move(file_data_ids_vec);

	logging::write(std::format("loaded {} movie variation mappings", movie_variation_map->size()));

	// JS: const entries = new Array(video_file_data_ids.length);
	std::vector<std::string> entries;
	entries.reserve(video_file_data_ids->size());

	for (const uint32_t fid : *video_file_data_ids) {
		std::string filename = casc::listfile::getByID(fid);

		if (filename.empty()) {
			filename = "interface/cinematics/unk_" + std::to_string(fid) + ".avi";
			casc::listfile::addEntry(fid, filename);
		}

		entries.push_back(std::format("{} [{}]", filename, fid));
	}

	// JS: if (core.view.config.listfileSortByID) ...
	if (core::view->config.value("listfileSortByID", false)) {
		std::sort(entries.begin(), entries.end(), [](const std::string& a, const std::string& b) {
			const auto fa = casc::listfile::getByFilename(casc::listfile::stripFileEntry(a));
			const auto fb = casc::listfile::getByFilename(casc::listfile::stripFileEntry(b));
			return fa.value_or(0) < fb.value_or(0);
		});
	} else {
		std::sort(entries.begin(), entries.end());
	}

	// JS: core.view.listfileVideos = entries;
	core::view->listfileVideos.clear();
	core::view->listfileVideos.reserve(entries.size());
	for (auto& e : entries)
		core::view->listfileVideos.push_back(std::move(e));

	logging::write(std::format("built video listfile with {} entries", core::view->listfileVideos.size()));
}

// JS: const get_movie_data = async (file_data_id) => { ... }
static std::optional<MovieData> get_movie_data(uint32_t file_data_id) {
	if (!movie_variation_map.has_value())
		return std::nullopt;

	auto it = movie_variation_map->find(file_data_id);
	if (it == movie_variation_map->end())
		return std::nullopt;

	uint32_t movie_id = it->second;

	try {
		// JS: const movie_row = await db2.Movie.getRow(movie_id);
		// TODO(conversion): DB2 Movie.getRow will be wired when DB2 system is fully integrated.
		auto& movie_table = casc::db2::getTable("Movie");
		(void)movie_table;
		nlohmann::json movie_row; // placeholder
		if (movie_row.is_null())
			return std::nullopt;

		return MovieData{
			static_cast<uint32_t>(movie_row.value("AudioFileDataID", 0)),
			static_cast<uint32_t>(movie_row.value("SubtitleFileDataID", 0)),
			movie_row.value("SubtitleFileFormat", 0)
		};
	} catch (const std::exception& e) {
		logging::write(std::format("failed to get movie data for fdid {}: {}", file_data_id, e.what()));
		return std::nullopt;
	}
}

// JS: const get_mp4_url = async (payload) => { ... }
static std::optional<std::string> get_mp4_url(const nlohmann::json& payload) {
	// JS: const send_request = async () => { ... };
	// JS: const poll_for_url = async () => { ... };

	// Recursive polling: send request, if 202 wait and retry, if 200 return url.
	while (true) {
		auto [status, data] = kino_post(payload);

		if (status == 200) {
			// JS: return data.url || null;
			if (data.contains("url") && data["url"].is_string())
				return data["url"].get<std::string>();
			return std::nullopt;
		} else if (status == 202) {
			// video queued, wait and retry
			// JS: await new Promise(resolve => setTimeout(resolve, constants.KINO.POLL_INTERVAL));
			std::this_thread::sleep_for(std::chrono::milliseconds(constants::KINO::POLL_INTERVAL));
			// return poll_for_url(); — loop continues
		} else {
			return std::nullopt;
		}
	}
}

// JS: const trigger_kino_processing = async () => { ... }
static void trigger_kino_processing() {
	if (!video_file_data_ids.has_value() || video_file_data_ids->empty()) {
		logging::write("kino_processing: no video file data ids loaded");
		core::setToast("error", "Videos not loaded. Open the Videos tab first.");
		return;
	}

	kino_processing_cancelled = false;
	const size_t total = video_file_data_ids->size();
	size_t processed = 0;
	size_t errors = 0;

	logging::write(std::format("kino_processing: starting processing of {} videos", total));

	// JS: const update_toast = () => { ... };
	auto update_toast = [&]() {
		if (kino_processing_cancelled)
			return;

		const std::string msg = std::format("Processing videos: {}/{} ({} errors)", processed, total, errors);
		core::setToast("progress", msg, nullptr, -1, true);
	};

	// JS: const cancel_processing = () => { ... };
	// JS: core.events.once('toast-cancelled', cancel_processing);
	size_t cancel_listener_id = core::events.once("toast-cancelled", [&]() {
		kino_processing_cancelled = true;
		logging::write(std::format("kino_processing: cancelled by user at {}/{}", processed, total));
		core::setToast("info", std::format("Video processing cancelled. Processed {}/{} videos.", processed, total));
	});

	update_toast();

	for (const uint32_t file_data_id : *video_file_data_ids) {
		if (kino_processing_cancelled)
			break;

		try {
			const auto build_result = build_payload(file_data_id);
			if (!build_result.has_value()) {
				logging::write(std::format("kino_processing: failed to build payload for fdid {}", file_data_id));
				errors++;
				processed++;
				update_toast();
				continue;
			}

			const auto& payload = build_result->payload;

			// poll until we get 200 or error
			bool done = false;
			while (!done && !kino_processing_cancelled) {
				auto [status, data] = kino_post(payload);

				if (status == 200) {
					done = true;
				} else if (status == 202) {
					// JS: await new Promise(resolve => setTimeout(resolve, constants.KINO.POLL_INTERVAL));
					std::this_thread::sleep_for(std::chrono::milliseconds(constants::KINO::POLL_INTERVAL));
				} else {
					logging::write(std::format("kino_processing: unexpected status {} for fdid {}", status, file_data_id));
					errors++;
					done = true;
				}
			}
		} catch (const std::exception& e) {
			logging::write(std::format("kino_processing: error processing fdid {}: {}", file_data_id, e.what()));
			errors++;
		}

		processed++;
		update_toast();
	}

	core::events.off("toast-cancelled", cancel_listener_id);

	if (!kino_processing_cancelled) {
		logging::write(std::format("kino_processing: completed {}/{} videos with {} errors", processed, total, errors));
		core::setToast("success", std::format("Video processing complete. {}/{} videos, {} errors.", processed, total, errors));
	}
}

// expose to window in dev mode
// JS: if (!BUILD_RELEASE) window.trigger_kino_processing = trigger_kino_processing;
// TODO(conversion): Dev-mode exposure of trigger_kino_processing handled via isDev flag and dev console.

// --- Export methods ---

// JS: methods.export_mp4()
static void export_mp4() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionVideos;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	casc::ExportHelper helper(static_cast<int>(user_selection.size()), "video");
	helper.start();

	const bool overwrite_files = view.config.value("overwriteFiles", false);
	for (const auto& sel_entry : user_selection) {
		if (helper.isCancelled())
			return;

		std::string file_name = casc::listfile::stripFileEntry(sel_entry.get<std::string>());
		const auto file_data_id_opt = casc::listfile::getByFilename(file_name);

		if (!file_data_id_opt.has_value()) {
			helper.mark(file_name, false, "File not found in listfile");
			continue;
		}

		const uint32_t file_data_id = *file_data_id_opt;

		// JS: let export_file_name = ExportHelper.replaceExtension(file_name, '.mp4');
		std::string export_file_name = casc::ExportHelper::replaceExtension(file_name, ".mp4");
		// JS: if (!this.$core.view.config.exportNamedFiles) { ... }
		if (!view.config.value("exportNamedFiles", true)) {
			namespace fs = std::filesystem;
			const std::string dir = fs::path(file_name).parent_path().string();
			const std::string file_data_id_name = std::to_string(file_data_id) + ".mp4";
			export_file_name = dir == "." ? file_data_id_name : (fs::path(dir) / file_data_id_name).string();
		}

		const std::string export_path = casc::ExportHelper::getExportPath(export_file_name);

		if (!overwrite_files && generics::fileExists(export_path)) {
			helper.mark(export_file_name, true);
			logging::write(std::format("Skipping MP4 export {} (file exists, overwrite disabled)", export_path));
			continue;
		}

		try {
			const auto build_result = build_payload(file_data_id);
			if (!build_result.has_value()) {
				helper.mark(export_file_name, false, "Failed to get encoding info");
				continue;
			}

			const auto& payload = build_result->payload;
			const auto mp4_url = get_mp4_url(payload);

			if (!mp4_url.has_value()) {
				helper.mark(export_file_name, false, "Failed to get MP4 URL from server");
				continue;
			}

			// JS: const response = await fetch(mp4_url, { headers: { 'User-Agent': constants.USER_AGENT } });
			std::vector<uint8_t> mp4_data = generics::get(*mp4_url);

			// JS: await fsp.mkdir(path.dirname(export_path), { recursive: true });
			namespace fs = std::filesystem;
			fs::create_directories(fs::path(export_path).parent_path());

			// JS: await fsp.writeFile(export_path, Buffer.from(buffer));
			std::ofstream out(export_path, std::ios::binary);
			out.write(reinterpret_cast<const char*>(mp4_data.data()), static_cast<std::streamsize>(mp4_data.size()));
			out.close();

			helper.mark(export_file_name, true);
		} catch (const std::exception& e) {
			helper.mark(export_file_name, false, e.what());
		}
	}

	helper.finish();
}

// JS: methods.export_avi()
static void export_avi() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionVideos;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	casc::ExportHelper helper(static_cast<int>(user_selection.size()), "video");
	helper.start();

	const bool overwrite_files = view.config.value("overwriteFiles", false);
	for (const auto& sel_entry : user_selection) {
		if (helper.isCancelled())
			return;

		std::string file_name = casc::listfile::stripFileEntry(sel_entry.get<std::string>());
		std::string export_file_name = file_name;

		// JS: if (!this.$core.view.config.exportNamedFiles) { ... }
		if (!view.config.value("exportNamedFiles", true)) {
			const auto file_data_id_opt = casc::listfile::getByFilename(file_name);
			if (file_data_id_opt.has_value()) {
				namespace fs = std::filesystem;
				const std::string ext = fs::path(file_name).extension().string();
				const std::string dir = fs::path(file_name).parent_path().string();
				const std::string file_data_id_name = std::to_string(*file_data_id_opt) + ext;
				export_file_name = dir == "." ? file_data_id_name : (fs::path(dir) / file_data_id_name).string();
			}
		}

		const std::string export_path = casc::ExportHelper::getExportPath(export_file_name);
		bool is_corrupted = false;

		if (overwrite_files || !generics::fileExists(export_path)) {
			try {
				// JS: const data = await this.$core.view.casc.getFileByName(file_name);
				// JS: await data.writeToFile(export_path);
				// TODO(conversion): CASC getFileByName + writeToFile will be wired when CASC integration is complete.

				helper.mark(export_file_name, true);
			} catch (const casc::BLTEIntegrityError&) {
				is_corrupted = true;
			} catch (const std::exception& e) {
				helper.mark(export_file_name, false, e.what());
			}

			if (is_corrupted) {
				try {
					logging::write("Local cinematic file is corrupted, forcing fallback.");

					// JS: const data = await this.$core.view.casc.getFileByName(file_name, false, false, true, true);
					// JS: await data.writeToFile(export_path);
					// TODO(conversion): CASC getFileByName with force fallback will be wired when CASC integration is complete.

					helper.mark(export_file_name, true);
				} catch (const std::exception& e) {
					helper.mark(export_file_name, false, e.what());
				}
			}
		} else {
			helper.mark(export_file_name, true);
			logging::write(std::format("Skipping video export {} (file exists, overwrite disabled)", export_path));
		}
	}

	helper.finish();
}

// JS: methods.export_mp3()
static void export_mp3() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionVideos;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	casc::ExportHelper helper(static_cast<int>(user_selection.size()), "audio track");
	helper.start();

	const bool overwrite_files = view.config.value("overwriteFiles", false);
	for (const auto& sel_entry : user_selection) {
		if (helper.isCancelled())
			return;

		std::string file_name = casc::listfile::stripFileEntry(sel_entry.get<std::string>());
		const auto file_data_id_opt = casc::listfile::getByFilename(file_name);

		if (!file_data_id_opt.has_value()) {
			helper.mark(file_name, false, "File not found in listfile");
			continue;
		}

		const auto movie_data = get_movie_data(*file_data_id_opt);
		if (!movie_data.has_value() || movie_data->AudioFileDataID == 0) {
			helper.mark(file_name, false, "No audio track available for this video");
			continue;
		}

		// JS: let export_file_name = ExportHelper.replaceExtension(file_name, '.mp3');
		std::string export_file_name = casc::ExportHelper::replaceExtension(file_name, ".mp3");
		// JS: if (!this.$core.view.config.exportNamedFiles) { ... }
		if (!view.config.value("exportNamedFiles", true)) {
			namespace fs = std::filesystem;
			const std::string dir = fs::path(file_name).parent_path().string();
			const std::string file_data_id_name = std::to_string(movie_data->AudioFileDataID) + ".mp3";
			export_file_name = dir == "." ? file_data_id_name : (fs::path(dir) / file_data_id_name).string();
		}

		const std::string export_path = casc::ExportHelper::getExportPath(export_file_name);

		if (!overwrite_files && generics::fileExists(export_path)) {
			helper.mark(export_file_name, true);
			logging::write(std::format("Skipping audio export {} (file exists, overwrite disabled)", export_path));
			continue;
		}

		try {
			// JS: const data = await this.$core.view.casc.getFile(movie_data.AudioFileDataID);
			// JS: await data.writeToFile(export_path);
			// TODO(conversion): CASC getFile + writeToFile will be wired when CASC integration is complete.
			helper.mark(export_file_name, true);
		} catch (const std::exception& e) {
			helper.mark(export_file_name, false, e.what());
		}
	}

	helper.finish();
}

// JS: methods.export_subtitles()
static void export_subtitles() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionVideos;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	casc::ExportHelper helper(static_cast<int>(user_selection.size()), "subtitle");
	helper.start();

	const bool overwrite_files = view.config.value("overwriteFiles", false);
	for (const auto& sel_entry : user_selection) {
		if (helper.isCancelled())
			return;

		std::string file_name = casc::listfile::stripFileEntry(sel_entry.get<std::string>());
		const auto file_data_id_opt = casc::listfile::getByFilename(file_name);

		if (!file_data_id_opt.has_value()) {
			helper.mark(file_name, false, "File not found in listfile");
			continue;
		}

		const auto movie_data = get_movie_data(*file_data_id_opt);
		if (!movie_data.has_value() || movie_data->SubtitleFileDataID == 0) {
			helper.mark(file_name, false, "No subtitles available for this video");
			continue;
		}

		// determine extension based on subtitle format
		// JS: const ext = movie_data.SubtitleFileFormat === subtitles.SUBTITLE_FORMAT.SBT ? '.sbt' : '.srt';
		const std::string ext = (movie_data->SubtitleFileFormat == static_cast<int>(subtitles::SubtitleFormat::SBT)) ? ".sbt" : ".srt";

		// JS: let export_file_name = ExportHelper.replaceExtension(file_name, ext);
		std::string export_file_name = casc::ExportHelper::replaceExtension(file_name, ext);
		// JS: if (!this.$core.view.config.exportNamedFiles) { ... }
		if (!view.config.value("exportNamedFiles", true)) {
			namespace fs = std::filesystem;
			const std::string dir = fs::path(file_name).parent_path().string();
			const std::string file_data_id_name = std::to_string(movie_data->SubtitleFileDataID) + ext;
			export_file_name = dir == "." ? file_data_id_name : (fs::path(dir) / file_data_id_name).string();
		}

		const std::string export_path = casc::ExportHelper::getExportPath(export_file_name);

		if (!overwrite_files && generics::fileExists(export_path)) {
			helper.mark(export_file_name, true);
			logging::write(std::format("Skipping subtitle export {} (file exists, overwrite disabled)", export_path));
			continue;
		}

		try {
			// JS: const data = await this.$core.view.casc.getFile(movie_data.SubtitleFileDataID);
			// JS: await data.writeToFile(export_path);
			// TODO(conversion): CASC getFile + writeToFile will be wired when CASC integration is complete.
			helper.mark(export_file_name, true);
		} catch (const std::exception& e) {
			helper.mark(export_file_name, false, e.what());
		}
	}

	helper.finish();
}

// --- Public API ---

// JS: register() { this.registerNavButton('Videos', 'film.svg', InstallType.CASC); }
void registerTab() {
	// TODO(conversion): Nav button registration will be wired when the module system is integrated.
}

// JS: async mounted() { ... }
void mounted() {
	auto& view = *core::view;

	// JS: await this.initialize();
	core::showLoadingScreen(1);
	core::progressLoadingScreen("Loading video metadata...");
	load_video_listfile();
	core::hideLoadingScreen();

	// Store initial config values for change-detection.
	prev_video_player_show_subtitles = view.config.value("videoPlayerShowSubtitles", false);
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for selection (equivalent to watch on selectionVideos) ---
	// JS: this.$core.view.$watch('selectionVideos', async selection => { ... });
	if (!view.selectionVideos.empty()) {
		const std::string first = casc::listfile::stripFileEntry(view.selectionVideos[0].get<std::string>());
		if (view.isBusy == 0 && !first.empty() && selected_file != first) {
			// cancel any pending polls when selection changes
			poll_cancelled = true;
			poll_active = false;

			selected_file = first;

			if (view.config.value("videoPlayerAutoPlay", false))
				stream_video(first);

			prev_selection_first = first;
		}
	}

	// --- Change-detection for subtitle visibility ---
	// JS: this.$core.view.$watch('config.videoPlayerShowSubtitles', show => { ... });
	const bool current_show_subtitles = view.config.value("videoPlayerShowSubtitles", false);
	if (current_show_subtitles != prev_video_player_show_subtitles) {
		// JS: if (current_subtitle_track && current_subtitle_track.track)
		//     current_subtitle_track.track.mode = show ? 'showing' : 'hidden';
		// TODO(conversion): Subtitle visibility toggle will be wired when video player subtitle rendering is integrated.
		prev_video_player_show_subtitles = current_show_subtitles;
	}

	// --- Template rendering ---

	// JS: <div class="tab list-tab" id="tab-video">

	// List container with context menu.
	// JS: <div class="list-container">
	//     <Listbox v-model:selection="selectionVideos" :items="listfileVideos"
	//         :filter="userInputFilterVideos" :keyinput="true"
	//         :regex="config.regexFilters" :copymode="config.copyMode"
	//         :pasteselection="config.pasteSelection"
	//         :copytrimwhitespace="config.removePathSpacesCopy"
	//         :includefilecount="true" unittype="video"
	//         persistscrollkey="videos" @contextmenu="handle_listbox_context">
	//     <ContextMenu :node="contextMenus.nodeListbox" ...>
	//         <span @click.self="copy_file_paths(...)">Copy file path(s)</span>
	//         <span v-if="hasFileDataIDs" @click.self="copy_listfile_format(...)">Copy file path(s) (listfile format)</span>
	//         <span v-if="hasFileDataIDs" @click.self="copy_file_data_ids(...)">Copy file data ID(s)</span>
	//         <span @click.self="copy_export_paths(...)">Copy export path(s)</span>
	//         <span @click.self="open_export_directory(...)">Open export directory</span>
	//     </ContextMenu>
	// </div>
	ImGui::BeginChild("videos-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox and ContextMenu component rendering will be wired when integration is complete.
	ImGui::Text("Video files: %zu", view.listfileVideos.size());
	ImGui::EndChild();

	ImGui::SameLine();

	// Right side — filter, preview, controls.
	ImGui::BeginGroup();

	// Filter.
	// JS: <div class="filter">
	//     <div class="regex-info" v-if="config.regexFilters" ...>Regex Enabled</div>
	//     <input type="text" v-model="userInputFilterVideos" placeholder="Filter videos..."/>
	// </div>
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterVideos.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterVideos", filter_buf, sizeof(filter_buf)))
		view.userInputFilterVideos = filter_buf;

	// Preview container.
	// JS: <div class="preview-container">
	//     <video ref="video_player" class="preview-background" style="..." controls ...></video>
	// </div>
	ImGui::BeginChild("video-preview-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), ImGuiChildFlags_Borders);
	// TODO(conversion): Video preview rendering will be wired when media playback integration is complete.
	// In the browser version, this is a <video> element with native playback controls.
	// In C++, this will require a media playback backend (e.g., FFmpeg + OpenGL texture).
	if (is_streaming || view.videoPlayerState)
		ImGui::TextUnformatted("Video playback active...");
	else
		ImGui::TextUnformatted("No video playing");
	ImGui::EndChild();

	// Preview controls.
	// JS: <div class="preview-controls">
	//     <label class="ui-checkbox">
	//         <input type="checkbox" v-model="config.videoPlayerAutoPlay"/>
	//         <span>Autoplay</span>
	//     </label>
	//     <label class="ui-checkbox">
	//         <input type="checkbox" v-model="config.videoPlayerShowSubtitles"/>
	//         <span>Show Subtitles</span>
	//     </label>
	//     <div class="tray"></div>
	//     <MenuButton :options="menuButtonVideos" :default="config.exportVideoFormat"
	//         @change="config.exportVideoFormat = $event" :disabled="isBusy"
	//         @click="export_selected">
	// </div>
	bool autoplay_val = view.config.value("videoPlayerAutoPlay", false);
	if (ImGui::Checkbox("Autoplay", &autoplay_val))
		view.config["videoPlayerAutoPlay"] = autoplay_val;

	ImGui::SameLine();

	bool show_subs = view.config.value("videoPlayerShowSubtitles", false);
	if (ImGui::Checkbox("Show Subtitles", &show_subs))
		view.config["videoPlayerShowSubtitles"] = show_subs;

	ImGui::SameLine();

	// Spacer (tray)
	ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 200.0f, 0));
	ImGui::SameLine();

	const bool busy = view.isBusy > 0;
	if (busy) ImGui::BeginDisabled();
	if (ImGui::Button("Export Selected"))
		export_selected();
	if (busy) ImGui::EndDisabled();

	ImGui::EndGroup();
}

// JS: methods.preview_video()
void preview_video() {
	auto& view = *core::view;

	if (view.videoPlayerState) {
		stop_video();
		return;
	}

	const auto& selection = view.selectionVideos;
	if (selection.empty()) {
		core::setToast("info", "Select a video file first");
		return;
	}

	const std::string file_name = casc::listfile::stripFileEntry(selection[0].get<std::string>());
	selected_file = file_name;
	stream_video(file_name);
}

// JS: methods.export_selected()
void export_selected() {
	const std::string format = core::view->config.value("exportVideoFormat", std::string("AVI"));

	if (format == "MP4") {
		export_mp4();
	} else if (format == "AVI") {
		export_avi();
	} else if (format == "MP3") {
		export_mp3();
	} else if (format == "SUBTITLES") {
		export_subtitles();
	}
}

} // namespace tab_videos
