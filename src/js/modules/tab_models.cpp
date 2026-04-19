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
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <future>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_models {

// --- File-local state ---

static std::map<std::string, db::caches::DBCreatures::CreatureDisplayInfo> active_skins_creature;
static std::map<std::string, db::caches::DBItemDisplays::ItemDisplay> active_skins_item;

static std::vector<uint32_t> selected_variant_texture_ids;

static std::string selected_skin_name;
static bool has_selected_skin_name = false;

static model_viewer_utils::RendererResult active_renderer_result;

static std::string active_path;

// View state proxy (created once in mounted()).
static model_viewer_utils::ViewStateProxy view_state;

// Animation methods helper.
static std::unique_ptr<model_viewer_utils::AnimationMethods> anim_methods;

// Change-detection for watches (replaces Vue $watch).
static std::vector<nlohmann::json> prev_skins_selection;
static std::optional<std::string> prev_anim_selection;
static std::vector<nlohmann::json> prev_selection_models;
static bool tab_initialized = false;

static bool is_initialized = false;

static listbox::ListboxState listbox_models_state;

// Cached items string vectors — only rebuilt when the source JSON changes.
static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);
static std::vector<std::string> s_override_cache;
static size_t s_override_cache_size = ~size_t(0);

// Component states for CheckboxList, ListboxB, and MenuButton.
static checkboxlist::CheckboxListState checkboxlist_geosets_state;
static checkboxlist::CheckboxListState checkboxlist_wmo_groups_state;
static checkboxlist::CheckboxListState checkboxlist_wmo_sets_state;
static listboxb::ListboxBState listboxb_skins_state;
static menu_button::MenuButtonState menu_button_models_state;

// Model viewer GL state/context (replaces Vue <ModelViewerGL :context="modelViewerContext"/>).
static model_viewer_gl::State viewer_state;
static model_viewer_gl::Context viewer_context;

struct PendingPreviewTask {
	std::string file_name;
	uint32_t file_data_id = 0;
	std::future<BufferWrapper> file_future;
	std::unique_ptr<BusyLock> busy_lock;
};

static std::optional<PendingPreviewTask> pending_preview_task;
static std::optional<std::string> queued_preview_file_name;

struct PendingExportTask {
	bool is_local = false;
	int export_id = -1;
	std::vector<nlohmann::json> files;
	size_t next_index = 0;
	std::string format;
	nlohmann::json manifest;
	std::optional<FileWriter> export_paths;
	std::optional<casc::ExportHelper> helper;
	bool preview_only = false;
};

static std::optional<PendingExportTask> pending_export_task;

// --- Internal helpers ---

static M2RendererGL* get_active_m2_renderer() {
	return active_renderer_result.m2.get();
}

static void dispose_active_renderer() {
	if (active_renderer_result.m2)
		active_renderer_result.m2->dispose();
	if (active_renderer_result.m3)
		active_renderer_result.m3->dispose();
	if (active_renderer_result.wmo)
		active_renderer_result.wmo->dispose();

	active_renderer_result = model_viewer_utils::RendererResult{};
	active_path.clear();
}

static model_viewer_utils::ViewStateProxy* get_view_state_ptr() {
	return &view_state;
}

struct DisplayVariant {
	uint32_t ID = 0;
	std::vector<uint32_t> textures;
	std::optional<std::vector<uint32_t>> extraGeosets;
	bool from_creature = false;
};

static std::vector<DisplayVariant> get_model_displays(uint32_t file_data_id) {
	const auto* creature_displays = db::caches::DBCreatures::getCreatureDisplaysByFileDataID(file_data_id);
	if (creature_displays) {
		std::vector<DisplayVariant> result;
		result.reserve(creature_displays->size());
		for (const auto& d_ref : *creature_displays) {
			const auto& d = d_ref.get();
			result.push_back({ d.ID, d.textures, d.extraGeosets, true });
		}
		return result;
	}

	const auto* item_displays = db::caches::DBItemDisplays::getItemDisplaysByFileDataID(file_data_id);
	if (item_displays) {
		std::vector<DisplayVariant> result;
		result.reserve(item_displays->size());
		for (const auto& d : *item_displays)
			result.push_back({ d.ID, d.textures, {}, false });
		return result;
	}

	return {};
}

static void preview_model(const std::string& file_name) {
	if (pending_preview_task.has_value()) {
		queued_preview_file_name = file_name;
		return;
	}

	queued_preview_file_name.reset();

	core::setToast("progress", std::format("Loading {}, please wait...", file_name), {}, -1, false);
	logging::write(std::format("Previewing model {}", file_name));

	auto& state = view_state;
	texture_ribbon::reset();
	model_viewer_utils::clear_texture_preview(state);

	auto& view = *core::view;
	view.modelViewerSkins.clear();
	view.modelViewerSkinsSelection.clear();
	view.modelViewerAnims.clear();
	view.modelViewerAnimSelection = nullptr;

	try {
		if (active_renderer_result.type != model_viewer_utils::ModelType::Unknown) {
			dispose_active_renderer();
		}

		active_skins_creature.clear();
		active_skins_item.clear();
		selected_variant_texture_ids.clear();
		selected_skin_name.clear();
		has_selected_skin_name = false;

		auto file_data_id_opt = casc::listfile::getByFilename(file_name);
		if (!file_data_id_opt.has_value()) {
			core::setToast("error", std::format("Unable to find file data ID for {}", file_name), {}, -1);
			return;
		}
		auto* casc = core::view->casc;
		if (!casc) {
			core::setToast("error", "CASC source is not available.", {}, -1);
			return;
		}

		PendingPreviewTask task;
		task.file_name = file_name;
		task.file_data_id = file_data_id_opt.value();
		task.busy_lock = std::make_unique<BusyLock>(core::create_busy_lock());
		task.file_future = std::async(std::launch::async, [casc, file_data_id = task.file_data_id]() {
			return casc->getVirtualFileByID(file_data_id);
		});
		pending_preview_task = std::move(task);
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to preview model " + file_name, {}, -1);
		logging::write(std::format("Failed to queue model preview: {}", e.what()));
	}
}

static void pump_preview_model_task() {
	if (!pending_preview_task.has_value())
		return;

	auto& task = *pending_preview_task;
	if (task.file_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	auto clear_task = [&]() {
		task.busy_lock.reset();
		pending_preview_task.reset();
	};

	auto queue_next_preview = [&]() {
		if (!queued_preview_file_name.has_value())
			return;

		const std::string next_file = *queued_preview_file_name;
		queued_preview_file_name.reset();

		if (!next_file.empty())
			preview_model(next_file);
	};

	try {
		BufferWrapper file = task.file_future.get();
		auto& view = *core::view;

		gl::GLContext* gl_ctx = viewer_context.gl_context;
		if (!gl_ctx) {
			core::setToast("error", "GL context not available — model viewer not initialized.", {}, -1);
			clear_task();
			queue_next_preview();
			return;
		}

		auto model_type = model_viewer_utils::detect_model_type_by_name(task.file_name);
		if (model_type == model_viewer_utils::ModelType::Unknown)
			model_type = model_viewer_utils::detect_model_type(file);

		if (model_type == model_viewer_utils::ModelType::M2)
			view.modelViewerActiveType = "m2";
		else if (model_type == model_viewer_utils::ModelType::M3)
			view.modelViewerActiveType = "m3";
		else
			view.modelViewerActiveType = "wmo";

		active_renderer_result = model_viewer_utils::create_renderer(
			file, model_type, *gl_ctx,
			view.config.value("modelViewerShowTextures", true),
			task.file_data_id,
			task.file_name
		);

		if (active_renderer_result.m2)
			active_renderer_result.m2->load();
		else if (active_renderer_result.m3)
			active_renderer_result.m3->load();
		else if (active_renderer_result.wmo)
			active_renderer_result.wmo->load();

		if (model_type == model_viewer_utils::ModelType::M2) {
			const auto displays = get_model_displays(task.file_data_id);

			std::vector<nlohmann::json> skin_list;
			std::string model_name = casc::listfile::getByID(task.file_data_id).value_or("");
			{
				auto pos = model_name.rfind('/');
				if (pos != std::string::npos) model_name = model_name.substr(pos + 1);
				pos = model_name.rfind('\\');
				if (pos != std::string::npos) model_name = model_name.substr(pos + 1);
				if (model_name.size() >= 2 && model_name.compare(model_name.size() - 2, 2, "m2") == 0)
					model_name = model_name.substr(0, model_name.size() - 2);
			}

			for (const auto& display : displays) {
				if (display.textures.empty())
					continue;

				uint32_t texture = display.textures[0];

				std::string clean_skin_name;
				std::string skin_name = casc::listfile::getByID(texture).value_or("");

				if (!skin_name.empty()) {
					{
						auto pos = skin_name.rfind('/');
						if (pos != std::string::npos) skin_name = skin_name.substr(pos + 1);
						pos = skin_name.rfind('\\');
						if (pos != std::string::npos) skin_name = skin_name.substr(pos + 1);
						if (skin_name.size() > 4 && skin_name.substr(skin_name.size() - 4) == ".blp")
							skin_name = skin_name.substr(0, skin_name.size() - 4);
					}
					clean_skin_name = skin_name;
					auto found = clean_skin_name.find(model_name);
					if (found != std::string::npos)
						clean_skin_name.erase(found, model_name.size());
					found = clean_skin_name.find('_');
					if (found != std::string::npos)
						clean_skin_name.erase(found, 1);
				} else {
					skin_name = "unknown_" + std::to_string(texture);
				}

				if (clean_skin_name.empty())
					clean_skin_name = "base";

				if (display.extraGeosets.has_value() && !display.extraGeosets->empty()) {
					std::string geo_str;
					for (size_t g = 0; g < display.extraGeosets->size(); ++g) {
						if (g > 0) geo_str += ',';
						geo_str += std::to_string((*display.extraGeosets)[g]);
					}
					skin_name += geo_str;
				}

				clean_skin_name += " (" + std::to_string(display.ID) + ")";

				if (active_skins_creature.contains(skin_name) || active_skins_item.contains(skin_name))
					continue;

				skin_list.push_back({ {"id", skin_name}, {"label", clean_skin_name} });

				if (display.from_creature) {
					active_skins_creature[skin_name] = {
						display.ID, 0, 0, display.textures, display.extraGeosets
					};
				} else {
					active_skins_item[skin_name] = { display.ID, display.textures };
				}
			}

			view.modelViewerSkins = skin_list;
			if (!skin_list.empty())
				view.modelViewerSkinsSelection = { skin_list[0] };
			else
				view.modelViewerSkinsSelection.clear();

			if (active_renderer_result.m2)
				view.modelViewerAnims = model_viewer_utils::extract_animations(*active_renderer_result.m2);
			view.modelViewerAnimSelection = "none";
		}

		active_path = task.file_name;

		// JS: const has_content = active_renderer.draw_calls?.length > 0 || active_renderer.groups?.length > 0;
		bool has_content = false;
		if (active_renderer_result.m2)
			has_content = !active_renderer_result.m2->get_draw_calls().empty();
		else if (active_renderer_result.m3)
			has_content = !active_renderer_result.m3->get_draw_calls().empty();
		else if (active_renderer_result.wmo)
			has_content = !active_renderer_result.wmo->get_groups().empty();

		if (!has_content) {
			core::setToast("info", std::format("The model {} doesn't have any 3D data associated with it.", task.file_name), {}, 4000);
		} else {
			core::hideToast();

			if (view.modelViewerAutoAdjust && viewer_context.fitCamera)
				viewer_context.fitCamera();
		}
	} catch (const casc::EncryptionError& e) {
		core::setToast("error", std::format("The model {} is encrypted with an unknown key ({}).", task.file_name, e.key), {}, -1);
		logging::write(std::format("Failed to decrypt model {} ({})", task.file_name, e.key));
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to preview model " + task.file_name, {}, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}

	clear_task();
	queue_next_preview();
}

static std::vector<uint32_t> get_variant_texture_ids(const std::string& file_name) {
	if (file_name == active_path)
		return selected_variant_texture_ids;

	auto file_data_id_opt = casc::listfile::getByFilename(file_name);
	if (!file_data_id_opt.has_value())
		return {};

	auto displays = get_model_displays(file_data_id_opt.value());

	for (const auto& d : displays) {
		if (!d.textures.empty())
			return d.textures;
	}
	return {};
}

// --- Template methods (mapped from Vue methods) ---

static void handle_listbox_context(const std::vector<std::string>& selection) {
	listbox_context::handle_context_menu(selection);
}

static void copy_file_paths(const std::vector<std::string>& selection) {
	listbox_context::copy_file_paths(selection);
}

static void copy_listfile_format(const std::vector<std::string>& selection) {
	listbox_context::copy_listfile_format(selection);
}

static void copy_file_data_ids(const std::vector<std::string>& selection) {
	listbox_context::copy_file_data_ids(selection);
}

static void copy_export_paths(const std::vector<std::string>& selection) {
	listbox_context::copy_export_paths(selection);
}

static void open_export_directory(const std::vector<std::string>& selection) {
	listbox_context::open_export_directory(selection);
}

static void preview_texture(uint32_t file_data_id, const std::string& display_name) {
	model_viewer_utils::preview_texture_by_id(
		view_state, get_active_m2_renderer(),
		file_data_id, display_name, core::view->casc
	);
}

static void export_ribbon_texture(uint32_t file_data_id, [[maybe_unused]] const std::string& display_name) {
	texture_exporter::exportSingleTexture(file_data_id, core::view->casc);
}

static void toggle_uv_layer(const std::string& layer_name) {
	model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
}

static void export_model_action() {
	auto& view = *core::view;

	const auto& user_selection = view.selectionModels;

	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	export_files(user_selection, false);
}

static void initialize() {
	auto& view = *core::view;

	int step_count = 2;
	if (view.config.value("enableUnknownFiles", false)) step_count++;
	if (view.config.value("enableM2Skins", false)) step_count += 2;

	core::showLoadingScreen(step_count);

	core::progressLoadingScreen("Loading model file data...");
	db::caches::DBModelFileData::initializeModelFileData();

	if (view.config.value("enableUnknownFiles", false)) {
		core::progressLoadingScreen("Loading unknown models...");
		casc::listfile::loadUnknownModels();
	}

	if (view.config.value("enableM2Skins", false)) {
		core::progressLoadingScreen("Loading item displays...");
		db::caches::DBItemDisplays::initializeItemDisplays();

		core::progressLoadingScreen("Loading creature data...");
		db::caches::DBCreatures::initializeCreatureData();
	}

	core::progressLoadingScreen("Initializing 3D preview...");

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

	core::hideLoadingScreen();
}

// --- Watch handler for modelViewerSkinsSelection ---
static void handle_skins_selection_change(const std::vector<nlohmann::json>& selection) {
	auto& view = *core::view;

	if (active_renderer_result.type == model_viewer_utils::ModelType::Unknown)
		return;
	if (active_skins_creature.empty() && active_skins_item.empty())
		return;

	if (selection.empty())
		return;
	const auto& selected = selection[0];
	std::string sel_id = selected.value("id", std::string(""));
	if (sel_id.empty())
		return;

	selected_skin_name = sel_id;
	has_selected_skin_name = true;

	// Try creature display first, then item display.
	bool has_extra_geosets = false;
	std::vector<uint32_t> extra_geosets;
	std::vector<uint32_t> display_textures;
	auto it_creature = active_skins_creature.find(sel_id);
	auto it_item = active_skins_item.find(sel_id);
	if (it_creature == active_skins_creature.end() && it_item == active_skins_item.end())
		return;

	if (it_creature != active_skins_creature.end()) {
		const auto& display = it_creature->second;
		if (display.extraGeosets.has_value()) {
			extra_geosets = *display.extraGeosets;
			has_extra_geosets = true;
		}
		display_textures = display.textures;
	} else {
		const auto& display = it_item->second;
		display_textures = display.textures;
	}

	auto& curr_geosets = view.modelViewerGeosets;

	if (has_extra_geosets) {
		for (auto& geoset : curr_geosets) {
			int gid = geoset.value("id", 0);
			if (gid > 0 && gid < 900)
				geoset["checked"] = false;
		}

		//         for (const geoset of curr_geosets) { if (geoset.id === extra_geoset) geoset.checked = true; } }
		for (uint32_t extra_geoset : extra_geosets) {
			for (auto& geoset : curr_geosets) {
				if (geoset.value("id", 0u) == extra_geoset)
					geoset["checked"] = true;
			}
		}
	} else {
		//         geoset.checked = (id.endsWith('0') || id.endsWith('01')); }
		for (auto& geoset : curr_geosets) {
			std::string id_str = std::to_string(geoset.value("id", 0));
			bool checked = id_str.ends_with("0") || id_str.ends_with("01");
			geoset["checked"] = checked;
		}
	}

	if (!display_textures.empty())
		selected_variant_texture_ids = display_textures;

	if (active_renderer_result.m2 && !display_textures.empty()) {
		M2DisplayInfo info;
		info.textures = display_textures;
		active_renderer_result.m2->applyReplaceableTextures(info);
	}
}

// --- Public API ---

void registerTab() {
	modules::register_nav_button("tab_models", "Models", "cube.svg", install_type::CASC);
}

void mounted() {
	auto& view = *core::view;

	view_state = model_viewer_utils::create_view_state("model");

	//         ext: ['.m2'],
	//         prompt: count => util.format('Export %d models as %s', count, this.$core.view.config.exportModelFormat),
	//         process: files => export_files(this.$core, files, true)
	//     });
	core::registerDropHandler({
		{".m2"},
		[]() -> std::string {
			return std::format("Export models as {}", core::view->config.value("exportModelFormat", std::string("OBJ")));
		},
		[](const std::vector<std::string>& files) {
			// JS: process: files => export_files(this.$core, files, true)
			std::vector<nlohmann::json> entries;
			entries.reserve(files.size());
			for (const auto& file : files) {
				nlohmann::json entry;
				entry["fileName"] = file;
				entries.push_back(std::move(entry));
			}
			export_files(entries, true);
		}
	});

	initialize();

	// Create animation methods helper.
	anim_methods = std::make_unique<model_viewer_utils::AnimationMethods>(
		get_active_m2_renderer,
		get_view_state_ptr
	);

	// Store initial state for change-detection.

	prev_skins_selection = view.modelViewerSkinsSelection;

	if (view.modelViewerAnimSelection.is_string())
		prev_anim_selection = view.modelViewerAnimSelection.get<std::string>();
	else
		prev_anim_selection.reset();

	prev_selection_models = view.selectionModels;

	core::events.on("toggle-uv-layer", EventEmitter::ArgCallback([](const std::any& arg) {
		const auto& layer_name = std::any_cast<const std::string&>(arg);
		model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
	}));

	is_initialized = true;
}

void export_files(const std::vector<nlohmann::json>& files, bool is_local, int export_id) {
	if (pending_export_task.has_value())
		return;

	auto& view = *core::view;

	PendingExportTask task;
	task.is_local = is_local;
	task.export_id = export_id;
	task.files = files;
	task.format = view.config.value("exportModelFormat", std::string("OBJ"));
	task.export_paths.emplace(core::openLastExportStream());
	task.manifest = {
		{"type", "MODELS"},
		{"exportID", export_id},
		{"succeeded", nlohmann::json::array()},
		{"failed", nlohmann::json::array()}
	};
	task.preview_only = (task.format == "PNG" || task.format == "CLIPBOARD");

	if (!task.preview_only)
		task.helper.emplace(static_cast<int>(files.size()), "model");

	pending_export_task = std::move(task);
}

static void finish_pending_export_task() {
	pending_export_task.reset();
}

static void pump_export_task() {
	if (!pending_export_task.has_value())
		return;

	auto& task = *pending_export_task;
	auto& view = *core::view;
	FileWriter* export_paths = task.export_paths.has_value() ? &task.export_paths.value() : nullptr;

	if (task.preview_only) {
		if (!active_path.empty()) {
			gl::GLContext* gl_ctx = viewer_context.gl_context;
			if (gl_ctx && viewer_state.fbo != 0) {
				glBindFramebuffer(GL_FRAMEBUFFER, viewer_state.fbo);
				model_viewer_utils::export_preview(task.format, *gl_ctx, active_path);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		} else {
			core::setToast("error", "The selected export option only works for model previews. Preview something first!", {}, -1);
		}

		finish_pending_export_task();
		return;
	}

	auto& helper = task.helper.value();
	if (task.next_index == 0)
		helper.start();

	if (helper.isCancelled()) {
		finish_pending_export_task();
		return;
	}

	if (task.next_index >= task.files.size()) {
		helper.finish();
		finish_pending_export_task();
		return;
	}

	auto* casc = view.casc;
	const auto& file_entry = task.files[task.next_index++];

	std::string file_name;
	uint32_t file_data_id = 0;

	if (file_entry.is_number()) {
		file_data_id = file_entry.get<uint32_t>();
		file_name = casc::listfile::getByID(file_data_id).value_or("");
	} else {
		file_name = casc::listfile::stripFileEntry(file_entry.get<std::string>());
		auto opt = casc::listfile::getByFilename(file_name);
		if (opt.has_value())
			file_data_id = opt.value();
	}

	std::vector<nlohmann::json> file_manifest;

	try {
		BufferWrapper data = task.is_local ? BufferWrapper::readFile(file_name) : casc->getVirtualFileByID(file_data_id);

		if (file_name.empty()) {
			auto detected_type = model_viewer_utils::detect_model_type(data);
			file_name = casc::listfile::formatUnknownFile(file_data_id, model_viewer_utils::get_model_extension(detected_type));
		}

		std::string export_path;
		std::string mark_file_name = file_name;

		bool is_active = (file_name == active_path);

		auto model_type = model_viewer_utils::detect_model_type_by_name(file_name);
		if (model_type == model_viewer_utils::ModelType::Unknown)
			model_type = model_viewer_utils::detect_model_type(data);

		if (task.is_local) {
			export_path = file_name;
		} else if (model_type == model_viewer_utils::ModelType::M2 && has_selected_skin_name && is_active && task.format != "RAW") {
			std::filesystem::path fp(file_name);
			std::string base_file_name = fp.stem().string();

			std::string skinned_name;
			if (selected_skin_name.starts_with(base_file_name))
				skinned_name = casc::ExportHelper::replaceBaseName(file_name, selected_skin_name);
			else
				skinned_name = casc::ExportHelper::replaceBaseName(file_name, base_file_name + "_" + selected_skin_name);

			export_path = casc::ExportHelper::getExportPath(skinned_name);
			mark_file_name = skinned_name;
		} else {
			export_path = casc::ExportHelper::getExportPath(file_name);
		}

		model_viewer_utils::ExportModelOptions opts;
		opts.data = &data;
		opts.file_data_id = file_data_id;
		opts.file_name = file_name;
		opts.format = task.format;
		opts.export_path = export_path;
		opts.helper = &helper;
		opts.casc = casc;
		opts.file_manifest = &file_manifest;

		auto vtids = get_variant_texture_ids(file_name);
		for (uint32_t id : vtids)
			opts.variant_textures.push_back(id);

		if (is_active) {
			opts.geoset_mask = &view.modelViewerGeosets;
			opts.wmo_group_mask = &view.modelViewerWMOGroups;
			opts.wmo_set_mask = &view.modelViewerWMOSets;
		}

		opts.export_paths = export_paths;

		std::string mark_name = model_viewer_utils::export_model(opts);

		helper.mark(mark_name, true);

		task.manifest["succeeded"].push_back({
			{"fileDataID", file_data_id},
			{"files", file_manifest}
		});
	} catch (const std::exception& e) {
		helper.mark(file_name, false, e.what());
		task.manifest["failed"].push_back({ {"fileDataID", file_data_id} });
	}
}

std::variant<std::monostate, M2RendererGL*, M3RendererGL*, WMORendererGL*> getActiveRenderer() {
	if (active_renderer_result.m2)
		return active_renderer_result.m2.get();
	if (active_renderer_result.m3)
		return active_renderer_result.m3.get();
	if (active_renderer_result.wmo)
		return active_renderer_result.wmo.get();
	return std::monostate{};
}

void render() {
	auto& view = *core::view;

	if (!is_initialized)
		return;

	pump_preview_model_task();
	pump_export_task();

	// Watch: modelViewerSkinsSelection → apply skin textures and geosets
	{
		if (view.modelViewerSkinsSelection != prev_skins_selection) {
			prev_skins_selection = view.modelViewerSkinsSelection;
			handle_skins_selection_change(view.modelViewerSkinsSelection);
		}
	}

	// Watch: modelViewerAnimSelection → handle_animation_change
	{
		std::optional<std::string> current_anim;
		if (view.modelViewerAnimSelection.is_string())
			current_anim = view.modelViewerAnimSelection.get<std::string>();

		if (current_anim != prev_anim_selection) {
			prev_anim_selection = current_anim;

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
	{
		if (view.selectionModels != prev_selection_models) {
			prev_selection_models = view.selectionModels;

			if (tab_initialized && view.config.value("modelsAutoPreview", false)) {
				if (!view.selectionModels.empty()) {
					std::string first;
					if (view.selectionModels[0].is_string())
						first = casc::listfile::stripFileEntry(view.selectionModels[0].get<std::string>());

					if (view.isBusy == 0 && !first.empty() && active_path != first)
						preview_model(first);
				}
			}
		}
	}


	if (app::layout::BeginTab("tab-models")) {
		auto regions = app::layout::CalcListTabRegions(true);

		// --- Left panel: List container (row 1, col 1) ---
		//     <Listbox v-model:selection="selectionModels" v-model:filter="userInputFilterModels"
		//         :items="listfileModels" :override="overrideModelList" :keyinput="true"
		//         :regex="config.regexFilters" :copymode="config.copyMode" ... @contextmenu="handle_listbox_context" />
		if (app::layout::BeginListContainer("models-list-container", regions)) {
			const auto& items_str = core::cached_json_strings(view.listfileModels, s_items_cache, s_items_cache_size);

			std::vector<std::string> selection_str;
			for (const auto& s : view.selectionModels)
				selection_str.push_back(s.get<std::string>());

			listbox::CopyMode copy_mode = listbox::CopyMode::Default;
			{
				std::string cm = view.config.value("copyMode", std::string("Default"));
				if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
				else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
			}

			// Convert override list (cached — usually empty).
			const auto& override_str = core::cached_json_strings(view.overrideModelList, s_override_cache, s_override_cache_size);

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
				view.modelQuickFilters,  // quickfilters
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

					if (ImGui::MenuItem(std::format("Copy file path{}", count > 1 ? "s" : "").c_str())) {
						copy_file_paths(sel_strings);
						view.contextMenus.nodeListbox = nullptr;
					}

					if (hasFileDataIDs) {
						if (ImGui::MenuItem(std::format("Copy file path{} (listfile format)", count > 1 ? "s" : "").c_str())) {
							copy_listfile_format(sel_strings);
							view.contextMenus.nodeListbox = nullptr;
						}
					}

					if (hasFileDataIDs) {
						if (ImGui::MenuItem(std::format("Copy file data ID{}", count > 1 ? "s" : "").c_str())) {
							copy_file_data_ids(sel_strings);
							view.contextMenus.nodeListbox = nullptr;
						}
					}

					if (ImGui::MenuItem(std::format("Copy export path{}", count > 1 ? "s" : "").c_str())) {
						copy_export_paths(sel_strings);
						view.contextMenus.nodeListbox = nullptr;
					}

					if (ImGui::MenuItem("Open export directory")) {
						open_export_directory(sel_strings);
						view.contextMenus.nodeListbox = nullptr;
					}

					ImGui::EndPopup();
				}
			}
		}
		app::layout::EndListContainer();

		// --- Status bar (between list and filter) ---
		if (app::layout::BeginStatusBar("models-status", regions)) {
			listbox::renderStatusBar("model", view.modelQuickFilters, listbox_models_state);
		}
		app::layout::EndStatusBar();

		// --- Filter bar (row 2, col 1) ---
		if (app::layout::BeginFilterBar("models-filter", regions)) {
			// <div class="regex-info" v-if="config.regexFilters" :title="regexTooltip">Regex Enabled</div>
			bool regexEnabled = view.config.value("regexFilters", false);
			float inputWidth = ImGui::GetContentRegionAvail().x;
			if (regexEnabled) {
				// Render "Regex Enabled" badge right-aligned.
				// CSS: .filter > .regex-info { position: absolute; right: 30px; background: var(--border);
				//       border-radius: 3px; padding: 2px 6px; font-size: 0.8em; }
				const char* regexLabel = "Regex Enabled";
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));
				float badgeWidth = ImGui::CalcTextSize(regexLabel).x + 12.0f;
				float rightPad = 10.0f;
				float badgeX = ImGui::GetContentRegionAvail().x - badgeWidth - rightPad;
				inputWidth = badgeX - 5.0f; // leave gap before badge
				// Render the badge at the right
				ImGui::SameLine(ImGui::GetCursorPosX() + badgeX);
				ImGui::PushStyleColor(ImGuiCol_Button, app::theme::BORDER);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, app::theme::BORDER);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, app::theme::BORDER);
				ImGui::SmallButton(regexLabel);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", view.regexTooltip.c_str());
				ImGui::PopStyleColor(3);
				ImGui::PopStyleVar();
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMin().x);
			}
			ImGui::SetNextItemWidth(inputWidth);
			char filter_buf[256] = {};
			std::strncpy(filter_buf, view.userInputFilterModels.c_str(), sizeof(filter_buf) - 1);
			if (ImGui::InputTextWithHint("##FilterModels", "Filter models...", filter_buf, sizeof(filter_buf)))
				view.userInputFilterModels = filter_buf;
		}
		app::layout::EndFilterBar();

		// --- Middle panel: Preview container (row 1, col 2) ---
		if (app::layout::BeginPreviewContainer("models-preview-container", regions)) {
			// Save the preview container origin and size for absolute overlay positioning.
			const ImVec2 previewOrigin = ImGui::GetCursorScreenPos();
			const ImVec2 previewSize = ImGui::GetContentRegionAvail();

			// --- 887: Checkerboard background pattern ---
			// CSS: .preview-container .preview-background { background-image: linear-gradient(45deg, ...);
			//       background-size: 30px 30px; border: 1px solid var(--border); box-shadow: black 0 0 3px 0; }
			{
				ImDrawList* dl = ImGui::GetWindowDrawList();
				const float checkSize = 15.0f; // half of 30px background-size
				const ImU32 colA = IM_COL32(35, 35, 35, 255);  // --trans-check-a (#232323)
				const ImU32 colB = IM_COL32(40, 40, 40, 255);  // --trans-check-b (#282828)
				for (float y = previewOrigin.y; y < previewOrigin.y + previewSize.y; y += checkSize) {
					for (float x = previewOrigin.x; x < previewOrigin.x + previewSize.x; x += checkSize) {
						int ix = static_cast<int>((x - previewOrigin.x) / checkSize);
						int iy = static_cast<int>((y - previewOrigin.y) / checkSize);
						ImU32 col = ((ix + iy) % 2 == 0) ? colA : colB;
						ImVec2 p0(x, y);
						ImVec2 p1(std::min(x + checkSize, previewOrigin.x + previewSize.x),
						          std::min(y + checkSize, previewOrigin.y + previewSize.y));
						dl->AddRectFilled(p0, p1, col);
					}
				}
				// Border: 1px solid var(--border)
				dl->AddRect(previewOrigin, ImVec2(previewOrigin.x + previewSize.x, previewOrigin.y + previewSize.y),
					app::theme::BORDER_U32, 0.0f, 0, 1.0f);
			}

			// Render the texture preview overlay if active (z-index: 1 over 3D viewport).
			// --- 889, 890, 891: Texture preview with toast, UV overlay, UV layer buttons ---
			if (!view.modelTexturePreviewURL.empty()) {
				// CSS: #model-texture-preview { position: absolute; z-index: 1; top: 0; ... }
				// Use the full preview area for the texture preview.
				if (view.modelTexturePreviewTexID != 0) {
					const ImVec2 avail = previewSize;
					const float tex_w = static_cast<float>(view.modelTexturePreviewWidth);
					const float tex_h = static_cast<float>(view.modelTexturePreviewHeight);
					const float scale = std::min(avail.x / tex_w, avail.y / tex_h);
					const ImVec2 img_size(tex_w * scale, tex_h * scale);

					// Center the image in the preview area.
					const float imgX = previewOrigin.x + (avail.x - img_size.x) * 0.5f;
					const float imgY = previewOrigin.y + (avail.y - img_size.y) * 0.5f;
					ImGui::SetCursorScreenPos(ImVec2(imgX, imgY));
					const ImVec2 imgPos = ImGui::GetCursorScreenPos();
					ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.modelTexturePreviewTexID)), img_size);

					// --- 891: UV overlay positioned absolute over texture preview ---
					if (view.modelTexturePreviewUVTexID != 0 && !view.modelTexturePreviewUVOverlay.empty()) {
						ImGui::SetCursorScreenPos(imgPos);
						ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.modelTexturePreviewUVTexID)), img_size);
					}

					// --- 891: UV layer buttons absolute top-left (top: 10px, left: 10px) ---
					if (!view.modelViewerUVLayers.empty()) {
						ImGui::SetCursorScreenPos(ImVec2(previewOrigin.x + 10.0f, previewOrigin.y + 10.0f));
						for (const auto& layer : view.modelViewerUVLayers) {
							std::string layer_name = layer.value("name", std::string(""));
							bool is_active = layer.value("active", false);

							// --- 890: UV layer button styling ---
							// CSS: .uv-layer-button { background: rgba(0,0,0,0.7); border: 1px solid var(--border);
							//       padding: 5px 10px; font-size: 12px; }
							// CSS: .uv-layer-button.active { border-color: #00ff00; color: #00ff00; }
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 5.0f));
							if (is_active) {
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
								ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
							} else {
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
								ImGui::PushStyleColor(ImGuiCol_Border, app::theme::BORDER);
								ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_PRIMARY);
							}
							ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

							if (ImGui::Button(layer_name.c_str()))
								toggle_uv_layer(layer_name);

							ImGui::PopStyleVar(2);
							ImGui::PopStyleColor(4);

							ImGui::SameLine();
						}
					}
				} else {
					ImGui::SetCursorScreenPos(previewOrigin);
					ImGui::Text("Preview: %s (%dx%d)", view.modelTexturePreviewName.c_str(),
						view.modelTexturePreviewWidth, view.modelTexturePreviewHeight);
				}

				// --- 889: "Close Preview" toast overlay ---
				// CSS: #model-texture-preview-toast { position: absolute; top: 10px; right: 10px;
				//       background: rgba(0,0,0,0.5); border: 1px solid var(--border); border-radius: 3px;
				//       padding: 5px 10px; font-size: 12px; cursor: pointer; z-index: 2; }
				{
					const char* closeLabel = "Close Preview";
					ImVec2 textSize = ImGui::CalcTextSize(closeLabel);
					float btnW = textSize.x + 20.0f;
					float btnH = textSize.y + 10.0f;
					ImGui::SetCursorScreenPos(ImVec2(
						previewOrigin.x + previewSize.x - btnW - 10.0f,
						previewOrigin.y + 10.0f));

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 5.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
					ImGui::PushStyleColor(ImGuiCol_Border, app::theme::BORDER);

					if (ImGui::Button(closeLabel))
						view.modelTexturePreviewURL.clear();

					ImGui::PopStyleColor(4);
					ImGui::PopStyleVar(3);
				}
			} else {
				// No texture preview — render 3D viewport and overlays.

				// --- 888: Background color picker absolute top-right ---
				// CSS: #background-color-input { position: absolute; top: 10px; right: 10px; width: 24px; height: 24px;
				//       border: 2px solid var(--border); border-radius: 4px; z-index: 100; }
				// Render the 3D viewport first, then overlay the color picker.
				if (!view.modelViewerContext.is_null()) {
					ImGui::SetCursorScreenPos(previewOrigin);
					model_viewer_gl::renderWidget("##model_viewer", viewer_state, viewer_context);
				}

				if (view.config.value("modelViewerShowBackground", false)) {
					std::string hex_str = view.config.value("modelViewerBackgroundColor", std::string("#343a40"));
					auto [cr, cg, cb] = model_viewer_gl::parse_hex_color(hex_str);
					float color[3] = {cr, cg, cb};
					ImGui::SetCursorScreenPos(ImVec2(
						previewOrigin.x + previewSize.x - 24.0f - 10.0f,
						previewOrigin.y + 10.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
					ImGui::PushStyleColor(ImGuiCol_Border, app::theme::BORDER);
					if (ImGui::ColorEdit3("##bg_color_models", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
						view.config["modelViewerBackgroundColor"] = std::format("#{:02x}{:02x}{:02x}",
							static_cast<int>(color[0] * 255.0f), static_cast<int>(color[1] * 255.0f), static_cast<int>(color[2] * 255.0f));
					ImGui::PopStyleColor();
					ImGui::PopStyleVar(2);
				}

				// --- 897: Animation dropdown positioned absolute top-left ---
				// CSS: .preview-dropdown-overlay { position: absolute; top: 10px; left: 10px; z-index: 1; }
				if (!view.modelViewerAnims.empty() && view.modelTexturePreviewURL.empty()) {
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

					// Position at top-left with offset.
					ImGui::SetCursorScreenPos(ImVec2(previewOrigin.x + 10.0f, previewOrigin.y + 10.0f));

					// CSS: select { background-color: var(--background); color: var(--font-primary);
					//       border: 1px solid var(--border); border-radius: 3px; padding: 5px 8px;
					//       font-size: 12px; min-width: 150px; }
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
					ImGui::PushStyleColor(ImGuiCol_FrameBg, app::theme::BG);
					ImGui::PushStyleColor(ImGuiCol_Border, app::theme::BORDER);
					ImGui::SetNextItemWidth(std::max(150.0f, ImGui::CalcTextSize(current_label.c_str()).x + 40.0f));

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

					ImGui::PopStyleColor(2);
					ImGui::PopStyleVar(3);

					// --- 895, 896: Animation controls ---
					if (current_id != "none" && !current_id.empty()) {
						// CSS: .anim-controls { display: flex; ... margin-top: 5px }
						// CSS: .anim-btn { width: 24px; height: 24px; border: 1px solid var(--border);
						//       background-color: var(--background); }
						ImGui::SetCursorScreenPos(ImVec2(previewOrigin.x + 10.0f, ImGui::GetCursorScreenPos().y + 5.0f));

						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
						ImGui::PushStyleColor(ImGuiCol_Button, app::theme::BG);
						ImGui::PushStyleColor(ImGuiCol_Border, app::theme::BORDER);

						bool anim_paused = view.modelViewerAnimPaused;

						// --- 895: Icon-style buttons ---
						// Step left: CSS uses arrow-left.svg, we use "<<" (or ◀)
						if (!anim_paused) ImGui::BeginDisabled();
						if (ImGui::Button(ICON_FA_ARROW_LEFT "##anim_prev", ImVec2(24.0f, 24.0f)))
							if (anim_methods) anim_methods->step_animation(-1);
						if (!anim_paused) ImGui::EndDisabled();

						ImGui::SameLine(0.0f, 2.0f);
						// Play/Pause: CSS uses play.svg / pause.svg
						if (ImGui::Button(anim_paused ? ICON_FA_PLAY "##anim_play" : ICON_FA_PAUSE "##anim_pause", ImVec2(24.0f, 24.0f)))
							if (anim_methods) anim_methods->toggle_animation_pause();

						ImGui::SameLine(0.0f, 2.0f);
						// Step right: CSS uses arrow-right.svg
						if (!anim_paused) ImGui::BeginDisabled();
						if (ImGui::Button(ICON_FA_ARROW_RIGHT "##anim_next", ImVec2(24.0f, 24.0f)))
							if (anim_methods) anim_methods->step_animation(1);
						if (!anim_paused) ImGui::EndDisabled();

						ImGui::PopStyleColor(2);
						ImGui::PopStyleVar();

						// --- 896: Animation scrubber ---
						// CSS: .anim-scrubber input[type="range"] { height: 6px; background: var(--background-dark);
						//       border: 1px solid var(--border); border-radius: 3px; }
						// CSS: .anim-scrubber input::-webkit-slider-thumb { width: 14px; height: 14px; }
						// CSS: .anim-frame-display { min-width: 32px; padding: 2px 4px; background: var(--background-dark);
						//       border: 1px solid var(--border); border-radius: 3px; font-size: 11px; text-align: center; }
						ImGui::SameLine(0.0f, 5.0f);
						int frame = view.modelViewerAnimFrame;
						int frame_max = view.modelViewerAnimFrameCount > 0 ? view.modelViewerAnimFrameCount - 1 : 0;

						ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 14.0f);
						ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 7.0f);
						ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleColor(ImGuiCol_FrameBg, app::theme::BG_DARK);
						ImGui::PushStyleColor(ImGuiCol_SliderGrab, app::theme::FONT_PRIMARY);
						float scrubberWidth = std::max(80.0f, previewSize.x - 160.0f);
						ImGui::SetNextItemWidth(scrubberWidth);
						if (ImGui::SliderInt("##ModelAnimFrame", &frame, 0, frame_max, "")) {
							if (anim_methods) anim_methods->seek_animation(frame);
						}
						if (ImGui::IsItemActivated()) {
							if (anim_methods) anim_methods->start_scrub();
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) {
							if (anim_methods) anim_methods->end_scrub();
						}
						ImGui::PopStyleColor(2);
						ImGui::PopStyleVar(4);

						// Frame display
						ImGui::SameLine(0.0f, 5.0f);
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
						ImGui::PushStyleColor(ImGuiCol_FrameBg, app::theme::BG_DARK);
						ImGui::PushStyleColor(ImGuiCol_Border, app::theme::BORDER);
						char frameBuf[16];
						std::snprintf(frameBuf, sizeof(frameBuf), "%d", view.modelViewerAnimFrame);
						ImGui::SetNextItemWidth(std::max(32.0f, ImGui::CalcTextSize(frameBuf).x + 8.0f));
						ImGui::InputText("##anim_frame_display", frameBuf, sizeof(frameBuf), ImGuiInputTextFlags_ReadOnly);
						ImGui::PopStyleColor(2);
						ImGui::PopStyleVar(3);
					}
				}
			}

			// --- 892, 893, 894: Texture ribbon absolute bottom overlay ---
			// CSS: #texture-ribbon { position: absolute; bottom: 10px; justify-content: center; z-index: 2; }
			if (view.config.value("modelViewerShowTextures", true) && !view.textureRibbonStack.empty()) {
				float ribbon_width = previewSize.x;
				texture_ribbon::onResize(static_cast<int>(ribbon_width));

				int maxPages = (view.textureRibbonSlotCount > 0)
					? static_cast<int>(std::ceil(static_cast<double>(view.textureRibbonStack.size()) / view.textureRibbonSlotCount))
					: 0;

				// Calculate total ribbon width for centering.
				int startIndex = view.textureRibbonPage * view.textureRibbonSlotCount;
				int endIndex = (std::min)(startIndex + view.textureRibbonSlotCount, static_cast<int>(view.textureRibbonStack.size()));
				int visibleSlots = endIndex - startIndex;
				bool hasPrev = view.textureRibbonPage > 0;
				bool hasNext = view.textureRibbonPage < maxPages - 1;

				// --- 893: Slot size is 64px with 5px margin each side ---
				float slotSize = 64.0f;
				float slotSpacing = 10.0f;  // 5px margin each side
				float prevNextWidth = 30.0f;
				float totalRibbonWidth = static_cast<float>(visibleSlots) * (slotSize + slotSpacing)
					+ (hasPrev ? prevNextWidth : 0.0f) + (hasNext ? prevNextWidth : 0.0f);
				float ribbonX = previewOrigin.x + (ribbon_width - totalRibbonWidth) * 0.5f;
				float ribbonY = previewOrigin.y + previewSize.y - slotSize - 20.0f; // bottom: 10px + some padding

				ImGui::SetCursorScreenPos(ImVec2(ribbonX, ribbonY));

				// --- 894: Prev button with ‹ glyph ---
				// CSS: #texture-ribbon-prev::before { content: "‹"; font-size: 4em; }
				if (hasPrev) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
					if (ImGui::Button("\xe2\x80\xb9##ribbon_prev", ImVec2(prevNextWidth, slotSize)))
						view.textureRibbonPage--;
					ImGui::PopStyleColor(2);
					ImGui::SameLine(0.0f, 0.0f);
				}

				// --- 893: Texture ribbon slots with border/shadow/background ---
				for (int si = startIndex; si < endIndex; si++) {
					auto& slot = view.textureRibbonStack[si];
					std::string slotDisplayName = slot.value("displayName", std::string(""));

					ImGui::PushID(si);
					// CSS: #texture-ribbon .slot { width: 64px; height: 64px; margin: 0 5px;
					//       border: 1px solid var(--border); box-shadow: black 0 0 3px 0;
					//       background-color: #232323; background-size: contain; }
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
					ImGui::PushStyleColor(ImGuiCol_Border, app::theme::BORDER);
					ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 35, 35, 255));

					GLuint slotTex = texture_ribbon::getSlotTexture(si);
					bool clicked = false;
					if (slotTex != 0) {
						clicked = ImGui::ImageButton("##ribbon_slot",
							static_cast<ImTextureID>(static_cast<uintptr_t>(slotTex)),
							ImVec2(slotSize, slotSize));
					} else {
						clicked = ImGui::Button(slotDisplayName.c_str(), ImVec2(slotSize, slotSize));
					}

					ImGui::PopStyleColor(2);
					ImGui::PopStyleVar();

					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", slotDisplayName.c_str());
					if (clicked) {
						view.contextMenus.nodeTextureRibbon = slot;
						ImGui::OpenPopup("ModelsTextureRibbonContextMenu");
					}
					ImGui::PopID();
					ImGui::SameLine(0.0f, slotSpacing);
				}

				// --- 894: Next button with › glyph ---
				if (hasNext) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
					if (ImGui::Button("\xe2\x80\xba##ribbon_next", ImVec2(prevNextWidth, slotSize)))
						view.textureRibbonPage++;
					ImGui::PopStyleColor(2);
				}

				// Texture ribbon context menu (unchanged logic).
				if (!view.contextMenus.nodeTextureRibbon.is_null()) {
					if (ImGui::BeginPopup("ModelsTextureRibbonContextMenu")) {
						const auto& node = view.contextMenus.nodeTextureRibbon;
						uint32_t fdid = node.value("fileDataID", 0u);
						std::string displayName = node.value("displayName", std::string(""));
						std::string fileName = node.value("fileName", std::string(""));

						if (ImGui::MenuItem(std::format("Preview {}", displayName).c_str())) {
							preview_texture(fdid, displayName);
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						if (ImGui::MenuItem(std::format("Export {}", displayName).c_str())) {
							export_ribbon_texture(fdid, displayName);
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						if (ImGui::MenuItem(std::format("Go to {}", displayName).c_str())) {
							tab_textures::goToTexture(fdid);
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						if (ImGui::MenuItem("Copy file data ID to clipboard")) {
							ImGui::SetClipboardText(std::to_string(fdid).c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						if (ImGui::MenuItem("Copy texture name to clipboard")) {
							ImGui::SetClipboardText(displayName.c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						if (ImGui::MenuItem("Copy file path to clipboard")) {
							ImGui::SetClipboardText(fileName.c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						if (ImGui::MenuItem("Copy export path to clipboard")) {
							ImGui::SetClipboardText(casc::ExportHelper::getExportPath(fileName).c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						ImGui::EndPopup();
					}
				}
			}
		}
		app::layout::EndPreviewContainer();

		// --- Bottom: Export controls (row 2, col 2) ---
		//     <MenuButton :options="menuButtonModels" :default="config.exportModelFormat"
		//         @change="config.exportModelFormat = $event" :disabled="isBusy" @click="export_model" class="upward" />
		if (app::layout::BeginPreviewControls("models-preview-controls", regions)) {
			// --- 905: Right-align the export button ---
			// CSS: .preview-controls { display: flex; justify-content: flex-end; margin-right: 20px; }
			{
				float buttonWidth = ImGui::GetContentRegionAvail().x;
				float menuBtnWidth = 200.0f; // approximate width of menu button
				float rightMargin = 20.0f;
				float offsetX = buttonWidth - menuBtnWidth - rightMargin;
				if (offsetX > 0.0f)
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
			}
			std::vector<menu_button::MenuOption> mb_options;
			for (const auto& opt : view.menuButtonModels)
				mb_options.push_back({ opt.label, opt.value });
			// --- 906: MenuButton with upward dropdown direction ---
			menu_button::render("##MenuButtonModels", mb_options,
				view.config.value("exportModelFormat", std::string("OBJ")),
				view.isBusy > 0, false, true, menu_button_models_state,
				[&](const std::string& val) { view.config["exportModelFormat"] = val; },
				[&]() { export_model_action(); });
		}
		app::layout::EndPreviewControls();

		// --- Right panel: Sidebar (col 3, spanning both rows) ---
		if (app::layout::BeginSidebar("models-sidebar", regions)) {
			// --- 898: Replace SeparatorText with plain text headers ---
			// CSS: .sidebar span.header { display: block; margin: 5px 0; }
			// Just a bold text label with vertical spacing, no horizontal rule.
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::TextUnformatted("Preview");
			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			// --- 899: Sidebar checkbox labels with ~18px checkbox and margin ---
			// CSS: .ui-checkbox { font-size: 18px; display: flex; margin: 0 15px; }
			// CSS: .sidebar label span { font-size: 16px; }
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 4.0f));
			ImGui::Indent(15.0f);
			auto show_checkbox_tooltip = [](const char* tooltip) {
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", tooltip);
			};

			//         <input type="checkbox" v-model="config.modelsAutoPreview"/> <span>Auto Preview</span>
			{
				bool auto_preview = view.config.value("modelsAutoPreview", false);
				if (ImGui::Checkbox("Auto Preview", &auto_preview))
					view.config["modelsAutoPreview"] = auto_preview;
				show_checkbox_tooltip("Automatically preview a model when selecting it");
			}

			//         <input type="checkbox" v-model="modelViewerAutoAdjust"/> <span>Auto Camera</span>
			ImGui::Checkbox("Auto Camera", &view.modelViewerAutoAdjust);
			show_checkbox_tooltip("Automatically adjust camera when selecting a new model");

			{
				bool show_grid = view.config.value("modelViewerShowGrid", true);
				if (ImGui::Checkbox("Show Grid", &show_grid))
					view.config["modelViewerShowGrid"] = show_grid;
				show_checkbox_tooltip("Show a grid in the 3D viewport");
			}

			{
				bool wireframe = view.config.value("modelViewerWireframe", false);
				if (ImGui::Checkbox("Show Wireframe", &wireframe))
					view.config["modelViewerWireframe"] = wireframe;
				show_checkbox_tooltip("Render the preview model as a wireframe");
			}

			{
				bool show_bones = view.config.value("modelViewerShowBones", false);
				if (ImGui::Checkbox("Show Bones", &show_bones))
					view.config["modelViewerShowBones"] = show_bones;
				show_checkbox_tooltip("Show the model's bone structure");
			}

			{
				bool show_textures = view.config.value("modelViewerShowTextures", true);
				if (ImGui::Checkbox("Show Textures", &show_textures))
					view.config["modelViewerShowTextures"] = show_textures;
				show_checkbox_tooltip("Show model textures in the preview pane");
			}

			{
				bool show_bg = view.config.value("modelViewerShowBackground", false);
				if (ImGui::Checkbox("Show Background", &show_bg))
					view.config["modelViewerShowBackground"] = show_bg;
				show_checkbox_tooltip("Show a background color in the 3D viewport");
			}

			ImGui::Unindent(15.0f);

			// --- 898: Export header ---
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::TextUnformatted("Export");
			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			ImGui::Indent(15.0f);

			{
				bool export_tex = view.config.value("modelsExportTextures", true);
				if (ImGui::Checkbox("Textures", &export_tex))
					view.config["modelsExportTextures"] = export_tex;
				show_checkbox_tooltip("Include textures when exporting models");
			}

			if (view.config.value("modelsExportTextures", true)) {
				bool export_alpha = view.config.value("modelsExportAlpha", true);
				if (ImGui::Checkbox("Texture Alpha", &export_alpha))
					view.config["modelsExportAlpha"] = export_alpha;
				show_checkbox_tooltip("Include alpha channel in exported model textures");
			}

			//         <input type="checkbox" v-model="config.modelsExportAnimations"/> <span>Export animations</span>
			std::string export_format = view.config.value("exportModelFormat", std::string("OBJ"));
			if (export_format == "GLTF" && view.modelViewerActiveType == "m2") {
				bool export_anims = view.config.value("modelsExportAnimations", false);
				if (ImGui::Checkbox("Export animations", &export_anims))
					view.config["modelsExportAnimations"] = export_anims;
				show_checkbox_tooltip("Include animations in export");
			}

			if (export_format == "RAW") {
				{
					bool v = view.config.value("modelsExportSkin", false);
					if (ImGui::Checkbox("M2 .skin Files", &v))
						view.config["modelsExportSkin"] = v;
					show_checkbox_tooltip("Export raw .skin files with M2 exports");
				}
				{
					bool v = view.config.value("modelsExportSkel", false);
					if (ImGui::Checkbox("M2 .skel Files", &v))
						view.config["modelsExportSkel"] = v;
					show_checkbox_tooltip("Export raw .skel files with M2 exports");
				}
				{
					bool v = view.config.value("modelsExportBone", false);
					if (ImGui::Checkbox("M2 .bone Files", &v))
						view.config["modelsExportBone"] = v;
					show_checkbox_tooltip("Export raw .bone files with M2 exports");
				}
				{
					bool v = view.config.value("modelsExportAnim", false);
					if (ImGui::Checkbox("M2 .anim files", &v))
						view.config["modelsExportAnim"] = v;
					show_checkbox_tooltip("Export raw .anim files with M2 exports");
				}
				{
					bool v = view.config.value("modelsExportWMOGroups", false);
					if (ImGui::Checkbox("WMO Groups##export", &v))
						view.config["modelsExportWMOGroups"] = v;
					show_checkbox_tooltip("Export WMO group files");
				}
			}

			if (export_format == "OBJ" && view.modelViewerActiveType == "wmo") {
				bool v = view.config.value("modelsExportSplitWMOGroups", false);
				if (ImGui::Checkbox("Split WMO Groups", &v))
					view.config["modelsExportSplitWMOGroups"] = v;
				show_checkbox_tooltip("Export each WMO group as a separate OBJ file");
			}

			ImGui::Unindent(15.0f);
			ImGui::PopStyleVar(); // ItemSpacing

			if (view.modelViewerActiveType == "m2") {
				// --- 898: Geosets header ---
				ImGui::Dummy(ImVec2(0.0f, 5.0f));
				ImGui::TextUnformatted("Geosets");
				ImGui::Dummy(ImVec2(0.0f, 5.0f));

				// --- 904: Constrain checkboxlist height to 156px ---
				// CSS: #tab-models #model-sidebar .ui-checkboxlist { height: 156px; }
				{
					float constrainedHeight = 156.0f;
					ImGui::BeginChild("##GeosetListWrapper", ImVec2(0.0f, constrainedHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
					checkboxlist::render("##ModelGeosets", view.modelViewerGeosets, checkboxlist_geosets_state);
					ImGui::EndChild();
				}

				// --- 900: "Enable All / Disable All" as text links ---
				// CSS: .list-toggles { font-size: 14px; text-align: center; margin-top: 5px; }
				// CSS: a { color: var(--font-primary); } a:hover { color: var(--font-highlight); text-decoration: underline; }
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					float totalWidth = ImGui::CalcTextSize("Enable All").x + ImGui::CalcTextSize(" / ").x + ImGui::CalcTextSize("Disable All").x;
					float availWidth = ImGui::GetContentRegionAvail().x;
					float startX = (availWidth - totalWidth) * 0.5f;
					if (startX > 0.0f)
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_PRIMARY);
					if (ImGui::SmallButton("Enable All##Geosets")) {
						for (auto& g : view.modelViewerGeosets)
							g["checked"] = true;
					}
					// Underline on hover
					if (ImGui::IsItemHovered()) {
						ImVec2 mn = ImGui::GetItemRectMin();
						ImVec2 mx = ImGui::GetItemRectMax();
						ImGui::GetWindowDrawList()->AddLine(ImVec2(mn.x, mx.y), mx, app::theme::FONT_HIGHLIGHT_U32);
					}
					ImGui::PopStyleColor(4);

					ImGui::SameLine(0.0f, 0.0f);
					ImGui::TextUnformatted(" / ");
					ImGui::SameLine(0.0f, 0.0f);

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_PRIMARY);
					if (ImGui::SmallButton("Disable All##Geosets")) {
						for (auto& g : view.modelViewerGeosets)
							g["checked"] = false;
					}
					if (ImGui::IsItemHovered()) {
						ImVec2 mn = ImGui::GetItemRectMin();
						ImVec2 mx = ImGui::GetItemRectMax();
						ImGui::GetWindowDrawList()->AddLine(ImVec2(mn.x, mx.y), mx, app::theme::FONT_HIGHLIGHT_U32);
					}
					ImGui::PopStyleColor(4);
				}

				if (view.config.value("modelsExportTextures", true)) {
					// --- 898: Skins header ---
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::TextUnformatted("Skins");
					ImGui::Dummy(ImVec2(0.0f, 5.0f));

					// --- 904: Constrain listbox height to 156px ---
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

						float constrainedHeight = 156.0f;
						ImGui::BeginChild("##SkinsListWrapper", ImVec2(0.0f, constrainedHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
						listboxb::render("##ModelSkins", skin_items, sel_indices, true, true, false,
							listboxb_skins_state,
							[&](const std::vector<int>& new_sel) {
								view.modelViewerSkinsSelection.clear();
								for (int idx : new_sel) {
									if (idx >= 0 && idx < static_cast<int>(view.modelViewerSkins.size()))
										view.modelViewerSkinsSelection.push_back(view.modelViewerSkins[idx]);
								}
							});
						ImGui::EndChild();
					}
				}
			}

			if (view.modelViewerActiveType == "wmo") {
				// --- 898: WMO Groups header ---
				ImGui::Dummy(ImVec2(0.0f, 5.0f));
				ImGui::TextUnformatted("WMO Groups");
				ImGui::Dummy(ImVec2(0.0f, 5.0f));

				// --- 901, 904: Use Checkboxlist component with 156px height ---
				{
					float constrainedHeight = 156.0f;
					ImGui::BeginChild("##WMOGroupsListWrapper", ImVec2(0.0f, constrainedHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
					checkboxlist::render("##ModelWMOGroups", view.modelViewerWMOGroups, checkboxlist_wmo_groups_state);
					ImGui::EndChild();
				}

				// --- 903, 900: "Enable All / Disable All" as text links ---
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					float totalWidth = ImGui::CalcTextSize("Enable All").x + ImGui::CalcTextSize(" / ").x + ImGui::CalcTextSize("Disable All").x;
					float availWidth = ImGui::GetContentRegionAvail().x;
					float startX = (availWidth - totalWidth) * 0.5f;
					if (startX > 0.0f)
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_PRIMARY);
					if (ImGui::SmallButton("Enable All##WMOGroups")) {
						for (auto& g : view.modelViewerWMOGroups)
							g["checked"] = true;
					}
					if (ImGui::IsItemHovered()) {
						ImVec2 mn = ImGui::GetItemRectMin();
						ImVec2 mx = ImGui::GetItemRectMax();
						ImGui::GetWindowDrawList()->AddLine(ImVec2(mn.x, mx.y), mx, app::theme::FONT_HIGHLIGHT_U32);
					}
					ImGui::PopStyleColor(4);

					ImGui::SameLine(0.0f, 0.0f);
					ImGui::TextUnformatted(" / ");
					ImGui::SameLine(0.0f, 0.0f);

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_PRIMARY);
					if (ImGui::SmallButton("Disable All##WMOGroups")) {
						for (auto& g : view.modelViewerWMOGroups)
							g["checked"] = false;
					}
					if (ImGui::IsItemHovered()) {
						ImVec2 mn = ImGui::GetItemRectMin();
						ImVec2 mx = ImGui::GetItemRectMax();
						ImGui::GetWindowDrawList()->AddLine(ImVec2(mn.x, mx.y), mx, app::theme::FONT_HIGHLIGHT_U32);
					}
					ImGui::PopStyleColor(4);
				}

				// --- 898: Doodad Sets header ---
				ImGui::Dummy(ImVec2(0.0f, 5.0f));
				ImGui::TextUnformatted("Doodad Sets");
				ImGui::Dummy(ImVec2(0.0f, 5.0f));

				// --- 902: Use Checkboxlist component for Doodad Sets ---
				{
					float constrainedHeight = 156.0f;
					ImGui::BeginChild("##WMODoodadSetsListWrapper", ImVec2(0.0f, constrainedHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
					checkboxlist::render("##ModelWMOSets", view.modelViewerWMOSets, checkboxlist_wmo_sets_state);
					ImGui::EndChild();
				}
			}
		}
		app::layout::EndSidebar();
	}
	app::layout::EndTab();
}

} // namespace tab_models
