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

class FileWriter {
public:
	/**
	 * Construct a new FileWriter instance.
	 * @param file Path to the file to write.
	 * @param encoding Encoding hint (unused in C++ — streams write raw bytes).
	 */
	FileWriter(const std::filesystem::path& file, std::string_view encoding = "utf8");

	/**
	 * Write a line to the file.
	 * @param line The line to write (newline appended automatically).
	 */
	void writeLine(std::string_view line);

	/**
	 * Close the file stream.
	 */
	void close();

private:
	void _drain();

	std::ofstream stream;
	bool blocked;
};
