/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_home.h"
#include "../modules.h"
#include "../core.h"

#include <imgui.h>

namespace tab_home {

/**
 * Render the home tab widget using ImGui.
 * JS template:
 *   <div class="tab" id="tab-home">
 *     <div id="home-changes"><div v-html="$core.view.whatsNewHTML"></div></div>
 *     <div id="home-help-buttons"> ... </div>
 *   </div>
 */
void render() {
	// #home-changes: display "What's New" content.
	if (!core::view->whatsNewHTML.empty()) {
		ImGui::TextWrapped("%s", core::view->whatsNewHTML.c_str());
		ImGui::Separator();
	}

	// #home-help-buttons: three informational help buttons.
	// JS: data-external="::DISCORD" / "::GITHUB" / "::PATREON"
	// ExternalLinks module was deleted; buttons render as informational text.
	ImGui::Spacing();

	if (ImGui::Button("Stuck? Need Help?##discord")) {
		// ExternalLinks.open('::DISCORD') — Removed: external-links module deleted
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Join our Discord community for support!");

	if (ImGui::Button("Gnomish Heritage?##github")) {
		// ExternalLinks.open('::GITHUB') — Removed: external-links module deleted
	}
	ImGui::SameLine();
	ImGui::TextDisabled("wow.export is open-source, tinkerers are welcome!");

	if (ImGui::Button("Support Us!##patreon")) {
		// ExternalLinks.open('::PATREON') — Removed: external-links module deleted
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Support development of wow.export through Patreon!");
}

void navigate(const char* module_name) {
	// JS: this.$modules[module_name].setActive();
	modules::set_active(module_name);
}

} // namespace tab_home
