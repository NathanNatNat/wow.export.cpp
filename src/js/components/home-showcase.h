/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>

/**
 * Home showcase component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component that displays a showcase entry from
 * showcase.json on the home screen. Features background image layers,
 * optional video, title overlay, and a "Refresh" link to cycle entries.
 */
namespace home_showcase {

/**
 * A single layer in a showcase background.
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
	std::string video;   // Optional video URL.
	std::string title;   // Optional title overlay.
	std::vector<ShowcaseLayer> layers;
};

/**
 * Persistent state for the home showcase widget.
 */
struct HomeShowcaseState {
	int index = -1;       // Current showcase index; -1 = uninitialized.
	bool initialized = false;
};

/**
 * Load showcase entries from the showcase.json data file.
 * Should be called once during application initialization.
 */
void loadShowcases(const std::string& jsonPath);

/**
 * Get the number of loaded showcase entries.
 */
size_t getShowcaseCount();

/**
 * Get the currently selected showcase entry.
 * Returns nullptr if no showcases are loaded.
 */
const ShowcaseEntry* getCurrentShowcase(const HomeShowcaseState& state);

/**
 * Advance to the next showcase entry (wraps around).
 */
void refresh(HomeShowcaseState& state);

/**
 * Render the home showcase component using ImGui.
 *
 * @param state  Persistent state across frames.
 */
void render(HomeShowcaseState& state);

} // namespace home_showcase
