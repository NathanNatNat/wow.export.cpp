/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <glad/gl.h>

/**
 * Texture ribbon UI state management.
 *
 * The texture ribbon displays a strip of texture slots at the bottom
 * of the model/decor viewer. Each slot shows a texture preview with a
 * file name.
 *
 * JS equivalent: module.exports = { reset, setSlotFile, setSlotFileLegacy,
 *     setSlotSrc, onResize, addSlot }
 */
namespace texture_ribbon {

/**
 * Invoked when the texture ribbon element resizes.
 * JS equivalent: onResize(width)
 * @param width Available width in pixels.
 */
void onResize(int width);

/**
 * Reset the texture ribbon.
 * Clears all slots and returns a new sync ID.
 * JS equivalent: reset()
 * @returns New sync ID for this preparation session.
 */
int reset();

/**
 * Set the file displayed in a given ribbon slot.
 * JS equivalent: setSlotFile(slotIndex, fileDataID, syncID)
 * @param slotIndex Slot index in the ribbon stack.
 * @param fileDataID File data ID to display.
 * @param syncID Sync ID from reset() — stale IDs are ignored.
 */
void setSlotFile(int slotIndex, uint32_t fileDataID, int syncID);

/**
 * Set the file displayed in a given ribbon slot (legacy — uses file path).
 * JS equivalent: setSlotFileLegacy(slotIndex, filePath, syncID)
 * @param slotIndex Slot index in the ribbon stack.
 * @param filePath File path to display.
 * @param syncID Sync ID from reset() — stale IDs are ignored.
 */
void setSlotFileLegacy(int slotIndex, const std::string& filePath, int syncID);

/**
 * Set the render source for a given ribbon slot.
 * JS equivalent: setSlotSrc(slotIndex, src, syncID)
 * @param slotIndex Slot index in the ribbon stack.
 * @param src Render source (data URL or path) for the slot thumbnail.
 * @param syncID Sync ID from reset() — stale IDs are ignored.
 */
void setSlotSrc(int slotIndex, const std::string& src, int syncID);

/**
 * Add an empty slot to the texture ribbon.
 * JS equivalent: addSlot()
 * @returns The index of the newly added slot.
 */
int addSlot();

/**
 * Get or create an OpenGL texture for the given ribbon slot.
 * Decodes the slot's data-URL src (base64 PNG) into an RGBA texture
 * on first access, then caches the result.
 * @param slotIndex Slot index in the ribbon stack.
 * @returns GL texture ID, or 0 if no src is available.
 */
GLuint getSlotTexture(int slotIndex);

/**
 * Delete all cached slot textures.  Called automatically by reset().
 */
void clearSlotTextures();

} // namespace texture_ribbon
