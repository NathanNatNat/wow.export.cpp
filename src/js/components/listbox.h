/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>

/**
 * Listbox component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['items', 'filter', 'selection', 'single',
 * 'keyinput', 'regex', 'copymode', 'pasteselection', 'copytrimwhitespace',
 * 'includefilecount', 'unittype', 'override', 'disable', 'persistscrollkey',
 * 'quickfilters', 'nocopy'],
 * emits: ['update:selection', 'update:filter', 'contextmenu'].
 *
 * A listbox with virtual scrolling, custom scrollbar drag, text/regex filtering
 * with debounce, single/multi-select, keyboard navigation, clipboard copy/paste,
 * quick file-extension filters, context menu support, and scroll persistence.
 */
namespace listbox {

/**
 * Copy mode for Ctrl+C behavior.
 * JS equivalent: copymode prop values.
 */
enum class CopyMode {
	Default,  // Copy items as-is.
	DIR,      // Copy parent directory of each item.
	FID       // Copy extracted file data ID from each item.
};

/**
 * Context menu event data.
 * JS equivalent: { item, selection, event } emitted via 'contextmenu'.
 * mousePos replaces the JS MouseEvent for context menu positioning.
 */
struct ContextMenuEvent {
	std::string item;
	std::vector<std::string> selection;
	float mousePosX = 0.0f;
	float mousePosY = 0.0f;
};

/**
 * Persistent state for a single Listbox widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct ListboxState {
	float scroll = 0.0f;
	float scrollRel = 0.0f;
	bool isScrolling = false;
	int slotCount = 1;
	std::string lastSelectItem;         // Empty string = null
	bool hasLastSelectItem = false;
	std::string debouncedFilter;
	std::optional<std::chrono::steady_clock::time_point> filterTimeoutStart;
	bool scrollPositionRestored = false;
	std::string activeQuickFilter;      // Empty string = null/inactive

	// Mouse drag tracking (equivalent to JS instance vars set in startMouse).
	float scrollStartY = 0.0f;
	float scrollStart = 0.0f;

	// Previous filter value for debounce change detection.
	std::string prevFilter;

	// Whether the component has been initialized (first frame).
	bool initialized = false;
};

/**
 * Render a listbox using ImGui.
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
 * @param state                Persistent state across frames.
 * @param onSelectionChanged   Callback when selection changes; receives new selection.
 * @param onContextMenu        Callback when context menu is requested; receives context menu event data.
 */
void render(const char* id,
            const std::vector<std::string>& items,
            const std::string& filter,
            const std::vector<std::string>& selection,
            bool single,
            bool keyinput,
            bool regex,
            CopyMode copymode,
            bool pasteselection,
            bool copytrimwhitespace,
            const std::string& unittype,
            const std::vector<std::string>* overrideItems,
            bool disable,
            const std::string& persistscrollkey,
            const std::vector<std::string>& quickfilters,
            bool nocopy,
            ListboxState& state,
            const std::function<void(const std::vector<std::string>&)>& onSelectionChanged,
            const std::function<void(const ContextMenuEvent&)>& onContextMenu);

/**
 * Save the scroll position for a listbox state.
 * Equivalent to the JS beforeUnmount scroll position save.
 * Callers should invoke this before discarding a ListboxState when persistscrollkey is set.
 */
void saveState(const ListboxState& state,
               const std::string& persistscrollkey,
               const std::vector<std::string>& filteredItems);

} // namespace listbox
