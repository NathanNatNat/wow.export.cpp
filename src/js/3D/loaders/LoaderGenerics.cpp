/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "LoaderGenerics.h"
#include "../../buffer.h"

/**
 * Process a null-terminated string block.
 *
 * @note Return-type deviation from the JS source: the JS implementation returns a
 *       plain object (`{}`) populated with numeric-string keys, which iterates in
 *       insertion order for integer-coercible keys. This C++ port returns
 *       `std::map<uint32_t, std::string>`, which iterates in ascending key order.
 *       Because keys are written here in monotonically increasing offset order both
 *       orderings happen to coincide, and every known caller (ADTLoader, WMOLoader,
 *       WMOLegacyLoader) performs offset-key lookups via `find()` / `operator[]` and
 *       never iterates the container, so the choice of ordered vs. unordered map has
 *       no observable effect. If iteration order ever becomes semantically important,
 *       switch to a vector-of-pairs (preserves insertion order) or
 *       `std::unordered_map` (matches JS hash semantics, but with arbitrary order).
 *
 * @param data       Source buffer to read the chunk from.
 * @param chunkSize  Size of the string block in bytes.
 * @return           Map of byte-offset within the chunk to the null-terminated string
 *                   beginning at that offset.
 */
std::map<uint32_t, std::string> ReadStringBlock(BufferWrapper& data, uint32_t chunkSize) {
	BufferWrapper chunk = data.readBuffer(chunkSize, false);
	const auto& raw = chunk.raw();
	std::map<uint32_t, std::string> entries;

	uint32_t readOfs = 0;
	for (uint32_t i = 0; i < chunkSize; i++) {
		if (raw[i] == 0x0) {
			// Skip padding bytes.
			if (readOfs == i) {
				readOfs += 1;
				continue;
			}

			// Extract string from readOfs to i, stripping any embedded nulls.
			std::string str(reinterpret_cast<const char*>(raw.data() + readOfs), i - readOfs);
			std::erase(str, '\0');
			entries[readOfs] = std::move(str);
			readOfs = i + 1;
		}
	}

	return entries;
}