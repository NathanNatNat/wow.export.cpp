/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "listbox.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <regex>
#include <filesystem>
#include <sstream>

#include "../core.h"

namespace listbox {

static constexpr int FILTER_DEBOUNCE_MS = 200;

static std::string fid_filter(const std::string& e) {
	const auto start = e.find(" [");
	const auto end = e.rfind(']');

	if (start != std::string::npos && end != std::string::npos)
		return e.substr(start + 2, end - (start + 2));

	return e;
}

/**
 * items: Item entries displayed in the list.
 * filter: Optional reactive filter for items.
 * selection: Reactive selection controller.
 * single: If set, only one entry can be selected.
 * keyinput: If true, listbox registers for keyboard input.
 * regex: If true, filter will be treated as a regular expression.
 * copymode: Defines the behavior of CTRL + C.
 * pasteselection: If true, CTRL + V will load a selection.
 * copytrimwhitespace: If true, whitespace is trimmed from copied paths.
 * includefilecount: If true, includes a file counter on the component.
 * unittype: Unit name for what the listbox contains. Used with includefilecount.
 * override: If provided, used as an override listfile.
 * disable: If provided, used as reactive disable flag.
 * persistscrollkey: If provided, enables scroll position persistence with this key.
 * quickfilters: Array of file extensions for quick filter links (e.g., ['m2', 'wmo']).
 */
// props: ['items', 'filter', 'selection', 'single', 'keyinput', 'regex', 'copymode', 'pasteselection', 'copytrimwhitespace', 'includefilecount', 'unittype', 'override', 'disable', 'persistscrollkey', 'quickfilters', 'nocopy']
// emits: ['update:selection', 'update:filter', 'contextmenu']

/**
 * Reactive instance data.
 */
// data: scroll, scrollRel, isScrolling, slotCount, lastSelectItem, debouncedFilter,
//       filterTimeout, scrollPositionRestored, activeQuickFilter — stored in ListboxState

/**
 * Invoked when the component is mounted.
 * Used to register global listeners and resize observer.
 */
// TODO(conversion): In ImGui, global mouse listeners and ResizeObserver are not needed.
// ImGui provides mouse state via ImGui::GetIO() and resizing is handled by layout each frame.

/**
 * Invoked when the component is activated (keep-alive).
 * Registers keyboard and paste listeners.
 */
// TODO(conversion): In ImGui, activation/deactivation is handled by the caller controlling
// whether render() is called each frame.

/**
 * Invoked when the component is deactivated (keep-alive).
 * Unregisters keyboard and paste listeners.
 */
// TODO(conversion): No explicit deactivation needed in ImGui immediate mode.

/**
 * Invoked when the component is destroyed.
 * Used to unregister global mouse listeners and resize observer.
 */
// TODO(conversion): No explicit unmount needed in ImGui immediate mode.
// Scroll position saving is done each frame when persistscrollkey is set.

/**
 * Offset of the scroll widget in pixels.
 * Between 0 and the height of the component.
 */
static float scrollOffset(const ListboxState& state) {
	return state.scroll;
}

/**
 * Index which array reading should start at, based on the current
 * relative scroll and the overal item count. Value is dynamically
 * capped based on slot count to prevent empty slots appearing.
 */
static int scrollIndex(const std::vector<std::string>& filteredItems, const ListboxState& state) {
	return static_cast<int>(std::round((static_cast<float>(filteredItems.size()) - static_cast<float>(state.slotCount)) * state.scrollRel));
}

/**
 * Returns the active item list to use.
 */
static const std::vector<std::string>& itemList(const std::vector<std::string>& items,
                                                  const std::vector<std::string>* overrideItems) {
	if (overrideItems != nullptr && !overrideItems->empty())
		return *overrideItems;
	return items;
}

/**
 * Weight (0-1) of a single item.
 */
static float itemWeight(const std::vector<std::string>& filteredItems) {
	if (filteredItems.empty())
		return 0.0f;
	return 1.0f / static_cast<float>(filteredItems.size());
}

/**
 * Invoked by a ResizeObserver when the main component node
 * is resized due to layout changes.
 */
static void resize(float containerHeight, float scrollerHeight, ListboxState& state) {
	state.scroll = (containerHeight - scrollerHeight) * state.scrollRel;
	state.slotCount = static_cast<int>(std::floor(containerHeight / 26.0f));
}

/**
 * Restricts the scroll offset to prevent overflowing and
 * calculates the relative (0-1) offset based on the scroll.
 */
static void recalculateBounds(float containerHeight, float scrollerHeight, ListboxState& state,
                               const std::string& persistscrollkey,
                               const std::vector<std::string>& filteredItems) {
	const float max = containerHeight - scrollerHeight;
	state.scroll = std::min(max, std::max(0.0f, state.scroll));
	state.scrollRel = (max > 0.0f) ? (state.scroll / max) : 0.0f;

	if (!persistscrollkey.empty())
		core::saveScrollPosition(persistscrollkey, static_cast<double>(state.scrollRel), scrollIndex(filteredItems, state));
}

/**
 * Invoked when a mouse-down event is captured on the scroll widget.
 * @param {MouseEvent} e
 */
static void startMouse(float mouseY, ListboxState& state) {
	state.scrollStartY = mouseY;
	state.scrollStart = state.scroll;
	state.isScrolling = true;
}

/**
 * Invoked when a mouse-move event is captured globally.
 * @param {MouseEvent} e
 */
static void moveMouse(float mouseY, float containerHeight, float scrollerHeight, ListboxState& state,
                       const std::string& persistscrollkey, const std::vector<std::string>& filteredItems) {
	if (state.isScrolling) {
		state.scroll = state.scrollStart + (mouseY - state.scrollStartY);
		recalculateBounds(containerHeight, scrollerHeight, state, persistscrollkey, filteredItems);
	}
}

/**
 * Invoked when a mouse-up event is captured globally.
 */
static void stopMouse(ListboxState& state) {
	state.isScrolling = false;
}

/**
 * Invoked when a user attempts to paste a selection.
 * @param {ClipboardEvent} e
 */
static void handlePaste(bool disable, bool pasteselection,
                          const std::vector<std::string>& sourceItems,
                          const std::vector<std::string>& selection,
                          const std::function<void(const std::vector<std::string>&)>& onSelectionChanged) {
	if (disable)
		return;

	// Paste selection must be enabled for this feature.
	if (!pasteselection)
		return;

	// Replace the current selection with one from the clipboard.
	const char* clipText = ImGui::GetClipboardText();
	if (!clipText)
		return;

	std::string text(clipText);
	std::vector<std::string> entries;
	std::istringstream stream(text);
	std::string line;
	while (std::getline(stream, line)) {
		// Remove trailing \r if present.
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (std::find(sourceItems.begin(), sourceItems.end(), line) != sourceItems.end())
			entries.push_back(line);
	}

	if (onSelectionChanged)
		onSelectionChanged(entries);
}

/**
 * Invoked when a mouse-wheel event is captured on the component node.
 * @param {WheelEvent} e
 */
static void wheelMouse(float wheelDelta, float containerHeight, float scrollerHeight, float itemHeight,
                        const std::vector<std::string>& filteredItems, ListboxState& state,
                        const std::string& persistscrollkey) {
	const float weight = containerHeight - scrollerHeight;

	if (itemHeight > 0.0f) {
		int scrollSpeed = 0;
		if (core::view)
			scrollSpeed = core::view->config.value("scrollSpeed", 0);

		const int scrollCount = (scrollSpeed == 0)
			? static_cast<int>(std::floor(containerHeight / itemHeight))
			: scrollSpeed;
		const float direction = wheelDelta > 0.0f ? 1.0f : -1.0f;
		state.scroll += ((static_cast<float>(scrollCount) * itemWeight(filteredItems)) * weight) * direction;
		recalculateBounds(containerHeight, scrollerHeight, state, persistscrollkey, filteredItems);
	}
}

/**
 * Helper: check if a string is in a vector.
 */
static bool isSelected(const std::vector<std::string>& selection, const std::string& item) {
	return std::find(selection.begin(), selection.end(), item) != selection.end();
}

/**
 * Helper: find index of string in vector, or -1.
 */
static int indexOf(const std::vector<std::string>& vec, const std::string& item) {
	auto it = std::find(vec.begin(), vec.end(), item);
	if (it != vec.end())
		return static_cast<int>(std::distance(vec.begin(), it));
	return -1;
}

/**
 * Remove all whitespace from a string.
 */
static std::string removeWhitespace(const std::string& s) {
	std::string result;
	result.reserve(s.size());
	for (char c : s) {
		if (!std::isspace(static_cast<unsigned char>(c)))
			result += c;
	}
	return result;
}

/**
 * Convert a string to lowercase.
 */
static std::string toLower(const std::string& s) {
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(),
	               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

/**
 * Trim whitespace from both ends of a string.
 */
static std::string trim(const std::string& s) {
	auto start = s.find_first_not_of(" \t\n\r\f\v");
	if (start == std::string::npos)
		return "";
	auto end = s.find_last_not_of(" \t\n\r\f\v");
	return s.substr(start, end - start + 1);
}

/**
 * Invoked when a keydown event is fired.
 * @param {KeyboardEvent} e
 */
static void handleKey(const std::vector<std::string>& filteredItems,
                       const std::vector<std::string>& selection,
                       bool single, bool disable, bool nocopy, CopyMode copymode,
                       bool copytrimwhitespace, ListboxState& state,
                       const std::function<void(const std::vector<std::string>&)>& onSelectionChanged,
                       float containerHeight, float scrollerHeight,
                       const std::string& persistscrollkey) {
	const ImGuiIO& io = ImGui::GetIO();

	// If document.activeElement is the document body, then we can safely assume
	// the user is not focusing anything, and can intercept keyboard input.
	// TODO(conversion): In ImGui, we check if no item is active (no text input focused, etc.).
	if (ImGui::IsAnyItemActive())
		return;

	// User hasn't selected anything in the listbox yet.
	if (!state.hasLastSelectItem)
		return;

	if (ImGui::IsKeyPressed(ImGuiKey_C) && io.KeyCtrl) {
		if (nocopy)
			return;

		// Copy selection to clipboard.
		std::vector<std::string> entries = selection;
		if (copymode == CopyMode::DIR) {
			for (auto& e : entries)
				e = std::filesystem::path(e).parent_path().string();
		} else if (copymode == CopyMode::FID) {
			for (auto& e : entries)
				e = fid_filter(e);
		}

		// Remove whitespace from paths to keep consistency with exports.
		if (copytrimwhitespace) {
			for (auto& e : entries)
				e = removeWhitespace(e);
		}

		std::string clipText;
		for (size_t i = 0; i < entries.size(); ++i) {
			if (i > 0) clipText += '\n';
			clipText += entries[i];
		}
		ImGui::SetClipboardText(clipText.c_str());
	} else {
		if (disable)
			return;

		// Arrow keys.
		const bool isArrowUp = ImGui::IsKeyPressed(ImGuiKey_UpArrow);
		const bool isArrowDown = ImGui::IsKeyPressed(ImGuiKey_DownArrow);
		if (isArrowUp || isArrowDown) {
			const int delta = isArrowUp ? -1 : 1;

			// Move/expand selection one.
			const int lastSelectIndex = indexOf(filteredItems, state.lastSelectItem);
			const int nextIndex = lastSelectIndex + delta;
			if (nextIndex >= 0 && nextIndex < static_cast<int>(filteredItems.size())) {
				const std::string& next = filteredItems[static_cast<size_t>(nextIndex)];
				const int currentScrollIdx = scrollIndex(filteredItems, state);
				const int lastViewIndex = isArrowUp ? currentScrollIdx : currentScrollIdx + state.slotCount;
				int diff = std::abs(nextIndex - lastViewIndex);
				if (isArrowDown)
					diff += 1;

				if ((isArrowUp && nextIndex < lastViewIndex) || (isArrowDown && nextIndex >= lastViewIndex)) {
					const float weight = containerHeight - scrollerHeight;
					state.scroll += ((static_cast<float>(diff) * itemWeight(filteredItems)) * weight) * static_cast<float>(delta);
					recalculateBounds(containerHeight, scrollerHeight, state, persistscrollkey, filteredItems);
				}

				std::vector<std::string> newSelection = selection;

				if (!io.KeyShift || single)
					newSelection.clear();

				newSelection.push_back(next);
				state.lastSelectItem = next;
				state.hasLastSelectItem = true;
				if (onSelectionChanged)
					onSelectionChanged(newSelection);
			}
		}
	}
}

/**
 * Invoked when a user selects an item in the list.
 * @param {string} item
 * @param {MouseEvent} e
 */
static void selectItem(const std::string& item, bool ctrlKey, bool shiftKey,
                         const std::vector<std::string>& filteredItems,
                         const std::vector<std::string>& selection,
                         bool single, bool disable, ListboxState& state,
                         const std::function<void(const std::vector<std::string>&)>& onSelectionChanged) {
	if (disable)
		return;

	const int checkIndex = indexOf(selection, item);
	std::vector<std::string> newSelection = selection;

	if (single) {
		// Listbox is in single-entry mode, replace selection.
		if (checkIndex == -1) {
			newSelection.clear();
			newSelection.push_back(item);
		}

		state.lastSelectItem = item;
		state.hasLastSelectItem = true;
	} else {
		if (ctrlKey) {
			// Ctrl-key held, so allow multiple selections.
			if (checkIndex > -1)
				newSelection.erase(newSelection.begin() + checkIndex);
			else
				newSelection.push_back(item);
		} else if (shiftKey) {
			// Shift-key held, select a range.
			if (state.hasLastSelectItem && state.lastSelectItem != item) {
				const int lastSelectIndex = indexOf(filteredItems, state.lastSelectItem);
				const int thisSelectIndex = indexOf(filteredItems, item);

				const int rangeLen = std::abs(lastSelectIndex - thisSelectIndex);
				const int lowest = std::min(lastSelectIndex, thisSelectIndex);

				for (int i = lowest; i <= lowest + rangeLen; ++i) {
					if (i >= 0 && i < static_cast<int>(filteredItems.size())) {
						const std::string& select = filteredItems[static_cast<size_t>(i)];
						if (indexOf(newSelection, select) == -1)
							newSelection.push_back(select);
					}
				}
			}
		} else if (checkIndex == -1 || (checkIndex > -1 && static_cast<int>(newSelection.size()) > 1)) {
			// Normal click, replace entire selection.
			newSelection.clear();
			newSelection.push_back(item);
		}

		state.lastSelectItem = item;
		state.hasLastSelectItem = true;
	}

	if (onSelectionChanged)
		onSelectionChanged(newSelection);
}

/**
 * Invoked when a quick filter link is clicked.
 * @param {string} ext - File extension (e.g., 'm2', 'wmo')
 */
static void applyQuickFilter(const std::string& ext, ListboxState& state) {
	if (state.activeQuickFilter == ext)
		state.activeQuickFilter.clear();
	else
		state.activeQuickFilter = ext;
}

/**
 * Invoked when a user right-clicks an item in the list.
 * @param {string} item
 * @param {MouseEvent} event
 */
static void handleContextMenu(const std::string& item,
                                const std::vector<std::string>& selection,
                                bool disable, ListboxState& state,
                                const std::function<void(const std::vector<std::string>&)>& onSelectionChanged,
                                const std::function<void(const ContextMenuEvent&)>& onContextMenu) {
	if (disable)
		return;

	// select item if not already in selection
	if (!isSelected(selection, item)) {
		std::vector<std::string> newSelection;
		newSelection.push_back(item);
		state.lastSelectItem = item;
		state.hasLastSelectItem = true;
		if (onSelectionChanged)
			onSelectionChanged(newSelection);
	}

	if (onContextMenu) {
		ContextMenuEvent evt;
		evt.item = item;
		evt.selection = isSelected(selection, item) ? selection : std::vector<std::string>{item};
		onContextMenu(evt);
	}
}

/**
 * Compute the filtered items list.
 * Reactively filtered version of the underlying data array.
 * Uses debounced filter to prevent UI stuttering on large datasets.
 */
static std::vector<std::string> computeFilteredItems(
    const std::vector<std::string>& sourceItems,
    const std::string& debouncedFilter,
    bool useRegex,
    const std::string& activeQuickFilter,
    const std::vector<std::string>& selection,
    const std::function<void(const std::vector<std::string>&)>& onSelectionChanged) {

	std::vector<std::string> res;
	res.reserve(sourceItems.size());

	// Start with all items.
	for (const auto& item : sourceItems)
		res.push_back(item);

	// apply text filter
	if (!debouncedFilter.empty()) {
		if (useRegex) {
			try {
				std::regex filter(trim(debouncedFilter), std::regex_constants::icase);
				std::vector<std::string> filtered;
				for (const auto& e : res) {
					if (std::regex_search(e, filter))
						filtered.push_back(e);
				}
				res = std::move(filtered);
			} catch (const std::regex_error&) {
				// Regular expression did not compile, skip filtering.
			}
		} else {
			const std::string filter = toLower(trim(debouncedFilter));
			if (!filter.empty()) {
				std::vector<std::string> filtered;
				for (const auto& e : res) {
					if (toLower(e).find(filter) != std::string::npos)
						filtered.push_back(e);
				}
				res = std::move(filtered);
			}
		}
	}

	// apply quick filter
	if (!activeQuickFilter.empty()) {
		try {
			std::string pattern = "\\." + toLower(activeQuickFilter) + "(\\s\\[\\d+\\])?$";
			std::regex quickRegex(pattern, std::regex_constants::icase);
			std::vector<std::string> filtered;
			for (const auto& e : res) {
				if (std::regex_search(e, quickRegex))
					filtered.push_back(e);
			}
			res = std::move(filtered);
		} catch (const std::regex_error&) {
			// Regex compilation failed, skip quick filtering.
		}
	}

	// Prune selection to only include items still in filtered results.
	bool hasChanges = false;
	std::vector<std::string> newSelection;
	for (const auto& item : selection) {
		if (std::find(res.begin(), res.end(), item) != res.end()) {
			newSelection.push_back(item);
		} else {
			hasChanges = true;
		}
	}

	if (hasChanges && onSelectionChanged)
		onSelectionChanged(newSelection);

	return res;
}

/**
 * HTML mark-up to render for this component.
 */
// template: converted to ImGui immediate-mode rendering below.

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
            const std::function<void(const ContextMenuEvent&)>& onContextMenu) {
	ImGui::PushID(id);

	// --- watch: filter (debounce) ---
	if (filter != state.prevFilter) {
		state.prevFilter = filter;
		state.filterTimeoutStart = std::chrono::steady_clock::now();
	}

	// Process debounce timeout.
	if (state.filterTimeoutStart.has_value()) {
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - *state.filterTimeoutStart).count();
		if (elapsed >= FILTER_DEBOUNCE_MS) {
			state.debouncedFilter = filter;
			state.filterTimeoutStart.reset();
		}
	}

	// Initialize debounced filter on first frame.
	if (!state.initialized) {
		state.debouncedFilter = filter;
		state.initialized = true;
	}

	// Get the active item list (override or items).
	const auto& sourceItems = itemList(items, overrideItems);

	// Compute filtered items.
	const std::vector<std::string> filteredItems = computeFilteredItems(
		sourceItems, state.debouncedFilter, regex, state.activeQuickFilter,
		selection, onSelectionChanged);

	// --- mounted/watch: persistscrollkey scroll position restoration ---
	if (!persistscrollkey.empty() && !filteredItems.empty() && !state.scrollPositionRestored) {
		auto saved_state = core::getScrollPosition(persistscrollkey);
		if (saved_state.has_value()) {
			state.scrollRel = static_cast<float>(saved_state->scrollRel);
			state.scrollPositionRestored = true;
		}
	}

	// Get the available content region as the container dimensions.
	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const float containerHeight = availSize.y;

	// The scroller thumb height is proportional to visible vs total items.
	const float itemHeight = 26.0f;
	const int totalItems = static_cast<int>(filteredItems.size());
	const float scrollerHeight = (totalItems > 0)
		? std::max(20.0f, containerHeight * (static_cast<float>(state.slotCount) / static_cast<float>(totalItems)))
		: containerHeight;

	// Equivalent of resize() — recalculate slot count and scroll each frame.
	resize(containerHeight, scrollerHeight, state);

	// Compute display range.
	const int idx = scrollIndex(filteredItems, state);
	const int startIdx = std::max(0, idx);
	const int endIdx = std::min(totalItems, startIdx + state.slotCount);

	// Begin a child region to contain the list.
	// <div ref="root" class="ui-listbox" @wheel="wheelMouse">
	ImGui::BeginChild("##listbox_container", ImVec2(availSize.x, containerHeight), ImGuiChildFlags_None,
	                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Handle mouse wheel on the container.
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None)) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			wheelMouse(-wheel, containerHeight, scrollerHeight, itemHeight, filteredItems, state, persistscrollkey);
		}
	}

	// Handle global mouse move/up for scrollbar dragging.
	const ImGuiIO& io = ImGui::GetIO();
	if (state.isScrolling) {
		moveMouse(io.MousePos.y, containerHeight, scrollerHeight, state, persistscrollkey, filteredItems);
		if (!io.MouseDown[0]) {
			stopMouse(state);
		}
	}

	// Handle paste (Ctrl+V).
	if (pasteselection && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
		handlePaste(disable, pasteselection, sourceItems, selection, onSelectionChanged);
	}

	// Handle keyboard input.
	if (keyinput) {
		handleKey(filteredItems, selection, single, disable, nocopy, copymode,
		          copytrimwhitespace, state, onSelectionChanged,
		          containerHeight, scrollerHeight, persistscrollkey);
	}

	// Draw the scrollbar on the right side.
	// <div class="scroller" ref="scroller" @mousedown="startMouse" :class="{ using: isScrolling }" :style="{ top: scrollOffset }"><div></div></div>
	{
		const ImVec2 winPos = ImGui::GetWindowPos();
		const float scrollbarWidth = 8.0f;
		const float scrollbarX = winPos.x + availSize.x - scrollbarWidth;
		const float thumbY = winPos.y + scrollOffset(state);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Scroller thumb.
		const ImVec2 thumbMin(scrollbarX, thumbY);
		const ImVec2 thumbMax(scrollbarX + scrollbarWidth, thumbY + scrollerHeight);

		// Determine if mouse is over the thumb for hover effect.
		const bool thumbHovered = ImGui::IsMouseHoveringRect(thumbMin, thumbMax) || state.isScrolling;
		const ImU32 thumbColor = thumbHovered
			? IM_COL32(255, 255, 255, 180)
			: IM_COL32(255, 255, 255, 80);

		drawList->AddRectFilled(thumbMin, thumbMax, thumbColor, 4.0f);

		// Handle mouse-down on the scroller thumb.
		if (ImGui::IsMouseHoveringRect(thumbMin, thumbMax) && ImGui::IsMouseClicked(0)) {
			startMouse(io.MousePos.y, state);
		}
	}

	// Render visible items.
	// <div v-for="(item, i) in displayItems" class="item" @click="selectItem(item, $event)" @contextmenu="handleContextMenu(item, $event)" :class="{ selected: selection.includes(item) }">
	//     <span v-for="(sub, si) in item.split('\\31')" :class="'sub sub-' + si" :data-item="sub">{{ sub }}</span>
	// </div>
	for (int i = startIdx; i < endIdx; ++i) {
		const std::string& item = filteredItems[static_cast<size_t>(i)];
		const bool itemSelected = isSelected(selection, item);

		// Selected highlight (:class="{ selected: selection.includes(item) }").
		if (itemSelected) {
			const ImVec2 rowMin = ImGui::GetCursorScreenPos();
			const ImVec2 rowMax(rowMin.x + availSize.x - 10.0f, rowMin.y + itemHeight);
			ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, IM_COL32(34, 181, 73, 40));
		}

		ImGui::PushID(i);

		// Split item by '\x19' (character 0x19, used as sub-field separator).
		// <span v-for="(sub, si) in item.split('\\31')" :class="'sub sub-' + si" :data-item="sub">{{ sub }}</span>
		// Note: JS '\\31' is octal for char 0x19 (ASCII 25).
		std::string displayText;
		{
			size_t pos = 0;
			size_t found;
			bool first = true;
			while ((found = item.find('\x19', pos)) != std::string::npos) {
				if (!first) displayText += ' ';
				displayText += item.substr(pos, found - pos);
				pos = found + 1;
				first = false;
			}
			if (!first) displayText += ' ';
			displayText += item.substr(pos);
		}

		// Clicking the row selects the item; right-clicking opens context menu.
		if (ImGui::Selectable(displayText.c_str(), itemSelected, ImGuiSelectableFlags_None,
		                      ImVec2(availSize.x - 10.0f, 0.0f))) {
			selectItem(item, io.KeyCtrl, io.KeyShift, filteredItems, selection,
			           single, disable, state, onSelectionChanged);
		}

		// Context menu on right-click.
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			handleContextMenu(item, selection, disable, state, onSelectionChanged, onContextMenu);
		}

		ImGui::PopID();
	}

	ImGui::EndChild();

	// <div class="list-status" v-if="unittype" :class="{ 'with-quick-filters': quickfilters && quickfilters.length > 0 }">
	//     <span>{{ filteredItems.length }} {{ unittype + (filteredItems.length != 1 ? 's' : '') }} found. {{ selection.length > 0 ? ' (' + selection.length + ' selected)' : '' }}</span>
	//     <span v-if="quickfilters && quickfilters.length > 0" class="quick-filters">
	//         Quick filter: <template v-for="(ext, index) in quickfilters" :key="ext">
	//             <a @click="applyQuickFilter(ext)" :class="{ active: activeQuickFilter === ext }">{{ ext.toUpperCase() }}</a>
	//             <span v-if="index < quickfilters.length - 1"> / </span>
	//         </template>
	//     </span>
	// </div>
	if (!unittype.empty()) {
		std::string statusText = std::to_string(filteredItems.size()) + " " + unittype;
		if (filteredItems.size() != 1)
			statusText += "s";
		statusText += " found.";
		if (!selection.empty())
			statusText += " (" + std::to_string(selection.size()) + " selected)";

		ImGui::Text("%s", statusText.c_str());

		// Quick filters.
		if (!quickfilters.empty()) {
			ImGui::SameLine();
			ImGui::Text("Quick filter:");
			for (size_t qi = 0; qi < quickfilters.size(); ++qi) {
				ImGui::SameLine();
				const std::string& ext = quickfilters[qi];
				std::string upperExt = ext;
				std::transform(upperExt.begin(), upperExt.end(), upperExt.begin(),
				               [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

				const bool isActive = (state.activeQuickFilter == ext);
				if (isActive)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.13f, 0.71f, 0.29f, 1.0f));

				if (ImGui::SmallButton(upperExt.c_str()))
					applyQuickFilter(ext, state);

				if (isActive)
					ImGui::PopStyleColor();

				if (qi < quickfilters.size() - 1) {
					ImGui::SameLine();
					ImGui::Text("/");
				}
			}
		}
	}

	ImGui::PopID();
}

} // namespace listbox
