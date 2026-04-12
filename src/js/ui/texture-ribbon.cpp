/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "texture-ribbon.h"
#include "../core.h"
#include "../casc/listfile.h"
#include "../buffer.h"

#include <filesystem>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <stb_image.h>

namespace fs = std::filesystem;

namespace texture_ribbon {

static int _syncID = 0;

static std::unordered_map<int, GLuint> s_slotTextures;
// detect when a slot's src has changed and re-upload.
static std::unordered_map<int, std::string> s_slotSrcCache;

/**
 * Invoked when the texture ribbon element resizes.
 */
void onResize(int width) {
	// Take the total available space of the texture ribbon element and reduce
	// it by the width of the next/previous buttons (30 each).
	width -= 60;

	// Divide the available space by the true size of the slot elements.
	// Slot = 64 width, 1 + 1 border, 5 + 5 margin.
	core::view->textureRibbonSlotCount = width / 76;
}

/**
 * Reset the texture ribbon.
 */
int reset() {
	clearSlotTextures();
	core::view->textureRibbonStack.clear();
	core::view->textureRibbonPage = 0;
	core::view->contextMenus.nodeTextureRibbon = nullptr;

	return ++_syncID;
}

/**
 * Set the file displayed in a given ribbon slot.
 * @param slotIndex
 * @param fileDataID
 * @param syncID
 */
void setSlotFile(int slotIndex, uint32_t fileDataID, int syncID) {
	// Only accept data from the latest preparation.
	if (syncID != _syncID)
		return;

	auto& stack = core::view->textureRibbonStack;
	if (slotIndex < 0 || slotIndex >= static_cast<int>(stack.size()))
		return;

	auto& slot = stack[slotIndex];
	slot["fileDataID"] = fileDataID;

	std::string fileName = casc::listfile::getByID(fileDataID);
	if (fileName.empty())
		fileName = std::to_string(fileDataID);

	slot["fileName"] = fileName;
	slot["displayName"] = fs::path(fileName).stem().string();
}

/**
 * Set the file displayed in a given ribbon slot (legacy - uses file path instead of fileDataID).
 * @param slotIndex
 * @param filePath
 * @param syncID
 */
void setSlotFileLegacy(int slotIndex, const std::string& filePath, int syncID) {
	if (syncID != _syncID)
		return;

	auto& stack = core::view->textureRibbonStack;
	if (slotIndex < 0 || slotIndex >= static_cast<int>(stack.size()))
		return;

	auto& slot = stack[slotIndex];
	slot["fileDataID"] = 0;
	slot["fileName"] = filePath;
	slot["displayName"] = fs::path(filePath).stem().string();
}

/**
 * Set the render source for a given ribbon slot.
 * @param slotIndex 
 * @param src 
 * @param syncID
 */
void setSlotSrc(int slotIndex, const std::string& src, int syncID) {
	// Only accept data from the latest preparation.
	if (syncID != _syncID)
		return;

	auto& stack = core::view->textureRibbonStack;
	if (slotIndex < 0 || slotIndex >= static_cast<int>(stack.size()))
		return;

	stack[slotIndex]["src"] = src;
}

/**
 * Add an empty slot to the texture ribbon.
 * @returns slotIndex
 */
int addSlot() {
	auto& stack = core::view->textureRibbonStack;
	int slotIndex = static_cast<int>(stack.size());

	nlohmann::json slot;
	slot["fileDataID"] = 0;
	slot["displayName"] = "Empty";
	slot["fileName"] = "";
	slot["src"] = "";
	stack.push_back(std::move(slot));

	return slotIndex;
}

/**
 * Delete all cached slot textures.
 */
void clearSlotTextures() {
	for (auto& [idx, tex] : s_slotTextures) {
		if (tex != 0)
			glDeleteTextures(1, &tex);
	}
	s_slotTextures.clear();
	s_slotSrcCache.clear();
}

/**
 * Get or create an OpenGL texture for the given ribbon slot.
 */
GLuint getSlotTexture(int slotIndex) {
	if (!core::view)
		return 0;

	auto& stack = core::view->textureRibbonStack;
	if (slotIndex < 0 || slotIndex >= static_cast<int>(stack.size()))
		return 0;

	const auto& slot = stack[slotIndex];
	std::string src = slot.value("src", std::string(""));
	if (src.empty())
		return 0;

	// Check if we already have a cached texture for this slot with the same src.
	auto texIt = s_slotTextures.find(slotIndex);
	auto srcIt = s_slotSrcCache.find(slotIndex);
	if (texIt != s_slotTextures.end() && srcIt != s_slotSrcCache.end() && srcIt->second == src)
		return texIt->second;

	// src changed or new slot — delete old texture if any.
	if (texIt != s_slotTextures.end() && texIt->second != 0)
		glDeleteTextures(1, &texIt->second);

	// Strip the data-URL header to get the base64 payload.
	// Format: "data:<mime>;base64,<payload>"
	std::string_view sv(src);
	auto commaPos = sv.find(',');
	if (commaPos == std::string_view::npos) {
		s_slotTextures[slotIndex] = 0;
		s_slotSrcCache[slotIndex] = src;
		return 0;
	}
	std::string_view b64 = sv.substr(commaPos + 1);

	// Decode base64 → PNG bytes.
	BufferWrapper pngBuf = BufferWrapper::fromBase64(b64);
	if (pngBuf.byteLength() == 0) {
		s_slotTextures[slotIndex] = 0;
		s_slotSrcCache[slotIndex] = src;
		return 0;
	}

	// Decode PNG → RGBA pixels via stb_image.
	int w = 0, h = 0, channels = 0;
	unsigned char* pixels = stbi_load_from_memory(
		pngBuf.raw().data(), static_cast<int>(pngBuf.byteLength()),
		&w, &h, &channels, 4);
	if (!pixels) {
		s_slotTextures[slotIndex] = 0;
		s_slotSrcCache[slotIndex] = src;
		return 0;
	}

	// Upload to GL texture.
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(pixels);

	s_slotTextures[slotIndex] = tex;
	s_slotSrcCache[slotIndex] = src;
	return tex;
}

} // namespace texture_ribbon