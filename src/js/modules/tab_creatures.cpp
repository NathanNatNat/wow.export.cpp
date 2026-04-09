/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "tab_creatures.h"
#include "../log.h"
#include "../core.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/blte-reader.h"
#include "../casc/blp.h"
#include "../casc/casc-source.h"
#include "../install-type.h"
#include "../modules.h"
#include "../ui/listbox-context.h"
#include "../ui/texture-ribbon.h"
#include "../ui/texture-exporter.h"
#include "../ui/model-viewer-utils.h"
#include "../ui/character-appearance.h"
#include "../3D/renderers/CharMaterialRenderer.h"
#include "../3D/renderers/M2RendererGL.h"
#include "../3D/renderers/M3RendererGL.h"
#include "../3D/renderers/WMORendererGL.h"
#include "../3D/exporters/M2Exporter.h"
#include "../db/caches/DBModelFileData.h"
#include "../db/caches/DBCreatures.h"
#include "../db/caches/DBCreatureList.h"
#include "../db/caches/DBCharacterCustomization.h"
#include "../db/caches/DBCreatureDisplayExtra.h"
#include "../db/caches/DBNpcEquipment.h"
#include "../db/caches/DBItemModels.h"
#include "../db/caches/DBItemGeosets.h"
#include "../db/caches/DBItemCharTextures.h"
#include "../db/caches/DBItems.h"
#include "../wow/EquipmentSlots.h"
#include "../file-writer.h"
#include "../components/model-viewer-gl.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_creatures {

// --- File-local structures ---

// JS: equipment_model_renderers entries: { renderers: [{ renderer, attachment_id }], display_id }
struct EquipmentModelEntry {
	struct RendererInfo {
		std::unique_ptr<M2RendererGL> renderer;
		int attachment_id = 0;
	};
	std::vector<RendererInfo> renderers;
	uint32_t display_id = 0;
};

// JS: collection_model_renderers entries: { renderers: [M2RendererGL], display_id }
struct CollectionModelEntry {
	std::vector<std::unique_ptr<M2RendererGL>> renderers;
	uint32_t display_id = 0;
};

// JS: creature_equipment entries: { display_id, item_id? }
struct EquipmentSlotEntry {
	uint32_t display_id = 0;
	std::optional<uint32_t> item_id;
};

// JS: build_equipment_checklist entries: { id, label, checked }
struct EquipmentChecklistEntry {
	int id = 0;
	std::string label;
	bool checked = true;
};

// JS: SLOT_TO_GEOSET_GROUPS entries
struct SlotGeosetMapping {
	int group_index;
	int char_geoset;
};

// --- File-local state ---

// JS: const active_skins = new Map();
static std::map<std::string, db::caches::DBCreatures::CreatureDisplayInfo> active_skins;

// JS: let selected_variant_texture_ids = new Array();
static std::vector<uint32_t> selected_variant_texture_ids;

// JS: let active_renderer;
static model_viewer_utils::RendererResult active_renderer_result;

// JS: let active_file_data_id;
static uint32_t active_file_data_id = 0;

// JS: let active_creature;
static const db::caches::DBCreatureList::CreatureEntry* active_creature = nullptr;

// JS: let is_character_model = false;
static bool is_character_model = false;

// JS: const creature_chr_materials = new Map();
static std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>> creature_chr_materials;

// equipment state
// JS: const equipment_model_renderers = new Map();
static std::map<int, EquipmentModelEntry> equipment_model_renderers;

// JS: const collection_model_renderers = new Map();
static std::map<int, CollectionModelEntry> collection_model_renderers;

// JS: let creature_equipment = null;
static std::optional<std::map<int, EquipmentSlotEntry>> creature_equipment;

// JS: creature_equipment._checklist — stored separately since C++ maps can't have ad-hoc properties
static std::vector<EquipmentChecklistEntry> creature_equipment_checklist;

// JS: let creature_extra_info = null;
static const db::caches::DBCreatureDisplayExtra::ExtraInfo* creature_extra_info = nullptr;

// JS: let creature_layout_id = 0;
static uint32_t creature_layout_id = 0;

// JS: let equipment_refresh_lock = false;
static bool equipment_refresh_lock = false;

// CG constants from DBItemGeosets
// JS: const CG = DBItemGeosets.CG;
namespace CG = db::caches::DBItemGeosets::CG;

// JS: const SLOT_TO_GEOSET_GROUPS = { ... };
// slot id to geoset group mapping for collection models
static const std::unordered_map<int, std::vector<SlotGeosetMapping>> SLOT_TO_GEOSET_GROUPS = {
	{ 1, { { 0, CG::HELM }, { 1, CG::SKULL } } },
	{ 5, { { 0, CG::SLEEVES }, { 1, CG::CHEST }, { 2, CG::TROUSERS }, { 3, CG::TORSO }, { 4, CG::ARM_UPPER } } },
	{ 6, { { 0, CG::BELT } } },
	{ 7, { { 0, CG::PANTS }, { 1, CG::KNEEPADS }, { 2, CG::TROUSERS } } },
	{ 8, { { 0, CG::BOOTS }, { 1, CG::FEET } } },
	{ 10, { { 0, CG::GLOVES }, { 1, CG::HAND_ATTACHMENT } } },
	{ 15, { { 0, CG::CLOAK } } }
};

// View state proxy (created once in mounted()).
static model_viewer_utils::ViewStateProxy view_state;

// Animation methods helper.
static std::unique_ptr<model_viewer_utils::AnimationMethods> anim_methods;

// Change-detection for watches (replaces Vue $watch).
static std::vector<nlohmann::json> prev_skins_selection;
static std::string prev_anim_selection;
static std::vector<nlohmann::json> prev_selection_creatures;
static std::vector<bool> prev_equipment_checked;

static bool is_initialized = false;

// Model viewer GL state/context (replaces Vue <ModelViewerGL :context="creatureViewerContext"/>).
static model_viewer_gl::State viewer_state;
static model_viewer_gl::Context viewer_context;

// Adapter maps for model-viewer-gl equipment/collection renderer callbacks.
// These hold non-owning pointers into equipment_model_renderers/collection_model_renderers.
static std::unordered_map<int, model_viewer_gl::EquipmentSlotRenderers> equip_adapter_map;
static std::unordered_map<int, model_viewer_gl::CollectionSlotRenderers> coll_adapter_map;

// Rebuild adapter maps from owned equipment/collection renderer storage.
static void rebuild_renderer_adapter_maps() {
	equip_adapter_map.clear();
	for (auto& [slot_id, entry] : equipment_model_renderers) {
		model_viewer_gl::EquipmentSlotRenderers slot;
		for (auto& ri : entry.renderers) {
			model_viewer_gl::EquipmentRendererEntry e;
			e.renderer = ri.renderer.get();
			e.attachment_id = ri.attachment_id;
			e.is_collection_style = false;
			slot.renderers.push_back(e);
		}
		equip_adapter_map[slot_id] = std::move(slot);
	}
	coll_adapter_map.clear();
	for (auto& [slot_id, entry] : collection_model_renderers) {
		model_viewer_gl::CollectionSlotRenderers slot;
		for (auto& renderer : entry.renderers)
			slot.renderers.push_back(renderer.get());
		coll_adapter_map[slot_id] = std::move(slot);
	}
}

// --- Internal helpers ---

// Helper to get the active M2 renderer (or nullptr).
static M2RendererGL* get_active_m2_renderer() {
	return active_renderer_result.m2.get();
}

// Helper to get the view state proxy pointer.
static model_viewer_utils::ViewStateProxy* get_view_state_ptr() {
	return &view_state;
}

// JS: const get_creature_displays = (file_data_id) => { ... }
static std::vector<db::caches::DBCreatures::CreatureDisplayInfo> get_creature_displays(uint32_t file_data_id) {
	// JS: return DBCreatures.getCreatureDisplaysByFileDataID(file_data_id) ?? [];
	const auto* displays = db::caches::DBCreatures::getCreatureDisplaysByFileDataID(file_data_id);
	if (displays)
		return *displays;
	return {};
}

/**
 * Build equipment data for a character-model creature.
 * Returns Map<slot_id, { display_id, item_id? }> or nullopt.
 *
 * JS: const build_creature_equipment = (extra_display_id, creature) => { ... }
 */
static std::optional<std::map<int, EquipmentSlotEntry>> build_creature_equipment(
	uint32_t extra_display_id,
	const db::caches::DBCreatureList::CreatureEntry& creature
) {
	std::map<int, EquipmentSlotEntry> equipment;

	// armor from NpcModelItemSlotDisplayInfo (display-ID-based)
	// JS: const npc_armor = DBNpcEquipment.get_equipment(extra_display_id);
	const auto* npc_armor = db::caches::DBNpcEquipment::get_equipment(extra_display_id);
	if (npc_armor) {
		// JS: for (const [slot_id, display_id] of npc_armor)
		for (const auto& [slot_id, display_id] : *npc_armor)
			equipment[slot_id] = { display_id, std::nullopt };
	}

	// weapons from Creature.AlwaysItem (item-ID-based)
	// JS: if (creature.always_items) { ... }
	if (!creature.always_items.empty()) {
		size_t count = (std::min)(creature.always_items.size(), static_cast<size_t>(2));
		for (size_t i = 0; i < count; i++) {
			uint32_t item_id = creature.always_items[i];
			int slot_id = (i == 0) ? 16 : 17;
			auto display_id = db::caches::DBItemModels::getDisplayId(item_id);
			if (display_id.has_value())
				equipment[slot_id] = { display_id.value(), item_id };
		}
	}

	// JS: return equipment.size > 0 ? equipment : null;
	if (!equipment.empty())
		return equipment;
	return std::nullopt;
}

/**
 * Build checklist array for equipment toggle UI.
 *
 * JS: const build_equipment_checklist = (equipment) => { ... }
 */
static std::vector<EquipmentChecklistEntry> build_equipment_checklist(
	const std::map<int, EquipmentSlotEntry>& equipment
) {
	std::vector<EquipmentChecklistEntry> list;

	// JS: for (const [slot_id, entry] of equipment) { ... }
	for (const auto& [slot_id, entry] : equipment) {
		// JS: const slot_name = get_slot_name(slot_id) ?? 'Slot ' + slot_id;
		auto slot_name_opt = wow::get_slot_name(slot_id);
		std::string slot_name = slot_name_opt.has_value()
			? std::string(slot_name_opt.value())
			: ("Slot " + std::to_string(slot_id));

		// JS: list.push({ id: slot_id, label: slot_name + ' (' + entry.display_id + ')', checked: true });
		list.push_back({
			slot_id,
			slot_name + " (" + std::to_string(entry.display_id) + ")",
			true
		});
	}

	// JS: list.sort((a, b) => a.id - b.id);
	std::sort(list.begin(), list.end(), [](const EquipmentChecklistEntry& a, const EquipmentChecklistEntry& b) {
		return a.id < b.id;
	});

	return list;
}

/**
 * Get enabled equipment slots from the checklist.
 *
 * JS: const get_enabled_equipment = () => { ... }
 */
static std::optional<std::map<int, EquipmentSlotEntry>> get_enabled_equipment() {
	// JS: if (!creature_equipment) return null;
	if (!creature_equipment.has_value())
		return std::nullopt;

	// JS: const checklist = creature_equipment._checklist;
	// JS: if (!checklist) return creature_equipment;
	if (creature_equipment_checklist.empty())
		return creature_equipment;

	std::map<int, EquipmentSlotEntry> enabled;

	// JS: for (const item of checklist) { ... }
	for (const auto& item : creature_equipment_checklist) {
		if (item.checked && creature_equipment->contains(item.id))
			enabled[item.id] = creature_equipment->at(item.id);
	}

	// JS: return enabled.size > 0 ? enabled : null;
	if (!enabled.empty())
		return enabled;
	return std::nullopt;
}

/**
 * Apply equipment geosets to creature character model.
 *
 * JS: const apply_creature_equipment_geosets = (core) => { ... }
 */
static void apply_creature_equipment_geosets() {
	// JS: if (!active_renderer || !is_character_model) return;
	if (!active_renderer_result.m2 || !is_character_model)
		return;

	auto& view = *core::view;

	// JS: const geosets = core.view.creatureViewerGeosets;
	auto& geosets = view.creatureViewerGeosets;
	// JS: if (!geosets || geosets.length === 0) return;
	if (geosets.empty())
		return;

	// JS: const enabled = get_enabled_equipment();
	auto enabled = get_enabled_equipment();
	// JS: if (!enabled) return;
	if (!enabled.has_value())
		return;

	// build display-id-based slot map for armor
	// JS: const slot_display_map = new Map();
	std::unordered_map<int, uint32_t> slot_display_map;
	// JS: for (const [slot_id, entry] of enabled) { ... }
	for (const auto& [slot_id, entry] : *enabled) {
		if (slot_id <= 19)
			slot_display_map[slot_id] = entry.display_id;
	}

	// JS: if (slot_display_map.size === 0) return;
	if (slot_display_map.empty())
		return;

	// JS: const equipment_geosets = DBItemGeosets.calculateEquipmentGeosetsByDisplay(slot_display_map);
	auto equipment_geosets = db::caches::DBItemGeosets::calculateEquipmentGeosetsByDisplay(slot_display_map);

	// JS: const affected_groups = DBItemGeosets.getAffectedCharGeosetsByDisplay(slot_display_map);
	auto affected_groups = db::caches::DBItemGeosets::getAffectedCharGeosetsByDisplay(slot_display_map);

	// JS: for (const char_geoset of affected_groups) { ... }
	for (int char_geoset : affected_groups) {
		int base = char_geoset * 100;
		int range_start = base + 1;
		int range_end = base + 99;

		// JS: for (const geoset of geosets) { if (geoset.id >= range_start && geoset.id <= range_end) geoset.checked = false; }
		for (auto& geoset : geosets) {
			int gid = geoset.value("id", 0);
			if (gid >= range_start && gid <= range_end)
				geoset["checked"] = false;
		}

		// JS: const value = equipment_geosets.get(char_geoset);
		auto it = equipment_geosets.find(char_geoset);
		if (it != equipment_geosets.end()) {
			int target_geoset_id = base + it->second;
			// JS: for (const geoset of geosets) { if (geoset.id === target_geoset_id) geoset.checked = true; }
			for (auto& geoset : geosets) {
				if (geoset.value("id", 0) == target_geoset_id)
					geoset["checked"] = true;
			}
		}
	}

	// helmet hide geosets
	// JS: const head_entry = enabled.get(1);
	auto head_it = enabled->find(1);
	if (head_it != enabled->end() && creature_extra_info) {
		// JS: const hide_groups = DBItemGeosets.getHelmetHideGeosetsByDisplayId(...)
		auto hide_groups = db::caches::DBItemGeosets::getHelmetHideGeosetsByDisplayId(
			head_it->second.display_id,
			creature_extra_info->DisplayRaceID,
			static_cast<int>(creature_extra_info->DisplaySexID)
		);

		// JS: for (const char_geoset of hide_groups) { ... }
		for (int char_geoset : hide_groups) {
			int base = char_geoset * 100;
			int range_start = base + 1;
			int range_end = base + 99;

			for (auto& geoset : geosets) {
				int gid = geoset.value("id", 0);
				if (gid >= range_start && gid <= range_end)
					geoset["checked"] = false;
			}
		}
	}

	// JS: active_renderer.updateGeosets();
	active_renderer_result.m2->updateGeosets();
}

/**
 * Apply equipment textures to creature character model.
 *
 * JS: const apply_creature_equipment_textures = async (core) => { ... }
 */
static void apply_creature_equipment_textures() {
	// JS: if (!active_renderer || !is_character_model) return;
	if (!active_renderer_result.m2 || !is_character_model)
		return;

	// JS: const enabled = get_enabled_equipment();
	auto enabled = get_enabled_equipment();
	// JS: if (!enabled || creature_layout_id === 0) return;
	if (!enabled.has_value() || creature_layout_id == 0)
		return;

	// JS: const sections = DBCharacterCustomization.get_texture_sections(creature_layout_id);
	const auto* sections = db::caches::DBCharacterCustomization::get_texture_sections(creature_layout_id);
	// JS: if (!sections) return;
	if (!sections)
		return;

	// JS: const section_by_type = new Map();
	std::unordered_map<int, const db::DataRecord*> section_by_type;
	// JS: for (const section of sections) section_by_type.set(section.SectionType, section);
	for (const auto& section : *sections) {
		int section_type = static_cast<int>(std::get<int64_t>(section.at("SectionType")));
		section_by_type[section_type] = &section;
	}

	// JS: const texture_layer_map = DBCharacterCustomization.get_model_texture_layer_map();
	const auto& texture_layer_map = db::caches::DBCharacterCustomization::get_model_texture_layer_map();

	// JS: let base_layer = null;
	const db::DataRecord* base_layer = nullptr;
	std::string layout_prefix = std::to_string(creature_layout_id) + "-";

	// JS: for (const [key, layer] of texture_layer_map) { ... }
	for (const auto& [key, layer] : texture_layer_map) {
		if (key.substr(0, layout_prefix.size()) != layout_prefix)
			continue;

		int bitmask = static_cast<int>(std::get<int64_t>(layer.at("TextureSectionTypeBitMask")));
		int tex_type = static_cast<int>(std::get<int64_t>(layer.at("TextureType")));

		// JS: if (layer.TextureSectionTypeBitMask === -1 && layer.TextureType === 1) { base_layer = layer; break; }
		if (bitmask == -1 && tex_type == 1) {
			base_layer = &layer;
			break;
		}
	}

	// JS: const layers_by_section = new Map();
	std::unordered_map<int, const db::DataRecord*> layers_by_section;
	for (const auto& [key, layer] : texture_layer_map) {
		if (key.substr(0, layout_prefix.size()) != layout_prefix)
			continue;

		int bitmask = static_cast<int>(std::get<int64_t>(layer.at("TextureSectionTypeBitMask")));
		// JS: if (layer.TextureSectionTypeBitMask === -1) continue;
		if (bitmask == -1)
			continue;

		// JS: for (let section_type = 0; section_type < 9; section_type++) { ... }
		for (int section_type = 0; section_type < 9; section_type++) {
			if ((1 << section_type) & bitmask) {
				if (!layers_by_section.contains(section_type))
					layers_by_section[section_type] = &layer;
			}
		}
	}

	// JS: if (base_layer) { for (let section_type = 0; section_type < 9; section_type++) { ... } }
	if (base_layer) {
		for (int section_type = 0; section_type < 9; section_type++) {
			if (!layers_by_section.contains(section_type))
				layers_by_section[section_type] = base_layer;
		}
	}

	// JS: for (const [slot_id, entry] of enabled) { ... }
	for (const auto& [slot_id, entry] : *enabled) {
		// JS: const item_textures = entry.item_id ? ... : ...;
		int race_id = creature_extra_info ? static_cast<int>(creature_extra_info->DisplayRaceID) : -1;
		int gender_index = creature_extra_info ? static_cast<int>(creature_extra_info->DisplaySexID) : -1;

		std::optional<std::vector<db::caches::DBItemCharTextures::TextureComponent>> item_textures;
		if (entry.item_id.has_value())
			item_textures = db::caches::DBItemCharTextures::getItemTextures(entry.item_id.value(), race_id, gender_index);
		else
			item_textures = db::caches::DBItemCharTextures::getTexturesByDisplayId(entry.display_id, race_id, gender_index);

		// JS: if (!item_textures) continue;
		if (!item_textures.has_value())
			continue;

		// JS: for (const texture of item_textures) { ... }
		for (const auto& texture : *item_textures) {
			// JS: const section = section_by_type.get(texture.section);
			auto section_it = section_by_type.find(texture.section);
			if (section_it == section_by_type.end())
				continue;

			// JS: const layer = layers_by_section.get(texture.section);
			auto layer_it = layers_by_section.find(texture.section);
			if (layer_it == layers_by_section.end())
				continue;

			const db::DataRecord& section_rec = *section_it->second;
			const db::DataRecord& layer_rec = *layer_it->second;

			int layer_tex_type = static_cast<int>(std::get<int64_t>(layer_rec.at("TextureType")));

			// JS: const chr_model_material = DBCharacterCustomization.get_model_material(creature_layout_id, layer.TextureType);
			const auto* chr_model_material = db::caches::DBCharacterCustomization::get_model_material(
				creature_layout_id, static_cast<uint32_t>(layer_tex_type)
			);
			if (!chr_model_material)
				continue;

			int mat_texture_type = static_cast<int>(std::get<int64_t>(chr_model_material->at("TextureType")));
			int mat_width = static_cast<int>(std::get<int64_t>(chr_model_material->at("Width")));
			int mat_height = static_cast<int>(std::get<int64_t>(chr_model_material->at("Height")));

			// JS: let chr_material;
			// JS: if (!creature_chr_materials.has(chr_model_material.TextureType)) { ... } else { ... }
			uint32_t mat_type_key = static_cast<uint32_t>(mat_texture_type);
			if (!creature_chr_materials.contains(mat_type_key)) {
				auto chr_material = std::make_unique<CharMaterialRenderer>(mat_texture_type, mat_width, mat_height);
				chr_material->init();
				creature_chr_materials[mat_type_key] = std::move(chr_material);
			}

			auto& chr_material = creature_chr_materials[mat_type_key];

			// JS: const slot_layer = get_slot_layer(slot_id);
			int slot_layer = wow::get_slot_layer(slot_id);

			// JS: const item_material = { ChrModelTextureTargetID: (slot_layer * 100) + texture.section, FileDataID: texture.fileDataID };
			int chr_model_texture_target_id = (slot_layer * 100) + texture.section;

			// JS: await chr_material.setTextureTarget(item_material, section, chr_model_material, layer, true);
			int section_x = static_cast<int>(std::get<int64_t>(section_rec.at("X")));
			int section_y = static_cast<int>(std::get<int64_t>(section_rec.at("Y")));
			int section_w = static_cast<int>(std::get<int64_t>(section_rec.at("Width")));
			int section_h = static_cast<int>(std::get<int64_t>(section_rec.at("Height")));
			int layer_blend_mode = static_cast<int>(std::get<int64_t>(layer_rec.at("BlendMode")));

			chr_material->setTextureTarget(
				chr_model_texture_target_id,
				texture.fileDataID,
				section_x, section_y, section_w, section_h,
				mat_texture_type, mat_width, mat_height,
				layer_blend_mode,
				true
			);
		}
	}
}

/**
 * Apply equipment 3D models (weapons, shoulders, helmets, capes, etc.).
 *
 * JS: const apply_creature_equipment_models = async (core) => { ... }
 */
static void apply_creature_equipment_models() {
	// JS: if (!active_renderer || !is_character_model) return;
	if (!active_renderer_result.m2 || !is_character_model)
		return;

	// JS: const gl_context = core.view.creatureViewerContext?.gl_context;
	// JS: if (!gl_context) return;
	gl::GLContext* gl_ctx = viewer_context.gl_context;
	if (!gl_ctx) return;

	auto enabled = get_enabled_equipment();

	// dispose models for slots no longer enabled
	// JS: for (const slot_id of equipment_model_renderers.keys()) { ... }
	for (auto it = equipment_model_renderers.begin(); it != equipment_model_renderers.end(); ) {
		if (!enabled.has_value() || !enabled->contains(it->first)) {
			// JS: for (const { renderer } of entry.renderers) renderer.dispose();
			for (auto& ri : it->second.renderers) {
				if (ri.renderer)
					ri.renderer->dispose();
			}
			it = equipment_model_renderers.erase(it);
		} else {
			++it;
		}
	}

	// JS: for (const slot_id of collection_model_renderers.keys()) { ... }
	for (auto it = collection_model_renderers.begin(); it != collection_model_renderers.end(); ) {
		if (!enabled.has_value() || !enabled->contains(it->first)) {
			for (auto& renderer : it->second.renderers) {
				if (renderer)
					renderer->dispose();
			}
			it = collection_model_renderers.erase(it);
		} else {
			++it;
		}
	}

	// JS: if (!enabled) return;
	if (!enabled.has_value())
		return;

	// JS: const race_id = creature_extra_info?.DisplayRaceID;
	int race_id = creature_extra_info ? static_cast<int>(creature_extra_info->DisplayRaceID) : -1;
	// JS: const gender_index = creature_extra_info?.DisplaySexID;
	int gender_index = creature_extra_info ? static_cast<int>(creature_extra_info->DisplaySexID) : -1;

	// JS: for (const [slot_id, entry] of enabled) { ... }
	for (const auto& [slot_id, entry] : *enabled) {
		// JS: const existing_equip = equipment_model_renderers.get(slot_id);
		auto existing_equip_it = equipment_model_renderers.find(slot_id);
		// JS: const existing_coll = collection_model_renderers.get(slot_id);
		auto existing_coll_it = collection_model_renderers.find(slot_id);

		bool has_existing_equip = (existing_equip_it != equipment_model_renderers.end());
		bool has_existing_coll = (existing_coll_it != collection_model_renderers.end());

		// JS: if ((existing_equip?.display_id === entry.display_id) && (...)) continue;
		if (has_existing_equip && existing_equip_it->second.display_id == entry.display_id &&
			(!has_existing_coll || existing_coll_it->second.display_id == entry.display_id))
			continue;

		// dispose old if display changed
		// JS: if (existing_equip) { ... }
		if (has_existing_equip) {
			for (auto& ri : existing_equip_it->second.renderers) {
				if (ri.renderer)
					ri.renderer->dispose();
			}
			equipment_model_renderers.erase(existing_equip_it);
		}

		// JS: if (existing_coll) { ... }
		if (has_existing_coll) {
			for (auto& renderer : existing_coll_it->second.renderers) {
				if (renderer)
					renderer->dispose();
			}
			collection_model_renderers.erase(existing_coll_it);
		}

		// JS: const display = entry.item_id ? DBItemModels.getItemDisplay(...) : DBItemModels.getDisplayData(...);
		std::optional<db::caches::DBItemModels::ItemDisplayData> display;
		if (entry.item_id.has_value())
			display = db::caches::DBItemModels::getItemDisplay(entry.item_id.value(), race_id, gender_index);
		else
			display = db::caches::DBItemModels::getDisplayData(entry.display_id, race_id, gender_index);

		// JS: if (!display?.models || display.models.length === 0) continue;
		if (!display.has_value() || display->models.empty())
			continue;

		// bows held in left hand
		// JS: let attachment_ids = get_attachment_ids_for_slot(slot_id) || [];
		auto attachment_ids_opt = wow::get_attachment_ids_for_slot(slot_id);
		std::vector<int> attachment_ids;
		if (attachment_ids_opt.has_value()) {
			for (int aid : attachment_ids_opt.value())
				attachment_ids.push_back(aid);
		}

		// JS: if (slot_id === 16 && entry.item_id && DBItems.isItemBow(entry.item_id))
		if (slot_id == 16 && entry.item_id.has_value() && db::caches::DBItems::isItemBow(entry.item_id.value()))
			attachment_ids = { wow::ATTACHMENT_ID::HAND_LEFT };

		// JS: const attachment_model_count = Math.min(display.models.length, attachment_ids.length);
		size_t attachment_model_count = (std::min)(display->models.size(), attachment_ids.size());
		// JS: const collection_start_index = attachment_model_count;
		size_t collection_start_index = attachment_model_count;

		// attachment models
		// JS: if (attachment_model_count > 0) { ... }
		if (attachment_model_count > 0) {
			EquipmentModelEntry equip_entry;
			equip_entry.display_id = entry.display_id;

			for (size_t i = 0; i < attachment_model_count; i++) {
				uint32_t fdid = display->models[i];
				int attachment_id = attachment_ids[i];

				try {
					// JS: const file = await core.view.casc.getFile(file_data_id);
					BufferWrapper file = core::view->casc->getVirtualFileByID(fdid);
					// JS: const renderer = new M2RendererGL(file, gl_context, false, false);
					auto renderer = std::make_unique<M2RendererGL>(file, *gl_ctx, false, false);
					// JS: await renderer.load();
					renderer->load();
					// JS: if (display.textures && display.textures.length > i)
					//     await renderer.applyReplaceableTextures({ textures: [display.textures[i]] });
					if (!display->textures.empty() && display->textures.size() > i) {
						M2DisplayInfo disp_info;
						disp_info.textures = { display->textures[i] };
						renderer->applyReplaceableTextures(disp_info);
					}
					// JS: renderers.push({ renderer, attachment_id });
					EquipmentModelEntry::RendererInfo ri;
					ri.renderer = std::move(renderer);
					ri.attachment_id = attachment_id;
					equip_entry.renderers.push_back(std::move(ri));
					logging::write(std::format("Loaded creature attachment model {} for slot {}", fdid, slot_id));
				} catch (const std::exception& e) {
					// JS: log.write('Failed to load creature attachment model %d: %s', file_data_id, e.message);
					logging::write(std::format("Failed to load creature attachment model {}: {}", fdid, e.what()));
				}
			}

			if (!equip_entry.renderers.empty())
				equipment_model_renderers[slot_id] = std::move(equip_entry);
		}

		// collection models
		// JS: if (display.models.length > collection_start_index) { ... }
		if (display->models.size() > collection_start_index) {
			CollectionModelEntry coll_entry;
			coll_entry.display_id = entry.display_id;

			for (size_t i = collection_start_index; i < display->models.size(); i++) {
				uint32_t fdid = display->models[i];

				try {
					// JS: const file = await core.view.casc.getFile(file_data_id);
					BufferWrapper file = core::view->casc->getVirtualFileByID(fdid);
					// JS: const renderer = new M2RendererGL(file, gl_context, false, false);
					auto renderer = std::make_unique<M2RendererGL>(file, *gl_ctx, false, false);
					// JS: await renderer.load();
					renderer->load();
					// JS: if (active_renderer?.bones) renderer.buildBoneRemapTable(active_renderer.bones);
					if (active_renderer_result.m2 && active_renderer_result.m2->get_bones_m2())
						renderer->buildBoneRemapTable(*active_renderer_result.m2->get_bones_m2());
					// JS: const slot_geosets = SLOT_TO_GEOSET_GROUPS[slot_id];
					// JS: if (slot_geosets && display.attachmentGeosetGroup) { ... }
					auto sgit = SLOT_TO_GEOSET_GROUPS.find(slot_id);
					if (sgit != SLOT_TO_GEOSET_GROUPS.end() && !display->attachmentGeosetGroup.empty()) {
						size_t coll_idx = i - collection_start_index;
						if (coll_idx < display->attachmentGeosetGroup.size()) {
							int geo_group_val = display->attachmentGeosetGroup[coll_idx];
							for (const auto& mapping : sgit->second)
								renderer->setGeosetGroupDisplay(mapping.group_index, 1 + geo_group_val);
						}
					}
					// JS: const texture_idx = i < display.textures?.length ? i : 0;
					size_t texture_idx = (i < display->textures.size()) ? i : 0;
					// JS: if (texture_fdid) await renderer.applyReplaceableTextures({ textures: [texture_fdid] });
					if (texture_idx < display->textures.size() && display->textures[texture_idx] != 0) {
						M2DisplayInfo disp_info;
						disp_info.textures = { display->textures[texture_idx] };
						renderer->applyReplaceableTextures(disp_info);
					}
					// JS: renderers.push(renderer);
					coll_entry.renderers.push_back(std::move(renderer));
					logging::write(std::format("Loaded creature collection model {} for slot {}", fdid, slot_id));
				} catch (const std::exception& e) {
					// JS: log.write('Failed to load creature collection model %d: %s', file_data_id, e.message);
					logging::write(std::format("Failed to load creature collection model {}: {}", fdid, e.what()));
				}
			}

			if (!coll_entry.renderers.empty())
				collection_model_renderers[slot_id] = std::move(coll_entry);
		}
	}
}

/**
 * Dispose all creature equipment model renderers.
 *
 * JS: const dispose_creature_equipment = () => { ... }
 */
static void dispose_creature_equipment() {
	// JS: for (const entry of equipment_model_renderers.values()) { ... }
	for (auto& [slot_id, entry] : equipment_model_renderers) {
		for (auto& ri : entry.renderers) {
			if (ri.renderer)
				ri.renderer->dispose();
		}
	}
	equipment_model_renderers.clear();

	// JS: for (const entry of collection_model_renderers.values()) { ... }
	for (auto& [slot_id, entry] : collection_model_renderers) {
		for (auto& renderer : entry.renderers) {
			if (renderer)
				renderer->dispose();
		}
	}
	collection_model_renderers.clear();

	// JS: creature_equipment = null;
	creature_equipment = std::nullopt;
	creature_equipment_checklist.clear();
	// JS: creature_extra_info = null;
	creature_extra_info = nullptr;
	// JS: creature_layout_id = 0;
	creature_layout_id = 0;
}

/**
 * Full equipment refresh: geosets, textures, models.
 *
 * JS: const refresh_creature_equipment = async (core) => { ... }
 */
static void refresh_creature_equipment() {
	// JS: if (!active_renderer || !is_character_model || !creature_equipment) return;
	if (!active_renderer_result.m2 || !is_character_model || !creature_equipment.has_value())
		return;

	auto& view = *core::view;

	// re-apply customization geosets first (reset)
	// JS: const display_info = DBCreatures.getDisplayInfo(active_creature.displayID);
	const auto* display_info = db::caches::DBCreatures::getDisplayInfo(active_creature->displayID);
	if (!display_info)
		return;

	// JS: const customization_choices = DBCreatureDisplayExtra.get_customization_choices(display_info.extendedDisplayInfoID);
	const auto& customization_choices_raw = db::caches::DBCreatureDisplayExtra::get_customization_choices(display_info->extendedDisplayInfoID);
	// Convert to JSON array for character_appearance API
	std::vector<nlohmann::json> customization_choices;
	for (const auto& choice : customization_choices_raw) {
		nlohmann::json j;
		j["optionID"] = choice.optionID;
		j["choiceID"] = choice.choiceID;
		customization_choices.push_back(std::move(j));
	}

	// JS: character_appearance.apply_customization_geosets(core.view.creatureViewerGeosets, customization_choices);
	character_appearance::apply_customization_geosets(view.creatureViewerGeosets, customization_choices);

	// re-apply customization textures (reset materials)
	// JS: let baked_npc_blp = null;
	std::unique_ptr<casc::BLPImage> baked_npc_blp;
	// JS: const bake_id = creature_extra_info.HDBakeMaterialResourcesID || creature_extra_info.BakeMaterialResourcesID;
	uint32_t bake_id = creature_extra_info->HDBakeMaterialResourcesID;
	if (bake_id == 0)
		bake_id = creature_extra_info->BakeMaterialResourcesID;

	if (bake_id > 0) {
		// JS: const bake_fdid = DBCharacterCustomization.get_texture_file_data_id(bake_id);
		uint32_t bake_fdid = db::caches::DBCharacterCustomization::get_texture_file_data_id(bake_id);
		if (bake_fdid != 0) {
			try {
				// JS: const bake_data = await core.view.casc.getFile(bake_fdid);
				// JS: baked_npc_blp = new BLPFile(bake_data);
				BufferWrapper bake_data = core::view->casc->getVirtualFileByID(bake_fdid);
				baked_npc_blp = std::make_unique<casc::BLPImage>(bake_data);
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to load baked NPC texture {}: {}", bake_fdid, e.what()));
			}
		}
	}

	// JS: await character_appearance.apply_customization_textures(active_renderer, customization_choices, creature_layout_id, creature_chr_materials, baked_npc_blp);
	character_appearance::apply_customization_textures(
		active_renderer_result.m2.get(),
		customization_choices,
		creature_layout_id,
		creature_chr_materials,
		baked_npc_blp.get()
	);

	// apply equipment on top
	// JS: apply_creature_equipment_geosets(core);
	apply_creature_equipment_geosets();
	// JS: await apply_creature_equipment_textures(core);
	apply_creature_equipment_textures();
	// JS: await character_appearance.upload_textures_to_gpu(active_renderer, creature_chr_materials);
	character_appearance::upload_textures_to_gpu(active_renderer_result.m2.get(), creature_chr_materials);
	// JS: await apply_creature_equipment_models(core);
	apply_creature_equipment_models();
}

// JS: const preview_creature = async (core, creature) => { ... }
static void preview_creature(const db::caches::DBCreatureList::CreatureEntry& creature) {
	auto _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", creature.name), {}, -1, false);
	logging::write(std::format("Previewing creature {} (ID: {})", creature.name, creature.id));

	auto& state = view_state;
	texture_ribbon::reset();
	model_viewer_utils::clear_texture_preview(state);

	auto& view = *core::view;
	// JS: core.view.creatureViewerSkins = [];
	view.creatureViewerSkins.clear();
	// JS: core.view.creatureViewerSkinsSelection = [];
	view.creatureViewerSkinsSelection.clear();
	// JS: core.view.creatureViewerAnims = [];
	view.creatureViewerAnims.clear();
	// JS: core.view.creatureViewerAnimSelection = null;
	view.creatureViewerAnimSelection = nullptr;

	try {
		// JS: if (active_renderer) { active_renderer.dispose(); active_renderer = null; ... }
		if (active_renderer_result.type != model_viewer_utils::ModelType::Unknown) {
			active_renderer_result = model_viewer_utils::RendererResult{};
			active_file_data_id = 0;
			active_creature = nullptr;
		}

		// JS: character_appearance.dispose_materials(creature_chr_materials);
		character_appearance::dispose_materials(creature_chr_materials);
		// JS: dispose_creature_equipment();
		dispose_creature_equipment();
		// JS: active_skins.clear();
		active_skins.clear();
		// JS: selected_variant_texture_ids.length = 0;
		selected_variant_texture_ids.clear();
		// JS: is_character_model = false;
		is_character_model = false;
		// JS: core.view.creatureViewerEquipment = [];
		view.creatureViewerEquipment.clear();

		// JS: const display_info = DBCreatures.getDisplayInfo(creature.displayID);
		const auto* display_info = db::caches::DBCreatures::getDisplayInfo(creature.displayID);

		if (display_info && display_info->extendedDisplayInfoID > 0) {
			// character-model creature
			// JS: const extra = DBCreatureDisplayExtra.get_extra(display_info.extendedDisplayInfoID);
			const auto* extra = db::caches::DBCreatureDisplayExtra::get_extra(display_info->extendedDisplayInfoID);
			if (!extra) {
				core::setToast("error", std::format("No extended display info found for creature {}.", creature.name), {}, -1);
				return;
			}

			// JS: const chr_model_id = DBCharacterCustomization.get_chr_model_id(extra.DisplayRaceID, extra.DisplaySexID);
			auto chr_model_id = db::caches::DBCharacterCustomization::get_chr_model_id(extra->DisplayRaceID, extra->DisplaySexID);
			if (!chr_model_id.has_value()) {
				core::setToast("error", std::format("No character model found for creature {} (race {}, sex {}).", creature.name, extra->DisplayRaceID, extra->DisplaySexID), {}, -1);
				return;
			}

			// JS: const file_data_id = DBCharacterCustomization.get_model_file_data_id(chr_model_id);
			uint32_t file_data_id = db::caches::DBCharacterCustomization::get_model_file_data_id(chr_model_id.value());
			if (file_data_id == 0) {
				core::setToast("error", std::format("No model file found for creature {}.", creature.name), {}, -1);
				return;
			}

			// JS: const file = await core.view.casc.getFile(file_data_id);
			BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);
			// JS: const gl_context = core.view.creatureViewerContext?.gl_context;

			// JS: core.view.creatureViewerActiveType = 'm2';
			view.creatureViewerActiveType = "m2";

			// JS: const gl_context = core.view.creatureViewerContext?.gl_context;
			gl::GLContext* gl_ctx = viewer_context.gl_context;
			if (!gl_ctx) {
				core::setToast("error", "GL context not available — model viewer not initialized.", {}, -1);
				return;
			}

			// JS: active_renderer = new M2RendererGL(file, gl_context, true, true);
			// JS: active_renderer.geosetKey = 'creatureViewerGeosets';
			// JS: await active_renderer.load();
			active_renderer_result.type = model_viewer_utils::ModelType::M2;
			active_renderer_result.m2 = std::make_unique<M2RendererGL>(file, *gl_ctx, true, true);
			active_renderer_result.m2->setGeosetKey("creatureViewerGeosets");
			active_renderer_result.m2->load();

			// apply customization geosets
			auto& geosets = view.creatureViewerGeosets;
			const auto& customization_choices_raw = db::caches::DBCreatureDisplayExtra::get_customization_choices(display_info->extendedDisplayInfoID);
			std::vector<nlohmann::json> customization_choices;
			for (const auto& choice : customization_choices_raw) {
				nlohmann::json j;
				j["optionID"] = choice.optionID;
				j["choiceID"] = choice.choiceID;
				customization_choices.push_back(std::move(j));
			}
			character_appearance::apply_customization_geosets(geosets, customization_choices);
			active_renderer_result.m2->updateGeosets();

			// resolve baked NPC texture
			std::unique_ptr<casc::BLPImage> baked_npc_blp;
			uint32_t bake_id = extra->HDBakeMaterialResourcesID;
			if (bake_id == 0) bake_id = extra->BakeMaterialResourcesID;
			if (bake_id > 0) {
				uint32_t bake_fdid = db::caches::DBCharacterCustomization::get_texture_file_data_id(bake_id);
				if (bake_fdid != 0) {
					try {
						BufferWrapper bake_data = view.casc->getVirtualFileByID(bake_fdid);
						baked_npc_blp = std::make_unique<casc::BLPImage>(bake_data);
					} catch (const std::exception& e) {
						logging::write(std::format("Failed to load baked NPC texture {}: {}", bake_fdid, e.what()));
					}
				}
			}

			// apply customization textures + baked NPC texture
			uint32_t layout_id = db::caches::DBCharacterCustomization::get_texture_layout_id(chr_model_id.value());
			character_appearance::apply_customization_textures(
				active_renderer_result.m2.get(),
				customization_choices,
				layout_id,
				creature_chr_materials,
				baked_npc_blp.get()
			);

			// load and apply equipment
			equipment_refresh_lock = true;
			creature_extra_info = extra;
			creature_layout_id = layout_id;
			creature_equipment = build_creature_equipment(display_info->extendedDisplayInfoID, creature);

			if (creature_equipment.has_value()) {
				creature_equipment_checklist = build_equipment_checklist(*creature_equipment);
				// update view
				view.creatureViewerEquipment.clear();
				for (const auto& item : creature_equipment_checklist) {
					nlohmann::json j;
					j["id"] = item.id;
					j["label"] = item.label;
					j["checked"] = item.checked;
					view.creatureViewerEquipment.push_back(std::move(j));
				}

				apply_creature_equipment_geosets();
				apply_creature_equipment_textures();
			}

			character_appearance::upload_textures_to_gpu(active_renderer_result.m2.get(), creature_chr_materials);

			if (creature_equipment.has_value())
				apply_creature_equipment_models();

			equipment_refresh_lock = false;

			view.creatureViewerAnims = model_viewer_utils::extract_animations(*active_renderer_result.m2);
			view.creatureViewerAnimSelection = "none";

			active_file_data_id = file_data_id;
			active_creature = &creature;
			is_character_model = true;
		} else {
			// standard creature model
			// JS: const file_data_id = DBCreatures.getFileDataIDByDisplayID(creature.displayID);
			uint32_t file_data_id = db::caches::DBCreatures::getFileDataIDByDisplayID(creature.displayID);
			if (file_data_id == 0) {
				core::setToast("error", std::format("No model data found for creature {}.", creature.name), {}, -1);
				return;
			}

			// JS: const file = await core.view.casc.getFile(file_data_id);
			BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);
			// JS: const gl_context = core.view.creatureViewerContext?.gl_context;
			gl::GLContext* gl_ctx = viewer_context.gl_context;
			if (!gl_ctx) {
				core::setToast("error", "GL context not available — model viewer not initialized.", {}, -1);
				return;
			}

			auto model_type = model_viewer_utils::detect_model_type(file);
			std::string file_name = casc::listfile::getByID(file_data_id);
			if (file_name.empty())
				file_name = casc::listfile::formatUnknownFile(file_data_id, model_viewer_utils::get_model_extension(model_type));

			if (model_type == model_viewer_utils::ModelType::M2)
				view.creatureViewerActiveType = "m2";
			else if (model_type == model_viewer_utils::ModelType::WMO)
				view.creatureViewerActiveType = "wmo";
			else
				view.creatureViewerActiveType = "m3";

			active_renderer_result = model_viewer_utils::create_renderer(
				file, model_type, *gl_ctx,
				view.config.value("modelViewerShowTextures", true),
				file_data_id
			);

			if (model_type == model_viewer_utils::ModelType::M2)
				active_renderer_result.m2->setGeosetKey("creatureViewerGeosets");
			else if (model_type == model_viewer_utils::ModelType::WMO) {
				active_renderer_result.wmo->setWmoGroupKey("creatureViewerWMOGroups");
				active_renderer_result.wmo->setWmoSetKey("creatureViewerWMOSets");
			}

			// load renderer
			if (active_renderer_result.m2)
				active_renderer_result.m2->load();
			else if (active_renderer_result.m3)
				active_renderer_result.m3->load();
			else if (active_renderer_result.wmo)
				active_renderer_result.wmo->load();

			if (model_type == model_viewer_utils::ModelType::M2) {
				auto displays = get_creature_displays(file_data_id);

				std::vector<nlohmann::json> skin_list;
				std::string model_name = casc::listfile::getByID(file_data_id);
				// basename — strip path
				{
					auto pos = model_name.rfind('/');
					if (pos != std::string::npos) model_name = model_name.substr(pos + 1);
					pos = model_name.rfind('\\');
					if (pos != std::string::npos) model_name = model_name.substr(pos + 1);
					// JS: path.basename(model_name, 'm2') — strip 'm2' extension
					auto ext_pos = model_name.rfind('.');
					if (ext_pos != std::string::npos) model_name = model_name.substr(0, ext_pos);
				}

				for (const auto& display : displays) {
					if (display.textures.empty())
						continue;

					uint32_t texture = display.textures[0];

					std::string clean_skin_name;
					std::string skin_name = casc::listfile::getByID(texture);
					if (!skin_name.empty()) {
						// basename + strip .blp
						{
							auto pos = skin_name.rfind('/');
							if (pos != std::string::npos) skin_name = skin_name.substr(pos + 1);
							pos = skin_name.rfind('\\');
							if (pos != std::string::npos) skin_name = skin_name.substr(pos + 1);
							if (skin_name.size() > 4 && skin_name.substr(skin_name.size() - 4) == ".blp")
								skin_name = skin_name.substr(0, skin_name.size() - 4);
						}
						// JS: clean_skin_name = skin_name.replace(model_name, '').replace('_', '');
						clean_skin_name = skin_name;
						auto found = clean_skin_name.find(model_name);
						if (found != std::string::npos)
							clean_skin_name.erase(found, model_name.size());
						auto us = clean_skin_name.find('_');
						if (us != std::string::npos)
							clean_skin_name.erase(us, 1);
					} else {
						skin_name = "unknown_" + std::to_string(texture);
					}

					if (clean_skin_name.empty())
						clean_skin_name = "base";

					// JS: if (display.extraGeosets?.length > 0) skin_name += display.extraGeosets.join(',');
					if (!display.extraGeosets.empty()) {
						std::string geoset_str;
						for (size_t gi = 0; gi < display.extraGeosets.size(); gi++) {
							if (gi > 0) geoset_str += ",";
							geoset_str += std::to_string(display.extraGeosets[gi]);
						}
						skin_name += geoset_str;
					}

					// JS: clean_skin_name += ' (' + display.ID + ')';
					clean_skin_name += " (" + std::to_string(display.ID) + ")";

					// JS: if (active_skins.has(skin_name)) continue;
					if (active_skins.contains(skin_name))
						continue;

					// JS: skin_list.push({ id: skin_name, label: clean_skin_name });
					nlohmann::json skin_entry;
					skin_entry["id"] = skin_name;
					skin_entry["label"] = clean_skin_name;
					skin_list.push_back(std::move(skin_entry));

					active_skins[skin_name] = display;
				}

				// JS: core.view.creatureViewerSkins = skin_list;
				view.creatureViewerSkins = std::move(skin_list);

				// JS: const matching_skin = skin_list.find(skin => active_skins.get(skin.id)?.ID === creature.displayID);
				nlohmann::json matching_skin;
				for (const auto& skin : view.creatureViewerSkins) {
					std::string sid = skin.value("id", "");
					auto it = active_skins.find(sid);
					if (it != active_skins.end() && it->second.ID == creature.displayID) {
						matching_skin = skin;
						break;
					}
				}

				// JS: core.view.creatureViewerSkinsSelection = matching_skin ? [matching_skin] : skin_list.slice(0, 1);
				view.creatureViewerSkinsSelection.clear();
				if (!matching_skin.is_null()) {
					view.creatureViewerSkinsSelection.push_back(matching_skin);
				} else if (!view.creatureViewerSkins.empty()) {
					view.creatureViewerSkinsSelection.push_back(view.creatureViewerSkins[0]);
				}

				// JS: core.view.creatureViewerAnims = modelViewerUtils.extract_animations(active_renderer);
				view.creatureViewerAnims = model_viewer_utils::extract_animations(*active_renderer_result.m2);
				view.creatureViewerAnimSelection = "none";
			}

			active_file_data_id = file_data_id;
			active_creature = &creature;

			// JS: const has_content = active_renderer.draw_calls?.length > 0 || active_renderer.groups?.length > 0;
			bool has_content = false;
			if (active_renderer_result.m2)
				has_content = !active_renderer_result.m2->get_draw_calls().empty();
			else if (active_renderer_result.m3)
				has_content = !active_renderer_result.m3->get_draw_calls().empty();
			else if (active_renderer_result.wmo)
				has_content = !active_renderer_result.wmo->get_groups().empty();

			if (!has_content) {
				core::setToast("info", std::format("The model {} doesn't have any 3D data associated with it.", creature.name), {}, 4000);
			} else {
				core::hideToast();

				// JS: if (core.view.creatureViewerAutoAdjust) requestAnimationFrame(() => core.view.creatureViewerContext?.fitCamera?.());
				if (view.creatureViewerAutoAdjust && viewer_context.fitCamera)
					viewer_context.fitCamera();
			}
		}
	} catch (const casc::EncryptionError& e) {
		// JS: core.setToast('error', util.format('The model %s is encrypted with an unknown key (%s).', creature.name, e.key), null, -1);
		core::setToast("error", std::format("The model {} is encrypted with an unknown key ({}).", creature.name, e.key), {}, -1);
		logging::write(std::format("Failed to decrypt model {} ({})", creature.name, e.key));
	} catch (const std::exception& e) {
		// JS: core.setToast('error', 'Unable to preview creature ' + creature.name, ...);
		core::setToast("error", "Unable to preview creature " + creature.name, {}, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}
}

// JS: const export_files = async (core, entries) => { ... }
static void export_files(const std::vector<const db::caches::DBCreatureList::CreatureEntry*>& entries) {
	auto& view = *core::view;

	// JS: const export_paths = core.openLastExportStream();
	auto export_paths = core::openLastExportStream();

	// JS: const format = core.view.config.exportCreatureFormat;
	const std::string format = view.config.value("exportCreatureFormat", std::string("OBJ"));

	// JS: if (format === 'PNG' || format === 'CLIPBOARD') { ... }
	if (format == "PNG" || format == "CLIPBOARD") {
		if (active_file_data_id != 0) {
			// JS: const canvas = document.getElementById('creature-preview').querySelector('canvas');
			// JS: const export_name = ExportHelper.sanitizeFilename(active_creature?.name ?? 'creature_' + active_file_data_id);
			std::string raw_name = active_creature ? active_creature->name : ("creature_" + std::to_string(active_file_data_id));
			const std::string export_name = casc::ExportHelper::sanitizeFilename(raw_name);

			// JS: await modelViewerUtils.export_preview(core, format, canvas, export_name, 'creatures');
			// TODO(conversion): GL context needed for export_preview; will be wired when renderer is integrated.
			// model_viewer_utils::export_preview(format, gl_context, export_name, "creatures");
			core::setToast("info", "Preview export not yet wired.", {}, 2000);
		} else {
			core::setToast("error", "The selected export option only works for model previews. Preview something first!", {}, -1);
		}

		// JS: export_paths?.close();
		return;
	}

	// JS: const casc = core.view.casc;
	casc::CASC* casc = core::view->casc;

	// JS: const helper = new ExportHelper(entries.length, 'creature');
	casc::ExportHelper helper(static_cast<int>(entries.size()), "creature");
	helper.start();

	for (const auto* creature : entries) {
		if (helper.isCancelled())
			break;

		// JS: const creature = typeof entry === 'object' ? entry : DBCreatureList.get_creature_by_id(entry);
		if (!creature)
			continue;

		// JS: const file_manifest = [];
		std::vector<M2ExportFileManifest> file_manifest;
		const std::string creature_name = casc::ExportHelper::sanitizeFilename(creature->name);

		// JS: const display_info = DBCreatures.getDisplayInfo(creature.displayID);
		const auto* display_info = db::caches::DBCreatures::getDisplayInfo(creature->displayID);

		if (display_info && display_info->extendedDisplayInfoID > 0) {
			// character-model creature export
			try {
				// JS: const extra = DBCreatureDisplayExtra.get_extra(display_info.extendedDisplayInfoID);
				const auto* extra = db::caches::DBCreatureDisplayExtra::get_extra(display_info->extendedDisplayInfoID);
				if (!extra) {
					helper.mark(creature_name, false, "No extended display info found");
					continue;
				}

				// JS: const chr_model_id = DBCharacterCustomization.get_chr_model_id(extra.DisplayRaceID, extra.DisplaySexID);
				auto chr_model_id = db::caches::DBCharacterCustomization::get_chr_model_id(extra->DisplayRaceID, extra->DisplaySexID);
				// JS: const file_data_id = chr_model_id !== undefined ? DBCharacterCustomization.get_model_file_data_id(chr_model_id) : undefined;
				uint32_t file_data_id = chr_model_id.has_value() ? db::caches::DBCharacterCustomization::get_model_file_data_id(chr_model_id.value()) : 0;
				if (file_data_id == 0) {
					helper.mark(creature_name, false, "No character model found");
					continue;
				}

				// JS: const data = await casc.getFile(file_data_id);
				BufferWrapper data = casc->getVirtualFileByID(file_data_id);
				// JS: const file_name = listfile.getByID(file_data_id) ?? listfile.formatUnknownFile(file_data_id, '.m2');
				std::string file_name = casc::listfile::getByID(file_data_id);
				if (file_name.empty())
					file_name = casc::listfile::formatUnknownFile(file_data_id, ".m2");

				// JS: const export_path = ExportHelper.getExportPath('creatures/' + creature_name + '.m2');
				std::string export_path = casc::ExportHelper::getExportPath("creatures/" + creature_name + ".m2");

				// JS: const is_active = file_data_id === active_file_data_id && is_character_model;
				const bool is_active = (file_data_id == active_file_data_id && is_character_model);

				if (format == "RAW") {
					// JS: const exporter = new M2Exporter(data, [], file_data_id);
					M2Exporter exporter(std::move(data), {}, file_data_id, casc);
					// JS: await export_paths?.writeLine(export_path);
					// JS: await exporter.exportRaw(export_path, helper, file_manifest);
					exporter.exportRaw(export_path, &helper, &file_manifest);
					helper.mark(creature_name, true);
				} else {
					// JS: const ext = modelViewerUtils.EXPORT_EXTENSIONS[format] ?? '.gltf';
					auto ext_it = model_viewer_utils::EXPORT_EXTENSIONS.find(format);
					std::string ext = (ext_it != model_viewer_utils::EXPORT_EXTENSIONS.end()) ? ext_it->second : ".gltf";

					// JS: const final_path = ExportHelper.replaceExtension(export_path, ext);
					std::string final_path = casc::ExportHelper::replaceExtension(export_path, ext);

					// JS: const exporter = new M2Exporter(data, [], file_data_id);
					M2Exporter exporter(std::move(data), {}, file_data_id, casc);

					// JS: if (is_active) { ... } else { ... }
					if (is_active) {
						// JS: for (const [texture_type, chr_material] of creature_chr_materials) exporter.addURITexture(texture_type, chr_material.getURI());
						// JS: exporter.setGeosetMask(core.view.creatureViewerGeosets);
						// TODO(conversion): Active character texture addURITexture will be wired when CharMaterialRenderer.getURI() is integrated.
					} else {
						// build textures for export
						// JS: const export_materials = new Map();
						std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>> export_materials;

						// JS: const customization_choices = DBCreatureDisplayExtra.get_customization_choices(display_info.extendedDisplayInfoID);
						const auto& customization_choices_raw = db::caches::DBCreatureDisplayExtra::get_customization_choices(display_info->extendedDisplayInfoID);
						std::vector<nlohmann::json> customization_choices;
						for (const auto& choice : customization_choices_raw) {
							nlohmann::json j;
							j["optionID"] = choice.optionID;
							j["choiceID"] = choice.choiceID;
							customization_choices.push_back(std::move(j));
						}

						// JS: const layout_id = DBCharacterCustomization.get_texture_layout_id(chr_model_id);
						uint32_t layout_id = db::caches::DBCharacterCustomization::get_texture_layout_id(chr_model_id.value());

						// JS: let baked_npc_blp = null;
						std::unique_ptr<casc::BLPImage> baked_npc_blp;
						uint32_t bake_id_val = extra->HDBakeMaterialResourcesID;
						if (bake_id_val == 0) bake_id_val = extra->BakeMaterialResourcesID;
						if (bake_id_val > 0) {
							uint32_t bake_fdid = db::caches::DBCharacterCustomization::get_texture_file_data_id(bake_id_val);
							if (bake_fdid != 0) {
								try {
									// JS: auto bake_data = casc->getFile(bake_fdid);
									// JS: baked_npc_blp = new BLPFile(bake_data);
									BufferWrapper bake_data = casc->getVirtualFileByID(bake_fdid);
									baked_npc_blp = std::make_unique<casc::BLPImage>(bake_data);
								} catch (const std::exception& e) {
									logging::write(std::format("Failed to load baked NPC texture {}: {}", bake_fdid, e.what()));
								}
							}
						}

						// JS: await character_appearance.apply_customization_textures(null, customization_choices, layout_id, export_materials, baked_npc_blp);
						character_appearance::apply_customization_textures(nullptr, customization_choices, layout_id, export_materials, baked_npc_blp.get());

						// apply equipment textures for export
						// JS: const export_equipment = build_creature_equipment(display_info.extendedDisplayInfoID, creature);
						auto export_equipment = build_creature_equipment(display_info->extendedDisplayInfoID, *creature);
						if (export_equipment.has_value()) {
							const auto* sections = db::caches::DBCharacterCustomization::get_texture_sections(layout_id);
							if (sections) {
								std::unordered_map<int, const db::DataRecord*> section_by_type;
								for (const auto& section : *sections) {
									int st = static_cast<int>(std::get<int64_t>(section.at("SectionType")));
									section_by_type[st] = &section;
								}

								const auto& texture_layer_map = db::caches::DBCharacterCustomization::get_model_texture_layer_map();
								const db::DataRecord* base_layer_exp = nullptr;
								std::unordered_map<int, const db::DataRecord*> layers_by_section_exp;
								std::string layout_prefix_exp = std::to_string(layout_id) + "-";

								for (const auto& [key, layer] : texture_layer_map) {
									if (key.substr(0, layout_prefix_exp.size()) != layout_prefix_exp)
										continue;

									int bitmask = static_cast<int>(std::get<int64_t>(layer.at("TextureSectionTypeBitMask")));
									int tex_type = static_cast<int>(std::get<int64_t>(layer.at("TextureType")));

									if (bitmask == -1 && tex_type == 1)
										base_layer_exp = &layer;
									else if (bitmask != -1) {
										for (int st = 0; st < 9; st++) {
											if ((1 << st) & bitmask) {
												if (!layers_by_section_exp.contains(st))
													layers_by_section_exp[st] = &layer;
											}
										}
									}
								}

								if (base_layer_exp) {
									for (int st = 0; st < 9; st++) {
										if (!layers_by_section_exp.contains(st))
											layers_by_section_exp[st] = base_layer_exp;
									}
								}

								for (const auto& [slot_id, eq_entry] : *export_equipment) {
									// JS: const item_textures = entry.item_id ? ...
									int race_id = creature_extra_info ? static_cast<int>(creature_extra_info->DisplayRaceID) : -1;
									int gender_index = creature_extra_info ? static_cast<int>(creature_extra_info->DisplaySexID) : -1;

									std::optional<std::vector<db::caches::DBItemCharTextures::TextureComponent>> item_textures;
									if (eq_entry.item_id.has_value())
										item_textures = db::caches::DBItemCharTextures::getItemTextures(eq_entry.item_id.value(), race_id, gender_index);
									else
										item_textures = db::caches::DBItemCharTextures::getTexturesByDisplayId(eq_entry.display_id, race_id, gender_index);

									if (!item_textures.has_value())
										continue;

									for (const auto& texture : *item_textures) {
										auto section_it = section_by_type.find(texture.section);
										if (section_it == section_by_type.end())
											continue;

										auto layer_it = layers_by_section_exp.find(texture.section);
										if (layer_it == layers_by_section_exp.end())
											continue;

										const db::DataRecord& section_rec = *section_it->second;
										const db::DataRecord& layer_rec = *layer_it->second;

										int layer_tex_type = static_cast<int>(std::get<int64_t>(layer_rec.at("TextureType")));

										const auto* chr_model_material = db::caches::DBCharacterCustomization::get_model_material(
											layout_id, static_cast<uint32_t>(layer_tex_type)
										);
										if (!chr_model_material)
											continue;

										int mat_texture_type = static_cast<int>(std::get<int64_t>(chr_model_material->at("TextureType")));
										int mat_width = static_cast<int>(std::get<int64_t>(chr_model_material->at("Width")));
										int mat_height = static_cast<int>(std::get<int64_t>(chr_model_material->at("Height")));

										uint32_t mat_type_key = static_cast<uint32_t>(mat_texture_type);
										if (!export_materials.contains(mat_type_key)) {
											auto chr_mat = std::make_unique<CharMaterialRenderer>(mat_texture_type, mat_width, mat_height);
											chr_mat->init();
											export_materials[mat_type_key] = std::move(chr_mat);
										}

										auto& chr_material = export_materials[mat_type_key];

										int slot_layer = wow::get_slot_layer(slot_id);
										int chr_model_texture_target_id = (slot_layer * 100) + texture.section;

										int sec_x = static_cast<int>(std::get<int64_t>(section_rec.at("X")));
										int sec_y = static_cast<int>(std::get<int64_t>(section_rec.at("Y")));
										int sec_w = static_cast<int>(std::get<int64_t>(section_rec.at("Width")));
										int sec_h = static_cast<int>(std::get<int64_t>(section_rec.at("Height")));
										int blend_mode = static_cast<int>(std::get<int64_t>(layer_rec.at("BlendMode")));

										chr_material->setTextureTarget(
											chr_model_texture_target_id,
											texture.fileDataID,
											sec_x, sec_y, sec_w, sec_h,
											mat_texture_type, mat_width, mat_height,
											blend_mode,
											true
										);
									}
								}
							}
						}

						// JS: for (const [texture_type, chr_material] of export_materials) { ... }
						for (auto& [texture_type, chr_material] : export_materials) {
							chr_material->update();
							// JS: exporter.addURITexture(texture_type, chr_material.getURI());
							// TODO(conversion): addURITexture requires CharMaterialRenderer.getURI() to return PNG BufferWrapper; will be wired when renderer is integrated.
						}

						// JS: character_appearance.dispose_materials(export_materials);
						character_appearance::dispose_materials(export_materials);
					}

					// JS: const mark_file_name = ExportHelper.getRelativeExport(final_path);
					std::string mark_file_name = casc::ExportHelper::getRelativeExport(final_path);

					// JS: if (format === 'OBJ') await exporter.exportAsOBJ(...)
					if (format == "OBJ" || format == "STL") {
						exporter.exportAsOBJ(final_path, (format == "STL"), &helper, &file_manifest);
					} else {
						exporter.exportAsGLTF(final_path, &helper, format);
					}

					// JS: await export_paths?.writeLine('M2_' + format + ':' + final_path);
					helper.mark(mark_file_name, true);
				}
			} catch (const std::exception& e) {
				// JS: helper.mark(creature_name, false, e.message, e.stack);
				helper.mark(creature_name, false, e.what());
			}

			continue;
		}

		// standard creature export
		// JS: const file_data_id = DBCreatures.getFileDataIDByDisplayID(creature.displayID);
		uint32_t file_data_id = db::caches::DBCreatures::getFileDataIDByDisplayID(creature->displayID);
		if (file_data_id == 0) {
			helper.mark(creature_name, false, "No model data found");
			continue;
		}

		try {
			// JS: const data = await casc.getFile(file_data_id);
			BufferWrapper data = casc->getVirtualFileByID(file_data_id);
			// JS: const model_type = modelViewerUtils.detect_model_type(data);
			auto model_type = model_viewer_utils::detect_model_type(data);
			// JS: const file_ext = modelViewerUtils.get_model_extension(model_type);
			std::string file_ext = model_viewer_utils::get_model_extension(model_type);
			// JS: const file_name = listfile.getByID(file_data_id) ?? listfile.formatUnknownFile(file_data_id, file_ext);
			std::string file_name = casc::listfile::getByID(file_data_id);
			if (file_name.empty())
				file_name = casc::listfile::formatUnknownFile(file_data_id, file_ext);
			// JS: const export_path = ExportHelper.getExportPath('creatures/' + creature_name + file_ext);
			std::string export_path = casc::ExportHelper::getExportPath("creatures/" + creature_name + file_ext);
			// JS: const is_active = file_data_id === active_file_data_id;
			const bool is_active = (file_data_id == active_file_data_id);

			// JS: const mark_name = await modelViewerUtils.export_model({ ... });
			model_viewer_utils::ExportModelOptions opts;
			opts.data = &data;
			opts.file_data_id = file_data_id;
			opts.file_name = file_name;
			opts.format = format;
			opts.export_path = export_path;
			opts.helper = &helper;
			opts.casc = casc;
			nlohmann::json variant_textures_json = nlohmann::json::array();
			if (is_active) {
				for (uint32_t tid : selected_variant_texture_ids)
					variant_textures_json.push_back(tid);
			}
			opts.variant_textures = variant_textures_json;
			opts.geoset_mask = is_active ? &view.creatureViewerGeosets : nullptr;
			opts.wmo_group_mask = is_active ? &view.creatureViewerWMOGroups : nullptr;
			opts.wmo_set_mask = is_active ? &view.creatureViewerWMOSets : nullptr;
			std::string mark_name = model_viewer_utils::export_model(opts);

			helper.mark(creature_name, true);
		} catch (const std::exception& e) {
			// JS: helper.mark(creature_name, false, e.message, e.stack);
			helper.mark(creature_name, false, e.what());
		}
	}

	helper.finish();
	// JS: export_paths?.close();
	export_paths.close();
}

// --- Vue methods converted to static functions ---

// JS: methods.handle_listbox_context(data)
static void handle_listbox_context(const std::vector<std::string>& selection) {
	listbox_context::handle_context_menu(selection);
}

// JS: methods.copy_creature_names(selection)
static void copy_creature_names(const std::vector<nlohmann::json>& selection) {
	std::string result;
	static const std::regex name_rx(R"(^(.+)\s+\[(\d+)\]$)");

	for (const auto& entry : selection) {
		std::string name;
		if (entry.is_string()) {
			const std::string str = entry.get<std::string>();
			std::smatch match;
			if (std::regex_match(str, match, name_rx))
				name = match[1].str();
			else
				name = str;
		}

		if (!name.empty()) {
			if (!result.empty())
				result += '\n';
			result += name;
		}
	}

	// JS: nw.Clipboard.get().set(names.join('\n'), 'text');
	ImGui::SetClipboardText(result.c_str());
}

// JS: methods.copy_creature_ids(selection)
static void copy_creature_ids(const std::vector<nlohmann::json>& selection) {
	std::string result;
	static const std::regex id_rx(R"(\[(\d+)\]$)");

	for (const auto& entry : selection) {
		std::string id_str;
		if (entry.is_string()) {
			const std::string str = entry.get<std::string>();
			std::smatch match;
			if (std::regex_search(str, match, id_rx))
				id_str = match[1].str();
		}

		if (!id_str.empty()) {
			if (!result.empty())
				result += '\n';
			result += id_str;
		}
	}

	ImGui::SetClipboardText(result.c_str());
}

// JS: methods.preview_texture(file_data_id, display_name)
static void preview_texture(uint32_t file_data_id, const std::string& display_name) {
	// JS: const state = modelViewerUtils.create_view_state(this.$core, 'creature');
	// JS: await modelViewerUtils.preview_texture_by_id(this.$core, state, active_renderer, file_data_id, display_name);
	model_viewer_utils::preview_texture_by_id(view_state, get_active_m2_renderer(), file_data_id, display_name, core::view->casc);
}

// JS: methods.export_ribbon_texture(file_data_id, display_name)
static void export_ribbon_texture(uint32_t file_data_id, [[maybe_unused]] const std::string& display_name) {
	// JS: await textureExporter.exportSingleTexture(file_data_id);
	texture_exporter::exportSingleTexture(file_data_id, core::view->casc);
}

// JS: methods.toggle_uv_layer(layer_name)
static void toggle_uv_layer(const std::string& layer_name) {
	// JS: const state = modelViewerUtils.create_view_state(this.$core, 'creature');
	// JS: modelViewerUtils.toggle_uv_layer(state, active_renderer, layer_name);
	model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
}

// JS: methods.export_creatures()
static void export_creatures() {
	auto& view = *core::view;

	// JS: const user_selection = this.$core.view.selectionCreatures;
	const auto& user_selection = view.selectionCreatures;

	// JS: if (user_selection.length === 0) { ... }
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any creatures to export; you should do that first.");
		return;
	}

	// JS: const creature_items = user_selection.map(entry => { ... }).filter(item => item);
	static const std::regex id_extract(R"(\[(\d+)\]$)");
	std::vector<const db::caches::DBCreatureList::CreatureEntry*> creature_items;

	for (const auto& entry : user_selection) {
		if (entry.is_string()) {
			const std::string str = entry.get<std::string>();
			std::smatch match;
			if (std::regex_search(str, match, id_extract)) {
				uint32_t creature_id = static_cast<uint32_t>(std::stoul(match[1].str()));
				const auto* item = db::caches::DBCreatureList::get_creature_by_id(creature_id);
				if (item)
					creature_items.push_back(item);
			}
		}
	}

	// JS: await export_files(this.$core, creature_items);
	export_files(creature_items);
}

// Animation methods: toggle_animation_pause, step_animation, seek_animation, start_scrub, end_scrub
// are delegated to anim_methods (created via model_viewer_utils::create_animation_methods).

// --- initialize ---
// JS: methods.initialize()
static void initialize() {
	auto& view = *core::view;

	// JS: this.$core.showLoadingScreen(8);
	core::showLoadingScreen(8);

	// JS: await this.$core.progressLoadingScreen('Loading model file data...');
	// JS: await DBModelFileData.initializeModelFileData();
	core::progressLoadingScreen("Loading model file data...");
	db::caches::DBModelFileData::initializeModelFileData();

	// JS: await this.$core.progressLoadingScreen('Loading creature data...');
	// JS: await DBCreatures.initializeCreatureData();
	core::progressLoadingScreen("Loading creature data...");
	db::caches::DBCreatures::initializeCreatureData();

	// JS: await this.$core.progressLoadingScreen('Loading character customization data...');
	// JS: await DBCharacterCustomization.ensureInitialized();
	core::progressLoadingScreen("Loading character customization data...");
	db::caches::DBCharacterCustomization::ensureInitialized();

	// JS: await this.$core.progressLoadingScreen('Loading creature display extras...');
	// JS: await DBCreatureDisplayExtra.ensureInitialized();
	core::progressLoadingScreen("Loading creature display extras...");
	db::caches::DBCreatureDisplayExtra::ensureInitialized();

	// JS: await this.$core.progressLoadingScreen('Loading NPC equipment data...');
	// JS: await DBNpcEquipment.ensureInitialized();
	core::progressLoadingScreen("Loading NPC equipment data...");
	db::caches::DBNpcEquipment::ensureInitialized();

	// JS: await this.$core.progressLoadingScreen('Loading item display data...');
	// JS: await DBItemModels.ensureInitialized();
	// JS: await DBItemGeosets.ensureInitialized();
	// JS: await DBItemCharTextures.ensureInitialized();
	core::progressLoadingScreen("Loading item display data...");
	db::caches::DBItemModels::ensureInitialized();
	db::caches::DBItemGeosets::ensureInitialized();
	db::caches::DBItemCharTextures::ensureInitialized();

	// JS: await this.$core.progressLoadingScreen('Loading item cache...');
	// JS: await DBItems.ensureInitialized();
	core::progressLoadingScreen("Loading item cache...");
	db::caches::DBItems::ensureInitialized();

	// JS: await this.$core.progressLoadingScreen('Loading creature list...');
	// JS: await DBCreatureList.initialize_creature_list();
	core::progressLoadingScreen("Loading creature list...");
	db::caches::DBCreatureList::initialize_creature_list();

	// JS: const creatures = DBCreatureList.get_all_creatures();
	const auto& creatures = db::caches::DBCreatureList::get_all_creatures();
	// JS: const entries = [];
	std::vector<nlohmann::json> entries;

	// JS: for (const [id, creature] of creatures) entries.push(`${creature.name} [${id}]`);
	for (const auto& [id, creature] : creatures)
		entries.push_back(std::format("{} [{}]", creature.name, id));

	// JS: entries.sort((a, b) => { ... });
	std::sort(entries.begin(), entries.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
		static const std::regex id_suffix(R"(\s+\[\d+\]$)");
		std::string name_a = std::regex_replace(a.get<std::string>(), id_suffix, "");
		std::string name_b = std::regex_replace(b.get<std::string>(), id_suffix, "");
		// toLowerCase equivalent
		std::transform(name_a.begin(), name_a.end(), name_a.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		std::transform(name_b.begin(), name_b.end(), name_b.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return name_a < name_b;
	});

	// JS: this.$core.view.listfileCreatures = entries;
	view.listfileCreatures = std::move(entries);

	// JS: if (!this.$core.view.creatureViewerContext) { ... }
	if (view.creatureViewerContext.is_null()) {
		// JS: this.$core.view.creatureViewerContext = Object.seal({ ... });
		view.creatureViewerContext = nlohmann::json::object();

		// Wire model viewer context callbacks.
		viewer_context.getActiveRenderer = []() -> M2RendererGL* {
			return get_active_m2_renderer();
		};
		viewer_context.renderActiveModel = [](const float* view_mat, const float* proj_mat) {
			if (active_renderer_result.m3)
				active_renderer_result.m3->render(view_mat, proj_mat);
			else if (active_renderer_result.wmo)
				active_renderer_result.wmo->render(view_mat, proj_mat);
		};
		viewer_context.setActiveModelTransform = [](const std::array<float, 3>& pos,
													const std::array<float, 3>& rot,
													const std::array<float, 3>& scale) {
			if (active_renderer_result.wmo)
				active_renderer_result.wmo->setTransform(pos, rot, scale);
		};
		viewer_context.getActiveBoundingBox = []() -> std::optional<model_viewer_gl::BoundingBox> {
			if (active_renderer_result.m2) {
				auto bb = active_renderer_result.m2->getBoundingBox();
				if (bb) return model_viewer_gl::BoundingBox{ bb->min, bb->max };
			} else if (active_renderer_result.m3) {
				auto bb = active_renderer_result.m3->getBoundingBox();
				if (bb) return model_viewer_gl::BoundingBox{ bb->min, bb->max };
			} else if (active_renderer_result.wmo) {
				auto bb = active_renderer_result.wmo->getBoundingBox();
				if (bb) return model_viewer_gl::BoundingBox{ bb->min, bb->max };
			}
			return std::nullopt;
		};
		viewer_context.useCharacterControls = true;
		viewer_context.getEquipmentRenderers = []() -> std::unordered_map<int, model_viewer_gl::EquipmentSlotRenderers>* {
			rebuild_renderer_adapter_maps();
			return equip_adapter_map.empty() ? nullptr : &equip_adapter_map;
		};
		viewer_context.getCollectionRenderers = []() -> std::unordered_map<int, model_viewer_gl::CollectionSlotRenderers>* {
			rebuild_renderer_adapter_maps();
			return coll_adapter_map.empty() ? nullptr : &coll_adapter_map;
		};
	}

	// JS: this.$core.hideLoadingScreen();
	core::hideLoadingScreen();
}

// --- Public API ---

// JS: register() { this.registerNavButton('Creatures', 'nessy.svg', InstallType.CASC); }
void registerTab() {
	modules::register_nav_button("tab_creatures", "Creatures", "nessy.svg", install_type::CASC);
}

// JS: async mounted() { await this.initialize(); ... }
void mounted() {
	// JS: const state = modelViewerUtils.create_view_state(this.$core, 'creature');
	view_state = model_viewer_utils::create_view_state("creature");

	// JS: await this.initialize();
	initialize();

	// Create animation methods helper.
	// JS: toggle_animation_pause, step_animation, seek_animation, start_scrub, end_scrub
	anim_methods = std::make_unique<model_viewer_utils::AnimationMethods>(
		get_active_m2_renderer,
		get_view_state_ptr
	);

	auto& view = *core::view;

	// JS: this.$core.view.$watch('creatureViewerSkinsSelection', ...)
	prev_skins_selection = view.creatureViewerSkinsSelection;

	// JS: this.$core.view.$watch('creatureViewerAnimSelection', ...)
	if (view.creatureViewerAnimSelection.is_string())
		prev_anim_selection = view.creatureViewerAnimSelection.get<std::string>();
	else
		prev_anim_selection.clear();

	// JS: this.$core.view.$watch('selectionCreatures', ...)
	prev_selection_creatures = view.selectionCreatures;

	// JS: this.$core.view.$watch('creatureViewerEquipment', ...)
	prev_equipment_checked.clear();
	for (const auto& item : view.creatureViewerEquipment)
		prev_equipment_checked.push_back(item.value("checked", true));

	// JS: this.$core.events.on('toggle-uv-layer', (layer_name) => { ... });
	core::events.on("toggle-uv-layer", EventEmitter::ArgCallback([](const std::any& arg) {
		const auto& layer_name = std::any_cast<const std::string&>(arg);
		model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
	}));

	is_initialized = true;
}

// JS: getActiveRenderer: () => active_renderer
M2RendererGL* getActiveRenderer() {
	return active_renderer_result.m2.get();
}

void render() {
	auto& view = *core::view;

	if (!is_initialized)
		return;

	// ─── Change-detection for watches ───────────────────────────────────────

	// Watch: creatureViewerSkinsSelection → apply skin textures/geosets
	// JS: this.$core.view.$watch('creatureViewerSkinsSelection', async selection => { ... });
	{
		if (view.creatureViewerSkinsSelection != prev_skins_selection) {
			prev_skins_selection = view.creatureViewerSkinsSelection;

			if (active_renderer_result.m2 && !active_skins.empty()) {
				// JS: const selected = selection[0];
				if (!view.creatureViewerSkinsSelection.empty()) {
					const auto& selected = view.creatureViewerSkinsSelection[0];
					if (!selected.is_null()) {
						std::string skin_id = selected.value("id", "");
						auto skin_it = active_skins.find(skin_id);
						if (skin_it != active_skins.end()) {
							const auto& display = skin_it->second;

							auto& curr_geosets = view.creatureViewerGeosets;

							// JS: if (display.extraGeosets !== undefined) { ... } else { ... }
							if (!display.extraGeosets.empty()) {
								for (auto& geoset : curr_geosets) {
									int gid = geoset.value("id", 0);
									if (gid > 0 && gid < 900)
										geoset["checked"] = false;
								}

								for (uint32_t extra_geoset : display.extraGeosets) {
									for (auto& geoset : curr_geosets) {
										if (static_cast<uint32_t>(geoset.value("id", 0)) == extra_geoset)
											geoset["checked"] = true;
									}
								}
							} else {
								for (auto& geoset : curr_geosets) {
									std::string id_str = std::to_string(geoset.value("id", 0));
									// JS: geoset.checked = (id.endsWith('0') || id.endsWith('01'));
									bool ends_with_0 = (!id_str.empty() && id_str.back() == '0');
									bool ends_with_01 = (id_str.size() >= 2 && id_str.substr(id_str.size() - 2) == "01");
									geoset["checked"] = (ends_with_0 || ends_with_01);
								}
							}

							// JS: if (display.textures.length > 0) selected_variant_texture_ids = [...display.textures];
							if (!display.textures.empty())
								selected_variant_texture_ids = display.textures;

							// JS: active_renderer.applyReplaceableTextures(display);
							if (active_renderer_result.m2) {
								M2DisplayInfo disp_info;
								disp_info.textures = display.textures;
								active_renderer_result.m2->applyReplaceableTextures(disp_info);
							}
						}
					}
				}
			}
		}
	}

	// Watch: creatureViewerAnimSelection → handle_animation_change
	// JS: this.$core.view.$watch('creatureViewerAnimSelection', async selected_animation_id => { ... });
	{
		std::string current_anim;
		if (view.creatureViewerAnimSelection.is_string())
			current_anim = view.creatureViewerAnimSelection.get<std::string>();

		if (current_anim != prev_anim_selection) {
			prev_anim_selection = current_anim;

			// JS: if (this.$core.view.creatureViewerAnims.length === 0) return;
			if (!view.creatureViewerAnims.empty()) {
				model_viewer_utils::handle_animation_change(
					get_active_m2_renderer(),
					view_state,
					current_anim
				);
			}
		}
	}

	// Watch: selectionCreatures → auto-preview if creatureAutoPreview
	// JS: this.$core.view.$watch('selectionCreatures', async selection => { ... });
	{
		if (view.selectionCreatures != prev_selection_creatures) {
			prev_selection_creatures = view.selectionCreatures;

			// JS: if (!this.$core.view.config.creatureAutoPreview) return;
			if (view.config.value("creatureAutoPreview", false)) {
				// JS: const first = selection[0];
				if (!view.selectionCreatures.empty() && view.isBusy == 0) {
					const auto& first = view.selectionCreatures[0];
					if (first.is_string()) {
						const std::string first_str = first.get<std::string>();
						static const std::regex id_rx(R"(\[(\d+)\]$)");
						std::smatch match;
						if (std::regex_search(first_str, match, id_rx)) {
							uint32_t creature_id = static_cast<uint32_t>(std::stoul(match[1].str()));
							const auto* creature_entry = db::caches::DBCreatureList::get_creature_by_id(creature_id);
							if (creature_entry)
								preview_creature(*creature_entry);
						}
					}
				}
			}
		}
	}

	// Watch: creatureViewerEquipment (deep) → refresh_creature_equipment
	// JS: this.$core.view.$watch('creatureViewerEquipment', async () => { ... }, { deep: true });
	{
		bool equipment_changed = false;
		if (prev_equipment_checked.size() == view.creatureViewerEquipment.size()) {
			for (size_t i = 0; i < view.creatureViewerEquipment.size(); ++i) {
				bool current = view.creatureViewerEquipment[i].value("checked", true);
				if (current != prev_equipment_checked[i]) {
					equipment_changed = true;
					break;
				}
			}
		} else {
			equipment_changed = true;
		}

		if (equipment_changed) {
			prev_equipment_checked.clear();
			for (const auto& item : view.creatureViewerEquipment)
				prev_equipment_checked.push_back(item.value("checked", true));

			// Sync checklist state back from view
			for (size_t i = 0; i < view.creatureViewerEquipment.size() && i < creature_equipment_checklist.size(); ++i)
				creature_equipment_checklist[i].checked = view.creatureViewerEquipment[i].value("checked", true);

			// JS: if (equipment_refresh_lock || !active_renderer || !is_character_model || !creature_equipment) return;
			if (!equipment_refresh_lock && active_renderer_result.m2 && is_character_model && creature_equipment.has_value())
				refresh_creature_equipment();
		}
	}

	// ─── Template rendering ─────────────────────────────────────────────────

	// JS: <div class="tab list-tab" id="tab-creatures">

	// --- Left panel: List container ---
	// JS: <div class="list-container">
	//     <Listbox v-model:selection="selectionCreatures" ... @contextmenu="handle_listbox_context" />
	ImGui::BeginChild("creature-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox component rendering will be wired when integration is complete.
	ImGui::Text("Creatures: %zu", view.listfileCreatures.size());

	// JS: <input type="text" v-model="$core.view.userInputFilterCreatures" placeholder="Filter creatures..."/>
	// TODO(conversion): Filter input will use ImGui::InputText when Listbox component is wired.

	ImGui::EndChild();

	ImGui::SameLine();

	// --- Middle panel: Preview container ---
	// JS: <div class="preview-container">
	ImGui::BeginChild("creature-preview-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.55f, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// JS: <component :is="$components.ResizeLayer" ... id="texture-ribbon" ... >
	// TODO(conversion): Texture ribbon rendering will be wired when integration is complete.

	// JS: <div id="creature-texture-preview" v-if="$core.view.creatureTexturePreviewURL.length > 0">
	if (!view.creatureTexturePreviewURL.empty()) {
		// JS: <div id="creature-texture-preview-toast" @click="...">Close Preview</div>
		if (ImGui::Button("Close Preview"))
			view.creatureTexturePreviewURL.clear();

		// JS: <div class="image" :style="..."> ... </div>
		// TODO(conversion): Texture preview rendering will use ImGui::Image when textures are wired.
		ImGui::Text("Texture preview: %dx%d", view.creatureTexturePreviewWidth, view.creatureTexturePreviewHeight);

		// JS: <div id="uv-layer-buttons" v-if="creatureViewerUVLayers.length > 0">
		if (!view.creatureViewerUVLayers.empty()) {
			for (auto& layer : view.creatureViewerUVLayers) {
				std::string layer_name = layer.value("name", "");
				bool is_active = layer.value("active", false);
				if (is_active)
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

				if (ImGui::Button(layer_name.c_str()))
					toggle_uv_layer(layer_name);

				if (is_active)
					ImGui::PopStyleColor();

				ImGui::SameLine();
			}
			ImGui::NewLine();
		}
	}

	// JS: <div class="preview-background" id="creature-preview">
	// JS: <input v-if="modelViewerShowBackground" type="color" ... />
	// TODO(conversion): Background color picker will be wired when config UI is integrated.

	// JS: <component :is="$components.ModelViewerGL" v-if="creatureViewerContext" :context="...">
	if (!view.creatureViewerContext.is_null()) {
		model_viewer_gl::renderWidget("##creature_viewer", viewer_state, viewer_context);
	}

	// JS: <div v-if="creatureViewerAnims && creatureViewerAnims.length > 0" class="preview-dropdown-overlay">
	if (!view.creatureViewerAnims.empty() && view.creatureTexturePreviewURL.empty()) {
		// JS: <select v-model="creatureViewerAnimSelection">
		std::string current_anim_label;
		std::string current_anim_id;
		if (view.creatureViewerAnimSelection.is_string())
			current_anim_id = view.creatureViewerAnimSelection.get<std::string>();

		for (const auto& anim : view.creatureViewerAnims) {
			if (anim.value("id", "") == current_anim_id) {
				current_anim_label = anim.value("label", "");
				break;
			}
		}

		if (ImGui::BeginCombo("##creature-animation", current_anim_label.c_str())) {
			for (const auto& anim : view.creatureViewerAnims) {
				std::string anim_id = anim.value("id", "");
				std::string anim_label = anim.value("label", "");
				bool is_selected = (anim_id == current_anim_id);
				if (ImGui::Selectable(anim_label.c_str(), is_selected))
					view.creatureViewerAnimSelection = anim_id;
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// JS: <div v-if="creatureViewerAnimSelection !== 'none'" class="anim-controls">
		if (current_anim_id != "none" && !current_anim_id.empty()) {
			// JS: <button ... @click="step_animation(-1)">Previous frame</button>
			bool is_paused = view.creatureViewerAnimPaused;

			ImGui::BeginDisabled(!is_paused);
			if (ImGui::Button("<##creature-step-left"))
				anim_methods->step_animation(-1);
			ImGui::EndDisabled();

			ImGui::SameLine();

			// JS: <button ... @click="toggle_animation_pause()">Play/Pause</button>
			if (ImGui::Button(is_paused ? "Play##creature" : "Pause##creature"))
				anim_methods->toggle_animation_pause();

			ImGui::SameLine();

			// JS: <button ... @click="step_animation(1)">Next frame</button>
			ImGui::BeginDisabled(!is_paused);
			if (ImGui::Button(">##creature-step-right"))
				anim_methods->step_animation(1);
			ImGui::EndDisabled();

			ImGui::SameLine();

			// JS: <div class="anim-scrubber">
			//     <input type="range" min="0" :max="animFrameCount - 1" :value="animFrame" @input="seek_animation" />
			int frame = view.creatureViewerAnimFrame;
			int max_frame = view.creatureViewerAnimFrameCount > 0 ? view.creatureViewerAnimFrameCount - 1 : 0;

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80.0f);
			if (ImGui::IsItemActivated())
				anim_methods->start_scrub();
			if (ImGui::SliderInt("##creature-scrub", &frame, 0, max_frame)) {
				anim_methods->seek_animation(frame);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
				anim_methods->end_scrub();

			ImGui::SameLine();
			// JS: <div class="anim-frame-display">{{ animFrame }}</div>
			ImGui::Text("%d", view.creatureViewerAnimFrame);
		}
	}

	ImGui::EndChild();

	ImGui::SameLine();

	// --- Right panel: Sidebar ---
	// JS: <div id="creature-sidebar" class="sidebar">
	ImGui::BeginChild("creature-sidebar", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// JS: <span class="header">Preview</span>
	ImGui::SeparatorText("Preview");

	// JS: <label class="ui-checkbox" title="Automatically preview a creature when selecting it">
	{
		bool auto_preview = view.config.value("creatureAutoPreview", false);
		if (ImGui::Checkbox("Auto Preview##creature", &auto_preview))
			view.config["creatureAutoPreview"] = auto_preview;
	}

	// JS: <label class="ui-checkbox" title="Automatically adjust camera when selecting a new creature">
	ImGui::Checkbox("Auto Camera##creature", &view.creatureViewerAutoAdjust);

	// JS: <label class="ui-checkbox" title="Show a grid in the 3D viewport">
	{
		bool show_grid = view.config.value("modelViewerShowGrid", false);
		if (ImGui::Checkbox("Show Grid##creature", &show_grid))
			view.config["modelViewerShowGrid"] = show_grid;
	}

	// JS: <label class="ui-checkbox" title="Render the preview model as a wireframe">
	{
		bool wireframe = view.config.value("modelViewerWireframe", false);
		if (ImGui::Checkbox("Show Wireframe##creature", &wireframe))
			view.config["modelViewerWireframe"] = wireframe;
	}

	// JS: <label class="ui-checkbox" title="Show the model's bone structure">
	{
		bool show_bones = view.config.value("modelViewerShowBones", false);
		if (ImGui::Checkbox("Show Bones##creature", &show_bones))
			view.config["modelViewerShowBones"] = show_bones;
	}

	// JS: <label class="ui-checkbox" title="Show model textures in the preview pane">
	{
		bool show_textures = view.config.value("modelViewerShowTextures", true);
		if (ImGui::Checkbox("Show Textures##creature", &show_textures))
			view.config["modelViewerShowTextures"] = show_textures;
	}

	// JS: <label class="ui-checkbox" title="Show a background color in the 3D viewport">
	{
		bool show_bg = view.config.value("modelViewerShowBackground", false);
		if (ImGui::Checkbox("Show Background##creature", &show_bg))
			view.config["modelViewerShowBackground"] = show_bg;
	}

	// JS: <span class="header">Export</span>
	ImGui::SeparatorText("Export");

	// JS: <label class="ui-checkbox" title="Include textures when exporting models">
	{
		bool export_textures = view.config.value("modelsExportTextures", true);
		if (ImGui::Checkbox("Textures##creature-export", &export_textures))
			view.config["modelsExportTextures"] = export_textures;

		// JS: <label v-if="modelsExportTextures" ... title="Include alpha channel ...">
		if (export_textures) {
			bool export_alpha = view.config.value("modelsExportAlpha", false);
			if (ImGui::Checkbox("Texture Alpha##creature", &export_alpha))
				view.config["modelsExportAlpha"] = export_alpha;
		}
	}

	// JS: <label v-if="exportCreatureFormat === 'GLTF' && creatureViewerActiveType === 'm2'" ...>
	{
		std::string fmt = view.config.value("exportCreatureFormat", std::string("OBJ"));
		if (fmt == "GLTF" && view.creatureViewerActiveType == "m2") {
			bool export_anims = view.config.value("modelsExportAnimations", false);
			if (ImGui::Checkbox("Export animations##creature", &export_anims))
				view.config["modelsExportAnimations"] = export_anims;
		}
	}

	// JS: <template v-if="creatureViewerActiveType === 'm2'">
	if (view.creatureViewerActiveType == "m2") {
		// JS: <template v-if="creatureViewerEquipment.length > 0">
		if (!view.creatureViewerEquipment.empty()) {
			ImGui::SeparatorText("Equipment");
			// JS: <component :is="$components.Checkboxlist" :items="creatureViewerEquipment">
			for (auto& item : view.creatureViewerEquipment) {
				bool checked = item.value("checked", true);
				std::string label = item.value("label", "");
				if (ImGui::Checkbox(label.c_str(), &checked))
					item["checked"] = checked;
			}
			// JS: <div class="list-toggles">
			//     <a @click="setAllCreatureEquipment(true)">Enable All</a> / <a @click="setAllCreatureEquipment(false)">Disable All</a>
			if (ImGui::SmallButton("Enable All##creature-equip")) {
				for (auto& item : view.creatureViewerEquipment)
					item["checked"] = true;
			}
			ImGui::SameLine();
			ImGui::Text("/");
			ImGui::SameLine();
			if (ImGui::SmallButton("Disable All##creature-equip")) {
				for (auto& item : view.creatureViewerEquipment)
					item["checked"] = false;
			}
		}

		// JS: <template v-if="creatureViewerGeosets.length > 0">
		if (!view.creatureViewerGeosets.empty()) {
			ImGui::SeparatorText("Geosets");
			// JS: <component :is="$components.Checkboxlist" :items="creatureViewerGeosets">
			for (auto& item : view.creatureViewerGeosets) {
				bool checked = item.value("checked", false);
				std::string label = item.value("label", std::to_string(item.value("id", 0)));
				std::string checkbox_id = label + "##creature-geoset-" + std::to_string(item.value("id", 0));
				if (ImGui::Checkbox(checkbox_id.c_str(), &checked))
					item["checked"] = checked;
			}
			// JS: <a @click="setAllCreatureGeosets(true)">Enable All</a> / <a @click="setAllCreatureGeosets(false)">Disable All</a>
			if (ImGui::SmallButton("Enable All##creature-geosets")) {
				for (auto& item : view.creatureViewerGeosets)
					item["checked"] = true;
			}
			ImGui::SameLine();
			ImGui::Text("/");
			ImGui::SameLine();
			if (ImGui::SmallButton("Disable All##creature-geosets")) {
				for (auto& item : view.creatureViewerGeosets)
					item["checked"] = false;
			}
		}

		// JS: <template v-if="config.modelsExportTextures && creatureViewerSkins.length > 0">
		bool show_textures = view.config.value("modelsExportTextures", true);
		if (show_textures && !view.creatureViewerSkins.empty()) {
			ImGui::SeparatorText("Skins");
			// JS: <component :is="$components.Listboxb" :items="creatureViewerSkins" v-model:selection="creatureViewerSkinsSelection" :single="true">
			for (size_t i = 0; i < view.creatureViewerSkins.size(); i++) {
				const auto& skin = view.creatureViewerSkins[i];
				std::string skin_label = skin.value("label", "");
				std::string skin_id = skin.value("id", "");
				bool is_selected = false;
				for (const auto& sel : view.creatureViewerSkinsSelection) {
					if (sel.value("id", "") == skin_id) {
						is_selected = true;
						break;
					}
				}
				if (ImGui::Selectable(skin_label.c_str(), is_selected)) {
					view.creatureViewerSkinsSelection.clear();
					view.creatureViewerSkinsSelection.push_back(skin);
				}
			}
		}
	}

	// JS: <template v-if="creatureViewerActiveType === 'wmo'">
	if (view.creatureViewerActiveType == "wmo") {
		// JS: <span class="header">WMO Groups</span>
		ImGui::SeparatorText("WMO Groups");
		for (auto& item : view.creatureViewerWMOGroups) {
			bool checked = item.value("checked", false);
			std::string label = item.value("label", "");
			if (ImGui::Checkbox(label.c_str(), &checked))
				item["checked"] = checked;
		}
		// JS: Enable All / Disable All
		if (ImGui::SmallButton("Enable All##creature-wmo-groups")) {
			for (auto& item : view.creatureViewerWMOGroups)
				item["checked"] = true;
		}
		ImGui::SameLine();
		ImGui::Text("/");
		ImGui::SameLine();
		if (ImGui::SmallButton("Disable All##creature-wmo-groups")) {
			for (auto& item : view.creatureViewerWMOGroups)
				item["checked"] = false;
		}

		// JS: <span class="header">Doodad Sets</span>
		ImGui::SeparatorText("Doodad Sets");
		for (auto& item : view.creatureViewerWMOSets) {
			bool checked = item.value("checked", false);
			std::string label = item.value("label", "");
			if (ImGui::Checkbox(label.c_str(), &checked))
				item["checked"] = checked;
		}
	}

	ImGui::EndChild();

	// --- Bottom: Export controls ---
	// JS: <div class="preview-controls">
	//     <component :is="$components.MenuButton" :options="menuButtonCreatures" :default="config.exportCreatureFormat" @change="..." @click="export_creatures">
	{
		std::string current_format = view.config.value("exportCreatureFormat", std::string("OBJ"));

		// JS: MenuButton rendering
		if (ImGui::Button(("Export " + current_format + "##creature-export").c_str()))
			export_creatures();

		ImGui::SameLine();

		// Format selector dropdown
		if (ImGui::BeginCombo("##creature-format", current_format.c_str())) {
			for (const auto& opt : view.menuButtonCreatures) {
				std::string opt_value = opt.value;
				std::string opt_label = opt.label;
				bool is_selected = (opt_value == current_format);
				if (ImGui::Selectable(opt_label.c_str(), is_selected))
					view.config["exportCreatureFormat"] = opt_value;
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}
}

} // namespace tab_creatures
