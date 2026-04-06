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
 * Renders a radial shadow plane beneath models.
 *
 * JS equivalent: class ShadowPlaneRenderer — module.exports = ShadowPlaneRenderer
 */
class ShadowPlaneRenderer {
public:
	/**
	 * @param gl_context
	 * @param size - size of the shadow plane
	 */
	ShadowPlaneRenderer(gl::GLContext& gl_context, float size = 2.0f);

	/**
	 * @param view_matrix
	 * @param projection_matrix
	 */
	void render(const float* view_matrix, const float* projection_matrix);

	void dispose();

	float shadow_radius = 8.0f;
	bool visible = true;

private:
	void _init();
	void _create_shader();
	void _create_geometry();

	gl::GLContext& ctx;
	float size;

	std::unique_ptr<gl::ShaderProgram> shader;
	GLuint vao = 0;
	GLuint vertex_buffer = 0;
	GLuint index_buffer = 0;
};
