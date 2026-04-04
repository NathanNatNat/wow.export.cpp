/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>

/**
 * GPU diagnostic information gathering and logging.
 *
 * JS equivalent: module.exports = { log_gpu_info }
 */
namespace gpu_info {

/**
 * Log GPU diagnostic information.
 * Queries OpenGL context info and platform-specific GPU details (VRAM, driver).
 * Errors are logged rather than thrown.
 */
void log_gpu_info();

} // namespace gpu_info
