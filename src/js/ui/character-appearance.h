/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <nlohmann/json.hpp>

namespace casc {
class BLPImage;
}

class CharMaterialRenderer;
class M2RendererGL;

/**
 * Character appearance utilities — geoset and texture customization.
 *
 * JS equivalent: module.exports = { apply_customization_geosets,
 *     apply_customization_textures, upload_textures_to_gpu, dispose_materials }
 */
namespace character_appearance {

/**
 * Reset geosets to model defaults, then apply customization choice geosets.
 * Does NOT apply equipment geosets — caller handles that.
 * @param geosets        Geoset checkbox array from view state (mutated in-place).
 * @param active_choices Array of { optionID, choiceID } objects.
 */
void apply_customization_geosets(
	std::vector<nlohmann::json>& geosets,
	const std::vector<nlohmann::json>& active_choices
);

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
	M2RendererGL* renderer,
	const std::vector<nlohmann::json>& active_choices,
	uint32_t layout_id,
	std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>>& chr_materials,
	casc::BLPImage* baked_npc_blp = nullptr
);

/**
 * Upload all chr_materials to the GPU via the renderer.
 * @param renderer      M2 renderer instance.
 * @param chr_materials Map of texture_type -> CharMaterialRenderer.
 */
void upload_textures_to_gpu(
	M2RendererGL* renderer,
	std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>>& chr_materials
);

/**
 * Dispose all CharMaterialRenderer instances in a map and clear the map.
 * @param chr_materials Map of texture_type -> CharMaterialRenderer (mutated — cleared).
 */
void dispose_materials(
	std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>>& chr_materials
);

} // namespace character_appearance
