/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "legacy_tab_textures.h"
#include "../log.h"
#include "../core.h"
#include "../buffer.h"
#include "../casc/blp.h"
#include "../ui/texture-exporter.h"
#include "../ui/listbox-context.h"
#include "../install-type.h"
#include "../modules.h"
#include "../mpq/mpq-install.h"
#include "../components/listbox.h"
#include "../components/context-menu.h"
#include "../components/menu-button.h"

#include <cstring>
#include <format>
#include <algorithm>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace legacy_tab_textures {

// --- File-local state ---

// JS: let selected_file = null;
static std::string selected_file;

// Change-detection for selection and config watches.
static std::string prev_selection_first;
static uint8_t prev_export_channel_mask = 0xFF;

static listbox::ListboxState legacy_tex_listbox_state;
static context_menu::ContextMenuState legacy_tex_ctx_state;
static menu_button::MenuButtonState legacy_tex_menu_state;

// --- Internal functions ---

// JS: const preview_texture = async (core, filename) => { ... }
static void preview_texture(const std::string& filename) {
	// Get extension.
	auto dot_pos = filename.rfind('.');
	std::string ext;
	if (dot_pos != std::string::npos) {
		ext = filename.substr(dot_pos);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	}

	BusyLock _lock = core::create_busy_lock();
	logging::write(std::format("previewing texture file {}", filename));

	try {
		// JS: const data = core.view.mpq.getFile(filename);
		mpq::MPQInstall* mpq = core::view->mpq.get();
		auto data = mpq ? mpq->getFile(filename) : std::nullopt;
		if (!data) {
			logging::write(std::format("failed to load texture: {}", filename));
			return;
		}

		if (ext == ".blp") {
			// JS: const buffer = Buffer.from(data);
			// JS: const wrapped = new BufferWrapper(buffer);
			// JS: const blp = new BLPFile(wrapped);
			BufferWrapper wrapped(std::move(*data));
			// TODO(conversion): BLP preview will be rendered when GL texture display is integrated.
			// BLPImage blp(std::move(wrapped));

			// JS: core.view.texturePreviewURL = blp.getDataURL(core.view.config.exportChannelMask);
			// JS: core.view.texturePreviewWidth = blp.width;
			// JS: core.view.texturePreviewHeight = blp.height;

			// JS: let info = '';
			// JS: switch (blp.encoding) { ... }
			// Encoding info logic:
			// case 1: info = "Palette"
			// case 2: info = "Compressed " + (alphaDepth > 1 ? (alphaEncoding === 7 ? "DXT5" : "DXT3") : "DXT1")
			// case 3: info = "ARGB"
			// default: info = "Unsupported [" + encoding + "]"

			// JS: core.view.texturePreviewInfo = `${blp.width}x${blp.height} (${info})`;
		} else if (ext == ".png" || ext == ".jpg") {
			// JS: const buffer = Buffer.from(data);
			// JS: const base64 = buffer.toString('base64');
			// JS: const mime_type = ext === '.png' ? 'image/png' : 'image/jpeg';
			// JS: const data_url = `data:${mime_type};base64,${base64}`;
			// TODO(conversion): PNG/JPG preview rendering will use stb_image + GL texture display.
			(void)data; // MPQ data is available; rendering wired in Phase 5.

			// JS: const img = new Image();
			// JS: img.onload = () => { core.view.texturePreviewWidth = img.width; ... };
			// JS: img.src = data_url;

			// JS: core.view.texturePreviewURL = data_url;
		}

		selected_file = filename;
	} catch (const std::exception& e) {
		logging::write(std::format("failed to preview legacy texture {}: {}", filename, e.what()));
		core::setToast("error", "unable to preview texture " + filename,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
	}
}

// JS: const refresh_blp_preview = (core) => { ... }
static void refresh_blp_preview() {
	if (selected_file.empty())
		return;

	auto dot_pos = selected_file.rfind('.');
	std::string ext;
	if (dot_pos != std::string::npos) {
		ext = selected_file.substr(dot_pos);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	}

	if (ext != ".blp")
		return;

	try {
		// JS: const data = core.view.mpq.getFile(selected_file);
		mpq::MPQInstall* mpq = core::view->mpq.get();
		auto data = mpq ? mpq->getFile(selected_file) : std::nullopt;
		if (data) {
			BufferWrapper wrapped(std::move(*data));
			// TODO(conversion): BLP re-render with updated channel mask will be wired in Phase 5.
			// BLPImage blp(std::move(wrapped));
			// core::view->texturePreviewURL = blp.getDataURL(core::view->config.value("exportChannelMask", 0b1111));
		}
	} catch (const std::exception& e) {
		logging::write(std::format("failed to refresh preview for {}: {}", selected_file, e.what()));
	}
}

// JS: const load_texture_list = async (core) => { ... }
static void load_texture_list() {
	if (core::view->listfileTextures.empty() && !core::view->isBusy) {
		BusyLock _lock = core::create_busy_lock();

		try {
			// JS: const blp_files = core.view.mpq.getFilesByExtension('.blp');
			// JS: const png_files = core.view.mpq.getFilesByExtension('.png');
			// JS: const jpg_files = core.view.mpq.getFilesByExtension('.jpg');
			// JS: core.view.listfileTextures = [...blp_files, ...png_files, ...jpg_files];
			mpq::MPQInstall* mpq = core::view->mpq.get();
			if (!mpq) return;
			auto blp_files = mpq->getFilesByExtension(".blp");
			auto png_files = mpq->getFilesByExtension(".png");
			auto jpg_files = mpq->getFilesByExtension(".jpg");
			std::vector<nlohmann::json> all_files;
			all_files.reserve(blp_files.size() + png_files.size() + jpg_files.size());
			for (auto& f : blp_files) all_files.push_back(std::move(f));
			for (auto& f : png_files) all_files.push_back(std::move(f));
			for (auto& f : jpg_files) all_files.push_back(std::move(f));
			core::view->listfileTextures = std::move(all_files);
		} catch (const std::exception& e) {
			logging::write(std::format("failed to load legacy textures: {}", e.what()));
		}
	}
}

// --- Public functions ---

// JS: register() { this.registerNavButton('Textures', 'image.svg', InstallType.MPQ); }
void registerTab() {
	// JS: this.registerNavButton('Textures', 'image.svg', InstallType.MPQ);
	modules::register_nav_button("legacy_tab_textures", "Textures", "image.svg", install_type::MPQ);
}

// JS: mounted()
void mounted() {
	// JS: await load_texture_list(this.$core);
	load_texture_list();

	// Initialize change-detection state.
	prev_selection_first.clear();
	prev_export_channel_mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
}

void render() {
	// --- Change-detection for selectionTextures watch ---
	// JS: this.$core.view.$watch('selectionTextures', async selection => { ... });
	{
		std::string current_first;
		if (!core::view->selectionTextures.empty())
			current_first = core::view->selectionTextures[0].get<std::string>();

		if (!current_first.empty() && current_first != prev_selection_first && !core::view->isBusy) {
			prev_selection_first = current_first;
			if (current_first != selected_file)
				preview_texture(current_first);
		} else if (current_first != prev_selection_first) {
			prev_selection_first = current_first;
		}
	}

	// --- Change-detection for config.exportChannelMask watch ---
	// JS: this.$core.view.$watch('config.exportChannelMask', () => { ... });
	{
		uint8_t current_mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
		if (current_mask != prev_export_channel_mask) {
			prev_export_channel_mask = current_mask;
			if (!core::view->isBusy)
				refresh_blp_preview();
		}
	}

	// --- Template rendering ---
	// JS: <div class="tab list-tab" id="legacy-tab-textures">
	auto& view = *core::view;

	// JS: <div class="list-container">
	//   Listbox with selectionTextures, listfileTextures, filter, context menu
	ImGui::BeginChild("legacy-tex-list", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, -ImGui::GetFrameHeightWithSpacing() * 2), ImGuiChildFlags_Borders);
	{
		// Convert json items to string array.
		std::vector<std::string> tex_strings;
		tex_strings.reserve(view.listfileTextures.size());
		for (const auto& t : view.listfileTextures)
			tex_strings.push_back(t.get<std::string>());

		// Build selection as string array.
		std::vector<std::string> sel_strings;
		for (const auto& s : view.selectionTextures)
			sel_strings.push_back(s.get<std::string>());

		listbox::render("##LegacyTexListbox", tex_strings,
			view.userInputFilterTextures, sel_strings,
			false,    // single
			true,     // keyinput
			view.config.value("regexFilters", false),
			listbox::CopyMode::Default,
			false,    // pasteselection
			false,    // copytrimwhitespace
			"texture",// unittype
			nullptr,  // overrideItems
			false,    // disable
			"legacy-textures", // persistscrollkey
			{".blp", ".png", ".jpg"}, // quickfilters
			false,    // nocopy
			legacy_tex_listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionTextures.clear();
				for (const auto& s : new_sel)
					view.selectionTextures.push_back(s);
			},
			[&](const listbox::ContextMenuEvent& ev) {
				nlohmann::json node;
				node["item"] = ev.item;
				node["selection"] = ev.selection;
				handle_listbox_context(node);
			});
	}
	ImGui::EndChild();

	// JS: <div class="filter">
	//   Regex info + filter input
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterTextures.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterLegacyTextures", filter_buf, sizeof(filter_buf)))
		view.userInputFilterTextures = filter_buf;

	ImGui::SameLine();

	// JS: <div class="preview-container">
	//   Preview info, channel mask toggles (R/G/B/A), texture preview image
	ImGui::BeginChild("legacy-tex-preview", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
	{
		// Texture preview info.
		if (!view.texturePreviewInfo.empty())
			ImGui::Text("%s", view.texturePreviewInfo.c_str());

		// Channel mask toggles (R/G/B/A).
		uint8_t mask = static_cast<uint8_t>(view.config.value("exportChannelMask", 0b1111));
		bool r_on = (mask & 0b0001) != 0;
		bool g_on = (mask & 0b0010) != 0;
		bool b_on = (mask & 0b0100) != 0;
		bool a_on = (mask & 0b1000) != 0;
		bool changed = false;
		if (ImGui::Checkbox("R##channel", &r_on)) { changed = true; }
		ImGui::SameLine();
		if (ImGui::Checkbox("G##channel", &g_on)) { changed = true; }
		ImGui::SameLine();
		if (ImGui::Checkbox("B##channel", &b_on)) { changed = true; }
		ImGui::SameLine();
		if (ImGui::Checkbox("A##channel", &a_on)) { changed = true; }

		if (changed) {
			uint8_t new_mask = 0;
			if (r_on) new_mask |= 0b0001;
			if (g_on) new_mask |= 0b0010;
			if (b_on) new_mask |= 0b0100;
			if (a_on) new_mask |= 0b1000;
			view.config["exportChannelMask"] = new_mask;
		}

		// Texture preview image display.
		if (!view.texturePreviewURL.empty()) {
			// TODO(conversion): GL texture display from texturePreviewURL will be wired when renderer is integrated.
			ImGui::TextWrapped("Texture loaded: %dx%d", view.texturePreviewWidth, view.texturePreviewHeight);
		}
	}
	ImGui::EndChild();

	// JS: <div class="preview-controls">
	//   MenuButton for export format + export action
	{
		std::vector<menu_button::MenuOption> tex_options;
		for (const auto& opt : view.menuButtonTextures)
			tex_options.push_back({ opt.label, opt.value });
		menu_button::render("##LegacyTexMenuButton", tex_options,
			view.config.value("exportTextureFormat", std::string("PNG")),
			view.isBusy > 0, false, legacy_tex_menu_state,
			[&](const std::string& val) { view.config["exportTextureFormat"] = val; },
			[&]() { export_textures(); });
	}
}

// JS: methods.handle_listbox_context(data)
static void handle_listbox_context(const nlohmann::json& data) {
	listbox_context::handle_context_menu(data, true);
}

// JS: methods.copy_file_paths(selection)
static void copy_file_paths(const nlohmann::json& selection) {
	listbox_context::copy_file_paths(selection);
}

// JS: methods.copy_export_paths(selection)
static void copy_export_paths(const nlohmann::json& selection) {
	listbox_context::copy_export_paths(selection);
}

// JS: methods.open_export_directory(selection)
static void open_export_directory(const nlohmann::json& selection) {
	listbox_context::open_export_directory(selection);
}

// JS: methods.export_textures()
void export_textures() {
	const auto& selected = core::view->selectionTextures;
	if (selected.empty()) {
		logging::write("no textures selected for export");
		return;
	}

	// JS: await textureExporter.exportFiles(selected, false, -1, true);
	// C++ signature: exportFiles(files, casc, mpq, isLocal, exportID)
	// For legacy MPQ textures: casc=nullptr, mpq=source, isLocal=true
	texture_exporter::exportFiles(selected, nullptr, core::view->mpq.get(), true, -1);
}

} // namespace legacy_tab_textures
