/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_blender.h"
#include "../log.h"
#include "../core.h"
#include "../modules.h"
#include "../constants.h"
#include "../generics.h"

#include <string>
#include <vector>
#include <chrono>
#include <format>
#include <regex>
#include <fstream>
#include <filesystem>
#include <future>
#include <algorithm>

#include <imgui.h>

namespace tab_blender {

static const std::regex PATTERN_ADDON_VER(R"('version': \((\d+), (\d+), (\d+)\),)");
static const std::regex PATTERN_BLENDER_VER(R"(\d+\.\d+\w?)");

struct ManifestResult {
	std::string version;
	std::string error;
	std::string message;
};

static ManifestResult parse_manifest_version(const std::filesystem::path& file) {
	try {
		std::ifstream ifs(file);
		if (!ifs.is_open())
			return { "", "file_not_found", "" };

		std::string data((std::istreambuf_iterator<char>(ifs)),
			std::istreambuf_iterator<char>());
		ifs.close();

		std::smatch match;
		if (std::regex_search(data, match, PATTERN_ADDON_VER))
			return { std::format("{}.{}.{}", match[1].str(), match[2].str(), match[3].str()), "", "" };

		return { "", "version_pattern_mismatch", "" };
	} catch (const std::exception& e) {
		return { "", "read_error", e.what() };
	}
}

static std::vector<std::string> get_blender_installations() {
	std::vector<std::string> installs;

	try {
		namespace fs = std::filesystem;
		const fs::path& blender_dir = constants::BLENDER::DIR();

		if (!fs::exists(blender_dir) || !fs::is_directory(blender_dir))
			return installs;

		for (const auto& entry : fs::directory_iterator(blender_dir)) {
			if (!entry.is_directory())
				continue;

			const std::string name = entry.path().filename().string();
			if (!std::regex_search(name, PATTERN_BLENDER_VER)) {
				logging::write(std::format("Skipping invalid Blender installation dir: {}", name));
				continue;
			}

			installs.push_back(name);
		}
	} catch (...) {
		// No blender installation or cannot access.
	}

	return installs;
}

struct PendingBlenderInstall {
	std::future<std::string> result_future;
	std::unique_ptr<BusyLock> busy_lock;
};

static std::optional<PendingBlenderInstall> pending_install;

static void pump_blender_install() {
	if (!pending_install.has_value())
		return;

	auto& task = *pending_install;
	if (task.result_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	try {
		std::string result = task.result_future.get();
		if (result.empty()) {
			logging::write("No valid Blender installation found, add-on install failed.");
			core::setToast("error", "Sorry, a valid Blender 2.8+ installation was not be detected on your system.", {}, -1);
		} else {
			core::setToast("success", result);
		}
	} catch (const std::exception& e) {
		logging::write(std::format("Installation failed due to exception: {}", e.what()));
		core::setToast("error", "Sorry, an unexpected error occurred trying to install the add-on.", {}, -1);
	}

	pending_install.reset();
}

static void start_automatic_install() {
	if (pending_install.has_value())
		return;

	core::setToast("progress", "Installing Blender add-on, please wait...", {}, -1, false);
	logging::write("Starting automatic installation of Blender add-on...");

	PendingBlenderInstall task;
	task.busy_lock = std::make_unique<BusyLock>(core::create_busy_lock());
	task.result_future = std::async(std::launch::async, []() -> std::string {
		namespace fs = std::filesystem;
		const auto versions = get_blender_installations();
		bool installed = false;

		for (const auto& version : versions) {
			double ver_num = 0;
			try { ver_num = std::stod(version); } catch (...) { continue; }

			if (ver_num >= constants::BLENDER::MIN_VER) {
				const fs::path addon_path = constants::BLENDER::DIR() / version / std::string(constants::BLENDER::ADDON_DIR);
				logging::write(std::format("Targeting Blender version {} ({})", version, addon_path.string()));

				generics::deleteDirectory(addon_path.string());
				generics::createDirectory(addon_path.string());

				const fs::path& local_dir = constants::BLENDER::LOCAL_DIR();
				if (fs::exists(local_dir) && fs::is_directory(local_dir)) {
					for (const auto& file : fs::directory_iterator(local_dir)) {
						if (file.is_directory())
							continue;

						const fs::path src_path = file.path();
						const fs::path dest_path = addon_path / file.path().filename();

						logging::write(std::format("{} -> {}", src_path.string(), dest_path.string()));
						fs::copy_file(src_path, dest_path, fs::copy_options::overwrite_existing);
					}
				}

				installed = true;
			}
		}

		if (installed)
			return "The latest add-on version has been installed! (You will need to restart Blender)";
		return "";
	});
	pending_install = std::move(task);
}

static void open_addon_directory() {
	core::openInExplorer(constants::BLENDER::LOCAL_DIR().string());
}

void registerTab() {
	modules::register_context_menu_option("tab_blender", "Install Blender Add-on", "../images/blender.png");
}

void render() {
	// Poll for pending async install.
	pump_blender_install();

	ImGui::Text("Installing the wow.export.cpp Add-on for Blender 2.8+");
	ImGui::Separator();

	ImGui::TextWrapped(
		"Blender users can make use of our special importer add-on which makes importing "
		"advanced objects as simple as a single click. WMO objects are imported with any "
		"exported doodad sets included. ADT map tiles are imported complete with all WMOs "
		"and doodads positioned as they would be in-game."
	);

	ImGui::Spacing();

	const bool busy = core::view->isBusy > 0;

	if (busy)
		ImGui::BeginDisabled();

	if (ImGui::Button("Install Automatically (Recommended)"))
		start_automatic_install();

	if (busy)
		ImGui::EndDisabled();

	if (ImGui::Button("Install Manually (Advanced)"))
		open_addon_directory();

	if (ImGui::Button("Go Back"))
		modules::go_to_landing();
}

void checkLocalVersion() {
	logging::write("Checking local Blender add-on version...");

	const auto versions = get_blender_installations();
	if (versions.empty()) {
		logging::write("Error: User does not have any Blender installations.");
		return;
	}

	std::string versionsStr;
	for (const auto& v : versions) {
		if (!versionsStr.empty())
			versionsStr += ", ";
		versionsStr += v;
	}
	logging::write(std::format("Available Blender installations: {}", versionsStr.empty() ? "None" : versionsStr));

	auto sortedVersions = versions;
	std::sort(sortedVersions.begin(), sortedVersions.end());
	const std::string blenderVersion = sortedVersions.back();

	double verNum = 0;
	try { verNum = std::stod(blenderVersion); } catch (...) {}
	if (verNum < constants::BLENDER::MIN_VER) {
		logging::write(std::format("Latest Blender install does not meet minimum requirements ({} < {})",
			blenderVersion, constants::BLENDER::MIN_VER));
		return;
	}

	namespace fs = std::filesystem;
	const fs::path latestManifest = constants::BLENDER::LOCAL_DIR() / std::string(constants::BLENDER::ADDON_ENTRY);
	const auto latestAddonVersion = parse_manifest_version(latestManifest);

	if (!latestAddonVersion.error.empty()) {
		if (latestAddonVersion.error == "file_not_found")
			logging::write(std::format("Error: Add-on entry file not found: {}", latestManifest.string()));
		else if (latestAddonVersion.error == "version_pattern_mismatch")
			logging::write(std::format("Error: Add-on entry file does not contain valid version pattern: {}", latestManifest.string()));
		else
			logging::write(std::format("Error: Failed to read add-on entry file ({}): {}", latestManifest.string(), latestAddonVersion.message));
		return;
	}

	const fs::path blenderManifest = constants::BLENDER::DIR() / blenderVersion
		/ std::string(constants::BLENDER::ADDON_DIR) / std::string(constants::BLENDER::ADDON_ENTRY);
	const auto blenderAddonVersion = parse_manifest_version(blenderManifest);

	logging::write(std::format("Latest add-on version: {}, Blender add-on version: {}",
		latestAddonVersion.version,
		blenderAddonVersion.error.empty() ? blenderAddonVersion.version : blenderAddonVersion.error));

	if (blenderAddonVersion.error.empty() && latestAddonVersion.version > blenderAddonVersion.version) {
		logging::write("Prompting user for Blender add-on update...");
		core::setToast("info", "A newer version of the Blender add-on is available for you.", {
			{"Install", []() { modules::setActive("tab_blender"); }},
			{"Maybe Later", []() {}}
		}, -1, false);
	}
}

} // namespace tab_blender
