/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <optional>

/**
 * Textures tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template,
 * previewTextureByID(), methods (handle_listbox_context, copy_file_paths,
 * copy_listfile_format, copy_file_data_ids, copy_export_paths,
 * open_export_directory, is_baked_npc_texture, remove_override_textures,
 * export_textures, export_atlas_regions, initialize, apply_baked_npc_texture),
 * and mounted().
 *
 * Provides: Textures nav button (CASC only), texture file listbox with
 * context menu, texture preview with channel mask toggle, atlas overlay,
 * baked NPC texture support, export in PNG/WEBP/BLP format.
 */
namespace tab_textures {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Textures', 'image.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the textures tab (unknown textures, atlas data, selection watch).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Render the textures tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Preview a texture by file data ID.
 * JS equivalent: previewTextureByID(core, file_data_id, texture)
 * @param file_data_id  File data ID of the texture.
 * @param texture       Optional display name override.
 */
void previewTextureByID(uint32_t file_data_id, const std::string& texture = "");

/**
 * Export selected textures.
 * JS equivalent: methods.export_textures()
 */
void export_textures();

/**
 * Clear the override texture filter (used by the toast "Remove" action).
 * JS equivalent: methods.remove_override_textures() → this.$core.view.removeOverrideTextures()
 */
void remove_override_textures();

/**
 * Export atlas regions of the currently previewed texture.
 * JS equivalent: methods.export_atlas_regions()
 */
void export_atlas_regions();

/**
 * Switch to the textures tab and preview a texture by file data ID.
 * JS equivalent: $core.view.goToTexture(fileDataID)
 * @param fileDataID  File data ID of the texture to navigate to.
 */
void goToTexture(uint32_t fileDataID);

} // namespace tab_textures
