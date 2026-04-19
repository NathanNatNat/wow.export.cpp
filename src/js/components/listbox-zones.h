/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <functional>

#include "listbox.h"

/**
 * Listbox-zones component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Extends the base listbox component with expansion filtering for zones.
 *
 * Additional props:
 * expansionFilter: Reactive value for filtering by expansion ID (-1 for all, 0+ for specific expansion)
 */
namespace listbox_zones {

/**
 * Persistent state for a single ListboxZones widget instance.
 * Extends ListboxState with expansion filter tracking.
 */
struct ListboxZonesState {
	listbox::ListboxState base;
	int prevExpansionFilter = -1;  // For change detection on expansionFilter.

	// Cached expansion-filtered items — rebuilt only when inputs change.
	std::vector<std::string> cachedExpansionFiltered;
	const std::string* cachedExpItemsData   = nullptr;
	size_t             cachedExpItemsSize    = ~size_t(0);
	int                cachedExpansionFilter = -2; // -2 = sentinel "never built"
};

/**
 * Render a listbox-zones using ImGui.
 *
 * @param id                   Unique ImGui ID string for this widget instance.
 * @param items                Array of item entries displayed in the list.
 * @param filter               Current text filter value (empty for no filter).
 * @param selection            Currently selected items (string values).
 * @param single               If true, only one entry can be selected.
 * @param keyinput             If true, listbox registers for keyboard input.
 * @param regex                If true, filter is treated as a regular expression.
 * @param copymode             Defines the behavior of Ctrl+C.
 * @param pasteselection       If true, Ctrl+V will load a selection.
 * @param copytrimwhitespace   If true, whitespace is trimmed from copied paths.
 * @param unittype             Unit name for file counter (empty to hide counter).
 * @param overrideItems        If non-null and non-empty, used as override listfile.
 * @param disable              If true, disables selection changes.
 * @param persistscrollkey     If non-empty, enables scroll position persistence with this key.
 * @param quickfilters         Array of file extensions for quick filter links.
 * @param nocopy               If true, disables Ctrl+C copy.
 * @param expansionFilter      Expansion ID filter (-1 for all, 0+ for specific expansion).
 * @param state                Persistent state across frames.
 * @param onSelectionChanged   Callback when selection changes.
 * @param onContextMenu        Callback when context menu is requested.
 */
void render(const char* id,
            const std::vector<std::string>& items,
            const std::string& filter,
            const std::vector<std::string>& selection,
            bool single,
            bool keyinput,
            bool regex,
            listbox::CopyMode copymode,
            bool pasteselection,
            bool copytrimwhitespace,
            const std::string& unittype,
            const std::vector<std::string>* overrideItems,
            bool disable,
            const std::string& persistscrollkey,
            const std::vector<std::string>& quickfilters,
            bool nocopy,
            int expansionFilter,
            ListboxZonesState& state,
            const std::function<void(const std::vector<std::string>&)>& onSelectionChanged,
            const std::function<void(const listbox::ContextMenuEvent&)>& onContextMenu);

} // namespace listbox_zones
