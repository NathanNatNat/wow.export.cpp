/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "legacy_tab_home.h"
#include "../modules.h"

namespace legacy_tab_home {

/**
 * Render the legacy home tab widget using ImGui.
 * Currently a blank placeholder — content will be added later.
 */
void render() {
	// TODO(conversion): Legacy home tab content stripped; will be re-added when UI is finalized.
}

void navigate(const char* module_name) {
	// JS: this.$modules[module_name].setActive();
	modules::set_active(module_name);
}

} // namespace legacy_tab_home
