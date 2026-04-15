/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "model-viewer-utils.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <vector>

#include <glad/gl.h>
#include <imgui.h>

#include "../core.h"
#include "../log.h"
#include "../buffer.h"
#include "../file-writer.h"
#include "../png-writer.h"
#include "../constants.h"
#include "../casc/blp.h"
#include "../casc/blte-reader.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../3D/AnimMapper.h"
#include "../3D/loaders/M2Loader.h"
#include "../3D/loaders/SKELLoader.h"
#include "../3D/renderers/M2RendererGL.h"
#include "../3D/renderers/M3RendererGL.h"
#include "../3D/renderers/WMORendererGL.h"
#include "../3D/exporters/M2Exporter.h"
#include "../3D/exporters/M3Exporter.h"
#include "../3D/exporters/WMOExporter.h"
#include "../3D/gl/GLContext.h"
#include "uv-drawer.h"

namespace model_viewer_utils {

namespace fs = std::filesystem;


AnimationMethods::AnimationMethods(
	std::function<M2RendererGL*()> get_renderer,
	std::function<ViewStateProxy*()> get_state)
	: get_renderer_(std::move(get_renderer))
	, get_state_(std::move(get_state))
{}

void AnimationMethods::toggle_animation_pause() {
	M2RendererGL* renderer = get_renderer_();
	ViewStateProxy* state = get_state_();
	if (!renderer || !state)
		return;

	const bool paused = !(*state->animPaused);
	*state->animPaused = paused;
	renderer->set_animation_paused(paused);
}

void AnimationMethods::step_animation(int delta) {
	ViewStateProxy* state = get_state_();
	if (!state || !(*state->animPaused))
		return;

	M2RendererGL* renderer = get_renderer_();
	if (!renderer)
		return;

	renderer->step_animation_frame(delta);
	*state->animFrame = renderer->get_animation_frame();
}

void AnimationMethods::seek_animation(int frame) {
	M2RendererGL* renderer = get_renderer_();
	ViewStateProxy* state = get_state_();
	if (!renderer || !state)
		return;

	renderer->set_animation_frame(frame);
	*state->animFrame = frame;
}

void AnimationMethods::start_scrub() {
	ViewStateProxy* state = get_state_();
	if (!state)
		return;
	_was_paused_before_scrub = *state->animPaused;
	if (!_was_paused_before_scrub) {
		*state->animPaused = true;
		M2RendererGL* renderer = get_renderer_();
		if (renderer)
			renderer->set_animation_paused(true);
	}
}

void AnimationMethods::end_scrub() {
	ViewStateProxy* state = get_state_();
	if (!state)
		return;
	if (!_was_paused_before_scrub) {
		*state->animPaused = false;
		M2RendererGL* renderer = get_renderer_();
		if (renderer)
			renderer->set_animation_paused(false);
	}
}


/**
 * Detect model type from file data magic.
 * @param data Model file data (seek position is restored).
 */
ModelType detect_model_type(BufferWrapper& data) {
	const uint32_t magic = data.readUInt32LE();
	data.seek(0);

	if (magic == constants::MAGIC::MD20 || magic == constants::MAGIC::MD21)
		return ModelType::M2;

	if (magic == constants::MAGIC::M3DT)
		return ModelType::M3;

	return ModelType::WMO;
}

/**
 * Detect model type from file name extension.
 */
ModelType detect_model_type_by_name(const std::string& file_name) {
	const std::string lower = [&] {
		std::string s = file_name;
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	}();

	if (lower.size() >= 3 && lower.substr(lower.size() - 3) == ".m2")
		return ModelType::M2;

	if (lower.size() >= 3 && lower.substr(lower.size() - 3) == ".m3")
		return ModelType::M3;

	if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".wmo")
		return ModelType::WMO;

	return ModelType::Unknown;
}

/**
 * Get canonical file extension for a model type.
 */
std::string get_model_extension(ModelType model_type) {
	if (model_type == ModelType::M2)
		return ".m2";
	if (model_type == ModelType::M3)
		return ".m3";
	return ".wmo";
}

/**
 * Upload RGBA pixel data to an OpenGL texture, returning the texture ID.
 * Deletes the previous texture if old_tex != 0.
 */
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

/**
 * Delete an OpenGL texture and reset the ID to 0.
 */
static void delete_gl_texture(uint32_t& tex_id) {
	if (tex_id != 0) {
		GLuint gl_tex = static_cast<GLuint>(tex_id);
		glDeleteTextures(1, &gl_tex);
		tex_id = 0;
	}
}

/**
 * Clear texture preview state (URL, UV overlay, UV layers).
 */
void clear_texture_preview(ViewStateProxy& state) {
	if (state.texturePreviewURL)       *state.texturePreviewURL       = "";
	if (state.texturePreviewUVOverlay) *state.texturePreviewUVOverlay = "";
	if (state.uvLayers)                *state.uvLayers                = {};
	if (state.texturePreviewTexID)     delete_gl_texture(*state.texturePreviewTexID);
	if (state.texturePreviewUVTexID)   delete_gl_texture(*state.texturePreviewUVTexID);
}

/**
 * Initialize UV layers from active M2 renderer.
 * If renderer is nullptr, UV layers are cleared.
 */
void initialize_uv_layers(ViewStateProxy& state, M2RendererGL* renderer) {
	if (!renderer || !state.uvLayers) {
		if (state.uvLayers)
			*state.uvLayers = {};
		return;
	}

	const auto uv_layer_data = renderer->getUVLayers();
	std::vector<nlohmann::json> layers;
	layers.push_back({{"name", "UV Off"}, {"data", nullptr}, {"active", true}});
	for (const auto& layer : uv_layer_data.layers) {
		layers.push_back({{"name", layer.name}, {"data", layer.data}, {"active", false}});
	}
	*state.uvLayers = std::move(layers);
}

/**
 * Toggle UV layer visibility and update UV overlay.
 */
void toggle_uv_layer(ViewStateProxy& state, M2RendererGL* renderer, const std::string& layer_name) {
	if (!state.uvLayers)
		return;

	auto& uv_layers = *state.uvLayers;

	// Find the requested layer
	auto it = std::find_if(uv_layers.begin(), uv_layers.end(),
		[&](const nlohmann::json& l) { return l.value("name", "") == layer_name; });
	if (it == uv_layers.end())
		return;

	// Set all layers inactive, then activate the selected one
	for (auto& l : uv_layers)
		l["active"] = false;
	(*it)["active"] = true;

	if (layer_name == "UV Off" || (*it).value("data", nlohmann::json(nullptr)).is_null()) {
		if (state.texturePreviewUVOverlay)
			*state.texturePreviewUVOverlay = "";
		if (state.texturePreviewUVTexID)
			delete_gl_texture(*state.texturePreviewUVTexID);
	} else if (renderer && state.texturePreviewUVOverlay) {
		const auto uv_layer_data = renderer->getUVLayers();
		if (!uv_layer_data.indices) {
			*state.texturePreviewUVOverlay = "";
			if (state.texturePreviewUVTexID)
				delete_gl_texture(*state.texturePreviewUVTexID);
			return;
		}

		const auto& layer_data_json = (*it)["data"];
		const std::vector<float> layer_data = layer_data_json.get<std::vector<float>>();

		const int tw = state.texturePreviewWidth  ? *state.texturePreviewWidth  : 256;
		const int th = state.texturePreviewHeight ? *state.texturePreviewHeight : 256;

		const std::vector<uint8_t> pixels = uv_drawer::generateUVLayerPixels(
			layer_data, tw, th, *uv_layer_data.indices);

		// Upload UV overlay pixels to GL texture for ImGui::Image display.
		if (state.texturePreviewUVTexID) {
			*state.texturePreviewUVTexID = upload_rgba_to_gl(
				pixels.data(), tw, th, *state.texturePreviewUVTexID);
		}

		// Encode as PNG data URL for UI display
		PNGWriter png_writer(static_cast<uint32_t>(tw), static_cast<uint32_t>(th));
		png_writer.getPixelData() = pixels;
		BufferWrapper buf = png_writer.getBuffer();
		*state.texturePreviewUVOverlay = "data:image/png;base64," + buf.toBase64();
	}
}

/**
 * Preview a texture by file data ID.
 */
void preview_texture_by_id(ViewStateProxy& state, M2RendererGL* renderer,
	uint32_t file_data_id, const std::string& name, casc::CASC* casc)
{
	std::string texture = casc::listfile::getByID(file_data_id);
	if (texture.empty())
		texture = casc::listfile::formatUnknownFile(file_data_id);

	auto _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", texture), {}, -1, false);
	logging::write(std::format("Previewing texture file {}", texture));

	try {
		BufferWrapper file = casc->getVirtualFileByID(file_data_id);
		casc::BLPImage blp(std::move(file));

		const uint8_t mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
		if (state.texturePreviewURL)       *state.texturePreviewURL    = blp.getDataURL(mask);
		if (state.texturePreviewWidth)     *state.texturePreviewWidth  = static_cast<int>(blp.width);
		if (state.texturePreviewHeight)    *state.texturePreviewHeight = static_cast<int>(blp.height);
		if (state.texturePreviewName)      *state.texturePreviewName   = name;

		// Upload BLP as GL texture for ImGui::Image display.
		if (state.texturePreviewTexID) {
			std::vector<uint8_t> pixels = blp.toUInt8Array(0, mask);
			*state.texturePreviewTexID = upload_rgba_to_gl(
				pixels.data(), static_cast<int>(blp.width), static_cast<int>(blp.height),
				*state.texturePreviewTexID);
		}

		initialize_uv_layers(state, renderer);
		core::hideToast();
	} catch (const casc::EncryptionError& e) {
		core::setToast("error",
			std::format("The texture {} is encrypted with an unknown key ({}).", texture, e.key),
			{}, -1);
		logging::write(std::format("Failed to decrypt texture {} ({})", texture, e.key));
	} catch (const std::exception& e) {
		core::setToast("error",
			"Unable to preview texture " + texture,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}
}

/**
 * Create appropriate renderer for model data.
 */
RendererResult create_renderer(BufferWrapper& data, ModelType model_type,
	gl::GLContext& ctx, bool show_textures, uint32_t file_data_id,
	const std::string& file_name)
{
	RendererResult result;
	result.type = model_type;

	if (model_type == ModelType::M2) {
		result.m2 = std::make_unique<M2RendererGL>(data, ctx, true, show_textures);
	} else if (model_type == ModelType::M3) {
		result.m3 = std::make_unique<M3RendererGL>(data, ctx, true, show_textures);
	} else {
		// JS: new WMORendererGL(data, file_name, gl_context, show_textures)
		// C++ WMORendererGL constructor takes uint32_t fileID — using file_data_id.
		// The file_name parameter is accepted for API parity with JS but not used here
		// because the C++ WMORendererGL is designed around numeric file data IDs for CASC lookup.
		(void)file_name;
		result.wmo = std::make_unique<WMORendererGL>(data, file_data_id, ctx, show_textures);
	}

	return result;
}

/**
 * Extract animation list from M2 renderer (using skelLoader or m2 animations).
 * JS uses Math.floor(animation.id) — this is a no-op because animation.id is uint16_t
 * in C++ (always integral). std::to_string(uint16_t) produces identical output.
 */
std::vector<nlohmann::json> extract_animations(const M2RendererGL& renderer) {
	std::vector<nlohmann::json> anim_list;
	anim_list.push_back({{"id", "none"}, {"label", "No Animation"}, {"m2Index", -1}});

	// Use skelLoader animations if available, otherwise fall back to m2 animations
	const SKELLoader* skel = renderer.getSkelLoader();
	if (skel) {
		for (size_t i = 0; i < skel->animations.size(); i++) {
			const auto& animation = skel->animations[i];
			const std::string id_str = std::to_string(animation.id) + "." + std::to_string(animation.variationIndex);
			const std::string label = get_anim_name(static_cast<int>(animation.id))
				+ " (" + std::to_string(animation.id) + "." + std::to_string(animation.variationIndex) + ")";
			anim_list.push_back({
				{"id", id_str},
				{"animationId", animation.id},
				{"m2Index", static_cast<int>(i)},
				{"label", label}
			});
		}
	} else if (renderer.m2) {
		for (size_t i = 0; i < renderer.m2->animations.size(); i++) {
			const auto& animation = renderer.m2->animations[i];
			const std::string id_str = std::to_string(animation.id) + "." + std::to_string(animation.variationIndex);
			const std::string label = get_anim_name(static_cast<int>(animation.id))
				+ " (" + std::to_string(animation.id) + "." + std::to_string(animation.variationIndex) + ")";
			anim_list.push_back({
				{"id", id_str},
				{"animationId", animation.id},
				{"m2Index", static_cast<int>(i)},
				{"label", label}
			});
		}
	}

	return anim_list;
}

/**
 * Handle animation selection change.
 */
void handle_animation_change(M2RendererGL* renderer, ViewStateProxy& state,
	const std::string& selected_animation_id)
{
	// JS: if (!renderer || !renderer.playAnimation) — checks method existence.
	// C++: M2RendererGL always has playAnimation, so null check suffices.
	// Non-M2 renderers (WMO, M3) would be passed as nullptr.
	if (!renderer)
		return;

	// reset animation state
	if (state.animPaused)    *state.animPaused    = false;
	if (state.animFrame)     *state.animFrame     = 0;
	if (state.animFrameCount) *state.animFrameCount = 0;

	// JS: if (selected_animation_id === null || selected_animation_id === undefined) return;
	// C++: empty string is the equivalent of JS null/undefined for std::string parameters.
	// In JS, empty string "" would pass through — but callers never pass "" in practice;
	// they pass null/undefined (no selection) or a valid ID string.
	if (selected_animation_id.empty())
		return;

	if (selected_animation_id == "none") {
		renderer->stopAnimation();
		return;
	}

	if (!state.anims)
		return;

	const auto& anims = *state.anims;
	const auto it = std::find_if(anims.begin(), anims.end(),
		[&](const nlohmann::json& anim) {
			return anim.value("id", "") == selected_animation_id;
		});

	if (it != anims.end()) {
		const int m2_index = (*it).value("m2Index", -1);
		if (m2_index >= 0) {
			logging::write(std::format("Playing animation {} at M2 index {}", selected_animation_id, m2_index));
			renderer->playAnimation(m2_index);

			if (state.animFrameCount)
				*state.animFrameCount = renderer->get_animation_frame_count();
		}
	}
}

/**
 * Export 3D preview as PNG (to disk) or to clipboard.
 */
bool export_preview(const std::string& format, gl::GLContext& ctx,
	const std::string& export_name, const std::string& export_subdir)
{
	core::setToast("progress", "Saving preview, hold on...", {}, -1, false);

	// Capture current OpenGL framebuffer
	const int width  = ctx.viewport_width;
	const int height = ctx.viewport_height;

	PNGWriter png_writer(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	auto& pixels = png_writer.getPixelData();
	pixels.resize(static_cast<size_t>(width * height * 4));
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	// Flip Y-axis (OpenGL bottom-left → image top-left)
	const int row_stride = width * 4;
	for (int y = 0; y < height / 2; y++) {
		uint8_t* top    = pixels.data() + y * row_stride;
		uint8_t* bottom = pixels.data() + (height - 1 - y) * row_stride;
		std::swap_ranges(top, top + row_stride, bottom);
	}

	BufferWrapper buf = png_writer.getBuffer();

	if (format == "PNG") {
		// JS: const export_paths = core.openLastExportStream(); with optional chaining (?.)
		// C++: FileWriter is a value type — always valid. core::openLastExportStream() creates
		// the file; if it fails, the stream methods are no-ops (ofstream in failed state).
		FileWriter export_paths = core::openLastExportStream();

		const std::string base_path = export_subdir.empty()
			? export_name
			: (export_subdir + "/" + export_name);
		const std::string export_path = casc::ExportHelper::getExportPath(base_path);
		std::string out_file = casc::ExportHelper::replaceExtension(export_path, ".png");

		if (core::view->config.value("modelsExportPngIncrements", false))
			out_file = casc::ExportHelper::getIncrementalFilename(out_file);

		const std::string out_dir = fs::path(out_file).parent_path().string();

		buf.writeToFile(fs::path(out_file));
		export_paths.writeLine("PNG:" + out_file);
		export_paths.close();

		logging::write(std::format("Saved 3D preview screenshot to {}", out_file));
		core::setToast("success",
			std::format("Successfully exported preview to {}", out_file),
			{ {"View in Explorer", [out_dir]() { core::openInExplorer(out_dir); }} }, -1);
	} else if (format == "CLIPBOARD") {
		// JS: clipboard.set(buf.toBase64(), 'png', true) — copies actual PNG image data.
		// C++ limitation: ImGui only supports text clipboard. Platform-specific PNG clipboard
		// APIs (e.g., Win32 CF_DIB, X11 image/png) would be needed for true image copy.
		// For now, copies base64-encoded PNG text as a fallback.
		ImGui::SetClipboardText(buf.toBase64().c_str());

		logging::write(std::format("Copied 3D preview to clipboard ({})", export_name));
		core::setToast("success", "3D preview has been copied to the clipboard", {}, -1, true);
	}

	return true;
}

/**
 * Export a model file.
 */
std::string export_model(const ExportModelOptions& options) {
	BufferWrapper& data      = *options.data;
	const uint32_t file_data_id = options.file_data_id;
	const std::string& file_name  = options.file_name;
	const std::string& format     = options.format;
	casc::ExportHelper* helper    = options.helper;
	casc::CASC*         casc      = options.casc;
	FileWriter*         ep        = options.export_paths;

	// Determine model type
	ModelType model_type = detect_model_type_by_name(file_name);
	if (model_type == ModelType::Unknown)
		model_type = detect_model_type(data);

	std::string final_export_path = options.export_path;
	std::string mark_file_name = casc::ExportHelper::getRelativeExport(final_export_path);

	// Helper: convert JSON geoset_mask to M2ExportGeosetMask vector
	auto make_m2_geoset_mask = [&]() -> std::vector<M2ExportGeosetMask> {
		std::vector<M2ExportGeosetMask> mask;
		if (options.geoset_mask) {
			for (const auto& entry : *options.geoset_mask) {
				M2ExportGeosetMask m;
				m.checked = entry.value("checked", false);
				mask.push_back(m);
			}
		}
		return mask;
	};

	// Helper: convert JSON wmo_group_mask to WMOExportGroupMask vector
	auto make_wmo_group_mask = [&]() -> std::vector<WMOExportGroupMask> {
		std::vector<WMOExportGroupMask> mask;
		if (options.wmo_group_mask) {
			for (size_t i = 0; i < options.wmo_group_mask->size(); i++) {
				const auto& entry = (*options.wmo_group_mask)[i];
				WMOExportGroupMask m;
				m.checked    = entry.value("checked", false);
				m.groupIndex = static_cast<uint32_t>(i);
				mask.push_back(m);
			}
		}
		return mask;
	};

	// Helper: convert JSON wmo_set_mask to WMOExportDoodadSetMask vector
	auto make_wmo_set_mask = [&]() -> std::vector<WMOExportDoodadSetMask> {
		std::vector<WMOExportDoodadSetMask> mask;
		if (options.wmo_set_mask) {
			for (const auto& entry : *options.wmo_set_mask) {
				WMOExportDoodadSetMask m;
				m.checked = entry.value("checked", false);
				mask.push_back(m);
			}
		}
		return mask;
	};

	// Helper: convert JSON variant_textures to vector<uint32_t>
	auto make_variant_textures = [&]() -> std::vector<uint32_t> {
		std::vector<uint32_t> vt;
		for (const auto& v : options.variant_textures) {
			if (v.is_number())
				vt.push_back(v.get<uint32_t>());
		}
		return vt;
	};

	// Helper: append manifest entries to generic file_manifest
	auto append_m2_manifest = [&](const std::vector<M2ExportFileManifest>& typed) {
		if (options.file_manifest) {
			for (const auto& entry : typed) {
				options.file_manifest->push_back({
					{"type", entry.type},
					{"fileDataID", entry.fileDataID},
					{"file", entry.file.string()}
				});
			}
		}
	};

	auto append_m3_manifest = [&](const std::vector<M3ExportFileManifest>& typed) {
		if (options.file_manifest) {
			for (const auto& entry : typed) {
				options.file_manifest->push_back({
					{"type", entry.type},
					{"fileDataID", entry.fileDataID},
					{"file", entry.file.string()}
				});
			}
		}
	};

	auto append_wmo_manifest = [&](const std::vector<WMOExportFileManifest>& typed) {
		if (options.file_manifest) {
			for (const auto& entry : typed) {
				options.file_manifest->push_back({
					{"type", entry.type},
					{"fileDataID", entry.fileDataID},
					{"file", entry.file.string()}
				});
			}
		}
	};

	const bool exportCollision = core::view->config.value("modelsExportCollision", false);
	const bool splitWMOGroups  = core::view->config.value("modelsExportSplitWMOGroups", false);

	if (format == "RAW") {
		if (ep) ep->writeLine(final_export_path);

		if (model_type == ModelType::M2) {
			std::vector<M2ExportFileManifest> typed_manifest;
			M2Exporter exporter(data, make_variant_textures(), file_data_id, casc);
			exporter.exportRaw(fs::path(final_export_path), helper, &typed_manifest);
			append_m2_manifest(typed_manifest);
		} else if (model_type == ModelType::M3) {
			std::vector<M3ExportFileManifest> typed_manifest;
			M3Exporter exporter(data, make_variant_textures(), file_data_id);
			exporter.exportRaw(fs::path(final_export_path), helper, &typed_manifest);
			append_m3_manifest(typed_manifest);
		} else {
			std::vector<WMOExportFileManifest> typed_manifest;
			WMOExporter exporter(data, file_data_id, casc);
			exporter.exportRaw(fs::path(final_export_path), helper, &typed_manifest);
			append_wmo_manifest(typed_manifest);
			WMOExporter::clearCache();
		}
	} else if (format == "OBJ" || format == "STL" || format == "GLTF" || format == "GLB") {
		const auto ext_it = EXPORT_EXTENSIONS.find(format);
		if (ext_it != EXPORT_EXTENSIONS.end()) {
			final_export_path = casc::ExportHelper::replaceExtension(final_export_path, ext_it->second);
			mark_file_name    = casc::ExportHelper::getRelativeExport(final_export_path);
		}

		if (model_type == ModelType::M2) {
			M2Exporter exporter(data, make_variant_textures(), file_data_id, casc);

			if (options.geoset_mask)
				exporter.setGeosetMask(make_m2_geoset_mask());

			if (format == "OBJ") {
				std::vector<M2ExportFileManifest> typed_manifest;
				exporter.exportAsOBJ(fs::path(final_export_path), exportCollision, helper, &typed_manifest);
				if (ep) ep->writeLine("M2_OBJ:" + final_export_path);
				append_m2_manifest(typed_manifest);
			} else if (format == "STL") {
				std::vector<M2ExportFileManifest> typed_manifest;
				exporter.exportAsSTL(fs::path(final_export_path), exportCollision, helper, &typed_manifest);
				if (ep) ep->writeLine("M2_STL:" + final_export_path);
				append_m2_manifest(typed_manifest);
			} else {
				// GLTF or GLB
				exporter.exportAsGLTF(fs::path(final_export_path), helper, [&] {
					std::string f = format; std::transform(f.begin(), f.end(), f.begin(), ::tolower); return f;
				}());
				if (ep) ep->writeLine("M2_" + format + ":" + final_export_path);
			}
		} else if (model_type == ModelType::M3) {
			M3Exporter exporter(data, make_variant_textures(), file_data_id);

			if (format == "OBJ") {
				std::vector<M3ExportFileManifest> typed_manifest;
				exporter.exportAsOBJ(fs::path(final_export_path), exportCollision, helper, &typed_manifest);
				if (ep) ep->writeLine("M3_OBJ:" + final_export_path);
				append_m3_manifest(typed_manifest);
			} else if (format == "STL") {
				std::vector<M3ExportFileManifest> typed_manifest;
				exporter.exportAsSTL(fs::path(final_export_path), exportCollision, helper, &typed_manifest);
				if (ep) ep->writeLine("M3_STL:" + final_export_path);
				append_m3_manifest(typed_manifest);
			} else {
				exporter.exportAsGLTF(fs::path(final_export_path), helper, [&] {
					std::string f = format; std::transform(f.begin(), f.end(), f.begin(), ::tolower); return f;
				}());
				if (ep) ep->writeLine("M3_" + format + ":" + final_export_path);
			}
		} else {
			// WMO
			// JS: new WMOExporter(data, file_name) — passes file_name string for non-RAW.
			// C++ WMOExporter constructor takes (data, uint32_t fileDataID, CASC*) — using
			// file_data_id because the C++ WMOExporter is designed around numeric IDs for
			// CASC-based group/doodad file lookup.
			WMOExporter exporter(data, file_data_id, casc);

			if (options.wmo_group_mask)
				exporter.setGroupMask(make_wmo_group_mask());

			if (options.wmo_set_mask)
				exporter.setDoodadSetMask(make_wmo_set_mask());

			if (format == "OBJ") {
				std::vector<WMOExportFileManifest> typed_manifest;
				exporter.exportAsOBJ(fs::path(final_export_path), helper, &typed_manifest, splitWMOGroups);
				if (ep) ep->writeLine("WMO_OBJ:" + final_export_path);
				append_wmo_manifest(typed_manifest);
			} else if (format == "STL") {
				std::vector<WMOExportFileManifest> typed_manifest;
				exporter.exportAsSTL(fs::path(final_export_path), helper, &typed_manifest);
				if (ep) ep->writeLine("WMO_STL:" + final_export_path);
				append_wmo_manifest(typed_manifest);
			} else {
				exporter.exportAsGLTF(fs::path(final_export_path), helper, [&] {
					std::string f = format; std::transform(f.begin(), f.end(), f.begin(), ::tolower); return f;
				}());
				if (ep) ep->writeLine("WMO_" + format + ":" + final_export_path);
			}

			WMOExporter::clearCache();
		}
	} else {
		throw std::runtime_error("Unexpected model export format: " + format);
	}

	return mark_file_name;
}

/**
 * Create animation control methods for a tab.
 */
AnimationMethods create_animation_methods(
	std::function<M2RendererGL*()> get_renderer,
	std::function<ViewStateProxy*()> get_state)
{
	return AnimationMethods(std::move(get_renderer), std::move(get_state));
}

/**
 * Create a view state proxy for a model viewer tab.
 * JS uses dynamic property access (core.view[prefix + 'TexturePreviewURL']) which works for
 * any prefix. C++ uses explicit field mapping. Only "model", "decor", and "creature" prefixes
 * are supported because AppState only defines fields for these three tab types. This matches
 * all actual callers — no other prefixes are used at runtime.
 */
ViewStateProxy create_view_state(const std::string& prefix) {
	ViewStateProxy proxy;
	AppState* s = core::view;

	if (prefix == "model") {
		proxy.texturePreviewURL      = &s->modelTexturePreviewURL;
		proxy.texturePreviewUVOverlay = &s->modelTexturePreviewUVOverlay;
		proxy.texturePreviewWidth    = &s->modelTexturePreviewWidth;
		proxy.texturePreviewHeight   = &s->modelTexturePreviewHeight;
		proxy.texturePreviewName     = &s->modelTexturePreviewName;
		proxy.texturePreviewTexID    = &s->modelTexturePreviewTexID;
		proxy.texturePreviewUVTexID  = &s->modelTexturePreviewUVTexID;
		proxy.uvLayers               = &s->modelViewerUVLayers;
		proxy.anims                  = &s->modelViewerAnims;
		proxy.animSelection          = &s->modelViewerAnimSelection;
		proxy.animPaused             = &s->modelViewerAnimPaused;
		proxy.animFrame              = &s->modelViewerAnimFrame;
		proxy.animFrameCount         = &s->modelViewerAnimFrameCount;
		proxy.autoAdjust             = &s->modelViewerAutoAdjust;
	} else if (prefix == "decor") {
		proxy.texturePreviewURL      = &s->decorTexturePreviewURL;
		proxy.texturePreviewUVOverlay = &s->decorTexturePreviewUVOverlay;
		proxy.texturePreviewWidth    = &s->decorTexturePreviewWidth;
		proxy.texturePreviewHeight   = &s->decorTexturePreviewHeight;
		proxy.texturePreviewName     = &s->decorTexturePreviewName;
		proxy.texturePreviewTexID    = &s->decorTexturePreviewTexID;
		proxy.texturePreviewUVTexID  = &s->decorTexturePreviewUVTexID;
		proxy.uvLayers               = &s->decorViewerUVLayers;
		proxy.anims                  = &s->decorViewerAnims;
		proxy.animSelection          = &s->decorViewerAnimSelection;
		proxy.animPaused             = &s->decorViewerAnimPaused;
		proxy.animFrame              = &s->decorViewerAnimFrame;
		proxy.animFrameCount         = &s->decorViewerAnimFrameCount;
		proxy.autoAdjust             = &s->decorViewerAutoAdjust;
	} else if (prefix == "creature") {
		proxy.texturePreviewURL      = &s->creatureTexturePreviewURL;
		proxy.texturePreviewUVOverlay = &s->creatureTexturePreviewUVOverlay;
		proxy.texturePreviewWidth    = &s->creatureTexturePreviewWidth;
		proxy.texturePreviewHeight   = &s->creatureTexturePreviewHeight;
		proxy.texturePreviewName     = &s->creatureTexturePreviewName;
		proxy.texturePreviewTexID    = &s->creatureTexturePreviewTexID;
		proxy.texturePreviewUVTexID  = &s->creatureTexturePreviewUVTexID;
		proxy.uvLayers               = &s->creatureViewerUVLayers;
		proxy.anims                  = &s->creatureViewerAnims;
		proxy.animSelection          = &s->creatureViewerAnimSelection;
		proxy.animPaused             = &s->creatureViewerAnimPaused;
		proxy.animFrame              = &s->creatureViewerAnimFrame;
		proxy.animFrameCount         = &s->creatureViewerAnimFrameCount;
		proxy.autoAdjust             = &s->creatureViewerAutoAdjust;
	}

	return proxy;
}

} // namespace model_viewer_utils
