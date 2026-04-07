/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "uv-drawer.h"

#include <cmath>
#include <algorithm>

namespace uv_drawer {

/**
 * Draw a single-pixel-wide line between two points using Bresenham's algorithm.
 * Plots white (RGBA 255,255,255,255) onto a transparent background.
 */
static void drawLine(std::vector<uint8_t>& pixels, int width, int height,
                     int x0, int y0, int x1, int y1) {
	int dx = std::abs(x1 - x0);
	int dy = -std::abs(y1 - y0);
	int sx = x0 < x1 ? 1 : -1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;

	while (true) {
		if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
			const int offset = (y0 * width + x0) * 4;
			pixels[offset + 0] = 255; // R
			pixels[offset + 1] = 255; // G
			pixels[offset + 2] = 255; // B
			pixels[offset + 3] = 255; // A
		}

		if (x0 == x1 && y0 == y1)
			break;

		int e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}
}

/**
 * Generate an RGBA pixel buffer containing UV wireframe lines.
 * White lines on a transparent background, matching the JS behavior.
 *
 * JS equivalent: generateUVLayerDataURL(uvCoords, textureWidth, textureHeight, indices)
 */
std::vector<uint8_t> generateUVLayerPixels(
	const std::vector<float>& uvCoords,
	int textureWidth,
	int textureHeight,
	const std::vector<uint16_t>& indices)
{
	// Allocate RGBA buffer initialized to transparent black.
	std::vector<uint8_t> pixels(static_cast<size_t>(textureWidth) * textureHeight * 4, 0);

	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		const int idx1 = indices[i] * 2;
		const int idx2 = indices[i + 1] * 2;
		const int idx3 = indices[i + 2] * 2;

		// Convert UV (0-1) to pixel coordinates, flipping V.
		const int u1 = static_cast<int>(uvCoords[idx1] * textureWidth);
		const int v1 = static_cast<int>((1.0f - uvCoords[idx1 + 1]) * textureHeight); // Flip V coordinate
		const int u2 = static_cast<int>(uvCoords[idx2] * textureWidth);
		const int v2 = static_cast<int>((1.0f - uvCoords[idx2 + 1]) * textureHeight);
		const int u3 = static_cast<int>(uvCoords[idx3] * textureWidth);
		const int v3 = static_cast<int>((1.0f - uvCoords[idx3 + 1]) * textureHeight);

		// Draw triangle edges.
		drawLine(pixels, textureWidth, textureHeight, u1, v1, u2, v2);
		drawLine(pixels, textureWidth, textureHeight, u2, v2, u3, v3);
		drawLine(pixels, textureWidth, textureHeight, u3, v3, u1, v1);
	}

	return pixels;
}

} // namespace uv_drawer