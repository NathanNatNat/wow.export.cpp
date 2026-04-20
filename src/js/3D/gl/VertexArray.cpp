/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "VertexArray.h"
#include "GLContext.h"

namespace gl {

VertexArray::VertexArray(GLContext& ctx) : ctx_(ctx) {
	glGenVertexArrays(1, &vao);
}

void VertexArray::bind() {
	ctx_.bind_vao(vao);
}

void VertexArray::set_vertex_buffer(const void* data, size_t size_bytes,
                                     GLenum usage) {
	if (!vbo)
		glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size_bytes),
	             data, usage);
}

void VertexArray::set_index_buffer(const uint16_t* data, size_t count,
                                    GLenum usage) {
	if (!ebo)
		glGenBuffers(1, &ebo);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             static_cast<GLsizeiptr>(count * sizeof(uint16_t)),
	             data, usage);

	index_count = static_cast<GLsizei>(count);
	index_type = GL_UNSIGNED_SHORT;
}

void VertexArray::set_index_buffer(const uint32_t* data, size_t count,
                                    GLenum usage) {
	if (!ebo)
		glGenBuffers(1, &ebo);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             static_cast<GLsizeiptr>(count * sizeof(uint32_t)),
	             data, usage);

	index_count = static_cast<GLsizei>(count);
	index_type = GL_UNSIGNED_INT;
}

void VertexArray::set_attribute(GLuint location, GLint size, GLenum type,
                                 GLboolean normalized, GLsizei stride,
                                 GLsizei offset) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, size, type, normalized, stride,
	                      reinterpret_cast<const void*>(static_cast<uintptr_t>(offset)));
}

void VertexArray::set_attribute_i(GLuint location, GLint size, GLenum type,
                                   GLsizei stride, GLsizei offset) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(location);
	glVertexAttribIPointer(location, size, type, stride,
	                       reinterpret_cast<const void*>(static_cast<uintptr_t>(offset)));
}

void VertexArray::setup_m2_vertex_format() {
	constexpr GLsizei stride = 40;

	bind();
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// position: vec3 at offset 0
	glEnableVertexAttribArray(AttributeLocation::POSITION);
	glVertexAttribPointer(AttributeLocation::POSITION, 3, GL_FLOAT,
	                      GL_FALSE, stride, reinterpret_cast<const void*>(0));

	// normal: vec3 at offset 12
	glEnableVertexAttribArray(AttributeLocation::NORMAL);
	glVertexAttribPointer(AttributeLocation::NORMAL, 3, GL_FLOAT,
	                      GL_FALSE, stride, reinterpret_cast<const void*>(12));

	// bone indices: uvec4 at offset 24
	glEnableVertexAttribArray(AttributeLocation::BONE_INDICES);
	glVertexAttribIPointer(AttributeLocation::BONE_INDICES, 4, GL_UNSIGNED_BYTE,
	                       stride, reinterpret_cast<const void*>(24));

	// bone weights: vec4 (normalized) at offset 28
	glEnableVertexAttribArray(AttributeLocation::BONE_WEIGHTS);
	glVertexAttribPointer(AttributeLocation::BONE_WEIGHTS, 4, GL_UNSIGNED_BYTE,
	                      GL_TRUE, stride, reinterpret_cast<const void*>(28));

	// texcoord1: vec2 at offset 32
	glEnableVertexAttribArray(AttributeLocation::TEXCOORD);
	glVertexAttribPointer(AttributeLocation::TEXCOORD, 2, GL_FLOAT,
	                      GL_FALSE, stride, reinterpret_cast<const void*>(32));

	// texcoord2 uses separate buffer or not present for simple models
}

void VertexArray::setup_m2_separate_buffers(GLuint pos_buffer,
                                             GLuint norm_buffer,
                                             GLuint uv_buffer,
                                             GLuint bone_idx_buffer,
                                             GLuint bone_weight_buffer,
                                             GLuint uv2_buffer) {
	bind();

	// position: vec3
	glBindBuffer(GL_ARRAY_BUFFER, pos_buffer);
	glEnableVertexAttribArray(AttributeLocation::POSITION);
	glVertexAttribPointer(AttributeLocation::POSITION, 3, GL_FLOAT,
	                      GL_FALSE, 0, nullptr);

	// normal: vec3
	glBindBuffer(GL_ARRAY_BUFFER, norm_buffer);
	glEnableVertexAttribArray(AttributeLocation::NORMAL);
	glVertexAttribPointer(AttributeLocation::NORMAL, 3, GL_FLOAT,
	                      GL_FALSE, 0, nullptr);

	// bone indices: uvec4
	glBindBuffer(GL_ARRAY_BUFFER, bone_idx_buffer);
	glEnableVertexAttribArray(AttributeLocation::BONE_INDICES);
	glVertexAttribIPointer(AttributeLocation::BONE_INDICES, 4, GL_UNSIGNED_BYTE,
	                       0, nullptr);

	// bone weights: vec4 normalized
	glBindBuffer(GL_ARRAY_BUFFER, bone_weight_buffer);
	glEnableVertexAttribArray(AttributeLocation::BONE_WEIGHTS);
	glVertexAttribPointer(AttributeLocation::BONE_WEIGHTS, 4, GL_UNSIGNED_BYTE,
	                      GL_TRUE, 0, nullptr);

	// texcoord1: vec2
	glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	glEnableVertexAttribArray(AttributeLocation::TEXCOORD);
	glVertexAttribPointer(AttributeLocation::TEXCOORD, 2, GL_FLOAT,
	                      GL_FALSE, 0, nullptr);

	// texcoord2: vec2 (optional)
	if (uv2_buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, uv2_buffer);
		glEnableVertexAttribArray(AttributeLocation::TEXCOORD2);
		glVertexAttribPointer(AttributeLocation::TEXCOORD2, 2, GL_FLOAT,
		                      GL_FALSE, 0, nullptr);
	} else {
		glDisableVertexAttribArray(AttributeLocation::TEXCOORD2);
	}
}

void VertexArray::setup_wmo_separate_buffers(GLuint pos_buffer,
                                              GLuint norm_buffer,
                                              GLuint uv_buffer,
                                              GLuint color_buffer,
                                              GLuint color2_buffer,
                                              GLuint color3_buffer,
                                              GLuint uv2_buffer,
                                              GLuint uv3_buffer,
                                              GLuint uv4_buffer) {
	bind();

	// position: vec3
	glBindBuffer(GL_ARRAY_BUFFER, pos_buffer);
	glEnableVertexAttribArray(AttributeLocation::POSITION);
	glVertexAttribPointer(AttributeLocation::POSITION, 3, GL_FLOAT,
	                      GL_FALSE, 0, nullptr);

	// normal: vec3
	glBindBuffer(GL_ARRAY_BUFFER, norm_buffer);
	glEnableVertexAttribArray(AttributeLocation::NORMAL);
	glVertexAttribPointer(AttributeLocation::NORMAL, 3, GL_FLOAT,
	                      GL_FALSE, 0, nullptr);

	// texcoord1: vec2
	glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	glEnableVertexAttribArray(AttributeLocation::TEXCOORD);
	glVertexAttribPointer(AttributeLocation::TEXCOORD, 2, GL_FLOAT,
	                      GL_FALSE, 0, nullptr);

	// disable unused
	glDisableVertexAttribArray(AttributeLocation::BONE_INDICES);
	glDisableVertexAttribArray(AttributeLocation::BONE_WEIGHTS);

	// vertex color (optional)
	if (color_buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
		glEnableVertexAttribArray(AttributeLocation::COLOR);
		glVertexAttribPointer(AttributeLocation::COLOR, 4, GL_UNSIGNED_BYTE,
		                      GL_TRUE, 0, nullptr);
	} else {
		glDisableVertexAttribArray(AttributeLocation::COLOR);
	}

	if (color2_buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, color2_buffer);
		glEnableVertexAttribArray(AttributeLocation::COLOR2);
		glVertexAttribPointer(AttributeLocation::COLOR2, 4, GL_UNSIGNED_BYTE,
		                      GL_TRUE, 0, nullptr);
	} else {
		glDisableVertexAttribArray(AttributeLocation::COLOR2);
	}

	// color 3 (optional)
	if (color3_buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, color3_buffer);
		glEnableVertexAttribArray(AttributeLocation::COLOR3);
		glVertexAttribPointer(AttributeLocation::COLOR3, 4, GL_UNSIGNED_BYTE,
		                      GL_TRUE, 0, nullptr);
	} else {
		glDisableVertexAttribArray(AttributeLocation::COLOR3);
	}

	// uv2 (optional)
	if (uv2_buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, uv2_buffer);
		glEnableVertexAttribArray(AttributeLocation::TEXCOORD2);
		glVertexAttribPointer(AttributeLocation::TEXCOORD2, 2, GL_FLOAT,
		                      GL_FALSE, 0, nullptr);
	} else {
		glDisableVertexAttribArray(AttributeLocation::TEXCOORD2);
	}

	// uv3 (optional)
	if (uv3_buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, uv3_buffer);
		glEnableVertexAttribArray(AttributeLocation::TEXCOORD3);
		glVertexAttribPointer(AttributeLocation::TEXCOORD3, 2, GL_FLOAT,
		                      GL_FALSE, 0, nullptr);
	} else {
		glDisableVertexAttribArray(AttributeLocation::TEXCOORD3);
	}

	// uv4 (optional)
	if (uv4_buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, uv4_buffer);
		glEnableVertexAttribArray(AttributeLocation::TEXCOORD4);
		glVertexAttribPointer(AttributeLocation::TEXCOORD4, 2, GL_FLOAT,
		                      GL_FALSE, 0, nullptr);
	} else {
		glDisableVertexAttribArray(AttributeLocation::TEXCOORD4);
	}
}

void VertexArray::draw(GLenum mode, GLsizei count, GLsizei offset) {
	if (count < 0)
		count = index_count;

	GLintptr byte_offset = static_cast<GLintptr>(offset) *
	    (index_type == GL_UNSIGNED_INT ? 4 : 2);
	ctx_.draw_elements(mode, count, index_type, byte_offset);
}

void VertexArray::dispose() {
	if (vao) {
		ctx_.unbind_vao(vao);
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}

	if (vbo) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}

	if (ebo) {
		glDeleteBuffers(1, &ebo);
		ebo = 0;
	}
}

} // namespace gl
