/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "char-texture-overlay.h"

#include <algorithm>

namespace char_texture_overlay {

static std::vector<uint32_t> layers;
static uint32_t active_layer = 0;

// rendering (layers.size() > 1 check), so this is a no-op.
static void update_button_visibility() {
	// In ImGui, the caller checks getLayerCount() > 1 to decide whether to
	// show next/prev buttons.
}

void add(uint32_t textureID) {
	layers.push_back(textureID);

	if (active_layer == 0) {
		active_layer = textureID;
	}

	update_button_visibility();
}

void remove(uint32_t textureID) {
	auto it = std::find(layers.begin(), layers.end(), textureID);
	if (it != layers.end())
		layers.erase(it);

	if (textureID == active_layer) {
		active_layer = 0;

		if (!layers.empty())
			active_layer = layers.back();
	}

	update_button_visibility();
}

void ensureActiveLayerAttached() {
	// The JS version uses process.nextTick() to re-attach a canvas to the DOM
	// after a tab switch. ImGui doesn't need this because getActiveLayer() is
	// read fresh each frame.
}

// legacy event for non-module usage

void nextOverlay() {
	if (layers.empty() || active_layer == 0)
		return;

	// Move to the next (or first) layer.
	auto it = std::find(layers.begin(), layers.end(), active_layer);
	if (it == layers.end())
		return;

	size_t index = static_cast<size_t>(std::distance(layers.begin(), it));
	size_t next_index = (index + 1) % layers.size();

	active_layer = layers[next_index];
}

void prevOverlay() {
	if (layers.empty() || active_layer == 0)
		return;

	// Move to the previous (or last) layer.
	auto it = std::find(layers.begin(), layers.end(), active_layer);
	if (it == layers.end())
		return;

	size_t index = static_cast<size_t>(std::distance(layers.begin(), it));
	size_t prev_index = (index - 1 + layers.size()) % layers.size();

	active_layer = layers[prev_index];
}

uint32_t getActiveLayer() {
	return active_layer;
}

size_t getLayerCount() {
	return layers.size();
}

} // namespace char_texture_overlay