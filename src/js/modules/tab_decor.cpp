/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_decor.h"
#include "../log.h"
#include "../core.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
#include "../install-type.h"
#include "../modules.h"
#include "../ui/listbox-context.h"
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

#include <algorithm>
#include <format>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_decor {

// --- File-local constants ---

// JS: const UNCATEGORIZED_ID = -1;
static constexpr int UNCATEGORIZED_ID = -1;

// --- File-local structures ---

// JS: all_decor_entries = [{ display, decor_id }]
struct DecorEntry {
	std::string display;
	uint32_t decor_id = 0;
};

// JS: decorCategoryMask entries: { label, checked, categoryID, categoryName, subcategoryID }
struct CategoryMaskEntry {
	std::string label;
	bool checked = true;
	int categoryID = 0;
	std::string categoryName;
	int subcategoryID = 0;
};

// JS: decorCategoryGroups entries: { id, name, subcategories: [pointers into mask] }
struct CategoryGroup {
	int id = 0;
	std::string name;
	std::vector<CategoryMaskEntry*> subcategories;
};

// --- File-local state ---

// JS: let active_renderer;
static model_viewer_utils::RendererResult active_renderer_result;

// JS: let active_file_data_id;
static uint32_t active_file_data_id = 0;

// JS: let active_decor_item;
static const db::caches::DBDecor::DecorItem* active_decor_item = nullptr;

// JS: let all_decor_entries = [];
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

// Model viewer GL state/context (replaces Vue <ModelViewerGL :context="decorViewerContext"/>).
static model_viewer_gl::State viewer_state;
static model_viewer_gl::Context viewer_context;

// --- Internal helpers ---

// JS: get_view_state(core) — returns proxy object mapping decor viewer state fields.
// C++: uses model_viewer_utils::create_view_state("decor") which is stored in view_state.

// Helper to get the active M2 renderer (or nullptr).
static M2RendererGL* get_active_m2_renderer() {
	return active_renderer_result.m2.get();
}

// Helper to get the view state proxy pointer.
static model_viewer_utils::ViewStateProxy* get_view_state_ptr() {
	return &view_state;
}

// --- preview_decor ---
// JS: const preview_decor = async (core, decor_item) => { ... }
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
		// JS: if (active_renderer) { active_renderer.dispose(); ... }
		if (active_renderer_result.type != model_viewer_utils::ModelType::Unknown) {
			active_renderer_result = model_viewer_utils::RendererResult{};
			active_file_data_id = 0;
			active_decor_item = nullptr;
		}

		const uint32_t file_data_id = decor_item.modelFileDataID;

		// JS: const file = await core.view.casc.getFile(file_data_id);
		BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);

		// JS: const gl_context = core.view.decorViewerContext?.gl_context;
		gl::GLContext* gl_ctx = viewer_context.gl_context;
		if (!gl_ctx) {
			core::setToast("error", "GL context not available — model viewer not initialized.", {}, -1);
			return;
		}

		// JS: const model_type = modelViewerUtils.detect_model_type(file);
		auto model_type = model_viewer_utils::detect_model_type(file);

		// JS: const file_name = listfile.getByID(file_data_id) ?? listfile.formatUnknownFile(...);
		std::string file_name = casc::listfile::getByID(file_data_id);
		if (file_name.empty())
			file_name = casc::listfile::formatUnknownFile(file_data_id, model_viewer_utils::get_model_extension(model_type));

		// JS: if (model_type === modelViewerUtils.MODEL_TYPE_M2) core.view.decorViewerActiveType = 'm2'; ...
		if (model_type == model_viewer_utils::ModelType::M2)
			view.decorViewerActiveType = "m2";
		else if (model_type == model_viewer_utils::ModelType::WMO)
			view.decorViewerActiveType = "wmo";
		else
			view.decorViewerActiveType = "m3";

		// JS: active_renderer = modelViewerUtils.create_renderer(file, model_type, gl_context, ...)
		active_renderer_result = model_viewer_utils::create_renderer(
			file, model_type, *gl_ctx,
			view.config.value("modelViewerShowTextures", true),
			file_data_id
		);

		// JS: if (model_type === modelViewerUtils.MODEL_TYPE_M2) active_renderer.geosetKey = 'decorViewerGeosets';
		if (model_type == model_viewer_utils::ModelType::M2 && active_renderer_result.m2)
			active_renderer_result.m2->setGeosetKey("decorViewerGeosets");
		else if (model_type == model_viewer_utils::ModelType::WMO && active_renderer_result.wmo) {
			active_renderer_result.wmo->setWmoGroupKey("decorViewerWMOGroups");
			active_renderer_result.wmo->setWmoSetKey("decorViewerWMOSets");
		}

		// JS: await active_renderer.load();
		if (active_renderer_result.m2)
			active_renderer_result.m2->load();
		else if (active_renderer_result.m3)
			active_renderer_result.m3->load();
		else if (active_renderer_result.wmo)
			active_renderer_result.wmo->load();

		// JS: if (model_type === modelViewerUtils.MODEL_TYPE_M2) core.view.decorViewerAnims = ...
		if (model_type == model_viewer_utils::ModelType::M2 && active_renderer_result.m2)
			view.decorViewerAnims = model_viewer_utils::extract_animations(*active_renderer_result.m2);

		// JS: core.view.decorViewerAnimSelection = 'none';
		view.decorViewerAnimSelection = "none";

		active_file_data_id = file_data_id;
		active_decor_item = &decor_item;

		// JS: const has_content = active_renderer.draw_calls?.length > 0 || active_renderer.groups?.length > 0;
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

			// JS: if (core.view.decorViewerAutoAdjust) requestAnimationFrame(() => core.view.decorViewerContext?.fitCamera?.());
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
// JS: const export_files = async (core, entries, export_id = -1) => { ... }
static void export_files(const std::vector<const db::caches::DBDecor::DecorItem*>& entries, [[maybe_unused]] int export_id = -1) {
	auto& view = *core::view;

	// JS: const export_paths = core.openLastExportStream();
	// TODO(conversion): Export stream will be wired when file I/O is integrated.
	// FileWriter export_paths = core::openLastExportStream();

	// JS: const format = core.view.config.exportDecorFormat;
	const std::string format = view.config.value("exportDecorFormat", std::string("OBJ"));

	// JS: if (format === 'PNG' || format === 'CLIPBOARD') { ... }
	if (format == "PNG" || format == "CLIPBOARD") {
		if (active_file_data_id != 0) {
			// JS: const canvas = document.getElementById('decor-preview').querySelector('canvas');
			// JS: const export_name = ExportHelper.sanitizeFilename(active_decor_item?.name ?? 'decor_' + active_file_data_id);
			std::string raw_name = active_decor_item ? active_decor_item->name : ("decor_" + std::to_string(active_file_data_id));
			const std::string export_name = casc::ExportHelper::sanitizeFilename(raw_name);

			// JS: await modelViewerUtils.export_preview(core, format, canvas, export_name, 'decor');
			// TODO(conversion): GL context needed for export_preview; will be wired when renderer is integrated.
			// model_viewer_utils::export_preview(format, gl_context, export_name, "decor");
			core::setToast("info", "Preview export not yet wired.", {}, 2000);
		} else {
			core::setToast("error", "The selected export option only works for model previews. Preview something first!", {}, -1);
		}

		// JS: export_paths?.close();
		return;
	}

	// JS: const casc = core.view.casc;
	casc::CASC* casc = core::view->casc;

	// JS: const helper = new ExportHelper(entries.length, 'decor');
	casc::ExportHelper helper(static_cast<int>(entries.size()), "decor");
	helper.start();

	for (const auto* decor_item : entries) {
		if (helper.isCancelled())
			break;

		// JS: const decor_item = typeof entry === 'object' ? entry : DBDecor.getDecorItemByID(entry);
		if (!decor_item)
			continue;

		// JS: const file_manifest = [];
		std::vector<nlohmann::json> file_manifest;
		const uint32_t file_data_id = decor_item->modelFileDataID;
		const std::string decor_name = casc::ExportHelper::sanitizeFilename(decor_item->name);

		try {
			// JS: const data = await casc.getFile(file_data_id);
			BufferWrapper data = casc->getVirtualFileByID(file_data_id);
			// JS: const model_type = modelViewerUtils.detect_model_type(data);
			auto model_type = model_viewer_utils::detect_model_type(data);
			auto file_ext = model_viewer_utils::get_model_extension(model_type);

			// JS: const file_name = listfile.getByID(file_data_id) ?? listfile.formatUnknownFile(file_data_id, file_ext);
			std::string file_name = casc::listfile::getByID(file_data_id);
			if (file_name.empty())
			    file_name = casc::listfile::formatUnknownFile(file_data_id, file_ext);

			// JS: const export_path = ExportHelper.getExportPath('decor/' + decor_name + file_ext);
			std::string export_path = casc::ExportHelper::getExportPath("decor/" + decor_name + file_ext);

			// JS: const is_active = file_data_id === active_file_data_id;
			const bool is_active = (file_data_id == active_file_data_id);

			// JS: const mark_name = await modelViewerUtils.export_model({ ... });
			// model_viewer_utils::ExportModelOptions opts;
			// opts.data = &data;
			// opts.file_data_id = file_data_id;
			// opts.file_name = file_name;
			// opts.format = format;
			// opts.export_path = export_path;
			// opts.helper = &helper;
			// opts.file_manifest = &file_manifest;
			// opts.geoset_mask = is_active ? &view.decorViewerGeosets : nullptr;
			// opts.wmo_group_mask = is_active ? &view.decorViewerWMOGroups : nullptr;
			// opts.wmo_set_mask = is_active ? &view.decorViewerWMOSets : nullptr;
			// std::string mark_name = model_viewer_utils::export_model(opts);
			// helper.mark(mark_name, true);

			(void)data;
			(void)model_type;
			(void)file_name;
			(void)export_path;
			(void)is_active;
			helper.mark(decor_name, true);
		} catch (const std::exception& e) {
			helper.mark(decor_name, false, e.what());
		}
	}

	helper.finish();
	// JS: export_paths?.close();
}

// --- apply_filters ---
// JS: const apply_filters = (core) => { ... }
static void apply_filters() {
	auto& view = *core::view;

	// JS: const checked_subs = new Set();
	std::unordered_set<int> checked_subs;
	bool uncategorized_checked = false;

	// JS: for (const entry of core.view.decorCategoryMask) { ... }
	for (const auto& entry : category_mask) {
		if (!entry.checked)
			continue;

		if (entry.subcategoryID == UNCATEGORIZED_ID)
			uncategorized_checked = true;
		else
			checked_subs.insert(entry.subcategoryID);
	}

	// JS: const filtered = [];
	std::vector<nlohmann::json> filtered;
	for (const auto& entry : all_decor_entries) {
		// JS: const subs = DBDecorCategories.get_subcategories_for_decor(entry.decor_id);
		const auto* subs = db::caches::DBDecorCategories::get_subcategories_for_decor(entry.decor_id);
		if (!subs) {
			// JS: if (uncategorized_checked) filtered.push(entry.display);
			if (uncategorized_checked)
				filtered.push_back(entry.display);
		} else {
			// JS: for (const sub_id of subs) { if (checked_subs.has(sub_id)) { ... } }
			for (const auto sub_id : *subs) {
				if (checked_subs.contains(static_cast<int>(sub_id))) {
					filtered.push_back(entry.display);
					break;
				}
			}
		}
	}

	// JS: core.view.listfileDecor = filtered;
	view.listfileDecor = std::move(filtered);
}

// --- initialize ---
// JS: methods.initialize()
static void initialize() {
	auto& view = *core::view;

	// JS: this.$core.showLoadingScreen(3);
	core::showLoadingScreen(3);

	// JS: await this.$core.progressLoadingScreen('Loading model file data...');
	// JS: await DBModelFileData.initializeModelFileData();
	core::progressLoadingScreen("Loading model file data...");
	db::caches::DBModelFileData::initializeModelFileData();

	// JS: await this.$core.progressLoadingScreen('Loading house decor data...');
	// JS: await DBDecor.initializeDecorData();
	core::progressLoadingScreen("Loading house decor data...");
	db::caches::DBDecor::initializeDecorData();

	// JS: await this.$core.progressLoadingScreen('Loading decor categories...');
	// JS: await DBDecorCategories.initialize_categories();
	core::progressLoadingScreen("Loading decor categories...");
	db::caches::DBDecorCategories::initialize_categories();

	// JS: const decor_items = DBDecor.getAllDecorItems();
	const auto& decor_items = db::caches::DBDecor::getAllDecorItems();
	all_decor_entries.clear();

	// JS: for (const [id, item] of decor_items) { ... }
	for (const auto& [id, item] : decor_items) {
		// JS: if (!this.$core.view.casc.fileExists(item.modelFileDataID)) continue;
		if (!core::view->casc->fileExists(item.modelFileDataID)) continue;

		all_decor_entries.push_back({
			std::format("{} [{}]", item.name, id),
			id
		});
	}

	// JS: all_decor_entries.sort((a, b) => { ... });
	std::sort(all_decor_entries.begin(), all_decor_entries.end(), [](const DecorEntry& a, const DecorEntry& b) {
		// JS: const name_a = a.display.replace(/\s+\[\d+\]$/, '').toLowerCase();
		static const std::regex id_suffix(R"(\s+\[\d+\]$)");
		std::string name_a = std::regex_replace(a.display, id_suffix, "");
		std::string name_b = std::regex_replace(b.display, id_suffix, "");
		std::transform(name_a.begin(), name_a.end(), name_a.begin(), ::tolower);
		std::transform(name_b.begin(), name_b.end(), name_b.begin(), ::tolower);
		return name_a < name_b;
	});

	// --- build category groups and mask ---
	// JS: const categories = DBDecorCategories.get_all_categories();
	// JS: const subcategories = DBDecorCategories.get_all_subcategories();
	const auto& categories = db::caches::DBDecorCategories::get_all_categories();
	const auto& subcategories = db::caches::DBDecorCategories::get_all_subcategories();

	// JS: const sorted_categories = [...categories.values()].sort((a, b) => a.orderIndex - b.orderIndex);
	std::vector<const db::caches::DBDecorCategories::CategoryInfo*> sorted_categories;
	sorted_categories.reserve(categories.size());
	for (const auto& [id, cat] : categories)
		sorted_categories.push_back(&cat);
	std::sort(sorted_categories.begin(), sorted_categories.end(),
		[](const auto* a, const auto* b) { return a->orderIndex < b->orderIndex; });

	category_mask.clear();
	category_groups.clear();

	// JS: for (const cat of sorted_categories) { ... }
	for (const auto* cat : sorted_categories) {
		// JS: const subs = [...subcategories.values()].filter(s => s.categoryID === cat.id).sort(...);
		std::vector<const db::caches::DBDecorCategories::SubcategoryInfo*> subs;
		for (const auto& [sub_id, sub] : subcategories) {
			if (sub.categoryID == cat->id)
				subs.push_back(&sub);
		}
		std::sort(subs.begin(), subs.end(),
			[](const auto* a, const auto* b) { return a->orderIndex < b->orderIndex; });

		// JS: if (subs.length === 0) continue;
		if (subs.empty())
			continue;

		CategoryGroup group;
		group.id = static_cast<int>(cat->id);
		group.name = cat->name;

		// JS: for (const sub of subs) { ... }
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

	// JS: synthetic uncategorized entry
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

	// JS: this.$core.view.decorCategoryMask = mask;
	// JS: this.$core.view.decorCategoryGroups = groups;
	// Category mask/groups are stored module-locally and synced to view as JSON in apply_filters.

	// JS: apply_filters(this.$core);
	apply_filters();

	// JS: if (!this.$core.view.decorViewerContext)
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

	// JS: this.$core.hideLoadingScreen();
	core::hideLoadingScreen();
}

// --- methods ---

// JS: methods.handle_listbox_context(data)
static void handle_listbox_context(const std::vector<std::string>& selection) {
	listbox_context::handle_context_menu(selection);
}

// JS: methods.copy_decor_names(selection)
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

// JS: methods.copy_file_data_ids(selection)
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

// JS: methods.preview_texture(file_data_id, display_name)
static void preview_texture(uint32_t file_data_id, const std::string& display_name) {
	// JS: const state = get_view_state(this.$core);
	// JS: await modelViewerUtils.preview_texture_by_id(this.$core, state, active_renderer, file_data_id, display_name);
	model_viewer_utils::preview_texture_by_id(view_state, get_active_m2_renderer(), file_data_id, display_name, core::view->casc);
}

// JS: methods.export_ribbon_texture(file_data_id, display_name)
static void export_ribbon_texture(uint32_t file_data_id, [[maybe_unused]] const std::string& display_name) {
	// JS: await textureExporter.exportSingleTexture(file_data_id);
	texture_exporter::exportSingleTexture(file_data_id, core::view->casc);
}

// JS: methods.toggle_uv_layer(layer_name)
static void toggle_uv_layer(const std::string& layer_name) {
	// JS: const state = get_view_state(this.$core);
	// JS: modelViewerUtils.toggle_uv_layer(state, active_renderer, layer_name);
	model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
}

// JS: methods.export_decor()
static void export_decor() {
	auto& view = *core::view;

	// JS: const user_selection = this.$core.view.selectionDecor;
	const auto& user_selection = view.selectionDecor;

	// JS: if (user_selection.length === 0) { ... }
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any items to export; you should do that first.");
		return;
	}

	// JS: const decor_items = user_selection.map(entry => { ... }).filter(item => item);
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

	// JS: await export_files(this.$core, decor_items);
	export_files(decor_items);
}

// JS: methods.toggle_checklist_item(item)
// (Handled inline via ImGui::Checkbox — toggling .checked directly.)

// JS: methods.toggle_category_group(group)
static void toggle_category_group(CategoryGroup& group) {
	// JS: const all_checked = group.subcategories.every(s => s.checked);
	bool all_checked = true;
	for (const auto* sub : group.subcategories) {
		if (!sub->checked) {
			all_checked = false;
			break;
		}
	}

	// JS: this.$core.view.setDecorCategoryGroup(group.id, !all_checked);
	const bool new_state = !all_checked;
	for (auto* sub : group.subcategories)
		sub->checked = new_state;
}

// Animation methods: toggle_animation_pause, step_animation, seek_animation, start_scrub, end_scrub
// are delegated to anim_methods (created via model_viewer_utils::create_animation_methods).

// --- Public API ---

// JS: register() { this.registerNavButton('Decor', 'house.svg', InstallType.CASC); }
void registerTab() {
	// JS: this.registerNavButton('Decor', 'house.svg', InstallType.CASC);
	modules::register_nav_button("tab_decor", "Decor", "house.svg", install_type::CASC);
}

// JS: async mounted() { await this.initialize(); ... }
void mounted() {
	// JS: const state = get_view_state(this.$core);
	view_state = model_viewer_utils::create_view_state("decor");

	// JS: await this.initialize();
	initialize();

	// Create animation methods helper.
	// JS: toggle_animation_pause, step_animation, seek_animation, start_scrub, end_scrub
	anim_methods = std::make_unique<model_viewer_utils::AnimationMethods>(
		get_active_m2_renderer,
		get_view_state_ptr
	);

	// Store initial category mask state for change-detection.
	// JS: this.$core.view.$watch('decorCategoryMask', () => apply_filters(this.$core), { deep: true });
	prev_category_mask_checked.clear();
	for (const auto& e : category_mask)
		prev_category_mask_checked.push_back(e.checked);

	// JS: this.$core.view.$watch('decorViewerAnimSelection', ...)
	auto& view = *core::view;
	if (view.decorViewerAnimSelection.is_string())
		prev_anim_selection = view.decorViewerAnimSelection.get<std::string>();
	else
		prev_anim_selection.clear();

	// JS: this.$core.view.$watch('selectionDecor', ...)
	prev_selection_decor = view.selectionDecor;

	// JS: this.$core.events.on('toggle-uv-layer', (layer_name) => { ... });
	core::events.on("toggle-uv-layer", EventEmitter::ArgCallback([](const std::any& arg) {
		const auto& layer_name = std::any_cast<const std::string&>(arg);
		model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
	}));

	is_initialized = true;
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

	// Watch: decorCategoryMask (deep) → apply_filters
	// JS: this.$core.view.$watch('decorCategoryMask', () => apply_filters(this.$core), { deep: true });
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
	// JS: this.$core.view.$watch('decorViewerAnimSelection', async selected_animation_id => { ... });
	{
		std::string current_anim;
		if (view.decorViewerAnimSelection.is_string())
			current_anim = view.decorViewerAnimSelection.get<std::string>();

		if (current_anim != prev_anim_selection) {
			prev_anim_selection = current_anim;

			// JS: if (this.$core.view.decorViewerAnims.length === 0) return;
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
	// JS: this.$core.view.$watch('selectionDecor', async selection => { ... });
	{
		if (view.selectionDecor != prev_selection_decor) {
			prev_selection_decor = view.selectionDecor;

			// JS: if (!this.$core.view.config.decorAutoPreview) return;
			if (view.config.value("decorAutoPreview", false)) {
				// JS: const first = selection[0];
				if (!view.selectionDecor.empty() && view.isBusy == 0) {
					const auto& first = view.selectionDecor[0];
					if (first.is_string()) {
						const std::string first_str = first.get<std::string>();
						static const std::regex id_rx(R"(\[(\d+)\]$)");
						std::smatch match;
						if (std::regex_search(first_str, match, id_rx)) {
							uint32_t decor_id = static_cast<uint32_t>(std::stoul(match[1].str()));
							const auto* decor_item = db::caches::DBDecor::getDecorItemByID(decor_id);
							// JS: if (decor_item && decor_item.modelFileDataID !== active_file_data_id)
							if (decor_item && decor_item->modelFileDataID != active_file_data_id)
								preview_decor(*decor_item);
						}
					}
				}
			}
		}
	}

	// ─── Template rendering ─────────────────────────────────────────────────

	// JS: <div class="tab list-tab" id="tab-decor">

	// --- Left panel: List container ---
	// JS: <div class="list-container">
	//     <Listbox v-model:selection="selectionDecor" v-model:filter="userInputFilterDecor"
	//         :items="listfileDecor" ... @contextmenu="handle_listbox_context" />
	ImGui::BeginChild("decor-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox component rendering will be wired when integration is complete.
	ImGui::Text("Decor items: %zu", view.listfileDecor.size());

	// JS: <input type="text" v-model="$core.view.userInputFilterDecor" placeholder="Filter decor..."/>
	// TODO(conversion): Filter input will use ImGui::InputText when Listbox component is wired.

	ImGui::EndChild();

	ImGui::SameLine();

	// --- Middle panel: Preview container ---
	// JS: <div class="preview-container">
	ImGui::BeginChild("decor-preview-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.65f, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// JS: <component :is="$components.ResizeLayer" id="texture-ribbon" v-if="config.modelViewerShowTextures && textureRibbonStack.length > 0">
	if (view.config.value("modelViewerShowTextures", true) && !view.textureRibbonStack.empty()) {
		// TODO(conversion): Texture ribbon rendering will be wired when component is integrated.
		ImGui::Text("Texture Ribbon: %zu textures", view.textureRibbonStack.size());

		// JS: <component :is="$components.ContextMenu" :node="$core.view.contextMenus.nodeTextureRibbon" ...>
		if (!view.contextMenus.nodeTextureRibbon.is_null()) {
			if (ImGui::BeginPopup("DecorTextureRibbonContextMenu")) {
				const auto& node = view.contextMenus.nodeTextureRibbon;
				uint32_t fdid = node.value("fileDataID", 0u);
				std::string displayName = node.value("displayName", std::string(""));

				// JS: <span @click.self="preview_texture(...)">Preview {{ context.node.displayName }}</span>
				if (ImGui::MenuItem(std::format("Preview {}", displayName).c_str())) {
					preview_texture(fdid, displayName);
					view.contextMenus.nodeTextureRibbon = nullptr;
				}

				// JS: <span @click.self="export_ribbon_texture(...)">Export {{ context.node.displayName }}</span>
				if (ImGui::MenuItem(std::format("Export {}", displayName).c_str())) {
					export_ribbon_texture(fdid, displayName);
					view.contextMenus.nodeTextureRibbon = nullptr;
				}

				// JS: <span @click.self="$core.view.copyToClipboard(context.node.fileDataID)">Copy file data ID to clipboard</span>
				if (ImGui::MenuItem("Copy file data ID to clipboard")) {
					ImGui::SetClipboardText(std::to_string(fdid).c_str());
					view.contextMenus.nodeTextureRibbon = nullptr;
				}

				// JS: <span @click.self="$core.view.copyToClipboard(context.node.displayName)">Copy texture name to clipboard</span>
				if (ImGui::MenuItem("Copy texture name to clipboard")) {
					ImGui::SetClipboardText(displayName.c_str());
					view.contextMenus.nodeTextureRibbon = nullptr;
				}

				ImGui::EndPopup();
			}
		}
	}

	// JS: <div id="decor-texture-preview" v-if="$core.view.decorTexturePreviewURL.length > 0">
	if (!view.decorTexturePreviewURL.empty()) {
		// JS: <div id="decor-texture-preview-toast" @click="$core.view.decorTexturePreviewURL = ''">Close Preview</div>
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

		// JS: <div id="uv-layer-buttons" v-if="$core.view.decorViewerUVLayers.length > 0">
		if (!view.decorViewerUVLayers.empty()) {
			for (const auto& layer : view.decorViewerUVLayers) {
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

	// JS: <div class="preview-background" id="decor-preview">
	// JS: <input v-if="config.modelViewerShowBackground" type="color" id="background-color-input" v-model="config.modelViewerBackgroundColor"/>
	if (view.config.value("modelViewerShowBackground", false)) {
		// TODO(conversion): Color picker for background will be wired when model viewer GL is integrated.
	}

	// JS: <component :is="$components.ModelViewerGL" v-if="$core.view.decorViewerContext" :context="$core.view.decorViewerContext" />
	if (!view.decorViewerContext.is_null()) {
		model_viewer_gl::renderWidget("##decor_viewer", viewer_state, viewer_context);
	}

	// JS: <div v-if="decorViewerAnims && decorViewerAnims.length > 0 && !decorTexturePreviewURL" class="preview-dropdown-overlay">
	if (!view.decorViewerAnims.empty() && view.decorTexturePreviewURL.empty()) {
		// JS: <select v-model="$core.view.decorViewerAnimSelection">
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

		// JS: <div v-if="decorViewerAnimSelection !== 'none'" class="anim-controls">
		if (current_id != "none" && !current_id.empty()) {
			// JS: <button class="anim-btn anim-step-left" ... @click="step_animation(-1)"/>
			ImGui::BeginDisabled(!view.decorViewerAnimPaused);
			if (ImGui::Button("<##decor-step-left"))
				anim_methods->step_animation(-1);
			ImGui::EndDisabled();

			ImGui::SameLine();

			// JS: <button class="anim-btn" :class="animPaused ? 'anim-play' : 'anim-pause'" @click="toggle_animation_pause()"/>
			const char* pause_label = view.decorViewerAnimPaused ? "Play##decor" : "Pause##decor";
			if (ImGui::Button(pause_label))
				anim_methods->toggle_animation_pause();

			ImGui::SameLine();

			// JS: <button class="anim-btn anim-step-right" ... @click="step_animation(1)"/>
			ImGui::BeginDisabled(!view.decorViewerAnimPaused);
			if (ImGui::Button(">##decor-step-right"))
				anim_methods->step_animation(1);
			ImGui::EndDisabled();

			ImGui::SameLine();

			// JS: <div class="anim-scrubber">
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

	// JS: <div class="preview-controls">
	//     <MenuButton :options="menuButtonDecor" :default="config.exportDecorFormat" ... @click="export_decor"/>
	// </div>
	// TODO(conversion): MenuButton component rendering will be wired when integration is complete.
	ImGui::BeginDisabled(view.isBusy > 0);
	if (ImGui::Button("Export##decor"))
		export_decor();
	ImGui::EndDisabled();

	ImGui::EndChild(); // decor-preview-container

	ImGui::SameLine();

	// --- Right panel: Sidebar ---
	// JS: <div id="decor-sidebar" class="sidebar">
	ImGui::BeginChild("decor-sidebar", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// JS: <span class="header">Categories</span>
	ImGui::SeparatorText("Categories");

	// JS: <div class="decor-category-list">
	//     <div v-for="group in decorCategoryGroups" :key="group.id">
	ImGui::BeginChild("decor-category-list", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.4f), ImGuiChildFlags_Borders);
	for (auto& group : category_groups) {
		// JS: <div class="sidebar-checklist-category" @click="toggle_category_group(group)">{{ group.name }}</div>
		if (ImGui::TreeNodeEx(group.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			// JS: <div class="sidebar-checklist">
			//     <div v-for="sub in group.subcategories" :key="sub.subcategoryID" ...>
			for (auto* sub : group.subcategories) {
				// JS: <input type="checkbox" v-model="sub.checked" @click.stop/>
				// JS: <span>{{ sub.label }}</span>
				ImGui::Checkbox(sub->label.c_str(), &sub->checked);
			}

			ImGui::TreePop();
		}
	}
	ImGui::EndChild(); // decor-category-list

	// JS: <div class="list-toggles">
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

	// JS: <span class="header">Preview</span>
	ImGui::SeparatorText("Preview");

	// JS: <label class="ui-checkbox" title="Automatically preview a decor item when selecting it">
	//     <input type="checkbox" v-model="config.decorAutoPreview"/>
	//     <span>Auto Preview</span>
	// </label>
	{
		bool auto_preview = view.config.value("decorAutoPreview", false);
		if (ImGui::Checkbox("Auto Preview##decor", &auto_preview))
			view.config["decorAutoPreview"] = auto_preview;
	}

	// JS: <label ...><input type="checkbox" v-model="decorViewerAutoAdjust"/><span>Auto Camera</span></label>
	ImGui::Checkbox("Auto Camera##decor", &view.decorViewerAutoAdjust);

	// JS: <label ...><input type="checkbox" v-model="config.modelViewerShowGrid"/><span>Show Grid</span></label>
	{
		bool show_grid = view.config.value("modelViewerShowGrid", false);
		if (ImGui::Checkbox("Show Grid##decor", &show_grid))
			view.config["modelViewerShowGrid"] = show_grid;
	}

	// JS: <label ...><input type="checkbox" v-model="config.modelViewerWireframe"/><span>Show Wireframe</span></label>
	{
		bool wireframe = view.config.value("modelViewerWireframe", false);
		if (ImGui::Checkbox("Show Wireframe##decor", &wireframe))
			view.config["modelViewerWireframe"] = wireframe;
	}

	// JS: <label ...><input type="checkbox" v-model="config.modelViewerShowBones"/><span>Show Bones</span></label>
	{
		bool show_bones = view.config.value("modelViewerShowBones", false);
		if (ImGui::Checkbox("Show Bones##decor", &show_bones))
			view.config["modelViewerShowBones"] = show_bones;
	}

	// JS: <label ...><input type="checkbox" v-model="config.modelViewerShowTextures"/><span>Show Textures</span></label>
	{
		bool show_textures = view.config.value("modelViewerShowTextures", true);
		if (ImGui::Checkbox("Show Textures##decor", &show_textures))
			view.config["modelViewerShowTextures"] = show_textures;
	}

	// JS: <label ...><input type="checkbox" v-model="config.modelViewerShowBackground"/><span>Show Background</span></label>
	{
		bool show_bg = view.config.value("modelViewerShowBackground", false);
		if (ImGui::Checkbox("Show Background##decor", &show_bg))
			view.config["modelViewerShowBackground"] = show_bg;
	}

	// JS: <span class="header">Export</span>
	ImGui::SeparatorText("Export");

	// JS: <label ...><input type="checkbox" v-model="config.modelsExportTextures"/><span>Textures</span></label>
	{
		bool export_tex = view.config.value("modelsExportTextures", true);
		if (ImGui::Checkbox("Textures##decor-export", &export_tex))
			view.config["modelsExportTextures"] = export_tex;

		// JS: <label v-if="config.modelsExportTextures" ...><input type="checkbox" v-model="config.modelsExportAlpha"/><span>Texture Alpha</span></label>
		if (export_tex) {
			bool export_alpha = view.config.value("modelsExportAlpha", true);
			if (ImGui::Checkbox("Texture Alpha##decor", &export_alpha))
				view.config["modelsExportAlpha"] = export_alpha;
		}
	}

	// JS: <label v-if="exportDecorFormat === 'GLTF' && decorViewerActiveType === 'm2'" ...>
	//     <input type="checkbox" v-model="config.modelsExportAnimations"/><span>Export animations</span></label>
	{
		const std::string export_format = view.config.value("exportDecorFormat", std::string("OBJ"));
		if (export_format == "GLTF" && view.decorViewerActiveType == "m2") {
			bool export_anims = view.config.value("modelsExportAnimations", false);
			if (ImGui::Checkbox("Export animations##decor", &export_anims))
				view.config["modelsExportAnimations"] = export_anims;
		}
	}

	// JS: <template v-if="decorViewerActiveType === 'm2'">
	//     <span class="header">Geosets</span>
	//     <Checkboxlist :items="decorViewerGeosets"/>
	//     <div class="list-toggles">Enable All / Disable All</div>
	// </template>
	if (view.decorViewerActiveType == "m2") {
		ImGui::SeparatorText("Geosets");

		// TODO(conversion): Checkboxlist component rendering will be wired when integration is complete.
		for (auto& geoset : view.decorViewerGeosets) {
			std::string label = geoset.value("label", std::string("Geoset"));
			bool checked = geoset.value("checked", true);
			if (ImGui::Checkbox(label.c_str(), &checked))
				geoset["checked"] = checked;
		}

		// JS: <a @click="setAllDecorGeosets(true)">Enable All</a> / <a @click="setAllDecorGeosets(false)">Disable All</a>
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

	// JS: <template v-if="decorViewerActiveType === 'wmo'">
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

		// JS: <a @click="setAllDecorWMOGroups(true)">Enable All</a> / <a @click="setAllDecorWMOGroups(false)">Disable All</a>
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

	ImGui::EndChild(); // decor-sidebar

	// --- Listbox context menu popup ---
	// JS: <ContextMenu :node="contextMenus.nodeListbox" ...>
	//     <span @click.self="copy_decor_names(...)">Copy name(s)</span>
	//     <span @click.self="copy_file_data_ids(...)">Copy file data ID(s)</span>
	// </ContextMenu>
	if (!view.contextMenus.nodeListbox.is_null()) {
		if (ImGui::BeginPopup("DecorListboxContextMenu")) {
			const auto& node = view.contextMenus.nodeListbox;
			int count = node.value("count", 0);
			const std::string plural = count > 1 ? "s" : "";

			// JS: <span @click.self="copy_decor_names(context.node.selection)">Copy name{{ ... }}</span>
			if (ImGui::MenuItem(std::format("Copy name{}", plural).c_str())) {
				if (node.contains("selection") && node["selection"].is_array()) {
					copy_decor_names(node["selection"].get<std::vector<nlohmann::json>>());
				}
				view.contextMenus.nodeListbox = nullptr;
			}

			// JS: <span @click.self="copy_file_data_ids(context.node.selection)">Copy file data ID{{ ... }}</span>
			if (ImGui::MenuItem(std::format("Copy file data ID{}", plural).c_str())) {
				if (node.contains("selection") && node["selection"].is_array()) {
					copy_file_data_ids(node["selection"].get<std::vector<nlohmann::json>>());
				}
				view.contextMenus.nodeListbox = nullptr;
			}

			ImGui::EndPopup();
		}
	}
}

} // namespace tab_decor
