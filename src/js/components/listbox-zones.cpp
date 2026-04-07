/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "listbox-zones.h"

#include <string>
#include <vector>
#include <algorithm>
#include <regex>
#include <functional>

namespace listbox_zones {

/**
 * Extends the base listbox component with expansion filtering for zones.
 *
 * Additional props:
 * expansionFilter: Reactive value for filtering by expansion ID (-1 for all, 0+ for specific expansion)
 */
// props: [...listboxComponent.props, 'expansionFilter']
// emits: listboxComponent.emits

// data: same as listboxComponent.data — stored in ListboxZonesState.base

// mounted: same as listboxComponent.mounted
// beforeUnmount: same as listboxComponent.beforeUnmount

// watch: expansionFilter — resets scroll on change (handled below)
// All other watches inherited from listboxComponent.watch

// computed: scrollOffset, scrollIndex, itemList, displayItems, itemWeight — inherited from listboxComponent

/**
 * Reactively filtered version of the underlying data array.
 * Applies both text filtering and expansion filtering.
 *
 * This overrides listboxComponent.computed.filteredItems to add
 * expansion-based filtering before the text filter.
 */
// NOTE: The expansion filtering is integrated into the render() call below
// by pre-filtering items before passing to the base listbox render.

static std::vector<std::string> filterByExpansion(const std::vector<std::string>& items, int expansionFilter) {
	if (expansionFilter == -1)
		return items;

	std::vector<std::string> filtered;
	for (const auto& item : items) {
		// Extract expansion ID from the zone entry format.
		// Items are formatted as: "ExpansionID\x19[ZoneID]\x19ZoneName\x19(AreaName)"
		const auto sepPos = item.find('\x19');
		if (sepPos != std::string::npos) {
			try {
				const int expansionId = std::stoi(item.substr(0, sepPos));
				if (expansionId == expansionFilter)
					filtered.push_back(item);
			} catch (...) {
				// Parse failed, skip item.
			}
		}
	}
	return filtered;
}

// methods: inherited from listboxComponent.methods
// template: inherited from listboxComponent.template

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
            const std::function<void(const listbox::ContextMenuEvent&)>& onContextMenu) {

	// watch: expansionFilter — reset scroll on change.
	if (expansionFilter != state.prevExpansionFilter) {
		state.prevExpansionFilter = expansionFilter;
		state.base.scroll = 0.0f;
		state.base.scrollRel = 0.0f;
	}

	// Pre-filter items by expansion before passing to base listbox.
	const std::vector<std::string> expansionFiltered = filterByExpansion(items, expansionFilter);

	// Delegate to the base listbox render with expansion-filtered items.
	listbox::render(id, expansionFiltered, filter, selection, single, keyinput, regex,
	                copymode, pasteselection, copytrimwhitespace, unittype, overrideItems,
	                disable, persistscrollkey, quickfilters, nocopy, state.base,
	                onSelectionChanged, onContextMenu);
}

} // namespace listbox_zones