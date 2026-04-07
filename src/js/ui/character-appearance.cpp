/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "character-appearance.h"

#include <algorithm>
#include <string>
#include <variant>

#include "../3D/renderers/CharMaterialRenderer.h"
#include "../3D/renderers/M2RendererGL.h"
#include "../db/WDCReader.h"
#include "../db/caches/DBCharacterCustomization.h"

namespace character_appearance {

/**
 * Helper: extract an integer field from a DataRecord variant map.
 */
static int64_t get_field_int(const db::DataRecord& rec, const std::string& key, int64_t def = 0) {
	auto it = rec.find(key);
	if (it == rec.end()) return def;
	return std::visit([def](const auto& v) -> int64_t {
		using T = std::decay_t<decltype(v)>;
		if constexpr (std::is_integral_v<T>)
			return static_cast<int64_t>(v);
		else if constexpr (std::is_floating_point_v<T>)
			return static_cast<int64_t>(v);
		else
			return def;
	}, it->second);
}

/**
 * Reset geosets to model defaults, then apply customization choice geosets.
 * Does NOT apply equipment geosets — caller handles that.
 * @param geosets        Geoset checkbox array from view state (mutated in-place).
 * @param active_choices Array of { optionID, choiceID } objects.
 */
void apply_customization_geosets(
	std::vector<nlohmann::json>& geosets,
	const std::vector<nlohmann::json>& active_choices)
{
	if (geosets.empty())
		return;

	// reset geosets to model defaults
	for (auto& geoset : geosets) {
		const int id = geoset.value("id", 0);
		const std::string id_str = std::to_string(id);
		const bool is_default = (id == 0 || (id_str.size() >= 2 && id_str.substr(id_str.size() - 2) == "01") || id_str.rfind("32", 0) == 0);
		const bool is_hidden_default = id_str.rfind("17", 0) == 0 || id_str.rfind("35", 0) == 0;

		geoset["checked"] = is_default && !is_hidden_default;
	}

	// apply customization geosets
	for (const auto& active_choice : active_choices) {
		const uint32_t optionID = active_choice.value("optionID", 0u);
		const uint32_t choiceID = active_choice.value("choiceID", 0u);

		const auto* available_choices = db::caches::DBCharacterCustomization::get_choices_for_option(optionID);
		if (!available_choices)
			continue;

		for (const auto& available_choice : *available_choices) {
			const auto chr_cust_geo_id = db::caches::DBCharacterCustomization::get_choice_geoset_raw(available_choice.id);
			if (!chr_cust_geo_id)
				continue;

			const auto geoset_id_opt = db::caches::DBCharacterCustomization::get_geoset_value(*chr_cust_geo_id);
			if (!geoset_id_opt)
				continue;

			const int geoset_id = *geoset_id_opt;

			for (auto& geoset : geosets) {
				const int gid = geoset.value("id", 0);
				if (gid == 0)
					continue;

				if (gid == geoset_id) {
					const bool should_be_checked = (available_choice.id == choiceID);
					geoset["checked"] = should_be_checked;
				}
			}
		}
	}
}

/**
 * Reset materials, apply baked NPC texture + customization textures, upload to GPU.
 * Does NOT apply equipment textures — caller handles that.
 * @param renderer       M2 renderer instance (unused but kept for API parity with JS).
 * @param active_choices Array of { optionID, choiceID } objects.
 * @param layout_id      CharComponentTextureLayoutID.
 * @param chr_materials  Map of texture_type -> CharMaterialRenderer (mutated).
 * @param baked_npc_blp  Pre-loaded BLP for baked NPC texture, or nullptr.
 * @returns The texture type used for the baked NPC texture, or nullopt.
 */
std::optional<uint32_t> apply_customization_textures(
	[[maybe_unused]] M2RendererGL* renderer,
	const std::vector<nlohmann::json>& active_choices,
	uint32_t layout_id,
	std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>>& chr_materials,
	casc::BLPImage* baked_npc_blp)
{
	// reset all existing materials
	for (auto& [type, chr_material] : chr_materials) {
		chr_material->reset();
		chr_material->update();
	}

	std::optional<uint32_t> baked_npc_texture_type;

	// apply baked NPC texture
	if (baked_npc_blp) {
		const auto& model_material_map = db::caches::DBCharacterCustomization::get_model_material_map();
		const std::string prefix = std::to_string(layout_id) + "-";

		struct AvailableType {
			std::string key;
			int64_t type;
			const db::DataRecord* material;
		};
		std::vector<AvailableType> available_types;

		for (const auto& [key, value] : model_material_map) {
			if (key.rfind(prefix, 0) == 0)
				available_types.push_back({key, get_field_int(value, "TextureType"), &value});
		}

		std::sort(available_types.begin(), available_types.end(),
			[](const AvailableType& a, const AvailableType& b) { return a.type < b.type; });

		const db::DataRecord* chr_model_material = available_types.empty() ? nullptr : available_types[0].material;
		const int64_t texture_type = available_types.empty() ? 0 : available_types[0].type;

		if (chr_model_material) {
			const uint32_t tex_type_u = static_cast<uint32_t>(texture_type);
			CharMaterialRenderer* chr_material = nullptr;

			if (chr_materials.find(tex_type_u) == chr_materials.end()) {
				auto mat = std::make_unique<CharMaterialRenderer>(
					static_cast<int>(texture_type),
					static_cast<int>(get_field_int(*chr_model_material, "Width", 512)),
					static_cast<int>(get_field_int(*chr_model_material, "Height", 512))
				);
				mat->init();
				chr_material = mat.get();
				chr_materials[tex_type_u] = std::move(mat);
			} else {
				chr_material = chr_materials[tex_type_u].get();
			}

			chr_material->setTextureTarget(
				0,                                                                     // chrModelTextureTargetID
				0,                                                                     // fileDataID
				0, 0,                                                                  // sectionX, sectionY
				static_cast<int>(get_field_int(*chr_model_material, "Width", 512)),
				static_cast<int>(get_field_int(*chr_model_material, "Height", 512)),
				static_cast<int>(get_field_int(*chr_model_material, "TextureType")),  // materialTextureType
				static_cast<int>(get_field_int(*chr_model_material, "Width", 512)),   // materialWidth
				static_cast<int>(get_field_int(*chr_model_material, "Height", 512)),  // materialHeight
				0,                                                                     // textureLayerBlendMode
				true,                                                                  // useAlpha
				baked_npc_blp                                                          // blpOverride
			);

			baked_npc_texture_type = tex_type_u;
		}
	}

	// apply customization textures
	for (const auto& active_choice : active_choices) {
		const uint32_t choiceID = active_choice.value("choiceID", 0u);

		const auto* chr_cust_mat_ids = db::caches::DBCharacterCustomization::get_choice_materials(choiceID);
		if (!chr_cust_mat_ids)
			continue;

		for (const auto& chr_cust_mat_id : *chr_cust_mat_ids) {
			if (chr_cust_mat_id.RelatedChrCustomizationChoiceID != 0) {
				bool has_related = false;
				for (const auto& selected : active_choices) {
					if (selected.value("choiceID", 0u) == chr_cust_mat_id.RelatedChrCustomizationChoiceID) {
						has_related = true;
						break;
					}
				}
				if (!has_related)
					continue;
			}

			const auto* chr_cust_mat = db::caches::DBCharacterCustomization::get_chr_cust_material(chr_cust_mat_id.ChrCustomizationMaterialID);
			if (!chr_cust_mat)
				continue;

			const uint32_t chr_model_texture_target = chr_cust_mat->ChrModelTextureTargetID;

			const db::DataRecord* chr_model_texture_layer = db::caches::DBCharacterCustomization::get_model_texture_layer(layout_id, chr_model_texture_target);
			if (!chr_model_texture_layer)
				continue;

			const int64_t texture_type_layer = get_field_int(*chr_model_texture_layer, "TextureType");
			const db::DataRecord* chr_model_material = db::caches::DBCharacterCustomization::get_model_material(layout_id, static_cast<uint32_t>(texture_type_layer));
			if (!chr_model_material)
				continue;

			const int64_t mat_texture_type = get_field_int(*chr_model_material, "TextureType");

			// skip if baked NPC texture covers this texture type
			if (baked_npc_texture_type && static_cast<uint32_t>(mat_texture_type) == *baked_npc_texture_type)
				continue;

			const uint32_t mat_type_u = static_cast<uint32_t>(mat_texture_type);
			CharMaterialRenderer* chr_material = nullptr;

			if (chr_materials.find(mat_type_u) == chr_materials.end()) {
				auto mat = std::make_unique<CharMaterialRenderer>(
					static_cast<int>(mat_texture_type),
					static_cast<int>(get_field_int(*chr_model_material, "Width", 512)),
					static_cast<int>(get_field_int(*chr_model_material, "Height", 512))
				);
				mat->init();
				chr_material = mat.get();
				chr_materials[mat_type_u] = std::move(mat);
			} else {
				chr_material = chr_materials[mat_type_u].get();
			}

			const int64_t bit_mask = get_field_int(*chr_model_texture_layer, "TextureSectionTypeBitMask");

			int sectionX = 0, sectionY = 0;
			int sectionWidth  = static_cast<int>(get_field_int(*chr_model_material, "Width", 512));
			int sectionHeight = static_cast<int>(get_field_int(*chr_model_material, "Height", 512));

			if (bit_mask != -1) {
				const auto* sections = db::caches::DBCharacterCustomization::get_texture_sections(layout_id);
				bool found_section = false;
				if (sections) {
					for (const auto& row : *sections) {
						const int64_t section_type = get_field_int(row, "SectionType");
						if ((1LL << section_type) & bit_mask) {
							sectionX      = static_cast<int>(get_field_int(row, "X"));
							sectionY      = static_cast<int>(get_field_int(row, "Y"));
							sectionWidth  = static_cast<int>(get_field_int(row, "Width"));
							sectionHeight = static_cast<int>(get_field_int(row, "Height"));
							found_section = true;
							break;
						}
					}
				}
				if (!found_section)
					continue;
			}

			chr_material->setTextureTarget(
				static_cast<int>(chr_cust_mat->ChrModelTextureTargetID),
				chr_cust_mat->FileDataID,
				sectionX, sectionY, sectionWidth, sectionHeight,
				static_cast<int>(get_field_int(*chr_model_material, "TextureType")),
				static_cast<int>(get_field_int(*chr_model_material, "Width", 512)),
				static_cast<int>(get_field_int(*chr_model_material, "Height", 512)),
				static_cast<int>(get_field_int(*chr_model_texture_layer, "BlendMode")),
				true
			);
		}
	}

	return baked_npc_texture_type;
}

/**
 * Upload all chr_materials to the GPU via the renderer.
 * @param renderer      M2 renderer instance.
 * @param chr_materials Map of texture_type -> CharMaterialRenderer.
 */
void upload_textures_to_gpu(
	M2RendererGL* renderer,
	std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>>& chr_materials)
{
	for (auto& [chr_model_texture_target, chr_material] : chr_materials) {
		chr_material->update();
		const std::vector<uint8_t> pixels = chr_material->getRawPixels();
		renderer->overrideTextureTypeWithPixels(
			chr_model_texture_target,
			chr_material->getWidth(),
			chr_material->getHeight(),
			pixels.data()
		);
	}
}

/**
 * Dispose all CharMaterialRenderer instances in a map and clear the map.
 * @param chr_materials Map of texture_type -> CharMaterialRenderer (mutated — cleared).
 */
void dispose_materials(
	std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>>& chr_materials)
{
	for (auto& [type, chr_material] : chr_materials)
		chr_material->dispose();

	chr_materials.clear();
}

} // namespace character_appearance
