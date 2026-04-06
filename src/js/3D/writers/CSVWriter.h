/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

class CSVWriter {
public:
	/**
	 * Construct a new CSVWriter instance.
	 * @param out Output path to write to.
	 */
	CSVWriter(const std::filesystem::path& out);

	/**
	 * Add a field to this CSV.
	 * @param field Field name.
	 */
	void addField(const std::string& field);

	/**
	 * Add multiple fields to this CSV.
	 * @param fields Field names.
	 */
	void addField(const std::vector<std::string>& fields);

	/**
	 * Add a row to this CSV.
	 * @param row Map of field name to value.
	 */
	void addRow(const std::unordered_map<std::string, std::string>& row);

	/**
	 * Escape a CSV field value if it contains special characters.
	 * @param value The value to escape.
	 * @returns The escaped value.
	 */
	std::string escapeCSVField(const std::string& value) const;

	/**
	 * Write the CSV to disk.
	 * @param overwrite Whether to overwrite existing files.
	 */
	void write(bool overwrite = true);

private:
	std::filesystem::path out;
	std::vector<std::string> fields;
	std::vector<std::unordered_map<std::string, std::string>> rows;
};
