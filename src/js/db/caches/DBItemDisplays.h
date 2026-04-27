/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <vector>

namespace db::caches::DBItemDisplays {

struct ItemDisplay {
	uint32_t ID = 0;
	std::vector<uint32_t> textures;
};

/**
 * Initialize item displays from ItemDisplayInfo.db2
 */
void initializeItemDisplays();

/**
 * Gets item skins from a given file data ID.
 * @param fileDataID File data ID.
 * @returns Pointer to vector of displays, or nullptr if not found.
 */
const std::vector<ItemDisplay>* getItemDisplaysByFileDataID(uint32_t fileDataID);

/**
 * Gets textures from a display ID directly.
 * @param displayId ItemDisplayInfoID.
 * @returns Pointer to vector of texture file data IDs, or nullptr if not found.
 */
const std::vector<uint32_t>* getTexturesByDisplayId(uint32_t displayId);

} // namespace db::caches::DBItemDisplays
