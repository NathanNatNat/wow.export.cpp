/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "GLContext.h"

#include <algorithm>
#include <cstring>
#include <string_view>

namespace gl {

GLContext::GLContext() {
	_init_extensions();
	_init_state();
}

void GLContext::_init_extensions() {
	// compressed texture formats — check for S3TC extension availability
	int num_extensions = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

	for (int i = 0; i < num_extensions; ++i) {
		std::string_view ext(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));

		if (ext == "GL_EXT_texture_compression_s3tc")
			ext_s3tc = true;
		else if (ext == "GL_EXT_texture_compression_s3tc_srgb")
			ext_s3tc_srgb = true;
		else if (ext == "GL_EXT_texture_filter_anisotropic" ||
		         ext == "GL_ARB_texture_filter_anisotropic")
			ext_aniso = true;
	}

	// anisotropic filtering — query max if extension is present
	if (ext_aniso)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);

	// float textures (core in GL 3.0+)
	ext_float_texture = true;
}

void GLContext::_init_state() {
	// default state
	glEnable(GL_DEPTH_TEST);
	_depth_test = true;
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);

	// clear color
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void GLContext::set_viewport(int width, int height) {
	glViewport(0, 0, width, height);
	viewport_width = width;
	viewport_height = height;
}

void GLContext::clear(bool color, bool depth, bool stencil) {
	GLbitfield bits = 0;

	if (color)
		bits |= GL_COLOR_BUFFER_BIT;

	if (depth)
		bits |= GL_DEPTH_BUFFER_BIT;

	if (stencil)
		bits |= GL_STENCIL_BUFFER_BIT;

	if (bits)
		glClear(bits);
}

void GLContext::set_clear_color(float r, float g, float b, float a) {
	glClearColor(r, g, b, a);
}

void GLContext::set_depth_test(bool enable) {
	if (_depth_test == enable)
		return;

	if (enable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	_depth_test = enable;
}

void GLContext::set_depth_write(bool enable) {
	if (_depth_write == enable)
		return;

	glDepthMask(enable ? GL_TRUE : GL_FALSE);
	_depth_write = enable;
}

void GLContext::set_depth_func(GLenum func) {
	if (_depth_func == func)
		return;

	glDepthFunc(func);
	_depth_func = func;
}

void GLContext::set_cull_face(bool enable) {
	if (_cull_face == enable)
		return;

	if (enable)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	_cull_face = enable;
}

void GLContext::set_cull_mode(GLenum mode) {
	if (_cull_mode == mode)
		return;

	glCullFace(mode);
	_cull_mode = mode;
}

void GLContext::set_blend(bool enable) {
	if (_blend == enable)
		return;

	if (enable)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	_blend = enable;
}

void GLContext::set_blend_func(GLenum src, GLenum dst) {
	if (_blend_src == src && _blend_dst == dst)
		return;

	glBlendFunc(src, dst);
	_blend_src = src;
	_blend_dst = dst;
}

void GLContext::set_blend_func_separate(GLenum src_rgb, GLenum dst_rgb,
                                        GLenum src_alpha, GLenum dst_alpha) {
	glBlendFuncSeparate(src_rgb, dst_rgb, src_alpha, dst_alpha);
	_blend_src = src_rgb;
	_blend_dst = dst_rgb;
}

void GLContext::apply_blend_mode(int blend_mode) {
	switch (blend_mode) {
		case BlendMode::OPAQUE:
			set_blend(false);
			set_depth_write(true);
			break;

		case BlendMode::ALPHA_KEY:
			// alpha test handled in shader, depth write enabled since discarded pixels don't write depth
			set_blend(true);
			set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			set_depth_write(true);
			break;

		case BlendMode::ALPHA:
			set_blend(true);
			set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			set_depth_write(false);
			break;

		case BlendMode::ADD:
		case BlendMode::NO_ALPHA_ADD:
			set_blend(true);
			set_blend_func(GL_SRC_ALPHA, GL_ONE);
			set_depth_write(false);
			break;

		case BlendMode::MOD:
			set_blend(true);
			set_blend_func(GL_DST_COLOR, GL_ZERO);
			set_depth_write(false);
			break;

		case BlendMode::MOD2X:
			set_blend(true);
			set_blend_func(GL_DST_COLOR, GL_SRC_COLOR);
			set_depth_write(false);
			break;

		case BlendMode::MOD_ADD:
			set_blend(true);
			set_blend_func_separate(GL_DST_COLOR, GL_ONE, GL_DST_ALPHA, GL_ONE);
			set_depth_write(false);
			break;

		case BlendMode::INV_SRC_ALPHA_ADD:
			set_blend(true);
			set_blend_func(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
			set_depth_write(false);
			break;

		case BlendMode::INV_SRC_ALPHA_OPAQUE:
			set_blend(true);
			set_blend_func(GL_ONE_MINUS_SRC_ALPHA, GL_ZERO);
			set_depth_write(true);
			break;

		case BlendMode::SRC_ALPHA_OPAQUE:
			set_blend(true);
			set_blend_func(GL_SRC_ALPHA, GL_ZERO);
			set_depth_write(true);
			break;

		case BlendMode::CONSTANT_ALPHA:
			set_blend(true);
			set_blend_func(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
			set_depth_write(false);
			break;

		case BlendMode::SCREEN:
			set_blend(true);
			set_blend_func(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
			set_depth_write(false);
			break;

		case BlendMode::BLEND_ADD:
			set_blend(true);
			set_blend_func_separate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
			                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			set_depth_write(false);
			break;

		default:
			set_blend(false);
			set_depth_write(true);
	}
}

void GLContext::use_program(GLuint program) {
	if (_current_program == program)
		return;

	glUseProgram(program);
	_current_program = program;
}

void GLContext::bind_vao(GLuint vao) {
	if (_current_vao == vao)
		return;

	glBindVertexArray(vao);
	_current_vao = vao;
}

void GLContext::unbind_vao(GLuint vao) {
	if (_current_vao == vao) {
		glBindVertexArray(0);
		_current_vao = 0;
	}
}

void GLContext::active_texture(int unit) {
	if (_active_texture_unit == unit)
		return;

	glActiveTexture(GL_TEXTURE0 + unit);
	_active_texture_unit = unit;
}

void GLContext::bind_texture(int unit, GLuint texture, GLenum target) {
	if (_bound_textures[unit] == texture)
		return;

	active_texture(unit);
	glBindTexture(target, texture);
	_bound_textures[unit] = texture;
}

void GLContext::draw_elements(GLenum mode, GLsizei count,
                               GLenum type, GLintptr offset) {
	glDrawElements(mode, count, type, reinterpret_cast<const void*>(offset));
}

void GLContext::draw_arrays(GLenum mode, GLint first, GLsizei count) {
	glDrawArrays(mode, first, count);
}

void GLContext::dispose() {
	// context is automatically cleaned up when the GLFW window is destroyed
}

void GLContext::invalidate_cache() {
	// Reset all cached state so subsequent set_*() / bind_*() calls always
	// issue the real GL commands.  This is necessary after an external system
	// (e.g. ImGui's OpenGL3 backend) has modified GL state behind our back.
	_depth_test = false;
	_depth_write = true;
	_depth_func = GL_LEQUAL;
	_cull_face = false;
	_cull_mode = GL_BACK;
	_blend = false;
	_blend_src = GL_ONE;
	_blend_dst = GL_ZERO;
	_current_program = 0;
	_current_vao = 0;
	_bound_textures.fill(0);
	_active_texture_unit = -1; // force re-issue of glActiveTexture
}

} // namespace gl
