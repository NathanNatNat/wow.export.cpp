/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "ShadowPlaneRenderer.h"
#include "../gl/GLContext.h"
#include "../gl/ShaderProgram.h"

#include <vector>

static const char* SHADOW_VERT_SHADER = R"(#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;

out vec2 v_uv;

void main() {
	gl_Position = u_projection_matrix * u_view_matrix * vec4(a_position, 1.0);
	v_uv = a_uv;
}
)";

static const char* SHADOW_FRAG_SHADER = R"(#version 460 core

in vec2 v_uv;
out vec4 frag_color;

uniform float u_shadow_radius;

void main() {
	vec2 center = vec2(0.5, 0.5);
	float dist = distance(v_uv, center) * 2.0;
	float alpha = smoothstep(1.0, 0.0, dist / (u_shadow_radius / 10.0));
	frag_color = vec4(0.0, 0.0, 0.0, alpha * 0.6);
}
)";

ShadowPlaneRenderer::ShadowPlaneRenderer(gl::GLContext& gl_context, float size)
	: ctx(gl_context), size(size)
{
	_init();
}

void ShadowPlaneRenderer::_init() {
	_create_shader();
	_create_geometry();
}

void ShadowPlaneRenderer::_create_shader() {
	shader = std::make_unique<gl::ShaderProgram>(ctx, SHADOW_VERT_SHADER, SHADOW_FRAG_SHADER);
}

void ShadowPlaneRenderer::_create_geometry() {
	const float half = size / 2.0f;

	// quad vertices: position (xyz) + uv (st)
	const float vertices[] = {
		-half, 0.0f, -half, 0.0f, 0.0f,
		 half, 0.0f, -half, 1.0f, 0.0f,
		 half, 0.0f,  half, 1.0f, 1.0f,
		-half, 0.0f,  half, 0.0f, 1.0f
	};

	const uint16_t indices[] = {
		0, 1, 2,
		0, 2, 3
	};

	// create VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// vertex buffer
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// index buffer
	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute (location 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, reinterpret_cast<void*>(0));

	// uv attribute (location 1)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, reinterpret_cast<void*>(12));

	glBindVertexArray(0);
}

/**
 * @param view_matrix
 * @param projection_matrix
 */
void ShadowPlaneRenderer::render(const float* view_matrix, const float* projection_matrix) {
	if (!visible || !shader || !shader->is_valid())
		return;

	// enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	shader->use();
	shader->set_uniform_mat4("u_view_matrix", false, view_matrix);
	shader->set_uniform_mat4("u_projection_matrix", false, projection_matrix);
	shader->set_uniform_1f("u_shadow_radius", shadow_radius);

	ctx.bind_vao(vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

	// restore state
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void ShadowPlaneRenderer::dispose() {
	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}

	if (vertex_buffer) {
		glDeleteBuffers(1, &vertex_buffer);
		vertex_buffer = 0;
	}

	if (index_buffer) {
		glDeleteBuffers(1, &index_buffer);
		index_buffer = 0;
	}

	if (shader) {
		shader->dispose();
		shader.reset();
	}
}
