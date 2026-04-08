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
#include "../buffer.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <regex>
#include <unordered_map>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_raw {

// --- File-local state ---

// JS: let is_dirty = true;
static bool is_dirty = true;

// Change-detection for config watches.
static uint32_t prev_cascLocale = 0;

// --- Internal functions ---

// JS: const compute_raw_files = async (core) => { ... }
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
		// JS: const root_entries = core.view.casc.getValidRootEntries();
		// JS: core.view.listfileRaw = await listfile.renderListfile(root_entries, true);
		std::vector<uint32_t> root_entries = core::view->casc->getValidRootEntries();
		auto rendered = casc::listfile::renderListfile(root_entries, true);
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

// JS: const detect_raw_files = async (core) => { ... }
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

	// JS: using _lock = core.create_busy_lock();
	BusyLock _lock = core::create_busy_lock();

	// JS: const extension_map = new Map();
	std::unordered_map<uint32_t, std::string> extension_map;
	int current_index = 1;

	for (const uint32_t file_data_id : filtered_selection) {
		core::setToast("progress", std::format("Identifying file {} ({} / {})",
			file_data_id, current_index++, filtered_selection.size()));

		try {
			// JS: const data = await core.view.casc.getFile(file_data_id);
			BufferWrapper data = core::view->casc->getVirtualFileByID(file_data_id);

			for (const auto& check : constants::FILE_IDENTIFIERS) {
				// JS: if (data.startsWith(check.match))
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

// JS: const export_raw_files = async (core) => { ... }
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
				// JS: const data = await core.view.casc.getFileByName(file_name, true);
				// JS: await data.writeToFile(export_path);
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
	// JS: this.registerContextMenuOption('Browse Raw Client Files', 'fish.svg');
	modules::register_context_menu_option("tab_raw", "Browse Raw Client Files", "fish.svg",
		[]() { modules::set_active("tab_raw"); });
}

void mounted() {
	compute_raw_files();

	// JS: this.$core.view.$watch('config.cascLocale', () => { is_dirty = true; });
	// Store initial config value for change-detection in render().
	prev_cascLocale = core::view->config.value("cascLocale", static_cast<uint32_t>(casc::locale_flags::enUS));
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for cascLocale config (equivalent to watch on config.cascLocale) ---
	// JS: this.$core.view.$watch('config.cascLocale', () => { is_dirty = true; });
	const uint32_t current_cascLocale = view.config.value("cascLocale", static_cast<uint32_t>(casc::locale_flags::enUS));
	if (current_cascLocale != prev_cascLocale) {
		is_dirty = true;
		prev_cascLocale = current_cascLocale;
		compute_raw_files();
	}

	// List container with context menu.
	ImGui::BeginChild("raw-list-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox and ContextMenu component rendering will be wired when integration is complete.
	// JS: <Listbox v-model:selection="selectionRaw" :items="listfileRaw" ...>
	// JS: <ContextMenu :node="contextMenus.nodeListbox" ...>
	//   copy_file_paths, copy_listfile_format, copy_file_data_ids, copy_export_paths, open_export_directory
	ImGui::Text("Raw files: %zu", view.listfileRaw.size());
	ImGui::EndChild();

	// Tray.
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterRaw.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterRaw", filter_buf, sizeof(filter_buf)))
		view.userInputFilterRaw = filter_buf;

	const bool busy = view.isBusy > 0;
	if (busy) ImGui::BeginDisabled();

	if (ImGui::Button("Auto-Detect Selected"))
		detect_raw_files();

	ImGui::SameLine();

	if (ImGui::Button("Export Selected"))
		export_raw_files();

	if (busy) ImGui::EndDisabled();
}

void detect_raw() {
	detect_raw_files();
}

void export_raw() {
	export_raw_files();
}

} // namespace tab_raw
