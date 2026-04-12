/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <vector>
#include <functional>

/**
 * Character texture overlay layer management.
 *
 * In JS, this manages DOM canvas elements as texture preview layers.
 *
 * JS equivalent: module.exports = { add, remove, ensureActiveLayerAttached, getActiveLayer }
 */
namespace char_texture_overlay {

/**
 * Add a texture layer (GL texture ID).
 * The first layer added becomes the active layer.
 * JS equivalent: add(canvas)
 * @param textureID OpenGL texture ID to add as a layer.
 */
void add(uint32_t textureID);

/**
 * Remove a texture layer.
 * If this was the active layer, the last remaining layer becomes active.
 * JS equivalent: remove(canvas)
 * @param textureID OpenGL texture ID to remove.
 */
void remove(uint32_t textureID);

/**
 * Ensure the active layer is attached for display.
 * JS equivalent: ensureActiveLayerAttached / ensure_active_layer_attached
 *
 * In JS, this uses process.nextTick() to re-attach the active canvas
 * to the DOM after a tab switch. In C++/ImGui, this is essentially a no-op
 * since ImGui redraws every frame, but we keep the function for API parity.
 */
void ensureActiveLayerAttached();

/**
 * Get the currently active layer texture ID.
 * JS equivalent: getActiveLayer / get_active_layer
 * @returns The active texture ID, or 0 if no layers exist.
 */
uint32_t getActiveLayer();

/**
 * Switch to the next overlay layer (wraps around).
 * JS equivalent: core.events.on('click-chr-next-overlay', ...)
 */
void nextOverlay();

/**
 * Switch to the previous overlay layer (wraps around).
 * JS equivalent: core.events.on('click-chr-prev-overlay', ...)
 */
void prevOverlay();

/**
 * Get the number of layers.
 * @returns Number of registered layers.
 */
size_t getLayerCount();

/**
 * Initialize event listeners on the core event emitter.
 * Must be called once at startup.
 */
void initEvents();

} // namespace char_texture_overlay
