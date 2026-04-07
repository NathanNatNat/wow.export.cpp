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
#include <atomic>
#include <memory>

/**
 * Font helper utilities.
 *
 * JS equivalent: module.exports = { detect_glyphs_async, get_random_quote, inject_font_face }
 *
 * In C++ (ImGui), glyph detection is done by testing whether a loaded ImFont
 * has a glyph for each codepoint. Font injection loads a TTF/OTF blob into
 * ImGui's font atlas. The glyph grid is rendered via ImGui instead of DOM.
 */
namespace font_helpers {

/**
 * Unicode range for glyph detection.
 * JS equivalent: GLYPH_RANGES array elements.
 */
struct GlyphRange {
	uint32_t start;
	uint32_t end;
	const char* name;
};

/// Predefined glyph ranges to scan.
extern const std::vector<GlyphRange> GLYPH_RANGES;

/// Number of codepoints to process per batch.
inline constexpr int BATCH_SIZE = 64;

/// Delay between batches in milliseconds (0 = immediate).
inline constexpr int BATCH_DELAY = 0;

/**
 * State for an asynchronous glyph detection operation.
 * Holds the detected glyph codepoints and cancellation state.
 */
struct GlyphDetectionState {
	std::atomic<bool> cancelled{false};
	std::vector<uint32_t> detected_codepoints;
	bool complete = false;
	int current_index = 0;
	int total_codepoints = 0;
};

/**
 * Check if a specific glyph codepoint is supported by the loaded font.
 * In ImGui, this checks ImFont::FindGlyphNoFallback().
 *
 * JS equivalent: check_glyph_support(ctx, font_family, char)
 *
 * @param font       Pointer to the ImFont to check (nullptr = current font).
 * @param codepoint  Unicode codepoint to test.
 * @returns true if the font has a glyph for this codepoint.
 */
bool check_glyph_support(void* font, uint32_t codepoint);

/**
 * Begin asynchronous glyph detection for a font.
 * Populates the detection state with discovered codepoints.
 * Call process_glyph_detection_batch() each frame to advance.
 *
 * JS equivalent: detect_glyphs_async(font_family, grid_element, on_glyph_click, on_complete)
 *
 * @param font      Pointer to the ImFont to scan.
 * @param state     Detection state to populate (will be reset).
 */
void detect_glyphs_async(void* font, GlyphDetectionState& state);

/**
 * Process one batch of glyph detection.
 * Call this each frame until state.complete is true.
 *
 * @param font   Pointer to the ImFont to scan.
 * @param state  Detection state being processed.
 */
void process_glyph_detection_batch(void* font, GlyphDetectionState& state);

/**
 * Get a random font preview quote.
 * JS equivalent: get_random_quote()
 *
 * @returns A random quote string.
 */
std::string get_random_quote();

/**
 * Load a TTF/OTF font blob into ImGui's font atlas.
 * The font data is copied into ImGui's atlas and rebuilt.
 *
 * JS equivalent: inject_font_face(font_id, blob_data, log, on_error)
 *
 * @param font_id    Unique identifier for the font.
 * @param data       Raw font file data (TTF/OTF).
 * @param data_size  Size of the font data in bytes.
 * @returns Pointer to the loaded ImFont, or nullptr on failure.
 */
void* inject_font_face(const std::string& font_id, const uint8_t* data, size_t data_size);

} // namespace font_helpers
