/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "legacy_tab_data.h"
#include "../log.h"
#include "../core.h"
#include "../buffer.h"
#include "../db/DBCReader.h"
#include "../ui/data-exporter.h"
#include "../install-type.h"
#include "../casc/export-helper.h"
#include "../mpq/mpq-install.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <algorithm>
#include <set>
#include <unordered_map>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace legacy_tab_data {

// --- File-local state ---

// JS: let selected_file = null;
static std::string selected_file;

// JS: let selected_file_path = null;
static std::string selected_file_path;

// JS: let selected_file_schema = null;
static const std::map<std::string, db::DBCSchemaField>* selected_file_schema = nullptr;

// JS: let dbc_listfile = [];
static std::vector<std::string> dbc_listfile;

// JS: let dbc_path_map = new Map();
static std::unordered_map<std::string, std::string> dbc_path_map;

// JS: const DBC_EXTENSION = '.dbc';
static constexpr const char* DBC_EXTENSION = ".dbc";

// JS: data() { return { dbcListfile: [], menuButtonDataLegacy: [...] }; }
static std::vector<std::string> local_dbc_listfile;

// Change-detection for selectionDB2s.
static std::string prev_selection_first;

// --- Internal functions ---

// JS: const initialize_dbc_listfile = async (core) => { ... }
static void initialize_dbc_listfile() {
	if (!dbc_listfile.empty())
		return;

	// JS: const mpq = core.view.mpq;
	// TODO(conversion): MPQ source will be wired when AppState.mpq is integrated.
	// mpq::MPQInstall* mpq = core::view->mpq;
	// if (!mpq) return;

	// JS: const all_dbc_files = mpq.getFilesByExtension(DBC_EXTENSION);
	// TODO(conversion): MPQ file scanning will be wired when integration is complete.
	// auto all_dbc_files = mpq->getFilesByExtension(DBC_EXTENSION);
	std::vector<std::string> all_dbc_files;

	dbc_path_map.clear();
	std::set<std::string> table_names;

	for (const auto& full_path : all_dbc_files) {
		// JS: const parts = full_path.split('\\');
		// JS: const dbc_file = parts[parts.length - 1];
		namespace fs = std::filesystem;
		std::string dbc_file = fs::path(full_path).filename().string();
		// JS: const table_name = dbc_file.replace(/\.dbc$/i, '');
		std::string table_name = dbc_file;
		const auto dot_pos = table_name.rfind('.');
		if (dot_pos != std::string::npos)
			table_name = table_name.substr(0, dot_pos);

		// JS: if (!dbc_path_map.has(table_name)) { ... }
		if (dbc_path_map.find(table_name) == dbc_path_map.end()) {
			dbc_path_map[table_name] = full_path;
			table_names.insert(table_name);
		}
	}

	// JS: dbc_listfile = Array.from(table_names).sort((a, b) => a.localeCompare(b));
	dbc_listfile.assign(table_names.begin(), table_names.end());
	std::sort(dbc_listfile.begin(), dbc_listfile.end());

	logging::write(std::format("initialized {} dbc files from mpq archives", dbc_listfile.size()));
}

// JS: const load_table = async (core, table_name) => { ... }
static void load_table(const std::string& table_name) {
	auto& view = *core::view;

	try {
		// JS: const mpq = core.view.mpq;
		// TODO(conversion): MPQ source will be wired when AppState.mpq is integrated.
		// mpq::MPQInstall* mpq = core::view->mpq;

		// JS: const full_path = dbc_path_map.get(table_name);
		auto it = dbc_path_map.find(table_name);
		if (it == dbc_path_map.end()) {
			core::setToast("error", std::format("Unable to find DBC file: {}", table_name), nullptr, -1);
			return;
		}
		const std::string& full_path = it->second;

		// JS: let raw_data = mpq.getFile(full_path);
		// TODO(conversion): MPQ source will be wired when AppState.mpq is integrated.
		// auto raw_data = mpq ? mpq->getFile(full_path) : std::nullopt;
		std::optional<std::vector<uint8_t>> raw_data = std::nullopt;

		if (!raw_data) {
			core::setToast("error", std::format("Unable to load DBC file: {}", full_path), nullptr, -1);
			return;
		}

		// JS: const data = new BufferWrapper(Buffer.from(raw_data));
		BufferWrapper data(*raw_data);

		// JS: const build_id = get_build_version(core);
		// JS: return core.view.mpq?.build_id ?? '1.12.1.5875';
		// TODO(conversion): MPQ source will provide build_id when wired.
		// std::string build_id = mpq ? mpq->build_id : "1.12.1.5875";
		std::string build_id = "1.12.1.5875";

		// JS: const dbc_reader = new DBCReader(table_name + '.dbc', build_id);
		db::DBCReader dbc_reader(table_name + ".dbc", build_id);
		// JS: await dbc_reader.parse(data);
		dbc_reader.parse(data);

		// JS: const all_headers = [...dbc_reader.schema.keys()];
		std::vector<std::string> all_headers;
		for (const auto& key : dbc_reader.schemaOrder)
			all_headers.push_back(key);

		// JS: const id_index = all_headers.findIndex(header => header.toUpperCase() === 'ID');
		int id_index = -1;
		for (int i = 0; i < static_cast<int>(all_headers.size()); i++) {
			std::string upper = all_headers[i];
			std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
			if (upper == "ID") {
				id_index = i;
				break;
			}
		}

		// JS: if (id_index > 0) { ... move ID to front }
		if (id_index > 0) {
			std::string id_header = all_headers[id_index];
			all_headers.erase(all_headers.begin() + id_index);
			all_headers.insert(all_headers.begin(), id_header);
		}

		// JS: core.view.tableBrowserHeaders = all_headers;
		view.tableBrowserHeaders.clear();
		for (const auto& h : all_headers)
			view.tableBrowserHeaders.push_back(h);

		// JS: core.view.selectionDataTable = [];
		view.selectionDataTable.clear();

		// JS: const rows = await dbc_reader.getAllRows();
		auto rows = dbc_reader.getAllRows();
		if (rows.empty())
			core::setToast("info", "Selected DBC has no rows.");
		else
			core::hideToast(false);

		// JS: const parsed = Array(rows.size);
		// JS: for (const row of rows.values()) { ... }
		view.tableBrowserRows.clear();
		for (const auto& [row_id, row] : rows) {
			nlohmann::json row_values = nlohmann::json::array();
			for (const auto& header : all_headers) {
				auto field_it = row.find(header);
				if (field_it == row.end()) {
					row_values.push_back("");
					continue;
				}

				const auto& value = field_it->second;
				// JS: if (Array.isArray(value)) row_values.push(value.join(', '));
				// JS: else row_values.push(value);
				std::visit([&row_values](const auto& v) {
					using T = std::decay_t<decltype(v)>;
					if constexpr (std::is_same_v<T, std::string>) {
						row_values.push_back(v);
					} else if constexpr (std::is_same_v<T, int64_t>) {
						row_values.push_back(v);
					} else if constexpr (std::is_same_v<T, uint64_t>) {
						row_values.push_back(v);
					} else if constexpr (std::is_same_v<T, float>) {
						row_values.push_back(v);
					} else if constexpr (std::is_same_v<T, std::vector<int64_t>>) {
						std::string joined;
						for (size_t i = 0; i < v.size(); i++) {
							if (i > 0) joined += ", ";
							joined += std::to_string(v[i]);
						}
						row_values.push_back(joined);
					} else if constexpr (std::is_same_v<T, std::vector<uint64_t>>) {
						std::string joined;
						for (size_t i = 0; i < v.size(); i++) {
							if (i > 0) joined += ", ";
							joined += std::to_string(v[i]);
						}
						row_values.push_back(joined);
					} else if constexpr (std::is_same_v<T, std::vector<float>>) {
						std::string joined;
						for (size_t i = 0; i < v.size(); i++) {
							if (i > 0) joined += ", ";
							joined += std::to_string(v[i]);
						}
						row_values.push_back(joined);
					} else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
						std::string joined;
						for (size_t i = 0; i < v.size(); i++) {
							if (i > 0) joined += ", ";
							joined += v[i];
						}
						row_values.push_back(joined);
					} else {
						row_values.push_back("");
					}
				}, value);
			}
			view.tableBrowserRows.push_back(std::move(row_values));
		}

		selected_file = table_name;
		selected_file_path = full_path;
		selected_file_schema = &dbc_reader.schema;
	} catch (const std::exception& e) {
		// JS: core.setToast('error', 'Unable to open DBC file ' + table_name, { 'View Log': () => log.openRuntimeLog() }, -1);
		core::setToast("error", "Unable to open DBC file " + table_name, nullptr, -1);
		// TODO(conversion): 'View Log' toast action will be wired when toast action callbacks are integrated.
		logging::write(std::format("Failed to open DBC file: {}", e.what()));
	}
}

// JS: const get_build_version = (core) => { ... }
static std::string get_build_version() {
	// JS: return core.view.mpq?.build_id ?? '1.12.1.5875';
	// TODO(conversion): MPQ source will provide build_id when wired.
	// mpq::MPQInstall* mpq = core::view->mpq;
	// return mpq ? mpq->build_id : "1.12.1.5875";
	return "1.12.1.5875";
}

// --- Public API ---

void registerTab() {
	// JS: this.registerNavButton('Data', 'database.svg', InstallType.MPQ);
	// TODO(conversion): Nav button registration will be wired when the module system is integrated.
}

void mounted() {
	// JS: this.$core.showLoadingScreen(1);
	core::showLoadingScreen(1);

	try {
		// JS: await this.$core.progressLoadingScreen('Scanning DBC files...');
		core::progressLoadingScreen("Scanning DBC files...");
		// JS: await initialize_dbc_listfile(this.$core);
		initialize_dbc_listfile();

		// JS: this.dbcListfile = dbc_listfile;
		local_dbc_listfile = dbc_listfile;
		core::hideLoadingScreen();
	} catch (const std::exception& e) {
		core::hideLoadingScreen();
		logging::write(std::format("Failed to initialize legacy data tab: {}", e.what()));
		core::setToast("error", "Failed to load DBC files. Check the log for details.");
	}

	// JS: this.$core.view.$watch('selectionDB2s', async selection => { ... });
	// Change-detection is handled in render() by comparing selectionDB2s[0] each frame.
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for selection (equivalent to watch on selectionDB2s) ---
	if (!view.selectionDB2s.empty()) {
		const std::string first = view.selectionDB2s[0].get<std::string>();
		if (view.isBusy == 0 && !first.empty() && first != prev_selection_first) {
			load_table(first);
			prev_selection_first = first;
		}
	}

	// --- Template rendering ---

	// Left panel: DBC table list.
	// JS: <div class="list-container">
	//     <Listbox v-model:selection="selectionDB2s" :items="dbcListfile" :filter="userInputFilterDB2s" ...>
	// </div>
	ImGui::BeginChild("dbc-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, -ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Borders);
	// TODO(conversion): Listbox component rendering will be wired when integration is complete.
	ImGui::Text("DBC tables: %zu", local_dbc_listfile.size());
	ImGui::EndChild();

	// Filter for DBC list.
	// JS: <div class="filter">
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_db2_buf[256] = {};
	std::strncpy(filter_db2_buf, view.userInputFilterDB2s.c_str(), sizeof(filter_db2_buf) - 1);
	if (ImGui::InputText("##FilterDBCs", filter_db2_buf, sizeof(filter_db2_buf)))
		view.userInputFilterDB2s = filter_db2_buf;

	ImGui::SameLine();

	// Right panel: data table.
	// JS: <div class="list-container">
	//     <DataTable ref="dataTable" :headers="tableBrowserHeaders" :rows="tableBrowserRows" ...>
	//     <ContextMenu :node="contextMenus.nodeDataTable" ...>
	//       copy_rows_csv, copy_rows_sql, copy_cell
	// </div>
	ImGui::BeginChild("data-table-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Borders);
	// TODO(conversion): DataTable and ContextMenu component rendering will be wired when integration is complete.
	ImGui::Text("Headers: %zu, Rows: %zu", view.tableBrowserHeaders.size(), view.tableBrowserRows.size());
	ImGui::EndChild();

	// Options row.
	// JS: <div id="tab-data-options">
	const std::string export_format = view.config.value("exportDataFormat", std::string("CSV"));

	// JS: <label> Copy Header </label> (only if CSV)
	if (export_format == "CSV") {
		bool copy_header = view.config.value("dataCopyHeader", false);
		if (ImGui::Checkbox("Copy Header", &copy_header))
			view.config["dataCopyHeader"] = copy_header;
		ImGui::SameLine();
	}

	// JS: <label> Create Table </label> (only if SQL)
	if (export_format == "SQL") {
		bool create_table_val = view.config.value("dataSQLCreateTable", false);
		if (ImGui::Checkbox("Create Table", &create_table_val))
			view.config["dataSQLCreateTable"] = create_table_val;
		ImGui::SameLine();
	}

	// JS: <label> Export all rows </label>
	bool export_all = view.config.value("dataExportAll", false);
	if (ImGui::Checkbox("Export all rows", &export_all))
		view.config["dataExportAll"] = export_all;

	// Bottom tray: data table filter + export button.
	// JS: <div id="tab-data-tray">
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_data_buf[256] = {};
	std::strncpy(filter_data_buf, view.userInputFilterDataTable.c_str(), sizeof(filter_data_buf) - 1);
	if (ImGui::InputText("##FilterDataTable", filter_data_buf, sizeof(filter_data_buf)))
		view.userInputFilterDataTable = filter_data_buf;

	ImGui::SameLine();

	// JS: <MenuButton :options="menuButtonDataLegacy" :default="config.exportDataFormat" @change="..." @click="export_data">
	const bool busy = view.isBusy > 0;
	const bool no_headers = view.tableBrowserHeaders.empty();
	if (busy || no_headers) ImGui::BeginDisabled();
	if (ImGui::Button(std::format("Export as {}", export_format).c_str()))
		export_data();
	if (busy || no_headers) ImGui::EndDisabled();
}

// --- Copy methods (context menu) ---

// JS: methods.copy_rows_csv()
static void copy_rows_csv() {
	// JS: const data_table = this.$refs.dataTable;
	// JS: const csv = data_table.getSelectedRowsAsCSV();
	// TODO(conversion): DataTable getSelectedRowsAsCSV will be wired when DataTable component is integrated.
	const auto& view = *core::view;
	const size_t count = view.selectionDataTable.size();
	if (count == 0)
		return;

	// JS: nw.Clipboard.get().set(csv, 'text');
	// ImGui::SetClipboardText(csv.c_str());
	core::setToast("success", std::format("Copied {} row{} as CSV to the clipboard", count, count != 1 ? "s" : ""), nullptr, 2000);
}

// JS: methods.copy_rows_sql()
static void copy_rows_sql() {
	// JS: const data_table = this.$refs.dataTable;
	// JS: const sql = data_table.getSelectedRowsAsSQL();
	// TODO(conversion): DataTable getSelectedRowsAsSQL will be wired when DataTable component is integrated.
	const auto& view = *core::view;
	const size_t count = view.selectionDataTable.size();
	if (count == 0)
		return;

	// JS: nw.Clipboard.get().set(sql, 'text');
	core::setToast("success", std::format("Copied {} row{} as SQL to the clipboard", count, count != 1 ? "s" : ""), nullptr, 2000);
}

// JS: methods.copy_cell(value)
static void copy_cell(const std::string& value) {
	if (value.empty())
		return;

	// JS: nw.Clipboard.get().set(String(value), 'text');
	ImGui::SetClipboardText(value.c_str());
}

// --- Export methods ---

// JS: methods.export_csv()
static void export_csv() {
	auto& view = *core::view;
	const auto& headers_json = view.tableBrowserHeaders;
	const auto& all_rows_json = view.tableBrowserRows;
	const auto& selection = view.selectionDataTable;
	const bool export_all_val = view.config.value("dataExportAll", false);

	if (headers_json.empty() || all_rows_json.empty()) {
		core::setToast("info", "No data table loaded to export.");
		return;
	}

	// Convert headers to string vector.
	std::vector<std::string> headers;
	for (const auto& h : headers_json)
		headers.push_back(h.get<std::string>());

	// Determine rows to export.
	std::vector<std::vector<std::string>> rows_to_export;
	if (export_all_val) {
		for (const auto& row : all_rows_json) {
			std::vector<std::string> str_row;
			for (const auto& val : row)
				str_row.push_back(val.is_string() ? val.get<std::string>() : val.dump());
			rows_to_export.push_back(std::move(str_row));
		}
	} else {
		if (selection.empty()) {
			core::setToast("info", "No rows selected. Please select some rows first or enable \"Export all rows\".");
			return;
		}

		// JS: rows_to_export = selection.map(row_index => all_rows[row_index]).filter(row => row !== undefined);
		for (const auto& row_index_json : selection) {
			const int row_index = row_index_json.get<int>();
			if (row_index >= 0 && row_index < static_cast<int>(all_rows_json.size())) {
				const auto& row = all_rows_json[row_index];
				std::vector<std::string> str_row;
				for (const auto& val : row)
					str_row.push_back(val.is_string() ? val.get<std::string>() : val.dump());
				rows_to_export.push_back(std::move(str_row));
			}
		}

		if (rows_to_export.empty()) {
			core::setToast("info", "No rows selected. Please select some rows first or enable \"Export all rows\".");
			return;
		}
	}

	// JS: await dataExporter.exportDataTable(headers, rows_to_export, selected_file || 'unknown_table');
	data_exporter::exportDataTable(headers, rows_to_export, selected_file.empty() ? "unknown_table" : selected_file);
}

// JS: methods.export_sql()
static void export_sql() {
	auto& view = *core::view;
	const auto& headers_json = view.tableBrowserHeaders;
	const auto& all_rows_json = view.tableBrowserRows;
	const auto& selection = view.selectionDataTable;
	const bool export_all_val = view.config.value("dataExportAll", false);

	if (headers_json.empty() || all_rows_json.empty()) {
		core::setToast("info", "No data table loaded to export.");
		return;
	}

	std::vector<std::string> headers;
	for (const auto& h : headers_json)
		headers.push_back(h.get<std::string>());

	std::vector<std::vector<std::string>> rows_to_export;
	if (export_all_val) {
		for (const auto& row : all_rows_json) {
			std::vector<std::string> str_row;
			for (const auto& val : row)
				str_row.push_back(val.is_string() ? val.get<std::string>() : val.dump());
			rows_to_export.push_back(std::move(str_row));
		}
	} else {
		if (selection.empty()) {
			core::setToast("info", "No rows selected. Please select some rows first or enable \"Export all rows\".");
			return;
		}

		for (const auto& row_index_json : selection) {
			const int row_index = row_index_json.get<int>();
			if (row_index >= 0 && row_index < static_cast<int>(all_rows_json.size())) {
				const auto& row = all_rows_json[row_index];
				std::vector<std::string> str_row;
				for (const auto& val : row)
					str_row.push_back(val.is_string() ? val.get<std::string>() : val.dump());
				rows_to_export.push_back(std::move(str_row));
			}
		}

		if (rows_to_export.empty()) {
			core::setToast("info", "No rows selected. Please select some rows first or enable \"Export all rows\".");
			return;
		}
	}

	// JS: const create_table = this.$core.view.config.dataSQLCreateTable;
	const bool create_table = view.config.value("dataSQLCreateTable", false);

	// JS: await dataExporter.exportDataTableSQL(headers, rows_to_export, selected_file || 'unknown_table', selected_file_schema, create_table);
	// Note: JS uses DBC schema (map<string, DBCSchemaField>), data_exporter expects map<string, SchemaField>.
	// TODO(conversion): Schema type conversion will be wired when data-exporter DBC support is complete.
	data_exporter::exportDataTableSQL(headers, rows_to_export,
		selected_file.empty() ? "unknown_table" : selected_file,
		nullptr, create_table);
}

// JS: methods.export_dbc()
static void export_dbc() {
	if (selected_file.empty() || selected_file_path.empty()) {
		core::setToast("info", "No DBC file selected to export.");
		return;
	}

	// JS: await dataExporter.exportRawDBC(selected_file, selected_file_path, this.$core.view.mpq);
	// TODO(conversion): MPQ source will be wired when AppState.mpq is integrated.
	// mpq::MPQInstall* mpq = core::view->mpq;
	data_exporter::exportRawDBC(selected_file, selected_file_path, nullptr);
}

void export_data() {
	// JS: const format = this.$core.view.config.exportDataFormat;
	const std::string format = core::view->config.value("exportDataFormat", std::string("CSV"));

	if (format == "CSV")
		export_csv();
	else if (format == "SQL")
		export_sql();
	else if (format == "DBC")
		export_dbc();
}

} // namespace legacy_tab_data
