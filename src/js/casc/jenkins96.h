/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	Based off original works by Robert John Jenkins Junior (Port by bryc).
	https://en.wikipedia.org/wiki/Jenkins_hash_function
	License: MIT
 */
#pragma once

#include <cstdint>
#include <span>
#include <utility>

namespace casc {

/**
 * Compute a Jenkins hashlittle2 hash of the given buffer.
 * @param k    Input bytes to hash.
 * @param init  Primary initialization seed (default 0).
 * @param init2 Secondary initialization seed (default 0).
 * @return Pair of (b, c) hash values as uint32_t.
 *         Use c as 32-bit hash; add b for 64-bit hash. a is not mixed well.
 */
std::pair<uint32_t, uint32_t> jenkins96(std::span<const uint8_t> k, uint32_t init = 0, uint32_t init2 = 0);

} // namespace casc
