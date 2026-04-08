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
#include "../install-type.h"
#include "../modules.h"

#include <cstring>
#include <format>
#include <filesystem>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_text {

// --- File-local state ---

// JS: let selected_file = null;
static std::string selected_file;

// Change-detection for selectionText.
static std::string prev_selection_first;

// --- Public API ---

void registerTab() {
	// JS: this.registerNavButton('Text', 'file-lines.svg', InstallType.CASC);
	modules::register_nav_button("tab_text", "Text", "file-lines.svg", install_type::CASC);
}

void mounted() {
	// JS: this.$core.view.$watch('selectionText', async selection => { ... });
	// Change-detection is handled in render() by comparing selectionText[0] each frame.
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for selection preview (equivalent to watch on selectionText) ---
	if (!view.selectionText.empty()) {
		const std::string first = casc::listfile::stripFileEntry(view.selectionText[0].get<std::string>());
		if (view.isBusy == 0 && !first.empty() && first != prev_selection_first) {
			try {
				// JS: const file = await this.$core.view.casc.getFileByName(first);
				// JS: this.$core.view.textViewerSelectedText = file.readString(undefined, 'utf8');
				// TODO(conversion): CASC getFileByName will be wired when CASC integration is complete.
				selected_file = first;
				prev_selection_first = first;
			} catch (const casc::EncryptionError& e) {
				core::setToast("error", std::format("The text file {} is encrypted with an unknown key ({}).", first, e.key), {}, -1);
				logging::write(std::format("Failed to decrypt texture {} ({})", first, e.key));
			} catch (const std::exception& e) {
				core::setToast("error", "Unable to preview text file " + first, {}, -1);
				logging::write(std::format("Failed to open CASC file: {}", e.what()));
			}
		}
	}

	// List container with context menu.
	ImGui::BeginChild("text-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox and ContextMenu component rendering will be wired when integration is complete.
	// JS: <Listbox v-model:selection="selectionText" :items="listfileText" :filter="userInputFilterText"
	//      :quickfilters="textQuickFilters" ...>
	// JS: <ContextMenu :node="contextMenus.nodeListbox" ...>
	//   copy_file_paths, copy_listfile_format, copy_file_data_ids, copy_export_paths, open_export_directory
	ImGui::Text("Text files: %zu", view.listfileText.size());
	ImGui::EndChild();

	ImGui::SameLine();

	// Right side — filter, preview, controls.
	ImGui::BeginGroup();

	// Filter.
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterText.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterText", filter_buf, sizeof(filter_buf)))
		view.userInputFilterText = filter_buf;

	// Preview container.
	ImGui::BeginChild("text-preview-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
	// JS: <pre>{{ $core.view.textViewerSelectedText }}</pre>
	ImGui::TextWrapped("%s", view.textViewerSelectedText.c_str());
	ImGui::EndChild();

	// Preview controls.
	if (ImGui::Button("Copy to Clipboard"))
		copy_text();

	ImGui::SameLine();

	const bool busy = view.isBusy > 0;
	if (busy) ImGui::BeginDisabled();
	if (ImGui::Button("Export Selected"))
		export_text();
	if (busy) ImGui::EndDisabled();

	ImGui::EndGroup();
}

void copy_text() {
	// JS: const clipboard = nw.Clipboard.get();
	// JS: clipboard.set(this.$core.view.textViewerSelectedText, 'text');
	ImGui::SetClipboardText(core::view->textViewerSelectedText.c_str());
	core::setToast("success", std::format("Copied contents of {} to the clipboard.", selected_file), {}, -1, true);
}

void export_text() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionText;
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
				logging::write(std::format("Skipping text export {} (file exists, overwrite disabled)", export_path));
			}

			helper.mark(export_file_name, true);
		} catch (const std::exception& e) {
			helper.mark(export_file_name, false, e.what());
		}
	}

	helper.finish();
}

} // namespace tab_text
