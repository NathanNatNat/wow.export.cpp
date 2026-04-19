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
#include <unordered_map>

/**
 * Font helper utilities.
 *
 * JS equivalent: module.exports = { detect_glyphs_async, get_random_quote, inject_font_face }
 *
 * Glyph detection scans configured Unicode ranges in batches. Font injection
 * creates a blob URL for the raw font data and loads the font into ImGui's atlas.
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
	std::function<void(const std::string&)> on_glyph_click;
	std::function<void()> on_complete;
	bool on_complete_called = false;
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
 * @param font            Pointer to the ImFont to scan.
 * @param state           Detection state to populate (will be reset).
 * @param on_glyph_click  Callback invoked by trigger_glyph_click().
 * @param on_complete     Callback invoked when detection finishes.
 */
void detect_glyphs_async(void* font, GlyphDetectionState& state,
	const std::function<void(const std::string&)>& on_glyph_click = {},
	const std::function<void()>& on_complete = {});

/**
 * Process one batch of glyph detection.
 * Call this each frame until state.complete is true.
 *
 * @param font   Pointer to the ImFont to scan.
 * @param state  Detection state being processed.
 */
void process_glyph_detection_batch(void* font, GlyphDetectionState& state);

/**
 * Trigger the stored glyph-click callback for a detected codepoint.
 * Mirrors JS per-cell click handlers that call on_glyph_click(char).
 *
 * @param state      Detection state containing the callback.
 * @param codepoint  Unicode codepoint clicked by the user.
 */
void trigger_glyph_click(GlyphDetectionState& state, uint32_t codepoint);

/**
 * Get a random font preview quote.
 * JS equivalent: get_random_quote()
 *
 * @returns A random quote string.
 */
std::string get_random_quote();

/**
 * Load a TTF/OTF font blob into ImGui's font atlas and return a blob URL.
 *
 * JS equivalent: inject_font_face(font_id, blob_data, log, on_error)
 *
 * @param font_id    Unique identifier for the font.
 * @param data       Raw font file data (TTF/OTF).
 * @param data_size  Size of the font data in bytes.
 * @returns Blob URL string for the loaded font data.
 */
std::string inject_font_face(const std::string& font_id, const uint8_t* data, size_t data_size);

/**
 * Retrieve an injected ImGui font by ID.
 * @param font_id Font identifier used with inject_font_face().
 * @returns Pointer to loaded ImFont or nullptr.
 */
void* get_injected_font(const std::string& font_id);

} // namespace font_helpers
