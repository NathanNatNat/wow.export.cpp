/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_videos.h"
#include "../log.h"
#include "../core.h"
#include <thread>
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
#include "../components/menu-button.h"
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

#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace tab_videos {

static std::optional<std::string> build_stack_trace(const char* function_name, const std::exception& e) {
	return std::format("{}: {}", function_name, e.what());
}

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

static std::optional<std::unordered_map<uint32_t, uint32_t>> movie_variation_map;

static std::optional<std::vector<uint32_t>> video_file_data_ids;

static std::string selected_file;

static std::string current_video_url;

static bool has_subtitle_track = false;

static std::string current_subtitle_vtt;

static bool is_streaming = false;

static std::atomic<bool> poll_active{false};

static std::atomic<bool> poll_cancelled{false};
static std::atomic<size_t> poll_cancel_listener_id{0};

static std::atomic<bool> kino_processing_cancelled{false};
static std::atomic<bool> kino_processing_active{false};

static std::unique_ptr<std::jthread> stream_worker_thread;
static std::mutex stream_result_mutex;

static bool prev_video_player_show_subtitles = false;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;
static menu_button::MenuButtonState menu_button_videos_state;

static std::unique_ptr<std::jthread> kino_processing_thread;
static std::unique_ptr<std::jthread> export_selected_thread;

static mpv_handle* mpv_ctx = nullptr;
static mpv_render_context* mpv_render_ctx = nullptr;
static GLuint mpv_fbo = 0;
static GLuint mpv_texture = 0;
static int mpv_video_width = 0;
static int mpv_video_height = 0;
static bool mpv_initialized = false;

static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

struct SubtitleInfo {
	uint32_t file_data_id = 0;
	int format = 0;
};

struct StreamResult {
	bool success = false;
	std::string video_url;
	std::optional<SubtitleInfo> subtitle;
	std::string error_message;
};
static std::optional<StreamResult> pending_stream_result;

struct MovieData {
	uint32_t AudioFileDataID = 0;
	uint32_t SubtitleFileDataID = 0;
	int SubtitleFileFormat = 0;
};

struct BuildPayloadResult {
	nlohmann::json payload;
	std::optional<SubtitleInfo> subtitle;
};

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

static std::pair<int, nlohmann::json> kino_post(const nlohmann::json& payload) {
	const std::string api_url(constants::KINO::API_URL);
	std::string_view sv(constants::KINO::API_URL);
	sv.remove_prefix(8);
	const auto slash = sv.find('/');
	const std::string host(sv.substr(0, slash));
	const std::string path(sv.substr(slash));

	httplib::SSLClient cli(host);
	cli.set_connection_timeout(30);
	cli.set_read_timeout(60);
	cli.set_follow_location(true);

	httplib::Headers headers;
	headers.emplace("User-Agent", std::string(constants::USER_AGENT()));

	const std::string body = payload.dump();
	auto res = cli.Post(path, headers, body, "application/json");

	if (!res)
		throw std::runtime_error(std::format("HTTP request failed for {}: {}", api_url, httplib::to_string(res.error())));

	nlohmann::json response_json;
	if (!res->body.empty()) {
		try {
			response_json = nlohmann::json::parse(res->body);
		} catch (...) {
		}
	}

	if (res->status >= 400)
		logging::write(std::format("kino server returned {}: {}", res->status, res->body));

	return { res->status, response_json };
}

static void* mpv_get_proc_address(void* /*ctx*/, const char* name) {
	return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

static void ensure_mpv_initialized() {
	if (mpv_initialized)
		return;

	mpv_ctx = mpv_create();
	if (!mpv_ctx) {
		logging::write("failed to create mpv context");
		return;
	}

	mpv_set_option_string(mpv_ctx, "vo", "libmpv");
	mpv_set_option_string(mpv_ctx, "idle", "yes");
	mpv_set_option_string(mpv_ctx, "input-default-bindings", "no");
	mpv_set_option_string(mpv_ctx, "input-vo-keyboard", "no");
	mpv_set_option_string(mpv_ctx, "osc", "no");
	mpv_set_option_string(mpv_ctx, "keep-open", "yes");

	if (mpv_initialize(mpv_ctx) < 0) {
		logging::write("failed to initialize mpv");
		mpv_destroy(mpv_ctx);
		mpv_ctx = nullptr;
		return;
	}

	mpv_opengl_init_params gl_init_params{};
	gl_init_params.get_proc_address = mpv_get_proc_address;

	mpv_render_param params[] = {
		{MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
		{MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
		{MPV_RENDER_PARAM_INVALID, nullptr}
	};

	if (mpv_render_context_create(&mpv_render_ctx, mpv_ctx, params) < 0) {
		logging::write("failed to create mpv render context");
		mpv_terminate_destroy(mpv_ctx);
		mpv_ctx = nullptr;
		return;
	}

	glGenFramebuffers(1, &mpv_fbo);
	glGenTextures(1, &mpv_texture);

	mpv_initialized = true;
	logging::write("mpv video player initialized");
}

static void destroy_mpv() {
	if (mpv_render_ctx) {
		mpv_render_context_free(mpv_render_ctx);
		mpv_render_ctx = nullptr;
	}
	if (mpv_ctx) {
		mpv_terminate_destroy(mpv_ctx);
		mpv_ctx = nullptr;
	}
	if (mpv_fbo) {
		glDeleteFramebuffers(1, &mpv_fbo);
		mpv_fbo = 0;
	}
	if (mpv_texture) {
		glDeleteTextures(1, &mpv_texture);
		mpv_texture = 0;
	}
	mpv_video_width = 0;
	mpv_video_height = 0;
	mpv_initialized = false;
}

static void stop_video() {
	poll_cancelled.store(true);

	poll_active = false;

	if (stream_worker_thread) {
		stream_worker_thread->request_stop();
		if (stream_worker_thread->joinable())
			stream_worker_thread->detach();
		stream_worker_thread.reset();
	}

	if (mpv_ctx) {
		const char* cmd[] = {"stop", nullptr};
		mpv_command(mpv_ctx, cmd);
	}

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
	current_video_url = url;

	ensure_mpv_initialized();
	if (!mpv_ctx) {
		logging::write("mpv not available, cannot play video inline");
		core::setToast("error", "Video player not available");
		is_streaming = false;
		core::view->videoPlayerState = false;
		return;
	}

	const char* cmd[] = {"loadfile", url.c_str(), nullptr};
	mpv_command(mpv_ctx, cmd);
	logging::write(std::format("playing video inline: {}", url));

	// load subtitles if available
	if (subtitle_info.has_value()) {
		try {
			subtitles::SubtitleFormat fmt = static_cast<subtitles::SubtitleFormat>(subtitle_info->format);
			current_subtitle_vtt = subtitles::get_subtitles_vtt(core::view->casc, subtitle_info->file_data_id, fmt);
			has_subtitle_track = true;

			// write VTT to temp file for mpv to load
			auto temp_path = std::filesystem::temp_directory_path() / "wow_export_subs.vtt";
			{
				std::ofstream ofs(temp_path, std::ios::binary);
				ofs << current_subtitle_vtt;
			}

			const char* sub_cmd[] = {"sub-add", temp_path.string().c_str(), "select", nullptr};
			mpv_command(mpv_ctx, sub_cmd);

			bool show_subs = core::view->config.value("videoPlayerShowSubtitles", true);
			int vis = show_subs ? 1 : 0;
			mpv_set_property(mpv_ctx, "sub-visibility", MPV_FORMAT_FLAG, &vis);

			logging::write(std::format("loaded subtitles for video (fdid: {}, format: {})",
				subtitle_info->file_data_id, subtitle_info->format));
		} catch (const std::exception& e) {
			logging::write(std::format("failed to load subtitles: {}", e.what()));
		}
	}
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

	if (stream_worker_thread) {
		stream_worker_thread->request_stop();
		if (stream_worker_thread->joinable())
			stream_worker_thread->detach();
		stream_worker_thread.reset();
	}

	stream_worker_thread = std::make_unique<std::jthread>([file_data_id,
	                                                        file_name]() {
		const auto build_result = build_payload(file_data_id);
		if (!build_result.has_value()) {
			core::postToMainThread([]() {
				core::setToast("error", "Failed to get video encoding info");
				is_streaming = false;
				core::view->videoPlayerState = false;
			});
			return;
		}

		nlohmann::json payload = build_result->payload;
		std::optional<SubtitleInfo> subtitle = build_result->subtitle;
		logging::write(std::format("sending kino request: {}", payload.dump()));
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

				core::postToMainThread([]() {
					if (!poll_cancelled.load()) {
						core::setToast("progress", "Video is being processed, please wait...", {}, -1, true);
						poll_cancel_listener_id.store(
							core::events.once("toast-cancelled", []() {
								poll_cancelled.store(true);
								poll_active.store(false);
								is_streaming = false;
								core::view->videoPlayerState = false;
								logging::write("video processing cancelled by user");
							})
						);
					}
				});

				bool poll_done = false;
				while (!poll_done && !poll_cancelled.load()) {
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
				if (size_t lid = poll_cancel_listener_id.exchange(0); lid != 0) {
					core::postToMainThread([lid]() {
						core::events.off("toast-cancelled", lid);
					});
				}
			} else {
				throw std::runtime_error(std::format("server returned {}", status));
			}
		} catch (const std::exception& e) {
			logging::write(std::format("failed to stream video {}: {}", file_name, e.what()));
			if (const auto trace = build_stack_trace("stream_video", e)) logging::write(*trace);
			std::lock_guard<std::mutex> lock(stream_result_mutex);
			pending_stream_result = StreamResult{false, {}, {}, std::string("Failed to stream video: ") + e.what()};
		}
	});
}

static std::vector<std::string> build_video_entries(bool sort_by_id) {
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

	if (sort_by_id) {
		std::sort(entries.begin(), entries.end(), [](const std::string& a, const std::string& b) {
			const auto fa = casc::listfile::getByFilename(casc::listfile::stripFileEntry(a));
			const auto fb = casc::listfile::getByFilename(casc::listfile::stripFileEntry(b));
			return fa.value_or(0) < fb.value_or(0);
		});
	} else {
		std::sort(entries.begin(), entries.end());
	}

	logging::write(std::format("built video listfile with {} entries", entries.size()));
	return entries;
}

static void load_video_listfile() {
	auto entries = build_video_entries(core::view->config.value("listfileSortByID", false));
	core::view->listfileVideos.clear();
	core::view->listfileVideos.reserve(entries.size());
	for (auto& e : entries)
		core::view->listfileVideos.push_back(std::move(e));
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

	while (true) {
		auto [status, data] = kino_post(payload);

		if (status == 200) {
			if (data.contains("url") && data["url"].is_string())
				return data["url"].get<std::string>();
			return std::nullopt;
		} else if (status == 202) {
			// video queued, wait and retry
			std::this_thread::sleep_for(std::chrono::milliseconds(constants::KINO::POLL_INTERVAL));
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

	if (kino_processing_active.load())
		return;

	kino_processing_cancelled.store(false);
	kino_processing_active.store(true);

	logging::write(std::format("kino_processing: starting processing of {} videos", video_file_data_ids->size()));

	kino_processing_thread.reset();
	kino_processing_thread = std::make_unique<std::jthread>([fdids = *video_file_data_ids]() {
		const size_t total = fdids.size();
		std::atomic<size_t> processed{0};
		std::atomic<size_t> errors{0};

		auto update_toast = [&processed, &errors, total]() {
			if (kino_processing_cancelled.load())
				return;

			const size_t p = processed.load();
			const size_t e = errors.load();
			core::postToMainThread([p, e, total]() {
				if (kino_processing_cancelled.load())
					return;
				const std::string msg = std::format("Processing videos: {}/{} ({} errors)", p, total, e);
				core::setToast("progress", msg, { {"Cancel", [p, total]() {
					kino_processing_cancelled.store(true);
					logging::write(std::format("kino_processing: cancelled by user at {}/{}", p, total));
					core::setToast("info", std::format("Video processing cancelled. Processed {}/{} videos.", p, total));
				}} }, -1, true);
			});
		};

		size_t cancel_listener_id = core::events.once("toast-cancelled", []() {
			kino_processing_cancelled.store(true);
			logging::write("kino_processing: cancelled by user (toast-cancelled)");
		});

		update_toast();

		for (const uint32_t file_data_id : fdids) {
			if (kino_processing_cancelled.load())
				break;

			try {
				const auto build_result = build_payload(file_data_id);
				if (!build_result.has_value()) {
					logging::write(std::format("kino_processing: failed to build payload for fdid {}", file_data_id));
					errors.fetch_add(1);
					processed.fetch_add(1);
					update_toast();
					continue;
				}

				const auto& payload = build_result->payload;

				// poll until we get 200 or error
				bool done = false;
				while (!done && !kino_processing_cancelled.load()) {
					auto [status, data] = kino_post(payload);

					if (status == 200) {
						done = true;
					} else if (status == 202) {
						std::this_thread::sleep_for(std::chrono::milliseconds(constants::KINO::POLL_INTERVAL));
					} else {
						logging::write(std::format("kino_processing: unexpected status {} for fdid {}", status, file_data_id));
						errors.fetch_add(1);
						done = true;
					}
				}
			} catch (const std::exception& e) {
				logging::write(std::format("kino_processing: error processing fdid {}: {}", file_data_id, e.what()));
				errors.fetch_add(1);
			}

			processed.fetch_add(1);
			update_toast();
		}

		core::events.off("toast-cancelled", cancel_listener_id);

		const size_t final_processed = processed.load();
		const size_t final_errors = errors.load();
		const bool was_cancelled = kino_processing_cancelled.load();

		core::postToMainThread([final_processed, total, final_errors, was_cancelled]() {
			if (was_cancelled) {
				core::setToast("info", std::format("Video processing cancelled. Processed {}/{} videos.", final_processed, total));
			} else {
				logging::write(std::format("kino_processing: completed {}/{} videos with {} errors", final_processed, total, final_errors));
				core::setToast("success", std::format("Video processing complete. {}/{} videos, {} errors.", final_processed, total, final_errors));
			}
		});

		kino_processing_active.store(false);
	});
}

// expose to window in dev mode

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
			helper.mark(export_file_name, false, e.what(), build_stack_trace("export_mp4", e));
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
				helper.mark(export_file_name, false, e.what(), build_stack_trace("export_avi", e));
			}

			if (is_corrupted) {
				try {
					logging::write("Local cinematic file is corrupted, forcing fallback.");

					casc::BLTEReader fallback_data = core::view->casc->getFileByName(file_name, false, false, true, true);
					fallback_data.writeToFile(export_path);

					helper.mark(export_file_name, true);
				} catch (const std::exception& e) {
					helper.mark(export_file_name, false, e.what(), build_stack_trace("export_avi", e));
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
			helper.mark(export_file_name, false, e.what(), build_stack_trace("export_mp3", e));
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
			helper.mark(export_file_name, false, e.what(), build_stack_trace("export_subtitles", e));
		}
	}

	helper.finish();
}

void registerTab() {
	modules::register_nav_button("tab_videos", "Videos", "film.svg", install_type::CASC);
}

void mounted() {
	auto& view = *core::view;

	prev_video_player_show_subtitles = view.config.value("videoPlayerShowSubtitles", true);
	const bool sort_by_id = view.config.value("listfileSortByID", false);

	view.isLoading = true;
	view.loadPct = 0;
	view.loadingProgress = "Loading video metadata...";
	view.isBusy++;

	std::thread([sort_by_id]() {
		auto entries = build_video_entries(sort_by_id);

		core::postToMainThread([entries = std::move(entries)]() mutable {
			core::view->listfileVideos.clear();
			core::view->listfileVideos.reserve(entries.size());
			for (auto& e : entries)
				core::view->listfileVideos.push_back(std::move(e));

			core::view->loadPct = -1;
			core::view->isLoading = false;
			core::view->isBusy--;
		});
	}).detach();
}

void render() {
	auto& view = *core::view;

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
					{ {"view log", []() { logging::openRuntimeLog(); }} }, -1);
			}
		}
	}

	if (!view.selectionVideos.empty()) {
		const std::string first = casc::listfile::stripFileEntry(view.selectionVideos[0].get<std::string>());
		if (view.isBusy == 0 && !first.empty() && selected_file != first) {
			// cancel any pending polls when selection changes
			poll_cancelled.store(true);
			poll_active = false;

			selected_file = first;

			if (view.config.value("videoPlayerAutoPlay", true))
				stream_video(first);
		}
	}

	const bool current_show_subtitles = view.config.value("videoPlayerShowSubtitles", true);
	if (current_show_subtitles != prev_video_player_show_subtitles) {
		logging::write(std::format("subtitle visibility changed to: {}", current_show_subtitles ? "showing" : "hidden"));
		prev_video_player_show_subtitles = current_show_subtitles;
		if (mpv_ctx) {
			int vis = current_show_subtitles ? 1 : 0;
			mpv_set_property(mpv_ctx, "sub-visibility", MPV_FORMAT_FLAG, &vis);
		}
	}

	if (mpv_ctx) {
		while (true) {
			mpv_event* event = mpv_wait_event(mpv_ctx, 0);
			if (event->event_id == MPV_EVENT_NONE)
				break;
			if (event->event_id == MPV_EVENT_END_FILE) {
				auto* end = static_cast<mpv_event_end_file*>(event->data);
				if (end->reason == MPV_END_FILE_REASON_EOF) {
					logging::write("video playback complete");
					is_streaming = false;
					view.videoPlayerState = false;
				} else if (end->reason == MPV_END_FILE_REASON_ERROR) {
					logging::write(std::format("video playback error: {}", mpv_error_string(end->error)));
					core::setToast("error", "Video playback error");
					is_streaming = false;
					view.videoPlayerState = false;
				}
			}
		}
	}

	if (app::layout::BeginTab("tab-video")) {

	auto regions = app::layout::CalcListTabRegions(false);

	if (app::layout::BeginListContainer("videos-list-container", regions)) {
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
			false,
			true,
			view.config.value("regexFilters", false),
			copy_mode,
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"video",
			nullptr,
			false,
			"videos",
			{},
			false,
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

	if (app::layout::BeginStatusBar("videos-status", regions)) {
		listbox::renderStatusBar("video", {}, listbox_state);
	}
	app::layout::EndStatusBar();

	if (app::layout::BeginFilterBar("videos-filter", regions)) {
		if (view.config.value("regexFilters", false)) {
			ImGui::TextUnformatted("Regex Enabled");
			if (ImGui::IsItemHovered() && !view.regexTooltip.empty())
				ImGui::SetTooltip("%s", view.regexTooltip.c_str());
			ImGui::SameLine();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_buf[4096] = {};
		std::strncpy(filter_buf, view.userInputFilterVideos.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputTextWithHint("##FilterVideos", "Filter videos...", filter_buf, sizeof(filter_buf)))
			view.userInputFilterVideos = filter_buf;
	}
	app::layout::EndFilterBar();

	if (app::layout::BeginPreviewContainer("video-preview-container", regions)) {
		if (is_streaming || view.videoPlayerState) {
			if (!current_video_url.empty() && mpv_render_ctx) {
				const float controls_height = 28.0f;
				ImVec2 avail = ImGui::GetContentRegionAvail();
				int render_w = static_cast<int>(avail.x);
				int render_h = static_cast<int>(avail.y - controls_height);
				if (render_w < 1) render_w = 1;
				if (render_h < 1) render_h = 1;

				if (render_w != mpv_video_width || render_h != mpv_video_height) {
					mpv_video_width = render_w;
					mpv_video_height = render_h;
					glBindTexture(GL_TEXTURE_2D, mpv_texture);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, render_w, render_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glBindFramebuffer(GL_FRAMEBUFFER, mpv_fbo);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mpv_texture, 0);
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
				}

				if (mpv_render_context_update(mpv_render_ctx) & MPV_RENDER_UPDATE_FRAME) {
					mpv_opengl_fbo fbo_params{};
					fbo_params.fbo = static_cast<int>(mpv_fbo);
					fbo_params.w = render_w;
					fbo_params.h = render_h;
					int flip_y = 1;

					mpv_render_param render_params[] = {
						{MPV_RENDER_PARAM_OPENGL_FBO, &fbo_params},
						{MPV_RENDER_PARAM_FLIP_Y, &flip_y},
						{MPV_RENDER_PARAM_INVALID, nullptr}
					};

					mpv_render_context_render(mpv_render_ctx, render_params);
				}

				ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(mpv_texture)),
					ImVec2(static_cast<float>(render_w), static_cast<float>(render_h)),
					ImVec2(0, 1), ImVec2(1, 0));

				double time_pos = 0.0, duration = 0.0;
				int paused = 0;
				double volume = 100.0;
				mpv_get_property(mpv_ctx, "time-pos", MPV_FORMAT_DOUBLE, &time_pos);
				mpv_get_property(mpv_ctx, "duration", MPV_FORMAT_DOUBLE, &duration);
				mpv_get_property(mpv_ctx, "pause", MPV_FORMAT_FLAG, &paused);
				mpv_get_property(mpv_ctx, "volume", MPV_FORMAT_DOUBLE, &volume);

				if (ImGui::SmallButton(paused ? ">" : "||")) {
					int toggle = !paused;
					mpv_set_property(mpv_ctx, "pause", MPV_FORMAT_FLAG, &toggle);
				}

				ImGui::SameLine();
				int cur_min = static_cast<int>(time_pos) / 60;
				int cur_sec = static_cast<int>(time_pos) % 60;
				int dur_min = static_cast<int>(duration) / 60;
				int dur_sec = static_cast<int>(duration) % 60;
				ImGui::Text("%d:%02d / %d:%02d", cur_min, cur_sec, dur_min, dur_sec);

				ImGui::SameLine();
				float seek_pos = (duration > 0) ? static_cast<float>(time_pos / duration) : 0.0f;
				float seek_width = ImGui::GetContentRegionAvail().x - 120.0f;
				if (seek_width < 50.0f) seek_width = 50.0f;
				ImGui::SetNextItemWidth(seek_width);
				if (ImGui::SliderFloat("##seek", &seek_pos, 0.0f, 1.0f, "")) {
					double new_pos = seek_pos * duration;
					mpv_set_property(mpv_ctx, "time-pos", MPV_FORMAT_DOUBLE, &new_pos);
				}

				ImGui::SameLine();
				float vol = static_cast<float>(volume);
				ImGui::SetNextItemWidth(60.0f);
				if (ImGui::SliderFloat("##vol", &vol, 0.0f, 100.0f, "%.0f%%")) {
					double new_vol = static_cast<double>(vol);
					mpv_set_property(mpv_ctx, "volume", MPV_FORMAT_DOUBLE, &new_vol);
				}

				ImGui::SameLine();
				if (ImGui::SmallButton("Stop"))
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

	if (app::layout::BeginPreviewControls("videos-preview-controls", regions)) {
		bool autoplay_val = view.config.value("videoPlayerAutoPlay", true);
		if (ImGui::Checkbox("Autoplay", &autoplay_val))
			view.config["videoPlayerAutoPlay"] = autoplay_val;

		ImGui::SameLine();

		bool show_subs = view.config.value("videoPlayerShowSubtitles", true);
		if (ImGui::Checkbox("Show Subtitles", &show_subs))
			view.config["videoPlayerShowSubtitles"] = show_subs;

		ImGui::SameLine();

		ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 200.0f, 0));
		ImGui::SameLine();

		{
			const bool busy = view.isBusy > 0;
			std::vector<menu_button::MenuOption> mb_options;
			for (const auto& opt : view.menuButtonVideos)
				mb_options.push_back({ opt.label, opt.value });
			menu_button::render("##MenuButtonVideos", mb_options,
				view.config.value("exportVideoFormat", std::string("MP4")),
				busy, false, menu_button_videos_state,
				[&](const std::string& val) { view.config["exportVideoFormat"] = val; },
				[]() { export_selected(); });
		}
	}
	app::layout::EndPreviewControls();

	}
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

	export_selected_thread.reset();
	export_selected_thread = std::make_unique<std::jthread>([format]() {
		if (format == "MP4") {
			export_mp4();
		} else if (format == "AVI") {
			export_avi();
		} else if (format == "MP3") {
			export_mp3();
		} else if (format == "SUBTITLES") {
			export_subtitles();
		}
	});
}

}
