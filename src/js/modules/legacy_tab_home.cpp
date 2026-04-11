/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "legacy_tab_home.h"
#include "tab_home.h"
#include "../modules.h"
#include "../core.h"

#include <imgui.h>

namespace legacy_tab_home {

/**
 * Render the legacy home tab widget using ImGui.
 * JS template:
 *   <div class="tab" id="legacy-tab-home">
 *     <HomeShowcase />
 *     <div id="home-changes"><div v-html="$core.view.whatsNewHTML"></div></div>
 *     <div id="home-help-buttons"> ... </div>
 *   </div>
 *
 * CSS: #tab-home, #legacy-tab-home share the same grid layout.
 * Uses the same renderHomeLayout() as tab_home.
 */
void render() {
	// JS: #legacy-tab-home uses the same layout as #tab-home
	tab_home::renderHomeLayout();
}

void navigate(const char* module_name) {
	// JS: this.$modules[module_name].setActive();
	modules::set_active(module_name);
}

} // namespace legacy_tab_home
