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
#include <optional>

namespace casc {

class BLTEStreamReader;

struct VP9Config {
	std::string codec;
	uint32_t codedWidth = 0;
	uint32_t codedHeight = 0;
	std::string hardwareAcceleration;
};

struct FrameInfo {
	std::string type;       // "key"
	int64_t timestamp = 0;  // microseconds
	int64_t duration = 0;   // microseconds
	std::vector<uint8_t> data;
};

struct ParseResult {
	std::vector<std::vector<uint8_t>> frames;
	std::vector<uint8_t> remainder;
	bool hasRemainder = false;
};

/**
 * VP9AVIDemuxer - Minimal AVI container parser for VP9-encoded video streams
 * Extracts video frames from VP9 AVI files for playback
 *
 * JS equivalent: class VP9AVIDemuxer — module.exports = VP9AVIDemuxer
 */
class VP9AVIDemuxer {
public:
	explicit VP9AVIDemuxer(BLTEStreamReader& stream_reader);

	/**
	 * Parse AVI header to extract codec configuration.
	 * @returns VP9Config with codec parameters, or std::nullopt if strf chunk is missing.
	 */
	std::optional<VP9Config> parse_header();

	/**
	 * Find chunk by fourCC identifier in AVI data.
	 * @param data AVI data buffer.
	 * @param fourcc Four-character code (null-terminated string literal) to search for.
	 * @returns Offset of chunk, or -1 if not found.
	 */
	int64_t find_chunk(const std::vector<uint8_t>& data, const char (&fourcc)[5]);

	/**
	 * Extract video frames from stream blocks.
	 * Invokes callback for each extracted frame.
	 * @param callback Function called with each frame's info.
	 */
	void extract_frames(const std::function<void(const FrameInfo&)>& callback);

	/**
	 * Parse movi chunk to extract individual video frames with remainder handling.
	 * @param data Block data containing video frames.
	 * @param data_size Size of the data buffer.
	 * @param skip_header Whether to skip AVI header on first block.
	 * @returns ParseResult with frames and remainder.
	 */
	ParseResult parse_movi_frames_with_remainder(const uint8_t* data, size_t data_size, bool skip_header);

	/**
	 * Parse movi chunk to extract individual video frames (deprecated, kept for compatibility).
	 * @param data Block data containing video frames.
	 * @param data_size Size of the data buffer.
	 * @param skip_header Whether to skip AVI header on first block.
	 * @returns Array of frame data buffers.
	 */
	std::vector<std::vector<uint8_t>> parse_movi_frames(const uint8_t* data, size_t data_size, bool skip_header);

private:
	BLTEStreamReader& reader;
	std::optional<VP9Config> config;
	double frame_rate;
};

} // namespace casc
