/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_audio.h"
#include "../log.h"
#include "../core.h"
#include <thread>
#include "../generics.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
#include "../casc/db2.h"
#include "../db/WDCReader.h"
#include "../ui/audio-helper.h"
#include "../ui/listbox-context.h"
#include "../constants.h"
#include "../install-type.h"
#include "../modules.h"
#include "../components/listbox.h"
#include "../components/context-menu.h"
#include "../../app.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <format>
#include <filesystem>
#include <future>
#include <optional>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_audio {

// --- File-local state ---

static std::string selected_file;

static std::optional<uint32_t> selected_file_data_id;

static bool seek_loop_active = false;


static AudioPlayer player;

// Change-detection for config watches and selection.
static float prev_sound_player_volume = -1.0f;
static bool prev_sound_player_loop = false;
static std::string prev_selection_first;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

// Cached items string vector — only rebuilt when the source JSON changes.
static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

// CSS: #sound-player-anim — 14-frame horizontal sprite strip, 309×397px per frame, 0.7s cycle.
static GLuint s_audiobox_tex = 0;
static constexpr int AUDIOBOX_FRAMES = 14;
static constexpr float AUDIOBOX_FRAME_W = 309.0f;
static constexpr float AUDIOBOX_FRAME_H = 397.0f;
static constexpr float AUDIOBOX_ANIM_DURATION = 0.7f;

// --- Internal functions ---

static void update_seek() {
	if (!player.is_playing) {
		seek_loop_active = false;
		return;
	}

	const double duration = player.get_duration();
	if (duration > 0)
		core::view->soundPlayerSeek = player.get_position() / duration;

	// In ImGui, the seek update happens every frame via render(), not via requestAnimationFrame.
	// seek_loop_active remains true to keep updating.
}

static void start_seek_loop() {
	if (!seek_loop_active) {
		seek_loop_active = true;
		update_seek();
	}
}

static void stop_seek_loop() {
	seek_loop_active = false;
}

// --- Async audio loading (follows tab_models pattern) ---

struct PendingAudioLoad {
	std::string file_name;
	std::optional<uint32_t> file_data_id;
	std::future<BufferWrapper> file_future;
	std::unique_ptr<BusyLock> busy_lock;
	bool auto_play = false;
};

static std::optional<PendingAudioLoad> pending_audio_load;

static bool load_track() {
	if (selected_file.empty())
		return false;

	// Cancel any pending async load.
	pending_audio_load.reset();

	core::setToast("progress", std::format("Loading {}, please wait...", selected_file), {}, -1, false);
	logging::write(std::format("Previewing sound file {}", selected_file));

	auto* casc = core::view->casc;
	if (!casc) {
		core::setToast("error", "CASC source is not available.", {}, -1);
		return false;
	}

	PendingAudioLoad task;
	task.file_name = selected_file;
	task.file_data_id = selected_file_data_id;
	task.busy_lock = std::make_unique<BusyLock>(core::create_busy_lock());
	task.auto_play = false;

	if (selected_file_data_id.has_value()) {
		uint32_t fdid = selected_file_data_id.value();
		task.file_future = std::async(std::launch::async, [casc, fdid]() {
			return casc->getVirtualFileByID(fdid);
		});
	} else {
		std::string fname = selected_file;
		task.file_future = std::async(std::launch::async, [casc, fname]() {
			return casc->getVirtualFileByName(fname);
		});
	}

	pending_audio_load = std::move(task);
	return false; // Not loaded yet; pump will handle it.
}

static void pump_audio_load() {
	if (!pending_audio_load.has_value())
		return;

	auto& task = *pending_audio_load;
	if (task.file_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	bool should_play = task.auto_play;

	try {
		BufferWrapper file_data_buf = task.file_future.get();

		if (task.file_name.ends_with(".unk_sound")) {
			const AudioType file_type = detectFileType(file_data_buf);
			if (file_type == AudioType::OGG)
				core::view->soundPlayerTitle += " (OGG Auto Detected)";
			else if (file_type == AudioType::MP3)
				core::view->soundPlayerTitle += " (MP3 Auto Detected)";
		}

		player.stop();
		player.load(file_data_buf.raw());
		core::view->soundPlayerDuration = player.get_duration();
		core::hideToast();

		// Auto-play if requested (from play_track).
		if (should_play) {
			player.play();
			core::view->soundPlayerState = true;
			start_seek_loop();
		}
	} catch (const casc::EncryptionError& e) {
		core::setToast("error", std::format("The audio file {} is encrypted with an unknown key ({}).", task.file_name, e.key), {}, -1);
		logging::write(std::format("Failed to decrypt audio file {} ({})", task.file_name, e.key));
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to preview audio " + task.file_name,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}

	pending_audio_load.reset();
}

static void unload_track() {
	pending_audio_load.reset();
	stop_seek_loop();
	player.unload();

	core::view->soundPlayerState = false;
	core::view->soundPlayerDuration = 0;
	core::view->soundPlayerSeek = 0;

}

static void play_track() {
	if (!player.is_loaded()) {
		if (selected_file.empty()) {
			core::setToast("info", "You need to select an audio track first!", {}, -1, true);
			return;
		}

		// Launch async load with auto-play.
		load_track();
		if (pending_audio_load.has_value())
			pending_audio_load->auto_play = true;
		return;
	}

	player.play();
	core::view->soundPlayerState = true;
	start_seek_loop();
}

static void pause_track() {
	player.pause();
	stop_seek_loop();
	core::view->soundPlayerState = false;
}

// --- Async export (one-file-per-frame, follows tab_models pattern) ---

struct PendingAudioExport {
	std::vector<nlohmann::json> files;
	size_t next_index = 0;
	bool overwrite_files = false;
	std::optional<casc::ExportHelper> helper;
};

static std::optional<PendingAudioExport> pending_audio_export;

static void pump_audio_export() {
	if (!pending_audio_export.has_value())
		return;

	auto& task = *pending_audio_export;
	auto& helper = task.helper.value();

	if (task.next_index == 0)
		helper.start();

	if (helper.isCancelled()) {
		pending_audio_export.reset();
		return;
	}

	if (task.next_index >= task.files.size()) {
		helper.finish();
		pending_audio_export.reset();
		return;
	}

	// Process one file per frame.
	const auto& sel_entry = task.files[task.next_index++];
	bool has_export_data = false;
	std::vector<uint8_t> export_data;

	std::string file_name = casc::listfile::stripFileEntry(sel_entry.get<std::string>());

	if (file_name.ends_with(".unk_sound")) {
		BufferWrapper export_buf = core::view->casc->getVirtualFileByName(file_name);
		export_data = std::move(export_buf.raw());
		has_export_data = true;

		const AudioType file_type = detectFileType(BufferWrapper(export_data));

		if (file_type == AudioType::OGG)
			file_name = casc::ExportHelper::replaceExtension(file_name, ".ogg");
		else if (file_type == AudioType::MP3)
			file_name = casc::ExportHelper::replaceExtension(file_name, ".mp3");
	}

	std::string export_file_name = file_name;
	auto& view = *core::view;

	if (!view.config.value("exportNamedFiles", true)) {
		auto file_data_id = casc::listfile::getByFilename(file_name);
		if (file_data_id) {
			namespace fs = std::filesystem;
			const std::string ext = fs::path(file_name).extension().string();
			const std::string dir = fs::path(file_name).parent_path().string();
			const std::string file_data_id_name = std::to_string(*file_data_id) + ext;
			export_file_name = dir == "." ? file_data_id_name : (fs::path(dir) / file_data_id_name).string();
		}
	}

	try {
		const std::string export_path = casc::ExportHelper::getExportPath(export_file_name);
		if (task.overwrite_files || !generics::fileExists(export_path)) {
			if (!has_export_data) {
				BufferWrapper export_buf = core::view->casc->getVirtualFileByName(file_name);
				export_data = std::move(export_buf.raw());
			}

			BufferWrapper out_buf(std::move(export_data));
			out_buf.writeToFile(export_path);
		} else {
			logging::write(std::format("Skipping audio export {} (file exists, overwrite disabled)", export_path));
		}

		helper.mark(export_file_name, true);
	} catch (const std::exception& e) {
		helper.mark(export_file_name, false, e.what());
	}
}

static void export_sounds() {
	if (pending_audio_export.has_value())
		return;

	auto& view = *core::view;
	const auto& user_selection = view.selectionSounds;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	PendingAudioExport task;
	task.files = std::vector<nlohmann::json>(user_selection.begin(), user_selection.end());
	task.overwrite_files = view.config.value("overwriteFiles", false);
	task.helper.emplace(static_cast<int>(user_selection.size()), "sound files");

	pending_audio_export = std::move(task);
}

// --- Helper: format seconds as MM:SS ---
static std::string format_time(double seconds) {
	if (seconds <= 0)
		return "00:00";

	const int total_seconds = static_cast<int>(seconds);
	const int minutes = total_seconds / 60;
	const int secs = total_seconds % 60;
	return std::format("{:02d}:{:02d}", minutes, secs);
}

// --- Public API ---

void registerTab() {
	modules::register_nav_button("tab_audio", "Audio", "music.svg", install_type::CASC);

	// Load the audiobox sprite sheet on first registration.
	if (!s_audiobox_tex)
		s_audiobox_tex = app::theme::loadImageTexture(constants::SRC_DIR() / "images" / "audiobox.png");
}

void mounted() {
	auto& view = *core::view;

	player.init();
	const float initial_volume = view.config.value("soundPlayerVolume", 1.0f);
	player.set_volume(initial_volume);
	prev_sound_player_volume = initial_volume;
	const bool initial_loop = view.config.value("soundPlayerLoop", false);
	player.set_loop(initial_loop);
	prev_sound_player_loop = initial_loop;

	player.on_ended = []() {
		stop_seek_loop();
		core::view->soundPlayerState = false;
		core::view->soundPlayerSeek = 0;
	};

	core::events.on("crash", []() {
		unload_track();
		player.destroy();
	});

	if (view.config.value("enableUnknownFiles", false)) {
		std::thread([]() {
			core::showLoadingScreen(1);
			core::progressLoadingScreen("Processing unknown sound files...");

			int unknown_count = 0;
			auto& sound_kit_entry = casc::db2::getTable("SoundKitEntry");
			if (!sound_kit_entry.isLoaded)
				sound_kit_entry.parse();

			std::vector<std::string> new_entries;
			for (const auto& [id, row] : sound_kit_entry.getAllRows()) {
				auto it = row.find("FileDataID");
				if (it == row.end())
					continue;

				uint32_t file_data_id = 0;
				if (auto* p = std::get_if<int64_t>(&it->second))
					file_data_id = static_cast<uint32_t>(*p);
				else if (auto* p = std::get_if<uint64_t>(&it->second))
					file_data_id = static_cast<uint32_t>(*p);

				if (file_data_id == 0)
					continue;

				if (!casc::listfile::existsByID(file_data_id)) {
					const std::string file_name = "unknown/" + std::to_string(file_data_id) + ".unk_sound";
					std::vector<std::string> listfile_vec;
					casc::listfile::addEntry(file_data_id, file_name, &listfile_vec);
					for (auto& s : listfile_vec)
						new_entries.push_back(std::move(s));
					unknown_count++;
				}
			}

			logging::write(std::format("Added {} unknown sound files from SoundKitEntry to listfile", unknown_count));

			core::postToMainThread([entries = std::move(new_entries)]() mutable {
				for (auto& s : entries)
					core::view->listfileSounds.push_back(std::move(s));
			});

			core::hideLoadingScreen();
		}).detach();
	}
}

void render() {
	auto& view = *core::view;

	// Poll for pending async audio load completion.
	pump_audio_load();
	// Poll for pending async audio export (one file per frame).
	pump_audio_export();

	// --- Change-detection for config watches ---

	const float current_volume = view.config.value("soundPlayerVolume", 1.0f);
	if (current_volume != prev_sound_player_volume) {
		player.set_volume(current_volume);
		prev_sound_player_volume = current_volume;
	}

	const bool current_loop = view.config.value("soundPlayerLoop", false);
	if (current_loop != prev_sound_player_loop) {
		player.set_loop(current_loop);
		prev_sound_player_loop = current_loop;
	}

	// --- Change-detection for selection (equivalent to watch on selectionSounds) ---
	if (!view.selectionSounds.empty()) {
		const auto entry = casc::listfile::parseFileEntry(view.selectionSounds[0].get<std::string>());
		if (view.isBusy == 0 && !entry.file_path.empty() && entry.file_path != selected_file) {
			namespace fs = std::filesystem;
			view.soundPlayerTitle = fs::path(entry.file_path).filename().string();

			selected_file = entry.file_path;
			selected_file_data_id = entry.file_data_id;
			unload_track();

			if (view.config.value("soundPlayerAutoPlay", false))
				play_track();

			prev_selection_first = entry.file_path;
		}
	}

	// --- Seek loop update (runs every frame while playing) ---
	if (seek_loop_active)
		update_seek();

	// --- Template rendering ---

	if (app::layout::BeginTab("tab-audio")) {

	auto regions = app::layout::CalcListTabRegions(false);

	// --- Left panel: List container (row 1, col 1) ---
	//     <Listbox v-model:selection="selectionSounds" :items="listfileSounds" ...>
	//     <ContextMenu :node="contextMenus.nodeListbox" ...>
	if (app::layout::BeginListContainer("sounds-list-container", regions)) {
		// Convert JSON items/selection to string vectors.
		const auto& items_str = core::cached_json_strings(view.listfileSounds, s_items_cache, s_items_cache_size);

		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionSounds)
			selection_str.push_back(s.get<std::string>());

		listbox::CopyMode copy_mode = listbox::CopyMode::Default;
		{
			std::string cm = view.config.value("copyMode", std::string("Default"));
			if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
			else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
		}

		listbox::render(
			"listbox-sounds",
			items_str,
			view.userInputFilterSounds,
			selection_str,
			false,   // single
			true,    // keyinput
			view.config.value("regexFilters", false),
			copy_mode,
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"sound file", // unittype — JS: unittype="sound file"
			nullptr, // overrideItems
			false,   // disable
			"sounds", // persistscrollkey
			view.audioQuickFilters, // quickfilters — JS: :quickfilters="audioQuickFilters"
			false,   // nocopy
			listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionSounds.clear();
				for (const auto& s : new_sel)
					view.selectionSounds.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				listbox_context::handle_context_menu(ev.selection);
			}
		);

		// Context menu for generic listbox.
		context_menu::render(
			"ctx-sounds",
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
	if (app::layout::BeginStatusBar("sounds-status", regions)) {
		listbox::renderStatusBar("sound file", view.audioQuickFilters, listbox_state);
	}
	app::layout::EndStatusBar();

	// --- Filter bar (row 2, col 1) ---
	if (app::layout::BeginFilterBar("sounds-filter", regions)) {
		if (view.config.value("regexFilters", false))
			ImGui::TextUnformatted("Regex Enabled");

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterSounds.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputText("##FilterSounds", filter_buf, sizeof(filter_buf)))
			view.userInputFilterSounds = filter_buf;
	}
	app::layout::EndFilterBar();

	// --- Right panel: Preview container (row 1, col 2) ---
	if (app::layout::BeginPreviewContainer("sounds-preview-container", regions)) {

		// CSS: #sound-player-anim — audiobox sprite strip animated at 14 fps / 0.7s cycle.
		// Plays when soundPlayerState is true; shows frame 0 when paused.
		if (s_audiobox_tex) {
			int frame = 0;
			if (view.soundPlayerState) {
				float t = std::fmod(static_cast<float>(ImGui::GetTime()), AUDIOBOX_ANIM_DURATION);
				frame = static_cast<int>(t / AUDIOBOX_ANIM_DURATION * AUDIOBOX_FRAMES) % AUDIOBOX_FRAMES;
			}
			float uv_x0 = frame / static_cast<float>(AUDIOBOX_FRAMES);
			float uv_x1 = (frame + 1) / static_cast<float>(AUDIOBOX_FRAMES);

			float avail_w = ImGui::GetContentRegionAvail().x;
			float disp_w = std::min(AUDIOBOX_FRAME_W, avail_w);
			float disp_h = disp_w * (AUDIOBOX_FRAME_H / AUDIOBOX_FRAME_W);
			float off_x = (avail_w - disp_w) * 0.5f;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off_x);
			ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(s_audiobox_tex)),
				ImVec2(disp_w, disp_h),
				ImVec2(uv_x0, 0.0f), ImVec2(uv_x1, 1.0f));
		}

		ImGui::Spacing();

		const std::string seek_formatted = format_time(view.soundPlayerSeek * view.soundPlayerDuration);
		const std::string duration_formatted = format_time(view.soundPlayerDuration);
		ImGui::Text("%s  %s  %s", seek_formatted.c_str(), view.soundPlayerTitle.c_str(), duration_formatted.c_str());

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		float seek_val = static_cast<float>(view.soundPlayerSeek);
		if (ImGui::SliderFloat("##SeekSlider", &seek_val, 0.0f, 1.0f, "")) {
			view.soundPlayerSeek = seek_val;
			const double duration = player.get_duration();
			if (duration > 0)
				player.seek(duration * seek_val);
		}

		//     <input type="button" :class="{ isPlaying: !soundPlayerState }" @click="toggle_playback"/>
		//     <Slider id="slider-volume" v-model="config.soundPlayerVolume">
		// </div>
		if (ImGui::Button(view.soundPlayerState ? "Pause" : "Play"))
			toggle_playback();

		ImGui::SameLine();

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		float vol = view.config.value("soundPlayerVolume", 1.0f);
		if (ImGui::SliderFloat("##VolumeSlider", &vol, 0.0f, 1.0f, "Vol: %.0f%%")) {
			view.config["soundPlayerVolume"] = vol;
		}
	}
	app::layout::EndPreviewContainer();

	// --- Bottom-right: Preview controls / export (row 2, col 2) ---
	// JS: div.preview-controls — Loop, Autoplay, Export Selected
	if (app::layout::BeginPreviewControls("sounds-preview-controls", regions)) {
		bool loop_val = view.config.value("soundPlayerLoop", false);
		if (ImGui::Checkbox("Loop", &loop_val))
			view.config["soundPlayerLoop"] = loop_val;

		ImGui::SameLine();

		bool autoplay_val = view.config.value("soundPlayerAutoPlay", false);
		if (ImGui::Checkbox("Autoplay", &autoplay_val))
			view.config["soundPlayerAutoPlay"] = autoplay_val;

		ImGui::SameLine();

		const bool busy = view.isBusy > 0;
		if (busy) ImGui::BeginDisabled();
		if (ImGui::Button("Export Selected"))
			export_selected();
		if (busy) ImGui::EndDisabled();
	}
	app::layout::EndPreviewControls();

	} // if BeginTab
	app::layout::EndTab();
}

void toggle_playback() {
	if (core::view->soundPlayerState)
		pause_track();
	else
		play_track();
}

void export_selected() {
	export_sounds();
}

} // namespace tab_audio
