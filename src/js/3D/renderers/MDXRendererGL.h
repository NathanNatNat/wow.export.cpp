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
#include <set>
#include <string>
#include <vector>

#include "../loaders/MDXLoader.h"
#include "../gl/GLContext.h"
#include "../gl/GLTexture.h"
#include "../gl/VertexArray.h"
#include "../gl/ShaderProgram.h"

class BufferWrapper;

// identity matrix
static constexpr std::array<float, 16> MDX_IDENTITY_MAT4 = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

// interpolation types
static constexpr int INTERP_NONE = 0;
static constexpr int INTERP_LINEAR = 1;
static constexpr int INTERP_HERMITE = 2;
static constexpr int INTERP_BEZIER = 3;

/**
 * Draw call descriptor for a single MDX geoset submesh.
 */
struct MDXDrawCall {
	gl::VertexArray* vao = nullptr;
	uint32_t start = 0;
	uint32_t count = 0;
	int textureId = -1;  // -1 = null (no texture)
	int blendMode = 0;
	bool twoSided = false;
	bool visible = true;
};

/**
 * Geoset entry for reactive UI.
 */
struct MDXGeosetEntry {
	std::string label;
	bool checked = false;
	int id = 0;
};

/**
 * MDX renderer using OpenGL.
 *
 * JS equivalent: class MDXRendererGL — module.exports = MDXRendererGL
 */
class MDXRendererGL {
public:
	MDXRendererGL(BufferWrapper& data, gl::GLContext& gl_context, bool reactive = false, bool useRibbon = true);

	static gl::ShaderProgram* load_shaders(gl::GLContext& ctx);

	void load();

	void _create_default_texture();
	void _load_textures();
	void _create_skeleton();
	void _build_geometry();

	void playAnimation(int index);
	void stopAnimation();
	void updateAnimation(float delta_time);

	void _update_node_matrices();

	std::array<float, 3> _sample_vec3(const MDXAnimVector& track, float frame);
	std::array<float, 4> _sample_quat(const MDXAnimVector& track, float frame);

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

	void dispose();

	// C++ accessors for JS public properties.
	const std::vector<MDXDrawCall>& get_draw_calls() const { return draw_calls; }

	// Animation pause control — matches the JS optional-call pattern (renderer.set_animation_paused?.(paused)).
	void set_animation_paused(bool paused) { animation_paused = paused; }

	// Public state (matches JS properties)
	std::unique_ptr<MDXLoader> mdx;
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
	std::vector<MDXDrawCall> draw_calls;

	// animation state (node-based skeleton)
	std::vector<MDXNode*> nodes;        // flattened non-null nodes
	std::vector<float> node_matrices;   // (maxId + 1) * 16 floats
	int current_animation = -1;         // -1 = null
	float animation_time = 0;
	bool animation_paused = false;

	// reactive state
	std::string geosetKey = "modelViewerGeosets";
	std::vector<MDXGeosetEntry> geosetArray;

	// transforms
	std::array<float, 16> model_matrix;
	std::array<float, 3> position = {0, 0, 0};
	std::array<float, 3> rotation = {0, 0, 0};
	std::array<float, 3> scale_val = {1, 1, 1};
};
