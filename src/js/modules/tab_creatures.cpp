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
#include "../components/listbox.h"
#include "../components/checkboxlist.h"
#include "../components/listboxb.h"
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
#include "../png-writer.h"
#include "../components/model-viewer-gl.h"
#include "../components/menu-button.h"
#include "../../app.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <locale>
#include <map>
#include <optional>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_creatures {

static std::optional<std::string> build_stack_trace(const char* function_name, const std::exception& e) {
	return std::format("{}: {}", function_name, e.what());
}

struct EquipmentModelEntry {
	struct RendererInfo {
		std::unique_ptr<M2RendererGL> renderer;
		int attachment_id = 0;
	};
	std::vector<RendererInfo> renderers;
	uint32_t display_id = 0;
};

struct CollectionModelEntry {
	std::vector<std::unique_ptr<M2RendererGL>> renderers;
	uint32_t display_id = 0;
};

struct EquipmentSlotEntry {
	uint32_t display_id = 0;
	std::optional<uint32_t> item_id;
};

struct EquipmentChecklistEntry {
	int id = 0;
	std::string label;
	bool checked = true;
};

struct SlotGeosetMapping {
	int group_index;
	int char_geoset;
};

static std::map<std::string, db::caches::DBCreatures::CreatureDisplayInfo> active_skins;

static std::vector<uint32_t> selected_variant_texture_ids;

static model_viewer_utils::RendererResult active_renderer_result;

static uint32_t active_file_data_id = 0;

static const db::caches::DBCreatureList::CreatureEntry* active_creature = nullptr;

static bool is_character_model = false;

static std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>> creature_chr_materials;

// equipment state
static std::map<int, EquipmentModelEntry> equipment_model_renderers;

static std::map<int, CollectionModelEntry> collection_model_renderers;

static std::optional<std::map<int, EquipmentSlotEntry>> creature_equipment;

static std::vector<EquipmentChecklistEntry> creature_equipment_checklist;

static const db::caches::DBCreatureDisplayExtra::ExtraInfo* creature_extra_info = nullptr;

static uint32_t creature_layout_id = 0;

static bool equipment_refresh_lock = false;

// CG constants from DBItemGeosets
namespace CG = db::caches::DBItemGeosets::CG;

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

static model_viewer_utils::ViewStateProxy view_state;

static std::unique_ptr<model_viewer_utils::AnimationMethods> anim_methods;

static std::vector<nlohmann::json> prev_skins_selection;
static std::optional<std::string> prev_anim_selection;
static std::vector<nlohmann::json> prev_selection_creatures;
static std::vector<bool> prev_equipment_checked;

static bool is_initialized = false;
static bool is_initializing = false;

static listbox::ListboxState listbox_creatures_state;
static menu_button::MenuButtonState menu_button_creatures_state;

static checkboxlist::CheckboxListState checkboxlist_creature_equipment_state;
static checkboxlist::CheckboxListState checkboxlist_creature_geosets_state;
static checkboxlist::CheckboxListState checkboxlist_creature_wmo_groups_state;
static checkboxlist::CheckboxListState checkboxlist_creature_wmo_sets_state;
static listboxb::ListboxBState listboxb_creature_skins_state;

static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

static model_viewer_gl::State viewer_state;
static model_viewer_gl::Context viewer_context;

static std::unordered_map<int, model_viewer_gl::EquipmentSlotRenderers> equip_adapter_map;
static std::unordered_map<int, model_viewer_gl::CollectionSlotRenderers> coll_adapter_map;

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

static M2RendererGL* get_active_m2_renderer() {
	return active_renderer_result.m2.get();
}

static model_viewer_utils::ViewStateProxy* get_view_state_ptr() {
	return &view_state;
}

static std::vector<db::caches::DBCreatures::CreatureDisplayInfo> get_creature_displays(uint32_t file_data_id) {
	const auto* displays = db::caches::DBCreatures::getCreatureDisplaysByFileDataID(file_data_id);
	if (displays) {
		std::vector<db::caches::DBCreatures::CreatureDisplayInfo> out;
		out.reserve(displays->size());
		for (const auto& display_ref : *displays)
			out.push_back(display_ref.get());
		return out;
	}
	return {};
}

/**
 * Build equipment data for a character-model creature.
 * Returns Map<slot_id, { display_id, item_id? }> or nullopt.
 *
 */
static std::optional<std::map<int, EquipmentSlotEntry>> build_creature_equipment(
	uint32_t extra_display_id,
	const db::caches::DBCreatureList::CreatureEntry& creature
) {
	std::map<int, EquipmentSlotEntry> equipment;

	// armor from NpcModelItemSlotDisplayInfo (display-ID-based)
	const auto* npc_armor = db::caches::DBNpcEquipment::get_equipment(extra_display_id);
	if (npc_armor) {
		for (const auto& [slot_id, display_id] : *npc_armor)
			equipment[slot_id] = { display_id, std::nullopt };
	}

	// weapons from Creature.AlwaysItem (item-ID-based)
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

	if (!equipment.empty())
		return equipment;
	return std::nullopt;
}

/**
 * Build checklist array for equipment toggle UI.
 *
 */
static std::vector<EquipmentChecklistEntry> build_equipment_checklist(
	const std::map<int, EquipmentSlotEntry>& equipment
) {
	std::vector<EquipmentChecklistEntry> list;

	for (const auto& [slot_id, entry] : equipment) {
		auto slot_name_opt = wow::get_slot_name(slot_id);
		std::string slot_name = slot_name_opt.has_value()
			? std::string(slot_name_opt.value())
			: ("Slot " + std::to_string(slot_id));

		list.push_back({
			slot_id,
			slot_name + " (" + std::to_string(entry.display_id) + ")",
			true
		});
	}

	std::sort(list.begin(), list.end(), [](const EquipmentChecklistEntry& a, const EquipmentChecklistEntry& b) {
		return a.id < b.id;
	});

	return list;
}

/**
 * Get enabled equipment slots from the checklist.
 *
 */
static std::optional<std::map<int, EquipmentSlotEntry>> get_enabled_equipment() {
	if (!creature_equipment.has_value())
		return std::nullopt;

	if (creature_equipment_checklist.empty())
		return creature_equipment;

	std::map<int, EquipmentSlotEntry> enabled;

	for (const auto& item : creature_equipment_checklist) {
		if (item.checked && creature_equipment->contains(item.id))
			enabled[item.id] = creature_equipment->at(item.id);
	}

	if (!enabled.empty())
		return enabled;
	return std::nullopt;
}

/**
 * Apply equipment geosets to creature character model.
 *
 */
static void apply_creature_equipment_geosets() {
	if (!active_renderer_result.m2 || !is_character_model)
		return;

	auto& view = *core::view;

	auto& geosets = view.creatureViewerGeosets;
	if (geosets.empty())
		return;

	auto enabled = get_enabled_equipment();
	if (!enabled.has_value())
		return;

	// build display-id-based slot map for armor
	std::unordered_map<int, uint32_t> slot_display_map;
	for (const auto& [slot_id, entry] : *enabled) {
		if (slot_id <= 19)
			slot_display_map[slot_id] = entry.display_id;
	}

	if (slot_display_map.empty())
		return;

	auto equipment_geosets = db::caches::DBItemGeosets::calculateEquipmentGeosetsByDisplay(slot_display_map);

	auto affected_groups = db::caches::DBItemGeosets::getAffectedCharGeosetsByDisplay(slot_display_map);

	for (int char_geoset : affected_groups) {
		int base = char_geoset * 100;
		int range_start = base + 1;
		int range_end = base + 99;

		for (auto& geoset : geosets) {
			int gid = geoset.value("id", 0);
			if (gid >= range_start && gid <= range_end)
				geoset["checked"] = false;
		}

		auto it = equipment_geosets.find(char_geoset);
		if (it != equipment_geosets.end()) {
			int target_geoset_id = base + it->second;
			for (auto& geoset : geosets) {
				if (geoset.value("id", 0) == target_geoset_id)
					geoset["checked"] = true;
			}
		}
	}

	// helmet hide geosets
	auto head_it = enabled->find(1);
	if (head_it != enabled->end() && creature_extra_info) {
		auto hide_groups = db::caches::DBItemGeosets::getHelmetHideGeosetsByDisplayId(
			head_it->second.display_id,
			creature_extra_info->DisplayRaceID,
			static_cast<int>(creature_extra_info->DisplaySexID)
		);

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

	active_renderer_result.m2->updateGeosets();
}

/**
 * Apply equipment textures to creature character model.
 *
 */
static void apply_creature_equipment_textures() {
	if (!active_renderer_result.m2 || !is_character_model)
		return;

	auto enabled = get_enabled_equipment();
	if (!enabled.has_value() || creature_layout_id == 0)
		return;

	const auto* sections = db::caches::DBCharacterCustomization::get_texture_sections(creature_layout_id);
	if (!sections)
		return;

	std::unordered_map<int, const db::DataRecord*> section_by_type;
	for (const auto& section : *sections) {
		int section_type = static_cast<int>(std::get<int64_t>(section.at("SectionType")));
		section_by_type[section_type] = &section;
	}

	const auto& texture_layer_map = db::caches::DBCharacterCustomization::get_model_texture_layer_map();

	const db::DataRecord* base_layer = nullptr;
	std::string layout_prefix = std::to_string(creature_layout_id) + "-";

	for (const auto& [key, layer] : texture_layer_map) {
		if (key.substr(0, layout_prefix.size()) != layout_prefix)
			continue;

		int bitmask = static_cast<int>(std::get<int64_t>(layer.at("TextureSectionTypeBitMask")));
		int tex_type = static_cast<int>(std::get<int64_t>(layer.at("TextureType")));

		if (bitmask == -1 && tex_type == 1) {
			base_layer = &layer;
			break;
		}
	}

	std::unordered_map<int, const db::DataRecord*> layers_by_section;
	for (const auto& [key, layer] : texture_layer_map) {
		if (key.substr(0, layout_prefix.size()) != layout_prefix)
			continue;

		int bitmask = static_cast<int>(std::get<int64_t>(layer.at("TextureSectionTypeBitMask")));
		if (bitmask == -1)
			continue;

		for (int section_type = 0; section_type < 9; section_type++) {
			if ((1 << section_type) & bitmask) {
				if (!layers_by_section.contains(section_type))
					layers_by_section[section_type] = &layer;
			}
		}
	}

	if (base_layer) {
		for (int section_type = 0; section_type < 9; section_type++) {
			if (!layers_by_section.contains(section_type))
				layers_by_section[section_type] = base_layer;
		}
	}

	for (const auto& [slot_id, entry] : *enabled) {
		int race_id = creature_extra_info ? static_cast<int>(creature_extra_info->DisplayRaceID) : -1;
		int gender_index = creature_extra_info ? static_cast<int>(creature_extra_info->DisplaySexID) : -1;

		std::optional<std::vector<db::caches::DBItemCharTextures::TextureComponent>> item_textures;
		if (entry.item_id.has_value())
			item_textures = db::caches::DBItemCharTextures::getItemTextures(entry.item_id.value(), race_id, gender_index);
		else
			item_textures = db::caches::DBItemCharTextures::getTexturesByDisplayId(entry.display_id, race_id, gender_index);

		if (!item_textures.has_value())
			continue;

		for (const auto& texture : *item_textures) {
			auto section_it = section_by_type.find(texture.section);
			if (section_it == section_by_type.end())
				continue;

			auto layer_it = layers_by_section.find(texture.section);
			if (layer_it == layers_by_section.end())
				continue;

			const db::DataRecord& section_rec = *section_it->second;
			const db::DataRecord& layer_rec = *layer_it->second;

			int layer_tex_type = static_cast<int>(std::get<int64_t>(layer_rec.at("TextureType")));

			const auto* chr_model_material = db::caches::DBCharacterCustomization::get_model_material(
				creature_layout_id, static_cast<uint32_t>(layer_tex_type)
			);
			if (!chr_model_material)
				continue;

			int mat_texture_type = static_cast<int>(std::get<int64_t>(chr_model_material->at("TextureType")));
			int mat_width = static_cast<int>(std::get<int64_t>(chr_model_material->at("Width")));
			int mat_height = static_cast<int>(std::get<int64_t>(chr_model_material->at("Height")));

			uint32_t mat_type_key = static_cast<uint32_t>(mat_texture_type);
			if (!creature_chr_materials.contains(mat_type_key)) {
				auto chr_material = std::make_unique<CharMaterialRenderer>(mat_texture_type, mat_width, mat_height);
				chr_material->init();
				creature_chr_materials[mat_type_key] = std::move(chr_material);
			}

			auto& chr_material = creature_chr_materials[mat_type_key];

			int slot_layer = wow::get_slot_layer(slot_id);

			int chr_model_texture_target_id = (slot_layer * 100) + texture.section;

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
 */
static void apply_creature_equipment_models() {
	if (!active_renderer_result.m2 || !is_character_model)
		return;

	gl::GLContext* gl_ctx = viewer_context.gl_context;
	if (!gl_ctx) return;

	auto enabled = get_enabled_equipment();

	// dispose models for slots no longer enabled
	for (auto it = equipment_model_renderers.begin(); it != equipment_model_renderers.end(); ) {
		if (!enabled.has_value() || !enabled->contains(it->first)) {
			for (auto& ri : it->second.renderers) {
				if (ri.renderer)
					ri.renderer->dispose();
			}
			it = equipment_model_renderers.erase(it);
		} else {
			++it;
		}
	}

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

	if (!enabled.has_value())
		return;

	int race_id = creature_extra_info ? static_cast<int>(creature_extra_info->DisplayRaceID) : -1;
	int gender_index = creature_extra_info ? static_cast<int>(creature_extra_info->DisplaySexID) : -1;

	for (const auto& [slot_id, entry] : *enabled) {
		auto existing_equip_it = equipment_model_renderers.find(slot_id);
		auto existing_coll_it = collection_model_renderers.find(slot_id);

		bool has_existing_equip = (existing_equip_it != equipment_model_renderers.end());
		bool has_existing_coll = (existing_coll_it != collection_model_renderers.end());

		if (has_existing_equip && existing_equip_it->second.display_id == entry.display_id &&
			(!has_existing_coll || existing_coll_it->second.display_id == entry.display_id))
			continue;

		// dispose old if display changed
		if (has_existing_equip) {
			for (auto& ri : existing_equip_it->second.renderers) {
				if (ri.renderer)
					ri.renderer->dispose();
			}
			equipment_model_renderers.erase(existing_equip_it);
		}

		if (has_existing_coll) {
			for (auto& renderer : existing_coll_it->second.renderers) {
				if (renderer)
					renderer->dispose();
			}
			collection_model_renderers.erase(existing_coll_it);
		}

		std::optional<db::caches::DBItemModels::ItemDisplayData> display;
		if (entry.item_id.has_value())
			display = db::caches::DBItemModels::getItemDisplay(entry.item_id.value(), race_id, gender_index);
		else
			display = db::caches::DBItemModels::getDisplayData(entry.display_id, race_id, gender_index);

		if (!display.has_value() || display->models.empty())
			continue;

		// bows held in left hand
		auto attachment_ids_opt = wow::get_attachment_ids_for_slot(slot_id);
		std::vector<int> attachment_ids;
		if (attachment_ids_opt.has_value()) {
			for (int aid : attachment_ids_opt.value())
				attachment_ids.push_back(aid);
		}

		if (slot_id == 16 && entry.item_id.has_value() && db::caches::DBItems::isItemBow(entry.item_id.value()))
			attachment_ids = { wow::ATTACHMENT_ID::HAND_LEFT };

		size_t attachment_model_count = (std::min)(display->models.size(), attachment_ids.size());
		size_t collection_start_index = attachment_model_count;

		// attachment models
		if (attachment_model_count > 0) {
			EquipmentModelEntry equip_entry;
			equip_entry.display_id = entry.display_id;

			for (size_t i = 0; i < attachment_model_count; i++) {
				uint32_t fdid = display->models[i];
				int attachment_id = attachment_ids[i];

				try {
					BufferWrapper file = core::view->casc->getVirtualFileByID(fdid);
					auto renderer = std::make_unique<M2RendererGL>(file, *gl_ctx, false, false);
					renderer->load().get();
					if (!display->textures.empty() && display->textures.size() > i) {
						M2DisplayInfo disp_info;
						disp_info.textures = { display->textures[i] };
						renderer->applyReplaceableTextures(disp_info).get();
					}
					EquipmentModelEntry::RendererInfo ri;
					ri.renderer = std::move(renderer);
					ri.attachment_id = attachment_id;
					equip_entry.renderers.push_back(std::move(ri));
					logging::write(std::format("Loaded creature attachment model {} for slot {}", fdid, slot_id));
				} catch (const std::exception& e) {
					logging::write(std::format("Failed to load creature attachment model {}: {}", fdid, e.what()));
				}
			}

			if (!equip_entry.renderers.empty())
				equipment_model_renderers[slot_id] = std::move(equip_entry);
		}

		// collection models
		if (display->models.size() > collection_start_index) {
			CollectionModelEntry coll_entry;
			coll_entry.display_id = entry.display_id;

			for (size_t i = collection_start_index; i < display->models.size(); i++) {
				uint32_t fdid = display->models[i];

				try {
					BufferWrapper file = core::view->casc->getVirtualFileByID(fdid);
					auto renderer = std::make_unique<M2RendererGL>(file, *gl_ctx, false, false);
					renderer->load().get();
					if (active_renderer_result.m2 && active_renderer_result.m2->get_bones_m2())
						renderer->buildBoneRemapTable(*active_renderer_result.m2->get_bones_m2());
					auto sgit = SLOT_TO_GEOSET_GROUPS.find(slot_id);
					if (sgit != SLOT_TO_GEOSET_GROUPS.end() && !display->attachmentGeosetGroup.empty()) {
						renderer->hideAllGeosets();
						for (const auto& mapping : sgit->second) {
							size_t lookup = static_cast<size_t>(mapping.group_index);
							if (lookup < display->attachmentGeosetGroup.size()) {
								int value = display->attachmentGeosetGroup[lookup];
								renderer->setGeosetGroupDisplay(mapping.char_geoset, 1 + value);
							}
						}
					}
					size_t texture_idx = (i < display->textures.size()) ? i : 0;
					if (texture_idx < display->textures.size() && display->textures[texture_idx] != 0) {
						M2DisplayInfo disp_info;
						disp_info.textures = { display->textures[texture_idx] };
						renderer->applyReplaceableTextures(disp_info).get();
					}
					coll_entry.renderers.push_back(std::move(renderer));
					logging::write(std::format("Loaded creature collection model {} for slot {}", fdid, slot_id));
				} catch (const std::exception& e) {
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
 */
static void dispose_creature_equipment() {
	for (auto& [slot_id, entry] : equipment_model_renderers) {
		for (auto& ri : entry.renderers) {
			if (ri.renderer)
				ri.renderer->dispose();
		}
	}
	equipment_model_renderers.clear();

	for (auto& [slot_id, entry] : collection_model_renderers) {
		for (auto& renderer : entry.renderers) {
			if (renderer)
				renderer->dispose();
		}
	}
	collection_model_renderers.clear();

	creature_equipment = std::nullopt;
	creature_equipment_checklist.clear();
	creature_extra_info = nullptr;
	creature_layout_id = 0;
}

/**
 * Full equipment refresh: geosets, textures, models.
 *
 */
static void refresh_creature_equipment() {
	if (!active_renderer_result.m2 || !is_character_model || !creature_equipment.has_value())
		return;

	auto& view = *core::view;

	// re-apply customization geosets first (reset)
	const auto* display_info = db::caches::DBCreatures::getDisplayInfo(active_creature->displayID);
	if (!display_info)
		return;

	const auto& customization_choices_raw = db::caches::DBCreatureDisplayExtra::get_customization_choices(display_info->extendedDisplayInfoID);
	std::vector<nlohmann::json> customization_choices;
	for (const auto& choice : customization_choices_raw) {
		nlohmann::json j;
		j["optionID"] = choice.optionID;
		j["choiceID"] = choice.choiceID;
		customization_choices.push_back(std::move(j));
	}

	character_appearance::apply_customization_geosets(view.creatureViewerGeosets, customization_choices);

	// re-apply customization textures (reset materials)
	std::unique_ptr<casc::BLPImage> baked_npc_blp;
	uint32_t bake_id = creature_extra_info->HDBakeMaterialResourcesID;
	if (bake_id == 0)
		bake_id = creature_extra_info->BakeMaterialResourcesID;

	if (bake_id > 0) {
		uint32_t bake_fdid = db::caches::DBCharacterCustomization::get_texture_file_data_id(bake_id).value_or(0);
		if (bake_fdid != 0) {
			try {
				BufferWrapper bake_data = core::view->casc->getVirtualFileByID(bake_fdid);
				baked_npc_blp = std::make_unique<casc::BLPImage>(bake_data);
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to load baked NPC texture {}: {}", bake_fdid, e.what()));
			}
		}
	}

	character_appearance::apply_customization_textures(
		active_renderer_result.m2.get(),
		customization_choices,
		creature_layout_id,
		creature_chr_materials,
		baked_npc_blp.get()
	);

	// apply equipment on top
	apply_creature_equipment_geosets();
	apply_creature_equipment_textures();
	character_appearance::upload_textures_to_gpu(active_renderer_result.m2.get(), creature_chr_materials);
	apply_creature_equipment_models();
}

static void preview_creature(const db::caches::DBCreatureList::CreatureEntry& creature) {
	auto _lock = core::create_busy_lock();
	core::setToast("progress", std::format("Loading {}, please wait...", creature.name), {}, -1, false);
	logging::write(std::format("Previewing creature {} (ID: {})", creature.name, creature.id));

	auto& state = view_state;
	texture_ribbon::reset();
	model_viewer_utils::clear_texture_preview(state);

	auto& view = *core::view;
	view.creatureViewerSkins.clear();
	view.creatureViewerSkinsSelection.clear();
	view.creatureViewerAnims.clear();
	view.creatureViewerAnimSelection = nullptr;

	try {
		if (active_renderer_result.type != model_viewer_utils::ModelType::Unknown) {
			active_renderer_result = model_viewer_utils::RendererResult{};
			active_file_data_id = 0;
			active_creature = nullptr;
		}

		character_appearance::dispose_materials(creature_chr_materials);
		dispose_creature_equipment();
		active_skins.clear();
		selected_variant_texture_ids.clear();
		is_character_model = false;
		view.creatureViewerEquipment.clear();

		const auto* display_info = db::caches::DBCreatures::getDisplayInfo(creature.displayID);

		if (display_info && display_info->extendedDisplayInfoID > 0) {
			// character-model creature
			const auto* extra = db::caches::DBCreatureDisplayExtra::get_extra(display_info->extendedDisplayInfoID);
			if (!extra) {
				core::setToast("error", std::format("No extended display info found for creature {}.", creature.name), {}, -1);
				return;
			}

			auto chr_model_id = db::caches::DBCharacterCustomization::get_chr_model_id(extra->DisplayRaceID, extra->DisplaySexID);
			if (!chr_model_id.has_value()) {
				core::setToast("error", std::format("No character model found for creature {} (race {}, sex {}).", creature.name, extra->DisplayRaceID, extra->DisplaySexID), {}, -1);
				return;
			}

			uint32_t file_data_id = db::caches::DBCharacterCustomization::get_model_file_data_id(chr_model_id.value()).value_or(0);
			if (file_data_id == 0) {
				core::setToast("error", std::format("No model file found for creature {}.", creature.name), {}, -1);
				return;
			}

			BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);

			view.creatureViewerActiveType = "m2";

			gl::GLContext* gl_ctx = viewer_context.gl_context;
			if (!gl_ctx) {
				core::setToast("error", "GL context not available — model viewer not initialized.", {}, -1);
				return;
			}

			active_renderer_result.type = model_viewer_utils::ModelType::M2;
			active_renderer_result.m2 = std::make_unique<M2RendererGL>(file, *gl_ctx, true, true);
			active_renderer_result.m2->setGeosetKey("creatureViewerGeosets");
			active_renderer_result.m2->load().get();

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
				uint32_t bake_fdid = db::caches::DBCharacterCustomization::get_texture_file_data_id(bake_id).value_or(0);
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
			uint32_t layout_id = db::caches::DBCharacterCustomization::get_texture_layout_id(chr_model_id.value()).value_or(0);
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
			uint32_t file_data_id = db::caches::DBCreatures::getFileDataIDByDisplayID(creature.displayID).value_or(0);
			if (file_data_id == 0) {
				core::setToast("error", std::format("No model data found for creature {}.", creature.name), {}, -1);
				return;
			}

			BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);
			gl::GLContext* gl_ctx = viewer_context.gl_context;
			if (!gl_ctx) {
				core::setToast("error", "GL context not available — model viewer not initialized.", {}, -1);
				return;
			}

			auto model_type = model_viewer_utils::detect_model_type(file);
			std::string file_name = casc::listfile::getByID(file_data_id).value_or("");
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
				file_data_id,
				file_name
			);

			if (model_type == model_viewer_utils::ModelType::M2)
				active_renderer_result.m2->setGeosetKey("creatureViewerGeosets");
			else if (model_type == model_viewer_utils::ModelType::WMO) {
				active_renderer_result.wmo->setWmoGroupKey("creatureViewerWMOGroups");
				active_renderer_result.wmo->setWmoSetKey("creatureViewerWMOSets");
			}

			if (active_renderer_result.m2)
				active_renderer_result.m2->load().get();
			else if (active_renderer_result.m3)
				active_renderer_result.m3->load();
			else if (active_renderer_result.wmo)
				active_renderer_result.wmo->load();

			if (model_type == model_viewer_utils::ModelType::M2) {
				auto displays = get_creature_displays(file_data_id);

				std::vector<nlohmann::json> skin_list;
				std::string model_name = casc::listfile::getByID(file_data_id).value_or("");
				{
					auto pos = model_name.rfind('/');
					if (pos != std::string::npos) model_name = model_name.substr(pos + 1);
					pos = model_name.rfind('\\');
					if (pos != std::string::npos) model_name = model_name.substr(pos + 1);
					if (model_name.size() >= 2 && model_name.substr(model_name.size() - 2) == "m2")
						model_name = model_name.substr(0, model_name.size() - 2);
				}

				for (const auto& display : displays) {
					if (display.textures.empty())
						continue;

					uint32_t texture = display.textures[0];

					std::string clean_skin_name;
					std::string skin_name = casc::listfile::getByID(texture).value_or("");
					if (!skin_name.empty()) {
						{
							auto pos = skin_name.rfind('/');
							if (pos != std::string::npos) skin_name = skin_name.substr(pos + 1);
							pos = skin_name.rfind('\\');
							if (pos != std::string::npos) skin_name = skin_name.substr(pos + 1);
							if (skin_name.size() > 4 && skin_name.substr(skin_name.size() - 4) == ".blp")
								skin_name = skin_name.substr(0, skin_name.size() - 4);
						}
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

					if (display.extraGeosets.has_value() && !display.extraGeosets->empty()) {
						std::string geoset_str;
						for (size_t gi = 0; gi < display.extraGeosets->size(); gi++) {
							if (gi > 0) geoset_str += ",";
							geoset_str += std::to_string((*display.extraGeosets)[gi]);
						}
						skin_name += geoset_str;
					}

					clean_skin_name += " (" + std::to_string(display.ID) + ")";

					if (active_skins.contains(skin_name))
						continue;

					nlohmann::json skin_entry;
					skin_entry["id"] = skin_name;
					skin_entry["label"] = clean_skin_name;
					skin_list.push_back(std::move(skin_entry));

					active_skins[skin_name] = display;
				}

				view.creatureViewerSkins = std::move(skin_list);

				nlohmann::json matching_skin;
				for (const auto& skin : view.creatureViewerSkins) {
					std::string sid = skin.value("id", "");
					auto it = active_skins.find(sid);
					if (it != active_skins.end() && it->second.ID == creature.displayID) {
						matching_skin = skin;
						break;
					}
				}

				view.creatureViewerSkinsSelection.clear();
				if (!matching_skin.is_null()) {
					view.creatureViewerSkinsSelection.push_back(matching_skin);
				} else if (!view.creatureViewerSkins.empty()) {
					view.creatureViewerSkinsSelection.push_back(view.creatureViewerSkins[0]);
				}

				view.creatureViewerAnims = model_viewer_utils::extract_animations(*active_renderer_result.m2);
				view.creatureViewerAnimSelection = "none";
			}

			active_file_data_id = file_data_id;
			active_creature = &creature;
		}

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

			if (view.creatureViewerAutoAdjust && viewer_context.fitCamera)
				viewer_context.fitCamera();
		}
	} catch (const casc::EncryptionError& e) {
		core::setToast("error", std::format("The model {} is encrypted with an unknown key ({}).", creature.name, e.key), {}, -1);
		logging::write(std::format("Failed to decrypt model {} ({})", creature.name, e.key));
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to preview creature " + creature.name,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}
}

static void export_files(const std::vector<const db::caches::DBCreatureList::CreatureEntry*>& entries) {
	auto& view = *core::view;

	auto export_paths = core::openLastExportStream();

	const std::string format = view.config.value("exportCreatureFormat", std::string("OBJ"));

	if (format == "PNG" || format == "CLIPBOARD") {
		if (active_file_data_id != 0) {
			std::string raw_name = active_creature ? active_creature->name : ("creature_" + std::to_string(active_file_data_id));
			const std::string export_name = casc::ExportHelper::sanitizeFilename(raw_name);

			gl::GLContext* gl_ctx = viewer_context.gl_context;
			if (gl_ctx && viewer_state.fbo != 0) {
				glBindFramebuffer(GL_FRAMEBUFFER, viewer_state.fbo);
				model_viewer_utils::export_preview(format, *gl_ctx, export_name, "creatures");
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		} else {
			core::setToast("error", "The selected export option only works for model previews. Preview something first!", {}, -1);
		}

		export_paths.close();
		return;
	}

	casc::CASC* casc = core::view->casc;

	casc::ExportHelper helper(static_cast<int>(entries.size()), "creature");
	helper.start();

	for (const auto* creature : entries) {
		if (helper.isCancelled())
			break;

		if (!creature)
			continue;

		std::vector<M2ExportFileManifest> file_manifest;
		const std::string creature_name = casc::ExportHelper::sanitizeFilename(creature->name);

		const auto* display_info = db::caches::DBCreatures::getDisplayInfo(creature->displayID);

		if (display_info && display_info->extendedDisplayInfoID > 0) {
			// character-model creature export
			try {
				const auto* extra = db::caches::DBCreatureDisplayExtra::get_extra(display_info->extendedDisplayInfoID);
				if (!extra) {
					helper.mark(creature_name, false, "No extended display info found");
					continue;
				}

				auto chr_model_id = db::caches::DBCharacterCustomization::get_chr_model_id(extra->DisplayRaceID, extra->DisplaySexID);
				uint32_t file_data_id = chr_model_id.has_value() ? db::caches::DBCharacterCustomization::get_model_file_data_id(chr_model_id.value()).value_or(0) : 0;
				if (file_data_id == 0) {
					helper.mark(creature_name, false, "No character model found");
					continue;
				}

				BufferWrapper data = casc->getVirtualFileByID(file_data_id);
				std::string file_name = casc::listfile::getByID(file_data_id).value_or("");
				if (file_name.empty())
					file_name = casc::listfile::formatUnknownFile(file_data_id, ".m2");

				std::string export_path = casc::ExportHelper::getExportPath("creatures/" + creature_name + ".m2");

				const bool is_active = (file_data_id == active_file_data_id && is_character_model);

				if (format == "RAW") {
					M2Exporter exporter(std::move(data), {}, file_data_id, casc);
					export_paths.writeLine(export_path);
					exporter.exportRaw(export_path, &helper, &file_manifest);
					helper.mark(creature_name, true);
				} else {
					auto ext_it = model_viewer_utils::EXPORT_EXTENSIONS.find(format);
					std::string ext = (ext_it != model_viewer_utils::EXPORT_EXTENSIONS.end()) ? ext_it->second : ".gltf";

					std::string final_path = casc::ExportHelper::replaceExtension(export_path, ext);

					M2Exporter exporter(std::move(data), {}, file_data_id, casc);

					if (is_active) {
						for (auto& [texture_type, chr_material] : creature_chr_materials) {
							auto pixels = chr_material->getRawPixels();
							PNGWriter png(static_cast<uint32_t>(chr_material->getWidth()), static_cast<uint32_t>(chr_material->getHeight()));
							auto& pd = png.getPixelData();
							std::memcpy(pd.data(), pixels.data(), pixels.size());
							exporter.addURITexture(texture_type, png.getBuffer());
						}

						std::vector<M2ExportGeosetMask> mask;
						for (const auto& g : view.creatureViewerGeosets) {
							mask.push_back({g.value("checked", false)});
						}
						exporter.setGeosetMask(std::move(mask));

						if (active_renderer_result.m2 && (format == "OBJ" || format == "STL")) {
							if (view.config.value("modelsExportApplyPose", false)) {
								auto baked = active_renderer_result.m2->getBakedGeometry();
								if (baked.has_value())
									exporter.setPosedGeometry(std::move(baked->vertices), std::move(baked->normals));
							}
						}
					} else {
						// build textures for export
						std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>> export_materials;

						const auto& customization_choices_raw = db::caches::DBCreatureDisplayExtra::get_customization_choices(display_info->extendedDisplayInfoID);
						std::vector<nlohmann::json> customization_choices;
						for (const auto& choice : customization_choices_raw) {
							nlohmann::json j;
							j["optionID"] = choice.optionID;
							j["choiceID"] = choice.choiceID;
							customization_choices.push_back(std::move(j));
						}

						uint32_t layout_id = db::caches::DBCharacterCustomization::get_texture_layout_id(chr_model_id.value()).value_or(0);

						std::unique_ptr<casc::BLPImage> baked_npc_blp;
						uint32_t bake_id_val = extra->HDBakeMaterialResourcesID;
						if (bake_id_val == 0) bake_id_val = extra->BakeMaterialResourcesID;
						if (bake_id_val > 0) {
							uint32_t bake_fdid = db::caches::DBCharacterCustomization::get_texture_file_data_id(bake_id_val).value_or(0);
							if (bake_fdid != 0) {
								try {
									BufferWrapper bake_data = casc->getVirtualFileByID(bake_fdid);
									baked_npc_blp = std::make_unique<casc::BLPImage>(bake_data);
								} catch (const std::exception& e) {
									logging::write(std::format("Failed to load baked NPC texture {}: {}", bake_fdid, e.what()));
								}
							}
						}

						character_appearance::apply_customization_textures(nullptr, customization_choices, layout_id, export_materials, baked_npc_blp.get());

						// apply equipment textures for export
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

						for (auto& [texture_type, chr_material] : export_materials) {
							chr_material->update();
							auto pixels = chr_material->getRawPixels();
							PNGWriter png(static_cast<uint32_t>(chr_material->getWidth()), static_cast<uint32_t>(chr_material->getHeight()));
							auto& pd = png.getPixelData();
							std::memcpy(pd.data(), pixels.data(), pixels.size());
							exporter.addURITexture(texture_type, png.getBuffer());
						}

						character_appearance::dispose_materials(export_materials);
					}

					std::string mark_file_name = casc::ExportHelper::getRelativeExport(final_path);

					if (format == "OBJ")
						exporter.exportAsOBJ(final_path, false, &helper, &file_manifest);
					else if (format == "STL")
						exporter.exportAsSTL(final_path, false, &helper, &file_manifest);
					else {
						std::string fmt_lower = format;
						std::transform(fmt_lower.begin(), fmt_lower.end(), fmt_lower.begin(), ::tolower);
						exporter.exportAsGLTF(final_path, &helper, fmt_lower);
					}

					export_paths.writeLine("M2_" + format + ":" + final_path);
					helper.mark(mark_file_name, true);
				}
			} catch (const std::exception& e) {
				helper.mark(creature_name, false, e.what(), build_stack_trace("export_files", e));
			}

			continue;
		}

		uint32_t file_data_id = db::caches::DBCreatures::getFileDataIDByDisplayID(creature->displayID).value_or(0);
		if (file_data_id == 0) {
			helper.mark(creature_name, false, "No model data found");
			continue;
		}

		try {
			BufferWrapper data = casc->getVirtualFileByID(file_data_id);
			auto model_type = model_viewer_utils::detect_model_type(data);
			std::string file_ext = model_viewer_utils::get_model_extension(model_type);
			std::string file_name = casc::listfile::getByID(file_data_id).value_or("");
			if (file_name.empty())
				file_name = casc::listfile::formatUnknownFile(file_data_id, file_ext);
			std::string export_path = casc::ExportHelper::getExportPath("creatures/" + creature_name + file_ext);
			const bool is_active = (file_data_id == active_file_data_id);

			std::vector<nlohmann::json> json_file_manifest;

			model_viewer_utils::ExportModelOptions opts;
			opts.data = &data;
			opts.file_data_id = file_data_id;
			opts.file_name = file_name;
			opts.format = format;
			opts.export_path = export_path;
			opts.helper = &helper;
			opts.casc = casc;
			opts.file_manifest = &json_file_manifest;
			opts.export_paths = &export_paths;
			nlohmann::json variant_textures_json = nlohmann::json::array();
			if (is_active) {
				for (uint32_t tid : selected_variant_texture_ids)
					variant_textures_json.push_back(tid);
			}
			opts.variant_textures = variant_textures_json;
			opts.geoset_mask = is_active ? &view.creatureViewerGeosets : nullptr;
			opts.wmo_group_mask = is_active ? &view.creatureViewerWMOGroups : nullptr;
			opts.wmo_set_mask = is_active ? &view.creatureViewerWMOSets : nullptr;
			opts.active_renderer = is_active ? active_renderer_result.m2.get() : nullptr;
			std::string mark_name = model_viewer_utils::export_model(opts);

			helper.mark(mark_name, true);
		} catch (const std::exception& e) {
			helper.mark(creature_name, false, e.what(), build_stack_trace("export_files", e));
		}
	}

	helper.finish();
	export_paths.close();
}

static void handle_listbox_context(const std::vector<std::string>& selection) {
	listbox_context::handle_context_menu(selection);
	ImGui::OpenPopup("CreaturesListboxContextMenu");
}

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

	ImGui::SetClipboardText(result.c_str());
}

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

static void preview_texture(uint32_t file_data_id, const std::string& display_name) {
	model_viewer_utils::preview_texture_by_id(view_state, get_active_m2_renderer(), file_data_id, display_name, core::view->casc);
}

static void export_ribbon_texture(uint32_t file_data_id, [[maybe_unused]] const std::string& display_name) {
	texture_exporter::exportSingleTexture(file_data_id, core::view->casc);
}

static void toggle_uv_layer(const std::string& layer_name) {
	model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
}

static void export_creatures() {
	auto& view = *core::view;

	const auto& user_selection = view.selectionCreatures;

	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any creatures to export; you should do that first.");
		return;
	}

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

	export_files(creature_items);
}

static void initialize() {
	core::showLoadingScreen(8);

	core::progressLoadingScreen("Loading model file data...");
	db::caches::DBModelFileData::initializeModelFileData();

	core::progressLoadingScreen("Loading creature data...");
	db::caches::DBCreatures::initializeCreatureData();

	core::progressLoadingScreen("Loading character customization data...");
	db::caches::DBCharacterCustomization::ensureInitialized();

	core::progressLoadingScreen("Loading creature display extras...");
	db::caches::DBCreatureDisplayExtra::ensureInitialized();

	core::progressLoadingScreen("Loading NPC equipment data...");
	db::caches::DBNpcEquipment::ensureInitialized();

	core::progressLoadingScreen("Loading item display data...");
	db::caches::DBItemModels::ensureInitialized();
	db::caches::DBItemGeosets::ensureInitialized();
	db::caches::DBItemCharTextures::ensureInitialized();

	core::progressLoadingScreen("Loading item cache...");
	db::caches::DBItems::ensureInitialized();

	core::progressLoadingScreen("Loading creature list...");
	db::caches::DBCreatureList::initialize_creature_list();

	const auto& creatures = db::caches::DBCreatureList::get_all_creatures();
	std::vector<nlohmann::json> entries;

	for (const auto& creature : creatures)
		entries.push_back(std::format("{} [{}]", creature.name, creature.id));

	std::sort(entries.begin(), entries.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
		static const std::regex id_suffix(R"(\s+\[\d+\]$)");
		std::string name_a = std::regex_replace(a.get<std::string>(), id_suffix, "");
		std::string name_b = std::regex_replace(b.get<std::string>(), id_suffix, "");
		static const std::locale loc("");
		const auto& coll = std::use_facet<std::collate<char>>(loc);
		return coll.compare(name_a.data(), name_a.data() + name_a.size(),
		                    name_b.data(), name_b.data() + name_b.size()) < 0;
	});

	core::postToMainThread([entries = std::move(entries)]() mutable {
		core::view->listfileCreatures = std::move(entries);

		if (core::view->creatureViewerContext.is_null()) {
			core::view->creatureViewerContext = nlohmann::json::object();

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

		is_initialized = true;
		is_initializing = false;
	});
	core::hideLoadingScreen();
}

void registerTab() {
	modules::register_nav_button("tab_creatures", "Creatures", "nessy.svg", install_type::CASC);
}

void mounted() {
	if (is_initializing) return;
	is_initializing = true;

	view_state = model_viewer_utils::create_view_state("creature");

	anim_methods = std::make_unique<model_viewer_utils::AnimationMethods>(
		get_active_m2_renderer,
		get_view_state_ptr
	);

	auto& view = *core::view;

	prev_skins_selection = view.creatureViewerSkinsSelection;

	if (view.creatureViewerAnimSelection.is_string())
		prev_anim_selection = view.creatureViewerAnimSelection.get<std::string>();
	else
		prev_anim_selection.reset();

	prev_selection_creatures = view.selectionCreatures;

	prev_equipment_checked.clear();
	for (const auto& item : view.creatureViewerEquipment)
		prev_equipment_checked.push_back(item.value("checked", true));

	core::events.on("toggle-uv-layer", EventEmitter::ArgCallback([](const std::any& arg) {
		const auto& layer_name = std::any_cast<const std::string&>(arg);
		model_viewer_utils::toggle_uv_layer(view_state, get_active_m2_renderer(), layer_name);
	}));

	std::thread(initialize).detach();
}

M2RendererGL* getActiveRenderer() {
	return active_renderer_result.m2.get();
}

void render() {
	auto& view = *core::view;

	if (!is_initialized)
		return;


	{
		if (view.creatureViewerSkinsSelection != prev_skins_selection) {
			prev_skins_selection = view.creatureViewerSkinsSelection;

			if (active_renderer_result.m2 && !active_skins.empty()) {
				if (!view.creatureViewerSkinsSelection.empty()) {
					const auto& selected = view.creatureViewerSkinsSelection[0];
					if (!selected.is_null()) {
						std::string skin_id = selected.value("id", "");
						auto skin_it = active_skins.find(skin_id);
						if (skin_it != active_skins.end()) {
							const auto& display = skin_it->second;

							auto& curr_geosets = view.creatureViewerGeosets;

							if (display.extraGeosets.has_value()) {
								for (auto& geoset : curr_geosets) {
									int gid = geoset.value("id", 0);
									if (gid > 0 && gid < 900)
										geoset["checked"] = false;
								}

								for (uint32_t extra_geoset : *display.extraGeosets) {
									for (auto& geoset : curr_geosets) {
										if (static_cast<uint32_t>(geoset.value("id", 0)) == extra_geoset)
											geoset["checked"] = true;
									}
								}
							} else {
								for (auto& geoset : curr_geosets) {
									std::string id_str = std::to_string(geoset.value("id", 0));
									bool ends_with_0 = (!id_str.empty() && id_str.back() == '0');
									bool ends_with_01 = (id_str.size() >= 2 && id_str.substr(id_str.size() - 2) == "01");
									geoset["checked"] = (ends_with_0 || ends_with_01);
								}
							}

							if (!display.textures.empty())
								selected_variant_texture_ids = display.textures;

							if (active_renderer_result.m2) {
								M2DisplayInfo disp_info;
								disp_info.textures = display.textures;
								active_renderer_result.m2->applyReplaceableTextures(disp_info).get();
							}
						}
					}
				}
			}
		}
	}

	{
		std::optional<std::string> current_anim;
		if (view.creatureViewerAnimSelection.is_string())
			current_anim = view.creatureViewerAnimSelection.get<std::string>();

		if (current_anim != prev_anim_selection) {
			prev_anim_selection = current_anim;

			if (!view.creatureViewerAnims.empty()) {
				model_viewer_utils::handle_animation_change(
					get_active_m2_renderer(),
					view_state,
					current_anim
				);
			}
		}
	}

	{
		if (view.selectionCreatures != prev_selection_creatures) {
			prev_selection_creatures = view.selectionCreatures;

			if (view.config.value("creatureAutoPreview", false)) {
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

			for (size_t i = 0; i < view.creatureViewerEquipment.size() && i < creature_equipment_checklist.size(); ++i)
				creature_equipment_checklist[i].checked = view.creatureViewerEquipment[i].value("checked", true);

			if (!equipment_refresh_lock && active_renderer_result.m2 && is_character_model && creature_equipment.has_value())
				refresh_creature_equipment();
		}
	}


	if (app::layout::BeginTab("tab-creatures")) {
		auto regions = app::layout::CalcListTabRegions(true);

		if (app::layout::BeginListContainer("creatures-list-container", regions)) {
			const auto& items_str = core::cached_json_strings(view.listfileCreatures, s_items_cache, s_items_cache_size);

			std::vector<std::string> selection_str;
			for (const auto& s : view.selectionCreatures)
				selection_str.push_back(s.get<std::string>());

			listbox::CopyMode copy_mode = listbox::CopyMode::Default;
			{
				std::string cm = view.config.value("copyMode", std::string("Default"));
				if (cm == "DIR") copy_mode = listbox::CopyMode::DIR;
				else if (cm == "FID") copy_mode = listbox::CopyMode::FID;
			}

			listbox::render(
				"listbox-creatures",
				items_str,
				view.userInputFilterCreatures,
				selection_str,
				false,      // single
				true,       // keyinput
				view.config.value("regexFilters", false),
				copy_mode,
				view.config.value("pasteSelection", false),
				view.config.value("removePathSpacesCopy", false),
				"creature", // unittype
				nullptr,    // overrideItems
				false,      // disable
				"creatures", // persistscrollkey
				{},         // quickfilters
				false,      // nocopy
				listbox_creatures_state,
				[&](const std::vector<std::string>& new_sel) {
					view.selectionCreatures.clear();
					for (const auto& s : new_sel)
						view.selectionCreatures.push_back(s);
				},
				[](const listbox::ContextMenuEvent& ev) {
					handle_listbox_context(ev.selection);
				}
			);
		}
		app::layout::EndListContainer();

		if (!view.contextMenus.nodeListbox.is_null()) {
			if (ImGui::BeginPopup("CreaturesListboxContextMenu")) {
				const auto& node = view.contextMenus.nodeListbox;
				int count = node.value("count", 0);
				const std::string plural = count > 1 ? "s" : "";
				if (ImGui::MenuItem(std::format("Copy name{}", plural).c_str())) {
					if (node.contains("selection") && node["selection"].is_array())
						copy_creature_names(node["selection"].get<std::vector<nlohmann::json>>());
					view.contextMenus.nodeListbox = nullptr;
				}
				if (ImGui::MenuItem(std::format("Copy ID{}", plural).c_str())) {
					if (node.contains("selection") && node["selection"].is_array())
						copy_creature_ids(node["selection"].get<std::vector<nlohmann::json>>());
					view.contextMenus.nodeListbox = nullptr;
				}
				ImGui::EndPopup();
			}
		}

		if (app::layout::BeginStatusBar("creatures-status", regions)) {
			listbox::renderStatusBar("creature", {}, listbox_creatures_state);
		}
		app::layout::EndStatusBar();

		if (app::layout::BeginFilterBar("creatures-filter", regions)) {
			if (view.config.value("regexFilters", false)) {
				ImGui::TextUnformatted("Regex Enabled");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", view.regexTooltip.c_str());
				ImGui::SameLine();
			}
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			char filter_buf[256] = {};
			std::strncpy(filter_buf, view.userInputFilterCreatures.c_str(), sizeof(filter_buf) - 1);
			if (ImGui::InputText("##FilterCreatures", filter_buf, sizeof(filter_buf)))
				view.userInputFilterCreatures = filter_buf;
		}
		app::layout::EndFilterBar();

		if (app::layout::BeginPreviewContainer("creatures-preview-container", regions)) {
			if (view.config.value("modelViewerShowTextures", true) && !view.textureRibbonStack.empty()) {
				float ribbon_width = ImGui::GetContentRegionAvail().x;
				texture_ribbon::onResize(static_cast<int>(ribbon_width));

				int maxPages = (view.textureRibbonSlotCount > 0)
					? static_cast<int>(std::ceil(static_cast<double>(view.textureRibbonStack.size()) / view.textureRibbonSlotCount))
					: 0;

				if (view.textureRibbonPage > 0) {
					if (ImGui::SmallButton("<##ribbon_prev"))
						view.textureRibbonPage--;
					ImGui::SameLine();
				}

				int startIndex = view.textureRibbonPage * view.textureRibbonSlotCount;
				int endIndex = (std::min)(startIndex + view.textureRibbonSlotCount, static_cast<int>(view.textureRibbonStack.size()));

				for (int si = startIndex; si < endIndex; si++) {
					auto& slot = view.textureRibbonStack[si];
					std::string slotDisplayName = slot.value("displayName", std::string(""));

					ImGui::PushID(si);
					GLuint slotTex = texture_ribbon::getSlotTexture(si);
					bool clicked = false;
					if (slotTex != 0) {
						clicked = ImGui::ImageButton("##ribbon_slot",
							static_cast<ImTextureID>(static_cast<uintptr_t>(slotTex)),
							ImVec2(64, 64));
					} else {
						clicked = ImGui::Button(slotDisplayName.c_str(), ImVec2(64, 64));
					}
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", slotDisplayName.c_str());
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right) || clicked) {
						view.contextMenus.nodeTextureRibbon = slot;
						ImGui::OpenPopup("CreatureTextureRibbonContextMenu");
					}
					ImGui::PopID();
					ImGui::SameLine();
				}

				if (view.textureRibbonPage < maxPages - 1) {
					if (ImGui::SmallButton(">##ribbon_next"))
						view.textureRibbonPage++;
				} else {
					ImGui::NewLine();
				}

				if (!view.contextMenus.nodeTextureRibbon.is_null()) {
					if (ImGui::BeginPopup("CreatureTextureRibbonContextMenu")) {
						const auto& node = view.contextMenus.nodeTextureRibbon;
						uint32_t fdid = node.value("fileDataID", 0u);
						std::string ctxDisplayName = node.value("displayName", std::string(""));

						if (ImGui::MenuItem(std::format("Preview {}", ctxDisplayName).c_str())) {
							preview_texture(fdid, ctxDisplayName);
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						if (ImGui::MenuItem(std::format("Export {}", ctxDisplayName).c_str())) {
							export_ribbon_texture(fdid, ctxDisplayName);
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						if (ImGui::MenuItem("Copy file data ID to clipboard")) {
							ImGui::SetClipboardText(std::to_string(fdid).c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						if (ImGui::MenuItem("Copy texture name to clipboard")) {
							ImGui::SetClipboardText(ctxDisplayName.c_str());
							view.contextMenus.nodeTextureRibbon = nullptr;
						}

						ImGui::EndPopup();
					}
				}
			}

			if (!view.creatureTexturePreviewURL.empty()) {
				if (ImGui::Button("Close Preview"))
					view.creatureTexturePreviewURL.clear();

				if (view.creatureTexturePreviewTexID != 0) {
					const ImVec2 avail = ImGui::GetContentRegionAvail();
					const float tex_w = static_cast<float>(view.creatureTexturePreviewWidth);
					const float tex_h = static_cast<float>(view.creatureTexturePreviewHeight);
					const float scale = std::min(avail.x / tex_w, avail.y / tex_h);
					const ImVec2 img_size(tex_w * scale, tex_h * scale);

					const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
					ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.creatureTexturePreviewTexID)), img_size);

					if (view.creatureTexturePreviewUVTexID != 0 && !view.creatureTexturePreviewUVOverlay.empty()) {
						ImGui::SetCursorScreenPos(cursor_pos);
						ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(view.creatureTexturePreviewUVTexID)), img_size);
					}
				} else {
					ImGui::Text("Texture preview: %dx%d", view.creatureTexturePreviewWidth, view.creatureTexturePreviewHeight);
				}

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

			if (view.config.value("modelViewerShowBackground", false)) {
				std::string hex_str = view.config.value("modelViewerBackgroundColor", std::string("#343a40"));
				auto [cr, cg, cb] = model_viewer_gl::parse_hex_color(hex_str);
				float color[3] = {cr, cg, cb};
				if (ImGui::ColorEdit3("##bg_color_creatures", color, ImGuiColorEditFlags_NoInputs))
					view.config["modelViewerBackgroundColor"] = std::format("#{:02x}{:02x}{:02x}",
						static_cast<int>(color[0] * 255.0f), static_cast<int>(color[1] * 255.0f), static_cast<int>(color[2] * 255.0f));
			}

			if (!view.creatureViewerContext.is_null()) {
				model_viewer_gl::renderWidget("##creature_viewer", viewer_state, viewer_context);
			}

			if (!view.creatureViewerAnims.empty() && view.creatureTexturePreviewURL.empty()) {
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

				if (current_anim_id != "none" && !current_anim_id.empty()) {
					bool is_paused = view.creatureViewerAnimPaused;

					ImGui::BeginDisabled(!is_paused);
					if (ImGui::Button("<##creature-step-left"))
						anim_methods->step_animation(-1);
					ImGui::EndDisabled();

					ImGui::SameLine();

					if (ImGui::Button(is_paused ? "Play##creature" : "Pause##creature"))
						anim_methods->toggle_animation_pause();

					ImGui::SameLine();

					ImGui::BeginDisabled(!is_paused);
					if (ImGui::Button(">##creature-step-right"))
						anim_methods->step_animation(1);
					ImGui::EndDisabled();

					ImGui::SameLine();

					int frame = view.creatureViewerAnimFrame;
					int max_frame = view.creatureViewerAnimFrameCount > 0 ? view.creatureViewerAnimFrameCount - 1 : 0;

					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80.0f);
					if (ImGui::SliderInt("##creature-scrub", &frame, 0, max_frame)) {
						anim_methods->seek_animation(frame);
					}
					if (ImGui::IsItemActivated())
						anim_methods->start_scrub();
					if (ImGui::IsItemDeactivatedAfterEdit())
						anim_methods->end_scrub();

					ImGui::SameLine();
					ImGui::Text("%d", view.creatureViewerAnimFrame);
				}
			}
		}
		app::layout::EndPreviewContainer();

		if (app::layout::BeginPreviewControls("creatures-preview-controls", regions)) {
			std::vector<menu_button::MenuOption> mb_options;
			for (const auto& opt : view.menuButtonCreatures)
				mb_options.push_back({ opt.label, opt.value });
			menu_button::render("##MenuButtonCreatures", mb_options,
				view.config.value("exportCreatureFormat", std::string("OBJ")),
				view.isBusy > 0, false, menu_button_creatures_state,
				[&](const std::string& val) { view.config["exportCreatureFormat"] = val; },
				[&]() { export_creatures(); });
		}
		app::layout::EndPreviewControls();

		if (app::layout::BeginSidebar("creatures-sidebar", regions)) {
			ImGui::SeparatorText("Preview");

			{
				bool auto_preview = view.config.value("creatureAutoPreview", false);
				if (ImGui::Checkbox("Auto Preview##creature", &auto_preview))
					view.config["creatureAutoPreview"] = auto_preview;
				ImGui::SetItemTooltip("Automatically preview a creature when selecting it");
			}

			ImGui::Checkbox("Auto Camera##creature", &view.creatureViewerAutoAdjust);
			ImGui::SetItemTooltip("Automatically adjust camera when selecting a new creature");

			{
				bool show_grid = view.config.value("modelViewerShowGrid", false);
				if (ImGui::Checkbox("Show Grid##creature", &show_grid))
					view.config["modelViewerShowGrid"] = show_grid;
				ImGui::SetItemTooltip("Show a grid in the 3D viewport");
			}

			{
				bool wireframe = view.config.value("modelViewerWireframe", false);
				if (ImGui::Checkbox("Show Wireframe##creature", &wireframe))
					view.config["modelViewerWireframe"] = wireframe;
				ImGui::SetItemTooltip("Render the preview model as a wireframe");
			}

			{
				bool show_bones = view.config.value("modelViewerShowBones", false);
				if (ImGui::Checkbox("Show Bones##creature", &show_bones))
					view.config["modelViewerShowBones"] = show_bones;
				ImGui::SetItemTooltip("Show the model's bone structure");
			}

			{
				bool show_textures = view.config.value("modelViewerShowTextures", true);
				if (ImGui::Checkbox("Show Textures##creature", &show_textures))
					view.config["modelViewerShowTextures"] = show_textures;
				ImGui::SetItemTooltip("Show model textures in the preview pane");
			}

			{
				bool show_bg = view.config.value("modelViewerShowBackground", false);
				if (ImGui::Checkbox("Show Background##creature", &show_bg))
					view.config["modelViewerShowBackground"] = show_bg;
				ImGui::SetItemTooltip("Show a background color in the 3D viewport");
			}

			ImGui::SeparatorText("Export");

			{
				bool export_textures = view.config.value("modelsExportTextures", true);
				if (ImGui::Checkbox("Textures##creature-export", &export_textures))
					view.config["modelsExportTextures"] = export_textures;

				if (export_textures) {
					bool export_alpha = view.config.value("modelsExportAlpha", false);
					if (ImGui::Checkbox("Texture Alpha##creature", &export_alpha))
						view.config["modelsExportAlpha"] = export_alpha;
				}
			}

			{
				std::string fmt = view.config.value("exportCreatureFormat", std::string("OBJ"));
				if (fmt == "GLTF" && view.creatureViewerActiveType == "m2") {
					bool export_anims = view.config.value("modelsExportAnimations", false);
					if (ImGui::Checkbox("Export animations##creature", &export_anims))
						view.config["modelsExportAnimations"] = export_anims;
				}

				if ((fmt == "OBJ" || fmt == "STL") && view.creatureViewerActiveType == "m2") {
					bool apply_pose = view.config.value("modelsExportApplyPose", false);
					if (ImGui::Checkbox("Apply pose##creature", &apply_pose))
						view.config["modelsExportApplyPose"] = apply_pose;
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Apply current animation pose to exported geometry");
				}
			}

			if (view.creatureViewerActiveType == "m2") {
				if (!view.creatureViewerEquipment.empty()) {
					ImGui::SeparatorText("Equipment");
					checkboxlist::render("##CreatureEquipment", view.creatureViewerEquipment, checkboxlist_creature_equipment_state);
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

				if (!view.creatureViewerGeosets.empty()) {
					ImGui::SeparatorText("Geosets");
					checkboxlist::render("##CreatureGeosets", view.creatureViewerGeosets, checkboxlist_creature_geosets_state);
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

				bool show_textures = view.config.value("modelsExportTextures", true);
				if (show_textures && !view.creatureViewerSkins.empty()) {
					ImGui::SeparatorText("Skins");

					std::vector<listboxb::ListboxBItem> skin_items;
					skin_items.reserve(view.creatureViewerSkins.size());
					for (const auto& skin : view.creatureViewerSkins)
						skin_items.push_back({ skin.value("label", std::string("")) });

					std::vector<int> sel_indices;
					for (const auto& sel : view.creatureViewerSkinsSelection) {
						std::string sel_id = sel.value("id", std::string(""));
						for (size_t i = 0; i < view.creatureViewerSkins.size(); ++i) {
							if (view.creatureViewerSkins[i].value("id", std::string("")) == sel_id) {
								sel_indices.push_back(static_cast<int>(i));
								break;
							}
						}
					}

					float constrainedHeight = 156.0f;
					ImGui::BeginChild("##CreatureSkinsListWrapper", ImVec2(0.0f, constrainedHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
					listboxb::render("##CreatureSkins", skin_items, sel_indices, true, true, false,
						listboxb_creature_skins_state,
						[&](const std::vector<int>& new_sel) {
							view.creatureViewerSkinsSelection.clear();
							for (int idx : new_sel) {
								if (idx >= 0 && idx < static_cast<int>(view.creatureViewerSkins.size()))
									view.creatureViewerSkinsSelection.push_back(view.creatureViewerSkins[idx]);
							}
						});
					ImGui::EndChild();
				}
			}

			if (view.creatureViewerActiveType == "wmo") {
				ImGui::SeparatorText("WMO Groups");
				checkboxlist::render("##CreatureWMOGroups", view.creatureViewerWMOGroups, checkboxlist_creature_wmo_groups_state);
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

				ImGui::SeparatorText("Doodad Sets");
				checkboxlist::render("##CreatureWMOSets", view.creatureViewerWMOSets, checkboxlist_creature_wmo_sets_state);
			}
		}
		app::layout::EndSidebar();
	}
	app::layout::EndTab();
}

} // namespace tab_creatures
