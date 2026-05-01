#include "renderer_utils.h"

#include <array>
#include <cstring>

#include "../gl/GLContext.h"
#include "../gl/ShaderProgram.h"

namespace renderer_utils {

static constexpr std::array<float, 16> IDENTITY_MAT4 = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

BonesUbo create_bones_ubo(gl::ShaderProgram& shader, gl::GLContext& ctx,
                          std::size_t bone_count) {
	shader.bind_uniform_block("VsBoneUbo", 0);

	const GLint ubo_size = shader.get_uniform_block_param(
		"VsBoneUbo", GL_UNIFORM_BLOCK_DATA_SIZE);

	const auto offsets = shader.get_active_uniform_offsets({"u_bone_matrices"});
	const GLint matrix_offset = offsets.empty() ? 0 : offsets[0];

	BonesUbo result;
	result.ubo = std::make_unique<gl::UniformBuffer>(
		ctx, static_cast<std::size_t>(ubo_size));
	result.matrix_offset = matrix_offset;
	result.max_bones = (ubo_size > matrix_offset)
		? static_cast<std::size_t>((ubo_size - matrix_offset) / 64)
		: 0;

	const std::size_t init_count = std::min(bone_count, result.max_bones);
	for (std::size_t i = 0; i < init_count; i++) {
		result.ubo->set_mat4(
			static_cast<std::size_t>(matrix_offset) + i * 64,
			IDENTITY_MAT4.data());
	}

	return result;
}

}
