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
		// TODO(conversion): MPQ source will be wired when AppState.mpq is integrated.
		// mpq::MPQInstall* mpq = core::view->mpq;
		// auto data = mpq ? mpq->getFile(filename) : std::nullopt;
		// if (!data) {
		//     logging::write(std::format("failed to load texture: {}", filename));
		//     return;
		// }

		if (ext == ".blp") {
			// JS: const buffer = Buffer.from(data);
			// JS: const wrapped = new BufferWrapper(buffer);
			// JS: const blp = new BLPFile(wrapped);
			// TODO(conversion): BLP preview will be wired when MPQ file loading is integrated.
			// BufferWrapper wrapped(std::move(*data));
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
			// TODO(conversion): PNG/JPG preview will be wired when MPQ file loading is integrated.
			// BufferWrapper wrapped(std::move(*data));
			// std::string base64 = wrapped.toBase64();
			// std::string mime_type = (ext == ".png") ? "image/png" : "image/jpeg";
			// std::string data_url = std::format("data:{};base64,{}", mime_type, base64);

			// JS: const img = new Image();
			// JS: img.onload = () => { core.view.texturePreviewWidth = img.width; ... };
			// JS: img.src = data_url;
			// TODO(conversion): Image dimension detection from raw data will use stb_image.

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
		// TODO(conversion): MPQ source will be wired when AppState.mpq is integrated.
		// mpq::MPQInstall* mpq = core::view->mpq;
		// auto data = mpq ? mpq->getFile(selected_file) : std::nullopt;
		// if (data) {
		//     BufferWrapper wrapped(std::move(*data));
		//     BLPImage blp(std::move(wrapped));
		//     core::view->texturePreviewURL = blp.getDataURL(core::view->config.value("exportChannelMask", 0b1111));
		// }
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
			// TODO(conversion): MPQ source will be wired when AppState.mpq is integrated.
			// mpq::MPQInstall* mpq = core::view->mpq;
			// if (!mpq) return;
			// auto blp_files = mpq->getFilesByExtension(".blp");
			// auto png_files = mpq->getFilesByExtension(".png");
			// auto jpg_files = mpq->getFilesByExtension(".jpg");
			// std::vector<nlohmann::json> all_files;
			// all_files.reserve(blp_files.size() + png_files.size() + jpg_files.size());
			// for (auto& f : blp_files) all_files.push_back(std::move(f));
			// for (auto& f : png_files) all_files.push_back(std::move(f));
			// for (auto& f : jpg_files) all_files.push_back(std::move(f));
			// core::view->listfileTextures = std::move(all_files);
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
	// TODO(conversion): Full ImGui rendering will be implemented when UI integration is complete.

	// JS: <div class="list-container">
	//   Listbox with selectionTextures, listfileTextures, filter, context menu
	// JS: <div class="filter">
	//   Regex info + filter input
	// JS: <div class="preview-container">
	//   Preview info, channel mask toggles (R/G/B/A), texture preview image
	// JS: <div class="preview-controls">
	//   MenuButton for export format + export action
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
	// TODO(conversion): MPQ source will be wired when AppState.mpq is integrated.
	texture_exporter::exportFiles(selected, nullptr, nullptr, true, -1);
}

} // namespace legacy_tab_textures
