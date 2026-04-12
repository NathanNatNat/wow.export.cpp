/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */

#include "DBCreatures.h"

#include "../../log.h"
#include "../../casc/db2.h"
#include "../WDCReader.h"

#include <format>
#include <unordered_map>

namespace db::caches::DBCreatures {

static uint32_t fieldToUint32(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<uint32_t>(*p);
	return 0;
}

static std::vector<uint32_t> fieldToUint32Vec(const db::FieldValue& val) {
	std::vector<uint32_t> result;
	if (auto* p = std::get_if<std::vector<int64_t>>(&val)) {
		result.reserve(p->size());
		for (auto v : *p)
			result.push_back(static_cast<uint32_t>(v));
	} else if (auto* p = std::get_if<std::vector<uint64_t>>(&val)) {
		result.reserve(p->size());
		for (auto v : *p)
			result.push_back(static_cast<uint32_t>(v));
	}
	return result;
}

static std::unordered_map<uint32_t, std::vector<CreatureDisplayInfo>> creatureDisplays;
static std::unordered_map<uint32_t, CreatureDisplayInfo> creatureDisplayInfoMap;
static std::unordered_map<uint32_t, uint32_t> displayIDToFileDataID;
static bool isInitialized = false;

/**
 * Initialize creature data.
 */
void initializeCreatureData() {
	if (isInitialized)
		return;

	logging::write("Loading creature textures...");

	std::unordered_map<uint32_t, std::vector<uint32_t>> creatureGeosetMap;

	// CreatureDisplayInfoID => Array of geosets to enable which should only be used if CreatureModelData.CreatureDisplayInfoGeosetData != 0
	auto geosetRows = casc::db2::preloadTable("CreatureDisplayInfoGeosetData").getAllRows();
	for (const auto& [_geosetId, geosetRow] : geosetRows) {
		(void)_geosetId;
		uint32_t creatureDisplayInfoID = fieldToUint32(geosetRow.at("CreatureDisplayInfoID"));
		uint32_t geosetIndex = fieldToUint32(geosetRow.at("GeosetIndex"));
		uint32_t geosetValue = fieldToUint32(geosetRow.at("GeosetValue"));

		creatureGeosetMap[creatureDisplayInfoID].push_back((geosetIndex + 1) * 100 + geosetValue);
	}

	std::unordered_map<uint32_t, std::vector<uint32_t>> modelIDToDisplayInfoMap;

	// Map all available texture fileDataIDs to model IDs.
	auto displayRows = casc::db2::preloadTable("CreatureDisplayInfo").getAllRows();
	for (const auto& [displayID, displayRow] : displayRows) {
		uint32_t modelID = fieldToUint32(displayRow.at("ModelID"));
		uint32_t extendedDisplayInfoID = fieldToUint32(displayRow.at("ExtendedDisplayInfoID"));

		auto allTextures = fieldToUint32Vec(displayRow.at("TextureVariationFileDataID"));
		std::vector<uint32_t> textures;
		for (auto t : allTextures) {
			if (t > 0)
				textures.push_back(t);
		}

		CreatureDisplayInfo info;
		info.ID = displayID;
		info.modelID = modelID;
		info.extendedDisplayInfoID = extendedDisplayInfoID;
		info.textures = std::move(textures);
		creatureDisplayInfoMap.emplace(displayID, std::move(info));

		modelIDToDisplayInfoMap[modelID].push_back(displayID);
	}

	auto modelRows = casc::db2::preloadTable("CreatureModelData").getAllRows();
	for (const auto& [modelID, modelRow] : modelRows) {
		auto it = modelIDToDisplayInfoMap.find(modelID);
		if (it != modelIDToDisplayInfoMap.end()) {
			uint32_t fileDataID = fieldToUint32(modelRow.at("FileDataID"));
			uint32_t creatureGeosetDataID = fieldToUint32(modelRow.at("CreatureGeosetDataID"));
			bool modelIDHasExtraGeosets = creatureGeosetDataID > 0;
			const auto& displayIDs = it->second;

			for (const auto displayID : displayIDs) {
				displayIDToFileDataID.emplace(displayID, fileDataID);

				auto& display = creatureDisplayInfoMap.at(displayID);

				if (modelIDHasExtraGeosets) {
					display.extraGeosets.clear();
					auto geoIt = creatureGeosetMap.find(displayID);
					if (geoIt != creatureGeosetMap.end())
						display.extraGeosets = geoIt->second;
				}

				creatureDisplays[fileDataID].push_back(display);
			}
		}
	}

	logging::write(std::format("Loaded textures for {} creatures", creatureDisplays.size()));
	isInitialized = true;
}

/**
 * Gets creature skins from a given file data ID.
 * @param fileDataID File data ID.
 * @returns Pointer to vector of displays, or nullptr if not found.
 */
const std::vector<CreatureDisplayInfo>* getCreatureDisplaysByFileDataID(uint32_t fileDataID) {
	auto it = creatureDisplays.find(fileDataID);
	if (it != creatureDisplays.end())
		return &it->second;
	return nullptr;
}

/**
 * Gets the file data ID for a given display ID.
 */
uint32_t getFileDataIDByDisplayID(uint32_t displayID) {
	auto it = displayIDToFileDataID.find(displayID);
	if (it != displayIDToFileDataID.end())
		return it->second;
	return 0;
}

const CreatureDisplayInfo* getDisplayInfo(uint32_t displayID) {
	auto it = creatureDisplayInfoMap.find(displayID);
	if (it != creatureDisplayInfoMap.end())
		return &it->second;
	return nullptr;
}

} // namespace db::caches::DBCreatures