/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "WMOLegacyRendererGL.h"
#include "M2LegacyRendererGL.h"

#include "../../core.h"
#include "../../log.h"
#include "../../buffer.h"

#include "../../casc/blp.h"
#include "../../mpq/mpq-install.h"
#include "../Shaders.h"

#include "../../ui/texture-ribbon.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>

// -----------------------------------------------------------------------
// legacy WMO shaders — simpler than modern WMO shaders
// -----------------------------------------------------------------------

struct WMOLegacyShaderEntry {
	int VertexShader;
	int PixelShader;
};

static constexpr std::array<WMOLegacyShaderEntry, 8> WMO_LEGACY_SHADERS = {{
	{0, 0},   // Diffuse
	{0, 1},   // Specular
	{0, 2},   // Metal
	{0, 0},   // Env
	{0, 0},   // Opaque
	{0, 0},   // EnvMetal
	{0, 0},   // TwoLayerDiffuse
	{0, 0},   // TwoLayerEnvMetal
}};

static const WMOLegacyShaderEntry& get_legacy_wmo_shader(uint32_t shader) {
	if (shader < WMO_LEGACY_SHADERS.size())
		return WMO_LEGACY_SHADERS[shader];
	return WMO_LEGACY_SHADERS[0];
}

static constexpr std::array<float, 16> WMO_LEGACY_IDENTITY_MAT4 = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

// -----------------------------------------------------------------------
// constructor
// -----------------------------------------------------------------------

WMOLegacyRendererGL::WMOLegacyRendererGL(BufferWrapper& data, uint32_t fileID, gl::GLContext& gl_context, bool useRibbon)
	: data_ptr(&data)
	, fileID(fileID)
	, ctx(gl_context)
	, useRibbon(useRibbon)
{
	model_matrix = WMO_LEGACY_IDENTITY_MAT4;
}

WMOLegacyRendererGL::WMOLegacyRendererGL(BufferWrapper& data, const std::string& fileName, gl::GLContext& gl_context, bool useRibbon)
	: data_ptr(&data)
	, fileID(0)
	, fileName(fileName)
	, ctx(gl_context)
	, useRibbon(useRibbon)
{
	model_matrix = WMO_LEGACY_IDENTITY_MAT4;
}

// -----------------------------------------------------------------------
// load_shaders
// -----------------------------------------------------------------------

std::unique_ptr<gl::ShaderProgram> WMOLegacyRendererGL::load_shaders(gl::GLContext& ctx) {
	return shaders::create_program(ctx, "wmo");
}

// -----------------------------------------------------------------------
// load
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::load() {
	if (!fileName.empty())
		wmo = std::make_unique<WMOLegacyLoader>(*data_ptr, fileName, true);
	else
		wmo = std::make_unique<WMOLegacyLoader>(*data_ptr, fileID, true);
	wmo->load();

	shader = WMOLegacyRendererGL::load_shaders(ctx);

	_create_default_texture();
	_load_textures();
	_load_groups();
	_setup_doodad_sets();

	core::view->modelViewerWMOGroups = groupArray;
	core::view->modelViewerWMOSets = setArray;
	{
		const auto& gv = core::view->modelViewerWMOGroups;
		prev_group_checked.resize(gv.size());
		for (size_t i = 0; i < gv.size(); i++)
			prev_group_checked[i] = gv[i].value("checked", true);

		const auto& sv = core::view->modelViewerWMOSets;
		prev_set_checked.resize(sv.size());
		for (size_t i = 0; i < sv.size(); i++)
			prev_set_checked[i] = sv[i].value("checked", false);
	}

	data_ptr = nullptr;
}

// -----------------------------------------------------------------------
// _create_default_texture
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::_create_default_texture() {
	const uint8_t pixels[] = {87, 175, 226, 255};
	default_texture = std::make_unique<gl::GLTexture>(ctx);
	gl::TextureOptions opts;
	opts.has_alpha = false;
	default_texture->set_rgba(pixels, 1, 1, opts);
}

// -----------------------------------------------------------------------
// _load_textures
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::_load_textures() {
	const auto& materials = wmo->materials;

	if (useRibbon)
		syncID = texture_ribbon::reset();

	// legacy wmos use texture names directly from MOTX
	const auto& textureNames = wmo->textureNames;

	mpq::MPQInstall* mpq = core::view->mpq.get();

	for (size_t i = 0; i < materials.size(); i++) {
		const auto& material = materials[i];

		// get texture filenames for this material
		auto get_tex_name = [&](uint32_t key) -> std::string {
			auto it = textureNames.find(key);
			return (it != textureNames.end()) ? it->second : std::string{};
		};

		const std::string tex1Name = get_tex_name(material.texture1);
		const std::string tex2Name = get_tex_name(material.texture2);
		const std::string tex3Name = get_tex_name(material.texture3);

		std::vector<std::string> textureFileNames = {tex1Name, tex2Name, tex3Name};
		materialTextures[static_cast<uint32_t>(i)] = textureFileNames;

		for (const auto& textureName : textureFileNames) {
			if (textureName.empty())
				continue;

			if (textures.count(textureName))
				continue;

			int ribbonSlot = useRibbon ? texture_ribbon::addSlot() : -1;

			if (ribbonSlot >= 0)
				// JS: textureRibbon.setSlotFile(ribbonSlot, textureName, this.syncID)
				// C++ uses setSlotFileLegacy for string paths (legacy WMO format).
				texture_ribbon::setSlotFileLegacy(ribbonSlot, textureName, syncID);

			try {
				// JS: mpq.getFile(textureName) throws if mpq is null, caught by catch.
				// C++ adds explicit guard — log warning instead of silently skipping.
				if (!mpq) {
					logging::write(std::format("Cannot load legacy WMO texture {}: MPQ source is null", textureName));
					continue;
				}

				auto file_data = mpq->getFile(textureName);
				if (file_data.has_value()) {
					casc::BLPImage blp(BufferWrapper(std::move(file_data.value())));
					auto gl_tex = std::make_unique<gl::GLTexture>(ctx);
					// JS: explicitly computes wrap_s/wrap_t from material flags 0x40/0x80.
					// set_blp defaults use 0x1/0x2 (M2 convention). Override for WMO flags.
					gl::BLPTextureFlags blp_flags;
					blp_flags.flags = material.flags;
					blp_flags.wrap_s = !(material.flags & 0x40);  // 0x40 = clamp
					blp_flags.wrap_t = !(material.flags & 0x80);  // 0x80 = clamp
					gl_tex->set_blp(blp, blp_flags);
					textures[textureName] = std::move(gl_tex);

					if (ribbonSlot >= 0)
						texture_ribbon::setSlotSrc(ribbonSlot, blp.getDataURL(0b0111), syncID);
				}
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to load legacy WMO texture {}: {}", textureName, e.what()));
			}
		}
	}
}

// -----------------------------------------------------------------------
// _load_groups
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::_load_groups() {
	for (uint32_t i = 0; i < wmo->groupCount; i++) {
		try {
			const auto& group = wmo->getGroup(i);

			if (group.renderBatches.empty())
				continue;

			WMOLegacyGroup grp;
			grp.vao = std::make_unique<gl::VertexArray>(ctx);
			grp.vao->bind();

			// vertex buffer
			GLuint vbo;
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER,
				static_cast<GLsizeiptr>(group.vertices.size() * sizeof(float)),
				group.vertices.data(), GL_STATIC_DRAW);
			buffers.push_back(vbo);

			// normal buffer
			GLuint nbo;
			glGenBuffers(1, &nbo);
			glBindBuffer(GL_ARRAY_BUFFER, nbo);
			glBufferData(GL_ARRAY_BUFFER,
				static_cast<GLsizeiptr>(group.normals.size() * sizeof(float)),
				group.normals.data(), GL_STATIC_DRAW);
			buffers.push_back(nbo);

			// UV buffer
			GLuint uvo = 0;
			if (!group.uvs.empty() && !group.uvs[0].empty()) {
				glGenBuffers(1, &uvo);
				glBindBuffer(GL_ARRAY_BUFFER, uvo);
				glBufferData(GL_ARRAY_BUFFER,
					static_cast<GLsizeiptr>(group.uvs[0].size() * sizeof(float)),
					group.uvs[0].data(), GL_STATIC_DRAW);
				buffers.push_back(uvo);
			}

			// color buffer
			GLuint cbo = 0;
			if (!group.vertexColours.empty() && !group.vertexColours[0].empty()) {
				glGenBuffers(1, &cbo);
				glBindBuffer(GL_ARRAY_BUFFER, cbo);
				glBufferData(GL_ARRAY_BUFFER,
					static_cast<GLsizeiptr>(group.vertexColours[0].size()),
					group.vertexColours[0].data(), GL_STATIC_DRAW);
				buffers.push_back(cbo);
			}

			// index buffer (managed by vao->dispose())
			GLuint ebo;
			glGenBuffers(1, &ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				static_cast<GLsizeiptr>(group.indices.size() * sizeof(uint16_t)),
				group.indices.data(), GL_STATIC_DRAW);
			grp.vao->ebo = ebo;

			grp.vao->setup_wmo_separate_buffers(vbo, nbo, uvo, cbo, 0, 0, 0, 0, 0);

			// build draw calls
			for (const auto& batch : group.renderBatches) {
				const uint32_t matID = ((batch.flags & 2) == 2)
					? static_cast<uint32_t>(batch.possibleBox2[2])
					: static_cast<uint32_t>(batch.materialID);

				const uint32_t shader_id = (matID < wmo->materials.size()) ? wmo->materials[matID].shader : 0;
				const uint32_t blend_mode = (matID < wmo->materials.size()) ? wmo->materials[matID].blendMode : 0;
				const auto& shEntry = get_legacy_wmo_shader(shader_id);

				WMOLegacyDrawCall dc;
				dc.start = batch.firstFace;
				dc.count = batch.numFaces;
				dc.blendMode = blend_mode;
				dc.material_id = matID;
				dc.vertex_shader = shEntry.VertexShader;
				dc.pixel_shader = shEntry.PixelShader;
				grp.draw_calls.push_back(dc);
			}

			grp.visible = true;
			grp.index = i;

			// group label from MOGN
			std::string label;
			{
				auto it = wmo->groupNames.find(group.nameOfs);
				if (it != wmo->groupNames.end() && !it->second.empty())
					label = it->second;
				else
					label = std::format("Group {}", i);
			}

			nlohmann::json groupEntry;
			groupEntry["label"] = label;
			groupEntry["checked"] = true;
			groupEntry["groupIndex"] = i;
			groupArray.push_back(groupEntry);

			groups.push_back(std::move(grp));
		} catch (const std::exception& e) {
			logging::write(std::format("Failed to load legacy WMO group {}: {}", i, e.what()));
		}
	}
}

// -----------------------------------------------------------------------
// _setup_doodad_sets
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::_setup_doodad_sets() {
	if (wmo->doodadSets.empty())
		return;

	const size_t setCount = wmo->doodadSets.size();
	doodadSets.resize(setCount, std::nullopt);

	for (size_t i = 0; i < setCount; i++) {
		nlohmann::json setEntry;
		setEntry["label"] = wmo->doodadSets[i].name;
		setEntry["index"] = i;
		setEntry["checked"] = false;
		setArray.push_back(setEntry);
	}
}

// -----------------------------------------------------------------------
// loadDoodadSet
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::loadDoodadSet(uint32_t index) {
	if (index >= wmo->doodadSets.size())
		throw std::runtime_error("Invalid doodad set: " + std::to_string(index));

	const auto& set = wmo->doodadSets[index];

	logging::write(std::format("Loading legacy doodad set: {}", set.name));

	auto _lock = core::create_busy_lock();
	core::setToast("progress",
		std::format("Loading doodad set {} ({} doodads)...", set.name, set.doodadCount),
		{}, -1, false);

	const uint32_t firstIndex = set.firstInstanceIndex;
	const uint32_t count = set.doodadCount;

	WMOLegacyDoodadSet doodadSetData;

	mpq::MPQInstall* mpq = core::view->mpq.get();

	for (uint32_t i = 0; i < count; i++) {
		// NOTE: JS does not guard against out-of-bounds — accessing wmo.doodads[outOfRange]
		// returns undefined, and subsequent property access throws, caught by catch.
		// C++ adds explicit bounds check to prevent undefined behavior (deliberate improvement).
		if (firstIndex + i >= wmo->doodads.size())
			continue;

		const auto& doodad = wmo->doodads[firstIndex + i];

		if (wmo->doodadNames.empty())
			continue;

		auto nameIt = wmo->doodadNames.find(doodad.offset);
		if (nameIt == wmo->doodadNames.end() || nameIt->second.empty())
			continue;

		const std::string& doodadName = nameIt->second;

		try {
			M2LegacyRendererGL* renderer = nullptr;

			auto rendIt = m2_renderers.find(doodadName);
			if (rendIt != m2_renderers.end()) {
				renderer = rendIt->second.get();
			} else {
				if (mpq) {
					auto fileData = mpq->getFile(doodadName);
					if (fileData.has_value()) {
						auto data = BufferWrapper(std::move(fileData.value()));
						uint32_t magic = data.readUInt32LE();
						data.seek(0);
						if (magic == 0x3032444D) { // 'MD20'
							auto r = std::make_unique<M2LegacyRendererGL>(data, ctx, false, false);
							r->load();
							renderer = r.get();
							m2_renderers[doodadName] = std::move(r);
						}
					}
				}
			}

			if (renderer) {
				WMOLegacyDoodadInstance inst;
				inst.renderer = renderer;
				// apply doodad transform (convert WoW coords to OpenGL)
				inst.position = {
					(doodad.position.size() > 0) ? doodad.position[0] : 0.0f,
					(doodad.position.size() > 2) ? doodad.position[2] : 0.0f,
					(doodad.position.size() > 1) ? doodad.position[1] * -1.0f : 0.0f
				};
				inst.rotation = {
					(doodad.rotation.size() > 0) ? doodad.rotation[0] : 0.0f,
					(doodad.rotation.size() > 2) ? doodad.rotation[2] : 0.0f,
					(doodad.rotation.size() > 1) ? doodad.rotation[1] * -1.0f : 0.0f,
					(doodad.rotation.size() > 3) ? doodad.rotation[3] : 1.0f
				};
				inst.scale = {doodad.scale, doodad.scale, doodad.scale};
				doodadSetData.renderers.push_back(inst);
			}
		} catch (const std::exception& e) {
			logging::write(std::format("Failed to load legacy doodad {}: {}", doodadName, e.what()));
		}
	}

	doodadSetData.visible = true;
	doodadSets[index] = std::move(doodadSetData);

	core::hideToast();
}

// -----------------------------------------------------------------------
// updateGroups
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::updateGroups() {
	// JS: this.groupArray === view.modelViewerWMOGroups (shared reference).
	// C++: view state is a separate copy; read from view to reflect UI-driven changes.
	if (!core::view)
		return;
	const auto& gv = core::view->modelViewerWMOGroups;
	for (size_t i = 0; i < groups.size() && i < gv.size(); i++)
		groups[i].visible = gv[i].value("checked", true);
}

// -----------------------------------------------------------------------
// updateSets
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::updateSets() {
	if (!wmo || !core::view)
		return;

	// JS: this.setArray === view.modelViewerWMOSets (shared reference).
	// C++: view state is a separate copy; read from view to reflect UI-driven changes.
	const auto& sv = core::view->modelViewerWMOSets;
	for (size_t i = 0; i < sv.size(); i++) {
		const bool state = sv[i].value("checked", false);
		if (i < doodadSets.size() && doodadSets[i].has_value()) {
			doodadSets[i]->visible = state;
		} else if (state) {
			loadDoodadSet(static_cast<uint32_t>(i));
		}
	}
}

// -----------------------------------------------------------------------
// setTransform
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::setTransform(const std::array<float, 3>& position,
                                        const std::array<float, 3>& rotation,
                                        const std::array<float, 3>& scale) {
	position_ = position;
	rotation_ = rotation;
	scale_ = scale;
	_update_model_matrix();
}

// -----------------------------------------------------------------------
// _update_model_matrix
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::_update_model_matrix() {
	auto& m = model_matrix;
	const float px = position_[0], py = position_[1], pz = position_[2];
	const float rx = rotation_[0], ry = rotation_[1], rz = rotation_[2];
	const float sx = scale_[0],    sy = scale_[1],    sz = scale_[2];

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

	m[8]  = (cx * siny * cz + sinx * sinz) * sz;
	m[9]  = (cx * siny * sinz - sinx * cz) * sz;
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

void WMOLegacyRendererGL::render(const float* view_matrix, const float* projection_matrix) {
	if (!shader)
		return;

	// NOTE: JS uses Vue $watch for reactive updates (groupWatcher, setWatcher,
	// wireframeWatcher). C++ replaces this with per-frame polling that compares
	// prev_group_checked/prev_set_checked. This is an architectural difference
	// inherent to porting from a reactive framework — functionally equivalent.
	{
		const auto& gv = core::view->modelViewerWMOGroups;
		bool groups_changed = gv.size() != prev_group_checked.size();
		if (!groups_changed) {
			for (size_t i = 0; i < gv.size(); i++) {
				if (gv[i].value("checked", true) != prev_group_checked[i]) {
					groups_changed = true;
					break;
				}
			}
		}
		if (groups_changed) {
			updateGroups();
			prev_group_checked.resize(gv.size());
			for (size_t i = 0; i < gv.size(); i++)
				prev_group_checked[i] = gv[i].value("checked", true);
		}

		const auto& sv = core::view->modelViewerWMOSets;
		bool sets_changed = sv.size() != prev_set_checked.size();
		if (!sets_changed) {
			for (size_t i = 0; i < sv.size(); i++) {
				if (sv[i].value("checked", false) != prev_set_checked[i]) {
					sets_changed = true;
					break;
				}
			}
		}
		if (sets_changed) {
			updateSets();
			prev_set_checked.resize(sv.size());
			for (size_t i = 0; i < sv.size(); i++)
				prev_set_checked[i] = sv[i].value("checked", false);
		}
	}

	const bool wireframe = core::view->config.value("modelViewerWireframe", false);

	shader->use();

	shader->set_uniform_mat4("u_view_matrix", false, view_matrix);
	shader->set_uniform_mat4("u_projection_matrix", false, projection_matrix);
	shader->set_uniform_mat4("u_model_matrix", false, model_matrix.data());

	// lighting
	const float lx = 0.5f, ly = -0.7f, lz = 0.5f;
	const float light_view_x = view_matrix[0] * lx + view_matrix[4] * ly + view_matrix[8] * lz;
	const float light_view_y = view_matrix[1] * lx + view_matrix[5] * ly + view_matrix[9] * lz;
	const float light_view_z = view_matrix[2] * lx + view_matrix[6] * ly + view_matrix[10] * lz;

	shader->set_uniform_1i("u_apply_lighting", 1);
	shader->set_uniform_3f("u_ambient_color", 0.5f, 0.5f, 0.5f);
	shader->set_uniform_3f("u_diffuse_color", 0.7f, 0.7f, 0.7f);
	shader->set_uniform_3f("u_light_dir", light_view_x, light_view_y, light_view_z);

	shader->set_uniform_1i("u_wireframe", wireframe ? 1 : 0);
	shader->set_uniform_4f("u_wireframe_color", 1, 1, 1, 1);

	shader->set_uniform_1i("u_use_vertex_color", 0);

	shader->set_uniform_1i("u_texture1", 0);
	shader->set_uniform_1i("u_texture2", 1);
	shader->set_uniform_1i("u_texture3", 2);
	shader->set_uniform_1i("u_texture4", 3);
	shader->set_uniform_1i("u_texture5", 4);
	shader->set_uniform_1i("u_texture6", 5);
	shader->set_uniform_1i("u_texture7", 6);
	shader->set_uniform_1i("u_texture8", 7);
	shader->set_uniform_1i("u_texture9", 8);

	ctx.set_depth_test(true);
	ctx.set_depth_write(true);
	ctx.set_cull_face(false);
	ctx.set_blend(false);

	for (const auto& group : groups) {
		if (!group.visible)
			continue;

		group.vao->bind();

		for (const auto& dc : group.draw_calls) {
			shader->set_uniform_1i("u_vertex_shader", dc.vertex_shader);
			shader->set_uniform_1i("u_pixel_shader", dc.pixel_shader);
			shader->set_uniform_1i("u_blend_mode", static_cast<int>(dc.blendMode));

			auto matIt = materialTextures.find(dc.material_id);
			for (int i = 0; i < 9; i++) {
				gl::GLTexture* tex = default_texture.get();
				if (matIt != materialTextures.end() && i < static_cast<int>(matIt->second.size())) {
					const auto& texName = matIt->second[i];
					if (!texName.empty()) {
						auto it = textures.find(texName);
						if (it != textures.end())
							tex = it->second.get();
					}
				}
				tex->bind(i);
			}

			glDrawElements(
				wireframe ? GL_LINES : GL_TRIANGLES,
				static_cast<GLsizei>(dc.count),
				GL_UNSIGNED_SHORT,
				reinterpret_cast<void*>(static_cast<uintptr_t>(dc.start) * 2)
			);
		}
	}

	// render doodad sets
	for (auto& ds : doodadSets) {
		if (!ds.has_value() || !ds->visible)
			continue;

		for (auto& doodad : ds->renderers) {
			doodad.renderer->setTransformQuat(doodad.position, doodad.rotation, doodad.scale);
			doodad.renderer->render(view_matrix, projection_matrix);
		}
	}
}

// -----------------------------------------------------------------------
// updateAnimation
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::updateAnimation(float delta_time) {
	for (auto& ds : doodadSets) {
		if (!ds.has_value() || !ds->visible)
			continue;

		for (auto& doodad : ds->renderers)
			// JS: doodad.renderer.updateAnimation?.(delta_time) — optional chaining
			if (doodad.renderer)
				doodad.renderer->updateAnimation(delta_time);
	}
}

// -----------------------------------------------------------------------
// getBoundingBox
// -----------------------------------------------------------------------

std::optional<WMOLegacyRendererGL::BoundingBoxResult> WMOLegacyRendererGL::getBoundingBox() {
	if (!wmo)
		return std::nullopt;

	const auto& src_min = wmo->boundingBox1;
	const auto& src_max = wmo->boundingBox2;

	if (src_min.size() < 3 || src_max.size() < 3)
		return std::nullopt;

	BoundingBoxResult bb;
	bb.min = {src_min[0], src_min[2], -src_max[1]};
	bb.max = {src_max[0], src_max[2], -src_min[1]};
	return bb;
}

// -----------------------------------------------------------------------
// dispose
// -----------------------------------------------------------------------

void WMOLegacyRendererGL::dispose() {
	// JS: this.groupWatcher?.(), this.setWatcher?.(), this.wireframeWatcher?.()
	// C++ uses polling instead of Vue watchers — clearing prev_ vectors is sufficient.
	prev_group_checked.clear();
	prev_set_checked.clear();

	for (auto& group : groups)
		group.vao->dispose();

	for (GLuint buf : buffers)
		glDeleteBuffers(1, &buf);

	for (auto& [name, tex] : textures)
		tex->dispose();

	textures.clear();
	materialTextures.clear();

	if (default_texture) {
		default_texture->dispose();
		default_texture.reset();
	}

	for (auto& [name, renderer] : m2_renderers)
		renderer->dispose();

	m2_renderers.clear();

	// JS: this.groupArray.splice(0) / this.setArray.splice(0).
	// In JS, groupArray/setArray are the same reference as view.modelViewerWMOGroups/Sets,
	// so splice(0) also empties the view property. C++ must explicitly clear view state.
	groupArray.clear();
	setArray.clear();
	if (core::view) {
		core::view->modelViewerWMOGroups.clear();
		core::view->modelViewerWMOSets.clear();
	}

	groups.clear();
	buffers.clear();
	doodadSets.clear();
}
