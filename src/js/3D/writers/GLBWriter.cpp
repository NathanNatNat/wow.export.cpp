/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#include "GLBWriter.h"
#include "../../buffer.h"

#include <span>

namespace {
	// glb magic number: 'glTF' in ascii
	constexpr uint32_t GLB_MAGIC = 0x46546C67;
	constexpr uint32_t GLB_VERSION = 2;

	// chunk types
	constexpr uint32_t CHUNK_TYPE_JSON = 0x4E4F534A;
	constexpr uint32_t CHUNK_TYPE_BIN = 0x004E4942;
}

GLBWriter::GLBWriter(const std::string& json_string, BufferWrapper& bin_buffer)
	: json_string(json_string), bin_buffer(bin_buffer) {}

BufferWrapper GLBWriter::pack() {
	const auto& json_bytes = reinterpret_cast<const uint8_t*>(json_string.data());
	const size_t json_len = json_string.size();

	// calculate padding for json chunk (must be 4-byte aligned, padded with spaces 0x20)
	const size_t json_padding = (4 - (json_len % 4)) % 4;
	const size_t json_chunk_length = json_len + json_padding;

	// calculate padding for bin chunk (must be 4-byte aligned, padded with zeros)
	const size_t bin_len = bin_buffer.byteLength();
	const size_t bin_padding = (4 - (bin_len % 4)) % 4;
	const size_t bin_chunk_length = bin_len + bin_padding;

	// calculate total file length
	// 12 bytes header + 8 bytes json chunk header + json data + 8 bytes bin chunk header + bin data
	const size_t total_length = 12 + 8 + json_chunk_length + 8 + bin_chunk_length;

	BufferWrapper glb = BufferWrapper::alloc(total_length, true);

	// write glb header
	glb.writeUInt32LE(GLB_MAGIC);
	glb.writeUInt32LE(GLB_VERSION);
	glb.writeUInt32LE(static_cast<uint32_t>(total_length));

	// write json chunk
	glb.writeUInt32LE(static_cast<uint32_t>(json_chunk_length));
	glb.writeUInt32LE(CHUNK_TYPE_JSON);
	glb.writeBuffer(std::span<const uint8_t>(json_bytes, json_len));

	// pad json chunk with spaces
	for (size_t i = 0; i < json_padding; i++)
		glb.writeUInt8(0x20);

	// write bin chunk
	glb.writeUInt32LE(static_cast<uint32_t>(bin_chunk_length));
	glb.writeUInt32LE(CHUNK_TYPE_BIN);
	glb.writeBuffer(std::span<const uint8_t>(bin_buffer.raw().data(), bin_len));

	// pad bin chunk with zeros
	for (size_t i = 0; i < bin_padding; i++)
		glb.writeUInt8(0x00);

	return glb;
}
