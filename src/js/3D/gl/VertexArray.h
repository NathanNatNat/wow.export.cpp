/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <glad/gl.h>
#include <cstddef>
#include <cstdint>

namespace gl {

class GLContext;

/**
 * Attribute location constants matching shader layout.
 *
 * JS equivalent: const AttributeLocation = { ... };
 *                VertexArray.AttributeLocation = AttributeLocation;
 */
namespace AttributeLocation {
	constexpr GLuint POSITION     = 0;
	constexpr GLuint NORMAL       = 1;
	constexpr GLuint BONE_INDICES = 2;
	constexpr GLuint BONE_WEIGHTS = 3;
	constexpr GLuint TEXCOORD     = 4;
	constexpr GLuint TEXCOORD2    = 5;
	constexpr GLuint COLOR        = 6;
	constexpr GLuint COLOR2       = 7;
	constexpr GLuint TEXCOORD3    = 8;
	constexpr GLuint TEXCOORD4    = 9;
	constexpr GLuint COLOR3       = 10;
} // namespace AttributeLocation

/**
 * Vertex Array Object (VAO) wrapper.
 *
 * JS equivalent: class VertexArray — module.exports = VertexArray
 */
class VertexArray {
public:
	explicit VertexArray(GLContext& ctx);

	void bind();

	void set_vertex_buffer(const void* data, size_t size_bytes,
	                       GLenum usage = GL_STATIC_DRAW);

	void set_index_buffer(const uint16_t* data, size_t count,
	                      GLenum usage = GL_STATIC_DRAW);
	void set_index_buffer(const uint32_t* data, size_t count,
	                      GLenum usage = GL_STATIC_DRAW);

	void set_attribute(GLuint location, GLint size, GLenum type,
	                   GLboolean normalized, GLsizei stride, GLsizei offset);
	void set_attribute_i(GLuint location, GLint size, GLenum type,
	                     GLsizei stride, GLsizei offset);

	void setup_m2_vertex_format();
	void setup_m2_separate_buffers(GLuint pos_buffer, GLuint norm_buffer,
	                               GLuint uv_buffer, GLuint bone_idx_buffer,
	                               GLuint bone_weight_buffer,
	                               GLuint uv2_buffer = 0);
	void setup_wmo_separate_buffers(GLuint pos_buffer, GLuint norm_buffer,
	                                GLuint uv_buffer, GLuint color_buffer,
	                                GLuint color2_buffer, GLuint color3_buffer,
	                                GLuint uv2_buffer, GLuint uv3_buffer,
	                                GLuint uv4_buffer);

	void draw(GLenum mode, GLsizei count = -1, GLsizei offset = 0);

	void dispose();

	// Public state
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	GLsizei index_count = 0;
	GLenum index_type = GL_UNSIGNED_SHORT;

private:
	GLContext& ctx_;
};

} // namespace gl
