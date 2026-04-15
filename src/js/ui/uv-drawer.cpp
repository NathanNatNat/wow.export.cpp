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
 * Plot a single pixel with alpha blending (white over transparent).
 * Alpha is pre-multiplied by the line intensity factor to simulate sub-pixel line width.
 */
static void plotPixel(std::vector<uint8_t>& pixels, int width, int height,
                      int x, int y, float alpha) {
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;

	// Clamp alpha to [0, 1].
	alpha = std::clamp(alpha, 0.0f, 1.0f);
	const uint8_t a = static_cast<uint8_t>(alpha * 255.0f + 0.5f);
	if (a == 0) return;

	const size_t offset = (static_cast<size_t>(y) * width + x) * 4;
	// Alpha-blend white (255,255,255) over existing pixel using "over" compositing.
	// Since background starts transparent and lines are all white, we can use max alpha.
	const uint8_t existing_a = pixels[offset + 3];
	if (a > existing_a) {
		pixels[offset + 0] = 255; // R
		pixels[offset + 1] = 255; // G
		pixels[offset + 2] = 255; // B
		pixels[offset + 3] = a;   // A
	}
}

/**
 * Draw an anti-aliased line using Xiaolin Wu's algorithm.
 * Alpha values are scaled by lineAlpha to simulate sub-pixel line width.
 *
 * JS equivalent: ctx.lineWidth = 0.5 with anti-aliased Canvas 2D rendering.
 * The 0.5px line width is simulated by multiplying all alpha values by 0.5.
 */
static void drawLineAA(std::vector<uint8_t>& pixels, int width, int height,
                        float x0, float y0, float x1, float y1, float lineAlpha = 0.5f) {
	const bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
	if (steep) {
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	const float dx = x1 - x0;
	const float dy = y1 - y0;
	const float gradient = (dx < 1e-6f) ? 1.0f : dy / dx;

	// Handle first endpoint.
	float xend = std::round(x0);
	float yend = y0 + gradient * (xend - x0);
	float xgap = 1.0f - (x0 + 0.5f - std::floor(x0 + 0.5f));
	int xpxl1 = static_cast<int>(xend);
	int ypxl1 = static_cast<int>(std::floor(yend));

	if (steep) {
		plotPixel(pixels, width, height, ypxl1,     xpxl1, (1.0f - (yend - std::floor(yend))) * xgap * lineAlpha);
		plotPixel(pixels, width, height, ypxl1 + 1, xpxl1, (yend - std::floor(yend)) * xgap * lineAlpha);
	} else {
		plotPixel(pixels, width, height, xpxl1, ypxl1,     (1.0f - (yend - std::floor(yend))) * xgap * lineAlpha);
		plotPixel(pixels, width, height, xpxl1, ypxl1 + 1, (yend - std::floor(yend)) * xgap * lineAlpha);
	}
	float intery = yend + gradient;

	// Handle second endpoint.
	xend = std::round(x1);
	yend = y1 + gradient * (xend - x1);
	xgap = x1 + 0.5f - std::floor(x1 + 0.5f);
	int xpxl2 = static_cast<int>(xend);
	int ypxl2 = static_cast<int>(std::floor(yend));

	if (steep) {
		plotPixel(pixels, width, height, ypxl2,     xpxl2, (1.0f - (yend - std::floor(yend))) * xgap * lineAlpha);
		plotPixel(pixels, width, height, ypxl2 + 1, xpxl2, (yend - std::floor(yend)) * xgap * lineAlpha);
	} else {
		plotPixel(pixels, width, height, xpxl2, ypxl2,     (1.0f - (yend - std::floor(yend))) * xgap * lineAlpha);
		plotPixel(pixels, width, height, xpxl2, ypxl2 + 1, (yend - std::floor(yend)) * xgap * lineAlpha);
	}

	// Main loop — plot pixels along the line.
	if (steep) {
		for (int x = xpxl1 + 1; x < xpxl2; ++x) {
			int iy = static_cast<int>(std::floor(intery));
			float frac = intery - std::floor(intery);
			plotPixel(pixels, width, height, iy,     x, (1.0f - frac) * lineAlpha);
			plotPixel(pixels, width, height, iy + 1, x, frac * lineAlpha);
			intery += gradient;
		}
	} else {
		for (int x = xpxl1 + 1; x < xpxl2; ++x) {
			int iy = static_cast<int>(std::floor(intery));
			float frac = intery - std::floor(intery);
			plotPixel(pixels, width, height, x, iy,     (1.0f - frac) * lineAlpha);
			plotPixel(pixels, width, height, x, iy + 1, frac * lineAlpha);
			intery += gradient;
		}
	}
}

/**
 * Generate an RGBA pixel buffer containing UV wireframe lines.
 * White anti-aliased lines on a transparent background, matching the JS behavior.
 *
 * JS equivalent: generateUVLayerDataURL(uvCoords, textureWidth, textureHeight, indices)
 *
 * Uses Xiaolin Wu's anti-aliased line algorithm with 0.5 alpha factor to simulate
 * the JS Canvas 2D ctx.lineWidth = 0.5 rendering, which produces semi-transparent
 * anti-aliased lines.
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
		const size_t idx1 = static_cast<size_t>(indices[i]) * 2;
		const size_t idx2 = static_cast<size_t>(indices[i + 1]) * 2;
		const size_t idx3 = static_cast<size_t>(indices[i + 2]) * 2;

		// Bounds check: ensure all UV coordinate indices are within range.
		// JS accessing out-of-bounds returns undefined→NaN (safe, no lines drawn).
		// C++ out-of-bounds access is undefined behavior — skip this triangle.
		if (idx1 + 1 >= uvCoords.size() || idx2 + 1 >= uvCoords.size() || idx3 + 1 >= uvCoords.size())
			continue;

		// Convert UV (0-1) to pixel coordinates, flipping V.
		// Use float precision (matching JS Canvas sub-pixel rendering), then round.
		const float u1 = uvCoords[idx1] * static_cast<float>(textureWidth);
		const float v1 = (1.0f - uvCoords[idx1 + 1]) * static_cast<float>(textureHeight);
		const float u2 = uvCoords[idx2] * static_cast<float>(textureWidth);
		const float v2 = (1.0f - uvCoords[idx2 + 1]) * static_cast<float>(textureHeight);
		const float u3 = uvCoords[idx3] * static_cast<float>(textureWidth);
		const float v3 = (1.0f - uvCoords[idx3 + 1]) * static_cast<float>(textureHeight);

		// Draw triangle edges with anti-aliased lines (lineAlpha=0.5 to match ctx.lineWidth=0.5).
		drawLineAA(pixels, textureWidth, textureHeight, u1, v1, u2, v2);
		drawLineAA(pixels, textureWidth, textureHeight, u2, v2, u3, v3);
		drawLineAA(pixels, textureWidth, textureHeight, u3, v3, u1, v1);
	}

	return pixels;
}

} // namespace uv_drawer