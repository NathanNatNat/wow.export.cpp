/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "DBDecor.h"

#include "../../log.h"
#include "../../casc/db2.h"
#include "../WDCReader.h"

#include <format>

namespace db::caches::DBDecor {

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

static std::unordered_map<uint32_t, DecorItem> decorItems;
static bool isInitialized = false;

/**
 * Initialize house decor data from HouseDecor DB2.
 */
void initializeDecorData() {
	if (isInitialized)
		return;

	logging::write("Loading house decor data...");

	auto allRows = casc::db2::preloadTable("HouseDecor").getAllRows();
	for (const auto& [id, row] : allRows) {
		uint32_t model_file_id = fieldToUint32(row.at("ModelFileDataID"));
		if (model_file_id == 0)
			continue;

		DecorItem item;
		item.id = id;

		std::string name = fieldToString(row.at("Name_lang"));
		item.name = name.empty() ? std::format("Decor {}", id) : std::move(name);

		item.modelFileDataID = model_file_id;

		auto thumbIt = row.find("ThumbnailFileDataID");
		item.thumbnailFileDataID = (thumbIt != row.end()) ? fieldToUint32(thumbIt->second) : 0;

		auto itemIdIt = row.find("ItemID");
		item.itemID = (itemIdIt != row.end()) ? fieldToUint32(itemIdIt->second) : 0;

		auto goIt = row.find("GameObjectID");
		item.gameObjectID = (goIt != row.end()) ? fieldToUint32(goIt->second) : 0;

		auto typeIt = row.find("Type");
		item.type = (typeIt != row.end()) ? fieldToUint32(typeIt->second) : 0;

		auto mtIt = row.find("ModelType");
		item.modelType = (mtIt != row.end()) ? fieldToUint32(mtIt->second) : 0;

		decorItems.emplace(id, std::move(item));
	}

	logging::write(std::format("Loaded {} house decor items", decorItems.size()));
	isInitialized = true;
}

/**
 * Get all decor items.
 * @returns Reference to the decor items map.
 */
const std::unordered_map<uint32_t, DecorItem>& getAllDecorItems() {
	return decorItems;
}

/**
 * Get a decor item by ID.
 * @param id Decor item ID.
 * @returns Pointer to DecorItem, or nullptr if not found.
 */
const DecorItem* getDecorItemByID(uint32_t id) {
	auto it = decorItems.find(id);
	if (it != decorItems.end())
		return &it->second;
	return nullptr;
}

/**
 * Get a decor item by model file data ID.
 * @param fileDataID Model file data ID.
 * @returns Pointer to DecorItem, or nullptr if not found.
 */
const DecorItem* getDecorItemByModelFileDataID(uint32_t fileDataID) {
	for (const auto& [_id, item] : decorItems) {
		(void)_id;
		if (item.modelFileDataID == fileDataID)
			return &item;
	}
	return nullptr;
}

} // namespace db::caches::DBDecor
