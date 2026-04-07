/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

#include "../3D/writers/SQLWriter.h"

class FileWriter;

namespace casc {
class ExportHelper;
class CASC;
}

namespace mpq {
class MPQInstall;
}

/**
 * Data export utilities — CSV, SQL, raw DB2/DBC export helpers.
 *
 * JS equivalent: module.exports = { exportDataTable, exportRawDB2,
 *     exportDataTableSQL, exportRawDBC }
 */
namespace data_exporter {

/**
 * Export a data table as a CSV file.
 * If helper and export_paths are nullptr, creates its own helper and export stream (standalone mode).
 * @param headers Column header names.
 * @param rows    Row data as strings (empty string = empty cell).
 * @param tableName Base name for the exported file.
 * @param helper  Export helper (optional; if nullptr, standalone mode).
 * @param export_paths FileWriter stream (optional; if nullptr, standalone mode).
 */
void exportDataTable(
	const std::vector<std::string>& headers,
	const std::vector<std::vector<std::string>>& rows,
	const std::string& tableName,
	casc::ExportHelper* helper = nullptr,
	FileWriter* export_paths = nullptr
);

/**
 * Export a raw DB2 file from CASC to disk.
 * If helper and export_paths are nullptr, creates its own helper and export stream (standalone mode).
 * @param tableName   Base name for the exported file.
 * @param fileDataID  File data ID to retrieve from CASC.
 * @param casc        CASC source for file retrieval.
 * @param helper      Export helper (optional; if nullptr, standalone mode).
 * @param export_paths FileWriter stream (optional; if nullptr, standalone mode).
 */
void exportRawDB2(
	const std::string& tableName,
	uint32_t fileDataID,
	casc::CASC* casc,
	casc::ExportHelper* helper = nullptr,
	FileWriter* export_paths = nullptr
);

/**
 * Export a data table as a SQL file.
 * If helper and export_paths are nullptr, creates its own helper and export stream (standalone mode).
 * @param headers     Column header names.
 * @param rows        Row data as strings (empty string = NULL in SQL).
 * @param tableName   Base name / SQL table name.
 * @param schema      SQL schema map (may be nullptr for no schema).
 * @param createTable If true, include CREATE TABLE DDL.
 * @param helper      Export helper (optional; if nullptr, standalone mode).
 * @param export_paths FileWriter stream (optional; if nullptr, standalone mode).
 */
void exportDataTableSQL(
	const std::vector<std::string>& headers,
	const std::vector<std::vector<std::string>>& rows,
	const std::string& tableName,
	const std::map<std::string, db::SchemaField>* schema,
	bool createTable,
	casc::ExportHelper* helper = nullptr,
	FileWriter* export_paths = nullptr
);

/**
 * Export a raw DBC file from MPQ to disk.
 * Always standalone — creates its own helper and export stream.
 * @param tableName Base name for the exported file (used as output filename).
 * @param filePath  Path of the file within the MPQ archive.
 * @param mpq       MPQ install to retrieve the file from.
 */
void exportRawDBC(
	const std::string& tableName,
	const std::string& filePath,
	mpq::MPQInstall* mpq
);

} // namespace data_exporter
