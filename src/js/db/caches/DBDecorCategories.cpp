/*!
	wow.export (https://github.com/Kruithne/wow.export)
	License: MIT
 */

#include "DBDecorCategories.h"

#include "../../log.h"
#include "../../casc/db2.h"
#include "../WDCReader.h"

#include <format>

namespace db::caches::DBDecorCategories {

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

// JS: const categories = new Map();
static std::unordered_map<uint32_t, CategoryInfo> categories;
// JS: const subcategories = new Map();
static std::unordered_map<uint32_t, SubcategoryInfo> subcategories;
// JS: const decor_subcategory_map = new Map();
static std::unordered_map<uint32_t, std::unordered_set<uint32_t>> decor_subcategory_map;

// JS: let is_initialized = false;
static bool is_initialized = false;

// JS: const initialize_categories = async () => { ... }
void initialize_categories() {
	if (is_initialized)
		return;

	logging::write("Loading decor category data...");

	// JS: for (const [id, row] of await db2.DecorCategory.getAllRows())
	auto catRows = casc::db2::preloadTable("DecorCategory").getAllRows();
	for (const auto& [id, row] : catRows) {
		CategoryInfo info;
		info.id = id;

		// JS: name: row.Name_lang || `Category ${id}`
		std::string name = fieldToString(row.at("Name_lang"));
		info.name = name.empty() ? std::format("Category {}", id) : std::move(name);

		// JS: orderIndex: row.OrderIndex ?? 0
		auto oiIt = row.find("OrderIndex");
		info.orderIndex = (oiIt != row.end()) ? fieldToUint32(oiIt->second) : 0;

		categories.emplace(id, std::move(info));
	}

	// JS: for (const [id, row] of await db2.DecorSubcategory.getAllRows())
	auto subRows = casc::db2::preloadTable("DecorSubcategory").getAllRows();
	for (const auto& [id, row] : subRows) {
		SubcategoryInfo info;
		info.id = id;

		// JS: name: row.Name_lang || `Subcategory ${id}`
		std::string name = fieldToString(row.at("Name_lang"));
		info.name = name.empty() ? std::format("Subcategory {}", id) : std::move(name);

		// JS: categoryID: row.DecorCategoryID ?? 0
		auto catIt = row.find("DecorCategoryID");
		info.categoryID = (catIt != row.end()) ? fieldToUint32(catIt->second) : 0;

		// JS: orderIndex: row.OrderIndex ?? 0
		auto oiIt = row.find("OrderIndex");
		info.orderIndex = (oiIt != row.end()) ? fieldToUint32(oiIt->second) : 0;

		subcategories.emplace(id, std::move(info));
	}

	// JS: for (const [, row] of await db2.DecorXDecorSubcategory.getAllRows())
	auto mapRows = casc::db2::preloadTable("DecorXDecorSubcategory").getAllRows();
	for (const auto& [_mapId, row] : mapRows) {
		(void)_mapId;
		// JS: const decor_id = row.HouseDecorID ?? row.DecorID;
		uint32_t decor_id = 0;
		auto hdIt = row.find("HouseDecorID");
		if (hdIt != row.end())
			decor_id = fieldToUint32(hdIt->second);
		if (decor_id == 0) {
			auto dIt = row.find("DecorID");
			if (dIt != row.end())
				decor_id = fieldToUint32(dIt->second);
		}

		// JS: const sub_id = row.DecorSubcategoryID;
		auto subIt = row.find("DecorSubcategoryID");
		if (subIt == row.end())
			continue;
		uint32_t sub_id = fieldToUint32(subIt->second);

		if (decor_id == 0)
			continue;

		decor_subcategory_map[decor_id].insert(sub_id);
	}

	logging::write(std::format("Loaded {} decor categories, {} subcategories, {} mappings",
		categories.size(), subcategories.size(), decor_subcategory_map.size()));
	is_initialized = true;
}

// JS: const get_all_categories = () => categories;
const std::unordered_map<uint32_t, CategoryInfo>& get_all_categories() {
	return categories;
}

// JS: const get_all_subcategories = () => subcategories;
const std::unordered_map<uint32_t, SubcategoryInfo>& get_all_subcategories() {
	return subcategories;
}

// JS: const get_subcategories_for_decor = (decor_id) => decor_subcategory_map.get(decor_id) ?? null;
const std::unordered_set<uint32_t>* get_subcategories_for_decor(uint32_t decor_id) {
	auto it = decor_subcategory_map.find(decor_id);
	if (it != decor_subcategory_map.end())
		return &it->second;
	return nullptr;
}

} // namespace db::caches::DBDecorCategories
