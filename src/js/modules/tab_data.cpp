/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_data.h"
#include "../log.h"
#include "../core.h"
#include <thread>
#include "../../app.h"
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

static std::optional<std::string> build_stack_trace(const char* function_name, const std::exception& e) {
	return std::format("{}: {}", function_name, e.what());
}

// --- File-local state ---

static std::string selected_file;

static std::optional<int> selected_file_data_id;

static const std::map<std::string, db::SchemaField>* selected_file_schema = nullptr;

static std::string active_table;

static listbox::ListboxState listbox_db2_state;
static data_table::DataTableState data_table_state;
static context_menu::ContextMenuState data_table_ctx_state;
static menu_button::MenuButtonState menu_button_data_state;

// Cached items string vector — only rebuilt when the source JSON changes.
static std::vector<std::string> s_items_cache;
static size_t s_items_cache_size = ~size_t(0);

// --- Forward declarations ---
static void copy_rows_csv();
static void copy_rows_sql();
static void copy_cell(const nlohmann::json& value);

// --- Internal functions ---

static void initialize_available_tables() {
	auto& view = *core::view;
	auto& manifest = view.dbdManifest;
	if (!manifest.empty())
		return;

	casc::dbd_manifest::prepareManifest();
	const auto table_names = casc::dbd_manifest::getAllTableNames();
	for (const auto& name : table_names)
		manifest.push_back(name);
	logging::write("initialized available db2 tables from dbd manifest");
}

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

struct ParseTableResult {
	std::vector<std::string> headers;
	std::vector<std::vector<std::string>> rows;
	const std::map<std::string, db::SchemaField>* schema = nullptr;
};

static ParseTableResult parse_table(const std::string& table_name) {
	auto& db2_reader = casc::db2::getTable(table_name);
	if (!db2_reader.isLoaded)
		db2_reader.parse();

	const auto& schema = db2_reader.schema;
	std::vector<std::string> all_headers;
	// Use schemaOrder to preserve insertion order (matches JS Map iteration order).
	for (const auto& key : db2_reader.schemaOrder)
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

static void load_table(const std::string& table_name) {
	auto& view = *core::view;

	try {
		selected_file_data_id = casc::dbd_manifest::getByTableName(table_name);

		const auto result = parse_table(table_name);

		view.tableBrowserHeaders.clear();
		for (const auto& h : result.headers)
			view.tableBrowserHeaders.push_back(h);

		view.selectionDataTable.clear();

		if (result.rows.empty())
			core::setToast("info", "Selected DB2 has no rows.");
		else
			core::hideToast(false);

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
		// JS: core.setToast('error', ..., { 'View Log': () => log.openRuntimeLog() }, -1)
		core::setToast("error", "Unable to open DB2 file " + table_name, casc::ExportHelper::TOAST_OPT_LOG, -1);
		logging::write(std::format("Failed to open CASC file: {}", e.what()));
	}
}

// --- Public API ---

void registerTab() {
	modules::register_nav_button("tab_data", "Data", "database.svg", install_type::CASC);
}

void mounted() {
	if (core::view->dbdManifest.empty()) {
		std::thread([]() {
			core::showLoadingScreen(1);
			core::progressLoadingScreen("Loading data table manifest...");

			casc::dbd_manifest::prepareManifest();
			auto table_names = casc::dbd_manifest::getAllTableNames();
			logging::write("initialized available db2 tables from dbd manifest");

			core::postToMainThread([names = std::move(table_names)]() {
				auto& view = *core::view;
				if (!view.dbdManifest.empty())
					return;
				for (const auto& name : names)
					view.dbdManifest.push_back(name);
			});

			core::hideLoadingScreen();
		}).detach();
	}

	// Change-detection is handled in render() by comparing against selected_file each frame.
}

void render() {
	auto& view = *core::view;

	// --- Change-detection for selection (equivalent to $watch('selectionDB2s') in JS) ---
	// JS: if (!core.view.isBusy && last && selected_file !== last)
	// selected_file is only updated inside load_table() on success, so failed loads can be retried.
	if (!view.selectionDB2s.empty()) {
		const std::string last = view.selectionDB2s.back().get<std::string>();
		if (view.isBusy == 0 && !last.empty() && last != selected_file) {
			load_table(last);
			active_table = selected_file;
		}
	}

	// --- Template rendering ---

	if (app::layout::BeginTab("tab-data")) {

	auto regions = app::layout::CalcListTabRegions(false, 1.0f / 7.0f);

	// --- Left panel: List container (row 1, col 1) ---
	//     <Listbox v-model:selection="selectionDB2s" :items="dbdManifest" ...>
	// </div>
	if (app::layout::BeginListContainer("db2-list-container", regions)) {
		const auto& items_str = core::cached_json_strings(view.dbdManifest, s_items_cache, s_items_cache_size);

		std::vector<std::string> selection_str;
		for (const auto& s : view.selectionDB2s)
			selection_str.push_back(s.get<std::string>());

		listbox::render(
			"listbox-db2",
			items_str,
			view.userInputFilterDB2s,
			selection_str,
			false,   // single
			true,    // keyinput
			view.config.value("regexFilters", false),
			listbox::CopyMode::Default,
			view.config.value("pasteSelection", false),          // pasteselection (JS: config.pasteSelection)
			view.config.value("removePathSpacesCopy", false),    // copytrimwhitespace (JS: config.removePathSpacesCopy)
			"db2 file", // unittype
			nullptr, // overrideItems
			false,   // disable
			"db2",   // persistscrollkey
			{},      // quickfilters
			true,    // nocopy
			listbox_db2_state,
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
	if (app::layout::BeginStatusBar("db2-status", regions)) {
		listbox::renderStatusBar("table", {}, listbox_db2_state);
	}
	app::layout::EndStatusBar();

	// --- Filter bar (row 2, col 1) ---
	if (app::layout::BeginFilterBar("db2-filter", regions)) {
		// <div class="regex-info" v-if="config.regexFilters" :title="regexTooltip">Regex Enabled</div>
		if (view.config.value("regexFilters", false)) {
			ImGui::TextUnformatted("Regex Enabled");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", view.regexTooltip.c_str());
			ImGui::SameLine();
		}
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char filter_db2_buf[256] = {};
		std::strncpy(filter_db2_buf, view.userInputFilterDB2s.c_str(), sizeof(filter_db2_buf) - 1);
		if (ImGui::InputTextWithHint("##FilterDB2s", "Filter DB2s..", filter_db2_buf, sizeof(filter_db2_buf)))
			view.userInputFilterDB2s = filter_db2_buf;
	}
	app::layout::EndFilterBar();

	// --- Right panel: Preview container (row 1, col 2) ---
	//     <DataTable ref="dataTable" :headers="tableBrowserHeaders" :rows="tableBrowserRows" ...>
	//     <ContextMenu :node="contextMenus.nodeDataTable" ...>
	// </div>
	if (app::layout::BeginPreviewContainer("data-table-container", regions)) {
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
				// Open the popup immediately on right-click (JS: ContextMenu opens on @contextmenu).
				ImGui::OpenPopup("##DataTableContextMenu");
			},
			[&]() { copy_rows_csv(); },
			nullptr);

		// Context menu for data table.
		// JS: <ContextMenu :node="contextMenus.nodeDataTable" @close="contextMenus.nodeDataTable = null">
		//       <span @click.self="copy_rows_csv">Copy {{ selectedCount }} row{{ ... }} as CSV</span>
		//       <span @click.self="copy_rows_sql">Copy {{ selectedCount }} row{{ ... }} as SQL</span>
		//       <span @click.self="copy_cell(cellValue)">Copy cell contents</span>
		if (ImGui::BeginPopup("##DataTableContextMenu")) {
			if (!view.contextMenus.nodeDataTable.is_null()) {
				const int selected_count = view.contextMenus.nodeDataTable.value("selectedCount", 0);
				const std::string csv_label = std::format("Copy {} row{} as CSV", selected_count, selected_count != 1 ? "s" : "");
				const std::string sql_label = std::format("Copy {} row{} as SQL", selected_count, selected_count != 1 ? "s" : "");
				if (ImGui::MenuItem(csv_label.c_str()))
					copy_rows_csv();
				if (ImGui::MenuItem(sql_label.c_str()))
					copy_rows_sql();
				if (view.contextMenus.nodeDataTable.contains("cellValue")) {
					if (ImGui::MenuItem("Copy cell contents"))
						copy_cell(view.contextMenus.nodeDataTable["cellValue"]);
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
	if (app::layout::BeginPreviewControls("data-preview-controls", regions)) {
		// <div class="regex-info" v-if="config.regexFilters" :title="regexTooltip">Regex Enabled</div>
		if (view.config.value("regexFilters", false)) {
			ImGui::TextUnformatted("Regex Enabled");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", view.regexTooltip.c_str());
			ImGui::SameLine();
		}
		char filter_data_buf[256] = {};
		std::strncpy(filter_data_buf, view.userInputFilterDataTable.c_str(), sizeof(filter_data_buf) - 1);
		if (ImGui::InputTextWithHint("##FilterDataTable", "Filter data table rows...", filter_data_buf, sizeof(filter_data_buf)))
			view.userInputFilterDataTable = filter_data_buf;

		ImGui::SameLine();

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
	app::layout::EndPreviewControls();

	}
	app::layout::EndTab();
}

// --- Helpers ---

static std::vector<std::vector<std::string>> jsonRowsToStrings(const nlohmann::json& rows_json) {
	std::vector<std::vector<std::string>> result;
	result.reserve(rows_json.size());
	for (const auto& row : rows_json) {
		std::vector<std::string> str_row;
		str_row.reserve(row.size());
		for (const auto& val : row)
			str_row.push_back(val.is_string() ? val.get<std::string>() : val.dump());
		result.push_back(std::move(str_row));
	}
	return result;
}

static std::vector<std::vector<std::string>> getDisplayRows(
		const std::vector<std::string>& headers) {
	const auto& view = *core::view;
	return data_table::getFilteredSortedRows(
		jsonRowsToStrings(view.tableBrowserRows), headers,
		view.userInputFilterDataTable,
		view.config.value("regexFilters", false),
		data_table_state);
}

// --- Export methods ---

static void copy_rows_csv() {
	const auto& view = *core::view;
	const size_t count = view.selectionDataTable.size();
	if (count == 0)
		return;

	std::vector<std::string> headers;
	for (const auto& h : view.tableBrowserHeaders)
		headers.push_back(h.get<std::string>());

	const auto sorted_rows = getDisplayRows(headers);

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

	std::vector<std::string> headers;
	for (const auto& h : view.tableBrowserHeaders)
		headers.push_back(h.get<std::string>());

	const auto sorted_rows = getDisplayRows(headers);

	std::vector<int> sel_indices;
	for (const auto& s : view.selectionDataTable)
		sel_indices.push_back(s.get<int>());

	std::string sql = data_table::getSelectedRowsAsSQL(headers, sorted_rows, sel_indices,
		selected_file.empty() ? "unknown_table" : selected_file);

	ImGui::SetClipboardText(sql.c_str());
	core::setToast("success", std::format("Copied {} row{} as SQL to the clipboard", count, count != 1 ? "s" : ""), {}, 2000);
}

static void copy_cell(const nlohmann::json& value) {
	if (value.is_null())
		return;

	// JS: nw.Clipboard.get().set(String(value), 'text')
	// String(value) for strings produces unquoted output — use get<std::string>() for strings,
	// dump() for all other types (numbers, arrays, objects, booleans).
	const std::string text = value.is_string() ? value.get<std::string>() : value.dump();
	ImGui::SetClipboardText(text.c_str());
}

static void export_csv() {
	auto& view = *core::view;
	const auto& user_selection = view.selectionDB2s;
	if (user_selection.empty()) {
		core::setToast("info", "You didn't select any tables to export.");
		return;
	}

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
			rows_to_export = jsonRowsToStrings(all_rows_json);
		} else {
			if (selection.empty()) {
				core::setToast("info", "No rows selected. Please select some rows first or enable \"Export all rows\".");
				return;
			}

			const auto sorted_rows = getDisplayRows(headers);
			for (const auto& row_index_json : selection) {
				const int row_index = row_index_json.get<int>();
				if (row_index >= 0 && row_index < static_cast<int>(sorted_rows.size()))
					rows_to_export.push_back(sorted_rows[static_cast<size_t>(row_index)]);
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
			helper.mark(table_name + ".csv", false, e.what(), build_stack_trace("export_csv", e));
			logging::write(std::format("Failed to export table {}: {}", table_name, e.what()));
		}
	}

	export_paths.close();
	helper.finish();
}

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
			rows_to_export = jsonRowsToStrings(all_rows_json);
		} else {
			if (selection.empty()) {
				core::setToast("info", "No rows selected. Please select some rows first or enable \"Export all rows\".");
				return;
			}

			const auto sorted_rows = getDisplayRows(headers);
			for (const auto& row_index_json : selection) {
				const int row_index = row_index_json.get<int>();
				if (row_index >= 0 && row_index < static_cast<int>(sorted_rows.size()))
					rows_to_export.push_back(sorted_rows[static_cast<size_t>(row_index)]);
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
			helper.mark(table_name + ".sql", false, e.what(), build_stack_trace("export_sql", e));
			logging::write(std::format("Failed to export table {}: {}", table_name, e.what()));
		}
	}

	export_paths.close();
	helper.finish();
}

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

			data_exporter::exportRawDB2(table_name, static_cast<uint32_t>(*file_data_id_opt),
				nullptr, &helper, &export_paths);
		} catch (const std::exception& e) {
			helper.mark(table_name + ".db2", false, e.what(), build_stack_trace("export_db2", e));
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
