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

#include "../loaders/WMOLegacyLoader.h"
#include "../gl/GLContext.h"
#include "../gl/GLTexture.h"
#include "../gl/VertexArray.h"
#include "../gl/ShaderProgram.h"

class BufferWrapper;
class M2LegacyRendererGL;

/**
 * Draw call descriptor for a single WMOLegacy render batch.
 */
struct WMOLegacyDrawCall {
	uint32_t start = 0;
	uint16_t count = 0;
	uint32_t blendMode = 0;
	uint32_t material_id = 0;
	int vertex_shader = 0;
	int pixel_shader = 0;
};

/**
 * Rendered group state.
 */
struct WMOLegacyGroup {
	std::unique_ptr<gl::VertexArray> vao;
	std::vector<WMOLegacyDrawCall> draw_calls;
	bool visible = true;
	uint32_t index = 0;
};

/**
 * A single doodad instance with transform.
 */
struct WMOLegacyDoodadInstance {
	M2LegacyRendererGL* renderer = nullptr;
	std::array<float, 3> position = {0, 0, 0};
	std::array<float, 4> rotation = {0, 0, 0, 1};
	std::array<float, 3> scale = {1, 1, 1};
};

/**
 * A doodad set with its instances.
 */
struct WMOLegacyDoodadSet {
	std::vector<WMOLegacyDoodadInstance> renderers;
	bool visible = true;
};

/**
 * Legacy WMO renderer using OpenGL.
 *
 * JS equivalent: class WMOLegacyRendererGL — module.exports = WMOLegacyRendererGL
 */
class WMOLegacyRendererGL {
public:
	WMOLegacyRendererGL(BufferWrapper& data, uint32_t fileID, gl::GLContext& gl_context, bool useRibbon = true);

	static std::unique_ptr<gl::ShaderProgram> load_shaders(gl::GLContext& ctx);

	void load();

	void _create_default_texture();
	void _load_textures();
	void _load_groups();
	void _setup_doodad_sets();

	void loadDoodadSet(uint32_t index);

	void updateGroups();
	void updateSets();

	void setTransform(const std::array<float, 3>& position, const std::array<float, 3>& rotation, const std::array<float, 3>& scale);
	void _update_model_matrix();

	void render(const float* view_matrix, const float* projection_matrix);
	void updateAnimation(float delta_time);

	struct BoundingBoxResult {
		std::array<float, 3> min;
		std::array<float, 3> max;
	};
	std::optional<BoundingBoxResult> getBoundingBox();

	void dispose();

	const std::vector<WMOLegacyGroup>& get_groups() const { return groups; }

	// Public state (matches JS properties)
	std::unique_ptr<WMOLegacyLoader> wmo;
	std::unique_ptr<gl::ShaderProgram> shader;
	int syncID = -1;

private:
	BufferWrapper* data_ptr;
	uint32_t fileID;
	gl::GLContext& ctx;
	bool useRibbon;

	// rendering state
	std::vector<WMOLegacyGroup> groups;
	std::unordered_map<std::string, std::unique_ptr<gl::GLTexture>> textures;
	std::unordered_map<uint32_t, std::vector<std::string>> materialTextures;
	std::unique_ptr<gl::GLTexture> default_texture;
	std::vector<GLuint> buffers;

	// doodad state
	std::vector<std::optional<WMOLegacyDoodadSet>> doodadSets;
	std::unordered_map<std::string, std::unique_ptr<M2LegacyRendererGL>> m2_renderers;

	// reactive state (view arrays, synced on load)
	std::vector<nlohmann::json> groupArray;
	std::vector<nlohmann::json> setArray;

	std::vector<bool> prev_group_checked;
	std::vector<bool> prev_set_checked;

	// transforms
	std::array<float, 16> model_matrix;
	std::array<float, 3> position_ = {0, 0, 0};
	std::array<float, 3> rotation_ = {0, 0, 0};
	std::array<float, 3> scale_ = {1, 1, 1};
};
