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
 * Defines static links which can be referenced by key, and provides
 * platform-specific methods for opening external URLs.
 *
 * JS equivalent: module.exports = class ExternalLinks (static methods only)
 */
namespace external_links {

/**
 * Open an external link on the system.
 * If the link starts with "::", it is resolved from the static links table.
 * @param link URL or static link key (e.g. "::GITHUB").
 */
void open(std::string_view link);

/**
 * Open a specific item on Wowhead.
 * @param itemID The Wowhead item ID to view.
 */
void wowHead_viewItem(int itemID);

} // namespace external_links
