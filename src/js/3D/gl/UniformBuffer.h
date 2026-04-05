/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <glad/gl.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace gl {

class GLContext;

/**
 * GPU uniform buffer (UBO) wrapper with CPU-side shadow copy.
 *
 * JS equivalent: class UniformBuffer — module.exports = UniformBuffer
 *
 * The JS version uses ArrayBuffer + DataView + Float32Array + Int32Array
 * views over a shared buffer.  In C++ we keep a std::vector<uint8_t> and
 * use std::memcpy for type-punned writes, which is well-defined and
 * avoids strict-aliasing issues.
 */
class UniformBuffer {
public:
	UniformBuffer(GLContext& ctx, size_t size, GLenum usage = GL_DYNAMIC_DRAW);

	void bind(GLuint binding_point);
	void bind_range(GLuint binding_point, GLintptr offset, GLsizeiptr size);

	void set_float(size_t offset, float value);
	void set_int(size_t offset, int32_t value);
	void set_vec2(size_t offset, float x, float y);
	void set_vec3(size_t offset, float x, float y, float z);
	void set_vec4(size_t offset, float x, float y, float z, float w);
	void set_ivec4(size_t offset, int32_t x, int32_t y, int32_t z, int32_t w);

	void set_mat4(size_t offset, const float* matrix);
	void set_mat4_array(size_t offset, const float* matrices, size_t count);
	void set_float_array(size_t offset, const float* values, size_t count);
	void set_vec4_array(size_t offset, const float* values, size_t count);

	void upload();
	void upload_range(size_t offset, size_t size);

	void dispose();

	// std140 alignment helpers
	static constexpr size_t ALIGN_VEC4 = 16;
	static constexpr size_t ALIGN_MAT4 = 64;
	static constexpr size_t SIZE_FLOAT = 4;
	static constexpr size_t SIZE_VEC2  = 8;
	static constexpr size_t SIZE_VEC3  = 12;
	static constexpr size_t SIZE_VEC4  = 16;
	static constexpr size_t SIZE_MAT4  = 64;

	/**
	 * Calculate std140-aligned offset for next element.
	 * Equivalent to Math.ceil(current_offset / alignment) * alignment.
	 */
	static constexpr size_t align(size_t current_offset, size_t alignment) {
		return ((current_offset + alignment - 1) / alignment) * alignment;
	}

	// Public state
	GLuint buffer = 0;
	size_t size = 0;
	GLenum usage = GL_DYNAMIC_DRAW;
	bool dirty = false;

private:
	GLContext& ctx_;
	std::vector<uint8_t> data_;
};

} // namespace gl
