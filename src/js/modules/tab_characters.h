/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>

class M2RendererGL;

/**
 * Characters tab module (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with register(), template, methods
 * (import_character, import_wmv, import_wowhead, toggle_animation_pause,
 * step_animation, seek_animation, start_scrub, end_scrub,
 * update_choice_for_option, randomize_customization, set_all_geosets,
 * toggle_color_picker, select_color_choice, get_selected_choice,
 * int_to_css_color, export_character, remove_baked_npc_texture,
 * open_saved_characters, open_save_prompt, confirm_save_character,
 * on_load_character, on_delete_character, on_export_character,
 * import_json, import_json_to_saved, export_json, chr_prev_overlay,
 * chr_next_overlay, chr_export_overlay, get_equipped_item,
 * open_slot_context, navigate_to_items_for_slot, unequip_slot,
 * replace_slot_item, copy_item_id, copy_item_name,
 * clear_all_equipment, is_guild_tabard_equipped, get_tabard_tier,
 * get_tabard_max, set_tabard_config, adjust_tabard_config,
 * toggle_tabard_color_picker, select_tabard_color, get_tabard_color_css,
 * get_tabard_color_list, get_tabard_color_list_for_key),
 * and mounted().
 *
 * Also exposes: get_default_characters_dir().
 *
 * Provides: Characters nav button (CASC only), race/body/customization
 * selectors, color swatch pickers, geoset toggle, 3D model preview
 * with animation controls, equipment panel with context menu,
 * guild tabard editor, saved characters screen with thumbnail capture,
 * Battle.net/Wowhead/WMV/JSON import/export.
 * Export in OBJ/STL/GLTF/GLB/PNG/CLIPBOARD format.
 */
namespace tab_characters {

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Characters', 'person-solid.svg', InstallType.CASC) }
 */
void registerTab();

/**
 * Initialize the characters tab (load DB caches, realmlist, set up watches).
 * JS equivalent: mounted()
 */
void mounted();

/**
 * Tear down the characters tab (release GL resources, clear state).
 * JS equivalent: unmounted() { reset_module_state(); }
 */
void unmounted();

/**
 * Render the characters tab widget using ImGui.
 * Equivalent to the Vue component's template rendering.
 */
void render();

/**
 * Get the default directory for saved character files.
 * JS equivalent: get_default_characters_dir()
 */
std::string get_default_characters_dir();

/**
 * Get the active model renderer (if any).
 * JS equivalent: getActiveRenderer: () => active_renderer
 * @returns Pointer to active M2RendererGL, or nullptr.
 */
M2RendererGL* getActiveRenderer();

} // namespace tab_characters
