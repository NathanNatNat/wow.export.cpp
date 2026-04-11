/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_audio.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
#include "../casc/db2.h"
#include "../db/WDCReader.h"
#include "../ui/audio-helper.h"
#include "../ui/listbox-context.h"
#include "../install-type.h"
#include "../modules.h"
#include "../components/listbox.h"
#include "../components/context-menu.h"
#include "../../app.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <optional>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_audio {

// --- File-local state ---

// JS: let selected_file = null;
static std::string selected_file;

// JS: let selected_file_data_id = null;
static std::optional<uint32_t> selected_file_data_id;

// JS: let animation_frame_id = null;
static bool seek_loop_active = false;

// JS: let file_data = null;
// TODO(conversion): file_data was a BufferWrapper used for revokeDataURL in JS; not needed in C++.

// JS: const player = new AudioPlayer();
static AudioPlayer player;

// Change-detection for config watches and selection.
static float prev_sound_player_volume = -1.0f;
static bool prev_sound_player_loop = false;
static std::string prev_selection_first;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

// --- Internal functions ---

// JS: const update_seek = (core) => { ... }
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

// JS: const start_seek_loop = (core) => { ... }
static void start_seek_loop() {
	if (!seek_loop_active) {
		seek_loop_active = true;
		update_seek();
	}
}

// JS: const stop_seek_loop = () => { ... }
static void stop_seek_loop() {
	seek_loop_active = false;
}

// JS: const load_track = async (core) => { ... }
static bool load_track() {
	if (selected_file.empty())
		return false;

	BusyLock _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", selected_file), {}, -1, false);
	logging::write(std::format("Previewing sound file {}", selected_file));

	try {
		// JS: if (selected_file_data_id !== null)
		//     file_data = await core.view.casc.getFile(selected_file_data_id);
		// else
		//     file_data = await core.view.casc.getFileByName(selected_file);
		BufferWrapper file_data_buf;
		if (selected_file_data_id.has_value())
			file_data_buf = core::view->casc->getVirtualFileByID(selected_file_data_id.value());
		else
			file_data_buf = core::view->casc->getVirtualFileByName(selected_file);
		const auto& audio_data = file_data_buf.raw();

		if (selected_file.ends_with(".unk_sound")) {
			const AudioType file_type = detectFileType(audio_data.data(), audio_data.size());
			if (file_type == AudioType::OGG)
				core::view->soundPlayerTitle += " (OGG Auto Detected)";
			else if (file_type == AudioType::MP3)
				core::view->soundPlayerTitle += " (MP3 Auto Detected)";
		}

		player.stop();
		// JS: player.buffer = await file_data.decodeAudio(player.context);
		player.load(audio_data);
		core::view->soundPlayerDuration = player.get_duration();
		core::hideToast();
		return true;
	} catch (const casc::EncryptionError& e) {
		core::setToast("error", std::format("The audio file {} is encrypted with an unknown key ({}).", selected_file, e.key), {}, -1);
		logging::write(std::format("Failed to decrypt audio file {} ({})", selected_file, e.key));
		return false;
	} catch (const std::exception& e) {
		// JS: core.setToast('error', 'Unable to preview audio ' + selected_file, { 'View Log': () => log.openRuntimeLog() }, -1);
		core::setToast("error", "Unable to preview audio " + selected_file,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
		return false;
	}
}

// JS: const unload_track = (core) => { ... }
static void unload_track() {
	stop_seek_loop();
	player.unload();

	core::view->soundPlayerState = false;
	core::view->soundPlayerDuration = 0;
	core::view->soundPlayerSeek = 0;

	// JS: file_data?.revokeDataURL();
	// TODO(conversion): revokeDataURL is a browser API; not needed in C++.
}

// JS: const play_track = async (core) => { ... }
static void play_track() {
	// JS: if (!player.buffer) { ... }
	// In C++, AudioPlayer has no public 'buffer' field; get_duration() returns 0 when no audio loaded.
	if (player.get_duration() <= 0) {
		if (selected_file.empty()) {
			core::setToast("info", "You need to select an audio track first!", {}, -1, true);
			return;
		}

		const bool loaded = load_track();
		if (!loaded)
			return;
	}

	player.play();
	core::view->soundPlayerState = true;
	start_seek_loop();
}

// JS: const pause_track = (core) => { ... }
static void pause_track() {
	player.pause();
	stop_seek_loop();
	core::view->soundPlayerState = false;
}

// JS: const export_sounds = async (core) => { ... }
static void export_sounds() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionSounds;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	casc::ExportHelper helper(static_cast<int>(user_selection.size()), "sound files");
	helper.start();

	const bool overwrite_files = view.config.value("overwriteFiles", false);
	for (const auto& sel_entry : user_selection) {
		if (helper.isCancelled())
			return;

		// JS: let export_data;
		bool has_export_data = false;
		std::vector<uint8_t> export_data;

		std::string file_name = casc::listfile::stripFileEntry(sel_entry.get<std::string>());

		if (file_name.ends_with(".unk_sound")) {
			// JS: export_data = await core.view.casc.getFileByName(file_name);
			BufferWrapper export_buf = core::view->casc->getVirtualFileByName(file_name);
			export_data = std::move(export_buf.raw());
			has_export_data = true;

			// JS: const file_type = detectFileType(export_data);
			const AudioType file_type = detectFileType(export_data.data(), export_data.size());

			if (file_type == AudioType::OGG)
				file_name = casc::ExportHelper::replaceExtension(file_name, ".ogg");
			else if (file_type == AudioType::MP3)
				file_name = casc::ExportHelper::replaceExtension(file_name, ".mp3");
		}

		std::string export_file_name = file_name;

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
			if (overwrite_files || !generics::fileExists(export_path)) {
				// JS: if (!export_data)
				//     export_data = await core.view.casc.getFileByName(file_name);
				if (!has_export_data) {
					BufferWrapper export_buf = core::view->casc->getVirtualFileByName(file_name);
					export_data = std::move(export_buf.raw());
				}

				// JS: await export_data.writeToFile(export_path);
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

	helper.finish();
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
	// JS: this.registerNavButton('Audio', 'music.svg', InstallType.CASC);
	modules::register_nav_button("tab_audio", "Audio", "music.svg", install_type::CASC);
}

void mounted() {
	auto& view = *core::view;

	// JS: player.init();
	player.init();
	// JS: player.set_volume(this.$core.view.config.soundPlayerVolume);
	const float initial_volume = view.config.value("soundPlayerVolume", 1.0f);
	player.set_volume(initial_volume);
	prev_sound_player_volume = initial_volume;
	// JS: player.set_loop(this.$core.view.config.soundPlayerLoop);
	const bool initial_loop = view.config.value("soundPlayerLoop", false);
	player.set_loop(initial_loop);
	prev_sound_player_loop = initial_loop;

	// JS: player.on_ended = () => { ... };
	player.on_ended = []() {
		stop_seek_loop();
		core::view->soundPlayerState = false;
		core::view->soundPlayerSeek = 0;
	};

	// JS: if (this.$core.view.config.enableUnknownFiles) { ... }
	if (view.config.value("enableUnknownFiles", false)) {
		core::showLoadingScreen(1);
		core::progressLoadingScreen("Processing unknown sound files...");

		int unknown_count = 0;
		// JS: for (const entry of (await db2.SoundKitEntry.getAllRows()).values()) { ... }
		auto& sound_kit_entry = casc::db2::getTable("SoundKitEntry");
		if (!sound_kit_entry.isLoaded)
			sound_kit_entry.parse();

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
					view.listfileSounds.push_back(std::move(s));
				unknown_count++;
			}
		}

		logging::write(std::format("Added {} unknown sound files from SoundKitEntry to listfile", unknown_count));
		core::hideLoadingScreen();
	}

	// JS: this.$core.events.on('crash', () => { unload_track(this.$core); player.destroy(); });
	core::events.on("crash", []() {
		unload_track();
		player.destroy();
	});
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for config watches ---

	// JS: this.$core.view.$watch('config.soundPlayerVolume', value => { player.set_volume(value); });
	const float current_volume = view.config.value("soundPlayerVolume", 1.0f);
	if (current_volume != prev_sound_player_volume) {
		player.set_volume(current_volume);
		prev_sound_player_volume = current_volume;
	}

	// JS: this.$core.view.$watch('config.soundPlayerLoop', value => { player.set_loop(value); });
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

	// List container with context menu.
	// JS: <div class="list-container">
	//     <Listbox v-model:selection="selectionSounds" :items="listfileSounds" ...>
	//     <ContextMenu :node="contextMenus.nodeListbox" ...>
	ImGui::BeginChild("sounds-list-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 5), ImGuiChildFlags_Borders);
	{
		// Convert JSON items/selection to string vectors.
		std::vector<std::string> items_str;
		items_str.reserve(view.listfileSounds.size());
		for (const auto& item : view.listfileSounds)
			items_str.push_back(item.get<std::string>());

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
			"sound", // unittype
			nullptr, // overrideItems
			false,   // disable
			"sounds", // persistscrollkey
			{},      // quickfilters
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
	ImGui::EndChild();

	// Filter.
	// JS: <div class="filter">
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterSounds.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterSounds", filter_buf, sizeof(filter_buf)))
		view.userInputFilterSounds = filter_buf;

	// Sound player.
	// JS: <div id="sound-player">
	ImGui::BeginChild("sound-player", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2.5f));

	// JS: <div id="sound-player-info">
	//     <span>{{ $core.view.soundPlayerSeekFormatted }}</span>
	//     <span class="title">{{ $core.view.soundPlayerTitle }}</span>
	//     <span>{{ $core.view.soundPlayerDurationFormatted }}</span>
	// </div>
	const std::string seek_formatted = format_time(view.soundPlayerSeek * view.soundPlayerDuration);
	const std::string duration_formatted = format_time(view.soundPlayerDuration);
	ImGui::Text("%s  %s  %s", seek_formatted.c_str(), view.soundPlayerTitle.c_str(), duration_formatted.c_str());

	// JS: <Slider id="slider-seek" v-model="soundPlayerSeek" @update:model-value="handle_seek">
	float seek_val = static_cast<float>(view.soundPlayerSeek);
	if (ImGui::SliderFloat("##SeekSlider", &seek_val, 0.0f, 1.0f, "")) {
		view.soundPlayerSeek = seek_val;
		// JS: handle_seek(seek) { ... player.seek(duration * seek); }
		const double duration = player.get_duration();
		if (duration > 0)
			player.seek(duration * seek_val);
	}

	// JS: <div class="buttons">
	//     <input type="button" :class="{ isPlaying: !soundPlayerState }" @click="toggle_playback"/>
	//     <Slider id="slider-volume" v-model="config.soundPlayerVolume">
	// </div>
	if (ImGui::Button(view.soundPlayerState ? "Pause" : "Play"))
		toggle_playback();

	ImGui::SameLine();

	float vol = view.config.value("soundPlayerVolume", 1.0f);
	if (ImGui::SliderFloat("##VolumeSlider", &vol, 0.0f, 1.0f, "Vol: %.0f%%")) {
		// This triggers the change-detection above on next frame.
		view.config["soundPlayerVolume"] = vol;
	}

	ImGui::EndChild();

	// Preview controls.
	// JS: <div class="preview-controls">
	bool loop_val = view.config.value("soundPlayerLoop", false);
	if (ImGui::Checkbox("Loop", &loop_val))
		view.config["soundPlayerLoop"] = loop_val;

	ImGui::SameLine();

	bool autoplay_val = view.config.value("soundPlayerAutoPlay", false);
	if (ImGui::Checkbox("Autoplay", &autoplay_val))
		view.config["soundPlayerAutoPlay"] = autoplay_val;

	ImGui::SameLine();

	const bool busy = view.isBusy > 0;
	if (busy) app::theme::BeginDisabledButton();
	if (ImGui::Button("Export Selected"))
		export_selected();
	if (busy) app::theme::EndDisabledButton();
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
