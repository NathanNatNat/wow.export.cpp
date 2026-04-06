/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <glad/gl.h>
#include <memory>

namespace gl {
class GLContext;
class ShaderProgram;
}

/**
 * Renders a flat grid in the 3D viewport.
 *
 * JS equivalent: class GridRenderer — module.exports = GridRenderer
 */
class GridRenderer {
public:
	/**
	 * @param gl_context
	 * @param size - total size of the grid
	 * @param divisions - number of divisions
	 */
	GridRenderer(gl::GLContext& gl_context, float size = 100.0f, int divisions = 100);

	/**
	 * @param view_matrix
	 * @param projection_matrix
	 */
	void render(const float* view_matrix, const float* projection_matrix);

	void dispose();

private:
	void _init();
	void _create_shader();
	void _create_geometry();

	gl::GLContext& ctx;
	float size;
	int divisions;

	std::unique_ptr<gl::ShaderProgram> shader;
	GLuint vao = 0;
	GLuint vertex_buffer = 0;
	GLsizei vertex_count = 0;
};
