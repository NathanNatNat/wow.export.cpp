/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#pragma once

#include <glad/gl.h>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "../loaders/M3Loader.h"
#include "../gl/GLContext.h"
#include "../gl/GLTexture.h"
#include "../gl/VertexArray.h"
#include "../gl/ShaderProgram.h"

class BufferWrapper;

/**
 * Draw call descriptor for a single M3 geoset.
 */
struct M3DrawCall {
	gl::VertexArray* vao = nullptr;
	GLuint ebo = 0;
	uint32_t count = 0;
	bool visible = true;
};

/**
 * M3 (Diablo III model) renderer using OpenGL.
 *
 * JS equivalent: class M3RendererGL — module.exports = M3RendererGL
 */
class M3RendererGL {
public:
	M3RendererGL(BufferWrapper& data, gl::GLContext& gl_context, bool reactive = false, bool useRibbon = true);

	static std::unique_ptr<gl::ShaderProgram> load_shaders(gl::GLContext& ctx);

	void load();

	void _create_default_texture();
	void loadLOD(int index);

	void updateGeosets();
	void updateWireframe();

	struct BoundingBoxResult {
		std::array<float, 3> min;
		std::array<float, 3> max;
	};
	std::optional<BoundingBoxResult> getBoundingBox();

	void render(const float* view_matrix, const float* projection_matrix);

	void _dispose_geometry();
	void dispose();

	// Accessor for draw_calls (JS: public property)
	const std::vector<M3DrawCall>& get_draw_calls() const { return draw_calls; }

	// Public state (matches JS properties)
	std::unique_ptr<M3Loader> m3;
	std::unique_ptr<gl::ShaderProgram> shader;

private:
	BufferWrapper* data_ptr;
	gl::GLContext& ctx;
	bool reactive;
	bool useRibbon;

	// rendering state
	std::vector<std::unique_ptr<gl::VertexArray>> vaos;
	std::vector<GLuint> buffers;
	std::vector<M3DrawCall> draw_calls;
	std::unique_ptr<gl::GLTexture> default_texture;

	// transforms
	std::array<float, 16> model_matrix;
};
