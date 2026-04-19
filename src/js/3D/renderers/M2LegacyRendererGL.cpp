/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "M2LegacyRendererGL.h"

#include "../../core.h"
#include "../../log.h"

#include "../../casc/blp.h"
#include "../../mpq/mpq-install.h"
#include "../GeosetMapper.h"
#include "../Shaders.h"

#include "../../ui/texture-ribbon.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <format>
#include <functional>
#include <optional>

// -----------------------------------------------------------------------
// Free-function math helpers (matching JS module-level functions exactly)
// -----------------------------------------------------------------------

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

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
static float track_value_to_float(const LegacyTrackValue& v) {
	if (auto* val = std::get_if<uint8_t>(&v))
		return static_cast<float>(*val);
	if (auto* val = std::get_if<uint32_t>(&v))
		return static_cast<float>(*val);
	if (auto* val = std::get_if<int16_t>(&v))
		return static_cast<float>(*val);
	return 0.0f;
}

static const std::vector<float>& track_value_to_vec(const LegacyTrackValue& v) {
	static const std::vector<float> empty;
	if (auto* val = std::get_if<std::vector<float>>(&v))
		return *val;
	return empty;
}

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

M2LegacyRendererGL::M2LegacyRendererGL(BufferWrapper& data, gl::GLContext& gl_context, bool reactive, bool useRibbon)
	: data_ptr(&data)
	, ctx(gl_context)
	, reactive(reactive)
	, useRibbon(useRibbon)
{
	m2 = nullptr;
	syncID = -1;

	// rendering state
	// vaos, textures, default_texture, buffers, draw_calls: default constructed

	// animation state
	bones = nullptr;
	// bone_matrices: default constructed
	current_animation = -1; // null equivalent
	animation_time = 0;
	animation_paused = false;

	// reactive state
	geosetKey = "modelViewerGeosets";
	// geosetArray: default constructed

	// transforms
	std::copy(IDENTITY_MAT4.begin(), IDENTITY_MAT4.end(), model_matrix.begin());
	position = {0, 0, 0};
	rotation = {0, 0, 0};
	scale_val = {1, 1, 1};

	// material data
	// material_props: default constructed
}

// -----------------------------------------------------------------------
// static load_shaders
// -----------------------------------------------------------------------

std::unique_ptr<gl::ShaderProgram> M2LegacyRendererGL::load_shaders(gl::GLContext& ctx) {
	return shaders::create_program(ctx, "m2");
}

// -----------------------------------------------------------------------
// load
// -----------------------------------------------------------------------

void M2LegacyRendererGL::load() {
	m2 = std::make_unique<M2LegacyLoader>(*data_ptr);
	m2->load().get();

	shader = M2LegacyRendererGL::load_shaders(ctx);

	_create_default_texture();
	_load_textures();

	if (!m2->vertices.empty()) {
		loadSkin(0);

		if (reactive) {
			// JS: this.geosetWatcher = core.view.$watch(this.geosetKey, () => this.updateGeosets(), { deep: true })
			// JS: this.wireframeWatcher = core.view.$watch('config.modelViewerWireframe', () => {}, { deep: true })
			// C++: initialize polling baseline after loadSkin() populates core::view->modelViewerGeosets
			auto& geosets = core::view->modelViewerGeosets;
			watcher_geoset_checked.resize(geosets.size());
			for (size_t i = 0; i < geosets.size(); i++)
				watcher_geoset_checked[i] = geosets[i].value("checked", false);
			watcher_state_initialized = true;
		}
	}

	data_ptr = nullptr;
}

// -----------------------------------------------------------------------
// _create_default_texture
// -----------------------------------------------------------------------

void M2LegacyRendererGL::_create_default_texture() {
	const uint8_t pixels[4] = {87, 175, 226, 255};
	default_texture = std::make_unique<gl::GLTexture>(ctx);
	gl::TextureOptions opts;
	opts.has_alpha = false;
	default_texture->set_rgba(pixels, 1, 1, opts);
}

// -----------------------------------------------------------------------
// _load_textures
// -----------------------------------------------------------------------

void M2LegacyRendererGL::_load_textures() {
	auto& tex_list = m2->textures;
	mpq::MPQInstall* mpq = core::view->mpq.get();

	if (useRibbon)
		syncID = texture_ribbon::reset();

	for (size_t i = 0, n = tex_list.size(); i < n; i++) {
		auto& texture = tex_list[i];
		int ribbonSlot = useRibbon ? texture_ribbon::addSlot() : -1;

		// legacy textures use fileName property set by loader
		const std::string& fileName = texture.fileName;

		if (!fileName.empty()) {
			if (ribbonSlot >= 0)
				texture_ribbon::setSlotFileLegacy(ribbonSlot, fileName, syncID);

			try {
				// MPQ file access — get texture file data
				std::optional<BufferWrapper> file_data;
				if (mpq) {
					auto raw = mpq->getFile(fileName);
					if (raw.has_value())
						file_data = BufferWrapper(std::move(raw.value()));
				}

				if (file_data.has_value()) {
					casc::BLPImage blp(std::move(file_data.value()));
					auto gl_tex = std::make_unique<gl::GLTexture>(ctx);
					gl::BLPTextureFlags blp_flags;
					blp_flags.flags = texture.flags;
					gl_tex->set_blp(blp, blp_flags);
					textures[static_cast<int>(i)] = std::move(gl_tex);

					if (ribbonSlot >= 0)
						texture_ribbon::setSlotSrc(ribbonSlot, blp.getDataURL(0b0111), syncID);
				}
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to load legacy texture {}: {}", fileName, e.what()));
			}
		}
	}
}

// -----------------------------------------------------------------------
// applyCreatureSkin
// -----------------------------------------------------------------------

void M2LegacyRendererGL::applyCreatureSkin(const std::vector<std::string>& texture_paths) {
	mpq::MPQInstall* mpq = core::view->mpq.get();
	const auto& textureTypes = m2->textureTypes;

	// creature skins map to textureType 11, 12, 13 (Monster1, Monster2, Monster3)
	const uint32_t MONSTER_TEX_START = 11;

	for (size_t i = 0; i < textureTypes.size(); i++) {
		const uint32_t textureType = textureTypes[i];

		// check if this is a monster replaceable texture slot
		if (textureType >= MONSTER_TEX_START && textureType < MONSTER_TEX_START + 3) {
			const uint32_t skin_index = textureType - MONSTER_TEX_START;

			if (skin_index < texture_paths.size()) {
				const std::string& texture_path = texture_paths[skin_index];

				try {
					if (!mpq)
						continue;

					auto file_data = mpq->getFile(texture_path);
					if (file_data.has_value()) {
						casc::BLPImage blp(BufferWrapper(std::move(file_data.value())));
						auto gl_tex = std::make_unique<gl::GLTexture>(ctx);
						gl::BLPTextureFlags blp_flags;
						blp_flags.flags = m2->textures[i].flags;
						gl_tex->set_blp(blp, blp_flags);
						textures[static_cast<int>(i)] = std::move(gl_tex);

						if (useRibbon) {
							texture_ribbon::setSlotFileLegacy(static_cast<int>(i), texture_path, syncID);
							texture_ribbon::setSlotSrc(static_cast<int>(i), blp.getDataURL(0b0111), syncID);
						}

						logging::write(std::format("Applied creature skin texture {}: {}", static_cast<int>(i), texture_path));
					}
				} catch (const std::exception& e) {
					logging::write(std::format("Failed to apply creature skin texture {}: {}", texture_path, e.what()));
				}
			}
		}
	}
}

// -----------------------------------------------------------------------
// loadSkin
// -----------------------------------------------------------------------

void M2LegacyRendererGL::loadSkin(int index) {
	_dispose_skin();

	auto* skin_ptr = m2->getSkin(index).get();
	if (!skin_ptr)
		throw std::runtime_error(std::format("Invalid legacy skin index: {}", index));
	auto& skin = *skin_ptr;

	_create_skeleton();

	// build interleaved vertex buffer
	// format: position(3f) + normal(3f) + bone_idx(4ub) + bone_weight(4ub) + uv(2f) = 40 bytes
	const size_t vertex_count = m2->vertices.size() / 3;
	const size_t stride = 40;
	std::vector<uint8_t> vertex_data(vertex_count * stride);

	for (size_t i = 0; i < vertex_count; i++) {
		const size_t offset = i * stride;
		const size_t v_idx = i * 3;
		const size_t uv_idx = i * 2;
		const size_t bone_idx = i * 4;

		// position
		std::memcpy(&vertex_data[offset + 0], &m2->vertices[v_idx], sizeof(float));
		std::memcpy(&vertex_data[offset + 4], &m2->vertices[v_idx + 1], sizeof(float));
		std::memcpy(&vertex_data[offset + 8], &m2->vertices[v_idx + 2], sizeof(float));

		// normal
		std::memcpy(&vertex_data[offset + 12], &m2->normals[v_idx], sizeof(float));
		std::memcpy(&vertex_data[offset + 16], &m2->normals[v_idx + 1], sizeof(float));
		std::memcpy(&vertex_data[offset + 20], &m2->normals[v_idx + 2], sizeof(float));

		// bone indices
		vertex_data[offset + 24] = m2->boneIndices[bone_idx];
		vertex_data[offset + 25] = m2->boneIndices[bone_idx + 1];
		vertex_data[offset + 26] = m2->boneIndices[bone_idx + 2];
		vertex_data[offset + 27] = m2->boneIndices[bone_idx + 3];

		// bone weights
		vertex_data[offset + 28] = m2->boneWeights[bone_idx];
		vertex_data[offset + 29] = m2->boneWeights[bone_idx + 1];
		vertex_data[offset + 30] = m2->boneWeights[bone_idx + 2];
		vertex_data[offset + 31] = m2->boneWeights[bone_idx + 3];

		// texcoord
		std::memcpy(&vertex_data[offset + 32], &m2->uv[uv_idx], sizeof(float));
		std::memcpy(&vertex_data[offset + 36], &m2->uv[uv_idx + 1], sizeof(float));
	}

	// map triangle indices
	std::vector<uint16_t> index_data(skin.triangles.size());
	for (size_t i = 0; i < skin.triangles.size(); i++)
		index_data[i] = skin.indices[skin.triangles[i]];

	// create VAO
	auto vao = std::make_unique<gl::VertexArray>(ctx);
	vao->bind();

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertex_data.size()), vertex_data.data(), GL_STATIC_DRAW);
	this->buffers.push_back(vbo);
	vao->vbo = vbo;

	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(index_data.size() * sizeof(uint16_t)), index_data.data(), GL_STATIC_DRAW);
	this->buffers.push_back(ebo);
	vao->ebo = ebo;

	vao->setup_m2_vertex_format();

	gl::VertexArray* vao_raw = vao.get();
	vaos.push_back(std::move(vao));

	if (reactive)
		geosetArray.resize(skin.subMeshes.size());

	// build draw calls per submesh
	draw_calls.clear();

	for (size_t i = 0; i < skin.subMeshes.size(); i++) {
		const auto& submesh = skin.subMeshes[i];

		// find texture unit matching this submesh
		const LegacyM2TextureUnit* tex_unit = nullptr;
		for (const auto& tu : skin.textureUnits) {
			if (tu.skinSectionIndex == static_cast<uint16_t>(i)) {
				tex_unit = &tu;
				break;
			}
		}

		std::array<int, 4> tex_indices = { -1, -1, -1, -1 };
		int vertex_shader = LEGACY_VERTEX_SHADER;
		int pixel_shader = LEGACY_PIXEL_SHADER;
		int blend_mode = 0;
		uint16_t flags = 0;
		uint16_t texture_count = 1;

		if (tex_unit) {
			texture_count = tex_unit->textureCount;

			for (uint16_t j = 0; j < std::min(texture_count, static_cast<uint16_t>(4)); j++) {
				const uint16_t combo_idx = tex_unit->textureComboIndex + j;
				if (combo_idx < m2->textureCombos.size())
					tex_indices[j] = static_cast<int>(m2->textureCombos[combo_idx]);
			}

			if (tex_unit->materialIndex < m2->materials.size()) {
				const auto& mat = m2->materials[tex_unit->materialIndex];
				blend_mode = mat.blendingMode;
				flags = mat.flags;
				material_props[tex_indices[0]] = { blend_mode, flags };

				// use Combiners_Mod (1) for alpha-using blend modes
				if (blend_mode > 0)
					pixel_shader = 1;
			}
		}

		M2LegacyDrawCall draw_call;
		draw_call.vao = vao_raw;
		draw_call.start = submesh.triangleStart;
		draw_call.count = submesh.triangleCount;
		draw_call.tex_indices = tex_indices;
		draw_call.texture_count = texture_count;
		draw_call.vertex_shader = vertex_shader;
		draw_call.pixel_shader = pixel_shader;
		draw_call.blend_mode = blend_mode;
		draw_call.flags = flags;
		draw_call.visible = true;

		if (reactive) {
			const std::string id_str = std::to_string(submesh.submeshID);
			bool is_default = (submesh.submeshID == 0 ||
				(id_str.size() >= 2 && id_str.substr(id_str.size() - 2) == "01") ||
				(id_str.size() >= 2 && id_str.substr(0, 2) == "32"));
			if ((id_str.size() >= 2 && id_str.substr(0, 2) == "17") ||
				(id_str.size() >= 2 && id_str.substr(0, 2) == "35"))
				is_default = false;

			M2LegacyGeosetEntry entry;
			entry.label = "Geoset " + std::to_string(i);
			entry.checked = is_default;
			entry.id = submesh.submeshID;
			geosetArray[i] = entry;
			draw_call.visible = is_default;
		}

		draw_calls.push_back(draw_call);
	}

	if (reactive) {
		auto& geosets = core::view->modelViewerGeosets;
		geosets.clear();
		for (const auto& entry : geosetArray) {
			nlohmann::json j;
			j["label"] = entry.label;
			j["checked"] = entry.checked;
			j["id"] = entry.id;
			geosets.push_back(j);
		}

		std::vector<geoset_mapper::Geoset> mapper_geosets;
		for (const auto& entry : geosetArray) {
			geoset_mapper::Geoset g;
			g.id = entry.id;
			g.label = entry.label;
			mapper_geosets.push_back(g);
		}
		geoset_mapper::map(mapper_geosets);

		// update labels back
		for (size_t i = 0; i < mapper_geosets.size() && i < geosetArray.size(); i++) {
			geosetArray[i].label = mapper_geosets[i].label;
		}

		// update core state labels too
		for (size_t i = 0; i < mapper_geosets.size() && i < geosets.size(); i++) {
			geosets[i]["label"] = mapper_geosets[i].label;
		}
	}

	updateGeosets();
}

// -----------------------------------------------------------------------
// _create_skeleton
// -----------------------------------------------------------------------

void M2LegacyRendererGL::_create_skeleton() {
	auto& bone_data = m2->bones;

	if (bone_data.empty()) {
		bones = nullptr;
		bone_matrices.assign(16, 0.0f);
		return;
	}

	bones = &bone_data;
	bone_matrices.resize(bone_data.size() * 16);

	for (size_t i = 0; i < bone_data.size(); i++)
		std::copy(IDENTITY_MAT4.begin(), IDENTITY_MAT4.end(), bone_matrices.begin() + static_cast<ptrdiff_t>(i * 16));
}

// -----------------------------------------------------------------------
// playAnimation
// -----------------------------------------------------------------------

void M2LegacyRendererGL::playAnimation(int index) {
	current_animation = index;
	animation_time = 0;
}

// -----------------------------------------------------------------------
// stopAnimation
// -----------------------------------------------------------------------

void M2LegacyRendererGL::stopAnimation() {
	animation_time = 0;
	animation_paused = false;

	// calculate bone matrices using animation 0 (stand) at time 0 for rest pose
	if (bones) {
		current_animation = 0;
		_update_bone_matrices();
		current_animation = -1; // null
	}
}

// -----------------------------------------------------------------------
// updateAnimation
// -----------------------------------------------------------------------

void M2LegacyRendererGL::updateAnimation(float delta_time) {
	if (current_animation < 0 || !bones)
		return;

	if (static_cast<size_t>(current_animation) >= m2->animations.size())
		return;

	const auto& anim = m2->animations[current_animation];

	if (!animation_paused)
		animation_time += delta_time;

	const float duration_sec = static_cast<float>(anim.duration) / 1000.0f;
	if (duration_sec > 0)
		animation_time = std::fmod(animation_time, duration_sec);

	_update_bone_matrices();
}

// -----------------------------------------------------------------------
// get_animation_duration
// -----------------------------------------------------------------------

float M2LegacyRendererGL::get_animation_duration() {
	if (current_animation < 0)
		return 0;

	if (static_cast<size_t>(current_animation) >= m2->animations.size())
		return 0;

	const auto& anim = m2->animations[current_animation];
	return static_cast<float>(anim.duration) / 1000.0f;
}

// -----------------------------------------------------------------------
// get_animation_frame_count
// -----------------------------------------------------------------------

int M2LegacyRendererGL::get_animation_frame_count() {
	const float duration = get_animation_duration();
	return std::max(1, static_cast<int>(std::floor(duration * 60)));
}

// -----------------------------------------------------------------------
// get_animation_frame
// -----------------------------------------------------------------------

int M2LegacyRendererGL::get_animation_frame() {
	const float duration = get_animation_duration();
	if (duration <= 0)
		return 0;

	return static_cast<int>(std::floor((animation_time / duration) * static_cast<float>(get_animation_frame_count())));
}

// -----------------------------------------------------------------------
// set_animation_frame
// -----------------------------------------------------------------------

void M2LegacyRendererGL::set_animation_frame(int frame) {
	const int frame_count = get_animation_frame_count();
	if (frame_count <= 0)
		return;

	const float duration = get_animation_duration();
	animation_time = (static_cast<float>(frame) / static_cast<float>(frame_count)) * duration;
	_update_bone_matrices();
}

// -----------------------------------------------------------------------
// set_animation_paused
// -----------------------------------------------------------------------

void M2LegacyRendererGL::set_animation_paused(bool paused) {
	animation_paused = paused;
}

// -----------------------------------------------------------------------
// step_animation_frame
// -----------------------------------------------------------------------

void M2LegacyRendererGL::step_animation_frame(int delta) {
	const int frame = get_animation_frame();
	const int frame_count = get_animation_frame_count();
	int new_frame = frame + delta;

	if (new_frame < 0)
		new_frame = frame_count - 1;
	else if (new_frame >= frame_count)
		new_frame = 0;

	set_animation_frame(new_frame);
}

// -----------------------------------------------------------------------
// _update_bone_matrices
// -----------------------------------------------------------------------

void M2LegacyRendererGL::_update_bone_matrices() {
	const float time_ms = animation_time * 1000.0f;
	const auto& bone_list = *bones;
	const size_t bone_count = bone_list.size();
	const int anim_idx = current_animation;

	// for legacy single-timeline, get animation time range
	uint32_t anim_start = 0;
	uint32_t anim_end = 0;
	if (m2->version < M2_LEGACY_VER_WOTLK && anim_idx >= 0 && static_cast<size_t>(anim_idx) < m2->animations.size()) {
		anim_start = m2->animations[anim_idx].startTimestamp;
		anim_end = m2->animations[anim_idx].endTimestamp;
	}

	float local_mat[16];
	float trans_mat[16];
	float rot_mat[16];
	float scale_mat[16];
	float pivot_mat[16];
	float neg_pivot_mat[16];
	float temp_result[16];

	std::vector<bool> calculated(bone_count, false);

	// Recursive bone calculation (using a lambda with capture for recursion)
	std::function<void(size_t)> calc_bone = [&](size_t idx) {
		if (calculated[idx])
			return;

		const auto& bone = bone_list[idx];
		const int16_t parent_idx = bone.parentBone;

		if (parent_idx >= 0 && static_cast<size_t>(parent_idx) < bone_count)
			calc_bone(static_cast<size_t>(parent_idx));

		const auto& pivot = bone.pivot;
		const float px = pivot[0], py = pivot[1], pz = pivot[2];

		// check for animation data
		bool has_trans = false, has_rot = false, has_scale = false, has_scale_fallback = false;

		if (m2->version < M2_LEGACY_VER_WOTLK) {
			// legacy single-timeline
			has_trans = !bone.translation.flatValues.empty();
			has_rot = !bone.rotation.flatValues.empty();
			has_scale = !bone.scale.flatValues.empty();
		} else {
			// per-animation timeline
			if (anim_idx >= 0) {
				has_trans = static_cast<size_t>(anim_idx) < bone.translation.nestedTimestamps.size() &&
					!bone.translation.nestedTimestamps[anim_idx].empty();
				has_rot = static_cast<size_t>(anim_idx) < bone.rotation.nestedTimestamps.size() &&
					!bone.rotation.nestedTimestamps[anim_idx].empty();
				has_scale = static_cast<size_t>(anim_idx) < bone.scale.nestedTimestamps.size() &&
					!bone.scale.nestedTimestamps[anim_idx].empty();
				has_scale_fallback = !has_scale && anim_idx != 0 &&
					!bone.scale.nestedTimestamps.empty() &&
					!bone.scale.nestedTimestamps[0].empty();
			}
		}

		const bool has_animation = has_trans || has_rot || has_scale || has_scale_fallback;

		mat4_copy(local_mat, IDENTITY_MAT4.data());

		if (has_animation) {
			mat4_from_translation(pivot_mat, px, py, pz);
			mat4_multiply(temp_result, local_mat, pivot_mat);
			mat4_copy(local_mat, temp_result);

			if (has_trans) {
				float tx, ty, tz;
				if (m2->version < M2_LEGACY_VER_WOTLK) {
					auto result = _sample_legacy_vec3(bone.translation, time_ms, anim_start, anim_end);
					tx = result[0]; ty = result[1]; tz = result[2];
				} else {
					const auto& ts = bone.translation.nestedTimestamps[anim_idx];
					const auto& vals = bone.translation.nestedValues[anim_idx];
					auto result = _sample_vec3(ts, vals, time_ms);
					tx = result[0]; ty = result[1]; tz = result[2];
				}

				mat4_from_translation(trans_mat, tx, ty, tz);
				mat4_multiply(temp_result, local_mat, trans_mat);
				mat4_copy(local_mat, temp_result);
			}

			if (has_rot) {
				float qx, qy, qz, qw;
				if (m2->version < M2_LEGACY_VER_WOTLK) {
					auto result = _sample_legacy_quat(bone.rotation, time_ms, anim_start, anim_end);
					qx = result[0]; qy = result[1]; qz = result[2]; qw = result[3];
				} else {
					const auto& ts = bone.rotation.nestedTimestamps[anim_idx];
					const auto& vals = bone.rotation.nestedValues[anim_idx];
					auto result = _sample_quat(ts, vals, time_ms);
					qx = result[0]; qy = result[1]; qz = result[2]; qw = result[3];
				}

				mat4_from_quat(rot_mat, qx, qy, qz, qw);
				mat4_multiply(temp_result, local_mat, rot_mat);
				mat4_copy(local_mat, temp_result);
			}

			// apply scale (fallback to animation 0 if current animation lacks scale data)
			if (has_scale || has_scale_fallback) {
				float sx, sy, sz;
				if (m2->version < M2_LEGACY_VER_WOTLK) {
					auto result = _sample_legacy_vec3(bone.scale, time_ms, anim_start, anim_end, {1, 1, 1});
					sx = result[0]; sy = result[1]; sz = result[2];
				} else {
					const int scale_anim_idx = has_scale ? anim_idx : 0;
					const auto& ts = bone.scale.nestedTimestamps[scale_anim_idx];
					const auto& vals = bone.scale.nestedValues[scale_anim_idx];
					const float scale_time = has_scale ? time_ms : 0;
					auto result = _sample_vec3(ts, vals, scale_time, {1, 1, 1});
					sx = result[0]; sy = result[1]; sz = result[2];
				}

				mat4_from_scale(scale_mat, sx, sy, sz);
				mat4_multiply(temp_result, local_mat, scale_mat);
				mat4_copy(local_mat, temp_result);
			}

			mat4_from_translation(neg_pivot_mat, -px, -py, -pz);
			mat4_multiply(temp_result, local_mat, neg_pivot_mat);
			mat4_copy(local_mat, temp_result);
		}

		const size_t offset = idx * 16;
		if (parent_idx >= 0 && static_cast<size_t>(parent_idx) < bone_count) {
			const size_t parent_offset = static_cast<size_t>(parent_idx) * 16;
			mat4_multiply(&bone_matrices[offset], &bone_matrices[parent_offset], local_mat);
		} else {
			std::copy(local_mat, local_mat + 16, bone_matrices.begin() + static_cast<ptrdiff_t>(offset));
		}

		calculated[idx] = true;
	};

	for (size_t i = 0; i < bone_count; i++)
		calc_bone(i);
}

// -----------------------------------------------------------------------
// _sample_legacy_vec3
// legacy single-timeline sampling: uses ranges array to find keyframes within animation bounds
// -----------------------------------------------------------------------

std::array<float, 3> M2LegacyRendererGL::_sample_legacy_vec3(const LegacyM2Track& track, float time_ms, uint32_t anim_start, uint32_t anim_end, const std::array<float, 3>& default_value) {
	const auto& timestamps = track.flatTimestamps;
	const auto& values = track.flatValues;
	const auto& ranges = track.ranges;

	if (timestamps.empty())
		return default_value;

	// absolute time in global timeline
	const float abs_time = static_cast<float>(anim_start) + time_ms;

	// find keyframes within this animation's range
	// size_t start_idx = 0;
	// size_t end_idx = timestamps.size() - 1;

	// use ranges if available to narrow search
	if (!ranges.empty()) {
		// ranges contains [start, end] pairs for each animation
		// but in legacy format, all animations share single timeline
		// find relevant range by searching
	}

	// find keyframes
	if (timestamps.size() == 1 || abs_time <= track_value_to_float(timestamps[0])) {
		const auto& v = track_value_to_vec(values[0]);
		if (v.size() >= 3)
			return {v[0], v[1], v[2]};
		return default_value;
	}

	if (abs_time >= track_value_to_float(timestamps[timestamps.size() - 1])) {
		const auto& v = track_value_to_vec(values[values.size() - 1]);
		if (v.size() >= 3)
			return {v[0], v[1], v[2]};
		return default_value;
	}

	size_t frame = 0;
	for (size_t i = 0; i < timestamps.size() - 1; i++) {
		if (abs_time >= track_value_to_float(timestamps[i]) && abs_time < track_value_to_float(timestamps[i + 1])) {
			frame = i;
			break;
		}
	}

	const float t0 = track_value_to_float(timestamps[frame]);
	const float t1 = track_value_to_float(timestamps[frame + 1]);
	const float alpha = (abs_time - t0) / (t1 - t0);

	const auto& v0 = track_value_to_vec(values[frame]);
	const auto& v1 = track_value_to_vec(values[frame + 1]);

	if (v0.size() >= 3 && v1.size() >= 3) {
		return {
			lerp(v0[0], v1[0], alpha),
			lerp(v0[1], v1[1], alpha),
			lerp(v0[2], v1[2], alpha)
		};
	}

	return default_value;
}

// -----------------------------------------------------------------------
// _sample_legacy_quat
// -----------------------------------------------------------------------

std::array<float, 4> M2LegacyRendererGL::_sample_legacy_quat(const LegacyM2Track& track, float time_ms, uint32_t anim_start, uint32_t anim_end) {
	const auto& timestamps = track.flatTimestamps;
	const auto& values = track.flatValues;

	if (timestamps.empty())
		return {0, 0, 0, 1};

	const float abs_time = static_cast<float>(anim_start) + time_ms;

	if (timestamps.size() == 1 || abs_time <= track_value_to_float(timestamps[0])) {
		const auto& v = track_value_to_vec(values[0]);
		if (v.size() >= 4)
			return {v[0], v[1], v[2], v[3]};
		return {0, 0, 0, 1};
	}

	if (abs_time >= track_value_to_float(timestamps[timestamps.size() - 1])) {
		const auto& v = track_value_to_vec(values[values.size() - 1]);
		if (v.size() >= 4)
			return {v[0], v[1], v[2], v[3]};
		return {0, 0, 0, 1};
	}

	size_t frame = 0;
	for (size_t i = 0; i < timestamps.size() - 1; i++) {
		if (abs_time >= track_value_to_float(timestamps[i]) && abs_time < track_value_to_float(timestamps[i + 1])) {
			frame = i;
			break;
		}
	}

	const float t0 = track_value_to_float(timestamps[frame]);
	const float t1 = track_value_to_float(timestamps[frame + 1]);
	const float alpha = (abs_time - t0) / (t1 - t0);

	const auto& q0 = track_value_to_vec(values[frame]);
	const auto& q1 = track_value_to_vec(values[frame + 1]);

	if (q0.size() >= 4 && q1.size() >= 4) {
		std::array<float, 4> out = {0, 0, 0, 1};
		quat_slerp(out.data(), q0[0], q0[1], q0[2], q0[3], q1[0], q1[1], q1[2], q1[3], alpha);
		return out;
	}

	return {0, 0, 0, 1};
}

// -----------------------------------------------------------------------
// _sample_vec3
// per-animation timeline sampling (wotlk)
// -----------------------------------------------------------------------

std::array<float, 3> M2LegacyRendererGL::_sample_vec3(const std::vector<LegacyTrackValue>& timestamps, const std::vector<LegacyTrackValue>& values, float time_ms, const std::array<float, 3>& default_value) {
	if (timestamps.empty())
		return default_value;

	if (timestamps.size() == 1 || time_ms <= track_value_to_float(timestamps[0])) {
		const auto& v = track_value_to_vec(values[0]);
		if (v.size() >= 3)
			return {v[0], v[1], v[2]};
		return default_value;
	}

	if (time_ms >= track_value_to_float(timestamps[timestamps.size() - 1])) {
		const auto& v = track_value_to_vec(values[values.size() - 1]);
		if (v.size() >= 3)
			return {v[0], v[1], v[2]};
		return default_value;
	}

	size_t frame = 0;
	for (size_t i = 0; i < timestamps.size() - 1; i++) {
		if (time_ms >= track_value_to_float(timestamps[i]) && time_ms < track_value_to_float(timestamps[i + 1])) {
			frame = i;
			break;
		}
	}

	const float t0 = track_value_to_float(timestamps[frame]);
	const float t1 = track_value_to_float(timestamps[frame + 1]);
	const float alpha = (time_ms - t0) / (t1 - t0);

	const auto& v0 = track_value_to_vec(values[frame]);
	const auto& v1 = track_value_to_vec(values[frame + 1]);

	if (v0.size() >= 3 && v1.size() >= 3) {
		return {
			lerp(v0[0], v1[0], alpha),
			lerp(v0[1], v1[1], alpha),
			lerp(v0[2], v1[2], alpha)
		};
	}

	return default_value;
}

// -----------------------------------------------------------------------
// _sample_quat
// -----------------------------------------------------------------------

std::array<float, 4> M2LegacyRendererGL::_sample_quat(const std::vector<LegacyTrackValue>& timestamps, const std::vector<LegacyTrackValue>& values, float time_ms) {
	if (timestamps.empty())
		return {0, 0, 0, 1};

	if (timestamps.size() == 1 || time_ms <= track_value_to_float(timestamps[0])) {
		const auto& v = track_value_to_vec(values[0]);
		if (v.size() >= 4)
			return {v[0], v[1], v[2], v[3]};
		return {0, 0, 0, 1};
	}

	if (time_ms >= track_value_to_float(timestamps[timestamps.size() - 1])) {
		const auto& v = track_value_to_vec(values[values.size() - 1]);
		if (v.size() >= 4)
			return {v[0], v[1], v[2], v[3]};
		return {0, 0, 0, 1};
	}

	size_t frame = 0;
	for (size_t i = 0; i < timestamps.size() - 1; i++) {
		if (time_ms >= track_value_to_float(timestamps[i]) && time_ms < track_value_to_float(timestamps[i + 1])) {
			frame = i;
			break;
		}
	}

	const float t0 = track_value_to_float(timestamps[frame]);
	const float t1 = track_value_to_float(timestamps[frame + 1]);
	const float alpha = (time_ms - t0) / (t1 - t0);

	const auto& q0 = track_value_to_vec(values[frame]);
	const auto& q1 = track_value_to_vec(values[frame + 1]);

	if (q0.size() >= 4 && q1.size() >= 4) {
		std::array<float, 4> out = {0, 0, 0, 1};
		quat_slerp(out.data(), q0[0], q0[1], q0[2], q0[3], q1[0], q1[1], q1[2], q1[3], alpha);
		return out;
	}

	return {0, 0, 0, 1};
}

// -----------------------------------------------------------------------
// updateGeosets
// -----------------------------------------------------------------------

void M2LegacyRendererGL::updateGeosets() {
	if (!reactive || draw_calls.empty())
		return;

	// JS: this.geosetArray is the same reference as core.view[this.geosetKey].
	// C++: read checked state from core::view->modelViewerGeosets (the UI-facing array) and
	//      sync it back into geosetArray so both stay in agreement.
	auto& geosets = core::view->modelViewerGeosets;
	const bool has_view_geosets = !geosets.empty();
	const size_t source_size = has_view_geosets ? geosets.size() : geosetArray.size();
	const size_t count = std::min(draw_calls.size(), source_size);

	for (size_t i = 0; i < count; i++) {
		const bool checked = has_view_geosets
			? geosets[i].value("checked", (i < geosetArray.size() ? geosetArray[i].checked : false))
			: geosetArray[i].checked;

		if (i < geosetArray.size())
			geosetArray[i].checked = checked;

		draw_calls[i].visible = checked;
	}
}

// -----------------------------------------------------------------------
// setTransform
// -----------------------------------------------------------------------

void M2LegacyRendererGL::setTransform(const std::array<float, 3>& position, const std::array<float, 3>& rotation, const std::array<float, 3>& scale) {
	this->position = position;
	this->rotation = rotation;
	this->scale_val = scale;
	_update_model_matrix();
}

// -----------------------------------------------------------------------
// setTransformQuat
// -----------------------------------------------------------------------

void M2LegacyRendererGL::setTransformQuat(const std::array<float, 3>& position, const std::array<float, 4>& quat, const std::array<float, 3>& scale) {
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

// -----------------------------------------------------------------------
// _update_model_matrix
// -----------------------------------------------------------------------

void M2LegacyRendererGL::_update_model_matrix() {
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

// -----------------------------------------------------------------------
// render
// -----------------------------------------------------------------------

void M2LegacyRendererGL::render(const float* view_matrix, const float* projection_matrix) {
	if (!shader || draw_calls.empty())
		return;

	// JS: geosetWatcher — poll core::view->modelViewerGeosets for checked state changes
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

	static const auto s_render_start = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	float time_sec = std::chrono::duration<float>(now - s_render_start).count();
	shader->set_uniform_1f("u_time", time_sec);

	// bone skinning disabled for legacy models until animation system is fixed
	shader->set_uniform_1i("u_bone_count", 0);

	shader->set_uniform_1i("u_has_tex_matrix1", 0);
	shader->set_uniform_1i("u_has_tex_matrix2", 0);
	shader->set_uniform_mat4("u_tex_matrix1", false, IDENTITY_MAT4.data());
	shader->set_uniform_mat4("u_tex_matrix2", false, IDENTITY_MAT4.data());

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

	// sort draw calls: opaque first, then transparent
	std::vector<const M2LegacyDrawCall*> sorted_calls;
	sorted_calls.reserve(draw_calls.size());
	for (const auto& dc : draw_calls)
		sorted_calls.push_back(&dc);

	std::stable_sort(sorted_calls.begin(), sorted_calls.end(), [](const M2LegacyDrawCall* a, const M2LegacyDrawCall* b) {
		const bool a_opaque = a->blend_mode == 0 || a->blend_mode == 1;
		const bool b_opaque = b->blend_mode == 0 || b->blend_mode == 1;
		if (a_opaque != b_opaque)
			return a_opaque;

		return false;
	});

	for (const auto* dc : sorted_calls) {
		if (!dc->visible)
			continue;

		shader->set_uniform_1i("u_vertex_shader", dc->vertex_shader);
		shader->set_uniform_1i("u_pixel_shader", dc->pixel_shader);
		shader->set_uniform_1i("u_blend_mode", dc->blend_mode);

		shader->set_uniform_4f("u_mesh_color", 1, 1, 1, 1);

		ctx.apply_blend_mode(dc->blend_mode);

		if (dc->flags & 0x04) {
			ctx.set_cull_face(false);
		} else {
			ctx.set_cull_face(true);
			ctx.set_cull_mode(GL_BACK);
		}

		if (dc->flags & 0x08)
			ctx.set_depth_test(false);
		else
			ctx.set_depth_test(true);

		for (int t = 0; t < 4; t++) {
			const int tex_idx = dc->tex_indices[t];
			gl::GLTexture* texture = nullptr;
			if (tex_idx >= 0) {
				auto it = textures.find(tex_idx);
				texture = (it != textures.end()) ? it->second.get() : default_texture.get();
			} else {
				texture = default_texture.get();
			}
			if (texture)
				texture->bind(t);
		}

		dc->vao->bind();
		glDrawElements(
			wireframe ? GL_LINES : GL_TRIANGLES,
			dc->count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>(static_cast<uintptr_t>(dc->start * 2))
		);
	}

	ctx.set_blend(false);
	ctx.set_depth_test(true);
	ctx.set_depth_write(true);
	ctx.set_cull_face(false);
}

// -----------------------------------------------------------------------
// getBoundingBox
// -----------------------------------------------------------------------

std::optional<M2LegacyRendererGL::BoundingBoxResult> M2LegacyRendererGL::getBoundingBox() {
	if (!m2 || m2->boundingBox.min.empty())
		return std::nullopt;

	const auto& src_min = m2->boundingBox.min;
	const auto& src_max = m2->boundingBox.max;

	BoundingBoxResult result;
	result.min = {src_min[0], src_min[2], -src_max[1]};
	result.max = {src_max[0], src_max[2], -src_min[1]};
	return result;
}

// -----------------------------------------------------------------------
// _dispose_skin
// -----------------------------------------------------------------------

void M2LegacyRendererGL::_dispose_skin() {
	for (auto& vao : vaos)
		vao->dispose();

	vaos.clear();

	// Delete GPU buffer objects (WebGL GC handles this automatically;
	// in desktop GL we must free explicitly).
	if (!buffers.empty())
		glDeleteBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
	buffers.clear();

	draw_calls.clear();

	if (!geosetArray.empty())
		geosetArray.clear();
}

// -----------------------------------------------------------------------
// dispose
// -----------------------------------------------------------------------

void M2LegacyRendererGL::dispose() {

	_dispose_skin();

	for (auto& [key, tex] : textures)
		tex->dispose();

	textures.clear();

	if (default_texture) {
		default_texture->dispose();
		default_texture.reset();
	}
}
