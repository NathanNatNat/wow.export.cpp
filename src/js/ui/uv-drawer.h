/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * UV layer preview rendering.
 *
 * JS equivalent: module.exports = { generateUVLayerDataURL }
 *
 * In the JS version, this renders UV wireframe overlays to a <canvas> element
 * and returns a PNG data URL.
 */
namespace uv_drawer {

/**
 * Generate a PNG data URL containing UV wireframe lines.
 * JS equivalent: generateUVLayerDataURL(uvCoords, textureWidth, textureHeight, indices)
 */
std::string generateUVLayerDataURL(
	const std::vector<float>& uvCoords,
	int textureWidth,
	int textureHeight,
	const std::vector<uint16_t>& indices
);

/**
 * Generate an RGBA pixel buffer containing UV wireframe lines.
 * This is the C++ helper used for GL texture upload.
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
