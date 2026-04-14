/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_changelog.h"
#include "../log.h"
#include "../modules.h"

#include <string>
#include <fstream>
#include <filesystem>
#include <format>

#include <imgui.h>

namespace tab_changelog {

// --- File-local state ---

static std::string changelog_text;
static bool has_loaded = false;

// --- Internal functions ---

static void load_changelog() {
	if (has_loaded)
		return;

	try {
		// JS equivalent: BUILD_RELEASE ? './src/CHANGELOG.md' : '../../CHANGELOG.md'
		// In C++, try both paths.
		namespace fs = std::filesystem;
		fs::path changelog_path = "src/CHANGELOG.md";

		if (!fs::exists(changelog_path))
			changelog_path = "../../CHANGELOG.md";

		if (!fs::exists(changelog_path))
			changelog_path = "CHANGELOG.md";

		std::ifstream file(changelog_path);
		if (!file.is_open()) {
			logging::write("failed to load changelog: file not found");
			changelog_text = "Error loading changelog";
			has_loaded = true;
			return;
		}

		changelog_text = std::string(
			(std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		file.close();

		has_loaded = true;
	} catch (const std::exception& e) {
		logging::write(std::format("failed to load changelog: {}", e.what()));
		changelog_text = "Error loading changelog";
		has_loaded = true;
	}
}

// --- Public API ---

void registerTab() {
	modules::register_context_menu_option("tab_changelog", "View Recent Changes", "list.svg");
}

void mounted() {
	load_changelog();
}

void render() {
	ImGui::Text("Changelog");
	ImGui::Separator();

	// Scrollable region for changelog content.
	ImGui::BeginChild("##changelog-text", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
	ImGui::TextWrapped("%s", changelog_text.c_str());
	ImGui::EndChild();

	// Bottom: Go Back button.
	if (ImGui::Button("Go Back"))
		modules::go_to_landing();
}

} // namespace tab_changelog
