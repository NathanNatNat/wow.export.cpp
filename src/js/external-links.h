/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <unordered_map>
#include <imgui.h>

/**
 * ExternalLinks — centralized external link resolution and opening.
 *
 * JS equivalent: src/js/external-links.js (ExternalLinks class).
 *
 * Defines static links referenced via data-external HTML attributes in the
 * original JS, and provides open() / wowHead_viewItem() static methods.
 */
namespace ExternalLinks {

/**
 * Static links which can be referenced via the ::NAME pattern.
 * JS equivalent: const STATIC_LINKS = { ... }
 */
extern const std::unordered_map<std::string, std::string> STATIC_LINKS;

/**
 * URL pattern for locating a specific item on Wowhead.
 * JS equivalent: const WOWHEAD_ITEM = 'https://www.wowhead.com/item=%d'
 */
inline constexpr const char* WOWHEAD_ITEM = "https://www.wowhead.com/item={}";

/**
 * Resolve a link identifier to its URL.
 * If the link starts with "::", it is looked up in STATIC_LINKS.
 * Otherwise, it is returned as-is.
 * @param link Link identifier or URL.
 * @return Resolved URL.
 */
std::string resolve(const std::string& link);

/**
 * Open an external link on the system.
 * If the link starts with "::", it is resolved via STATIC_LINKS first.
 * JS equivalent: ExternalLinks.open(link)
 * @param link Link identifier (e.g., "::DISCORD") or direct URL.
 */
void open(const std::string& link);

/**
 * Open a specific item on Wowhead.
 * JS equivalent: ExternalLinks.wowHead_viewItem(itemID)
 * @param itemID Item ID to view on Wowhead.
 */
void wowHead_viewItem(int itemID);

/**
 * Render an ImGui clickable text link that opens an external URL when clicked.
 *
 * This provides the C++ equivalent of the JS global `data-external` click
 * handler (app.js lines 115–131). In the JS, any element with a
 * `data-external` attribute automatically opens the URL on click. In Dear
 * ImGui there is no DOM, so each link must call this function explicitly.
 *
 * The link text is rendered with the specified color (defaults to current text
 * color) and shows a hand cursor on hover.
 *
 * @param link   Link identifier (e.g. "::DISCORD") or direct URL.
 * @param label  Visible text for the link.
 * @param color  Optional text color. Pass nullptr to use default ImGui text color.
 */
void renderLink(const char* link, const char* label, const ImVec4* color = nullptr);

} // namespace ExternalLinks
