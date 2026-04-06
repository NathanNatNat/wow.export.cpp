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
#include <string>
#include <vector>

#include "../loaders/M2LegacyLoader.h"
#include "../Texture.h"
#include "../gl/GLContext.h"
#include "../gl/GLTexture.h"
#include "../gl/VertexArray.h"
#include "../gl/ShaderProgram.h"

// m2 version constants
static constexpr uint32_t M2_LEGACY_VER_WOTLK = 264;

// identity matrix
static constexpr std::array<float, 16> IDENTITY_MAT4 = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

// legacy shaders are simpler - map to basic shader ids
static constexpr int LEGACY_VERTEX_SHADER = 0; // Diffuse_T1
static constexpr int LEGACY_PIXEL_SHADER = 0;  // Combiners_Opaque

/**
 * Draw call descriptor for a single submesh.
 */
struct M2LegacyDrawCall {
	gl::VertexArray* vao = nullptr;
	uint32_t start = 0;
	uint16_t count = 0;
	std::array<int, 4> tex_indices = { -1, -1, -1, -1 };
	uint16_t texture_count = 1;
	int vertex_shader = LEGACY_VERTEX_SHADER;
	int pixel_shader = LEGACY_PIXEL_SHADER;
	int blend_mode = 0;
	uint16_t flags = 0;
	bool visible = true;
};

/**
 * Material properties for a texture.
 */
struct M2LegacyMaterialProps {
	int blendMode = 0;
	uint16_t flags = 0;
};

/**
 * Geoset entry for reactive UI.
 */
struct M2LegacyGeosetEntry {
	std::string label;
	bool checked = false;
	uint16_t id = 0;
};

/**
 * Legacy M2 renderer using OpenGL.
 *
 * JS equivalent: class M2LegacyRendererGL — module.exports = M2LegacyRendererGL
 */
class M2LegacyRendererGL {
public:
	M2LegacyRendererGL(BufferWrapper& data, gl::GLContext& gl_context, bool reactive = false, bool useRibbon = true);

	static gl::ShaderProgram* load_shaders(gl::GLContext& ctx);

	void load();

	void _create_default_texture();
	void _load_textures();

	/**
	 * Apply creature skin textures (replaceable textures from CreatureDisplayInfo)
	 * @param texture_paths - Array of texture file paths
	 */
	void applyCreatureSkin(const std::vector<std::string>& texture_paths);

	void loadSkin(int index);

	void _create_skeleton();

	void playAnimation(int index);
	void stopAnimation();
	void updateAnimation(float delta_time);

	float get_animation_duration();
	int get_animation_frame_count();
	int get_animation_frame();
	void set_animation_frame(int frame);
	void set_animation_paused(bool paused);
	void step_animation_frame(int delta);

	void _update_bone_matrices();

	// legacy single-timeline sampling: uses ranges array to find keyframes within animation bounds
	std::array<float, 3> _sample_legacy_vec3(const LegacyM2Track& track, float time_ms, uint32_t anim_start, uint32_t anim_end, const std::array<float, 3>& default_value = {0, 0, 0});

	std::array<float, 4> _sample_legacy_quat(const LegacyM2Track& track, float time_ms, uint32_t anim_start, uint32_t anim_end);

	// per-animation timeline sampling (wotlk)
	std::array<float, 3> _sample_vec3(const std::vector<LegacyTrackValue>& timestamps, const std::vector<LegacyTrackValue>& values, float time_ms, const std::array<float, 3>& default_value = {0, 0, 0});

	std::array<float, 4> _sample_quat(const std::vector<LegacyTrackValue>& timestamps, const std::vector<LegacyTrackValue>& values, float time_ms);

	void updateGeosets();

	void setTransform(const std::array<float, 3>& position, const std::array<float, 3>& rotation, const std::array<float, 3>& scale);
	void setTransformQuat(const std::array<float, 3>& position, const std::array<float, 4>& quat, const std::array<float, 3>& scale);

	void _update_model_matrix();

	void render(const float* view_matrix, const float* projection_matrix);

	struct BoundingBoxResult {
		std::array<float, 3> min;
		std::array<float, 3> max;
	};
	std::optional<BoundingBoxResult> getBoundingBox();

	void _dispose_skin();
	void dispose();

	// Public state (matches JS properties)
	std::unique_ptr<M2LegacyLoader> m2;
	gl::ShaderProgram* shader = nullptr;
	int syncID = -1;

private:
	BufferWrapper* data_ptr;
	gl::GLContext& ctx;
	bool reactive;
	bool useRibbon;

	// rendering state
	std::vector<std::unique_ptr<gl::VertexArray>> vaos;
	std::map<int, std::unique_ptr<gl::GLTexture>> textures;
	std::unique_ptr<gl::GLTexture> default_texture;
	std::vector<GLuint> buffers;
	std::vector<M2LegacyDrawCall> draw_calls;

	// animation state
	std::vector<LegacyM2Bone>* bones = nullptr;
	std::vector<float> bone_matrices;
	int current_animation = -1; // -1 = null
	float animation_time = 0;
	bool animation_paused = false;

	// reactive state
	std::string geosetKey = "modelViewerGeosets";
	std::vector<M2LegacyGeosetEntry> geosetArray;

	// transforms
	std::array<float, 16> model_matrix;
	std::array<float, 3> position = {0, 0, 0};
	std::array<float, 3> rotation = {0, 0, 0};
	std::array<float, 3> scale_val = {1, 1, 1};

	// material data
	std::map<int, M2LegacyMaterialProps> material_props;
};
