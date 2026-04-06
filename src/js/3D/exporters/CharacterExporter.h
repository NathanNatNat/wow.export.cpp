/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <vector>

class M2RendererGL;

/**
 * Equipment renderer entry within an equipment slot.
 */
struct EquipmentRendererEntry {
	M2RendererGL* renderer = nullptr;
	uint32_t attachment_id = 0;
	bool is_collection_style = false;
};

/**
 * Equipment slot entry with one or more renderers and an item ID.
 */
struct EquipmentSlotEntry {
	std::vector<EquipmentRendererEntry> renderers;
	int item_id = 0;
};

/**
 * Collection slot entry with renderers that share the character skeleton.
 */
struct CollectionSlotEntry {
	std::vector<M2RendererGL*> renderers;
	int item_id = 0;
};

/**
 * Result from processing equipment geometry.
 */
struct EquipmentGeometry {
	int slot_id = 0;
	int item_id = 0;
	std::optional<uint32_t> attachment_id;
	bool is_collection_style = false;
	M2RendererGL* renderer = nullptr;
	std::vector<float> vertices;
	std::vector<float> normals;
	const std::vector<float>* uv = nullptr;
	const std::vector<float>* uv2 = nullptr;
	std::vector<uint8_t> boneIndices;
	const std::vector<uint8_t>* boneWeights = nullptr;
};

/**
 * Collects equipment model data for export, handling transforms and geometry baking.
 */
class CharacterExporter {
public:
	/**
	 * @param char_renderer - character renderer with bone matrices
	 * @param equipment_renderers - slot_id -> { renderers: [{renderer, attachment_id, is_collection_style}], item_id }
	 * @param collection_renderers - slot_id -> { renderers: [renderer], item_id }
	 */
	CharacterExporter(
		M2RendererGL* char_renderer,
		std::map<int, EquipmentSlotEntry> equipment_renderers = {},
		std::map<int, CollectionSlotEntry> collection_renderers = {}
	);

	/**
	 * Check if there are any equipment models to export
	 */
	bool has_equipment() const;

	/**
	 * Get all equipment models with their transforms applied.
	 * Returns geometry ready for export (vertices/normals already transformed).
	 * @param apply_pose - apply current animation pose to equipment
	 * @returns vector of EquipmentGeometry
	 */
	std::vector<EquipmentGeometry> get_equipment_geometry(bool apply_pose = true);

	/**
	 * Get list of all equipment slots with models
	 */
	std::vector<int> get_equipped_slots() const;

	/**
	 * Get item ID for a slot
	 */
	std::optional<int> get_item_id_for_slot(int slot_id) const;

private:
	struct ProcessedGeometry {
		std::vector<float> vertices;
		std::vector<float> normals;
		const std::vector<float>* uv = nullptr;
		const std::vector<float>* uv2 = nullptr;
		std::vector<uint8_t> boneIndices;
		const std::vector<uint8_t>* boneWeights = nullptr;
	};

	std::optional<ProcessedGeometry> _process_equipment_renderer(
		M2RendererGL* renderer,
		std::optional<uint32_t> attachment_id,
		bool is_collection_style,
		const std::vector<float>* char_bone_matrices,
		bool apply_pose
	);

	M2RendererGL* char_renderer;
	std::map<int, EquipmentSlotEntry> equipment_renderers;
	std::map<int, CollectionSlotEntry> collection_renderers;
};
