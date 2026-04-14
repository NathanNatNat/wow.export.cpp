/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

/**
 * Subtitle format conversion utilities.
 */
namespace subtitles {

enum class SubtitleFormat : int {
	SRT = 118,
	SBT = 7
};

/**
 * Convert subtitle text to WebVTT format, handling BOM stripping
 * and format detection.
 *
 * In the JS original, this is an async function that loads the file
 * from CASC. In C++, the caller is responsible for loading the file
 * and passing the decoded text. The CASC integration happens at the
 * call site.
 *
 * @param text Raw subtitle text content (UTF-8, may contain BOM).
 * @param format The subtitle format (SRT or SBT).
 * @return WebVTT formatted subtitle string.
 */
std::string get_subtitles_vtt(std::string_view text, SubtitleFormat format);

// Note: The following functions are internal implementation details and
// are not exported in the JS module. They are kept file-local in
// subtitles.cpp. Only SUBTITLE_FORMAT and get_subtitles_vtt are
// part of the public API (matching JS: module.exports = { SUBTITLE_FORMAT, get_subtitles_vtt }).

} // namespace subtitles
