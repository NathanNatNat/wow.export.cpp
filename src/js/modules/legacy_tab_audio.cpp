/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "legacy_tab_audio.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../buffer.h"
#include "../casc/export-helper.h"
#include "../ui/audio-helper.h"
#include "../ui/listbox-context.h"
#include "../install-type.h"
#include "../modules.h"
#include "../mpq/mpq-install.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <fstream>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace legacy_tab_audio {

// --- File-local state ---

// JS: let selected_file = null;
static std::string selected_file;

// JS: let animation_frame_id = null;
static bool seek_loop_active = false;

// JS: const player = new AudioPlayer();
static AudioPlayer player;

// Change-detection for config watches and selection.
static float prev_sound_player_volume = -1.0f;
static bool prev_sound_player_loop = false;
static std::string prev_selection_first;

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
		// JS: const raw_data = core.view.mpq.getFile(selected_file);
		mpq::MPQInstall* mpq = core::view->mpq.get();
		std::optional<std::vector<uint8_t>> raw_data = mpq ? mpq->getFile(selected_file) : std::nullopt;

		if (!raw_data) {
			logging::write(std::format("Failed to load audio: {}", selected_file));
			core::setToast("error", "Failed to load audio file " + selected_file, {}, -1);
			return false;
		}

		// JS: const buffer = Buffer.from(raw_data);
		// JS: const data = new BufferWrapper(buffer);
		BufferWrapper data(*raw_data);

		// JS: const ext = selected_file.slice(selected_file.lastIndexOf('.')).toLowerCase();
		namespace fs = std::filesystem;
		std::string ext = fs::path(selected_file).extension().string();
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		if (ext == ".wav_") {
			core::view->soundPlayerTitle += " (WAV)";
		} else {
			// JS: const file_type = detectFileType(data);
			const AudioType file_type = detectFileType(raw_data->data(), raw_data->size());
			if (file_type == AudioType::OGG)
				core::view->soundPlayerTitle += " (OGG)";
			else if (file_type == AudioType::MP3)
				core::view->soundPlayerTitle += " (MP3)";
		}

		// JS: log.write('audio decode: buffer length=%d, byteOffset=%d, byteLength=%d', ...);
		logging::write(std::format("audio decode: buffer length={}, byteOffset=0, byteLength={}", raw_data->size(), raw_data->size()));
		// JS: log.write('audio decode: first 16 bytes: %s', data.readHexString(16));
		logging::write(std::format("audio decode: first 16 bytes: {}", data.readHexString(16)));

		// JS: log.write('audio decode: sliced array_buffer length=%d', array_buffer.byteLength);
		logging::write(std::format("audio decode: sliced array_buffer length={}", raw_data->size()));

		// JS: await player.load(array_buffer);
		player.load(*raw_data);
		core::view->soundPlayerDuration = player.get_duration();
		core::hideToast();
		return true;
	} catch (const std::exception& e) {
		// JS: core.setToast('error', 'Unable to preview audio ' + selected_file, { 'View Log': () => log.openRuntimeLog() }, -1);
		core::setToast("error", "Unable to preview audio " + selected_file,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to load MPQ audio file: {}", e.what()));
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

// JS: const load_sound_list = async (core) => { ... }
static void load_sound_list() {
	auto& view = *core::view;
	if (!view.listfileSounds.empty() || view.isBusy > 0)
		return;

	BusyLock _lock = core::create_busy_lock();

	try {
		// JS: const ogg_files = core.view.mpq.getFilesByExtension('.ogg');
		// JS: const wav_files = core.view.mpq.getFilesByExtension('.wav');
		// JS: const mp3_files = core.view.mpq.getFilesByExtension('.mp3');
		// JS: const wav__files = core.view.mpq.getFilesByExtension('.wav_');
		mpq::MPQInstall* mpq = core::view->mpq.get();
		if (!mpq) return;
		auto ogg_files = mpq->getFilesByExtension(".ogg");
		auto wav_files = mpq->getFilesByExtension(".wav");
		auto mp3_files = mpq->getFilesByExtension(".mp3");
		auto wav__files = mpq->getFilesByExtension(".wav_");

		// JS: core.view.listfileSounds = [...ogg_files, ...wav_files, ...mp3_files, ...wav__files];
		view.listfileSounds.clear();
		view.listfileSounds.reserve(ogg_files.size() + wav_files.size() + mp3_files.size() + wav__files.size());
		for (auto& f : ogg_files) view.listfileSounds.push_back(std::move(f));
		for (auto& f : wav_files) view.listfileSounds.push_back(std::move(f));
		for (auto& f : mp3_files) view.listfileSounds.push_back(std::move(f));
		for (auto& f : wav__files) view.listfileSounds.push_back(std::move(f));
	} catch (const std::exception& e) {
		logging::write(std::format("failed to load legacy sounds: {}", e.what()));
	}
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

	mpq::MPQInstall* mpq = core::view->mpq.get();
	const bool overwrite_files = view.config.value("overwriteFiles", false);
	for (const auto& sel_entry : user_selection) {
		if (helper.isCancelled())
			return;

		std::string file_name = sel_entry.get<std::string>();
		std::string export_file_name = file_name;

		try {
			// JS: const ext = file_name.slice(file_name.lastIndexOf('.')).toLowerCase();
			namespace fs = std::filesystem;
			std::string ext = fs::path(file_name).extension().string();
			for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

			if (ext == ".wav_") {
				// JS: export_file_name = file_name.slice(0, -1);
				export_file_name = file_name.substr(0, file_name.size() - 1);
			} else {
				// JS: const raw_data = core.view.mpq.getFile(file_name);
				std::optional<std::vector<uint8_t>> raw_data = mpq ? mpq->getFile(file_name) : std::nullopt;

				if (raw_data) {
					// JS: const file_type = detectFileType(wrapped);
					const AudioType file_type = detectFileType(raw_data->data(), raw_data->size());

					if (file_type == AudioType::OGG)
						export_file_name = casc::ExportHelper::replaceExtension(file_name, ".ogg");
					else if (file_type == AudioType::MP3)
						export_file_name = casc::ExportHelper::replaceExtension(file_name, ".mp3");
				}
			}

			// JS: const export_path = ExportHelper.getExportPath(export_file_name);
			const std::string export_path = casc::ExportHelper::getExportPath(export_file_name);
			if (overwrite_files || !generics::fileExists(export_path)) {
				// JS: const raw_data = core.view.mpq.getFile(file_name);
				auto raw_data2 = mpq ? mpq->getFile(file_name) : std::nullopt;
				if (!raw_data2)
					throw std::runtime_error("Failed to read file from MPQ");

				// JS: await fsp.mkdir(path.dirname(export_path), { recursive: true });
				fs::create_directories(fs::path(export_path).parent_path());
				// JS: await fsp.writeFile(export_path, new Uint8Array(raw_data));
				std::ofstream ofs(export_path, std::ios::binary);
				ofs.write(reinterpret_cast<const char*>(raw_data2->data()), static_cast<std::streamsize>(raw_data2->size()));
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
	// JS: this.registerNavButton('Audio', 'music.svg', InstallType.MPQ);
	modules::register_nav_button("legacy_tab_audio", "Audio", "music.svg", install_type::MPQ);
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

	// JS: this.$core.events.on('crash', () => { unload_track(this.$core); player.destroy(); });
	core::events.on("crash", []() {
		unload_track();
		player.destroy();
	});

	// JS: await load_sound_list(this.$core);
	load_sound_list();
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
		const std::string first = view.selectionSounds[0].get<std::string>();
		if (view.isBusy == 0 && !first.empty() && first != prev_selection_first) {
			// JS: this.$core.view.soundPlayerTitle = path.basename(first);
			namespace fs = std::filesystem;
			view.soundPlayerTitle = fs::path(first).filename().string();

			selected_file = first;
			unload_track();

			// JS: if (this.$core.view.config.soundPlayerAutoPlay) play_track(this.$core);
			if (view.config.value("soundPlayerAutoPlay", false))
				play_track();

			prev_selection_first = first;
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
	//       copy_file_paths, copy_export_paths, open_export_directory
	ImGui::BeginChild("sounds-list-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 5), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox and ContextMenu component rendering will be wired when integration is complete.
	ImGui::Text("Sound files: %zu", view.listfileSounds.size());
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
	if (busy) ImGui::BeginDisabled();
	if (ImGui::Button("Export Selected"))
		export_selected();
	if (busy) ImGui::EndDisabled();
}

void toggle_playback() {
	// JS: if (this.$core.view.soundPlayerState) pause_track(this.$core); else play_track(this.$core);
	if (core::view->soundPlayerState)
		pause_track();
	else
		play_track();
}

void export_selected() {
	// JS: await export_sounds(this.$core);
	export_sounds();
}

} // namespace legacy_tab_audio
