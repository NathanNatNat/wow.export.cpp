/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <imgui.h>
#include <glad/gl.h>

/**
 * Home showcase component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: src/js/components/home-showcase.js
 *
 * Renders a showcase entry from showcase.json using background image layers
 * composited on-screen. Video playback (TODO #135) is not implemented.
 */
namespace home_showcase {

/**
 * A single layer in a showcase background.
 * JS equivalent: element of BASE_LAYERS or showcase.layers array.
 */
struct ShowcaseLayer {
	std::string image;
	std::string size;
	std::string position;
};

/**
 * A single showcase entry loaded from showcase.json.
 */
struct ShowcaseEntry {
	std::string id;
	std::string link;
	std::string video;   // Optional video path (not rendered; TODO #135).
	std::string title;   // Optional title overlay.
	std::vector<ShowcaseLayer> layers;
};

/**
 * Cached GL texture with dimensions.
 */
struct TextureEntry {
	GLuint id = 0;
	int width = 0;
	int height = 0;
};

/**
 * Persistent state for the home showcase widget.
 * JS equivalent: data() { return { index: get_random_index() }; }
 */
struct HomeShowcaseState {
	int index = -1;          // Current showcase index; -1 = uninitialized.
	bool initialized = false;
	// Image texture cache: image path → loaded GL texture + dimensions.
	// TODO #134: BASE_LAYERS is rendered as a std::vector (fixed from singular BASE_LAYER).
	std::unordered_map<std::string, TextureEntry> textureCache;
};

/**
 * Load showcase entries from the showcase.json data file.
 * JS equivalent: const showcases = require('../showcase.json');
 */
void loadShowcases(const std::string& jsonPath);

/**
 * Get the number of loaded showcase entries.
 */
size_t getShowcaseCount();

/**
 * Get the currently selected showcase entry, or nullptr.
 */
const ShowcaseEntry* getCurrentShowcase(const HomeShowcaseState& state);

/**
 * Advance to the next showcase entry (wraps around).
 * JS equivalent: refresh() { this.index = (this.index + 1) % showcases.length; }
 */
void refresh(HomeShowcaseState& state);

/**
 * Render the home showcase component using ImGui into the given column bounds.
 *
 * Draws:
 *   1. h1 header "Made with wow.export.cpp" above the showcase area.
 *   2. #home-showcase: bordered, rounded panel with background image layers.
 *   3. #home-showcase-links: Refresh / Feedback links below the panel.
 *
 * @param state   Persistent state across frames.
 * @param colMin  Top-left of the available left column area (screen coords).
 * @param colMax  Bottom-right of the available left column area (screen coords).
 */
void render(HomeShowcaseState& state, ImVec2 colMin, ImVec2 colMax);

/**
 * Free all GL textures held in the state's texture cache.
 */
void cleanup(HomeShowcaseState& state);

} // namespace home_showcase
