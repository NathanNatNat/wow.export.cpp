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
#include "../../app.h"

#include <glad/gl.h>
#include <stb_image.h>
#include <cstring>
#include <format>
#include <algorithm>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace legacy_tab_textures {

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

static std::string selected_file;

static std::string prev_selection_first;
static uint8_t prev_export_channel_mask = 0xFF;

static listbox::ListboxState legacy_tex_listbox_state;
static context_menu::ContextMenuState legacy_tex_ctx_state;
static menu_button::MenuButtonState legacy_tex_menu_state;

static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

static void handle_listbox_context(const nlohmann::json& data);

static void preview_texture(const std::string& filename) {
	auto& view = *core::view;
	auto dot_pos = filename.rfind('.');
	std::string ext;
	if (dot_pos != std::string::npos) {
		ext = filename.substr(dot_pos);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	}

	BusyLock _lock = core::create_busy_lock();
	logging::write(std::format("previewing texture file {}", filename));

	try {
		mpq::MPQInstall* mpq = core::view->mpq.get();
		auto data = mpq ? mpq->getFile(filename) : std::nullopt;
		if (!data) {
			logging::write(std::format("failed to load texture: {}", filename));
			return;
		}

		if (ext == ".blp") {
			BufferWrapper wrapped(std::move(*data));
			casc::BLPImage blp(wrapped);

			uint8_t mask = static_cast<uint8_t>(view.config.value("exportChannelMask", 0b1111));
			view.texturePreviewURL = blp.getDataURL(mask);
			view.texturePreviewWidth = static_cast<int>(blp.width);
			view.texturePreviewHeight = static_cast<int>(blp.height);

			std::string info;
			switch (blp.encoding) {
				case 1: info = "Palette"; break;
				case 2:
					if (blp.alphaDepth > 1)
						info = std::string("Compressed ") + (blp.alphaEncoding == 7 ? "DXT5" : "DXT3");
					else
						info = "Compressed DXT1";
					break;
				case 3: info = "ARGB"; break;
				default: info = std::format("Unsupported [{}]", blp.encoding); break;
			}

			view.texturePreviewInfo = std::format("{}x{} ({})", blp.width, blp.height, info);

			std::vector<uint8_t> pixels = blp.toUInt8Array(0, mask);
			view.texturePreviewTexID = upload_rgba_to_gl(
				pixels.data(), static_cast<int>(blp.width), static_cast<int>(blp.height),
				view.texturePreviewTexID);
		} else if (ext == ".png" || ext == ".jpg") {
			int img_w = 0, img_h = 0, img_channels = 0;
			unsigned char* pixels = stbi_load_from_memory(
				data->data(), static_cast<int>(data->size()),
				&img_w, &img_h, &img_channels, 4);
			if (pixels) {
				view.texturePreviewWidth = img_w;
				view.texturePreviewHeight = img_h;
				view.texturePreviewTexID = upload_rgba_to_gl(
					pixels, img_w, img_h, view.texturePreviewTexID);
				stbi_image_free(pixels);

				view.texturePreviewURL = "loaded";
				std::string ext_upper = ext.substr(1);
				std::transform(ext_upper.begin(), ext_upper.end(), ext_upper.begin(), ::toupper);
				view.texturePreviewInfo = std::format("{}x{} ({})", img_w, img_h, ext_upper);
			}
		}

		selected_file = filename;
	} catch (const std::exception& e) {
		logging::write(std::format("failed to preview legacy texture {}: {}", filename, e.what()));
		core::setToast("error", "unable to preview texture " + filename,
			{ {"view log", []() { logging::openRuntimeLog(); }} }, -1);
	}
}

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
		mpq::MPQInstall* mpq = core::view->mpq.get();
		auto data = mpq ? mpq->getFile(selected_file) : std::nullopt;
		if (data) {
			BufferWrapper wrapped(std::move(*data));
			casc::BLPImage blp(wrapped);
			uint8_t mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
			core::view->texturePreviewURL = blp.getDataURL(mask);

			std::vector<uint8_t> pixels = blp.toUInt8Array(0, mask);
			core::view->texturePreviewTexID = upload_rgba_to_gl(
				pixels.data(), static_cast<int>(blp.width), static_cast<int>(blp.height),
				core::view->texturePreviewTexID);
		}
	} catch (const std::exception& e) {
		logging::write(std::format("failed to refresh preview for {}: {}", selected_file, e.what()));
	}
}

static void load_texture_list() {
	if (core::view->listfileTextures.empty() && !core::view->isBusy) {
		BusyLock _lock = core::create_busy_lock();

		try {
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

void registerTab() {
	modules::register_nav_button("legacy_tab_textures", "Textures", "image.svg", install_type::MPQ);
}

void mounted() {
	load_texture_list();

	prev_selection_first.clear();
	prev_export_channel_mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
}

static void renderCheckerboard(ImDrawList* dl, ImVec2 pos, ImVec2 size, float cellSize = 8.0f) {
	const ImU32 colA = IM_COL32(204, 204, 204, 255);
	const ImU32 colB = IM_COL32(255, 255, 255, 255);
	for (float y = 0; y < size.y; y += cellSize) {
		for (float x = 0; x < size.x; x += cellSize) {
			int ix = static_cast<int>(x / cellSize);
			int iy = static_cast<int>(y / cellSize);
			ImU32 col = ((ix + iy) % 2 == 0) ? colA : colB;
			ImVec2 pMin(pos.x + x, pos.y + y);
			ImVec2 pMax(pos.x + std::min(x + cellSize, size.x),
			            pos.y + std::min(y + cellSize, size.y));
			dl->AddRectFilled(pMin, pMax, col);
		}
	}
}

void render() {
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

	{
		uint8_t current_mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
		if (current_mask != prev_export_channel_mask) {
			prev_export_channel_mask = current_mask;
			if (!core::view->isBusy)
				refresh_blp_preview();
		}
	}

	auto& view = *core::view;

	if (app::layout::BeginTab("legacy-tab-textures")) {

	auto regions = app::layout::CalcListTabRegions(false);

	if (app::layout::BeginListContainer("legacy-tex-list", regions)) {
		const auto& tex_strings = core::cached_json_strings(view.listfileTextures, s_items_cache, s_items_cache_size);

		std::vector<std::string> sel_strings;
		for (const auto& s : view.selectionTextures)
			sel_strings.push_back(s.get<std::string>());

		listbox::render("##LegacyTexListbox", tex_strings,
			view.userInputFilterTextures, sel_strings,
			false,
			true,
			view.config.value("regexFilters", false),
			[&]() -> listbox::CopyMode {
				std::string cm = view.config.value("copyMode", std::string("Default"));
				if (cm == "DIR") return listbox::CopyMode::DIR;
				if (cm == "FID") return listbox::CopyMode::FID;
				return listbox::CopyMode::Default;
			}(),
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"texture",
			nullptr,
			false,
			"textures",
			{".blp", ".png", ".jpg"},
			false,
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
	app::layout::EndListContainer();

	context_menu::render(
		"ctx-legacy-textures",
		view.contextMenus.nodeListbox,
		legacy_tex_ctx_state,
		[&]() { view.contextMenus.nodeListbox = nullptr; },
		[](const nlohmann::json& node) {
			std::vector<std::string> sel;
			if (node.contains("selection") && node["selection"].is_array())
				for (const auto& s : node["selection"])
					sel.push_back(s.get<std::string>());
			int count = node.value("count", 0);
			std::string plural = count > 1 ? "s" : "";

			if (ImGui::Selectable(std::format("Copy file path{}", plural).c_str()))
				listbox_context::copy_file_paths(sel);
			if (ImGui::Selectable(std::format("Copy export path{}", plural).c_str()))
				listbox_context::copy_export_paths(sel);
			if (ImGui::Selectable("Open export directory"))
				listbox_context::open_export_directory(sel);
		}
	);

	if (app::layout::BeginStatusBar("legacy-tex-status", regions)) {
		listbox::renderStatusBar("texture", {".blp", ".png", ".jpg"}, legacy_tex_listbox_state);
	}
	app::layout::EndStatusBar();

	if (app::layout::BeginFilterBar("legacy-tex-filter", regions)) {
		if (view.config.value("regexFilters", false)) {
			ImGui::TextUnformatted("Regex Enabled");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", view.regexTooltip.c_str());
			ImGui::SameLine();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterTextures.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputTextWithHint("##FilterLegacyTextures", "Filter textures...", filter_buf, sizeof(filter_buf)))
			view.userInputFilterTextures = filter_buf;
	}
	app::layout::EndFilterBar();

	if (app::layout::BeginPreviewContainer("legacy-tex-preview", regions)) {
		if (!view.texturePreviewInfo.empty())
			ImGui::Text("%s", view.texturePreviewInfo.c_str());

		if (!view.texturePreviewURL.empty()) {
			int mask = static_cast<int>(view.config.value("exportChannelMask", 0b1111));
			struct ChannelChip { const char* id; const char* label; int bit; const char* tooltip; };
			static const ChannelChip channels[] = {
				{"channel-red",   "R", 0b0001, "Toggle red colour channel."},
				{"channel-green", "G", 0b0010, "Toggle green colour channel."},
				{"channel-blue",  "B", 0b0100, "Toggle blue colour channel."},
				{"channel-alpha", "A", 0b1000, "Toggle alpha channel."},
			};
			for (const auto& channel : channels) {
				const bool sel = (mask & channel.bit) != 0;
				ImGui::PushID(channel.id);
				if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
				if (ImGui::Button(channel.label, ImVec2(28.0f, 24.0f))) {
					mask ^= channel.bit;
					view.config["exportChannelMask"] = mask;
				}
				if (sel) ImGui::PopStyleColor();
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", channel.tooltip);
				ImGui::PopID();
				ImGui::SameLine();
			}
			ImGui::NewLine();
		}

		if (view.texturePreviewTexID != 0) {
			const ImVec2 avail = ImGui::GetContentRegionAvail();
			const float tex_w = static_cast<float>(view.texturePreviewWidth);
			const float tex_h = static_cast<float>(view.texturePreviewHeight);
			if (tex_w > 0 && tex_h > 0) {
				const float scale = std::min({avail.x / tex_w, avail.y / tex_h, 1.0f});
				const ImVec2 img_size(tex_w * scale, tex_h * scale);

				const ImVec2 checkerPos = ImGui::GetCursorScreenPos();
				renderCheckerboard(ImGui::GetWindowDrawList(), checkerPos, img_size);

				ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.texturePreviewTexID)), img_size);
			}
		} else if (!view.texturePreviewURL.empty()) {
			ImGui::TextWrapped("Texture loaded: %dx%d", view.texturePreviewWidth, view.texturePreviewHeight);
		}
	}
	app::layout::EndPreviewContainer();

	if (app::layout::BeginPreviewControls("legacy-tex-controls", regions)) {
		std::vector<menu_button::MenuOption> tex_options;
		for (const auto& opt : view.menuButtonTextures)
			tex_options.push_back({ opt.label, opt.value });
		menu_button::render("##LegacyTexMenuButton", tex_options,
			view.config.value("exportTextureFormat", std::string("PNG")),
			view.isBusy > 0, false, legacy_tex_menu_state,
			[&](const std::string& val) { view.config["exportTextureFormat"] = val; },
			[&]() { export_textures(); });
	}
	app::layout::EndPreviewControls();

	}
	app::layout::EndTab();
}

static void handle_listbox_context(const nlohmann::json& data) {
	listbox_context::handle_context_menu(data, true);
}

static void copy_file_paths(const nlohmann::json& selection) {
	listbox_context::copy_file_paths(selection);
}

static void copy_export_paths(const nlohmann::json& selection) {
	listbox_context::copy_export_paths(selection);
}

static void open_export_directory(const nlohmann::json& selection) {
	listbox_context::open_export_directory(selection);
}

void export_textures() {
	const auto& selected = core::view->selectionTextures;
	if (selected.empty()) {
		logging::write("no textures selected for export");
		return;
	}

	texture_exporter::exportFiles(selected, nullptr, core::view->mpq.get(), false, -1);
}

}
