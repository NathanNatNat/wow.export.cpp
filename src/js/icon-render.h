/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>

/**
 * Icon loading and texture caching for item icons.
 *
 * In the JS version, icons were loaded as BLP files, converted to data URLs,
 * and set as CSS background-image properties on dynamic stylesheet rules.
 *
 * in a cache. The queue mechanism with priority ordering is preserved.
 *
 * JS equivalent: module.exports = { loadIcon }
 */
namespace icon_render {

/**
 * Load an icon by fileDataID.
 * If the icon is not already cached, it will be queued for loading.
 * If the fileDataID is 0, only the default placeholder is set.
 * @param fileDataID The CASC file data ID for the icon's BLP file.
 */
void loadIcon(uint32_t fileDataID);

/**
 * Get the OpenGL texture handle for a loaded icon.
 * Returns the default icon texture if the icon has not been loaded yet.
 * Returns 0 if no icons have been loaded at all.
 * @param fileDataID The CASC file data ID for the icon.
 * @returns OpenGL texture handle, or 0 if unavailable.
 */
uint32_t getIconTexture(uint32_t fileDataID);

} // namespace icon_render
