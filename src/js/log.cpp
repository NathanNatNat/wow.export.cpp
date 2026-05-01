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
#include <cerrno>
#include <cstring>

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
bool streamInitialized = false;
std::deque<std::string> pool;
std::ofstream stream;
std::mutex logMutex;

void ensureStreamOpen() {
	if (!streamInitialized) {
		stream.open(constants::RUNTIME_LOG().string());
		streamInitialized = true;
	}
}

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
	char buf[9];
	std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
	return std::string(buf);
}

bool drainPending = false;

/**
 * Invoked when the stream has finished flushing.
 */
void drainPool() {
	isClogged = false;
	drainPending = false;

	// If the pool is empty, don't slip into a loop.
	if (pool.empty())
		return;

	int ticks = 0;
	while (!isClogged && ticks < MAX_DRAIN_PER_TICK && !pool.empty()) {
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

	// Only schedule another drain if we're not blocked and we have
	// something remaining in the pool.
	if (!isClogged && !pool.empty())
		drainPending = true;
}

} // anonymous namespace

namespace logging {

void init() {
	std::lock_guard<std::mutex> lock(logMutex);
	// Initialize the logging stream.
	if (!streamInitialized) {
		stream.open(constants::RUNTIME_LOG().string());
		streamInitialized = true;
	}
}

/**
 * Write a message to the log.
 */
void write(std::string_view message) {
	std::lock_guard<std::mutex> lock(logMutex);

	ensureStreamOpen();

	std::string line = "[" + getTimestamp() + "] " + std::string(message) + "\n";

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

#ifndef BUILD_RELEASE
	// Mirror output to debugger.
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
	std::string cmd = "xdg-open '" + constants::RUNTIME_LOG().string() + "' &";
	std::system(cmd.c_str());
#endif
}

void flush() {
	std::lock_guard<std::mutex> lock(logMutex);

	ensureStreamOpen();
	stream.clear();
	isClogged = false;
	drainPending = false;

	while (!pool.empty()) {
		stream << pool.front();
		pool.pop_front();
	}
	stream.flush();
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
			return std::string("Unable to obtain runtime log: ") + std::strerror(errno);

		return std::string(
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>());
	} catch (const std::exception& e) {
		return std::string("Unable to obtain runtime log: ") + e.what();
	}
}