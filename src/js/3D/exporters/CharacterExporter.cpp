/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "CharacterExporter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <set>

#include "../renderers/M2RendererGL.h"

namespace {

/**
 * Transform a position by a 4x4 matrix
 */
std::array<float, 3> transform_position(float x, float y, float z, const float* mat) {
	return {
		mat[0] * x + mat[4] * y + mat[8] * z + mat[12],
		mat[1] * x + mat[5] * y + mat[9] * z + mat[13],
		mat[2] * x + mat[6] * y + mat[10] * z + mat[14]
	};
}

/**
 * Transform a normal by the upper 3x3 of a 4x4 matrix (no translation)
 */
std::array<float, 3> transform_normal(float x, float y, float z, const float* mat) {
	const float nx = mat[0] * x + mat[4] * y + mat[8] * z;
	const float ny = mat[1] * x + mat[5] * y + mat[9] * z;
	const float nz = mat[2] * x + mat[6] * y + mat[10] * z;

	// normalize
	const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
	if (len > 0.0001f)
		return { nx / len, ny / len, nz / len };

	return { nx, ny, nz };
}

struct TransformResult {
	std::vector<float> vertices;
	std::vector<float> normals;
};

/**
 * Bake bone transforms into geometry using provided bone matrices.
 * This replicates getBakedGeometry but uses explicit bone matrices.
 */
TransformResult bake_geometry_with_bones(const M2Loader& m2, const std::vector<float>& bone_matrices) {
	const auto& src_verts = m2.vertices;
	const auto& src_normals = m2.normals;
	const auto& bone_indices = m2.boneIndices;
	const auto& bone_weights = m2.boneWeights;

	const size_t vertex_count = src_verts.size() / 3;
	std::vector<float> out_verts(vertex_count * 3, 0.0f);
	std::vector<float> out_normals(vertex_count * 3, 0.0f);

	if (bone_indices.empty() || bone_weights.empty() || bone_matrices.empty()) {
		out_verts = src_verts;
		out_normals = src_normals;
		return { std::move(out_verts), std::move(out_normals) };
	}

	for (size_t i = 0; i < vertex_count; i++) {
		const size_t v_idx = i * 3;
		const size_t b_idx = i * 4;

		const float vx = src_verts[v_idx];
		const float vy = src_verts[v_idx + 1];
		const float vz = src_verts[v_idx + 2];

		const float nx = src_normals[v_idx];
		const float ny = src_normals[v_idx + 1];
		const float nz = src_normals[v_idx + 2];

		float out_x = 0, out_y = 0, out_z = 0;
		float out_nx = 0, out_ny = 0, out_nz = 0;

		// blend up to 4 bone influences
		for (int j = 0; j < 4; j++) {
			const uint8_t bone_idx = bone_indices[b_idx + j];
			const float weight = bone_weights[b_idx + j] / 255.0f;

			if (weight == 0.0f)
				continue;

			const size_t mat_offset = static_cast<size_t>(bone_idx) * 16;
			if (mat_offset + 16 > bone_matrices.size())
				continue;

			const float* m = bone_matrices.data();

			// transform position
			const float tx = m[mat_offset + 0] * vx + m[mat_offset + 4] * vy + m[mat_offset + 8] * vz + m[mat_offset + 12];
			const float ty = m[mat_offset + 1] * vx + m[mat_offset + 5] * vy + m[mat_offset + 9] * vz + m[mat_offset + 13];
			const float tz = m[mat_offset + 2] * vx + m[mat_offset + 6] * vy + m[mat_offset + 10] * vz + m[mat_offset + 14];

			out_x += tx * weight;
			out_y += ty * weight;
			out_z += tz * weight;

			// transform normal (no translation)
			const float tnx = m[mat_offset + 0] * nx + m[mat_offset + 4] * ny + m[mat_offset + 8] * nz;
			const float tny = m[mat_offset + 1] * nx + m[mat_offset + 5] * ny + m[mat_offset + 9] * nz;
			const float tnz = m[mat_offset + 2] * nx + m[mat_offset + 6] * ny + m[mat_offset + 10] * nz;

			out_nx += tnx * weight;
			out_ny += tny * weight;
			out_nz += tnz * weight;
		}

		// normalize the blended normal
		const float len = std::sqrt(out_nx * out_nx + out_ny * out_ny + out_nz * out_nz);
		if (len > 0.0001f) {
			out_nx /= len;
			out_ny /= len;
			out_nz /= len;
		}

		out_verts[v_idx] = out_x;
		out_verts[v_idx + 1] = out_y;
		out_verts[v_idx + 2] = out_z;

		out_normals[v_idx] = out_nx;
		out_normals[v_idx + 1] = out_ny;
		out_normals[v_idx + 2] = out_nz;
	}

	return { std::move(out_verts), std::move(out_normals) };
}

/**
 * Remap bone indices from equipment model to character skeleton
 */
std::vector<uint8_t> remap_bone_indices(const std::vector<uint8_t>& bone_indices, const std::vector<int16_t>& remap_table) {
	std::vector<uint8_t> remapped(bone_indices.size());

	for (size_t i = 0; i < bone_indices.size(); i++) {
		const uint8_t original_idx = bone_indices[i];
		if (static_cast<size_t>(original_idx) < remap_table.size())
			remapped[i] = static_cast<uint8_t>(remap_table[original_idx]);
		else
			remapped[i] = original_idx;
	}

	return remapped;
}

/**
 * Apply a transform matrix to all vertices and normals
 */
TransformResult apply_transform_to_geometry(const std::vector<float>& vertices, const std::vector<float>& normals, const std::array<float, 16>& transform) {
	const size_t vertex_count = vertices.size() / 3;
	std::vector<float> out_verts(vertex_count * 3);
	std::vector<float> out_normals(vertex_count * 3);

	for (size_t i = 0; i < vertex_count; i++) {
		const size_t vi = i * 3;
		const float vx = vertices[vi], vy = vertices[vi + 1], vz = vertices[vi + 2];
		const float nx = normals[vi], ny = normals[vi + 1], nz = normals[vi + 2];

		const auto [tx, ty, tz] = transform_position(vx, vy, vz, transform.data());
		const auto [tnx, tny, tnz] = transform_normal(nx, ny, nz, transform.data());

		out_verts[vi] = tx;
		out_verts[vi + 1] = ty;
		out_verts[vi + 2] = tz;

		out_normals[vi] = tnx;
		out_normals[vi + 1] = tny;
		out_normals[vi + 2] = tnz;
	}

	return { std::move(out_verts), std::move(out_normals) };
}

} // anonymous namespace

/**
 * Collects equipment model data for export, handling transforms and geometry baking.
 */

CharacterExporter::CharacterExporter(
	M2RendererGL* char_renderer,
	std::map<int, EquipmentSlotEntry> equipment_renderers,
	std::map<int, CollectionSlotEntry> collection_renderers)
	: char_renderer(char_renderer)
	, equipment_renderers(std::move(equipment_renderers))
	, collection_renderers(std::move(collection_renderers))
{
}

/**
 * Check if there are any equipment models to export
 */
bool CharacterExporter::has_equipment() const {
	return !equipment_renderers.empty() || !collection_renderers.empty();
}

/**
 * Get all equipment models with their transforms applied.
 * Returns geometry ready for export (vertices/normals already transformed).
 * @param apply_pose - apply current animation pose to equipment
 * @returns vector of EquipmentGeometry
 */
std::vector<EquipmentGeometry> CharacterExporter::get_equipment_geometry(bool apply_pose) {
	std::vector<EquipmentGeometry> results;

	const std::vector<float>* char_bone_matrices = nullptr;
	if (char_renderer) {
		const auto& bm = char_renderer->get_bone_matrices();
		if (!bm.empty())
			char_bone_matrices = &bm;
	}

	// process attachment models (weapons, helms, shoulders, etc)
	for (const auto& [slot_id, entry] : equipment_renderers) {
		for (const auto& re : entry.renderers) {
			if (!re.renderer || !re.renderer->m2)
				continue;

			auto geometry = _process_equipment_renderer(
				re.renderer,
				re.attachment_id,
				re.is_collection_style,
				char_bone_matrices,
				apply_pose
			);

			if (geometry) {
				EquipmentGeometry eg;
				eg.slot_id = slot_id;
				eg.item_id = entry.item_id;
				eg.attachment_id = re.attachment_id;
				eg.is_collection_style = re.is_collection_style;
				eg.renderer = re.renderer;
				eg.vertices = std::move(geometry->vertices);
				eg.normals = std::move(geometry->normals);
				eg.uv = geometry->uv;
				eg.uv2 = geometry->uv2;
				eg.boneIndices = std::move(geometry->boneIndices);
				eg.boneWeights = geometry->boneWeights;
				results.push_back(std::move(eg));
			}
		}
	}

	// process collection models (armor pieces that share character skeleton)
	for (const auto& [slot_id, entry] : collection_renderers) {
		for (M2RendererGL* renderer : entry.renderers) {
			if (!renderer || !renderer->m2)
				continue;

			auto geometry = _process_equipment_renderer(
				renderer,
				std::nullopt,
				true,
				char_bone_matrices,
				apply_pose
			);

			if (geometry) {
				EquipmentGeometry eg;
				eg.slot_id = slot_id;
				eg.item_id = entry.item_id;
				eg.is_collection_style = true;
				eg.renderer = renderer;
				eg.vertices = std::move(geometry->vertices);
				eg.normals = std::move(geometry->normals);
				eg.uv = geometry->uv;
				eg.uv2 = geometry->uv2;
				eg.boneIndices = std::move(geometry->boneIndices);
				eg.boneWeights = geometry->boneWeights;
				results.push_back(std::move(eg));
			}
		}
	}

	return results;
}

/**
 * Process a single equipment renderer and get posed geometry
 * @private
 */
std::optional<CharacterExporter::ProcessedGeometry> CharacterExporter::_process_equipment_renderer(
	M2RendererGL* renderer,
	std::optional<uint32_t> attachment_id,
	bool is_collection_style,
	const std::vector<float>* char_bone_matrices,
	bool apply_pose)
{
	const auto* m2 = renderer->m2.get();
	if (!m2)
		return std::nullopt;

	ProcessedGeometry result;
	result.uv = &m2->uv;
	result.uv2 = &m2->uv2;

	if (is_collection_style && apply_pose && char_bone_matrices) {
		// collection-style models use character's bone matrices via remapping
		// first ensure bone matrices are updated from character
		renderer->applyExternalBoneMatrices(char_bone_matrices->data(), char_bone_matrices->size() / 16);

		// bake geometry using the remapped bone matrices
		if (!renderer->get_bone_matrices().empty() && !m2->boneIndices.empty() && !m2->boneWeights.empty()) {
			auto baked = bake_geometry_with_bones(*m2, renderer->get_bone_matrices());
			result.vertices = std::move(baked.vertices);
			result.normals = std::move(baked.normals);
		} else {
			result.vertices = m2->vertices;
			result.normals = m2->normals;
		}

		// for GLTF export, we need remapped bone indices
		if (!renderer->get_bone_remap_table().empty() && !m2->boneIndices.empty()) {
			result.boneIndices = remap_bone_indices(m2->boneIndices, renderer->get_bone_remap_table());
			result.boneWeights = &m2->boneWeights;
		}
	} else if (!is_collection_style && attachment_id.has_value() && apply_pose) {
		// attachment models need the attachment transform applied
		std::optional<std::array<float, 16>> attach_transform;
		if (char_renderer)
			attach_transform = char_renderer->getAttachmentTransform(attachment_id.value());

		if (attach_transform) {
			auto transformed = apply_transform_to_geometry(m2->vertices, m2->normals, attach_transform.value());
			result.vertices = std::move(transformed.vertices);
			result.normals = std::move(transformed.normals);
		} else {
			result.vertices = m2->vertices;
			result.normals = m2->normals;
		}
	} else {
		// no pose - use original geometry
		result.vertices = m2->vertices;
		result.normals = m2->normals;

		// still provide remapped bone indices for non-posed GLTF export
		if (is_collection_style && !renderer->get_bone_remap_table().empty() && !m2->boneIndices.empty()) {
			result.boneIndices = remap_bone_indices(m2->boneIndices, renderer->get_bone_remap_table());
			result.boneWeights = &m2->boneWeights;
		}
	}

	return result;
}

/**
 * Get list of all equipment slots with models
 */
std::vector<int> CharacterExporter::get_equipped_slots() const {
	std::set<int> slots;

	for (const auto& [slot_id, _] : equipment_renderers)
		slots.insert(slot_id);

	for (const auto& [slot_id, _] : collection_renderers)
		slots.insert(slot_id);

	return std::vector<int>(slots.begin(), slots.end());
}

/**
 * Get item ID for a slot
 */
std::optional<int> CharacterExporter::get_item_id_for_slot(int slot_id) const {
	auto it = equipment_renderers.find(slot_id);
	if (it != equipment_renderers.end())
		return it->second.item_id;

	auto it2 = collection_renderers.find(slot_id);
	if (it2 != collection_renderers.end())
		return it2->second.item_id;

	return std::nullopt;
}
