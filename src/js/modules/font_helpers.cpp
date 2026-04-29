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

std::string codepoint_to_utf8(uint32_t cp) {
	std::string out;
	if (cp <= 0x7F) {
		out.push_back(static_cast<char>(cp));
	} else if (cp <= 0x7FF) {
		out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
	} else if (cp <= 0xFFFF) {
		out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
	} else if (cp <= 0x10FFFF) {
		out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
	}
	return out;
}

// DEVIATION FROM JS: The JS implementation (font_helpers.js lines 38-54) detects glyph
// support by rendering the codepoint twice to an off-screen 32x32 canvas — once with
// `32px monospace` (the fallback) and once with `32px "<font_family>", monospace` (the
// target) — and compares the alpha-channel sums of the two rasters. A non-zero delta
// means the target font produced a different glyph than the fallback, so the codepoint
// is supported.
//
// That approach cannot be mirrored 1:1 in C++ because ImGui has no DOM/canvas
// equivalent: there is no off-screen browser raster of arbitrary system fonts to
// pixel-compare against. Instead we look the codepoint up in the already-loaded ImGui
// font atlas via `ImFontBaked::FindGlyphNoFallback()`. The behavioural consequence is
// that glyphs which exist in the underlying TTF/OTF but were never baked into the
// atlas (e.g. excluded by the active glyph ranges) will be reported as unsupported,
// whereas the JS canvas test would still detect them. This is an unavoidable
// architectural difference between browser/DOM rendering and ImGui's atlas-backed
// glyph storage.
bool check_glyph_support(void* font, uint32_t codepoint) {
	auto* im_font = static_cast<ImFont*>(font);
	if (!im_font)
		im_font = ImGui::GetFont();

	// Use non-fallback baked lookup so missing glyphs that map to fallback are rejected.
	const float baked_size = (im_font == ImGui::GetFont()) ? ImGui::GetFontSize() : im_font->LegacySize;
	ImFontBaked* baked = im_font->GetFontBaked(baked_size);
	return baked && baked->FindGlyphNoFallback(static_cast<ImWchar>(codepoint)) != nullptr;
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

	const std::string utf8 = codepoint_to_utf8(codepoint);
	if (utf8.empty())
		return;

	state.on_glyph_click(utf8);
}

std::string get_random_quote() {
	const auto& quotes = constants::FONT_PREVIEW_QUOTES;

	static std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<size_t> dist(0, quotes.size() - 1);

	return std::string(quotes[dist(rng)]);
}

// JS parity (font_helpers.js lines 124-130): the JS awaits document.fonts.load() and
// then calls document.fonts.check() to confirm the browser actually decoded the
// @font-face. If decoding failed, JS removes the <style> node and throws.
//
// ImGui's AddFontFromMemoryTTF() only queues the font config — the TTF parse and atlas
// rasterisation are deferred until the atlas is built on the next frame. At this point
// in the lifecycle there is no equivalent of document.fonts.check() because the font
// hasn't been parsed yet, so a true post-load decode verification is impossible here.
// The closest synchronous check available is a header sanity check on the raw data
// (recognised sfnt scaler tags), which catches obviously-corrupt blobs the same way
// document.fonts.check() catches undecodable @font-face data.
static bool is_valid_sfnt_header(const uint8_t* data, size_t data_size) {
	if (data == nullptr || data_size < 4)
		return false;

	const uint32_t scaler_tag = (static_cast<uint32_t>(data[0]) << 24) |
		(static_cast<uint32_t>(data[1]) << 16) |
		(static_cast<uint32_t>(data[2]) << 8) |
		static_cast<uint32_t>(data[3]);

	// TrueType (0x00010000), 'true', 'typ1', OpenType ('OTTO'), and TrueType/OpenType
	// collections ('ttcf'). Anything else is not a valid sfnt.
	return scaler_tag == 0x00010000u ||
		scaler_tag == 0x74727565u /* 'true' */ ||
		scaler_tag == 0x74797031u /* 'typ1' */ ||
		scaler_tag == 0x4F54544Fu /* 'OTTO' */ ||
		scaler_tag == 0x74746366u /* 'ttcf' */;
}

std::string inject_font_face(const std::string& font_id, const uint8_t* data, size_t data_size) {
	const std::string style_id = "font-style-" + font_id;
	(void)style_id; // Preserved for JS parity; C++ has no DOM style node.
	const std::string url = URLPolyfill::createObjectURL(std::span<const uint8_t>(data, data_size), "font/ttf");

	if (!is_valid_sfnt_header(data, data_size)) {
		URLPolyfill::revokeObjectURL(url);
		throw std::runtime_error("font failed to decode");
	}

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
