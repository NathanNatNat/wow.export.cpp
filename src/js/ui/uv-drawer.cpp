/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "uv-drawer.h"

#include <cmath>
#include <algorithm>

#include "../png-writer.h"

namespace uv_drawer {

static void plotPixel(std::vector<uint8_t>& pixels, int width, int height, int x, int y) {
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;

	const size_t offset = (static_cast<size_t>(y) * width + x) * 4;
	pixels[offset + 0] = 255;
	pixels[offset + 1] = 255;
	pixels[offset + 2] = 255;
	pixels[offset + 3] = 255;
}

static void drawLinePath(std::vector<uint8_t>& pixels, int width, int height,
	float x0, float y0, float x1, float y1) {
	const float dx = x1 - x0;
	const float dy = y1 - y0;
	const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dy)))));

	for (int i = 0; i <= steps; ++i) {
		const float t = static_cast<float>(i) / static_cast<float>(steps);
		const int x = static_cast<int>(std::round(x0 + dx * t));
		const int y = static_cast<int>(std::round(y0 + dy * t));
		plotPixel(pixels, width, height, x, y);
	}
}

std::vector<uint8_t> generateUVLayerPixels(
	const std::vector<float>& uvCoords,
	int textureWidth,
	int textureHeight,
	const std::vector<uint16_t>& indices)
{
	std::vector<uint8_t> pixels(static_cast<size_t>(textureWidth) * textureHeight * 4, 0);

	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		const size_t idx1 = static_cast<size_t>(indices[i]) * 2;
		const size_t idx2 = static_cast<size_t>(indices[i + 1]) * 2;
		const size_t idx3 = static_cast<size_t>(indices[i + 2]) * 2;

		if (idx1 + 1 >= uvCoords.size() || idx2 + 1 >= uvCoords.size() || idx3 + 1 >= uvCoords.size())
			continue;

		const float u1 = uvCoords[idx1] * static_cast<float>(textureWidth);
		const float v1 = (1.0f - uvCoords[idx1 + 1]) * static_cast<float>(textureHeight);
		const float u2 = uvCoords[idx2] * static_cast<float>(textureWidth);
		const float v2 = (1.0f - uvCoords[idx2 + 1]) * static_cast<float>(textureHeight);
		const float u3 = uvCoords[idx3] * static_cast<float>(textureWidth);
		const float v3 = (1.0f - uvCoords[idx3 + 1]) * static_cast<float>(textureHeight);

		drawLinePath(pixels, textureWidth, textureHeight, u1, v1, u2, v2);
		drawLinePath(pixels, textureWidth, textureHeight, u2, v2, u3, v3);
		drawLinePath(pixels, textureWidth, textureHeight, u3, v3, u1, v1);
	}

	return pixels;
}

std::string generateUVLayerDataURL(
	const std::vector<float>& uvCoords,
	int textureWidth,
	int textureHeight,
	const std::vector<uint16_t>& indices)
{
	const std::vector<uint8_t> pixels = generateUVLayerPixels(uvCoords, textureWidth, textureHeight, indices);
	PNGWriter png_writer(static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
	png_writer.getPixelData() = pixels;
	BufferWrapper buf = png_writer.getBuffer();
	return "data:image/png;base64," + buf.toBase64();
}

}
