/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_text.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
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

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_text {

static std::string selected_file;

static std::string prev_selection_first;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

struct PendingTextPreview {
	std::string file_name;
	std::future<BufferWrapper> file_future;
	std::unique_ptr<BusyLock> busy_lock;
};

static std::optional<PendingTextPreview> pending_text_preview;

static void preview_text(const std::string& file_name) {
	pending_text_preview.reset();

	auto* casc = core::view->casc;
	if (!casc)
		return;

	PendingTextPreview task;
	task.file_name = file_name;
	task.busy_lock = std::make_unique<BusyLock>(core::create_busy_lock());
	task.file_future = std::async(std::launch::async, [casc, file_name]() {
		return casc->getVirtualFileByName(file_name);
	});
	pending_text_preview = std::move(task);
}

static void pump_text_preview() {
	if (!pending_text_preview.has_value())
		return;

	auto& task = *pending_text_preview;
	if (task.file_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	try {
		BufferWrapper file = task.file_future.get();
		core::view->textViewerSelectedText = file.readString();
		selected_file = task.file_name;
		prev_selection_first = task.file_name;
	} catch (const casc::EncryptionError& e) {
		core::setToast("error", std::format("The text file {} is encrypted with an unknown key ({}).", task.file_name, e.key), {}, -1);
		logging::write(std::format("Failed to decrypt texture {} ({})", task.file_name, e.key));
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to preview text file " + task.file_name,
		               { {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}

	pending_text_preview.reset();
}

void registerTab() {
	modules::register_nav_button("tab_text", "Text", "file-lines.svg", install_type::CASC);
}

void mounted() {
}

static void pump_text_export();

void render() {
	auto& view = *core::view;

	pump_text_preview();
	pump_text_export();

	if (!view.selectionText.empty()) {
		const std::string first = casc::listfile::stripFileEntry(view.selectionText[0].get<std::string>());
		if (view.isBusy == 0 && !first.empty() && first != prev_selection_first) {
			preview_text(first);
		}
	}

	if (app::layout::BeginTab("tab-text")) {

	auto regions = app::layout::CalcListTabRegions(false);

	if (app::layout::BeginListContainer("text-list-container", regions)) {
		const auto& items_str = core::cached_json_strings(view.listfileText, s_items_cache, s_items_cache_size);

		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionText)
			selection_str.push_back(s.get<std::string>());

		listbox::CopyMode copy_mode = listbox::CopyMode::Default;
		{
			std::string cm = view.config.value("copyMode", std::string("Default"));
			if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
			else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
		}

		listbox::render(
			"listbox-text",
			items_str,
			view.userInputFilterText,
			selection_str,
			false,
			true,
			view.config.value("regexFilters", false),
			copy_mode,
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"text file",
			nullptr,
			false,
			"text",
			view.textQuickFilters,
			false,
			listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionText.clear();
				for (const auto& s : new_sel)
					view.selectionText.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				listbox_context::handle_context_menu(ev.selection);
			}
		);

		context_menu::render(
			"ctx-text",
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

	if (app::layout::BeginStatusBar("text-status", regions)) {
		listbox::renderStatusBar("text file", view.textQuickFilters, listbox_state);
	}
	app::layout::EndStatusBar();

	if (app::layout::BeginFilterBar("text-filter", regions)) {
		if (view.config.value("regexFilters", false)) {
			ImGui::TextUnformatted("Regex Enabled");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", view.regexTooltip.c_str());
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterText.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputTextWithHint("##FilterText", "Filter text files...", filter_buf, sizeof(filter_buf)))
			view.userInputFilterText = filter_buf;
	}
	app::layout::EndFilterBar();

	if (app::layout::BeginPreviewContainer("text-preview-container", regions)) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 15.0f));
		ImGui::BeginChild("text-preview-background", ImVec2(0, 0), ImGuiChildFlags_None,
		                  ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::TextUnformatted(view.textViewerSelectedText.c_str());

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
	app::layout::EndPreviewContainer();

	if (app::layout::BeginPreviewControls("text-preview-controls", regions)) {
		if (ImGui::Button("Copy to Clipboard"))
			copy_text();

		ImGui::SameLine();

		const bool busy = view.isBusy > 0;
		if (busy) app::theme::BeginDisabledButton();
		if (ImGui::Button("Export Selected"))
			export_text();
		if (busy) app::theme::EndDisabledButton();
	}
	app::layout::EndPreviewControls();

	}
	app::layout::EndTab();
}

void copy_text() {
	ImGui::SetClipboardText(core::view->textViewerSelectedText.c_str());
	core::setToast("success", std::format("Copied contents of {} to the clipboard.", selected_file), {}, -1, true);
}

struct PendingTextExport {
	std::vector<nlohmann::json> files;
	size_t next_index = 0;
	bool overwrite_files = false;
	std::optional<casc::ExportHelper> helper;
};

static std::optional<PendingTextExport> pending_text_export;

static void pump_text_export() {
	if (!pending_text_export.has_value())
		return;

	auto& task = *pending_text_export;
	auto& helper = task.helper.value();

	if (task.next_index == 0)
		helper.start();

	if (helper.isCancelled()) {
		pending_text_export.reset();
		return;
	}

	if (task.next_index >= task.files.size()) {
		helper.finish();
		pending_text_export.reset();
		return;
	}

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
			logging::write(std::format("Skipping text export {} (file exists, overwrite disabled)", export_path));
		}

		helper.mark(export_file_name, true);
	} catch (const std::exception& e) {
		helper.mark(export_file_name, false, e.what(),
			std::string("pump_text_export: ") + e.what());
	}
}

void export_text() {
	if (pending_text_export.has_value())
		return;

	auto& view = *core::view;
	const auto& user_selection = view.selectionText;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	PendingTextExport task;
	task.files = std::vector<nlohmann::json>(user_selection.begin(), user_selection.end());
	task.overwrite_files = view.config.value("overwriteFiles", false);
	task.helper.emplace(static_cast<int>(user_selection.size()), "file");

	pending_text_export = std::move(task);
}

}
