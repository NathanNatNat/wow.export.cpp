/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "file-writer.h"
#include <stdexcept>

/**
 * Construct a new FileWriter instance.
 * @param file Path to the file to write.
 * @param encoding Encoding hint (unused in C++ — streams write raw bytes).
 */
FileWriter::FileWriter(const std::filesystem::path& file, std::string_view /*encoding*/)
	: stream(file, std::ios::out | std::ios::trunc),
	  blocked(false) {
	write_mutex = std::make_shared<std::mutex>();
	auto ready = std::make_shared<std::promise<void>>();
	ready->set_value();
	resolver = ready->get_future().share();
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
 * JS uses Node.js stream backpressure (write() + drain + await).
 * C++ mirrors this contract with an async write future chain.
 */
std::shared_future<void> FileWriter::writeLine(std::string_view line) {
	if (blocked && resolver.valid())
		resolver.wait();

	if (closed || !stream.is_open())
		throw std::runtime_error("write after end");

	blocked = true;

	std::string line_copy(line);
	std::shared_ptr<std::mutex> mutex = write_mutex;
	resolver = std::async(std::launch::async, [this, mutex, line_copy = std::move(line_copy)] {
		std::lock_guard lock(*mutex);

		if (closed || !stream.is_open())
			throw std::runtime_error("write after end");

		stream << line_copy << '\n';
		if (!stream)
			throw std::runtime_error("failed to write line");

		_drain();
	}).share();

	return resolver;
}

void FileWriter::_drain() {
	blocked = false;
}

void FileWriter::close() {
	if (blocked && resolver.valid())
		resolver.wait();

	closed = true;

	if (stream.is_open())
		stream.close();
}
