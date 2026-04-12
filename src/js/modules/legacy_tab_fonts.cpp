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
#include "../components/listbox.h"
#include "../components/context-menu.h"
#include "../install-type.h"
#include "../modules.h"
#include "../mpq/mpq-install.h"
#include "../../app.h"

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

static std::unordered_map<std::string, void*> loaded_fonts;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

static std::string get_font_id(const std::string& file_name) {
	// Use uint32_t for the computation to avoid signed overflow UB,
	// then cast to int32_t at the end to match JS `| 0` (ToInt32) semantics.
	uint32_t hash = 0;
	for (char c : file_name)
		hash = (hash << 5) - hash + static_cast<unsigned char>(c);

	return "font_legacy_" + std::to_string(std::abs(static_cast<int32_t>(hash)));
}

static void* load_font(const std::string& file_name) {
	const std::string font_id = get_font_id(file_name);

	auto it = loaded_fonts.find(font_id);
	if (it != loaded_fonts.end())
		return it->second;

	try {
		mpq::MPQInstall* mpq = core::view->mpq.get();
		std::optional<std::vector<uint8_t>> data = mpq ? mpq->getFile(file_name) : std::nullopt;

		if (!data) {
			logging::write(std::format("failed to load legacy font: {}", file_name));
			return nullptr;
		}

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

static void load_font_list() {
	auto& view = *core::view;
	if (!view.listfileFonts.empty() || view.isBusy > 0)
		return;

	BusyLock _lock = core::create_busy_lock();

	try {
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
	modules::register_nav_button("legacy_tab_fonts", "Fonts", "font.svg", install_type::MPQ);
}

void mounted() {
	auto& view = *core::view;

	load_font_list();

	view.fontPreviewPlaceholder = font_helpers::get_random_quote();
	view.fontPreviewText.clear();
	view.fontPreviewFontFamily.clear();

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

	if (app::layout::BeginTab("tab-legacy-fonts")) {

	auto regions = app::layout::CalcListTabRegions(false);

	// --- Left panel: List container (row 1, col 1) ---
	//     <Listbox v-model:selection="selectionFonts" :items="listfileFonts" ...>
	//     <ContextMenu :node="contextMenus.nodeListbox" ...>
	//       copy_file_paths, copy_export_paths, open_export_directory
	if (app::layout::BeginListContainer("legacy-fonts-list-container", regions)) {
		// Convert JSON items/selection to string vectors.
		std::vector<std::string> items_str;
		items_str.reserve(view.listfileFonts.size());
		for (const auto& item : view.listfileFonts)
			items_str.push_back(item.get<std::string>());

		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionFonts)
			selection_str.push_back(s.get<std::string>());

		listbox::CopyMode copy_mode = listbox::CopyMode::Default;
		{
			std::string cm = view.config.value("copyMode", std::string("Default"));
			if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
			else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
		}

		listbox::render(
			"listbox-legacy-fonts",
			items_str,
			view.userInputFilterFonts,
			selection_str,
			false,   // single
			true,    // keyinput
			view.config.value("regexFilters", false),
			copy_mode,
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"font",  // unittype
			nullptr, // overrideItems
			false,   // disable
			"legacy-fonts", // persistscrollkey
			{},      // quickfilters
			false,   // nocopy
			listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionFonts.clear();
				for (const auto& s : new_sel)
					view.selectionFonts.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				listbox_context::handle_context_menu(ev.selection, true);
			}
		);

		// Context menu for generic listbox.
		context_menu::render(
			"ctx-legacy-fonts",
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

	// --- Filter bar (row 2, col 1) ---
	if (app::layout::BeginFilterBar("legacy-fonts-filter", regions)) {
		if (view.config.value("regexFilters", false))
			ImGui::TextUnformatted("Regex Enabled");

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterFonts.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputText("##FilterFonts", filter_buf, sizeof(filter_buf)))
			view.userInputFilterFonts = filter_buf;
	}
	app::layout::EndFilterBar();

	// --- Right panel: Preview container (row 1, col 2) ---
	if (app::layout::BeginPreviewContainer("legacy-fonts-preview-container", regions)) {
		// Glyph grid.
		ImGui::BeginChild("font-character-grid", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), ImGuiChildFlags_Borders);

		// Set 2px spacing to match CSS: flex-wrap: wrap; gap: 2px
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 2.0f));
		float avail_width = ImGui::GetContentRegionAvail().x;
		float cursor_x = 0.0f;

		for (const auto& codepoint : glyph_state.detected_codepoints) {
			char utf8_buf[5] = {};
			ImTextCharToUtf8(utf8_buf, static_cast<unsigned int>(codepoint));

			// Each glyph cell is a small selectable button.
			ImGui::PushID(static_cast<int>(codepoint));
			if (ImGui::Selectable(utf8_buf, false, 0, ImVec2(24, 24))) {
				view.fontPreviewText += utf8_buf;
			}
			if (ImGui::IsItemHovered()) {
				char tooltip[32];
				std::snprintf(tooltip, sizeof(tooltip), "U+%04X", codepoint);
				ImGui::SetTooltip("%s", tooltip);
			}
			ImGui::PopID();

			// Manual wrap: continue on same line if next button fits.
			cursor_x += 24.0f + 2.0f;
			if (cursor_x + 24.0f <= avail_width) {
				ImGui::SameLine();
			} else {
				cursor_x = 0.0f;
			}
		}
		ImGui::NewLine();
		ImGui::PopStyleVar();
		ImGui::EndChild();

		// Font preview text input.
		char preview_buf[4096] = {};
		std::strncpy(preview_buf, view.fontPreviewText.c_str(), sizeof(preview_buf) - 1);
		if (ImGui::InputTextMultiline("##FontPreviewText", preview_buf, sizeof(preview_buf),
			ImVec2(-1, ImGui::GetContentRegionAvail().y)))
			view.fontPreviewText = preview_buf;

		if (view.fontPreviewText.empty())
			ImGui::SetItemTooltip("%s", view.fontPreviewPlaceholder.c_str());
	}
	app::layout::EndPreviewContainer();

	// --- Bottom-right: Preview controls (row 2, col 2) ---
	//     <input type="button" value="Export Selected" @click="export_fonts" :class="{ disabled: isBusy }"/>
	if (app::layout::BeginPreviewControls("legacy-fonts-preview-controls", regions)) {
		const bool busy = view.isBusy > 0;
		if (busy) app::theme::BeginDisabledButton();
		if (ImGui::Button("Export Selected"))
			export_fonts();
		if (busy) app::theme::EndDisabledButton();
	}
	app::layout::EndPreviewControls();

	} // if BeginTab
	app::layout::EndTab();
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
			namespace fs = std::filesystem;
			const fs::path export_path = fs::path(export_dir) / file_name;

			mpq::MPQInstall* mpq = core::view->mpq.get();
			std::optional<std::vector<uint8_t>> data = mpq ? mpq->getFile(file_name) : std::nullopt;

			if (data) {
				fs::create_directories(export_path.parent_path());
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
