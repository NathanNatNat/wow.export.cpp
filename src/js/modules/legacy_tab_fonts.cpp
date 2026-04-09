/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "legacy_tab_fonts.h"
#include "font_helpers.h"
#include "../log.h"
#include "../core.h"
#include "../ui/listbox-context.h"
#include "../install-type.h"
#include "../modules.h"
#include "../mpq/mpq-install.h"

#include <cstring>
#include <cmath>
#include <format>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#include <imgui.h>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>

namespace legacy_tab_fonts {

// --- File-local state ---

// JS: const loaded_fonts = new Map();
static std::unordered_map<std::string, void*> loaded_fonts;

// JS: const get_font_id = (file_name) => { ... }
static std::string get_font_id(const std::string& file_name) {
	// JS: let hash = 0;
	// JS: for (let i = 0; i < file_name.length; i++)
	// JS:     hash = ((hash << 5) - hash + file_name.charCodeAt(i)) | 0;
	// JS: return 'font_legacy_' + Math.abs(hash);
	// Use uint32_t for the computation to avoid signed overflow UB,
	// then cast to int32_t at the end to match JS `| 0` (ToInt32) semantics.
	uint32_t hash = 0;
	for (char c : file_name)
		hash = (hash << 5) - hash + static_cast<unsigned char>(c);

	return "font_legacy_" + std::to_string(std::abs(static_cast<int32_t>(hash)));
}

// JS: const load_font = async (core, file_name) => { ... }
static void* load_font(const std::string& file_name) {
	const std::string font_id = get_font_id(file_name);

	auto it = loaded_fonts.find(font_id);
	if (it != loaded_fonts.end())
		return it->second;

	try {
		// JS: const data = core.view.mpq.getFile(file_name);
		mpq::MPQInstall* mpq = core::view->mpq.get();
		std::optional<std::vector<uint8_t>> data = mpq ? mpq->getFile(file_name) : std::nullopt;

		if (!data) {
			logging::write(std::format("failed to load legacy font: {}", file_name));
			return nullptr;
		}

		// JS: const url = await inject_font_face(font_id, new Uint8Array(data), log);
		void* font = font_helpers::inject_font_face(font_id, data->data(), data->size());

		if (font) {
			loaded_fonts[font_id] = font;
			logging::write(std::format("loaded legacy font {} as {}", file_name, font_id));
		}

		return font;
	} catch (const std::exception& e) {
		logging::write(std::format("failed to load legacy font {}: {}", file_name, e.what()));
		core::setToast("error", std::string("Failed to load font: ") + e.what());
		return nullptr;
	}
}

// Glyph detection state for current font.
static font_helpers::GlyphDetectionState glyph_state;

// Change-detection for selectionFonts.
static std::string prev_selection_first;

// --- Internal functions ---

// JS: const load_font_list = async (core) => { ... }
static void load_font_list() {
	auto& view = *core::view;
	if (!view.listfileFonts.empty() || view.isBusy > 0)
		return;

	BusyLock _lock = core::create_busy_lock();

	try {
		// JS: core.view.listfileFonts = core.view.mpq.getFilesByExtension('.ttf');
		mpq::MPQInstall* mpq = core::view->mpq.get();
		if (!mpq) return;
		auto ttf_files = mpq->getFilesByExtension(".ttf");
		view.listfileFonts.assign(std::make_move_iterator(ttf_files.begin()), std::make_move_iterator(ttf_files.end()));
	} catch (const std::exception& e) {
		logging::write(std::format("failed to load legacy fonts: {}", e.what()));
	}
}

// --- Public API ---

void registerTab() {
	// JS: this.registerNavButton('Fonts', 'font.svg', InstallType.MPQ);
	modules::register_nav_button("legacy_tab_fonts", "Fonts", "font.svg", install_type::MPQ);
}

void mounted() {
	auto& view = *core::view;

	// JS: await load_font_list(this.$core);
	load_font_list();

	// JS: this.$core.view.fontPreviewPlaceholder = get_random_quote();
	view.fontPreviewPlaceholder = font_helpers::get_random_quote();
	// JS: this.$core.view.fontPreviewText = '';
	view.fontPreviewText.clear();
	// JS: this.$core.view.fontPreviewFontFamily = '';
	view.fontPreviewFontFamily.clear();

	// JS: const grid_element = this.$el.querySelector('.font-character-grid');
	// JS: const on_glyph_click = (char) => this.$core.view.fontPreviewText += char;
	// JS: this.$core.view.$watch('selectionFonts', async selection => { ... });
	// Change-detection is handled in render() by comparing selectionFonts[0] each frame.
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for selection (equivalent to watch on selectionFonts) ---
	if (!view.selectionFonts.empty()) {
		const std::string first = view.selectionFonts[0].get<std::string>();
		if (!first.empty() && view.isBusy == 0 && first != prev_selection_first) {
			void* font = load_font(first);
			if (font) {
				view.fontPreviewFontFamily = get_font_id(first);
				font_helpers::detect_glyphs_async(font, glyph_state);
			}
			prev_selection_first = first;
		}
	}

	// Process glyph detection batch each frame.
	if (!glyph_state.complete && !glyph_state.cancelled) {
		// Get the loaded font pointer for the current font family.
		void* font = nullptr;
		auto it = loaded_fonts.find(view.fontPreviewFontFamily);
		if (it != loaded_fonts.end())
			font = it->second;

		if (font)
			font_helpers::process_glyph_detection_batch(font, glyph_state);
	}

	// List container with context menu.
	// JS: <div class="list-container">
	//     <Listbox v-model:selection="selectionFonts" :items="listfileFonts" ...>
	//     <ContextMenu :node="contextMenus.nodeListbox" ...>
	//       copy_file_paths, copy_export_paths, open_export_directory
	ImGui::BeginChild("legacy-fonts-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox and ContextMenu component rendering will be wired when integration is complete.
	ImGui::Text("Font files: %zu", view.listfileFonts.size());
	ImGui::EndChild();

	ImGui::SameLine();

	// Right side — filter, preview, controls.
	ImGui::BeginGroup();

	// Filter.
	// JS: <div class="filter">
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterFonts.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterFonts", filter_buf, sizeof(filter_buf)))
		view.userInputFilterFonts = filter_buf;

	// Font preview container.
	// JS: <div class="preview-container font-preview">
	ImGui::BeginChild("font-preview-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// Glyph grid.
	// JS: <div class="font-character-grid"></div>
	ImGui::BeginChild("font-character-grid", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), ImGuiChildFlags_Borders);
	for (const auto& codepoint : glyph_state.detected_codepoints) {
		char utf8_buf[5] = {};
		ImTextCharToUtf8(utf8_buf, static_cast<unsigned int>(codepoint));

		// Each glyph cell is a small selectable button.
		ImGui::PushID(static_cast<int>(codepoint));
		if (ImGui::Selectable(utf8_buf, false, 0, ImVec2(24, 24))) {
			// JS: on_glyph_click(char) => this.$core.view.fontPreviewText += char;
			view.fontPreviewText += utf8_buf;
		}
		if (ImGui::IsItemHovered()) {
			char tooltip[32];
			std::snprintf(tooltip, sizeof(tooltip), "U+%04X", codepoint);
			ImGui::SetTooltip("%s", tooltip);
		}
		ImGui::PopID();
		ImGui::SameLine();
	}
	ImGui::NewLine();
	ImGui::EndChild();

	// Font preview text input.
	// JS: <textarea :style="{ fontFamily: fontPreviewFontFamily }" :placeholder="fontPreviewPlaceholder" v-model="fontPreviewText">
	char preview_buf[4096] = {};
	std::strncpy(preview_buf, view.fontPreviewText.c_str(), sizeof(preview_buf) - 1);
	if (ImGui::InputTextMultiline("##FontPreviewText", preview_buf, sizeof(preview_buf),
		ImVec2(-1, ImGui::GetContentRegionAvail().y)))
		view.fontPreviewText = preview_buf;

	if (view.fontPreviewText.empty())
		ImGui::SetItemTooltip("%s", view.fontPreviewPlaceholder.c_str());

	ImGui::EndChild();

	// Preview controls.
	// JS: <div class="preview-controls">
	//     <input type="button" value="Export Selected" @click="export_fonts" :class="{ disabled: isBusy }"/>
	const bool busy = view.isBusy > 0;
	if (busy) ImGui::BeginDisabled();
	if (ImGui::Button("Export Selected"))
		export_fonts();
	if (busy) ImGui::EndDisabled();

	ImGui::EndGroup();
}

void export_fonts() {
	auto& view = *core::view;
	const auto& selected = view.selectionFonts;
	if (selected.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	BusyLock _lock = core::create_busy_lock();
	const std::string export_dir = view.config.value("exportDirectory", std::string(""));

	int exported = 0;
	int failed = 0;
	std::string last_export_path;

	for (const auto& sel_entry : selected) {
		const std::string file_name = sel_entry.get<std::string>();

		try {
			// JS: const export_path = path.join(export_dir, file_name);
			namespace fs = std::filesystem;
			const fs::path export_path = fs::path(export_dir) / file_name;

			// JS: const data = this.$core.view.mpq.getFile(file_name);
			mpq::MPQInstall* mpq = core::view->mpq.get();
			std::optional<std::vector<uint8_t>> data = mpq ? mpq->getFile(file_name) : std::nullopt;

			if (data) {
				// JS: await fsp.mkdir(path.dirname(export_path), { recursive: true });
				fs::create_directories(export_path.parent_path());
				// JS: await fsp.writeFile(export_path, new Uint8Array(data));
				std::ofstream ofs(export_path, std::ios::binary);
				ofs.write(reinterpret_cast<const char*>(data->data()), static_cast<std::streamsize>(data->size()));
				last_export_path = export_path.string();
				exported++;
			} else {
				logging::write(std::format("failed to read font file from MPQ: {}", file_name));
				failed++;
			}
		} catch (const std::exception& e) {
			logging::write(std::format("failed to export font {}: {}", file_name, e.what()));
			failed++;
		}
	}

	if (failed > 0) {
		core::setToast("error", std::format("Exported {} fonts with {} failures.", exported, failed));
	} else if (!last_export_path.empty()) {
		namespace fs = std::filesystem;
		// JS: const dir = path.dirname(last_export_path);
		// JS: const toast_opt = { 'View in Explorer': () => nw.Shell.openItem(dir) };
		const std::string dir = fs::path(last_export_path).parent_path().string();
		std::vector<ToastAction> toast_opt = { {"View in Explorer", [dir]() { core::openInExplorer(dir); }} };

		if (selected.size() > 1)
			core::setToast("success", std::format("Successfully exported {} fonts.", exported), toast_opt, -1);
		else
			core::setToast("success", std::format("Successfully exported {}.",
				fs::path(last_export_path).filename().string()), toast_opt, -1);
	}
}

} // namespace legacy_tab_fonts
