/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "UniformBuffer.h"
#include "GLContext.h"

#include <cstring>

namespace gl {

/*
 * Endianness deviation from JS source (`src/js/3D/gl/UniformBuffer.js`):
 *
 * The JS implementation writes scalar values via DataView with the
 * little-endian flag explicitly set, e.g.:
 *     this.view.setFloat32(offset, value, true);   // line 51
 *     this.view.setInt32  (offset, value, true);   // line 60
 *     this.view.setFloat32(offset + 4, y, true);   // lines 70–71, 82–84,
 *                                                  //       96–99
 *     this.view.setInt32  (offset + 4, y, true);   // lines 111–114
 *
 * The C++ port below uses `std::memcpy`, which writes the value in the
 * host's native byte order.  Per CLAUDE.md the only supported platforms
 * are Windows x64 (MSVC) and Linux x64 (GCC); both are little-endian, so
 * the on-buffer byte layout is identical to the JS DataView output and
 * the GPU receives the same bytes.  This is a documented platform
 * assumption — porting to a big-endian target would require switching
 * these writes to explicit little-endian byte writes (or a byte-swap on
 * `memcpy`) to preserve JS-equivalent behaviour.
 */

UniformBuffer::UniformBuffer(GLContext& ctx, size_t sz, GLenum usg)
	: ctx_(ctx), size(sz), usage(usg), data_(sz, 0) {
	glGenBuffers(1, &buffer);

	// allocate gpu buffer
	glBindBuffer(GL_UNIFORM_BUFFER, buffer);
	glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(sz), nullptr, usg);
}

void UniformBuffer::bind(GLuint binding_point) {
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, buffer);
}

void UniformBuffer::bind_range(GLuint binding_point, GLintptr offset,
                                GLsizeiptr sz) {
	glBindBufferRange(GL_UNIFORM_BUFFER, binding_point, buffer, offset, sz);
}

void UniformBuffer::set_float(size_t offset, float value) {
	std::memcpy(data_.data() + offset, &value, sizeof(float));
	dirty = true;
}

void UniformBuffer::set_int(size_t offset, int32_t value) {
	std::memcpy(data_.data() + offset, &value, sizeof(int32_t));
	dirty = true;
}

void UniformBuffer::set_vec2(size_t offset, float x, float y) {
	float values[2] = { x, y };
	std::memcpy(data_.data() + offset, values, sizeof(values));
	dirty = true;
}

void UniformBuffer::set_vec3(size_t offset, float x, float y, float z) {
	float values[3] = { x, y, z };
	std::memcpy(data_.data() + offset, values, sizeof(values));
	dirty = true;
}

void UniformBuffer::set_vec4(size_t offset, float x, float y,
                              float z, float w) {
	float values[4] = { x, y, z, w };
	std::memcpy(data_.data() + offset, values, sizeof(values));
	dirty = true;
}

void UniformBuffer::set_ivec4(size_t offset, int32_t x, int32_t y,
                               int32_t z, int32_t w) {
	int32_t values[4] = { x, y, z, w };
	std::memcpy(data_.data() + offset, values, sizeof(values));
	dirty = true;
}

void UniformBuffer::set_mat4(size_t offset, const float* matrix) {
	// 16 floats = 64 bytes
	std::memcpy(data_.data() + offset, matrix, 16 * sizeof(float));
	dirty = true;
}

void UniformBuffer::set_mat4_array(size_t offset, const float* matrices,
                                    size_t count) {
	std::memcpy(data_.data() + offset, matrices,
	            count * 16 * sizeof(float));
	dirty = true;
}

void UniformBuffer::set_float_array(size_t offset, const float* values,
                                     size_t count) {
	std::memcpy(data_.data() + offset, values, count * sizeof(float));
	dirty = true;
}

void UniformBuffer::set_vec4_array(size_t offset, const float* values,
                                    size_t count) {
	std::memcpy(data_.data() + offset, values, count * 4 * sizeof(float));
	dirty = true;
}

std::span<float> UniformBuffer::get_float32_view(size_t byte_offset,
                                                   size_t float_count) {
	return std::span<float>(
	    reinterpret_cast<float*>(data_.data() + byte_offset), float_count);
}

void UniformBuffer::upload() {
	if (!dirty)
		return;

	glBindBuffer(GL_UNIFORM_BUFFER, buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0,
	                static_cast<GLsizeiptr>(data_.size()), data_.data());
	dirty = false;
}

void UniformBuffer::upload_range(size_t offset, size_t sz) {
	glBindBuffer(GL_UNIFORM_BUFFER, buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, static_cast<GLintptr>(offset),
	                static_cast<GLsizeiptr>(sz), data_.data() + offset);
}

void UniformBuffer::dispose() {
	if (buffer) {
		glDeleteBuffers(1, &buffer);
		buffer = 0;
	}

	data_.clear();
	data_.shrink_to_fit();
}

} // namespace gl
