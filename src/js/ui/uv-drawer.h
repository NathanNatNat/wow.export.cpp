/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <vector>

/**
 * UV layer preview rendering.
 *
 * JS equivalent: module.exports = { generateUVLayerDataURL }
 *
 * In the JS version, this renders UV wireframe overlays to a <canvas> element
 * and returns a PNG data URL. In the C++ version, we render to an RGBA pixel
 * buffer instead, which can be uploaded as an OpenGL texture or encoded to PNG.
 */
namespace uv_drawer {

/**
 * Generate an RGBA pixel buffer containing UV wireframe lines.
 * White lines on a transparent background, matching the JS behavior.
 *
 * JS equivalent: generateUVLayerDataURL(uvCoords, textureWidth, textureHeight, indices)
 *
 * @param uvCoords       Flat array of UV coordinates (pairs of u,v values 0-1).
 * @param textureWidth   Width of the texture.
 * @param textureHeight  Height of the texture.
 * @param indices        Triangle indices for the mesh.
 * @returns RGBA pixel buffer (textureWidth * textureHeight * 4 bytes).
 */
std::vector<uint8_t> generateUVLayerPixels(
	const std::vector<float>& uvCoords,
	int textureWidth,
	int textureHeight,
	const std::vector<uint16_t>& indices
);

} // namespace uv_drawer
