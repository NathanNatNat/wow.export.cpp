/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#include "data-table.h"

#include <imgui.h>
#include "../../app.h"
#include <cmath>
#include <algorithm>
#include <regex>
#include <cctype>

namespace data_table {

// ─── props ──────────────────────────────────────────────────────────
// props: ['headers', 'rows', 'filter', 'regex', 'selection', 'copyheader', 'tablename']
// emits: ['update:selection', 'contextmenu', 'copy']

// ─── data ───────────────────────────────────────────────────────────
// Reactive instance data — stored in DataTableState.

/**
 * selectedOption: An array of strings denoting options shown in the menu.
 */

// ─── mounted / beforeUnmount ────────────────────────────────────────

/**
 * Invoked when the component is mounted.
 * Used to register global listeners and resize observer.
 */
// TODO(conversion): In ImGui, global mouse listeners, ResizeObserver, and keydown listeners
// are not needed. ImGui provides mouse/keyboard state via ImGui::GetIO() each frame.
// Resize is handled by layout recalculation every frame.

/**
 * Invoked when the component is destroyed.
 * Used to unregister global mouse listeners and resize observer.
 */
// TODO(conversion): No explicit unmount needed in ImGui immediate mode.
// Animation frame IDs (horizontalScrollAnimationId, resizeAnimationId) are not needed
// since ImGui redraws every frame.

// ─── computed properties ────────────────────────────────────────────

/**
 * Offset of the scroll widget in pixels.
 * Between 0 and the height of the component.
 */
static float scrollOffset(const DataTableState& state) {
	return state.scroll;
}

/**
 * Index which array reading should start at, based on the current
 * relative scroll and the overall item count. Value is dynamically
 * capped based on slot count to prevent empty slots appearing.
 */
static int scrollIndex(int sortedItemCount, const DataTableState& state) {
	return static_cast<int>(std::round((static_cast<float>(sortedItemCount) - static_cast<float>(state.slotCount)) * state.scrollRel));
}

/**
 * Parse filter input to extract column-specific filters and general filter.
 * @param filterInput - The filter input string
 * @param headers - Column header names
 * @returns Pair of (columnFilters map, generalFilter string)
 */
static std::pair<std::unordered_map<int, std::string>, std::string> parseFilterInput(
		const std::string& filterInput, const std::vector<std::string>& headers) {
	std::unordered_map<int, std::string> columnFilters;
	std::string generalFilter;

	if (filterInput.empty())
		return { columnFilters, generalFilter };

	// Split by spaces, but preserve quoted strings
	std::regex partRegex(R"((?:[^\s"]+|"[^"]*")+)");
	auto partsBegin = std::sregex_iterator(filterInput.begin(), filterInput.end(), partRegex);
	auto partsEnd = std::sregex_iterator();

	for (auto it = partsBegin; it != partsEnd; ++it) {
		std::string part = (*it).str();
		auto colonIndex = part.find(':');

		if (colonIndex != std::string::npos && colonIndex > 0 && colonIndex < part.length() - 1) {
			// This looks like a column filter (column:value)
			std::string columnName = part.substr(0, colonIndex);
			std::string filterValue = part.substr(colonIndex + 1);

			// Convert column name to lowercase for comparison
			std::string columnNameLower = columnName;
			std::transform(columnNameLower.begin(), columnNameLower.end(), columnNameLower.begin(),
			               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			int headerIndex = -1;
			for (int i = 0; i < static_cast<int>(headers.size()); ++i) {
				std::string headerLower = headers[static_cast<size_t>(i)];
				std::transform(headerLower.begin(), headerLower.end(), headerLower.begin(),
				               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				if (headerLower == columnNameLower) {
					headerIndex = i;
					break;
				}
			}

			if (headerIndex != -1) {
				columnFilters[headerIndex] = filterValue;
				continue;
			}
		}

		// Not a valid column filter, add to general filter
		if (!generalFilter.empty())
			generalFilter += ' ';

		generalFilter += part;
	}

	// Trim general filter
	while (!generalFilter.empty() && generalFilter.back() == ' ')
		generalFilter.pop_back();
	while (!generalFilter.empty() && generalFilter.front() == ' ')
		generalFilter.erase(generalFilter.begin());

	return { columnFilters, generalFilter };
}

/**
 * Helper: convert a string to lowercase.
 */
static std::string toLower(const std::string& s) {
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(),
	               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

/**
 * Check if a row matches the given column filters.
 * @param row - The row data
 * @param columnFilters - Column-specific filters
 * @param useRegex - Whether to use regex matching
 * @returns True if row matches all column filters
 */
static bool matchesColumnFilters(const std::vector<std::string>& row,
                                  const std::unordered_map<int, std::string>& columnFilters,
                                  bool useRegex) {
	for (const auto& [columnIndex, filterValue] : columnFilters) {
		if (columnIndex < 0 || columnIndex >= static_cast<int>(row.size()))
			return false;

		const std::string& cellValue = row[static_cast<size_t>(columnIndex)];

		if (useRegex) {
			try {
				std::regex filter(filterValue, std::regex::icase);
				if (!std::regex_search(cellValue, filter)) {
					return false;
				}
			} catch (const std::regex_error&) {
				// Invalid regex, fall back to string matching
				if (toLower(cellValue).find(toLower(filterValue)) == std::string::npos) {
					return false;
				}
			}
		} else {
			if (toLower(cellValue).find(toLower(filterValue)) == std::string::npos) {
				return false;
			}
		}
	}
	return true;
}

/**
 * Check if a row matches the general filter.
 * @param row - The row data
 * @param generalFilter - General filter string
 * @param useRegex - Whether to use regex matching
 * @returns True if row matches general filter
 */
static bool matchesGeneralFilter(const std::vector<std::string>& row,
                                  const std::string& generalFilter,
                                  bool useRegex) {
	if (generalFilter.empty()) return true;

	if (useRegex) {
		try {
			std::regex filter(generalFilter, std::regex::icase);
			for (const auto& field : row) {
				if (std::regex_search(field, filter))
					return true;
			}
			return false;
		} catch (const std::regex_error&) {
			// Invalid regex, fall back to string matching
			std::string filterLower = toLower(generalFilter);
			for (const auto& field : row) {
				if (toLower(field).find(filterLower) != std::string::npos)
					return true;
			}
			return false;
		}
	} else {
		std::string filterLower = toLower(generalFilter);
		for (const auto& field : row) {
			if (toLower(field).find(filterLower) != std::string::npos)
				return true;
		}
		return false;
	}
}

/**
 * Reactively filtered version of the underlying data array.
 * Automatically refilters when the filter input is changed.
 * Supports both column-specific filters (e.g., "id:5000 name:test") and general filters.
 */
static std::vector<std::vector<std::string>> filteredItems(
		const std::vector<std::vector<std::string>>& rows,
		const std::string& filter,
		bool useRegex,
		const std::vector<std::string>& headers,
		const std::vector<int>& selection,
		const std::function<void(const std::vector<int>&)>& onSelectionChanged) {
	// Skip filtering if no filter is set.
	if (filter.empty())
		return rows;

	std::string trimmedFilter = filter;
	while (!trimmedFilter.empty() && trimmedFilter.back() == ' ')
		trimmedFilter.pop_back();
	while (!trimmedFilter.empty() && trimmedFilter.front() == ' ')
		trimmedFilter.erase(trimmedFilter.begin());

	auto [columnFilters, generalFilter] = parseFilterInput(trimmedFilter, headers);
	if (columnFilters.empty() && generalFilter.empty())
		return rows;

	std::vector<std::vector<std::string>> res;
	for (const auto& row : rows) {
		bool passesColumnFilters = matchesColumnFilters(row, columnFilters, useRegex);
		bool passesGeneralFilter = matchesGeneralFilter(row, generalFilter, useRegex);

		if (passesColumnFilters && passesGeneralFilter)
			res.push_back(row);
	}

	// Remove anything from the user selection that has now been filtered out.
	// Iterate backwards here due to re-indexing as elements are spliced.
	bool hasChanges = false;
	std::vector<int> newSelection;
	for (const auto& rowIndex : selection) {
		if (rowIndex < static_cast<int>(res.size())) {
			newSelection.push_back(rowIndex);
		} else {
			hasChanges = true;
		}
	}

	if (hasChanges && onSelectionChanged)
		onSelectionChanged(newSelection);

	return res;
}

/**
 * Helper: try to parse a string as a double. Returns success and the value.
 */
static bool tryParseNumber(const std::string& s, double& out) {
	if (s.empty()) return false;
	try {
		size_t pos = 0;
		out = std::stod(s, &pos);
		// Ensure the entire string was consumed (like JS !isNaN(Number(val)))
		return pos == s.size();
	} catch (...) {
		return false;
	}
}

/**
 * Sorted version of the filtered data array.
 * Applies sorting based on sortColumn and sortDirection.
 */
static std::vector<std::vector<std::string>> sortedItems(
		const std::vector<std::vector<std::string>>& filtered,
		const DataTableState& state) {
	if (state.sortColumn == -1 || state.sortDirection == SortDirection::Off)
		return filtered;

	std::vector<std::vector<std::string>> sorted = filtered;
	const int columnIndex = state.sortColumn;
	const bool ascending = (state.sortDirection == SortDirection::Asc);

	std::sort(sorted.begin(), sorted.end(), [columnIndex, ascending](const std::vector<std::string>& a, const std::vector<std::string>& b) {
		// Handle null/undefined values (empty strings as null equivalent)
		const bool aEmpty = (columnIndex >= static_cast<int>(a.size()));
		const bool bEmpty = (columnIndex >= static_cast<int>(b.size()));
		const std::string& aVal = aEmpty ? std::string() : a[static_cast<size_t>(columnIndex)];
		const std::string& bVal = bEmpty ? std::string() : b[static_cast<size_t>(columnIndex)];

		// Handle null/undefined values
		if (aEmpty && bEmpty) return false;
		if (aEmpty) return ascending;
		if (bEmpty) return !ascending;

		// Numeric comparison
		double aNum = 0.0, bNum = 0.0;
		bool aIsNum = tryParseNumber(aVal, aNum);
		bool bIsNum = tryParseNumber(bVal, bNum);

		if (aIsNum && bIsNum) {
			if (aNum == bNum) return false;
			return ascending ? (aNum < bNum) : (aNum > bNum);
		}

		// String comparison
		std::string aStr = toLower(aVal);
		std::string bStr = toLower(bVal);
		int cmp = aStr.compare(bStr);
		if (cmp == 0) return false;
		return ascending ? (cmp < 0) : (cmp > 0);
	});

	return sorted;
}

/**
 * Weight (0-1) of a single item.
 */
static float itemWeight(int sortedItemCount) {
	if (sortedItemCount <= 0)
		return 0.0f;
	return 1.0f / static_cast<float>(sortedItemCount);
}

/**
 * Calculate column widths based on header text length ONLY.
 * No DOM measurements. No dynamic shit. Just text length.
 */
static void calculateColumnWidths(const std::vector<std::string>& headers, DataTableState& state) {
	if (headers.empty()) return;

	std::vector<float> widths;

	for (size_t index = 0; index < headers.size(); ++index) {
		const std::string& header = headers[index];
		const std::string& columnName = header;

		auto it = state.manuallyResizedColumns.find(columnName);
		if (it != state.manuallyResizedColumns.end()) {
			widths.push_back(it->second);
		} else {
			// Calculate width based on text length: 8px per character + 40px for icons/padding
			float textWidth = static_cast<float>(header.length()) * 8.0f + 40.0f;
			widths.push_back(std::max(120.0f, textWidth));
		}
	}

	state.columnWidths = widths;
}

/**
 * Reset horizontal scroll position and force recalculation.
 * Called when new table data is loaded.
 */
static void resetHorizontalScroll(DataTableState& state) {
	state.horizontalScroll = 0.0f;
	state.horizontalScrollRel = 0.0f;

	// In JS: this.$refs.table.offsetHeight forces a re-evaluation.
	// TODO(conversion): In ImGui, we increment forceHorizontalUpdate to trigger recalculation.
	state.forceHorizontalUpdate++;
}

/**
 * Invoked by a ResizeObserver when the main component node
 * is resized due to layout changes.
 */
static void resize(float containerHeight, float headerHeight, float scrollerHeight,
                    float containerWidth, float hScrollerWidth,
                    DataTableState& state) {
	// Calculate available height for scrolling (subtract header and scrollbar widget)
	const float availableHeight = containerHeight - headerHeight;
	state.scroll = (availableHeight - scrollerHeight) * state.scrollRel;
	state.slotCount = std::max(1, static_cast<int>(std::floor(availableHeight / 32.0f)) - 2);

	state.horizontalScroll = (containerWidth - hScrollerWidth) * state.horizontalScrollRel;
}

/**
 * Restricts the scroll offset to prevent overflowing and
 * calculates the relative (0-1) offset based on the scroll.
 */
static void recalculateBounds(float containerHeight, float headerHeight, float scrollerHeight,
                               DataTableState& state) {
	const float availableHeight = containerHeight - headerHeight;
	const float maxVal = availableHeight - scrollerHeight;
	state.scroll = std::min(maxVal, std::max(0.0f, state.scroll));
	state.scrollRel = (maxVal > 0.0f) ? (state.scroll / maxVal) : 0.0f;
}

/**
 * Restricts the horizontal scroll offset to prevent overflowing and
 * calculates the relative (0-1) offset based on the horizontal scroll.
 */
static void recalculateHorizontalBounds(float containerWidth, float hScrollerWidth,
                                          DataTableState& state) {
	const float maxVal = containerWidth - hScrollerWidth;
	state.horizontalScroll = std::min(maxVal, std::max(0.0f, state.horizontalScroll));
	state.horizontalScrollRel = (maxVal > 0.0f) ? (state.horizontalScroll / maxVal) : 0.0f;
}

/**
 * Determines if horizontal scrolling is needed based on table width vs container width.
 */
static bool needsHorizontalScrolling(float tableWidth, float containerWidth) {
	return tableWidth > containerWidth;
}

/**
 * Sync custom scrollbar position with native scroll position
 */
// TODO(conversion): In ImGui we don't have native scrollbar sync; horizontal scroll
// is fully managed by our custom scrollbar logic.

/**
 * Invoked when a mouse-down event is captured on the scroll widget.
 * @param {MouseEvent} e 
 */
static void startMouse(float mouseY, DataTableState& state) {
	state.scrollStartY = mouseY;
	state.scrollStart = state.scroll;
	state.isScrolling = true;
}

/**
 * Invoked when a mouse-move event is captured globally.
 * @param {MouseEvent} e 
 */
static void moveMouse(float mouseX, float mouseY,
                       float containerHeight, float headerHeight, float scrollerHeight,
                       float containerWidth, float hScrollerWidth,
                       DataTableState& state) {
	if (state.isScrolling) {
		state.scroll = state.scrollStart + (mouseY - state.scrollStartY);
		recalculateBounds(containerHeight, headerHeight, scrollerHeight, state);
	}

	if (state.isHorizontalScrolling) {
		// In JS, this used requestAnimationFrame batching. In ImGui we update directly each frame.
		state.horizontalScroll = state.horizontalScrollStart + (mouseX - state.horizontalScrollStartX);
		recalculateHorizontalBounds(containerWidth, hScrollerWidth, state);
	}

	if (state.isResizing) {
		const float deltaX = mouseX - state.resizeStartX;
		state.targetColumnWidth = std::max(50.0f, state.resizeStartWidth + deltaX); // Minimum width of 50px

		// Update the column width
		if (!state.columnWidths.empty() && state.resizeColumnIndex >= 0 &&
		    state.resizeColumnIndex < static_cast<int>(state.columnWidths.size())) {
			state.columnWidths[static_cast<size_t>(state.resizeColumnIndex)] = state.targetColumnWidth;
		}
	}
}

/**
 * Invoked when a mouse-up event is captured globally.
 */
static void stopMouse(DataTableState& state, const std::vector<std::string>& headers) {
	state.isScrolling = false;
	state.isHorizontalScrolling = false;

	// In JS: cancelAnimationFrame(this.horizontalScrollAnimationId)
	// TODO(conversion): No animation frame batching needed in ImGui; updates happen immediately.

	if (state.isResizing) {
		// Finalize column width and mark as manually resized
		if (state.targetColumnWidth != 0.0f && !state.columnWidths.empty() &&
		    state.resizeColumnIndex >= 0 && state.resizeColumnIndex < static_cast<int>(state.columnWidths.size())) {
			state.columnWidths[static_cast<size_t>(state.resizeColumnIndex)] = state.targetColumnWidth;
			if (state.resizeColumnIndex < static_cast<int>(headers.size())) {
				const std::string& columnName = headers[static_cast<size_t>(state.resizeColumnIndex)];
				state.manuallyResizedColumns[columnName] = state.targetColumnWidth;
			}
		}

		state.isResizing = false;
		state.resizeColumnIndex = -1;
		state.isOverResizeZone = false;
		state.resizeZoneColumnIndex = -1;
	}
}

/**
 * Invoked when a mouse-down event is captured on the horizontal scroll widget.
 * @param {MouseEvent} e 
 */
static void startHorizontalMouse(float mouseX, DataTableState& state) {
	state.horizontalScrollStartX = mouseX;
	state.horizontalScrollStart = state.horizontalScroll;
	state.isHorizontalScrolling = true;
}

/**
 * Handle column header clicks for sorting.
 * @param {number} columnIndex - Index of the clicked column
 */
static void toggleSort(int columnIndex, DataTableState& state) {
	if (state.sortColumn == columnIndex) {
		// Same column - cycle through: off -> asc -> desc -> off
		if (state.sortDirection == SortDirection::Off) {
			state.sortDirection = SortDirection::Asc;
		} else if (state.sortDirection == SortDirection::Asc) {
			state.sortDirection = SortDirection::Desc;
		} else {
			state.sortDirection = SortDirection::Off;
			state.sortColumn = -1;
		}
	} else {
		// Different column - set to ascending
		state.sortColumn = columnIndex;
		state.sortDirection = SortDirection::Asc;
	}
}

/**
 * Get sort icon name for a given column.
 * @param {number} columnIndex - Index of the column
 * @returns {string} Sort icon class name
 */
static const char* getSortIconName(int columnIndex, const DataTableState& state) {
	if (state.sortColumn != columnIndex || state.sortDirection == SortDirection::Off)
		return "sort-icon-off";

	return (state.sortDirection == SortDirection::Asc) ? "sort-icon-up" : "sort-icon-down";
}

/**
 * Handle clicking the filter icon for a column.
 * Inserts the column filter prefix and focuses the filter input.
 * @param {number} columnIndex - Index of the column
 * @param {Event} e - The click event
 */
static void handleFilterIconClick(int columnIndex, const std::vector<std::string>& headers,
                                    const std::string& currentFilter,
                                    const std::function<void(const std::string&)>& onFilterChanged) {
	if (columnIndex < 0 || columnIndex >= static_cast<int>(headers.size()))
		return;

	std::string columnName = toLower(headers[static_cast<size_t>(columnIndex)]);
	std::string filterPrefix = columnName + ":";

	std::string newFilter;
	if (!currentFilter.empty())
		newFilter = currentFilter + " " + filterPrefix;
	else
		newFilter = filterPrefix;

	if (onFilterChanged)
		onFilterChanged(newFilter);

	// In JS: this.$nextTick(() => { filterInput.focus(); filterInput.setSelectionRange(...) });
	// TODO(conversion): In ImGui, keyboard focus management is handled differently.
	// The caller should set focus to the filter input if desired.
}

/**
 * Prevent middle mouse button from triggering autopan.
 * @param {MouseEvent} e
 */
// TODO(conversion): In ImGui, there is no native auto-pan behavior from middle mouse button.
// This is a no-op but preserved for completeness.

/**
 * Invoked when a user selects a row in the table.
 * @param {number} rowIndex - Index of the row in sortedItems
 * @param {MouseEvent} event
 */
static void selectRow(int rowIndex, bool ctrlKey, bool shiftKey,
                       const std::vector<int>& selection,
                       DataTableState& state,
                       const std::function<void(const std::vector<int>&)>& onSelectionChanged) {
	// Check if rowIndex is in the current selection
	auto checkIt = std::find(selection.begin(), selection.end(), rowIndex);
	const int checkIndex = (checkIt != selection.end()) ? static_cast<int>(std::distance(selection.begin(), checkIt)) : -1;
	std::vector<int> newSelection = selection;

	if (ctrlKey) {
		// Ctrl-key held, so allow multiple selections.
		if (checkIndex > -1)
			newSelection.erase(newSelection.begin() + checkIndex);
		else
			newSelection.push_back(rowIndex);
	} else if (shiftKey) {
		// Shift-key held, select a range.
		if (state.lastSelectItem != -1 && state.lastSelectItem != rowIndex) {
			const int lastSelectIndex = state.lastSelectItem;
			const int thisSelectIndex = rowIndex;

			const int delta = std::abs(lastSelectIndex - thisSelectIndex);
			const int lowest = std::min(lastSelectIndex, thisSelectIndex);
			const int highest = lowest + delta;

			for (int i = lowest; i <= highest; i++) {
				if (std::find(newSelection.begin(), newSelection.end(), i) == newSelection.end())
					newSelection.push_back(i);
			}
		}
	} else if (checkIndex == -1 || (checkIndex > -1 && static_cast<int>(newSelection.size()) > 1)) {
		// Normal click, replace entire selection.
		newSelection.clear();
		newSelection.push_back(rowIndex);
	}

	state.lastSelectItem = rowIndex;
	if (onSelectionChanged)
		onSelectionChanged(newSelection);
}

/**
 * Invoked when a user right-clicks on a row in the table.
 * @param {number} rowIndex - Index of the row in sortedItems
 * @param {number} columnIndex - Index of the column
 * @param {MouseEvent} event
 */
static void handleContextMenu(int rowIndex, int columnIndex,
                                const std::vector<std::vector<std::string>>& sorted,
                                const std::vector<int>& selection,
                                DataTableState& state,
                                const std::function<void(const std::vector<int>&)>& onSelectionChanged,
                                const std::function<void(const ContextMenuEvent&)>& onContextMenu) {
	// if the row is not already selected, select it
	if (std::find(selection.begin(), selection.end(), rowIndex) == selection.end()) {
		state.lastSelectItem = rowIndex;
		if (onSelectionChanged)
			onSelectionChanged({ rowIndex });
	}

	std::string cellValue;
	if (rowIndex >= 0 && rowIndex < static_cast<int>(sorted.size())) {
		const auto& row = sorted[static_cast<size_t>(rowIndex)];
		if (columnIndex >= 0 && columnIndex < static_cast<int>(row.size()))
			cellValue = row[static_cast<size_t>(columnIndex)];
	}

	if (onContextMenu) {
		ContextMenuEvent evt;
		evt.rowIndex = rowIndex;
		evt.columnIndex = columnIndex;
		evt.cellValue = cellValue;
		evt.selectedCount = std::max(1, static_cast<int>(selection.size()));
		onContextMenu(evt);
	}
}

/**
 * Invoked when a keydown event is fired.
 * @param {KeyboardEvent} e
 */
static void handleKey(const std::vector<std::vector<std::string>>& sorted,
                       const std::vector<int>& selection,
                       float containerHeight, float headerHeight, float scrollerHeight,
                       DataTableState& state,
                       const std::function<void(const std::vector<int>&)>& onSelectionChanged,
                       const std::function<void()>& onCopy) {
	// If any ImGui item is active (e.g. text input focused), don't intercept.
	if (ImGui::IsAnyItemActive())
		return;

	// User hasn't selected anything in the table yet.
	if (state.lastSelectItem == -1)
		return;

	const ImGuiIO& io = ImGui::GetIO();

	// CTRL+C to copy selection as CSV.
	if (ImGui::IsKeyPressed(ImGuiKey_C) && io.KeyCtrl) {
		if (onCopy)
			onCopy();
		return;
	}

	// Arrow keys.
	const bool isArrowUp = ImGui::IsKeyPressed(ImGuiKey_UpArrow);
	const bool isArrowDown = ImGui::IsKeyPressed(ImGuiKey_DownArrow);
	if (isArrowUp || isArrowDown) {
		const int delta = isArrowUp ? -1 : 1;
		const int sortedCount = static_cast<int>(sorted.size());

		// Move/expand selection one.
		const int lastSelectIndex = state.lastSelectItem;
		const int nextIndex = lastSelectIndex + delta;
		if (nextIndex >= 0 && nextIndex < sortedCount) {
			const int currScrollIndex = scrollIndex(sortedCount, state);
			const int lastViewIndex = isArrowUp ? currScrollIndex : currScrollIndex + state.slotCount;
			int diff = std::abs(nextIndex - lastViewIndex);
			if (isArrowDown)
				diff += 1;

			if ((isArrowUp && nextIndex < lastViewIndex) || (isArrowDown && nextIndex >= lastViewIndex)) {
				const float availableHeight = containerHeight - headerHeight;
				const float weight = availableHeight - scrollerHeight;
				state.scroll += ((static_cast<float>(diff) * itemWeight(sortedCount)) * weight) * static_cast<float>(delta);
				recalculateBounds(containerHeight, headerHeight, scrollerHeight, state);
			}

			std::vector<int> newSelection = selection;

			if (!io.KeyShift)
				newSelection.clear();

			newSelection.push_back(nextIndex);
			state.lastSelectItem = nextIndex;
			if (onSelectionChanged)
				onSelectionChanged(newSelection);
		}
	}
}

// ─── CSV / SQL export helpers ───────────────────────────────────────

/**
 * Helper: escape a value for CSV.
 */
static std::string escape_csv(const std::string& val) {
	if (val.find(',') != std::string::npos || val.find('"') != std::string::npos ||
	    val.find('\n') != std::string::npos || val.find('\r') != std::string::npos) {
		std::string escaped;
		escaped.reserve(val.size() + 4);
		escaped += '"';
		for (char c : val) {
			if (c == '"')
				escaped += "\"\"";
			else
				escaped += c;
		}
		escaped += '"';
		return escaped;
	}
	return val;
}

/**
 * Get selected rows as CSV string.
 * @returns {string} CSV formatted string
 */
std::string getSelectedRowsAsCSV(const std::vector<std::string>& headers,
                                  const std::vector<std::vector<std::string>>& sorted,
                                  const std::vector<int>& selection,
                                  bool copyheader) {
	if (selection.empty() || headers.empty())
		return "";

	// Sort selection indices
	std::vector<int> sortedSelection = selection;
	std::sort(sortedSelection.begin(), sortedSelection.end());

	std::vector<std::vector<std::string>> rows;
	for (int idx : sortedSelection) {
		if (idx >= 0 && idx < static_cast<int>(sorted.size()))
			rows.push_back(sorted[static_cast<size_t>(idx)]);
	}

	if (rows.empty())
		return "";

	std::string result;

	if (copyheader) {
		for (size_t i = 0; i < headers.size(); ++i) {
			if (i > 0) result += ',';
			result += escape_csv(headers[i]);
		}
		result += '\n';
	}

	for (const auto& row : rows) {
		for (size_t i = 0; i < row.size(); ++i) {
			if (i > 0) result += ',';
			result += escape_csv(row[i]);
		}
		result += '\n';
	}

	// Remove trailing newline
	if (!result.empty() && result.back() == '\n')
		result.pop_back();

	return result;
}

/**
 * Get selected rows as SQL INSERT statements.
 * @returns {string} SQL formatted string
 */
std::string getSelectedRowsAsSQL(const std::vector<std::string>& headers,
                                  const std::vector<std::vector<std::string>>& sorted,
                                  const std::vector<int>& selection,
                                  const std::string& tablename) {
	if (selection.empty() || headers.empty())
		return "";

	// Sort selection indices
	std::vector<int> sortedSelection = selection;
	std::sort(sortedSelection.begin(), sortedSelection.end());

	std::vector<std::vector<std::string>> rows;
	for (int idx : sortedSelection) {
		if (idx >= 0 && idx < static_cast<int>(sorted.size()))
			rows.push_back(sorted[static_cast<size_t>(idx)]);
	}

	if (rows.empty())
		return "";

	std::string table_name = tablename.empty() ? "unknown_table" : tablename;

	auto escape_identifier = [](const std::string& name) -> std::string {
		std::string result = "`";
		for (char c : name) {
			if (c == '`')
				result += "``";
			else
				result += c;
		}
		result += '`';
		return result;
	};

	auto escape_value = [](const std::string& val) -> std::string {
		if (val.empty())
			return "NULL";

		// Check if numeric
		double num = 0.0;
		if (tryParseNumber(val, num))
			return val;

		std::string result = "'";
		for (char c : val) {
			if (c == '\'')
				result += "''";
			else
				result += c;
		}
		result += '\'';
		return result;
	};

	std::string escaped_table = escape_identifier(table_name);
	std::string escaped_fields;
	for (size_t i = 0; i < headers.size(); ++i) {
		if (i > 0) escaped_fields += ", ";
		escaped_fields += escape_identifier(headers[i]);
	}

	std::string result;
	for (size_t r = 0; r < rows.size(); ++r) {
		const auto& row = rows[r];
		std::string values;
		for (size_t i = 0; i < row.size(); ++i) {
			if (i > 0) values += ", ";
			values += escape_value(row[i]);
		}
		if (r > 0) result += '\n';
		result += "INSERT INTO " + escaped_table + " (" + escaped_fields + ") VALUES (" + values + ");";
	}

	return result;
}

// ─── template (ImGui render) ────────────────────────────────────────

/**
 * HTML mark-up to render for this component.
 */
// template: converted to ImGui immediate-mode rendering below.

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
            const std::function<void(const std::string&)>& onFilterChanged) {
	ImGui::PushID(id);

	// ── Watch: headers ──────────────────────────────────────────────
	// Watch for header changes to recalculate column widths.
	if (state.prevHeaders != headers) {
		state.prevHeaders = headers;
		state.manuallyResizedColumns.clear();
		calculateColumnWidths(headers, state);
		resetHorizontalScroll(state);
	}

	// ── Watch: rows ─────────────────────────────────────────────────
	// Watch for rows changes to reset selection (new table loaded).
	if (state.prevRowCount != rows.size() || state.prevRowsPtr != static_cast<const void*>(rows.data())) {
		state.prevRowCount = rows.size();
		state.prevRowsPtr = static_cast<const void*>(rows.data());
		state.lastSelectItem = -1;
		if (onSelectionChanged)
			onSelectionChanged({});
		resetHorizontalScroll(state);
	}

	// ── Compute filtered and sorted items ───────────────────────────
	auto filtered = filteredItems(rows, filter, regex, headers, selection, onSelectionChanged);
	auto sorted = sortedItems(filtered, state);
	const int sortedCount = static_cast<int>(sorted.size());

	// ── Container dimensions ────────────────────────────────────────
	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const float containerWidth = availSize.x;
	const float containerHeight = availSize.y;

	// Row height matching CSS: min-height: 32px
	const float rowHeight = 32.0f;
	// Header height matching CSS: padding 10px top/bottom + ~20px text
	const float headerHeight = 40.0f;

	// Vertical scroller dimensions
	const float scrollerHeight = 45.0f; // CSS: .scroller height: 45px

	// Calculate total table width from column widths
	float tableWidth = 0.0f;
	for (float w : state.columnWidths)
		tableWidth += w;

	// Horizontal scroller dimensions
	const bool showHScroll = needsHorizontalScrolling(tableWidth, containerWidth);
	const float hScrollerWidth = showHScroll
		? std::max(45.0f, (containerWidth / tableWidth) * (containerWidth - 16.0f))
		: 45.0f;

	// ── Resize (equivalent of ResizeObserver callback) ──────────────
	resize(containerHeight, headerHeight, scrollerHeight, containerWidth, hScrollerWidth, state);

	// ── Compute display range ───────────────────────────────────────
	const int idx = scrollIndex(sortedCount, state);
	const int startIdx = std::max(0, idx);
	const int endIdx = std::min(sortedCount, startIdx + state.slotCount);

	// ── Begin container child ───────────────────────────────────────
	// <div ref="root" class="ui-datatable" @wheel="wheelMouse">
	ImGui::BeginChild("##datatable_root", availSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	const ImVec2 winPos = ImGui::GetWindowPos();
	const ImGuiIO& io = ImGui::GetIO();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// ── Handle mouse wheel ──────────────────────────────────────────
	// @wheel="wheelMouse"
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None)) {
		const float wheelY = io.MouseWheel;
		const float wheelX = io.MouseWheelH;

		if (wheelX != 0.0f || wheelY != 0.0f) {
			float delta = -wheelY;
			if (wheelX != 0.0f)
				delta = wheelX;

			if ((io.KeyShift || wheelX != 0.0f) && showHScroll) {
				// Horizontal scrolling with shift+wheel
				const float direction = delta > 0.0f ? 1.0f : -1.0f;
				const float scrollAmount = 50.0f; // Fixed scroll amount for horizontal
				state.horizontalScroll += scrollAmount * direction;
				recalculateHorizontalBounds(containerWidth, hScrollerWidth, state);
			} else {
				const float availableHeight = containerHeight - headerHeight;
				const float weight = availableHeight - scrollerHeight;

				if (sortedCount > 0) {
					const int scrollCount = state.slotCount;
					const float direction = delta > 0.0f ? 1.0f : -1.0f;
					state.scroll += ((static_cast<float>(scrollCount) * itemWeight(sortedCount)) * weight) * direction;
					recalculateBounds(containerHeight, headerHeight, scrollerHeight, state);
				}
			}
		}
	}

	// ── Handle global mouse move/up for scrollbar/resize dragging ───
	if (state.isScrolling || state.isHorizontalScrolling || state.isResizing) {
		moveMouse(io.MousePos.x, io.MousePos.y,
		          containerHeight, headerHeight, scrollerHeight,
		          containerWidth, hScrollerWidth, state);
		if (!io.MouseDown[0]) {
			stopMouse(state, headers);
		}
	}

	// ── Handle keyboard input ───────────────────────────────────────
	handleKey(sorted, selection, containerHeight, headerHeight, scrollerHeight,
	          state, onSelectionChanged, onCopy);

	// ── Compute horizontal offset ───────────────────────────────────
	float horizontalOffset = 0.0f;
	if (showHScroll) {
		const float maxScroll = std::max(0.0f, tableWidth - containerWidth);
		horizontalOffset = -maxScroll * state.horizontalScrollRel;
	}

	// ── Draw header ─────────────────────────────────────────────────
	// <thead ref="datatableheader" @mousemove="headerMouseMove" @mousedown="headerMouseDown" :style="headerCursorStyle">
	{
		const float headerStartX = winPos.x + horizontalOffset;
		const float headerStartY = winPos.y;

		// Header background
		drawList->AddRectFilled(
			ImVec2(winPos.x, headerStartY),
			ImVec2(winPos.x + containerWidth, headerStartY + headerHeight),
			app::theme::BG_U32
		);

		float colX = headerStartX;
		for (int i = 0; i < static_cast<int>(headers.size()); ++i) {
			float colWidth = (i < static_cast<int>(state.columnWidths.size()))
				? state.columnWidths[static_cast<size_t>(i)]
				: 120.0f;

			// Clip: only render if visible
			if (colX + colWidth > winPos.x && colX < winPos.x + containerWidth) {
				// Header cell border (CSS: border: 1px solid var(--border))
				drawList->AddRect(
					ImVec2(colX, headerStartY),
					ImVec2(colX + colWidth, headerStartY + headerHeight),
					app::theme::BORDER_U32
				);

				// Header text (CSS: padding: 10px)
				const float textPadding = 10.0f;
				const float iconAreaWidth = 36.0f; // Space for sort+filter icons
				float textMaxWidth = colWidth - textPadding * 2.0f - iconAreaWidth;

				ImVec2 textPos(colX + textPadding, headerStartY + (headerHeight - ImGui::GetTextLineHeight()) / 2.0f);
				drawList->PushClipRect(ImVec2(colX + 1.0f, headerStartY), ImVec2(colX + colWidth - iconAreaWidth, headerStartY + headerHeight), true);
				drawList->AddText(textPos, app::theme::FONT_PRIMARY_U32, headers[static_cast<size_t>(i)].c_str());
				drawList->PopClipRect();

				// ── Header icons (filter + sort) ────────────────────
				const float iconSize = 12.0f;
				const float iconGap = 4.0f;
				const float iconsX = colX + colWidth - textPadding - iconSize * 2.0f - iconGap;
				const float iconsY = headerStartY + (headerHeight - iconSize) / 2.0f;

				// Filter icon button
				ImVec2 filterIconMin(iconsX, iconsY);
				ImVec2 filterIconMax(iconsX + iconSize, iconsY + iconSize);

				// Sort icon button
				ImVec2 sortIconMin(iconsX + iconSize + iconGap, iconsY);
				ImVec2 sortIconMax(sortIconMin.x + iconSize, iconsY + iconSize);

				// Render filter icon (funnel shape)
				bool filterHovered = ImGui::IsMouseHoveringRect(filterIconMin, filterIconMax);
				ImU32 filterColor = filterHovered ? app::theme::FONT_HIGHLIGHT_U32 : app::theme::ICON_DEFAULT_U32;
				// Simple funnel icon
				float fx = filterIconMin.x, fy = filterIconMin.y;
				drawList->AddTriangleFilled(
					ImVec2(fx, fy + 2.0f),
					ImVec2(fx + iconSize, fy + 2.0f),
					ImVec2(fx + iconSize / 2.0f, fy + iconSize / 2.0f),
					filterColor
				);
				drawList->AddRectFilled(
					ImVec2(fx + iconSize / 2.0f - 1.0f, fy + iconSize / 2.0f),
					ImVec2(fx + iconSize / 2.0f + 1.0f, fy + iconSize - 1.0f),
					filterColor
				);

				if (filterHovered && ImGui::IsMouseClicked(0)) {
					handleFilterIconClick(i, headers, filter, onFilterChanged);
				}

				// Render sort icon
				const char* sortIconName = getSortIconName(i, state);
				bool sortHovered = ImGui::IsMouseHoveringRect(sortIconMin, sortIconMax);
				ImU32 sortColor = sortHovered ? app::theme::FONT_HIGHLIGHT_U32 : app::theme::ICON_DEFAULT_U32;

				float sx = sortIconMin.x;
				float sy = sortIconMin.y;
				if (std::string(sortIconName) == "sort-icon-up") {
					// Up arrow
					drawList->AddTriangleFilled(
						ImVec2(sx + iconSize / 2.0f, sy + 1.0f),
						ImVec2(sx + 1.0f, sy + iconSize - 1.0f),
						ImVec2(sx + iconSize - 1.0f, sy + iconSize - 1.0f),
						sortColor
					);
				} else if (std::string(sortIconName) == "sort-icon-down") {
					// Down arrow
					drawList->AddTriangleFilled(
						ImVec2(sx + 1.0f, sy + 1.0f),
						ImVec2(sx + iconSize - 1.0f, sy + 1.0f),
						ImVec2(sx + iconSize / 2.0f, sy + iconSize - 1.0f),
						sortColor
					);
				} else {
					// Both arrows (sort-icon-off) — dimmed double arrow
					float midY = sy + iconSize / 2.0f;
					drawList->AddTriangleFilled(
						ImVec2(sx + iconSize / 2.0f, sy + 1.0f),
						ImVec2(sx + 2.0f, midY - 1.0f),
						ImVec2(sx + iconSize - 2.0f, midY - 1.0f),
						sortColor
					);
					drawList->AddTriangleFilled(
						ImVec2(sx + 2.0f, midY + 1.0f),
						ImVec2(sx + iconSize - 2.0f, midY + 1.0f),
						ImVec2(sx + iconSize / 2.0f, sy + iconSize - 1.0f),
						sortColor
					);
				}

				if (sortHovered && ImGui::IsMouseClicked(0) && !state.isResizing) {
					toggleSort(i, state);
				}
			}

			// ── Column resize detection ─────────────────────────────
			// headerMouseMove: detect resize zones near column borders
			if (!state.isResizing) {
				const float resizeZoneWidth = 5.0f;
				if (i < static_cast<int>(headers.size()) - 1) {
					float borderX = colX + colWidth;
					if (io.MousePos.y >= headerStartY && io.MousePos.y <= headerStartY + headerHeight &&
					    io.MousePos.x >= borderX - resizeZoneWidth && io.MousePos.x <= borderX + resizeZoneWidth) {
						state.isOverResizeZone = true;
						state.resizeZoneColumnIndex = i;
					}
				}
			}

			colX += colWidth;
		}

		// headerMouseDown: start resize if in resize zone
		if (state.isOverResizeZone && state.resizeZoneColumnIndex >= 0 &&
		    ImGui::IsMouseClicked(0) &&
		    io.MousePos.y >= headerStartY && io.MousePos.y <= headerStartY + headerHeight) {
			state.isResizing = true;
			state.resizeColumnIndex = state.resizeZoneColumnIndex;
			state.resizeStartX = io.MousePos.x;
			if (state.resizeZoneColumnIndex < static_cast<int>(state.columnWidths.size()))
				state.resizeStartWidth = state.columnWidths[static_cast<size_t>(state.resizeZoneColumnIndex)];
		}

		// Reset resize zone detection each frame if not actively resizing
		if (!state.isResizing) {
			// Only clear if mouse is NOT over a resize zone
			bool overAny = false;
			float checkX = winPos.x + horizontalOffset;
			for (int i = 0; i < static_cast<int>(headers.size()); ++i) {
				float cw = (i < static_cast<int>(state.columnWidths.size()))
					? state.columnWidths[static_cast<size_t>(i)]
					: 120.0f;
				if (i < static_cast<int>(headers.size()) - 1) {
					float borderX = checkX + cw;
					if (io.MousePos.y >= headerStartY && io.MousePos.y <= headerStartY + headerHeight &&
					    io.MousePos.x >= borderX - 5.0f && io.MousePos.x <= borderX + 5.0f) {
						overAny = true;
						break;
					}
				}
				checkX += cw;
			}
			if (!overAny) {
				state.isOverResizeZone = false;
				state.resizeZoneColumnIndex = -1;
			}
		}

		// Set cursor when over resize zone or resizing
		if (state.isOverResizeZone || state.isResizing) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}
	}

	// ── Draw table body ─────────────────────────────────────────────
	// <tbody>
	{
		const float bodyStartY = winPos.y + headerHeight;
		const float bodyStartX = winPos.x + horizontalOffset;

		for (int rowIdx = startIdx; rowIdx < endIdx; ++rowIdx) {
			const int displayRow = rowIdx - startIdx;
			const float rowY = bodyStartY + static_cast<float>(displayRow) * rowHeight;

			// Skip if row is below visible area
			if (rowY > winPos.y + containerHeight)
				break;

			const auto& row = sorted[static_cast<size_t>(rowIdx)];
			const bool isSelected = std::find(selection.begin(), selection.end(), rowIdx) != selection.end();

			// Row background
			// CSS: .ui-datatable table tr { background: var(--background-dark) }
			// CSS: .ui-datatable table tr:nth-child(even) { background: var(--background-alt) }
			ImU32 rowBg;
			if (isSelected) {
				// CSS: .ui-datatable table tr.selected { background: var(--font-alt) !important }
				rowBg = app::theme::TABLE_ROW_HOVER_U32;
			} else if (displayRow % 2 == 1) {
				rowBg = app::theme::BG_ALT_U32;
			} else {
				rowBg = app::theme::BG_DARK_U32;
			}

			ImVec2 rowMin(winPos.x, rowY);
			ImVec2 rowMax(winPos.x + containerWidth, rowY + rowHeight);
			drawList->AddRectFilled(rowMin, rowMax, rowBg);

			// Row hover effect
			// CSS: .ui-datatable table tbody tr:hover { background: var(--font-alt) }
			if (!isSelected && ImGui::IsMouseHoveringRect(rowMin, rowMax)) {
				drawList->AddRectFilled(rowMin, rowMax, app::theme::TABLE_ROW_SELECTED_U32);
			}

			// Row click handling
			if (ImGui::IsMouseHoveringRect(rowMin, rowMax)) {
				if (ImGui::IsMouseClicked(0)) {
					// selectRow(scrollIndex + rowIndex, $event)
					selectRow(rowIdx, io.KeyCtrl, io.KeyShift, selection, state, onSelectionChanged);
				}

				// Right-click context menu
				if (ImGui::IsMouseClicked(1)) {
					// Determine which column was clicked
					float checkColX = bodyStartX;
					int clickedCol = 0;
					for (int c = 0; c < static_cast<int>(headers.size()); ++c) {
						float cw = (c < static_cast<int>(state.columnWidths.size()))
							? state.columnWidths[static_cast<size_t>(c)]
							: 120.0f;
						if (io.MousePos.x >= checkColX && io.MousePos.x < checkColX + cw) {
							clickedCol = c;
							break;
						}
						checkColX += cw;
					}
					handleContextMenu(rowIdx, clickedCol, sorted, selection, state, onSelectionChanged, onContextMenu);
				}
			}

			// Draw cells
			// <td v-for="(field, index) in row" :style="columnStyles['col-' + index]">{{field}}</td>
			float cellX = bodyStartX;
			for (int c = 0; c < static_cast<int>(row.size()); ++c) {
				float cw = (c < static_cast<int>(state.columnWidths.size()))
					? state.columnWidths[static_cast<size_t>(c)]
					: 120.0f;

				// Clip: only render if visible
				if (cellX + cw > winPos.x && cellX < winPos.x + containerWidth) {
					// CSS: td { padding: 5px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis }
					const float cellPadding = 5.0f;
					ImVec2 textPos(cellX + cellPadding, rowY + (rowHeight - ImGui::GetTextLineHeight()) / 2.0f);

					drawList->PushClipRect(ImVec2(cellX, rowY), ImVec2(cellX + cw, rowY + rowHeight), true);
					drawList->AddText(textPos, app::theme::FONT_PRIMARY_U32, row[static_cast<size_t>(c)].c_str());
					drawList->PopClipRect();
				}

				cellX += cw;
			}
		}
	}

	// ── Draw vertical scroller ──────────────────────────────────────
	// <div class="scroller" ref="dtscroller" @mousedown="startMouse" :class="{ using: isScrolling }" :style="{ top: scrollOffset }">
	{
		const float scrollerWidth = 8.0f; // CSS: .scroller width: 8px
		const float scrollerMarginTop = headerHeight + 6.0f; // CSS: .ui-datatable .scroller margin-top: 46px
		const float scrollerX = winPos.x + containerWidth - scrollerWidth - 3.0f; // CSS: right: 3px
		const float thumbY = winPos.y + scrollerMarginTop + scrollOffset(state);

		ImVec2 thumbMin(scrollerX, thumbY);
		ImVec2 thumbMax(scrollerX + scrollerWidth, thumbY + scrollerHeight);

		// Inner div with rounded corners
		// CSS: .scroller > div { top: 3px; bottom: 3px; border-radius: 4px; background: var(--font-primary) }
		ImVec2 innerMin(thumbMin.x, thumbMin.y + 3.0f);
		ImVec2 innerMax(thumbMax.x, thumbMax.y - 3.0f);

		const bool thumbHovered = ImGui::IsMouseHoveringRect(thumbMin, thumbMax) || state.isScrolling;
		// CSS: .scroller > div { background: var(--font-primary) } / .scroller:hover > div { background: var(--font-highlight) }
		const ImU32 thumbColor = thumbHovered
			? app::theme::FONT_HIGHLIGHT_U32
			: app::theme::FONT_PRIMARY_U32;

		// CSS: opacity: 0.7
		const ImU32 thumbColorWithOpacity = (thumbColor & 0x00FFFFFF) | (static_cast<ImU32>(static_cast<float>((thumbColor >> 24) & 0xFF) * 0.7f) << 24);

		drawList->AddRectFilled(innerMin, innerMax, thumbColorWithOpacity, 4.0f);

		// Handle mouse-down on the scroller thumb.
		if (ImGui::IsMouseHoveringRect(thumbMin, thumbMax) && ImGui::IsMouseClicked(0)) {
			startMouse(io.MousePos.y, state);
		}
	}

	// ── Draw horizontal scroller ────────────────────────────────────
	// <div class="hscroller" ref="dthscroller" @mousedown="startHorizontalMouse" :class="{ using: isHorizontalScrolling }" :style="...">
	if (showHScroll) {
		const float hScrollerHeight = 8.0f; // CSS: .hscroller height: 8px
		const float hScrollerY = winPos.y + containerHeight - hScrollerHeight - 3.0f; // CSS: bottom: 3px
		const float hScrollerX = winPos.x + state.horizontalScroll;

		ImVec2 thumbMin(hScrollerX, hScrollerY);
		ImVec2 thumbMax(hScrollerX + hScrollerWidth, hScrollerY + hScrollerHeight);

		// Inner div
		// CSS: .hscroller > div { left: 3px; right: 3px; border-radius: 4px }
		ImVec2 innerMin(thumbMin.x + 3.0f, thumbMin.y);
		ImVec2 innerMax(thumbMax.x - 3.0f, thumbMax.y);

		const bool hThumbHovered = ImGui::IsMouseHoveringRect(thumbMin, thumbMax) || state.isHorizontalScrolling;
		const ImU32 hThumbColor = hThumbHovered
			? app::theme::FONT_HIGHLIGHT_U32
			: app::theme::FONT_PRIMARY_U32;

		const ImU32 hThumbColorWithOpacity = (hThumbColor & 0x00FFFFFF) | (static_cast<ImU32>(static_cast<float>((hThumbColor >> 24) & 0xFF) * 0.7f) << 24);

		drawList->AddRectFilled(innerMin, innerMax, hThumbColorWithOpacity, 4.0f);

		if (ImGui::IsMouseHoveringRect(thumbMin, thumbMax) && ImGui::IsMouseClicked(0)) {
			startHorizontalMouse(io.MousePos.x, state);
		}
	}

	ImGui::EndChild();

	// ── Status bar ──────────────────────────────────────────────────
	// <div class="list-status" v-if="rows && rows.length > 0">
	if (!rows.empty()) {
		const int filteredCount = static_cast<int>(filtered.size());
		const int totalCount = static_cast<int>(rows.size());

		std::string statusText;
		if (filteredCount != totalCount) {
			// <span v-if="filteredItems.length !== rows.length">
			//     Showing {{ filteredItems.length.toLocaleString() }} of {{ rows.length.toLocaleString() }} rows
			// </span>
			statusText = "Showing " + std::to_string(filteredCount) + " of " + std::to_string(totalCount) + " rows";
		} else {
			// <span v-else>{{ rows.length.toLocaleString() }} rows</span>
			statusText = std::to_string(totalCount) + " rows";
		}

		ImGui::TextDisabled("%s", statusText.c_str());
	}

	ImGui::PopID();
}

} // namespace data_table
