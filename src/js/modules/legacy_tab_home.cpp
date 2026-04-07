/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "legacy_tab_home.h"

namespace legacy_tab_home {

/**
 * Render the legacy home tab widget using ImGui.
 * Currently a blank placeholder — content will be added later.
 */
void render() {
	// TODO(conversion): Legacy home tab content stripped; will be re-added when UI is finalized.
}

void navigate(const char* /*module_name*/) {
	// JS: this.$modules[module_name].setActive();
	// TODO(conversion): Module activation will be wired when the module system is integrated.
}

} // namespace legacy_tab_home
