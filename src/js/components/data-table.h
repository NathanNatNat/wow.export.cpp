/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

/**
 * Data table component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['headers', 'rows', 'filter', 'regex',
 * 'selection', 'copyheader', 'tablename'],
 * emits: ['update:selection', 'contextmenu', 'copy'].
 *
 * Renders a scrollable, sortable, filterable table with column resizing,
 * virtual scrolling (vertical + horizontal), and multi-select support.
 */
namespace data_table {

/**
 * Sort direction for a column.
 */
enum class SortDirection {
	Off,
	Asc,
	Desc
};

/**
 * Context menu event data emitted by the table.
 * Equivalent to the JS object emitted via $emit('contextmenu', { ... }).
 * Includes the mouse event position data so consumers can position the context menu.
 */
struct ContextMenuEvent {
	int rowIndex = -1;
	int columnIndex = -1;
	std::string cellValue;
	int selectedCount = 0;
	float mouseX = 0.0f;  // Mouse X position (from the event object in JS)
	float mouseY = 0.0f;  // Mouse Y position (from the event object in JS)
};

/**
 * Persistent state for a single DataTable widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct DataTableState {
	float scroll = 0.0f;
	float scrollRel = 0.0f;
	bool isScrolling = false;
	float horizontalScroll = 0.0f;
	float horizontalScrollRel = 0.0f;
	bool isHorizontalScrolling = false;
	int slotCount = 1;
	int lastSelectItem = -1;  // -1 = null equivalent
	std::vector<float> columnWidths;
	std::unordered_map<std::string, float> manuallyResizedColumns;
	bool isResizing = false;
	int resizeColumnIndex = -1;
	float resizeStartX = 0.0f;
	float resizeStartWidth = 0.0f;
	bool isOverResizeZone = false;
	int resizeZoneColumnIndex = -1;
	int sortColumn = -1;
	SortDirection sortDirection = SortDirection::Off;
	float targetHorizontalScroll = 0.0f;
	float targetColumnWidth = 0.0f;
	int forceHorizontalUpdate = 0;

	// Mouse drag tracking (equivalent to JS instance vars set in startMouse/startHorizontalMouse).
	float scrollStartY = 0.0f;
	float scrollStart = 0.0f;
	float horizontalScrollStartX = 0.0f;
	float horizontalScrollStart = 0.0f;

	// Change-detection for watchers.
	std::vector<std::string> prevHeaders;
	size_t prevRowCount = 0;
	const void* prevRowsPtr = nullptr;
	// Content-based change detection: version counter incremented externally
	// when rows are mutated in-place. This catches changes that don't alter
	// size or base pointer (e.g., editing cell content).
	size_t rowsVersion = 0;
	size_t prevRowsVersion = 0;

	// Cached sorted/filtered items (recomputed each frame).
	// These are indices into the original rows array for filtered items,
	// or row data for sorted items.
};

/**
 * Render a data table using ImGui.
 *
 * @param id             Unique ImGui ID string for this widget instance.
 * @param headers        Column header names.
 * @param rows           2D array of row data (each row is a vector of string cell values).
 * @param filter         Current filter string (empty = no filter).
 * @param regex          Whether to use regex matching for the filter.
 * @param selection      Currently selected row indices (into sortedItems).
 * @param copyheader     Whether to include headers when copying as CSV.
 * @param tablename      Table name used for SQL export.
 * @param state          Persistent state across frames.
 * @param onSelectionChanged  Callback when selection changes; receives new selection vector.
 * @param onContextMenu       Callback for right-click context menu; receives ContextMenuEvent.
 * @param onCopy              Callback for Ctrl+C copy action.
 * @param onFilterChanged     Callback when filter is modified (e.g. column filter icon click).
 */
void render(const char* id,
            const std::vector<std::string>& headers,
            const std::vector<std::vector<std::string>>& rows,
            const std::string& filter,
            bool regex,
            const std::vector<int>& selection,
            bool copyheader,
            const std::string& tablename,
            DataTableState& state,
            const std::function<void(const std::vector<int>&)>& onSelectionChanged,
            const std::function<void(const ContextMenuEvent&)>& onContextMenu,
            const std::function<void()>& onCopy,
            const std::function<void(const std::string&)>& onFilterChanged);

/**
 * Get selected rows as CSV string.
 * @param headers      Column header names.
 * @param sortedItems  The sorted rows data.
 * @param selection    Currently selected row indices.
 * @param copyheader   Whether to include headers in the CSV.
 * @returns CSV formatted string.
 */
std::string getSelectedRowsAsCSV(const std::vector<std::string>& headers,
                                  const std::vector<std::vector<std::string>>& sortedItems,
                                  const std::vector<int>& selection,
                                  bool copyheader);

/**
 * Get selected rows as SQL INSERT statements.
 * @param headers      Column header names.
 * @param sortedItems  The sorted rows data.
 * @param selection    Currently selected row indices.
 * @param tablename    Table name for the INSERT statements.
 * @returns SQL formatted string.
 */
std::string getSelectedRowsAsSQL(const std::vector<std::string>& headers,
                                  const std::vector<std::vector<std::string>>& sortedItems,
                                  const std::vector<int>& selection,
                                  const std::string& tablename);

/**
 * Apply filter and sort to a rows array using the given DataTableState.
 * Returns the filtered+sorted rows in the same order the table displays them.
 * @param rows     Full unfiltered rows array.
 * @param headers  Column header names (used for column-filter parsing).
 * @param filter   Current filter string.
 * @param regex    Whether to use regex matching.
 * @param state    DataTableState providing the current sort column/direction.
 * @returns Filtered and sorted rows.
 */
std::vector<std::vector<std::string>> getFilteredSortedRows(
        const std::vector<std::vector<std::string>>& rows,
        const std::vector<std::string>& headers,
        const std::string& filter,
        bool regex,
        const DataTableState& state);

} // namespace data_table
