/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "module_test_a.h"
#include "../log.h"
#include "../modules.h"

#include <imgui.h>

namespace module_test_a {

// --- File-local state (JS equivalent: data() { return { counter: 0 }; }) ---
static int counter = 0;

void render() {
	ImGui::Text("Module Test A");
	ImGui::Separator();

	ImGui::Text("Counter: %d", counter);

	if (ImGui::Button("Increment"))
		counter++;

	if (ImGui::Button("Switch to Module B"))
		modules::set_active("module_test_b");
}

void mounted() {
	logging::write("module_test_a mounted");
}

} // namespace module_test_a
