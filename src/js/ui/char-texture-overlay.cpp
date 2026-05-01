/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "char-texture-overlay.h"
#include "../core.h"

#include <algorithm>

namespace char_texture_overlay {

static std::vector<uint32_t> layers;
static uint32_t active_layer = 0;
static bool overlay_buttons_visible = false;
static bool event_hooks_registered = false;

static void update_button_visibility() {
	overlay_buttons_visible = (layers.size() > 1);
}

static void nextOverlay() {
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

static void prevOverlay() {
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

static void register_event_hooks() {
	if (event_hooks_registered)
		return;

	core::events.on("screen-tab-characters", []() {
		ensureActiveLayerAttached();
	});
	core::events.on("click-chr-next-overlay", []() {
		nextOverlay();
	});
	core::events.on("click-chr-prev-overlay", []() {
		prevOverlay();
	});

	event_hooks_registered = true;
}

void add(uint32_t textureID) {
	register_event_hooks();

	layers.push_back(textureID);

	if (active_layer == 0)
		active_layer = textureID;

	update_button_visibility();
}

void remove(uint32_t textureID) {
	register_event_hooks();

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
	register_event_hooks();
}

uint32_t getActiveLayer() {
	register_event_hooks();
	return active_layer;
}

bool areButtonsVisible() {
	register_event_hooks();
	return overlay_buttons_visible;
}

} // namespace char_texture_overlay
