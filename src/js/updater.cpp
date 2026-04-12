/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "updater.h"
#include "constants.h"
#include "generics.h"
#include "core.h"
#include "log.h"

#include <format>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <thread>
#include <chrono>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

namespace updater {

namespace {

// JS: let updateManifest;
nlohmann::json updateManifest;

/**
 * Read the local application manifest (package.json).
 * JS equivalent: nw.App.manifest
 */
nlohmann::json getLocalManifest() {
	auto manifestPath = constants::INSTALL_PATH() / "package.json";
	std::ifstream file(manifestPath);
	if (!file.is_open())
		throw std::runtime_error(std::format("Failed to open local manifest: {}", manifestPath.string()));

	nlohmann::json manifest;
	file >> manifest;
	return manifest;
}

/**
 * Format a URL template by replacing the first %s with the given argument.
 * JS equivalent: util.format(template, arg)
 */
std::string format_url(const std::string& fmt, const std::string& arg) {
	std::string result = fmt;
	auto pos = result.find("%s");
	if (pos != std::string::npos)
		result.replace(pos, 2, arg);
	return result;
}

/**
 * Launch the external updater process and exit.
 */
void launchUpdater() {
	// On the rare occurrence that we've updated the updater, the updater
	// cannot update the updater, so instead we update the updater here.
	auto helperApp = constants::INSTALL_PATH() / constants::UPDATE::HELPER();
	auto updatedApp = constants::UPDATE::DIRECTORY() / constants::UPDATE::HELPER();

	try {
		logging::write(std::format("Checking for updater application at {}", updatedApp.string()));
		bool updaterExists = generics::fileExists(updatedApp);
		logging::write(std::format("Updater exists check: {}", updaterExists));

		if (updaterExists) {
			logging::write(std::format("Renaming updater from {} to {}", updatedApp.string(), helperApp.string()));
			std::filesystem::rename(updatedApp, helperApp);
			logging::write("Updater renamed successfully");
		}

#ifdef _WIN32
		DWORD pid = GetCurrentProcessId();
#else
		pid_t pid = getpid();
#endif

		logging::write(std::format("Spawning updater process: {} with parent PID {}",
			helperApp.string(), static_cast<int64_t>(pid)));

		// Launch the updater application.
#ifdef _WIN32
		std::wstring cmdLine = L"\"" + helperApp.wstring() + L"\" " + std::to_wstring(pid);

		STARTUPINFOW si = {};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi = {};

		BOOL created = CreateProcessW(
			nullptr,
			cmdLine.data(),
			nullptr, nullptr,
			FALSE,
			DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
			nullptr, nullptr,
			&si, &pi
		);

		if (!created)
			throw std::runtime_error(std::format("Failed to spawn updater (error: {})", GetLastError()));

		logging::write(std::format("Updater spawned successfully (PID: {}), detaching...", pi.dwProcessId));
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
#else
		pid_t child = fork();
		if (child == -1)
			throw std::runtime_error("Failed to fork for updater process");

		if (child == 0) {
			// Child process: create new session and exec the updater.
			setsid();
			std::string helperStr = helperApp.string();
			std::string pidStr = std::to_string(pid);
			execl(helperStr.c_str(), helperStr.c_str(), pidStr.c_str(), nullptr);
			_exit(1); // exec failed
		}

		logging::write(std::format("Updater spawned successfully (PID: {}), detaching...", child));
#endif

		// JS: await new Promise(resolve => setTimeout(resolve, 100));
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		logging::write("Exiting main process to allow update...");
		// JS: process.exit();
		std::exit(0);
	} catch (const std::exception& e) {
		logging::write(std::format("Failed to restart for update: {}", e.what()));
	}
}

} // anonymous namespace

/**
 * Check if there are any available updates.
 * Returns true if an update is available.
 */
bool checkForUpdates() {
	try {
		// JS: const localManifest = nw.App.manifest;
		nlohmann::json localManifest = getLocalManifest();
		std::string flavour = localManifest.value("flavour", std::string{});
		std::string localGuid = localManifest.value("guid", std::string{});

		// JS: const manifestURL = util.format(core.view.config.updateURL, localManifest.flavour) + 'update.json';
		std::string updateURL = core::view->config.value("updateURL", std::string{});
		std::string manifestURL = format_url(updateURL, flavour) + "update.json";
		logging::write(std::format("Checking for updates ({})...", manifestURL));

		// JS: const manifest = await generics.getJSON(manifestURL);
		nlohmann::json manifest = generics::getJSON(manifestURL);

		// JS: assert(typeof manifest.guid === 'string', ...)
		if (!manifest.contains("guid") || !manifest["guid"].is_string())
			throw std::runtime_error("Update manifest does not contain a valid build GUID");
		// JS: assert(typeof manifest.contents === 'object', ...)
		if (!manifest.contains("contents") || !manifest["contents"].is_object())
			throw std::runtime_error("Update manifest does not contain a valid contents list");

		std::string remoteGuid = manifest["guid"].get<std::string>();

		if (remoteGuid != localGuid) {
			updateManifest = manifest;
			logging::write(std::format("Update available, prompting using ({} != {})", remoteGuid, localGuid));
			return true;
		}

		logging::write(std::format("Not updating ({} == {})", remoteGuid, localGuid));
		return false;
	} catch (const std::exception& e) {
		logging::write(std::format("Not updating due to error: {}", e.what()));
		return false;
	}
}

/**
 * Apply an outstanding update.
 */
void applyUpdate() {
	// JS: const entries = Object.entries(updateManifest.contents);
	auto contentsObj = updateManifest["contents"];
	std::vector<std::pair<std::string, nlohmann::json>> entries;
	for (auto& [key, val] : contentsObj.items())
		entries.emplace_back(key, val);

	// JS: core.showLoadingScreen(entries.length, 'Verifying local files...');
	core::showLoadingScreen(static_cast<int>(entries.size()), "Verifying local files...");

	std::string updateGuid = updateManifest["guid"].get<std::string>();
	logging::write(std::format("Starting update to {}...", updateGuid));

	struct RequiredFile {
		std::string file;
		nlohmann::json meta;
	};

	std::vector<RequiredFile> requiredFiles;

	for (size_t i = 0; i < entries.size(); i++) {
		const auto& [file, meta] = entries[i];

		// JS: await core.progressLoadingScreen((i + 1) + ' / ' + n);
		core::progressLoadingScreen(std::format("{} / {}", i + 1, entries.size()));

		// JS: const localPath = path.join(constants.INSTALL_PATH, file);
		auto localPath = constants::INSTALL_PATH() / file;
		RequiredFile node{file, meta};

		try {
			logging::write(std::format("Verifying local file: {}", file));

			// JS: const stats = await fsp.stat(localPath);
			if (!std::filesystem::exists(localPath))
				throw std::runtime_error("File does not exist");

			auto fileSize = static_cast<int64_t>(std::filesystem::file_size(localPath));
			int64_t expectedSize = meta.value("size", int64_t(0));

			// If the file size is different, skip hashing and just mark for update.
			if (fileSize != expectedSize) {
				logging::write(std::format("Marking {} for update due to size mismatch ({} != {})",
					file, fileSize, expectedSize));
				requiredFiles.push_back(std::move(node));
				continue;
			}

			// Verify local sha256 hash with remote one.
			logging::write(std::format("Hashing local file {} for verification (size: {} bytes)...",
				file, fileSize));
			std::string localHash = generics::getFileHash(localPath, "sha256", "hex");
			logging::write(std::format("Hash calculated for {}: {}", file, localHash));

			std::string expectedHash = meta.value("hash", std::string{});
			if (localHash != expectedHash) {
				logging::write(std::format("Marking {} for update due to hash mismatch ({} != {})",
					file, localHash, expectedHash));
				requiredFiles.push_back(std::move(node));
				continue;
			}

			logging::write(std::format("File {} verified successfully", file));
		} catch (const std::exception& e) {
			// Error thrown, likely due to file not existing.
			logging::write(std::format("Marking {} for update due to local error: {}", file, e.what()));
			requiredFiles.push_back(std::move(node));
		}
	}

	// JS: const downloadSize = generics.filesize(requiredFiles.map(e => e.meta.compSize).reduce(...));
	int64_t totalCompSize = 0;
	for (const auto& node : requiredFiles)
		totalCompSize += node.meta.value("compSize", int64_t(0));

	std::string downloadSize = generics::filesize(static_cast<double>(totalCompSize));
	logging::write(std::format("{} files ({}) marked for download.", requiredFiles.size(), downloadSize));

	// JS: core.showLoadingScreen(requiredFiles.length, 'Downloading updates...');
	core::showLoadingScreen(static_cast<int>(requiredFiles.size()), "Downloading updates...");

	// Create .update directory here and then check if it exists and is writeable. If not, we can't continue.
	try {
		generics::createDirectory(constants::UPDATE::DIRECTORY());
		if (!generics::directoryIsWritable(constants::UPDATE::DIRECTORY()))
			throw std::runtime_error(".update directory does not exist or is not writable");
	} catch (const std::exception& e) {
		logging::write(std::format("Failed to create directory for update files: {}", e.what()));
		core::view->loadingTitle = "wow.export.cpp couldn't create the update directory. Ensure it has write permissions to its folder and restart the app, or update manually.";
		return;
	}

	// JS: const remoteEndpoint = util.format(core.view.config.updateURL, nw.App.manifest.flavour) + 'update';
	nlohmann::json localManifest = getLocalManifest();
	std::string flavour = localManifest.value("flavour", std::string{});
	std::string updateURL = core::view->config.value("updateURL", std::string{});
	std::string remoteEndpoint = format_url(updateURL, flavour) + "update";

	for (size_t i = 0; i < requiredFiles.size(); i++) {
		const auto& node = requiredFiles[i];
		auto localFile = constants::UPDATE::DIRECTORY() / node.file;
		logging::write(std::format("Downloading {} to {}", node.file, localFile.string()));

		// JS: await core.progressLoadingScreen(util.format('%d / %d (%s)', i + 1, n, downloadSize));
		core::progressLoadingScreen(std::format("{} / {} ({})", i + 1, requiredFiles.size(), downloadSize));

		int64_t ofs = node.meta.value("ofs", int64_t(0));
		int64_t compSize = node.meta.value("compSize", int64_t(0));
		// JS: await generics.downloadFile(remoteEndpoint, localFile, node.meta.ofs, node.meta.compSize, true);
		generics::downloadFile(remoteEndpoint, localFile.string(), ofs, compSize, true);
	}

	// JS: core.view.loadingTitle = 'Restarting application...';
	core::view->loadingTitle = "Restarting application...";
	// JS: await launchUpdater();
	launchUpdater();
}

} // namespace updater
