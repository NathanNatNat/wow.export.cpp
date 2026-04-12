/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * Provides helpers for opening external links.
 * JS equivalent: module.exports = class ExternalLinks { ... }
 */
namespace ExternalLinks {

/**
 * Open an external link on the system.
 * Links prefixed with "::" are resolved from a static lookup table.
 * @param link URL or static link key (e.g. "::GITHUB").
 */
void open(const std::string& link);

/**
 * Open a specific item on Wowhead.
 * @param itemID Item ID to view.
 */
void wowHead_viewItem(int itemID);

} // namespace ExternalLinks
