/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>

class BufferWrapper;

// glb magic number: 'glTF' in ascii
inline constexpr uint32_t GLB_MAGIC = 0x46546C67;
inline constexpr uint32_t GLB_VERSION = 2;

// chunk types
inline constexpr uint32_t CHUNK_TYPE_JSON = 0x4E4F534A;
inline constexpr uint32_t CHUNK_TYPE_BIN = 0x004E4942;

class GLBWriter {
public:
	/**
	 * Construct glb writer with json and binary data.
	 * @param json_string The JSON string.
	 * @param bin_buffer The binary buffer.
	 */
	GLBWriter(const std::string& json_string, BufferWrapper& bin_buffer);

	/**
	 * Pack json and binary data into glb container.
	 * @returns A BufferWrapper containing the GLB data.
	 */
	BufferWrapper pack();

private:
	std::string json_string;
	BufferWrapper& bin_buffer;
};
