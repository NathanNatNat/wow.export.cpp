/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "font_helpers.h"
#include "../constants.h"
#include "../blob.h"

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <random>
#include <stdexcept>

#include <imgui.h>

namespace font_helpers {

const std::vector<GlyphRange> GLYPH_RANGES = {
	{ 0x0020, 0x007F, "Basic Latin" },
	{ 0x00A0, 0x00FF, "Latin-1 Supplement" },
	{ 0x0100, 0x017F, "Latin Extended-A" },
	{ 0x0180, 0x024F, "Latin Extended-B" },
	{ 0x0400, 0x04FF, "Cyrillic" },
	{ 0x0370, 0x03FF, "Greek" }
};

// --- Internal state ---
static GlyphDetectionState* active_detection = nullptr;
static std::unordered_map<std::string, ImFont*> loaded_font_faces;

bool check_glyph_support(void* font, uint32_t codepoint) {
	auto* im_font = static_cast<ImFont*>(font);
	if (!im_font)
		im_font = ImGui::GetFont();

	// Use non-fallback lookup so missing glyphs that map to fallback are rejected.
	return im_font->FindGlyphNoFallback(static_cast<ImWchar>(codepoint)) != nullptr;
}

void detect_glyphs_async(void* /*font*/, GlyphDetectionState& state,
	const std::function<void(const std::string&)>& on_glyph_click,
	const std::function<void()>& on_complete) {
	if (active_detection && active_detection != &state)
		active_detection->cancelled = true;

	active_detection = &state;
	state.cancelled = false;
	state.detected_codepoints.clear();
	state.complete = false;
	state.current_index = 0;
	state.on_glyph_click = on_glyph_click;
	state.on_complete = on_complete;
	state.on_complete_called = false;

	// Build the list of all codepoints to check.
	state.total_codepoints = 0;
	for (const auto& range : GLYPH_RANGES)
		state.total_codepoints += static_cast<int>(range.end - range.start + 1);

	// First batch will be processed on next call to process_glyph_detection_batch().
}

void process_glyph_detection_batch(void* font, GlyphDetectionState& state) {
	if (state.cancelled || state.complete)
		return;

	if (active_detection != &state) {
		state.cancelled = true;
		return;
	}

	// Build a flat codepoint list (same as JS all_codepoints).
	// For efficiency, we compute it on the fly from ranges + current_index.
	int global_index = 0;
	int batch_processed = 0;

	for (const auto& range : GLYPH_RANGES) {
		for (uint32_t code = range.start; code <= range.end; code++) {
			if (global_index < state.current_index) {
				global_index++;
				continue;
			}

			if (batch_processed >= BATCH_SIZE) {
				state.current_index = global_index;
				return;
			}

			if (check_glyph_support(font, code))
				state.detected_codepoints.push_back(code);

			global_index++;
			batch_processed++;
		}
	}

	// All codepoints have been processed.
	state.current_index = global_index;
	state.complete = true;

	if (!state.on_complete_called && state.on_complete) {
		state.on_complete_called = true;
		state.on_complete();
	}
}

void trigger_glyph_click(GlyphDetectionState& state, uint32_t codepoint) {
	if (!state.on_glyph_click)
		return;

	char utf8_buf[5] = {};
	const int bytes = ImTextCharToUtf8(utf8_buf, static_cast<unsigned int>(codepoint));
	if (bytes <= 0)
		return;

	state.on_glyph_click(std::string(utf8_buf, utf8_buf + bytes));
}

std::string get_random_quote() {
	const auto& quotes = constants::FONT_PREVIEW_QUOTES;

	static std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<size_t> dist(0, quotes.size() - 1);

	return std::string(quotes[dist(rng)]);
}

std::string inject_font_face(const std::string& font_id, const uint8_t* data, size_t data_size) {
	const std::string style_id = "font-style-" + font_id;
	(void)style_id; // Preserved for JS parity; C++ has no DOM style node.
	const std::string url = URLPolyfill::createObjectURL(std::span<const uint8_t>(data, data_size), "font/ttf");

	ImGuiIO& io = ImGui::GetIO();

	void* font_data_copy = ImGui::MemAlloc(data_size);
	if (!font_data_copy) {
		URLPolyfill::revokeObjectURL(url);
		throw std::runtime_error("font failed to decode");
	}

	std::memcpy(font_data_copy, data, data_size);

	ImFontConfig config;
	config.FontDataOwnedByAtlas = true;

	ImFont* font = io.Fonts->AddFontFromMemoryTTF(font_data_copy, static_cast<int>(data_size), 16.0f, &config);
	if (!font) {
		ImGui::MemFree(font_data_copy);
		URLPolyfill::revokeObjectURL(url);
		throw std::runtime_error("font failed to decode");
	}

	if (font->FindGlyphNoFallback(static_cast<ImWchar>('A')) == nullptr &&
		font->FindGlyphNoFallback(static_cast<ImWchar>('a')) == nullptr) {
		URLPolyfill::revokeObjectURL(url);
		throw std::runtime_error("font failed to decode");
	}

	loaded_font_faces[font_id] = font;
	return url;
}

void* get_injected_font(const std::string& font_id) {
	const auto it = loaded_font_faces.find(font_id);
	if (it == loaded_font_faces.end())
		return nullptr;
	return it->second;
}

} // namespace font_helpers
