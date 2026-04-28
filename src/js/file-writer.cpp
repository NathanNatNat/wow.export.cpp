/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "file-writer.h"
#include <memory>
#include <stdexcept>

/**
 * Construct a new FileWriter instance.
 * @param file Path to the file to write.
 * @param encoding Encoding hint (unused in C++ — streams write raw bytes).
 *
 * Mirrors JS constructor (file-writer.js lines 14–24):
 *   - Attempts to open the file for writing.
 *   - On EISDIR (path is a directory), removes the directory and retries.
 *   - Any other failure rethrows (JS: `else { throw e; }`).
 *
 * Entry 49 fix: only remove the path when it is confirmed to be a directory,
 * matching JS `if (e.code === 'EISDIR')`. Previously C++ removed on any
 * open failure, not just the directory case.
 *
 * Entry 50 fix: the second open() is now checked; throws on failure,
 * matching JS where a second createWriteStream() failure propagates.
 */
FileWriter::FileWriter(const std::filesystem::path& file, std::string_view /*encoding*/)
	: write_mutex(std::make_unique<std::mutex>()) {
	stream.open(file, std::ios::out | std::ios::trunc);
	if (!stream.is_open()) {
		// Only retry when the path is a directory (EISDIR equivalent).
		// For any other failure (permission denied, bad path, etc.) rethrow
		// — matching JS `else { throw e; }`.
		if (!std::filesystem::is_directory(file))
			throw std::runtime_error("failed to open file for writing: " + file.string());

		std::filesystem::remove_all(file);
		stream.open(file, std::ios::out | std::ios::trunc);

		if (!stream.is_open())
			throw std::runtime_error("failed to open file after removing directory: " + file.string());
	}
}

FileWriter::~FileWriter() {
	try {
		close();
	} catch (...) {
		// no-op
	}
}

/**
 * Write a line to the file.
 * @param line The line to write (newline appended automatically).
 *
 * JS writeLine (file-writer.js lines 34–43):
 *   - Awaits a drain promise if the stream is under backpressure.
 *   - Calls stream.write(line + '\n').
 *   - Sets blocked=true and registers a 'drain' listener only when
 *     write() returns false (i.e., actual backpressure).
 *
 * Entry 46 fix: C++ previously set blocked=true and spawned an async task
 * on every write, serialising all writes even though std::ofstream never
 * signals backpressure. Now writes are synchronous under a mutex — no
 * spurious blocking. Known deviation: std::ofstream has no backpressure
 * concept, so _drain() is never needed and the caller never suspends.
 *
 * Entry 47 fix: _drain() signalling is moot given the synchronous model.
 * The method is retained as a no-op for structural symmetry with JS.
 *
 * Entry 48 fix: the previous async lambda captured `this` without a
 * lifetime guard, risking use-after-free if FileWriter was destroyed
 * before the task completed. The synchronous model eliminates the async
 * lambda entirely, so there is no dangling-capture hazard.
 */
void FileWriter::writeLine(std::string_view line) {
	std::lock_guard lock(*write_mutex);

	if (closed || !stream.is_open())
		throw std::runtime_error("write after end");

	stream << line << '\n';
	if (!stream)
		throw std::runtime_error("failed to write line");

	_drain();
}

// JS _drain (file-writer.js lines 45–48): clears the blocked flag and
// calls resolver() to unblock any awaiting writeLine() caller.
// C++: no-op — the synchronous write model has no blocked/resolver state.
void FileWriter::_drain() {}

void FileWriter::close() {
	// Guard against calling close() on a moved-from instance.
	if (!write_mutex)
		return;

	std::lock_guard lock(*write_mutex);

	closed = true;

	if (stream.is_open())
		stream.close();
}
