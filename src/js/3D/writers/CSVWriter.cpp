/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "CSVWriter.h"
#include "../../generics.h"
#include "../../file-writer.h"

#include <algorithm>

CSVWriter::CSVWriter(const std::filesystem::path& out)
	: out(out) {}

void CSVWriter::addField(const std::string& field) {
	fields.push_back(field);
}

void CSVWriter::addField(const std::vector<std::string>& fields) {
	this->fields.insert(this->fields.end(), fields.begin(), fields.end());
}

void CSVWriter::addRow(const std::unordered_map<std::string, std::string>& row) {
	rows.push_back(row);
}

std::string CSVWriter::escapeCSVField(const std::string& value) const {
	if (value.empty())
		return "";

	if (value.find(';') != std::string::npos ||
		value.find('"') != std::string::npos ||
		value.find('\n') != std::string::npos) {
		// Escape double quotes by doubling them
		std::string escaped;
		escaped.reserve(value.size() + 2);
		escaped += '"';
		for (char c : value) {
			if (c == '"')
				escaped += "\"\"";
			else
				escaped += c;
		}
		escaped += '"';
		return escaped;
	}

	return value;
}

void CSVWriter::write(bool overwrite) {
	// Don't bother writing an empty CSV file.
	if (rows.empty())
		return;

	// If overwriting is disabled, check file existence.
	if (!overwrite && generics::fileExists(out))
		return;

	generics::createDirectory(out.parent_path());
	FileWriter writer(out);

	// Write header.
	std::string header;
	for (size_t i = 0; i < fields.size(); i++) {
		if (i > 0)
			header += ';';
		header += escapeCSVField(fields[i]);
	}
	writer.writeLine(header);

	// Write rows.
	const size_t nFields = fields.size();
	for (const auto& row : rows) {
		std::string rowOut;
		for (size_t i = 0; i < nFields; i++) {
			if (i > 0)
				rowOut += ';';
			auto it = row.find(fields[i]);
			rowOut += escapeCSVField(it != row.end() ? it->second : "");
		}
		writer.writeLine(rowOut);
	}

	writer.close();
}