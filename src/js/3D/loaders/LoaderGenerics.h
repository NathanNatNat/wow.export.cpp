/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <map>
#include <string>

class BufferWrapper;

/**
 * Process a null-terminated string block.
 *
 * @note Return-type deviation from the JS source: the JS implementation returns a
 *       plain object (`{}`) populated with numeric-string keys (e.g. `entries[42] = ...`),
 *       which iterates in insertion order for integer-coercible keys. This C++ port
 *       returns `std::map<uint32_t, std::string>`, which iterates in ascending key order.
 *       In practice both orderings are equivalent here because keys are written in
 *       monotonically increasing offset order by this function. More importantly, every
 *       known caller (ADTLoader textures/m2Names/wmoNames; WMOLoader and WMOLegacyLoader
 *       textureNames/groupNames/doodadNames) performs offset-key lookups via `find()` or
 *       `operator[]` and never iterates the container, so the choice of ordered vs.
 *       unordered map has no observable effect. If iteration order ever becomes
 *       semantically important, switch to a vector-of-pairs (preserves insertion order)
 *       or `std::unordered_map` (matches JS hash semantics, but with arbitrary order).
 *
 * @param data       Source buffer to read the chunk from.
 * @param chunkSize  Size of the string block in bytes.
 * @return           Map of byte-offset within the chunk to the null-terminated string
 *                   beginning at that offset.
 */
std::map<uint32_t, std::string> ReadStringBlock(BufferWrapper& data, uint32_t chunkSize);
