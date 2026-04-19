/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "legacy_tab_files.h"
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
#include <format>
#include <filesystem>
#include <fstream>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace legacy_tab_files {

// --- File-local state ---

static bool files_loaded = false;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

// Cached items string vector — only rebuilt when the source JSON changes.
static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

// --- Internal functions ---

static void load_files() {
	auto& view = *core::view;
	if (files_loaded || view.isBusy > 0)
		return;

	BusyLock _lock = core::create_busy_lock();

	try {
		mpq::MPQInstall* mpq = core::view->mpq.get();
		if (!mpq) return;
		auto files = mpq->getAllFiles();
		view.listfileRaw.assign(std::make_move_iterator(files.begin()), std::make_move_iterator(files.end()));

		files_loaded = true;
	} catch (const std::exception& e) {
		logging::write(std::format("failed to load legacy files: {}", e.what()));
	}
}

static void export_files() {
	auto& view = *core::view;
	const auto& selection = view.selectionRaw;
	if (selection.empty())
		return;

	BusyLock _lock = core::create_busy_lock();

	try {
		const std::string export_dir = view.config.value("exportDirectory", std::string(""));
		std::string last_export_path;

		for (const auto& sel_entry : selection) {
			const std::string display_path = sel_entry.get<std::string>();

			mpq::MPQInstall* mpq = core::view->mpq.get();
			std::optional<std::vector<uint8_t>> data = mpq ? mpq->getFile(display_path) : std::nullopt;

			if (!data) {
				logging::write(std::format("failed to read file: {}", display_path));
				continue;
			}

			namespace fs = std::filesystem;
			const fs::path output_path = fs::path(export_dir) / display_path;
			const fs::path output_dir = output_path.parent_path();

			fs::create_directories(output_dir);
			std::ofstream ofs(output_path, std::ios::binary);
			ofs.write(reinterpret_cast<const char*>(data->data()), static_cast<std::streamsize>(data->size()));

			last_export_path = output_path.string();
			logging::write(std::format("exported: {}", display_path));
		}

		if (!last_export_path.empty()) {
			namespace fs = std::filesystem;
			const std::string dir = fs::path(last_export_path).parent_path().string();
			std::vector<ToastAction> toast_opt = { {"View in Explorer", [dir]() { core::openInExplorer(dir); }} };

			if (selection.size() > 1)
				core::setToast("success", std::format("Successfully exported {} files.", selection.size()), toast_opt, -1);
			else
				core::setToast("success", std::format("Successfully exported {}.",
					fs::path(last_export_path).filename().string()), toast_opt, -1);
		}
	} catch (const std::exception& e) {
		logging::write(std::format("failed to export legacy files: {}", e.what()));
		core::setToast("error", "Failed to export files");
	}
}

// --- Public API ---

void registerTab() {
	modules::register_nav_button("legacy_tab_files", "Files", "file-lines.svg", install_type::MPQ);
}

void mounted() {
	load_files();
}

void render() {
	auto& view = *core::view;

	// List container with context menu.
	//     <Listbox v-model:selection="selectionRaw" :items="listfileRaw" :filter="userInputFilterRaw" ...>
	//     <ContextMenu :node="contextMenus.nodeListbox" ...>
	//       copy_file_paths, copy_export_paths, open_export_directory
	ImGui::BeginChild("legacy-files-list-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), ImGuiChildFlags_Borders);
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
			"listbox-legacy-files",
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
			"legacy-files", // persistscrollkey
			{},      // quickfilters
			false,   // nocopy
			listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionRaw.clear();
				for (const auto& s : new_sel)
					view.selectionRaw.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				listbox_context::handle_context_menu(ev.selection, true);
			}
		);

		// Context menu for generic listbox.
		context_menu::render(
			"ctx-legacy-files",
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

	// Tray.
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterRaw.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterFiles", filter_buf, sizeof(filter_buf)))
		view.userInputFilterRaw = filter_buf;

	ImGui::SameLine();

	const bool disabled = view.isBusy > 0 || view.selectionRaw.empty();
	if (disabled) app::theme::BeginDisabledButton();
	if (ImGui::Button("Export Selected"))
		export_selected();
	if (disabled) app::theme::EndDisabledButton();
}

void export_selected() {
	export_files();
}

} // namespace legacy_tab_files
