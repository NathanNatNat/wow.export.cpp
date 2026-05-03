/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "MDXRendererGL.h"

#include "../../core.h"
#include "../../log.h"

#include "../../casc/blp.h"
#include "../../mpq/mpq-install.h"
#include "../Shaders.h"

#include "../../ui/texture-ribbon.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <format>
#include <set>

static void mat4_multiply(float* out, const float* a, const float* b) {
	const float a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3];
	const float a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7];
	const float a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11];
	const float a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

	float b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
	out[0] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
	out[1] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
	out[2] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
	out[3] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

	b0 = b[4]; b1 = b[5]; b2 = b[6]; b3 = b[7];
	out[4] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
	out[5] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
	out[6] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
	out[7] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

	b0 = b[8]; b1 = b[9]; b2 = b[10]; b3 = b[11];
	out[8] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
	out[9] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
	out[10] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
	out[11] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

	b0 = b[12]; b1 = b[13]; b2 = b[14]; b3 = b[15];
	out[12] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
	out[13] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
	out[14] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
	out[15] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;
}

static void mat4_from_translation(float* out, float x, float y, float z) {
	out[0] = 1; out[1] = 0; out[2] = 0; out[3] = 0;
	out[4] = 0; out[5] = 1; out[6] = 0; out[7] = 0;
	out[8] = 0; out[9] = 0; out[10] = 1; out[11] = 0;
	out[12] = x; out[13] = y; out[14] = z; out[15] = 1;
}

static void mat4_from_quat(float* out, float x, float y, float z, float w) {
	const float x2 = x + x, y2 = y + y, z2 = z + z;
	const float xx = x * x2, xy = x * y2, xz = x * z2;
	const float yy = y * y2, yz = y * z2, zz = z * z2;
	const float wx = w * x2, wy = w * y2, wz = w * z2;

	out[0] = 1 - (yy + zz);
	out[1] = xy + wz;
	out[2] = xz - wy;
	out[3] = 0;
	out[4] = xy - wz;
	out[5] = 1 - (xx + zz);
	out[6] = yz + wx;
	out[7] = 0;
	out[8] = xz + wy;
	out[9] = yz - wx;
	out[10] = 1 - (xx + yy);
	out[11] = 0;
	out[12] = 0;
	out[13] = 0;
	out[14] = 0;
	out[15] = 1;
}

static void mat4_from_scale(float* out, float x, float y, float z) {
	out[0] = x; out[1] = 0; out[2] = 0; out[3] = 0;
	out[4] = 0; out[5] = y; out[6] = 0; out[7] = 0;
	out[8] = 0; out[9] = 0; out[10] = z; out[11] = 0;
	out[12] = 0; out[13] = 0; out[14] = 0; out[15] = 1;
}

static void mat4_copy(float* out, const float* src) {
	std::memcpy(out, src, 16 * sizeof(float));
}

static float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

static void quat_slerp(float* out, float ax, float ay, float az, float aw, float bx, float by, float bz, float bw, float t) {
	float cosom = ax * bx + ay * by + az * bz + aw * bw;

	if (cosom < 0) {
		cosom = -cosom;
		bx = -bx; by = -by; bz = -bz; bw = -bw;
	}

	float scale0, scale1;
	if (1 - cosom > 0.000001f) {
		const float omega = std::acos(cosom);
		const float sinom = std::sin(omega);
		scale0 = std::sin((1 - t) * omega) / sinom;
		scale1 = std::sin(t * omega) / sinom;
	} else {
		scale0 = 1 - t;
		scale1 = t;
	}

	out[0] = scale0 * ax + scale1 * bx;
	out[1] = scale0 * ay + scale1 * by;
	out[2] = scale0 * az + scale1 * bz;
	out[3] = scale0 * aw + scale1 * bw;
}

static const auto MDX_PERFORMANCE_BASELINE = std::chrono::steady_clock::now();

static std::array<float, 3> anim_value_to_vec3(const MDXAnimValue& val) {
	if (const auto* vec = std::get_if<std::vector<float>>(&val)) {
		if (vec->size() >= 3)
			return {(*vec)[0], (*vec)[1], (*vec)[2]};
		const float f = vec->empty() ? 0.0f : (*vec)[0];
		return {f, f, f};
	}
	if (const auto* f = std::get_if<float>(&val))
		return {*f, *f, *f};
	if (const auto* i = std::get_if<int32_t>(&val)) {
		const float fv = static_cast<float>(*i);
		return {fv, fv, fv};
	}
	return {0, 0, 0};
}

static std::array<float, 4> anim_value_to_vec4(const MDXAnimValue& val) {
	if (const auto* vec = std::get_if<std::vector<float>>(&val)) {
		if (vec->size() >= 4)
			return {(*vec)[0], (*vec)[1], (*vec)[2], (*vec)[3]};
		return {0, 0, 0, 1};
	}
	return {0, 0, 0, 1};
}

MDXRendererGL::MDXRendererGL(BufferWrapper& data, gl::GLContext& gl_context, bool reactive, bool useRibbon)
	: data_ptr(&data)
	, ctx(gl_context)
	, reactive(reactive)
	, useRibbon(useRibbon)
{
	mdx = nullptr;
	syncID = -1;

	// rendering

	// animation
	current_animation = -1;
	animation_time = 0;
	animation_paused = false;

	// reactive
	geosetKey = "modelViewerGeosets";

	// transforms
	std::copy(MDX_IDENTITY_MAT4.begin(), MDX_IDENTITY_MAT4.end(), model_matrix.begin());
	position = {0, 0, 0};
	rotation = {0, 0, 0};
	scale_val = {1, 1, 1};
}

std::unique_ptr<gl::ShaderProgram> MDXRendererGL::load_shaders(gl::GLContext& ctx) {
	return shaders::create_program(ctx, "m2");
}

void MDXRendererGL::load() {
	mdx = std::make_unique<MDXLoader>(*data_ptr);
	mdx->load();

	shader = MDXRendererGL::load_shaders(ctx);

	_create_default_texture();
	_load_textures();
	_create_skeleton();
	_build_geometry();

	if (reactive) {
		core::view->modelViewerGeosets.clear();
		for (const auto& entry : geosetArray) {
			nlohmann::json j;
			j["label"] = entry.label;
			j["checked"] = entry.checked;
			j["id"] = entry.id;
			core::view->modelViewerGeosets.push_back(j);
		}
		auto& geosets = core::view->modelViewerGeosets;
		watcher_geoset_checked.resize(geosets.size());
		for (size_t i = 0; i < geosets.size(); i++)
			watcher_geoset_checked[i] = geosets[i].value("checked", false);
		watcher_state_initialized = true;
	}

	data_ptr = nullptr;
}

void MDXRendererGL::_create_default_texture() {
	const uint8_t pixels[4] = {87, 175, 226, 255};
	default_texture = std::make_unique<gl::GLTexture>(ctx);
	gl::TextureOptions opts;
	opts.has_alpha = false;
	default_texture->set_rgba(pixels, 1, 1, opts);
}

void MDXRendererGL::_load_textures() {
	auto& tex_list = mdx->textures;
	mpq::MPQInstall* mpq = core::view->mpq.get();

	if (useRibbon)
		syncID = texture_ribbon::reset();

	for (size_t i = 0, n = tex_list.size(); i < n; i++) {
		auto& texture = tex_list[i];
		int ribbonSlot = useRibbon ? texture_ribbon::addSlot() : -1;

		// mdx uses image filename directly
		const std::string& fileName = texture.image;
		if (!fileName.empty()) {
			if (ribbonSlot >= 0)
				texture_ribbon::setSlotFileLegacy(ribbonSlot, fileName, syncID);

			try {
				if (!mpq)
					continue;

				auto file_data = mpq->getFile(fileName);
				if (file_data.has_value()) {
					casc::BLPImage blp(BufferWrapper(std::move(file_data.value())));
					auto gl_tex = std::make_unique<gl::GLTexture>(ctx);

					const GLenum wrap_s = (texture.flags & 1) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
					const GLenum wrap_t = (texture.flags & 2) ? GL_REPEAT : GL_CLAMP_TO_EDGE;

					auto pixels = blp.toUInt8Array(0, 0b1111);
					gl::TextureOptions opts;
					opts.wrap_s = wrap_s;
					opts.wrap_t = wrap_t;
					opts.has_alpha = blp.alphaDepth > 0;
					opts.generate_mipmaps = true;
					gl_tex->set_rgba(pixels.data(), blp.width, blp.height, opts);

					textures[static_cast<int>(i)] = std::move(gl_tex);

					if (ribbonSlot >= 0)
						texture_ribbon::setSlotSrc(ribbonSlot, blp.getDataURL(0b0111), syncID);
				}
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to load MDX texture {}: {}", fileName, e.what()));
			}
		}
	}
}

void MDXRendererGL::_create_skeleton() {
	const auto& src_nodes = mdx->nodes;

	if (src_nodes.empty()) {
		nodes.clear();
		return;
	}

	// flatten nodes array (may have gaps)
	nodes.clear();
	int maxId = 0;
	for (size_t i = 0; i < src_nodes.size(); i++) {
		if (src_nodes[i]) {
			nodes.push_back(src_nodes[i]);
			if (src_nodes[i]->objectId.has_value() && src_nodes[i]->objectId.value() > maxId)
				maxId = src_nodes[i]->objectId.value();
		}
	}

	node_matrices.resize(static_cast<size_t>((maxId + 1)) * 16);
	for (int i = 0; i <= maxId; i++)
		std::copy(MDX_IDENTITY_MAT4.begin(), MDX_IDENTITY_MAT4.end(), node_matrices.begin() + static_cast<ptrdiff_t>(i) * 16);
}

void MDXRendererGL::_build_geometry() {
	if (reactive)
		geosetArray.clear();

	bones_ubo = renderer_utils::create_bones_ubo(*shader, ctx, nodes.size());

	for (size_t g = 0; g < mdx->geosets.size(); g++) {
		const auto& geoset = mdx->geosets[g];

		// convert vertices (mdx uses different coordinate system)
		const size_t vertCount = geoset.vertices.size() / 3;
		std::vector<float> vertices(vertCount * 3);
		std::vector<float> normals(vertCount * 3);

		for (size_t i = 0; i < vertCount; i++) {
			// x, y, z -> x, z, -y (convert to webgl y-up)
			vertices[i * 3] = geoset.vertices[i * 3];
			vertices[i * 3 + 1] = geoset.vertices[i * 3 + 2];
			vertices[i * 3 + 2] = -geoset.vertices[i * 3 + 1];

			normals[i * 3] = geoset.normals[i * 3];
			normals[i * 3 + 1] = geoset.normals[i * 3 + 2];
			normals[i * 3 + 2] = -geoset.normals[i * 3 + 1];
		}

		std::vector<float> uvs;
		if (!geoset.tVertices.empty() && !geoset.tVertices[0].empty()) {
			uvs = geoset.tVertices[0];
		} else {
			uvs.resize(vertCount * 2, 0.0f);
		}

		// create VAO
		auto vao = std::make_unique<gl::VertexArray>(ctx);
		vao->bind();

		// vertex buffer
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);
		buffers.push_back(vbo);

		// normal buffer
		GLuint nbo;
		glGenBuffers(1, &nbo);
		glBindBuffer(GL_ARRAY_BUFFER, nbo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(normals.size() * sizeof(float)), normals.data(), GL_STATIC_DRAW);
		buffers.push_back(nbo);

		// uv buffer
		GLuint uvo;
		glGenBuffers(1, &uvo);
		glBindBuffer(GL_ARRAY_BUFFER, uvo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(uvs.size() * sizeof(float)), uvs.data(), GL_STATIC_DRAW);
		buffers.push_back(uvo);

		// bone index/weight (mdx uses group-based skinning, simplified for now)
		std::vector<uint8_t> boneIndices(vertCount * 4, 0);
		std::vector<uint8_t> boneWeights(vertCount * 4, 0);

		const std::vector<int32_t> default_bone_group{0};

		for (size_t i = 0; i < vertCount; i++) {
			const uint8_t groupIdx = (i < geoset.vertexGroup.size()) ? geoset.vertexGroup[i] : 0;
			const std::vector<int32_t>& group = (groupIdx < geoset.groups.size())
				? geoset.groups[groupIdx]
				: default_bone_group;

			// assign up to 4 bones per vertex with equal weight
			const size_t boneCount = std::min(group.size(), static_cast<size_t>(4));
			const uint8_t weight = static_cast<uint8_t>(255 / (boneCount > 0 ? boneCount : 1));

			for (size_t b = 0; b < 4; b++) {
				if (b < boneCount) {
					boneIndices[i * 4 + b] = static_cast<uint8_t>(group[b] >= 0 ? group[b] : 0);
					boneWeights[i * 4 + b] = weight;
				} else {
					boneIndices[i * 4 + b] = 0;
					boneWeights[i * 4 + b] = 0;
				}
			}
		}

		GLuint bibo;
		glGenBuffers(1, &bibo);
		glBindBuffer(GL_ARRAY_BUFFER, bibo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(boneIndices.size()), boneIndices.data(), GL_STATIC_DRAW);
		buffers.push_back(bibo);

		GLuint bwbo;
		glGenBuffers(1, &bwbo);
		glBindBuffer(GL_ARRAY_BUFFER, bwbo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(boneWeights.size()), boneWeights.data(), GL_STATIC_DRAW);
		buffers.push_back(bwbo);

		// index buffer
		GLuint ebo;
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(geoset.faces.size() * sizeof(uint16_t)), geoset.faces.data(), GL_STATIC_DRAW);
		buffers.push_back(ebo);
		vao->ebo = ebo;

		// wireframe index buffer
		{
			auto line_indices = gl::VertexArray::triangles_to_lines(geoset.faces.data(), geoset.faces.size());
			vao->set_wireframe_index_buffer(line_indices.data(), line_indices.size());
		}

		vao->setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo);

		// material/texture
		int textureId = -1;
		int blendMode = 0;
		bool twoSided = false;

		if (geoset.materialId >= 0 && static_cast<size_t>(geoset.materialId) < mdx->materials.size()) {
			const auto& material = mdx->materials[static_cast<size_t>(geoset.materialId)];
			if (!material.layers.empty()) {
				const auto& layer = material.layers[0];
				textureId = layer.textureId;
				blendMode = layer.filterMode;
				twoSided = !!(layer.shading & 0x10);
			}
		}

		MDXDrawCall drawCall;
		drawCall.vao = vao.get();
		drawCall.start = 0;
		drawCall.count = static_cast<uint32_t>(geoset.faces.size());
		drawCall.textureId = textureId;
		drawCall.blendMode = blendMode;
		drawCall.twoSided = twoSided;
		drawCall.visible = true;

		draw_calls.push_back(drawCall);

		vaos.push_back(std::move(vao));

		if (reactive) {
			MDXGeosetEntry entry;
			entry.label = "Geoset " + std::to_string(g);
			entry.checked = true;
			entry.id = static_cast<int>(g);
			geosetArray.push_back(entry);
		}
	}
}

void MDXRendererGL::playAnimation(int index) {
	current_animation = index;
	animation_time = 0;
}

void MDXRendererGL::stopAnimation() {
	current_animation = -1;
	animation_time = 0;
	animation_paused = false;

	if (!nodes.empty() && node_matrices.size() >= 16) {
		std::copy(MDX_IDENTITY_MAT4.begin(), MDX_IDENTITY_MAT4.end(), node_matrices.begin());
	}
}

int MDXRendererGL::get_animation_frame_count() {
	if (current_animation < 0 || !mdx)
		return 1;

	if (current_animation >= static_cast<int>(mdx->sequences.size()))
		return 1;

	const auto& seq = mdx->sequences[static_cast<size_t>(current_animation)];
	const float duration_ms = static_cast<float>(seq.interval[1]) - static_cast<float>(seq.interval[0]);
	if (duration_ms <= 0)
		return 1;

	return std::max(1, static_cast<int>(std::floor((duration_ms / 1000.0f) * 60.0f)));
}

int MDXRendererGL::get_animation_frame() {
	if (current_animation < 0 || !mdx)
		return 0;

	if (current_animation >= static_cast<int>(mdx->sequences.size()))
		return 0;

	const auto& seq = mdx->sequences[static_cast<size_t>(current_animation)];
	const float duration_ms = static_cast<float>(seq.interval[1]) - static_cast<float>(seq.interval[0]);
	if (duration_ms <= 0)
		return 0;

	return static_cast<int>(std::floor((animation_time / duration_ms) * static_cast<float>(get_animation_frame_count())));
}

void MDXRendererGL::set_animation_frame(int frame) {
	if (current_animation < 0 || !mdx)
		return;

	if (current_animation >= static_cast<int>(mdx->sequences.size()))
		return;

	const int frame_count = get_animation_frame_count();
	if (frame_count <= 0)
		return;

	const auto& seq = mdx->sequences[static_cast<size_t>(current_animation)];
	const float duration_ms = static_cast<float>(seq.interval[1]) - static_cast<float>(seq.interval[0]);
	animation_time = (static_cast<float>(frame) / static_cast<float>(frame_count)) * duration_ms;

	if (!nodes.empty())
		_update_node_matrices();
}

void MDXRendererGL::step_animation_frame(int delta) {
	const int frame = get_animation_frame();
	const int frame_count = get_animation_frame_count();
	int new_frame = frame + delta;

	if (new_frame < 0)
		new_frame = frame_count - 1;
	else if (new_frame >= frame_count)
		new_frame = 0;

	set_animation_frame(new_frame);
}

void MDXRendererGL::updateAnimation(float delta_time) {
	if (current_animation < 0 || nodes.empty())
		return;

	if (current_animation >= static_cast<int>(mdx->sequences.size()))
		return;

	const auto& seq = mdx->sequences[static_cast<size_t>(current_animation)];

	if (!animation_paused) {
		animation_time += delta_time * 1000.0f; // convert to ms

		const float duration = static_cast<float>(seq.interval[1]) - static_cast<float>(seq.interval[0]);
		if (duration > 0) {
			while (animation_time >= duration)
				animation_time -= duration;
		}
	}

	_update_node_matrices();
}

void MDXRendererGL::_update_node_matrices() {
	const float frame = static_cast<float>(mdx->sequences[static_cast<size_t>(current_animation)].interval[0]) + animation_time;

	float local_mat[16];
	float trans_mat[16];
	float rot_mat[16];
	float scale_mat[16];
	float pivot_mat[16];
	float neg_pivot_mat[16];
	float temp_result[16];

	std::set<int> calculated;

	std::function<void(MDXNode*)> calc_node = [&](MDXNode* node) {
		if (!node || !node->objectId.has_value())
			return;

		const int obj_id = node->objectId.value();
		if (calculated.count(obj_id))
			return;

		// calc parent first
		if (node->parent.has_value()) {
			const int parent_id = node->parent.value();
			if (parent_id >= 0 && static_cast<size_t>(parent_id) < mdx->nodes.size() && mdx->nodes[static_cast<size_t>(parent_id)])
				calc_node(mdx->nodes[static_cast<size_t>(parent_id)]);
		}

		const auto& pivot = node->pivotPoint;
		// convert pivot (same coord conversion)
		const float px = !pivot.empty() ? pivot[0] : 0.0f;
		const float py = pivot.size() >= 3 ? pivot[2] : 0.0f;
		const float pz = pivot.size() >= 2 ? -pivot[1] : 0.0f;

		mat4_copy(local_mat, MDX_IDENTITY_MAT4.data());

		const bool has_trans = node->translation.has_value() && !node->translation->keys.empty();
		const bool has_rot = node->rotation.has_value() && !node->rotation->keys.empty();
		const bool has_scale = node->scale.has_value() && !node->scale->keys.empty();

		if (has_trans || has_rot || has_scale) {
			mat4_from_translation(pivot_mat, px, py, pz);
			mat4_multiply(temp_result, local_mat, pivot_mat);
			mat4_copy(local_mat, temp_result);

			if (has_trans) {
				auto [tx, ty, tz] = _sample_vec3(node->translation.value(), frame);
				mat4_from_translation(trans_mat, tx, tz, -ty);
				mat4_multiply(temp_result, local_mat, trans_mat);
				mat4_copy(local_mat, temp_result);
			}

			if (has_rot) {
				auto [qx, qy, qz, qw] = _sample_quat(node->rotation.value(), frame);
				mat4_from_quat(rot_mat, qx, qz, -qy, qw);
				mat4_multiply(temp_result, local_mat, rot_mat);
				mat4_copy(local_mat, temp_result);
			}

			if (has_scale) {
				auto [sx, sy, sz] = _sample_vec3(node->scale.value(), frame);
				mat4_from_scale(scale_mat, sx, sz, sy);
				mat4_multiply(temp_result, local_mat, scale_mat);
				mat4_copy(local_mat, temp_result);
			}

			mat4_from_translation(neg_pivot_mat, -px, -py, -pz);
			mat4_multiply(temp_result, local_mat, neg_pivot_mat);
			mat4_copy(local_mat, temp_result);
		}

		const size_t offset = static_cast<size_t>(obj_id) * 16;
		if (node->parent.has_value()) {
			const int parent_id = node->parent.value();
			if (parent_id >= 0 && static_cast<size_t>(parent_id) < mdx->nodes.size() && mdx->nodes[static_cast<size_t>(parent_id)]) {
				const size_t parentOffset = static_cast<size_t>(parent_id) * 16;
				mat4_multiply(&node_matrices[offset], &node_matrices[parentOffset], local_mat);
			} else {
				std::copy(local_mat, local_mat + 16, node_matrices.begin() + static_cast<ptrdiff_t>(offset));
			}
		} else {
			std::copy(local_mat, local_mat + 16, node_matrices.begin() + static_cast<ptrdiff_t>(offset));
		}

		calculated.insert(obj_id);
	};

	for (auto* node : nodes)
		calc_node(node);
}

std::array<float, 3> MDXRendererGL::_sample_vec3(const MDXAnimVector& track, float frame) {
	const auto& keys = track.keys;
	if (keys.empty())
		return {0, 0, 0};

	if (keys.size() == 1 || frame <= static_cast<float>(keys[0].frame)) {
		return anim_value_to_vec3(keys[0].value);
	}

	if (frame >= static_cast<float>(keys[keys.size() - 1].frame)) {
		return anim_value_to_vec3(keys[keys.size() - 1].value);
	}

	size_t idx = 0;
	for (size_t i = 0; i < keys.size() - 1; i++) {
		if (frame >= static_cast<float>(keys[i].frame) && frame < static_cast<float>(keys[i + 1].frame)) {
			idx = i;
			break;
		}
	}

	const auto& k0 = keys[idx];
	const auto& k1 = keys[idx + 1];
	const float t = (frame - static_cast<float>(k0.frame)) / (static_cast<float>(k1.frame) - static_cast<float>(k0.frame));

	const auto v0 = anim_value_to_vec3(k0.value);
	const auto v1 = anim_value_to_vec3(k1.value);

	return {
		lerp(v0[0], v1[0], t),
		lerp(v0[1], v1[1], t),
		lerp(v0[2], v1[2], t)
	};
}

std::array<float, 4> MDXRendererGL::_sample_quat(const MDXAnimVector& track, float frame) {
	const auto& keys = track.keys;
	if (keys.empty())
		return {0, 0, 0, 1};

	if (keys.size() == 1 || frame <= static_cast<float>(keys[0].frame))
		return anim_value_to_vec4(keys[0].value);

	if (frame >= static_cast<float>(keys[keys.size() - 1].frame))
		return anim_value_to_vec4(keys[keys.size() - 1].value);

	size_t idx = 0;
	for (size_t i = 0; i < keys.size() - 1; i++) {
		if (frame >= static_cast<float>(keys[i].frame) && frame < static_cast<float>(keys[i + 1].frame)) {
			idx = i;
			break;
		}
	}

	const auto& k0 = keys[idx];
	const auto& k1 = keys[idx + 1];
	const float t = (frame - static_cast<float>(k0.frame)) / (static_cast<float>(k1.frame) - static_cast<float>(k0.frame));

	const auto q0 = anim_value_to_vec4(k0.value);
	const auto q1 = anim_value_to_vec4(k1.value);

	std::array<float, 4> out = {0, 0, 0, 1};
	quat_slerp(out.data(), q0[0], q0[1], q0[2], q0[3], q1[0], q1[1], q1[2], q1[3], t);
	return out;
}

void MDXRendererGL::updateGeosets() {
	if (!reactive || draw_calls.empty())
		return;

	auto& geosets = core::view->modelViewerGeosets;
	const bool has_view_geosets = !geosets.empty();
	const size_t source_size = has_view_geosets ? geosets.size() : geosetArray.size();

	for (size_t i = 0; i < source_size && i < draw_calls.size(); i++) {
		const bool checked = has_view_geosets
			? geosets[i].value("checked", (i < geosetArray.size() ? geosetArray[i].checked : false))
			: geosetArray[i].checked;
		if (i < geosetArray.size())
			geosetArray[i].checked = checked;
		draw_calls[i].visible = checked;
	}
}

void MDXRendererGL::setTransform(const std::array<float, 3>& position, const std::array<float, 3>& rotation, const std::array<float, 3>& scale) {
	this->position = position;
	this->rotation = rotation;
	this->scale_val = scale;
	_update_model_matrix();
}

void MDXRendererGL::setTransformQuat(const std::array<float, 3>& position, const std::array<float, 4>& quat, const std::array<float, 3>& scale) {
	const float px = position[0], py = position[1], pz = position[2];
	const float qx = quat[0], qy = quat[1], qz = quat[2], qw = quat[3];
	const float sx = scale[0], sy = scale[1], sz = scale[2];

	const float x2 = qx + qx, y2 = qy + qy, z2 = qz + qz;
	const float xx = qx * x2, xy = qx * y2, xz = qx * z2;
	const float yy = qy * y2, yz = qy * z2, zz = qz * z2;
	const float wx = qw * x2, wy = qw * y2, wz = qw * z2;

	auto& m = model_matrix;
	m[0] = (1 - (yy + zz)) * sx;
	m[1] = (xy + wz) * sx;
	m[2] = (xz - wy) * sx;
	m[3] = 0;
	m[4] = (xy - wz) * sy;
	m[5] = (1 - (xx + zz)) * sy;
	m[6] = (yz + wx) * sy;
	m[7] = 0;
	m[8] = (xz + wy) * sz;
	m[9] = (yz - wx) * sz;
	m[10] = (1 - (xx + yy)) * sz;
	m[11] = 0;
	m[12] = px;
	m[13] = py;
	m[14] = pz;
	m[15] = 1;
}

void MDXRendererGL::_update_model_matrix() {
	auto& m = model_matrix;
	const float px = position[0], py = position[1], pz = position[2];
	const float rx = rotation[0], ry = rotation[1], rz = rotation[2];
	const float sx = scale_val[0], sy = scale_val[1], sz = scale_val[2];

	const float cx = std::cos(rx), sinx = std::sin(rx);
	const float cy = std::cos(ry), siny = std::sin(ry);
	const float cz = std::cos(rz), sinz = std::sin(rz);

	m[0] = cy * cz * sx;
	m[1] = cy * sinz * sx;
	m[2] = -siny * sx;
	m[3] = 0;

	m[4] = (sinx * siny * cz - cx * sinz) * sy;
	m[5] = (sinx * siny * sinz + cx * cz) * sy;
	m[6] = sinx * cy * sy;
	m[7] = 0;

	m[8] = (cx * siny * cz + sinx * sinz) * sz;
	m[9] = (cx * siny * sinz - sinx * cz) * sz;
	m[10] = cx * cy * sz;
	m[11] = 0;

	m[12] = px;
	m[13] = py;
	m[14] = pz;
	m[15] = 1;
}

void MDXRendererGL::render(const float* view_matrix, const float* projection_matrix) {
	if (!shader || draw_calls.empty())
		return;

	if (reactive) {
		auto& geosets = core::view->modelViewerGeosets;
		bool geosets_changed = !watcher_state_initialized || geosets.size() != watcher_geoset_checked.size();
		if (!geosets_changed) {
			for (size_t i = 0; i < geosets.size(); i++) {
				if (geosets[i].value("checked", false) != watcher_geoset_checked[i]) {
					geosets_changed = true;
					break;
				}
			}
		}
		if (geosets_changed) {
			updateGeosets();
			watcher_geoset_checked.resize(geosets.size());
			for (size_t i = 0; i < geosets.size(); i++)
				watcher_geoset_checked[i] = geosets[i].value("checked", false);
		}
		watcher_state_initialized = true;
	}

	const bool wireframe = core::view->config.value("modelViewerWireframe", false);

	shader->use();

	shader->set_uniform_mat4("u_view_matrix", false, view_matrix);
	shader->set_uniform_mat4("u_projection_matrix", false, projection_matrix);
	shader->set_uniform_mat4("u_model_matrix", false, model_matrix.data());
	shader->set_uniform_3f("u_view_up", 0, 1, 0);

	float time_sec = std::chrono::duration<float>(std::chrono::steady_clock::now() - MDX_PERFORMANCE_BASELINE).count();
	shader->set_uniform_1f("u_time", time_sec);

	// bone matrices (mdx uses node-based skeleton)
	const std::size_t node_count = (!nodes.empty() && bones_ubo.ubo)
		? std::min(nodes.size(), bones_ubo.max_bones)
		: 0;
	shader->set_uniform_1i("u_bone_count", static_cast<int>(node_count));
	if (node_count > 0 && !node_matrices.empty()) {
		bones_ubo.ubo->set_mat4_array(
			static_cast<std::size_t>(bones_ubo.matrix_offset),
			node_matrices.data(), node_count);
		bones_ubo.ubo->upload();
		bones_ubo.ubo->bind(0);
	}

	shader->set_uniform_mat4("u_tex_matrix1", false, MDX_IDENTITY_MAT4.data());
	shader->set_uniform_mat4("u_tex_matrix2", false, MDX_IDENTITY_MAT4.data());

	// lighting
	const float lx = 3, ly = -0.7f, lz = -2;
	const float light_view_x = view_matrix[0] * lx + view_matrix[4] * ly + view_matrix[8] * lz;
	const float light_view_y = view_matrix[1] * lx + view_matrix[5] * ly + view_matrix[9] * lz;
	const float light_view_z = view_matrix[2] * lx + view_matrix[6] * ly + view_matrix[10] * lz;

	shader->set_uniform_1i("u_apply_lighting", 1);
	shader->set_uniform_3f("u_ambient_color", 0.5f, 0.5f, 0.5f);
	shader->set_uniform_3f("u_diffuse_color", 0.7f, 0.7f, 0.7f);
	shader->set_uniform_3f("u_light_dir", light_view_x, light_view_y, light_view_z);

	shader->set_uniform_1i("u_wireframe", wireframe ? 1 : 0);
	shader->set_uniform_4f("u_wireframe_color", 1, 1, 1, 1);

	shader->set_uniform_1f("u_alpha_test", 0.501960814f);

	shader->set_uniform_1i("u_texture1", 0);
	shader->set_uniform_1i("u_texture2", 1);
	shader->set_uniform_1i("u_texture3", 2);
	shader->set_uniform_1i("u_texture4", 3);

	shader->set_uniform_3f("u_tex_sample_alpha", 1, 1, 1);

	// use basic shader mode for mdx
	shader->set_uniform_1i("u_vertex_shader", 0);
	shader->set_uniform_1i("u_pixel_shader", 0);
	shader->set_uniform_4f("u_mesh_color", 1, 1, 1, 1);

	ctx.set_depth_test(true);
	ctx.set_depth_write(true);

	for (const auto& dc : draw_calls) {
		if (!dc.visible)
			continue;

		// map mdx blend modes to m2 blend modes
		shader->set_uniform_1i("u_blend_mode", dc.blendMode);
		ctx.apply_blend_mode(dc.blendMode);

		if (dc.twoSided) {
			ctx.set_cull_face(false);
		} else {
			ctx.set_cull_face(true);
			ctx.set_cull_mode(GL_BACK);
		}

		// bind texture
		gl::GLTexture* texture = nullptr;
		if (dc.textureId >= 0) {
			auto it = textures.find(dc.textureId);
			texture = (it != textures.end()) ? it->second.get() : default_texture.get();
		} else {
			texture = default_texture.get();
		}

		texture->bind(0);
		default_texture->bind(1);
		default_texture->bind(2);
		default_texture->bind(3);

		dc.vao->bind();
		if (wireframe) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dc.vao->wireframe_ebo);
			glDrawElements(GL_LINES,
			               static_cast<GLsizei>(dc.count * 2),
			               GL_UNSIGNED_SHORT,
			               reinterpret_cast<void*>(static_cast<uintptr_t>(dc.start) * 4));
		} else {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dc.vao->ebo);
			glDrawElements(GL_TRIANGLES,
			               static_cast<GLsizei>(dc.count),
			               GL_UNSIGNED_SHORT,
			               reinterpret_cast<void*>(static_cast<uintptr_t>(dc.start) * 2));
		}
	}

	ctx.set_blend(false);
	ctx.set_depth_test(true);
	ctx.set_depth_write(true);
	ctx.set_cull_face(false);
}

std::optional<MDXRendererGL::BoundingBoxResult> MDXRendererGL::getBoundingBox() {
	if (!mdx)
		return std::nullopt;

	const auto& info = mdx->info;
	if (info.minExtent.empty() || info.maxExtent.empty())
		return std::nullopt;

	// convert coords
	BoundingBoxResult result;
	result.min = {info.minExtent[0], info.minExtent[2], -info.maxExtent[1]};
	result.max = {info.maxExtent[0], info.maxExtent[2], -info.minExtent[1]};
	return result;
}

void MDXRendererGL::dispose() {
	watcher_geoset_checked.clear();
	watcher_state_initialized = false;

	for (auto& vao : vaos)
		vao->dispose();

	for (auto buf : buffers)
		glDeleteBuffers(1, &buf);

	if (bones_ubo.ubo) {
		bones_ubo.ubo->dispose();
		bones_ubo.ubo.reset();
	}

	for (auto& [key, tex] : textures)
		tex->dispose();

	textures.clear();

	if (default_texture) {
		default_texture->dispose();
		default_texture.reset();
	}

	vaos.clear();
	buffers.clear();
	draw_calls.clear();

	if (!geosetArray.empty())
		geosetArray.clear();
}
