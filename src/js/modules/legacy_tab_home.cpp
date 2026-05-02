#include "legacy_tab_home.h"
#include "tab_home.h"
#include "../modules.h"
#include "../core.h"

#include <imgui.h>

namespace legacy_tab_home {

void render() {
	tab_home::renderHomeLayout();
}

void navigate(const char* module_name) {
	modules::set_active(module_name);
}

}
