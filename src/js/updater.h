/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Application auto-updater.
 *
 * JS equivalent: updater.js — module.exports = { checkForUpdates, applyUpdate }
 */
namespace updater {

/**
 * Check if there are any available updates.
 * @returns true if an update is available, false otherwise.
 */
bool checkForUpdates();

/**
 * Apply an outstanding update.
 * Downloads required files and launches the external updater process.
 */
void applyUpdate();

} // namespace updater
