/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <variant>
#include <filesystem>

#include "../../db/FieldType.h"

// Forward declaration for SchemaField
namespace db {
	using SchemaField = std::variant<db::FieldType, std::pair<db::FieldType, int>>;
}

class SQLWriter {
public:
	/**
	 * Construct a new SQLWriter instance.
	 * @param out Output path to write to.
	 * @param table_name SQL table name.
	 */
	SQLWriter(const std::filesystem::path& out, const std::string& table_name);

	/**
	 * Set the schema for DDL generation.
	 * @param schema WDCReader schema map.
	 */
	void setSchema(const std::map<std::string, db::SchemaField>& schema);

	/**
	 * Enable or disable DDL generation.
	 * @param include Whether to include DDL.
	 */
	void setIncludeDDL(bool include);

	/**
	 * Add a field to this SQL.
	 * @param field Field name.
	 */
	void addField(const std::string& field);

	/**
	 * Add multiple fields to this SQL.
	 * @param fields Field names.
	 */
	void addField(const std::vector<std::string>& fields);

	/**
	 * Add a row to this SQL.
	 * @param row Map of field name to value.
	 */
	void addRow(const std::unordered_map<std::string, std::string>& row);

	/**
	 * Escape a SQL value for safe insertion.
	 * @param value The value to escape.
	 * @returns The escaped value.
	 */
	std::string escapeSQLValue(const std::string& value) const;

	/**
	 * Escape a SQL identifier (table/column name).
	 * @param name The identifier to escape.
	 * @returns The escaped identifier.
	 */
	std::string escapeIdentifier(const std::string& name) const;

	/**
	 * Convert a FieldType to a SQL type string.
	 * @param field_type SchemaField variant.
	 * @param field_name Field name for context.
	 * @returns SQL type string.
	 */
	std::string fieldTypeToSQL(const db::SchemaField& field_type, const std::string& field_name) const;

	/**
	 * Generate DROP TABLE and CREATE TABLE DDL.
	 * @returns DDL statements.
	 */
	std::string generateDDL() const;

	/**
	 * Convert rows to batched SQL INSERT statements.
	 * @returns SQL INSERT statements.
	 */
	std::string toSQL() const;

	/**
	 * Write the SQL to disk.
	 * @param overwrite Whether to overwrite existing files.
	 */
	void write(bool overwrite = true);

private:
	static constexpr size_t BATCH_SIZE = 100;

	std::filesystem::path out;
	std::string table_name;
	std::vector<std::string> fields;
	std::vector<std::unordered_map<std::string, std::string>> rows;
	const std::map<std::string, db::SchemaField>* schema;
	bool include_ddl;
};
