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
 * Uses the same renderHomeLayout() as tab_home.
 */
void render() {
	tab_home::renderHomeLayout();
}

void navigate(const char* module_name) {
	modules::set_active(module_name);
}

} // namespace legacy_tab_home
