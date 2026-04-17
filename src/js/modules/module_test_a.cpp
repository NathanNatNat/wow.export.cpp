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
// JS creates fresh component state on each mount, resetting counter to 0.
// In C++, mounted() resets counter to match this behavior.
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
	// JS: data() returns { counter: 0 } on each mount — reset to match.
	counter = 0;
	// JS uses console.log (browser devtools output).
	// C++ uses logging::write (runtime log file) as the unified logging mechanism.
	logging::write("module_test_a mounted");
}

void unmounted() {
	// JS uses console.log; C++ uses logging::write as the unified equivalent.
	logging::write("module_test_a unmounted");
}

} // namespace module_test_a
