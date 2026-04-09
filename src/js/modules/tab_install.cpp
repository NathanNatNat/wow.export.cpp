/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_install.h"
#include "../modules.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/install-manifest.h"
#include "../components/listbox.h"

#include <format>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include <imgui.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#endif

namespace tab_install {

// --- File-local state ---

// JS: let manifest = null;
static std::unique_ptr<casc::InstallManifest> manifest;

// JS: const MIN_STRING_LENGTH = 4;
static constexpr int MIN_STRING_LENGTH = 4;

static listbox::ListboxState listbox_install_state;
static listbox::ListboxState listbox_install_strings_state;

// --- Internal functions ---

/**
 * extract printable strings from binary data.
 * @param {Buffer} data
 * @returns {string[]}
 */
std::vector<std::string> extract_strings(const uint8_t* data, size_t size) {
	std::vector<std::string> strings;
	std::string current;

	for (size_t i = 0; i < size; i++) {
		const uint8_t byte = data[i];

		// printable ascii range (0x20-0x7E) plus tab (0x09)
		if ((byte >= 0x20 && byte <= 0x7E) || byte == 0x09) {
			current += static_cast<char>(byte);
		} else {
			if (current.length() >= MIN_STRING_LENGTH)
				strings.push_back(current);

			current.clear();
		}
	}

	// handle trailing string
	if (current.length() >= MIN_STRING_LENGTH)
		strings.push_back(current);

	return strings;
}

void update_install_listfile() {
	if (!manifest)
		return;

	auto& view = *core::view;
	std::vector<nlohmann::json> filtered;

	for (const auto& file : manifest->files) {
		bool matched = false;
		for (const auto& tag : view.installTags) {
			if (tag.contains("enabled") && tag["enabled"].get<bool>()) {
				const std::string label = tag["label"].get<std::string>();
				for (const auto& file_tag : file.tags) {
					if (file_tag == label) {
						matched = true;
						break;
					}
				}
			}
			if (matched)
				break;
		}

		if (matched) {
			std::string tag_list;
			for (size_t i = 0; i < file.tags.size(); i++) {
				if (i > 0)
					tag_list += ", ";
				tag_list += file.tags[i];
			}

			filtered.push_back(file.name + " [" + tag_list + "]");
		}
	}

	view.listfileInstall = std::move(filtered);
}

static void export_install_files() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionInstall;
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

		// Find the file in the manifest.
		const casc::InstallFile* file = nullptr;
		for (const auto& f : manifest->files) {
			if (f.name == file_name) {
				file = &f;
				break;
			}
		}

		if (!file) {
			helper.mark(file_name, false, "File not found in manifest");
			continue;
		}

		const std::string export_path = casc::ExportHelper::getExportPath(file_name);

		if (overwrite_files || !generics::fileExists(export_path)) {
			try {
				// JS: const data = await core.view.casc.getFile(0, false, false, true, false, file.hash);
				std::string enc_key = core::view->casc->getEncodingKeyForContentKey(file->hash);
				std::string cached_path = core::view->casc->_ensureFileInCache(enc_key, 0, false);
				BufferWrapper data = BufferWrapper::readFile(cached_path);
				data.writeToFile(export_path);

				helper.mark(file_name, true);
			} catch (const std::exception& e) {
				helper.mark(file_name, false, e.what());
			}
		} else {
			helper.mark(file_name, true);
			logging::write(std::format("Skipping file export {} (file exists, overwrite disabled)", export_path));
		}
	}

	helper.finish();
}

static void view_strings_impl() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionInstall;
	if (user_selection.size() != 1) {
		core::setToast("info", "Please select exactly one file to view strings.");
		return;
	}

	const std::string file_name = casc::listfile::stripFileEntry(user_selection[0].get<std::string>());

	// Find file in manifest.
	const casc::InstallFile* file = nullptr;
	for (const auto& f : manifest->files) {
		if (f.name == file_name) {
			file = &f;
			break;
		}
	}

	if (!file) {
		core::setToast("error", "File not found in manifest.");
		return;
	}

	core::setToast("progress", "Analyzing binary for strings...", {}, -1, false);
	view.isBusy++;

	try {
		// JS: const data = await core.view.casc.getFile(0, false, false, true, false, file.hash);
		// JS: data.processAllBlocks();
		// JS: const strings = extract_strings(data.raw);
		std::string enc_key = core::view->casc->getEncodingKeyForContentKey(file->hash);
		std::string cached_path = core::view->casc->_ensureFileInCache(enc_key, 0, false);
		BufferWrapper data = BufferWrapper::readFile(cached_path);
		const auto& raw = data.raw();
		std::vector<std::string> strings = extract_strings(raw.data(), raw.size());

		view.installStrings = strings;
		view.installStringsFileName = file_name;
		view.selectionInstallStrings.clear();
		view.userInputFilterInstallStrings.clear();
		view.installStringsView = true;

		logging::write(std::format("Extracted {} strings from {}", strings.size(), file_name));
	} catch (const std::exception& e) {
		core::setToast("error", std::string("Failed to analyze binary: ") + e.what());
		logging::write(std::format("Failed to extract strings from {}: {}", file_name, e.what()));
		view.isBusy--;
		return;
	}

	view.isBusy--;
	core::hideToast();
}

static void export_strings_impl() {
	auto& view = *core::view;
	const auto& strings = view.installStrings;
	if (strings.empty()) {
		core::setToast("info", "No strings to export.");
		return;
	}

	namespace fs = std::filesystem;
	const fs::path file_path(view.installStringsFileName);
	const std::string base_name = file_path.stem().string();
	const std::string export_path = casc::ExportHelper::getExportPath(base_name + "_strings.txt");

	try {
		generics::createDirectory(fs::path(export_path).parent_path());

		std::ofstream ofs(export_path);
		if (!ofs)
			throw std::runtime_error("Failed to open file for writing");

		for (size_t i = 0; i < strings.size(); i++) {
			if (i > 0)
				ofs << '\n';
			ofs << strings[i];
		}
		ofs.close();

		const std::string dir_path = fs::path(export_path).parent_path().string();
		// JS: { 'View in Explorer': () => nw.Shell.openItem(dir_path) }
		core::setToast("success", std::format("Exported {} strings.", strings.size()),
			{ {"View in Explorer", [dir_path]() { core::openInExplorer(dir_path); }} });
		logging::write(std::format("Exported {} strings to {}", strings.size(), export_path));
	} catch (const std::exception& e) {
		core::setToast("error", std::string("Failed to export strings: ") + e.what());
		logging::write(std::format("Failed to export strings: {}", e.what()));
	}
}

static void back_to_manifest_impl() {
	auto& view = *core::view;
	view.installStringsView = false;
	view.installStrings.clear();
	view.installStringsFileName.clear();
	view.selectionInstallStrings.clear();
	view.userInputFilterInstallStrings.clear();
}

// --- Public API ---

void registerTab() {
	// JS: this.registerContextMenuOption('Browse Install Manifest', 'clipboard-list.svg');
	modules::register_context_menu_option("tab_install", "Browse Install Manifest", "clipboard-list.svg",
		[]() { modules::set_active("tab_install"); });
}

void mounted() {
	// JS: this.$core.setToast('progress', 'Retrieving installation manifest...', null, -1, false);
	// JS: manifest = await this.$core.view.casc.getInstallManifest();
	core::setToast("progress", "Retrieving installation manifest...", {}, -1, false);

	auto inst = core::view->casc->getInstallManifest();
	manifest = std::make_unique<casc::InstallManifest>(std::move(inst));

	auto& view = *core::view;
	view.installTags.clear();
	for (const auto& tag : manifest->tags) {
		nlohmann::json tag_entry;
		tag_entry["label"] = tag.name;
		tag_entry["enabled"] = true;
		tag_entry["mask"] = tag.mask;
		view.installTags.push_back(std::move(tag_entry));
	}

	update_install_listfile();
	core::hideToast();
}

void render() {
	auto& view = *core::view;

	if (!view.installStringsView) {
		// Main manifest view.

		// List container.
		ImGui::BeginChild("install-list-container", ImVec2(-150, -ImGui::GetFrameHeightWithSpacing() * 2), ImGuiChildFlags_Borders);
		{
			std::vector<std::string> items_str;
			items_str.reserve(view.listfileInstall.size());
			for (const auto& item : view.listfileInstall)
				items_str.push_back(item.get<std::string>());

			std::vector<std::string> selection_str;
			for (const auto& s : view.selectionInstall)
				selection_str.push_back(s.get<std::string>());

			listbox::render(
				"listbox-install",
				items_str,
				view.userInputFilterInstall,
				selection_str,
				false,    // single
				true,     // keyinput
				view.config.value("regexFilters", false),
				listbox::CopyMode::Default,
				false,    // pasteselection
				false,    // copytrimwhitespace
				"file",   // unittype
				nullptr,  // overrideItems
				false,    // disable
				"install", // persistscrollkey
				{},       // quickfilters
				false,    // nocopy
				listbox_install_state,
				[&](const std::vector<std::string>& new_sel) {
					view.selectionInstall.clear();
					for (const auto& s : new_sel)
						view.selectionInstall.push_back(s);
				},
				nullptr  // no context menu
			);
		}
		ImGui::EndChild();

		// Tray.
		// Filter input.
		ImGui::BeginGroup();
		if (view.config.value("regexFilters", false))
			ImGui::TextUnformatted("Regex Enabled");

		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterInstall.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputText("##FilterInstall", filter_buf, sizeof(filter_buf)))
			view.userInputFilterInstall = filter_buf;

		// Buttons.
		const bool busy = view.isBusy > 0;
		if (busy) ImGui::BeginDisabled();
		if (ImGui::Button("View Strings"))
			view_strings_impl();

		ImGui::SameLine();

		if (ImGui::Button("Export Selected"))
			export_install_files();
		if (busy) ImGui::EndDisabled();
		ImGui::EndGroup();

		// Sidebar — tag checkboxes.
		ImGui::SameLine();
		ImGui::BeginChild("install-sidebar", ImVec2(0, 0), ImGuiChildFlags_Borders);
		bool tags_changed = false;
		for (auto& tag : view.installTags) {
			bool enabled = tag.value("enabled", true);
			if (ImGui::Checkbox(tag["label"].get<std::string>().c_str(), &enabled)) {
				tag["enabled"] = enabled;
				tags_changed = true;
			}
		}
		if (tags_changed)
			update_install_listfile();
		ImGui::EndChild();
	} else {
		// String viewer.

		// List container.
		ImGui::BeginChild("install-strings-list-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Borders);
		{
			std::vector<std::string> selection_str;
			for (const auto& s : view.selectionInstallStrings)
				selection_str.push_back(s.get<std::string>());

			listbox::render(
				"listbox-install-strings",
				view.installStrings,
				view.userInputFilterInstallStrings,
				selection_str,
				false,    // single
				true,     // keyinput
				view.config.value("regexFilters", false),
				listbox::CopyMode::Default,
				false,    // pasteselection
				false,    // copytrimwhitespace
				"string", // unittype
				nullptr,  // overrideItems
				false,    // disable
				"install-strings", // persistscrollkey
				{},       // quickfilters
				true,     // nocopy
				listbox_install_strings_state,
				[&](const std::vector<std::string>& new_sel) {
					view.selectionInstallStrings.clear();
					for (const auto& s : new_sel)
						view.selectionInstallStrings.push_back(s);
				},
				nullptr  // no context menu
			);
		}
		ImGui::EndChild();

		// Tray.
		if (view.config.value("regexFilters", false))
			ImGui::TextUnformatted("Regex Enabled");

		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterInstallStrings.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputText("##FilterInstallStrings", filter_buf, sizeof(filter_buf)))
			view.userInputFilterInstallStrings = filter_buf;

		if (ImGui::Button("Back to Manifest"))
			back_to_manifest_impl();

		ImGui::SameLine();

		const bool busy = view.isBusy > 0;
		if (busy) ImGui::BeginDisabled();
		if (ImGui::Button("Export Strings"))
			export_strings_impl();
		if (busy) ImGui::EndDisabled();

		// Sidebar — strings info.
		ImGui::BeginChild("install-strings-info", ImVec2(0, 0), ImGuiChildFlags_Borders);
		ImGui::TextUnformatted("Strings from:");
		ImGui::TextWrapped("%s", view.installStringsFileName.c_str());
		ImGui::EndChild();
	}
}

void export_install() {
	export_install_files();
}

void view_strings() {
	view_strings_impl();
}

void export_strings() {
	export_strings_impl();
}

void back_to_manifest() {
	back_to_manifest_impl();
}

} // namespace tab_install
