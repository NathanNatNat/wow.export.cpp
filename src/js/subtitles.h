/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace casc {
class CASC;
}

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
 * @param casc CASC source used to load subtitle file contents.
 * @param file_data_id File data ID of subtitle payload.
 * @param format The subtitle format (SRT or SBT).
 * @return WebVTT formatted subtitle string.
 */
std::string get_subtitles_vtt(casc::CASC* casc, uint32_t file_data_id, SubtitleFormat format);

// Internal helper used by the public API above.
std::string get_subtitles_vtt_from_text(std::string_view text, SubtitleFormat format);

} // namespace subtitles
