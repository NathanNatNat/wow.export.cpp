/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "home-showcase.h"

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <random>
#include <cmath>

#include "../external-links.h"

namespace home_showcase {

// const showcases = require('../showcase.json');
static std::vector<ShowcaseEntry> showcases;

// const BASE_LAYERS = [{ image: './images/logo.png', size: '50px', position: 'bottom 10px right 10px' }];
static const ShowcaseLayer BASE_LAYER = { "./images/logo.png", "50px", "bottom 10px right 10px" };

/**
 * function get_random_index() { return Math.floor(Math.random() * showcases.length); }
 */
static int get_random_index() {
	if (showcases.empty())
		return 0;

	static std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, static_cast<int>(showcases.size()) - 1);
	return dist(rng);
}

void loadShowcases(const std::string& jsonPath) {
	std::ifstream file(jsonPath);
	if (!file.is_open())
		return;

	try {
		nlohmann::json data = nlohmann::json::parse(file);
		showcases.clear();
		for (const auto& entry : data) {
			ShowcaseEntry sc;
			sc.id = entry.value("id", "");
			sc.link = entry.value("link", "");
			sc.video = entry.value("video", "");
			sc.title = entry.value("title", "");

			if (entry.contains("layers") && entry["layers"].is_array()) {
				for (const auto& layer : entry["layers"]) {
					ShowcaseLayer sl;
					sl.image = layer.value("image", "");
					sl.size = layer.value("size", "");
					sl.position = layer.value("position", "");
					sc.layers.push_back(std::move(sl));
				}
			}

			showcases.push_back(std::move(sc));
		}
	} catch (...) {
		// JSON parse failed — leave showcases empty.
	}
}

size_t getShowcaseCount() {
	return showcases.size();
}

const ShowcaseEntry* getCurrentShowcase(const HomeShowcaseState& state) {
	if (showcases.empty() || state.index < 0 || state.index >= static_cast<int>(showcases.size()))
		return nullptr;
	return &showcases[static_cast<size_t>(state.index)];
}

// methods: refresh() { this.index = (this.index + 1) % showcases.length; }
void refresh(HomeShowcaseState& state) {
	if (!showcases.empty())
		state.index = (state.index + 1) % static_cast<int>(showcases.size());
}

/**
 * HTML mark-up to render for this component.
 *
 * template: `
 *   <h1 id="home-showcase-header">Made with wow.export</h1>
 *   <a id="home-showcase" :data-external="current.link" :style="background_style">
 *     <video v-if="current.video" :src="current.video" autoplay loop muted playsinline></video>
 *     <span v-if="current.title" class="showcase-title">{{ current.title }}</span>
 *   </a>
 *   <div id="home-showcase-links">
 *     <a @click="refresh">Refresh</a>
 *     <a data-kb-link="KB011">Feedback</a>
 *   </div>
 * `
 */
void render(HomeShowcaseState& state) {
	// data() { return { index: get_random_index() }; }
	if (!state.initialized) {
		state.index = get_random_index();
		state.initialized = true;
	}

	// <h1 id="home-showcase-header">Made with wow.export</h1>
	ImGui::Text("Made with wow.export.cpp");
	ImGui::Separator();

	// computed: current() { return showcases[this.index]; }
	const ShowcaseEntry* current = getCurrentShowcase(state);
	if (current) {
		// <a id="home-showcase" :data-external="current.link">
		// Showcase link — clicking opens external link.
		if (!current->link.empty()) {
			ImGui::Text("Showcase: %s", current->id.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Open Link"))
				ExternalLinks::open(current->link);
		}

		// <span v-if="current.title" class="showcase-title">{{ current.title }}</span>
		if (!current->title.empty())
			ImGui::Text("%s", current->title.c_str());

		// Note: Video playback and background image layers are not supported in ImGui.
		// These are visual features that require a full graphics pipeline.
		// The background_style() computed property builds CSS background layers
		// from BASE_LAYERS + showcase.layers, which has no ImGui equivalent.
	} else {
		ImGui::Text("No showcases available.");
	}

	// <div id="home-showcase-links">
	//   <a @click="refresh">Refresh</a>
	//   <a data-kb-link="KB011">Feedback</a>
	// </div>
	if (ImGui::SmallButton("Refresh"))
		refresh(state);
	ImGui::SameLine();
	if (ImGui::SmallButton("Feedback"))
		ExternalLinks::open("KB011");
}

} // namespace home_showcase
