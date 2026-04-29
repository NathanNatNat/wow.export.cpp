/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstddef>
#include <memory>

#include "../gl/UniformBuffer.h"

namespace gl { class GLContext; class ShaderProgram; }

namespace renderer_utils {

/**
 * UBO + offset captured for the `VsBoneUbo` block in m2.vertex.shader.
 *
 * Each renderer owns one of these and writes bone matrices into the CPU
 * shadow buffer at @p matrix_offset, then calls `ubo->upload(); ubo->bind(0);`
 * each frame before issuing draw calls.
 */
struct BonesUbo {
	std::unique_ptr<gl::UniformBuffer> ubo;
	int matrix_offset = 0;     // byte offset of u_bone_matrices[0] inside the UBO
	std::size_t max_bones = 0; // capacity (in mat4 slots) — set from queried block size
};

/**
 * JS equivalent: `create_bones_ubo(shader, gl, ctx, ubos, bone_count)` in
 * `src/js/3D/renderers/renderer_utils.js`.
 *
 * Binds the `VsBoneUbo` uniform block to binding point 0, allocates a UBO
 * sized to the block's `GL_UNIFORM_BLOCK_DATA_SIZE`, captures the byte
 * offset of `u_bone_matrices[0]`, and initialises the first @p bone_count
 * matrix slots to identity.
 */
BonesUbo create_bones_ubo(gl::ShaderProgram& shader,
                          gl::GLContext& ctx,
                          std::size_t bone_count = 0);

} // namespace renderer_utils
