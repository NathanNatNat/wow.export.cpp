/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "ItemSlot.h"

#include <array>
#include <string_view>

namespace wow {

namespace {

constexpr std::array<std::string_view, 29> ITEM_SLOTS = {
	"Non-equippable", // 0
	"Head",           // 1
	"Neck",           // 2
	"Shoulder",       // 3
	"Shirt",          // 4
	"Chest",          // 5
	"Waist",          // 6
	"Legs",           // 7
	"Feet",           // 8
	"Wrist",          // 9
	"Hands",          // 10
	"Finger",         // 11
	"Trinket",        // 12
	"One-hand",       // 13
	"Off-hand",       // 14
	"Ranged",         // 15
	"Back",           // 16
	"Two-hand",       // 17
	"Bag",            // 18
	"Tabard",         // 19
	"Chest",          // 20
	"Main-hand",      // 21
	"Off-hand",       // 22
	"Off-hand",       // 23
	"Ammo",           // 24
	"Thrown",         // 25
	"Ranged",         // 26
	"Quiver",         // 27
	"Relic"           // 28
};

} // anonymous namespace

/**
 * Get the label for an item slot based on the ID.
 * @param id 
 */
std::string_view getSlotName(int id) {
	if (id >= 0 && id < static_cast<int>(ITEM_SLOTS.size()))
		return ITEM_SLOTS[id];
	return "Unknown";
}

} // namespace wow
