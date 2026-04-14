/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_raw.h"
#include "../modules.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../constants.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/locale-flags.h"
#include "../ui/listbox-context.h"
#include "../components/listbox.h"
#include "../components/context-menu.h"
#include "../buffer.h"
#include "../../app.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <regex>
#include <unordered_map>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_raw {

// --- File-local state ---

static bool is_dirty = true;

// Change-detection for config watches.
static uint32_t prev_cascLocale = 0;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

// --- Internal functions ---

static void compute_raw_files() {
	if (!is_dirty)
		return;

	is_dirty = false;

	auto& view = *core::view;
	const bool enable_unknown = view.config.value("enableUnknownFiles", false);

	core::setToast("progress", enable_unknown
		? "Scanning game client for all files..."
		: "Scanning game client for all known files...");
	generics::redraw();

	if (enable_unknown) {
		std::vector<uint32_t> root_entries = core::view->casc->getValidRootEntries();
		auto rendered = casc::listfile::renderListfile(std::optional<std::vector<uint32_t>>(root_entries), true);
		view.listfileRaw.clear();
		for (auto& s : rendered)
			view.listfileRaw.push_back(std::move(s));
	} else {
		auto rendered = casc::listfile::renderListfile();
		view.listfileRaw.clear();
		for (auto& s : rendered)
			view.listfileRaw.push_back(std::move(s));
	}

	core::setToast("success", std::format("Found {} files in the game client", view.listfileRaw.size()));
}

static void detect_raw_files() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionRaw;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to detect; you should do that first.");
		return;
	}

	std::vector<uint32_t> filtered_selection;
	static const std::regex unknown_pattern(R"(^unknown/(\d+)(\.[a-zA-Z_]+)?$)");

	for (const auto& entry : user_selection) {
		std::string file_name = casc::listfile::stripFileEntry(entry.get<std::string>());
		std::smatch match;

		if (std::regex_match(file_name, match, unknown_pattern))
			filtered_selection.push_back(static_cast<uint32_t>(std::stoi(match[1].str())));
	}

	if (filtered_selection.empty()) {
		core::setToast("info", "You haven't selected any unknown files to identify.");
		return;
	}

	BusyLock _lock = core::create_busy_lock();

	std::unordered_map<uint32_t, std::string> extension_map;
	int current_index = 1;

	for (const uint32_t file_data_id : filtered_selection) {
		core::setToast("progress", std::format("Identifying file {} ({} / {})",
			file_data_id, current_index++, filtered_selection.size()));

		try {
			BufferWrapper data = core::view->casc->getVirtualFileByID(file_data_id);

			for (const auto& check : constants::FILE_IDENTIFIERS) {
				std::vector<std::string_view> patterns(check.matches.begin(), check.matches.begin() + std::min(static_cast<size_t>(check.match_count), check.matches.size()));
				if (data.startsWith(patterns)) {
					extension_map[file_data_id] = std::string(check.ext);
					logging::write(std::format("Successfully identified file {} as {}", file_data_id, check.ext));
					break;
				}
			}
		} catch (const std::exception&) {
			logging::write(std::format("Failed to identify file {} due to CASC error", file_data_id));
		}
	}

	if (!extension_map.empty()) {
		std::vector<std::pair<uint32_t, std::string>> entries(extension_map.begin(), extension_map.end());
		casc::listfile::ingestIdentifiedFiles(entries);
		is_dirty = true;
		compute_raw_files();

		if (extension_map.size() == 1) {
			auto it = extension_map.begin();
			core::setToast("success", std::format("{} has been identified as a {} file", it->first, it->second));
		} else {
			core::setToast("success", std::format("Successfully identified {} files", extension_map.size()));
		}

		core::setToast("success", std::format("{} of the {} selected files have been identified and added to relevant file lists",
			extension_map.size(), filtered_selection.size()));
	} else {
		core::setToast("info", "Unable to identify any of the selected files.");
	}
}

static void export_raw_files() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionRaw;
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

		const std::string export_path = casc::ExportHelper::getExportPath(export_file_name);

		if (overwrite_files || !generics::fileExists(export_path)) {
			try {
				BufferWrapper data = core::view->casc->getVirtualFileByName(file_name);
				data.writeToFile(export_path);
				helper.mark(export_file_name, true);
			} catch (const std::exception& e) {
				helper.mark(export_file_name, false, e.what());
			}
		} else {
			helper.mark(export_file_name, true);
			logging::write(std::format("Skipping file export {} (file exists, overwrite disabled)", export_path));
		}
	}

	helper.finish();
}

// --- Public API ---

void registerTab() {
	modules::register_context_menu_option("tab_raw", "Browse Raw Client Files", "fish.svg",
		[]() { modules::set_active("tab_raw"); });
}

void mounted() {
	compute_raw_files();

	// Store initial config value for change-detection in render().
	prev_cascLocale = core::view->config.value("cascLocale", static_cast<uint32_t>(casc::locale_flags::enUS));
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for cascLocale config (equivalent to watch on config.cascLocale) ---
	const uint32_t current_cascLocale = view.config.value("cascLocale", static_cast<uint32_t>(casc::locale_flags::enUS));
	if (current_cascLocale != prev_cascLocale) {
		is_dirty = true;
		prev_cascLocale = current_cascLocale;
		compute_raw_files();
	}

	if (app::layout::BeginTab("tab-raw")) {

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const ImVec2 cursor = ImGui::GetCursorPos();
	constexpr float FILTER_H = app::layout::FILTER_BAR_HEIGHT; // 60px
	constexpr float MARGIN = 10.0f;

	// --- List container (row 1, single column) ---
	const float listTopM = 20.0f;
	const float listLeftM = 20.0f;
	const float listRightM = 10.0f;
	const float topH = avail.y - FILTER_H;

	ImGui::SetCursorPos(ImVec2(cursor.x + listLeftM, cursor.y + listTopM));
	ImGui::BeginChild("raw-list-container",
		ImVec2(avail.x - listLeftM - listRightM, topH - listTopM));
	{
		// Convert JSON items/selection to string vectors.
		std::vector<std::string> items_str;
		items_str.reserve(view.listfileRaw.size());
		for (const auto& item : view.listfileRaw)
			items_str.push_back(item.get<std::string>());

		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionRaw)
			selection_str.push_back(s.get<std::string>());

		listbox::CopyMode copy_mode = listbox::CopyMode::Default;
		{
			std::string cm = view.config.value("copyMode", std::string("Default"));
			if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
			else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
		}

		listbox::render(
			"listbox-raw",
			items_str,
			view.userInputFilterRaw,
			selection_str,
			false,   // single
			true,    // keyinput
			view.config.value("regexFilters", false),
			copy_mode,
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"file",  // unittype
			nullptr, // overrideItems
			false,   // disable
			"raw",   // persistscrollkey
			{},      // quickfilters
			false,   // nocopy
			listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionRaw.clear();
				for (const auto& s : new_sel)
					view.selectionRaw.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				listbox_context::handle_context_menu(ev.selection);
			}
		);

		// Context menu for generic listbox.
		context_menu::render(
			"ctx-raw",
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

	// --- Tray (row 2) ---
	ImGui::SetCursorPos(ImVec2(cursor.x, cursor.y + topH));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(MARGIN, 0.0f));
	ImGui::BeginChild("raw-tray", ImVec2(avail.x, FILTER_H), ImGuiChildFlags_None,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Vertically center content.
	float padY = (FILTER_H - ImGui::GetFrameHeight()) * 0.5f;
	if (padY > 0.0f)
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);

	if (view.config.value("regexFilters", false)) {
		ImGui::TextUnformatted("Regex Enabled");
		ImGui::SameLine();
	}

	const bool busy = view.isBusy > 0;

	// Calculate button widths to give filter the remaining space.
	float btnDetectW = ImGui::CalcTextSize("Auto-Detect Selected").x + ImGui::GetStyle().FramePadding.x * 2;
	float btnExportW = ImGui::CalcTextSize("Export Selected").x + ImGui::GetStyle().FramePadding.x * 2;
	float buttonsW = btnDetectW + 5.0f + btnExportW + 5.0f;
	float filterW = ImGui::GetContentRegionAvail().x - buttonsW;
	if (filterW < 50.0f) filterW = 50.0f;

	ImGui::SetNextItemWidth(filterW);
	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterRaw.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterRaw", filter_buf, sizeof(filter_buf)))
		view.userInputFilterRaw = filter_buf;

	ImGui::SameLine(0.0f, 5.0f);
	if (busy) app::theme::BeginDisabledButton();

	if (ImGui::Button("Auto-Detect Selected"))
		detect_raw_files();

	ImGui::SameLine(0.0f, 5.0f);

	if (ImGui::Button("Export Selected"))
		export_raw_files();

	if (busy) app::theme::EndDisabledButton();

	ImGui::EndChild();
	ImGui::PopStyleVar(); // WindowPadding

	} // if BeginTab
	app::layout::EndTab();
}

void detect_raw() {
	detect_raw_files();
}

void export_raw() {
	export_raw_files();
}

} // namespace tab_raw
