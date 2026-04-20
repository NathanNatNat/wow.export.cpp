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

#include <chrono>
#include <cstring>
#include <format>
#include <filesystem>
#include <future>
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

// Cached items string vector — only rebuilt when the source JSON changes.
static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

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

// --- Async detect (follows tab_models pattern) ---

struct PendingDetectTask {
	std::vector<uint32_t> file_ids;
	size_t next_index = 0;
	std::unordered_map<uint32_t, std::string> extension_map;
	std::unique_ptr<BusyLock> busy_lock;
};

static std::optional<PendingDetectTask> pending_detect_task;

static void pump_detect_task() {
	if (!pending_detect_task.has_value())
		return;

	auto& task = *pending_detect_task;

	if (task.next_index >= task.file_ids.size()) {
		// Done detecting — process results.
		if (!task.extension_map.empty()) {
			std::vector<std::pair<uint32_t, std::string>> entries(task.extension_map.begin(), task.extension_map.end());
			casc::listfile::ingestIdentifiedFiles(entries);
			is_dirty = true;
			compute_raw_files();

			if (task.extension_map.size() == 1) {
				auto it = task.extension_map.begin();
				core::setToast("success", std::format("{} has been identified as a {} file", it->first, it->second));
			} else {
				core::setToast("success", std::format("Successfully identified {} files", task.extension_map.size()));
			}

			core::setToast("success", std::format("{} of the {} selected files have been identified and added to relevant file lists",
				task.extension_map.size(), task.file_ids.size()));
		} else {
			core::setToast("info", "Unable to identify any of the selected files.");
		}
		pending_detect_task.reset();
		return;
	}

	// Process one file per frame.
	const uint32_t file_data_id = task.file_ids[task.next_index++];
	core::setToast("progress", std::format("Identifying file {} ({} / {})",
		file_data_id, task.next_index, task.file_ids.size()));

	try {
		BufferWrapper data = core::view->casc->getVirtualFileByID(file_data_id);

		for (const auto& check : constants::FILE_IDENTIFIERS) {
			std::vector<std::string_view> patterns(check.matches.begin(), check.matches.begin() + std::min(static_cast<size_t>(check.match_count), check.matches.size()));
			if (data.startsWith(patterns)) {
				task.extension_map[file_data_id] = std::string(check.ext);
				logging::write(std::format("Successfully identified file {} as {}", file_data_id, check.ext));
				break;
			}
		}
	} catch (const std::exception&) {
		logging::write(std::format("Failed to identify file {} due to CASC error", file_data_id));
	}
}

static void detect_raw_files() {
	if (pending_detect_task.has_value())
		return;

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

	PendingDetectTask task;
	task.file_ids = std::move(filtered_selection);
	task.busy_lock = std::make_unique<BusyLock>(core::create_busy_lock());
	pending_detect_task = std::move(task);
}

// --- Async export (one-file-per-frame, follows tab_models pattern) ---

struct PendingRawExport {
	std::vector<nlohmann::json> files;
	size_t next_index = 0;
	bool overwrite_files = false;
	std::optional<casc::ExportHelper> helper;
};

static std::optional<PendingRawExport> pending_raw_export;

static void pump_raw_export() {
	if (!pending_raw_export.has_value())
		return;

	auto& task = *pending_raw_export;
	auto& helper = task.helper.value();

	if (task.next_index == 0)
		helper.start();

	if (helper.isCancelled()) {
		pending_raw_export.reset();
		return;
	}

	if (task.next_index >= task.files.size()) {
		helper.finish();
		pending_raw_export.reset();
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

	const std::string export_path = casc::ExportHelper::getExportPath(export_file_name);

	if (task.overwrite_files || !generics::fileExists(export_path)) {
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

static void export_raw_files() {
	if (pending_raw_export.has_value())
		return;

	auto& view = *core::view;
	const auto& user_selection = view.selectionRaw;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	PendingRawExport task;
	task.files = std::vector<nlohmann::json>(user_selection.begin(), user_selection.end());
	task.overwrite_files = view.config.value("overwriteFiles", false);
	task.helper.emplace(static_cast<int>(user_selection.size()), "file");
	pending_raw_export = std::move(task);
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

	// Poll for pending async detect/export tasks (one file per frame).
	pump_detect_task();
	pump_raw_export();

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
		const auto& items_str = core::cached_json_strings(view.listfileRaw, s_items_cache, s_items_cache_size);

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
