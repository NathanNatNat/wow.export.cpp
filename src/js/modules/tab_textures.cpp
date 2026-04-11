/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_textures.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../buffer.h"
#include "../png-writer.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
#include "../casc/blp.h"
#include "../casc/db2.h"
#include "../db/WDCReader.h"
#include "../ui/texture-exporter.h"
#include "../ui/listbox-context.h"
#include "../components/listbox.h"
#include "../components/context-menu.h"
#include "../install-type.h"
#include "../modules.h"
#include "../../app.h"

#include <webp/encode.h>

#include <cstring>
#include <format>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <optional>

#include <imgui.h>
#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace tab_textures {

// Helper to extract uint32_t from a FieldValue.
static uint32_t fieldToUint32(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<uint32_t>(*p);
	return 0;
}

// Helper to extract int from a FieldValue.
static int fieldToInt(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<int>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<int>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<int>(*p);
	return 0;
}

// Helper to extract std::string from a FieldValue.
static std::string fieldToString(const db::FieldValue& val) {
	if (auto* p = std::get_if<std::string>(&val))
		return *p;
	return "";
}

// --- Atlas data structures ---

struct AtlasRegion {
	std::string name;
	int width = 0;
	int height = 0;
	int left = 0;
	int top = 0;
};

struct AtlasEntry {
	int width = 0;
	int height = 0;
	std::vector<int> regions; // region IDs
};

// --- File-local state ---

// JS: const texture_atlas_entries = new Map();
static std::unordered_map<int, AtlasEntry> texture_atlas_entries;

// JS: const texture_atlas_regions = new Map();
static std::unordered_map<int, AtlasRegion> texture_atlas_regions;

// JS: const texture_atlas_map = new Map();
static std::unordered_map<uint32_t, int> texture_atlas_map;

// JS: let has_loaded_atlas_table = false;
static bool has_loaded_atlas_table = false;

// JS: let has_loaded_unknown_textures = false;
static bool has_loaded_unknown_textures = false;

// JS: let selected_file_data_id = 0;
static uint32_t selected_file_data_id = 0;

// JS: let resize_observer = null;
// In ImGui, resize observation is implicit (immediate mode).

// Change-detection for selection and config watches.
static uint32_t prev_selected_file_data_id = 0;
static uint8_t prev_export_channel_mask = 0xFF;
static bool prev_export_texture_alpha = false;
static bool prev_show_texture_atlas = false;
static listbox::ListboxState listbox_state;
static context_menu::ContextMenuState context_menu_state;

// Upload RGBA pixel data to an OpenGL texture, returning the texture ID.
// Deletes the previous texture if old_tex != 0.
static uint32_t upload_rgba_to_gl(const uint8_t* pixels, int w, int h, uint32_t old_tex = 0) {
	if (old_tex != 0) {
		GLuint old_gl = static_cast<GLuint>(old_tex);
		glDeleteTextures(1, &old_gl);
	}
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	return static_cast<uint32_t>(tex);
}

// --- Internal functions ---

// JS: const update_texture_atlas_overlay = (core) => { ... }
static void update_texture_atlas_overlay() {
	auto atlas_it = texture_atlas_map.find(selected_file_data_id);
	if (atlas_it != texture_atlas_map.end()) {
		auto entry_it = texture_atlas_entries.find(atlas_it->second);
		if (entry_it != texture_atlas_entries.end()) {
			const auto& entry = entry_it->second;
			core::view->textureAtlasOverlayWidth = entry.width;
			core::view->textureAtlasOverlayHeight = entry.height;

			nlohmann::json render_regions = nlohmann::json::array();
			for (const int id : entry.regions) {
				auto region_it = texture_atlas_regions.find(id);
				if (region_it != texture_atlas_regions.end()) {
					const auto& region = region_it->second;
					nlohmann::json r;
					r["id"] = id;
					r["name"] = region.name;
					r["width"] = std::format("{}%", (static_cast<float>(region.width) / entry.width) * 100);
					r["height"] = std::format("{}%", (static_cast<float>(region.height) / entry.height) * 100);
					r["top"] = std::format("{}%", (static_cast<float>(region.top) / entry.height) * 100);
					r["left"] = std::format("{}%", (static_cast<float>(region.left) / entry.width) * 100);
					render_regions.push_back(std::move(r));
				}
			}

			core::view->textureAtlasOverlayRegions.clear();
			for (auto& r : render_regions)
				core::view->textureAtlasOverlayRegions.push_back(std::move(r));
		}
	} else {
		core::view->textureAtlasOverlayRegions.clear();
	}
}

// JS: const preview_texture_by_id = async (core, file_data_id, texture = null) => { ... }
static void preview_texture_by_id_impl(uint32_t file_data_id, const std::string& texture_name) {
	std::string texture = texture_name;
	if (texture.empty()) {
		texture = casc::listfile::getByID(file_data_id);
		if (texture.empty())
			texture = casc::listfile::formatUnknownFile(file_data_id);
	}

	BusyLock _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", texture), {}, -1, false);
	logging::write(std::format("Previewing texture file {}", texture));

	try {
		// JS: const file = await core.view.casc.getFile(file_data_id);
		// JS: const blp = new BLPFile(file);
		BufferWrapper file_data = core::view->casc->getVirtualFileByID(file_data_id);
		casc::BLPImage blp(file_data);

		// JS: core.view.texturePreviewURL = blp.getDataURL(core.view.config.exportChannelMask);
		const uint8_t channel_mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
		core::view->texturePreviewURL = blp.getDataURL(channel_mask);
		core::view->texturePreviewWidth = static_cast<int>(blp.width);
		core::view->texturePreviewHeight = static_cast<int>(blp.height);

		// Upload BLP as GL texture for ImGui::Image display.
		std::vector<uint8_t> pixels = blp.toUInt8Array(0, channel_mask);
		core::view->texturePreviewTexID = upload_rgba_to_gl(
			pixels.data(), static_cast<int>(blp.width), static_cast<int>(blp.height),
			core::view->texturePreviewTexID);

		std::string info;
		switch (blp.encoding) {
			case 1:
				info = "Palette";
				break;
			case 2:
				info = "Compressed " + std::string(blp.alphaDepth > 1 ? (blp.alphaEncoding == 7 ? "DXT5" : "DXT3") : "DXT1");
				break;
			case 3:
				info = "ARGB";
				break;
			default:
				info = std::format("Unsupported [{}]", blp.encoding);
		}

		namespace fs = std::filesystem;
		core::view->texturePreviewInfo = std::format("{} {} x {} ({})",
			fs::path(texture).filename().string(), blp.width, blp.height, info);
		selected_file_data_id = file_data_id;

		// JS: update_texture_atlas_overlay(core);
		update_texture_atlas_overlay();

		core::hideToast();
	} catch (const casc::EncryptionError& e) {
		core::setToast("error", std::format("The texture {} is encrypted with an unknown key ({}).", texture, e.key), {}, -1);
		logging::write(std::format("Failed to decrypt texture {} ({})", texture, e.key));
	} catch (const std::exception& e) {
		// JS: core.setToast('error', 'Unable to preview texture ' + texture, { 'View Log': () => log.openRuntimeLog() }, -1);
		core::setToast("error", "Unable to preview texture " + texture,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}
}

// JS: const load_texture_atlas_data = async (core) => { ... }
static void load_texture_atlas_data() {
	auto& view = *core::view;
	if (!has_loaded_atlas_table && view.config.value("showTextureAtlas", false)) {
		core::progressLoadingScreen("Parsing texture atlases...");

		// JS: for (const [id, row] of await db2.UiTextureAtlas.getAllRows()) { ... }
		auto& ui_texture_atlas = casc::db2::getTable("UiTextureAtlas");
		auto& ui_texture_atlas_member = casc::db2::getTable("UiTextureAtlasMember");
		if (!ui_texture_atlas.isLoaded)
			ui_texture_atlas.parse();
		if (!ui_texture_atlas_member.isLoaded)
			ui_texture_atlas_member.parse();

		for (const auto& [id, row] : ui_texture_atlas.getAllRows()) {
			uint32_t file_data_id = 0;
			auto fdid_it = row.find("FileDataID");
			if (fdid_it != row.end())
				file_data_id = fieldToUint32(fdid_it->second);

			int atlas_width = 0, atlas_height = 0;
			auto aw_it = row.find("AtlasWidth");
			if (aw_it != row.end())
				atlas_width = fieldToInt(aw_it->second);
			auto ah_it = row.find("AtlasHeight");
			if (ah_it != row.end())
				atlas_height = fieldToInt(ah_it->second);

			texture_atlas_map[file_data_id] = static_cast<int>(id);
			texture_atlas_entries[static_cast<int>(id)] = { atlas_width, atlas_height, {} };
		}

		for (const auto& [id, row] : ui_texture_atlas_member.getAllRows()) {
			int atlas_id = 0;
			auto atlas_it = row.find("UiTextureAtlasID");
			if (atlas_it != row.end())
				atlas_id = fieldToInt(atlas_it->second);

			auto entry_it = texture_atlas_entries.find(atlas_id);
			if (entry_it != texture_atlas_entries.end()) {
				entry_it->second.regions.push_back(static_cast<int>(id));

				std::string committed_name;
				int width = 0, height = 0, left = 0, top = 0;

				auto cn_it = row.find("CommittedName");
				if (cn_it != row.end())
					committed_name = fieldToString(cn_it->second);
				auto w_it = row.find("Width");
				if (w_it != row.end())
					width = fieldToInt(w_it->second);
				auto h_it = row.find("Height");
				if (h_it != row.end())
					height = fieldToInt(h_it->second);
				auto cl_it = row.find("CommittedLeft");
				if (cl_it != row.end())
					left = fieldToInt(cl_it->second);
				auto ct_it = row.find("CommittedTop");
				if (ct_it != row.end())
					top = fieldToInt(ct_it->second);

				texture_atlas_regions[static_cast<int>(id)] = { committed_name, width, height, left, top };
			}
		}

		logging::write(std::format("Loaded {} texture atlases with {} regions",
			texture_atlas_entries.size(), texture_atlas_regions.size()));
		has_loaded_atlas_table = true;
	}
}

// JS: const reload_texture_atlas_data = async (core) => { ... }
static void reload_texture_atlas_data() {
	auto& view = *core::view;
	if (!has_loaded_atlas_table && view.config.value("showTextureAtlas", false) && view.isBusy == 0) {
		core::showLoadingScreen(1);

		try {
			load_texture_atlas_data();
			core::hideLoadingScreen();
		} catch (const std::exception& error) {
			core::hideLoadingScreen();
			logging::write(std::format("Failed to load texture atlas data: {}", error.what()));
			core::setToast("error", "Failed to load texture atlas data. Check the log for details.");
		}
	}
}

// JS: const update_texture_atlas_overlay_scaling = (core) => { ... }
// In ImGui, atlas overlay scaling is handled by the rendering code using available region size.
// The JS version manipulates DOM element styles; in ImGui this is implicit.

// JS: const attach_overlay_listener = (core) => { ... }
// In ImGui, no ResizeObserver needed; immediate-mode handles this per frame.

// JS: const update_texture_atlas_overlay = (core) => { ... }
// This is inlined into preview_texture_by_id_impl above.

// JS: const export_texture_atlas_regions = async (core, file_data_id) => { ... }
static void export_texture_atlas_regions_impl(uint32_t file_data_id) {
	auto atlas_map_it = texture_atlas_map.find(file_data_id);
	if (atlas_map_it == texture_atlas_map.end())
		return;

	auto atlas_it = texture_atlas_entries.find(atlas_map_it->second);
	if (atlas_it == texture_atlas_entries.end())
		return;

	const auto& atlas = atlas_it->second;

	const std::string file_name = casc::listfile::getByID(file_data_id);
	const std::string export_dir = casc::ExportHelper::replaceExtension(file_name);

	casc::ExportHelper helper(static_cast<int>(atlas.regions.size()), "texture");
	helper.start();

	std::string export_file_name = file_name;
	const std::string format = core::view->config.value("exportTextureFormat", std::string("PNG"));
	const std::string ext = format == "WEBP" ? ".webp" : ".png";
	const std::string mime_type = format == "WEBP" ? "image/webp" : "image/png";
	const float quality = core::view->config.value("exportWebPQuality", 0.9f);

	try {
		// JS: const data = await core.view.casc.getFile(file_data_id);
		// JS: const blp = new BLPFile(data);
		BufferWrapper file_data_buf = core::view->casc->getVirtualFileByID(file_data_id);
		casc::BLPImage blp(file_data_buf);

		// JS: const canvas = blp.toCanvas();
		// JS: const ctx = canvas.getContext('2d');
		// In C++, we get raw RGBA pixel data instead.
		std::vector<uint8_t> rgba_data = blp.toUInt8Array(0, 0b1111);
		const uint32_t blp_width = blp.width;
		const uint32_t blp_height = blp.height;

		for (const int region_id : atlas.regions) {
			if (helper.isCancelled())
				return;

			auto region_it = texture_atlas_regions.find(region_id);
			if (region_it == texture_atlas_regions.end())
				continue;

			const auto& region = region_it->second;

			namespace fs = std::filesystem;
			export_file_name = (fs::path(export_dir) / region.name).string();
			const std::string export_path = casc::ExportHelper::getExportPath(export_file_name + ext);

			// JS: const crop = ctx.getImageData(region.left, region.top, region.width, region.height);
			// Crop the RGBA data manually.
			std::vector<uint8_t> cropped(region.width * region.height * 4);
			for (int y = 0; y < region.height; y++) {
				const int src_row = (region.top + y) * static_cast<int>(blp_width) * 4;
				const int src_offset = src_row + region.left * 4;
				const int dst_offset = y * region.width * 4;
				if (src_offset + region.width * 4 <= static_cast<int>(rgba_data.size()))
					std::memcpy(cropped.data() + dst_offset, rgba_data.data() + src_offset, region.width * 4);
			}

			// JS: const buf = await BufferWrapper.fromCanvas(save_canvas, mime_type, quality);
			// JS: await buf.writeToFile(export_path);
			if (mime_type == "image/webp") {
				uint8_t* output = nullptr;
				size_t outputSize = 0;
				int webp_quality = static_cast<int>(quality * 100.0f);
				if (webp_quality >= 100) {
					outputSize = WebPEncodeLosslessRGBA(
						cropped.data(), region.width, region.height,
						region.width * 4, &output);
				} else {
					outputSize = WebPEncodeRGBA(
						cropped.data(), region.width, region.height,
						region.width * 4, static_cast<float>(webp_quality), &output);
				}
				if (outputSize > 0 && output) {
					BufferWrapper webpBuf = BufferWrapper::from(std::span<const uint8_t>(output, outputSize));
					WebPFree(output);
					generics::createDirectory(std::filesystem::path(export_path).parent_path());
					webpBuf.writeToFile(export_path);
				} else {
					if (output) WebPFree(output);
					throw std::runtime_error("WebP encoding failed");
				}
			} else {
				PNGWriter png(static_cast<uint32_t>(region.width), static_cast<uint32_t>(region.height));
				std::memcpy(png.getPixelData().data(), cropped.data(), cropped.size());
				generics::createDirectory(std::filesystem::path(export_path).parent_path());
				png.write(export_path);
			}

			helper.mark(export_file_name, true);
		}
	} catch (const std::exception& e) {
		helper.mark(export_file_name, false, e.what());
	}

	helper.finish();
}

// JS: const is_baked_npc_texture = (core) => { ... }
static bool is_baked_npc_texture() {
	auto& view = *core::view;
	if (view.selectionTextures.empty())
		return false;

	const std::string first = casc::listfile::stripFileEntry(view.selectionTextures[0].get<std::string>());
	if (first.empty())
		return false;

	// Case-insensitive check for "textures/bakednpctextures/"
	std::string lower = first;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return lower.starts_with("textures/bakednpctextures/");
}

// --- Public API ---

void registerTab() {
	// JS: this.registerNavButton('Textures', 'image.svg', InstallType.CASC);
	modules::register_nav_button("tab_textures", "Textures", "image.svg", install_type::CASC);
}

void mounted() {
	auto& view = *core::view;

	// JS: await this.initialize();
	const bool needs_unknown_textures = view.config.value("enableUnknownFiles", false) && !has_loaded_unknown_textures;
	const bool needs_atlas_data = !has_loaded_atlas_table && view.config.value("showTextureAtlas", false);

	if (needs_unknown_textures || needs_atlas_data) {
		int step_count = 0;
		if (needs_unknown_textures)
			step_count += 2;
		if (needs_atlas_data)
			step_count += 1;

		core::showLoadingScreen(step_count);

		if (needs_unknown_textures) {
			core::progressLoadingScreen("Loading texture file data...");
			core::progressLoadingScreen("Loading unknown textures...");
			casc::listfile::loadUnknownTextures();
			has_loaded_unknown_textures = true;
		}

		if (needs_atlas_data)
			load_texture_atlas_data();

		core::hideLoadingScreen();
	}

	// JS: attach_overlay_listener(this.$core);
	// In ImGui, overlay listeners are not needed (immediate mode).

	// Store initial config values for change-detection.
	prev_export_channel_mask = static_cast<uint8_t>(view.config.value("exportChannelMask", 0b1111));
	prev_export_texture_alpha = view.config.value("exportTextureAlpha", false);
	prev_show_texture_atlas = view.config.value("showTextureAtlas", false);

	// JS: this.$core.registerDropHandler({ ext: ['.blp'], ... });
	core::registerDropHandler({
		{".blp"},
		[&view]() -> std::string {
			return std::format("Export textures as {}", view.config.value("exportTextureFormat", std::string("PNG")));
		},
		[](const std::string& file) {
			// JS: process: files => textureExporter.exportFiles(files, true)
			nlohmann::json entry;
			entry["fileName"] = file;
			texture_exporter::exportFiles({entry}, core::view->casc, nullptr, true);
		}
	});
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for config watches ---

	// JS: this.$core.view.$watch('config.exportTextureAlpha', () => { ... });
	const bool current_export_alpha = view.config.value("exportTextureAlpha", false);
	if (current_export_alpha != prev_export_texture_alpha) {
		if (view.isBusy == 0 && selected_file_data_id > 0)
			preview_texture_by_id_impl(selected_file_data_id, "");
		prev_export_texture_alpha = current_export_alpha;
	}

	// JS: this.$core.view.$watch('config.exportChannelMask', () => { ... });
	const uint8_t current_channel_mask = static_cast<uint8_t>(view.config.value("exportChannelMask", 0b1111));
	if (current_channel_mask != prev_export_channel_mask) {
		if (view.isBusy == 0 && selected_file_data_id > 0)
			preview_texture_by_id_impl(selected_file_data_id, "");
		prev_export_channel_mask = current_channel_mask;
	}

	// JS: this.$core.view.$watch('config.showTextureAtlas', async () => { ... });
	const bool current_show_atlas = view.config.value("showTextureAtlas", false);
	if (current_show_atlas != prev_show_texture_atlas) {
		reload_texture_atlas_data();
		// JS: update_texture_atlas_overlay(core) — just update overlay data, don't reload the full texture.
		update_texture_atlas_overlay();
		prev_show_texture_atlas = current_show_atlas;
	}

	// --- Change-detection for selection (equivalent to watch on selectionTextures) ---
	if (!view.selectionTextures.empty()) {
		const std::string first = casc::listfile::stripFileEntry(view.selectionTextures[0].get<std::string>());
		if (!first.empty() && view.isBusy == 0) {
			const auto file_data_id_opt = casc::listfile::getByFilename(first);
			if (file_data_id_opt.has_value()) {
				const uint32_t fid = *file_data_id_opt;
				if (selected_file_data_id != fid)
					preview_texture_by_id_impl(fid, "");
			}
		}
	}

	// --- Template rendering ---

	// Override texture toast.
	// JS: <div id="toast" v-if="!$core.view.toast && $core.view.overrideTextureList.length > 0" class="progress">
	if (!view.toast.has_value() && !view.overrideTextureList.empty()) {
		ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), "Filtering textures for item: %s", view.overrideTextureName.c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Remove")) {
			// JS: this.$core.view.removeOverrideTextures();
			view.overrideTextureList.clear();
			view.overrideTextureName.clear();
		}
	}

	// List container with context menu.
	// JS: <div class="list-container">
	//     <Listbox v-model:selection="selectionTextures" :items="listfileTextures" ...>
	//     <ContextMenu :node="contextMenus.nodeListbox" ...>
	ImGui::BeginChild("textures-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), ImGuiChildFlags_Borders);
	{
		// Convert JSON items/selection to string vectors.
		std::vector<std::string> items_str;
		items_str.reserve(view.listfileTextures.size());
		for (const auto& item : view.listfileTextures)
			items_str.push_back(item.get<std::string>());

		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionTextures)
			selection_str.push_back(s.get<std::string>());

		listbox::CopyMode copy_mode = listbox::CopyMode::Default;
		{
			std::string cm = view.config.value("copyMode", std::string("Default"));
			if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
			else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
		}

		listbox::render(
			"listbox-textures",
			items_str,
			view.userInputFilterTextures,
			selection_str,
			false,   // single
			true,    // keyinput
			view.config.value("regexFilters", false),
			copy_mode,
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"texture", // unittype
			nullptr, // overrideItems
			false,   // disable
			"textures", // persistscrollkey
			{},      // quickfilters
			false,   // nocopy
			listbox_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionTextures.clear();
				for (const auto& s : new_sel)
					view.selectionTextures.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				listbox_context::handle_context_menu(ev.selection);
			}
		);

		// Context menu for generic listbox.
		context_menu::render(
			"ctx-textures",
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

	ImGui::SameLine();

	// Right side — filter, preview, controls.
	ImGui::BeginGroup();

	// Filter.
	// JS: <div class="filter">
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterTextures.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterTextures", filter_buf, sizeof(filter_buf)))
		view.userInputFilterTextures = filter_buf;

	// Preview container.
	// JS: <div class="preview-container">
	//     <div class="preview-info" ...>
	//     <ul class="preview-channels" ...>
	//     <div class="preview-background" id="texture-preview" ...>
	if (!view.texturePreviewInfo.empty())
		ImGui::TextUnformatted(view.texturePreviewInfo.c_str());

	// Channel mask toggles.
	// JS: <ul class="preview-channels" v-if="$core.view.texturePreviewURL.length > 0">
	if (!view.texturePreviewURL.empty()) {
		int mask = view.config.value("exportChannelMask", 0b1111);

		bool r = (mask & 0b0001) != 0;
		bool g = (mask & 0b0010) != 0;
		bool b = (mask & 0b0100) != 0;
		bool a = (mask & 0b1000) != 0;

		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1, 0, 0, 1));
		if (ImGui::Checkbox("R", &r)) { mask ^= 0b0001; view.config["exportChannelMask"] = mask; }
		ImGui::PopStyleColor();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0, 1, 0, 1));
		if (ImGui::Checkbox("G", &g)) { mask ^= 0b0010; view.config["exportChannelMask"] = mask; }
		ImGui::PopStyleColor();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.3f, 0.3f, 1, 1));
		if (ImGui::Checkbox("B", &b)) { mask ^= 0b0100; view.config["exportChannelMask"] = mask; }
		ImGui::PopStyleColor();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1, 1, 1, 1));
		if (ImGui::Checkbox("A", &a)) { mask ^= 0b1000; view.config["exportChannelMask"] = mask; }
		ImGui::PopStyleColor();
	}

	// Texture preview area.
	ImGui::BeginChild("texture-preview-area", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// JS: <div class="image" :style="{ 'background-image': 'url(' + texturePreviewURL + ')' }">
	if (view.texturePreviewTexID != 0) {
		// Fit the texture into the available area while preserving aspect ratio.
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const float tex_w = static_cast<float>(view.texturePreviewWidth);
		const float tex_h = static_cast<float>(view.texturePreviewHeight);
		const float scale = std::min(avail.x / tex_w, avail.y / tex_h);
		const ImVec2 img_size(tex_w * scale, tex_h * scale);

		const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
		ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.texturePreviewTexID)), img_size);

		// Atlas overlay regions drawn on top of the texture image.
		// JS: <div id="atlas-overlay" v-if="$core.view.config.showTextureAtlas">
		if (view.config.value("showTextureAtlas", false) && !view.textureAtlasOverlayRegions.empty()) {
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			for (const auto& region : view.textureAtlasOverlayRegions) {
				if (!region.contains("name"))
					continue;

				// Region positions are stored as percentage strings (e.g. "25.5%").
				auto parse_pct = [](const nlohmann::json& j, const std::string& key) -> float {
					if (!j.contains(key)) return 0.0f;
					const std::string s = j[key].get<std::string>();
					return std::stof(s) / 100.0f;
				};

				const float pct_left   = parse_pct(region, "left");
				const float pct_top    = parse_pct(region, "top");
				const float pct_width  = parse_pct(region, "width");
				const float pct_height = parse_pct(region, "height");

				const ImVec2 rect_min(cursor_pos.x + pct_left * img_size.x,
				                      cursor_pos.y + pct_top  * img_size.y);
				const ImVec2 rect_max(rect_min.x + pct_width  * img_size.x,
				                      rect_min.y + pct_height * img_size.y);

				draw_list->AddRect(rect_min, rect_max, IM_COL32(255, 255, 0, 200), 0.0f, 0, 1.0f);
				const std::string name = region["name"].get<std::string>();
				draw_list->AddText(ImVec2(rect_min.x + 2, rect_min.y + 1), IM_COL32(255, 255, 0, 255), name.c_str());
			}
		}
	} else if (!view.texturePreviewURL.empty()) {
		ImGui::Text("[Texture Preview: %dx%d]", view.texturePreviewWidth, view.texturePreviewHeight);
	}

	ImGui::EndChild();

	// Preview controls.
	// JS: <div class="preview-controls">
	bool show_atlas = view.config.value("showTextureAtlas", false);
	if (ImGui::Checkbox("Atlas Regions", &show_atlas))
		view.config["showTextureAtlas"] = show_atlas;

	ImGui::SameLine();

	// JS: <input v-if="is_baked_npc_texture()" type="button" value="Apply to Character" ...>
	if (is_baked_npc_texture()) {
		const bool busy = view.isBusy > 0;
		if (busy) app::theme::BeginDisabledButton();
		if (ImGui::Button("Apply to Character")) {
			// JS: methods.apply_baked_npc_texture()
			BusyLock _lock = core::create_busy_lock();
			core::setToast("progress", "loading baked npc texture...", {}, -1, false);

			try {
				const std::string first = casc::listfile::stripFileEntry(view.selectionTextures[0].get<std::string>());
				const auto file_data_id_opt = casc::listfile::getByFilename(first);
				if (file_data_id_opt.has_value()) {
					// JS: const file = await this.$core.view.casc.getFile(file_data_id);
					// JS: const blp = new BLPFile(file);
					// JS: view.chrCustBakedNPCTexture = blp;
					// In C++, store the file data ID so tab_characters can load the BLP on demand.
					core::view->chrCustBakedNPCTexture = file_data_id_opt.value();
					core::setToast("success", "baked npc texture applied to character", {}, 3000);
					logging::write(std::format("applied baked npc texture {} to character", first));
				}
			} catch (const std::exception& e) {
				core::setToast("error", "failed to load baked npc texture", {}, -1);
				logging::write(std::format("failed to load baked npc texture: {}", e.what()));
			}
		}
		if (busy) app::theme::EndDisabledButton();
		ImGui::SameLine();
	}

	// JS: <input v-if="showTextureAtlas" type="button" value="Export Atlas Regions" ...>
	if (view.config.value("showTextureAtlas", false)) {
		const bool busy = view.isBusy > 0;
		if (busy) app::theme::BeginDisabledButton();
		if (ImGui::Button("Export Atlas Regions"))
			export_atlas_regions();
		if (busy) app::theme::EndDisabledButton();
		ImGui::SameLine();
	}

	// JS: <MenuButton :options="menuButtonTextures" :default="config.exportTextureFormat" ... @click="export_textures">
	{
		const bool busy = view.isBusy > 0;
		if (busy) app::theme::BeginDisabledButton();
		const std::string export_format = view.config.value("exportTextureFormat", std::string("PNG"));
		if (ImGui::Button(std::format("Export as {}", export_format).c_str()))
			export_textures();
		if (busy) app::theme::EndDisabledButton();
	}

	ImGui::EndGroup();
}

void previewTextureByID(uint32_t file_data_id, const std::string& texture) {
	preview_texture_by_id_impl(file_data_id, texture);
}

void export_textures() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionTextures;
	if (!user_selection.empty()) {
		texture_exporter::exportFiles(user_selection);
	} else if (selected_file_data_id > 0) {
		std::vector<nlohmann::json> files;
		files.push_back(selected_file_data_id);
		texture_exporter::exportFiles(files);
	} else {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
	}
}

void export_atlas_regions() {
	export_texture_atlas_regions_impl(selected_file_data_id);
}

void goToTexture(uint32_t fileDataID) {
	auto* view = core::view;
	modules::setActive("tab_textures");

	// Directly preview the requested file, even if it's not in the listfile.
	previewTextureByID(fileDataID);

	// Since we're doing a direct preview, we need to reset the users current
	// selection, so if they hit export, they get the expected result.
	view->selectionTextures.clear();

	// If the user has fileDataIDs shown, filter by that.
	if (view->config.contains("regexFilters") && view->config["regexFilters"].get<bool>())
		view->userInputFilterTextures = "\\[" + std::to_string(fileDataID) + "\\]";
	else
		view->userInputFilterTextures = "[" + std::to_string(fileDataID) + "]";
}

} // namespace tab_textures