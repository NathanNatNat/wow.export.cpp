/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "log.h"
#include "constants.h"

#include <fstream>
#include <deque>
#include <format>
#include <string>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <mutex>
#include <iterator>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#endif

namespace {

constexpr int MAX_LOG_POOL = 10000; // 1-2MB~
constexpr int MAX_DRAIN_PER_TICK = 50;

std::chrono::steady_clock::time_point markTimer{};
bool isClogged = false;
std::deque<std::string> pool;
std::ofstream stream;
std::mutex logMutex;

/**
 * Return a HH:MM:SS formatted timestamp.
 */
std::string getTimestamp() {
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &time);
#else
	localtime_r(&time, &tm);
#endif
	char buf[9]; // "HH:MM:SS\0"
	std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
	return std::string(buf);
}

/**
 * Invoked when the stream has finished flushing.
 *
 * In JS, this is triggered by the stream 'drain' event when the
 * internal buffer has been flushed. In C++, file I/O is synchronous,
 * so this is called at the start of write() when the stream was
 * previously in a failed state and has recovered.
 */
/**
 * Flag indicating that a deferred drain has been requested.
 * When true, the next call to write() will trigger drainPool()
 * even if isClogged is false, to drain remaining pooled items.
 */
bool drainPending = false;

void drainPool() {
	isClogged = false;
	drainPending = false;

	// If the pool is empty, don't slip into a loop.
	if (pool.empty())
		return;

	int ticks = 0;
	while (!isClogged && ticks < MAX_DRAIN_PER_TICK && !pool.empty()) {
		// JS: pool.shift() always removes the item before writing.
		// Node.js stream.write() returning false means backpressure but
		// the data IS buffered and will be written. Match that by always
		// removing from pool.
		std::string item = std::move(pool.front());
		pool.pop_front();

		stream << item;
		stream.flush();
		if (stream.fail()) {
			stream.clear();
			isClogged = true;
		}
		ticks++;
	}

	// JS equivalent: process.nextTick(drainPool) — schedule another drain
	// if we're not blocked and there are remaining items in the pool.
	// In C++, we set a flag so the next write() call triggers drainPool().
	if (!isClogged && !pool.empty())
		drainPending = true;
}

} // anonymous namespace

namespace logging {

void init() {
	// Initialize the logging stream.
	stream.open(constants::RUNTIME_LOG().string());
}

/**
 * Write a message to the log.
 *
 * JS equivalent: write(...parameters) uses util.format(...parameters).
 * In C++, callers should use std::format() to build the message before
 * passing it. This accepts a pre-formatted string for API simplicity,
 * matching the JS behavior of producing a single formatted line.
 */
void write(std::string_view message) {
	std::lock_guard<std::mutex> lock(logMutex);
	std::string line = "[" + getTimestamp() + "] " + std::string(message) + "\n";

	// Try to drain pooled entries if the stream was previously clogged,
	// or if a deferred drain was scheduled (JS equivalent of process.nextTick(drainPool)).
	if (isClogged || drainPending) {
		stream.clear();
		drainPool();
	}

	if (!isClogged) {
		stream << line;
		stream.flush();
		if (stream.fail()) {
			stream.clear();
			isClogged = true;
		}
	}

	if (isClogged) {
		// Stream is blocked, pool instead.
		if (pool.size() < static_cast<std::size_t>(MAX_LOG_POOL)) {
			pool.push_back(line);
		} else if (pool.size() == static_cast<std::size_t>(MAX_LOG_POOL)) {
			pool.push_back("[" + getTimestamp() + "] WARNING: Log pool overflow - some log entries have been truncated.\n");
		}
	}

	// Mirror output to debugger.
#ifndef NDEBUG
	std::fputs(line.c_str(), stdout);
#endif
}

/**
 * Internally mark the current timestamp for measuring
 * performance times with logging::timeEnd().
 */
void timeLog() {
	markTimer = std::chrono::steady_clock::now();
}

/**
 * Logs the time (in milliseconds) between the last logging::timeLog()
 * call and this call, with the given label prefixed.
 *
 * JS equivalent: timeEnd(label, ...params) => write(label + ' (%dms)', ...params, elapsed)
 * The JS version supports variadic format parameters; in C++, the label
 * should already be formatted by the caller using std::format(). The
 * elapsed time is appended automatically.
 *
 * @param label Label to prefix the time output.
 */
void timeEnd(std::string_view label) {
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - markTimer).count();
	write(std::format("{} ({}ms)", label, elapsed));
}

/**
 * Open the runtime log in the user's external editor.
 */
void openRuntimeLog() {
#ifdef _WIN32
	ShellExecuteW(nullptr, L"open",
		constants::RUNTIME_LOG().wstring().c_str(),
		nullptr, nullptr, SW_SHOWNORMAL);
#else
	// xdg-open is the standard Linux way to open files with the default application.
	// Single-quote the path to prevent shell interpretation of special characters.
	std::string cmd = "xdg-open '" + constants::RUNTIME_LOG().string() + "' &";
	std::system(cmd.c_str());
#endif
}

} // namespace logging

/**
 * Attempts to return the contents of the runtime log.
 * This is defined as a global as it is requested during
 * an application crash where modules may not be loaded.
 */
std::string getErrorDump() {
	try {
		std::ifstream file(constants::RUNTIME_LOG());
		if (!file)
			return "Unable to obtain runtime log: file could not be opened";

		return std::string(
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>());
	} catch (const std::exception& e) {
		return std::string("Unable to obtain runtime log: ") + e.what();
	}
}