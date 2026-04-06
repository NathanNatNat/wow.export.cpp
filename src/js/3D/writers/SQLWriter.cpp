/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "SQLWriter.h"
#include "../../generics.h"
#include "../../file-writer.h"

#include <algorithm>

SQLWriter::SQLWriter(const std::filesystem::path& out, const std::string& table_name)
	: out(out), table_name(table_name), schema(nullptr), include_ddl(false) {}

void SQLWriter::setSchema(const std::map<std::string, db::SchemaField>& schema) {
	this->schema = &schema;
}

void SQLWriter::setIncludeDDL(bool include) {
	include_ddl = include;
}

void SQLWriter::addField(const std::string& field) {
	fields.push_back(field);
}

void SQLWriter::addField(const std::vector<std::string>& fields) {
	this->fields.insert(this->fields.end(), fields.begin(), fields.end());
}

void SQLWriter::addRow(const std::unordered_map<std::string, std::string>& row) {
	rows.push_back(row);
}

std::string SQLWriter::escapeSQLValue(const std::string& value) const {
	if (value.empty())
		return "NULL";

	// numeric values don't need quotes
	// Check if the string is a valid number
	bool isNumeric = true;
	bool hasContent = false;
	for (size_t i = 0; i < value.size(); i++) {
		char c = value[i];
		if (c == ' ' || c == '\t') {
			if (hasContent) {
				// trailing whitespace after digits — check remaining
				for (size_t j = i; j < value.size(); j++) {
					if (value[j] != ' ' && value[j] != '\t') {
						isNumeric = false;
						break;
					}
				}
				break;
			}
			continue;
		}
		hasContent = true;
		if (c == '-' || c == '+') {
			if (i > 0 && (value[i-1] != ' ' && value[i-1] != '\t'))
				isNumeric = false;
		} else if (c == '.') {
			// allow decimal point
		} else if (c < '0' || c > '9') {
			isNumeric = false;
		}
	}

	if (isNumeric && hasContent)
		return value;

	// escape single quotes and wrap in quotes
	std::string escaped;
	escaped.reserve(value.size() + 2);
	escaped += '\'';
	for (char c : value) {
		if (c == '\'')
			escaped += "''";
		else
			escaped += c;
	}
	escaped += '\'';
	return escaped;
}

std::string SQLWriter::escapeIdentifier(const std::string& name) const {
	std::string escaped;
	escaped.reserve(name.size() + 2);
	escaped += '`';
	for (char c : name) {
		if (c == '`')
			escaped += "``";
		else
			escaped += c;
	}
	escaped += '`';
	return escaped;
}

std::string SQLWriter::fieldTypeToSQL(const db::SchemaField& field_type, const std::string& field_name) const {
	// Check if it's an array type [FieldType, arrayLength]
	if (std::holds_alternative<std::pair<db::FieldType, int>>(field_type)) {
		// arrays stored as TEXT (JSON or comma-separated)
		return "TEXT";
	}

	db::FieldType base_type = std::get<db::FieldType>(field_type);

	switch (base_type) {
		case db::FieldType::String:
			return "TEXT";

		case db::FieldType::Int8:
		case db::FieldType::UInt8:
			return "SMALLINT";

		case db::FieldType::Int16:
		case db::FieldType::UInt16:
			return "SMALLINT";

		case db::FieldType::Int32:
		case db::FieldType::UInt32:
		case db::FieldType::Relation:
		case db::FieldType::NonInlineID:
			return "INT";

		case db::FieldType::Int64:
		case db::FieldType::UInt64:
			return "BIGINT";

		case db::FieldType::Float:
			return "REAL";

		default:
			return "TEXT";
	}
}

std::string SQLWriter::generateDDL() const {
	if (!schema || fields.empty())
		return "";

	const std::string escaped_table = escapeIdentifier(table_name);
	std::string result;

	result += "DROP TABLE IF EXISTS " + escaped_table + ";\n";
	result += "\n";

	std::vector<std::string> column_defs;
	std::string primary_key;

	for (const auto& field : fields) {
		auto it = schema->find(field);
		if (it == schema->end())
			continue;

		const std::string sql_type = fieldTypeToSQL(it->second, field);
		const std::string escaped_field = escapeIdentifier(field);

		// ID field is typically the primary key
		std::string upper_field = field;
		std::transform(upper_field.begin(), upper_field.end(), upper_field.begin(), ::toupper);
		if (upper_field == "ID") {
			column_defs.push_back("\t" + escaped_field + " " + sql_type + " NOT NULL");
			primary_key = escaped_field;
		} else {
			column_defs.push_back("\t" + escaped_field + " " + sql_type);
		}
	}

	if (!primary_key.empty())
		column_defs.push_back("\tPRIMARY KEY (" + primary_key + ")");

	result += "CREATE TABLE " + escaped_table + " (\n";
	for (size_t i = 0; i < column_defs.size(); i++) {
		result += column_defs[i];
		if (i + 1 < column_defs.size())
			result += ",";
		result += "\n";
	}
	result += ");\n";
	result += "\n";

	return result;
}

std::string SQLWriter::toSQL() const {
	if (rows.empty())
		return "";

	std::string result;
	const std::string escaped_table = escapeIdentifier(table_name);

	std::string escaped_fields;
	for (size_t i = 0; i < fields.size(); i++) {
		if (i > 0)
			escaped_fields += ", ";
		escaped_fields += escapeIdentifier(fields[i]);
	}

	for (size_t i = 0; i < rows.size(); i += BATCH_SIZE) {
		const size_t batch_end = std::min(i + BATCH_SIZE, rows.size());

		result += "INSERT INTO " + escaped_table + " (" + escaped_fields + ") VALUES\n";

		for (size_t j = i; j < batch_end; j++) {
			const auto& row = rows[j];
			std::string values;
			for (size_t k = 0; k < fields.size(); k++) {
				if (k > 0)
					values += ", ";
				auto it = row.find(fields[k]);
				values += escapeSQLValue(it != row.end() ? it->second : "");
			}

			result += "(" + values + ")";
			if (j + 1 < batch_end)
				result += ",";
			result += "\n";
		}

		result += ";\n\n";
	}

	return result;
}

void SQLWriter::write(bool overwrite) {
	if (rows.empty())
		return;

	if (!overwrite && generics::fileExists(out))
		return;

	generics::createDirectory(out.parent_path());
	FileWriter writer(out);

	if (include_ddl && schema) {
		const std::string ddl = generateDDL();
		if (!ddl.empty())
			writer.writeLine(ddl);
	}

	const std::string sql = toSQL();
	writer.writeLine(sql);

	writer.close();
}
