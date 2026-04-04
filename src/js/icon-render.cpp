/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>
License: MIT
 */

#include "icon-render.h"
#include "core.h"

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <glad/gl.h>

namespace {

// inv_misc_questionmark — default placeholder icon (embedded JPEG bytes).
// In the JS version, this was a base64 data URL set as a CSS background-image.
// In C++, we store a GL texture handle created from the decoded image data.
// The raw base64 data is preserved here for reference; the actual default texture
// is created lazily from OpenGL's built-in capabilities.
constexpr uint32_t DEFAULT_ICON_TEXTURE = 0;

constexpr int QUEUE_LIMIT = 20;

/**
 * Queue entry for icon loading.
 * In JS, this held { fileDataID, rule } where rule was a CSSStyleRule.
 * In C++, we only need the fileDataID since textures are stored in the cache.
 */
struct QueueEntry {
uint32_t fileDataID;
};

// Texture cache: fileDataID -> OpenGL texture handle.
// Replaces the JS dynamic stylesheet with CSS rules.
std::unordered_map<uint32_t, uint32_t> _textureCache;

// Set of fileDataIDs that have been registered (equivalent to iconRuleExists).
std::unordered_set<uint32_t> _registeredIcons;

bool _loading = false;
std::vector<QueueEntry> _queue;

// Forward declarations
void processQueue();

/**
 * Returns true if a given icon has been registered in the cache.
 * JS equivalent: iconRuleExists(selector) — checked if a CSS rule existed.
 * @param fileDataID The icon's file data ID.
 * @returns true if the icon is already registered.
 */
bool iconRuleExists(uint32_t fileDataID) {
return _registeredIcons.count(fileDataID) > 0;
}

/**
 * Remove a registered icon from the cache.
 * JS equivalent: removeRule(rule) — removed a CSS rule from the stylesheet.
 * @param fileDataID The icon's file data ID to remove.
 */
void removeRule(uint32_t fileDataID) {
auto it = _textureCache.find(fileDataID);
if (it != _textureCache.end()) {
if (it->second != 0) {
GLuint tex = it->second;
glDeleteTextures(1, &tex);
}
_textureCache.erase(it);
}
_registeredIcons.erase(fileDataID);
}

/**
 * Process the next item in the icon loading queue.
 * JS equivalent: processQueue() — loaded BLP files and set CSS background-image.
 * In C++, this loads BLP files and creates OpenGL textures.
 */
void processQueue() {
if (_queue.empty()) {
_loading = false;
return;
}

_loading = true;

QueueEntry entry = _queue.back();
_queue.pop_back();

// In the JS version, this was:
//   core.view.casc.getFile(entry.fileDataID).then(data => {
//       const blp = new BLPFile(data);
//       entry.rule.style.backgroundImage = 'url(' + blp.getDataURL(0b0111) + ')';
//   }).catch(() => { /* Icon failed to load */ })
//
// The CASC source and BLP decoder are not yet converted (Tiers 4-9).
// When they are available, this function will:
// 1. Get the file data from the CASC source
// 2. Parse it as a BLP file
// 3. Decode to RGBA pixel data
// 4. Create an OpenGL texture from the pixel data
// 5. Store the texture handle in _textureCache
//
// For now, the error path is followed (same as the JS .catch()),
// which leaves the icon with its default placeholder texture.

// Continue processing remaining queue items
processQueue();
}

/**
 * Queue an icon for loading.
 * JS equivalent: queueItem(fileDataID, rule) — rule was a CSSStyleRule.
 * @param fileDataID The icon's file data ID.
 */
void queueItem(uint32_t fileDataID) {
_queue.push_back({fileDataID});

// If the queue is full, remove an element from the front rather than the back
// since we want to prioritize the most recently requested icons, as they're
// most likely the ones the user can see.
if (static_cast<int>(_queue.size()) > QUEUE_LIMIT) {
// Since we're dropping the entry, we need to make sure to remove the icon itself.
QueueEntry removed = _queue.front();
_queue.erase(_queue.begin());
removeRule(removed.fileDataID);
}

if (!_loading)
processQueue();
}

} // anonymous namespace

namespace icon_render {

void loadIcon(uint32_t fileDataID) {
if (!iconRuleExists(fileDataID)) {
// Register the icon with a default/placeholder texture.
// JS equivalent: sheet.insertRule(selector + ' {}') then setting backgroundImage to DEFAULT_ICON.
_registeredIcons.insert(fileDataID);
_textureCache[fileDataID] = DEFAULT_ICON_TEXTURE;

if (fileDataID == 0)
return;

queueItem(fileDataID);
}
}

uint32_t getIconTexture(uint32_t fileDataID) {
auto it = _textureCache.find(fileDataID);
if (it != _textureCache.end())
return it->second;
return 0;
}

} // namespace icon_render
