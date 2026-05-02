#include "module_test_a.h"
#include "../log.h"
#include "../modules.h"

#include <imgui.h>

namespace module_test_a {

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
	counter = 0;
	logging::write("module_test_a mounted");
}

void unmounted() {
	logging::write("module_test_a unmounted");
}

} // namespace module_test_a
