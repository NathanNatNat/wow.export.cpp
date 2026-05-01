/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "WMORendererGL.h"
#include "M2RendererGL.h"

#include "../../core.h"
#include "../../log.h"
#include "../../buffer.h"
#include "../../constants.h"

#include "../../casc/blp.h"
#include "../../casc/casc-source.h"
#include "../../casc/listfile.h"
#include "../Texture.h"
#include "../Shaders.h"

#include "../../ui/texture-ribbon.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>

static constexpr std::array<float, 16> WMO_IDENTITY_MAT4 = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

WMORendererGL::WMORendererGL(BufferWrapper& data, uint32_t fileID, gl::GLContext& gl_context, bool useRibbon)
	: data_ptr(&data)
	, fileID(fileID)
	, fileName(casc::listfile::getByID(fileID).value_or(""))
	, ctx(gl_context)
	, useRibbon(useRibbon)
{
	model_matrix = WMO_IDENTITY_MAT4;
}

WMORendererGL::WMORendererGL(BufferWrapper& data, const std::string& fileName, gl::GLContext& gl_context, bool useRibbon)
	: data_ptr(&data)
	, fileID(casc::listfile::getByFilename(fileName).value_or(0))
	, fileName(fileName)
	, ctx(gl_context)
	, useRibbon(useRibbon)
{
	model_matrix = WMO_IDENTITY_MAT4;
}

std::unique_ptr<gl::ShaderProgram> WMORendererGL::load_shaders(gl::GLContext& ctx) {
	return shaders::create_program(ctx, "wmo");
}

std::vector<nlohmann::json>& WMORendererGL::get_wmo_groups_view() {
	if (wmoGroupKey == "creatureViewerWMOGroups") return core::view->creatureViewerWMOGroups;
	if (wmoGroupKey == "decorViewerWMOGroups")   return core::view->decorViewerWMOGroups;
	return core::view->modelViewerWMOGroups;
}

std::vector<nlohmann::json>& WMORendererGL::get_wmo_sets_view() {
	if (wmoSetKey == "creatureViewerWMOSets") return core::view->creatureViewerWMOSets;
	if (wmoSetKey == "decorViewerWMOSets")   return core::view->decorViewerWMOSets;
	return core::view->modelViewerWMOSets;
}

void WMORendererGL::load() {
	if (!casc_source_ && core::view && core::view->casc)
		casc_source_ = core::view->casc;

	// parse WMO data
	if (!fileName.empty())
		wmo = std::make_unique<WMOLoader>(*data_ptr, fileName, true);
	else
		wmo = std::make_unique<WMOLoader>(*data_ptr, fileID, true);
	wmo->load();

	// load shader program
	shader = WMORendererGL::load_shaders(ctx);

	// create default texture
	_create_default_texture();

	// load textures
	_load_textures();

	// load groups
	_load_groups();

	// setup doodad sets
	_setup_doodad_sets();

	// setup reactive controls
	get_wmo_groups_view() = groupArray;
	get_wmo_sets_view() = setArray;
	{
		const auto& gv = get_wmo_groups_view();
		prev_group_checked.resize(gv.size());
		for (size_t i = 0; i < gv.size(); i++)
			prev_group_checked[i] = gv[i].value("checked", true);

		const auto& sv = get_wmo_sets_view();
		prev_set_checked.resize(sv.size());
		for (size_t i = 0; i < sv.size(); i++)
			prev_set_checked[i] = sv[i].value("checked", false);
	}

	// drop reference to raw data
	data_ptr = nullptr;
}

void WMORendererGL::_create_default_texture() {
	const uint8_t pixels[] = {87, 175, 226, 255};
	default_texture = std::make_unique<gl::GLTexture>(ctx);
	gl::TextureOptions opts;
	opts.has_alpha = false;
	default_texture->set_rgba(pixels, 1, 1, opts);
}

void WMORendererGL::_load_textures() {
	const auto& materials = wmo->materials;

	if (useRibbon)
		syncID = texture_ribbon::reset();

	const bool isClassic = wmo->hasMotxChunk;

	for (size_t i = 0; i < materials.size(); i++) {
		const auto& material = materials[i];

		int pixelShader = wmo_shader_mapper::MapObjDiffuse;
		{
			auto it = wmo_shader_mapper::WMOShaderMap.find(static_cast<int>(material.shader));
			if (it != wmo_shader_mapper::WMOShaderMap.end())
				pixelShader = static_cast<int>(it->second.PixelShader);
		}

		// Don't load LOD textures
		if (pixelShader == wmo_shader_mapper::MapObjLod)
			continue;

		std::vector<uint32_t> textureFileDataIDs;

		if (isClassic) {
			auto resolve_name = [&](uint32_t key) -> uint32_t {
				auto it = wmo->textureNames.find(key);
				if (it != wmo->textureNames.end() && !it->second.empty()) {
					auto fdid = casc::listfile::getByFilename(it->second);
					return fdid.value_or(0);
				}
				return 0;
			};
			textureFileDataIDs.push_back(resolve_name(material.texture1));
			textureFileDataIDs.push_back(resolve_name(material.texture2));
			textureFileDataIDs.push_back(resolve_name(material.texture3));
		} else {
			textureFileDataIDs.push_back(material.texture1);
			textureFileDataIDs.push_back(material.texture2);
			textureFileDataIDs.push_back(material.texture3);
		}

		if (pixelShader == wmo_shader_mapper::MapObjParallax_PS) {
			// MapObjParallax
			textureFileDataIDs.push_back(material.color2);
			textureFileDataIDs.push_back(material.flags3);
			if (!material.runtimeData.empty())
				textureFileDataIDs.push_back(material.runtimeData[0]);
		} else if (pixelShader == wmo_shader_mapper::MapObjDFShader) {
			textureFileDataIDs.push_back(material.color3);
			textureFileDataIDs.push_back(material.flags3);
			for (const uint32_t rtd : material.runtimeData)
				textureFileDataIDs.push_back(rtd);
		}

		materialTextures[static_cast<uint32_t>(i)] = textureFileDataIDs;

		for (const uint32_t textureFileDataID : textureFileDataIDs) {
			if (textureFileDataID == 0)
				continue;

			if (textures.count(textureFileDataID))
				continue;

			int ribbonSlot = useRibbon ? texture_ribbon::addSlot() : -1;

			if (ribbonSlot >= 0)
				texture_ribbon::setSlotFile(ribbonSlot, textureFileDataID, syncID);

			try {
				Texture texture(material.flags, textureFileDataID);
				auto data = texture.getTextureFile();
				if (!data.has_value()) {
					logging::write(std::format("Failed to load WMO texture {}: getTextureFile returned null", textureFileDataID));
					continue;
				}

				casc::BLPImage blp(std::move(data.value()));
				auto gl_tex = std::make_unique<gl::GLTexture>(ctx);

				// WMO wrap flags are inverted (0x40/0x80 = clamp)
				gl::TextureOptions opts;
				opts.wrap_s = (material.flags & 0x40) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
				opts.wrap_t = (material.flags & 0x80) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
				opts.has_alpha = blp.alphaDepth > 0;
				opts.generate_mipmaps = true;

				const auto pixels = blp.toUInt8Array(0, 0b1111);
				gl_tex->set_rgba(pixels.data(), static_cast<int>(blp.width), static_cast<int>(blp.height), opts);

				textures[textureFileDataID] = std::move(gl_tex);

				if (ribbonSlot >= 0)
					texture_ribbon::setSlotSrc(ribbonSlot, blp.getDataURL(0b0111), syncID);
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to load WMO texture {}: {}", textureFileDataID, e.what()));
			}
		}
	}
}

void WMORendererGL::_load_groups() {
	for (uint32_t i = 0; i < wmo->groupCount; i++) {
		try {
			const auto& group = wmo->getGroup(i);

			if (group.renderBatches.empty())
				continue;

			// create VAO for this group
			WMOGroup grp;
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

			// UV buffers
			GLuint uvo = 0, uv2o = 0, uv3o = 0, uv4o = 0;
			if (!group.uvs.empty()) {
				if (!group.uvs[0].empty()) {
					glGenBuffers(1, &uvo);
					glBindBuffer(GL_ARRAY_BUFFER, uvo);
					glBufferData(GL_ARRAY_BUFFER,
						static_cast<GLsizeiptr>(group.uvs[0].size() * sizeof(float)),
						group.uvs[0].data(), GL_STATIC_DRAW);
					buffers.push_back(uvo);
				}

				if (group.uvs.size() > 1 && !group.uvs[1].empty()) {
					glGenBuffers(1, &uv2o);
					glBindBuffer(GL_ARRAY_BUFFER, uv2o);
					glBufferData(GL_ARRAY_BUFFER,
						static_cast<GLsizeiptr>(group.uvs[1].size() * sizeof(float)),
						group.uvs[1].data(), GL_STATIC_DRAW);
					buffers.push_back(uv2o);
				}

				if (group.uvs.size() > 2 && !group.uvs[2].empty()) {
					glGenBuffers(1, &uv3o);
					glBindBuffer(GL_ARRAY_BUFFER, uv3o);
					glBufferData(GL_ARRAY_BUFFER,
						static_cast<GLsizeiptr>(group.uvs[2].size() * sizeof(float)),
						group.uvs[2].data(), GL_STATIC_DRAW);
					buffers.push_back(uv3o);
				}

				if (group.uvs.size() > 3 && !group.uvs[3].empty()) {
					glGenBuffers(1, &uv4o);
					glBindBuffer(GL_ARRAY_BUFFER, uv4o);
					glBufferData(GL_ARRAY_BUFFER,
						static_cast<GLsizeiptr>(group.uvs[3].size() * sizeof(float)),
						group.uvs[3].data(), GL_STATIC_DRAW);
					buffers.push_back(uv4o);
				}
			}

			// Color buffers
			GLuint cbo = 0, cbo2 = 0, cbo3 = 0;
			if (!group.vertexColours.empty() && !group.vertexColours[0].empty()) {
				glGenBuffers(1, &cbo);
				glBindBuffer(GL_ARRAY_BUFFER, cbo);
				glBufferData(GL_ARRAY_BUFFER,
					static_cast<GLsizeiptr>(group.vertexColours[0].size()),
					group.vertexColours[0].data(), GL_STATIC_DRAW);
				buffers.push_back(cbo);

				if (group.vertexColours.size() > 1 && !group.vertexColours[1].empty()) {
					glGenBuffers(1, &cbo2);
					glBindBuffer(GL_ARRAY_BUFFER, cbo2);
					glBufferData(GL_ARRAY_BUFFER,
						static_cast<GLsizeiptr>(group.vertexColours[1].size()),
						group.vertexColours[1].data(), GL_STATIC_DRAW);
					buffers.push_back(cbo2);
				}
			}

			if (!group.colors2.empty()) {
				glGenBuffers(1, &cbo3);
				glBindBuffer(GL_ARRAY_BUFFER, cbo3);
				glBufferData(GL_ARRAY_BUFFER,
					static_cast<GLsizeiptr>(group.colors2.size()),
					group.colors2.data(), GL_STATIC_DRAW);
				buffers.push_back(cbo3);
			}

			// index buffer (managed by vao->dispose())
			GLuint ebo;
			glGenBuffers(1, &ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				static_cast<GLsizeiptr>(group.indices.size() * sizeof(uint16_t)),
				group.indices.data(), GL_STATIC_DRAW);
			grp.vao->ebo = ebo;

			// wireframe index buffer
			{
				auto line_indices = gl::VertexArray::triangles_to_lines(group.indices.data(), group.indices.size());
				grp.vao->set_wireframe_index_buffer(line_indices.data(), line_indices.size());
			}

			// set up vertex attributes
			grp.vao->setup_wmo_separate_buffers(vbo, nbo, uvo, cbo, cbo2, cbo3, uv2o, uv3o, uv4o);

			// build draw calls for each batch
			for (const auto& batch : group.renderBatches) {
				const uint32_t matID = ((batch.flags & 2) == 2)
					? static_cast<uint32_t>(batch.possibleBox2[2])
					: static_cast<uint32_t>(batch.materialID);

				const uint32_t shader_id = (matID < wmo->materials.size()) ? wmo->materials[matID].shader : 0;
				const uint32_t blend_mode = (matID < wmo->materials.size()) ? wmo->materials[matID].blendMode : 0;

				wmo_shader_mapper::WMOShaderEntry sh_entry = {
					wmo_shader_mapper::MapObjDiffuse_T1,
					wmo_shader_mapper::MapObjDiffuse
				};
				{
					auto it = wmo_shader_mapper::WMOShaderMap.find(static_cast<int>(shader_id));
					if (it != wmo_shader_mapper::WMOShaderMap.end())
						sh_entry = it->second;
				}

				WMODrawCall dc;
				dc.start = batch.firstFace;
				dc.count = batch.numFaces;
				dc.blendMode = blend_mode;
				dc.material_id = matID;
				dc.shader = sh_entry;
				grp.draw_calls.push_back(dc);
			}

			grp.visible = true;
			grp.index = i;

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
			logging::write(std::format("Failed to load WMO group {}: {}", i, e.what()));
		}
	}
}

void WMORendererGL::_setup_doodad_sets() {
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

/**
 * Load a doodad set
 * @param index
 */
void WMORendererGL::loadDoodadSet(uint32_t index) {
	if (index >= wmo->doodadSets.size())
		throw std::runtime_error("Invalid doodad set: " + std::to_string(index));

	const auto& set = wmo->doodadSets[index];

	logging::write(std::format("Loading doodad set: {}", set.name));

	auto _lock = core::create_busy_lock();
	core::setToast("progress",
		std::format("Loading doodad set {} ({} doodads)...", set.name, set.doodadCount),
		{}, -1, false);

	const uint32_t firstIndex = set.firstInstanceIndex;
	const uint32_t count = set.doodadCount;

	WMODoodadSetData doodadSetData;

	for (uint32_t i = 0; i < count; i++) {
		if (firstIndex + i >= wmo->doodads.size())
			continue;

		const auto& doodad = wmo->doodads[firstIndex + i];
		uint32_t fileDataID = 0;

		if (!wmo->fileDataIDs.empty() && doodad.offset < wmo->fileDataIDs.size())
			fileDataID = wmo->fileDataIDs[doodad.offset];
		else {
			auto nameIt = wmo->doodadNames.find(doodad.offset);
			if (nameIt != wmo->doodadNames.end() && !nameIt->second.empty()) {
				auto fdid = casc::listfile::getByFilename(nameIt->second);
				fileDataID = fdid.value_or(0);
			}
		}

		if (fileDataID == 0)
			continue;

		try {
			M2RendererGL* renderer = nullptr;

			auto rendIt = m2_renderers.find(fileDataID);
			if (rendIt != m2_renderers.end()) {
				// reuse existing renderer data
				renderer = rendIt->second.get();
			} else {
				if (!casc_source_)
					continue;

				auto data_buf = std::make_unique<BufferWrapper>(casc_source_->getVirtualFileByID(fileDataID));
				const uint32_t magic = data_buf->readUInt32LE();
				data_buf->seek(0);

				if (magic == constants::MAGIC::MD21) {
					auto r = std::make_unique<M2RendererGL>(*data_buf, ctx, false, false);
					r->setCASCSource(casc_source_);
					r->load().get();
					r->loadSkin(0).get();
					renderer = r.get();
					m2_renderers[fileDataID] = std::move(r);
					m2_data_buffers_.push_back(std::move(data_buf));
				}
			}

			if (renderer) {
				// apply doodad transform (convert WoW coords to OpenGL)
				WMODoodadInstance inst;
				inst.renderer = renderer;
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
			logging::write(std::format("Failed to load doodad {}: {}", fileDataID, e.what()));
		}
	}

	doodadSetData.visible = true;
	doodadSets[index] = std::move(doodadSetData);

	core::hideToast();
}

void WMORendererGL::updateGroups() {
	const auto& groups_view = get_wmo_groups_view();
	for (size_t i = 0; i < groups.size() && i < groups_view.size(); i++)
		groups[i].visible = groups_view[i].value("checked", true);
}

void WMORendererGL::updateSets() {
	if (!wmo)
		return;

	const auto& sets_view = get_wmo_sets_view();
	for (size_t i = 0; i < sets_view.size(); i++) {
		const bool state = sets_view[i].value("checked", false);
		if (i < doodadSets.size() && doodadSets[i].has_value()) {
			doodadSets[i]->visible = state;
		} else if (state) {
			loadDoodadSet(static_cast<uint32_t>(i));
		}
	}
}

void WMORendererGL::updateWireframe() {
	// handled in render()
}

/**
 * Set model transformation
 * @param position
 * @param rotation
 * @param scale
 */
void WMORendererGL::setTransform(const std::array<float, 3>& position,
                                  const std::array<float, 3>& rotation,
                                  const std::array<float, 3>& scale) {
	position_ = position;
	rotation_ = rotation;
	scale_ = scale;
	_update_model_matrix();
}

void WMORendererGL::_update_model_matrix() {
	// build model matrix from position/rotation/scale (TRS order)
	auto& m = model_matrix;
	const float px = position_[0], py = position_[1], pz = position_[2];
	const float rx = rotation_[0], ry = rotation_[1], rz = rotation_[2];
	const float sx = scale_[0],    sy = scale_[1],    sz = scale_[2];

	// rotation (ZYX euler order, column-major)
	const float cx = std::cos(rx), sinx = std::sin(rx);
	const float cy = std::cos(ry), siny = std::sin(ry);
	const float cz = std::cos(rz), sinz = std::sin(rz);

	// column 0 (scaled by sx)
	m[0] = cy * cz * sx;
	m[1] = cy * sinz * sx;
	m[2] = -siny * sx;
	m[3] = 0;

	// column 1 (scaled by sy)
	m[4] = (sinx * siny * cz - cx * sinz) * sy;
	m[5] = (sinx * siny * sinz + cx * cz) * sy;
	m[6] = sinx * cy * sy;
	m[7] = 0;

	// column 2 (scaled by sz)
	m[8]  = (cx * siny * cz + sinx * sinz) * sz;
	m[9]  = (cx * siny * sinz - sinx * cz) * sz;
	m[10] = cx * cy * sz;
	m[11] = 0;

	// column 3 (translation)
	m[12] = px;
	m[13] = py;
	m[14] = pz;
	m[15] = 1;
}

/**
 * Render the model
 * @param view_matrix
 * @param projection_matrix
 */
void WMORendererGL::render(const float* view_matrix, const float* projection_matrix) {
	if (!shader)
		return;

	{
		const auto& gv = get_wmo_groups_view();
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

		const auto& sv = get_wmo_sets_view();
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

	// set scene uniforms
	shader->set_uniform_mat4("u_view_matrix", false, view_matrix);
	shader->set_uniform_mat4("u_projection_matrix", false, projection_matrix);
	shader->set_uniform_mat4("u_model_matrix", false, model_matrix.data());

	// lighting
	shader->set_uniform_1i("u_apply_lighting", 1);
	shader->set_uniform_3f("u_light_dir", 0.5f, 1.0f, 0.5f);

	// wireframe
	shader->set_uniform_1i("u_wireframe", wireframe ? 1 : 0);
	shader->set_uniform_4f("u_wireframe_color", 1, 1, 1, 1);

	// vertex color
	shader->set_uniform_1i("u_use_vertex_color", 0);

	// texture samplers
	shader->set_uniform_1i("u_texture1", 0);
	shader->set_uniform_1i("u_texture2", 1);
	shader->set_uniform_1i("u_texture3", 2);
	shader->set_uniform_1i("u_texture4", 3);
	shader->set_uniform_1i("u_texture5", 4);
	shader->set_uniform_1i("u_texture6", 5);
	shader->set_uniform_1i("u_texture7", 6);
	shader->set_uniform_1i("u_texture8", 7);
	shader->set_uniform_1i("u_texture9", 8);

	// render state
	ctx.set_depth_test(true);
	ctx.set_depth_write(true);
	ctx.set_cull_face(false);
	ctx.set_blend(false);

	// render each group
	for (const auto& group : groups) {
		if (!group.visible)
			continue;

		group.vao->bind();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireframe ? group.vao->wireframe_ebo : group.vao->ebo);

		for (const auto& dc : group.draw_calls) {
			// set shader mode
			shader->set_uniform_1i("u_vertex_shader", static_cast<int>(dc.shader.VertexShader));
			shader->set_uniform_1i("u_pixel_shader", static_cast<int>(dc.shader.PixelShader));
			shader->set_uniform_1i("u_blend_mode", static_cast<int>(dc.blendMode));

			// set blend mode
			ctx.apply_blend_mode(static_cast<int>(dc.blendMode));

			// bind textures
			auto matIt = materialTextures.find(dc.material_id);
			for (int i = 0; i < 9; i++) {
				uint32_t textureFileDataID = 0;
				if (matIt != materialTextures.end() && i < static_cast<int>(matIt->second.size()))
					textureFileDataID = matIt->second[i];

				gl::GLTexture* tex = default_texture.get();
				if (textureFileDataID != 0) {
					auto it = textures.find(textureFileDataID);
					if (it != textures.end())
						tex = it->second.get();
				}
				tex->bind(i);
			}

			// draw
			if (wireframe)
				glDrawElements(GL_LINES,
				               static_cast<GLsizei>(dc.count * 2),
				               GL_UNSIGNED_SHORT,
				               reinterpret_cast<void*>(static_cast<uintptr_t>(dc.start) * 4));
			else
				glDrawElements(GL_TRIANGLES,
				               static_cast<GLsizei>(dc.count),
				               GL_UNSIGNED_SHORT,
				               reinterpret_cast<void*>(static_cast<uintptr_t>(dc.start) * 2));
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

void WMORendererGL::updateAnimation(float delta_time) {
	// update doodad animations
	for (auto& ds : doodadSets) {
		if (!ds.has_value() || !ds->visible)
			continue;

		for (auto& doodad : ds->renderers)
			if (doodad.renderer)
				doodad.renderer->updateAnimation(delta_time);
	}
}

/**
 * Get model bounding box (converted from WoW Z-up to OpenGL Y-up)
 * @returns BoundingBoxResult or nullopt if not loaded
 */
std::optional<WMORendererGL::BoundingBoxResult> WMORendererGL::getBoundingBox() {
	if (!wmo)
		return std::nullopt;

	const auto& src_min = wmo->boundingBox1;
	const auto& src_max = wmo->boundingBox2;

	if (src_min.size() < 3 || src_max.size() < 3)
		return std::nullopt;

	// wow coords: X=right, Y=forward, Z=up
	// opengl coords: X=right, Y=up, Z=forward (negated)
	BoundingBoxResult bb;
	bb.min = {src_min[0], src_min[2], -src_max[1]};
	bb.max = {src_max[0], src_max[2], -src_min[1]};
	return bb;
}

void WMORendererGL::dispose() {
	// unregister watchers
	prev_group_checked.clear();
	prev_set_checked.clear();

	// dispose groups
	for (auto& group : groups)
		group.vao->dispose();

	// dispose buffers
	for (GLuint buf : buffers)
		glDeleteBuffers(1, &buf);

	// dispose textures
	for (auto& [id, tex] : textures)
		tex->dispose();

	textures.clear();
	materialTextures.clear();

	if (default_texture) {
		default_texture->dispose();
		default_texture.reset();
	}

	// dispose M2 renderers
	for (auto& [id, renderer] : m2_renderers)
		renderer->dispose();

	m2_renderers.clear();
	m2_data_buffers_.clear();

	// clear arrays
	groupArray.clear();
	setArray.clear();
	get_wmo_groups_view().clear();
	get_wmo_sets_view().clear();

	groups.clear();
	buffers.clear();
	doodadSets.clear();
}
