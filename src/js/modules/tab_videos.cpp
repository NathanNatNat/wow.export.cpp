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
#include "../db/WDCReader.h"
#include "../ui/listbox-context.h"
#include "../components/listbox.h"
#include "../components/context-menu.h"
#include "../install-type.h"
#include "../modules.h"
#include "../../app.h"

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
#include <mutex>
#include <atomic>

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <httplib.h>

namespace tab_videos {

static uint32_t fieldToUint32(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<uint32_t>(*p);
	return 0;
}

static int fieldToInt(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<int>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<int>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<int>(*p);
	return 0;
}

// --- File-local state ---

static std::optional<std::unordered_map<uint32_t, uint32_t>> movie_variation_map;

static std::optional<std::vector<uint32_t>> video_file_data_ids;

static std::string selected_file;

// Video playback uses Kino server streaming; no local video element in C++.
static std::string current_video_url;

static bool has_subtitle_track = false;

static std::string current_subtitle_vtt;

static bool is_streaming = false;

static std::atomic<bool> poll_active{false};

static std::atomic<bool> poll_cancelled{false};

static bool kino_processing_cancelled = false;

// Background thread for stream_video HTTP + polling (replaces JS async/await).
static std::unique_ptr<std::jthread> stream_worker_thread;
static std::mutex stream_result_mutex;

// Change-detection for selection and config watches.
static std::string prev_selection_first;
static bool prev_video_player_show_subtitles = false;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

// Cached items string vector — only rebuilt when the source JSON changes.
static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

struct SubtitleInfo {
	uint32_t file_data_id = 0;
	int format = 0;
};

// Results from the background streaming thread, consumed on the main thread.
struct StreamResult {
	bool success = false;
	std::string video_url;
	std::optional<SubtitleInfo> subtitle;
	std::string error_message;
};
static std::optional<StreamResult> pending_stream_result;

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

// --- Helper: convert FileEncodingInfo to JSON for Kino API ---
static nlohmann::json encoding_info_to_json(const casc::FileEncodingInfo& info) {
	nlohmann::json j;
	j["enc"] = info.enc;
	if (info.arc.has_value()) {
		j["arc"] = {
			{"key", info.arc->key},
			{"ofs", info.arc->ofs},
			{"len", info.arc->len}
		};
	}
	return j;
}

// --- HTTP helper: POST JSON to Kino API ---
// Returns (status_code, response_body_json). Throws on connection error.
static std::pair<int, nlohmann::json> kino_post(const nlohmann::json& payload) {
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

static void stop_video() {
	poll_cancelled.store(true);

	poll_active = false;

	// Video element pause/unload is a browser API; in C++ we reset state.
	// The video was opened externally via the platform shell; we clear the URL.
	current_video_url.clear();
	has_subtitle_track = false;
	current_subtitle_vtt.clear();

	is_streaming = false;
	core::view->videoPlayerState = false;
}

static std::optional<BuildPayloadResult> build_payload(uint32_t file_data_id) {

	// get video encoding info
	auto vid_info_opt = core::view->casc->getFileEncodingInfo(file_data_id);
	if (!vid_info_opt.has_value()) {
		logging::write(std::format("failed to get encoding info for video file {}", file_data_id));
		return std::nullopt;
	}
	nlohmann::json vid_info = encoding_info_to_json(*vid_info_opt);

	nlohmann::json payload;
	payload["vid"] = vid_info;
	std::optional<SubtitleInfo> subtitle_info;

	// check if we have movie mapping
	if (movie_variation_map.has_value()) {
		auto it = movie_variation_map->find(file_data_id);
		if (it != movie_variation_map->end()) {
			uint32_t movie_id = it->second;
			try {
				auto& movie_table = casc::db2::getTable("Movie");
				if (!movie_table.isLoaded)
					movie_table.parse();
				auto movie_row_opt = movie_table.getRow(movie_id);
				if (movie_row_opt.has_value()) {
					const auto& movie_row = *movie_row_opt;
					// get audio file encoding info
					uint32_t audio_fdid = 0;
					auto aud_it = movie_row.find("AudioFileDataID");
					if (aud_it != movie_row.end())
						audio_fdid = fieldToUint32(aud_it->second);
					if (audio_fdid != 0) {
						auto aud_info_opt = core::view->casc->getFileEncodingInfo(audio_fdid);
						if (aud_info_opt.has_value())
							payload["aud"] = encoding_info_to_json(*aud_info_opt);
					}

					// get subtitle file encoding info for server + store for local loading
					uint32_t sub_fdid = 0;
					auto sub_it = movie_row.find("SubtitleFileDataID");
					if (sub_it != movie_row.end())
						sub_fdid = fieldToUint32(sub_it->second);
					int sub_format = 0;
					auto fmt_it = movie_row.find("SubtitleFileFormat");
					if (fmt_it != movie_row.end())
						sub_format = fieldToInt(fmt_it->second);
					if (sub_fdid != 0) {
						auto srt_info_opt = core::view->casc->getFileEncodingInfo(sub_fdid);
						if (srt_info_opt.has_value()) {
							nlohmann::json srt_info = encoding_info_to_json(*srt_info_opt);
							srt_info["type"] = sub_format;
							payload["srt"] = srt_info;
						}

						subtitle_info = SubtitleInfo{
							sub_fdid,
							sub_format
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

static void play_streaming_video(const std::string& url, const std::optional<SubtitleInfo>& subtitle_info) {
	// The Kino server provides a direct MP4 URL. In C++ there is no built-in video element;
	// the URL is opened in the system's default media player / browser via the platform shell
	// (ShellExecuteW on Windows, xdg-open on Linux), matching the NW.js → C++ translation
	// pattern for browser-specific APIs.
	current_video_url = url;

	// always load subtitles if available, toggle visibility based on config
	if (subtitle_info.has_value()) {
		try {
			subtitles::SubtitleFormat fmt = static_cast<subtitles::SubtitleFormat>(subtitle_info->format);
			current_subtitle_vtt = subtitles::get_subtitles_vtt(core::view->casc, subtitle_info->file_data_id, fmt);
			has_subtitle_track = true;

			logging::write(std::format("loaded subtitles for video (fdid: {}, format: {})",
				subtitle_info->file_data_id, subtitle_info->format));
		} catch (const std::exception& e) {
			logging::write(std::format("failed to load subtitles: {}", e.what()));
		}
	}

	// Open the video URL in the system's default handler (browser/media player).
	core::openInExplorer(url);
	logging::write(std::format("opened video URL in system handler: {}", url));

	// With external playback there is no direct "ended" callback.
	// The user stops playback via stop_video() which resets:
	//   is_streaming = false; core::view->videoPlayerState = false;

	// With external playback there is no direct "error" callback.
	// HTTP errors are caught by kino_post(); launch errors are logged above.
}

static void stream_video(const std::string& file_name) {
	const auto file_data_id_opt = casc::listfile::getByFilename(file_name);

	if (!file_data_id_opt.has_value()) {
		core::setToast("error", "Unable to find file in listfile");
		return;
	}

	const uint32_t file_data_id = *file_data_id_opt;
	logging::write(std::format("stream_video called for: {} (fdid: {})", file_name, file_data_id));

	stop_video();
	poll_cancelled.store(false);
	is_streaming = true;
	core::view->videoPlayerState = true;

	const auto build_result = build_payload(file_data_id);
	if (!build_result.has_value()) {
		core::setToast("error", "Failed to get video encoding info");
		is_streaming = false;
		core::view->videoPlayerState = false;
		return;
	}

	// Capture payload and subtitle by value for the background thread.
	nlohmann::json payload = build_result->payload;
	std::optional<SubtitleInfo> subtitle = build_result->subtitle;
	logging::write(std::format("sending kino request: {}", payload.dump()));

	// Show a progress toast on the main thread; it will be hidden when the result arrives.
	core::setToast("progress", "Connecting to video server...", {}, -1, true);

	// Register cancellation handler on the main thread (toast-cancelled is emitted on main thread).
	// Only set the atomic flag here; main-thread state (is_streaming, videoPlayerState) is
	// cleaned up in the result-consumption block in render() to avoid races with the background thread.
	core::events.once("toast-cancelled", []() {
		poll_cancelled.store(true);
		poll_active.store(false);
		is_streaming = false;
		core::view->videoPlayerState = false;
		logging::write("video streaming cancelled by user");
	});

	// Ensure any previous background thread is completed before launching a new one.
	if (stream_worker_thread)
		stream_worker_thread.reset();

	// Launch the HTTP request + polling on a background thread so the UI stays responsive.
	// Results are posted via stream_result_mutex / pending_stream_result and consumed in render().
	stream_worker_thread = std::make_unique<std::jthread>([payload = std::move(payload),
	                                                        subtitle = std::move(subtitle),
	                                                        file_name]() {
		try {
			auto [status, data] = kino_post(payload);

			if (poll_cancelled.load())
				return;

			if (status == 200) {
				if (data.contains("url") && data["url"].is_string()) {
					std::string video_url = data["url"].get<std::string>();
					logging::write(std::format("received video url: {}", video_url));

					std::lock_guard<std::mutex> lock(stream_result_mutex);
					pending_stream_result = StreamResult{true, std::move(video_url), subtitle, {}};
				} else {
					throw std::runtime_error("server returned 200 but no url");
				}
			} else if (status == 202) {
				logging::write(std::format("video is queued for processing, polling in {}ms",
					constants::KINO::POLL_INTERVAL));

				poll_active = true;

				bool poll_done = false;
				while (!poll_done && !poll_cancelled.load()) {
					// Sleep for poll interval, then retry
					std::this_thread::sleep_for(std::chrono::milliseconds(constants::KINO::POLL_INTERVAL));

					if (poll_cancelled.load())
						return;

					try {
						auto [poll_status, poll_data] = kino_post(payload);

						if (poll_cancelled.load())
							return;

						if (poll_status == 200) {
							if (poll_data.contains("url") && poll_data["url"].is_string()) {
								std::string video_url = poll_data["url"].get<std::string>();
								logging::write(std::format("received video url: {}", video_url));

								std::lock_guard<std::mutex> lock(stream_result_mutex);
								pending_stream_result = StreamResult{true, std::move(video_url), subtitle, {}};
								poll_done = true;
							} else {
								throw std::runtime_error("server returned 200 but no url");
							}
						} else if (poll_status == 202) {
							// Still processing — loop continues (matches JS recursive setTimeout).
							logging::write(std::format("video still processing, polling again in {}ms",
								constants::KINO::POLL_INTERVAL));
						} else {
							throw std::runtime_error(std::format("server returned {}", poll_status));
						}
					} catch (const std::exception& e) {
						if (!poll_cancelled.load()) {
							logging::write(std::format("poll request failed: {}", e.what()));
							std::lock_guard<std::mutex> lock(stream_result_mutex);
							pending_stream_result = StreamResult{false, {}, {}, std::string("Failed to check video status: ") + e.what()};
						}
						poll_done = true;
					}
				}

				poll_active = false;
			} else {
				throw std::runtime_error(std::format("server returned {}", status));
			}
		} catch (const std::exception& e) {
			logging::write(std::format("failed to stream video {}: {}", file_name, e.what()));
			std::lock_guard<std::mutex> lock(stream_result_mutex);
			pending_stream_result = StreamResult{false, {}, {}, std::string("Failed to stream video: ") + e.what()};
		}
	});
}

static void load_video_listfile() {
	logging::write("loading MovieVariation table...");
	auto& movie_variation = casc::db2::preloadTable("MovieVariation");

	movie_variation_map.emplace();
	std::unordered_set<uint32_t> seen_ids;
	std::vector<uint32_t> file_data_ids_vec;

	for (const auto& [id, row] : movie_variation.getAllRows()) {
		uint32_t fdid = 0;
		uint32_t mid = 0;
		auto fdid_it = row.find("FileDataID");
		if (fdid_it != row.end())
			fdid = fieldToUint32(fdid_it->second);
		auto mid_it = row.find("MovieID");
		if (mid_it != row.end())
			mid = fieldToUint32(mid_it->second);
		if (fdid != 0 && mid != 0) {
			movie_variation_map->emplace(fdid, mid);
			if (!seen_ids.contains(fdid)) {
				seen_ids.insert(fdid);
				file_data_ids_vec.push_back(fdid);
			}
		}
	}

	video_file_data_ids = std::move(file_data_ids_vec);

	logging::write(std::format("loaded {} movie variation mappings", movie_variation_map->size()));

	std::vector<std::string> entries;
	entries.reserve(video_file_data_ids->size());

	for (const uint32_t fid : *video_file_data_ids) {
		std::string filename = casc::listfile::getByID(fid).value_or("");

		if (filename.empty()) {
			filename = "interface/cinematics/unk_" + std::to_string(fid) + ".avi";
			casc::listfile::addEntry(fid, filename);
		}

		entries.push_back(std::format("{} [{}]", filename, fid));
	}

	if (core::view->config.value("listfileSortByID", false)) {
		std::sort(entries.begin(), entries.end(), [](const std::string& a, const std::string& b) {
			const auto fa = casc::listfile::getByFilename(casc::listfile::stripFileEntry(a));
			const auto fb = casc::listfile::getByFilename(casc::listfile::stripFileEntry(b));
			return fa.value_or(0) < fb.value_or(0);
		});
	} else {
		std::sort(entries.begin(), entries.end());
	}

	core::view->listfileVideos.clear();
	core::view->listfileVideos.reserve(entries.size());
	for (auto& e : entries)
		core::view->listfileVideos.push_back(std::move(e));

	logging::write(std::format("built video listfile with {} entries", core::view->listfileVideos.size()));
}

static std::optional<MovieData> get_movie_data(uint32_t file_data_id) {
	if (!movie_variation_map.has_value())
		return std::nullopt;

	auto it = movie_variation_map->find(file_data_id);
	if (it == movie_variation_map->end())
		return std::nullopt;

	uint32_t movie_id = it->second;

	try {
		auto& movie_table = casc::db2::getTable("Movie");
		if (!movie_table.isLoaded)
			movie_table.parse();
		auto movie_row_opt = movie_table.getRow(movie_id);
		if (!movie_row_opt.has_value())
			return std::nullopt;

		const auto& movie_row = *movie_row_opt;
		uint32_t audio_fdid = 0;
		auto aud_it = movie_row.find("AudioFileDataID");
		if (aud_it != movie_row.end())
			audio_fdid = fieldToUint32(aud_it->second);

		uint32_t sub_fdid = 0;
		auto sub_it = movie_row.find("SubtitleFileDataID");
		if (sub_it != movie_row.end())
			sub_fdid = fieldToUint32(sub_it->second);

		int sub_format = 0;
		auto fmt_it = movie_row.find("SubtitleFileFormat");
		if (fmt_it != movie_row.end())
			sub_format = fieldToInt(fmt_it->second);

		return MovieData{
			audio_fdid,
			sub_fdid,
			sub_format
		};
	} catch (const std::exception& e) {
		logging::write(std::format("failed to get movie data for fdid {}: {}", file_data_id, e.what()));
		return std::nullopt;
	}
}

static std::optional<std::string> get_mp4_url(const nlohmann::json& payload) {

	// Recursive polling: send request, if 202 wait and retry, if 200 return url.
	while (true) {
		auto [status, data] = kino_post(payload);

		if (status == 200) {
			if (data.contains("url") && data["url"].is_string())
				return data["url"].get<std::string>();
			return std::nullopt;
		} else if (status == 202) {
			// video queued, wait and retry
			std::this_thread::sleep_for(std::chrono::milliseconds(constants::KINO::POLL_INTERVAL));
			// return poll_for_url(); — loop continues
		} else {
			return std::nullopt;
		}
	}
}

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

	auto update_toast = [&]() {
		if (kino_processing_cancelled)
			return;

		const std::string msg = std::format("Processing videos: {}/{} ({} errors)", processed, total, errors);
		core::setToast("progress", msg, {}, -1, true);
	};

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

// --- Export methods ---

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

		std::string export_file_name = casc::ExportHelper::replaceExtension(file_name, ".mp4");
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

			auto mp4_response = generics::get(*mp4_url);
			if (!mp4_response.ok)
				throw std::runtime_error(std::format("Failed to download MP4 (HTTP {})", mp4_response.status));

			namespace fs = std::filesystem;
			fs::create_directories(fs::path(export_path).parent_path());

			std::ofstream out(export_path, std::ios::binary);
			out.write(reinterpret_cast<const char*>(mp4_response.body.data()), static_cast<std::streamsize>(mp4_response.body.size()));
			out.close();

			helper.mark(export_file_name, true);
		} catch (const std::exception& e) {
			helper.mark(export_file_name, false, e.what());
		}
	}

	helper.finish();
}

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
				BufferWrapper data = core::view->casc->getVirtualFileByName(file_name);
				data.writeToFile(export_path);

				helper.mark(export_file_name, true);
			} catch (const casc::BLTEIntegrityError&) {
				is_corrupted = true;
			} catch (const std::exception& e) {
				helper.mark(export_file_name, false, e.what());
			}

			if (is_corrupted) {
				try {
					logging::write("Local cinematic file is corrupted, forcing fallback.");

					// Note: C++ getVirtualFileByName doesn't support forceFallback; retry normally.
					BufferWrapper data = core::view->casc->getVirtualFileByName(file_name);
					data.writeToFile(export_path);

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

		std::string export_file_name = casc::ExportHelper::replaceExtension(file_name, ".mp3");
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
			BufferWrapper data = core::view->casc->getVirtualFileByID(movie_data->AudioFileDataID);
			data.writeToFile(export_path);
			helper.mark(export_file_name, true);
		} catch (const std::exception& e) {
			helper.mark(export_file_name, false, e.what());
		}
	}

	helper.finish();
}

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
		const std::string ext = (movie_data->SubtitleFileFormat == static_cast<int>(subtitles::SubtitleFormat::SBT)) ? ".sbt" : ".srt";

		std::string export_file_name = casc::ExportHelper::replaceExtension(file_name, ext);
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
			BufferWrapper data = core::view->casc->getVirtualFileByID(movie_data->SubtitleFileDataID);
			data.writeToFile(export_path);
			helper.mark(export_file_name, true);
		} catch (const std::exception& e) {
			helper.mark(export_file_name, false, e.what());
		}
	}

	helper.finish();
}

// --- Public API ---

void registerTab() {
	modules::register_nav_button("tab_videos", "Videos", "film.svg", install_type::CASC);
}

void mounted() {
	auto& view = *core::view;

	core::showLoadingScreen(1);
	core::progressLoadingScreen("Loading video metadata...");
	load_video_listfile();
	core::hideLoadingScreen();

	// Store initial config values for change-detection.
	prev_video_player_show_subtitles = view.config.value("videoPlayerShowSubtitles", false);
}

void render() {
	auto& view = *core::view;

	// --- Consume results from the background streaming thread (main-thread only) ---
	{
		std::lock_guard<std::mutex> lock(stream_result_mutex);
		if (pending_stream_result.has_value()) {
			auto result = std::move(*pending_stream_result);
			pending_stream_result.reset();

			if (result.success) {
				core::hideToast();
				play_streaming_video(result.video_url, result.subtitle);
			} else {
				is_streaming = false;
				core::view->videoPlayerState = false;
				core::setToast("error", result.error_message,
					{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
			}
		}
	}

	// --- Change-detection for selection (equivalent to watch on selectionVideos) ---
	if (!view.selectionVideos.empty()) {
		const std::string first = casc::listfile::stripFileEntry(view.selectionVideos[0].get<std::string>());
		if (view.isBusy == 0 && !first.empty() && selected_file != first) {
			// cancel any pending polls when selection changes
			poll_cancelled.store(true);
			poll_active = false;

			selected_file = first;

			if (view.config.value("videoPlayerAutoPlay", false))
				stream_video(first);

			prev_selection_first = first;
		}
	}

	// --- Change-detection for subtitle visibility ---
	const bool current_show_subtitles = view.config.value("videoPlayerShowSubtitles", false);
	if (current_show_subtitles != prev_video_player_show_subtitles) {
		//     current_subtitle_track.track.mode = show ? 'showing' : 'hidden';
		// Subtitle visibility is applied in the preview rendering below;
		// the config value is read directly when deciding whether to show subtitles.
		logging::write(std::format("subtitle visibility changed to: {}", current_show_subtitles ? "showing" : "hidden"));
		prev_video_player_show_subtitles = current_show_subtitles;
	}

	// --- Template rendering ---

	if (app::layout::BeginTab("tab-video")) {

	auto regions = app::layout::CalcListTabRegions(false);

	// --- Left panel: List container (row 1, col 1) ---
	//     <Listbox v-model:selection="selectionVideos" :items="listfileVideos" ...>
	//     <ContextMenu :node="contextMenus.nodeListbox" ...>
	if (app::layout::BeginListContainer("videos-list-container", regions)) {
		// Convert JSON items/selection to string vectors.
		const auto& items_str = core::cached_json_strings(view.listfileVideos, s_items_cache, s_items_cache_size);

		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionVideos)
			selection_str.push_back(s.get<std::string>());

		listbox::CopyMode copy_mode = listbox::CopyMode::Default;
		{
			std::string cm = view.config.value("copyMode", std::string("Default"));
			if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
			else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
		}

		listbox::render(
			"listbox-videos",
			items_str,
			view.userInputFilterVideos,
			selection_str,
			false,   // single
			true,    // keyinput
			view.config.value("regexFilters", false),
			copy_mode,
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"video", // unittype
			nullptr, // overrideItems
			false,   // disable
			"videos", // persistscrollkey
			{},      // quickfilters
			false,   // nocopy
			listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionVideos.clear();
				for (const auto& s : new_sel)
					view.selectionVideos.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				listbox_context::handle_context_menu(ev.selection);
			}
		);

		// Context menu for generic listbox.
		context_menu::render(
			"ctx-videos",
			view.contextMenus.nodeListbox,
			context_menu_state,
			[&]() { view.contextMenus.nodeListbox = nullptr; },
			[](const nlohmann::json& node) {
				std::vector<std::string> sel;
				if (node.contains("selection") && node["selection"].is_array())
					for (const auto& s : node["selection"])
						sel.push_back(s.get<std::string>());
				int count = node.value("count", 0);
				std::string plural = count > 1 ? "s" : "";
				bool hasFileDataIDs = node.value("hasFileDataIDs", false);

				if (ImGui::Selectable(std::format("Copy file path{}", plural).c_str()))
					listbox_context::copy_file_paths(sel);
				if (hasFileDataIDs && ImGui::Selectable(std::format("Copy file path{} (listfile format)", plural).c_str()))
					listbox_context::copy_listfile_format(sel);
				if (hasFileDataIDs && ImGui::Selectable(std::format("Copy file data ID{}", plural).c_str()))
					listbox_context::copy_file_data_ids(sel);
				if (ImGui::Selectable(std::format("Copy export path{}", plural).c_str()))
					listbox_context::copy_export_paths(sel);
				if (ImGui::Selectable("Open export directory"))
					listbox_context::open_export_directory(sel);
			}
		);
	}
	app::layout::EndListContainer();

	// --- Status bar ---
	if (app::layout::BeginStatusBar("videos-status", regions)) {
		listbox::renderStatusBar("video", {}, listbox_state);
	}
	app::layout::EndStatusBar();

	// --- Filter bar (row 2, col 1) ---
	//     <div class="regex-info" v-if="config.regexFilters" ...>Regex Enabled</div>
	//     <input type="text" v-model="userInputFilterVideos" placeholder="Filter videos..."/>
	// </div>
	if (app::layout::BeginFilterBar("videos-filter", regions)) {
		if (view.config.value("regexFilters", false)) {
			ImGui::TextUnformatted("Regex Enabled");
			ImGui::SameLine();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterVideos.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputText("##FilterVideos", filter_buf, sizeof(filter_buf)))
			view.userInputFilterVideos = filter_buf;
	}
	app::layout::EndFilterBar();

	// --- Right panel: Preview container (row 1, col 2) ---
	//     <video ref="video_player" class="preview-background" style="..." controls ...></video>
	// </div>
	if (app::layout::BeginPreviewContainer("video-preview-container", regions)) {
		// This preview area shows the current streaming state and subtitle text.
		if (is_streaming || view.videoPlayerState) {
			if (!current_video_url.empty()) {
				ImGui::TextUnformatted("Video opened in external player");
				ImGui::Spacing();
				ImGui::TextWrapped("URL: %s", current_video_url.c_str());

				// Show subtitle text overlay when enabled and available.
				if (has_subtitle_track && !current_subtitle_vtt.empty() &&
				    view.config.value("videoPlayerShowSubtitles", false)) {
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::TextUnformatted("Subtitles (VTT):");
					ImGui::TextWrapped("%s", current_subtitle_vtt.c_str());
				}

				ImGui::Spacing();
				if (ImGui::Button("Stop Video"))
					stop_video();
			} else if (poll_active) {
				ImGui::TextUnformatted("Video is being processed, please wait...");
			} else {
				ImGui::TextUnformatted("Connecting to video server...");
			}
		} else {
			ImGui::TextUnformatted("No video playing");
		}
	}
	app::layout::EndPreviewContainer();

	// --- Bottom-right: Preview controls / export (row 2, col 2) ---
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
	if (app::layout::BeginPreviewControls("videos-preview-controls", regions)) {
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
		if (busy) app::theme::BeginDisabledButton();
		if (ImGui::Button("Export Selected"))
			export_selected();
		if (busy) app::theme::EndDisabledButton();
	}
	app::layout::EndPreviewControls();

	} // if BeginTab
	app::layout::EndTab();
}

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
