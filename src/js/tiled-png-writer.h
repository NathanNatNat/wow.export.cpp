/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <filesystem>
#include <future>

#include "buffer.h"

/**
 * Sparse PNG writer that can stitch together tiles without loading
 * the entire image into memory at once.
 *
 * JS equivalent: class TiledPNGWriter — module.exports = TiledPNGWriter
 */
class TiledPNGWriter {
public:
	/**
	 * Construct a new TiledPNGWriter instance.
	 * @param width    Total width of the final image.
	 * @param height   Total height of the final image.
	 * @param tileSize Size of individual tiles (assumes square tiles).
	 */
	TiledPNGWriter(uint32_t width, uint32_t height, uint32_t tileSize);

	/**
	 * ImageData equivalent for tile pixel data.
	 */
	struct ImageData {
		std::vector<uint8_t> data;
		uint32_t width;
		uint32_t height;
	};

	/**
	 * Add a tile at the specified position.
	 * @param tileX     Tile X coordinate (in tile units).
	 * @param tileY     Tile Y coordinate (in tile units).
	 * @param imageData Tile image data.
	 */
	void addTile(uint32_t tileX, uint32_t tileY, ImageData imageData);

	/**
	 * Generate the final PNG buffer.
	 * @returns BufferWrapper containing the complete PNG file.
	 */
	BufferWrapper getBuffer();

	/**
	 * Write this PNG to a file.
	 * @param file Path to write the PNG file to.
	 * @returns Shared future that resolves when file writing completes.
	 */
	std::shared_future<void> write(const std::filesystem::path& file);

	/**
	 * Statistics about the tiled image.
	 */
	struct Stats {
		size_t totalTiles;
		uint32_t imageWidth;
		uint32_t imageHeight;
		uint32_t tileSize;
		uint64_t expectedTiles; // JS uses 64-bit Number; uint32_t overflows for large tile grids
		double sparseRatio;
	};

	/**
	 * Get information about the tiles that will be included.
	 * @returns Statistics about the tiled image.
	 */
	Stats getStats() const;

private:
	struct Tile {
		uint32_t x;
		uint32_t y;
		ImageData data;
		uint32_t actualWidth;
		uint32_t actualHeight;
	};

	/**
	 * Write a tile's data to the pixel buffer at the correct position.
	 * Uses alpha blending for proper compositing of overlapping tiles.
	 * @param tile      Tile object with position and data.
	 * @param pixelData Target pixel buffer.
	 */
	void _writeTileToPixelData(const Tile& tile, std::vector<uint8_t>& pixelData);

	uint32_t width;
	uint32_t height;
	uint32_t tileSize;
	uint32_t tileCols;
	uint32_t tileRows;
	std::map<std::string, Tile> tiles;
};
