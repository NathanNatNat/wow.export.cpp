/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_item_sets.h"
#include "../log.h"
#include "../core.h"
#include "../casc/db2.h"
#include "../db/caches/DBItems.h"
#include "../db/WDCReader.h"
#include "../wow/EquipmentSlots.h"
#include "../install-type.h"
#include "../modules.h"
#include "../components/itemlistbox.h"

#include <cstring>
#include <algorithm>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_item_sets {

// --- File-local structures ---

struct ItemSet {
	uint32_t id = 0;
	std::string name;
	std::vector<uint32_t> item_ids;
	uint32_t icon = 0;
	int quality = 0;

	std::string displayName() const {
		return std::format("{} ({})", name, id);
	}
};

// --- File-local helpers ---

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

static std::vector<ItemSet> item_sets;

static bool is_initialized = false;
static itemlistbox::ItemListboxState itemlistbox_item_sets_state;

// --- Internal functions ---

static void initialize_item_sets() {
	item_sets.clear();

	core::progressLoadingScreen("Loading item data...");
	db::caches::DBItems::ensureInitialized();

	//         appearance_map.set(row.ItemID, row.ItemAppearanceID);
	core::progressLoadingScreen("Loading item appearance data...");
	std::unordered_map<uint32_t, uint32_t> appearance_map;
	for (const auto& [_id, row] : casc::db2::preloadTable("ItemModifiedAppearance").getAllRows()) {
		uint32_t itemID = fieldToUint32(row.at("ItemID"));
		uint32_t itemAppearanceID = fieldToUint32(row.at("ItemAppearanceID"));
		appearance_map[itemID] = itemAppearanceID;
	}

	core::progressLoadingScreen("Loading item sets...");
	auto& item_appearance_table = casc::db2::preloadTable("ItemAppearance");
	const auto& item_set_rows = casc::db2::preloadTable("ItemSet").getAllRows();

	for (const auto& [set_id, set_row] : item_set_rows) {
		auto item_id_it = set_row.find("ItemID");
		if (item_id_it == set_row.end())
			continue;

		std::vector<uint32_t> item_ids_raw = fieldToUint32Vec(item_id_it->second);
		std::vector<uint32_t> item_ids;
		for (uint32_t id : item_ids_raw) {
			if (id != 0)
				item_ids.push_back(id);
		}

		if (item_ids.empty())
			continue;

		// get first item for icon/quality
		uint32_t first_icon = 0;
		int first_quality = 0;
		bool found_first = false;

		for (const uint32_t item_id : item_ids) {
			const auto* item = db::caches::DBItems::getItemById(item_id);
			if (item) {
				auto app_it = appearance_map.find(item_id);
				uint32_t icon = 0;

				if (app_it != appearance_map.end()) {
					auto appearance_row = item_appearance_table.getRow(app_it->second);

					if (appearance_row.has_value()) {
						auto iconIt = appearance_row->find("DefaultIconFileDataID");
						icon = (iconIt != appearance_row->end()) ? fieldToUint32(iconIt->second) : 0;
					}
				}

				first_icon = icon;
				first_quality = item->quality;
				found_first = true;

				if (first_icon != 0)
					break;
			}
		}

		ItemSet new_set;
		new_set.id = set_id;

		auto name_it = set_row.find("Name_lang");
		new_set.name = (name_it != set_row.end()) ? fieldToString(name_it->second) : "";

		new_set.item_ids = std::move(item_ids);
		new_set.icon = found_first ? first_icon : 0;
		new_set.quality = found_first ? first_quality : 0;

		item_sets.push_back(std::move(new_set));
	}

	logging::write(std::format("Loaded {} item sets", item_sets.size()));
}

static void apply_filter() {
	auto& view = *core::view;
	view.listfileItemSets.clear();

	for (const auto& set : item_sets) {
		nlohmann::json j;
		j["id"] = set.id;
		j["name"] = set.name;
		j["displayName"] = set.displayName();
		j["icon"] = set.icon;
		j["quality"] = set.quality;

		// Store item_ids as JSON array for equip_set to use.
		j["item_ids"] = nlohmann::json::array();
		for (uint32_t id : set.item_ids)
			j["item_ids"].push_back(id);

		view.listfileItemSets.push_back(std::move(j));
	}
}

static void equip_set(const nlohmann::json& set) {
	int equipped_count = 0;
	std::string set_name = set.value("name", std::string("Unknown"));

	if (!set.contains("item_ids") || !set["item_ids"].is_array())
		return;

	auto& view = *core::view;

	for (const auto& id_val : set["item_ids"]) {
		uint32_t item_id = id_val.get<uint32_t>();

		auto slot_id_opt = db::caches::DBItems::getItemSlotId(item_id);

		if (slot_id_opt.has_value()) {
			view.chrEquippedItems[std::to_string(slot_id_opt.value())] = item_id;
			equipped_count++;
		}
	}

	if (equipped_count > 0) {
		core::setToast("success", std::format("Equipped {} items from {}.", equipped_count, set_name), {}, 2000);
	} else {
		core::setToast("info", "No equippable items in this set.", {}, 2000);
	}
}

// --- Public API ---

void registerTab() {
	modules::register_nav_button("tab_item_sets", "Item Sets", "armour.svg", install_type::CASC);
}

void mounted() {
	core::showLoadingScreen(3);

	initialize_item_sets();

	core::hideLoadingScreen();

	apply_filter();

	is_initialized = true;
}

void render() {
	auto& view = *core::view;

	if (!is_initialized)
		return;

	// --- Template rendering ---

	//     <div class="list-container list-container-full">
	//         <Itemlistbox id="listbox-item-sets" v-model:selection="$core.view.selectionItemSets"
	//          :items="$core.view.listfileItemSets" :filter="$core.view.userInputFilterItemSets"
	//          :keyinput="true" :includefilecount="true" unittype="set" @equip="equip_set">
	ImGui::BeginChild("item-sets-list-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

	{
		// Convert json items to ItemEntry array.
		std::vector<itemlistbox::ItemEntry> item_entries;
		item_entries.reserve(view.listfileItemSets.size());
		for (const auto& j : view.listfileItemSets) {
			itemlistbox::ItemEntry entry;
			entry.id = j.value("id", 0);
			entry.name = j.value("name", std::string(""));
			entry.displayName = j.value("displayName", std::string(""));
			entry.icon = j.value("icon", 0u);
			entry.quality = j.value("quality", 0);
			item_entries.push_back(std::move(entry));
		}

		// Build selection as item IDs.
		std::vector<int> sel_ids;
		for (const auto& sel : view.selectionItemSets)
			sel_ids.push_back(sel.value("id", 0));

		itemlistbox::render("##ItemSetListbox", item_entries,
			view.userInputFilterItemSets, sel_ids, false, true,
			view.config.value("regexFilters", false),
			"set",
			itemlistbox_item_sets_state,
			[&](const std::vector<int>& new_sel) {
				view.selectionItemSets.clear();
				for (int id : new_sel) {
					for (const auto& j : view.listfileItemSets) {
						if (j.value("id", 0) == id) {
							view.selectionItemSets.push_back(j);
							break;
						}
					}
				}
			},
			[&](const itemlistbox::ItemEntry& item) {
				// @equip — find matching json and call equip_set
				for (const auto& j : view.listfileItemSets) {
					if (j.value("id", 0) == item.id) {
						equip_set(j);
						break;
					}
				}
			},
			nullptr);  // no @options callback for item sets
	}

	ImGui::EndChild(); // item-sets-list-container

	//     <div class="regex-info" v-if="$core.view.config.regexFilters" ...>Regex Enabled</div>
	//     <input type="text" v-model="$core.view.userInputFilterItemSets" placeholder="Filter item sets..."/>
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_buf[256] = {};
	std::strncpy(filter_buf, view.userInputFilterItemSets.c_str(), sizeof(filter_buf) - 1);
	if (ImGui::InputText("##FilterItemSets", filter_buf, sizeof(filter_buf)))
		view.userInputFilterItemSets = filter_buf;
}

} // namespace tab_item_sets
