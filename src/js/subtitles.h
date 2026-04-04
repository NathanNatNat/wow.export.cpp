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
 * Parse an SBT timestamp (HH:MM:SS:FF) into milliseconds.
 * @param timestamp SBT timestamp string (e.g., "00:01:14:12").
 * @return Timestamp in milliseconds.
 */
int64_t parse_sbt_timestamp(std::string_view timestamp);

/**
 * Format milliseconds as an SRT timestamp (HH:MM:SS,mmm).
 * @param ms Time in milliseconds.
 * @return Formatted SRT timestamp string.
 */
std::string format_srt_timestamp(int64_t ms);

/**
 * Format milliseconds as a VTT timestamp (HH:MM:SS.mmm).
 * @param ms Time in milliseconds.
 * @return Formatted VTT timestamp string.
 */
std::string format_vtt_timestamp(int64_t ms);

/**
 * Parse an SRT timestamp (HH:MM:SS,mmm) into milliseconds.
 * @param timestamp SRT timestamp string (e.g., "00:01:14,500").
 * @return Timestamp in milliseconds.
 */
int64_t parse_srt_timestamp(std::string_view timestamp);

/**
 * Convert SBT subtitle text to SRT format.
 * @param sbt SBT subtitle content.
 * @return SRT formatted subtitle string.
 */
std::string sbt_to_srt(std::string_view sbt);

/**
 * Convert SRT subtitle text to WebVTT format.
 * @param srt SRT subtitle content.
 * @return WebVTT formatted subtitle string.
 */
std::string srt_to_vtt(std::string_view srt);

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

} // namespace subtitles
