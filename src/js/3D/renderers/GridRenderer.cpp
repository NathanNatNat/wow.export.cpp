#include "GridRenderer.h"
#include "../gl/GLContext.h"
#include "../gl/ShaderProgram.h"

#include <cmath>
#include <vector>

static const char* GRID_VERT_SHADER = R"(#version 430 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;

out vec3 v_color;

void main() {
	gl_Position = u_projection_matrix * u_view_matrix * vec4(a_position, 1.0);
	v_color = a_color;
}
)";

static const char* GRID_FRAG_SHADER = R"(#version 430 core

in vec3 v_color;
out vec4 frag_color;

void main() {
	frag_color = vec4(v_color, 1.0);
}
)";

// grid colors (matching Three.js GridHelper defaults)
static constexpr float CENTER_COLOR[3] = { 0.34f, 0.68f, 0.89f }; // 0x57afe2
static constexpr float LINE_COLOR[3]   = { 0.5f,  0.5f,  0.5f };  // 0x808080

GridRenderer::GridRenderer(gl::GLContext& gl_context, float size, int divisions)
	: ctx(gl_context), size(size), divisions(divisions)
{
	_init();
}

void GridRenderer::_init() {
	_create_shader();
	_create_geometry();
}

void GridRenderer::_create_shader() {
	shader = std::make_unique<gl::ShaderProgram>(ctx, GRID_VERT_SHADER, GRID_FRAG_SHADER);
}

void GridRenderer::_create_geometry() {
	const float half = size / 2.0f;
	const float step = size / static_cast<float>(divisions);

	std::vector<float> vertices;

	// generate grid lines
	for (int i = 0; i <= divisions; i++) {
		const float pos = -half + i * step;
		const bool is_center = std::abs(pos) < step * 0.5f;
		const float* color = is_center ? CENTER_COLOR : LINE_COLOR;

		// line along Z axis (at x = pos)
		vertices.insert(vertices.end(), { pos, 0.0f, -half, color[0], color[1], color[2] });
		vertices.insert(vertices.end(), { pos, 0.0f, half,  color[0], color[1], color[2] });

		// line along X axis (at z = pos)
		vertices.insert(vertices.end(), { -half, 0.0f, pos, color[0], color[1], color[2] });
		vertices.insert(vertices.end(), { half,  0.0f, pos, color[0], color[1], color[2] });
	}

	vertex_count = static_cast<GLsizei>(vertices.size() / 6);

	// create VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create buffer
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

	// position attribute (location 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, reinterpret_cast<void*>(0));

	// color attribute (location 1)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, reinterpret_cast<void*>(12));

	glBindVertexArray(0);
}

/**
 * @param view_matrix
 * @param projection_matrix
 */
void GridRenderer::render(const float* view_matrix, const float* projection_matrix) {
	if (!shader || !shader->is_valid())
		return;

	shader->use();
	shader->set_uniform_mat4("u_view_matrix", false, view_matrix);
	shader->set_uniform_mat4("u_projection_matrix", false, projection_matrix);

	ctx.bind_vao(vao);
	glDrawArrays(GL_LINES, 0, vertex_count);
}

void GridRenderer::dispose() {
	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}

	if (vertex_buffer) {
		glDeleteBuffers(1, &vertex_buffer);
		vertex_buffer = 0;
	}

	if (shader) {
		shader->dispose();
		shader.reset();
	}
}
