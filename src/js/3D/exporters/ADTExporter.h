/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace casc {
	class CASC;
	class ExportHelper;
}

class ADTLoader;

/**
 * Result of an ADT tile export.
 */
struct ADTExportResult {
	std::string type;
	std::filesystem::path path;
};

/**
 * UV coordinate bounds for normalization.
 */
struct UVBounds {
	float minU = 0.0f;
	float maxU = 0.0f;
	float minV = 0.0f;
	float maxV = 0.0f;
};

/**
 * Game object entry for export from map viewer.
 * Mirrors the JS game object structure with PascalCase fields.
 */
struct ADTGameObject {
	uint32_t FileDataID = 0;
	std::vector<float> Position;
	std::vector<float> Rotation;
	float scale = 0.0f;
	uint32_t uniqueId = 0;
};

class ADTExporter {
public:
	/**
	 * Construct a new ADTLoader instance.
	 * @param mapID
	 * @param mapDir
	 * @param tileIndex
	 */
	ADTExporter(uint32_t mapID, const std::string& mapDir, uint32_t tileIndex);

	/**
	 * Calculate UV bounds for normalization.
	 * @param rootAdt The root ADT data
	 * @param firstChunkX First chunk X coordinate
	 * @param firstChunkY First chunk Y coordinate
	 * @returns UV bounds {minU, maxU, minV, maxV}
	 */
	UVBounds calculateUVBounds(const ADTLoader& rootAdt, float firstChunkX, float firstChunkY);

	/**
	 * Export the ADT tile.
	 * @param dir Directory to export the tile into.
	 * @param quality
	 * @param gameObjects Additional game objects to export.
	 * @param helper
	 * @returns Export result with type and path.
	 */
	// TODO(conversion): Renamed from `export` to `exportTile` because `export` is a C++ keyword (modules).
	ADTExportResult exportTile(const std::filesystem::path& dir, int quality,
		const std::vector<ADTGameObject>* gameObjects, casc::ExportHelper* helper);

	/**
	 * Clear internal tile-loading cache.
	 */
	static void clearCache();

private:
	uint32_t mapID;
	std::string mapDir;
	uint32_t tileX;
	uint32_t tileY;
	std::string tileID;
	uint32_t tileIndex;
};
