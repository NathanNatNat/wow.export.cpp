/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <glad/gl.h>
#include <array>
#include <cstdint>

namespace gl {

// Anisotropy constants — core in GL 4.6 (ARB_texture_filter_anisotropic).
// Provide fallback defines in case GLAD2 does not include them.
#ifndef GL_TEXTURE_MAX_ANISOTROPY
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY
#define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
#endif

/**
 * WoW blend modes matching the M2/WMO material blend modes.
 *
 * JS equivalent: const BlendMode = { ... };  GLContext.BlendMode = BlendMode;
 */
enum BlendMode : int {
	OPAQUE = 0,
	ALPHA_KEY = 1,
	ALPHA = 2,
	ADD = 3,
	MOD = 4,
	MOD2X = 5,
	MOD_ADD = 6,
	INV_SRC_ALPHA_ADD = 7,
	INV_SRC_ALPHA_OPAQUE = 8,
	SRC_ALPHA_OPAQUE = 9,
	NO_ALPHA_ADD = 10,
	CONSTANT_ALPHA = 11,
	SCREEN = 12,
	BLEND_ADD = 13
};

/**
 * OpenGL rendering context wrapper with state caching.
 *
 * JS equivalent: class GLContext — module.exports = GLContext
 *
 * In the JS version this wraps a WebGL2RenderingContext obtained from an
 * HTML canvas.  In the C++ desktop build the OpenGL 4.6 core context is
 * created by GLFW before this object is constructed, so the constructor
 * simply initialises GL state and queries capabilities.
 */
class GLContext {
public:
	GLContext();

	void set_viewport(int width, int height);
	void clear(bool color = true, bool depth = true, bool stencil = false);
	void set_clear_color(float r, float g, float b, float a = 1.0f);

	void set_depth_test(bool enable);
	void set_depth_write(bool enable);
	void set_depth_func(GLenum func);
	void set_cull_face(bool enable);
	void set_cull_mode(GLenum mode);
	void set_blend(bool enable);
	void set_blend_func(GLenum src, GLenum dst);
	void set_blend_func_separate(GLenum src_rgb, GLenum dst_rgb,
	                             GLenum src_alpha, GLenum dst_alpha);

	void apply_blend_mode(int blend_mode);

	void use_program(GLuint program);
	void bind_vao(GLuint vao);
	void active_texture(int unit);
	void bind_texture(int unit, GLuint texture, GLenum target = GL_TEXTURE_2D);

	void draw_elements(GLenum mode, GLsizei count, GLenum type, GLintptr offset);
	void draw_arrays(GLenum mode, GLint first, GLsizei count);

	void dispose();

	// Extension / capability flags (public — read by GLTexture etc.)
	bool ext_s3tc = false;
	bool ext_s3tc_srgb = false;
	bool ext_aniso = false;
	float max_anisotropy = 1.0f;
	bool ext_float_texture = false;

	int viewport_width = 0;
	int viewport_height = 0;

private:
	void _init_extensions();
	void _init_state();

	// State cache
	bool _depth_test = false;
	bool _depth_write = true;
	GLenum _depth_func = GL_LEQUAL;
	bool _cull_face = false;
	GLenum _cull_mode = GL_BACK;
	bool _blend = false;
	GLenum _blend_src = GL_ONE;
	GLenum _blend_dst = GL_ZERO;
	GLuint _current_program = 0;
	GLuint _current_vao = 0;
	std::array<GLuint, 16> _bound_textures{};
	int _active_texture_unit = 0;
};

} // namespace gl
