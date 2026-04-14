/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "tiled-png-writer.h"
#include "png-writer.h"

#include <cmath>
#include <cstddef>
#include <format>

/**
 * Construct a new TiledPNGWriter instance.
 */
TiledPNGWriter::TiledPNGWriter(uint32_t width, uint32_t height, uint32_t tileSize)
	: width(width)
	, height(height)
	, tileSize(tileSize)
	, tileCols(static_cast<uint32_t>(std::ceil(static_cast<double>(width) / tileSize)))
	, tileRows(static_cast<uint32_t>(std::ceil(static_cast<double>(height) / tileSize)))
{
}

/**
 * Add a tile at the specified position.
 */
void TiledPNGWriter::addTile(uint32_t tileX, uint32_t tileY, ImageData imageData) {
	const std::string key = std::format("{},{}", tileX, tileY);
	Tile tile;
	tile.x = tileX;
	tile.y = tileY;
	tile.actualWidth = imageData.width;
	tile.actualHeight = imageData.height;
	tile.data = std::move(imageData);
	tiles[key] = std::move(tile);
}

/**
 * Generate the final PNG buffer.
 */
BufferWrapper TiledPNGWriter::getBuffer() {
	PNGWriter png(width, height);
	std::vector<uint8_t>& pixelData = png.getPixelData();

	std::fill(pixelData.begin(), pixelData.end(), 0);

	for (const auto& [key, tile] : tiles)
		_writeTileToPixelData(tile, pixelData);

	return png.getBuffer();
}

/**
 * Write a tile's data to the pixel buffer at the correct position.
 * Uses alpha blending for proper compositing of overlapping tiles.
 */
void TiledPNGWriter::_writeTileToPixelData(const Tile& tile, std::vector<uint8_t>& pixelData) {
	const uint32_t pixelX = tile.x * tileSize;
	const uint32_t pixelY = tile.y * tileSize;

	const std::vector<uint8_t>& tileData = tile.data.data;
	const uint32_t tileWidth = tile.actualWidth;
	const uint32_t tileHeight = tile.actualHeight;

	for (uint32_t y = 0; y < tileHeight; y++) {
		for (uint32_t x = 0; x < tileWidth; x++) {
			const uint32_t targetX = pixelX + x;
			const uint32_t targetY = pixelY + y;

			if (targetX >= width || targetY >= height)
				continue;

			const size_t sourceIndex = (static_cast<size_t>(y) * tileWidth + x) * 4;
			const size_t targetIndex = (static_cast<size_t>(targetY) * width + targetX) * 4;

			const double srcA = tileData[sourceIndex + 3] / 255.0;

			// fully transparent source pixel, skip
			if (srcA == 0)
				continue;

			// fully opaque source pixel, overwrite
			if (srcA == 1) {
				pixelData[targetIndex] = tileData[sourceIndex];
				pixelData[targetIndex + 1] = tileData[sourceIndex + 1];
				pixelData[targetIndex + 2] = tileData[sourceIndex + 2];
				pixelData[targetIndex + 3] = 255;
				continue;
			}

			// alpha blend (Porter-Duff "over" operation)
			const double dstA = pixelData[targetIndex + 3] / 255.0;
			const double outA = srcA + dstA * (1 - srcA);

			if (outA > 0) {
				pixelData[targetIndex] = static_cast<uint8_t>((tileData[sourceIndex] * srcA + pixelData[targetIndex] * dstA * (1 - srcA)) / outA);
				pixelData[targetIndex + 1] = static_cast<uint8_t>((tileData[sourceIndex + 1] * srcA + pixelData[targetIndex + 1] * dstA * (1 - srcA)) / outA);
				pixelData[targetIndex + 2] = static_cast<uint8_t>((tileData[sourceIndex + 2] * srcA + pixelData[targetIndex + 2] * dstA * (1 - srcA)) / outA);
				pixelData[targetIndex + 3] = static_cast<uint8_t>(outA * 255);
			}
		}
	}
}

/**
 * Write this PNG to a file.
 *
 * Deviation: JS version is async (returns a Promise). C++ version is synchronous
 * since file I/O in this codebase is handled synchronously. The caller should wrap
 * in std::async if non-blocking behavior is needed.
 */
void TiledPNGWriter::write(const std::filesystem::path& file) {
	getBuffer().writeToFile(file);
}

/**
 * Get information about the tiles that will be included.
 */
TiledPNGWriter::Stats TiledPNGWriter::getStats() const {
	return {
		.totalTiles = tiles.size(),
		.imageWidth = width,
		.imageHeight = height,
		.tileSize = tileSize,
		.expectedTiles = tileCols * tileRows,
		.sparseRatio = static_cast<double>(tiles.size()) / (static_cast<double>(tileCols) * tileRows)
	};
}