/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "M3RendererGL.h"

#include "../../core.h"
#include "../Shaders.h"
#include "../../buffer.h"

#include "../../ui/texture-ribbon.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>

static constexpr std::array<float, 16> M3_IDENTITY_MAT4 = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

// -----------------------------------------------------------------------
// M3RendererGL constructor
// -----------------------------------------------------------------------

M3RendererGL::M3RendererGL(BufferWrapper& data, gl::GLContext& gl_context, bool reactive, bool useRibbon)
	: data_ptr(&data)
	, ctx(gl_context)
	, reactive(reactive)
	, useRibbon(useRibbon)
{
	model_matrix = M3_IDENTITY_MAT4;
}

// -----------------------------------------------------------------------
// load_shaders
// -----------------------------------------------------------------------

gl::ShaderProgram* M3RendererGL::load_shaders(gl::GLContext& ctx) {
	return shaders::create_program(ctx, "m2");
}

// -----------------------------------------------------------------------
// load
// -----------------------------------------------------------------------

void M3RendererGL::load() {
	m3 = std::make_unique<M3Loader>(*data_ptr);
	m3->load();

	shader = M3RendererGL::load_shaders(ctx);

	_create_default_texture();

	if (!m3->vertices.empty())
		loadLOD(0);

	data_ptr = nullptr;
}

// -----------------------------------------------------------------------
// _create_default_texture
// -----------------------------------------------------------------------

void M3RendererGL::_create_default_texture() {
	const uint8_t pixels[] = {87, 175, 226, 255}; // 0x57afe2 blue
	default_texture = std::make_unique<gl::GLTexture>(ctx);
	gl::TextureOptions opts;
	opts.has_alpha = false;
	default_texture->set_rgba(pixels, 1, 1, opts);
}

// -----------------------------------------------------------------------
// loadLOD
// -----------------------------------------------------------------------

void M3RendererGL::loadLOD(int index) {
	_dispose_geometry();

	// build interleaved vertex buffer matching M2 format
	// format: position(3f) + normal(3f) + bone_idx(4ub) + bone_weight(4ub) + uv(2f) = 40 bytes
	const size_t vertex_count = m3->vertices.size() / 3;
	const size_t stride = 40;
	std::vector<uint8_t> vertex_data(vertex_count * stride, 0);

	for (size_t i = 0; i < vertex_count; i++) {
		const size_t offset = i * stride;
		const size_t v_idx = i * 3;
		const size_t uv_idx = i * 2;

		// position
		std::memcpy(&vertex_data[offset + 0], &m3->vertices[v_idx], sizeof(float));
		std::memcpy(&vertex_data[offset + 4], &m3->vertices[v_idx + 1], sizeof(float));
		std::memcpy(&vertex_data[offset + 8], &m3->vertices[v_idx + 2], sizeof(float));

		// normal
		const float nx = m3->normals.empty() ? 0.0f : m3->normals[v_idx];
		const float ny = m3->normals.empty() ? 1.0f : m3->normals[v_idx + 1];
		const float nz = m3->normals.empty() ? 0.0f : m3->normals[v_idx + 2];
		std::memcpy(&vertex_data[offset + 12], &nx, sizeof(float));
		std::memcpy(&vertex_data[offset + 16], &ny, sizeof(float));
		std::memcpy(&vertex_data[offset + 20], &nz, sizeof(float));

		// bone indices (all zero - no skinning)
		vertex_data[offset + 24] = 0;
		vertex_data[offset + 25] = 0;
		vertex_data[offset + 26] = 0;
		vertex_data[offset + 27] = 0;

		// bone weights (first weight = 255, rest = 0 for identity transform)
		vertex_data[offset + 28] = 255;
		vertex_data[offset + 29] = 0;
		vertex_data[offset + 30] = 0;
		vertex_data[offset + 31] = 0;

		// texcoord
		const float u = m3->uv.empty() ? 0.0f : m3->uv[uv_idx];
		const float v = m3->uv.empty() ? 0.0f : m3->uv[uv_idx + 1];
		std::memcpy(&vertex_data[offset + 32], &u, sizeof(float));
		std::memcpy(&vertex_data[offset + 36], &v, sizeof(float));
	}

	// create VAO
	auto vao = std::make_unique<gl::VertexArray>(ctx);
	vao->bind();

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertex_data.size()), vertex_data.data(), GL_STATIC_DRAW);
	buffers.push_back(vbo);
	vao->vbo = vbo;

	// use same vertex format as M2
	vao->setup_m2_vertex_format();

	gl::VertexArray* vao_raw = vao.get();
	vaos.push_back(std::move(vao));

	// build draw calls for LOD geosets
	draw_calls.clear();

	const size_t geosets_per_lod = (m3->geosetCountPerLOD > 0) ? static_cast<size_t>(m3->geosetCountPerLOD) : m3->geosets.size();
	const size_t start_geo = static_cast<size_t>(index) * geosets_per_lod;
	const size_t end_geo = std::min(start_geo + geosets_per_lod, m3->geosets.size());

	for (size_t geo_idx = start_geo; geo_idx < end_geo; geo_idx++) {
		const auto& geoset = m3->geosets[geo_idx];

		std::vector<uint16_t> indices(
			m3->indices.begin() + static_cast<ptrdiff_t>(geoset.indexStart),
			m3->indices.begin() + static_cast<ptrdiff_t>(geoset.indexStart + geoset.indexCount)
		);

		GLuint ebo;
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(uint16_t)), indices.data(), GL_STATIC_DRAW);
		buffers.push_back(ebo);

		M3DrawCall dc;
		dc.vao = vao_raw;
		dc.ebo = ebo;
		dc.count = geoset.indexCount;
		dc.visible = true;
		draw_calls.push_back(dc);
	}
}

// -----------------------------------------------------------------------
// updateGeosets
// -----------------------------------------------------------------------

void M3RendererGL::updateGeosets() {
	// no geoset management for M3
}

// -----------------------------------------------------------------------
// updateWireframe
// -----------------------------------------------------------------------

void M3RendererGL::updateWireframe() {
	// handled in render()
}

// -----------------------------------------------------------------------
// getBoundingBox
// -----------------------------------------------------------------------

std::optional<M3RendererGL::BoundingBoxResult> M3RendererGL::getBoundingBox() {
	if (!m3 || m3->vertices.empty())
		return std::nullopt;

	const auto& verts = m3->vertices;
	BoundingBoxResult bb;
	bb.min = {std::numeric_limits<float>::infinity(),
	          std::numeric_limits<float>::infinity(),
	          std::numeric_limits<float>::infinity()};
	bb.max = {-std::numeric_limits<float>::infinity(),
	          -std::numeric_limits<float>::infinity(),
	          -std::numeric_limits<float>::infinity()};

	for (size_t i = 0; i < verts.size(); i += 3) {
		bb.min[0] = std::min(bb.min[0], verts[i]);
		bb.min[1] = std::min(bb.min[1], verts[i + 1]);
		bb.min[2] = std::min(bb.min[2], verts[i + 2]);
		bb.max[0] = std::max(bb.max[0], verts[i]);
		bb.max[1] = std::max(bb.max[1], verts[i + 1]);
		bb.max[2] = std::max(bb.max[2], verts[i + 2]);
	}

	return bb;
}

// -----------------------------------------------------------------------
// render
// -----------------------------------------------------------------------

void M3RendererGL::render(const float* view_matrix, const float* projection_matrix) {
	if (!shader || draw_calls.empty())
		return;

	const bool wireframe = core::view->config.value("modelViewerWireframe", false);

	shader->use();

	// scene uniforms
	shader->set_uniform_mat4("u_view_matrix", false, view_matrix);
	shader->set_uniform_mat4("u_projection_matrix", false, projection_matrix);
	shader->set_uniform_mat4("u_model_matrix", false, model_matrix.data());
	shader->set_uniform_3f("u_view_up", 0, 1, 0);

	const float time = static_cast<float>(
		std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()
	);
	shader->set_uniform_1f("u_time", time);

	// set identity bone matrix for bone 0 (M3 has no skeleton)
	shader->set_uniform_1i("u_bone_count", 1);
	const GLint loc = shader->get_uniform_location("u_bone_matrices");
	if (loc != -1)
		glUniformMatrix4fv(loc, 1, GL_FALSE, M3_IDENTITY_MAT4.data());

	// texture matrix defaults
	shader->set_uniform_1i("u_has_tex_matrix1", 0);
	shader->set_uniform_1i("u_has_tex_matrix2", 0);
	shader->set_uniform_mat4("u_tex_matrix1", false, M3_IDENTITY_MAT4.data());
	shader->set_uniform_mat4("u_tex_matrix2", false, M3_IDENTITY_MAT4.data());

	// lighting
	const float lx = 3, ly = -0.7f, lz = -2;
	const float light_view_x = view_matrix[0] * lx + view_matrix[4] * ly + view_matrix[8] * lz;
	const float light_view_y = view_matrix[1] * lx + view_matrix[5] * ly + view_matrix[9] * lz;
	const float light_view_z = view_matrix[2] * lx + view_matrix[6] * ly + view_matrix[10] * lz;

	shader->set_uniform_1i("u_apply_lighting", 1);
	shader->set_uniform_3f("u_ambient_color", 0.5f, 0.5f, 0.5f);
	shader->set_uniform_3f("u_diffuse_color", 0.7f, 0.7f, 0.7f);
	shader->set_uniform_3f("u_light_dir", light_view_x, light_view_y, light_view_z);

	// wireframe
	shader->set_uniform_1i("u_wireframe", wireframe ? 1 : 0);
	shader->set_uniform_4f("u_wireframe_color", 1, 1, 1, 1);

	// alpha test
	shader->set_uniform_1f("u_alpha_test", 0.501960814f);

	// texture samplers
	shader->set_uniform_1i("u_texture1", 0);
	shader->set_uniform_1i("u_texture2", 1);
	shader->set_uniform_1i("u_texture3", 2);
	shader->set_uniform_1i("u_texture4", 3);

	// material settings (opaque)
	shader->set_uniform_1i("u_vertex_shader", 0);
	shader->set_uniform_1i("u_pixel_shader", 0);
	shader->set_uniform_1i("u_blend_mode", 0);
	shader->set_uniform_4f("u_mesh_color", 1, 1, 1, 1);
	shader->set_uniform_3f("u_tex_sample_alpha", 1, 1, 1);

	// bind default texture
	default_texture->bind(0);
	default_texture->bind(1);
	default_texture->bind(2);
	default_texture->bind(3);

	ctx.set_blend(false);
	ctx.set_depth_test(true);
	ctx.set_cull_face(false);

	// render each draw call
	for (const auto& dc : draw_calls) {
		if (!dc.visible)
			continue;

		dc.vao->bind();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dc.ebo);
		glDrawElements(
			wireframe ? GL_LINES : GL_TRIANGLES,
			static_cast<GLsizei>(dc.count),
			GL_UNSIGNED_SHORT,
			nullptr
		);
	}

	ctx.set_cull_face(false);
}

// -----------------------------------------------------------------------
// _dispose_geometry
// -----------------------------------------------------------------------

void M3RendererGL::_dispose_geometry() {
	for (auto& vao : vaos)
		vao->dispose();

	for (GLuint buf : buffers)
		glDeleteBuffers(1, &buf);

	vaos.clear();
	buffers.clear();
	draw_calls.clear();
}

// -----------------------------------------------------------------------
// dispose
// -----------------------------------------------------------------------

void M3RendererGL::dispose() {
	_dispose_geometry();

	if (default_texture) {
		default_texture->dispose();
		default_texture.reset();
	}
}
