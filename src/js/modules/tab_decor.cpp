/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_decor.h"
#include "../../app.h"
#include "../log.h"
#include "../core.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
#include "../install-type.h"
#include "../modules.h"
#include "../ui/listbox-context.h"
#include "../components/listbox.h"
#include "../db/caches/DBDecor.h"
#include "../db/caches/DBModelFileData.h"
#include "../db/caches/DBDecorCategories.h"
#include "../ui/texture-ribbon.h"
#include "../ui/texture-exporter.h"
#include "../ui/model-viewer-utils.h"
#include "../file-writer.h"
#include "../components/model-viewer-gl.h"
#include "../3D/renderers/M2RendererGL.h"
#include "../3D/renderers/M3RendererGL.h"
#include "../3D/renderers/WMORendererGL.h"
#include "../components/checkboxlist.h"
#include "../components/menu-button.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_decor {

// --- File-local constants ---

static constexpr int UNCATEGORIZED_ID = -1;

// --- File-local structures ---

struct DecorEntry {
	std::string display;
	uint32_t decor_id = 0;
};

struct CategoryMaskEntry {
	std::string label;
	bool checked = true;
	int categoryID = 0;
	std::string categoryName;
	int subcategoryID = 0;
};

struct CategoryGroup {
	int id = 0;
	std::string name;
	std::vector<CategoryMaskEntry*> subcategories;
};

// --- File-local state ---

static model_viewer_utils::RendererResult active_renderer_result;

static uint32_t active_file_data_id = 0;

static const db::caches::DBDecor::DecorItem* active_decor_item = nullptr;

static std::vector<DecorEntry> all_decor_entries;

// Category mask and groups (module-local, replaces core.view.decorCategoryMask / decorCategoryGroups).
static std::vector<CategoryMaskEntry> category_mask;
static std::vector<CategoryGroup> category_groups;

// Change-detection for watches (replaces Vue $watch).
static std::vector<bool> prev_category_mask_checked;
static std::string prev_anim_selection;
static std::vector<nlohmann::json> prev_selection_decor;

// View state proxy (created once in mounted()).
static model_viewer_utils::ViewStateProxy view_state;

// Animation methods helper.
static std::unique_ptr<model_viewer_utils::AnimationMethods> anim_methods;

static bool is_initialized = false;

static listbox::ListboxState listbox_decor_state;

// Component states for CheckboxList and MenuButton.
static checkboxlist::CheckboxListState checkboxlist_decor_geosets_state;
static checkboxlist::CheckboxListState checkboxlist_decor_wmo_groups_state;
static checkboxlist::CheckboxListState checkboxlist_decor_wmo_sets_state;
static menu_button::MenuButtonState menu_button_decor_state;

// Model viewer GL state/context (replaces Vue <ModelViewerGL :context="decorViewerContext"/>).
static model_viewer_gl::State viewer_state;
static model_viewer_gl::Context viewer_context;

// --- Internal helpers ---


static M2RendererGL* get_active_m2_renderer() {
	return active_renderer_result.m2.get();
}

static model_viewer_utils::ViewStateProxy* get_view_state_ptr() {
	return &view_state;
}

// --- preview_decor ---
static void preview_decor(const db::caches::DBDecor::DecorItem& decor_item) {
	auto _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", decor_item.name), {}, -1, false);
	logging::write(std::format("Previewing decor {} (FileDataID: {})", decor_item.name, decor_item.modelFileDataID));

	auto& state = view_state;
	texture_ribbon::reset();
	model_viewer_utils::clear_texture_preview(state);

	auto& view = *core::view;
	view.decorViewerAnims.clear();
	view.decorViewerAnimSelection = nullptr;

	try {
		if (active_renderer_result.type != model_viewer_utils::ModelType::Unknown) {
			active_renderer_result = model_viewer_utils::RendererResult{};
			active_file_data_id = 0;
			active_decor_item = nullptr;
		}

		const uint32_t file_data_id = decor_item.modelFileDataID;

		BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);

		gl::GLContext* gl_ctx = viewer_context.gl_context;
		if (!gl_ctx) {
			core::setToast("error", "GL context not available — model viewer not initialized.", {}, -1);
			return;
		}

		auto model_type = model_viewer_utils::detect_model_type(file);

		std::string file_name = casc::listfile::getByID(file_data_id);
		if (file_name.empty())
			file_name = casc::listfile::formatUnknownFile(file_data_id, model_viewer_utils::get_model_extension(model_type));

		if (model_type == model_viewer_utils::ModelType::M2)
			view.decorViewerActiveType = "m2";
		else if (model_type == model_viewer_utils::ModelType::WMO)
			view.decorViewerActiveType = "wmo";
		else
			view.decorViewerActiveType = "m3";

		active_renderer_result = model_viewer_utils::create_renderer(
			file, model_type, *gl_ctx,
			view.config.value("modelViewerShowTextures", true),
			file_data_id
		);

		if (model_type == model_viewer_utils::ModelType::M2 && active_renderer_result.m2)
			active_renderer_result.m2->setGeosetKey("decorViewerGeosets");
		else if (model_type == model_viewer_utils::ModelType::WMO && active_renderer_result.wmo) {
			active_renderer_result.wmo->setWmoGroupKey("decorViewerWMOGroups");
			active_renderer_result.wmo->setWmoSetKey("decorViewerWMOSets");
		}

		if (active_renderer_result.m2)
			active_renderer_result.m2->load();
		else if (active_renderer_result.m3)
			active_renderer_result.m3->load();
		else if (active_renderer_result.wmo)
			active_renderer_result.wmo->load();

		if (model_type == model_viewer_utils::ModelType::M2 && active_renderer_result.m2)
			view.decorViewerAnims = model_viewer_utils::extract_animations(*active_renderer_result.m2);

		view.decorViewerAnimSelection = "none";

		active_file_data_id = file_data_id;
		active_decor_item = &decor_item;

		bool has_content = false;
		if (active_renderer_result.m2)
			has_content = !active_renderer_result.m2->get_draw_calls().empty();
		else if (active_renderer_result.m3)
			has_content = !active_renderer_result.m3->get_draw_calls().empty();
		else if (active_renderer_result.wmo)
			has_content = !active_renderer_result.wmo->get_groups().empty();

		if (!has_content) {
			core::setToast("info", std::format("The model {} doesn't have any 3D data associated with it.", decor_item.name), {}, 4000);
		} else {
			core::hideToast();

			if (view.decorViewerAutoAdjust && viewer_context.fitCamera)
				viewer_context.fitCamera();
		}
	} catch (const casc::EncryptionError& e) {
		core::setToast("error", std::format("The model {} is encrypted with an unknown key ({}).", decor_item.name, e.key), {}, -1);
		logging::write(std::format("Failed to decrypt model {} ({})", decor_item.name, e.key));
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to preview model " + decor_item.name, {}, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}
}

// --- export_files ---
static void export_files(const std::vector<const db::caches::DBDecor::DecorItem*>& entries, [[maybe_unused]] int export_id = -1) {
	auto& view = *core::view;

	auto export_paths = core::openLastExportStream();

	const std::string format = view.config.value("exportDecorFormat", std::string("OBJ"));

	if (format == "PNG" || format == "CLIPBOARD") {
		if (active_file_data_id != 0) {
			std::string raw_name = active_decor_item ? active_decor_item->name : ("decor_" + std::to_string(active_file_data_id));
			const std::string export_name = casc::ExportHelper::sanitizeFilename(raw_name);

			gl::GLContext* gl_ctx = viewer_context.gl_context;
			if (gl_ctx && viewer_state.fbo != 0) {
				glBindFramebuffer(GL_FRAMEBUFFER, viewer_state.fbo);
				model_viewer_utils::export_preview(format, *gl_ctx, export_name, "decor");
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		} else {
			core::setToast("error", "The selected export option only works for model previews. Preview something first!", {}, -1);
		}

		export_paths.close();
	}

	casc::CASC* casc = core::view->casc;

	casc::ExportHelper helper(static_cast<int>(entries.size()), "decor");
	helper.start();

	for (const auto* decor_item : entries) {
		if (helper.isCancelled())
			break;

		if (!decor_item)
			continue;

		std::vector<nlohmann::json> file_manifest;
		const uint32_t file_data_id = decor_item->modelFileDataID;
		const std::string decor_name = casc::ExportHelper::sanitizeFilename(decor_item->name);

		try {
			BufferWrapper data = casc->getVirtualFileByID(file_data_id);
			auto model_type = model_viewer_utils::detect_model_type(data);
			auto file_ext = model_viewer_utils::get_model_extension(model_type);

			std::string file_name = casc::listfile::getByID(file_data_id);
			if (file_name.empty())
			    file_name = casc::listfile::formatUnknownFile(file_data_id, file_ext);

			std::string export_path = casc::ExportHelper::getExportPath("decor/" + decor_name + file_ext);

			const bool is_active = (file_data_id == active_file_data_id);

			model_viewer_utils::ExportModelOptions opts;
			opts.data = &data;
			opts.file_data_id = file_data_id;
			opts.file_name = file_name;
			opts.format = format;
			opts.export_path = export_path;
			opts.helper = &helper;
			opts.casc = casc;
			opts.file_manifest = &file_manifest;
			opts.geoset_mask = is_active ? &view.decorViewerGeosets : nullptr;
			opts.wmo_group_mask = is_active ? &view.decorViewerWMOGroups : nullptr;
			opts.wmo_set_mask = is_active ? &view.decorViewerWMOSets : nullptr;
			opts.export_paths = &export_paths;
			std::string mark_name = model_viewer_utils::export_model(opts);

			helper.mark(mark_name, true);
		} catch (const std::exception& e) {
			helper.mark(decor_name, false, e.what());
		}
	}

	helper.finish();
	export_paths.close();
}

// --- apply_filters ---
static void apply_filters() {
	auto& view = *core::view;

	std::unordered_set<int> checked_subs;
	bool uncategorized_checked = false;

	for (const auto& entry : category_mask) {
		if (!entry.checked)
			continue;

		if (entry.subcategoryID == UNCATEGORIZED_ID)
			uncategorized_checked = true;
		else
			checked_subs.insert(entry.subcategoryID);
	}

	std::vector<nlohmann::json> filtered;
	for (const auto& entry : all_decor_entries) {
		const auto* subs = db::caches::DBDecorCategories::get_subcategories_for_decor(entry.decor_id);
		if (!subs) {
			if (uncategorized_checked)
				filtered.push_back(entry.display);
		} else {
			for (const auto sub_id : *subs) {
				if (checked_subs.contains(static_cast<int>(sub_id))) {
					filtered.push_back(entry.display);
					break;
				}
			}
		}
	}

	view.listfileDecor = std::move(filtered);
}

// --- initialize ---
static void initialize() {
	auto& view = *core::view;

	core::showLoadingScreen(3);

	core::progressLoadingScreen("Loading model file data...");
	db::caches::DBModelFileData::initializeModelFileData();

	core::progressLoadingScreen("Loading house decor data...");
	db::caches::DBDecor::initializeDecorData();

	core::progressLoadingScreen("Loading decor categories...");
	db::caches::DBDecorCategories::initialize_categories();

	const auto& decor_items = db::caches::DBDecor::getAllDecorItems();
	all_decor_entries.clear();

	for (const auto& [id, item] : decor_items) {
		if (!core::view->casc->fileExists(item.modelFileDataID)) continue;

		all_decor_entries.push_back({
			std::format("{} [{}]", item.name, id),
			id
		});
	}

	std::sort(all_decor_entries.begin(), all_decor_entries.end(), [](const DecorEntry& a, const DecorEntry& b) {
		static const std::regex id_suffix(R"(\s+\[\d+\]$)");
		std::string name_a = std::regex_replace(a.display, id_suffix, "");
		std::string name_b = std::regex_replace(b.display, id_suffix, "");
		std::transform(name_a.begin(), name_a.end(), name_a.begin(), ::tolower);
		std::transform(name_b.begin(), name_b.end(), name_b.begin(), ::tolower);
		return name_a < name_b;
	});

	// --- build category groups and mask ---
	const auto& categories = db::caches::DBDecorCategories::get_all_categories();
	const auto& subcategories = db::caches::DBDecorCategories::get_all_subcategories();

	std::vector<const db::caches::DBDecorCategories::CategoryInfo*> sorted_categories;
	sorted_categories.reserve(categories.size());
	for (const auto& [id, cat] : categories)
		sorted_categories.push_back(&cat);
	std::sort(sorted_categories.begin(), sorted_categories.end(),
		[](const auto* a, const auto* b) { return a->orderIndex < b->orderIndex; });

	category_mask.clear();
	category_groups.clear();

	for (const auto* cat : sorted_categories) {
		std::vector<const db::caches::DBDecorCategories::SubcategoryInfo*> subs;
		for (const auto& [sub_id, sub] : subcategories) {
			if (sub.categoryID == cat->id)
				subs.push_back(&sub);
		}
		std::sort(subs.begin(), subs.end(),
			[](const auto* a, const auto* b) { return a->orderIndex < b->orderIndex; });

		if (subs.empty())
			continue;

		CategoryGroup group;
		group.id = static_cast<int>(cat->id);
		group.name = cat->name;

		for (const auto* sub : subs) {
			CategoryMaskEntry entry;
			entry.label = sub->name;
			entry.checked = true;
			entry.categoryID = static_cast<int>(cat->id);
			entry.categoryName = cat->name;
			entry.subcategoryID = static_cast<int>(sub->id);
			category_mask.push_back(std::move(entry));
		}

		// Build pointers after all entries for this group are added.
		// We fix up pointers after the loop since push_back may invalidate them.
		category_groups.push_back(std::move(group));
	}

	{
		CategoryMaskEntry uncategorized;
		uncategorized.label = "Uncategorized";
		uncategorized.checked = true;
		uncategorized.categoryID = UNCATEGORIZED_ID;
		uncategorized.categoryName = "Uncategorized";
		uncategorized.subcategoryID = UNCATEGORIZED_ID;
		category_mask.push_back(std::move(uncategorized));

		CategoryGroup uncategorized_group;
		uncategorized_group.id = UNCATEGORIZED_ID;
		uncategorized_group.name = "Uncategorized";
		category_groups.push_back(std::move(uncategorized_group));
	}

	// Fix up group subcategory pointers now that category_mask is stable.
	{
		size_t mask_idx = 0;
		for (auto& group : category_groups) {
			group.subcategories.clear();
			while (mask_idx < category_mask.size() && category_mask[mask_idx].categoryID == group.id) {
				group.subcategories.push_back(&category_mask[mask_idx]);
				++mask_idx;
			}
		}
	}

	// Category mask/groups are stored module-locally and synced to view as JSON in apply_filters.

	apply_filters();

	//     this.$core.view.decorViewerContext = Object.seal({ getActiveRenderer: () => active_renderer, gl_context: null, fitCamera: null });
	if (view.decorViewerContext.is_null()) {
		view.decorViewerContext = nlohmann::json::object();

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

// --- methods ---

static void handle_listbox_context(const std::vector<std::string>& selection) {
	listbox_context::handle_context_menu(selection);
}

static void copy_decor_names(const std::vector<nlohmann::json>& selection) {
	std::string result;
	static const std::regex name_pattern(R"(^(.+)\s+\[\d+\]$)");

	for (const auto& entry : selection) {
		if (!result.empty())
			result += '\n';

		if (entry.is_string()) {
			const std::string str = entry.get<std::string>();
			std::smatch match;
			if (std::regex_match(str, match, name_pattern))
				result += match[1].str();
			else
				result += str;
		} else if (entry.is_object()) {
			result += entry.value("name", std::string(""));
		}
	}

	ImGui::SetClipboardText(result.c_str());
}

static void copy_file_data_ids(const std::vector<nlohmann::json>& selection) {
	std::string result;
	static const std::regex id_pattern(R"(\[(\d+)\]$)");

	for (const auto& entry : selection) {
		std::string id_str;

		if (entry.is_string()) {
			const std::string str = entry.get<std::string>();
			std::smatch match;
			if (std::regex_search(str, match, id_pattern))
				id_str = match[1].str();
		} else if (entry.is_object()) {
			uint32_t fdid = entry.value("modelFileDataID", 0u);
			if (fdid != 0)
				id_str = std::to_string(fdid);
		}

		if (!id_str.empty()) {
			if (!result.empty())
				result += '\n';
			result += id_str;
		}
	}

	ImGui::SetClipboardText(result.c_str());
}

static void preview_texture(uint32_t file_data_id, const std::string& display_name) {
	model_viewer_utils::preview_texture_by_id(view_state, get_active_m2_renderer(), file_data_id, display_name, core::view->casc);
}

static void export_ribbon_texture(uint32_t file_data_id, [[maybe_unused]] const std::string& display_name) {
	texture_exporter::exportSingleTexture(file_data_id, core::view->casc);
}

static void toggle_uv_layer(const std::string& layer_name) {
	model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
}

static void export_decor() {
	auto& view = *core::view;

	const auto& user_selection = view.selectionDecor;

	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any items to export; you should do that first.");
		return;
	}

	static const std::regex id_extract(R"(\[(\d+)\]$)");
	std::vector<const db::caches::DBDecor::DecorItem*> decor_items;

	for (const auto& entry : user_selection) {
		if (entry.is_string()) {
			const std::string str = entry.get<std::string>();
			std::smatch match;
			if (std::regex_search(str, match, id_extract)) {
				uint32_t decor_id = static_cast<uint32_t>(std::stoul(match[1].str()));
				const auto* item = db::caches::DBDecor::getDecorItemByID(decor_id);
				if (item)
					decor_items.push_back(item);
			}
		}
	}

	export_files(decor_items);
}

// (Handled inline via ImGui::Checkbox — toggling .checked directly.)

static void toggle_category_group(CategoryGroup& group) {
	bool all_checked = true;
	for (const auto* sub : group.subcategories) {
		if (!sub->checked) {
			all_checked = false;
			break;
		}
	}

	const bool new_state = !all_checked;
	for (auto* sub : group.subcategories)
		sub->checked = new_state;
}

// Animation methods: toggle_animation_pause, step_animation, seek_animation, start_scrub, end_scrub
// are delegated to anim_methods (created via model_viewer_utils::create_animation_methods).

// --- Public API ---

void registerTab() {
	modules::register_nav_button("tab_decor", "Decor", "house.svg", install_type::CASC);
}

void mounted() {
	view_state = model_viewer_utils::create_view_state("decor");

	initialize();

	// Create animation methods helper.
	anim_methods = std::make_unique<model_viewer_utils::AnimationMethods>(
		get_active_m2_renderer,
		get_view_state_ptr
	);

	// Store initial category mask state for change-detection.
	prev_category_mask_checked.clear();
	for (const auto& e : category_mask)
		prev_category_mask_checked.push_back(e.checked);

	auto& view = *core::view;
	if (view.decorViewerAnimSelection.is_string())
		prev_anim_selection = view.decorViewerAnimSelection.get<std::string>();
	else
		prev_anim_selection.clear();

	prev_selection_decor = view.selectionDecor;

	core::events.on("toggle-uv-layer", EventEmitter::ArgCallback([](const std::any& arg) {
		const auto& layer_name = std::any_cast<const std::string&>(arg);
		model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
	}));

	is_initialized = true;
}

M2RendererGL* getActiveRenderer() {
	return active_renderer_result.m2.get();
}

void render() {
	auto& view = *core::view;

	if (!is_initialized)
		return;


	// Watch: decorCategoryMask (deep) → apply_filters
	{
		bool mask_changed = false;
		if (prev_category_mask_checked.size() == category_mask.size()) {
			for (size_t i = 0; i < category_mask.size(); ++i) {
				if (category_mask[i].checked != prev_category_mask_checked[i]) {
					mask_changed = true;
					break;
				}
			}
		} else {
			mask_changed = true;
		}

		if (mask_changed) {
			apply_filters();
			prev_category_mask_checked.clear();
			for (const auto& e : category_mask)
				prev_category_mask_checked.push_back(e.checked);
		}
	}

	// Watch: decorViewerAnimSelection → handle_animation_change
	{
		std::string current_anim;
		if (view.decorViewerAnimSelection.is_string())
			current_anim = view.decorViewerAnimSelection.get<std::string>();

		if (current_anim != prev_anim_selection) {
			prev_anim_selection = current_anim;

			if (!view.decorViewerAnims.empty()) {
				model_viewer_utils::handle_animation_change(
					get_active_m2_renderer(),
					view_state,
					current_anim
				);
			}
		}
	}

	// Watch: selectionDecor → auto-preview if decorAutoPreview
	{
		if (view.selectionDecor != prev_selection_decor) {
			prev_selection_decor = view.selectionDecor;

			if (view.config.value("decorAutoPreview", false)) {
				if (!view.selectionDecor.empty() && view.isBusy == 0) {
					const auto& first = view.selectionDecor[0];
					if (first.is_string()) {
						const std::string first_str = first.get<std::string>();
						static const std::regex id_rx(R"(\[(\d+)\]$)");
						std::smatch match;
						if (std::regex_search(first_str, match, id_rx)) {
							uint32_t decor_id = static_cast<uint32_t>(std::stoul(match[1].str()));
							const auto* decor_item = db::caches::DBDecor::getDecorItemByID(decor_id);
							if (decor_item && decor_item->modelFileDataID != active_file_data_id)
								preview_decor(*decor_item);
						}
					}
				}
			}
		}
	}


	if (app::layout::BeginTab("tab-decor")) {

	auto regions = app::layout::CalcListTabRegions(true);

	// --- Left panel: List container (row 1, col 1) ---
	//     <Listbox v-model:selection="selectionDecor" v-model:filter="userInputFilterDecor"
	//         :items="listfileDecor" ... @contextmenu="handle_listbox_context" />
	if (app::layout::BeginListContainer("decor-list-container", regions)) {
		std::vector<std::string> items_str;
		items_str.reserve(view.listfileDecor.size());
		for (const auto& item : view.listfileDecor)
			items_str.push_back(item.get<std::string>());

		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionDecor)
			selection_str.push_back(s.get<std::string>());

		listbox::CopyMode copy_mode = listbox::CopyMode::Default;
		{
			std::string cm = view.config.value("copyMode", std::string("Default"));
			if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
			else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
		}

		listbox::render(
			"listbox-decor",
			items_str,
			view.userInputFilterDecor,
			selection_str,
			false,    // single
			true,     // keyinput
			view.config.value("regexFilters", false),
			copy_mode,
			view.config.value("pasteSelection", false),
			view.config.value("removePathSpacesCopy", false),
			"decor item", // unittype
			nullptr,  // overrideItems
			false,    // disable
			"decor",  // persistscrollkey
			{},       // quickfilters
			false,    // nocopy
			listbox_decor_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionDecor.clear();
				for (const auto& s : new_sel)
					view.selectionDecor.push_back(s);
			},
			[](const listbox::ContextMenuEvent& ev) {
				handle_listbox_context(ev.selection);
			}
		);
	}
	app::layout::EndListContainer();

	// --- Filter bar (row 2, col 1) ---
	if (app::layout::BeginFilterBar("decor-filter", regions)) {
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterDecor.c_str(), sizeof(filter_buf) - 1);
		if (ImGui::InputText("##FilterDecor", filter_buf, sizeof(filter_buf)))
			view.userInputFilterDecor = filter_buf;
	}
	app::layout::EndFilterBar();

	// --- Middle panel: Preview container (row 1, col 2) ---
	if (app::layout::BeginPreviewContainer("decor-preview-container", regions)) {

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
				ImGui::OpenPopup("DecorTextureRibbonContextMenu");
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

		if (!view.contextMenus.nodeTextureRibbon.is_null()) {
			if (ImGui::BeginPopup("DecorTextureRibbonContextMenu")) {
				const auto& node = view.contextMenus.nodeTextureRibbon;
				uint32_t fdid = node.value("fileDataID", 0u);
				std::string displayName = node.value("displayName", std::string(""));

				if (ImGui::MenuItem(std::format("Preview {}", displayName).c_str())) {
					preview_texture(fdid, displayName);
					view.contextMenus.nodeTextureRibbon = nullptr;
				}

				if (ImGui::MenuItem(std::format("Export {}", displayName).c_str())) {
					export_ribbon_texture(fdid, displayName);
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

				ImGui::EndPopup();
			}
		}
	}

	if (!view.decorTexturePreviewURL.empty()) {
		if (ImGui::Button("Close Preview"))
			view.decorTexturePreviewURL.clear();

		// Texture preview image rendering via ImGui::Image.
		if (view.decorTexturePreviewTexID != 0) {
			const ImVec2 avail = ImGui::GetContentRegionAvail();
			const float tex_w = static_cast<float>(view.decorTexturePreviewWidth);
			const float tex_h = static_cast<float>(view.decorTexturePreviewHeight);
			const float scale = std::min(avail.x / tex_w, avail.y / tex_h);
			const ImVec2 img_size(tex_w * scale, tex_h * scale);

			const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
			ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.decorTexturePreviewTexID)), img_size);

			// UV overlay on top of texture preview.
			if (view.decorTexturePreviewUVTexID != 0 && !view.decorTexturePreviewUVOverlay.empty()) {
				ImGui::SetCursorScreenPos(cursor_pos);
				ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.decorTexturePreviewUVTexID)), img_size);
			}
		} else {
			ImGui::Text("Preview: %s (%dx%d)", view.decorTexturePreviewName.c_str(),
				view.decorTexturePreviewWidth, view.decorTexturePreviewHeight);
		}

		if (!view.decorViewerUVLayers.empty()) {
			for (const auto& layer : view.decorViewerUVLayers) {
				std::string layer_name = layer.value("name", std::string(""));
				bool is_active = layer.value("active", false);

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

	if (view.config.value("modelViewerShowBackground", false)) {
		std::string hex_str = view.config.value("modelViewerBackgroundColor", std::string("#343a40"));
		auto [cr, cg, cb] = model_viewer_gl::parse_hex_color(hex_str);
		float color[3] = {cr, cg, cb};
		if (ImGui::ColorEdit3("##bg_color_decor", color, ImGuiColorEditFlags_NoInputs))
			view.config["modelViewerBackgroundColor"] = std::format("#{:02x}{:02x}{:02x}",
				static_cast<int>(color[0] * 255.0f), static_cast<int>(color[1] * 255.0f), static_cast<int>(color[2] * 255.0f));
	}

	if (!view.decorViewerContext.is_null()) {
		model_viewer_gl::renderWidget("##decor_viewer", viewer_state, viewer_context);
	}

	if (!view.decorViewerAnims.empty() && view.decorTexturePreviewURL.empty()) {
		//     <option v-for="animation in decorViewerAnims" :key="animation.id" :value="animation.id">{{ animation.label }}</option>
		// </select>
		std::string current_label = "No Animation";
		std::string current_id;
		if (view.decorViewerAnimSelection.is_string())
			current_id = view.decorViewerAnimSelection.get<std::string>();

		for (const auto& anim : view.decorViewerAnims) {
			if (anim.value("id", std::string("")) == current_id) {
				current_label = anim.value("label", std::string(""));
				break;
			}
		}

		if (ImGui::BeginCombo("##decor-anim-select", current_label.c_str())) {
			for (const auto& anim : view.decorViewerAnims) {
				std::string anim_id = anim.value("id", std::string(""));
				std::string anim_label = anim.value("label", std::string(""));
				bool is_selected = (anim_id == current_id);

				if (ImGui::Selectable(anim_label.c_str(), is_selected))
					view.decorViewerAnimSelection = anim_id;

				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (current_id != "none" && !current_id.empty()) {
			ImGui::BeginDisabled(!view.decorViewerAnimPaused);
			if (ImGui::Button("<##decor-step-left"))
				anim_methods->step_animation(-1);
			ImGui::EndDisabled();

			ImGui::SameLine();

			const char* pause_label = view.decorViewerAnimPaused ? "Play##decor" : "Pause##decor";
			if (ImGui::Button(pause_label))
				anim_methods->toggle_animation_pause();

			ImGui::SameLine();

			ImGui::BeginDisabled(!view.decorViewerAnimPaused);
			if (ImGui::Button(">##decor-step-right"))
				anim_methods->step_animation(1);
			ImGui::EndDisabled();

			ImGui::SameLine();

			//     <input type="range" min="0" :max="decorViewerAnimFrameCount - 1" :value="decorViewerAnimFrame" @input="seek_animation($event.target.value)"/>
			//     <div class="anim-frame-display">{{ decorViewerAnimFrame }}</div>
			// </div>
			int frame = view.decorViewerAnimFrame;
			int max_frame = std::max(view.decorViewerAnimFrameCount - 1, 0);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
			if (ImGui::SliderInt("##decor-scrubber", &frame, 0, max_frame)) {
				anim_methods->seek_animation(frame);
			}

			ImGui::SameLine();
			ImGui::Text("%d", view.decorViewerAnimFrame);
		}
	}

	}
	app::layout::EndPreviewContainer();

	// --- Bottom: Export controls (row 2, col 2) ---
	//     <MenuButton :options="menuButtonDecor" :default="config.exportDecorFormat" ... @click="export_decor"/>
	// </div>
	if (app::layout::BeginPreviewControls("decor-preview-controls", regions)) {
		std::vector<menu_button::MenuOption> mb_options;
		for (const auto& opt : view.menuButtonDecor)
			mb_options.push_back({ opt.label, opt.value });
		menu_button::render("##MenuButtonDecor", mb_options,
			view.config.value("exportDecorFormat", std::string("OBJ")),
			view.isBusy > 0, false, menu_button_decor_state,
			[&](const std::string& val) { view.config["exportDecorFormat"] = val; },
			[&]() { export_decor(); });
	}
	app::layout::EndPreviewControls();

	// --- Right panel: Sidebar (col 3, spanning both rows) ---
	if (app::layout::BeginSidebar("decor-sidebar", regions)) {

	ImGui::SeparatorText("Categories");

	//     <div v-for="group in decorCategoryGroups" :key="group.id">
	ImGui::BeginChild("decor-category-list", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.4f), ImGuiChildFlags_Borders);
	for (auto& group : category_groups) {
		if (ImGui::TreeNodeEx(group.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			//     <div v-for="sub in group.subcategories" :key="sub.subcategoryID" ...>
			for (auto* sub : group.subcategories) {
				ImGui::Checkbox(sub->label.c_str(), &sub->checked);
			}

			ImGui::TreePop();
		}
	}
	ImGui::EndChild(); // decor-category-list

	//     <a @click="$core.view.setAllDecorCategories(true)">Enable All</a>
	//     / <a @click="$core.view.setAllDecorCategories(false)">Disable All</a>
	if (ImGui::SmallButton("Enable All##decor-cats")) {
		for (auto& entry : category_mask)
			entry.checked = true;
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("/");
	ImGui::SameLine();
	if (ImGui::SmallButton("Disable All##decor-cats")) {
		for (auto& entry : category_mask)
			entry.checked = false;
	}

	ImGui::SeparatorText("Preview");

	//     <input type="checkbox" v-model="config.decorAutoPreview"/>
	//     <span>Auto Preview</span>
	// </label>
	{
		bool auto_preview = view.config.value("decorAutoPreview", false);
		if (ImGui::Checkbox("Auto Preview##decor", &auto_preview))
			view.config["decorAutoPreview"] = auto_preview;
	}

	ImGui::Checkbox("Auto Camera##decor", &view.decorViewerAutoAdjust);

	{
		bool show_grid = view.config.value("modelViewerShowGrid", false);
		if (ImGui::Checkbox("Show Grid##decor", &show_grid))
			view.config["modelViewerShowGrid"] = show_grid;
	}

	{
		bool wireframe = view.config.value("modelViewerWireframe", false);
		if (ImGui::Checkbox("Show Wireframe##decor", &wireframe))
			view.config["modelViewerWireframe"] = wireframe;
	}

	{
		bool show_bones = view.config.value("modelViewerShowBones", false);
		if (ImGui::Checkbox("Show Bones##decor", &show_bones))
			view.config["modelViewerShowBones"] = show_bones;
	}

	{
		bool show_textures = view.config.value("modelViewerShowTextures", true);
		if (ImGui::Checkbox("Show Textures##decor", &show_textures))
			view.config["modelViewerShowTextures"] = show_textures;
	}

	{
		bool show_bg = view.config.value("modelViewerShowBackground", false);
		if (ImGui::Checkbox("Show Background##decor", &show_bg))
			view.config["modelViewerShowBackground"] = show_bg;
	}

	ImGui::SeparatorText("Export");

	{
		bool export_tex = view.config.value("modelsExportTextures", true);
		if (ImGui::Checkbox("Textures##decor-export", &export_tex))
			view.config["modelsExportTextures"] = export_tex;

		if (export_tex) {
			bool export_alpha = view.config.value("modelsExportAlpha", true);
			if (ImGui::Checkbox("Texture Alpha##decor", &export_alpha))
				view.config["modelsExportAlpha"] = export_alpha;
		}
	}

	//     <input type="checkbox" v-model="config.modelsExportAnimations"/><span>Export animations</span></label>
	{
		const std::string export_format = view.config.value("exportDecorFormat", std::string("OBJ"));
		if (export_format == "GLTF" && view.decorViewerActiveType == "m2") {
			bool export_anims = view.config.value("modelsExportAnimations", false);
			if (ImGui::Checkbox("Export animations##decor", &export_anims))
				view.config["modelsExportAnimations"] = export_anims;
		}
	}

	//     <span class="header">Geosets</span>
	//     <Checkboxlist :items="decorViewerGeosets"/>
	//     <div class="list-toggles">Enable All / Disable All</div>
	// </template>
	if (view.decorViewerActiveType == "m2") {
		ImGui::SeparatorText("Geosets");

		checkboxlist::render("##DecorGeosets", view.decorViewerGeosets, checkboxlist_decor_geosets_state);

		if (ImGui::SmallButton("Enable All##decor-geo")) {
			for (auto& g : view.decorViewerGeosets)
				g["checked"] = true;
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("/");
		ImGui::SameLine();
		if (ImGui::SmallButton("Disable All##decor-geo")) {
			for (auto& g : view.decorViewerGeosets)
				g["checked"] = false;
		}
	}

	//     <span class="header">WMO Groups</span>
	//     <Checkboxlist :items="decorViewerWMOGroups"/>
	//     Enable All / Disable All
	//     <span class="header">Doodad Sets</span>
	//     <Checkboxlist :items="decorViewerWMOSets"/>
	// </template>
	if (view.decorViewerActiveType == "wmo") {
		ImGui::SeparatorText("WMO Groups");

		for (auto& group : view.decorViewerWMOGroups) {
			std::string label = group.value("label", std::string("Group"));
			bool checked = group.value("checked", true);
			if (ImGui::Checkbox(label.c_str(), &checked))
				group["checked"] = checked;
		}

		if (ImGui::SmallButton("Enable All##decor-wmo")) {
			for (auto& g : view.decorViewerWMOGroups)
				g["checked"] = true;
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("/");
		ImGui::SameLine();
		if (ImGui::SmallButton("Disable All##decor-wmo")) {
			for (auto& g : view.decorViewerWMOGroups)
				g["checked"] = false;
		}

		ImGui::SeparatorText("Doodad Sets");

		for (auto& set : view.decorViewerWMOSets) {
			std::string label = set.value("label", std::string("Set"));
			bool checked = set.value("checked", true);
			if (ImGui::Checkbox(label.c_str(), &checked))
				set["checked"] = checked;
		}
	}

	}
	app::layout::EndSidebar();

	// --- Listbox context menu popup ---
	//     <span @click.self="copy_decor_names(...)">Copy name(s)</span>
	//     <span @click.self="copy_file_data_ids(...)">Copy file data ID(s)</span>
	// </ContextMenu>
	if (!view.contextMenus.nodeListbox.is_null()) {
		if (ImGui::BeginPopup("DecorListboxContextMenu")) {
			const auto& node = view.contextMenus.nodeListbox;
			int count = node.value("count", 0);
			const std::string plural = count > 1 ? "s" : "";

			if (ImGui::MenuItem(std::format("Copy name{}", plural).c_str())) {
				if (node.contains("selection") && node["selection"].is_array()) {
					copy_decor_names(node["selection"].get<std::vector<nlohmann::json>>());
				}
				view.contextMenus.nodeListbox = nullptr;
			}

			if (ImGui::MenuItem(std::format("Copy file data ID{}", plural).c_str())) {
				if (node.contains("selection") && node["selection"].is_array()) {
					copy_file_data_ids(node["selection"].get<std::vector<nlohmann::json>>());
				}
				view.contextMenus.nodeListbox = nullptr;
			}

			ImGui::EndPopup();
		}
	}

	} // if BeginTab
	app::layout::EndTab();
}

} // namespace tab_decor
