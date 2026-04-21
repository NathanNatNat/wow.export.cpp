/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_models_legacy.h"
#include "../log.h"
#include "../core.h"
#include "../buffer.h"
#include "../casc/export-helper.h"
#include "../install-type.h"
#include "../modules.h"
#include "../ui/listbox-context.h"
#include "../components/listbox.h"
#include "../constants.h"
#include "../3D/renderers/M2LegacyRendererGL.h"
#include "../3D/renderers/WMOLegacyRendererGL.h"
#include "../3D/renderers/MDXRendererGL.h"
#include "../3D/exporters/M2LegacyExporter.h"
#include "../3D/exporters/WMOLegacyExporter.h"
#include "../ui/texture-ribbon.h"
#include "../ui/model-viewer-utils.h"
#include "../3D/AnimMapper.h"
#include "../db/caches/DBCreaturesLegacy.h"
#include "../file-writer.h"
#include "../mpq/mpq-install.h"
#include "../components/model-viewer-gl.h"
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
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_models_legacy {

// --- File-local constants ---

static constexpr uint32_t MAGIC_MD20 = 0x3032444D;

static constexpr uint32_t MAGIC_MDLX = 0x584C444D;

enum class LegacyModelType {
	Unknown = 0,
	MDX,
	M2,
	WMO
};

// --- File-local state ---

static std::unique_ptr<M2LegacyRendererGL> active_renderer_m2;
static std::unique_ptr<WMOLegacyRendererGL> active_renderer_wmo;
static std::unique_ptr<MDXRendererGL> active_renderer_mdx;
static LegacyModelType active_renderer_type = LegacyModelType::Unknown;

static std::string active_path;

static std::map<std::string, db::caches::DBCreaturesLegacy::LegacyCreatureDisplay> active_skins;

// Change-detection for watches.
static std::string prev_anim_selection;
static std::vector<nlohmann::json> prev_selection_legacy_models;
static std::vector<nlohmann::json> prev_skins_selection;
static bool _was_paused_before_scrub = false;

static bool is_initialized = false;

static listbox::ListboxState listbox_legacy_models_state;

// Cached items string vector — only rebuilt when the source JSON changes.
static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

// Component states for CheckboxList, ListboxB, and MenuButton.
static checkboxlist::CheckboxListState checkboxlist_legacy_geosets_state;
static checkboxlist::CheckboxListState checkboxlist_legacy_wmo_groups_state;
static checkboxlist::CheckboxListState checkboxlist_legacy_wmo_sets_state;
static listboxb::ListboxBState listboxb_legacy_skins_state;
static menu_button::MenuButtonState menu_button_legacy_models_state;

// Model viewer GL state/context (replaces Vue <ModelViewerGL :context="legacyModelViewerContext"/>).
static model_viewer_gl::State viewer_state;
static model_viewer_gl::Context viewer_context;

// --- Internal helpers ---

static void clear_texture_preview() {
	core::view->legacyModelTexturePreviewURL.clear();
}

// Dispose active renderer.
static void dispose_active_renderer() {
	active_renderer_m2.reset();
	active_renderer_wmo.reset();
	active_renderer_mdx.reset();
	active_renderer_type = LegacyModelType::Unknown;
}

// --- Async preview model (background MPQ fetch, GL setup on main thread) ---

struct PendingLegacyPreview {
	std::string file_name;
	std::future<std::optional<std::vector<uint8_t>>> file_future;
	std::unique_ptr<BusyLock> busy_lock;
};

static std::optional<PendingLegacyPreview> pending_legacy_preview;

static void pump_legacy_preview() {
	if (!pending_legacy_preview.has_value())
		return;

	auto& task = *pending_legacy_preview;
	if (task.file_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	// File data is ready — process on main thread (GL operations).
	const std::string file_name = task.file_name;
	auto& view = *core::view;

	try {
		auto file_data_opt = task.file_future.get();
		if (!file_data_opt.has_value())
			throw std::runtime_error("File not found in MPQ: " + file_name);

		BufferWrapper data(std::move(file_data_opt.value()));

		uint32_t magic = data.readUInt32LE();
		data.seek(0);

		std::string file_name_lower = file_name;
		std::transform(file_name_lower.begin(), file_name_lower.end(), file_name_lower.begin(), ::tolower);

		gl::GLContext* gl_ctx = viewer_context.gl_context;
		if (!gl_ctx)
			throw std::runtime_error("GL context not available — model viewer not initialized.");

		if (magic == MAGIC_MDLX) {
			view.legacyModelViewerActiveType = "mdx";
			active_renderer_mdx = std::make_unique<MDXRendererGL>(data, *gl_ctx, true,
				view.config.value("modelViewerShowTextures", true));
			active_renderer_type = LegacyModelType::MDX;
		} else if (magic == MAGIC_MD20) {
			view.legacyModelViewerActiveType = "m2";
			active_renderer_m2 = std::make_unique<M2LegacyRendererGL>(data, *gl_ctx, true,
				view.config.value("modelViewerShowTextures", true));
			active_renderer_type = LegacyModelType::M2;
		} else if (file_name_lower.ends_with(".wmo")) {
			view.legacyModelViewerActiveType = "wmo";
			active_renderer_wmo = std::make_unique<WMOLegacyRendererGL>(data, file_name, *gl_ctx,
				view.config.value("modelViewerShowTextures", true));
			active_renderer_type = LegacyModelType::WMO;
		} else {
			throw std::runtime_error(std::format("Unknown legacy model format: 0x{:08x}", magic));
		}

		if (active_renderer_m2)
			active_renderer_m2->load();
		else if (active_renderer_mdx)
			active_renderer_mdx->load();
		else if (active_renderer_wmo)
			active_renderer_wmo->load();

		if (view.legacyModelViewerActiveType == "m2" || view.legacyModelViewerActiveType == "mdx") {
			std::vector<nlohmann::json> anim_list;

			if (view.legacyModelViewerActiveType == "m2" && active_renderer_m2 && active_renderer_m2->m2) {
				const auto& animations = active_renderer_m2->m2->animations;
				for (size_t i = 0; i < animations.size(); ++i) {
					const auto& animation = animations[i];
					anim_list.push_back({
						{"id", std::format("{}.{}", animation.id, animation.variationIndex)},
						{"animationId", animation.id},
						{"m2Index", static_cast<int>(i)},
						{"label", std::format("{} ({}.{})", get_anim_name(animation.id), animation.id, animation.variationIndex)}
					});
				}
			} else if (view.legacyModelViewerActiveType == "mdx" && active_renderer_mdx && active_renderer_mdx->mdx) {
				const auto& sequences = active_renderer_mdx->mdx->sequences;
				for (size_t i = 0; i < sequences.size(); ++i) {
					const auto& seq = sequences[i];
					std::string label = seq.name.empty() ? ("Animation " + std::to_string(i)) : seq.name;
					anim_list.push_back({
						{"id", std::to_string(i)},
						{"m2Index", static_cast<int>(i)},
						{"label", label}
					});
				}
			}

			std::vector<nlohmann::json> final_anim_list;
			final_anim_list.push_back({ {"id", "none"}, {"label", "No Animation"}, {"m2Index", -1} });
			final_anim_list.insert(final_anim_list.end(), anim_list.begin(), anim_list.end());

			view.legacyModelViewerAnims = final_anim_list;
			view.legacyModelViewerAnimSelection = "none";
		}

		if (view.legacyModelViewerActiveType == "m2") {
			const auto* displays = db::caches::DBCreaturesLegacy::getCreatureDisplaysByPath(file_name);

			if (displays && !displays->empty()) {
				std::vector<nlohmann::json> skin_list;

				std::string model_name;
				{
					std::filesystem::path fp(file_name);
					model_name = fp.stem().string();
					std::transform(model_name.begin(), model_name.end(), model_name.begin(), ::tolower);
				}

				for (const auto& display : *displays) {
					if (display.textures.empty())
						continue;

					const std::string& first_texture = display.textures[0];

					std::string skin_name;
					{
						std::filesystem::path tp(first_texture);
						skin_name = tp.stem().string();
						std::transform(skin_name.begin(), skin_name.end(), skin_name.begin(), ::tolower);
					}

					if (skin_name.starts_with(model_name))
						skin_name = skin_name.substr(model_name.size());

					if (skin_name.empty() || skin_name == "skin")
						skin_name = "base";

					std::string skin_id = std::to_string(display.id);

					std::string label = skin_name + " (" + skin_id + ")";

					if (active_skins.contains(skin_id))
						continue;

					skin_list.push_back({ {"id", skin_id}, {"label", label} });
					active_skins[skin_id] = display;
				}

				if (!skin_list.empty()) {
					view.legacyModelViewerSkins = skin_list;
					view.legacyModelViewerSkinsSelection = { skin_list[0] };
				}
			}
		}

		active_path = file_name;

		bool has_content = false;
		if (active_renderer_m2)
			has_content = !active_renderer_m2->get_draw_calls().empty();
		else if (active_renderer_mdx)
			has_content = !active_renderer_mdx->get_draw_calls().empty();
		else if (active_renderer_wmo)
			has_content = !active_renderer_wmo->get_groups().empty();

		if (!has_content) {
			core::setToast("info", std::format("The model {} doesn't have any 3D data associated with it.", file_name), {}, 4000);
		} else {
			core::hideToast();

			if (view.legacyModelViewerAutoAdjust && viewer_context.fitCamera) {
				core::postToMainThread([]() {
					if (viewer_context.fitCamera)
						viewer_context.fitCamera();
				});
			}
		}
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to preview model " + file_name,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to load legacy model: {}", e.what()));
	}

	pending_legacy_preview.reset();
}

static void preview_model(const std::string& file_name) {
	// Cancel any pending preview.
	pending_legacy_preview.reset();

	core::setToast("progress", std::format("Loading {}, please wait...", file_name), {}, -1, false);
	logging::write(std::format("Previewing legacy model {}", file_name));

	texture_ribbon::reset();
	clear_texture_preview();

	auto& view = *core::view;
	view.legacyModelViewerAnims.clear();
	view.legacyModelViewerAnimSelection = nullptr;
	view.legacyModelViewerSkins.clear();
	view.legacyModelViewerSkinsSelection.clear();
	active_skins.clear();

	if (active_renderer_type != LegacyModelType::Unknown) {
		dispose_active_renderer();
		active_path.clear();
	}

	mpq::MPQInstall* mpq = view.mpq.get();

	PendingLegacyPreview task;
	task.file_name = file_name;
	task.busy_lock = std::make_unique<BusyLock>(core::create_busy_lock());
	task.file_future = std::async(std::launch::async, [mpq, file_name]() {
		return mpq->getFile(file_name);
	});
	pending_legacy_preview = std::move(task);
}

// --- Template methods (mapped from Vue methods) ---

static void handle_listbox_context(const std::vector<std::string>& selection) {
	listbox_context::handle_context_menu(selection, true /* isLegacy */);
}

static void copy_file_paths(const std::vector<std::string>& selection) {
	listbox_context::copy_file_paths(selection);
}

static void copy_export_paths(const std::vector<std::string>& selection) {
	listbox_context::copy_export_paths(selection);
}

static void open_export_directory(const std::vector<std::string>& selection) {
	listbox_context::open_export_directory(selection);
}

static void export_model_action() {
	auto& view = *core::view;

	const auto& user_selection = view.selectionLegacyModels;

	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	export_files(user_selection);
}

static void toggle_animation_pause() {
	auto& view = *core::view;

	if (active_renderer_type == LegacyModelType::Unknown)
		return;

	bool paused = !view.legacyModelViewerAnimPaused;
	view.legacyModelViewerAnimPaused = paused;

	if (active_renderer_m2)
		active_renderer_m2->set_animation_paused(paused);
	else if (active_renderer_mdx) {
		active_renderer_mdx->set_animation_paused(paused);
	}
}

static void step_animation(int delta) {
	auto& view = *core::view;

	if (!view.legacyModelViewerAnimPaused)
		return;

	if (active_renderer_type == LegacyModelType::Unknown)
		return;

	if (active_renderer_m2) {
		active_renderer_m2->step_animation_frame(delta);
		view.legacyModelViewerAnimFrame = active_renderer_m2->get_animation_frame();
	} else if (active_renderer_mdx) {
		view.legacyModelViewerAnimFrame = 0;
	}
}

static void seek_animation(int frame) {
	auto& view = *core::view;

	if (active_renderer_type == LegacyModelType::Unknown)
		return;

	if (active_renderer_m2) {
		active_renderer_m2->set_animation_frame(frame);
		view.legacyModelViewerAnimFrame = frame;
	} else if (active_renderer_mdx) {
		view.legacyModelViewerAnimFrame = frame;
	}
}

static void start_scrub() {
	auto& view = *core::view;

	_was_paused_before_scrub = view.legacyModelViewerAnimPaused;

	if (!_was_paused_before_scrub) {
		view.legacyModelViewerAnimPaused = true;
		if (active_renderer_m2)
			active_renderer_m2->set_animation_paused(true);
		else if (active_renderer_mdx)
			active_renderer_mdx->set_animation_paused(true);
	}
}

static void end_scrub() {
	auto& view = *core::view;

	if (!_was_paused_before_scrub) {
		view.legacyModelViewerAnimPaused = false;
		if (active_renderer_m2)
			active_renderer_m2->set_animation_paused(false);
		else if (active_renderer_mdx)
			active_renderer_mdx->set_animation_paused(false);
	}
}

// --- Public API ---

void registerTab() {
	modules::register_nav_button("tab_models_legacy", "Models", "cube.svg", install_type::MPQ);
}

void mounted() {
	auto& view = *core::view;

	core::showLoadingScreen(3);

	try {
		core::progressLoadingScreen("Building legacy model list...");

		mpq::MPQInstall* mpq = view.mpq.get();
		const auto all_files = mpq->getAllFiles();

		// Filters: .m2, .mdx, .wmo (excluding WMO group files via LISTFILE_MODEL_FILTER)
		std::vector<std::string> model_files;
		for (const auto& f : all_files) {
			std::string lower = f;
			std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

			if (lower.ends_with(".m2") || lower.ends_with(".mdx")) {
				model_files.push_back(f);
			} else if (lower.ends_with(".wmo")) {
				if (!std::regex_search(lower, constants::LISTFILE_MODEL_FILTER()))
					model_files.push_back(f);
			}
		}

		std::sort(model_files.begin(), model_files.end());
		view.listfileLegacyModels.clear();
		for (const auto& f : model_files)
			view.listfileLegacyModels.push_back(f);

		core::progressLoadingScreen("Loading creature skin data...");

		db::caches::DBCreaturesLegacy::initializeCreatureData(
			[mpq](const std::string& path) -> std::vector<uint8_t> {
				auto data = mpq->getFile(path);
				return data.has_value() ? std::move(data.value()) : std::vector<uint8_t>{};
			},
			mpq->build_id
		);

		core::progressLoadingScreen("Initializing 3D preview...");

		//     this.$core.view.legacyModelViewerContext = Object.seal({ getActiveRenderer: () => active_renderer, gl_context: null, fitCamera: null });
		if (view.legacyModelViewerContext.is_null()) {
			view.legacyModelViewerContext = nlohmann::json::object();

			// Wire model viewer context callbacks.
			// Legacy renderers are not M2RendererGL; getActiveRenderer returns nullptr.
			// renderActiveModel handles all legacy renderer types.
			viewer_context.renderActiveModel = [](const float* view_mat, const float* proj_mat) {
				if (active_renderer_m2)
					active_renderer_m2->render(view_mat, proj_mat);
				else if (active_renderer_mdx)
					active_renderer_mdx->render(view_mat, proj_mat);
				else if (active_renderer_wmo)
					active_renderer_wmo->render(view_mat, proj_mat);
			};
			viewer_context.setActiveModelTransform = [](const std::array<float, 3>& pos,
														const std::array<float, 3>& rot,
														const std::array<float, 3>& scale) {
				if (active_renderer_m2)
					active_renderer_m2->setTransform(pos, rot, scale);
				else if (active_renderer_mdx)
					active_renderer_mdx->setTransform(pos, rot, scale);
				else if (active_renderer_wmo)
					active_renderer_wmo->setTransform(pos, rot, scale);
			};
			viewer_context.getActiveBoundingBox = []() -> std::optional<model_viewer_gl::BoundingBox> {
				if (active_renderer_m2) {
					auto bb = active_renderer_m2->getBoundingBox();
					if (bb) return model_viewer_gl::BoundingBox{ bb->min, bb->max };
				} else if (active_renderer_mdx) {
					auto bb = active_renderer_mdx->getBoundingBox();
					if (bb) return model_viewer_gl::BoundingBox{ bb->min, bb->max };
				} else if (active_renderer_wmo) {
					auto bb = active_renderer_wmo->getBoundingBox();
					if (bb) return model_viewer_gl::BoundingBox{ bb->min, bb->max };
				}
				return std::nullopt;
			};
		}

		core::hideLoadingScreen();
	} catch (const std::exception& error) {
		core::hideLoadingScreen();
		logging::write(std::format("Failed to initialize legacy models tab: {}", error.what()));
		core::setToast("error", "Failed to initialize legacy models tab. Check the log for details.");
	}

	// Initialize change-detection state for watches.

	if (view.legacyModelViewerAnimSelection.is_string())
		prev_anim_selection = view.legacyModelViewerAnimSelection.get<std::string>();
	else
		prev_anim_selection.clear();

	prev_selection_legacy_models = view.selectionLegacyModels;

	prev_skins_selection = view.legacyModelViewerSkinsSelection;

	is_initialized = true;
}

// --- Async export (one-file-per-frame, follows tab_models pattern) ---

struct PendingLegacyExport {
	std::vector<nlohmann::json> files;
	size_t next_index = 0;
	std::string format;
	int export_id = 0;
	std::optional<FileWriter> export_paths_writer;
	nlohmann::json manifest;
	std::optional<casc::ExportHelper> helper;
	bool helper_started = false;
};

static std::optional<PendingLegacyExport> pending_legacy_export;

static std::optional<std::string> build_stack_trace(const char* function_name, const std::exception& e) {
	return std::format("{}: {}", function_name, e.what());
}

static void show_tooltip_if_hovered(const char* text) {
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", text);
}

// Forward declaration
static void pump_legacy_export();

void export_files(const std::vector<nlohmann::json>& files, int export_id) {
	if (pending_legacy_export.has_value())
		return;

	auto& view = *core::view;

	std::string format = view.config.value("exportLegacyModelFormat", std::string("OBJ"));

	if (format == "PNG" || format == "CLIPBOARD") {
		// Single operation — no loop needed.
		if (!active_path.empty()) {
			gl::GLContext* gl_ctx = viewer_context.gl_context;
			if (gl_ctx && viewer_state.fbo != 0) {
				std::optional<FileWriter> export_paths_writer;
				if (format == "PNG")
					export_paths_writer.emplace(core::openLastExportStream());
				glBindFramebuffer(GL_FRAMEBUFFER, viewer_state.fbo);
				model_viewer_utils::export_preview(format, *gl_ctx, active_path, "",
					export_paths_writer.has_value() ? &export_paths_writer.value() : nullptr);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				if (export_paths_writer.has_value())
					export_paths_writer->close();
			}
		} else {
			core::setToast("error", "The selected export option only works for model previews. Preview something first!", {}, -1);
		}
		return;
	}

	if (format == "OBJ" || format == "STL" || format == "RAW") {
		PendingLegacyExport task;
		task.files = files;
		task.format = format;
		task.export_id = export_id;
		task.export_paths_writer.emplace(core::openLastExportStream());
		task.manifest = {
			{"type", "LEGACY_MODELS"},
			{"exportID", export_id},
			{"succeeded", nlohmann::json::array()},
			{"failed", nlohmann::json::array()}
		};
		task.helper.emplace(static_cast<int>(files.size()), "model");
		pending_legacy_export = std::move(task);
	} else {
		core::setToast("error", "Export format not yet implemented for legacy models: " + format, {}, -1);
	}
}

static void pump_legacy_export() {
	if (!pending_legacy_export.has_value())
		return;

	auto& task = *pending_legacy_export;
	auto& helper = task.helper.value();
	auto& view = *core::view;

	if (!task.helper_started) {
		helper.start();
		WMOLegacyExporter::clearCache();
		task.helper_started = true;
	}

	if (helper.isCancelled()) {
		pending_legacy_export.reset();
		return;
	}

	if (task.next_index >= task.files.size()) {
		helper.finish();
		pending_legacy_export.reset();
		return;
	}

	// Process one file per frame.
	const auto& file_entry_json = task.files[task.next_index++];
	FileWriter* export_paths = task.export_paths_writer.has_value() ? &task.export_paths_writer.value() : nullptr;
	mpq::MPQInstall* mpq = view.mpq.get();

	std::string file_name;
	if (file_entry_json.is_string())
		file_name = file_entry_json.get<std::string>();
	else
		return;

	std::vector<FileManifestEntry> file_manifest;

	try {
		auto file_data_opt = mpq->getFile(file_name);
		if (!file_data_opt.has_value())
			throw std::runtime_error("File not found in MPQ");

		std::string export_path = casc::ExportHelper::getExportPath(file_name);

		BufferWrapper data(std::move(file_data_opt.value()));

		std::string file_name_lower = file_name;
		std::transform(file_name_lower.begin(), file_name_lower.end(), file_name_lower.begin(), ::tolower);

		if (file_name_lower.ends_with(".wmo")) {
			WMOLegacyExporter exporter(std::move(data), file_name, mpq);

			if (file_name == active_path) {
				{
					std::vector<WMOGroupMaskEntry> group_mask;
					for (const auto& entry : view.modelViewerWMOGroups) {
						WMOGroupMaskEntry e;
						e.checked = entry.value("checked", false);
						e.groupIndex = entry.value("groupIndex", 0u);
						group_mask.push_back(e);
					}
					exporter.setGroupMask(group_mask);
				}
				{
					std::vector<WMODoodadSetMaskEntry> set_mask;
					for (const auto& entry : view.modelViewerWMOSets) {
						WMODoodadSetMaskEntry e;
						e.checked = entry.value("checked", false);
						set_mask.push_back(e);
					}
					exporter.setDoodadSetMask(set_mask);
				}
			}

			if (task.format == "OBJ") {
				export_path = casc::ExportHelper::replaceExtension(export_path, ".obj");
				exporter.exportAsOBJ(export_path, &helper, &file_manifest);
				if (export_paths) export_paths->writeLine("WMO_OBJ:" + export_path);
			} else if (task.format == "STL") {
				export_path = casc::ExportHelper::replaceExtension(export_path, ".stl");
				exporter.exportAsSTL(export_path, &helper, &file_manifest);
				if (export_paths) export_paths->writeLine("WMO_STL:" + export_path);
			} else {
				exporter.exportRaw(export_path, &helper, &file_manifest);
				if (export_paths) export_paths->writeLine("WMO_RAW:" + export_path);
			}
		} else if (file_name_lower.ends_with(".m2")) {
			M2LegacyExporter exporter(std::move(data), file_name, mpq);

			if (file_name == active_path) {
				const auto& skin_selection = view.legacyModelViewerSkinsSelection;
				if (!skin_selection.empty()) {
					const auto& selected_skin = skin_selection[0];
					std::string sel_id = selected_skin.value("id", std::string(""));
					auto it = active_skins.find(sel_id);
					if (it != active_skins.end() && !it->second.textures.empty())
						exporter.setSkinTextures(it->second.textures);
				}

				{
					std::vector<GeosetMaskEntry> geo_mask;
					for (const auto& entry : view.modelViewerGeosets) {
						GeosetMaskEntry e;
						e.checked = entry.value("checked", false);
						geo_mask.push_back(e);
					}
					exporter.setGeosetMask(geo_mask);
				}
			}

			if (task.format == "OBJ") {
				export_path = casc::ExportHelper::replaceExtension(export_path, ".obj");
				exporter.exportAsOBJ(export_path, &helper, &file_manifest);
				if (export_paths) export_paths->writeLine("M2_OBJ:" + export_path);
			} else if (task.format == "STL") {
				export_path = casc::ExportHelper::replaceExtension(export_path, ".stl");
				exporter.exportAsSTL(export_path, &helper, &file_manifest);
				if (export_paths) export_paths->writeLine("M2_STL:" + export_path);
			} else {
				exporter.exportRaw(export_path, &helper, &file_manifest);
				if (export_paths) export_paths->writeLine("M2_RAW:" + export_path);
			}
		} else {
			data.writeToFile(export_path);
			file_manifest.push_back({ "RAW", export_path });
			if (export_paths) export_paths->writeLine("RAW:" + export_path);
		}

		helper.mark(file_name, true);

		nlohmann::json files_json = nlohmann::json::array();
		for (const auto& entry : file_manifest)
			files_json.push_back({ {"type", entry.type}, {"file", entry.file.string()} });
		task.manifest["succeeded"].push_back({ {"file", file_name}, {"files", files_json} });
	} catch (const std::exception& e) {
		helper.mark(file_name, false, e.what(), build_stack_trace("export_files", e));
		task.manifest["failed"].push_back({ {"file", file_name} });
	}
}

std::variant<std::monostate, M2LegacyRendererGL*, MDXRendererGL*, WMOLegacyRendererGL*> getActiveRenderer() {
	if (active_renderer_m2)
		return active_renderer_m2.get();
	if (active_renderer_mdx)
		return active_renderer_mdx.get();
	if (active_renderer_wmo)
		return active_renderer_wmo.get();
	return std::monostate{};
}

void render() {
	auto& view = *core::view;

	// Poll for pending async preview/export.
	pump_legacy_preview();
	pump_legacy_export();

	if (!is_initialized)
		return;


	// Watch: legacyModelViewerAnimSelection → play animation
	{
		std::string current_anim;
		if (view.legacyModelViewerAnimSelection.is_string())
			current_anim = view.legacyModelViewerAnimSelection.get<std::string>();

		if (current_anim != prev_anim_selection) {
			prev_anim_selection = current_anim;

			bool has_play_animation = (active_renderer_m2 != nullptr || active_renderer_mdx != nullptr);
			if (has_play_animation && !view.legacyModelViewerAnims.empty()) {
				view.legacyModelViewerAnimPaused = false;
				view.legacyModelViewerAnimFrame = 0;
				view.legacyModelViewerAnimFrameCount = 0;

				if (!current_anim.empty()) {
					if (current_anim == "none") {
						if (active_renderer_m2)
							active_renderer_m2->stopAnimation();
						else if (active_renderer_mdx)
							active_renderer_mdx->stopAnimation();
					} else {
						int m2_index = -1;
						for (const auto& anim : view.legacyModelViewerAnims) {
							if (anim.value("id", std::string("")) == current_anim) {
								m2_index = anim.value("m2Index", -1);
								break;
							}
						}

						if (m2_index >= 0) {
							logging::write(std::format("Playing legacy animation at index {}", m2_index));
							if (active_renderer_m2) {
								active_renderer_m2->playAnimation(m2_index);
								view.legacyModelViewerAnimFrameCount = active_renderer_m2->get_animation_frame_count();
							} else if (active_renderer_mdx) {
								active_renderer_mdx->playAnimation(m2_index);
							}
						}
					}
				}
			}
		}
	}

	// Watch: selectionLegacyModels → auto-preview if legacyModelsAutoPreview
	{
		if (view.selectionLegacyModels != prev_selection_legacy_models) {
			prev_selection_legacy_models = view.selectionLegacyModels;

			if (view.config.value("legacyModelsAutoPreview", false)) {
				if (!view.selectionLegacyModels.empty()) {
					std::string first;
					if (view.selectionLegacyModels[0].is_string())
						first = view.selectionLegacyModels[0].get<std::string>();

					if (view.isBusy == 0 && !first.empty() && active_path != first)
						preview_model(first);
				}
			}
		}
	}

	// Watch: legacyModelViewerSkinsSelection → apply creature skin
	{
		if (view.legacyModelViewerSkinsSelection != prev_skins_selection) {
			prev_skins_selection = view.legacyModelViewerSkinsSelection;

			if (active_renderer_m2 && !active_skins.empty()) {
				if (!view.legacyModelViewerSkinsSelection.empty()) {
					const auto& selected = view.legacyModelViewerSkinsSelection[0];
					std::string sel_id = selected.value("id", std::string(""));

					auto it = active_skins.find(sel_id);
					if (it != active_skins.end()) {
						logging::write(std::format("Applying creature skin {} with {} textures", sel_id, it->second.textures.size()));
						active_renderer_m2->applyCreatureSkin(it->second.textures);
					}
				}
			}
		}
	}


	if (app::layout::BeginTab("tab-models-legacy")) {
		auto regions = app::layout::CalcListTabRegions(true);

		// --- Left panel: List container (row 1, col 1) ---
		//     <Listbox v-model:selection="selectionLegacyModels" v-model:filter="userInputFilterLegacyModels"
		//         :items="listfileLegacyModels" :keyinput="true" :regex="config.regexFilters" ...
		//         @contextmenu="handle_listbox_context" />
		if (app::layout::BeginListContainer("legacy-models-list-container", regions)) {
			const auto& items_str = core::cached_json_strings(view.listfileLegacyModels, s_items_cache, s_items_cache_size);

			std::vector<std::string> selection_str;
			for (const auto& s : view.selectionLegacyModels)
				selection_str.push_back(s.get<std::string>());

			listbox::render(
				"listbox-legacy-models",
				items_str,
				view.userInputFilterLegacyModels,
				selection_str,
				false,    // single
				true,     // keyinput
				view.config.value("regexFilters", false),
				[&]() {
					std::string cm = view.config.value("copyMode", std::string("Default"));
					if (cm == "DIR")
						return listbox::CopyMode::DIR;
					if (cm == "FID")
						return listbox::CopyMode::FID;
					return listbox::CopyMode::Default;
				}(),
				view.config.value("pasteSelection", false),
				view.config.value("removePathSpacesCopy", false),
				"model",  // unittype
				nullptr,  // overrideItems
				false,    // disable
				"legacy-models", // persistscrollkey
				view.legacyModelQuickFilters,
				false,    // nocopy
				listbox_legacy_models_state,
				[&](const std::vector<std::string>& new_sel) {
					view.selectionLegacyModels.clear();
					for (const auto& s : new_sel)
						view.selectionLegacyModels.push_back(s);
				},
				[](const listbox::ContextMenuEvent& ev) {
					handle_listbox_context(ev.selection);
				}
			);

			if (!view.contextMenus.nodeListbox.is_null()) {
				if (ImGui::BeginPopup("LegacyModelsListboxContextMenu")) {
					const auto& node = view.contextMenus.nodeListbox;
					std::vector<std::string> sel_strings;
					if (node.contains("selection") && node["selection"].is_array()) {
						for (const auto& s : node["selection"]) {
							if (s.is_string())
								sel_strings.push_back(s.get<std::string>());
						}
					}
					int count = node.value("count", 1);

					if (ImGui::MenuItem(std::format("Copy file path{}", count > 1 ? "s" : "").c_str())) {
						copy_file_paths(sel_strings);
						view.contextMenus.nodeListbox = nullptr;
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

		// --- Status bar ---
		if (app::layout::BeginStatusBar("legacy-models-status", regions)) {
			listbox::renderStatusBar("model", {}, listbox_legacy_models_state);
		}
		app::layout::EndStatusBar();

		// --- Filter bar (row 2, col 1) ---
		if (app::layout::BeginFilterBar("legacy-models-filter", regions)) {
			bool regexEnabled = view.config.value("regexFilters", false);
			float inputWidth = ImGui::GetContentRegionAvail().x;
			if (regexEnabled) {
				const char* regexLabel = "Regex Enabled";
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));
				float badgeWidth = ImGui::CalcTextSize(regexLabel).x + 12.0f;
				float rightPad = 10.0f;
				float badgeX = ImGui::GetContentRegionAvail().x - badgeWidth - rightPad;
				inputWidth = badgeX - 5.0f;
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
			std::strncpy(filter_buf, view.userInputFilterLegacyModels.c_str(), sizeof(filter_buf) - 1);
			if (ImGui::InputTextWithHint("##FilterLegacyModels", "Filter models...", filter_buf, sizeof(filter_buf)))
				view.userInputFilterLegacyModels = filter_buf;
		}
		app::layout::EndFilterBar();

		// --- Middle panel: Preview container (row 1, col 2) ---
		if (app::layout::BeginPreviewContainer("legacy-models-preview-container", regions)) {
			//         v-if="config.modelViewerShowTextures && textureRibbonStack.length > 0">
			if (view.config.value("modelViewerShowTextures", true) && !view.textureRibbonStack.empty()) {
				// Texture ribbon slot rendering with pagination (no context menu for legacy)
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
					if (slotTex != 0) {
						ImGui::ImageButton("##ribbon_slot",
							static_cast<ImTextureID>(static_cast<uintptr_t>(slotTex)),
							ImVec2(64, 64));
					} else {
						ImGui::Button(slotDisplayName.c_str(), ImVec2(64, 64));
					}
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", slotDisplayName.c_str());
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
				// Note: Legacy texture ribbon does NOT have a context menu (unlike modern models tab).
			}

			if (view.config.value("modelViewerShowBackground", false)) {
				std::string hex_str = view.config.value("modelViewerBackgroundColor", std::string("#343a40"));
				auto [cr, cg, cb] = model_viewer_gl::parse_hex_color(hex_str);
				float color[3] = {cr, cg, cb};
				if (ImGui::ColorEdit3("##bg_color_legacy_models", color, ImGuiColorEditFlags_NoInputs))
					view.config["modelViewerBackgroundColor"] = std::format("#{:02x}{:02x}{:02x}",
						static_cast<int>(color[0] * 255.0f), static_cast<int>(color[1] * 255.0f), static_cast<int>(color[2] * 255.0f));
			}

			if (!view.legacyModelViewerContext.is_null()) {
				model_viewer_gl::renderWidget("##legacy_model_viewer", viewer_state, viewer_context);
			}

			//     <div v-if="legacyModelViewerAnims && legacyModelViewerAnims.length > 0" class="preview-dropdown-overlay"> ... -->
			// The animation controls exist in the code but are not rendered in the template.
			/*
			if (!view.legacyModelViewerAnims.empty()) {
				// Animation dropdown and controls — disabled in original JS.
				// See the commented-out template block in the original source.
			}
			*/
		}
		app::layout::EndPreviewContainer();

		// --- Bottom: Export controls (row 2, col 2) ---
		//     <MenuButton :options="menuButtonLegacyModels" :default="config.exportLegacyModelFormat"
		//         @change="config.exportLegacyModelFormat = $event" :disabled="isBusy" @click="export_model" />
		if (app::layout::BeginPreviewControls("legacy-models-preview-controls", regions)) {
			std::vector<menu_button::MenuOption> mb_options;
			for (const auto& opt : view.menuButtonLegacyModels)
				mb_options.push_back({ opt.label, opt.value });
			menu_button::render("##MenuButtonLegacyModels", mb_options,
				view.config.value("exportLegacyModelFormat", std::string("OBJ")),
				view.isBusy > 0, false, true, menu_button_legacy_models_state,
				[&](const std::string& val) { view.config["exportLegacyModelFormat"] = val; },
				[&]() { export_model_action(); });
		}
		app::layout::EndPreviewControls();

		// --- Right panel: Sidebar (col 3, spanning both rows) ---
		if (app::layout::BeginSidebar("legacy-models-sidebar", regions)) {
			ImGui::SeparatorText("Preview");

			{
				bool auto_preview = view.config.value("legacyModelsAutoPreview", false);
				if (ImGui::Checkbox("Auto Preview##Legacy", &auto_preview))
					view.config["legacyModelsAutoPreview"] = auto_preview;
				show_tooltip_if_hovered("Automatically preview a model when selecting it");
			}

			ImGui::Checkbox("Auto Camera##Legacy", &view.legacyModelViewerAutoAdjust);
			show_tooltip_if_hovered("Automatically adjust camera when selecting a new model");

			{
				bool show_grid = view.config.value("modelViewerShowGrid", true);
				if (ImGui::Checkbox("Show Grid##Legacy", &show_grid))
					view.config["modelViewerShowGrid"] = show_grid;
				show_tooltip_if_hovered("Show a grid in the 3D viewport");
			}

			{
				bool wireframe = view.config.value("modelViewerWireframe", false);
				if (ImGui::Checkbox("Show Wireframe##Legacy", &wireframe))
					view.config["modelViewerWireframe"] = wireframe;
				show_tooltip_if_hovered("Render the preview model as a wireframe");
			}

			{
				bool show_textures = view.config.value("modelViewerShowTextures", true);
				if (ImGui::Checkbox("Show Textures##Legacy", &show_textures))
					view.config["modelViewerShowTextures"] = show_textures;
				show_tooltip_if_hovered("Show model textures in the preview pane");
			}

			{
				bool show_bg = view.config.value("modelViewerShowBackground", false);
				if (ImGui::Checkbox("Show Background##Legacy", &show_bg))
					view.config["modelViewerShowBackground"] = show_bg;
				show_tooltip_if_hovered("Show a background color in the 3D viewport");
			}

			if (view.legacyModelViewerActiveType == "m2" && !view.legacyModelViewerSkins.empty()) {
				ImGui::SeparatorText("Skins");

				{
					// Convert json skins to ListboxBItem array.
					std::vector<listboxb::ListboxBItem> skin_items;
					skin_items.reserve(view.legacyModelViewerSkins.size());
					for (const auto& skin : view.legacyModelViewerSkins)
						skin_items.push_back({ skin.value("label", std::string("")) });

					// Build selection indices from legacyModelViewerSkinsSelection.
					std::vector<int> sel_indices;
					for (const auto& sel : view.legacyModelViewerSkinsSelection) {
						std::string sel_id = sel.value("id", std::string(""));
						for (size_t i = 0; i < view.legacyModelViewerSkins.size(); ++i) {
							if (view.legacyModelViewerSkins[i].value("id", std::string("")) == sel_id) {
								sel_indices.push_back(static_cast<int>(i));
								break;
							}
						}
					}

					listboxb::render("##LegacyModelSkins", skin_items, sel_indices, true, true, false,
						listboxb_legacy_skins_state,
						[&](const std::vector<int>& new_sel) {
							view.legacyModelViewerSkinsSelection.clear();
							for (int idx : new_sel) {
								if (idx >= 0 && idx < static_cast<int>(view.legacyModelViewerSkins.size()))
									view.legacyModelViewerSkinsSelection.push_back(view.legacyModelViewerSkins[idx]);
							}
						});
				}
			}

			if (view.legacyModelViewerActiveType == "m2" || view.legacyModelViewerActiveType == "mdx") {
				ImGui::SeparatorText("Geosets");

				checkboxlist::render("##LegacyGeosets", view.modelViewerGeosets, checkboxlist_legacy_geosets_state);

				if (ImGui::SmallButton("Enable All##LegacyGeosets")) {
					for (auto& g : view.modelViewerGeosets)
						g["checked"] = true;
				}
				ImGui::SameLine();
				ImGui::Text("/");
				ImGui::SameLine();
				if (ImGui::SmallButton("Disable All##LegacyGeosets")) {
					for (auto& g : view.modelViewerGeosets)
						g["checked"] = false;
				}
			}

			if (view.legacyModelViewerActiveType == "wmo") {
				ImGui::SeparatorText("WMO Groups");

				checkboxlist::render("##LegacyWMOGroups", view.modelViewerWMOGroups, checkboxlist_legacy_wmo_groups_state);

				if (ImGui::SmallButton("Enable All##LegacyWMOGroups")) {
					for (auto& g : view.modelViewerWMOGroups)
						g["checked"] = true;
				}
				ImGui::SameLine();
				ImGui::Text("/");
				ImGui::SameLine();
				if (ImGui::SmallButton("Disable All##LegacyWMOGroups")) {
					for (auto& g : view.modelViewerWMOGroups)
						g["checked"] = false;
				}

				ImGui::SeparatorText("Doodad Sets");

				checkboxlist::render("##LegacyWMOSets", view.modelViewerWMOSets, checkboxlist_legacy_wmo_sets_state);
			}
		}
		app::layout::EndSidebar();
	}
	app::layout::EndTab();
}

} // namespace tab_models_legacy
