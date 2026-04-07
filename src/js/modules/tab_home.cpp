/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_home.h"
#include "../core.h"

#include <imgui.h>

namespace tab_home {

/**
 * Render the home tab widget using ImGui.
 *
 * template:
 *   <div class="tab" id="tab-home">
 *       <div id="home-changes">
 *           <div v-html="$core.view.whatsNewHTML"></div>
 *       </div>
 *       <div id="home-help-buttons">
 *           <div data-external="::DISCORD">
 *               <b>Stuck? Need Help?</b>
 *               <span>Join our Discord community for support!</span>
 *           </div>
 *           <div data-external="::GITHUB">
 *               <b>Gnomish Heritage?</b>
 *               <span>wow.export is open-source, tinkerers are welcome!</span>
 *           </div>
 *           <div data-external="::PATREON">
 *               <b>Support Us!</b>
 *               <span>Support development of wow.export through Patreon!</span>
 *           </div>
 *       </div>
 *   </div>
 */
void render() {
	// #home-changes — render whatsNewHTML content.
	ImGui::BeginChild("home-changes", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_None);
	ImGui::TextWrapped("%s", core::view->whatsNewHTML.c_str());
	ImGui::EndChild();

	// #home-help-buttons — three help/support buttons.
	ImGui::Separator();

	// Discord button
	if (ImGui::Button("Stuck? Need Help?\nJoin our Discord community for support!")) {
		// TODO(conversion): data-external="::DISCORD" — external link opening will be wired when external-links module is available.
	}

	ImGui::SameLine();

	// GitHub button
	if (ImGui::Button("Gnomish Heritage?\nwow.export is open-source, tinkerers are welcome!")) {
		// TODO(conversion): data-external="::GITHUB" — external link opening will be wired when external-links module is available.
	}

	ImGui::SameLine();

	// Patreon button
	if (ImGui::Button("Support Us!\nSupport development of wow.export through Patreon!")) {
		// TODO(conversion): data-external="::PATREON" — external link opening will be wired when external-links module is available.
	}
}

void navigate(const char* /*module_name*/) {
	// JS: this.$modules[module_name].setActive();
	// TODO(conversion): Module activation will be wired when the module system is integrated.
}

} // namespace tab_home
