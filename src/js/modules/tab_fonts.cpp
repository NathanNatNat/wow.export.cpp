/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_fonts.h"
#include "font_helpers.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../ui/listbox-context.h"
#include "../install-type.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <unordered_map>

#include <imgui.h>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>

namespace tab_fonts {

// --- File-local state ---

// JS: const loaded_fonts = new Map();
static std::unordered_map<std::string, void*> loaded_fonts;

// JS: const get_font_id = (file_data_id) => 'font_id_' + file_data_id;
static std::string get_font_id(uint32_t file_data_id) {
	return "font_id_" + std::to_string(file_data_id);
}

// JS: const load_font = async (core, file_name) => { ... }
static void* load_font(const std::string& file_name) {
	auto file_data_id = casc::listfile::getByFilename(file_name);
	if (!file_data_id)
		return nullptr;

	const std::string font_id = get_font_id(*file_data_id);

	auto it = loaded_fonts.find(font_id);
	if (it != loaded_fonts.end())
		return it->second;

	try {
		// JS: const data = await core.view.casc.getFileByName(file_name);
		// JS: data.processAllBlocks();
		// JS: const url = await inject_font_face(font_id, data.raw, log);
		// TODO(conversion): CASC getFileByName will be wired when CASC integration is complete.
		// For now, the font loading pipeline is preserved but stub-wired.

		// Placeholder: when CASC is available, get file data and call:
		// void* font = font_helpers::inject_font_face(font_id, data_ptr, data_size);
		void* font = nullptr;

		if (font) {
			loaded_fonts[font_id] = font;
			logging::write(std::format("loaded font {} as {}", file_name, font_id));
		}

		return font;
	} catch (const std::exception& e) {
		logging::write(std::format("failed to load font {}: {}", file_name, e.what()));
		core::setToast("error", std::string("Failed to load font: ") + e.what());
		return nullptr;
	}
}

// Glyph detection state for current font.
static font_helpers::GlyphDetectionState glyph_state;

// Change-detection for selectionFonts.
static std::string prev_selection_first;

// --- Public API ---

void registerTab() {
	// JS: this.registerNavButton('Fonts', 'font.svg', InstallType.CASC);
	// TODO(conversion): Nav button registration will be wired when the module system is integrated.
}

void mounted() {
	auto& view = *core::view;

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
		const std::string first = casc::listfile::stripFileEntry(view.selectionFonts[0].get<std::string>());
		if (!first.empty() && view.isBusy == 0 && first != prev_selection_first) {
			void* font = load_font(first);
			if (font) {
				view.fontPreviewFontFamily = get_font_id(*casc::listfile::getByFilename(first));
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
	ImGui::BeginChild("fonts-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox and ContextMenu component rendering will be wired when integration is complete.
	// JS: <Listbox v-model:selection="selectionFonts" :items="listfileFonts" :filter="userInputFilterFonts" ...>
	// JS: <ContextMenu :node="contextMenus.nodeListbox" ...>
	//   copy_file_paths, copy_listfile_format, copy_file_data_ids, copy_export_paths, open_export_directory
	ImGui::Text("Font files: %zu", view.listfileFonts.size());
	ImGui::EndChild();

	ImGui::SameLine();

	// Right side — filter, preview, controls.
	ImGui::BeginGroup();

	// Filter.
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterFonts.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterFonts", filter_buf, sizeof(filter_buf)))
		view.userInputFilterFonts = filter_buf;

	// Font preview container.
	ImGui::BeginChild("font-preview-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// Glyph grid.
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
	const bool busy = view.isBusy > 0;
	if (busy) ImGui::BeginDisabled();
	if (ImGui::Button("Export Selected"))
		export_fonts();
	if (busy) ImGui::EndDisabled();

	ImGui::EndGroup();
}

void export_fonts() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionFonts;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	casc::ExportHelper helper(static_cast<int>(user_selection.size()), "file");
	helper.start();

	const bool overwrite_files = view.config.value("overwriteFiles", false);
	for (const auto& sel_entry : user_selection) {
		if (helper.isCancelled())
			return;

		std::string file_name = casc::listfile::stripFileEntry(sel_entry.get<std::string>());
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
				// JS: const data = await this.$core.view.casc.getFileByName(file_name);
				// JS: await data.writeToFile(export_path);
				// TODO(conversion): CASC getFileByName will be wired when CASC integration is complete.
			} else {
				logging::write(std::format("Skipping font export {} (file exists, overwrite disabled)", export_path));
			}

			helper.mark(export_file_name, true);
		} catch (const std::exception& e) {
			helper.mark(export_file_name, false, e.what());
		}
	}

	helper.finish();
}

} // namespace tab_fonts
