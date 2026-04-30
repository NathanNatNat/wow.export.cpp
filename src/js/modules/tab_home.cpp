/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_home.h"
#include "../modules.h"

namespace tab_home {

void renderHomeLayout() {
}

void render() {
	renderHomeLayout();
}

void navigate(const char* module_name) {
	modules::set_active(module_name);
}

void cleanup() {
}

} // namespace tab_home
