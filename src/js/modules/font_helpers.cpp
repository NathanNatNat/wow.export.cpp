/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "font_helpers.h"
#include "../constants.h"

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <random>

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

// JS: let glyph_detection_canvas = null;
// JS: let glyph_detection_ctx = null;
// JS: let active_detection = null;
// In C++ (ImGui), we don't need a canvas — we check ImFont::FindGlyphNoFallback directly.

// JS: const compute_alpha_sum = (data) => { ... }
// Not needed in C++: ImGui glyph detection doesn't use pixel-based comparison.

// JS: const check_glyph_support = (ctx, font_family, char) => { ... }
bool check_glyph_support(void* font, uint32_t codepoint) {
	auto* im_font = static_cast<ImFont*>(font);
	if (!im_font)
		im_font = ImGui::GetFont();

	// ImFont::IsGlyphInFont checks whether this codepoint has a glyph loaded.
	return im_font->IsGlyphInFont(static_cast<ImWchar>(codepoint));
}

// JS: const detect_glyphs_async = (font_family, grid_element, on_glyph_click, on_complete) => { ... }
void detect_glyphs_async(void* font, GlyphDetectionState& state) {
	// Cancel any previous detection.
	state.cancelled = false;
	state.detected_codepoints.clear();
	state.complete = false;
	state.current_index = 0;

	// Build the list of all codepoints to check.
	state.total_codepoints = 0;
	for (const auto& range : GLYPH_RANGES)
		state.total_codepoints += static_cast<int>(range.end - range.start + 1);

	// First batch will be processed on next call to process_glyph_detection_batch().
}

void process_glyph_detection_batch(void* font, GlyphDetectionState& state) {
	if (state.cancelled || state.complete)
		return;

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
}

// JS: const get_random_quote = () => { ... }
std::string get_random_quote() {
	const auto& quotes = constants::FONT_PREVIEW_QUOTES;

	static std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<size_t> dist(0, quotes.size() - 1);

	return std::string(quotes[dist(rng)]);
}

// JS: const inject_font_face = async (font_id, blob_data, log, on_error) => { ... }
void* inject_font_face(const std::string& /*font_id*/, const uint8_t* data, size_t data_size) {
	// In JS, this creates a @font-face CSS rule with a blob URL and loads it.
	// In C++ (ImGui), we add the font data to ImGui's font atlas.

	ImGuiIO& io = ImGui::GetIO();

	// ImGui takes ownership of the data if we use AddFontFromMemoryTTF,
	// so we need to provide a copy that ImGui can free.
	void* font_data_copy = ImGui::MemAlloc(data_size);
	if (!font_data_copy)
		return nullptr;

	std::memcpy(font_data_copy, data, data_size);

	ImFontConfig config;
	config.FontDataOwnedByAtlas = true;

	ImFont* font = io.Fonts->AddFontFromMemoryTTF(font_data_copy, static_cast<int>(data_size), 16.0f, &config);
	if (!font) {
		// ImGui frees font_data_copy on failure if FontDataOwnedByAtlas is true.
		return nullptr;
	}

	// Rebuild the font atlas.
	io.Fonts->Build();

	return font;
}

} // namespace font_helpers
