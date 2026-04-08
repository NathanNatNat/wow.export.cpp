/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#pragma once

#include <glad/gl.h>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "../loaders/WMOLoader.h"
#include "../Texture.h"
#include "../WMOShaderMapper.h"
#include "../gl/GLContext.h"
#include "../gl/GLTexture.h"
#include "../gl/VertexArray.h"
#include "../gl/ShaderProgram.h"

class BufferWrapper;
class M2RendererGL;
namespace casc { class CASC; }

/**
 * Draw call descriptor for a single WMO render batch.
 */
struct WMODrawCall {
	uint32_t start = 0;
	uint16_t count = 0;
	uint32_t blendMode = 0;
	uint32_t material_id = 0;
	wmo_shader_mapper::WMOShaderEntry shader = {
		wmo_shader_mapper::MapObjDiffuse_T1,
		wmo_shader_mapper::MapObjDiffuse
	};
};

/**
 * Rendered group state.
 */
struct WMOGroup {
	std::unique_ptr<gl::VertexArray> vao;
	std::vector<WMODrawCall> draw_calls;
	bool visible = true;
	uint32_t index = 0;
};

/**
 * A single doodad instance with transform.
 */
struct WMODoodadInstance {
	M2RendererGL* renderer = nullptr;
	std::array<float, 3> position = {0, 0, 0};
	std::array<float, 4> rotation = {0, 0, 0, 1};
	std::array<float, 3> scale = {1, 1, 1};
};

/**
 * A doodad set with its instances.
 */
struct WMODoodadSetData {
	std::vector<WMODoodadInstance> renderers;
	bool visible = true;
};

/**
 * Modern WMO renderer using OpenGL.
 *
 * JS equivalent: class WMORendererGL — module.exports = WMORendererGL
 */
class WMORendererGL {
public:
	/**
	 * @param data
	 * @param fileID File name or fileDataID
	 * @param gl_context
	 * @param useRibbon
	 */
	WMORendererGL(BufferWrapper& data, uint32_t fileID, gl::GLContext& gl_context, bool useRibbon = true);

	static gl::ShaderProgram* load_shaders(gl::GLContext& ctx);

	/**
	 * Set the active CASC source for texture and doodad loading.
	 * JS equivalent: accesses core.view.casc directly.
	 */
	void setCASCSource(casc::CASC* source) { casc_source_ = source; }

	void load();

	void _create_default_texture();
	void _load_textures();
	void _load_groups();
	void _setup_doodad_sets();

	/**
	 * Load a doodad set
	 * @param index
	 */
	void loadDoodadSet(uint32_t index);

	void updateGroups();
	void updateSets();

	void updateWireframe();

	/**
	 * Set model transformation
	 * @param position
	 * @param rotation
	 * @param scale
	 */
	void setTransform(const std::array<float, 3>& position, const std::array<float, 3>& rotation, const std::array<float, 3>& scale);
	void _update_model_matrix();

	/**
	 * Render the model
	 * @param view_matrix
	 * @param projection_matrix
	 */
	void render(const float* view_matrix, const float* projection_matrix);
	void updateAnimation(float delta_time);

	/**
	 * Get model bounding box (converted from WoW Z-up to OpenGL Y-up)
	 * @returns BoundingBoxResult or nullopt if not loaded
	 */
	struct BoundingBoxResult {
		std::array<float, 3> min;
		std::array<float, 3> max;
	};
	std::optional<BoundingBoxResult> getBoundingBox();

	void dispose();

	// Public state (matches JS properties)
	std::unique_ptr<WMOLoader> wmo;
	gl::ShaderProgram* shader = nullptr;
	int syncID = -1;

private:
	// resolve view array for WMO groups by wmoGroupKey
	std::vector<nlohmann::json>& get_wmo_groups_view();
	std::vector<nlohmann::json>& get_wmo_sets_view();

	BufferWrapper* data_ptr;
	uint32_t fileID;
	gl::GLContext& ctx;
	bool useRibbon;

	// CASC source for texture/doodad loading
	casc::CASC* casc_source_ = nullptr;

	// rendering state
	std::vector<WMOGroup> groups;
	std::map<uint32_t, std::unique_ptr<gl::GLTexture>> textures;
	std::unordered_map<uint32_t, std::vector<uint32_t>> materialTextures;
	std::unique_ptr<gl::GLTexture> default_texture;
	std::vector<GLuint> buffers;

	// doodad state
	std::vector<std::optional<WMODoodadSetData>> doodadSets;
	std::map<uint32_t, std::unique_ptr<M2RendererGL>> m2_renderers;
	// owned data buffers for doodad M2 files (kept alive while renderer exists)
	std::vector<std::unique_ptr<BufferWrapper>> m2_data_buffers_;

	// reactive state
	std::string wmoGroupKey = "modelViewerWMOGroups";
	std::string wmoSetKey = "modelViewerWMOSets";
	std::vector<nlohmann::json> groupArray;
	std::vector<nlohmann::json> setArray;

	// change detection for Vue watcher replacement
	std::vector<bool> prev_group_checked;
	std::vector<bool> prev_set_checked;

	// transforms
	std::array<float, 16> model_matrix;
	std::array<float, 3> position_ = {0, 0, 0};
	std::array<float, 3> rotation_ = {0, 0, 0};
	std::array<float, 3> scale_ = {1, 1, 1};
};
