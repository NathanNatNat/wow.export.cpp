/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <optional>

/**
 * Map viewer component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props:
 *   ['loader', 'tileSize', 'map', 'zoom', 'mask', 'selection', 'selectable', 'gridSize']
 * emits: ['update:selection']
 *
 * Renders a pannable, zoomable tile grid with tile selection (single/shift-drag/box-select/Ctrl+A),
 * overlay rendering for selection/hover highlights, and efficient double-buffer panning.
 *
 * In ImGui, there is no HTML Canvas. Tiles are rendered as ImGui textures (GL textures)
 * composed via the draw list. The overlay is drawn via ImDrawList primitives.
 */
namespace map_viewer {

// [x, y, index, tileSize, renderTarget]
// renderTarget: 0 = main, 1 = double-buffer
struct TileQueueNode {
	int x = 0;
	int y = 0;
	int index = 0;
	int tileSize = 0;
	int renderTarget = 0; // 0 = 'main', 1 = 'double-buffer'
};

struct MapPosition {
	int tileX = 0;
	int tileY = 0;
	double posX = 0.0;
	double posY = 0.0;
};

struct Point {
	float x = 0.0f;
	float y = 0.0f;
};

struct TileRerenderInfo {
	int x = 0;
	int y = 0;
	int index = 0;
	int tileSize = 0;
};

/**
 * Persisted state for the map-viewer component. This generally goes against the
 * principals of reactive instanced components, but unfortunately nothing else worked
 * for maintaining state. This just means we can only have one map-viewer component.
 */
struct MapViewerPersistedState {
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	int zoomFactor = 2;
	std::vector<TileQueueNode> tileQueue;
	std::unordered_set<int> selectCache;
	std::unordered_set<int> requested; // Track which tiles have been requested to avoid duplicate requests
	std::unordered_set<int> rendered;  // Track which tiles are currently rendered on the canvas
	float prevOffsetX = 0.0f; // Previous offsets to detect panning vs full redraws
	float prevOffsetY = 0.0f;
	int prevZoomFactor = 2;
	bool prevOffsetsValid = false; // Replaces null checks on prevOffsetX/Y
	bool needsFinalPass = false;   // Track if we need to run the final pass after queue is empty
	double finalPassTimer = -1.0;  // Timer for delayed final pass execution (-1 = inactive)
	int activeTileRequests = 0;    // Track number of tiles currently being loaded
	int maxConcurrentTiles = 4;    // Maximum number of tiles to load concurrently
	int renderGeneration = 0;
	/// CPU-side tile pixel cache — stores RGBA data keyed by tile index.
	/// Used for tileHasUnexpectedTransparency() final-pass sampling.
	std::unordered_map<int, std::vector<uint8_t>> tilePixelCache;
	int tilePixelCacheTileSize = 0; // tileSize when cache was populated (invalidated on zoom change)
	int lastGridSize = 64;           // last effective grid size, for coordinate→index mapping
};

/**
 * Per-frame instance state for the map-viewer component.
 * Equivalent to the Vue component's data() reactive state.
 *
 * Props:
 *   loader: Tile loader function.
 *   tileSize: Base size of tiles (before zoom).
 *   map: ID of the current map. We use this to listen for map changes.
 *   zoom: Maximum zoom-out factor allowed.
 *   mask: Chunk mask. Expected gridSize ^ 2 array.
 *   selection: Array defining selected tiles.
 *   selectable: Whether tile selection is enabled (default true).
 *   gridSize: Size of the tile grid (default MAP_SIZE, 64).
 */
struct MapViewerState {
	// data() reactive fields
	std::string hoverInfo;
	int hoverTile = -1;          // -1 = null equivalent
	bool isHovering = false;
	bool isPanning = false;
	bool isSelecting = false;
	bool selectState = true;
	bool isBoxSelectMode = false;
	bool isBoxSelecting = false;
	std::optional<Point> boxSelectStart;
	std::optional<Point> boxSelectEnd;

	// Instance vars (equivalent to non-reactive JS instance properties)
	bool awaitingTile = false;
	float mouseBaseX = 0.0f;
	float mouseBaseY = 0.0f;
	float panBaseX = 0.0f;
	float panBaseY = 0.0f;

	// Change-detection for watchers
	int prevMap = -1;           // Previous map ID for detecting map changes
	int prevHoverTile = -1;     // Previous hover tile for detecting hover changes
	size_t prevSelectionSize = 0; // Previous selection size for detecting changes
	bool initialized = false;   // Whether setToDefaultPosition has been called

	// Viewport dimensions cached from ImGui layout
	float viewportWidth = 0.0f;
	float viewportHeight = 0.0f;
	float canvasWidth = 0.0f;
	float canvasHeight = 0.0f;

	// Screen-space origin of the canvas area, captured before InvisibleButton
	// advances the cursor. Used by mapPositionFromClientPoint and renderOverlay
	// so that coordinate conversion uses the correct origin.
	float canvasOriginX = 0.0f;
	float canvasOriginY = 0.0f;
};

/**
 * Tile loader callback type.
 * Given (x, y, tileSize), asynchronously provides pixel data.
 * Returns std::vector<uint8_t> of RGBA pixels (tileSize * tileSize * 4 bytes),
 * or an empty vector if the tile failed to load.
 *
 * In the JS source this returns a Promise<ImageData|false>.
 * (tile loading is queued and rate-limited by the component itself).
 */
using TileLoader = std::function<std::vector<uint8_t>(int x, int y, int tileSize)>;

/**
 * Selection changed callback type.
 * Equivalent to $emit('update:selection', newSelection).
 */
using SelectionChangedCallback = std::function<void(const std::vector<int>&)>;

MapViewerPersistedState& getPersistedState();


/**
 * Clear tile queue, requested set, and rendered set.
 * Also reset previous tracking to force full redraw.
 */
void clearTileState();

/**
 * Returns the effective grid size, defaulting to MAP_SIZE if not specified.
 */
int effectiveGridSize(int gridSize);

/**
 * Process tiles in the loading queue up to the concurrency limit.
 */
void checkTileQueue(MapViewerState& state, const TileLoader& loader);

/**
 * Perform a final pass to detect and fix tiles with transparency issues.
 * This addresses seams caused by tiles being clipped but still marked as rendered.
 *
 * In the JS source this checks canvas pixel data via getImageData.
 */
void performFinalPass(MapViewerState& state, int tileSize_prop, int gridSize,
                      const std::vector<int>& mask, const TileLoader& loader);

/**
 * Check if a tile has unexpected transparency (indicating clipping issues).
 * Uses efficient sampling to detect transparency without checking every pixel.
 * @param drawX Canvas X position of tile
 * @param drawY Canvas Y position of tile
 * @param tileSize Size of tile
 * @returns True if tile has unexpected transparency
 *
 * In JS this reads Canvas 2D pixel data. In C++ we sample from the
 * CPU-side tilePixelCache populated by loadTile().
 */
bool tileHasUnexpectedTransparency(float drawX, float drawY, int tileSize);

/**
 * Add a tile to the queue to be loaded if not already requested.
 * @param x
 * @param y
 * @param index
 * @param tileSize
 */
void queueTile(MapViewerState& state, int x, int y, int index, int tileSize, const TileLoader& loader);

/**
 * Add a tile to the queue to be loaded for double-buffer rendering.
 * @param x
 * @param y
 * @param index
 * @param tileSize
 */
void queueTileForDoubleBuffer(MapViewerState& state, int x, int y, int index, int tileSize, const TileLoader& loader);

/**
 * Load a given tile and draw it to the appropriate canvas.
 * Triggers a queue-check once loaded.
 * @param tile
 */
void loadTile(MapViewerState& state, const TileQueueNode& tile, const TileLoader& loader);

/**
 * Set the map to a sensible default position. Centers the view on the
 * middle of available tiles in the mask.
 */
void setToDefaultPosition(MapViewerState& state, int tileSize_prop, int gridSize,
                          const std::vector<int>& mask);

/**
 * Calculate optimal canvas dimensions based on tile size and zoom levels.
 * Canvas is sized to accommodate full tiles with a buffer zone that ensures
 * tiles are never rendered partially at any zoom level.
 */
std::pair<float, float> calculateCanvasSize(const MapViewerState& state, int tileSize_prop);

/**
 * Update the position of the internal container with double-buffer optimization.
 */
void render(MapViewerState& state, int tileSize_prop, int gridSize,
            const std::vector<int>& mask, const TileLoader& loader);

/**
 * Render using double-buffer technique for efficient panning.
 */
void renderWithDoubleBuffer(MapViewerState& state, float canvasW, float canvasH,
                            int tileSize, int gridSize, const std::vector<int>& mask,
                            const TileLoader& loader);

/**
 * Render with full redraw (used for zoom changes, map changes, etc.).
 */
void renderFullRedraw(MapViewerState& state, float canvasW, float canvasH,
                      int tileSize, int gridSize, const std::vector<int>& mask,
                      const TileLoader& loader);

/**
 * Render only the overlay canvas with selection and hover states.
 */
void renderOverlay(MapViewerState& state, int tileSize_prop, int gridSize,
                   const std::vector<int>& mask, const std::vector<int>& selection);

/**
 * Invoked when a key press event is fired on the document.
 * @param event — ImGui key state
 */
void handleKeyPress(MapViewerState& state, int gridSize,
                    const std::vector<int>& mask, const std::vector<int>& selection,
                    bool selectable, const SelectionChangedCallback& onSelectionChanged);

/**
 * @param event
 * @param isFirst
 */
void handleTileInteraction(MapViewerState& state, float clientX, float clientY,
                           int tileSize_prop, int gridSize,
                           const std::vector<int>& mask, std::vector<int>& selection,
                           bool isFirst);

/**
 * Invoked on mousemove events captured on the document.
 * @param event
 */
void handleMouseMove(MapViewerState& state, float clientX, float clientY,
                     int tileSize_prop, int gridSize,
                     const std::vector<int>& mask, std::vector<int>& selection,
                     const TileLoader& loader);

/**
 * Invoked on mouseup events captured on the document.
 */
void handleMouseUp(MapViewerState& state, int tileSize_prop, int gridSize,
                   const std::vector<int>& mask, std::vector<int>& selection,
                   bool selectable, const SelectionChangedCallback& onSelectionChanged);

/**
 * Finalize box selection by selecting all tiles within the box.
 */
void finalizeBoxSelection(MapViewerState& state, int tileSize_prop, int gridSize,
                          const std::vector<int>& mask, std::vector<int>& selection,
                          const SelectionChangedCallback& onSelectionChanged);

/**
 * Invoked on mousedown events captured on the container element.
 * @param event
 */
void handleMouseDown(MapViewerState& state, float clientX, float clientY,
                     int tileSize_prop, int gridSize,
                     const std::vector<int>& mask, std::vector<int>& selection,
                     bool selectable);

/**
 * Convert an absolute client point (such as cursor position) to a relative
 * position on the map. Returns { tileX, tileY posX, posY }
 * @param x
 * @param y
 */
MapPosition mapPositionFromClientPoint(const MapViewerState& state, float x, float y,
                                       int tileSize_prop, int gridSize);

/**
 * Centers the map on a given X, Y in-game position.
 * @param x
 * @param y
 */
void setMapPosition(MapViewerState& state, double x, double y,
                    int tileSize_prop, int gridSize,
                    const std::vector<int>& mask, const TileLoader& loader);

/**
 * Set the zoom factor. This will invalidate the cache.
 * This function will not re-render the preview.
 * @param factor
 */
void setZoomFactor(int factor);

/**
 * Invoked when the mouse is moved over the component.
 * @param event
 */
void handleMouseOver(MapViewerState& state, float clientX, float clientY,
                     int tileSize_prop, int gridSize);

/**
 * Invoked when the mouse leaves the component.
 */
void handleMouseOut(MapViewerState& state);

/**
 * Invoked on mousewheel events captured on the container element.
 * @param event
 */
void handleMouseWheel(MapViewerState& state, float deltaY, float clientX, float clientY,
                      int tileSize_prop, int gridSize, int zoom,
                      const std::vector<int>& mask, const TileLoader& loader);

/**
 * Main render entry point for the map-viewer ImGui widget.
 *
 * This is the top-level function that replaces the Vue template + component lifecycle.
 * Call once per frame from the parent tab/module.
 *
 * @param id              Unique ImGui ID for this widget.
 * @param state           Per-instance state.
 * @param loader          Tile loader callback.
 * @param tileSize_prop   Base tile size (before zoom).
 * @param mapId           Current map ID (-1 = no map).
 * @param zoom            Maximum zoom-out factor.
 * @param mask            Chunk mask (gridSize^2 entries, 1=valid).
 * @param selection       Currently selected tile indices.
 * @param selectable      Whether tile selection is enabled.
 * @param gridSize        Size of the tile grid (0 = use MAP_SIZE default).
 * @param onSelectionChanged Callback when selection changes.
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
                  const SelectionChangedCallback& onSelectionChanged);

} // namespace map_viewer
