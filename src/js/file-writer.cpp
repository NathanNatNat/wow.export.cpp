/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "file-writer.h"

/**
 * Construct a new FileWriter instance.
 * @param file Path to the file to write.
 * @param encoding Encoding hint (unused in C++ — streams write raw bytes).
 */
FileWriter::FileWriter(const std::filesystem::path& file, std::string_view /*encoding*/)
	: stream(file, std::ios::out | std::ios::trunc),
	  blocked(false) {}

/**
 * Write a line to the file.
 * @param line The line to write (newline appended automatically).
 *
 * JS uses Node.js stream backpressure: stream.write() returns false when
 * the internal buffer is full, and the 'drain' event fires when it can
 * accept more data. std::ofstream is synchronous and does not need
 * backpressure — writes block until the OS buffer accepts the data.
 * The blocked/_drain mechanism is preserved for structural fidelity but
 * is effectively a no-op in the C++ implementation.
 */
void FileWriter::writeLine(std::string_view line) {
	// Guard: JS uses exportPaths?.writeLine(...) — silently ignore when stream isn't open.
	if (!stream.is_open())
		return;

	// In JS: if (this.blocked) await new Promise(resolve => this.resolver = resolve);
	// std::ofstream is synchronous, so no waiting is needed.

	stream << line << '\n';

	// In JS: if (!result) { this.blocked = true; stream.once('drain', () => this._drain()); }
	// std::ofstream does not have backpressure. If stream.fail() is true, it indicates
	// a real I/O error (disk full, permissions), not a recoverable backpressure condition.
	// We do not set blocked on I/O error as clearing error flags would mask real problems.
}

void FileWriter::_drain() {
	blocked = false;
}

void FileWriter::close() {
	// Guard: JS uses exportPaths?.close() — silently ignore when stream isn't open.
	if (!stream.is_open())
		return;
	stream.close();
}