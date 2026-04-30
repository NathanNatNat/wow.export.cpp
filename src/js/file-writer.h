/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include <filesystem>
#include <mutex>
#include <memory>

class FileWriter {
public:
	~FileWriter();
	FileWriter(const FileWriter&) = delete;
	FileWriter& operator=(const FileWriter&) = delete;
	FileWriter(FileWriter&&) noexcept = default;
	FileWriter& operator=(FileWriter&&) noexcept = default;

	/**
	 * Construct a new FileWriter instance.
	 * @param file Path to the file to write.
	 * @param encoding Encoding hint (unused in C++ — streams write raw bytes).
	 *
	 * If the path is a directory (EISDIR in JS), removes it and retries.
	 * Throws std::runtime_error if the stream cannot be opened.
	 */
	FileWriter(const std::filesystem::path& file, std::string_view encoding = "utf8");

	/**
	 * Check if the underlying stream is open and ready for writing.
	 * Used for null-safety matching JS optional chaining (exportPaths?.writeLine).
	 */
	bool isOpen() const { return stream.is_open(); }

	/**
	 * Write a line to the file.
	 * @param line The line to write (newline appended automatically).
	 */
	void writeLine(std::string_view line);

	/**
	 * Close the file stream.
	 * Safe to call when !isOpen() — the call is silently ignored (matches JS ?. behavior).
	 */
	void close();

private:
	std::ofstream stream;
	bool closed = false;
	// std::mutex is not movable; wrap in unique_ptr so FileWriter remains movable
	// (needed when returning FileWriter by value from openLastExportStream()).
	std::unique_ptr<std::mutex> write_mutex;
};
