/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_data.h"
#include "../log.h"
#include "../core.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/dbd-manifest.h"
#include "../casc/db2.h"
#include "../db/WDCReader.h"
#include "../ui/data-exporter.h"
#include "../install-type.h"
#include "../modules.h"
#include "../file-writer.h"
#include "../components/listbox.h"
#include "../components/data-table.h"
#include "../components/context-menu.h"
#include "../components/menu-button.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <optional>
#include <algorithm>
#include <variant>

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace tab_data {

// --- File-local state ---

// JS: let selected_file = null;
static std::string selected_file;

// JS: let selected_file_data_id = null;
static std::optional<int> selected_file_data_id;

// JS: let selected_file_schema = null;
static const std::map<std::string, db::SchemaField>* selected_file_schema = nullptr;

// JS: data() { return { active_table: '' }; }
static std::string active_table;

// Change-detection for selectionDB2s.
static std::string prev_selection_last;

static listbox::ListboxState listbox_db2_state;
static data_table::DataTableState data_table_state;
static context_menu::ContextMenuState data_table_ctx_state;
static menu_button::MenuButtonState menu_button_data_state;

// --- Forward declarations ---
static void copy_rows_csv();
static void copy_rows_sql();
static void copy_cell(const nlohmann::json& value);

// --- Internal functions ---

// JS: const initialize_available_tables = async (core) => { ... }
static void initialize_available_tables() {
	auto& view = *core::view;
	auto& manifest = view.dbdManifest;
	if (!manifest.empty())
		return;

	casc::dbd_manifest::prepareManifest();
	const auto table_names = casc::dbd_manifest::getAllTableNames();
	// JS: manifest.push(...table_names);
	for (const auto& name : table_names)
		manifest.push_back(name);
	logging::write("initialized available db2 tables from dbd manifest");
}

// Helper to convert a FieldValue to a display string.
static std::string field_value_to_string(const db::FieldValue& val) {
	return std::visit([](const auto& v) -> std::string {
		using T = std::decay_t<decltype(v)>;
		if constexpr (std::is_same_v<T, int64_t>)
			return std::to_string(v);
		else if constexpr (std::is_same_v<T, uint64_t>)
			return std::to_string(v);
		else if constexpr (std::is_same_v<T, float>)
			return std::to_string(v);
		else if constexpr (std::is_same_v<T, std::string>)
			return v;
		else if constexpr (std::is_same_v<T, std::vector<int64_t>>) {
			std::string result = "[";
			for (size_t i = 0; i < v.size(); i++) {
				if (i > 0) result += ", ";
				result += std::to_string(v[i]);
			}
			return result + "]";
		} else if constexpr (std::is_same_v<T, std::vector<uint64_t>>) {
			std::string result = "[";
			for (size_t i = 0; i < v.size(); i++) {
				if (i > 0) result += ", ";
				result += std::to_string(v[i]);
			}
			return result + "]";
		} else if constexpr (std::is_same_v<T, std::vector<float>>) {
			std::string result = "[";
			for (size_t i = 0; i < v.size(); i++) {
				if (i > 0) result += ", ";
				result += std::to_string(v[i]);
			}
			return result + "]";
		} else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
			std::string result = "[";
			for (size_t i = 0; i < v.size(); i++) {
				if (i > 0) result += ", ";
				result += v[i];
			}
			return result + "]";
		} else {
			return "";
		}
	}, val);
}

// JS: const parse_table = async (table_name) => { ... }
struct ParseTableResult {
	std::vector<std::string> headers;
	std::vector<std::vector<std::string>> rows;
	const std::map<std::string, db::SchemaField>* schema = nullptr;
};

static ParseTableResult parse_table(const std::string& table_name) {
	// JS: const db2_reader = new WDCReader('DBFilesClient/' + table_name + '.db2');
	// JS: await db2_reader.parse();
	auto& db2_reader = casc::db2::getTable(table_name);
	if (!db2_reader.isLoaded)
		db2_reader.parse();

	// JS: const all_headers = [...db2_reader.schema.keys()];
	const auto& schema = db2_reader.schema;
	std::vector<std::string> all_headers;
	// Use schemaOrder to preserve insertion order (matches JS Map iteration order).
	for (const auto& key : db2_reader.schemaOrder)
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

	// JS: const rows = await db2_reader.getAllRows();
	// JS: for (const row of rows.values()) { ... }
	std::vector<std::vector<std::string>> parsed;

	for (const auto& [id, row] : db2_reader.getAllRows()) {
		std::vector<std::string> row_values;
		for (const auto& header : all_headers) {
			auto it = row.find(header);
			if (it != row.end())
				row_values.push_back(field_value_to_string(it->second));
			else
				row_values.push_back("");
		}
		parsed.push_back(std::move(row_values));
	}

	return { std::move(all_headers), std::move(parsed), &schema };
}

// JS: const load_table = async (core, table_name) => { ... }
static void load_table(const std::string& table_name) {
	auto& view = *core::view;

	try {
		// JS: selected_file_data_id = dbd_manifest.getByTableName(table_name) || null;
		selected_file_data_id = casc::dbd_manifest::getByTableName(table_name);

		const auto result = parse_table(table_name);

		// JS: core.view.tableBrowserHeaders = result.headers;
		view.tableBrowserHeaders.clear();
		for (const auto& h : result.headers)
			view.tableBrowserHeaders.push_back(h);

		// JS: core.view.selectionDataTable = [];
		view.selectionDataTable.clear();

		if (result.rows.empty())
			core::setToast("info", "Selected DB2 has no rows.");
		else
			core::hideToast(false);

		// JS: core.view.tableBrowserRows = result.rows;
		view.tableBrowserRows.clear();
		for (const auto& row : result.rows) {
			nlohmann::json json_row = nlohmann::json::array();
			for (const auto& val : row)
				json_row.push_back(val);
			view.tableBrowserRows.push_back(std::move(json_row));
		}

		selected_file = table_name;
		selected_file_schema = result.schema;
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to open DB2 file " + table_name, {}, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}
}

// --- Public API ---

void registerTab() {
	// JS: this.registerNavButton('Data', 'database.svg', InstallType.CASC);
	modules::register_nav_button("tab_data", "Data", "database.svg", install_type::CASC);
}

void mounted() {
	auto& view = *core::view;

	// JS: await this.initialize();
	core::showLoadingScreen(1);
	core::progressLoadingScreen("Loading data table manifest...");
	initialize_available_tables();
	core::hideLoadingScreen();

	// JS: this.$core.view.$watch('selectionDB2s', async selection => { ... });
	// Change-detection is handled in render() by comparing selectionDB2s.back() each frame.
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for selection (equivalent to watch on selectionDB2s) ---
	if (!view.selectionDB2s.empty()) {
		const std::string last = view.selectionDB2s.back().get<std::string>();
		if (view.isBusy == 0 && !last.empty() && last != prev_selection_last) {
			load_table(last);
			active_table = selected_file;
			prev_selection_last = last;
		}
	}

	// --- Template rendering ---

	// Left panel: DB2 table list.
	// JS: <div class="list-container">
	//     <Listbox v-model:selection="selectionDB2s" :items="dbdManifest" ...>
	// </div>
	ImGui::BeginChild("db2-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, -ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Borders);
	{
		std::vector<std::string> items_str;
		items_str.reserve(view.dbdManifest.size());
		for (const auto& item : view.dbdManifest)
			items_str.push_back(item.get<std::string>());

		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionDB2s)
			selection_str.push_back(s.get<std::string>());

		listbox::render(
			"listbox-db2",
			items_str,
			view.userInputFilterDB2s,
			selection_str,
			true,    // single
			true,    // keyinput
			view.config.value("regexFilters", false),
			listbox::CopyMode::Default,
			false,   // pasteselection
			false,   // copytrimwhitespace
			"table", // unittype
			nullptr, // overrideItems
			false,   // disable
			"db2",   // persistscrollkey
			{},      // quickfilters
			false,   // nocopy
			listbox_db2_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionDB2s.clear();
				for (const auto& s : new_sel)
					view.selectionDB2s.push_back(s);
			},
			nullptr  // no context menu
		);
	}
	ImGui::EndChild();

	// Filter for DB2 list.
	// JS: <div class="filter">
	if (view.config.value("regexFilters", false))
		ImGui::TextUnformatted("Regex Enabled");

	char filter_db2_buf[256] = {};
	std::strncpy(filter_db2_buf, view.userInputFilterDB2s.c_str(), sizeof(filter_db2_buf) - 1);
	if (ImGui::InputText("##FilterDB2s", filter_db2_buf, sizeof(filter_db2_buf)))
		view.userInputFilterDB2s = filter_db2_buf;

	ImGui::SameLine();

	// Right panel: data table.
	// JS: <div class="list-container">
	//     <DataTable ref="dataTable" :headers="tableBrowserHeaders" :rows="tableBrowserRows" ...>
	//     <ContextMenu :node="contextMenus.nodeDataTable" ...>
	// </div>
	ImGui::BeginChild("data-table-container", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Borders);
	{
		// Convert json headers to string array.
		std::vector<std::string> headers_str;
		for (const auto& h : view.tableBrowserHeaders)
			headers_str.push_back(h.get<std::string>());

		// Convert json rows to string 2D array.
		std::vector<std::vector<std::string>> rows_str;
		for (const auto& row : view.tableBrowserRows) {
			std::vector<std::string> str_row;
			for (const auto& val : row)
				str_row.push_back(val.is_string() ? val.get<std::string>() : val.dump());
			rows_str.push_back(std::move(str_row));
		}

		// Convert selection indices from json to int array.
		std::vector<int> sel_indices;
		for (const auto& s : view.selectionDataTable)
			sel_indices.push_back(s.get<int>());

		data_table::render("##DataTable", headers_str, rows_str,
			view.userInputFilterDataTable,
			view.config.value("regexFilters", false),
			sel_indices,
			view.config.value("dataCopyHeader", false),
			selected_file.empty() ? "unknown_table" : selected_file,
			data_table_state,
			[&](const std::vector<int>& new_sel) {
				view.selectionDataTable.clear();
				for (int idx : new_sel)
					view.selectionDataTable.push_back(idx);
			},
			[&](const data_table::ContextMenuEvent& ev) {
				nlohmann::json node;
				node["rowIndex"] = ev.rowIndex;
				node["columnIndex"] = ev.columnIndex;
				node["cellValue"] = ev.cellValue;
				node["selectedCount"] = ev.selectedCount;
				view.contextMenus.nodeDataTable = node;
			},
			[&]() { copy_rows_csv(); },
			nullptr);

		// Context menu for data table.
		if (!view.contextMenus.nodeDataTable.is_null()) {
			if (ImGui::BeginPopupContextItem("##DataTableContextMenu")) {
				if (ImGui::MenuItem("Copy selected rows as CSV"))
					copy_rows_csv();
				if (ImGui::MenuItem("Copy selected rows as SQL"))
					copy_rows_sql();
				if (view.contextMenus.nodeDataTable.contains("cellValue")) {
					std::string cell_val = view.contextMenus.nodeDataTable["cellValue"].is_string()
						? view.contextMenus.nodeDataTable["cellValue"].get<std::string>()
						: view.contextMenus.nodeDataTable["cellValue"].dump();
					if (ImGui::MenuItem(std::format("Copy cell: {}", cell_val.substr(0, 30)).c_str()))
						copy_cell(view.contextMenus.nodeDataTable["cellValue"]);
				}
				ImGui::EndPopup();
			}
		}
	}
	ImGui::EndChild();

	// Options row.
	// JS: <div id="tab-data-options">
	const std::string export_format = view.config.value("exportDataFormat", std::string("CSV"));

	if (export_format == "CSV") {
		bool copy_header = view.config.value("dataCopyHeader", false);
		if (ImGui::Checkbox("Copy Header", &copy_header))
			view.config["dataCopyHeader"] = copy_header;
		ImGui::SameLine();
	}

	if (export_format == "SQL") {
		bool create_table_val = view.config.value("dataSQLCreateTable", false);
		if (ImGui::Checkbox("Create Table", &create_table_val))
			view.config["dataSQLCreateTable"] = create_table_val;
		ImGui::SameLine();
	}

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

	// JS: <MenuButton :options="menuButtonData" :default="config.exportDataFormat" @change="..." @click="export_data">
	{
		std::vector<menu_button::MenuOption> mb_options;
		for (const auto& opt : view.menuButtonData)
			mb_options.push_back({ opt.label, opt.value });
		const bool busy = view.isBusy > 0;
		const bool no_selection = view.selectionDB2s.empty();
		menu_button::render("##MenuButtonData", mb_options,
			view.config.value("exportDataFormat", std::string("CSV")),
			busy || no_selection, false, menu_button_data_state,
			[&](const std::string& val) { view.config["exportDataFormat"] = val; },
			[&]() { export_data(); });
	}
}

// --- Export methods ---

// JS: methods.copy_rows_csv()
static void copy_rows_csv() {
	// JS: const data_table = this.$refs.dataTable;
	// JS: const csv = data_table.getSelectedRowsAsCSV();
	const auto& view = *core::view;
	const size_t count = view.selectionDataTable.size();
	if (count == 0)
		return;

	// Convert headers to string vector.
	std::vector<std::string> headers;
	for (const auto& h : view.tableBrowserHeaders)
		headers.push_back(h.get<std::string>());

	// Convert all rows to string 2D array (sorted items is the same as rows for now).
	std::vector<std::vector<std::string>> sorted_rows;
	for (const auto& row : view.tableBrowserRows) {
		std::vector<std::string> str_row;
		for (const auto& val : row)
			str_row.push_back(val.is_string() ? val.get<std::string>() : val.dump());
		sorted_rows.push_back(std::move(str_row));
	}

	// Convert selection to int indices.
	std::vector<int> sel_indices;
	for (const auto& s : view.selectionDataTable)
		sel_indices.push_back(s.get<int>());

	std::string csv = data_table::getSelectedRowsAsCSV(headers, sorted_rows, sel_indices,
		view.config.value("dataCopyHeader", false));

	// JS: nw.Clipboard.get().set(csv, 'text');
	ImGui::SetClipboardText(csv.c_str());
	core::setToast("success", std::format("Copied {} row{} as CSV to the clipboard", count, count != 1 ? "s" : ""), {}, 2000);
}

// JS: methods.copy_rows_sql()
static void copy_rows_sql() {
	// JS: const data_table = this.$refs.dataTable;
	// JS: const sql = data_table.getSelectedRowsAsSQL();
	const auto& view = *core::view;
	const size_t count = view.selectionDataTable.size();
	if (count == 0)
		return;

	// Convert headers to string vector.
	std::vector<std::string> headers;
	for (const auto& h : view.tableBrowserHeaders)
		headers.push_back(h.get<std::string>());

	// Convert all rows to string 2D array.
	std::vector<std::vector<std::string>> sorted_rows;
	for (const auto& row : view.tableBrowserRows) {
		std::vector<std::string> str_row;
		for (const auto& val : row)
			str_row.push_back(val.is_string() ? val.get<std::string>() : val.dump());
		sorted_rows.push_back(std::move(str_row));
	}

	// Convert selection to int indices.
	std::vector<int> sel_indices;
	for (const auto& s : view.selectionDataTable)
		sel_indices.push_back(s.get<int>());

	std::string sql = data_table::getSelectedRowsAsSQL(headers, sorted_rows, sel_indices,
		selected_file.empty() ? "unknown_table" : selected_file);

	// JS: nw.Clipboard.get().set(sql, 'text');
	ImGui::SetClipboardText(sql.c_str());
	core::setToast("success", std::format("Copied {} row{} as SQL to the clipboard", count, count != 1 ? "s" : ""), {}, 2000);
}

// JS: methods.copy_cell(value)
static void copy_cell(const nlohmann::json& value) {
	if (value.is_null())
		return;

	// JS: nw.Clipboard.get().set(String(value), 'text');
	ImGui::SetClipboardText(value.dump().c_str());
}

// JS: methods.export_csv()
static void export_csv() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionDB2s;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any tables to export.");
		return;
	}

	// single table: use row selection behavior
	if (user_selection.size() == 1) {
		// JS: const headers = this.$core.view.tableBrowserHeaders;
		// JS: const all_rows = this.$core.view.tableBrowserRows;
		// JS: const selection = this.$core.view.selectionDataTable;
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

		data_exporter::exportDataTable(headers, rows_to_export, selected_file.empty() ? "unknown_table" : selected_file);
		return;
	}

	// multiple tables: export all rows from each
	casc::ExportHelper helper(static_cast<int>(user_selection.size()), "table");
	helper.start();

	FileWriter export_paths = core::openLastExportStream();

	for (const auto& table_json : user_selection) {
		if (helper.isCancelled())
			break;

		const std::string table_name = table_json.get<std::string>();

		try {
			const auto result = parse_table(table_name);

			data_exporter::exportDataTable(result.headers, result.rows, table_name, &helper, &export_paths);
		} catch (const std::exception& e) {
			helper.mark(table_name + ".csv", false, e.what());
			logging::write(std::format("Failed to export table {}: {}", table_name, e.what()));
		}
	}

	// JS: export_paths?.close();
	export_paths.close();
	helper.finish();
}

// JS: methods.export_sql()
static void export_sql() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionDB2s;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any tables to export.");
		return;
	}

	const bool create_table_val = view.config.value("dataSQLCreateTable", false);

	// single table: use row selection behavior
	if (user_selection.size() == 1) {
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

		data_exporter::exportDataTableSQL(headers, rows_to_export,
			selected_file.empty() ? "unknown_table" : selected_file,
			selected_file_schema, create_table_val);
		return;
	}

	// multiple tables: export all rows from each
	casc::ExportHelper helper(static_cast<int>(user_selection.size()), "table");
	helper.start();

	FileWriter export_paths = core::openLastExportStream();

	for (const auto& table_json : user_selection) {
		if (helper.isCancelled())
			break;

		const std::string table_name = table_json.get<std::string>();

		try {
			const auto result = parse_table(table_name);
			data_exporter::exportDataTableSQL(result.headers, result.rows, table_name,
				result.schema, create_table_val, &helper, &export_paths);
		} catch (const std::exception& e) {
			helper.mark(table_name + ".sql", false, e.what());
			logging::write(std::format("Failed to export table {}: {}", table_name, e.what()));
		}
	}

	export_paths.close();
	helper.finish();
}

// JS: methods.export_db2()
static void export_db2() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionDB2s;
	if (user_selection.empty()) {
		core::setToast("info", "No DB2 files selected to export.");
		return;
	}

	// single table
	if (user_selection.size() == 1) {
		if (selected_file.empty() || !selected_file_data_id.has_value()) {
			core::setToast("info", "No DB2 file selected to export.");
			return;
		}

		// JS: await dataExporter.exportRawDB2(selected_file, selected_file_data_id);
		data_exporter::exportRawDB2(selected_file, static_cast<uint32_t>(*selected_file_data_id), core::view->casc);
		return;
	}

	// multiple tables
	casc::ExportHelper helper(static_cast<int>(user_selection.size()), "db2");
	helper.start();

	FileWriter export_paths = core::openLastExportStream();

	for (const auto& table_json : user_selection) {
		if (helper.isCancelled())
			break;

		const std::string table_name = table_json.get<std::string>();

		try {
			const auto file_data_id_opt = casc::dbd_manifest::getByTableName(table_name);
			if (!file_data_id_opt.has_value())
				throw std::runtime_error("No file data ID found for table " + table_name);

			// JS: await dataExporter.exportRawDB2(table_name, file_data_id, { helper, export_paths });
			data_exporter::exportRawDB2(table_name, static_cast<uint32_t>(*file_data_id_opt),
				nullptr, &helper, &export_paths);
		} catch (const std::exception& e) {
			helper.mark(table_name + ".db2", false, e.what());
			logging::write(std::format("Failed to export DB2 {}: {}", table_name, e.what()));
		}
	}

	export_paths.close();
	helper.finish();
}

void export_data() {
	const std::string format = core::view->config.value("exportDataFormat", std::string("CSV"));

	if (format == "CSV")
		export_csv();
	else if (format == "SQL")
		export_sql();
	else if (format == "DB2")
		export_db2();
}

} // namespace tab_data
