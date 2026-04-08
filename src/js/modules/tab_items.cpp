/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_items.h"
#include "../log.h"
#include "../core.h"
#include "../MultiMap.h"
#include "../casc/listfile.h"
#include "../casc/db2.h"
#include "../db/caches/DBModelFileData.h"
#include "../db/caches/DBTextureFileData.h"
#include "../db/caches/DBItems.h"
#include "../db/WDCReader.h"
#include "../wow/ItemSlot.h"
#include "../wow/EquipmentSlots.h"
#include "../install-type.h"
#include "../modules.h"
// const ExternalLinks = require('../external-links'); // Removed: external-links module deleted

#include <cstring>
#include <algorithm>
#include <format>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_items {

// --- File-local structures ---

// JS: class Item { constructor(id, item_sparse_row, item_appearance_row, textures, models) { ... } }
struct Item {
	uint32_t id = 0;
	std::string name;
	int inventoryType = 0;
	int quality = 0;
	uint32_t icon = 0;
	std::vector<uint32_t> models;
	std::vector<uint32_t> textures;
	int modelCount = 0;
	int textureCount = 0;

	// JS: get itemSlotName() { return ItemSlot.getSlotName(this.inventoryType); }
	std::string_view itemSlotName() const {
		return wow::getSlotName(inventoryType);
	}

	// JS: get displayName() { return this.name + ' (' + this.id + ')'; }
	std::string displayName() const {
		return std::format("{} ({})", name, id);
	}
};

// JS: itemViewerTypeMask entries: { label, checked }
struct TypeMaskEntry {
	std::string label;
	bool checked = false;
};

// JS: itemViewerQualityMask entries: { id, label, checked }
struct QualityMaskEntry {
	int id = 0;
	std::string label;
	bool checked = false;
};

// --- File-local constants ---

// JS: const ITEM_SLOTS_IGNORED = [0, 18, 11, 12, 24, 25, 27, 28];
static const std::vector<int> ITEM_SLOTS_IGNORED = { 0, 18, 11, 12, 24, 25, 27, 28 };

// JS: const ITEM_QUALITIES = [ { id: 0, label: 'Poor' }, ... ];
struct QualityDef {
	int id;
	const char* label;
};

static const QualityDef ITEM_QUALITIES[] = {
	{ 0, "Poor" },
	{ 1, "Common" },
	{ 2, "Uncommon" },
	{ 3, "Rare" },
	{ 4, "Epic" },
	{ 5, "Legendary" },
	{ 6, "Artifact" },
	{ 7, "Heirloom" }
};

// JS: const ITEM_SLOTS_MERGED = { 'Head': [1], 'Neck': [2], ... };
// Using a vector of pairs to preserve insertion order (matching JS Object.keys() order).
struct SlotMergedEntry {
	const char* label;
	std::vector<int> slots;
};

static const SlotMergedEntry ITEM_SLOTS_MERGED[] = {
	{ "Head",      { 1 } },
	{ "Neck",      { 2 } },
	{ "Shoulder",  { 3 } },
	{ "Shirt",     { 4 } },
	{ "Chest",     { 5, 20 } },
	{ "Waist",     { 6 } },
	{ "Legs",      { 7 } },
	{ "Feet",      { 8 } },
	{ "Wrist",     { 9 } },
	{ "Hands",     { 10 } },
	{ "One-hand",  { 13 } },
	{ "Off-hand",  { 14, 22, 23 } },
	{ "Two-hand",  { 17 } },
	{ "Main-hand", { 21 } },
	{ "Ranged",    { 15, 26 } },
	{ "Back",      { 16 } },
	{ "Tabard",    { 19 } }
};

// --- File-local helper ---

static uint32_t fieldToUint32(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<uint32_t>(*p);
	return 0;
}

static std::string fieldToString(const db::FieldValue& val) {
	if (auto* p = std::get_if<std::string>(&val))
		return *p;
	return "";
}

static std::vector<uint32_t> fieldToUint32Vec(const db::FieldValue& val) {
	if (auto* p = std::get_if<std::vector<int64_t>>(&val)) {
		std::vector<uint32_t> result;
		result.reserve(p->size());
		for (int64_t v : *p)
			result.push_back(static_cast<uint32_t>(v));
		return result;
	}
	if (auto* p = std::get_if<std::vector<uint64_t>>(&val)) {
		std::vector<uint32_t> result;
		result.reserve(p->size());
		for (uint64_t v : *p)
			result.push_back(static_cast<uint32_t>(v));
		return result;
	}
	return {};
}

// --- File-local state ---

// JS: let items = [];
static std::vector<Item> items;

// JS: itemViewerTypeMask / itemViewerQualityMask — module-local filter state.
// (JS stores these on core.view but they are objects with {label, checked};
//  core.h declares them as std::vector<int> which is insufficient, so we keep
//  the real state here and sync back as needed.)
static std::vector<TypeMaskEntry> type_mask_entries;
static std::vector<QualityMaskEntry> quality_mask_entries;

// Change-detection for watches (replaces Vue $watch).
static std::vector<bool> prev_type_mask_checked;
static std::vector<bool> prev_quality_mask_checked;

static bool is_initialized = false;

// --- Internal functions ---

// JS: const view_item_models = (core, modules, item) => { ... }
static void view_item_models(const Item& item) {
	modules::set_active("tab_models");

	std::set<std::string> list;

	for (const uint32_t model_id : item.models) {
		const auto* file_data_ids = db::caches::DBModelFileData::getModelFileDataID(model_id);
		if (!file_data_ids)
			continue;

		for (const uint32_t file_data_id : *file_data_ids) {
			std::string entry = casc::listfile::getByID(file_data_id);

			if (!entry.empty())
				list.insert(std::format("{} [{}]", entry, file_data_id));
		}
	}

	auto& view = *core::view;
	view.userInputFilterModels = "";

	view.overrideModelList.clear();
	view.selectionModels.clear();
	for (const auto& s : list) {
		view.overrideModelList.push_back(s);
		view.selectionModels.push_back(s);
	}
	view.overrideModelName = item.name;
}

// JS: const view_item_textures = async (core, modules, item) => { ... }
static void view_item_textures(const Item& item) {
	modules::set_active("tab_textures");
	db::caches::DBTextureFileData::ensureInitialized();

	std::set<std::string> list;

	for (const uint32_t texture_id : item.textures) {
		const auto* file_data_ids = db::caches::DBTextureFileData::getTextureFDIDsByMatID(texture_id);
		if (!file_data_ids)
			continue;

		for (const uint32_t file_data_id : *file_data_ids) {
			std::string entry = casc::listfile::getByID(file_data_id);

			if (!entry.empty())
				list.insert(std::format("{} [{}]", entry, file_data_id));
		}
	}

	auto& view = *core::view;
	view.userInputFilterTextures = "";

	view.overrideTextureList.clear();
	view.selectionTextures.clear();
	for (const auto& s : list) {
		view.overrideTextureList.push_back(s);
		view.selectionTextures.push_back(s);
	}
	view.overrideTextureName = item.name;
}

// JS: const initialize_items = async (core) => { ... }
static void initialize_items() {
	items.clear();

	core::progressLoadingScreen("Loading model file data...");
	db::caches::DBModelFileData::initializeModelFileData();

	core::progressLoadingScreen("Loading item data...");
	db::caches::DBItems::ensureInitialized();

	// JS: const item_sparse_rows = await db2.ItemSparse.getAllRows();
	auto item_sparse_rows = casc::db2::preloadTable("ItemSparse").getAllRows();

	// JS: const appearance_map = new Map();
	// JS: for (const row of (await db2.ItemModifiedAppearance.getAllRows()).values())
	//         appearance_map.set(row.ItemID, row.ItemAppearanceID);
	std::unordered_map<uint32_t, uint32_t> appearance_map;
	for (const auto& [_id, row] : casc::db2::preloadTable("ItemModifiedAppearance").getAllRows()) {
		uint32_t itemID = fieldToUint32(row.at("ItemID"));
		uint32_t itemAppearanceID = fieldToUint32(row.at("ItemAppearanceID"));
		appearance_map[itemID] = itemAppearanceID;
	}

	// JS: const material_map = new MultiMap();
	// JS: for (const row of (await db2.ItemDisplayInfoMaterialRes.getAllRows()).values())
	//         material_map.set(row.ItemDisplayInfoID, row.MaterialResourcesID);
	MultiMap<uint32_t, uint32_t> material_map;
	for (const auto& [_id, row] : casc::db2::preloadTable("ItemDisplayInfoMaterialRes").getAllRows()) {
		uint32_t displayInfoID = fieldToUint32(row.at("ItemDisplayInfoID"));
		uint32_t materialResID = fieldToUint32(row.at("MaterialResourcesID"));
		material_map.set(displayInfoID, materialResID);
	}

	auto& item_appearance_table = casc::db2::preloadTable("ItemAppearance");
	auto& item_display_info_table = casc::db2::preloadTable("ItemDisplayInfo");

	for (const auto& [item_id, item_row] : item_sparse_rows) {
		int inventoryType = static_cast<int>(fieldToUint32(item_row.at("InventoryType")));

		// JS: if (ITEM_SLOTS_IGNORED.includes(item_row.inventoryType)) continue;
		if (std::find(ITEM_SLOTS_IGNORED.begin(), ITEM_SLOTS_IGNORED.end(), inventoryType) != ITEM_SLOTS_IGNORED.end())
			continue;

		Item new_item;
		new_item.id = item_id;
		new_item.inventoryType = inventoryType;

		// JS: this.name = item_sparse_row.Display_lang;
		std::string display_name = fieldToString(item_row.at("Display_lang"));
		new_item.name = display_name.empty() ? std::format("Unknown item #{}", item_id) : std::move(display_name);

		// JS: this.quality = item_sparse_row.OverallQualityID ?? 0;
		auto qualIt = item_row.find("OverallQualityID");
		new_item.quality = (qualIt != item_row.end()) ? static_cast<int>(fieldToUint32(qualIt->second)) : 0;

		// JS: const item_appearance_id = appearance_map.get(item_id);
		// JS: const item_appearance_row = await db2.ItemAppearance.getRow(item_appearance_id);
		auto app_it = appearance_map.find(item_id);
		std::optional<db::DataRecord> item_appearance_row;
		if (app_it != appearance_map.end())
			item_appearance_row = item_appearance_table.getRow(app_it->second);

		// JS: this.icon = item_appearance_row?.DefaultIconFileDataID ?? 0;
		if (item_appearance_row.has_value()) {
			auto iconIt = item_appearance_row->find("DefaultIconFileDataID");
			new_item.icon = (iconIt != item_appearance_row->end()) ? fieldToUint32(iconIt->second) : 0;
		}

		// JS: if (this.icon == 0) this.icon = item_sparse_row.IconFileDataID;
		if (new_item.icon == 0) {
			auto sparseIconIt = item_row.find("IconFileDataID");
			if (sparseIconIt != item_row.end())
				new_item.icon = fieldToUint32(sparseIconIt->second);
		}

		// JS: if (item_appearance_row !== null) { materials = []; models = []; ... }
		if (item_appearance_row.has_value()) {
			uint32_t displayInfoID = fieldToUint32(item_appearance_row->at("ItemDisplayInfoID"));
			auto item_display_info_row = item_display_info_table.getRow(displayInfoID);

			if (item_display_info_row.has_value()) {
				// JS: materials.push(...item_display_info_row.ModelMaterialResourcesID);
				auto matIt = item_display_info_row->find("ModelMaterialResourcesID");
				if (matIt != item_display_info_row->end()) {
					auto vec = fieldToUint32Vec(matIt->second);
					new_item.textures.insert(new_item.textures.end(), vec.begin(), vec.end());
				}

				// JS: models.push(...item_display_info_row.ModelResourcesID);
				auto modIt = item_display_info_row->find("ModelResourcesID");
				if (modIt != item_display_info_row->end()) {
					auto vec = fieldToUint32Vec(modIt->second);
					new_item.models.insert(new_item.models.end(), vec.begin(), vec.end());
				}
			}

			// JS: const material_res = material_map.get(item_appearance_row.ItemDisplayInfoID);
			// JS: if (material_res !== undefined)
			//         Array.isArray(material_res) ? materials.push(...material_res) : materials.push(material_res);
			const auto* material_res = material_map.get(displayInfoID);
			if (material_res) {
				if (auto* single = std::get_if<uint32_t>(material_res)) {
					new_item.textures.push_back(*single);
				} else if (auto* multi = std::get_if<std::vector<uint32_t>>(material_res)) {
					new_item.textures.insert(new_item.textures.end(), multi->begin(), multi->end());
				}
			}

			// JS: materials = materials.filter(e => e !== 0);
			std::erase(new_item.textures, 0u);
			// JS: models = models.filter(e => e !== 0);
			std::erase(new_item.models, 0u);
		}

		new_item.modelCount = static_cast<int>(new_item.models.size());
		new_item.textureCount = static_cast<int>(new_item.textures.size());

		items.push_back(std::move(new_item));
	}

	// JS: if (core.view.config.itemViewerShowAll) { ... }
	if (core::view->config.value("itemViewerShowAll", false)) {
		auto& item_db = casc::db2::preloadTable("Item");

		for (const auto& [item_id, item_row] : item_db.getAllRows()) {
			int inventoryType = static_cast<int>(fieldToUint32(item_row.at("InventoryType")));

			// JS: if (ITEM_SLOTS_IGNORED.includes(item_row.inventoryType)) continue;
			if (std::find(ITEM_SLOTS_IGNORED.begin(), ITEM_SLOTS_IGNORED.end(), inventoryType) != ITEM_SLOTS_IGNORED.end())
				continue;

			// JS: if (item_sparse_rows.has(item_id)) continue;
			if (item_sparse_rows.contains(item_id))
				continue;

			// For items only in the Item table (not ItemSparse), create with minimal data.
			// JS: items.push(Object.freeze(new Item(item_id, item_row, null, null, null)));
			Item new_item;
			new_item.id = item_id;
			new_item.inventoryType = inventoryType;

			// Item table may not have Display_lang; use name from DBItems cache or generate.
			const auto* cached_info = db::caches::DBItems::getItemById(item_id);
			if (cached_info && !cached_info->name.empty())
				new_item.name = cached_info->name;
			else
				new_item.name = std::format("Unknown item #{}", item_id);

			new_item.quality = 0;
			new_item.icon = 0;
			new_item.modelCount = 0;
			new_item.textureCount = 0;

			items.push_back(std::move(new_item));
		}
	}

	logging::write(std::format("Loaded {} items", items.size()));
}

// JS: const apply_filters = (core) => { ... }
static void apply_filters() {
	auto& view = *core::view;

	// JS: const type_filter = core.view.itemViewerTypeMask.filter(e => e.checked);
	// JS: const type_mask = [];
	// JS: type_filter.forEach(e => type_mask.push(...ITEM_SLOTS_MERGED[e.label]));
	std::vector<int> type_mask;
	for (const auto& entry : type_mask_entries) {
		if (!entry.checked)
			continue;

		for (const auto& merged : ITEM_SLOTS_MERGED) {
			if (entry.label == merged.label) {
				type_mask.insert(type_mask.end(), merged.slots.begin(), merged.slots.end());
				break;
			}
		}
	}

	// JS: const quality_mask = core.view.itemViewerQualityMask.filter(e => e.checked).map(e => e.id);
	std::vector<int> quality_mask;
	for (const auto& entry : quality_mask_entries) {
		if (entry.checked)
			quality_mask.push_back(entry.id);
	}

	// JS: const filtered = items.filter(item => type_mask.includes(item.inventoryType) && quality_mask.includes(item.quality));
	view.listfileItems.clear();
	for (const auto& item : items) {
		bool type_match = std::find(type_mask.begin(), type_mask.end(), item.inventoryType) != type_mask.end();
		bool quality_match = std::find(quality_mask.begin(), quality_mask.end(), item.quality) != quality_mask.end();

		if (type_match && quality_match) {
			nlohmann::json j;
			j["id"] = item.id;
			j["name"] = item.name;
			j["displayName"] = item.displayName();
			j["icon"] = item.icon;
			j["quality"] = item.quality;
			j["inventoryType"] = item.inventoryType;
			j["modelCount"] = item.modelCount;
			j["textureCount"] = item.textureCount;
			view.listfileItems.push_back(std::move(j));
		}
	}

	// JS: core.view.config.itemViewerEnabledTypes = core.view.itemViewerTypeMask.filter(e => e.checked).map(e => e.label);
	nlohmann::json enabled_types = nlohmann::json::array();
	for (const auto& entry : type_mask_entries) {
		if (entry.checked)
			enabled_types.push_back(entry.label);
	}
	view.config["itemViewerEnabledTypes"] = enabled_types;

	// JS: core.view.config.itemViewerEnabledQualities = quality_mask;
	view.config["itemViewerEnabledQualities"] = quality_mask;
}

// Looks up an Item by id from the module-local items vector.
static const Item* find_item_by_id(uint32_t item_id) {
	for (const auto& item : items) {
		if (item.id == item_id)
			return &item;
	}
	return nullptr;
}

// --- methods ---

// JS: methods.copy_to_clipboard(value)
static void copy_to_clipboard(const std::string& value) {
	ImGui::SetClipboardText(value.c_str());
}

// Removed: view_on_wowhead() — external-links module deleted
// JS: view_on_wowhead(item_id) { return; }
static void view_on_wowhead([[maybe_unused]] uint32_t item_id) {
	return;
}

// JS: methods.toggle_checklist_item(item) { item.checked = !item.checked; }
// (Handled inline in render via ImGui::Checkbox)

// JS: methods.equip_item(item)
static void equip_item(const nlohmann::json& item_json) {
	uint32_t item_id = item_json.value("id", 0u);
	std::string item_name = item_json.value("name", std::string("Unknown"));

	auto slot_id_opt = db::caches::DBItems::getItemSlotId(item_id);
	if (!slot_id_opt.has_value()) {
		core::setToast("info", "This item cannot be equipped.", {}, 2000);
		return;
	}

	int slot_id = slot_id_opt.value();
	auto& view = *core::view;

	// JS: this.$core.view.chrEquippedItems[slot_id] = item.id;
	view.chrEquippedItems[std::to_string(slot_id)] = item_id;

	// JS: const slot_name = get_slot_name(slot_id);
	auto slot_name_opt = wow::get_slot_name(slot_id);
	std::string slot_name = slot_name_opt.has_value() ? std::string(slot_name_opt.value()) : "Unknown";

	// JS: this.$core.setToast('success', `Equipped ${item.name} to ${slot_name} slot.`, null, 2000);
	core::setToast("success", std::format("Equipped {} to {} slot.", item_name, slot_name), {}, 2000);
}

// --- Public API ---

// JS: register() { this.registerNavButton('Items', 'sword.svg', InstallType.CASC); }
void registerTab() {
	modules::register_nav_button("tab_items", "Items", "sword.svg", install_type::CASC);
}

// JS: async mounted() { await this.initialize(); }
void mounted() {
	auto& view = *core::view;

	// JS: this.$core.showLoadingScreen(2);
	core::showLoadingScreen(2);

	// JS: await initialize_items(this.$core);
	initialize_items();

	// JS: this.$core.hideLoadingScreen();
	core::hideLoadingScreen();

	// JS: const enabled_types = this.$core.view.config.itemViewerEnabledTypes;
	nlohmann::json enabled_types_json = view.config.value("itemViewerEnabledTypes", nlohmann::json::array());

	// JS: const pending_slot = this.$core.view.pendingItemSlotFilter;
	const std::string pending_slot = view.pendingItemSlotFilter;

	// JS: const type_mask = [];
	// JS: for (const label of Object.keys(ITEM_SLOTS_MERGED)) { ... }
	type_mask_entries.clear();
	for (const auto& merged : ITEM_SLOTS_MERGED) {
		TypeMaskEntry entry;
		entry.label = merged.label;

		if (!pending_slot.empty()) {
			// JS: type_mask.push({ label, checked: label === pending_slot });
			entry.checked = (entry.label == pending_slot);
		} else {
			// JS: type_mask.push({ label, checked: enabled_types.includes(label) });
			entry.checked = false;
			for (const auto& et : enabled_types_json) {
				if (et.is_string() && et.get<std::string>() == entry.label) {
					entry.checked = true;
					break;
				}
			}
		}

		type_mask_entries.push_back(std::move(entry));
	}

	// JS: this.$core.view.pendingItemSlotFilter = null;
	view.pendingItemSlotFilter.clear();

	// JS: const enabled_qualities = this.$core.view.config.itemViewerEnabledQualities;
	nlohmann::json enabled_qualities_json = view.config.value("itemViewerEnabledQualities", nlohmann::json());

	// JS: const quality_mask = ITEM_QUALITIES.map(q => ({
	//         id: q.id, label: q.label,
	//         checked: enabled_qualities === undefined || enabled_qualities.includes(q.id)
	//     }));
	quality_mask_entries.clear();
	for (const auto& q : ITEM_QUALITIES) {
		QualityMaskEntry entry;
		entry.id = q.id;
		entry.label = q.label;

		if (enabled_qualities_json.is_null() || !enabled_qualities_json.is_array()) {
			entry.checked = true;
		} else {
			entry.checked = false;
			for (const auto& eq : enabled_qualities_json) {
				if (eq.is_number_integer() && eq.get<int>() == q.id) {
					entry.checked = true;
					break;
				}
			}
		}

		quality_mask_entries.push_back(std::move(entry));
	}

	// Store initial state for change-detection (replaces Vue $watch).
	prev_type_mask_checked.clear();
	for (const auto& e : type_mask_entries)
		prev_type_mask_checked.push_back(e.checked);

	prev_quality_mask_checked.clear();
	for (const auto& e : quality_mask_entries)
		prev_quality_mask_checked.push_back(e.checked);

	// Initial filter application.
	// JS: this.$core.view.itemViewerQualityMask = quality_mask;
	// JS: this.$core.view.itemViewerTypeMask = type_mask;
	// (Vue watches fire immediately on assignment — replicate by calling apply_filters.)
	apply_filters();

	is_initialized = true;
}

void setActive() {
	modules::set_active("tab_items");
}

void render() {
	auto& view = *core::view;

	if (!is_initialized)
		return;

	// --- Change-detection for type and quality mask watches ---
	// JS: this.$core.view.$watch('itemViewerTypeMask', () => apply_filters(this.$core), { deep: true });
	// JS: this.$core.view.$watch('itemViewerQualityMask', () => apply_filters(this.$core), { deep: true });
	{
		bool type_changed = false;
		if (prev_type_mask_checked.size() == type_mask_entries.size()) {
			for (size_t i = 0; i < type_mask_entries.size(); ++i) {
				if (type_mask_entries[i].checked != prev_type_mask_checked[i]) {
					type_changed = true;
					break;
				}
			}
		} else {
			type_changed = true;
		}

		bool quality_changed = false;
		if (prev_quality_mask_checked.size() == quality_mask_entries.size()) {
			for (size_t i = 0; i < quality_mask_entries.size(); ++i) {
				if (quality_mask_entries[i].checked != prev_quality_mask_checked[i]) {
					quality_changed = true;
					break;
				}
			}
		} else {
			quality_changed = true;
		}

		if (type_changed || quality_changed) {
			apply_filters();

			prev_type_mask_checked.clear();
			for (const auto& e : type_mask_entries)
				prev_type_mask_checked.push_back(e.checked);

			prev_quality_mask_checked.clear();
			for (const auto& e : quality_mask_entries)
				prev_quality_mask_checked.push_back(e.checked);
		}
	}

	// --- Template rendering ---

	// JS: <div class="tab" id="tab-items">

	// JS: <div class="list-container">
	//     <Itemlistbox id="listbox-items" ... @options="contextMenus.nodeItem = $event" @equip="equip_item">
	ImGui::BeginChild("items-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
	// TODO(conversion): Itemlistbox component rendering will be wired when integration is complete.
	ImGui::Text("Items: %zu", view.listfileItems.size());
	ImGui::EndChild();

	ImGui::SameLine();

	// JS: <div id="items-sidebar" class="sidebar">
	ImGui::BeginChild("items-sidebar", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	// JS: <span class="header">Item Types</span>
	ImGui::SeparatorText("Item Types");

	// JS: <div class="sidebar-checklist">
	//     <div v-for="item in $core.view.itemViewerTypeMask" ...>
	//         <input type="checkbox" v-model="item.checked" @click.stop/>
	//         <span>{{ item.label }}</span>
	//     </div>
	for (auto& entry : type_mask_entries)
		ImGui::Checkbox(entry.label.c_str(), &entry.checked);

	// JS: <div class="list-toggles">
	//     <a @click="setAllItemTypes(true)">Enable All</a> / <a @click="setAllItemTypes(false)">Disable All</a>
	if (ImGui::SmallButton("Enable All##types")) {
		for (auto& entry : type_mask_entries)
			entry.checked = true;
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("/");
	ImGui::SameLine();
	if (ImGui::SmallButton("Disable All##types")) {
		for (auto& entry : type_mask_entries)
			entry.checked = false;
	}

	ImGui::Spacing();

	// JS: <span class="header">Quality</span>
	ImGui::SeparatorText("Quality");

	// JS: <div class="sidebar-checklist">
	//     <div v-for="item in $core.view.itemViewerQualityMask" ...>
	//         <input type="checkbox" v-model="item.checked" :class="'quality-' + item.id" @click.stop/>
	//         <span>{{ item.label }}</span>
	//     </div>
	// Quality color mapping (matching WoW quality colors from app.css).
	static const ImVec4 quality_colors[] = {
		ImVec4(0.62f, 0.62f, 0.62f, 1.0f), // 0 = Poor (grey)
		ImVec4(1.00f, 1.00f, 1.00f, 1.0f), // 1 = Common (white)
		ImVec4(0.12f, 1.00f, 0.00f, 1.0f), // 2 = Uncommon (green)
		ImVec4(0.00f, 0.44f, 0.87f, 1.0f), // 3 = Rare (blue)
		ImVec4(0.64f, 0.21f, 0.93f, 1.0f), // 4 = Epic (purple)
		ImVec4(1.00f, 0.50f, 0.00f, 1.0f), // 5 = Legendary (orange)
		ImVec4(0.90f, 0.80f, 0.50f, 1.0f), // 6 = Artifact (gold)
		ImVec4(0.00f, 0.80f, 1.00f, 1.0f), // 7 = Heirloom (cyan)
	};

	for (auto& entry : quality_mask_entries) {
		int idx = entry.id;
		if (idx >= 0 && idx < static_cast<int>(std::size(quality_colors)))
			ImGui::PushStyleColor(ImGuiCol_CheckMark, quality_colors[idx]);

		ImGui::Checkbox(std::format("{}##quality", entry.label).c_str(), &entry.checked);

		if (idx >= 0 && idx < static_cast<int>(std::size(quality_colors)))
			ImGui::PopStyleColor();
	}

	// JS: <div class="list-toggles">
	//     <a @click="setAllItemQualities(true)">Enable All</a> / <a @click="setAllItemQualities(false)">Disable All</a>
	if (ImGui::SmallButton("Enable All##qualities")) {
		for (auto& entry : quality_mask_entries)
			entry.checked = true;
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("/");
	ImGui::SameLine();
	if (ImGui::SmallButton("Disable All##qualities")) {
		for (auto& entry : quality_mask_entries)
			entry.checked = false;
	}

	ImGui::EndChild(); // items-sidebar

	// JS: <div class="filter">
	//     <div class="regex-info" v-if="$core.view.config.regexFilters" ...>Regex Enabled</div>
	//     <input type="text" v-model="$core.view.userInputFilterItems" placeholder="Filter items..."/>
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterItems.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterItems", filter_buf, sizeof(filter_buf)))
		view.userInputFilterItems = filter_buf;

	// --- Context menu ---
	// JS: <ContextMenu :node="contextMenus.nodeItem" ...>
	//     <span v-if="context.node.modelCount > 0" @click.self="view_models(context.node)">View related models ({{ context.node.modelCount }})</span>
	//     <span v-if="context.node.textureCount > 0" @click.self="view_textures(context.node)">View related textures ({{ context.node.textureCount }})</span>
	//     <span @click.self="copy_to_clipboard(context.node.name)">Copy item name to clipboard</span>
	//     <span @click.self="copy_to_clipboard(context.node.id)">Copy item ID to clipboard</span>
	//     <span @click.self="view_on_wowhead(context.node.id)">View item on Wowhead (web)</span>
	// TODO(conversion): ContextMenu component rendering will be wired when integration is complete.
	// For now, implement as an ImGui popup context menu.
	if (!view.contextMenus.nodeItem.is_null()) {
		const auto& node = view.contextMenus.nodeItem;

		if (ImGui::BeginPopup("ItemContextMenu")) {
			int model_count = node.value("modelCount", 0);
			int texture_count = node.value("textureCount", 0);
			uint32_t node_id = node.value("id", 0u);
			std::string node_name = node.value("name", std::string(""));

			// JS: <span v-if="context.node.modelCount > 0" ...>View related models</span>
			if (model_count > 0) {
				if (ImGui::MenuItem(std::format("View related models ({})", model_count).c_str())) {
					const Item* item = find_item_by_id(node_id);
					if (item)
						view_item_models(*item);
					view.contextMenus.nodeItem = nullptr;
				}
			}

			// JS: <span v-if="context.node.textureCount > 0" ...>View related textures</span>
			if (texture_count > 0) {
				if (ImGui::MenuItem(std::format("View related textures ({})", texture_count).c_str())) {
					const Item* item = find_item_by_id(node_id);
					if (item)
						view_item_textures(*item);
					view.contextMenus.nodeItem = nullptr;
				}
			}

			// JS: <span @click.self="copy_to_clipboard(context.node.name)">Copy item name to clipboard</span>
			if (ImGui::MenuItem("Copy item name to clipboard")) {
				copy_to_clipboard(node_name);
				view.contextMenus.nodeItem = nullptr;
			}

			// JS: <span @click.self="copy_to_clipboard(context.node.id)">Copy item ID to clipboard</span>
			if (ImGui::MenuItem("Copy item ID to clipboard")) {
				copy_to_clipboard(std::to_string(node_id));
				view.contextMenus.nodeItem = nullptr;
			}

			// JS: <span @click.self="view_on_wowhead(context.node.id)">View item on Wowhead (web)</span>
			if (ImGui::MenuItem("View item on Wowhead (web)")) {
				view_on_wowhead(node_id);
				view.contextMenus.nodeItem = nullptr;
			}

			ImGui::EndPopup();
		}
	}
}

} // namespace tab_items
