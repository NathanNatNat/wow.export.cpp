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
 */
void FileWriter::writeLine(std::string_view line) {
	// In JS, writeLine awaits if the stream is blocked (backpressure).
	// complete, so explicit backpressure handling is unnecessary.
	// If the stream was previously in a failed state, attempt recovery.
	if (blocked)
		_drain();

	stream << line << '\n';
	if (stream.fail()) {
		blocked = true;
		stream.clear();
	}
}

void FileWriter::_drain() {
	blocked = false;
}

void FileWriter::close() {
	stream.close();
}