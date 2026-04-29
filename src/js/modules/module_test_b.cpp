/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "module_test_b.h"
#include "../log.h"
#include "../core.h"
#include "../modules.h"

#include <string>

#include <imgui.h>

namespace module_test_b {

// --- File-local state (JS equivalent: data() { return { message: 'Hello Thrall' }; }) ---
// JS has no length limit on the message string. Using std::string with
// ImGui resize callback to match JS unlimited string behavior.
static std::string message = "Hello Thrall";

// ImGui InputText resize callback for std::string.
// Standard pattern: resize the backing string to data->BufSize (the requested
// capacity) and re-point data->Buf at the (possibly relocated) buffer.
static int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
		std::string* str = static_cast<std::string*>(data->UserData);
		str->resize(static_cast<size_t>(data->BufSize));
		data->Buf = str->data();
	}
	return 0;
}

void render() {
	ImGui::Text("Module Test B");
	ImGui::Separator();

	ImGui::Text("Message: %s", message.c_str());

	ImGui::InputText("##message", message.data(), message.capacity() + 1,
	                 ImGuiInputTextFlags_CallbackResize, inputTextResizeCallback, &message);

	// JS: {{ $core.view.isDev }} — Vue renders boolean as "true"/"false"
	ImGui::Text("Dev Mode: %s", core::view->isDev ? "true" : "false");
	// JS: {{ $core.view.isBusy }} — Vue renders boolean as "true"/"false"
	ImGui::Text("Busy State: %s", core::view->isBusy ? "true" : "false");
	// JS: {{ $core.view.casc !== null }} — Vue renders boolean as "true"/"false"
	ImGui::Text("CASC Loaded: %s", core::view->casc != nullptr ? "true" : "false");

	if (ImGui::Button("Switch to Module A"))
		modules::set_active("module_test_a");

	if (ImGui::Button("Reload Module B"))
		modules::reload_module("module_test_b");

	if (ImGui::Button("Show Toast"))
		core::setToast("info", "test toast from module b");
}

void mounted() {
	// JS: data() returns { message: 'Hello Thrall' } on each mount — reset to match.
	message = "Hello Thrall";
	// JS uses console.log; C++ uses logging::write as the unified equivalent.
	logging::write("module_test_b mounted");
}

void unmounted() {
	// JS uses console.log; C++ uses logging::write as the unified equivalent.
	logging::write("module_test_b unmounted");
}

} // namespace module_test_b
