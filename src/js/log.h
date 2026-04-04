/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <string_view>

namespace logging {

/// Initialize the logging stream. Must be called once at startup
/// after constants::init(), which provides constants::RUNTIME_LOG().
void init();

/**
 * Write a message to the log.
 */
void write(std::string_view message);

/**
 * Internally mark the current timestamp for measuring
 * performance times with logging::timeEnd().
 */
void timeLog();

/**
 * Logs the time (in milliseconds) between the last logging::timeLog()
 * call and this call, with the given label prefixed.
 * @param label Label to prefix the time output.
 */
void timeEnd(std::string_view label);

/**
 * Open the runtime log in the user's external editor.
 */
void openRuntimeLog();

} // namespace logging

/**
 * Attempts to return the contents of the runtime log.
 * This is defined as a global as it is requested during
 * an application crash where modules may not be loaded.
 */
std::string getErrorDump();
