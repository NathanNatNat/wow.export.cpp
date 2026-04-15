/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>

class BufferWrapper;

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
