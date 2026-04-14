/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

namespace updater {

/**
 * Check if there are any available updates.
 * Returns true if an update is available, false otherwise.
 * JS equivalent: checkForUpdates()
 */
bool checkForUpdates();

/**
 * Apply an outstanding update.
 * Downloads required files and launches the external updater process.
 * JS equivalent: applyUpdate()
 */
void applyUpdate();

} // namespace updater
