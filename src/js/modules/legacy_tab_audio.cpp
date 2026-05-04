#include "legacy_tab_audio.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../buffer.h"
#include "../casc/export-helper.h"
#include "../ui/audio-helper.h"
#include "../ui/listbox-context.h"
#include "../components/listbox.h"
#include "../components/context-menu.h"
#include "../install-type.h"
#include "../modules.h"
#include "../mpq/mpq-install.h"
#include "../../app.h"

#include <cmath>
#include <cstring>
#include <format>
#include <filesystem>
#include <fstream>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace legacy_tab_audio {

static std::optional<std::string> build_stack_trace(const char* function_name, const std::exception& e) {
	return std::format("{}: {}", function_name, e.what());
}

static std::string selected_file;

static bool seek_loop_active = false;

static AudioPlayer player;

static float prev_sound_player_volume = -1.0f;
static bool prev_sound_player_loop = false;
static std::string prev_selection_first;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

static void update_seek() {
	if (!player.is_playing) {
		seek_loop_active = false;
		return;
	}

	const double duration = player.get_duration();
	if (duration > 0)
		core::view->soundPlayerSeek = player.get_position() / duration;
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

static bool load_track() {
	if (selected_file.empty())
		return false;

	BusyLock _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", selected_file), {}, -1, false);
	logging::write(std::format("Previewing sound file {}", selected_file));

	try {
		mpq::MPQInstall* mpq = core::view->mpq.get();
		std::optional<std::vector<uint8_t>> raw_data = mpq ? mpq->getFile(selected_file) : std::nullopt;

		if (!raw_data) {
			logging::write(std::format("Failed to load audio: {}", selected_file));
			core::setToast("error", "Failed to load audio file " + selected_file, {}, -1);
			return false;
		}

		BufferWrapper data(*raw_data);

		namespace fs = std::filesystem;
		std::string ext = fs::path(selected_file).extension().string();
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		if (ext == ".wav_") {
			core::view->soundPlayerTitle += " (WAV)";
		} else {
			const AudioType file_type = detectFileType(data);
			if (file_type == AudioType::OGG)
				core::view->soundPlayerTitle += " (OGG)";
			else if (file_type == AudioType::MP3)
				core::view->soundPlayerTitle += " (MP3)";
		}

		logging::write(std::format("audio decode: buffer length={}, byteOffset=0, byteLength={}", raw_data->size(), raw_data->size()));
		logging::write(std::format("audio decode: first 16 bytes: {}", data.readHexString(16)));

		logging::write(std::format("audio decode: sliced array_buffer length={}", raw_data->size()));

		player.load(*raw_data);
		core::view->soundPlayerDuration = player.get_duration();
		core::hideToast();
		return true;
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to preview audio " + selected_file,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to load MPQ audio file: {}", e.what()));
		return false;
	}
}

static void unload_track() {
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

		const bool loaded = load_track();
		if (!loaded)
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

static void load_sound_list() {
	auto& view = *core::view;
	if (!view.listfileSounds.empty() || view.isBusy > 0)
		return;

	BusyLock _lock = core::create_busy_lock();

	try {
		mpq::MPQInstall* mpq = core::view->mpq.get();
		if (!mpq) return;
		auto ogg_files = mpq->getFilesByExtension(".ogg");
		auto wav_files = mpq->getFilesByExtension(".wav");
		auto mp3_files = mpq->getFilesByExtension(".mp3");
		auto wav__files = mpq->getFilesByExtension(".wav_");

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
			namespace fs = std::filesystem;
			std::string ext = fs::path(file_name).extension().string();
			for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

			if (ext == ".wav_") {
				export_file_name = file_name.substr(0, file_name.size() - 1);
			} else {
				std::optional<std::vector<uint8_t>> raw_data = mpq ? mpq->getFile(file_name) : std::nullopt;

				if (raw_data) {
					const AudioType file_type = detectFileType(BufferWrapper(*raw_data));

					if (file_type == AudioType::OGG)
						export_file_name = casc::ExportHelper::replaceExtension(file_name, ".ogg");
					else if (file_type == AudioType::MP3)
						export_file_name = casc::ExportHelper::replaceExtension(file_name, ".mp3");
				}
			}

			const std::string export_path = casc::ExportHelper::getExportPath(export_file_name);
			if (overwrite_files || !generics::fileExists(export_path)) {
				auto raw_data2 = mpq ? mpq->getFile(file_name) : std::nullopt;
				if (!raw_data2)
					throw std::runtime_error("Failed to read file from MPQ");

				fs::create_directories(fs::path(export_path).parent_path());
				std::ofstream ofs(export_path, std::ios::binary);
				ofs.write(reinterpret_cast<const char*>(raw_data2->data()), static_cast<std::streamsize>(raw_data2->size()));
			} else {
				logging::write(std::format("Skipping audio export {} (file exists, overwrite disabled)", export_path));
			}

			helper.mark(export_file_name, true);
		} catch (const std::exception& e) {
			helper.mark(export_file_name, false, e.what(), build_stack_trace("export_sounds", e));
		}
	}

	helper.finish();
}

void registerTab() {
	modules::register_nav_button("legacy_tab_audio", "Audio", "music.svg", install_type::MPQ);
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

	load_sound_list();
}

void render() {
	auto& view = *core::view;

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

	if (!view.selectionSounds.empty()) {
		const std::string first = view.selectionSounds[0].get<std::string>();
		if (view.isBusy == 0 && !first.empty() && first != prev_selection_first) {
			namespace fs = std::filesystem;
			view.soundPlayerTitle = fs::path(first).filename().string();

			selected_file = first;
			unload_track();

			if (view.config.value("soundPlayerAutoPlay", false))
				play_track();

			prev_selection_first = first;
		}
	}

	if (seek_loop_active)
		update_seek();

	if (app::layout::BeginTab("tab-legacy-audio")) {

	auto regions = app::layout::CalcListTabRegions(false);

	if (app::layout::BeginListContainer("legacy-sounds-list-container", regions)) {
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
			"listbox-legacy-sounds",
			items_str,
			view.userInputFilterSounds,
			selection_str,
			false,
			true,
			view.config.value("regexFilters", false),
			copy_mode,
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"sound file",
			nullptr,
			false,
			"sounds",
			{},
			false,
			listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionSounds.clear();
				for (const auto& s : new_sel)
					view.selectionSounds.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				listbox_context::handle_context_menu(ev.selection, true);
			}
		);

		context_menu::render(
			"ctx-legacy-sounds",
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

				if (ImGui::Selectable(std::format("Copy file path{}", plural).c_str()))
					listbox_context::copy_file_paths(sel);
				if (ImGui::Selectable(std::format("Copy export path{}", plural).c_str()))
					listbox_context::copy_export_paths(sel);
				if (ImGui::Selectable("Open export directory"))
					listbox_context::open_export_directory(sel);
			}
		);
	}
	app::layout::EndListContainer();

	if (app::layout::BeginStatusBar("legacy-sounds-status", regions)) {
		listbox::renderStatusBar("sound file", {}, listbox_state);
	}
	app::layout::EndStatusBar();

	if (app::layout::BeginFilterBar("legacy-sounds-filter", regions)) {
		if (view.config.value("regexFilters", false)) {
			ImGui::TextUnformatted("Regex Enabled");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", view.regexTooltip.c_str());
			ImGui::SameLine();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterSounds.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputTextWithHint("##FilterLegacySounds", "Filter sound files...", filter_buf, sizeof(filter_buf)))
			view.userInputFilterSounds = filter_buf;
	}
	app::layout::EndFilterBar();

	if (app::layout::BeginPreviewContainer("legacy-sounds-preview-container", regions)) {

		if (player.is_playing) {
			ImFont* iconFont = app::theme::getIconFont();
			if (iconFont) {
				float t = static_cast<float>(ImGui::GetTime());
				float animSize = 48.0f * (1.0f + 0.08f * std::sin(t * 3.0f));
				ImGui::PushFont(iconFont, animSize);
				float availW = ImGui::GetContentRegionAvail().x;
				float textW = ImGui::CalcTextSize(ICON_FA_MUSIC).x;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - textW) * 0.5f);
				ImGui::TextUnformatted(ICON_FA_MUSIC);
				ImGui::PopFont();
			}
		}

		ImGui::Spacing();

		const std::string seek_formatted = generics::formatPlaybackSeconds(view.soundPlayerSeek * view.soundPlayerDuration);
		const std::string duration_formatted = generics::formatPlaybackSeconds(view.soundPlayerDuration);
		ImGui::Text("%s  %s  %s", seek_formatted.c_str(), view.soundPlayerTitle.c_str(), duration_formatted.c_str());

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		float seek_val = static_cast<float>(view.soundPlayerSeek);
		if (ImGui::SliderFloat("##LegacySeekSlider", &seek_val, 0.0f, 1.0f, "")) {
			view.soundPlayerSeek = seek_val;
			const double duration = player.get_duration();
			if (duration > 0)
				player.seek(duration * seek_val);
		}

		if (ImGui::Button(view.soundPlayerState ? "Pause" : "Play"))
			toggle_playback();

		ImGui::SameLine();

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		float vol = view.config.value("soundPlayerVolume", 1.0f);
		if (ImGui::SliderFloat("##LegacyVolumeSlider", &vol, 0.0f, 1.0f, "Vol: %.0f%%")) {
			view.config["soundPlayerVolume"] = vol;
		}

	}
	app::layout::EndPreviewContainer();

	if (app::layout::BeginPreviewControls("legacy-sounds-preview-controls", regions)) {
		bool loop_val = view.config.value("soundPlayerLoop", false);
		if (ImGui::Checkbox("Loop##legacy", &loop_val))
			view.config["soundPlayerLoop"] = loop_val;
		ImGui::SameLine();
		bool autoplay_val = view.config.value("soundPlayerAutoPlay", false);
		if (ImGui::Checkbox("Autoplay##legacy", &autoplay_val))
			view.config["soundPlayerAutoPlay"] = autoplay_val;
		ImGui::SameLine();
		const bool busy = view.isBusy > 0;
		if (busy) ImGui::BeginDisabled();
		if (ImGui::Button("Export Selected"))
			export_selected();
		if (busy) ImGui::EndDisabled();
	}
	app::layout::EndPreviewControls();

	}
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

}
