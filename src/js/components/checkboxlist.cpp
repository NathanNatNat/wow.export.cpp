/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "checkboxlist.h"

#include <imgui.h>
#include <string>

namespace checkboxlist {

/**
 * items: Item entries displayed in the list.
 */
// props: ['items']

/**
 * Reactive instance data.
 *
 * The JS component owns `scroll`, `scrollRel`, `isScrolling`, and `slotCount`
 * to drive a hand-rolled drag scroller and virtual list. The C++ port hands
 * scroll handling to a native ImGui child window with a built-in scrollbar,
 * so none of those fields are needed here.
 */

/**
 * Index which array reading should start at, based on the current
 * relative scroll and the overall item count. Value is dynamically
 * capped based on slot count to prevent empty slots appearing.
 *
 * The JS computed property never returns a negative value because `scrollRel`
 * is clamped to [0, 1] by `recalculateBounds()` and only multiplies a
 * non-negative `(items.length - slotCount)` once `items.length >= slotCount`.
 * Native ImGui scrolling (used below) makes the helper unnecessary; the JS
 * contract of "always >= 0" is preserved by simply not exposing one.
 */
// scrollIndex / displayItems / itemWeight / wheelMouse / startMouse / moveMouse /
// stopMouse / recalculateBounds / propagateClick — all subsumed by ImGui's
// native child-window scrolling and Selectable click handling.

/**
 * HTML mark-up to render for this component.
 *
 * Template:
 *   <div class="ui-checkboxlist" @wheel="wheelMouse">
 *     <div class="scroller" ...>...</div>
 *     <div v-for="(item, i) in displayItems" class="item" :class="{ selected: item.checked }">
 *       <input type="checkbox" v-model="item.checked"/>
 *       <span>{{ item.label }}</span>
 *     </div>
 *   </div>
 */
void render(const char* id, std::vector<nlohmann::json>& items, CheckboxListState& /*state*/) {
	ImGui::PushID(id);

	const ImVec2 availSize = ImGui::GetContentRegionAvail();

	// CSS: .ui-checkboxlist { border: 1px solid var(--border); }
	// ImGuiChildFlags_Borders draws the border using ImGuiCol_Border.
	// The native vertical scrollbar replaces the JS `.scroller` thumb, satisfying
	// the CLAUDE.md rule against using raw ImDrawList primitives for things
	// native widgets can do.
	ImGui::BeginChild("##checkboxlist_container", availSize, ImGuiChildFlags_Borders);

	// Render every item. Native ImGui scrolling clips invisible rows for us, so
	// the JS virtual-scroll machinery (scrollIndex / displayItems / slotCount) is
	// no longer required.
	const int totalItems = static_cast<int>(items.size());
	for (int i = 0; i < totalItems; ++i) {
		auto& item = items[static_cast<std::size_t>(i)];
		bool checked = item.value("checked", false);
		const std::string label = item.value("label", std::string(""));

		ImGui::PushID(i);

		// Capture row start so the checkbox + label can be overlaid on top of
		// the Selectable's selection background.
		const ImVec2 rowStart = ImGui::GetCursorPos();
		const float rowWidth = ImGui::GetContentRegionAvail().x;

		// CSS: .item.selected { background: var(--selection); } — JS adds the
		// `selected` class to the row when item.checked is true. ImGui paints
		// this background by passing `selected = checked` to Selectable, which
		// uses ImGuiCol_Header. AllowOverlap lets the checkbox/label widgets
		// rendered afterwards still receive clicks. Sized to match the
		// checkbox's frame height so the row advance stays consistent.
		const float rowHeight = ImGui::GetFrameHeight();
		if (ImGui::Selectable("##row", checked, ImGuiSelectableFlags_AllowOverlap, ImVec2(rowWidth, rowHeight))) {
			checked = !checked;
			item["checked"] = checked;
		}

		// Overlay the checkbox + label on top of the Selectable.
		// CSS: .item { padding: 2px 8px } → 8px left padding for checkbox and label.
		ImGui::SetCursorPos(ImVec2(rowStart.x + 8.0f, rowStart.y));
		if (ImGui::Checkbox("##cb", &checked))
			item["checked"] = checked;

		// CSS: .item span { margin: 0 0 1px 5px } → 5px left margin from checkbox to label.
		ImGui::SameLine(0.0f, 5.0f);
		ImGui::TextUnformatted(label.c_str());

		ImGui::PopID();
	}

	ImGui::EndChild();
	ImGui::PopID();
}

} // namespace checkboxlist
