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
#include <span>
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

	/**
	 * Returns a view into the CPU shadow buffer as floats.
	 * Equivalent to JS: new Float32Array(this.data, offset, length)
	 * The caller must call upload_range() after writing through this span.
	 * @param byte_offset byte offset into the buffer
	 * @param float_count number of floats in the view
	 */
	std::span<float> get_float32_view(size_t byte_offset, size_t float_count);

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
	/**
	 * CPU-side shadow copy of the GPU uniform buffer contents.
	 *
	 * Encapsulation deviation from JS: the original
	 * `src/js/3D/gl/UniformBuffer.js` (lines 20–22) exposes three public
	 * properties over a shared ArrayBuffer:
	 *   - `view`       : DataView          (byte/typed reads + writes)
	 *   - `float_view` : Float32Array      (typed float access)
	 *   - `int_view`   : Int32Array        (typed int access)
	 *
	 * In C++ the backing storage is a single private `std::vector<uint8_t>`
	 * (`data_`) and all writes flow through the typed setter methods
	 * (`set_float`, `set_int`, `set_vec*`, `set_ivec4`, `set_mat4`,
	 * `set_mat4_array`, `set_float_array`, `set_vec4_array`).  A read-only
	 * span is exposed via `get_float32_view` for the one JS call site that
	 * needs direct float access (callers must invoke `upload_range` after
	 * mutating it, since dirty tracking is bypassed).
	 *
	 * Any future port of code that direct-accesses the JS view properties
	 * (e.g. `ubo.float_view[i] = x`, `ubo.view.setFloat32(...)`) must be
	 * rewritten to use the typed setters above.
	 */
	std::vector<uint8_t> data_;
};

} // namespace gl
