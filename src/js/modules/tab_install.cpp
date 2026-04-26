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
#include "../../app.h"

#include <chrono>
#include <format>
#include <filesystem>
#include <fstream>
#include <future>
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

static std::unique_ptr<casc::InstallManifest> manifest;

static constexpr int MIN_STRING_LENGTH = 4;

static listbox::ListboxState listbox_install_state;
static listbox::ListboxState listbox_install_strings_state;

// Cached items string vector — only rebuilt when the source JSON changes.
static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

// --- Internal functions ---

/**
 * extract printable strings from binary data.
 * @param {Buffer} data
 * @returns {string[]}
 */
static std::vector<std::string> extract_strings(const uint8_t* data, size_t size) {
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

static void update_install_listfile() {
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

// --- Async export (one-file-per-frame, follows tab_models pattern) ---

struct PendingInstallExport {
	std::vector<nlohmann::json> files;
	size_t next_index = 0;
	bool overwrite_files = false;
	std::optional<casc::ExportHelper> helper;
};

static std::optional<PendingInstallExport> pending_install_export;

static void pump_install_export() {
	if (!pending_install_export.has_value())
		return;

	auto& task = *pending_install_export;
	auto& helper = task.helper.value();

	if (task.next_index == 0)
		helper.start();

	if (helper.isCancelled()) {
		pending_install_export.reset();
		return;
	}

	if (task.next_index >= task.files.size()) {
		helper.finish();
		pending_install_export.reset();
		return;
	}

	// Process one file per frame.
	const auto& sel_entry = task.files[task.next_index++];
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
		return;
	}

	const std::string export_path = casc::ExportHelper::getExportPath(file_name);

	if (task.overwrite_files || !generics::fileExists(export_path)) {
		try {
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

static void export_install_files() {
	if (pending_install_export.has_value())
		return;

	auto& view = *core::view;
	const auto& user_selection = view.selectionInstall;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	PendingInstallExport task;
	task.files = std::vector<nlohmann::json>(user_selection.begin(), user_selection.end());
	task.overwrite_files = view.config.value("overwriteFiles", false);
	task.helper.emplace(static_cast<int>(user_selection.size()), "file");
	pending_install_export = std::move(task);
}

// --- Async view strings (background CASC fetch) ---

struct PendingViewStrings {
	std::string file_name;
	std::future<BufferWrapper> file_future;
	std::unique_ptr<BusyLock> busy_lock;
};

static std::optional<PendingViewStrings> pending_view_strings;

static void pump_view_strings() {
	if (!pending_view_strings.has_value())
		return;

	auto& task = *pending_view_strings;
	if (task.file_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	auto& view = *core::view;

	try {
		BufferWrapper data = task.file_future.get();
		const auto& raw = data.raw();
		std::vector<std::string> strings = extract_strings(raw.data(), raw.size());

		view.installStrings = strings;
		view.installStringsFileName = task.file_name;
		view.selectionInstallStrings.clear();
		view.userInputFilterInstallStrings.clear();
		view.installStringsView = true;

		logging::write(std::format("Extracted {} strings from {}", strings.size(), task.file_name));
		core::hideToast();
	} catch (const std::exception& e) {
		core::setToast("error", std::string("Failed to analyze binary: ") + e.what());
		logging::write(std::format("Failed to extract strings from {}: {}", task.file_name, e.what()));
	}

	pending_view_strings.reset();
}

static void view_strings_impl() {
	if (pending_view_strings.has_value())
		return;

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

	auto* casc = core::view->casc;
	std::string hash = file->hash;

	PendingViewStrings task_data;
	task_data.file_name = file_name;
	task_data.busy_lock = std::make_unique<BusyLock>(core::create_busy_lock());
	task_data.file_future = std::async(std::launch::async, [casc, hash]() {
		std::string enc_key = casc->getEncodingKeyForContentKey(hash);
		std::string cached_path = casc->_ensureFileInCache(enc_key, 0, false);
		return BufferWrapper::readFile(cached_path);
	});
	pending_view_strings = std::move(task_data);
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
	modules::register_context_menu_option("tab_install", "Browse Install Manifest", "clipboard-list.svg",
		[]() { modules::set_active("tab_install"); });
}

void mounted() {
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

	// Poll for pending async tasks (one file per frame).
	pump_install_export();
	pump_view_strings();

	if (app::layout::BeginTab("tab-install")) {

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const ImVec2 cursor = ImGui::GetCursorPos();
	constexpr float FILTER_H = app::layout::FILTER_BAR_HEIGHT; // 60px
	constexpr float SIDEBAR_W = app::layout::SIDEBAR_WIDTH;    // 210px

	const float gridW = avail.x - SIDEBAR_W;
	const float topH = avail.y - FILTER_H;

	if (!view.installStringsView) {
		// Main manifest view.

		// --- List container (row 1, col 1) ---
		constexpr float listTopM = app::layout::LIST_MARGIN_TOP;     // 20px
		constexpr float listLeftM = app::layout::LIST_MARGIN_LEFT;   // 20px
		constexpr float listRightM = app::layout::LIST_MARGIN_RIGHT; // 10px

		ImGui::SetCursorPos(ImVec2(cursor.x + listLeftM, cursor.y + listTopM));
		ImGui::BeginChild("install-list-container",
			ImVec2(gridW - listLeftM - listRightM, topH - listTopM));
		{
			const auto& items_str = core::cached_json_strings(view.listfileInstall, s_items_cache, s_items_cache_size);

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
				"install file", // unittype
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

		// --- Tray (row 2, col 1) ---
		constexpr float TRAY_M = 10.0f;
		ImGui::SetCursorPos(ImVec2(cursor.x, cursor.y + topH));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(TRAY_M, 0.0f));
		ImGui::BeginChild("install-tray", ImVec2(gridW, FILTER_H), ImGuiChildFlags_None,
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

		// Calculate button widths so filter gets remaining space.
		float btnStringsW = ImGui::CalcTextSize("View Strings").x + ImGui::GetStyle().FramePadding.x * 2;
		float btnExportW = ImGui::CalcTextSize("Export Selected").x + ImGui::GetStyle().FramePadding.x * 2;
		float buttonsW = 5.0f + btnStringsW + 5.0f + btnExportW;
		float filterW = ImGui::GetContentRegionAvail().x - buttonsW;
		if (filterW < 50.0f) filterW = 50.0f;

		ImGui::SetNextItemWidth(filterW);
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterInstall.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputText("##FilterInstall", filter_buf, sizeof(filter_buf)))
			view.userInputFilterInstall = filter_buf;

		ImGui::SameLine(0.0f, 5.0f);
		if (busy) ImGui::BeginDisabled();
		if (ImGui::Button("View Strings"))
			view_strings_impl();
		ImGui::SameLine(0.0f, 5.0f);
		if (ImGui::Button("Export Selected"))
			export_install_files();
		if (busy) ImGui::EndDisabled();

		ImGui::EndChild();
		ImGui::PopStyleVar(); // WindowPadding

		// --- Sidebar (col 2, spanning both rows) ---
		constexpr float sidebarTopM = app::layout::SIDEBAR_MARGIN_TOP;     // 20px
		constexpr float sidebarPadR = app::layout::SIDEBAR_PADDING_RIGHT;  // 20px
		ImGui::SetCursorPos(ImVec2(cursor.x + gridW, cursor.y + sidebarTopM));
		ImGui::BeginChild("install-sidebar",
			ImVec2(SIDEBAR_W - sidebarPadR, avail.y - sidebarTopM));
		{
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
		}
		ImGui::EndChild();

	} else {
		// String viewer.

		// --- List container (row 1, col 1) ---
		constexpr float listTopM = app::layout::LIST_MARGIN_TOP;
		constexpr float listLeftM = app::layout::LIST_MARGIN_LEFT;
		constexpr float listRightM = app::layout::LIST_MARGIN_RIGHT;

		ImGui::SetCursorPos(ImVec2(cursor.x + listLeftM, cursor.y + listTopM));
		ImGui::BeginChild("install-strings-list-container",
			ImVec2(gridW - listLeftM - listRightM, topH - listTopM));
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
				false,    // nocopy
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

		// --- Tray (row 2, col 1) ---
		constexpr float TRAY_M = 10.0f;
		ImGui::SetCursorPos(ImVec2(cursor.x, cursor.y + topH));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(TRAY_M, 0.0f));
		ImGui::BeginChild("install-strings-tray", ImVec2(gridW, FILTER_H), ImGuiChildFlags_None,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		float padY = (FILTER_H - ImGui::GetFrameHeight()) * 0.5f;
		if (padY > 0.0f)
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);

		if (view.config.value("regexFilters", false)) {
			ImGui::TextUnformatted("Regex Enabled");
			ImGui::SameLine();
		}

		const bool busy = view.isBusy > 0;

		float btnBackW = ImGui::CalcTextSize("Back to Manifest").x + ImGui::GetStyle().FramePadding.x * 2;
		float btnExportW = ImGui::CalcTextSize("Export Strings").x + ImGui::GetStyle().FramePadding.x * 2;
		float buttonsW = 5.0f + btnBackW + 5.0f + btnExportW;
		float filterW = ImGui::GetContentRegionAvail().x - buttonsW;
		if (filterW < 50.0f) filterW = 50.0f;

		ImGui::SetNextItemWidth(filterW);
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterInstallStrings.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputText("##FilterInstallStrings", filter_buf, sizeof(filter_buf)))
			view.userInputFilterInstallStrings = filter_buf;

		ImGui::SameLine(0.0f, 5.0f);
		if (ImGui::Button("Back to Manifest"))
			back_to_manifest_impl();
		ImGui::SameLine(0.0f, 5.0f);
		if (busy) ImGui::BeginDisabled();
		if (ImGui::Button("Export Strings"))
			export_strings_impl();
		if (busy) ImGui::EndDisabled();

		ImGui::EndChild();
		ImGui::PopStyleVar(); // WindowPadding

		// --- Sidebar: strings info (col 2, spanning both rows) ---
		constexpr float sidebarTopM = app::layout::SIDEBAR_MARGIN_TOP;
		constexpr float sidebarPadR = app::layout::SIDEBAR_PADDING_RIGHT;
		ImGui::SetCursorPos(ImVec2(cursor.x + gridW, cursor.y + sidebarTopM));
		ImGui::BeginChild("install-strings-info",
			ImVec2(SIDEBAR_W - sidebarPadR, avail.y - sidebarTopM));
		{
			ImGui::TextDisabled("Strings from:");

			ImGui::Spacing();

			ImGui::TextWrapped("%s", view.installStringsFileName.c_str());
		}
		ImGui::EndChild();
	}

	} // if BeginTab
	app::layout::EndTab();
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
