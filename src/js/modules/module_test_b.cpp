#include "module_test_b.h"
#include "../log.h"
#include "../core.h"
#include "../modules.h"

#include <string>

#include <imgui.h>

namespace module_test_b {

static std::string message = "Hello Thrall";

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
	message = "Hello Thrall";
	logging::write("module_test_b mounted");
}

void unmounted() {
	logging::write("module_test_b unmounted");
}

} // namespace module_test_b
