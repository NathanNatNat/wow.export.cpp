/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "map-viewer.h"

#include <imgui.h>
#include <glad/gl.h>
#include "../../app.h"
#include <cmath>
#include <algorithm>
#include <format>
#include <spdlog/spdlog.h>

#include "../core.h"
#include "../constants.h"

namespace map_viewer {

static const int MAP_SIZE = constants::GAME::MAP_SIZE;
static const double MAP_COORD_BASE = constants::GAME::MAP_COORD_BASE;
static const double TILE_SIZE = constants::GAME::TILE_SIZE;

// Persisted state for the map-viewer component. This generally goes against the
// principals of reactive instanced components, but unfortunately nothing else worked
// for maintaining state. This just means we can only have one map-viewer component.
static MapViewerPersistedState s_state;

MapViewerPersistedState& getPersistedState() {
	return s_state;
}

/**
 * Returns the effective grid size, defaulting to MAP_SIZE if not specified.
 */
int effectiveGridSize(int gridSize) {
	return gridSize > 0 ? gridSize : MAP_SIZE;
}


/**
 * Invoked when this component is mounted in the DOM.
 */

/**
 * Invoked when this component is about to be destroyed.
 */


/**
 * Invoked when the map property changes for this component.
 * This indicates that a new map has been selected for rendering.
 */

/**
 * Invoked when the tile being hovered over changes.
 */

/**
 * Invoked when the selection changes.
 */


/**
 * Clear tile queue, requested set, and rendered set.
 * Also reset previous tracking to force full redraw.
 */
void clearTileState() {
	s_state.tileQueue.clear();
	s_state.requested.clear();
	s_state.rendered.clear();
	s_state.tilePixelCache.clear();
	s_state.tilePixelCacheTileSize = 0;
	for (auto& [idx, tex] : s_state.tileTextures) {
		if (tex != 0) {
			GLuint gl_tex = static_cast<GLuint>(tex);
			glDeleteTextures(1, &gl_tex);
		}
	}
	s_state.tileTextures.clear();
	s_state.prevOffsetsValid = false;
	s_state.prevZoomFactor = 0;
	s_state.needsFinalPass = false;
	s_state.activeTileRequests = 0;
	s_state.renderGeneration++;

	// Clean up final pass timeout
	s_state.finalPassTimer = -1.0;
}

static constexpr int MAX_TILES_PER_FRAME = 3;

void checkTileQueue(MapViewerState& state, const TileLoader& loader) {
	int tiles_processed = 0;
	while (!s_state.tileQueue.empty() && tiles_processed < MAX_TILES_PER_FRAME) {
		TileQueueNode tile = s_state.tileQueue.front();
		s_state.tileQueue.erase(s_state.tileQueue.begin());
		loadTile(state, tile, loader);
		tiles_processed++;
	}

	// Check if we're done processing all tiles
	if (s_state.tileQueue.empty() && s_state.activeTileRequests == 0) {
		state.awaitingTile = false;
		// Trigger final pass once all tiles are processed, but only if needed
		// Add a small delay to avoid running it too frequently during rapid panning
		if (s_state.needsFinalPass) {
			s_state.needsFinalPass = false;
			s_state.finalPassTimer = 0.1; // 100ms in seconds
		}
	}
}

/**
 * Perform a final pass to detect and fix tiles with transparency issues.
 * This addresses seams caused by tiles being clipped but still marked as rendered.
 */
void performFinalPass(MapViewerState& state, int tileSize_prop, int gridSize,
                      const std::vector<int>& mask, const TileLoader& loader) {
	// Skip if no map or canvas available
	if (state.canvasWidth <= 0.0f || state.canvasHeight <= 0.0f)
		return;

	const int tileSize = static_cast<int>(std::floor(static_cast<float>(tileSize_prop) / static_cast<float>(s_state.zoomFactor)));

	// Calculate viewport bounds relative to canvas
	const float bufferX = (state.canvasWidth - state.viewportWidth) / 2.0f;
	const float bufferY = (state.canvasHeight - state.viewportHeight) / 2.0f;

	// Calculate visible tile range
	const int grid_size = effectiveGridSize(gridSize);
	const int startX = std::max(0, static_cast<int>(std::floor(-s_state.offsetX / static_cast<float>(tileSize))));
	const int startY = std::max(0, static_cast<int>(std::floor(-s_state.offsetY / static_cast<float>(tileSize))));
	const int endX = std::min(grid_size, startX + static_cast<int>(std::ceil(state.canvasWidth / static_cast<float>(tileSize))) + 1);
	const int endY = std::min(grid_size, startY + static_cast<int>(std::ceil(state.canvasHeight / static_cast<float>(tileSize))) + 1);

	std::vector<TileRerenderInfo> tilesNeedingRerender;

	// Check each visible tile for transparency issues
	for (int x = startX; x < endX; x++) {
		for (int y = startY; y < endY; y++) {
			const int index = (x * grid_size) + y;

			// Skip if not masked or not supposedly rendered
			if (!mask.empty() && (index < 0 || index >= static_cast<int>(mask.size()) || mask[index] != 1))
				continue;
			if (s_state.rendered.find(index) == s_state.rendered.end())
				continue;

			const float drawX = static_cast<float>(x * tileSize) + s_state.offsetX;
			const float drawY = static_cast<float>(y * tileSize) + s_state.offsetY;

			// Skip tiles completely outside viewport
			if (drawX + static_cast<float>(tileSize) <= bufferX || drawX >= bufferX + state.viewportWidth ||
				drawY + static_cast<float>(tileSize) <= bufferY || drawY >= bufferY + state.viewportHeight)
				continue;

			// Check if this tile has transparency where it shouldn't
			if (tileHasUnexpectedTransparency(drawX, drawY, tileSize)) {
				tilesNeedingRerender.push_back({ x, y, index, tileSize });
				// Remove from rendered set so it can be re-queued
				s_state.rendered.erase(index);
				s_state.requested.erase(index);
			}
		}
	}

	// Re-queue tiles that need re-rendering
	for (const auto& tile : tilesNeedingRerender) {
		queueTile(state, tile.x, tile.y, tile.index, tile.tileSize, loader);
	}
}

/**
 * Check if a tile has unexpected transparency (indicating clipping issues).
 * Uses efficient sampling to detect transparency without checking every pixel.
 * @param drawX Canvas X position of tile
 * @param drawY Canvas Y position of tile
 * @param tileSize Size of tile
 * @returns True if tile has unexpected transparency
 */
bool tileHasUnexpectedTransparency(float drawX, float drawY, int tileSize) {
	const int tileX = static_cast<int>(std::floor((drawX - s_state.offsetX) / static_cast<float>(tileSize)));
	const int tileY = static_cast<int>(std::floor((drawY - s_state.offsetY) / static_cast<float>(tileSize)));
	const int index = (tileX * s_state.lastGridSize) + tileY;

	auto it = s_state.tilePixelCache.find(index);
	if (it == s_state.tilePixelCache.end() || it->second.empty())
		return false;

	const auto& pixels = it->second;
	const int expectedSize = tileSize * tileSize * 4;
	if (static_cast<int>(pixels.size()) != expectedSize)
		return false;

	const int width = tileSize;
	const int height = tileSize;

	if (width < 4 || height < 4)
		return false;

	// Sample 9 points (corners, edge centers, center)
	const std::pair<int, int> samplePoints[] = {
		{0, 0}, {width - 1, 0}, {0, height - 1}, {width - 1, height - 1},
		{width / 2, 0}, {width / 2, height - 1},
		{0, height / 2}, {width - 1, height / 2},
		{width / 2, height / 2}
	};

	for (const auto& [px, py] : samplePoints) {
		const int pixelIndex = (py * width + px) * 4;
		if (pixelIndex + 3 < static_cast<int>(pixels.size())) {
			const uint8_t alpha = pixels[pixelIndex + 3];
			if (alpha == 0)
				return true;
		}
	}

	return false;
}

/**
 * Add a tile to the queue to be loaded if not already requested.
 * @param x
 * @param y
 * @param index
 * @param tileSize
 */
void queueTile(MapViewerState& state, int x, int y, int index, int tileSize, const TileLoader& loader) {
	// Skip if already requested
	if (s_state.requested.count(index))
		return;

	s_state.requested.insert(index);
	TileQueueNode node{ x, y, index, tileSize, 0 };

	if (state.awaitingTile)
		s_state.tileQueue.push_back(node);
	else
		loadTile(state, node, loader);
}

/**
 * Add a tile to the queue to be loaded for double-buffer rendering.
 * @param x
 * @param y
 * @param index
 * @param tileSize
 */
void queueTileForDoubleBuffer(MapViewerState& state, int x, int y, int index, int tileSize, const TileLoader& loader) {
	// Skip if already requested
	if (s_state.requested.count(index))
		return;

	s_state.requested.insert(index);
	TileQueueNode node{ x, y, index, tileSize, 1 };

	if (state.awaitingTile)
		s_state.tileQueue.push_back(node);
	else
		loadTile(state, node, loader);
}

/**
 * Load a given tile and draw it to the appropriate canvas.
 * Triggers a queue-check once loaded.
 * @param tile
 */
void loadTile(MapViewerState& state, const TileQueueNode& tile, const TileLoader& loader) {
	state.awaitingTile = true;
	s_state.activeTileRequests++;

	const int x = tile.x;
	const int y = tile.y;
	const int index = tile.index;
	const int tileSize = tile.tileSize;
	[[maybe_unused]] const int renderTarget = tile.renderTarget;
	const int currentGeneration = s_state.renderGeneration;

	std::vector<uint8_t> data = loader(x, y, tileSize);

	if (currentGeneration == s_state.renderGeneration) {
		// Only draw if tile loaded successfully
		if (!data.empty()) {
			// Calculate draw position
			[[maybe_unused]] const float drawX = static_cast<float>(x * tileSize) + s_state.offsetX;
			[[maybe_unused]] const float drawY = static_cast<float>(y * tileSize) + s_state.offsetY;

			s_state.tilePixelCache[index] = std::move(data);
			s_state.tilePixelCacheTileSize = tileSize;

			auto existingTexIt = s_state.tileTextures.find(index);
			if (existingTexIt != s_state.tileTextures.end() && existingTexIt->second != 0) {
				GLuint old_gl = static_cast<GLuint>(existingTexIt->second);
				glDeleteTextures(1, &old_gl);
				existingTexIt->second = 0;
			}

			GLuint tex = 0;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tileSize, tileSize, 0, GL_RGBA, GL_UNSIGNED_BYTE,
			             s_state.tilePixelCache[index].data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
			s_state.tileTextures[index] = static_cast<uint32_t>(tex);

			// Mark this tile as rendered
			s_state.rendered.insert(index);
		}

		// Remove from requested set since loading is complete
		s_state.requested.erase(index);
		s_state.activeTileRequests--;
	}

	checkTileQueue(state, loader);
}

/**
 * Set the map to a sensible default position. Centers the view on the
 * middle of available tiles in the mask.
 */
void setToDefaultPosition(MapViewerState& state, int tileSize_prop, int gridSize,
                          const std::vector<int>& mask, const TileLoader& loader) {
	const int grid_size = effectiveGridSize(gridSize);
	const int tileSize = static_cast<int>(std::floor(static_cast<float>(tileSize_prop) / static_cast<float>(s_state.zoomFactor)));
	const int maxTileSize = tileSize_prop;

	if (!mask.empty()) {
		// find bounding box of enabled tiles
		int min_x = INT_MAX, max_x = INT_MIN;
		int min_y = INT_MAX, max_y = INT_MIN;

		for (int i = 0; i < static_cast<int>(mask.size()); i++) {
			if (mask[i] != 1)
				continue;

			const int x = i / grid_size;
			const int y = i % grid_size;

			if (x < min_x) min_x = x;
			if (x > max_x) max_x = x;
			if (y < min_y) min_y = y;
			if (y > max_y) max_y = y;
		}

		if (min_x != INT_MAX) {
			// center on the middle of the bounding box
			const float center_x = static_cast<float>(min_x + max_x + 1) / 2.0f;
			const float center_y = static_cast<float>(min_y + max_y + 1) / 2.0f;

			s_state.offsetX = (state.viewportWidth / 2.0f) + static_cast<float>(maxTileSize) - (center_x * static_cast<float>(tileSize));
			s_state.offsetY = (state.viewportHeight / 2.0f) + static_cast<float>(maxTileSize) - (center_y * static_cast<float>(tileSize));
			render(state, tileSize_prop, gridSize, mask, loader);
			return;
		}
	}

	// fallback: center on grid center
	const float center = static_cast<float>(grid_size) / 2.0f;
	s_state.offsetX = (state.viewportWidth / 2.0f) + static_cast<float>(maxTileSize) - (center * static_cast<float>(tileSize));
	s_state.offsetY = (state.viewportHeight / 2.0f) + static_cast<float>(maxTileSize) - (center * static_cast<float>(tileSize));
	render(state, tileSize_prop, gridSize, mask, loader);
}


/**
 * Calculate optimal canvas dimensions based on tile size and zoom levels.
 * Canvas is sized to accommodate full tiles with a buffer zone that ensures
 * tiles are never rendered partially at any zoom level.
 */
std::pair<float, float> calculateCanvasSize(const MapViewerState& state, int tileSize_prop) {
	const float viewportWidth = state.viewportWidth;
	const float viewportHeight = state.viewportHeight;

	// Buffer must be large enough for the largest possible tile (zoom factor = 1)
	const float maxTileSize = static_cast<float>(tileSize_prop); // At zoom factor 1 (most zoomed in)

	// Canvas needs to be viewport size + buffer on all sides to ensure full tiles
	return {
		viewportWidth + (maxTileSize * 2.0f),  // +1 tile buffer on each side
		viewportHeight + (maxTileSize * 2.0f)   // +1 tile buffer on each side
	};
}

/**
 * Update the position of the internal container with double-buffer optimization.
 */
void render(MapViewerState& state, int tileSize_prop, int gridSize,
            const std::vector<int>& mask, const TileLoader& loader) {
	// If no map has been selected, do not render.

	// Calculate optimal canvas size
	auto [canvasW, canvasH] = calculateCanvasSize(state, tileSize_prop);

	// Update canvas dimensions only if they've changed to avoid unnecessary redraws
	if (state.canvasWidth != canvasW || state.canvasHeight != canvasH) {
		state.canvasWidth = canvasW;
		state.canvasHeight = canvasH;

		// Force full redraw when canvas size changes
		s_state.prevOffsetsValid = false;
		s_state.prevZoomFactor = 0;
	}

	// Update double-buffer dimensions to match

	// Update overlay canvas dimensions to match

	s_state.lastGridSize = effectiveGridSize(gridSize);

	// Calculate current tile size based on zoom factor
	const int tileSize = static_cast<int>(std::floor(static_cast<float>(tileSize_prop) / static_cast<float>(s_state.zoomFactor)));

	// Check if this is a simple pan (same zoom, only offset changed)
	const bool isPan = s_state.prevZoomFactor == s_state.zoomFactor && s_state.prevOffsetsValid;

	if (isPan)
		renderWithDoubleBuffer(state, canvasW, canvasH, tileSize, gridSize, mask, loader);
	else
		renderFullRedraw(state, canvasW, canvasH, tileSize, gridSize, mask, loader);

	// Mark that we may need a final pass to check for clipping issues
	s_state.needsFinalPass = true;

	// Update previous state for next render
	s_state.prevOffsetX = s_state.offsetX;
	s_state.prevOffsetY = s_state.offsetY;
	s_state.prevZoomFactor = s_state.zoomFactor;
	s_state.prevOffsetsValid = true;

	// Render overlays after main canvas rendering
}

/**
 * Render using double-buffer technique for efficient panning.
 */
void renderWithDoubleBuffer(MapViewerState& state, float canvasW, float canvasH,
                            int tileSize, int gridSize, const std::vector<int>& mask,
                            const TileLoader& loader) {
	const int grid_size = effectiveGridSize(gridSize);

	// Calculate the offset delta from last render.
	[[maybe_unused]] const float deltaX = s_state.offsetX - s_state.prevOffsetX;
	[[maybe_unused]] const float deltaY = s_state.offsetY - s_state.prevOffsetY;

	const float bufferX = (canvasW - state.viewportWidth) / 2.0f;
	const float bufferY = (canvasH - state.viewportHeight) / 2.0f;

	// Calculate which tiles should be visible in the current view
	const int startX = std::max(0, static_cast<int>(std::floor(-s_state.offsetX / static_cast<float>(tileSize))));
	const int startY = std::max(0, static_cast<int>(std::floor(-s_state.offsetY / static_cast<float>(tileSize))));
	const int endX = std::min(grid_size, startX + static_cast<int>(std::ceil(canvasW / static_cast<float>(tileSize))) + 1);
	const int endY = std::min(grid_size, startY + static_cast<int>(std::ceil(canvasH / static_cast<float>(tileSize))) + 1);

	// Track tiles that should be visible but aren't rendered

	// Check all tiles in the visible range
	for (int x = startX; x < endX; x++) {
		for (int y = startY; y < endY; y++) {
			const int index = (x * grid_size) + y;

			// Skip if this tile is masked out
			if (!mask.empty() && (index < 0 || index >= static_cast<int>(mask.size()) || mask[index] != 1))
				continue;

			const float drawX = static_cast<float>(x * tileSize) + s_state.offsetX;
			const float drawY = static_cast<float>(y * tileSize) + s_state.offsetY;

			// Check if this tile should be visible in viewport
			const bool isInViewport = !(drawX + static_cast<float>(tileSize) <= bufferX || drawX >= bufferX + state.viewportWidth ||
			                             drawY + static_cast<float>(tileSize) <= bufferY || drawY >= bufferY + state.viewportHeight);

			if (isInViewport) {
				if (s_state.rendered.find(index) == s_state.rendered.end()) {
					// Missing tile — queue for double-buffer rendering
					queueTileForDoubleBuffer(state, x, y, index, tileSize, loader);
				}
				// else: tile already rendered (tracked)
			}
		}
	}

	// Clean up tiles that are no longer visible anywhere on canvas
	std::vector<int> tilesToRemove;
	for (const int index : s_state.rendered) {
		const int x = index / grid_size;
		const int y = index % grid_size;
		const float drawX = static_cast<float>(x * tileSize) + s_state.offsetX;
		const float drawY = static_cast<float>(y * tileSize) + s_state.offsetY;

		if (drawX + static_cast<float>(tileSize) <= 0.0f || drawX >= canvasW ||
		    drawY + static_cast<float>(tileSize) <= 0.0f || drawY >= canvasH)
			tilesToRemove.push_back(index);
	}

	for (int i = 0; i < static_cast<int>(tilesToRemove.size()); i++) {
		s_state.tilePixelCache.erase(tilesToRemove[i]);
		s_state.rendered.erase(tilesToRemove[i]);
		auto texIt = s_state.tileTextures.find(tilesToRemove[i]);
		if (texIt != s_state.tileTextures.end()) {
			if (texIt->second != 0) {
				GLuint gl_tex = static_cast<GLuint>(texIt->second);
				glDeleteTextures(1, &gl_tex);
			}
			s_state.tileTextures.erase(texIt);
		}
	}

	// Copy double-buffer back to main canvas
}

/**
 * Render with full redraw (used for zoom changes, map changes, etc.).
 */
void renderFullRedraw(MapViewerState& state, float canvasW, float canvasH,
                      int tileSize, int gridSize, const std::vector<int>& mask,
                      const TileLoader& loader) {
	const int grid_size = effectiveGridSize(gridSize);

	spdlog::debug("[map-viewer] renderFullRedraw grid_size={} tileSize={} offset=({},{})",
	              grid_size, tileSize, s_state.offsetX, s_state.offsetY);
	spdlog::debug("[map-viewer] mask length={}", mask.size());

	// Clear the entire canvas and rendered set
	for (auto& [idx, tex] : s_state.tileTextures) {
		if (tex != 0) {
			GLuint gl_tex = static_cast<GLuint>(tex);
			glDeleteTextures(1, &gl_tex);
		}
	}
	s_state.tileTextures.clear();
	s_state.rendered.clear();
	s_state.tilePixelCache.clear();
	s_state.tilePixelCacheTileSize = 0;

	// Calculate which tiles are visible
	const int startX = std::max(0, static_cast<int>(std::floor(-s_state.offsetX / static_cast<float>(tileSize))));
	const int startY = std::max(0, static_cast<int>(std::floor(-s_state.offsetY / static_cast<float>(tileSize))));
	const int endX = std::min(grid_size, startX + static_cast<int>(std::ceil(canvasW / static_cast<float>(tileSize))) + 1);
	const int endY = std::min(grid_size, startY + static_cast<int>(std::ceil(canvasH / static_cast<float>(tileSize))) + 1);

	spdlog::debug("[map-viewer] tile range x=[{},{}) y=[{},{})", startX, endX, startY, endY);

	const float bufferX = (canvasW - state.viewportWidth) / 2.0f;
	const float bufferY = (canvasH - state.viewportHeight) / 2.0f;

	int queued = 0;
	// Queue all visible tiles for loading
	for (int x = startX; x < endX; x++) {
		for (int y = startY; y < endY; y++) {
			const float drawX = static_cast<float>(x * tileSize) + s_state.offsetX;
			const float drawY = static_cast<float>(y * tileSize) + s_state.offsetY;

			if (drawX + static_cast<float>(tileSize) <= bufferX || drawX >= bufferX + state.viewportWidth ||
				drawY + static_cast<float>(tileSize) <= bufferY || drawY >= bufferY + state.viewportHeight)
				continue;

			const int index = (x * grid_size) + y;

			// Skip if this tile is masked out
			if (!mask.empty() && (index < 0 || index >= static_cast<int>(mask.size()) || mask[index] != 1))
				continue;

			// Queue tile for loading
			queueTile(state, x, y, index, tileSize, loader);
			queued++;
		}
	}
	spdlog::debug("[map-viewer] queued {} tiles", queued);
}

void renderTiles(const MapViewerState& state, int tileSize_prop) {
	if (state.canvasWidth <= 0.0f || state.canvasHeight <= 0.0f)
		return;

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (!drawList)
		return;

	const int tileSize = static_cast<int>(std::floor(static_cast<float>(tileSize_prop) / static_cast<float>(s_state.zoomFactor)));
	const int grid_size = s_state.lastGridSize;

	const float bufferX = (state.canvasWidth - state.viewportWidth) / 2.0f;
	const float bufferY = (state.canvasHeight - state.viewportHeight) / 2.0f;

	const ImVec2 contentOrigin(state.canvasOriginX, state.canvasOriginY);

	drawList->PushClipRect(
		contentOrigin,
		ImVec2(contentOrigin.x + state.viewportWidth, contentOrigin.y + state.viewportHeight),
		true);

	for (const int index : s_state.rendered) {
		auto texIt = s_state.tileTextures.find(index);
		if (texIt == s_state.tileTextures.end() || texIt->second == 0)
			continue;

		const int x = index / grid_size;
		const int y = index % grid_size;

		const float drawX = static_cast<float>(x * tileSize) + s_state.offsetX;
		const float drawY = static_cast<float>(y * tileSize) + s_state.offsetY;

		const float screenX = contentOrigin.x + drawX - bufferX;
		const float screenY = contentOrigin.y + drawY - bufferY;

		drawList->AddImage(
			static_cast<ImTextureID>(static_cast<uintptr_t>(texIt->second)),
			ImVec2(screenX, screenY),
			ImVec2(screenX + static_cast<float>(tileSize), screenY + static_cast<float>(tileSize)),
			ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
			IM_COL32(255, 255, 255, 255));
	}

	drawList->PopClipRect();
}

/**
 * Render only the overlay canvas with selection and hover states.
 */
void renderOverlay(MapViewerState& state, int tileSize_prop, int gridSize,
                   const std::vector<int>& mask, const std::vector<int>& selection) {
	// If no map has been selected, do not render.

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (!drawList)
		return;

	// Clear the overlay canvas

	// Calculate current tile size based on zoom factor
	const int tileSize = static_cast<int>(std::floor(static_cast<float>(tileSize_prop) / static_cast<float>(s_state.zoomFactor)));
	const int grid_size = effectiveGridSize(gridSize);

	// Calculate which tiles might be visible
	const int startX = std::max(0, static_cast<int>(std::floor(-s_state.offsetX / static_cast<float>(tileSize))));
	const int startY = std::max(0, static_cast<int>(std::floor(-s_state.offsetY / static_cast<float>(tileSize))));
	const int endX = std::min(grid_size, startX + static_cast<int>(std::ceil(state.canvasWidth / static_cast<float>(tileSize))) + 1);
	const int endY = std::min(grid_size, startY + static_cast<int>(std::ceil(state.canvasHeight / static_cast<float>(tileSize))) + 1);

	const float bufferX = (state.canvasWidth - state.viewportWidth) / 2.0f;
	const float bufferY = (state.canvasHeight - state.viewportHeight) / 2.0f;

	const ImVec2 contentOrigin(state.canvasOriginX, state.canvasOriginY);

	// calculate box selection tile range for highlighting
	int boxMinTileX = -1, boxMaxTileX = -1, boxMinTileY = -1, boxMaxTileY = -1;
	if (state.isBoxSelecting && state.boxSelectStart.has_value() && state.boxSelectEnd.has_value()) {
		const MapPosition startPoint = mapPositionFromClientPoint(state, state.boxSelectStart->x, state.boxSelectStart->y, tileSize_prop, gridSize);
		const MapPosition endPoint = mapPositionFromClientPoint(state, state.boxSelectEnd->x, state.boxSelectEnd->y, tileSize_prop, gridSize);
		boxMinTileX = std::min(startPoint.tileX, endPoint.tileX);
		boxMaxTileX = std::max(startPoint.tileX, endPoint.tileX);
		boxMinTileY = std::min(startPoint.tileY, endPoint.tileY);
		boxMaxTileY = std::max(startPoint.tileY, endPoint.tileY);
	}

	// Render overlays for visible tiles
	for (int x = startX; x < endX; x++) {
		for (int y = startY; y < endY; y++) {
			const float drawX = static_cast<float>(x * tileSize) + s_state.offsetX;
			const float drawY = static_cast<float>(y * tileSize) + s_state.offsetY;

			if (drawX + static_cast<float>(tileSize) <= bufferX || drawX >= bufferX + state.viewportWidth ||
				drawY + static_cast<float>(tileSize) <= bufferY || drawY >= bufferY + state.viewportHeight)
				continue;

			const int index = (x * grid_size) + y;

			// This chunk is masked out, so skip rendering it.
			if (!mask.empty() && (index < 0 || index >= static_cast<int>(mask.size()) || mask[index] != 1))
				continue;

			const float screenX = contentOrigin.x + drawX - bufferX;
			const float screenY = contentOrigin.y + drawY - bufferY;

			// Draw the selection overlay if this tile is selected.
			if (std::find(selection.begin(), selection.end(), index) != selection.end()) {
				drawList->AddRectFilled(
					ImVec2(screenX, screenY),
					ImVec2(screenX + static_cast<float>(tileSize), screenY + static_cast<float>(tileSize)),
					(IM_COL32(159, 241, 161, 255) & 0x00FFFFFFu) | 0x80000000u);
			}

			// Draw box selection preview highlight
			if (state.isBoxSelecting && x >= boxMinTileX && x <= boxMaxTileX && y >= boxMinTileY && y <= boxMaxTileY) {
				drawList->AddRectFilled(
					ImVec2(screenX, screenY),
					ImVec2(screenX + static_cast<float>(tileSize), screenY + static_cast<float>(tileSize)),
					(IM_COL32(87, 175, 226, 255) & 0x00FFFFFFu) | 0x80000000u);
			} else if (!state.isBoxSelectMode && state.hoverTile == index) {
				// Draw the hover overlay only when not in box select mode
				drawList->AddRectFilled(
					ImVec2(screenX, screenY),
					ImVec2(screenX + static_cast<float>(tileSize), screenY + static_cast<float>(tileSize)),
					(IM_COL32(87, 175, 226, 255) & 0x00FFFFFFu) | 0x80000000u);
			}
		}
	}

	// draw box selection rectangle outline
	if (state.isBoxSelecting && state.boxSelectStart.has_value() && state.boxSelectEnd.has_value()) {
		const float startScreenX = state.boxSelectStart->x;
		const float startScreenY = state.boxSelectStart->y;
		const float endScreenX = state.boxSelectEnd->x;
		const float endScreenY = state.boxSelectEnd->y;

		const float rectX = std::min(startScreenX, endScreenX);
		const float rectY = std::min(startScreenY, endScreenY);
		const float rectW = std::abs(endScreenX - startScreenX);
		const float rectH = std::abs(endScreenY - startScreenY);

		const ImU32 dashColor = IM_COL32(255, 255, 255, 230);
		const float dashLen = 5.0f;
		const float gapLen = 5.0f;
		const float thickness = 2.0f;

		auto drawDashedLine = [&](float ax, float ay, float bx, float by) {
			const float dx = bx - ax;
			const float dy = by - ay;
			const float lineLen = std::sqrt(dx * dx + dy * dy);
			if (lineLen < 0.001f)
				return;
			const float ux = dx / lineLen;
			const float uy = dy / lineLen;
			float dist = 0.0f;
			bool drawing = true;
			while (dist < lineLen) {
				const float segLen = drawing ? dashLen : gapLen;
				const float end = std::min(dist + segLen, lineLen);
				if (drawing) {
					drawList->AddLine(
						ImVec2(ax + ux * dist, ay + uy * dist),
						ImVec2(ax + ux * end, ay + uy * end),
						dashColor, thickness);
				}
				dist = end;
				drawing = !drawing;
			}
		};

		drawDashedLine(rectX, rectY, rectX + rectW, rectY);
		drawDashedLine(rectX + rectW, rectY, rectX + rectW, rectY + rectH);
		drawDashedLine(rectX + rectW, rectY + rectH, rectX, rectY + rectH);
		drawDashedLine(rectX, rectY + rectH, rectX, rectY);
	}
}

/**
 * Invoked when a key press event is fired on the document.
 * @param event
 */
void handleKeyPress(MapViewerState& state, int gridSize,
                    const std::vector<int>& mask, const std::vector<int>& selection,
                    bool selectable, const SelectionChangedCallback& onSelectionChanged) {
	// Check if the user cursor is over the map viewer.
	if (state.isHovering == false)
		return;

	// Skip selection shortcuts if selection is disabled
	if (selectable == false)
		return;

	const ImGuiIO& io = ImGui::GetIO();

	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
		// Without a WDT mask, we can't reliably select everything.
		if (mask.empty()) {
			core::setToast("error", "Unable to perform Select All operation on this map (Missing WDT)", {}, -1);
			return;
		}

		std::vector<int> newSelection;

		// Iterate over all available tiles in the mask and select them.
		for (int i = 0, n = static_cast<int>(mask.size()); i < n; i++) {
			if (mask[i] == 1)
				newSelection.push_back(i);
		}

		if (onSelectionChanged)
			onSelectionChanged(newSelection);

		// Trigger an overlay re-render to show the new selection.

		// Absorb this event preventing further action.
	} else if (ImGui::IsKeyPressed(ImGuiKey_D)) {
		if (onSelectionChanged)
			onSelectionChanged({});
		// Trigger an overlay re-render to show the new selection.
	} else if (ImGui::IsKeyPressed(ImGuiKey_B)) {
		state.isBoxSelectMode = !state.isBoxSelectMode;

		// clear any in-progress box selection when toggling off
		if (!state.isBoxSelectMode) {
			state.isBoxSelecting = false;
			state.boxSelectStart.reset();
			state.boxSelectEnd.reset();
		}
	}
}

/**
 * @param event
 * @param isFirst
 */
void handleTileInteraction(MapViewerState& state, float clientX, float clientY,
                           int tileSize_prop, int gridSize,
                           const std::vector<int>& mask, std::vector<int>& selection,
                           bool isFirst) {
	// Calculate which chunk we shift-clicked on.
	const MapPosition point = mapPositionFromClientPoint(state, clientX, clientY, tileSize_prop, gridSize);
	const int grid_size = effectiveGridSize(gridSize);
	const int index = (point.tileX * grid_size) + point.tileY;

	// Prevent toggling a tile that we've already touched during this selection.
	if (s_state.selectCache.count(index))
		return;

	s_state.selectCache.insert(index);

	if (!mask.empty()) {
		// If we have a WDT, and this tile is not defined, disallow selection.
		if (index < 0 || index >= static_cast<int>(mask.size()) || mask[index] != 1)
			return;
	}

	auto it = std::find(selection.begin(), selection.end(), index);
	const int check = (it != selection.end()) ? static_cast<int>(std::distance(selection.begin(), it)) : -1;

	if (isFirst)
		state.selectState = (check > -1);

	if (state.selectState && check > -1)
		selection.erase(selection.begin() + check);
	else if (!state.selectState && check == -1)
		selection.push_back(index);

	// Trigger an overlay re-render to show the selection change.
}

/**
 * Invoked on mousemove events captured on the document.
 * @param event
 */
void handleMouseMove(MapViewerState& state, float clientX, float clientY,
                     int tileSize_prop, int gridSize,
                     const std::vector<int>& mask, std::vector<int>& selection,
                     const TileLoader& loader) {
	if (state.isBoxSelecting) {
		state.boxSelectEnd = Point{ clientX, clientY };
	} else if (state.isSelecting) {
		handleTileInteraction(state, clientX, clientY, tileSize_prop, gridSize, mask, selection, false);
	} else if (state.isPanning) {
		// Calculate the distance from our mousedown event.
		const float deltaX = state.mouseBaseX - clientX;
		const float deltaY = state.mouseBaseY - clientY;

		// Update the offset based on our pan base.
		s_state.offsetX = state.panBaseX - deltaX;
		s_state.offsetY = state.panBaseY - deltaY;

		// Offsets are not reactive, manually trigger an update.
		render(state, tileSize_prop, gridSize, mask, loader);
	}
}

/**
 * Invoked on mouseup events captured on the document.
 */
void handleMouseUp(MapViewerState& state, int tileSize_prop, int gridSize,
                   const std::vector<int>& mask, std::vector<int>& selection,
                   bool selectable, const SelectionChangedCallback& onSelectionChanged) {
	if (state.isBoxSelecting) {
		finalizeBoxSelection(state, tileSize_prop, gridSize, mask, selection, onSelectionChanged);
		state.isBoxSelecting = false;
		state.boxSelectStart.reset();
		state.boxSelectEnd.reset();
	}

	if (state.isPanning)
		state.isPanning = false;

	if (state.isSelecting) {
		state.isSelecting = false;
		s_state.selectCache.clear();
	}
}

/**
 * Finalize box selection by selecting all tiles within the box.
 */
void finalizeBoxSelection(MapViewerState& state, int tileSize_prop, int gridSize,
                          const std::vector<int>& mask, std::vector<int>& selection,
                          const SelectionChangedCallback& onSelectionChanged) {
	if (!state.boxSelectStart.has_value() || !state.boxSelectEnd.has_value())
		return;

	const MapPosition startPoint = mapPositionFromClientPoint(state, state.boxSelectStart->x, state.boxSelectStart->y, tileSize_prop, gridSize);
	const MapPosition endPoint = mapPositionFromClientPoint(state, state.boxSelectEnd->x, state.boxSelectEnd->y, tileSize_prop, gridSize);

	const int minTileX = std::min(startPoint.tileX, endPoint.tileX);
	const int maxTileX = std::max(startPoint.tileX, endPoint.tileX);
	const int minTileY = std::min(startPoint.tileY, endPoint.tileY);
	const int maxTileY = std::max(startPoint.tileY, endPoint.tileY);

	const int grid_size = effectiveGridSize(gridSize);
	std::vector<int> newSelection = selection;

	for (int x = minTileX; x <= maxTileX; x++) {
		for (int y = minTileY; y <= maxTileY; y++) {
			if (x < 0 || x >= grid_size || y < 0 || y >= grid_size)
				continue;

			const int index = (x * grid_size) + y;

			// skip masked tiles
			if (!mask.empty() && (index < 0 || index >= static_cast<int>(mask.size()) || mask[index] != 1))
				continue;

			// add to selection if not already selected
			if (std::find(newSelection.begin(), newSelection.end(), index) == newSelection.end())
				newSelection.push_back(index);
		}
	}

	if (onSelectionChanged)
		onSelectionChanged(newSelection);
}

/**
 * Invoked on mousedown events captured on the container element.
 * @param event
 */
void handleMouseDown(MapViewerState& state, float clientX, float clientY,
                     int tileSize_prop, int gridSize,
                     const std::vector<int>& mask, std::vector<int>& selection,
                     bool selectable) {
	const ImGuiIO& io = ImGui::GetIO();

	if (state.isBoxSelectMode && selectable != false) {
		state.isBoxSelecting = true;
		state.boxSelectStart = Point{ clientX, clientY };
		state.boxSelectEnd = Point{ clientX, clientY };
	} else if (io.KeyShift && selectable != false) {
		handleTileInteraction(state, clientX, clientY, tileSize_prop, gridSize, mask, selection, true);
		state.isSelecting = true;
	} else if (!state.isPanning) {
		state.isPanning = true;

		// Store the X/Y of the mouse event to calculate drag deltas.
		state.mouseBaseX = clientX;
		state.mouseBaseY = clientY;

		// Store the current offsetX/offsetY used for relative panning
		// as the user drags the component.
		state.panBaseX = s_state.offsetX;
		state.panBaseY = s_state.offsetY;
	}
}

/**
 * Convert an absolute client point (such as cursor position) to a relative
 * position on the map. Returns { tileX, tileY posX, posY }
 * @param x
 * @param y
 */
MapPosition mapPositionFromClientPoint(const MapViewerState& state, float x, float y,
                                       int tileSize_prop, int gridSize) {
	const float viewportX = state.canvasOriginX;
	const float viewportY = state.canvasOriginY;

	// Calculate canvas position relative to viewport (centered)
	const float canvasOffsetX = (state.viewportWidth - state.canvasWidth) / 2.0f;
	const float canvasOffsetY = (state.viewportHeight - state.canvasHeight) / 2.0f;

	// Convert client coordinates to canvas coordinates
	const float viewOfsX = (x - viewportX - canvasOffsetX) - s_state.offsetX;
	const float viewOfsY = (y - viewportY - canvasOffsetY) - s_state.offsetY;

	const float tileSize = std::floor(static_cast<float>(tileSize_prop) / static_cast<float>(s_state.zoomFactor));

	const double tileX = static_cast<double>(viewOfsX) / static_cast<double>(tileSize);
	const double tileY = static_cast<double>(viewOfsY) / static_cast<double>(tileSize);

	const double posX = MAP_COORD_BASE - (TILE_SIZE * tileX);
	const double posY = MAP_COORD_BASE - (TILE_SIZE * tileY);

	return { static_cast<int>(std::floor(tileX)), static_cast<int>(std::floor(tileY)), posY, posX };
}

/**
 * Centers the map on a given X, Y in-game position.
 * @param x
 * @param y
 */
void setMapPosition(MapViewerState& state, double x, double y,
                    int tileSize_prop, int gridSize,
                    const std::vector<int>& mask, const TileLoader& loader) {
	// Translate to WoW co-ordinates.
	const double posX = y;
	const double posY = x;

	const float tileSize = std::floor(static_cast<float>(tileSize_prop) / static_cast<float>(s_state.zoomFactor));

	const float ofsX = static_cast<float>(((posX - MAP_COORD_BASE) / TILE_SIZE) * static_cast<double>(tileSize));
	const float ofsY = static_cast<float>(((posY - MAP_COORD_BASE) / TILE_SIZE) * static_cast<double>(tileSize));

	const float maxTileSize = static_cast<float>(tileSize_prop);
	s_state.offsetX = ofsX + (state.viewportWidth / 2.0f) + maxTileSize;
	s_state.offsetY = ofsY + (state.viewportHeight / 2.0f) + maxTileSize;

	render(state, tileSize_prop, gridSize, mask, loader);
}

/**
 * Set the zoom factor. This will invalidate the cache.
 * This function will not re-render the preview.
 * @param factor
 */
void setZoomFactor(int factor) {
	s_state.zoomFactor = factor;

	// Invalidate the cache so that tiles are re-rendered.
	clearTileState();
}

/**
 * Invoked when the mouse is moved over the component.
 * @param event
 */
void handleMouseOver(MapViewerState& state, float clientX, float clientY,
                     int tileSize_prop, int gridSize) {
	state.isHovering = true;

	const MapPosition point = mapPositionFromClientPoint(state, clientX, clientY, tileSize_prop, gridSize);
	state.hoverInfo = std::format("{} {} ({} {})",
	                              static_cast<int>(std::floor(point.posX)),
	                              static_cast<int>(std::floor(point.posY)),
	                              point.tileX, point.tileY);

	// If we're not panning, highlight the current tile.
	if (!state.isPanning)
		state.hoverTile = (point.tileX * effectiveGridSize(gridSize)) + point.tileY;
}

/**
 * Invoked when the mouse leaves the component.
 */
void handleMouseOut(MapViewerState& state) {
	state.isHovering = false;

	// Remove the current hover overlay.
	state.hoverTile = -1;
}

/**
 * Invoked on mousewheel events captured on the container element.
 * @param event
 */
void handleMouseWheel(MapViewerState& state, float deltaY, float clientX, float clientY,
                      int tileSize_prop, int gridSize, int zoom,
                      const std::vector<int>& mask, const TileLoader& loader) {
	const int delta = deltaY > 0.0f ? 1 : -1;
	const int newZoom = std::max(1, std::min(zoom, s_state.zoomFactor + delta));

	// Setting the new zoom factor even if it hasn't changed would have no effect due to
	// the zoomFactor watcher being reactive, but we still check it here so that we only
	// pan the map to the new zoom point if we're actually zooming.
	if (newZoom != s_state.zoomFactor) {
		// Get the in-game position of the mouse cursor.
		const MapPosition point = mapPositionFromClientPoint(state, clientX, clientY, tileSize_prop, gridSize);

		// Set the new zoom factor. This will not trigger a re-render.
		setZoomFactor(newZoom);

		// Pan the map to the cursor position.
		setMapPosition(state, point.posX, point.posY, tileSize_prop, gridSize, mask, loader);
	}
}


/**
 * HTML mark-up to render for this component.
 */
void renderWidget(const char* id,
                  MapViewerState& state,
                  const TileLoader& loader,
                  int tileSize_prop,
                  int mapId,
                  int zoom,
                  const std::vector<int>& mask,
                  std::vector<int>& selection,
                  bool selectable,
                  int gridSize,
                  const SelectionChangedCallback& onSelectionChanged) {
	ImGui::PushID(id);

	if (state.prevMap != mapId) {
		state.prevMap = mapId;
		// Reset the cache.
		clearTileState();
		state.initialized = false;
	}

	ImVec2 avail = ImGui::GetContentRegionAvail();
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::BeginChild("##map_viewer_container", avail, ImGuiChildFlags_Borders,
	                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	ImVec2 regionAvail = ImGui::GetContentRegionAvail();
	state.viewportWidth = regionAvail.x;
	state.viewportHeight = regionAvail.y;

	if (!state.initialized && mapId != -1 && state.viewportWidth > 0.0f && state.viewportHeight > 0.0f) {
		state.initialized = true;
		// Set the map position to a default position.
		// This will trigger a re-render for us too.
		setToDefaultPosition(state, tileSize_prop, gridSize, mask, loader);
	}

	ImGui::BeginGroup();
	ImGui::TextUnformatted("Navigate: Click + Drag");
	ImGui::SameLine();
	ImGui::TextUnformatted("Select Tile: Shift + Click");
	ImGui::SameLine();
	ImGui::TextUnformatted("Zoom: Mouse Wheel");
	ImGui::SameLine();
	ImGui::TextUnformatted("Select All: Control + A");
	ImGui::SameLine();
	ImGui::TextUnformatted("Deselect All: D");
	ImGui::SameLine();
	if (state.isBoxSelectMode) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		ImGui::TextUnformatted("Box Select: B");
		ImGui::PopStyleColor();
	} else {
		ImGui::TextUnformatted("Box Select: B");
	}
	ImGui::EndGroup();

	ImVec2 canvasAvail = ImGui::GetContentRegionAvail();
	ImVec2 canvasPos = ImGui::GetCursorScreenPos();

	state.canvasOriginX = canvasPos.x;
	state.canvasOriginY = canvasPos.y;

	ImGui::InvisibleButton("##map_canvas", canvasAvail,
	                        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

	const bool isCanvasHovered = ImGui::IsItemHovered();
	[[maybe_unused]] const bool isCanvasActive = ImGui::IsItemActive();

	const ImGuiIO& io = ImGui::GetIO();
	const ImVec2 mousePos = io.MousePos;


	if (isCanvasHovered) {
		handleMouseOver(state, mousePos.x, mousePos.y, tileSize_prop, gridSize);
	} else if (state.isHovering && !state.isPanning && !state.isSelecting && !state.isBoxSelecting) {
		handleMouseOut(state);
	}

	if (isCanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		handleMouseDown(state, mousePos.x, mousePos.y, tileSize_prop, gridSize, mask, selection, selectable);
	}

	if (state.isPanning || state.isSelecting || state.isBoxSelecting) {
		handleMouseMove(state, mousePos.x, mousePos.y, tileSize_prop, gridSize, mask, selection, loader);
	}

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		if (state.isPanning || state.isSelecting || state.isBoxSelecting) {
			handleMouseUp(state, tileSize_prop, gridSize, mask, selection, selectable, onSelectionChanged);
		}
	}

	if (isCanvasHovered && io.MouseWheel != 0.0f) {
		handleMouseWheel(state, -io.MouseWheel, mousePos.x, mousePos.y,
		                 tileSize_prop, gridSize, zoom, mask, loader);
	}

	if (isCanvasHovered || state.isHovering) {
		handleKeyPress(state, gridSize, mask, selection, selectable, onSelectionChanged);
	}

	if (s_state.finalPassTimer > 0.0) {
		s_state.finalPassTimer -= static_cast<double>(io.DeltaTime);
		if (s_state.finalPassTimer <= 0.0) {
			s_state.finalPassTimer = -1.0;
			performFinalPass(state, tileSize_prop, gridSize, mask, loader);
		}
	}

	if (mapId != -1) {
		renderTiles(state, tileSize_prop);
		renderOverlay(state, tileSize_prop, gridSize, mask, selection);
	}

	if (!state.hoverInfo.empty()) {
		ImDrawList* dl = ImGui::GetWindowDrawList();
		const float textY = canvasPos.y + canvasAvail.y - ImGui::GetFontSize() - 3.0f;
		dl->AddText(ImVec2(canvasPos.x + 3.0f, textY), IM_COL32_WHITE, state.hoverInfo.c_str());
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopID();
}

} // namespace map_viewer
