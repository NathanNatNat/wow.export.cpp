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
#include "../components/listbox.h"
#include "../components/context-menu.h"
#include "../install-type.h"
#include "../modules.h"
#include "../../app.h"

#include <chrono>
#include <cstring>
#include <format>
#include <filesystem>
#include <future>
#include <unordered_map>

#include <imgui.h>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>

namespace tab_fonts {

static std::optional<std::string> build_stack_trace(const char* function_name, const std::exception& e) {
	return std::format("{}: {}", function_name, e.what());
}

// --- File-local state ---

static std::unordered_map<std::string, void*> loaded_fonts;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

// Cached items string vector — only rebuilt when the source JSON changes.
static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

static std::string get_font_id(uint32_t file_data_id) {
	return "font_id_" + std::to_string(file_data_id);
}

// Synchronous load_font for cached fonts (returns immediately if cached).
static void* load_font_cached(const std::string& file_name) {
	auto file_data_id = casc::listfile::getByFilename(file_name);
	if (!file_data_id)
		return nullptr;

	const std::string font_id = get_font_id(*file_data_id);
	auto it = loaded_fonts.find(font_id);
	if (it != loaded_fonts.end())
		return it->second;

	return nullptr;
}

// --- Async font loading (follows tab_models pattern) ---

// Glyph detection state for current font (declared early for pump_font_load).
static font_helpers::GlyphDetectionState glyph_state;

struct PendingFontLoad {
	std::string file_name;
	uint32_t file_data_id = 0;
	std::string font_id;
	std::future<BufferWrapper> file_future;
	std::unique_ptr<BusyLock> busy_lock;
};

static std::optional<PendingFontLoad> pending_font_load;

static void load_font_async(const std::string& file_name) {
	// Cancel any pending font load.
	pending_font_load.reset();

	auto file_data_id = casc::listfile::getByFilename(file_name);
	if (!file_data_id)
		return;

	const std::string font_id = get_font_id(*file_data_id);

	// If already cached, skip async load.
	auto it = loaded_fonts.find(font_id);
	if (it != loaded_fonts.end())
		return; // Already loaded; render() will use the cached version.

	auto* casc = core::view->casc;
	if (!casc)
		return;

	PendingFontLoad task;
	task.file_name = file_name;
	task.file_data_id = *file_data_id;
	task.font_id = font_id;
	task.busy_lock = std::make_unique<BusyLock>(core::create_busy_lock());
	task.file_future = std::async(std::launch::async, [casc, file_name]() {
		return casc->getVirtualFileByName(file_name);
	});
	pending_font_load = std::move(task);
}

static void pump_font_load() {
	if (!pending_font_load.has_value())
		return;

	auto& task = *pending_font_load;
	if (task.file_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	try {
		BufferWrapper data = task.file_future.get();
		font_helpers::inject_font_face(task.font_id, data.raw().data(), data.byteLength());
		void* font = font_helpers::get_injected_font(task.font_id);

		if (font) {
			loaded_fonts[task.font_id] = font;
			logging::write(std::format("loaded font {} as {}", task.file_name, task.font_id));

			// Set up font preview and glyph detection now that the font is loaded.
			auto& view = *core::view;
			view.fontPreviewFontFamily = task.font_id;
			font_helpers::detect_glyphs_async(
				font,
				glyph_state,
				[](const std::string& ch) { core::view->fontPreviewText += ch; }
			);
		}
	} catch (const std::exception& e) {
		logging::write(std::format("failed to load font {}: {}", task.file_name, e.what()));
		core::setToast("error", std::string("Failed to load font: ") + e.what());
	}

	pending_font_load.reset();
}

// Change-detection for selectionFonts.
static std::string prev_selection_first;

// --- Public API ---

void registerTab() {
	modules::register_nav_button("tab_fonts", "Fonts", "font.svg", install_type::CASC);
}

void mounted() {
	auto& view = *core::view;

	view.fontPreviewPlaceholder = font_helpers::get_random_quote();
	view.fontPreviewText.clear();
	view.fontPreviewFontFamily.clear();

	// Change-detection is handled in render() by comparing selectionFonts[0] each frame.
}

// Forward declaration for pump_font_export (defined after render).
static void pump_font_export();

void render() {
	auto& view = *core::view;

	// Poll for pending async font load completion.
	pump_font_load();
	// Poll for pending async font export (one file per frame).
	pump_font_export();

	// --- Change-detection for selection (equivalent to watch on selectionFonts) ---
	if (!view.selectionFonts.empty()) {
		const std::string first = casc::listfile::stripFileEntry(view.selectionFonts[0].get<std::string>());
		if (!first.empty() && view.isBusy == 0 && first != prev_selection_first) {
			void* font = load_font_cached(first);
			if (font) {
				// Font already cached — use it directly.
				view.fontPreviewFontFamily = get_font_id(*casc::listfile::getByFilename(first));
				font_helpers::detect_glyphs_async(
					font,
					glyph_state,
					[&view](const std::string& ch) { view.fontPreviewText += ch; }
				);
			} else {
				// Need to load from CASC — do it asynchronously.
				load_font_async(first);
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

	if (app::layout::BeginTab("tab-fonts")) {

	auto regions = app::layout::CalcListTabRegions(false);

	// --- Left panel: List container (row 1, col 1) ---
	if (app::layout::BeginListContainer("fonts-list-container", regions)) {
		// Convert JSON items/selection to string vectors.
		const auto& items_str = core::cached_json_strings(view.listfileFonts, s_items_cache, s_items_cache_size);

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
			"listbox-fonts",
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
			"fonts", // persistscrollkey
			{},      // quickfilters
			false,   // nocopy
			listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionFonts.clear();
				for (const auto& s : new_sel)
					view.selectionFonts.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				listbox_context::handle_context_menu(ev.selection);
			}
		);

		// Context menu for generic listbox.
		context_menu::render(
			"ctx-fonts",
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
	if (app::layout::BeginStatusBar("fonts-status", regions)) {
		listbox::renderStatusBar("font", {}, listbox_state);
	}
	app::layout::EndStatusBar();

	// --- Filter bar (row 2, col 1) ---
	if (app::layout::BeginFilterBar("fonts-filter", regions)) {
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
	if (app::layout::BeginPreviewContainer("fonts-preview-container", regions)) {
		// Glyph grid.
		// CSS: .font-character-grid { bottom: 140px; } — fills all space except bottom 140px for preview input.
		const float gridHeight = std::max(50.0f, ImGui::GetContentRegionAvail().y - 140.0f);
		ImGui::BeginChild("font-character-grid", ImVec2(0, gridHeight), ImGuiChildFlags_Borders);

		// Set 2px spacing to match CSS: flex-wrap: wrap; gap: 2px
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 2.0f));
		float avail_width = ImGui::GetContentRegionAvail().x;
		float cursor_x = 0.0f;

		for (const auto& codepoint : glyph_state.detected_codepoints) {
			char utf8_buf[5] = {};
			ImTextCharToUtf8(utf8_buf, static_cast<unsigned int>(codepoint));

			// Each glyph cell is a small selectable button.
			// CSS: .font-glyph-cell { width: 32px; height: 32px; background: var(--background-alt); border-radius: 3px; }
			// CSS: .font-glyph-cell:hover { background: var(--font-alt); }
			ImGui::PushID(static_cast<int>(codepoint));
			if (ImGui::Selectable(utf8_buf, false, 0, ImVec2(32, 32))) {
				font_helpers::trigger_glyph_click(glyph_state, codepoint);
			}
			if (ImGui::IsItemHovered()) {
				char tooltip[32];
				std::snprintf(tooltip, sizeof(tooltip), "U+%04X", codepoint);
				ImGui::SetTooltip("%s", tooltip);
			}
			ImGui::PopID();

			// Manual wrap: continue on same line if next button fits.
			cursor_x += 32.0f + 2.0f;
			if (cursor_x + 32.0f <= avail_width) {
				ImGui::SameLine();
			} else {
				cursor_x = 0.0f;
			}
		}
		ImGui::NewLine();
		ImGui::PopStyleVar();
		ImGui::EndChild();

		// Font preview text input.
		// CSS: .font-preview-input { height: 120px; font-size: 32px; background: var(--background-dark); border: 1px solid var(--border); }
		const float previewInputHeight = std::min(120.0f, ImGui::GetContentRegionAvail().y);

		// JS: <textarea :style="{ fontFamily: $core.view.fontPreviewFontFamily }">
		// Push the loaded ImGui font so the preview textarea renders in the selected WoW font.
		void* preview_font = nullptr;
		{
			auto fit = loaded_fonts.find(view.fontPreviewFontFamily);
			if (fit != loaded_fonts.end())
				preview_font = fit->second;
		}
		if (preview_font)
			ImGui::PushFont(static_cast<ImFont*>(preview_font));

		char preview_buf[4096] = {};
		std::strncpy(preview_buf, view.fontPreviewText.c_str(), sizeof(preview_buf) - 1);
		if (ImGui::InputTextMultiline("##FontPreviewText", preview_buf, sizeof(preview_buf),
			ImVec2(-1, previewInputHeight)))
			view.fontPreviewText = preview_buf;

		if (preview_font)
			ImGui::PopFont();

		if (view.fontPreviewText.empty())
			ImGui::SetItemTooltip("%s", view.fontPreviewPlaceholder.c_str());
	}
	app::layout::EndPreviewContainer();

	// --- Bottom-right: Preview controls / export (row 2, col 2) ---
	if (app::layout::BeginPreviewControls("fonts-preview-controls", regions)) {
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

// --- Async export (one-file-per-frame, follows tab_models pattern) ---

struct PendingFontExport {
	std::vector<nlohmann::json> files;
	size_t next_index = 0;
	bool overwrite_files = false;
	std::optional<casc::ExportHelper> helper;
};

static std::optional<PendingFontExport> pending_font_export;

static void pump_font_export() {
	if (!pending_font_export.has_value())
		return;

	auto& task = *pending_font_export;
	auto& helper = task.helper.value();

	if (task.next_index == 0)
		helper.start();

	if (helper.isCancelled()) {
		pending_font_export.reset();
		return;
	}

	if (task.next_index >= task.files.size()) {
		helper.finish();
		pending_font_export.reset();
		return;
	}

	// Process one file per frame.
	const auto& sel_entry = task.files[task.next_index++];
	std::string file_name = casc::listfile::stripFileEntry(sel_entry.get<std::string>());
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
			BufferWrapper data = core::view->casc->getVirtualFileByName(file_name);
			data.writeToFile(export_path);
		} else {
			logging::write(std::format("Skipping font export {} (file exists, overwrite disabled)", export_path));
		}

		helper.mark(export_file_name, true);
	} catch (const std::exception& e) {
		helper.mark(export_file_name, false, e.what(), build_stack_trace("export_fonts", e));
	}
}

void export_fonts() {
	if (pending_font_export.has_value())
		return;

	auto& view = *core::view;
	const auto& user_selection = view.selectionFonts;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	PendingFontExport task;
	task.files = std::vector<nlohmann::json>(user_selection.begin(), user_selection.end());
	task.overwrite_files = view.config.value("overwriteFiles", false);
	task.helper.emplace(static_cast<int>(user_selection.size()), "file");

	pending_font_export = std::move(task);
}

} // namespace tab_fonts
