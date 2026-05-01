/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "checkboxlist.h"

#include <imgui.h>
#include <string>

namespace checkboxlist {

// props: ['items']

void render(const char* id, std::vector<nlohmann::json>& items, CheckboxListState& /*state*/) {
	ImGui::PushID(id);

	const ImVec2 availSize = ImGui::GetContentRegionAvail();

	ImGui::BeginChild("##checkboxlist_container", availSize, ImGuiChildFlags_Borders);

	const int totalItems = static_cast<int>(items.size());
	for (int i = 0; i < totalItems; ++i) {
		auto& item = items[static_cast<std::size_t>(i)];
		bool checked = item.value("checked", false);
		const std::string label = item.value("label", std::string(""));

		ImGui::PushID(i);

		const ImVec2 rowStart = ImGui::GetCursorPos();
		const float rowWidth = ImGui::GetContentRegionAvail().x;

		const float rowHeight = ImGui::GetFrameHeight();
		if (ImGui::Selectable("##row", checked, ImGuiSelectableFlags_AllowOverlap, ImVec2(rowWidth, rowHeight))) {
			checked = !checked;
			item["checked"] = checked;
		}

		ImGui::SetCursorPos(ImVec2(rowStart.x + 8.0f, rowStart.y));
		if (ImGui::Checkbox("##cb", &checked))
			item["checked"] = checked;

		ImGui::SameLine(0.0f, 5.0f);
		ImGui::TextUnformatted(label.c_str());

		ImGui::PopID();
	}

	ImGui::EndChild();
	ImGui::PopID();
}

} // namespace checkboxlist
