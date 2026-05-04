/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "item-picker-modal.h"
#include "../core.h"
#include "../icon-render.h"
#include "../modules/tab_items.h"
#include "../wow/equip-item.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <format>
#include <functional>
#include <string>
#include <vector>

#include <imgui.h>

namespace item_picker_modal {

struct SlotFilterEntry {
	const char* label;
	std::vector<int> type_ids;
};

static const SlotFilterEntry ITEM_SLOTS_MERGED[] = {
	{ "Head",      { 1 } },
	{ "Neck",      { 2 } },
	{ "Shoulder",  { 3 } },
	{ "Shirt",     { 4 } },
	{ "Chest",     { 5, 20 } },
	{ "Waist",     { 6 } },
	{ "Legs",      { 7 } },
	{ "Feet",      { 8 } },
	{ "Wrist",     { 9 } },
	{ "Hands",     { 10 } },
	{ "One-hand",  { 13 } },
	{ "Off-hand",  { 14, 22, 23 } },
	{ "Two-hand",  { 17 } },
	{ "Main-hand", { 21 } },
	{ "Ranged",    { 15, 26 } },
	{ "Back",      { 16 } },
	{ "Tabard",    { 19 } }
};

static bool s_is_open = false;
static int  s_slot_id = 0;
static std::string s_slot_filter;

static char s_filter_buf[256] = {};
static int  s_scroll_offset   = 0;

static bool s_is_loading = false;
static bool s_load_error = false;

static std::function<void()> s_on_open_items_tab;

static bool s_just_opened = false;

static std::string s_last_filter;
static int         s_last_slot_id = -1;
static const tab_items::ItemData* s_last_items_ptr = nullptr;
static std::vector<const tab_items::ItemData*> s_filtered_items;

static constexpr int PAGE_SIZE = 10;

static void rebuild_filtered(const tab_items::ItemData* items_ptr, size_t items_count) {
	const std::vector<int>* type_ids = nullptr;
	for (const auto& entry : ITEM_SLOTS_MERGED) {
		if (s_slot_filter == entry.label) {
			type_ids = &entry.type_ids;
			break;
		}
	}

	std::string filter_lower(s_filter_buf);
	{
		static constexpr const char* WS = " \t\n\r\f\v";
		const auto first = filter_lower.find_first_not_of(WS);
		if (first == std::string::npos) {
			filter_lower.clear();
		} else {
			const auto last = filter_lower.find_last_not_of(WS);
			filter_lower = filter_lower.substr(first, last - first + 1);
		}
	}
	std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(),
	               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	s_filtered_items.clear();
	for (size_t i = 0; i < items_count; ++i) {
		const auto& item = items_ptr[i];

		if (type_ids) {
			bool match = false;
			for (int tid : *type_ids) {
				if (item.inventoryType == tid) { match = true; break; }
			}
			if (!match)
				continue;
		}

		if (!filter_lower.empty()) {
			std::string name_lower = item.displayName;
			std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
			               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (name_lower.find(filter_lower) == std::string::npos)
				continue;
		}

		s_filtered_items.push_back(&item);
	}
}

static void close_modal() {
	s_is_open = false;
	ImGui::CloseCurrentPopup();
}

void open(int slot_id, const std::string& slot_filter,
          std::function<void()> on_open_items_tab) {
	s_is_open           = true;
	s_slot_id           = slot_id;
	s_slot_filter       = slot_filter;
	s_on_open_items_tab = std::move(on_open_items_tab);
	s_just_opened       = true;
	std::memset(s_filter_buf, 0, sizeof(s_filter_buf));
	s_scroll_offset = 0;
	s_last_filter    = "";
	s_last_slot_id   = -1;
	s_last_items_ptr = nullptr;
	s_is_loading = false;
	s_load_error = false;
}

bool is_open() {
	return s_is_open;
}

void render() {
	if (!s_is_open)
		return;

	const ImGuiViewport* vp = ImGui::GetMainViewport();
	ImGui::GetBackgroundDrawList()->AddRectFilled(
		vp->Pos,
		ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y),
		IM_COL32(0, 0, 0, 128));

	bool was_just_opened = s_just_opened;
	if (s_just_opened) {
		ImGui::OpenPopup("Select Item##item-picker-modal");
		s_just_opened = false;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(480, 400), ImGuiCond_Appearing);

	bool modal_open = true;
	if (!ImGui::BeginPopupModal("Select Item##item-picker-modal", &modal_open,
	                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
		if (!modal_open)
			s_is_open = false;
		return;
	}

	if (!modal_open) {
		s_is_open = false;
		ImGui::EndPopup();
		return;
	}

	if (!was_just_opened && ImGui::IsMouseClicked(0) &&
	    !ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
		close_modal();
		ImGui::EndPopup();
		return;
	}

	const std::vector<tab_items::ItemData>* all_items = tab_items::getAllItems();
	const tab_items::ItemData* items_ptr   = all_items ? all_items->data() : nullptr;
	size_t                     items_count = all_items ? all_items->size() : 0;

	if (!items_ptr) {
		if (!s_load_error) {
			s_is_loading = true;
			tab_items::mounted();
		}
	} else {
		s_is_loading = false;
	}

	bool filter_changed = (std::strcmp(s_filter_buf, s_last_filter.c_str()) != 0);
	bool slot_changed   = (s_slot_id != s_last_slot_id);
	bool data_changed   = (items_ptr != s_last_items_ptr);

	if (filter_changed || slot_changed || data_changed) {
		if (filter_changed)
			s_scroll_offset = 0;
		rebuild_filtered(items_ptr, items_count);
		s_last_filter    = s_filter_buf;
		s_last_slot_id   = s_slot_id;
		s_last_items_ptr = items_ptr;
	}

	int result_count = static_cast<int>(s_filtered_items.size());

	ImGui::Text("Select Item");
	ImGui::SameLine();
	ImGui::Text("(%d items)", result_count);

	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##item-filter", "Search items...", s_filter_buf, sizeof(s_filter_buf));

	if (ImGui::IsWindowAppearing())
		ImGui::SetKeyboardFocusHere(-1);

	int start = std::clamp(s_scroll_offset, 0, std::max(0, result_count - PAGE_SIZE));
	int end   = std::min(start + PAGE_SIZE, result_count);

	for (int i = start; i < end; ++i) {
		if (s_filtered_items[static_cast<size_t>(i)]->icon != 0)
			icon_render::loadIcon(s_filtered_items[static_cast<size_t>(i)]->icon);
	}

	bool can_scroll_up   = s_scroll_offset > 0;
	bool can_scroll_down = s_scroll_offset + PAGE_SIZE < result_count;

	ImGui::BeginChild("##item-list", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2.0f - 8.0f),
	                   ImGuiChildFlags_Borders);

	if (s_is_loading) {
		ImGui::TextUnformatted("Loading items...");
	} else if (s_load_error) {
		ImGui::TextUnformatted("Failed to load items.");
	} else if (result_count == 0) {
		ImGui::TextUnformatted("No items found.");
	} else {
		static const ImVec4 quality_colors[] = {
			ImVec4(0.62f, 0.62f, 0.62f, 1.0f),
			ImVec4(1.00f, 1.00f, 1.00f, 1.0f),
			ImVec4(0.12f, 1.00f, 0.00f, 1.0f),
			ImVec4(0.00f, 0.44f, 0.87f, 1.0f),
			ImVec4(0.64f, 0.21f, 0.93f, 1.0f),
			ImVec4(1.00f, 0.50f, 0.00f, 1.0f),
			ImVec4(0.90f, 0.80f, 0.50f, 1.0f),
			ImVec4(0.00f, 0.80f, 1.00f, 1.0f),
		};

		for (int i = start; i < end; ++i) {
			const auto* item = s_filtered_items[static_cast<size_t>(i)];
			int quality = item->quality;
			bool has_color = (quality >= 0 && quality < static_cast<int>(std::size(quality_colors)));

			const uint32_t iconTex = icon_render::getIconTexture(item->icon);
			if (iconTex != 0) {
				ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(iconTex)),
				             ImVec2(24.0f, 24.0f));
				ImGui::SameLine();
			}

			if (has_color)
				ImGui::PushStyleColor(ImGuiCol_Text, quality_colors[quality]);

			bool clicked = ImGui::Selectable(
				std::format("{}  ({})##item_{}", item->name, item->id, item->id).c_str(),
				false);

			if (has_color)
				ImGui::PopStyleColor();

			if (clicked) {
				if (wow::equip_item(item->id, item->name, s_slot_id)) {
					close_modal();
					ImGui::EndChild();
					ImGui::EndPopup();
					return;
				}
			}
		}
	}

	if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
		float delta = ImGui::GetIO().MouseWheel;
		if (delta < 0.0f && can_scroll_down)
			s_scroll_offset = std::min(s_scroll_offset + 3, result_count - PAGE_SIZE);
		else if (delta > 0.0f && can_scroll_up)
			s_scroll_offset = std::max(s_scroll_offset - 3, 0);
	}

	ImGui::EndChild();

	if (ImGui::SmallButton("Search in Items Tab")) {
		if (s_on_open_items_tab)
			s_on_open_items_tab();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		close_modal();
		ImGui::EndPopup();
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
		close_modal();
		ImGui::EndPopup();
		return;
	}

	ImGui::EndPopup();
}

} // namespace item_picker_modal
