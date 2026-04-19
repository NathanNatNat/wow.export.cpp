/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "data-exporter.h"

#include <format>
#include <optional>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "../core.h"
#include "../log.h"
#include "../generics.h"
#include "../file-writer.h"
#include "../casc/export-helper.h"
#include "../casc/casc-source.h"
#include "../mpq/mpq-install.h"
#include "../3D/writers/CSVWriter.h"
#include "../3D/writers/SQLWriter.h"

namespace data_exporter {

namespace fs = std::filesystem;

static std::optional<std::string> build_stack_trace(const char* function_name, const std::exception& e) {
	return std::format("{}: {}", function_name, e.what());
}

/**
 * Export data table to CSV format.
 * @param headers Column header names.
 * @param rows    Row data as strings.
 * @param tableName Base name for the exported file.
 * @param helper  Export helper (nullptr = standalone).
 * @param export_paths FileWriter stream (nullptr = standalone).
 */
void exportDataTable(
	const std::vector<std::string>& headers,
	const std::vector<std::vector<std::string>>& rows,
	const std::string& tableName,
	casc::ExportHelper* helper,
	FileWriter* export_paths)
{
	if (headers.empty() || rows.empty()) {
		if (!helper)
			core::setToast("info", "No data available to export.");
		return;
	}

	const bool standalone = (helper == nullptr);
	std::optional<casc::ExportHelper> own_helper;
	std::optional<FileWriter> own_export_paths;
	casc::ExportHelper* h;
	FileWriter* ep;

	if (standalone) {
		own_helper.emplace(1, "table");
		own_helper->start();
		own_export_paths.emplace(core::openLastExportStream());
		h = &own_helper.value();
		ep = &own_export_paths.value();
	} else {
		h = helper;
		ep = export_paths;
	}

	const std::string fileName = tableName + ".csv";

	try {
		const std::string exportPath = casc::ExportHelper::getExportPath(fileName);

		const bool overwriteFiles = core::view->config.value("overwriteFiles", false);
		if (!overwriteFiles && generics::fileExists(exportPath)) {
			logging::write(std::format("Skipping export of {} (file exists, overwrite disabled)", exportPath));
			h->mark(fileName, true);
		} else {
			CSVWriter csvWriter(exportPath);

			csvWriter.addField(headers);

			for (const auto& row : rows) {
				std::unordered_map<std::string, std::string> rowObject;
				for (size_t i = 0; i < headers.size(); i++) {
					rowObject[headers[i]] = (i < row.size()) ? row[i] : "";
				}
				csvWriter.addRow(rowObject);
			}

			csvWriter.write(overwriteFiles);
			if (ep) ep->writeLine("CSV:" + exportPath);

			h->mark(fileName, true);
			logging::write(std::format("Successfully exported data table to {}", exportPath));
		}
	} catch (const std::exception& e) {
		h->mark(fileName, false, e.what(), build_stack_trace("exportDataTable", e));
		logging::write(std::format("Failed to export data table: {}", e.what()));
	}

	if (standalone) {
		if (ep) ep->close();
		h->finish();
	}
}

/**
 * Export raw DB2 file directly from CASC.
 * @param tableName  Base name for the exported file.
 * @param fileDataID File data ID to retrieve from CASC.
 * @param casc       CASC source.
 * @param helper     Export helper (nullptr = standalone).
 * @param export_paths FileWriter stream (nullptr = standalone).
 */
void exportRawDB2(
	const std::string& tableName,
	uint32_t fileDataID,
	casc::CASC* casc,
	casc::ExportHelper* helper,
	FileWriter* export_paths)
{
	if (tableName.empty() || fileDataID == 0) {
		if (!helper)
			core::setToast("info", "No DB2 file information available to export.");
		return;
	}

	const bool standalone = (helper == nullptr);
	std::optional<casc::ExportHelper> own_helper;
	std::optional<FileWriter> own_export_paths;
	casc::ExportHelper* h;
	FileWriter* ep;

	if (standalone) {
		own_helper.emplace(1, "db2");
		own_helper->start();
		own_export_paths.emplace(core::openLastExportStream());
		h = &own_helper.value();
		ep = &own_export_paths.value();
	} else {
		h = helper;
		ep = export_paths;
	}

	const std::string fileName = tableName + ".db2";

	try {
		const std::string exportPath = casc::ExportHelper::getExportPath(fileName);

		const bool overwriteFiles = core::view->config.value("overwriteFiles", false);
		if (!overwriteFiles && generics::fileExists(exportPath)) {
			logging::write(std::format("Skipping export of {} (file exists, overwrite disabled)", exportPath));
			h->mark(fileName, true);
		} else {
			BufferWrapper fileData = casc->getVirtualFileByID(fileDataID, true);
			if (fileData.byteLength() == 0)
				throw std::runtime_error("Failed to retrieve DB2 file from CASC");

			fileData.writeToFile(fs::path(exportPath));
			if (ep) ep->writeLine("DB2:" + exportPath);

			h->mark(fileName, true);
			logging::write(std::format("Successfully exported raw DB2 file to {}", exportPath));
		}
	} catch (const std::exception& e) {
		h->mark(fileName, false, e.what(), build_stack_trace("exportRawDB2", e));
		logging::write(std::format("Failed to export raw DB2 file: {}", e.what()));
	}

	if (standalone) {
		if (ep) ep->close();
		h->finish();
	}
}

/**
 * Export data table to SQL format.
 * @param headers     Column header names.
 * @param rows        Row data as strings (empty = NULL).
 * @param tableName   Base name / SQL table name.
 * @param schema      SQL schema map (nullptr = no schema).
 * @param createTable If true, include CREATE TABLE DDL.
 * @param helper      Export helper (nullptr = standalone).
 * @param export_paths FileWriter stream (nullptr = standalone).
 */
void exportDataTableSQL(
	const std::vector<std::string>& headers,
	const std::vector<std::vector<std::string>>& rows,
	const std::string& tableName,
	const std::map<std::string, db::SchemaField>* schema,
	bool createTable,
	casc::ExportHelper* helper,
	FileWriter* export_paths)
{
	if (headers.empty() || rows.empty()) {
		if (!helper)
			core::setToast("info", "No data available to export.");
		return;
	}

	const bool standalone = (helper == nullptr);
	std::optional<casc::ExportHelper> own_helper;
	std::optional<FileWriter> own_export_paths;
	casc::ExportHelper* h;
	FileWriter* ep;

	if (standalone) {
		own_helper.emplace(1, "table");
		own_helper->start();
		own_export_paths.emplace(core::openLastExportStream());
		h = &own_helper.value();
		ep = &own_export_paths.value();
	} else {
		h = helper;
		ep = export_paths;
	}

	const std::string fileName = tableName + ".sql";

	try {
		const std::string exportPath = casc::ExportHelper::getExportPath(fileName);

		const bool overwriteFiles = core::view->config.value("overwriteFiles", false);
		if (!overwriteFiles && generics::fileExists(exportPath)) {
			logging::write(std::format("Skipping export of {} (file exists, overwrite disabled)", exportPath));
			h->mark(fileName, true);
		} else {
			SQLWriter sqlWriter(exportPath, tableName);

			if (schema)
				sqlWriter.setSchema(*schema);

			sqlWriter.setIncludeDDL(createTable);
			sqlWriter.addField(headers);

			for (const auto& row : rows) {
				std::unordered_map<std::string, SQLWriter::SQLValue> rowObject;
				for (size_t i = 0; i < headers.size(); i++) {
					if (i < row.size())
						rowObject[headers[i]] = row[i];
					else
						rowObject[headers[i]] = std::nullopt;
				}
				sqlWriter.addRow(rowObject);
			}

			sqlWriter.write(overwriteFiles);
			if (ep) ep->writeLine("SQL:" + exportPath);

			h->mark(fileName, true);
			logging::write(std::format("Successfully exported data table to {}", exportPath));
		}
	} catch (const std::exception& e) {
		h->mark(fileName, false, e.what(), build_stack_trace("exportDataTableSQL", e));
		logging::write(std::format("Failed to export data table: {}", e.what()));
	}

	if (standalone) {
		if (ep) ep->close();
		h->finish();
	}
}

/**
 * Export raw DBC file from MPQ archive.
 * Always standalone — creates its own helper and export stream.
 * @param tableName Base name for the exported file.
 * @param filePath  Path of the file within the MPQ archive.
 * @param mpq       MPQ install to retrieve the file from.
 */
void exportRawDBC(
	const std::string& tableName,
	const std::string& filePath,
	mpq::MPQInstall* mpq)
{
	if (tableName.empty() || filePath.empty() || !mpq) {
		core::setToast("info", "No DBC file information available to export.");
		return;
	}

	casc::ExportHelper helper(1, "dbc");
	helper.start();

	FileWriter exportPaths = core::openLastExportStream();

	const std::string fileName = tableName + ".dbc";

	try {
		const std::string exportPath = casc::ExportHelper::getExportPath(fileName);

		const bool overwriteFiles = core::view->config.value("overwriteFiles", false);
		if (!overwriteFiles && generics::fileExists(exportPath)) {
			logging::write(std::format("Skipping export of {} (file exists, overwrite disabled)", exportPath));
			helper.mark(fileName, true);
		} else {
			auto raw_data = mpq->getFile(filePath);

			if (!raw_data)
				throw std::runtime_error("Failed to retrieve DBC file from MPQ");

			const fs::path outPath(exportPath);
			fs::create_directories(outPath.parent_path());
			std::ofstream ofs(outPath, std::ios::binary);
			if (!ofs.is_open())
				throw std::runtime_error("Failed to open file for writing: " + exportPath);

			ofs.write(reinterpret_cast<const char*>(raw_data->data()), static_cast<std::streamsize>(raw_data->size()));
			if (!ofs.good())
				throw std::runtime_error("Failed to write DBC file: " + exportPath);

			ofs.close();

			exportPaths.writeLine("DBC:" + exportPath);

			helper.mark(fileName, true);
			logging::write(std::format("Successfully exported raw DBC file to {}", exportPath));
		}
	} catch (const std::exception& e) {
		helper.mark(fileName, false, e.what(), build_stack_trace("exportRawDBC", e));
		logging::write(std::format("Failed to export raw DBC file: {}", e.what()));
	}

	exportPaths.close();
	helper.finish();
}

} // namespace data_exporter
