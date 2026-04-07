/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "texture-ribbon.h"
#include "../core.h"
#include "../casc/listfile.h"

#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace texture_ribbon {

static int _syncID = 0;

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

} // namespace texture_ribbon