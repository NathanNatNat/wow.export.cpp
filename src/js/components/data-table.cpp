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
#include <cctype>
#include <cstring>
#include <cwchar>
#include <limits>
#include <regex>
#include <unordered_map>
#include <unordered_set>

namespace data_table {

/**
 * Compute a stable key for a row (used for unordered_set lookups).
 */
static std::string rowKey(const std::vector<std::string>& row) {
	std::string key;
	size_t total = 0;
	for (const auto& field : row)
		total += field.size() + 1;
	key.reserve(total);
	for (size_t i = 0; i < row.size(); ++i) {
		if (i > 0) key += '\x1F';
		key += row[i];
	}
	return key;
}

static std::unordered_set<std::string> buildSelectionSet(const std::vector<std::vector<std::string>>& selection) {
	std::unordered_set<std::string> s;
	s.reserve(selection.size());
	for (const auto& row : selection)
		s.insert(rowKey(row));
	return s;
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

	while (!generalFilter.empty() && generalFilter.back() == ' ')
		generalFilter.pop_back();
	while (!generalFilter.empty() && generalFilter.front() == ' ')
		generalFilter.erase(generalFilter.begin());

	return { columnFilters, generalFilter };
}

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
		const std::vector<std::vector<std::string>>& selection,
		const std::function<void(const std::vector<std::vector<std::string>>&)>& onSelectionChanged) {
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
	std::unordered_set<std::string> filteredSet;
	filteredSet.reserve(res.size());
	for (const auto& row : res)
		filteredSet.insert(rowKey(row));

	bool hasChanges = false;
	std::vector<std::vector<std::string>> newSelection;
	newSelection.reserve(selection.size());
	for (const auto& row : selection) {
		if (filteredSet.count(rowKey(row))) {
			newSelection.push_back(row);
		} else {
			hasChanges = true;
		}
	}

	if (hasChanges && onSelectionChanged)
		onSelectionChanged(newSelection);

	return res;
}

static bool tryParseNumber(const std::string& s, double& out) {
	if (s.empty()) { out = 0.0; return true; }
	try {
		size_t pos = 0;
		out = std::stod(s, &pos);
		return pos == s.size();
	} catch (...) {
		return false;
	}
}

static int localeCompare(const std::string& aStr, const std::string& bStr) {
	auto toWide = [](const std::string& s) -> std::wstring {
		std::wstring result;
		result.reserve(s.size());
		std::mbstate_t state{};
		const char* src = s.c_str();
		size_t remaining = s.size();
		while (remaining > 0) {
			wchar_t wc;
			size_t n = std::mbrtowc(&wc, src, remaining, &state);
			if (n == static_cast<size_t>(-1) || n == static_cast<size_t>(-2)) {
				result.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*src)));
				++src; --remaining;
				state = std::mbstate_t{};
			} else if (n == 0) {
				break;
			} else {
				result.push_back(wc);
				src += n;
				remaining -= n;
			}
		}
		return result;
	};

	const std::wstring aw = toWide(aStr);
	const std::wstring bw = toWide(bStr);
	const int cmp = std::wcscoll(aw.c_str(), bw.c_str());
	if (cmp != 0)
		return cmp;
	return aStr.compare(bStr);
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

	std::stable_sort(sorted.begin(), sorted.end(), [columnIndex, ascending](const std::vector<std::string>& a, const std::vector<std::string>& b) {
		// Handle null/undefined values
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
		int cmp = localeCompare(aStr, bStr);
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
static std::vector<float> calculateColumnWidths(const std::vector<std::string>& headers) {
	std::vector<float> widths;
	if (headers.empty()) return widths;

	for (const auto& header : headers) {
		// 8px per character + 40px for icons/padding
		float textWidth = static_cast<float>(header.length()) * 8.0f + 40.0f;
		widths.push_back(std::max(120.0f, textWidth));
	}

	return widths;
}

/**
 * Invoked by a ResizeObserver when the main component node
 * is resized due to layout changes.
 */
static void resize(float containerHeight, float headerHeight, float scrollerHeight,
                    DataTableState& state) {
	// Calculate available height for scrolling (subtract header and scrollbar widget)
	const float availableHeight = containerHeight - headerHeight;
	state.scroll = (availableHeight - scrollerHeight) * state.scrollRel;
	state.slotCount = std::max(1, static_cast<int>(std::floor(availableHeight / 32.0f)) - 2);
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
 * Determines if horizontal scrolling is needed based on table width vs container width.
 */
static bool needsHorizontalScrolling(float tableWidth, float containerWidth) {
	return tableWidth > containerWidth;
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
 * @param columnIndex - Index of the column
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
}

/**
 * Invoked when a user selects a row in the table.
 * @param rowIndex - Index of the row in sortedItems
 */
static void selectRow(int rowIndex, bool ctrlKey, bool shiftKey,
                       const std::vector<std::vector<std::string>>& sorted,
                       const std::vector<std::vector<std::string>>& selection,
                       DataTableState& state,
                       const std::function<void(const std::vector<std::vector<std::string>>&)>& onSelectionChanged) {
	if (rowIndex < 0 || rowIndex >= static_cast<int>(sorted.size()))
		return;

	const auto& row = sorted[static_cast<size_t>(rowIndex)];
	const std::string rowK = rowKey(row);

	int checkIndex = -1;
	for (int i = 0; i < static_cast<int>(selection.size()); ++i) {
		if (rowKey(selection[static_cast<size_t>(i)]) == rowK) {
			checkIndex = i;
			break;
		}
	}

	std::vector<std::vector<std::string>> newSelection = selection;

	if (ctrlKey) {
		// Ctrl-key held, so allow multiple selections.
		if (checkIndex > -1)
			newSelection.erase(newSelection.begin() + checkIndex);
		else
			newSelection.push_back(row);
	} else if (shiftKey) {
		// Shift-key held, select a range.
		if (state.lastSelectItem != -1 && state.lastSelectItem != rowIndex) {
			const int lastSelectIndex = state.lastSelectItem;
			const int thisSelectIndex = rowIndex;

			const int delta = std::abs(lastSelectIndex - thisSelectIndex);
			const int lowest = std::min(lastSelectIndex, thisSelectIndex);
			const int highest = lowest + delta;

			std::unordered_set<std::string> existing;
			existing.reserve(newSelection.size());
			for (const auto& r : newSelection)
				existing.insert(rowKey(r));

			for (int i = lowest; i <= highest && i < static_cast<int>(sorted.size()); i++) {
				const auto& r = sorted[static_cast<size_t>(i)];
				const std::string rk = rowKey(r);
				if (existing.insert(rk).second)
					newSelection.push_back(r);
			}
		}
	} else if (checkIndex == -1 || (checkIndex > -1 && static_cast<int>(newSelection.size()) > 1)) {
		// Normal click, replace entire selection.
		newSelection.clear();
		newSelection.push_back(row);
	}

	state.lastSelectItem = rowIndex;
	if (onSelectionChanged)
		onSelectionChanged(newSelection);
}

/**
 * Invoked when a user right-clicks on a row in the table.
 * @param rowIndex - Index of the row in sortedItems
 * @param columnIndex - Index of the column
 */
static void handleContextMenu(int rowIndex, int columnIndex,
                                const std::vector<std::vector<std::string>>& sorted,
                                const std::vector<std::vector<std::string>>& selection,
                                const std::unordered_set<std::string>& selectionSet,
                                DataTableState& state,
                                const std::function<void(const std::vector<std::vector<std::string>>&)>& onSelectionChanged,
                                const std::function<void(const ContextMenuEvent&)>& onContextMenu) {
	if (rowIndex < 0 || rowIndex >= static_cast<int>(sorted.size()))
		return;

	const auto& row = sorted[static_cast<size_t>(rowIndex)];

	// if the row is not already selected, select it
	if (!selectionSet.count(rowKey(row))) {
		state.lastSelectItem = rowIndex;
		if (onSelectionChanged)
			onSelectionChanged({ row });
	}

	std::string cellValue;
	if (columnIndex >= 0 && columnIndex < static_cast<int>(row.size()))
		cellValue = row[static_cast<size_t>(columnIndex)];

	if (onContextMenu) {
		ContextMenuEvent evt;
		evt.rowIndex = rowIndex;
		evt.columnIndex = columnIndex;
		evt.cellValue = cellValue;
		evt.selectedCount = std::max(1, static_cast<int>(selection.size()));
		const ImGuiIO& io = ImGui::GetIO();
		evt.mouseX = io.MousePos.x;
		evt.mouseY = io.MousePos.y;
		onContextMenu(evt);
	}
}

/**
 * Invoked when a keydown event is fired.
 */
static void handleKey(const std::vector<std::vector<std::string>>& sorted,
                       const std::vector<std::vector<std::string>>& selection,
                       float containerHeight, float headerHeight, float scrollerHeight,
                       DataTableState& state,
                       const std::function<void(const std::vector<std::vector<std::string>>&)>& onSelectionChanged,
                       const std::function<void()>& onCopy) {
	// If document.activeElement is the document body, then we can safely assume
	// the user is not focusing anything, and can intercept keyboard input.
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

			std::vector<std::vector<std::string>> newSelection = selection;

			if (!io.KeyShift)
				newSelection.clear();

			newSelection.push_back(sorted[static_cast<size_t>(nextIndex)]);
			state.lastSelectItem = nextIndex;
			if (onSelectionChanged)
				onSelectionChanged(newSelection);
		}
	}
}

static std::string formatWithThousandsSep(int value) {
	std::string str = std::to_string(value);
	if (str.size() <= 3)
		return str;

	std::string result;
	int count = 0;
	for (auto it = str.rbegin(); it != str.rend(); ++it) {
		if (count > 0 && count % 3 == 0)
			result.insert(result.begin(), ',');
		result.insert(result.begin(), *it);
		count++;
	}
	return result;
}

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

static std::vector<std::vector<std::string>> sortSelectionByDisplayOrder(
		const std::vector<std::vector<std::string>>& sorted,
		const std::vector<std::vector<std::string>>& selection) {
	std::unordered_map<std::string, int> indexMap;
	indexMap.reserve(sorted.size());
	for (size_t i = 0; i < sorted.size(); ++i)
		indexMap[rowKey(sorted[i])] = static_cast<int>(i);

	std::vector<std::pair<int, const std::vector<std::string>*>> indexed;
	indexed.reserve(selection.size());
	for (const auto& row : selection) {
		auto it = indexMap.find(rowKey(row));
		const int idx = (it != indexMap.end()) ? it->second : std::numeric_limits<int>::max();
		indexed.emplace_back(idx, &row);
	}

	std::stable_sort(indexed.begin(), indexed.end(),
		[](const auto& a, const auto& b) { return a.first < b.first; });

	std::vector<std::vector<std::string>> result;
	result.reserve(indexed.size());
	for (const auto& [_, rowPtr] : indexed)
		result.push_back(*rowPtr);
	return result;
}

/**
 * Get selected rows as CSV string.
 * @returns {string} CSV formatted string
 */
std::string getSelectedRowsAsCSV(const std::vector<std::string>& headers,
                                  const std::vector<std::vector<std::string>>& sorted,
                                  const std::vector<std::vector<std::string>>& selection,
                                  bool copyheader) {
	if (selection.empty() || headers.empty())
		return "";

	const auto rows = sortSelectionByDisplayOrder(sorted, selection);

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
                                  const std::vector<std::vector<std::string>>& selection,
                                  const std::string& tablename) {
	if (selection.empty() || headers.empty())
		return "";

	const auto rows = sortSelectionByDisplayOrder(sorted, selection);

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
		if (!val.empty()) {
			double num = 0.0;
			if (tryParseNumber(val, num))
				return val;
		}

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


std::vector<std::vector<std::string>> getFilteredSortedRows(
        const std::vector<std::vector<std::string>>& rows,
        const std::vector<std::string>& headers,
        const std::string& filter,
        bool regex,
        const DataTableState& state) {
	auto filtered = filteredItems(rows, filter, regex, headers, {}, nullptr);
	return sortedItems(filtered, state);
}

void render(const char* id,
            const std::vector<std::string>& headers,
            const std::vector<std::vector<std::string>>& rows,
            const std::string& filter,
            bool regex,
            const std::vector<std::vector<std::string>>& selection,
            [[maybe_unused]] bool copyheader,
            [[maybe_unused]] const std::string& tablename,
            DataTableState& state,
            const std::function<void(const std::vector<std::vector<std::string>>&)>& onSelectionChanged,
            const std::function<void(const ContextMenuEvent&)>& onContextMenu,
            const std::function<void()>& onCopy,
            const std::function<void(const std::string&)>& onFilterChanged) {
	ImGui::PushID(id);

	if (state.prevHeaders != headers)
		state.prevHeaders = headers;

	if (state.prevRowCount != rows.size() ||
	    state.prevRowsPtr != static_cast<const void*>(rows.data()) ||
	    state.prevRowsVersion != state.rowsVersion) {
		state.prevRowCount = rows.size();
		state.prevRowsPtr = static_cast<const void*>(rows.data());
		state.prevRowsVersion = state.rowsVersion;
		state.lastSelectItem = -1;
		if (onSelectionChanged)
			onSelectionChanged({});
	}

	auto filtered = filteredItems(rows, filter, regex, headers, selection, onSelectionChanged);
	auto sorted = sortedItems(filtered, state);
	const int sortedCount = static_cast<int>(sorted.size());

	const auto selectionSet = buildSelectionSet(selection);

	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const float containerWidth = availSize.x;
	const float containerHeight = availSize.y;

	const std::vector<float> columnWidths = calculateColumnWidths(headers);

	const float rowHeight = 32.0f;
	const float headerHeight = 40.0f;
	const float scrollerHeight = 45.0f;

	float tableWidth = 0.0f;
	for (float w : columnWidths)
		tableWidth += w;

	const bool showHScroll = needsHorizontalScrolling(tableWidth, containerWidth);

	if (state.lastResizeWidth != containerWidth ||
	    state.lastResizeHeight != containerHeight ||
	    state.lastResizeSortedCount != sortedCount) {
		resize(containerHeight, headerHeight, scrollerHeight, state);
		state.lastResizeWidth = containerWidth;
		state.lastResizeHeight = containerHeight;
		state.lastResizeSortedCount = sortedCount;
	}

	const int idx = scrollIndex(sortedCount, state);
	const int startIdx = std::max(0, idx);
	const int endIdx = std::min(sortedCount, startIdx + state.slotCount);

	const float statusBarHeight = rows.empty() ? 0.0f : ImGui::GetTextLineHeightWithSpacing();

	const ImVec2 tableRegionSize(containerWidth, std::max(0.0f, containerHeight - statusBarHeight));

	ImGui::BeginChild("##datatable_root", tableRegionSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	const ImGuiIO& io = ImGui::GetIO();
	const bool windowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

	if (windowHovered) {
		const float wheelY = io.MouseWheel;
		const float wheelX = io.MouseWheelH;

		if (wheelY != 0.0f && !(io.KeyShift && showHScroll) && wheelX == 0.0f && sortedCount > 0) {
			const float availableHeight = containerHeight - headerHeight;
			const float weight = availableHeight - scrollerHeight;
			const float direction = wheelY > 0.0f ? -1.0f : 1.0f;
			state.scroll += ((static_cast<float>(state.slotCount) * itemWeight(sortedCount)) * weight) * direction;
			recalculateBounds(containerHeight, headerHeight, scrollerHeight, state);
			ImGui::GetIO().MouseWheel = 0.0f;
		}
	}

	handleKey(sorted, selection, containerHeight, headerHeight, scrollerHeight,
	          state, onSelectionChanged, onCopy);

	const int columnCount = static_cast<int>(headers.size());
	if (columnCount > 0) {
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_BordersV |
			ImGuiTableFlags_BordersOuterH |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_Sortable |
			ImGuiTableFlags_ScrollX |
			ImGuiTableFlags_NoSavedSettings;

		ImGui::SetNextWindowContentSize(ImVec2(tableWidth, 0.0f));

		if (ImGui::BeginTable("##datatable", columnCount, tableFlags, ImVec2(0.0f, 0.0f))) {
			for (int i = 0; i < columnCount; ++i) {
				const float w = (i < static_cast<int>(columnWidths.size())) ? columnWidths[i] : 120.0f;
				ImGui::TableSetupColumn(headers[i].c_str(),
					ImGuiTableColumnFlags_WidthFixed, w);
			}
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableNextRow(ImGuiTableRowFlags_Headers, headerHeight);
			for (int i = 0; i < columnCount; ++i) {
				ImGui::TableSetColumnIndex(i);
				ImGui::PushID(i);

				const char* sortIconName = getSortIconName(i, state);
				const char* sortGlyph = "";
				if (std::strcmp(sortIconName, "sort-icon-up") == 0) sortGlyph = " \xE2\x96\xB2";
				else if (std::strcmp(sortIconName, "sort-icon-down") == 0) sortGlyph = " \xE2\x96\xBC";

				const float availW = ImGui::GetContentRegionAvail().x;
				const float iconW = ImGui::GetFrameHeight() * 0.7f;
				const float labelW = std::max(0.0f, availW - iconW * 2.0f - 6.0f);

				const std::string label = headers[static_cast<size_t>(i)] + sortGlyph;
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered]);
				if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(labelW, headerHeight - 4.0f))) {
					toggleSort(i, state);
				}
				ImGui::PopStyleColor();

				ImGui::SameLine();
				if (ImGui::SmallButton("F##filter")) {
					handleFilterIconClick(i, headers, filter, onFilterChanged);
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Filter this column");

				ImGui::PopID();
			}

			for (int rowIdx = startIdx; rowIdx < endIdx; ++rowIdx) {
				const auto& row = sorted[static_cast<size_t>(rowIdx)];
				const std::string rowK = rowKey(row);
				const bool isSelected = selectionSet.count(rowK) > 0;

				ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
				ImGui::PushID(rowIdx);

				for (int c = 0; c < columnCount; ++c) {
					ImGui::TableSetColumnIndex(c);
					const std::string& cell = (c < static_cast<int>(row.size())) ? row[static_cast<size_t>(c)] : std::string();

					if (c == 0) {
						const ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
						if (ImGui::Selectable(cell.empty() ? "##empty" : cell.c_str(), isSelected, flags, ImVec2(0.0f, rowHeight - 4.0f))) {
							selectRow(rowIdx, io.KeyCtrl, io.KeyShift, sorted, selection, state, onSelectionChanged);
						}
						if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
							handleContextMenu(rowIdx, c, sorted, selection, selectionSet, state, onSelectionChanged, onContextMenu);
						}
					} else {
						ImGui::TextUnformatted(cell.c_str());
						if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
							handleContextMenu(rowIdx, c, sorted, selection, selectionSet, state, onSelectionChanged, onContextMenu);
						}
					}
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}
	}

	ImGui::EndChild();

	if (!rows.empty()) {
		const int filteredCount = static_cast<int>(filtered.size());
		const int totalCount = static_cast<int>(rows.size());

		std::string statusText;
		if (filteredCount != totalCount) {
			statusText = "Showing " + formatWithThousandsSep(filteredCount) + " of " + formatWithThousandsSep(totalCount) + " rows";
		} else {
			statusText = formatWithThousandsSep(totalCount) + " rows";
		}

		ImGui::TextDisabled("%s", statusText.c_str());
	}

	ImGui::PopID();
}

} // namespace data_table
