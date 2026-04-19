/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once
#include <future>

namespace casc {
namespace realmlist {

/**
 * Load the realm list from cache and/or remote URL.
 *
 * First attempts to read a cached realmlist from disk. Then fetches
 * the latest version from the configured URL, updates the cache,
 * and populates core::view->realmList.
 */
void load();
std::future<void> loadAsync();

} // namespace realmlist
} // namespace casc
