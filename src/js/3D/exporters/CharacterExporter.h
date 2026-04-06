/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

class M2RendererGL;

/**
 * A single renderer entry within an equipment or collection slot.
 */
struct RendererEntry {
	M2RendererGL* renderer = nullptr;
	std::optional<uint32_t> attachment_id;
	bool is_collection_style = false;
};

/**
 * A slot entry: list of renderers plus item ID.
 */
struct SlotEntry {
	std::vector<RendererEntry> renderers;
	int item_id = 0;
};

/**
 * Result from get_equipment_geometry(): posed geometry for one equipment piece.
 */
struct EquipmentGeometry {
	int slot_id = 0;
	int item_id = 0;
	std::optional<uint32_t> attachment_id;
	bool is_collection_style = false;
	M2RendererGL* renderer = nullptr;
	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> uv;
	std::vector<float> uv2;
	std::optional<std::vector<uint8_t>> boneIndices;
	std::optional<std::vector<uint8_t>> boneWeights;
};

/**
 * Collects equipment model data for export, handling transforms and geometry baking.
 *
 * JS equivalent: class CharacterExporter — module.exports = CharacterExporter
 */
class CharacterExporter {
public:
	/**
	 * Construct a new CharacterExporter.
	 * @param char_renderer   Character renderer with bone matrices.
	 * @param equipment_renderers Attachment models (weapons, helms, etc): slot_id → SlotEntry.
	 * @param collection_renderers Armor pieces sharing character skeleton: slot_id → SlotEntry.
	 */
	CharacterExporter(
		M2RendererGL* char_renderer,
		std::unordered_map<int, SlotEntry> equipment_renderers,
		std::unordered_map<int, SlotEntry> collection_renderers);

	/**
	 * Check if there are any equipment models to export.
	 */
	bool has_equipment() const;

	/**
	 * Get all equipment models with their transforms applied.
	 * Returns geometry ready for export (vertices/normals already transformed).
	 * @param apply_pose Apply current animation pose to equipment.
	 */
	std::vector<EquipmentGeometry> get_equipment_geometry(bool apply_pose = true);

	/**
	 * Get list of all equipment slots with models.
	 */
	std::vector<int> get_equipped_slots() const;

	/**
	 * Get item ID for a slot.
	 * @param slot_id The equipment slot ID.
	 * @returns item_id or -1 if not found.
	 */
	int get_item_id_for_slot(int slot_id) const;

private:
	/**
	 * Process a single equipment renderer and get posed geometry.
	 */
	std::optional<EquipmentGeometry> _process_equipment_renderer(
		M2RendererGL* renderer,
		std::optional<uint32_t> attachment_id,
		bool is_collection_style,
		const std::vector<float>* char_bone_matrices,
		bool apply_pose);

	M2RendererGL* char_renderer_;
	std::unordered_map<int, SlotEntry> equipment_renderers_;
	std::unordered_map<int, SlotEntry> collection_renderers_;
};
