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

#include "../core.h"

namespace listbox_zones {

/**
 * Extends the base listbox component with expansion filtering for zones.
 *
 * Additional props:
 * expansionFilter: Reactive value for filtering by expansion ID (-1 for all, 0+ for specific expansion)
 */

/**
 * Reactively filtered version of the underlying data array.
 * Applies both text filtering and expansion filtering.
 */

static std::vector<std::string> filterByExpansion(const std::vector<std::string>& items, int expansionFilter) {
	if (expansionFilter == -1)
		return items;

	std::vector<std::string> filtered;
	for (const auto& item : items) {
		// Extract expansion ID from the zone entry format
		const auto sepPos = item.find('\x19');
		if (sepPos != std::string::npos) {
			try {
				const int expansionId = std::stoi(item.substr(0, sepPos));
				if (expansionId == expansionFilter)
					filtered.push_back(item);
			} catch (...) {
			}
		}
	}
	return filtered;
}

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

	if (expansionFilter != state.prevExpansionFilter) {
		state.prevExpansionFilter = expansionFilter;
		state.base.scroll = 0.0f;
		state.base.scrollRel = 0.0f;
		if (!persistscrollkey.empty())
			core::saveScrollPosition(persistscrollkey, 0.0, 0);
	}

	const auto& resolvedItems = (overrideItems != nullptr && !overrideItems->empty())
		? *overrideItems : items;

	{
		const std::string* curData = resolvedItems.empty() ? nullptr : resolvedItems.data();
		const bool needRecompute =
			curData                   != state.cachedExpItemsData
			|| resolvedItems.size()   != state.cachedExpItemsSize
			|| expansionFilter        != state.cachedExpansionFilter;

		if (needRecompute) {
			state.cachedExpansionFiltered = filterByExpansion(resolvedItems, expansionFilter);
			state.cachedExpItemsData    = curData;
			state.cachedExpItemsSize    = resolvedItems.size();
			state.cachedExpansionFilter = expansionFilter;
		}
	}
	const std::vector<std::string>& expansionFiltered = state.cachedExpansionFiltered;

	listbox::render(id, expansionFiltered, filter, selection, single, keyinput, regex,
	                copymode, pasteselection, copytrimwhitespace, unittype, nullptr,
	                disable, persistscrollkey, quickfilters, nocopy, state.base,
	                onSelectionChanged, onContextMenu);
}

} // namespace listbox_zones