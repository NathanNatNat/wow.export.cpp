/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

// Stub: see home-showcase.js for the full implementation reference.
//
// The JS Vue component renders (src/js/components/home-showcase.js):
//
//   Template:
//     <h1 id="home-showcase-header">Made with wow.export</h1>
//     <a id="home-showcase" :data-external="current.link" :style="background_style">
//       <video v-if="current.video" :src="current.video" autoplay loop muted playsinline></video>
//       <span v-if="current.title" class="showcase-title">{{ current.title }}</span>
//     </a>
//     <div id="home-showcase-links">
//       <a @click="refresh">Refresh</a>
//       <a data-kb-link="KB011">Feedback</a>
//     </div>
//
//   Data: showcase entries loaded from showcase.json; one chosen randomly at startup.
//   BASE_LAYERS: [{ image: './images/logo.png', size: '50px', position: 'bottom 10px right 10px' }]
//   background_style: CSS background-image compositing of BASE_LAYERS + current.layers.
//   Video: autoplay/loop/muted <video> element (not implemented).
//   Title font: CSS font-family "Gambler" 40px white (not available in ImGui; TODO #140).
//   Feedback: data-kb-link="KB011" → open KB011 help article (tab_help removed; see TODO_TRACKER.md).
//
// See TODO entries 130-140 in TODO_TRACKER.md.

#include "home-showcase.h"

#include <nlohmann/json.hpp>
#include <fstream>

#include "../constants.h"

namespace home_showcase {

static std::vector<ShowcaseEntry> showcases;

void loadShowcases(const std::string& jsonPath) {
	std::ifstream file(jsonPath);
	if (!file.is_open())
		return;

	try {
		nlohmann::json data = nlohmann::json::parse(file);
		showcases.clear();
		for (const auto& entry : data) {
			ShowcaseEntry sc;
			sc.id    = entry.value("id", "");
			sc.link  = entry.value("link", "");
			sc.video = entry.value("video", "");
			sc.title = entry.value("title", "");

			if (entry.contains("layers") && entry["layers"].is_array()) {
				for (const auto& layer : entry["layers"]) {
					ShowcaseLayer sl;
					sl.image    = layer.value("image", "");
					sl.size     = layer.value("size", "");
					sl.position = layer.value("position", "");
					sc.layers.push_back(std::move(sl));
				}
			}
			showcases.push_back(std::move(sc));
		}
	} catch (...) {}
}

size_t getShowcaseCount() {
	return showcases.size();
}

const ShowcaseEntry* getCurrentShowcase(const HomeShowcaseState& state) {
	if (showcases.empty() || state.index < 0 || state.index >= static_cast<int>(showcases.size()))
		return nullptr;
	return &showcases[static_cast<size_t>(state.index)];
}

void refresh(HomeShowcaseState& state) {
	if (!showcases.empty())
		state.index = (state.index + 1) % static_cast<int>(showcases.size());
}

void cleanup(HomeShowcaseState& state) {
	for (auto& [path, entry] : state.textureCache) {
		if (entry.id)
			glDeleteTextures(1, &entry.id);
	}
	state.textureCache.clear();
}

void render(HomeShowcaseState& state, ImVec2 colMin, ImVec2 colMax) {
	(void)state; (void)colMin; (void)colMax;
	// Stub: see file-level comment and TODO entries 130-140.
}

} // namespace home_showcase
