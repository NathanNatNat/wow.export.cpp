/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "tab_characters.h"
#include "tab_items.h"
#include "../log.h"
#include "../core.h"
#include "../buffer.h"
#include "../generics.h"
#include "../png-writer.h"
#include "../file-writer.h"
#include "../install-type.h"
#include "../modules.h"
#include "../casc/blp.h"
#include "../casc/casc-source.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/realmlist.h"
#include "../components/file-field.h"
#include "../ui/char-texture-overlay.h"
#include "../ui/character-appearance.h"
#include "../ui/model-viewer-utils.h"
#include "../3D/AnimMapper.h"
#include "../3D/renderers/CharMaterialRenderer.h"
#include "../3D/renderers/M2RendererGL.h"
#include "../3D/exporters/M2Exporter.h"
#include "../3D/exporters/CharacterExporter.h"
#include "../db/caches/DBCharacterCustomization.h"
#include "../db/caches/DBItems.h"
#include "../db/caches/DBItemCharTextures.h"
#include "../db/caches/DBItemGeosets.h"
#include "../db/caches/DBItemModels.h"
#include "../db/caches/DBGuildTabard.h"
#include "../casc/db2.h"
#include "../db/WDCReader.h"
#include "../wow/EquipmentSlots.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace tab_characters {

// --- Namespace alias for CG geoset group constants ---
namespace CG = db::caches::DBItemGeosets::CG;

// --- File-local structures ---

// JS: equipment_model_renderers entries: { renderers: [{ renderer, attachment_id, is_collection_style }], item_id }
struct EquipmentModelEntry {
struct RendererInfo {
std::unique_ptr<M2RendererGL> renderer;
int attachment_id = 0;
bool is_collection_style = false;
};
std::vector<RendererInfo> renderers;
uint32_t item_id = 0;
};

// JS: collection_model_renderers entries: { renderers: [M2RendererGL], item_id }
struct CollectionModelEntry {
std::vector<std::unique_ptr<M2RendererGL>> renderers;
uint32_t item_id = 0;
};

// slot id to geoset group mapping for collection models
struct SlotGeosetMapping {
int group_index;
int char_geoset;
};

// --- CG alias + SLOT_TO_GEOSET_GROUPS ---

// JS: const SLOT_TO_GEOSET_GROUPS = { ... };
static const std::unordered_map<int, std::vector<SlotGeosetMapping>> SLOT_TO_GEOSET_GROUPS = {
{ 1,  {{ 0, CG::HELM }, { 1, CG::SKULL }} },
{ 5,  {{ 0, CG::SLEEVES }, { 1, CG::CHEST }, { 2, CG::TROUSERS }, { 3, CG::TORSO }, { 4, CG::ARM_UPPER }} },
{ 6,  {{ 0, CG::BELT }} },
{ 7,  {{ 0, CG::PANTS }, { 1, CG::KNEEPADS }, { 2, CG::TROUSERS }} },
{ 8,  {{ 0, CG::BOOTS }, { 1, CG::FEET }} },
{ 10, {{ 0, CG::GLOVES }, { 1, CG::HAND_ATTACHMENT }} },
{ 15, {{ 0, CG::CLOAK }} }
};

// JS: function get_slot_geoset_mapping(slot_id) { return SLOT_TO_GEOSET_GROUPS[slot_id] || null; }
static const std::vector<SlotGeosetMapping>* get_slot_geoset_mapping(int slot_id) {
auto it = SLOT_TO_GEOSET_GROUPS.find(slot_id);
return (it != SLOT_TO_GEOSET_GROUPS.end()) ? &it->second : nullptr;
}

// JS: const THUMBNAIL_PRESETS = { ... };
// format: [cam_x, cam_y, cam_z, tgt_x, tgt_y, tgt_z, rot]
struct ThumbnailPreset {
float cam_x, cam_y, cam_z;
float tgt_x, tgt_y, tgt_z;
float rot;
};

// race_id -> { gender_index -> preset }
static const std::unordered_map<int, std::unordered_map<int, ThumbnailPreset>> THUMBNAIL_PRESETS = {
{ 1,  {{ 0, { 0.008f, 1.787f, 0.813f, 0.008f, 1.714f, 0.444f, -1.711f }}, { 1, {-0.014f, 1.664f, 0.736f, -0.014f, 1.610f, 0.464f, -1.711f }}} },
{ 2,  {{ 0, {-0.008f, 1.797f, 1.293f, -0.008f, 1.630f, 0.460f, -1.666f }}, { 1, { 0.036f, 1.869f, 0.550f, 0.036f, 1.842f, 0.418f, -1.836f }}} },
{ 3,  {{ 0, { 0.032f, 1.297f, 0.757f, 0.032f, 1.253f, 0.536f, -1.641f }}, { 1, { 0.0f, 1.285f, 0.726f, 0.0f, 1.247f, 0.537f, -1.836f }}} },
{ 4,  {{ 0, { 0.025f, 2.152f, 0.852f, 0.025f, 2.057f, 0.375f, -1.581f }}, { 1, { 0.014f, 2.051f, 0.741f, 0.014f, 1.981f, 0.390f, -1.671f }}} },
{ 5,  {{ 0, { 0.008f, 1.570f, 0.767f, 0.008f, 1.513f, 0.484f, -1.646f }}, { 1, {-0.010f, 1.643f, 0.861f, -0.010f, 1.565f, 0.473f, -1.791f }}} },
{ 6,  {{ 0, {-0.048f, 2.186f, 1.806f, -0.048f, 1.906f, 0.405f, -1.686f }}, { 1, {-0.034f, 2.389f, 1.135f, -0.034f, 2.230f, 0.340f, -1.771f }}} },
{ 7,  {{ 0, { 0.037f, 0.897f, 0.893f, 0.037f, 0.842f, 0.618f, -1.761f }}, { 1, { 0.046f, 0.864f, 0.766f, 0.046f, 0.835f, 0.619f, -1.761f }}} },
{ 8,  {{ 0, {-0.014f, 1.964f, 1.222f, -0.014f, 1.805f, 0.425f, -1.771f }}, { 1, {-0.043f, 2.114f, 0.728f, -0.043f, 2.044f, 0.378f, -1.771f }}} },
{ 9,  {{ 0, { 0.007f, 1.081f, 0.767f, 0.007f, 1.044f, 0.578f, -1.391f }}, { 1, { 0.051f, 1.133f, 0.666f, 0.051f, 1.112f, 0.564f, -1.761f }}} },
{ 10, {{ 0, {-0.068f, 1.930f, 0.583f, -0.068f, 1.895f, 0.407f, -1.526f }}, { 1, { 0.015f, 1.749f, 0.543f, 0.015f, 1.728f, 0.441f, -1.496f }}} },
{ 11, {{ 0, {-0.047f, 2.227f, 1.112f, -0.047f, 2.079f, 0.371f, -1.611f }}, { 1, { 0.035f, 2.118f, 0.744f, 0.035f, 2.045f, 0.377f, -1.836f }}} },
{ 22, {{ 0, {-0.001f, 1.885f, 1.327f, -0.001f, 1.708f, 0.445f, -1.736f }}, { 1, {-0.005f, 2.161f, 0.904f, -0.005f, 2.055f, 0.375f, -1.736f }}} },
{ 23, {{ 0, { 0.025f, 1.824f, 0.734f, 0.025f, 1.764f, 0.434f, -1.726f }}, { 1, { 0.004f, 1.703f, 0.701f, 0.004f, 1.654f, 0.456f, -1.726f }}} },
{ 24, {{ 0, {-0.026f, 2.060f, 0.704f, -0.026f, 1.997f, 0.387f, -1.856f }}, { 1, { 0.015f, 1.903f, 0.984f, 0.015f, 1.792f, 0.428f, -1.731f }}} },
{ 27, {{ 0, { 0.034f, 2.124f, 0.833f, 0.034f, 2.034f, 0.380f, -1.671f }}, { 1, { 0.015f, 2.047f, 0.706f, 0.015f, 1.983f, 0.390f, -1.671f }}} },
{ 28, {{ 0, {-0.002f, 2.227f, 1.927f, -0.002f, 1.922f, 0.402f, -1.541f }}, { 1, {-0.026f, 2.295f, 1.345f, -0.026f, 2.099f, 0.367f, -1.541f }}} },
{ 29, {{ 0, {-0.074f, 1.833f, 0.784f, -0.074f, 1.763f, 0.434f, -1.666f }}, { 1, { 0.019f, 1.717f, 0.650f, 0.019f, 1.677f, 0.451f, -1.711f }}} },
{ 30, {{ 0, {-0.078f, 2.224f, 1.129f, -0.078f, 2.072f, 0.372f, -1.861f }}, { 1, { 0.050f, 2.121f, 0.690f, 0.050f, 2.058f, 0.375f, -1.726f }}} },
{ 31, {{ 0, {-0.030f, 2.455f, 1.124f, -0.030f, 2.296f, 0.327f, -1.536f }}, { 1, {-0.058f, 2.430f, 0.909f, -0.058f, 2.313f, 0.324f, -1.536f }}} },
{ 32, {{ 0, { 0.037f, 2.220f, 1.090f, 0.037f, 2.076f, 0.371f, -1.521f }}, { 1, { 0.013f, 2.148f, 0.782f, 0.013f, 2.067f, 0.373f, -1.726f }}} },
{ 34, {{ 0, { 0.021f, 1.312f, 0.681f, 0.021f, 1.282f, 0.530f, -1.491f }}, { 1, { 0.009f, 1.366f, 0.620f, 0.009f, 1.345f, 0.517f, -1.491f }}} },
{ 35, {{ 0, { 0.009f, 1.075f, 0.801f, 0.009f, 1.031f, 0.580f, -1.736f }}, { 1, { 0.015f, 1.039f, 0.847f, 0.015f, 0.987f, 0.589f, -1.711f }}} },
{ 36, {{ 0, {-0.022f, 1.742f, 1.356f, -0.022f, 1.565f, 0.473f, -1.701f }}, { 1, { 0.032f, 1.831f, 0.617f, 0.032f, 1.793f, 0.428f, -1.841f }}} },
{ 37, {{ 0, { 0.028f, 0.829f, 0.965f, 0.028f, 0.763f, 0.634f, -1.806f }}, { 1, { 0.051f, 0.823f, 0.890f, 0.051f, 0.771f, 0.632f, -1.831f }}} },
{ 52, {{ 0, {-0.018f, 2.501f, 0.863f, -0.018f, 2.390f, 0.308f, -1.326f }}, { 1, {-0.018f, 2.501f, 0.863f, -0.018f, 2.390f, 0.308f, -1.326f }}} },
{ 70, {{ 0, {-0.018f, 2.501f, 0.863f, -0.018f, 2.390f, 0.308f, -1.326f }}, { 1, {-0.018f, 2.501f, 0.863f, -0.018f, 2.390f, 0.308f, -1.326f }}} },
{ 75, {{ 0, {-0.043f, 1.839f, 0.716f, -0.043f, 1.782f, 0.430f, -1.666f }}, { 1, { 0.009f, 1.725f, 0.553f, 0.009f, 1.704f, 0.446f, -1.746f }}} },
{ 76, {{ 0, {-0.043f, 1.839f, 0.716f, -0.043f, 1.782f, 0.430f, -1.666f }}, { 1, { 0.009f, 1.725f, 0.553f, 0.009f, 1.704f, 0.446f, -1.746f }}} },
{ 84, {{ 0, { 0.058f, 1.434f, 1.022f, 0.058f, 1.333f, 0.520f, -1.746f }}, { 1, { 0.027f, 1.473f, 0.760f, 0.027f, 1.422f, 0.502f, -1.746f }}} },
{ 85, {{ 0, { 0.058f, 1.434f, 1.022f, 0.058f, 1.333f, 0.520f, -1.746f }}, { 1, { 0.027f, 1.473f, 0.760f, 0.027f, 1.422f, 0.502f, -1.746f }}} },
{ 86, {{ 0, { 0.003f, 2.222f, 0.639f, 0.003f, 2.165f, 0.353f, -1.571f }}, { 1, { 0.006f, 2.078f, 0.637f, 0.006f, 2.027f, 0.381f, -1.571f }}} }
};

// --- File-local state ---

// JS: const active_skins = new Map();
static std::map<std::string, nlohmann::json> active_skins;

// JS: let gl_context = null;
// TODO(conversion): GL context stored as json placeholder; real GL context managed by model-viewer-gl.
static nlohmann::json gl_context;

// JS: let active_renderer; let active_model;
static std::unique_ptr<M2RendererGL> active_renderer;
static uint32_t active_model = 0;

// JS: const skinned_model_renderers = new Map(); const skinned_model_meshes = new Set();
static std::map<uint32_t, std::unique_ptr<M2RendererGL>> skinned_model_renderers;
static std::unordered_set<uint32_t> skinned_model_meshes;

// JS: const chr_materials = new Map();
static std::map<uint32_t, std::unique_ptr<CharMaterialRenderer>> chr_materials;

// JS: const equipment_model_renderers = new Map();
static std::map<int, EquipmentModelEntry> equipment_model_renderers;

// JS: const collection_model_renderers = new Map();
static std::map<int, CollectionModelEntry> collection_model_renderers;

// JS: let current_char_component_texture_layout_id = 0;
static uint32_t current_char_component_texture_layout_id = 0;

// JS: let is_importing = false;
static bool is_importing = false;

// Animation state proxy for model_viewer_utils
static model_viewer_utils::ViewStateProxy view_state;
static std::unique_ptr<model_viewer_utils::AnimationMethods> anim_methods;

// Change-detection variables (Vue watch equivalents)
static std::vector<nlohmann::json> prev_race_selection;
static std::vector<nlohmann::json> prev_model_selection;
static std::vector<nlohmann::json> prev_option_selection;
static std::vector<nlohmann::json> prev_choice_selection;
static std::vector<nlohmann::json> prev_active_choices;
static nlohmann::json prev_equipped_items;
static nlohmann::json prev_guild_tabard_config;
static std::string prev_anim_selection;
static bool prev_include_base_clothing = true;
static bool prev_chr_import_region_inited = false;
static std::string prev_chr_import_region;
static bool prev_chr_import_classic_realms = false;

// Scrubber state for animation controls
static bool _was_paused_before_scrub = false;

// JS: const base_regions = ['us', 'eu', 'kr', 'tw'];
static const std::vector<std::string> base_regions = { "us", "eu", "kr", "tw" };

// JS: const tabard_options = [...]
struct TabardOptionDef {
std::string key;
std::string label;
std::string type; // "value" or "color"
std::string colors; // getter method name for color maps
};

static const std::vector<TabardOptionDef> tabard_options = {
{ "background", "Background", "color", "getBackgroundColors" },
{ "border_style", "Border", "value", "" },
{ "border_color", "Border Color", "color", "getBorderColors" },
{ "emblem_design", "Emblem", "value", "" },
{ "emblem_color", "Emblem Color", "color", "getEmblemColors" }
};

// --- Forward declarations of file-local functions ---
static void refresh_character_appearance();
static void update_geosets();
static void update_textures();
static void update_equipment_models();
static void load_character_model(uint32_t file_data_id);
static void dispose_skinned_models();
static void dispose_equipment_models();
static void dispose_collection_models();
static void clear_materials();
static void fit_camera();
static void update_chr_model_list();
static void update_model_selection();
static void update_customization_type();
static void update_customization_choice();
static void update_choice_for_option(uint32_t option_id, uint32_t choice_id);
static void randomize_customization();
static void import_character();
static void import_wmv_character();
static void import_wowhead_character();
static void apply_import_data(const nlohmann::json& data, const std::string& source);
static void load_saved_characters();
static void save_character(const std::string& name, const std::string& thumb_data);
static void delete_character(const nlohmann::json& character);
static void load_character(const nlohmann::json& character);
static std::string capture_character_thumbnail();
static nlohmann::json get_current_character_data();
static void export_json_character();
static void export_saved_character(const nlohmann::json& character);
static void import_json_character(bool save_to_my_characters);
static void update_chr_race_list();
static void export_char_model();
static void export_chr_texture();
static ImU32 int_to_imgui_color(uint32_t value);
static std::string int_to_css_color(uint32_t value);
static const db::caches::DBCharacterCustomization::ChoiceEntry* get_selected_choice(uint32_t option_id);
static std::string get_saved_characters_dir();
static void update_realm_list();

// --- Utility: race/gender ---

struct RaceGender {
uint32_t raceID;
int genderIndex;
};

/**
 * Get current character race ID and gender index from view state.
 * JS equivalent: function get_current_race_gender(core)
 */
static std::optional<RaceGender> get_current_race_gender() {
auto& view = *core::view;

if (view.chrCustRaceSelection.empty() || view.chrCustModelSelection.empty())
return std::nullopt;

const auto& race_selection = view.chrCustRaceSelection[0];
const auto& model_selection = view.chrCustModelSelection[0];

if (!race_selection.contains("id") || !model_selection.contains("id"))
return std::nullopt;

uint32_t race_id = race_selection["id"].get<uint32_t>();
uint32_t selected_model_id = model_selection["id"].get<uint32_t>();

const auto* models_for_race = db::caches::DBCharacterCustomization::get_race_models(race_id);
if (!models_for_race)
return std::nullopt;

// find the sex that matches the selected model ID
for (const auto& [sex, model_id] : *models_for_race) {
if (model_id == selected_model_id)
return RaceGender{ race_id, static_cast<int>(sex) };
}

return std::nullopt;
}

// --- reset_module_state ---

// JS: function reset_module_state()
static void reset_module_state() {
active_skins.clear();
skinned_model_renderers.clear();
skinned_model_meshes.clear();
clear_materials();
dispose_equipment_models();
dispose_collection_models();
current_char_component_texture_layout_id = 0;

if (active_renderer) {
active_renderer->dispose();
active_renderer.reset();
}
active_model = 0;
}

//region appearance

// JS: async function refresh_character_appearance(core)
static void refresh_character_appearance() {
if (!active_renderer || is_importing)
return;

logging::write("Refreshing character appearance...");

update_geosets();
update_textures();
update_equipment_models();

logging::write("Character appearance refresh complete");
}

/**
 * Updates all geoset visibility based on customization choices and equipped items.
 * Order: 1) Reset to model defaults, 2) Apply customization, 3) Apply equipment
 * JS: function update_geosets(core)
 */
static void update_geosets() {
if (!active_renderer)
return;

auto& view = *core::view;
auto& geosets = view.chrCustGeosets;
if (geosets.empty())
return;

// steps 1+2: reset to defaults and apply customization geosets
character_appearance::apply_customization_geosets(geosets, view.chrCustActiveChoices);

// step 3: apply equipment geosets (overrides customization where applicable)
const auto& equipped_items = view.chrEquippedItems;
if (equipped_items.is_object() && !equipped_items.empty()) {
// build int -> uint32_t map for DBItemGeosets
std::unordered_map<int, uint32_t> equipped_map;
for (auto& [slot_str, item_val] : equipped_items.items()) {
int slot_id = std::stoi(slot_str);
uint32_t item_id = item_val.get<uint32_t>();
equipped_map[slot_id] = item_id;
}

const auto equipment_geosets = db::caches::DBItemGeosets::calculateEquipmentGeosets(equipped_map);
const auto affected_groups = db::caches::DBItemGeosets::getAffectedCharGeosets(equipped_map);

for (int char_geoset : affected_groups) {
const int base = char_geoset * 100;
const int range_start = base + 1;
const int range_end = base + 99;

// hide all geosets in this group's range
for (auto& geoset : geosets) {
int gid = geoset.value("id", 0);
if (gid >= range_start && gid <= range_end)
geoset["checked"] = false;
}

// show the specific geoset for this group
auto git = equipment_geosets.find(char_geoset);
if (git != equipment_geosets.end()) {
int target_geoset_id = base + git->second;
for (auto& geoset : geosets) {
if (geoset.value("id", 0) == target_geoset_id)
geoset["checked"] = true;
}
}
}

// apply helmet hide geosets (hair, ears, etc.)
if (equipped_items.contains("1")) {
uint32_t head_item = equipped_items["1"].get<uint32_t>();
auto char_info = get_current_race_gender();
if (char_info) {
auto hide_groups = db::caches::DBItemGeosets::getHelmetHideGeosets(
head_item, char_info->raceID, char_info->genderIndex);
for (int cg : hide_groups) {
const int base = cg * 100;
const int range_start = base + 1;
const int range_end = base + 99;

for (auto& geoset : geosets) {
int gid = geoset.value("id", 0);
if (gid >= range_start && gid <= range_end)
geoset["checked"] = false;
}
}
}
}
}

// step 4: sync to renderer
active_renderer->updateGeosets();
}

/**
 * Updates all character textures based on baked NPC texture, customization, and equipment.
 * JS: async function update_textures(core)
 *
 * Steps:
 *   1-3: Reset materials, apply baked NPC texture, apply customization textures.
 *   4: Apply equipment textures (item textures, guild tabard composition).
 *   5: Upload all textures to GPU.
 *
 * Step 4 mirrors the tab_creatures equipment texture pattern. Both use:
 *   - section_by_type from get_texture_sections()
 *   - layers_by_section from get_model_texture_layer_map()
 *   - CharMaterialRenderer.setTextureTarget for each item texture
 *
 * CASC file loading is now wired. CharMaterialRenderer texture compositing
 * and GPU upload are a later rendering phase.
 */
static void update_textures() {
if (!active_renderer)
return;

auto& view = *core::view;

// steps 1-3: reset, apply baked NPC texture, apply customization textures
// JS: if (core.view.chrCustBakedNPCTexture) baked_npc_blp = core.view.chrCustBakedNPCTexture;
std::unique_ptr<casc::BLPImage> baked_npc_blp;
if (!view.chrCustBakedNPCTexture.is_null() && view.chrCustBakedNPCTexture.is_number_unsigned()) {
	uint32_t bake_fdid = view.chrCustBakedNPCTexture.get<uint32_t>();
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
active_renderer.get(),
view.chrCustActiveChoices,
current_char_component_texture_layout_id,
chr_materials,
baked_npc_blp.get()
);

// step 4: apply equipment textures
// Uses same pattern as tab_creatures: section_by_type from get_texture_sections(),
// layers_by_section from get_model_texture_layer_map(), then for each equipped item:
//   - getItemTextures() for texture components
//   - get_model_material() for target material info
//   - CharMaterialRenderer.setTextureTarget() to composite
// Guild tabard: getBackgroundFDID/getEmblemFDID/getBorderFDID for custom composition.
const auto& equipped_items = view.chrEquippedItems;
if (equipped_items.is_object() && !equipped_items.empty()) {
auto char_info = get_current_race_gender();
const auto* sections = db::caches::DBCharacterCustomization::get_texture_sections(current_char_component_texture_layout_id);
if (sections) {
// Build section_by_type map
std::unordered_map<int, const db::DataRecord*> section_by_type;
for (const auto& section : *sections) {
int section_type = static_cast<int>(std::get<int64_t>(section.at("SectionType")));
section_by_type[section_type] = &section;
}

// Build texture layer maps
const auto& texture_layer_map = db::caches::DBCharacterCustomization::get_model_texture_layer_map();
const db::DataRecord* base_layer = nullptr;
std::string layout_prefix = std::to_string(current_char_component_texture_layout_id) + "-";

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

// Apply item textures for each equipped slot
for (auto& [slot_str, item_val] : equipped_items.items()) {
int slot_id = std::stoi(slot_str);
uint32_t item_id = item_val.get<uint32_t>();

if (db::caches::DBGuildTabard::isGuildTabard(item_id))
continue;

int race_id = char_info ? static_cast<int>(char_info->raceID) : -1;
int gender_idx = char_info ? char_info->genderIndex : -1;
auto item_textures = db::caches::DBItemCharTextures::getItemTextures(item_id, race_id, gender_idx);
if (!item_textures)
continue;

for (const auto& texture : *item_textures) {
auto section_it = section_by_type.find(texture.section);
if (section_it == section_by_type.end())
continue;

auto layer_it = layers_by_section.find(texture.section);
if (layer_it == layers_by_section.end())
continue;

int layer_tex_type = static_cast<int>(std::get<int64_t>(layer_it->second->at("TextureType")));
const auto* chr_model_material = db::caches::DBCharacterCustomization::get_model_material(
current_char_component_texture_layout_id, layer_tex_type);
if (!chr_model_material)
continue;

int mat_texture_type = static_cast<int>(std::get<int64_t>(chr_model_material->at("TextureType")));

auto& chr_material = chr_materials[mat_texture_type];
if (!chr_material) {
int width = static_cast<int>(std::get<int64_t>(chr_model_material->at("Width")));
int height = static_cast<int>(std::get<int64_t>(chr_model_material->at("Height")));
chr_material = std::make_unique<CharMaterialRenderer>(mat_texture_type, width, height);
chr_material->init();
}

int slot_layer = wow::get_slot_layer(slot_id);
int chr_model_texture_target_id = (slot_layer * 100) + texture.section;

int section_x = static_cast<int>(std::get<int64_t>(section_it->second->at("X")));
int section_y = static_cast<int>(std::get<int64_t>(section_it->second->at("Y")));
int section_w = static_cast<int>(std::get<int64_t>(section_it->second->at("Width")));
int section_h = static_cast<int>(std::get<int64_t>(section_it->second->at("Height")));
int mat_width = static_cast<int>(std::get<int64_t>(chr_model_material->at("Width")));
int mat_height = static_cast<int>(std::get<int64_t>(chr_model_material->at("Height")));
int layer_blend_mode = static_cast<int>(std::get<int64_t>(layer_it->second->at("BlendMode")));

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

// Guild tabard texture composition
if (equipped_items.contains("19")) {
uint32_t tabard_item_id = equipped_items["19"].get<uint32_t>();
if (db::caches::DBGuildTabard::isGuildTabard(tabard_item_id)) {
int tier = db::caches::DBGuildTabard::getTabardTier(tabard_item_id);
const auto& config = view.chrGuildTabardConfig;
int TABARD_LAYER = wow::get_slot_layer(19);

const int components[] = { 3, 4 }; // TORSO_UPPER, TORSO_LOWER

struct TabardLayerInfo {
uint32_t fdid;
int section_type;
int target_id;
int blend_mode;
};

std::vector<TabardLayerInfo> tabard_layers;
for (int comp : components) {
uint32_t bg_fdid = db::caches::DBGuildTabard::getBackgroundFDID(tier, comp, config.background);
if (bg_fdid)
tabard_layers.push_back({ bg_fdid, comp, (TABARD_LAYER * 100) + comp, 1 });
uint32_t emblem_fdid = db::caches::DBGuildTabard::getEmblemFDID(comp, config.emblem_design, config.emblem_color);
if (emblem_fdid)
tabard_layers.push_back({ emblem_fdid, comp, (TABARD_LAYER * 100) + 10 + comp, 1 });
uint32_t border_fdid = db::caches::DBGuildTabard::getBorderFDID(tier, comp, config.border_style, config.border_color);
if (border_fdid)
tabard_layers.push_back({ border_fdid, comp, (TABARD_LAYER * 100) + 20 + comp, 1 });
}

for (const auto& tl : tabard_layers) {
auto section_it = section_by_type.find(tl.section_type);
if (section_it == section_by_type.end())
continue;
auto layer_it = layers_by_section.find(tl.section_type);
if (layer_it == layers_by_section.end())
continue;

int layer_tex_type = static_cast<int>(std::get<int64_t>(layer_it->second->at("TextureType")));
const auto* chr_model_material = db::caches::DBCharacterCustomization::get_model_material(
current_char_component_texture_layout_id, layer_tex_type);
if (!chr_model_material)
continue;

int mat_texture_type = static_cast<int>(std::get<int64_t>(chr_model_material->at("TextureType")));
auto& chr_material = chr_materials[mat_texture_type];
if (!chr_material) {
int width = static_cast<int>(std::get<int64_t>(chr_model_material->at("Width")));
int height = static_cast<int>(std::get<int64_t>(chr_model_material->at("Height")));
chr_material = std::make_unique<CharMaterialRenderer>(mat_texture_type, width, height);
chr_material->init();
}

int section_x = static_cast<int>(std::get<int64_t>(section_it->second->at("X")));
int section_y = static_cast<int>(std::get<int64_t>(section_it->second->at("Y")));
int section_w = static_cast<int>(std::get<int64_t>(section_it->second->at("Width")));
int section_h = static_cast<int>(std::get<int64_t>(section_it->second->at("Height")));
int mat_width = static_cast<int>(std::get<int64_t>(chr_model_material->at("Width")));
int mat_height = static_cast<int>(std::get<int64_t>(chr_model_material->at("Height")));

chr_material->setTextureTarget(
tl.target_id,
tl.fdid,
section_x, section_y, section_w, section_h,
mat_texture_type, mat_width, mat_height,
tl.blend_mode,
true
);
}
}
}
}
}

// step 5: upload all textures to GPU
character_appearance::upload_textures_to_gpu(active_renderer.get(), chr_materials);
}

/**
 * Updates equipment model renderers based on equipped items.
 * JS: async function update_equipment_models(core)
 */
static void update_equipment_models() {
if (gl_context.is_null())
return;

auto& view = *core::view;
const auto& equipped_items = view.chrEquippedItems;

std::unordered_set<int> current_slots;
if (equipped_items.is_object()) {
for (auto& [slot_str, _] : equipped_items.items())
current_slots.insert(std::stoi(slot_str));
}

// dispose attachment models for slots no longer equipped
for (auto it = equipment_model_renderers.begin(); it != equipment_model_renderers.end(); ) {
if (current_slots.find(it->first) == current_slots.end()) {
for (auto& ri : it->second.renderers)
ri.renderer->dispose();

logging::write(std::format("Disposed equipment models for slot {}", it->first));
it = equipment_model_renderers.erase(it);
} else {
++it;
}
}

// dispose collection models for slots no longer equipped
for (auto it = collection_model_renderers.begin(); it != collection_model_renderers.end(); ) {
if (current_slots.find(it->first) == current_slots.end()) {
for (auto& r : it->second.renderers)
r->dispose();

logging::write(std::format("Disposed collection models for slot {}", it->first));
it = collection_model_renderers.erase(it);
} else {
++it;
}
}

// load models for equipped items
if (!equipped_items.is_object())
return;

for (auto& [slot_str, item_val] : equipped_items.items()) {
int slot_id = std::stoi(slot_str);
uint32_t item_id = item_val.get<uint32_t>();

// check if we already have renderers for this slot with same item
auto existing_eq_it = equipment_model_renderers.find(slot_id);
auto existing_col_it = collection_model_renderers.find(slot_id);
bool eq_ok = (existing_eq_it != equipment_model_renderers.end() && existing_eq_it->second.item_id == item_id);
bool col_ok = (existing_col_it == collection_model_renderers.end()) ||
              (existing_col_it != collection_model_renderers.end() && existing_col_it->second.item_id == item_id);
if (eq_ok && col_ok)
continue;

// dispose old renderers if item changed
if (existing_eq_it != equipment_model_renderers.end()) {
for (auto& ri : existing_eq_it->second.renderers)
ri.renderer->dispose();
equipment_model_renderers.erase(existing_eq_it);
}

if (existing_col_it != collection_model_renderers.end()) {
for (auto& r : existing_col_it->second.renderers)
r->dispose();
collection_model_renderers.erase(existing_col_it);
}

// get race/gender for model filtering
auto char_info = get_current_race_gender();

// get display data for this item
int race_id = char_info ? static_cast<int>(char_info->raceID) : -1;
int gender_idx = char_info ? char_info->genderIndex : -1;
auto display = db::caches::DBItemModels::getItemDisplay(item_id, race_id, gender_idx);
if (!display || display->models.empty())
continue;

// get attachment IDs for this slot
auto attachment_ids_span = wow::get_attachment_ids_for_slot(slot_id);
std::vector<int> attachment_ids;
if (attachment_ids_span)
attachment_ids.assign(attachment_ids_span->begin(), attachment_ids_span->end());

// bows are held in the left hand despite being main-hand items
if (slot_id == 16 && db::caches::DBItems::isItemBow(item_id))
attachment_ids = { wow::ATTACHMENT_ID::HAND_LEFT };

// split models into attachment vs collection
size_t attachment_model_count = (std::min)(display->models.size(), attachment_ids.size());
size_t collection_start_index = attachment_model_count;

// load attachment models
if (attachment_model_count > 0) {
EquipmentModelEntry entry;
entry.item_id = item_id;

for (size_t i = 0; i < attachment_model_count; i++) {
uint32_t file_data_id = display->models[i];
int attachment_id = attachment_ids[i];

try {
// JS: const file = await core.view.casc.getFile(file_data_id);
BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);
// TODO(conversion): M2RendererGL creation will be wired when renderer integration is complete.
// const renderer = new M2RendererGL(file, gl_context, false, false);
// await renderer.load();
logging::write(std::format("Loaded attachment model {} for slot {} attachment {} (item {})",
file_data_id, slot_id, attachment_id, item_id));
} catch (const std::exception& e) {
logging::write(std::format("Failed to load attachment model {}: {}", file_data_id, e.what()));
}
}

if (!entry.renderers.empty())
equipment_model_renderers[slot_id] = std::move(entry);
}

// load collection models
if (display->models.size() > collection_start_index) {
CollectionModelEntry entry;
entry.item_id = item_id;

for (size_t i = collection_start_index; i < display->models.size(); i++) {
uint32_t file_data_id = display->models[i];

try {
// JS: const file = await core.view.casc.getFile(file_data_id);
BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);
logging::write(std::format("Loaded collection model {} for slot {} (item {})",
file_data_id, slot_id, item_id));
} catch (const std::exception& e) {
logging::write(std::format("Failed to load collection model {}: {}", file_data_id, e.what()));
}
}

if (!entry.renderers.empty())
collection_model_renderers[slot_id] = std::move(entry);
}
}
}

//endregion

//region models

// JS: async function load_character_model(core, file_data_id)
static void load_character_model(uint32_t file_data_id) {
if (file_data_id == 0 || active_model == file_data_id)
return;

auto& view = *core::view;
view.chrModelLoading = true;
logging::write(std::format("Loading character model {}", file_data_id));

view.modelViewerSkins.clear();
view.modelViewerSkinsSelection.clear();

view.chrModelViewerAnims.clear();
view.chrModelViewerAnimSelection = nlohmann::json();

try {
if (active_renderer) {
active_renderer->dispose();
active_renderer.reset();
active_model = 0;
}

active_skins.clear();
dispose_skinned_models();
dispose_equipment_models();
dispose_collection_models();

// JS: const file = await core.view.casc.getFile(file_data_id);
BufferWrapper file = core::view->casc->getVirtualFileByID(file_data_id);
// TODO(conversion): M2RendererGL creation will be wired when renderer integration is complete.
// active_renderer = std::make_unique<M2RendererGL>(file, gl_context, true, false);
// active_renderer->geosetKey = "chrCustGeosets";
// active_renderer->load();
// fit_camera();

active_model = file_data_id;

// populate animation list
// TODO(conversion): Animation list population requires loaded M2 data.
view.chrModelViewerAnims = {
nlohmann::json{{ "id", "none" }, { "label", "No Animation" }, { "m2Index", -1 }}
};
view.chrModelViewerAnimSelection = "none";

// JS: const has_content = active_renderer.draw_calls?.length > 0;
// if (!has_content) setToast...

// refresh appearance after model is fully loaded
refresh_character_appearance();

} catch (const std::exception& e) {
core::setToast("error", std::format("Unable to load model {}", file_data_id),
{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
logging::write(std::format("Failed to load character model: {}", e.what()));
}

view.chrModelLoading = false;
}

// JS: function dispose_skinned_models()
static void dispose_skinned_models() {
for (auto& [_, renderer] : skinned_model_renderers)
renderer->dispose();

skinned_model_renderers.clear();
skinned_model_meshes.clear();
}

// JS: function dispose_equipment_models()
static void dispose_equipment_models() {
for (auto& [_, entry] : equipment_model_renderers) {
for (auto& ri : entry.renderers)
ri.renderer->dispose();
}
equipment_model_renderers.clear();
}

// JS: function dispose_collection_models()
static void dispose_collection_models() {
for (auto& [_, entry] : collection_model_renderers) {
for (auto& r : entry.renderers)
r->dispose();
}
collection_model_renderers.clear();
}

// JS: function clear_materials()
static void clear_materials() {
character_appearance::dispose_materials(chr_materials);
}

// JS: function fit_camera(core)
static void fit_camera() {
auto& view = *core::view;
if (view.chrModelViewerContext.is_object() && view.chrModelViewerContext.contains("fitCamera")) {
// TODO(conversion): fitCamera callback will be wired via model-viewer-gl integration.
}
}

//endregion

//region character state

// JS: function update_chr_model_list(core)
static void update_chr_model_list() {
auto& view = *core::view;

if (view.chrCustRaceSelection.empty())
return;

const auto& race_selection = view.chrCustRaceSelection[0];
if (!race_selection.contains("id"))
return;

uint32_t race_id = race_selection["id"].get<uint32_t>();
const auto* models_for_race = db::caches::DBCharacterCustomization::get_race_models(race_id);
if (!models_for_race)
return;

int selection_index = 0;

if (!view.chrCustModelSelection.empty()) {
uint32_t selected_id = view.chrCustModelSelection[0].value("id", 0u);
for (size_t i = 0; i < view.chrCustModels.size(); i++) {
if (view.chrCustModels[i].value("id", 0u) == selected_id) {
selection_index = static_cast<int>(i);
break;
}
}
}

view.chrCustModels.clear();

std::vector<uint32_t> listed_model_ids;

for (const auto& [chr_sex, chr_model_id] : *models_for_race) {
nlohmann::json new_model = {
{ "id", chr_model_id },
{ "label", std::format("Type {}", chr_sex + 1) }
};
view.chrCustModels.push_back(new_model);
listed_model_ids.push_back(chr_model_id);
}

if (view.chrImportChrModelID != 0) {
auto it = std::find(listed_model_ids.begin(), listed_model_ids.end(),
static_cast<uint32_t>(view.chrImportChrModelID));
if (it != listed_model_ids.end())
selection_index = static_cast<int>(std::distance(listed_model_ids.begin(), it));
view.chrImportChrModelID = 0;
} else {
if (static_cast<int>(view.chrCustModels.size()) <= selection_index || selection_index < 0)
selection_index = 0;
}

if (!view.chrCustModels.empty())
view.chrCustModelSelection = { view.chrCustModels[selection_index] };
}

/**
 * Handles body type selection change - loads new model and sets up customization options.
 * JS: async function update_model_selection(core)
 */
static void update_model_selection() {
auto& state = *core::view;

if (state.chrCustModelSelection.empty())
return;

const auto& selected = state.chrCustModelSelection[0];
if (!selected.contains("id"))
return;

uint32_t selected_id = selected["id"].get<uint32_t>();
logging::write(std::format("Model selection changed to ID {}", selected_id));

const auto* available_options = db::caches::DBCharacterCustomization::get_options_for_model(selected_id);
if (!available_options)
return;

// update texture layout for the new model
current_char_component_texture_layout_id = db::caches::DBCharacterCustomization::get_texture_layout_id(selected_id);

// clear materials for new model
clear_materials();

// update customization options list
state.chrCustOptions.clear();
state.chrCustOptionSelection.clear();
state.chrCustActiveChoices.clear();

const auto& option_to_choices = db::caches::DBCharacterCustomization::get_option_to_choices_map();
const auto& default_option_ids = db::caches::DBCharacterCustomization::get_default_options();

// use imported choices if available and we're loading the target model, otherwise use defaults
if (!state.chrImportChoices.empty() && static_cast<uint32_t>(state.chrImportTargetModelID) == selected_id) {
state.chrCustActiveChoices = state.chrImportChoices;
state.chrImportChoices.clear();
state.chrImportTargetModelID = 0;
} else {
for (const auto& option : *available_options) {
auto choices_it = option_to_choices.find(option.id);
bool is_default = std::find(default_option_ids.begin(), default_option_ids.end(), option.id) != default_option_ids.end();
if (is_default && choices_it != option_to_choices.end() && !choices_it->second.empty()) {
state.chrCustActiveChoices.push_back(nlohmann::json{
{ "optionID", option.id },
{ "choiceID", choices_it->second[0].id }
});
}
}
}

for (const auto& option : *available_options) {
state.chrCustOptions.push_back(nlohmann::json{
{ "id", option.id },
{ "label", option.label },
{ "is_color_swatch", option.is_color_swatch }
});
}

if (!state.chrCustOptions.empty())
state.chrCustOptionSelection = { state.chrCustOptions[0] };

// load the model
uint32_t file_data_id = db::caches::DBCharacterCustomization::get_model_file_data_id(selected_id);
load_character_model(file_data_id);
}

// JS: function update_customization_type(core)
static void update_customization_type() {
auto& state = *core::view;

if (state.chrCustOptionSelection.empty())
return;

const auto& selected = state.chrCustOptionSelection[0];
uint32_t option_id = selected.value("id", 0u);

const auto* available_choices = db::caches::DBCharacterCustomization::get_choices_for_option(option_id);
if (!available_choices)
return;

state.chrCustChoices.clear();
state.chrCustChoiceSelection.clear();

for (const auto& choice : *available_choices) {
state.chrCustChoices.push_back(nlohmann::json{
{ "id", choice.id },
{ "label", choice.label },
{ "swatch_color_0", choice.swatch_color_0 },
{ "swatch_color_1", choice.swatch_color_1 }
});
}
}

// JS: function update_customization_choice(core)
static void update_customization_choice() {
auto& state = *core::view;

if (state.chrCustChoiceSelection.empty())
return;

const auto& selected = state.chrCustChoiceSelection[0];
uint32_t choice_id = selected.value("id", 0u);

if (state.chrCustOptionSelection.empty())
return;

uint32_t option_id = state.chrCustOptionSelection[0].value("id", 0u);

// find existing or add new
bool found = false;
for (auto& ac : state.chrCustActiveChoices) {
if (ac.value("optionID", 0u) == option_id) {
ac["choiceID"] = choice_id;
found = true;
break;
}
}

if (!found) {
state.chrCustActiveChoices.push_back(nlohmann::json{
{ "optionID", option_id },
{ "choiceID", choice_id }
});
}
}

// JS: function update_choice_for_option(core, option_id, choice_id)
static void update_choice_for_option(uint32_t option_id, uint32_t choice_id) {
auto& state = *core::view;

for (auto& ac : state.chrCustActiveChoices) {
if (ac.value("optionID", 0u) == option_id) {
ac["choiceID"] = choice_id;
return;
}
}

state.chrCustActiveChoices.push_back(nlohmann::json{
{ "optionID", option_id },
{ "choiceID", choice_id }
});
}

// JS: function randomize_customization(core)
static void randomize_customization() {
auto& state = *core::view;

static std::mt19937 rng(std::random_device{}());

for (const auto& option : state.chrCustOptions) {
uint32_t option_id = option.value("id", 0u);
const auto* choices = db::caches::DBCharacterCustomization::get_choices_for_option(option_id);
if (choices && !choices->empty()) {
std::uniform_int_distribution<size_t> dist(0, choices->size() - 1);
uint32_t random_choice_id = (*choices)[dist(rng)].id;
update_choice_for_option(option_id, random_choice_id);
}
}
}

//endregion

//region import

// JS: async function import_character(core)
static void import_character() {
auto& view = *core::view;
view.characterImportMode = "none";
view.chrModelLoading = true;

std::string character_name = view.chrImportChrName;
const auto& selected_realm = view.chrImportSelectedRealm;
std::string base_region = view.chrImportSelectedRegion;
std::string effective_region = view.chrImportClassicRealms ? "classic-" + base_region : base_region;

if (selected_realm.is_null()) {
core::setToast("error", "Please enter a valid realm.", {}, 3000);
view.chrModelLoading = false;
return;
}

std::string realm_label = selected_realm.value("label", "");
std::string realm_value = selected_realm.value("value", "");
std::string character_label = std::format("{} ({}-{})", character_name, effective_region, realm_label);

std::string armory_url = view.config.value("armoryURL", "");

// URL-encode helper (JS: encodeURIComponent)
auto url_encode = [](const std::string& s) -> std::string {
	std::string encoded;
	encoded.reserve(s.size() * 3);
	for (unsigned char c : s) {
		if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			encoded += static_cast<char>(c);
		else
			encoded += std::format("%{:02X}", c);
	}
	return encoded;
};

// JS: const url = util.format(core.view.config.armoryURL, encodeURIComponent(region), ...)
// URL template uses %s placeholders for region, realm, character.
std::string encoded_region = url_encode(effective_region);
std::string encoded_realm = url_encode(realm_value);
std::string encoded_name = url_encode(character_name);
// Simple %s substitution matching util.format behavior.
std::string url = armory_url;
auto replace_first = [](std::string& str, const std::string& from, const std::string& to) {
	size_t pos = str.find(from);
	if (pos != std::string::npos)
		str.replace(pos, from.size(), to);
};
replace_first(url, "%s", encoded_region);
replace_first(url, "%s", encoded_realm);
replace_first(url, "%s", encoded_name);

logging::write(std::format("Retrieving character data for {} from {}", character_label, url));

try {
// JS: const res = await generics.get(url);
nlohmann::json res = generics::getJSON(url);
apply_import_data(res, "bnet");
} catch (const std::exception& e) {
logging::write(std::format("Failed to import character: {}", e.what()));

std::string err_msg = e.what();
if (err_msg.find("404") != std::string::npos)
core::setToast("error", "Could not find character " + character_label, {}, -1);
else
core::setToast("error", "Failed to import character " + character_label, {}, -1);
}

view.chrModelLoading = false;
}

// Removed: import_wmv_character() — wmv module deleted
// JS: async function import_wmv_character(core) { return; }
static void import_wmv_character() {
return;
}

// Removed: import_wowhead_character() — wowhead module deleted
// JS: async function import_wowhead_character(core) { return; }
static void import_wowhead_character() {
return;
}

/**
 * Unified import handler - parses import data and applies it.
 * JS: async function apply_import_data(core, data, source)
 */
static void apply_import_data(const nlohmann::json& data, const std::string& source) {
auto& view = *core::view;
uint32_t race_id = 0;
int gender_index = 0;
std::vector<nlohmann::json> customizations;
nlohmann::json equipment = nlohmann::json::object();

if (source == "bnet") {
race_id = data["playable_race"]["id"].get<uint32_t>();

// pandaren with faction -> use neutral
if (race_id == 25 || race_id == 26)
race_id = 24;

// dracthyr horde -> use alliance
if (race_id == 70)
race_id = 52;

// worgen/dracthyr visage
if (race_id == 22 && view.chrImportLoadVisage)
race_id = 23;
if (race_id == 52 && view.chrImportLoadVisage)
race_id = 75;

gender_index = (data["gender"]["type"].get<std::string>() == "MALE") ? 0 : 1;

auto chr_model_id_opt = db::caches::DBCharacterCustomization::get_chr_model_id(race_id, gender_index);
if (!chr_model_id_opt)
return;

uint32_t chr_model_id = *chr_model_id_opt;
const auto* available_options = db::caches::DBCharacterCustomization::get_options_for_model(chr_model_id);
if (!available_options)
return;

std::unordered_set<uint32_t> available_option_ids;
for (const auto& opt : *available_options)
available_option_ids.insert(opt.id);

for (auto& [_, customization_entry] : data["customizations"].items()) {
uint32_t opt_id = customization_entry["option"]["id"].get<uint32_t>();
if (available_option_ids.count(opt_id)) {
customizations.push_back(nlohmann::json{
{ "optionID", opt_id },
{ "choiceID", customization_entry["choice"]["id"].get<uint32_t>() }
});
}
}

if (data.contains("items") && data["items"].is_array()) {
for (const auto& item : data["items"]) {
int slot_id = item["internal_slot_id"].get<int>() + 1;
equipment[std::to_string(slot_id)] = item["id"].get<uint32_t>();
}
}

} else if (source == "wmv") {
race_id = data["race"].get<uint32_t>();
gender_index = data["gender"].get<int>();

auto chr_model_id_opt = db::caches::DBCharacterCustomization::get_chr_model_id(race_id, gender_index);
if (!chr_model_id_opt)
return;

uint32_t chr_model_id = *chr_model_id_opt;
const auto* available_options = db::caches::DBCharacterCustomization::get_options_for_model(chr_model_id);
if (!available_options)
return;

std::unordered_set<uint32_t> available_option_ids;
for (const auto& opt : *available_options)
available_option_ids.insert(opt.id);

if (data.contains("legacy_values")) {
// legacy WMV format
const auto& legacy = data["legacy_values"];
std::unordered_map<std::string, int> option_map = {
{ "skin", legacy.value("skin_color", 0) },
{ "face", legacy.value("face_type", 0) },
{ "hair color", legacy.value("hair_color", 0) },
{ "hair style", legacy.value("hair_style", 0) },
{ "facial", legacy.value("facial_hair", 0) }
};

for (const auto& option : *available_options) {
std::string label_lower = option.label;
std::transform(label_lower.begin(), label_lower.end(), label_lower.begin(), ::tolower);

for (const auto& [key, value] : option_map) {
if (label_lower.find(key) != std::string::npos) {
const auto* choices = db::caches::DBCharacterCustomization::get_choices_for_option(option.id);
if (choices && static_cast<size_t>(value) < choices->size()) {
customizations.push_back(nlohmann::json{
{ "optionID", option.id },
{ "choiceID", (*choices)[value].id }
});
break;
}
}
}
}
} else {
// modern WMV format
for (const auto& customization : data["customizations"]) {
uint32_t opt_id = customization["option_id"].get<uint32_t>();
if (available_option_ids.count(opt_id)) {
customizations.push_back(nlohmann::json{
{ "optionID", opt_id },
{ "choiceID", customization["choice_id"].get<uint32_t>() }
});
}
}
}

equipment = data.value("equipment", nlohmann::json::object());

} else if (source == "wowhead") {
race_id = data["race"].get<uint32_t>();
gender_index = data["gender"].get<int>();

auto chr_model_id_opt = db::caches::DBCharacterCustomization::get_chr_model_id(race_id, gender_index);
if (!chr_model_id_opt)
return;

uint32_t chr_model_id = *chr_model_id_opt;
const auto* available_options = db::caches::DBCharacterCustomization::get_options_for_model(chr_model_id);
if (!available_options)
return;

std::unordered_set<uint32_t> available_option_ids;
for (const auto& opt : *available_options)
available_option_ids.insert(opt.id);

// JS: for (const choice_id of data.customizations) { ... db2.ChrCustomizationChoice.getRow(choice_id) ... }
auto& chr_choice_table = casc::db2::getTable("ChrCustomizationChoice");
if (!chr_choice_table.isLoaded)
	chr_choice_table.parse();

if (data.contains("customizations") && data["customizations"].is_array()) {
	for (const auto& choice_id_json : data["customizations"]) {
		uint32_t choice_id = choice_id_json.get<uint32_t>();
		auto choice_row_opt = chr_choice_table.getRow(choice_id);
		if (!choice_row_opt.has_value())
			continue;

		const auto& choice_row = *choice_row_opt;
		uint32_t option_id = 0;
		auto opt_it = choice_row.find("ChrCustomizationOptionID");
		if (opt_it != choice_row.end()) {
			if (auto* p = std::get_if<int64_t>(&opt_it->second))
				option_id = static_cast<uint32_t>(*p);
			else if (auto* p = std::get_if<uint64_t>(&opt_it->second))
				option_id = static_cast<uint32_t>(*p);
		}

		if (available_option_ids.count(option_id)) {
			customizations.push_back(nlohmann::json{
				{ "optionID", option_id },
				{ "choiceID", choice_id }
			});
		}
	}
}

equipment = data.value("equipment", nlohmann::json::object());
}

is_importing = true;

try {
view.chrEquippedItems = equipment;

view.chrImportChoices.clear();
view.chrImportChoices = customizations;

auto chr_model_id_opt = db::caches::DBCharacterCustomization::get_chr_model_id(race_id, gender_index);
if (chr_model_id_opt) {
uint32_t chr_model_id = *chr_model_id_opt;
view.chrImportChrModelID = chr_model_id;
view.chrImportTargetModelID = chr_model_id;

// find the race entry in chrCustRaces
for (const auto& race : view.chrCustRaces) {
if (race.value("id", 0u) == race_id) {
view.chrCustRaceSelection = { race };
break;
}
}

update_model_selection();
}
} catch (...) {
// ensure we always clear importing flag
}

is_importing = false;
}

//endregion

//region saved characters

// JS: function get_default_characters_dir()
std::string get_default_characters_dir() {
namespace fs = std::filesystem;
const char* home = nullptr;
#ifdef _WIN32
home = std::getenv("USERPROFILE");
#else
home = std::getenv("HOME");
#endif
if (!home)
home = ".";
return (fs::path(home) / "wow.export" / "My Characters").string();
}

// JS: function get_saved_characters_dir(core)
static std::string get_saved_characters_dir() {
auto& view = *core::view;
std::string custom_path = view.config.value("characterExportPath", "");
if (!custom_path.empty()) {
// trim
auto start = custom_path.find_first_not_of(" \t");
auto end = custom_path.find_last_not_of(" \t");
if (start != std::string::npos)
return custom_path.substr(start, end - start + 1);
}
return get_default_characters_dir();
}

// JS: function generate_character_id()
static std::string generate_character_id() {
static std::mt19937 rng(std::random_device{}());
std::uniform_int_distribution<int> dist(10000, 99999);
return std::to_string(dist(rng));
}

// JS: async function load_saved_characters(core)
static void load_saved_characters() {
namespace fs = std::filesystem;
auto& view = *core::view;
std::string dir = get_saved_characters_dir();
view.chrSavedCharacters.clear();

std::error_code ec;
if (!fs::exists(dir, ec))
return;

std::vector<nlohmann::json> characters;

for (const auto& entry : fs::directory_iterator(dir, ec)) {
if (!entry.is_regular_file())
continue;

std::string file_name = entry.path().filename().string();
if (file_name.size() < 5 || file_name.substr(file_name.size() - 5) != ".json")
continue;

// match pattern: name-XXXXX.json
std::string base = file_name.substr(0, file_name.size() - 5);
if (base.size() < 7) // at least "x-12345"
continue;

size_t dash_pos = base.rfind('-');
if (dash_pos == std::string::npos || dash_pos + 6 != base.size())
continue;

std::string id_part = base.substr(dash_pos + 1);
bool all_digits = !id_part.empty() && std::all_of(id_part.begin(), id_part.end(), ::isdigit);
if (!all_digits || id_part.size() != 5)
continue;

std::string name = base.substr(0, dash_pos);
std::string id = id_part;
std::string thumb_path = (fs::path(dir) / (name + "-" + id + ".png")).string();

nlohmann::json thumb_data = nullptr;
if (fs::exists(thumb_path, ec)) {
try {
std::ifstream thumb_file(thumb_path, std::ios::binary);
std::vector<uint8_t> thumb_buffer((std::istreambuf_iterator<char>(thumb_file)),
                                   std::istreambuf_iterator<char>());
// JS: thumb_data = 'data:image/png;base64,' + thumb_buffer.toString('base64');
BufferWrapper thumb_bw(thumb_buffer);
thumb_data = "data:image/png;base64," + thumb_bw.toBase64();
} catch (...) {
// no thumbnail
}
}

characters.push_back(nlohmann::json{
{ "name", name },
{ "id", id },
{ "thumb", thumb_data },
{ "file_name", file_name }
});
}

view.chrSavedCharacters = characters;
}

// JS: async function save_character(core, name, thumb_data)
static void save_character(const std::string& name, const std::string& thumb_data) {
namespace fs = std::filesystem;
auto& view = *core::view;
std::string dir = get_saved_characters_dir();
generics::createDirectory(dir);

// generate unique id
std::string id = generate_character_id();
std::unordered_set<std::string> existing_ids;
for (const auto& c : view.chrSavedCharacters)
existing_ids.insert(c.value("id", ""));
while (existing_ids.count(id))
id = generate_character_id();

// gather character data
nlohmann::json data = get_current_character_data();

std::string json_path = (fs::path(dir) / (name + "-" + id + ".json")).string();
try {
std::ofstream out(json_path);
out << data.dump(1, '\t');
} catch (const std::exception& e) {
logging::write(std::format("Failed to save character: {}", e.what()));
return;
}

// save thumbnail if provided
if (!thumb_data.empty()) {
// JS: const base64 = data.thumb.split(',')[1]; Buffer.from(base64, 'base64')
std::string thumb_str = thumb_data;
size_t comma_pos = thumb_str.find(',');
if (comma_pos != std::string::npos)
thumb_str = thumb_str.substr(comma_pos + 1);

std::string thumb_path = (fs::path(dir) / (name + "-" + id + ".png")).string();
std::vector<uint8_t> thumb_bytes = BufferWrapper::fromBase64(thumb_str).raw();
std::ofstream thumb_out(thumb_path, std::ios::binary);
thumb_out.write(reinterpret_cast<const char*>(thumb_bytes.data()), static_cast<std::streamsize>(thumb_bytes.size()));
}

load_saved_characters();
core::setToast("success", std::format("Character \"{}\" saved.", name), {}, 3000);
}

// JS: async function delete_character(core, character)
static void delete_character(const nlohmann::json& character) {
namespace fs = std::filesystem;
auto& view = *core::view;
std::string dir = get_saved_characters_dir();
std::string json_path = (fs::path(dir) / character.value("file_name", "")).string();
std::string name = character.value("name", "");
std::string id = character.value("id", "");
std::string thumb_path = (fs::path(dir) / (name + "-" + id + ".png")).string();

std::error_code ec;
fs::remove(json_path, ec);
if (ec)
logging::write(std::format("failed to delete character json: {}", ec.message()));

fs::remove(thumb_path, ec); // thumbnail may not exist

// remove from list
auto& chars = view.chrSavedCharacters;
chars.erase(std::remove_if(chars.begin(), chars.end(),
[&id](const nlohmann::json& c) { return c.value("id", "") == id; }),
chars.end());

core::setToast("success", std::format("Character \"{}\" deleted.", name), {}, 3000);
}

// JS: async function load_character(core, character)
static void load_character(const nlohmann::json& character) {
namespace fs = std::filesystem;
auto& view = *core::view;
std::string dir = get_saved_characters_dir();
std::string json_path = (fs::path(dir) / character.value("file_name", "")).string();

try {
std::ifstream in(json_path);
if (!in.is_open())
throw std::runtime_error("Could not open file");

nlohmann::json data = nlohmann::json::parse(in);

view.chrModelLoading = true;
view.chrSavedCharactersScreen = false;

// apply equipment
view.chrEquippedItems = data.value("equipment", nlohmann::json::object());

// apply guild tabard config
if (data.contains("guild_tabard")) {
const auto& gt = data["guild_tabard"];
view.chrGuildTabardConfig.background = gt.value("background", 0);
view.chrGuildTabardConfig.border_style = gt.value("border_style", 0);
view.chrGuildTabardConfig.border_color = gt.value("border_color", 0);
view.chrGuildTabardConfig.emblem_design = gt.value("emblem_design", 0);
view.chrGuildTabardConfig.emblem_color = gt.value("emblem_color", 0);
}

// apply customization
view.chrImportChoices.clear();
if (data.contains("choices") && data["choices"].is_array())
view.chrImportChoices = data["choices"].get<std::vector<nlohmann::json>>();

uint32_t model_id = data.value("model_id", 0u);
view.chrImportChrModelID = model_id;
view.chrImportTargetModelID = model_id;

// apply race selection
uint32_t race_id = data.value("race_id", 0u);
for (const auto& race : view.chrCustRaces) {
if (race.value("id", 0u) == race_id) {
view.chrCustRaceSelection = { race };
break;
}
}

view.chrModelLoading = false;
} catch (const std::exception& e) {
logging::write(std::format("failed to load character: {}", e.what()));
core::setToast("error", std::format("Failed to load character: {}", e.what()), {}, -1);
}
}

// JS: async function capture_character_thumbnail(core)
static std::string capture_character_thumbnail() {
auto& view = *core::view;

if (!active_renderer)
return "";

// Get race/gender preset for camera positioning.
auto race_gender = get_current_race_gender();
const ThumbnailPreset* preset = nullptr;
if (race_gender) {
auto race_it = THUMBNAIL_PRESETS.find(static_cast<int>(race_gender->raceID));
if (race_it != THUMBNAIL_PRESETS.end()) {
auto gender_it = race_it->second.find(race_gender->genderIndex);
if (gender_it != race_it->second.end())
preset = &gender_it->second;
}
}

// TODO(conversion): Full thumbnail capture requires model-viewer-gl State integration.
// Once the model viewer State/Context is wired (replacing chrModelViewerContext JSON),
// this function will:
// 1. Save camera position/target/rotation
// 2. Apply THUMBNAIL_PRESETS camera settings
// 3. Set animation to stand (index 0), frame 0
// 4. Render one frame to FBO
// 5. glReadPixels the FBO, crop to square, encode as PNG base64 data URI
// 6. Restore camera/animation state
// The preset data and active_renderer are ready; only the FBO integration is pending.
(void)preset; // suppress unused warning until wired

return "";
}

// JS: function get_current_character_data(core)
static nlohmann::json get_current_character_data() {
auto& view = *core::view;

uint32_t race_id = 0;
uint32_t model_id = 0;

if (!view.chrCustRaceSelection.empty())
race_id = view.chrCustRaceSelection[0].value("id", 0u);
if (!view.chrCustModelSelection.empty())
model_id = view.chrCustModelSelection[0].value("id", 0u);

return nlohmann::json{
{ "race_id", race_id },
{ "model_id", model_id },
{ "choices", view.chrCustActiveChoices },
{ "equipment", view.chrEquippedItems },
{ "guild_tabard", nlohmann::json{
{ "background", view.chrGuildTabardConfig.background },
{ "border_style", view.chrGuildTabardConfig.border_style },
{ "border_color", view.chrGuildTabardConfig.border_color },
{ "emblem_design", view.chrGuildTabardConfig.emblem_design },
{ "emblem_color", view.chrGuildTabardConfig.emblem_color }
}}
};
}

// JS: async function export_json_character(core)
static void export_json_character() {
auto& view = *core::view;

nlohmann::json data = get_current_character_data();
if (data.value("race_id", 0u) == 0 || data.value("model_id", 0u) == 0) {
core::setToast("error", "No character loaded to export.", {}, 3000);
return;
}

// JS: const thumb_data = await capture_character_thumbnail(core);
std::string thumb_data = capture_character_thumbnail();
if (!thumb_data.empty())
data["thumb"] = thumb_data;

// JS: file_input.setAttribute('nwsaveas', 'character.json');
std::string file_path = file_field::saveFileDialog("character.json", "JSON Files", "*.json");
if (file_path.empty())
return;

try {
std::ofstream out(file_path);
out << data.dump(1, '\t');
core::setToast("success", "Character exported successfully.", {}, 3000);
} catch (const std::exception& e) {
logging::write(std::format("failed to export character: {}", e.what()));
core::setToast("error", std::format("Failed to export character: {}", e.what()), {}, -1);
}
}

// JS: async function export_saved_character(core, character)
static void export_saved_character(const nlohmann::json& character) {
namespace fs = std::filesystem;
std::string dir = get_saved_characters_dir();
std::string json_path = (fs::path(dir) / character.value("file_name", "")).string();
std::string name = character.value("name", "");

nlohmann::json data;
try {
std::ifstream in(json_path);
if (!in.is_open())
throw std::runtime_error("Could not open file");
data = nlohmann::json::parse(in);
data["name"] = name;

// include existing thumbnail if available
if (character.contains("thumb") && !character["thumb"].is_null())
data["thumb"] = character["thumb"];
} catch (const std::exception& e) {
logging::write(std::format("failed to read character for export: {}", e.what()));
core::setToast("error", std::format("Failed to read character: {}", e.what()), {}, -1);
return;
}

// JS: file_input.setAttribute('nwsaveas', character.name + '.json');
std::string file_path = file_field::saveFileDialog(name + ".json", "JSON Files", "*.json");
if (file_path.empty())
return;

try {
std::ofstream out(file_path);
out << data.dump(1, '\t');
core::setToast("success", std::format("Character \"{}\" exported successfully.", name), {}, 3000);
} catch (const std::exception& e) {
logging::write(std::format("failed to export character: {}", e.what()));
core::setToast("error", std::format("Failed to export character: {}", e.what()), {}, -1);
}
}

// JS: async function import_json_character(core, save_to_my_characters)
static void import_json_character(bool save_to_my_characters) {
namespace fs = std::filesystem;
auto& view = *core::view;

// JS: file_input.setAttribute('accept', '.json');
std::string file_path = file_field::openFileDialog("JSON Files", "*.json");
if (file_path.empty())
return;

try {
std::ifstream in(file_path);
if (!in.is_open())
throw std::runtime_error("Could not open file");

nlohmann::json data = nlohmann::json::parse(in);

if (!data.contains("race_id") || !data.contains("model_id")) {
core::setToast("error", "Invalid character file: missing race_id or model_id.", {}, -1);
return;
}

if (save_to_my_characters) {
// import into My Characters
std::string name;
if (data.contains("name") && data["name"].is_string())
name = data["name"].get<std::string>();
if (name.empty()) {
// use filename without extension
name = fs::path(file_path).stem().string();
}

std::string dir = get_saved_characters_dir();
generics::createDirectory(dir);

std::string id = generate_character_id();
std::unordered_set<std::string> existing_ids;
for (const auto& c : view.chrSavedCharacters)
existing_ids.insert(c.value("id", ""));
while (existing_ids.count(id))
id = generate_character_id();

// remove name/thumb from data before saving (stored separately)
nlohmann::json save_data = {
{ "race_id", data.value("race_id", 0u) },
{ "model_id", data.value("model_id", 0u) },
{ "choices", data.value("choices", nlohmann::json::array()) },
{ "equipment", data.value("equipment", nlohmann::json::object()) }
};

if (data.contains("guild_tabard"))
save_data["guild_tabard"] = data["guild_tabard"];

std::string save_path = (fs::path(dir) / (name + "-" + id + ".json")).string();
std::ofstream out(save_path);
out << save_data.dump(1, '\t');

// save thumbnail if provided
if (data.contains("thumb") && data["thumb"].is_string()) {
std::string thumb_str = data["thumb"].get<std::string>();
// strip data URI prefix if present: "data:image/png;base64,"
size_t comma_pos = thumb_str.find(',');
if (comma_pos != std::string::npos)
thumb_str = thumb_str.substr(comma_pos + 1);

std::vector<uint8_t> thumb_bytes = BufferWrapper::fromBase64(thumb_str).raw();
std::string thumb_path = (fs::path(dir) / (name + "-" + id + ".png")).string();
std::ofstream thumb_out(thumb_path, std::ios::binary);
thumb_out.write(reinterpret_cast<const char*>(thumb_bytes.data()), static_cast<std::streamsize>(thumb_bytes.size()));
}

load_saved_characters();
core::setToast("success", std::format("Character \"{}\" imported.", name), {}, 3000);
} else {
// load directly into viewer
view.chrModelLoading = true;
view.chrSavedCharactersScreen = false;

view.chrEquippedItems = data.value("equipment", nlohmann::json::object());

if (data.contains("guild_tabard")) {
const auto& gt = data["guild_tabard"];
view.chrGuildTabardConfig.background = gt.value("background", 0);
view.chrGuildTabardConfig.border_style = gt.value("border_style", 0);
view.chrGuildTabardConfig.border_color = gt.value("border_color", 0);
view.chrGuildTabardConfig.emblem_design = gt.value("emblem_design", 0);
view.chrGuildTabardConfig.emblem_color = gt.value("emblem_color", 0);
}

view.chrImportChoices.clear();
if (data.contains("choices") && data["choices"].is_array())
view.chrImportChoices = data["choices"].get<std::vector<nlohmann::json>>();

uint32_t model_id = data.value("model_id", 0u);
view.chrImportChrModelID = model_id;
view.chrImportTargetModelID = model_id;

uint32_t race_id = data.value("race_id", 0u);
for (const auto& race : view.chrCustRaces) {
if (race.value("id", 0u) == race_id) {
view.chrCustRaceSelection = { race };
break;
}
}

view.chrModelLoading = false;
core::setToast("success", "Character loaded.", {}, 3000);
}
} catch (const std::exception& e) {
logging::write(std::format("failed to import character: {}", e.what()));
core::setToast("error", std::format("Failed to import character: {}", e.what()), {}, -1);
}
}

//endregion

//region race

// JS: function update_chr_race_list(core)
static void update_chr_race_list() {
auto& view = *core::view;

std::vector<uint32_t> listed_model_ids;
std::vector<uint32_t> listed_race_ids;

view.chrCustRacesPlayable.clear();
view.chrCustRacesNPC.clear();

const auto& chr_race_map = db::caches::DBCharacterCustomization::get_chr_race_map();
const auto& chr_race_x_chr_model_map = db::caches::DBCharacterCustomization::get_chr_race_x_chr_model_map();

for (const auto& [chr_race_id, chr_race_info] : chr_race_map) {
auto model_it = chr_race_x_chr_model_map.find(chr_race_id);
if (model_it == chr_race_x_chr_model_map.end())
continue;

const auto& chr_models = model_it->second;
for (const auto& [_, chr_model_id] : chr_models) {
if (std::find(listed_model_ids.begin(), listed_model_ids.end(), chr_model_id) != listed_model_ids.end())
continue;

listed_model_ids.push_back(chr_model_id);

if (std::find(listed_race_ids.begin(), listed_race_ids.end(), chr_race_id) != listed_race_ids.end())
continue;

listed_race_ids.push_back(chr_race_id);

nlohmann::json new_race = {
{ "id", chr_race_info.id },
{ "label", chr_race_info.name }
};

if (chr_race_info.isNPCRace)
view.chrCustRacesNPC.push_back(new_race);
else
view.chrCustRacesPlayable.push_back(new_race);

if (!view.chrCustRaceSelection.empty() &&
    new_race.value("id", 0u) == view.chrCustRaceSelection[0].value("id", 0u))
view.chrCustRaceSelection = { new_race };
}
}

auto sort_by_label = [](const nlohmann::json& a, const nlohmann::json& b) {
return a.value("label", "") < b.value("label", "");
};

std::sort(view.chrCustRacesPlayable.begin(), view.chrCustRacesPlayable.end(), sort_by_label);
std::sort(view.chrCustRacesNPC.begin(), view.chrCustRacesNPC.end(), sort_by_label);

view.chrCustRaces.clear();
view.chrCustRaces.insert(view.chrCustRaces.end(), view.chrCustRacesPlayable.begin(), view.chrCustRacesPlayable.end());
view.chrCustRaces.insert(view.chrCustRaces.end(), view.chrCustRacesNPC.begin(), view.chrCustRacesNPC.end());

if (view.chrCustRaceSelection.empty() ||
    std::find(listed_race_ids.begin(), listed_race_ids.end(),
              view.chrCustRaceSelection[0].value("id", 0u)) == listed_race_ids.end()) {
if (!view.chrCustRacesPlayable.empty())
view.chrCustRaceSelection = { view.chrCustRacesPlayable[0] };
}
}

//endregion

//region export

// JS: const export_char_model = async (core) => { ... }
static void export_char_model() {
auto& view = *core::view;
auto export_paths = core::openLastExportStream();
std::string format = view.config.value("exportCharacterFormat", "GLTF");

if (format == "PNG" || format == "CLIPBOARD") {
if (active_model != 0) {
core::setToast("progress", "saving preview, hold on...", {}, -1, false);

// JS: const canvas = document.querySelector('.char-preview canvas');
// JS: const buf = await BufferWrapper.fromCanvas(canvas, 'image/png');
// TODO(conversion): GL framebuffer capture will be wired when model-viewer-gl State is integrated.
// Once the FBO is accessible, this will use the export_preview pattern from model-viewer-utils.

if (format == "PNG") {
std::string file_name = casc::listfile::getByID(active_model);
std::string export_path = casc::ExportHelper::getExportPath(file_name);
std::string out_file = casc::ExportHelper::replaceExtension(export_path, ".png");

if (view.config.value("modelsExportPngIncrements", false))
out_file = casc::ExportHelper::getIncrementalFilename(out_file);

// TODO(conversion): Write PNG buffer to file once FBO is accessible.
logging::write(std::format("saved 3d preview screenshot to {}", out_file));
} else if (format == "CLIPBOARD") {
// TODO(conversion): Copy PNG to clipboard once FBO capture is accessible.
// Will use: ImGui::SetClipboardText(buf.toBase64().c_str());
logging::write(std::format("copied 3d preview to clipboard (character {})", active_model));
core::setToast("success", "3D preview has been copied to the clipboard", {}, -1, true);
}
} else {
core::setToast("error", "the selected export option only works for character previews. preview something first!", {}, -1);
}

export_paths.close();
return;
}

casc::ExportHelper helper(1, "model");
helper.start();

if (helper.isCancelled())
return;

uint32_t file_data_id = active_model;
std::string file_name = casc::listfile::getByID(file_data_id);

try {
if (format == "OBJ" || format == "STL") {
if (!active_renderer || !active_renderer->m2) {
core::setToast("error", "no character model loaded to export", {}, -1);
export_paths.close();
return;
}

std::string ext = (format == "STL") ? ".stl" : ".obj";
std::string mark_file_name = casc::ExportHelper::replaceExtension(file_name, ext);
std::string export_path = casc::ExportHelper::getExportPath(mark_file_name);

// JS: const data = await casc.getFile(file_data_id);
BufferWrapper data = core::view->casc->getVirtualFileByID(file_data_id);
// TODO(conversion): M2Exporter instantiation and exportAsOBJ/exportAsSTL will be wired when renderer integration is complete.
// M2Exporter exporter(data, {}, file_data_id, core::view->casc);

logging::write(std::format("Character OBJ/STL export requested for {}", file_name));

if (helper.isCancelled())
return;

helper.mark(mark_file_name, true);
} else {
// GLTF/GLB
std::string mark_file_name = casc::ExportHelper::replaceExtension(file_name, ".gltf");
std::string export_path = casc::ExportHelper::getExportPath(mark_file_name);

// JS: const data = await casc.getFile(file_data_id);
BufferWrapper data = core::view->casc->getVirtualFileByID(file_data_id);
// TODO(conversion): M2Exporter instantiation and exportAsGLTF will be wired when renderer integration is complete.
// M2Exporter exporter(data, {}, file_data_id, core::view->casc);

logging::write(std::format("Character GLTF export requested for {}", file_name));

if (helper.isCancelled())
return;

helper.mark(mark_file_name, true);
}
} catch (const std::exception& e) {
helper.mark(file_name, false, e.what(), "");
}

helper.finish();
export_paths.close();
}

// JS: const export_chr_texture = async (core) => { ... }
static void export_chr_texture() {
uint32_t active_canvas = char_texture_overlay::getActiveLayer();
if (active_canvas == 0) {
core::setToast("error", "no texture is currently being previewed", {}, -1);
return;
}

auto export_paths = core::openLastExportStream();
core::setToast("progress", "exporting texture, hold on...", {}, -1, false);

// TODO(conversion): CharMaterialRenderer getCanvas/getRawPixels integration needed.
// For now, log the request.
logging::write("Character texture export requested (CharMaterialRenderer integration needed)");
core::setToast("info", "Character texture export is not yet fully wired in this build.", {}, 5000);

export_paths.close();
}

//endregion

//region utils

// JS: function int_to_css_color(value)
static std::string int_to_css_color(uint32_t value) {
if (value == 0)
return "transparent";

uint32_t unsigned_val = value;
uint8_t r = (unsigned_val >> 16) & 0xFF;
uint8_t g = (unsigned_val >> 8) & 0xFF;
uint8_t b = unsigned_val & 0xFF;

return std::format("rgb({}, {}, {})", r, g, b);
}

// Convert color int to ImGui ImU32 (ABGR)
static ImU32 int_to_imgui_color(uint32_t value) {
if (value == 0)
return IM_COL32(0, 0, 0, 0); // transparent

uint8_t r = (value >> 16) & 0xFF;
uint8_t g = (value >> 8) & 0xFF;
uint8_t b = value & 0xFF;

return IM_COL32(r, g, b, 255);
}

// JS: function get_selected_choice(core, option_id)
static const db::caches::DBCharacterCustomization::ChoiceEntry* get_selected_choice(uint32_t option_id) {
auto& view = *core::view;

uint32_t active_choice_id = 0;
bool found = false;
for (const auto& ac : view.chrCustActiveChoices) {
if (ac.value("optionID", 0u) == option_id) {
active_choice_id = ac.value("choiceID", 0u);
found = true;
break;
}
}

if (!found)
return nullptr;

const auto* choices = db::caches::DBCharacterCustomization::get_choices_for_option(option_id);
if (!choices)
return nullptr;

for (const auto& c : *choices) {
if (c.id == active_choice_id)
return &c;
}

return nullptr;
}

// JS: function update_realm_list()
static void update_realm_list() {
auto& state = *core::view;
std::string base_region = state.chrImportSelectedRegion;
std::string effective_region = state.chrImportClassicRealms ? "classic-" + base_region : base_region;

if (!state.realmList.contains(effective_region))
return;

state.chrImportRealms.clear();
for (const auto& realm : state.realmList[effective_region]) {
state.chrImportRealms.push_back(nlohmann::json{
{ "label", realm.value("name", "") },
{ "value", realm.value("slug", "") }
});
}

if (!state.chrImportSelectedRealm.is_null()) {
std::string selected_value = state.chrImportSelectedRealm.value("value", "");
bool found_matching = false;
for (const auto& realm : state.chrImportRealms) {
if (realm.value("value", "") == selected_value) {
state.chrImportSelectedRealm = realm;
found_matching = true;
break;
}
}
if (!found_matching)
state.chrImportSelectedRealm = nullptr;
}
}

//endregion

//region tabard helpers

// JS: is_guild_tabard_equipped()
static bool is_guild_tabard_equipped() {
auto& view = *core::view;
if (!view.chrEquippedItems.contains("19"))
return false;
uint32_t item_id = view.chrEquippedItems["19"].get<uint32_t>();
return db::caches::DBGuildTabard::isGuildTabard(item_id);
}

// JS: get_tabard_tier()
static int get_tabard_tier() {
auto& view = *core::view;
if (!view.chrEquippedItems.contains("19"))
return -1;
uint32_t item_id = view.chrEquippedItems["19"].get<uint32_t>();
return db::caches::DBGuildTabard::getTabardTier(item_id);
}

// JS: get_tabard_max(key)
static int get_tabard_max(const std::string& key) {
if (key == "background") return db::caches::DBGuildTabard::getBackgroundColorCount();
if (key == "border_style") return db::caches::DBGuildTabard::getBorderStyleCount(get_tabard_tier());
if (key == "border_color") return db::caches::DBGuildTabard::getBorderColorCount();
if (key == "emblem_design") return db::caches::DBGuildTabard::getEmblemDesignCount();
if (key == "emblem_color") return db::caches::DBGuildTabard::getEmblemColorCount();
return 0;
}

// JS: set_tabard_config(key, value)
static void set_tabard_config(const std::string& key, int value) {
auto& view = *core::view;
int max_val = get_tabard_max(key);
if (max_val > 0)
value = (std::max)(0, (std::min)(value, max_val - 1));

auto& cfg = view.chrGuildTabardConfig;
if (key == "background") cfg.background = value;
else if (key == "border_style") cfg.border_style = value;
else if (key == "border_color") cfg.border_color = value;
else if (key == "emblem_design") cfg.emblem_design = value;
else if (key == "emblem_color") cfg.emblem_color = value;
}

// JS: adjust_tabard_config(key, delta)
static void adjust_tabard_config(const std::string& key, int delta) {
auto& view = *core::view;
int max_val = get_tabard_max(key);
if (max_val <= 0)
return;

auto& cfg = view.chrGuildTabardConfig;
int current = 0;
if (key == "background") current = cfg.background;
else if (key == "border_style") current = cfg.border_style;
else if (key == "border_color") current = cfg.border_color;
else if (key == "emblem_design") current = cfg.emblem_design;
else if (key == "emblem_color") current = cfg.emblem_color;

int value = (current + delta) % max_val;
if (value < 0)
value += max_val;

set_tabard_config(key, value);
}

// JS: get_tabard_color_css(key)
static ImU32 get_tabard_color_imgui(const std::string& key) {
auto& view = *core::view;
int color_id = 0;
auto& cfg = view.chrGuildTabardConfig;
if (key == "background") color_id = cfg.background;
else if (key == "border_color") color_id = cfg.border_color;
else if (key == "emblem_color") color_id = cfg.emblem_color;

const std::unordered_map<uint32_t, db::caches::DBGuildTabard::ColorRGB>* color_map = nullptr;
if (key == "background") color_map = &db::caches::DBGuildTabard::getBackgroundColors();
else if (key == "border_color") color_map = &db::caches::DBGuildTabard::getBorderColors();
else if (key == "emblem_color") color_map = &db::caches::DBGuildTabard::getEmblemColors();

if (!color_map)
return IM_COL32(0, 0, 0, 0);

auto it = color_map->find(static_cast<uint32_t>(color_id));
if (it == color_map->end())
return IM_COL32(0, 0, 0, 0);

return IM_COL32(it->second.r, it->second.g, it->second.b, 255);
}

// get_tabard_color_list_for_key(key)
static const std::unordered_map<uint32_t, db::caches::DBGuildTabard::ColorRGB>* get_tabard_color_list_for_key(const std::string& key) {
if (key == "background") return &db::caches::DBGuildTabard::getBackgroundColors();
if (key == "border_color") return &db::caches::DBGuildTabard::getBorderColors();
if (key == "emblem_color") return &db::caches::DBGuildTabard::getEmblemColors();
return nullptr;
}

//endregion

// JS: get_equipped_item(slot_id)
static const db::caches::DBItems::ItemInfo* get_equipped_item(int slot_id) {
auto& view = *core::view;
std::string slot_str = std::to_string(slot_id);
if (!view.chrEquippedItems.contains(slot_str))
return nullptr;
uint32_t item_id = view.chrEquippedItems[slot_str].get<uint32_t>();
return db::caches::DBItems::getItemById(item_id);
}

//region render

// Color picker state
static std::string color_picker_open_for;
static ImVec2 color_picker_position;
static std::string character_import_mode = "none";

/**
 * Render the characters tab widget using ImGui.
 * JS equivalent: the Vue component's template rendering.
 */
void render() {
auto& view = *core::view;
const auto& option_to_choices = db::caches::DBCharacterCustomization::get_option_to_choices_map();

// --- Change detection (Vue watch equivalents) ---

// watch chrCustRaceSelection
if (view.chrCustRaceSelection != prev_race_selection) {
prev_race_selection = view.chrCustRaceSelection;
update_chr_model_list();
}

// watch chrCustModelSelection (deep)
if (view.chrCustModelSelection != prev_model_selection) {
prev_model_selection = view.chrCustModelSelection;
update_model_selection();
}

// watch chrCustOptionSelection (deep)
if (view.chrCustOptionSelection != prev_option_selection) {
prev_option_selection = view.chrCustOptionSelection;
update_customization_type();
}

// watch chrCustChoiceSelection (deep)
if (view.chrCustChoiceSelection != prev_choice_selection) {
prev_choice_selection = view.chrCustChoiceSelection;
update_customization_choice();
}

// watch chrCustActiveChoices (deep)
if (view.chrCustActiveChoices != prev_active_choices) {
prev_active_choices = view.chrCustActiveChoices;
refresh_character_appearance();
}

// watch chrEquippedItems (deep)
if (view.chrEquippedItems != prev_equipped_items) {
prev_equipped_items = view.chrEquippedItems;
refresh_character_appearance();
}

// watch chrGuildTabardConfig
{
nlohmann::json current_tabard = nlohmann::json{
{ "background", view.chrGuildTabardConfig.background },
{ "border_style", view.chrGuildTabardConfig.border_style },
{ "border_color", view.chrGuildTabardConfig.border_color },
{ "emblem_design", view.chrGuildTabardConfig.emblem_design },
{ "emblem_color", view.chrGuildTabardConfig.emblem_color }
};
if (current_tabard != prev_guild_tabard_config) {
prev_guild_tabard_config = current_tabard;
refresh_character_appearance();
}
}

// watch config.chrIncludeBaseClothing
if (view.config.value("chrIncludeBaseClothing", true) != prev_include_base_clothing) {
prev_include_base_clothing = view.config.value("chrIncludeBaseClothing", true);
refresh_character_appearance();
}

// watch chrImportSelectedRegion / chrImportClassicRealms
{
std::string cur_region = view.chrImportSelectedRegion;
bool cur_classic = view.chrImportClassicRealms;
if (cur_region != prev_chr_import_region || cur_classic != prev_chr_import_classic_realms) {
prev_chr_import_region = cur_region;
prev_chr_import_classic_realms = cur_classic;
update_realm_list();
}
}

// watch chrModelViewerAnimSelection
{
std::string current_anim;
if (view.chrModelViewerAnimSelection.is_string())
current_anim = view.chrModelViewerAnimSelection.get<std::string>();

if (current_anim != prev_anim_selection) {
prev_anim_selection = current_anim;

if (!view.chrModelViewerAnims.empty()) {
model_viewer_utils::handle_animation_change(
active_renderer.get(),
view_state,
current_anim
);
}
}
}

// --- Saved Characters Screen ---
if (view.chrSavedCharactersScreen) {
ImGui::Text("My Characters");
ImGui::Separator();

for (size_t i = 0; i < view.chrSavedCharacters.size(); i++) {
const auto& character = view.chrSavedCharacters[i];
std::string name = character.value("name", "Unknown");
std::string id = character.value("id", "");

ImGui::PushID(static_cast<int>(i));

// thumbnail placeholder
ImGui::BeginGroup();
ImGui::Button(name.c_str(), ImVec2(120, 140));

// Export button
ImGui::SameLine();
if (ImGui::SmallButton("Export"))
export_saved_character(character);

ImGui::SameLine();
if (ImGui::SmallButton("Delete"))
delete_character(character);

if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
load_character(character);

ImGui::EndGroup();
ImGui::PopID();
}

ImGui::Separator();

if (ImGui::Button("Save Character")) {
view.chrPendingThumbnail = capture_character_thumbnail();  // returns std::string, auto-converts to json
view.chrSaveCharacterName = "";
view.chrSaveCharacterPrompt = true;
}

ImGui::SameLine();
if (ImGui::Button("Import Character"))
import_json_character(true);

ImGui::SameLine();
if (ImGui::Button("Back"))
view.chrSavedCharactersScreen = false;

// Save prompt modal
if (view.chrSaveCharacterPrompt) {
ImGui::OpenPopup("Save Character##popup");
if (ImGui::BeginPopupModal("Save Character##popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
static char name_buf[256] = {};
ImGui::InputText("Character Name", name_buf, sizeof(name_buf));

if (ImGui::Button("Save")) {
std::string name(name_buf);
if (name.empty()) {
core::setToast("error", "Please enter a character name.", {}, 3000);
} else {
view.chrSaveCharacterPrompt = false;
save_character(name, view.chrPendingThumbnail.is_string() ? view.chrPendingThumbnail.get<std::string>() : "");
name_buf[0] = '\0';
}
}

ImGui::SameLine();
if (ImGui::Button("Cancel")) {
view.chrSaveCharacterPrompt = false;
name_buf[0] = '\0';
}

ImGui::EndPopup();
}
}

return; // don't render main UI when saved characters screen is active
}

// --- Main Character Viewer ---

// Animation controls overlay
if (!view.chrModelViewerAnims.empty()) {
// Animation dropdown
std::string current_anim_label = "No Animation";
std::string current_anim_id;
if (view.chrModelViewerAnimSelection.is_string())
current_anim_id = view.chrModelViewerAnimSelection.get<std::string>();

for (const auto& anim : view.chrModelViewerAnims) {
if (anim.value("id", "") == current_anim_id) {
current_anim_label = anim.value("label", "");
break;
}
}

if (ImGui::BeginCombo("Animation##chr", current_anim_label.c_str())) {
for (const auto& anim : view.chrModelViewerAnims) {
std::string anim_id = anim.value("id", "");
std::string anim_label = anim.value("label", "");
bool is_selected = (anim_id == current_anim_id);
if (ImGui::Selectable(anim_label.c_str(), is_selected))
view.chrModelViewerAnimSelection = anim_id;
if (is_selected)
ImGui::SetItemDefaultFocus();
}
ImGui::EndCombo();
}

// Animation playback controls
if (current_anim_id != "none" && !current_anim_id.empty()) {
bool is_paused = view.chrModelViewerAnimPaused;

if (ImGui::Button("|<##chr_step_left")) {
if (is_paused && anim_methods)
anim_methods->step_animation(-1);
}

ImGui::SameLine();
if (ImGui::Button(is_paused ? ">##chr_play" : "||##chr_pause")) {
if (anim_methods)
anim_methods->toggle_animation_pause();
}

ImGui::SameLine();
if (ImGui::Button(">|##chr_step_right")) {
if (is_paused && anim_methods)
anim_methods->step_animation(1);
}

// Scrubber
ImGui::SameLine();
int frame = view.chrModelViewerAnimFrame;
int frame_count = view.chrModelViewerAnimFrameCount;
if (frame_count > 0) {
ImGui::SetNextItemWidth(200.0f);
if (ImGui::SliderInt("##chr_scrub", &frame, 0, frame_count - 1)) {
if (anim_methods)
anim_methods->seek_animation(frame);
}
if (ImGui::IsItemActivated()) {
_was_paused_before_scrub = view.chrModelViewerAnimPaused;
if (!_was_paused_before_scrub && anim_methods)
anim_methods->start_scrub();
}
if (ImGui::IsItemDeactivatedAfterEdit()) {
if (!_was_paused_before_scrub && anim_methods)
anim_methods->end_scrub();
}
ImGui::SameLine();
ImGui::Text("%d", frame);
}
}
}

// Import buttons row
{
if (ImGui::Button("My Characters")) {
load_saved_characters();
view.chrSavedCharactersScreen = true;
}
ImGui::SameLine();
if (ImGui::Button("Save"))
view.chrSaveCharacterPrompt = true;

ImGui::SameLine();
if (ImGui::Button("Import JSON"))
import_json_character(false);
ImGui::SameLine();
if (ImGui::Button("Export JSON"))
export_json_character();

ImGui::SameLine();
bool bnet_active = (character_import_mode == "BNET");
if (bnet_active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
if (ImGui::Button("Battle.net"))
character_import_mode = bnet_active ? "none" : "BNET";
if (bnet_active) ImGui::PopStyleColor();

ImGui::SameLine();
bool whead_active = (character_import_mode == "WHEAD");
if (whead_active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
if (ImGui::Button("Wowhead"))
character_import_mode = whead_active ? "none" : "WHEAD";
if (whead_active) ImGui::PopStyleColor();

ImGui::SameLine();
if (ImGui::Button("WMV"))
import_wmv_character();
}

// Battle.net import panel
if (character_import_mode == "BNET") {
ImGui::Separator();
ImGui::Text("Character Import");

// Region buttons
for (size_t i = 0; i < base_regions.size(); i++) {
if (i > 0) ImGui::SameLine();
bool selected = (view.chrImportSelectedRegion == base_regions[i]);
if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

std::string upper_region = base_regions[i];
std::transform(upper_region.begin(), upper_region.end(), upper_region.begin(), ::toupper);
if (ImGui::Button(upper_region.c_str()))
view.chrImportSelectedRegion = base_regions[i];

if (selected) ImGui::PopStyleColor();
}

ImGui::Checkbox("Classic Realms", &view.chrImportClassicRealms);

// Character name
static char chr_name_buf[256] = {};
ImGui::InputText("Character Name##bnet", chr_name_buf, sizeof(chr_name_buf));
view.chrImportChrName = chr_name_buf;

// Realm combo (simplified)
// TODO(conversion): Full ComboBox with search for realm list.
if (!view.chrImportRealms.empty()) {
std::string realm_label = view.chrImportSelectedRealm.is_null() ? "Select Realm" :
view.chrImportSelectedRealm.value("label", "Select Realm");
if (ImGui::BeginCombo("Realm##bnet", realm_label.c_str())) {
for (const auto& realm : view.chrImportRealms) {
std::string label = realm.value("label", "");
bool is_selected = (!view.chrImportSelectedRealm.is_null() &&
                    realm.value("value", "") == view.chrImportSelectedRealm.value("value", ""));
if (ImGui::Selectable(label.c_str(), is_selected))
view.chrImportSelectedRealm = realm;
if (is_selected)
ImGui::SetItemDefaultFocus();
}
ImGui::EndCombo();
}
}

ImGui::Checkbox("Load visage model (Dracthyr/Worgen)", &view.chrImportLoadVisage);

if (ImGui::Button("Import Character##bnet") && !view.chrModelLoading)
import_character();

ImGui::Separator();
}

// Wowhead import panel
if (character_import_mode == "WHEAD") {
ImGui::Separator();
ImGui::Text("Wowhead Import");

static char wowhead_url_buf[1024] = {};
ImGui::InputText("Wowhead URL##whead", wowhead_url_buf, sizeof(wowhead_url_buf));
view.chrImportWowheadURL = wowhead_url_buf;

if (ImGui::Button("Import Character##whead") && !view.chrModelLoading)
import_wowhead_character();

ImGui::Separator();
}

// --- Three-panel layout: left (customization) | center (preview) | right (equipment) ---

float available_width = ImGui::GetContentRegionAvail().x;
float left_width = 250.0f;
float right_width = 250.0f;
float center_width = available_width - left_width - right_width - 20.0f;

// LEFT PANEL - Race/body/customization
ImGui::BeginChild("##chr_left_panel", ImVec2(left_width, -1), ImGuiChildFlags_Borders);
{
if (!view.chrShowGeosetControl) {
// Race selector
{
std::string race_label = "None";
if (!view.chrCustRaceSelection.empty())
race_label = view.chrCustRaceSelection[0].value("label", "None");

if (ImGui::BeginCombo("Race##chr", race_label.c_str())) {
if (!view.chrCustRacesPlayable.empty()) {
ImGui::TextDisabled("Playable Races");
for (const auto& race : view.chrCustRacesPlayable) {
std::string label = race.value("label", "");
uint32_t rid = race.value("id", 0u);
bool is_selected = (!view.chrCustRaceSelection.empty() &&
                    view.chrCustRaceSelection[0].value("id", 0u) == rid);
if (ImGui::Selectable(label.c_str(), is_selected))
view.chrCustRaceSelection = { race };
if (is_selected)
ImGui::SetItemDefaultFocus();
}
}
if (!view.chrCustRacesNPC.empty()) {
ImGui::Separator();
ImGui::TextDisabled("NPC Races");
for (const auto& race : view.chrCustRacesNPC) {
std::string label = race.value("label", "");
uint32_t rid = race.value("id", 0u);
bool is_selected = (!view.chrCustRaceSelection.empty() &&
                    view.chrCustRaceSelection[0].value("id", 0u) == rid);
if (ImGui::Selectable(label.c_str(), is_selected))
view.chrCustRaceSelection = { race };
if (is_selected)
ImGui::SetItemDefaultFocus();
}
}
ImGui::EndCombo();
}
}

// Body type selector
{
std::string body_label = "None";
if (!view.chrCustModelSelection.empty())
body_label = view.chrCustModelSelection[0].value("label", "None");

if (ImGui::BeginCombo("Body##chr", body_label.c_str())) {
for (const auto& model : view.chrCustModels) {
std::string label = model.value("label", "");
uint32_t mid = model.value("id", 0u);
bool is_selected = (!view.chrCustModelSelection.empty() &&
                    view.chrCustModelSelection[0].value("id", 0u) == mid);
if (ImGui::Selectable(label.c_str(), is_selected))
view.chrCustModelSelection = { model };
if (is_selected)
ImGui::SetItemDefaultFocus();
}
ImGui::EndCombo();
}
}

// Customization options
for (const auto& option : view.chrCustOptions) {
uint32_t option_id = option.value("id", 0u);
std::string option_label = option.value("label", "");
bool is_color = option.value("is_color_swatch", false);

ImGui::PushID(static_cast<int>(option_id));

if (!is_color) {
// Regular dropdown
auto choices_it = option_to_choices.find(option_id);
std::string current_choice_label = "None";

uint32_t active_choice_id = 0;
for (const auto& ac : view.chrCustActiveChoices) {
if (ac.value("optionID", 0u) == option_id) {
active_choice_id = ac.value("choiceID", 0u);
break;
}
}

if (choices_it != option_to_choices.end()) {
for (const auto& c : choices_it->second) {
if (c.id == active_choice_id) {
current_choice_label = c.label;
break;
}
}
}

std::string combo_label = option_label + "##opt";
if (ImGui::BeginCombo(combo_label.c_str(), current_choice_label.c_str())) {
if (choices_it != option_to_choices.end()) {
for (const auto& choice : choices_it->second) {
bool is_sel = (choice.id == active_choice_id);
if (ImGui::Selectable(choice.label.c_str(), is_sel))
update_choice_for_option(option_id, choice.id);
if (is_sel)
ImGui::SetItemDefaultFocus();
}
}
ImGui::EndCombo();
}
} else {
// Color swatch picker
const auto* sel_choice = get_selected_choice(option_id);

ImGui::Text("%s:", option_label.c_str());
ImGui::SameLine();

// Draw the selected swatch
if (sel_choice) {
ImU32 col0 = int_to_imgui_color(sel_choice->swatch_color_0);
ImU32 col1 = int_to_imgui_color(sel_choice->swatch_color_1);

ImVec2 swatch_size(20, 20);
ImVec2 cursor = ImGui::GetCursorScreenPos();

if (sel_choice->swatch_color_0 == 0 && sel_choice->swatch_color_1 == 0) {
// "none" swatch
ImGui::GetWindowDrawList()->AddRectFilled(cursor,
ImVec2(cursor.x + swatch_size.x, cursor.y + swatch_size.y),
IM_COL32(60, 60, 60, 255));
ImGui::GetWindowDrawList()->AddLine(cursor,
ImVec2(cursor.x + swatch_size.x, cursor.y + swatch_size.y),
IM_COL32(200, 200, 200, 255));
} else if (sel_choice->swatch_color_1 != 0) {
// dual swatch
ImGui::GetWindowDrawList()->AddRectFilled(cursor,
ImVec2(cursor.x + swatch_size.x / 2, cursor.y + swatch_size.y), col0);
ImGui::GetWindowDrawList()->AddRectFilled(
ImVec2(cursor.x + swatch_size.x / 2, cursor.y),
ImVec2(cursor.x + swatch_size.x, cursor.y + swatch_size.y), col1);
} else {
// single swatch
ImGui::GetWindowDrawList()->AddRectFilled(cursor,
ImVec2(cursor.x + swatch_size.x, cursor.y + swatch_size.y), col0);
}

ImGui::InvisibleButton("##swatch", swatch_size);
if (ImGui::IsItemClicked()) {
if (color_picker_open_for == std::to_string(option_id))
color_picker_open_for.clear();
else
color_picker_open_for = std::to_string(option_id);
}
}

// Color picker popup
if (color_picker_open_for == std::to_string(option_id)) {
auto choices_it = option_to_choices.find(option_id);
if (choices_it != option_to_choices.end()) {
ImGui::BeginTooltip();
int cols = 8;
int col_idx = 0;
for (const auto& choice : choices_it->second) {
if (col_idx > 0 && col_idx % cols != 0)
ImGui::SameLine();

ImVec2 swatch_sz(22, 22);
ImVec2 cursor = ImGui::GetCursorScreenPos();
ImU32 c0 = int_to_imgui_color(choice.swatch_color_0);
ImU32 c1 = int_to_imgui_color(choice.swatch_color_1);

if (choice.swatch_color_0 == 0 && choice.swatch_color_1 == 0) {
ImGui::GetWindowDrawList()->AddRectFilled(cursor,
ImVec2(cursor.x + swatch_sz.x, cursor.y + swatch_sz.y),
IM_COL32(60, 60, 60, 255));
} else if (choice.swatch_color_1 != 0) {
ImGui::GetWindowDrawList()->AddRectFilled(cursor,
ImVec2(cursor.x + swatch_sz.x / 2, cursor.y + swatch_sz.y), c0);
ImGui::GetWindowDrawList()->AddRectFilled(
ImVec2(cursor.x + swatch_sz.x / 2, cursor.y),
ImVec2(cursor.x + swatch_sz.x, cursor.y + swatch_sz.y), c1);
} else {
ImGui::GetWindowDrawList()->AddRectFilled(cursor,
ImVec2(cursor.x + swatch_sz.x, cursor.y + swatch_sz.y), c0);
}

// Highlight selected
uint32_t active_cid = 0;
for (const auto& ac : view.chrCustActiveChoices) {
if (ac.value("optionID", 0u) == option_id) {
active_cid = ac.value("choiceID", 0u);
break;
}
}
if (choice.id == active_cid) {
ImGui::GetWindowDrawList()->AddRect(cursor,
ImVec2(cursor.x + swatch_sz.x, cursor.y + swatch_sz.y),
IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
}

ImGui::InvisibleButton(("##color_" + std::to_string(choice.id)).c_str(), swatch_sz);
if (ImGui::IsItemClicked()) {
update_choice_for_option(option_id, choice.id);
color_picker_open_for.clear();
}

col_idx++;
}
ImGui::EndTooltip();
}
}
}

ImGui::PopID();
}

// Underwear toggle
{
bool show_underwear = view.config.value("chrIncludeBaseClothing", true);
std::string underwear_label = show_underwear ? "Visible" : "Hidden";
if (ImGui::BeginCombo("Underwear##chr", underwear_label.c_str())) {
if (ImGui::Selectable("Visible", show_underwear))
view.config["chrIncludeBaseClothing"] = true;
if (ImGui::Selectable("Hidden", !show_underwear))
view.config["chrIncludeBaseClothing"] = false;
ImGui::EndCombo();
}
}
} else {
// Geoset control mode
ImGui::Text("Geoset Control");
ImGui::Separator();

for (auto& geoset : view.chrCustGeosets) {
int gid = geoset.value("id", 0);
if (gid == 0) continue;

std::string label = geoset.value("label", std::format("Geoset {}", gid));
bool checked = geoset.value("checked", false);
if (ImGui::Checkbox(label.c_str(), &checked))
geoset["checked"] = checked;
}

if (ImGui::Button("Enable All")) {
for (auto& g : view.chrCustGeosets)
if (g.value("id", 0) != 0) g["checked"] = true;
}
ImGui::SameLine();
if (ImGui::Button("Disable All")) {
for (auto& g : view.chrCustGeosets)
if (g.value("id", 0) != 0) g["checked"] = false;
}
}

ImGui::Separator();

if (!view.chrShowGeosetControl) {
if (ImGui::Button("Randomize Customization"))
randomize_customization();
if (ImGui::Button("Custom Geoset Control"))
view.chrShowGeosetControl = true;
} else {
if (ImGui::Button("Return to Customization"))
view.chrShowGeosetControl = false;
}
}
ImGui::EndChild();

ImGui::SameLine();

// CENTER PANEL - 3D preview and export
ImGui::BeginChild("##chr_center_panel", ImVec2(center_width, -1), ImGuiChildFlags_Borders);
{
// Loading spinner
if (view.chrModelLoading)
ImGui::Text("Loading model...");

// 3D model viewer
// TODO(conversion): ModelViewerGL component will be rendered here.
ImGui::Text("[3D Preview Area]");

// Remove baked texture button
if (!view.chrCustBakedNPCTexture.is_null()) {
if (ImGui::Button("Remove Baked Texture")) {
view.chrCustBakedNPCTexture = nullptr;
refresh_character_appearance();
}
}

ImGui::Separator();

// Export tabs
static int export_tab = 0; // 0=Export, 1=Textures, 2=Settings

if (ImGui::Button("Export##tab")) export_tab = 0;
ImGui::SameLine();
if (ImGui::Button("Textures##tab")) export_tab = 1;
ImGui::SameLine();
if (ImGui::Button("Settings##tab")) export_tab = 2;

if (export_tab == 0) {
// Export panel
std::string format = view.config.value("exportCharacterFormat", "GLTF");

if (format == "GLTF" || format == "GLB") {
bool export_anims = view.config.value("modelsExportAnimations", false);
if (ImGui::Checkbox("Export animations##chr", &export_anims))
view.config["modelsExportAnimations"] = export_anims;
}

if (format == "OBJ" || format == "STL") {
bool apply_pose = view.config.value("chrExportApplyPose", false);
if (ImGui::Checkbox("Apply pose##chr", &apply_pose))
view.config["chrExportApplyPose"] = apply_pose;
}

// Format selector + export button
const char* formats[] = { "OBJ", "STL", "GLTF", "GLB", "PNG", "CLIPBOARD" };
int current_format_idx = 0;
for (int i = 0; i < 6; i++) {
if (format == formats[i]) { current_format_idx = i; break; }
}
ImGui::SetNextItemWidth(100.0f);
if (ImGui::Combo("Format##chr_export", &current_format_idx, formats, 6))
view.config["exportCharacterFormat"] = formats[current_format_idx];

ImGui::SameLine();
if (ImGui::Button("Export##chr_do") && !view.chrModelLoading)
export_char_model();

} else if (export_tab == 1) {
// Texture preview panel
ImGui::Text("[Texture Preview]");

if (ImGui::Button("<##chr_tex"))
char_texture_overlay::prevOverlay();
ImGui::SameLine();
if (ImGui::Button("Export Texture##chr"))
export_chr_texture();
ImGui::SameLine();
if (ImGui::Button(">##chr_tex"))
char_texture_overlay::nextOverlay();

} else if (export_tab == 2) {
// Settings panel
bool render_shadow = view.config.value("chrRenderShadow", false);
if (ImGui::Checkbox("Render shadow##chr", &render_shadow))
view.config["chrRenderShadow"] = render_shadow;

bool use_3d_camera = view.config.value("chrUse3DCamera", false);
if (ImGui::Checkbox("Use 3D camera##chr", &use_3d_camera))
view.config["chrUse3DCamera"] = use_3d_camera;

bool show_bg = view.config.value("chrShowBackground", false);
if (ImGui::Checkbox("Show background##chr", &show_bg))
view.config["chrShowBackground"] = show_bg;

if (show_bg) {
std::string bg_color_str = view.config.value("chrBackgroundColor", "#343a40");
// TODO(conversion): ImGui color picker for background.
}
}
}
ImGui::EndChild();

ImGui::SameLine();

// RIGHT PANEL - Equipment and tabard
ImGui::BeginChild("##chr_right_panel", ImVec2(right_width, -1), ImGuiChildFlags_Borders);
{
// Guild Tabard panel
if (is_guild_tabard_equipped()) {
ImGui::Text("Guild Tabard");
ImGui::Separator();

for (const auto& opt : tabard_options) {
ImGui::PushID(opt.key.c_str());

if (opt.type == "value") {
// Numeric value with arrows
auto& cfg = view.chrGuildTabardConfig;
int value = 0;
if (opt.key == "background") value = cfg.background;
else if (opt.key == "border_style") value = cfg.border_style;
else if (opt.key == "border_color") value = cfg.border_color;
else if (opt.key == "emblem_design") value = cfg.emblem_design;
else if (opt.key == "emblem_color") value = cfg.emblem_color;

ImGui::Text("%s:", opt.label.c_str());
ImGui::SameLine();
if (ImGui::ArrowButton("##left", ImGuiDir_Left))
adjust_tabard_config(opt.key, -1);
ImGui::SameLine();
ImGui::Text("%d", value);
ImGui::SameLine();
if (ImGui::ArrowButton("##right", ImGuiDir_Right))
adjust_tabard_config(opt.key, 1);

} else {
// Color swatch
ImGui::Text("%s:", opt.label.c_str());
ImGui::SameLine();

ImU32 col = get_tabard_color_imgui(opt.key);
ImVec2 sz(20, 20);
ImVec2 cursor = ImGui::GetCursorScreenPos();
ImGui::GetWindowDrawList()->AddRectFilled(cursor,
ImVec2(cursor.x + sz.x, cursor.y + sz.y), col);

std::string picker_key = "tabard_" + opt.key;
ImGui::InvisibleButton("##tabswatch", sz);
if (ImGui::IsItemClicked()) {
if (color_picker_open_for == picker_key)
color_picker_open_for.clear();
else
color_picker_open_for = picker_key;
}

if (color_picker_open_for == picker_key) {
const auto* colors = get_tabard_color_list_for_key(opt.key);
if (colors) {
ImGui::BeginTooltip();
int cols = 8;
int col_idx = 0;
auto& cfg = view.chrGuildTabardConfig;
int current_color = 0;
if (opt.key == "background") current_color = cfg.background;
else if (opt.key == "border_color") current_color = cfg.border_color;
else if (opt.key == "emblem_color") current_color = cfg.emblem_color;

for (const auto& [cid, crgb] : *colors) {
if (col_idx > 0 && col_idx % cols != 0)
ImGui::SameLine();

ImVec2 ss(22, 22);
ImVec2 cp = ImGui::GetCursorScreenPos();
ImGui::GetWindowDrawList()->AddRectFilled(cp,
ImVec2(cp.x + ss.x, cp.y + ss.y),
IM_COL32(crgb.r, crgb.g, crgb.b, 255));

if (static_cast<int>(cid) == current_color) {
ImGui::GetWindowDrawList()->AddRect(cp,
ImVec2(cp.x + ss.x, cp.y + ss.y),
IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
}

ImGui::InvisibleButton(("##tc_" + std::to_string(cid)).c_str(), ss);
if (ImGui::IsItemClicked()) {
set_tabard_config(opt.key, static_cast<int>(cid));
color_picker_open_for.clear();
}

col_idx++;
}
ImGui::EndTooltip();
}
}
}

ImGui::PopID();
}

ImGui::Separator();
}

// Equipment list
ImGui::Text("Equipment");
ImGui::Separator();

const auto& slots = wow::EQUIPMENT_SLOTS;
for (const auto& slot : slots) {
ImGui::PushID(slot.id);

auto slot_name_opt = wow::get_slot_name(slot.id);
std::string slot_label = slot_name_opt.has_value() ? std::string(slot_name_opt.value()) : std::format("Slot {}", slot.id);
const auto* item = get_equipped_item(slot.id);

if (item) {
std::string item_text = std::format("{}: {} ({})", slot_label, item->name, item->id);
ImGui::Text("%s", item_text.c_str());
} else {
ImGui::TextDisabled("%s: Empty", slot_label.c_str());
}

// Context menu
if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && item) {
ImGui::OpenPopup("##equip_ctx");
}
if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !item) {
// navigate to items for this slot
tab_items::setActive();
}

if (ImGui::BeginPopup("##equip_ctx")) {
if (ImGui::MenuItem("Replace Item")) {
tab_items::setActive();
}
if (ImGui::MenuItem("Remove Item")) {
std::string slot_str = std::to_string(slot.id);
view.chrEquippedItems.erase(slot_str);
}
if (item) {
std::string copy_id_label = std::format("Copy Item ID ({})", item->id);
if (ImGui::MenuItem(copy_id_label.c_str())) {
ImGui::SetClipboardText(std::to_string(item->id).c_str());
}
if (ImGui::MenuItem("Copy Item Name")) {
ImGui::SetClipboardText(item->name.c_str());
}
}
ImGui::EndPopup();
}

ImGui::PopID();
}

ImGui::Separator();
if (ImGui::Button("Clear All Equipment"))
view.chrEquippedItems = nlohmann::json::object();
}
ImGui::EndChild();
}

//endregion

//region mounted

/**
 * Initialize the characters tab (load DB caches, realmlist, set up watches).
 * JS equivalent: mounted()
 */
void mounted() {
auto& state = *core::view;

reset_module_state();

core::showLoadingScreen(8);

core::progressLoadingScreen("Retrieving realmlist...");
casc::realmlist::load();

// preserve region/realm selection across module reloads
if (state.chrImportSelectedRegion.empty())
state.chrImportSelectedRegion = "us";
else
update_realm_list();

core::progressLoadingScreen("Loading character customization data...");
db::caches::DBCharacterCustomization::ensureInitialized();

core::progressLoadingScreen("Loading item data...");
db::caches::DBItems::ensureInitialized();

core::progressLoadingScreen("Loading item character textures...");
db::caches::DBItemCharTextures::ensureInitialized();

core::progressLoadingScreen("Loading item geosets...");
db::caches::DBItemGeosets::ensureInitialized();

core::progressLoadingScreen("Loading item models...");
db::caches::DBItemModels::ensureInitialized();

core::progressLoadingScreen("Loading guild tabard data...");
db::caches::DBGuildTabard::ensureInitialized();

core::progressLoadingScreen("Loading character shaders...");

// set up model viewer context
state.chrModelViewerContext = nlohmann::json{
{ "gl_context", nullptr },
{ "controls", nullptr },
{ "useCharacterControls", true },
{ "fitCamera", nullptr }
};

// Set up animation ViewStateProxy
view_state.anims = &state.chrModelViewerAnims;
view_state.animSelection = &state.chrModelViewerAnimSelection;
view_state.animPaused = &state.chrModelViewerAnimPaused;
view_state.animFrame = &state.chrModelViewerAnimFrame;
view_state.animFrameCount = &state.chrModelViewerAnimFrameCount;
view_state.autoAdjust = nullptr; // TODO(conversion): wire autoAdjust.

anim_methods = std::make_unique<model_viewer_utils::AnimationMethods>(
[]() -> M2RendererGL* { return active_renderer.get(); },
[]() -> model_viewer_utils::ViewStateProxy* { return &view_state; }
);

// Initialize change detection state
prev_race_selection = state.chrCustRaceSelection;
prev_model_selection = state.chrCustModelSelection;
prev_option_selection = state.chrCustOptionSelection;
prev_choice_selection = state.chrCustChoiceSelection;
prev_active_choices = state.chrCustActiveChoices;
prev_equipped_items = state.chrEquippedItems;
prev_include_base_clothing = state.config.value("chrIncludeBaseClothing", true);
prev_chr_import_region = state.chrImportSelectedRegion;
prev_chr_import_classic_realms = state.chrImportClassicRealms;

// trigger initial race/model load
update_chr_race_list();

core::hideLoadingScreen();

char_texture_overlay::ensureActiveLayerAttached();
}

//endregion

//region registerTab

/**
 * Register the tab (nav button).
 * JS equivalent: register() { this.registerNavButton('Characters', 'person-solid.svg', InstallType.CASC) }
 */
void registerTab() {
// JS: this.registerNavButton('Characters', 'person-solid.svg', InstallType.CASC);
modules::register_nav_button("tab_characters", "Characters", "person-solid.svg", install_type::CASC);
}

/**
 * Get the active model renderer (if any).
 * JS equivalent: getActiveRenderer: () => active_renderer
 * @returns Pointer to active M2RendererGL, or nullptr.
 */
M2RendererGL* getActiveRenderer() {
return active_renderer.get();
}

//endregion

} // namespace tab_characters
