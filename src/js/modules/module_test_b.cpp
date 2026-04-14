/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "module_test_b.h"
#include "../log.h"
#include "../core.h"
#include "../modules.h"

#include <cstring>

#include <imgui.h>

namespace module_test_b {

// --- File-local state (JS equivalent: data() { return { message: 'Hello Thrall' }; }) ---
static char message[256] = "Hello Thrall";

void render() {
	ImGui::Text("Module Test B");
	ImGui::Separator();

	ImGui::Text("Message: %s", message);
	ImGui::InputText("##message", message, sizeof(message));

	ImGui::Text("Dev Mode: %s", core::view->isDev ? "true" : "false");
	ImGui::Text("Busy State: %d", core::view->isBusy);
	ImGui::Text("CASC Loaded: %s", core::view->casc != nullptr ? "true" : "false");

	if (ImGui::Button("Switch to Module A"))
		modules::set_active("module_test_a");

	if (ImGui::Button("Reload Module B"))
		modules::reload_module("module_test_b");

	if (ImGui::Button("Show Toast"))
		core::setToast("info", "test toast from module b");
}

void mounted() {
	logging::write("module_test_b mounted");
}

} // namespace module_test_b
