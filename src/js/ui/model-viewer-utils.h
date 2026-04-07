/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "../buffer.h"

namespace casc {
class ExportHelper;
class CASC;
}

namespace gl {
struct GLContext;
}

class FileWriter;
class M2RendererGL;
class M3RendererGL;
class WMORendererGL;

/**
 * Model viewer shared utilities — shared across model viewer tab modules.
 *
 * JS equivalent: module.exports = { MODEL_TYPE_M2, MODEL_TYPE_M3, MODEL_TYPE_WMO,
 *     EXPORT_EXTENSIONS, detect_model_type, detect_model_type_by_name,
 *     get_model_extension, clear_texture_preview, initialize_uv_layers,
 *     toggle_uv_layer, preview_texture_by_id, create_renderer,
 *     extract_animations, handle_animation_change, export_preview,
 *     export_model, create_animation_methods, create_view_state }
 */
namespace model_viewer_utils {

/**
 * Model type enum.
 * JS equivalent: symbols MODEL_TYPE_M2, MODEL_TYPE_M3, MODEL_TYPE_WMO.
 */
enum class ModelType {
	Unknown = 0,
	M2,
	M3,
	WMO
};

/**
 * File extension map for export formats.
 * JS equivalent: EXPORT_EXTENSIONS = { 'OBJ': '.obj', 'STL': '.stl', ... }
 */
static const std::map<std::string, std::string> EXPORT_EXTENSIONS = {
	{"OBJ",  ".obj"},
	{"STL",  ".stl"},
	{"GLTF", ".gltf"},
	{"GLB",  ".glb"}
};

/**
 * View state proxy — maps generic names to prefixed AppState fields.
 * JS equivalent: object returned by create_view_state(core, prefix).
 * Holds raw pointers; valid only as long as core::view is live.
 */
struct ViewStateProxy {
	std::string*              texturePreviewURL    = nullptr;
	std::string*              texturePreviewUVOverlay = nullptr;
	int*                      texturePreviewWidth  = nullptr;
	int*                      texturePreviewHeight = nullptr;
	std::string*              texturePreviewName   = nullptr;
	std::vector<nlohmann::json>*  uvLayers         = nullptr;
	std::vector<nlohmann::json>*  anims            = nullptr;
	nlohmann::json*           animSelection        = nullptr;
	bool*                     animPaused           = nullptr;
	int*                      animFrame            = nullptr;
	int*                      animFrameCount       = nullptr;
	bool*                     autoAdjust           = nullptr;
};

/**
 * Result from create_renderer — owns the created renderer.
 * Exactly one of m2/m3/wmo is non-null.
 */
struct RendererResult {
	ModelType                        type = ModelType::Unknown;
	std::unique_ptr<M2RendererGL>    m2;
	std::unique_ptr<M3RendererGL>    m3;
	std::unique_ptr<WMORendererGL>   wmo;
};

/**
 * Options for export_model.
 * JS equivalent: the 'options' object argument to export_model().
 */
struct ExportModelOptions {
	BufferWrapper*               data           = nullptr;
	uint32_t                     file_data_id   = 0;
	std::string                  file_name;
	std::string                  format;
	std::string                  export_path;
	casc::ExportHelper*          helper         = nullptr;
	casc::CASC*                  casc           = nullptr;
	std::vector<nlohmann::json>* file_manifest  = nullptr;
	std::vector<nlohmann::json>  variant_textures;
	const std::vector<nlohmann::json>* geoset_mask    = nullptr;
	const std::vector<nlohmann::json>* wmo_group_mask = nullptr;
	const std::vector<nlohmann::json>* wmo_set_mask   = nullptr;
	FileWriter*                  export_paths   = nullptr;
};

/**
 * Animation control methods for a tab.
 * JS equivalent: object returned by create_animation_methods(get_renderer, get_state).
 */
class AnimationMethods {
public:
	/**
	 * Construct with factory functions for renderer and state.
	 * @param get_renderer Returns the active M2 renderer (or nullptr).
	 * @param get_state    Returns the active ViewStateProxy (or nullptr).
	 */
	AnimationMethods(
		std::function<M2RendererGL*()> get_renderer,
		std::function<ViewStateProxy*()> get_state
	);

	void toggle_animation_pause();
	void step_animation(int delta);
	void seek_animation(int frame);
	void start_scrub();
	void end_scrub();

private:
	std::function<M2RendererGL*()>   get_renderer_;
	std::function<ViewStateProxy*()> get_state_;
	bool _was_paused_before_scrub = false;
};

// ─── Functions ──────────────────────────────────────────────────────────────

/**
 * Detect model type from file data magic.
 * @param data Model file data (seek position is restored).
 */
ModelType detect_model_type(BufferWrapper& data);

/**
 * Detect model type from file name extension.
 * @returns ModelType or Unknown if unrecognized.
 */
ModelType detect_model_type_by_name(const std::string& file_name);

/**
 * Get canonical file extension for a model type.
 */
std::string get_model_extension(ModelType model_type);

/**
 * Clear texture preview state (URL, UV overlay, UV layers).
 */
void clear_texture_preview(ViewStateProxy& state);

/**
 * Initialize UV layers from active M2 renderer.
 * If renderer is nullptr, UV layers are cleared.
 */
void initialize_uv_layers(ViewStateProxy& state, M2RendererGL* renderer);

/**
 * Toggle UV layer visibility and update UV overlay.
 */
void toggle_uv_layer(ViewStateProxy& state, M2RendererGL* renderer, const std::string& layer_name);

/**
 * Preview a texture by file data ID.
 * @param state       View state proxy.
 * @param renderer    Active renderer (for UV layers; may be nullptr).
 * @param file_data_id Texture file data ID.
 * @param name        Display name.
 * @param casc        CASC source.
 */
void preview_texture_by_id(ViewStateProxy& state, M2RendererGL* renderer,
	uint32_t file_data_id, const std::string& name, casc::CASC* casc);

/**
 * Create appropriate renderer for model data.
 * @param data         Model file data (passed by reference; renderer takes a ref).
 * @param model_type   Model type.
 * @param ctx          GL context.
 * @param show_textures Whether to show textures (reactive/useRibbon flag).
 * @param file_data_id  File data ID (used for WMO renderer).
 */
RendererResult create_renderer(BufferWrapper& data, ModelType model_type,
	gl::GLContext& ctx, bool show_textures, uint32_t file_data_id = 0);

/**
 * Extract animation list from M2 renderer (using skelLoader or m2 animations).
 * @param renderer The M2 renderer.
 * @returns JSON array of { id, animationId, m2Index, label } entries,
 *          prepended with { id: "none", label: "No Animation", m2Index: -1 }.
 */
std::vector<nlohmann::json> extract_animations(const M2RendererGL& renderer);

/**
 * Handle animation selection change.
 * @param renderer             Active M2 renderer (may be nullptr).
 * @param state                View state with animation properties.
 * @param selected_animation_id Selected animation ID string (or empty/"none").
 */
void handle_animation_change(M2RendererGL* renderer, ViewStateProxy& state,
	const std::string& selected_animation_id);

/**
 * Export 3D preview as PNG (to disk) or to clipboard.
 * @param format       "PNG" or "CLIPBOARD".
 * @param ctx          GL context for framebuffer capture.
 * @param export_name  Base name for export file.
 * @param export_subdir Subdirectory prefix (empty = none).
 * @returns true on success.
 */
bool export_preview(const std::string& format, gl::GLContext& ctx,
	const std::string& export_name, const std::string& export_subdir = "");

/**
 * Export a model file.
 * @param options Export options.
 * @returns Relative export path (mark_file_name).
 */
std::string export_model(const ExportModelOptions& options);

/**
 * Create animation control methods for a tab.
 * @param get_renderer Factory that returns the active M2 renderer (or nullptr).
 * @param get_state    Factory that returns the active ViewStateProxy (or nullptr).
 */
AnimationMethods create_animation_methods(
	std::function<M2RendererGL*()> get_renderer,
	std::function<ViewStateProxy*()> get_state
);

/**
 * Create a view state proxy for a model viewer tab.
 * Binds proxy fields to the corresponding prefixed AppState properties.
 * @param prefix Property prefix: "model", "decor", "creature", etc.
 * @returns ViewStateProxy with raw pointers into core::view.
 */
ViewStateProxy create_view_state(const std::string& prefix);

} // namespace model_viewer_utils
