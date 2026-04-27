/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "legacy_tab_data.h"
#include "../log.h"
#include "../core.h"
#include "../../app.h"
#include "../buffer.h"
#include "../db/DBCReader.h"
#include "../ui/data-exporter.h"
#include "../install-type.h"
#include "../modules.h"
#include "../casc/export-helper.h"
#include "../components/listbox.h"
#include "../components/data-table.h"
#include "../components/context-menu.h"
#include "../components/menu-button.h"
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

static std::string selected_file;

static std::string selected_file_path;

// Stored as a copy (not a pointer) because the DBCReader is a local variable in load_table().
static std::map<std::string, db::DBCSchemaField> selected_file_schema;

static std::vector<std::string> dbc_listfile;

static std::unordered_map<std::string, std::string> dbc_path_map;

static constexpr const char* DBC_EXTENSION = ".dbc";

static std::vector<std::string> local_dbc_listfile;

// Change-detection for selectionDB2s.
static std::string prev_selection_first;

static listbox::ListboxState listbox_dbc_state;
static data_table::DataTableState legacy_data_table_state;
static context_menu::ContextMenuState legacy_data_table_ctx_state;
static menu_button::MenuButtonState legacy_menu_button_data_state;

// --- Forward declarations ---
static void copy_rows_csv();
static void copy_rows_sql();
static void copy_cell(const std::string& value);

// --- Internal functions ---

static void initialize_dbc_listfile() {
	if (!dbc_listfile.empty())
		return;

	mpq::MPQInstall* mpq = core::view->mpq.get();
	if (!mpq) return;

	auto all_dbc_files = mpq->getFilesByExtension(DBC_EXTENSION);

	dbc_path_map.clear();
	std::set<std::string> table_names;

	for (const auto& full_path : all_dbc_files) {
		const auto sep_pos = full_path.find_last_of('\\');
		std::string dbc_file = (sep_pos != std::string::npos) ? full_path.substr(sep_pos + 1) : full_path;
		std::string table_name = dbc_file;
		const auto dot_pos = table_name.rfind('.');
		if (dot_pos != std::string::npos)
			table_name = table_name.substr(0, dot_pos);

		if (dbc_path_map.find(table_name) == dbc_path_map.end()) {
			dbc_path_map[table_name] = full_path;
			table_names.insert(table_name);
		}
	}

	dbc_listfile.assign(table_names.begin(), table_names.end());
	std::sort(dbc_listfile.begin(), dbc_listfile.end());

	logging::write(std::format("initialized {} dbc files from mpq archives", dbc_listfile.size()));
}

static void load_table(const std::string& table_name) {
	auto& view = *core::view;

	try {
		mpq::MPQInstall* mpq = core::view->mpq.get();

		auto it = dbc_path_map.find(table_name);
		if (it == dbc_path_map.end()) {
			core::setToast("error", std::format("Unable to find DBC file: {}", table_name), {}, -1);
			return;
		}
		const std::string& full_path = it->second;

		auto raw_data = mpq ? mpq->getFile(full_path) : std::nullopt;

		if (!raw_data) {
			core::setToast("error", std::format("Unable to load DBC file: {}", full_path), {}, -1);
			return;
		}

		BufferWrapper data(*raw_data);

		std::string build_id = mpq ? mpq->build_id : "1.12.1.5875";

		db::DBCReader dbc_reader(table_name + ".dbc", build_id);
		dbc_reader.parse(data);

		std::vector<std::string> all_headers;
		for (const auto& key : dbc_reader.schemaOrder)
			all_headers.push_back(key);

		int id_index = -1;
		for (int i = 0; i < static_cast<int>(all_headers.size()); i++) {
			std::string upper = all_headers[i];
			std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
			if (upper == "ID") {
				id_index = i;
				break;
			}
		}

		if (id_index > 0) {
			std::string id_header = all_headers[id_index];
			all_headers.erase(all_headers.begin() + id_index);
			all_headers.insert(all_headers.begin(), id_header);
		}

		view.tableBrowserHeaders.clear();
		for (const auto& h : all_headers)
			view.tableBrowserHeaders.push_back(h);

		view.selectionDataTable.clear();

		auto rows = dbc_reader.getAllRows();
		if (rows.empty())
			core::setToast("info", "Selected DBC has no rows.");
		else
			core::hideToast(false);

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
		selected_file_schema = dbc_reader.schema;
	} catch (const std::exception& e) {
		core::setToast("error", "Unable to open DBC file " + table_name,
			{ {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
		logging::write(std::format("Failed to open DBC file: {}", e.what()));
	}
}

static std::string get_build_version() {
	mpq::MPQInstall* mpq = core::view->mpq.get();
	return mpq ? mpq->build_id : "1.12.1.5875";
}

// --- Public API ---

void registerTab() {
	modules::register_nav_button("legacy_tab_data", "Data", "database.svg", install_type::MPQ);
}

void mounted() {
	core::showLoadingScreen(1);

	try {
		core::progressLoadingScreen("Scanning DBC files...");
		initialize_dbc_listfile();

		local_dbc_listfile = dbc_listfile;
		core::hideLoadingScreen();
	} catch (const std::exception& e) {
		core::hideLoadingScreen();
		logging::write(std::format("Failed to initialize legacy data tab: {}", e.what()));
		core::setToast("error", "Failed to load DBC files. Check the log for details.");
	}

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

	if (app::layout::BeginTab("tab-legacy-data")) {

	auto regions = app::layout::CalcListTabRegions(false, 1.0f / 7.0f);

	// --- Left panel: List container (row 1, col 1) ---
	//     <Listbox v-model:selection="selectionDB2s" :items="dbcListfile" :filter="userInputFilterDB2s" ...>
	// </div>
	if (app::layout::BeginListContainer("dbc-list-container", regions)) {
		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionDB2s)
			selection_str.push_back(s.get<std::string>());

		listbox::render(
			"listbox-dbc",
			local_dbc_listfile,
			view.userInputFilterDB2s,
			selection_str,
			true,    // single
			true,    // keyinput
			view.config.value("regexFilters", false),
			listbox::CopyMode::Default,
			false,   // pasteselection
			false,   // copytrimwhitespace
			"dbc file", // unittype
			nullptr, // overrideItems
			false,   // disable
			"dbc",   // persistscrollkey
			{},      // quickfilters
			true,    // nocopy
			listbox_dbc_state,
			[&](const std::vector<std::string>& new_sel) {
				view.selectionDB2s.clear();
				for (const auto& s : new_sel)
					view.selectionDB2s.push_back(s);
			},
			nullptr  // no context menu
		);
	}
	app::layout::EndListContainer();

	// --- Status bar ---
	if (app::layout::BeginStatusBar("dbc-status", regions)) {
		listbox::renderStatusBar("table", {}, listbox_dbc_state);
	}
	app::layout::EndStatusBar();

	// --- Filter bar (row 2, col 1) ---
	if (app::layout::BeginFilterBar("dbc-filter", regions)) {
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_db2_buf[256] = {};
		std::strncpy(filter_db2_buf, view.userInputFilterDB2s.c_str(), sizeof(filter_db2_buf) - 1);
		if (ImGui::InputText("##FilterDBCs", filter_db2_buf, sizeof(filter_db2_buf)))
			view.userInputFilterDB2s = filter_db2_buf;
	}
	app::layout::EndFilterBar();

	// --- Right panel: Preview container (row 1, col 2) ---
	//     <DataTable ref="dataTable" :headers="tableBrowserHeaders" :rows="tableBrowserRows" ...>
	//     <ContextMenu :node="contextMenus.nodeDataTable" ...>
	//       copy_rows_csv, copy_rows_sql, copy_cell
	// </div>
	if (app::layout::BeginPreviewContainer("legacy-data-table-container", regions)) {
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

		data_table::render("##LegacyDataTable", headers_str, rows_str,
			view.userInputFilterDataTable,
			view.config.value("regexFilters", false),
			sel_indices,
			view.config.value("dataCopyHeader", false),
			selected_file.empty() ? "unknown_table" : selected_file,
			legacy_data_table_state,
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
				// Open the popup immediately on right-click (JS: ContextMenu opens on @contextmenu).
				ImGui::OpenPopup("##LegacyDataTableContextMenu");
			},
			[&]() { copy_rows_csv(); },
			nullptr);

		// Context menu for data table.
		// JS: <ContextMenu :node="contextMenus.nodeDataTable" @close="contextMenus.nodeDataTable = null">
		if (ImGui::BeginPopup("##LegacyDataTableContextMenu")) {
			if (!view.contextMenus.nodeDataTable.is_null()) {
				const int selected_count = view.contextMenus.nodeDataTable.value("selectedCount", 0);
				const std::string csv_label = std::format("Copy {} row{} as CSV", selected_count, selected_count != 1 ? "s" : "");
				const std::string sql_label = std::format("Copy {} row{} as SQL", selected_count, selected_count != 1 ? "s" : "");
				if (ImGui::MenuItem(csv_label.c_str()))
					copy_rows_csv();
				if (ImGui::MenuItem(sql_label.c_str()))
					copy_rows_sql();
				if (view.contextMenus.nodeDataTable.contains("cellValue")) {
					std::string cell_val = view.contextMenus.nodeDataTable["cellValue"].is_string()
						? view.contextMenus.nodeDataTable["cellValue"].get<std::string>()
						: view.contextMenus.nodeDataTable["cellValue"].dump();
					if (ImGui::MenuItem("Copy cell contents"))
						copy_cell(cell_val);
				}
			}
			ImGui::EndPopup();
		} else if (!view.contextMenus.nodeDataTable.is_null()) {
			// @close event: clear nodeDataTable when popup closes (JS: @close="nodeDataTable = null").
			view.contextMenus.nodeDataTable = nullptr;
		}

		// Options row.
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
	}
	app::layout::EndPreviewContainer();

	// --- Bottom: Preview controls (row 2, col 2) ---
	//     <input> filter + <MenuButton> export
	if (app::layout::BeginPreviewControls("legacy-data-preview-controls", regions)) {
		char filter_data_buf[256] = {};
		std::strncpy(filter_data_buf, view.userInputFilterDataTable.c_str(), sizeof(filter_data_buf) - 1);
		if (ImGui::InputText("##FilterDataTable", filter_data_buf, sizeof(filter_data_buf)))
			view.userInputFilterDataTable = filter_data_buf;

		ImGui::SameLine();

		{
			// JS menuButtonDataLegacy: CSV, SQL, DBC (Raw).
			static const std::vector<menu_button::MenuOption> legacy_data_opts = {
				{ "Export as CSV", "CSV" },
				{ "Export as SQL", "SQL" },
				{ "Export DBC (Raw)", "DBC" }
			};
			const bool busy = view.isBusy > 0;
			const bool no_headers = view.tableBrowserHeaders.empty();
			menu_button::render("##MenuButtonDataLegacy", legacy_data_opts,
				view.config.value("exportDataFormat", std::string("CSV")),
				busy || no_headers, false, legacy_menu_button_data_state,
				[&](const std::string& val) { view.config["exportDataFormat"] = val; },
				[&]() { export_data(); });
		}
	}
	app::layout::EndPreviewControls();

	}
	app::layout::EndTab();
}

// --- Copy methods (context menu) ---

static void copy_rows_csv() {
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

	std::string csv = data_table::getSelectedRowsAsCSV(headers, sorted_rows, sel_indices,
		view.config.value("dataCopyHeader", false));

	ImGui::SetClipboardText(csv.c_str());
	core::setToast("success", std::format("Copied {} row{} as CSV to the clipboard", count, count != 1 ? "s" : ""), {}, 2000);
}

static void copy_rows_sql() {
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

	ImGui::SetClipboardText(sql.c_str());
	core::setToast("success", std::format("Copied {} row{} as SQL to the clipboard", count, count != 1 ? "s" : ""), {}, 2000);
}

static void copy_cell(const std::string& value) {
	// JS: if (value === null || value === undefined) return; — copies empty strings.
	ImGui::SetClipboardText(value.c_str());
}

// --- Export methods ---

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
}

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

	const bool create_table = view.config.value("dataSQLCreateTable", false);

	// Convert DBC schema (map<string, DBCSchemaField>) to WDC schema (map<string, SchemaField>).
	std::map<std::string, db::SchemaField> converted_schema;
	for (const auto& [name, field] : selected_file_schema) {
		if (field.array_length > 0)
			converted_schema[name] = std::pair<db::FieldType, int>(field.type, field.array_length);
		else
			converted_schema[name] = field.type;
	}
	data_exporter::exportDataTableSQL(headers, rows_to_export,
		selected_file.empty() ? "unknown_table" : selected_file,
		&converted_schema, create_table);
}

static void export_dbc() {
	if (selected_file.empty() || selected_file_path.empty()) {
		core::setToast("info", "No DBC file selected to export.");
		return;
	}

	mpq::MPQInstall* mpq = core::view->mpq.get();
	data_exporter::exportRawDBC(selected_file, selected_file_path, mpq);
}

void export_data() {
	const std::string format = core::view->config.value("exportDataFormat", std::string("CSV"));

	if (format == "CSV")
		export_csv();
	else if (format == "SQL")
		export_sql();
	else if (format == "DBC")
		export_dbc();
}

} // namespace legacy_tab_data
