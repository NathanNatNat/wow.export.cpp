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
 * Mirrors JS constructor (file-writer.js lines 14–24): on EISDIR (the path
 * is a directory), removes it and retries. Any other failure rethrows.
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
 * JS writeLine (file-writer.js lines 34–43) only suspends the caller when
 * Node stream signals backpressure (write() returns false). std::ofstream
 * has no backpressure concept — writes are synchronous under a mutex, so
 * the caller never suspends.
 */
void FileWriter::writeLine(std::string_view line) {
	std::lock_guard lock(*write_mutex);

	if (closed || !stream.is_open())
		throw std::runtime_error("write after end");

	stream << line << '\n';
	if (!stream)
		throw std::runtime_error("failed to write line");
}

void FileWriter::close() {
	// Guard against calling close() on a moved-from instance.
	if (!write_mutex)
		return;

	std::lock_guard lock(*write_mutex);

	closed = true;

	if (stream.is_open())
		stream.close();
}
