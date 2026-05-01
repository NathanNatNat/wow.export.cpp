/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once
#include <future>

namespace casc {
namespace realmlist {

void load();
std::future<void> loadAsync();

} // namespace realmlist
} // namespace casc
