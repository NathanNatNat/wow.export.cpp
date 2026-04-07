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
#include "../ui/listbox-context.h"
#include "../constants.h"
#include "../3D/renderers/M2LegacyRendererGL.h"
#include "../3D/renderers/WMOLegacyRendererGL.h"
#include "../3D/renderers/MDXRendererGL.h"
#include "../3D/exporters/M2LegacyExporter.h"
#include "../3D/exporters/WMOLegacyExporter.h"
#include "../ui/texture-ribbon.h"
#include "../3D/AnimMapper.h"
#include "../db/caches/DBCreaturesLegacy.h"
#include "../file-writer.h"
#include "../mpq/mpq-install.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_models_legacy {

// --- File-local constants ---

// JS: const MAGIC_MD20 = 0x3032444D; // 'MD20'
static constexpr uint32_t MAGIC_MD20 = 0x3032444D;

// JS: const MAGIC_MDLX = 0x584C444D; // 'MDLX'
static constexpr uint32_t MAGIC_MDLX = 0x584C444D;

// JS: const MODEL_TYPE_MDX = Symbol('modelMDX');
// JS: const MODEL_TYPE_M2 = Symbol('modelM2');
// JS: const MODEL_TYPE_WMO = Symbol('modelWMO');
enum class LegacyModelType {
	Unknown = 0,
	MDX,
	M2,
	WMO
};

// --- File-local state ---

// JS: let active_renderer;
static std::unique_ptr<M2LegacyRendererGL> active_renderer_m2;
static std::unique_ptr<WMOLegacyRendererGL> active_renderer_wmo;
static std::unique_ptr<MDXRendererGL> active_renderer_mdx;
static LegacyModelType active_renderer_type = LegacyModelType::Unknown;

// JS: let active_path;
static std::string active_path;

// JS: const active_skins = new Map(); // skin_id -> display info
static std::map<std::string, db::caches::DBCreaturesLegacy::LegacyCreatureDisplay> active_skins;

// Change-detection for watches.
static std::string prev_anim_selection;
static std::vector<nlohmann::json> prev_selection_legacy_models;
static std::vector<nlohmann::json> prev_skins_selection;
static bool _was_paused_before_scrub = false;

static bool is_initialized = false;

// --- Internal helpers ---

// JS: const clear_texture_preview = (core) => { core.view.legacyModelTexturePreviewURL = ''; };
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

// JS: const preview_model = async (core, file_name) => { ... }
static void preview_model(const std::string& file_name) {
	auto _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", file_name), nullptr, -1, false);
	logging::write(std::format("Previewing legacy model {}", file_name));

	texture_ribbon::reset();
	clear_texture_preview();

	auto& view = *core::view;
	// JS: core.view.legacyModelViewerAnims = [];
	view.legacyModelViewerAnims.clear();
	// JS: core.view.legacyModelViewerAnimSelection = null;
	view.legacyModelViewerAnimSelection = nullptr;
	// JS: core.view.legacyModelViewerSkins = [];
	view.legacyModelViewerSkins.clear();
	// JS: core.view.legacyModelViewerSkinsSelection = [];
	view.legacyModelViewerSkinsSelection.clear();
	// JS: active_skins.clear();
	active_skins.clear();

	try {
		// JS: if (active_renderer) { active_renderer.dispose(); active_renderer = null; active_path = null; }
		if (active_renderer_type != LegacyModelType::Unknown) {
			dispose_active_renderer();
			active_path.clear();
		}

		// JS: const mpq = core.view.mpq;
		// TODO(conversion): MPQ source will be wired when integration is complete.
		// mpq::MPQInstall* mpq = view.mpq;

		// JS: const file_data = mpq.getFile(file_name);
		// JS: if (!file_data) throw new Error('File not found in MPQ: ' + file_name);
		// TODO(conversion): MPQ file loading will be wired when integration is complete.
		core::setToast("info", std::format("MPQ integration pending — cannot preview {} yet.", file_name), nullptr, 4000);
		logging::write(std::format("MPQ not yet integrated — skipping preview for {}", file_name));

		// The following code is the complete conversion and will work once MPQ is wired:
		/*
		auto file_data_opt = mpq->getFile(file_name);
		if (!file_data_opt.has_value())
			throw std::runtime_error("File not found in MPQ: " + file_name);

		// JS: const data = new BufferWrapper(Buffer.from(file_data));
		BufferWrapper data(std::move(file_data_opt.value()));

		// JS: const magic = data.readUInt32LE();
		uint32_t magic = data.readUInt32LE();
		// JS: data.seek(0);
		data.seek(0);

		// JS: const file_name_lower = file_name.toLowerCase();
		std::string file_name_lower = file_name;
		std::transform(file_name_lower.begin(), file_name_lower.end(), file_name_lower.begin(), ::tolower);

		// JS: const gl_context = core.view.legacyModelViewerContext?.gl_context;
		// TODO(conversion): GL context retrieval from legacyModelViewerContext will be wired.

		if (magic == MAGIC_MDLX) {
			// JS: core.view.legacyModelViewerActiveType = 'mdx';
			view.legacyModelViewerActiveType = "mdx";
			// JS: active_renderer = new MDXRendererGL(data, gl_context, true, core.view.config.modelViewerShowTextures);
			active_renderer_mdx = std::make_unique<MDXRendererGL>(data, gl_context, true,
				view.config.value("modelViewerShowTextures", true));
			active_renderer_type = LegacyModelType::MDX;
		} else if (magic == MAGIC_MD20) {
			// JS: core.view.legacyModelViewerActiveType = 'm2';
			view.legacyModelViewerActiveType = "m2";
			// JS: active_renderer = new M2LegacyRendererGL(data, gl_context, true, core.view.config.modelViewerShowTextures);
			active_renderer_m2 = std::make_unique<M2LegacyRendererGL>(data, gl_context, true,
				view.config.value("modelViewerShowTextures", true));
			active_renderer_type = LegacyModelType::M2;
		} else if (file_name_lower.ends_with(".wmo")) {
			// JS: core.view.legacyModelViewerActiveType = 'wmo';
			view.legacyModelViewerActiveType = "wmo";
			// JS: active_renderer = new WMOLegacyRendererGL(data, file_name, gl_context, core.view.config.modelViewerShowTextures);
			active_renderer_wmo = std::make_unique<WMOLegacyRendererGL>(data, 0, gl_context,
				view.config.value("modelViewerShowTextures", true));
			active_renderer_type = LegacyModelType::WMO;
		} else {
			throw std::runtime_error(std::format("Unknown legacy model format: 0x{:08x}", magic));
		}

		// JS: await active_renderer.load();
		if (active_renderer_m2)
			active_renderer_m2->load();
		else if (active_renderer_mdx)
			active_renderer_mdx->load();
		else if (active_renderer_wmo)
			active_renderer_wmo->load();

		// JS: if (core.view.legacyModelViewerActiveType === 'm2' || core.view.legacyModelViewerActiveType === 'mdx') { setup animations }
		if (view.legacyModelViewerActiveType == "m2" || view.legacyModelViewerActiveType == "mdx") {
			std::vector<nlohmann::json> anim_list;

			if (view.legacyModelViewerActiveType == "m2" && active_renderer_m2 && active_renderer_m2->m2) {
				// JS: const model = active_renderer.m2;
				const auto& animations = active_renderer_m2->m2->animations;
				for (size_t i = 0; i < animations.size(); ++i) {
					const auto& animation = animations[i];
					// JS: anim_list.push({ id: `${animation.id}.${animation.variationIndex}`, animationId: animation.id, m2Index: i,
					//         label: AnimMapper.get_anim_name(animation.id) + ' (' + animation.id + '.' + animation.variationIndex + ')' });
					anim_list.push_back({
						{"id", std::format("{}.{}", animation.id, animation.variationIndex)},
						{"animationId", animation.id},
						{"m2Index", static_cast<int>(i)},
						{"label", std::format("{} ({}.{})", get_anim_name(animation.id), animation.id, animation.variationIndex)}
					});
				}
			} else if (view.legacyModelViewerActiveType == "mdx" && active_renderer_mdx && active_renderer_mdx->mdx) {
				// JS: const model = active_renderer.mdx;
				// MDXLoader uses 'sequences' not 'animations'
				const auto& sequences = active_renderer_mdx->mdx->sequences;
				for (size_t i = 0; i < sequences.size(); ++i) {
					const auto& seq = sequences[i];
					// JS: anim_list.push({ id: i.toString(), m2Index: i, label: animation.name || ('Animation ' + i) });
					std::string label = seq.name.empty() ? ("Animation " + std::to_string(i)) : seq.name;
					anim_list.push_back({
						{"id", std::to_string(i)},
						{"m2Index", static_cast<int>(i)},
						{"label", label}
					});
				}
			}

			// JS: const final_anim_list = [{ id: 'none', label: 'No Animation', m2Index: -1 }, ...anim_list];
			std::vector<nlohmann::json> final_anim_list;
			final_anim_list.push_back({ {"id", "none"}, {"label", "No Animation"}, {"m2Index", -1} });
			final_anim_list.insert(final_anim_list.end(), anim_list.begin(), anim_list.end());

			// JS: core.view.legacyModelViewerAnims = final_anim_list;
			view.legacyModelViewerAnims = final_anim_list;
			// JS: core.view.legacyModelViewerAnimSelection = 'none';
			view.legacyModelViewerAnimSelection = "none";
		}

		// JS: if (core.view.legacyModelViewerActiveType === 'm2') { setup skins }
		if (view.legacyModelViewerActiveType == "m2") {
			// JS: const displays = DBCreaturesLegacy.getCreatureDisplaysByPath(file_name);
			const auto* displays = db::caches::DBCreaturesLegacy::getCreatureDisplaysByPath(file_name);

			if (displays && !displays->empty()) {
				std::vector<nlohmann::json> skin_list;

				// JS: const model_name = path.basename(file_name, '.m2').toLowerCase();
				std::string model_name;
				{
					std::filesystem::path fp(file_name);
					model_name = fp.stem().string();
					std::transform(model_name.begin(), model_name.end(), model_name.begin(), ::tolower);
				}

				for (const auto& display : *displays) {
					// JS: if (display.textures.length === 0) continue;
					if (display.textures.empty())
						continue;

					// JS: const first_texture = display.textures[0];
					const std::string& first_texture = display.textures[0];

					// JS: let skin_name = path.basename(first_texture, '.blp').toLowerCase();
					std::string skin_name;
					{
						std::filesystem::path tp(first_texture);
						skin_name = tp.stem().string();
						std::transform(skin_name.begin(), skin_name.end(), skin_name.begin(), ::tolower);
					}

					// JS: if (skin_name.startsWith(model_name)) skin_name = skin_name.substring(model_name.length);
					if (skin_name.starts_with(model_name))
						skin_name = skin_name.substr(model_name.size());

					// JS: if (skin_name.length === 0 || skin_name === 'skin') skin_name = 'base';
					if (skin_name.empty() || skin_name == "skin")
						skin_name = "base";

					// JS: const skin_id = display.id.toString();
					std::string skin_id = std::to_string(display.id);

					// JS: const label = skin_name + ' (' + display.id + ')';
					std::string label = skin_name + " (" + skin_id + ")";

					// JS: if (active_skins.has(skin_id)) continue;
					if (active_skins.contains(skin_id))
						continue;

					// JS: skin_list.push({ id: skin_id, label: label });
					skin_list.push_back({ {"id", skin_id}, {"label", label} });
					// JS: active_skins.set(skin_id, display);
					active_skins[skin_id] = display;
				}

				// JS: if (skin_list.length > 0) { ... }
				if (!skin_list.empty()) {
					// JS: core.view.legacyModelViewerSkins = skin_list;
					view.legacyModelViewerSkins = skin_list;
					// JS: core.view.legacyModelViewerSkinsSelection = skin_list.slice(0, 1);
					view.legacyModelViewerSkinsSelection = { skin_list[0] };
				}
			}
		}

		// JS: active_path = file_name;
		active_path = file_name;

		// JS: const has_content = active_renderer.draw_calls?.length > 0 || active_renderer.groups?.length > 0;
		bool has_content = false;
		if (active_renderer_m2)
			has_content = !active_renderer_m2->draw_calls.empty();
		else if (active_renderer_mdx)
			has_content = !active_renderer_mdx->draw_calls.empty();
		else if (active_renderer_wmo)
			has_content = !active_renderer_wmo->groups.empty();

		if (!has_content) {
			// JS: core.setToast('info', util.format('The model %s doesn\'t have any 3D data associated with it.', file_name), null, 4000);
			core::setToast("info", std::format("The model {} doesn't have any 3D data associated with it.", file_name), nullptr, 4000);
		} else {
			core::hideToast();

			// JS: if (core.view.legacyModelViewerAutoAdjust) requestAnimationFrame(() => core.view.legacyModelViewerContext?.fitCamera?.());
			// TODO(conversion): fitCamera will be wired when model viewer GL is integrated.
		}
		*/
	} catch (const std::exception& e) {
		// JS: core.setToast('error', 'Unable to preview model ' + file_name, ...);
		core::setToast("error", "Unable to preview model " + file_name, nullptr, -1);
		logging::write(std::format("Failed to load legacy model: {}", e.what()));
	}
}

// --- Template methods (mapped from Vue methods) ---

// JS: methods.handle_listbox_context(data) { listboxContext.handle_context_menu(data); }
static void handle_listbox_context(const std::vector<std::string>& selection) {
	listbox_context::handle_context_menu(selection, true /* isLegacy */);
}

// JS: methods.copy_file_paths(selection) { listboxContext.copy_file_paths(selection); }
static void copy_file_paths(const std::vector<std::string>& selection) {
	listbox_context::copy_file_paths(selection);
}

// JS: methods.copy_export_paths(selection) { listboxContext.copy_export_paths(selection); }
static void copy_export_paths(const std::vector<std::string>& selection) {
	listbox_context::copy_export_paths(selection);
}

// JS: methods.open_export_directory(selection) { listboxContext.open_export_directory(selection); }
static void open_export_directory(const std::vector<std::string>& selection) {
	listbox_context::open_export_directory(selection);
}

// JS: methods.export_model() { ... }
static void export_model_action() {
	auto& view = *core::view;

	// JS: const user_selection = this.$core.view.selectionLegacyModels;
	const auto& user_selection = view.selectionLegacyModels;

	// JS: if (user_selection.length === 0) { ... }
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any files to export; you should do that first.");
		return;
	}

	// JS: await export_files(this.$core, user_selection);
	export_files(user_selection);
}

// JS: methods.toggle_animation_pause() { ... }
static void toggle_animation_pause() {
	auto& view = *core::view;

	// JS: const renderer = active_renderer; if (!renderer) return;
	if (active_renderer_type == LegacyModelType::Unknown)
		return;

	// JS: const paused = !this.$core.view.legacyModelViewerAnimPaused;
	bool paused = !view.legacyModelViewerAnimPaused;
	// JS: this.$core.view.legacyModelViewerAnimPaused = paused;
	view.legacyModelViewerAnimPaused = paused;

	// JS: renderer.set_animation_paused?.(paused);
	if (active_renderer_m2)
		active_renderer_m2->set_animation_paused(paused);
	else if (active_renderer_mdx) {
		// MDX renderer may not support pausing; optional call pattern
		// active_renderer_mdx->set_animation_paused(paused); // TODO(conversion): verify MDX pause support
	}
}

// JS: methods.step_animation(delta) { ... }
static void step_animation(int delta) {
	auto& view = *core::view;

	// JS: if (!this.$core.view.legacyModelViewerAnimPaused) return;
	if (!view.legacyModelViewerAnimPaused)
		return;

	// JS: const renderer = active_renderer; if (!renderer) return;
	if (active_renderer_type == LegacyModelType::Unknown)
		return;

	// JS: renderer.step_animation_frame?.(delta);
	// JS: this.$core.view.legacyModelViewerAnimFrame = renderer.get_animation_frame?.() || 0;
	if (active_renderer_m2) {
		active_renderer_m2->step_animation_frame(delta);
		view.legacyModelViewerAnimFrame = active_renderer_m2->get_animation_frame();
	}
	// MDX renderer may not support frame stepping
}

// JS: methods.seek_animation(frame) { ... }
static void seek_animation(int frame) {
	auto& view = *core::view;

	// JS: const renderer = active_renderer; if (!renderer) return;
	if (active_renderer_type == LegacyModelType::Unknown)
		return;

	// JS: renderer.set_animation_frame?.(parseInt(frame));
	// JS: this.$core.view.legacyModelViewerAnimFrame = parseInt(frame);
	if (active_renderer_m2) {
		active_renderer_m2->set_animation_frame(frame);
		view.legacyModelViewerAnimFrame = frame;
	}
}

// JS: methods.start_scrub() { ... }
static void start_scrub() {
	auto& view = *core::view;

	// JS: this._was_paused_before_scrub = this.$core.view.legacyModelViewerAnimPaused;
	_was_paused_before_scrub = view.legacyModelViewerAnimPaused;

	// JS: if (!this._was_paused_before_scrub) { ... }
	if (!_was_paused_before_scrub) {
		view.legacyModelViewerAnimPaused = true;
		if (active_renderer_m2)
			active_renderer_m2->set_animation_paused(true);
	}
}

// JS: methods.end_scrub() { ... }
static void end_scrub() {
	auto& view = *core::view;

	// JS: if (!this._was_paused_before_scrub) { ... }
	if (!_was_paused_before_scrub) {
		view.legacyModelViewerAnimPaused = false;
		if (active_renderer_m2)
			active_renderer_m2->set_animation_paused(false);
	}
}

// --- Public API ---

// JS: register() { this.registerNavButton('Models', 'cube.svg', InstallType.MPQ); }
void registerTab() {
	// TODO(conversion): Nav button registration will be wired when the module system is integrated.
}

// JS: async mounted() { ... }
void mounted() {
	auto& view = *core::view;

	// JS: this.$core.showLoadingScreen(3);
	core::showLoadingScreen(3);

	try {
		// JS: await this.$core.progressLoadingScreen('Building legacy model list...');
		core::progressLoadingScreen("Building legacy model list...");

		// JS: const mpq = this.$core.view.mpq;
		// JS: const all_files = mpq.getAllFiles();
		// TODO(conversion): MPQ source will be wired when integration is complete.
		// const auto all_files = mpq->getAllFiles();

		// JS: const model_files = all_files.filter(f => { ... });
		// Filters: .m2, .mdx, .wmo (excluding WMO group files via LISTFILE_MODEL_FILTER)
		/*
		std::vector<std::string> model_files;
		for (const auto& f : all_files) {
			std::string lower = f;
			std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

			if (lower.ends_with(".m2") || lower.ends_with(".mdx")) {
				model_files.push_back(f);
			} else if (lower.ends_with(".wmo")) {
				// JS: return !constants.LISTFILE_MODEL_FILTER.test(lower);
				if (!std::regex_search(lower, constants::LISTFILE_MODEL_FILTER()))
					model_files.push_back(f);
			}
		}

		// JS: this.$core.view.listfileLegacyModels = model_files.sort();
		std::sort(model_files.begin(), model_files.end());
		view.listfileLegacyModels.clear();
		for (const auto& f : model_files)
			view.listfileLegacyModels.push_back(f);
		*/

		// JS: await this.$core.progressLoadingScreen('Loading creature skin data...');
		core::progressLoadingScreen("Loading creature skin data...");

		// JS: await DBCreaturesLegacy.initializeCreatureData(mpq, mpq.build_id);
		// TODO(conversion): MPQ creature data loading will be wired when integration is complete.
		/*
		db::caches::DBCreaturesLegacy::initializeCreatureData(
			[mpq](const std::string& path) -> std::vector<uint8_t> {
				auto data = mpq->getFile(path);
				return data.has_value() ? std::move(data.value()) : std::vector<uint8_t>{};
			},
			mpq->build_id
		);
		*/

		// JS: await this.$core.progressLoadingScreen('Initializing 3D preview...');
		core::progressLoadingScreen("Initializing 3D preview...");

		// JS: if (!this.$core.view.legacyModelViewerContext)
		//     this.$core.view.legacyModelViewerContext = Object.seal({ getActiveRenderer: () => active_renderer, gl_context: null, fitCamera: null });
		if (view.legacyModelViewerContext.is_null()) {
			view.legacyModelViewerContext = nlohmann::json::object();
		}

		// JS: this.$core.hideLoadingScreen();
		core::hideLoadingScreen();
	} catch (const std::exception& error) {
		core::hideLoadingScreen();
		logging::write(std::format("Failed to initialize legacy models tab: {}", error.what()));
		core::setToast("error", "Failed to initialize legacy models tab. Check the log for details.");
	}

	// Initialize change-detection state for watches.

	// JS: this.$core.view.$watch('legacyModelViewerAnimSelection', async selected_animation_id => { ... });
	if (view.legacyModelViewerAnimSelection.is_string())
		prev_anim_selection = view.legacyModelViewerAnimSelection.get<std::string>();
	else
		prev_anim_selection.clear();

	// JS: this.$core.view.$watch('selectionLegacyModels', async selection => { ... });
	prev_selection_legacy_models = view.selectionLegacyModels;

	// JS: this.$core.view.$watch('legacyModelViewerSkinsSelection', async selection => { ... });
	prev_skins_selection = view.legacyModelViewerSkinsSelection;

	is_initialized = true;
}

// JS: export_files = async (core, files, export_id = -1) => { ... }
void export_files(const std::vector<nlohmann::json>& files, int export_id) {
	auto& view = *core::view;

	// JS: const export_paths = core.openLastExportStream();
	FileWriter export_paths_writer = core::openLastExportStream();
	FileWriter* export_paths = &export_paths_writer;

	// JS: const format = core.view.config.exportLegacyModelFormat;
	std::string format = view.config.value("exportLegacyModelFormat", std::string("OBJ"));

	// JS: const manifest = { type: 'LEGACY_MODELS', exportID: export_id, succeeded: [], failed: [] };
	nlohmann::json manifest = {
		{"type", "LEGACY_MODELS"},
		{"exportID", export_id},
		{"succeeded", nlohmann::json::array()},
		{"failed", nlohmann::json::array()}
	};

	// JS: if (format === 'PNG' || format === 'CLIPBOARD') { ... }
	if (format == "PNG" || format == "CLIPBOARD") {
		if (!active_path.empty()) {
			// JS: core.setToast('progress', 'Saving preview, hold on...', null, -1, false);
			core::setToast("progress", "Saving preview, hold on...", nullptr, -1, false);

			// JS: const canvas = document.getElementById('legacy-model-preview').querySelector('canvas');
			// JS: const buf = await BufferWrapper.fromCanvas(canvas, 'image/png');
			// TODO(conversion): GL framebuffer capture will be wired when model viewer GL is integrated.

			if (format == "PNG") {
				// JS: const export_path = ExportHelper.getExportPath(active_path);
				std::string export_path = casc::ExportHelper::getExportPath(active_path);
				// JS: let out_file = ExportHelper.replaceExtension(export_path, '.png');
				std::string out_file = casc::ExportHelper::replaceExtension(export_path, ".png");

				// JS: if (core.view.config.modelsExportPngIncrements) out_file = await ExportHelper.getIncrementalFilename(out_file);
				if (view.config.value("modelsExportPngIncrements", false))
					out_file = casc::ExportHelper::getIncrementalFilename(out_file);

				// JS: const out_dir = path.dirname(out_file);
				std::string out_dir = std::filesystem::path(out_file).parent_path().string();

				// JS: await buf.writeToFile(out_file);
				// TODO(conversion): PNG write from framebuffer will be wired when GL is integrated.

				// JS: await export_paths?.writeLine('PNG:' + out_file);
				if (export_paths)
					export_paths->writeLine("PNG:" + out_file);

				logging::write(std::format("Saved legacy 3D preview screenshot to {}", out_file));
				// JS: core.setToast('success', util.format('Successfully exported preview to %s', out_file), ...);
				core::setToast("success", std::format("Successfully exported preview to {}", out_file), nullptr, -1);
			} else if (format == "CLIPBOARD") {
				// JS: const clipboard = nw.Clipboard.get(); clipboard.set(buf.toBase64(), 'png', true);
				// TODO(conversion): Clipboard PNG copy will be wired when GL integration is complete.

				logging::write(std::format("Copied legacy 3D preview to clipboard ({})", active_path));
				core::setToast("success", "3D preview has been copied to the clipboard", nullptr, -1, true);
			}
		} else {
			// JS: core.setToast('error', 'The selected export option only works for model previews. Preview something first!', null, -1);
			core::setToast("error", "The selected export option only works for model previews. Preview something first!", nullptr, -1);
		}
	} else if (format == "OBJ" || format == "STL" || format == "RAW") {
		// JS: const mpq = core.view.mpq;
		// TODO(conversion): MPQ source will be wired when integration is complete.

		// JS: const helper = new ExportHelper(files.length, 'model');
		casc::ExportHelper helper(static_cast<int>(files.size()), "model");
		// JS: helper.start();
		helper.start();

		// JS: WMOLegacyExporter.clearCache();
		WMOLegacyExporter::clearCache();

		for (const auto& file_entry_json : files) {
			// JS: if (helper.isCancelled()) return;
			if (helper.isCancelled())
				return;

			// JS: let file_name = file_entry;
			std::string file_name;
			if (file_entry_json.is_string())
				file_name = file_entry_json.get<std::string>();
			else
				continue;

			std::vector<nlohmann::json> file_manifest;

			try {
				// JS: const file_data = mpq.getFile(file_name);
				// JS: if (!file_data) throw new Error('File not found in MPQ');
				// TODO(conversion): MPQ file loading will be wired when integration is complete.
				throw std::runtime_error("MPQ integration pending");

				/*
				auto file_data_opt = mpq->getFile(file_name);
				if (!file_data_opt.has_value())
					throw std::runtime_error("File not found in MPQ");

				// JS: let export_path = ExportHelper.getExportPath(file_name);
				std::string export_path = casc::ExportHelper::getExportPath(file_name);

				// JS: const data = new BufferWrapper(Buffer.from(file_data));
				BufferWrapper data(std::move(file_data_opt.value()));

				// JS: const file_name_lower = file_name.toLowerCase();
				std::string file_name_lower = file_name;
				std::transform(file_name_lower.begin(), file_name_lower.end(), file_name_lower.begin(), ::tolower);

				if (file_name_lower.ends_with(".wmo")) {
					// JS: const exporter = new WMOLegacyExporter(data, file_name, mpq);
					WMOLegacyExporter exporter(std::move(data), file_name, mpq_archive);

					// JS: if (file_name === active_path) { exporter.setGroupMask(...); exporter.setDoodadSetMask(...); }
					if (file_name == active_path) {
						// TODO(conversion): Group/set mask conversion from JSON to typed vectors
						// exporter.setGroupMask(view.modelViewerWMOGroups);
						// exporter.setDoodadSetMask(view.modelViewerWMOSets);
					}

					if (format == "OBJ") {
						export_path = casc::ExportHelper::replaceExtension(export_path, ".obj");
						exporter.exportAsOBJ(export_path, &helper, &file_manifest);
						if (export_paths) export_paths->writeLine("WMO_OBJ:" + export_path);
					} else if (format == "STL") {
						export_path = casc::ExportHelper::replaceExtension(export_path, ".stl");
						exporter.exportAsSTL(export_path, &helper, &file_manifest);
						if (export_paths) export_paths->writeLine("WMO_STL:" + export_path);
					} else {
						exporter.exportRaw(export_path, &helper, &file_manifest);
						if (export_paths) export_paths->writeLine("WMO_RAW:" + export_path);
					}
				} else if (file_name_lower.ends_with(".m2")) {
					// JS: const exporter = new M2LegacyExporter(data, file_name, mpq);
					M2LegacyExporter exporter(std::move(data), file_name, mpq_archive);

					// JS: if (file_name === active_path) { ... }
					if (file_name == active_path) {
						const auto& skin_selection = view.legacyModelViewerSkinsSelection;
						if (!skin_selection.empty()) {
							const auto& selected_skin = skin_selection[0];
							std::string sel_id = selected_skin.value("id", std::string(""));
							auto it = active_skins.find(sel_id);
							if (it != active_skins.end() && !it->second.textures.empty())
								exporter.setSkinTextures(it->second.textures);
						}

						// exporter.setGeosetMask(view.modelViewerGeosets);
					}

					if (format == "OBJ") {
						export_path = casc::ExportHelper::replaceExtension(export_path, ".obj");
						exporter.exportAsOBJ(export_path, &helper, &file_manifest);
						if (export_paths) export_paths->writeLine("M2_OBJ:" + export_path);
					} else if (format == "STL") {
						export_path = casc::ExportHelper::replaceExtension(export_path, ".stl");
						exporter.exportAsSTL(export_path, &helper, &file_manifest);
						if (export_paths) export_paths->writeLine("M2_STL:" + export_path);
					} else {
						exporter.exportRaw(export_path, &helper, &file_manifest);
						if (export_paths) export_paths->writeLine("M2_RAW:" + export_path);
					}
				} else {
					// JS: // mdx or unknown - just export raw file
					// JS: await data.writeToFile(export_path);
					data.writeToFile(export_path);
					file_manifest.push_back({ {"type", "RAW"}, {"file", export_path} });
					if (export_paths) export_paths->writeLine("RAW:" + export_path);
				}
				*/

				// JS: helper.mark(file_name, true);
				helper.mark(file_name, true);
				manifest["succeeded"].push_back({ {"file", file_name}, {"files", file_manifest} });
			} catch (const std::exception& e) {
				// JS: helper.mark(file_name, false, e.message, e.stack);
				helper.mark(file_name, false, e.what());
				manifest["failed"].push_back({ {"file", file_name} });
			}
		}

		// JS: helper.finish();
		helper.finish();
	} else {
		// JS: core.setToast('error', 'Export format not yet implemented for legacy models: ' + format, null, -1);
		core::setToast("error", "Export format not yet implemented for legacy models: " + format, nullptr, -1);
	}

	// JS: export_paths?.close();
}

// JS: getActiveRenderer: () => active_renderer
M2LegacyRendererGL* getActiveRenderer() {
	return active_renderer_m2.get();
}

void render() {
	auto& view = *core::view;

	if (!is_initialized)
		return;

	// ─── Change-detection for watches ───────────────────────────────────────

	// Watch: legacyModelViewerAnimSelection → play animation
	// JS: this.$core.view.$watch('legacyModelViewerAnimSelection', async selected_animation_id => { ... });
	{
		std::string current_anim;
		if (view.legacyModelViewerAnimSelection.is_string())
			current_anim = view.legacyModelViewerAnimSelection.get<std::string>();

		if (current_anim != prev_anim_selection) {
			prev_anim_selection = current_anim;

			// JS: if (!active_renderer || !active_renderer.playAnimation || legacyModelViewerAnims.length === 0) return;
			bool has_play_animation = (active_renderer_m2 != nullptr || active_renderer_mdx != nullptr);
			if (has_play_animation && !view.legacyModelViewerAnims.empty()) {
				// JS: this.$core.view.legacyModelViewerAnimPaused = false;
				view.legacyModelViewerAnimPaused = false;
				// JS: this.$core.view.legacyModelViewerAnimFrame = 0;
				view.legacyModelViewerAnimFrame = 0;
				// JS: this.$core.view.legacyModelViewerAnimFrameCount = 0;
				view.legacyModelViewerAnimFrameCount = 0;

				// JS: if (selected_animation_id !== null && selected_animation_id !== undefined) { ... }
				if (!current_anim.empty()) {
					if (current_anim == "none") {
						// JS: active_renderer?.stopAnimation?.();
						if (active_renderer_m2)
							active_renderer_m2->stopAnimation();
						else if (active_renderer_mdx)
							active_renderer_mdx->stopAnimation();
					} else {
						// JS: const anim_info = legacyModelViewerAnims.find(anim => anim.id == selected_animation_id);
						int m2_index = -1;
						for (const auto& anim : view.legacyModelViewerAnims) {
							if (anim.value("id", std::string("")) == current_anim) {
								m2_index = anim.value("m2Index", -1);
								break;
							}
						}

						// JS: if (anim_info && anim_info.m2Index !== undefined && anim_info.m2Index >= 0) { ... }
						if (m2_index >= 0) {
							logging::write(std::format("Playing legacy animation at index {}", m2_index));
							if (active_renderer_m2) {
								active_renderer_m2->playAnimation(m2_index);
								// JS: this.$core.view.legacyModelViewerAnimFrameCount = active_renderer.get_animation_frame_count?.() || 0;
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
	// JS: this.$core.view.$watch('selectionLegacyModels', async selection => { ... });
	{
		if (view.selectionLegacyModels != prev_selection_legacy_models) {
			prev_selection_legacy_models = view.selectionLegacyModels;

			// JS: if (!this.$core.view.config.legacyModelsAutoPreview) return;
			if (view.config.value("legacyModelsAutoPreview", false)) {
				// JS: const first = selection[0];
				if (!view.selectionLegacyModels.empty()) {
					std::string first;
					if (view.selectionLegacyModels[0].is_string())
						first = view.selectionLegacyModels[0].get<std::string>();

					// JS: if (!this.$core.view.isBusy && first && active_path !== first)
					if (view.isBusy == 0 && !first.empty() && active_path != first)
						preview_model(first);
				}
			}
		}
	}

	// Watch: legacyModelViewerSkinsSelection → apply creature skin
	// JS: this.$core.view.$watch('legacyModelViewerSkinsSelection', async selection => { ... });
	{
		if (view.legacyModelViewerSkinsSelection != prev_skins_selection) {
			prev_skins_selection = view.legacyModelViewerSkinsSelection;

			// JS: if (!active_renderer || !active_renderer.applyCreatureSkin || active_skins.size === 0) return;
			if (active_renderer_m2 && !active_skins.empty()) {
				// JS: const selected = selection[0];
				if (!view.legacyModelViewerSkinsSelection.empty()) {
					const auto& selected = view.legacyModelViewerSkinsSelection[0];
					std::string sel_id = selected.value("id", std::string(""));

					// JS: const display = active_skins.get(selected.id);
					auto it = active_skins.find(sel_id);
					if (it != active_skins.end()) {
						// JS: log.write('Applying creature skin %s with textures: %o', selected.id, display.textures);
						logging::write(std::format("Applying creature skin {} with {} textures", sel_id, it->second.textures.size()));
						// JS: await active_renderer.applyCreatureSkin(display.textures);
						active_renderer_m2->applyCreatureSkin(it->second.textures);
					}
				}
			}
		}
	}

	// ─── Template rendering ─────────────────────────────────────────────────

	// JS: <div class="tab list-tab" id="tab-models-legacy">

	// --- Left panel: List container ---
	// JS: <div class="list-container">
	//     <Listbox v-model:selection="selectionLegacyModels" v-model:filter="userInputFilterLegacyModels"
	//         :items="listfileLegacyModels" :keyinput="true" :regex="config.regexFilters" ...
	//         @contextmenu="handle_listbox_context" />
	ImGui::BeginChild("legacy-models-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox component rendering will be wired when integration is complete.
	ImGui::Text("Legacy Models: %zu", view.listfileLegacyModels.size());

	// JS: <component :is="$components.ContextMenu" :node="$core.view.contextMenus.nodeListbox" ...>
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

			// JS: <span @click.self="copy_file_paths(...)">Copy file path(s)</span>
			if (ImGui::MenuItem(std::format("Copy file path{}", count > 1 ? "s" : "").c_str())) {
				copy_file_paths(sel_strings);
				view.contextMenus.nodeListbox = nullptr;
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

	// JS: <div class="filter"> <input type="text" v-model="userInputFilterLegacyModels" placeholder="Filter models..."/> </div>
	// TODO(conversion): Filter input will use ImGui::InputText when Listbox component is wired.

	ImGui::EndChild();

	ImGui::SameLine();

	// --- Middle panel: Preview container ---
	// JS: <div class="preview-container">
	ImGui::BeginChild("legacy-models-preview-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.65f, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// JS: <component :is="$components.ResizeLayer" id="texture-ribbon"
	//         v-if="config.modelViewerShowTextures && textureRibbonStack.length > 0">
	if (view.config.value("modelViewerShowTextures", true) && !view.textureRibbonStack.empty()) {
		// TODO(conversion): Texture ribbon rendering will be wired when component is integrated.
		ImGui::Text("Texture Ribbon: %zu textures", view.textureRibbonStack.size());
		// Note: Legacy texture ribbon does NOT have a context menu (unlike modern models tab).
	}

	// JS: <div class="preview-background" id="legacy-model-preview">
	// JS: <input v-if="config.modelViewerShowBackground" type="color" id="background-color-input" v-model="config.modelViewerBackgroundColor"/>
	if (view.config.value("modelViewerShowBackground", false)) {
		// TODO(conversion): Color picker for background will be wired when model viewer GL is integrated.
	}

	// JS: <component :is="$components.ModelViewerGL" v-if="legacyModelViewerContext" :context="legacyModelViewerContext" />
	// TODO(conversion): ModelViewerGL component rendering will be wired when GL integration is complete.

	// JS: <!-- legacy animation support disabled - needs fixing
	//     <div v-if="legacyModelViewerAnims && legacyModelViewerAnims.length > 0" class="preview-dropdown-overlay"> ... -->
	// NOTE: The original JS has this block COMMENTED OUT (disabled). We preserve it commented out.
	// The animation controls exist in the code but are not rendered in the template.
	/*
	if (!view.legacyModelViewerAnims.empty()) {
		// Animation dropdown and controls — disabled in original JS.
		// See the commented-out template block in the original source.
	}
	*/

	ImGui::EndChild();

	// --- Bottom: Export controls ---
	// JS: <div class="preview-controls">
	//     <MenuButton :options="menuButtonLegacyModels" :default="config.exportLegacyModelFormat"
	//         @change="config.exportLegacyModelFormat = $event" :disabled="isBusy" @click="export_model" />
	// TODO(conversion): MenuButton component rendering will be wired when integration is complete.
	if (ImGui::Button("Export Model##Legacy") && view.isBusy == 0)
		export_model_action();

	ImGui::SameLine();

	// --- Right panel: Sidebar ---
	// JS: <div id="model-sidebar" class="sidebar">
	ImGui::BeginChild("legacy-models-sidebar", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// JS: <span class="header">Preview</span>
	ImGui::SeparatorText("Preview");

	// JS: <input type="checkbox" v-model="config.legacyModelsAutoPreview"/> <span>Auto Preview</span>
	{
		bool auto_preview = view.config.value("legacyModelsAutoPreview", false);
		if (ImGui::Checkbox("Auto Preview##Legacy", &auto_preview))
			view.config["legacyModelsAutoPreview"] = auto_preview;
	}

	// JS: <input type="checkbox" v-model="legacyModelViewerAutoAdjust"/> <span>Auto Camera</span>
	ImGui::Checkbox("Auto Camera##Legacy", &view.legacyModelViewerAutoAdjust);

	// JS: <input type="checkbox" v-model="config.modelViewerShowGrid"/> <span>Show Grid</span>
	{
		bool show_grid = view.config.value("modelViewerShowGrid", true);
		if (ImGui::Checkbox("Show Grid##Legacy", &show_grid))
			view.config["modelViewerShowGrid"] = show_grid;
	}

	// JS: <input type="checkbox" v-model="config.modelViewerWireframe"/> <span>Show Wireframe</span>
	{
		bool wireframe = view.config.value("modelViewerWireframe", false);
		if (ImGui::Checkbox("Show Wireframe##Legacy", &wireframe))
			view.config["modelViewerWireframe"] = wireframe;
	}

	// JS: <input type="checkbox" v-model="config.modelViewerShowTextures"/> <span>Show Textures</span>
	{
		bool show_textures = view.config.value("modelViewerShowTextures", true);
		if (ImGui::Checkbox("Show Textures##Legacy", &show_textures))
			view.config["modelViewerShowTextures"] = show_textures;
	}

	// JS: <input type="checkbox" v-model="config.modelViewerShowBackground"/> <span>Show Background</span>
	{
		bool show_bg = view.config.value("modelViewerShowBackground", false);
		if (ImGui::Checkbox("Show Background##Legacy", &show_bg))
			view.config["modelViewerShowBackground"] = show_bg;
	}

	// JS: <template v-if="legacyModelViewerActiveType === 'm2' && legacyModelViewerSkins && legacyModelViewerSkins.length > 0">
	if (view.legacyModelViewerActiveType == "m2" && !view.legacyModelViewerSkins.empty()) {
		// JS: <span class="header">Skins</span>
		ImGui::SeparatorText("Skins");

		// JS: <component :is="$components.Listboxb" :items="legacyModelViewerSkins" v-model:selection="legacyModelViewerSkinsSelection" :single="true" />
		// TODO(conversion): Listboxb component rendering will be wired when integration is complete.
		for (const auto& skin : view.legacyModelViewerSkins) {
			std::string skin_id = skin.value("id", std::string(""));
			std::string skin_label = skin.value("label", std::string(""));
			bool is_selected = false;
			for (const auto& sel : view.legacyModelViewerSkinsSelection) {
				if (sel.value("id", std::string("")) == skin_id) {
					is_selected = true;
					break;
				}
			}

			if (ImGui::Selectable(skin_label.c_str(), is_selected)) {
				view.legacyModelViewerSkinsSelection = { skin };
			}
		}
	}

	// JS: <template v-if="legacyModelViewerActiveType === 'm2' || legacyModelViewerActiveType === 'mdx'">
	if (view.legacyModelViewerActiveType == "m2" || view.legacyModelViewerActiveType == "mdx") {
		// JS: <span class="header">Geosets</span>
		ImGui::SeparatorText("Geosets");

		// JS: <component :is="$components.Checkboxlist" :items="modelViewerGeosets" />
		// TODO(conversion): Checkboxlist component rendering will be wired when integration is complete.
		for (auto& geoset : view.modelViewerGeosets) {
			std::string label = geoset.value("label", std::string("Geoset"));
			bool checked = geoset.value("checked", true);
			if (ImGui::Checkbox(label.c_str(), &checked))
				geoset["checked"] = checked;
		}

		// JS: <a @click="setAllGeosets(true, modelViewerGeosets)">Enable All</a> / <a @click="setAllGeosets(false, modelViewerGeosets)">Disable All</a>
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

	// JS: <template v-if="legacyModelViewerActiveType === 'wmo'">
	if (view.legacyModelViewerActiveType == "wmo") {
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

	ImGui::EndChild();
}

} // namespace tab_models_legacy
