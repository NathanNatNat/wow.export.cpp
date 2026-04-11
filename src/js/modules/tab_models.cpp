/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_models.h"
#include "tab_textures.h"
#include "../log.h"
#include "../core.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/blte-reader.h"
#include "../casc/casc-source.h"
#include "../install-type.h"
#include "../modules.h"
#include "../ui/listbox-context.h"
#include "../components/listbox.h"
#include "../db/caches/DBModelFileData.h"
#include "../db/caches/DBItemDisplays.h"
#include "../db/caches/DBCreatures.h"
#include "../ui/texture-ribbon.h"
#include "../ui/texture-exporter.h"
#include "../ui/model-viewer-utils.h"
#include "../file-writer.h"
#include "../components/model-viewer-gl.h"
#include "../3D/renderers/M2RendererGL.h"
#include "../3D/renderers/M3RendererGL.h"
#include "../3D/renderers/WMORendererGL.h"
#include "../components/checkboxlist.h"
#include "../components/listboxb.h"
#include "../components/menu-button.h"
#include "../../app.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <map>
#include <string>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_models {

// --- File-local state ---

// JS: const active_skins = new Map();
static std::map<std::string, db::caches::DBCreatures::CreatureDisplayInfo> active_skins_creature;
static std::map<std::string, db::caches::DBItemDisplays::ItemDisplay> active_skins_item;

// JS: let selected_variant_texture_ids = new Array();
static std::vector<uint32_t> selected_variant_texture_ids;

// JS: let selected_skin_name = null;
static std::string selected_skin_name;
static bool has_selected_skin_name = false;

// JS: let active_renderer;
static model_viewer_utils::RendererResult active_renderer_result;

// JS: let active_path;
static std::string active_path;

// View state proxy (created once in mounted()).
static model_viewer_utils::ViewStateProxy view_state;

// Animation methods helper.
static std::unique_ptr<model_viewer_utils::AnimationMethods> anim_methods;

// Change-detection for watches (replaces Vue $watch).
static std::vector<nlohmann::json> prev_skins_selection;
static std::string prev_anim_selection;
static std::vector<nlohmann::json> prev_selection_models;
static bool tab_initialized = false;

static bool is_initialized = false;

static listbox::ListboxState listbox_models_state;

// Component states for CheckboxList, ListboxB, and MenuButton.
static checkboxlist::CheckboxListState checkboxlist_geosets_state;
static checkboxlist::CheckboxListState checkboxlist_wmo_groups_state;
static listboxb::ListboxBState listboxb_skins_state;
static menu_button::MenuButtonState menu_button_models_state;

// Model viewer GL state/context (replaces Vue <ModelViewerGL :context="modelViewerContext"/>).
static model_viewer_gl::State viewer_state;
static model_viewer_gl::Context viewer_context;

// --- Internal helpers ---

// Helper to get the active M2 renderer (or nullptr).
static M2RendererGL* get_active_m2_renderer() {
	return active_renderer_result.m2.get();
}

// Helper to get the view state proxy pointer.
static model_viewer_utils::ViewStateProxy* get_view_state_ptr() {
	return &view_state;
}

// JS: const get_model_displays = (file_data_id) => { ... }
struct DisplayVariant {
	uint32_t ID = 0;
	std::vector<uint32_t> textures;
	std::vector<uint32_t> extraGeosets;
	bool from_creature = false;
};

static std::vector<DisplayVariant> get_model_displays(uint32_t file_data_id) {
	// JS: let displays = DBCreatures.getCreatureDisplaysByFileDataID(file_data_id);
	const auto* creature_displays = db::caches::DBCreatures::getCreatureDisplaysByFileDataID(file_data_id);
	if (creature_displays) {
		std::vector<DisplayVariant> result;
		result.reserve(creature_displays->size());
		for (const auto& d : *creature_displays)
			result.push_back({ d.ID, d.textures, d.extraGeosets, true });
		return result;
	}

	// JS: if (displays === undefined) displays = DBItemDisplays.getItemDisplaysByFileDataID(file_data_id);
	const auto* item_displays = db::caches::DBItemDisplays::getItemDisplaysByFileDataID(file_data_id);
	if (item_displays) {
		std::vector<DisplayVariant> result;
		result.reserve(item_displays->size());
		for (const auto& d : *item_displays)
			result.push_back({ d.ID, d.textures, {}, false });
		return result;
	}

	// JS: return displays ?? [];
	return {};
}

// JS: const preview_model = async (core, file_name) => { ... }
static void preview_model(const std::string& file_name) {
	auto _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", file_name), {}, -1, false);
	logging::write(std::format("Previewing model {}", file_name));

	auto& state = view_state;
	texture_ribbon::reset();
	model_viewer_utils::clear_texture_preview(state);

	auto& view = *core::view;
	// JS: core.view.modelViewerSkins = [];
	view.modelViewerSkins.clear();
	// JS: core.view.modelViewerSkinsSelection = [];
	view.modelViewerSkinsSelection.clear();
	// JS: core.view.modelViewerAnims = [];
	view.modelViewerAnims.clear();
	// JS: core.view.modelViewerAnimSelection = null;
	view.modelViewerAnimSelection = nullptr;

	try {
		// JS: if (active_renderer) { active_renderer.dispose(); active_renderer = null; active_path = null; }
		if (active_renderer_result.type != model_viewer_utils::ModelType::Unknown) {
			active_renderer_result = model_viewer_utils::RendererResult{};
			active_path.clear();
		}

		// JS: active_skins.clear();
		active_skins_creature.clear();
		active_skins_item.clear();
		// JS: selected_variant_texture_ids.length = 0;
		selected_variant_texture_ids.clear();
		// JS: selected_skin_name = null;
		selected_skin_name.clear();
		has_selected_skin_name = false;

		// JS: const file_data_id = listfile.getByFilename(file_name);
		auto file_data_id_opt = casc::listfile::getByFilename(file_name);
		if (!file_data_id_opt.has_value()) {
			core::setToast("error", std::format("Unable to find file data ID for {}", file_name), {}, -1);
			return;
		}
		uint32_t file_data_id = file_data_id_opt.value();

		// JS: const file = await core.view.casc.getFile(file_data_id);
		BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);

		// JS: const gl_context = core.view.modelViewerContext?.gl_context;
		gl::GLContext* gl_ctx = viewer_context.gl_context;
		if (!gl_ctx) {
			core::setToast("error", "GL context not available — model viewer not initialized.", {}, -1);
			return;
		}

		// JS: const model_type = modelViewerUtils.detect_model_type_by_name(file_name) ?? modelViewerUtils.detect_model_type(file);
		auto model_type = model_viewer_utils::detect_model_type_by_name(file_name);
		if (model_type == model_viewer_utils::ModelType::Unknown)
			model_type = model_viewer_utils::detect_model_type(file);

		// JS: if (model_type === modelViewerUtils.MODEL_TYPE_M2) core.view.modelViewerActiveType = 'm2';
		// JS: else if (model_type === modelViewerUtils.MODEL_TYPE_M3) core.view.modelViewerActiveType = 'm3';
		// JS: else core.view.modelViewerActiveType = 'wmo';
		if (model_type == model_viewer_utils::ModelType::M2)
			view.modelViewerActiveType = "m2";
		else if (model_type == model_viewer_utils::ModelType::M3)
			view.modelViewerActiveType = "m3";
		else
			view.modelViewerActiveType = "wmo";

		// JS: active_renderer = modelViewerUtils.create_renderer(file, model_type, gl_context, core.view.config.modelViewerShowTextures, file_name);
		active_renderer_result = model_viewer_utils::create_renderer(
			file, model_type, *gl_ctx,
			view.config.value("modelViewerShowTextures", true),
			file_data_id
		);

		// JS: await active_renderer.load();
		if (active_renderer_result.m2)
			active_renderer_result.m2->load();
		else if (active_renderer_result.m3)
			active_renderer_result.m3->load();
		else if (active_renderer_result.wmo)
			active_renderer_result.wmo->load();

		// JS: if (model_type === modelViewerUtils.MODEL_TYPE_M2) { ... setup skins and animations ... }
		if (model_type == model_viewer_utils::ModelType::M2) {
			// JS: const displays = get_model_displays(file_data_id);
			const auto displays = get_model_displays(file_data_id);

			// JS: const skin_list = [];
			std::vector<nlohmann::json> skin_list;
			// JS: let model_name = listfile.getByID(file_data_id);
			// JS: model_name = path.basename(model_name, 'm2');
			std::string model_name = casc::listfile::getByID(file_data_id);
			{
				auto pos = model_name.rfind('/');
				if (pos != std::string::npos) model_name = model_name.substr(pos + 1);
				pos = model_name.rfind('\\');
				if (pos != std::string::npos) model_name = model_name.substr(pos + 1);
				// Strip extension — JS uses path.basename(name, 'm2') which strips trailing 'm2'
				auto ext_pos = model_name.rfind('.');
				if (ext_pos != std::string::npos) model_name = model_name.substr(0, ext_pos);
			}

			for (const auto& display : displays) {
				// JS: if (display.textures.length === 0) continue;
				if (display.textures.empty())
					continue;

				// JS: const texture = display.textures[0];
				uint32_t texture = display.textures[0];

				// JS: let clean_skin_name = ''; let skin_name = listfile.getByID(texture);
				std::string clean_skin_name;
				std::string skin_name = casc::listfile::getByID(texture);

				if (!skin_name.empty()) {
					// JS: skin_name = path.basename(skin_name, '.blp');
					{
						auto pos = skin_name.rfind('/');
						if (pos != std::string::npos) skin_name = skin_name.substr(pos + 1);
						pos = skin_name.rfind('\\');
						if (pos != std::string::npos) skin_name = skin_name.substr(pos + 1);
						// Strip .blp extension
						if (skin_name.size() > 4 && skin_name.substr(skin_name.size() - 4) == ".blp")
							skin_name = skin_name.substr(0, skin_name.size() - 4);
					}
					// JS: clean_skin_name = skin_name.replace(model_name, '').replace('_', '');
					clean_skin_name = skin_name;
					auto found = clean_skin_name.find(model_name);
					if (found != std::string::npos)
						clean_skin_name.erase(found, model_name.size());
					found = clean_skin_name.find('_');
					if (found != std::string::npos)
						clean_skin_name.erase(found, 1);
				} else {
					// JS: skin_name = 'unknown_' + texture;
					skin_name = "unknown_" + std::to_string(texture);
				}

				// JS: if (clean_skin_name.length === 0) clean_skin_name = 'base';
				if (clean_skin_name.empty())
					clean_skin_name = "base";

				// JS: if (display.extraGeosets?.length > 0) skin_name += display.extraGeosets.join(',');
				if (!display.extraGeosets.empty()) {
					std::string geo_str;
					for (size_t g = 0; g < display.extraGeosets.size(); ++g) {
						if (g > 0) geo_str += ',';
						geo_str += std::to_string(display.extraGeosets[g]);
					}
					skin_name += geo_str;
				}

				// JS: clean_skin_name += ' (' + display.ID + ')';
				clean_skin_name += " (" + std::to_string(display.ID) + ")";

				// JS: if (active_skins.has(skin_name)) continue;
				if (active_skins_creature.contains(skin_name) || active_skins_item.contains(skin_name))
					continue;

				// JS: skin_list.push({ id: skin_name, label: clean_skin_name });
				skin_list.push_back({ {"id", skin_name}, {"label", clean_skin_name} });

				// JS: active_skins.set(skin_name, display);
				if (display.from_creature) {
					active_skins_creature[skin_name] = {
						display.ID, 0, 0, display.textures, display.extraGeosets
					};
				} else {
					active_skins_item[skin_name] = { display.ID, display.textures };
				}
			}

			// JS: core.view.modelViewerSkins = skin_list;
			view.modelViewerSkins = skin_list;
			// JS: core.view.modelViewerSkinsSelection = skin_list.slice(0, 1);
			if (!skin_list.empty())
				view.modelViewerSkinsSelection = { skin_list[0] };
			else
				view.modelViewerSkinsSelection.clear();

			// JS: core.view.modelViewerAnims = modelViewerUtils.extract_animations(active_renderer);
			if (active_renderer_result.m2)
				view.modelViewerAnims = model_viewer_utils::extract_animations(*active_renderer_result.m2);
			// JS: core.view.modelViewerAnimSelection = 'none';
			view.modelViewerAnimSelection = "none";
		}

		// JS: active_path = file_name;
		active_path = file_name;

		// JS: const has_content = active_renderer.draw_calls?.length > 0 || active_renderer.groups?.length > 0;
		bool has_content = false;
		if (active_renderer_result.m2)
			has_content = !active_renderer_result.m2->get_draw_calls().empty();
		else if (active_renderer_result.m3)
			has_content = true; // M3 always has content if loaded
		else if (active_renderer_result.wmo)
			has_content = !active_renderer_result.wmo->get_groups().empty();

		if (!has_content) {
			// JS: core.setToast('info', util.format('The model %s doesn\'t have any 3D data associated with it.', file_name), null, 4000);
			core::setToast("info", std::format("The model {} doesn't have any 3D data associated with it.", file_name), {}, 4000);
		} else {
			core::hideToast();

			// JS: if (core.view.modelViewerAutoAdjust) requestAnimationFrame(() => core.view.modelViewerContext?.fitCamera?.());
			if (view.modelViewerAutoAdjust && viewer_context.fitCamera)
				viewer_context.fitCamera();
		}
	} catch (const casc::EncryptionError& e) {
		// JS: if (e instanceof EncryptionError) { ... }
		core::setToast("error", std::format("The model {} is encrypted with an unknown key ({}).", file_name, e.key), {}, -1);
		logging::write(std::format("Failed to decrypt model {} ({})", file_name, e.key));
	} catch (const std::exception& e) {
		// JS: core.setToast('error', 'Unable to preview model ' + file_name, ...);
		core::setToast("error", "Unable to preview model " + file_name, {}, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}
}

// JS: const get_variant_texture_ids = (file_name) => { ... }
static std::vector<uint32_t> get_variant_texture_ids(const std::string& file_name) {
	// JS: if (file_name === active_path) return selected_variant_texture_ids;
	if (file_name == active_path)
		return selected_variant_texture_ids;

	// JS: const file_data_id = listfile.getByFilename(file_name);
	auto file_data_id_opt = casc::listfile::getByFilename(file_name);
	if (!file_data_id_opt.has_value())
		return {};

	// JS: const displays = get_model_displays(file_data_id);
	auto displays = get_model_displays(file_data_id_opt.value());

	// JS: return displays.find(e => e.textures.length > 0)?.textures ?? [];
	for (const auto& d : displays) {
		if (!d.textures.empty())
			return d.textures;
	}
	return {};
}

// --- Template methods (mapped from Vue methods) ---

// JS: methods.handle_listbox_context(data) { listboxContext.handle_context_menu(data); }
static void handle_listbox_context(const std::vector<std::string>& selection) {
	listbox_context::handle_context_menu(selection);
}

// JS: methods.copy_file_paths(selection) { listboxContext.copy_file_paths(selection); }
static void copy_file_paths(const std::vector<std::string>& selection) {
	listbox_context::copy_file_paths(selection);
}

// JS: methods.copy_listfile_format(selection) { listboxContext.copy_listfile_format(selection); }
static void copy_listfile_format(const std::vector<std::string>& selection) {
	listbox_context::copy_listfile_format(selection);
}

// JS: methods.copy_file_data_ids(selection) { listboxContext.copy_file_data_ids(selection); }
static void copy_file_data_ids(const std::vector<std::string>& selection) {
	listbox_context::copy_file_data_ids(selection);
}

// JS: methods.copy_export_paths(selection) { listboxContext.copy_export_paths(selection); }
static void copy_export_paths(const std::vector<std::string>& selection) {
	listbox_context::copy_export_paths(selection);
}

// JS: methods.open_export_directory(selection) { listboxContext.open_export_directory(selection); }
static void open_export_directory(const std::vector<std::string>& selection) {
	listbox_context::open_export_directory(selection);
}

// JS: methods.preview_texture(file_data_id, display_name) { ... }
static void preview_texture(uint32_t file_data_id, const std::string& display_name) {
	// JS: const state = get_view_state(this.$core);
	// JS: await modelViewerUtils.preview_texture_by_id(this.$core, state, active_renderer, file_data_id, display_name);
	model_viewer_utils::preview_texture_by_id(
		view_state, get_active_m2_renderer(),
		file_data_id, display_name, core::view->casc
	);
}

// JS: methods.export_ribbon_texture(file_data_id, display_name) { ... }
static void export_ribbon_texture(uint32_t file_data_id, [[maybe_unused]] const std::string& display_name) {
	// JS: await textureExporter.exportSingleTexture(file_data_id);
	texture_exporter::exportSingleTexture(file_data_id, core::view->casc);
}

// JS: methods.toggle_uv_layer(layer_name) { ... }
static void toggle_uv_layer(const std::string& layer_name) {
	// JS: const state = get_view_state(this.$core);
	// JS: modelViewerUtils.toggle_uv_layer(state, active_renderer, layer_name);
	model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
}

// JS: methods.export_model() { ... }
static void export_model_action() {
	auto& view = *core::view;

	// JS: const user_selection = this.$core.view.selectionModels;
	const auto& user_selection = view.selectionModels;

	// JS: if (user_selection.length === 0) { ... }
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	// JS: await export_files(this.$core, user_selection, false);
	export_files(user_selection, false);
}

// JS: methods.initialize() { ... }
static void initialize() {
	auto& view = *core::view;

	// JS: let step_count = 2;
	int step_count = 2;
	// JS: if (this.$core.view.config.enableUnknownFiles) step_count++;
	if (view.config.value("enableUnknownFiles", false)) step_count++;
	// JS: if (this.$core.view.config.enableM2Skins) step_count += 2;
	if (view.config.value("enableM2Skins", true)) step_count += 2;

	// JS: this.$core.showLoadingScreen(step_count);
	core::showLoadingScreen(step_count);

	// JS: await this.$core.progressLoadingScreen('Loading model file data...');
	core::progressLoadingScreen("Loading model file data...");
	// JS: await DBModelFileData.initializeModelFileData();
	db::caches::DBModelFileData::initializeModelFileData();

	// JS: if (this.$core.view.config.enableUnknownFiles) { ... }
	if (view.config.value("enableUnknownFiles", false)) {
		core::progressLoadingScreen("Loading unknown models...");
		// JS: await listfile.loadUnknownModels();
		casc::listfile::loadUnknownModels();
	}

	// JS: if (this.$core.view.config.enableM2Skins) { ... }
	if (view.config.value("enableM2Skins", true)) {
		core::progressLoadingScreen("Loading item displays...");
		// JS: await DBItemDisplays.initializeItemDisplays();
		db::caches::DBItemDisplays::initializeItemDisplays();

		core::progressLoadingScreen("Loading creature data...");
		// JS: await DBCreatures.initializeCreatureData();
		db::caches::DBCreatures::initializeCreatureData();
	}

	// JS: await this.$core.progressLoadingScreen('Initializing 3D preview...');
	core::progressLoadingScreen("Initializing 3D preview...");

	// JS: if (!this.$core.view.modelViewerContext)
	//     this.$core.view.modelViewerContext = Object.seal({ getActiveRenderer: () => active_renderer, gl_context: null, fitCamera: null });
	if (view.modelViewerContext.is_null()) {
		view.modelViewerContext = nlohmann::json::object();

		// Wire model viewer context callbacks.
		viewer_context.getActiveRenderer = []() -> M2RendererGL* {
			return get_active_m2_renderer();
		};
		viewer_context.renderActiveModel = [](const float* view_mat, const float* proj_mat) {
			if (active_renderer_result.m3)
				active_renderer_result.m3->render(view_mat, proj_mat);
			else if (active_renderer_result.wmo)
				active_renderer_result.wmo->render(view_mat, proj_mat);
		};
		viewer_context.setActiveModelTransform = [](const std::array<float, 3>& pos,
													const std::array<float, 3>& rot,
													const std::array<float, 3>& scale) {
			if (active_renderer_result.wmo)
				active_renderer_result.wmo->setTransform(pos, rot, scale);
		};
		viewer_context.getActiveBoundingBox = []() -> std::optional<model_viewer_gl::BoundingBox> {
			if (active_renderer_result.m2) {
				auto bb = active_renderer_result.m2->getBoundingBox();
				if (bb) return model_viewer_gl::BoundingBox{ bb->min, bb->max };
			} else if (active_renderer_result.m3) {
				auto bb = active_renderer_result.m3->getBoundingBox();
				if (bb) return model_viewer_gl::BoundingBox{ bb->min, bb->max };
			} else if (active_renderer_result.wmo) {
				auto bb = active_renderer_result.wmo->getBoundingBox();
				if (bb) return model_viewer_gl::BoundingBox{ bb->min, bb->max };
			}
			return std::nullopt;
		};
	}

	// JS: this.$core.hideLoadingScreen();
	core::hideLoadingScreen();
}

// --- Watch handler for modelViewerSkinsSelection ---
// JS: this.$core.view.$watch('modelViewerSkinsSelection', async selection => { ... })
static void handle_skins_selection_change(const std::vector<nlohmann::json>& selection) {
	auto& view = *core::view;

	// JS: if (!active_renderer || active_skins.size === 0) return;
	if (active_renderer_result.type == model_viewer_utils::ModelType::Unknown)
		return;
	if (active_skins_creature.empty() && active_skins_item.empty())
		return;

	// JS: const selected = selection[0];
	if (selection.empty())
		return;
	const auto& selected = selection[0];
	std::string sel_id = selected.value("id", std::string(""));

	// JS: selected_skin_name = selected.id;
	selected_skin_name = sel_id;
	has_selected_skin_name = true;

	// Try creature display first, then item display.
	bool has_extra_geosets = false;
	std::vector<uint32_t> extra_geosets;
	std::vector<uint32_t> display_textures;

	auto it_creature = active_skins_creature.find(sel_id);
	if (it_creature != active_skins_creature.end()) {
		const auto& display = it_creature->second;
		extra_geosets = display.extraGeosets;
		has_extra_geosets = !extra_geosets.empty();
		display_textures = display.textures;
	} else {
		auto it_item = active_skins_item.find(sel_id);
		if (it_item != active_skins_item.end()) {
			const auto& display = it_item->second;
			display_textures = display.textures;
		}
	}

	// JS: let curr_geosets = this.$core.view.modelViewerGeosets;
	auto& curr_geosets = view.modelViewerGeosets;

	// JS: if (display.extraGeosets !== undefined) { ... } else { ... }
	if (has_extra_geosets) {
		// JS: for (const geoset of curr_geosets) { if (geoset.id > 0 && geoset.id < 900) geoset.checked = false; }
		for (auto& geoset : curr_geosets) {
			int gid = geoset.value("id", 0);
			if (gid > 0 && gid < 900)
				geoset["checked"] = false;
		}

		// JS: for (const extra_geoset of display.extraGeosets) {
		//         for (const geoset of curr_geosets) { if (geoset.id === extra_geoset) geoset.checked = true; } }
		for (uint32_t extra_geoset : extra_geosets) {
			for (auto& geoset : curr_geosets) {
				if (geoset.value("id", 0u) == extra_geoset)
					geoset["checked"] = true;
			}
		}
	} else {
		// JS: for (const geoset of curr_geosets) { const id = geoset.id.toString();
		//         geoset.checked = (id.endsWith('0') || id.endsWith('01')); }
		for (auto& geoset : curr_geosets) {
			std::string id_str = std::to_string(geoset.value("id", 0));
			bool checked = id_str.ends_with("0") || id_str.ends_with("01");
			geoset["checked"] = checked;
		}
	}

	// JS: if (display.textures.length > 0) selected_variant_texture_ids = [...display.textures];
	if (!display_textures.empty())
		selected_variant_texture_ids = display_textures;

	// JS: active_renderer.applyReplaceableTextures(display);
	if (active_renderer_result.m2 && !display_textures.empty()) {
		M2DisplayInfo info;
		info.textures = display_textures;
		active_renderer_result.m2->applyReplaceableTextures(info);
	}
}

// --- Public API ---

// JS: register() { this.registerNavButton('Models', 'cube.svg', InstallType.CASC); }
void registerTab() {
	// JS: this.registerNavButton('Models', 'cube.svg', InstallType.CASC);
	modules::register_nav_button("tab_models", "Models", "cube.svg", install_type::CASC);
}

// JS: async mounted() { ... }
void mounted() {
	auto& view = *core::view;

	// JS: const state = get_view_state(this.$core);
	view_state = model_viewer_utils::create_view_state("model");

	// JS: this.$core.registerDropHandler({
	//         ext: ['.m2'],
	//         prompt: count => util.format('Export %d models as %s', count, this.$core.view.config.exportModelFormat),
	//         process: files => export_files(this.$core, files, true)
	//     });
	core::registerDropHandler({
		{".m2"},
		[]() -> std::string {
			return std::format("Export models as {}", core::view->config.value("exportModelFormat", std::string("OBJ")));
		},
		[](const std::string& file) {
			nlohmann::json entry;
			entry["fileName"] = file;
			export_files({entry}, true);
		}
	});

	// JS: await this.initialize();
	initialize();

	// Create animation methods helper.
	anim_methods = std::make_unique<model_viewer_utils::AnimationMethods>(
		get_active_m2_renderer,
		get_view_state_ptr
	);

	// Store initial state for change-detection.

	// JS: this.$core.view.$watch('modelViewerSkinsSelection', async selection => { ... });
	prev_skins_selection = view.modelViewerSkinsSelection;

	// JS: this.$core.view.$watch('modelViewerAnimSelection', async selected_animation_id => { ... });
	if (view.modelViewerAnimSelection.is_string())
		prev_anim_selection = view.modelViewerAnimSelection.get<std::string>();
	else
		prev_anim_selection.clear();

	// JS: this.$core.view.$watch('selectionModels', async selection => { ... });
	prev_selection_models = view.selectionModels;

	// JS: this.$core.events.on('toggle-uv-layer', (layer_name) => { ... });
	core::events.on("toggle-uv-layer", EventEmitter::ArgCallback([](const std::any& arg) {
		const auto& layer_name = std::any_cast<const std::string&>(arg);
		model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
	}));

	is_initialized = true;
}

// JS: export_files = async (core, files, is_local = false, export_id = -1) => { ... }
void export_files(const std::vector<nlohmann::json>& files, bool is_local, int export_id) {
	auto& view = *core::view;

	// JS: const export_paths = core.openLastExportStream();
	FileWriter export_paths_writer = core::openLastExportStream();
	FileWriter* export_paths = &export_paths_writer;

	// JS: const format = core.view.config.exportModelFormat;
	std::string format = view.config.value("exportModelFormat", std::string("OBJ"));

	// JS: const manifest = { type: 'MODELS', exportID: export_id, succeeded: [], failed: [] };
	nlohmann::json manifest = {
		{"type", "MODELS"},
		{"exportID", export_id},
		{"succeeded", nlohmann::json::array()},
		{"failed", nlohmann::json::array()}
	};

	// JS: if (format === 'PNG' || format === 'CLIPBOARD') { ... }
	if (format == "PNG" || format == "CLIPBOARD") {
		if (!active_path.empty()) {
			// JS: const canvas = document.getElementById('model-preview').querySelector('canvas');
			// JS: await modelViewerUtils.export_preview(core, format, canvas, active_path);
			gl::GLContext* gl_ctx = viewer_context.gl_context;
			if (gl_ctx && viewer_state.fbo != 0) {
				glBindFramebuffer(GL_FRAMEBUFFER, viewer_state.fbo);
				model_viewer_utils::export_preview(format, *gl_ctx, active_path);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		} else {
			// JS: core.setToast('error', 'The selected export option only works for model previews. Preview something first!', null, -1);
			core::setToast("error", "The selected export option only works for model previews. Preview something first!", {}, -1);
		}

		// JS: export_paths?.close();
		return;
	}

	// JS: const casc = core.view.casc;
	// JS: const casc = core.view.casc;
	auto* casc = view.casc;

	// JS: const helper = new ExportHelper(files.length, 'model');
	casc::ExportHelper helper(static_cast<int>(files.size()), "model");
	// JS: helper.start();
	helper.start();

	for (const auto& file_entry : files) {
		// JS: if (helper.isCancelled()) break;
		if (helper.isCancelled())
			break;

		std::string file_name;
		uint32_t file_data_id = 0;

		// JS: if (typeof file_entry === 'number') { ... } else { ... }
		if (file_entry.is_number()) {
			file_data_id = file_entry.get<uint32_t>();
			file_name = casc::listfile::getByID(file_data_id);
		} else {
			file_name = casc::listfile::stripFileEntry(file_entry.get<std::string>());
			auto opt = casc::listfile::getByFilename(file_name);
			if (opt.has_value())
				file_data_id = opt.value();
		}

		std::vector<nlohmann::json> file_manifest;

		try {
			// JS: const data = await (is_local ? require('../buffer').readFile(file_name) : casc.getFile(file_data_id));
			BufferWrapper data = is_local ? BufferWrapper::readFile(file_name) : casc->getVirtualFileByID(file_data_id);

			// JS: if (file_name === undefined) { ... }
			if (file_name.empty()) {
				auto detected_type = model_viewer_utils::detect_model_type(data);
				file_name = casc::listfile::formatUnknownFile(file_data_id, model_viewer_utils::get_model_extension(detected_type));
			}

			std::string export_path;
			std::string mark_file_name = file_name;

			bool is_active = (file_name == active_path);

			// JS: const model_type = modelViewerUtils.detect_model_type_by_name(file_name) ?? modelViewerUtils.detect_model_type(data);
			auto model_type = model_viewer_utils::detect_model_type_by_name(file_name);
			if (model_type == model_viewer_utils::ModelType::Unknown)
				model_type = model_viewer_utils::detect_model_type(data);

			if (is_local) {
				// JS: export_path = file_name;
				export_path = file_name;
			} else if (model_type == model_viewer_utils::ModelType::M2 && has_selected_skin_name && is_active && format != "RAW") {
				// JS: const base_file_name = path.basename(file_name, path.extname(file_name));
				std::filesystem::path fp(file_name);
				std::string base_file_name = fp.stem().string();

				std::string skinned_name;
				// JS: if (selected_skin_name.startsWith(base_file_name))
				if (selected_skin_name.starts_with(base_file_name))
					skinned_name = casc::ExportHelper::replaceBaseName(file_name, selected_skin_name);
				else
					skinned_name = casc::ExportHelper::replaceBaseName(file_name, base_file_name + "_" + selected_skin_name);

				export_path = casc::ExportHelper::getExportPath(skinned_name);
				mark_file_name = skinned_name;
			} else {
				// JS: export_path = ExportHelper.getExportPath(file_name);
				export_path = casc::ExportHelper::getExportPath(file_name);
			}

			// JS: const mark_name = await modelViewerUtils.export_model({ ... });
			model_viewer_utils::ExportModelOptions opts;
			opts.data = &data;
			opts.file_data_id = file_data_id;
			opts.file_name = file_name;
			opts.format = format;
			opts.export_path = export_path;
			opts.helper = &helper;
			opts.casc = casc;
			opts.file_manifest = &file_manifest;

			// JS: variant_textures: get_variant_texture_ids(file_name),
			auto vtids = get_variant_texture_ids(file_name);
			for (uint32_t id : vtids)
				opts.variant_textures.push_back(id);

			// JS: geoset_mask: is_active ? core.view.modelViewerGeosets : null,
			if (is_active) {
				opts.geoset_mask = &view.modelViewerGeosets;
				opts.wmo_group_mask = &view.modelViewerWMOGroups;
				opts.wmo_set_mask = &view.modelViewerWMOSets;
			}

			opts.export_paths = export_paths;

			std::string mark_name = model_viewer_utils::export_model(opts);

			// JS: helper.mark(mark_name, true);
			helper.mark(mark_name, true);

			// JS: manifest.succeeded.push({ fileDataID: file_data_id, files: file_manifest });
			manifest["succeeded"].push_back({
				{"fileDataID", file_data_id},
				{"files", file_manifest}
			});
		} catch (const std::exception& e) {
			// JS: helper.mark(file_name, false, e.message, e.stack);
			helper.mark(file_name, false, e.what());
			// JS: manifest.failed.push({ fileDataID: file_data_id });
			manifest["failed"].push_back({ {"fileDataID", file_data_id} });
		}
	}

	// JS: helper.finish();
	helper.finish();
	// JS: export_paths?.close();
}

// JS: getActiveRenderer: () => active_renderer
M2RendererGL* getActiveRenderer() {
	return active_renderer_result.m2.get();
}

void render() {
	auto& view = *core::view;

	if (!is_initialized)
		return;

	// ─── Change-detection for watches ───────────────────────────────────────

	// Watch: modelViewerSkinsSelection → apply skin textures and geosets
	// JS: this.$core.view.$watch('modelViewerSkinsSelection', async selection => { ... });
	{
		if (view.modelViewerSkinsSelection != prev_skins_selection) {
			prev_skins_selection = view.modelViewerSkinsSelection;
			handle_skins_selection_change(view.modelViewerSkinsSelection);
		}
	}

	// Watch: modelViewerAnimSelection → handle_animation_change
	// JS: this.$core.view.$watch('modelViewerAnimSelection', async selected_animation_id => { ... });
	{
		std::string current_anim;
		if (view.modelViewerAnimSelection.is_string())
			current_anim = view.modelViewerAnimSelection.get<std::string>();

		if (current_anim != prev_anim_selection) {
			prev_anim_selection = current_anim;

			// JS: if (this.$core.view.modelViewerAnims.length === 0) return;
			if (!view.modelViewerAnims.empty()) {
				model_viewer_utils::handle_animation_change(
					get_active_m2_renderer(),
					view_state,
					current_anim
				);
			}
		}
	}

	// Watch: selectionModels → auto-preview if modelsAutoPreview
	// JS: this.$core.view.$watch('selectionModels', async selection => { ... });
	{
		if (view.selectionModels != prev_selection_models) {
			prev_selection_models = view.selectionModels;

			// JS: if (!this._tab_initialized) return;
			if (!tab_initialized) {
				tab_initialized = true;
			} else {
				// JS: if (!this.$core.view.config.modelsAutoPreview) return;
				if (view.config.value("modelsAutoPreview", false)) {
					// JS: const first = listfile.stripFileEntry(selection[0]);
					if (!view.selectionModels.empty()) {
						std::string first;
						if (view.selectionModels[0].is_string())
							first = casc::listfile::stripFileEntry(view.selectionModels[0].get<std::string>());

						// JS: if (!this.$core.view.isBusy && first && active_path !== first)
						if (view.isBusy == 0 && !first.empty() && active_path != first)
							preview_model(first);
					}
				}
			}
		}
	}

	// ─── Template rendering ─────────────────────────────────────────────────

	// JS: <div class="tab list-tab" id="tab-models">
	if (app::layout::BeginTab("tab-models")) {
		auto regions = app::layout::CalcListTabRegions(true);

		// --- Left panel: List container (row 1, col 1) ---
		// JS: <div class="list-container">
		//     <Listbox v-model:selection="selectionModels" v-model:filter="userInputFilterModels"
		//         :items="listfileModels" :override="overrideModelList" :keyinput="true"
		//         :regex="config.regexFilters" :copymode="config.copyMode" ... @contextmenu="handle_listbox_context" />
		if (app::layout::BeginListContainer("models-list-container", regions)) {
			std::vector<std::string> items_str;
			items_str.reserve(view.listfileModels.size());
			for (const auto& item : view.listfileModels)
				items_str.push_back(item.get<std::string>());

			std::vector<std::string> selection_str;
			for (const auto& s : view.selectionModels)
				selection_str.push_back(s.get<std::string>());

			listbox::CopyMode copy_mode = listbox::CopyMode::Default;
			{
				std::string cm = view.config.value("copyMode", std::string("Default"));
				if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
				else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
			}

			// Convert override list.
			std::vector<std::string> override_str;
			if (!view.overrideModelList.empty()) {
				override_str.reserve(view.overrideModelList.size());
				for (const auto& item : view.overrideModelList)
					override_str.push_back(item.get<std::string>());
			}

			listbox::render(
				"listbox-models",
				items_str,
				view.userInputFilterModels,
				selection_str,
				false,    // single
				true,     // keyinput
				view.config.value("regexFilters", false),
				copy_mode,
				view.config.value("pasteSelection", false),
				view.config.value("removePathSpacesCopy", false),
				"model",  // unittype
				view.overrideModelList.empty() ? nullptr : &override_str,
				false,    // disable
				"models", // persistscrollkey
				{},       // quickfilters
				false,    // nocopy
				listbox_models_state,
				[&](const std::vector<std::string>& new_sel) {
					view.selectionModels.clear();
					for (const auto& s : new_sel)
						view.selectionModels.push_back(s);
				},
				[](const listbox::ContextMenuEvent& ev) {
					handle_listbox_context(ev.selection);
				}
			);

			// JS: <component :is="$components.ContextMenu" :node="$core.view.contextMenus.nodeListbox" ...>
			if (!view.contextMenus.nodeListbox.is_null()) {
				if (ImGui::BeginPopup("ModelsListboxContextMenu")) {
					const auto& node = view.contextMenus.nodeListbox;
					// Extract selection from context node
					std::vector<std::string> sel_strings;
					if (node.contains("selection") && node["selection"].is_array()) {
						for (const auto& s : node["selection"]) {
							if (s.is_string())
								sel_strings.push_back(s.get<std::string>());
						}
					}
					int count = node.value("count", 1);
					bool hasFileDataIDs = node.value("hasFileDataIDs", false);

					// JS: <span @click.self="copy_file_paths(...)">Copy file path(s)</span>
					if (ImGui::MenuItem(std::format("Copy file path{}", count > 1 ? "s" : "").c_str())) {
						copy_file_paths(sel_strings);
						view.contextMenus.nodeListbox = nullptr;
					}

					// JS: <span v-if="context.node.hasFileDataIDs" @click.self="copy_listfile_format(...)">
					if (hasFileDataIDs) {
						if (ImGui::MenuItem(std::format("Copy file path{} (listfile format)", count > 1 ? "s" : "").c_str())) {
							copy_listfile_format(sel_strings);
							view.contextMenus.nodeListbox = nullptr;
						}
					}

					// JS: <span v-if="context.node.hasFileDataIDs" @click.self="copy_file_data_ids(...)">
					if (hasFileDataIDs) {
						if (ImGui::MenuItem(std::format("Copy file data ID{}", count > 1 ? "s" : "").c_str())) {
							copy_file_data_ids(sel_strings);
							view.contextMenus.nodeListbox = nullptr;
						}
					}

					// JS: <span @click.self="copy_export_paths(...)">Copy export path(s)</span>
					if (ImGui::MenuItem(std::format("Copy export path{}", count > 1 ? "s" : "").c_str())) {
						copy_export_paths(sel_strings);
						view.contextMenus.nodeListbox = nullptr;
					}

					// JS: <span @click.self="open_export_directory(...)">Open export directory</span>
					if (ImGui::MenuItem("Open export directory")) {
						open_export_directory(sel_strings);
						view.contextMenus.nodeListbox = nullptr;
					}

					ImGui::EndPopup();
				}
			}
		}
		app::layout::EndListContainer();

		// --- Filter bar (row 2, col 1) ---
		// JS: <div class="filter"> <input type="text" v-model="userInputFilterModels" placeholder="Filter models..."/> </div>
		if (app::layout::BeginFilterBar("models-filter", regions)) {
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			char filter_buf[256] = {};
			std::strncpy(filter_buf, view.userInputFilterModels.c_str(), sizeof(filter_buf) - 1);
			if (ImGui::InputText("##FilterModels", filter_buf, sizeof(filter_buf)))
				view.userInputFilterModels = filter_buf;
		}
		app::layout::EndFilterBar();

		// --- Middle panel: Preview container (row 1, col 2) ---
		// JS: <div class="preview-container">
		if (app::layout::BeginPreviewContainer("models-preview-container", regions)) {
			// JS: <component :is="$components.ResizeLayer" id="texture-ribbon"
			//         v-if="config.modelViewerShowTextures && textureRibbonStack.length > 0">
			if (view.config.value("modelViewerShowTextures", true) && !view.textureRibbonStack.empty()) {
				// Texture ribbon slot rendering with pagination
				float ribbon_width = ImGui::GetContentRegionAvail().x;
				texture_ribbon::onResize(static_cast<int>(ribbon_width));

				int maxPages = (view.textureRibbonSlotCount > 0)
					? static_cast<int>(std::ceil(static_cast<double>(view.textureRibbonStack.size()) / view.textureRibbonSlotCount))
					: 0;

				// Prev button
				if (view.textureRibbonPage > 0) {
					if (ImGui::SmallButton("<##ribbon_prev"))
						view.textureRibbonPage--;
					ImGui::SameLine();
				}

				// Visible slots
				int startIndex = view.textureRibbonPage * view.textureRibbonSlotCount;
				int endIndex = (std::min)(startIndex + view.textureRibbonSlotCount, static_cast<int>(view.textureRibbonStack.size()));

				for (int si = startIndex; si < endIndex; si++) {
					auto& slot = view.textureRibbonStack[si];
					std::string slotDisplayName = slot.value("displayName", std::string(""));

					ImGui::PushID(si);
					// JS: <div v-for="slot in textureRibbonDisplay" :title="slot.displayName"
					//         :style="{ backgroundImage: 'url(' + slot.src + ')' }" class="slot"
					//         @click="contextMenus.nodeTextureRibbon = slot">
					GLuint slotTex = texture_ribbon::getSlotTexture(si);
					bool clicked = false;
					if (slotTex != 0) {
						clicked = ImGui::ImageButton("##ribbon_slot",
							static_cast<ImTextureID>(static_cast<uintptr_t>(slotTex)),
							ImVec2(64, 64));
					} else {
						clicked = ImGui::Button(slotDisplayName.c_str(), ImVec2(64, 64));
					}
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", slotDisplayName.c_str());
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right) || clicked) {
						view.contextMenus.nodeTextureRibbon = slot;
						ImGui::OpenPopup("ModelsTextureRibbonContextMenu");
					}
					ImGui::PopID();
					ImGui::SameLine();
				}

				// Next button
				if (view.textureRibbonPage < maxPages - 1) {
					if (ImGui::SmallButton(">##ribbon_next"))
						view.textureRibbonPage++;
				} else {
					ImGui::NewLine();
				}

				// JS: <component :is="$components.ContextMenu" :node="$core.view.contextMenus.nodeTextureRibbon" ...>
				if (!view.contextMenus.nodeTextureRibbon.is_null()) {
					if (ImGui::BeginPopup("ModelsTextureRibbonContextMenu")) {
						const auto& node = view.contextMenus.nodeTextureRibbon;
						uint32_t fdid = node.value("fileDataID", 0u);
						std::string displayName = node.value("displayName", std::string(""));
						std::string fileName = node.value("fileName", std::string(""));

						// JS: <span @click.self="preview_texture(...)">Preview {{ displayName }}</span>
						if (ImGui::MenuItem(std::format("Preview {}", displayName).c_str())) {
							preview_texture(fdid, displayName);
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						// JS: <span @click.self="export_ribbon_texture(...)">Export {{ displayName }}</span>
						if (ImGui::MenuItem(std::format("Export {}", displayName).c_str())) {
							export_ribbon_texture(fdid, displayName);
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						// JS: <span @click.self="$core.view.goToTexture(...)">Go to {{ displayName }}</span>
						if (ImGui::MenuItem(std::format("Go to {}", displayName).c_str())) {
							tab_textures::goToTexture(fdid);
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						// JS: <span @click.self="$core.view.copyToClipboard(fileDataID)">Copy file data ID to clipboard</span>
						if (ImGui::MenuItem("Copy file data ID to clipboard")) {
							ImGui::SetClipboardText(std::to_string(fdid).c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						// JS: <span @click.self="$core.view.copyToClipboard(displayName)">Copy texture name to clipboard</span>
						if (ImGui::MenuItem("Copy texture name to clipboard")) {
							ImGui::SetClipboardText(displayName.c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						// JS: <span @click.self="$core.view.copyToClipboard(fileName)">Copy file path to clipboard</span>
						if (ImGui::MenuItem("Copy file path to clipboard")) {
							ImGui::SetClipboardText(fileName.c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						// JS: <span @click.self="$core.view.copyToClipboard($core.view.getExportPath(fileName))">Copy export path to clipboard</span>
						if (ImGui::MenuItem("Copy export path to clipboard")) {
							ImGui::SetClipboardText(casc::ExportHelper::getExportPath(fileName).c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						ImGui::EndPopup();
					}
				}
			}

			// JS: <div id="model-texture-preview" v-if="$core.view.modelTexturePreviewURL.length > 0">
			if (!view.modelTexturePreviewURL.empty()) {
				// JS: <div id="model-texture-preview-toast" @click="$core.view.modelTexturePreviewURL = ''">Close Preview</div>
				if (ImGui::Button("Close Preview"))
					view.modelTexturePreviewURL.clear();

				// JS: <div class="image" :style="{ 'max-width': ... 'px', 'max-height': ... 'px' }">
				if (view.modelTexturePreviewTexID != 0) {
					const ImVec2 avail = ImGui::GetContentRegionAvail();
					const float tex_w = static_cast<float>(view.modelTexturePreviewWidth);
					const float tex_h = static_cast<float>(view.modelTexturePreviewHeight);
					const float scale = std::min(avail.x / tex_w, avail.y / tex_h);
					const ImVec2 img_size(tex_w * scale, tex_h * scale);

					const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
					ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.modelTexturePreviewTexID)), img_size);

					// JS: <div class="uv-overlay" v-if="modelTexturePreviewUVOverlay" ...>
					if (view.modelTexturePreviewUVTexID != 0 && !view.modelTexturePreviewUVOverlay.empty()) {
						ImGui::SetCursorScreenPos(cursor_pos);
						ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.modelTexturePreviewUVTexID)), img_size);
					}
				} else {
					ImGui::Text("Preview: %s (%dx%d)", view.modelTexturePreviewName.c_str(),
						view.modelTexturePreviewWidth, view.modelTexturePreviewHeight);
				}

				// JS: <div id="uv-layer-buttons" v-if="modelViewerUVLayers.length > 0">
				if (!view.modelViewerUVLayers.empty()) {
					for (const auto& layer : view.modelViewerUVLayers) {
						std::string layer_name = layer.value("name", std::string(""));
						bool is_active = layer.value("active", false);

						// JS: <button :class="{ active: layer.active }" @click="toggle_uv_layer(layer.name)">{{ layer.name }}</button>
						if (is_active)
							ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

						if (ImGui::Button(layer_name.c_str()))
							toggle_uv_layer(layer_name);

						if (is_active)
							ImGui::PopStyleColor();

						ImGui::SameLine();
					}
					ImGui::NewLine();
				}
			}

			// JS: <div class="preview-background" id="model-preview">
			// JS: <input v-if="config.modelViewerShowBackground" type="color" id="background-color-input" v-model="config.modelViewerBackgroundColor"/>
			if (view.config.value("modelViewerShowBackground", false)) {
				// JS: <input type="color" id="background-color-input" v-model="config.modelViewerBackgroundColor"/>
				std::string hex_str = view.config.value("modelViewerBackgroundColor", std::string("#343a40"));
				auto [cr, cg, cb] = model_viewer_gl::parse_hex_color(hex_str);
				float color[3] = {cr, cg, cb};
				if (ImGui::ColorEdit3("##bg_color_models", color, ImGuiColorEditFlags_NoInputs))
					view.config["modelViewerBackgroundColor"] = std::format("#{:02x}{:02x}{:02x}",
						static_cast<int>(color[0] * 255.0f), static_cast<int>(color[1] * 255.0f), static_cast<int>(color[2] * 255.0f));
			}

			// JS: <component :is="$components.ModelViewerGL" v-if="modelViewerContext" :context="modelViewerContext" />
			if (!view.modelViewerContext.is_null()) {
				model_viewer_gl::renderWidget("##model_viewer", viewer_state, viewer_context);
			}

			// JS: <div v-if="modelViewerAnims && modelViewerAnims.length > 0 && !modelTexturePreviewURL" class="preview-dropdown-overlay">
			if (!view.modelViewerAnims.empty() && view.modelTexturePreviewURL.empty()) {
				// JS: <select v-model="modelViewerAnimSelection">
				std::string current_label = "No Animation";
				std::string current_id;
				if (view.modelViewerAnimSelection.is_string())
					current_id = view.modelViewerAnimSelection.get<std::string>();

				for (const auto& anim : view.modelViewerAnims) {
					if (anim.value("id", std::string("")) == current_id) {
						current_label = anim.value("label", std::string(""));
						break;
					}
				}

				if (ImGui::BeginCombo("##ModelAnimSelect", current_label.c_str())) {
					for (const auto& anim : view.modelViewerAnims) {
						std::string anim_id = anim.value("id", std::string(""));
						std::string anim_label = anim.value("label", std::string(""));
						bool is_selected = (anim_id == current_id);

						if (ImGui::Selectable(anim_label.c_str(), is_selected))
							view.modelViewerAnimSelection = anim_id;

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				// JS: <div v-if="modelViewerAnimSelection !== 'none'" class="anim-controls">
				if (current_id != "none" && !current_id.empty()) {
					// JS: <button class="anim-btn anim-step-left" :class="{ disabled: !animPaused }" @click="step_animation(-1)">
					ImGui::SameLine();
					bool anim_paused = view.modelViewerAnimPaused;

					if (!anim_paused) ImGui::BeginDisabled();
					if (ImGui::Button("<<"))
						if (anim_methods) anim_methods->step_animation(-1);
					if (!anim_paused) ImGui::EndDisabled();

					// JS: <button class="anim-btn" :class="animPaused ? 'anim-play' : 'anim-pause'" @click="toggle_animation_pause()">
					ImGui::SameLine();
					if (ImGui::Button(anim_paused ? "Play" : "Pause"))
						if (anim_methods) anim_methods->toggle_animation_pause();

					// JS: <button class="anim-btn anim-step-right" :class="{ disabled: !animPaused }" @click="step_animation(1)">
					ImGui::SameLine();
					if (!anim_paused) ImGui::BeginDisabled();
					if (ImGui::Button(">>"))
						if (anim_methods) anim_methods->step_animation(1);
					if (!anim_paused) ImGui::EndDisabled();

					// JS: <div class="anim-scrubber" @mousedown="start_scrub" @mouseup="end_scrub">
					//         <input type="range" min="0" :max="animFrameCount - 1" :value="animFrame" @input="seek_animation($event.target.value)" />
					//         <div class="anim-frame-display">{{ animFrame }}</div>
					ImGui::SameLine();
					int frame = view.modelViewerAnimFrame;
					int frame_max = view.modelViewerAnimFrameCount > 0 ? view.modelViewerAnimFrameCount - 1 : 0;
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80.0f);
					if (ImGui::SliderInt("##ModelAnimFrame", &frame, 0, frame_max)) {
						if (anim_methods) anim_methods->seek_animation(frame);
					}
					// JS: start_scrub() pauses animation while dragging, end_scrub() resumes.
					if (ImGui::IsItemActivated()) {
						if (anim_methods) anim_methods->start_scrub();
					}
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						if (anim_methods) anim_methods->end_scrub();
					}
					ImGui::SameLine();
					ImGui::Text("%d", view.modelViewerAnimFrame);
				}
			}
		}
		app::layout::EndPreviewContainer();

		// --- Bottom: Export controls (row 2, col 2) ---
		// JS: <div class="preview-controls">
		//     <MenuButton :options="menuButtonModels" :default="config.exportModelFormat"
		//         @change="config.exportModelFormat = $event" :disabled="isBusy" @click="export_model" />
		if (app::layout::BeginPreviewControls("models-preview-controls", regions)) {
			std::vector<menu_button::MenuOption> mb_options;
			for (const auto& opt : view.menuButtonModels)
				mb_options.push_back({ opt.label, opt.value });
			menu_button::render("##MenuButtonModels", mb_options,
				view.config.value("exportModelFormat", std::string("OBJ")),
				view.isBusy > 0, false, menu_button_models_state,
				[&](const std::string& val) { view.config["exportModelFormat"] = val; },
				[&]() { export_model_action(); });
		}
		app::layout::EndPreviewControls();

		// --- Right panel: Sidebar (col 3, spanning both rows) ---
		// JS: <div id="model-sidebar" class="sidebar">
		if (app::layout::BeginSidebar("models-sidebar", regions)) {
			// JS: <span class="header">Preview</span>
			ImGui::SeparatorText("Preview");

			// JS: <label class="ui-checkbox" title="Automatically preview a model when selecting it">
			//         <input type="checkbox" v-model="config.modelsAutoPreview"/> <span>Auto Preview</span>
			{
				bool auto_preview = view.config.value("modelsAutoPreview", false);
				if (ImGui::Checkbox("Auto Preview", &auto_preview))
					view.config["modelsAutoPreview"] = auto_preview;
			}

			// JS: <label class="ui-checkbox" title="Automatically adjust camera when selecting a new model">
			//         <input type="checkbox" v-model="modelViewerAutoAdjust"/> <span>Auto Camera</span>
			ImGui::Checkbox("Auto Camera", &view.modelViewerAutoAdjust);

			// JS: <input type="checkbox" v-model="config.modelViewerShowGrid"/> <span>Show Grid</span>
			{
				bool show_grid = view.config.value("modelViewerShowGrid", true);
				if (ImGui::Checkbox("Show Grid", &show_grid))
					view.config["modelViewerShowGrid"] = show_grid;
			}

			// JS: <input type="checkbox" v-model="config.modelViewerWireframe"/> <span>Show Wireframe</span>
			{
				bool wireframe = view.config.value("modelViewerWireframe", false);
				if (ImGui::Checkbox("Show Wireframe", &wireframe))
					view.config["modelViewerWireframe"] = wireframe;
			}

			// JS: <input type="checkbox" v-model="config.modelViewerShowBones"/> <span>Show Bones</span>
			{
				bool show_bones = view.config.value("modelViewerShowBones", false);
				if (ImGui::Checkbox("Show Bones", &show_bones))
					view.config["modelViewerShowBones"] = show_bones;
			}

			// JS: <input type="checkbox" v-model="config.modelViewerShowTextures"/> <span>Show Textures</span>
			{
				bool show_textures = view.config.value("modelViewerShowTextures", true);
				if (ImGui::Checkbox("Show Textures", &show_textures))
					view.config["modelViewerShowTextures"] = show_textures;
			}

			// JS: <input type="checkbox" v-model="config.modelViewerShowBackground"/> <span>Show Background</span>
			{
				bool show_bg = view.config.value("modelViewerShowBackground", false);
				if (ImGui::Checkbox("Show Background", &show_bg))
					view.config["modelViewerShowBackground"] = show_bg;
			}

			// JS: <span class="header">Export</span>
			ImGui::SeparatorText("Export");

			// JS: <input type="checkbox" v-model="config.modelsExportTextures"/> <span>Textures</span>
			{
				bool export_tex = view.config.value("modelsExportTextures", true);
				if (ImGui::Checkbox("Textures", &export_tex))
					view.config["modelsExportTextures"] = export_tex;
			}

			// JS: <label v-if="config.modelsExportTextures"> <input type="checkbox" v-model="config.modelsExportAlpha"/> <span>Texture Alpha</span>
			if (view.config.value("modelsExportTextures", true)) {
				bool export_alpha = view.config.value("modelsExportAlpha", true);
				if (ImGui::Checkbox("Texture Alpha", &export_alpha))
					view.config["modelsExportAlpha"] = export_alpha;
			}

			// JS: <label v-if="exportModelFormat === 'GLTF' && modelViewerActiveType === 'm2'">
			//         <input type="checkbox" v-model="config.modelsExportAnimations"/> <span>Export animations</span>
			std::string export_format = view.config.value("exportModelFormat", std::string("OBJ"));
			if (export_format == "GLTF" && view.modelViewerActiveType == "m2") {
				bool export_anims = view.config.value("modelsExportAnimations", false);
				if (ImGui::Checkbox("Export animations", &export_anims))
					view.config["modelsExportAnimations"] = export_anims;
			}

			// JS: <template v-if="config.exportModelFormat === 'RAW'">
			if (export_format == "RAW") {
				// JS: <input type="checkbox" v-model="config.modelsExportSkin"/> <span>M2 .skin Files</span>
				{
					bool v = view.config.value("modelsExportSkin", false);
					if (ImGui::Checkbox("M2 .skin Files", &v))
						view.config["modelsExportSkin"] = v;
				}
				// JS: <input type="checkbox" v-model="config.modelsExportSkel"/> <span>M2 .skel Files</span>
				{
					bool v = view.config.value("modelsExportSkel", false);
					if (ImGui::Checkbox("M2 .skel Files", &v))
						view.config["modelsExportSkel"] = v;
				}
				// JS: <input type="checkbox" v-model="config.modelsExportBone"/> <span>M2 .bone Files</span>
				{
					bool v = view.config.value("modelsExportBone", false);
					if (ImGui::Checkbox("M2 .bone Files", &v))
						view.config["modelsExportBone"] = v;
				}
				// JS: <input type="checkbox" v-model="config.modelsExportAnim"/> <span>M2 .anim files</span>
				{
					bool v = view.config.value("modelsExportAnim", false);
					if (ImGui::Checkbox("M2 .anim files", &v))
						view.config["modelsExportAnim"] = v;
				}
				// JS: <input type="checkbox" v-model="config.modelsExportWMOGroups"/> <span>WMO Groups</span>
				{
					bool v = view.config.value("modelsExportWMOGroups", false);
					if (ImGui::Checkbox("WMO Groups", &v))
						view.config["modelsExportWMOGroups"] = v;
				}
			}

			// JS: <template v-if="config.exportModelFormat === 'OBJ' && modelViewerActiveType === 'wmo'">
			if (export_format == "OBJ" && view.modelViewerActiveType == "wmo") {
				// JS: <input type="checkbox" v-model="config.modelsExportSplitWMOGroups"/> <span>Split WMO Groups</span>
				bool v = view.config.value("modelsExportSplitWMOGroups", false);
				if (ImGui::Checkbox("Split WMO Groups", &v))
					view.config["modelsExportSplitWMOGroups"] = v;
			}

			// JS: <template v-if="modelViewerActiveType === 'm2'">
			if (view.modelViewerActiveType == "m2") {
				// JS: <span class="header">Geosets</span>
				ImGui::SeparatorText("Geosets");

				// JS: <component :is="$components.Checkboxlist" :items="modelViewerGeosets" />
				checkboxlist::render("##ModelGeosets", view.modelViewerGeosets, checkboxlist_geosets_state);

				// JS: <div class="list-toggles">
				//         <a @click="setAllGeosets(true, modelViewerGeosets)">Enable All</a> /
				//         <a @click="setAllGeosets(false, modelViewerGeosets)">Disable All</a>
				if (ImGui::SmallButton("Enable All##Geosets")) {
					for (auto& g : view.modelViewerGeosets)
						g["checked"] = true;
				}
				ImGui::SameLine();
				ImGui::Text("/");
				ImGui::SameLine();
				if (ImGui::SmallButton("Disable All##Geosets")) {
					for (auto& g : view.modelViewerGeosets)
						g["checked"] = false;
				}

				// JS: <template v-if="config.modelsExportTextures">
				if (view.config.value("modelsExportTextures", true)) {
					// JS: <span class="header">Skins</span>
					ImGui::SeparatorText("Skins");

					// JS: <component :is="$components.Listboxb" :items="modelViewerSkins" v-model:selection="modelViewerSkinsSelection" :single="true" />
					{
						// Convert json skins to ListboxBItem array.
						std::vector<listboxb::ListboxBItem> skin_items;
						skin_items.reserve(view.modelViewerSkins.size());
						for (const auto& skin : view.modelViewerSkins)
							skin_items.push_back({ skin.value("label", std::string("")) });

						// Build selection indices from modelViewerSkinsSelection.
						std::vector<int> sel_indices;
						for (const auto& sel : view.modelViewerSkinsSelection) {
							std::string sel_id = sel.value("id", std::string(""));
							for (size_t i = 0; i < view.modelViewerSkins.size(); ++i) {
								if (view.modelViewerSkins[i].value("id", std::string("")) == sel_id) {
									sel_indices.push_back(static_cast<int>(i));
									break;
								}
							}
						}

						listboxb::render("##ModelSkins", skin_items, sel_indices, true, true, false,
							listboxb_skins_state,
							[&](const std::vector<int>& new_sel) {
								view.modelViewerSkinsSelection.clear();
								for (int idx : new_sel) {
									if (idx >= 0 && idx < static_cast<int>(view.modelViewerSkins.size()))
										view.modelViewerSkinsSelection.push_back(view.modelViewerSkins[idx]);
								}
							});
					}
				}
			}

			// JS: <template v-if="modelViewerActiveType === 'wmo'">
			if (view.modelViewerActiveType == "wmo") {
				// JS: <span class="header">WMO Groups</span>
				ImGui::SeparatorText("WMO Groups");

				// JS: <component :is="$components.Checkboxlist" :items="modelViewerWMOGroups" />
				for (auto& group : view.modelViewerWMOGroups) {
					std::string label = group.value("label", std::string("Group"));
					bool checked = group.value("checked", true);
					if (ImGui::Checkbox(label.c_str(), &checked))
						group["checked"] = checked;
				}

				// JS: <a @click="setAllWMOGroups(true)">Enable All</a> / <a @click="setAllWMOGroups(false)">Disable All</a>
				if (ImGui::SmallButton("Enable All##WMOGroups")) {
					for (auto& g : view.modelViewerWMOGroups)
						g["checked"] = true;
				}
				ImGui::SameLine();
				ImGui::Text("/");
				ImGui::SameLine();
				if (ImGui::SmallButton("Disable All##WMOGroups")) {
					for (auto& g : view.modelViewerWMOGroups)
						g["checked"] = false;
				}

				// JS: <span class="header">Doodad Sets</span>
				ImGui::SeparatorText("Doodad Sets");

				// JS: <component :is="$components.Checkboxlist" :items="modelViewerWMOSets" />
				for (auto& set : view.modelViewerWMOSets) {
					std::string label = set.value("label", std::string("Set"));
					bool checked = set.value("checked", true);
					if (ImGui::Checkbox(label.c_str(), &checked))
						set["checked"] = checked;
				}
			}
		}
		app::layout::EndSidebar();
	}
	app::layout::EndTab();
}

} // namespace tab_models
