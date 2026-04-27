/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_items.h"
#include "../log.h"
#include "../core.h"
#include <thread>
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
#include "../external-links.h"
#include "../modules.h"
#include "../components/context-menu.h"
#include "../components/itemlistbox.h"
#include "../../app.h"

#include <cstring>
#include <algorithm>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_items {

// --- File-local structures ---

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

	std::string_view itemSlotName() const {
		return wow::getSlotName(inventoryType);
	}

	std::string displayName() const {
		return std::format("{} ({})", name, id);
	}
};

struct TypeMaskEntry {
	std::string label;
	bool checked = false;
};

struct QualityMaskEntry {
	int id = 0;
	std::string label;
	bool checked = false;
};

// --- File-local constants ---

static const std::vector<int> ITEM_SLOTS_IGNORED = { 0, 18, 11, 12, 24, 25, 27, 28 };

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

static std::vector<Item> items;

// (JS stores these on core.view but they are objects with {label, checked};
//  core.h declares them as std::vector<int> which is insufficient, so we keep
//  the real state here and sync back as needed.)
static std::vector<TypeMaskEntry> type_mask_entries;
static std::vector<QualityMaskEntry> quality_mask_entries;

// Change-detection for watches (replaces Vue $watch).
static std::vector<bool> prev_type_mask_checked;
static std::vector<bool> prev_quality_mask_checked;

static bool is_initialized = false;
static bool is_mounting = false;
static context_menu::ContextMenuState context_menu_item_state;
static itemlistbox::ItemListboxState itemlistbox_items_state;

// Cached ItemEntry vector — only rebuilt when the source JSON changes.
static std::vector<itemlistbox::ItemEntry> s_item_entries_cache;
static size_t s_item_entries_cache_size = ~size_t(0);

// --- Internal functions ---

static void view_item_models(const Item& item) {
	modules::set_active("tab_models");

	// JS uses Set which preserves insertion order; use vector with uniqueness check.
	std::vector<std::string> list;

	for (const uint32_t model_id : item.models) {
		const auto* file_data_ids = db::caches::DBModelFileData::getModelFileDataID(model_id);
		if (!file_data_ids)
			continue;

		for (const uint32_t file_data_id : *file_data_ids) {
			std::string entry = casc::listfile::getByID(file_data_id).value_or("");

			if (!entry.empty()) {
				std::string s = std::format("{} [{}]", entry, file_data_id);
				if (std::find(list.begin(), list.end(), s) == list.end())
					list.push_back(std::move(s));
			}
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

static void view_item_textures(const Item& item) {
	modules::set_active("tab_textures");
	db::caches::DBTextureFileData::ensureInitialized();

	// JS uses Set which preserves insertion order; use vector with uniqueness check.
	std::vector<std::string> list;

	for (const uint32_t texture_id : item.textures) {
		const auto* file_data_ids = db::caches::DBTextureFileData::getTextureFDIDsByMatID(texture_id);
		if (!file_data_ids)
			continue;

		for (const uint32_t file_data_id : *file_data_ids) {
			std::string entry = casc::listfile::getByID(file_data_id).value_or("");

			if (!entry.empty()) {
				std::string s = std::format("{} [{}]", entry, file_data_id);
				if (std::find(list.begin(), list.end(), s) == list.end())
					list.push_back(std::move(s));
			}
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

static void initialize_items() {
	items.clear();

	core::progressLoadingScreen("Loading model file data...");
	db::caches::DBModelFileData::initializeModelFileData();

	core::progressLoadingScreen("Loading item data...");
	db::caches::DBItems::ensureInitialized();

	auto item_sparse_rows = casc::db2::preloadTable("ItemSparse").getAllRows();

	//         appearance_map.set(row.ItemID, row.ItemAppearanceID);
	std::unordered_map<uint32_t, uint32_t> appearance_map;
	for (const auto& [_id, row] : casc::db2::preloadTable("ItemModifiedAppearance").getAllRows()) {
		uint32_t itemID = fieldToUint32(row.at("ItemID"));
		uint32_t itemAppearanceID = fieldToUint32(row.at("ItemAppearanceID"));
		appearance_map[itemID] = itemAppearanceID;
	}

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

		if (std::find(ITEM_SLOTS_IGNORED.begin(), ITEM_SLOTS_IGNORED.end(), inventoryType) != ITEM_SLOTS_IGNORED.end())
			continue;

		Item new_item;
		new_item.id = item_id;
		new_item.inventoryType = inventoryType;

		std::string display_name = fieldToString(item_row.at("Display_lang"));
		new_item.name = display_name.empty() ? std::format("Unknown item #{}", item_id) : std::move(display_name);

		auto qualIt = item_row.find("OverallQualityID");
		new_item.quality = (qualIt != item_row.end()) ? static_cast<int>(fieldToUint32(qualIt->second)) : 0;

		auto app_it = appearance_map.find(item_id);
		std::optional<db::DataRecord> item_appearance_row;
		if (app_it != appearance_map.end())
			item_appearance_row = item_appearance_table.getRow(app_it->second);

		if (item_appearance_row.has_value()) {
			auto iconIt = item_appearance_row->find("DefaultIconFileDataID");
			new_item.icon = (iconIt != item_appearance_row->end()) ? fieldToUint32(iconIt->second) : 0;
		}

		if (new_item.icon == 0) {
			auto sparseIconIt = item_row.find("IconFileDataID");
			if (sparseIconIt != item_row.end())
				new_item.icon = fieldToUint32(sparseIconIt->second);
		}

		if (item_appearance_row.has_value()) {
			uint32_t displayInfoID = fieldToUint32(item_appearance_row->at("ItemDisplayInfoID"));
			auto item_display_info_row = item_display_info_table.getRow(displayInfoID);

			if (item_display_info_row.has_value()) {
				auto matIt = item_display_info_row->find("ModelMaterialResourcesID");
				if (matIt != item_display_info_row->end()) {
					auto vec = fieldToUint32Vec(matIt->second);
					new_item.textures.insert(new_item.textures.end(), vec.begin(), vec.end());
				}

				auto modIt = item_display_info_row->find("ModelResourcesID");
				if (modIt != item_display_info_row->end()) {
					auto vec = fieldToUint32Vec(modIt->second);
					new_item.models.insert(new_item.models.end(), vec.begin(), vec.end());
				}
			}

			//         Array.isArray(material_res) ? materials.push(...material_res) : materials.push(material_res);
			const auto* material_res = material_map.get(displayInfoID);
			if (material_res) {
				if (auto* single = std::get_if<uint32_t>(material_res)) {
					new_item.textures.push_back(*single);
				} else if (auto* multi = std::get_if<std::vector<uint32_t>>(material_res)) {
					new_item.textures.insert(new_item.textures.end(), multi->begin(), multi->end());
				}
			}

			std::erase(new_item.textures, 0u);
			std::erase(new_item.models, 0u);
		}

		new_item.modelCount = static_cast<int>(new_item.models.size());
		new_item.textureCount = static_cast<int>(new_item.textures.size());

		items.push_back(std::move(new_item));
	}

	if (core::view->config.value("itemViewerShowAll", false)) {
		auto& item_db = casc::db2::preloadTable("Item");

		for (const auto& [item_id, item_row] : item_db.getAllRows()) {
			int inventoryType = static_cast<int>(fieldToUint32(item_row.at("InventoryType")));

			if (std::find(ITEM_SLOTS_IGNORED.begin(), ITEM_SLOTS_IGNORED.end(), inventoryType) != ITEM_SLOTS_IGNORED.end())
				continue;

			if (item_sparse_rows.contains(item_id))
				continue;

			// For items only in the Item table (not ItemSparse), create with minimal data.
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

static void apply_filters() {
	auto& view = *core::view;

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

	std::vector<int> quality_mask;
	for (const auto& entry : quality_mask_entries) {
		if (entry.checked)
			quality_mask.push_back(entry.id);
	}

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

	nlohmann::json enabled_types = nlohmann::json::array();
	for (const auto& entry : type_mask_entries) {
		if (entry.checked)
			enabled_types.push_back(entry.label);
	}
	view.config["itemViewerEnabledTypes"] = enabled_types;

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

static void copy_to_clipboard(const std::string& value) {
	ImGui::SetClipboardText(value.c_str());
}

static void view_on_wowhead(uint32_t item_id) {
	ExternalLinks::wowHead_viewItem(static_cast<int>(item_id));
}

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

	view.chrEquippedItems[std::to_string(slot_id)] = item_id;

	auto slot_name_opt = wow::get_slot_name(slot_id);
	std::string slot_name = slot_name_opt.has_value() ? std::string(slot_name_opt.value()) : "Unknown";

	core::setToast("success", std::format("Equipped {} to {} slot.", item_name, slot_name), {}, 2000);
}

// --- Public API ---

void registerTab() {
	modules::register_nav_button("tab_items", "Items", "sword.svg", install_type::CASC);
}

void mounted() {
	if (is_mounting) return;
	is_mounting = true;

	std::thread([]() {
		core::showLoadingScreen(2);

		initialize_items();

		core::postToMainThread([]() {
			auto& view = *core::view;

			nlohmann::json enabled_types_json = view.config.value("itemViewerEnabledTypes", nlohmann::json::array());
			const std::string pending_slot = view.pendingItemSlotFilter;

			type_mask_entries.clear();
			for (const auto& merged : ITEM_SLOTS_MERGED) {
				TypeMaskEntry entry;
				entry.label = merged.label;

				if (!pending_slot.empty()) {
					entry.checked = (entry.label == pending_slot);
				} else {
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

			view.pendingItemSlotFilter.clear();

			nlohmann::json enabled_qualities_json = view.config.value("itemViewerEnabledQualities", nlohmann::json());

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
			// (Vue watches fire immediately on assignment — replicate by calling apply_filters.)
			apply_filters();

			is_initialized = true;
			is_mounting = false;
		});

		core::hideLoadingScreen();
	}).detach();
}

void setActive() {
	modules::set_active("tab_items");
}

void render() {
	auto& view = *core::view;

	if (!is_initialized)
		return;

	// --- Change-detection for type and quality mask watches ---
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

	if (app::layout::BeginTab("tab-items")) {

	// Calculate layout manually since items uses 1fr auto (not standard 2-col list-tab).
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const ImVec2 cursor = ImGui::GetCursorPos();

	constexpr float SIDEBAR_W = app::layout::SIDEBAR_WIDTH; // 210px
	constexpr float FILTER_H = 70.0f;

	const float listW = avail.x - SIDEBAR_W;
	const float topH = avail.y - FILTER_H;

	// --- List container (row 1, col 1) ---
	//     <Itemlistbox id="listbox-items" ... @options="contextMenus.nodeItem = $event" @equip="equip_item">
	ImGui::SetCursorPos(ImVec2(cursor.x + app::layout::LIST_MARGIN_LEFT,
	                           cursor.y + app::layout::LIST_MARGIN_TOP));
	ImGui::BeginChild("items-list-container",
		ImVec2(listW - app::layout::LIST_MARGIN_LEFT - app::layout::LIST_MARGIN_RIGHT,
		       topH - app::layout::LIST_MARGIN_TOP));
	{
		// Convert json items to ItemEntry array — only when the source changes.
		if (view.listfileItems.size() != s_item_entries_cache_size) {
			s_item_entries_cache_size = view.listfileItems.size();
			s_item_entries_cache.clear();
			s_item_entries_cache.reserve(s_item_entries_cache_size);
			for (const auto& j : view.listfileItems) {
				itemlistbox::ItemEntry entry;
				entry.id = j.value("id", 0);
				entry.name = j.value("name", std::string(""));
				entry.displayName = j.value("displayName", std::string(""));
				entry.icon = j.value("icon", 0u);
				entry.quality = j.value("quality", 0);
				s_item_entries_cache.push_back(std::move(entry));
			}
		}
		const auto& item_entries = s_item_entries_cache;

		// Build selection as item IDs.
		std::vector<int> sel_ids;
		for (const auto& sel : view.selectionItems)
			sel_ids.push_back(sel.value("id", 0));

		itemlistbox::render("##ItemListbox", item_entries,
			view.userInputFilterItems, sel_ids, false, true,
			view.config.value("regexFilters", false),
			"item",
			itemlistbox_items_state,
			[&](const std::vector<int>& new_sel) {
				view.selectionItems.clear();
				for (int id : new_sel) {
					for (const auto& j : view.listfileItems) {
						if (j.value("id", 0) == id) {
							view.selectionItems.push_back(j);
							break;
						}
					}
				}
			},
			[&](const itemlistbox::ItemEntry& item) {
				// @equip — find matching json and call equip_item
				for (const auto& j : view.listfileItems) {
					if (j.value("id", 0) == item.id) {
						equip_item(j);
						break;
					}
				}
			},
			[&](const itemlistbox::ItemEntry& item) {
				// @options — set context menu node
				for (const auto& j : view.listfileItems) {
					if (j.value("id", 0) == item.id) {
						view.contextMenus.nodeItem = j;
						break;
					}
				}
			});
	}
	ImGui::EndChild();

	// --- Sidebar (col 2, spanning both rows) ---
	ImGui::SetCursorPos(ImVec2(cursor.x + listW,
	                           cursor.y + app::layout::SIDEBAR_MARGIN_TOP));
	ImGui::BeginChild("items-sidebar",
		ImVec2(SIDEBAR_W - app::layout::SIDEBAR_PADDING_RIGHT,
		       avail.y - app::layout::SIDEBAR_MARGIN_TOP));

	// JS: <span class="header">Item Types</span>
	ImGui::TextUnformatted("Item Types");

	//     <div v-for="item in $core.view.itemViewerTypeMask" class="sidebar-checklist-item"
	//          :class="{ selected: item.checked }" @click="toggle_checklist_item(item)">
	//         <input type="checkbox" v-model="item.checked" @click.stop/>
	//         <span>{{ item.label }}</span>
	//     </div>
	for (auto& entry : type_mask_entries) {
		ImVec2 pos = ImGui::GetCursorPos();
		// Full-width selectable provides .selected-class highlight and whole-row click.
		bool row_toggled = ImGui::Selectable(
			std::format("##rowtype_{}", entry.label).c_str(),
			entry.checked, ImGuiSelectableFlags_AllowOverlap);
		ImGui::SetCursorPos(pos);
		bool cb_changed = ImGui::Checkbox(entry.label.c_str(), &entry.checked);
		if (row_toggled && !cb_changed)
			entry.checked = !entry.checked;
	}

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
	ImGui::TextUnformatted("Quality");

	//     <div v-for="item in $core.view.itemViewerQualityMask" class="sidebar-checklist-item"
	//          :class="{ selected: item.checked }" @click="toggle_checklist_item(item)">
	//         <input type="checkbox" v-model="item.checked" :class="'quality-' + item.id" @click.stop/>
	//         <span>{{ item.label }}</span>
	//     </div>
	// WoW quality colors — applied to both the checkmark and the label text.
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
		bool has_color = (idx >= 0 && idx < static_cast<int>(std::size(quality_colors)));

		ImVec2 pos = ImGui::GetCursorPos();
		bool row_toggled = ImGui::Selectable(
			std::format("##rowquality_{}", entry.label).c_str(),
			entry.checked, ImGuiSelectableFlags_AllowOverlap);
		ImGui::SetCursorPos(pos);

		if (has_color) {
			ImGui::PushStyleColor(ImGuiCol_CheckMark, quality_colors[idx]);
			ImGui::PushStyleColor(ImGuiCol_Text, quality_colors[idx]);
		}
		bool cb_changed = ImGui::Checkbox(std::format("{}##quality", entry.label).c_str(), &entry.checked);
		if (has_color)
			ImGui::PopStyleColor(2);

		if (row_toggled && !cb_changed)
			entry.checked = !entry.checked;
	}

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

	// --- Filter bar (row 2, col 1) ---
	//     <div class="regex-info" v-if="$core.view.config.regexFilters" ...>Regex Enabled</div>
	//     <input type="text" v-model="$core.view.userInputFilterItems" placeholder="Filter items..."/>
	ImGui::SetCursorPos(ImVec2(cursor.x, cursor.y + topH));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 0.0f));
	ImGui::BeginChild("items-filter", ImVec2(listW, FILTER_H), ImGuiChildFlags_None,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	{
		// Vertically center the filter input.
		float itemH = ImGui::GetFrameHeight();
		float padY = (FILTER_H - itemH) * 0.5f;
		if (padY > 0.0f)
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);

		// <div class="regex-info" v-if="config.regexFilters" :title="regexTooltip">Regex Enabled</div>
		if (view.config.value("regexFilters", false)) {
			ImGui::TextUnformatted("Regex Enabled");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", view.regexTooltip.c_str());
			ImGui::SameLine();
		}

		// <input type="text" v-model="userInputFilterItems" placeholder="Filter items..."/>
		char filter_buf[256] = {};
		std::strncpy(filter_buf, view.userInputFilterItems.c_str(), sizeof(filter_buf) - 1);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::InputTextWithHint("##FilterItems", "Filter items...", filter_buf, sizeof(filter_buf)))
			view.userInputFilterItems = filter_buf;
	}
	ImGui::EndChild();
	ImGui::PopStyleVar(); // WindowPadding

	} // if BeginTab
	app::layout::EndTab();

	// --- Context menu ---
	//     <span v-if="context.node.modelCount > 0" @click.self="view_models(context.node)">View related models ({{ context.node.modelCount }})</span>
	//     <span v-if="context.node.textureCount > 0" @click.self="view_textures(context.node)">View related textures ({{ context.node.textureCount }})</span>
	//     <span @click.self="copy_to_clipboard(context.node.name)">Copy item name to clipboard</span>
	//     <span @click.self="copy_to_clipboard(context.node.id)">Copy item ID to clipboard</span>
	//     <span @click.self="view_on_wowhead(context.node.id)">View item on Wowhead (web)</span>
	context_menu::render(
		"ctx-item",
		view.contextMenus.nodeItem,
		context_menu_item_state,
		[&]() { view.contextMenus.nodeItem = nullptr; },
		[&](const nlohmann::json& node) {
			int model_count = node.value("modelCount", 0);
			int texture_count = node.value("textureCount", 0);
			uint32_t node_id = node.value("id", 0u);
			std::string node_name = node.value("name", std::string(""));

			if (model_count > 0) {
				if (ImGui::Selectable(std::format("View related models ({})", model_count).c_str())) {
					const Item* item = find_item_by_id(node_id);
					if (item)
						view_item_models(*item);
				}
			}

			if (texture_count > 0) {
				if (ImGui::Selectable(std::format("View related textures ({})", texture_count).c_str())) {
					const Item* item = find_item_by_id(node_id);
					if (item)
						view_item_textures(*item);
				}
			}

			if (ImGui::Selectable("Copy item name to clipboard"))
				copy_to_clipboard(node_name);

			if (ImGui::Selectable("Copy item ID to clipboard"))
				copy_to_clipboard(std::to_string(node_id));

			if (ImGui::Selectable("View item on Wowhead (web)"))
				view_on_wowhead(node_id);
		}
	);
}

void setAllItemTypes(bool state) {
	for (auto& entry : type_mask_entries)
		entry.checked = state;
}

void setAllItemQualities(bool state) {
	for (auto& entry : quality_mask_entries)
		entry.checked = state;
}

} // namespace tab_items
