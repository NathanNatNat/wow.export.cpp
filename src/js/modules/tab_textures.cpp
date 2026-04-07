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
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
#include "../casc/blp.h"
#include "../casc/db2.h"
#include "../ui/texture-exporter.h"
#include "../ui/listbox-context.h"
#include "../install-type.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <optional>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_textures {

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
// TODO(conversion): In ImGui, resize observation is implicit (immediate mode).

// Change-detection for selection and config watches.
static uint32_t prev_selected_file_data_id = 0;
static uint8_t prev_export_channel_mask = 0xFF;
static bool prev_show_texture_atlas = false;

// --- Internal functions ---

// JS: const preview_texture_by_id = async (core, file_data_id, texture = null) => { ... }
static void preview_texture_by_id_impl(uint32_t file_data_id, const std::string& texture_name) {
	std::string texture = texture_name;
	if (texture.empty()) {
		texture = casc::listfile::getByID(file_data_id);
		if (texture.empty())
			texture = casc::listfile::formatUnknownFile(file_data_id);
	}

	BusyLock _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", texture), nullptr, -1, false);
	logging::write(std::format("Previewing texture file {}", texture));

	try {
		// JS: const file = await core.view.casc.getFile(file_data_id);
		// JS: const blp = new BLPFile(file);
		// TODO(conversion): CASC getFile will be wired when CASC integration is complete.
		BufferWrapper file_data;
		casc::BLPImage blp(file_data);

		// JS: core.view.texturePreviewURL = blp.getDataURL(core.view.config.exportChannelMask);
		const uint8_t channel_mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
		core::view->texturePreviewURL = blp.getDataURL(channel_mask);
		core::view->texturePreviewWidth = static_cast<int>(blp.width);
		core::view->texturePreviewHeight = static_cast<int>(blp.height);

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
		// Atlas overlay update.
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

		core::hideToast();
	} catch (const casc::EncryptionError& e) {
		core::setToast("error", std::format("The texture {} is encrypted with an unknown key ({}).", texture, e.key), nullptr, -1);
		logging::write(std::format("Failed to decrypt texture {} ({})", texture, e.key));
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to preview texture " + texture, nullptr, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}
}

// JS: const load_texture_atlas_data = async (core) => { ... }
static void load_texture_atlas_data() {
	auto& view = *core::view;
	if (!has_loaded_atlas_table && view.config.value("showTextureAtlas", false)) {
		core::progressLoadingScreen("Parsing texture atlases...");

		// JS: for (const [id, row] of await db2.UiTextureAtlas.getAllRows()) { ... }
		// TODO(conversion): DB2 UiTextureAtlas/UiTextureAtlasMember iteration will be wired when DB2 is fully integrated.
		auto& ui_texture_atlas = casc::db2::getTable("UiTextureAtlas");
		auto& ui_texture_atlas_member = casc::db2::getTable("UiTextureAtlasMember");
		(void)ui_texture_atlas;
		(void)ui_texture_atlas_member;

		// Placeholder: when wired, iterate:
		// for (const auto& [id, row] : ui_texture_atlas.getAllRows()) {
		//     texture_atlas_map[row.at("FileDataID")] = id;
		//     texture_atlas_entries[id] = { row.at("AtlasWidth"), row.at("AtlasHeight"), {} };
		// }
		// for (const auto& [id, row] : ui_texture_atlas_member.getAllRows()) {
		//     auto it = texture_atlas_entries.find(row.at("UiTextureAtlasID"));
		//     if (it != texture_atlas_entries.end()) {
		//         it->second.regions.push_back(id);
		//         texture_atlas_regions[id] = { row.at("CommittedName"), row.at("Width"),
		//             row.at("Height"), row.at("CommittedLeft"), row.at("CommittedTop") };
		//     }
		// }

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
// TODO(conversion): In ImGui, atlas overlay scaling is handled by the rendering code using available region size.
// The JS version manipulates DOM element styles; in ImGui this is implicit.

// JS: const attach_overlay_listener = (core) => { ... }
// TODO(conversion): In ImGui, no ResizeObserver needed; immediate-mode handles this per frame.

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

	try {
		// JS: const data = await core.view.casc.getFile(file_data_id);
		// JS: const blp = new BLPFile(data);
		// TODO(conversion): CASC getFile will be wired when CASC integration is complete.
		BufferWrapper file_data_buf;
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
			// TODO(conversion): BufferWrapper::fromCanvas equivalent (PNG/WebP encoding from raw RGBA) will be wired.
			// For now the export pipeline is preserved structurally.

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
	// TODO(conversion): Nav button registration will be wired when the module system is integrated.
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
	// TODO(conversion): In ImGui, overlay listeners are not needed (immediate mode).

	// Store initial config values for change-detection.
	prev_export_channel_mask = static_cast<uint8_t>(view.config.value("exportChannelMask", 0b1111));
	prev_show_texture_atlas = view.config.value("showTextureAtlas", false);

	// JS: this.$core.registerDropHandler({ ext: ['.blp'], ... });
	core::registerDropHandler({
		{".blp"},
		[&view]() -> std::string {
			return std::format("Export textures as {}", view.config.value("exportTextureFormat", std::string("PNG")));
		},
		[](const std::string& files_str) {
			// JS: process: files => textureExporter.exportFiles(files, true)
			// TODO(conversion): Drop handler processing will be wired when drop support is integrated.
		}
	});
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for config watches ---

	// JS: this.$core.view.$watch('config.exportTextureAlpha', () => { ... });
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
		// Refresh atlas overlay on currently previewed texture.
		if (selected_file_data_id > 0)
			preview_texture_by_id_impl(selected_file_data_id, "");
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
	// TODO(conversion): Listbox and ContextMenu component rendering will be wired when integration is complete.
	ImGui::Text("Textures: %zu", view.listfileTextures.size());
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

	// Atlas overlay regions.
	// JS: <div id="atlas-overlay" v-if="$core.view.config.showTextureAtlas">
	if (view.config.value("showTextureAtlas", false) && !view.textureAtlasOverlayRegions.empty()) {
		// TODO(conversion): Atlas region rendering as ImGui overlay rectangles will be wired
		// when the texture preview is rendered as an OpenGL texture. For now, show region names.
		for (const auto& region : view.textureAtlasOverlayRegions) {
			if (region.contains("name"))
				ImGui::TextUnformatted(region["name"].get<std::string>().c_str());
		}
	}

	// JS: <div class="image" :style="{ 'background-image': 'url(' + texturePreviewURL + ')' }">
	// TODO(conversion): Texture preview will be rendered as an ImGui::Image when GL texture loading is integrated.
	if (!view.texturePreviewURL.empty())
		ImGui::Text("[Texture Preview: %dx%d]", view.texturePreviewWidth, view.texturePreviewHeight);

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
		if (busy) ImGui::BeginDisabled();
		if (ImGui::Button("Apply to Character")) {
			// JS: methods.apply_baked_npc_texture()
			BusyLock _lock = core::create_busy_lock();
			core::setToast("progress", "loading baked npc texture...", nullptr, -1, false);

			try {
				const std::string first = casc::listfile::stripFileEntry(view.selectionTextures[0].get<std::string>());
				const auto file_data_id_opt = casc::listfile::getByFilename(first);
				if (file_data_id_opt.has_value()) {
					// JS: const file = await this.$core.view.casc.getFile(file_data_id);
					// JS: const blp = new BLPFile(file);
					// TODO(conversion): CASC getFile will be wired when CASC integration is complete.
					// view.chrCustBakedNPCTexture = blp;
					core::setToast("success", "baked npc texture applied to character", nullptr, 3000);
					logging::write(std::format("applied baked npc texture {} to character", first));
				}
			} catch (const std::exception& e) {
				core::setToast("error", "failed to load baked npc texture", nullptr, -1);
				logging::write(std::format("failed to load baked npc texture: {}", e.what()));
			}
		}
		if (busy) ImGui::EndDisabled();
		ImGui::SameLine();
	}

	// JS: <input v-if="showTextureAtlas" type="button" value="Export Atlas Regions" ...>
	if (view.config.value("showTextureAtlas", false)) {
		const bool busy = view.isBusy > 0;
		if (busy) ImGui::BeginDisabled();
		if (ImGui::Button("Export Atlas Regions"))
			export_atlas_regions();
		if (busy) ImGui::EndDisabled();
		ImGui::SameLine();
	}

	// JS: <MenuButton :options="menuButtonTextures" :default="config.exportTextureFormat" ... @click="export_textures">
	{
		const bool busy = view.isBusy > 0;
		if (busy) ImGui::BeginDisabled();
		const std::string export_format = view.config.value("exportTextureFormat", std::string("PNG"));
		if (ImGui::Button(std::format("Export as {}", export_format).c_str()))
			export_textures();
		if (busy) ImGui::EndDisabled();
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

} // namespace tab_textures