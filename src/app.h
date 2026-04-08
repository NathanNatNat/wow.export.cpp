/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Application-level functions (app.cpp).
 *
 * JS equivalent: Functions defined in the root Vue app (app.js).
 */
namespace app {

/**
 * Restart the application by re-executing the current binary.
 * JS equivalent: restartApplication() which calls chrome.runtime.reload().
 */
void restartApplication();

} // namespace app
