/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "updater.h"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "constants.h"
#include "core.h"
#include "generics.h"
#include "buffer.h"
#include "log.h"

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#else
#	include <unistd.h>
#	include <sys/types.h>
#	include <sys/wait.h>
#	include <signal.h>
#	include <fcntl.h>
#	include <cerrno>
#	include <cstring>
#endif

namespace fs = std::filesystem;

namespace updater {

// Module-level state mirroring the JS `let updateManifest;`
static nlohmann::json updateManifest;

// Forward declaration (internal helper, not exported).
static void launchUpdater();

/**
 * Helper to replicate Node.js util.format() behavior: replaces the first
 * occurrence of %s in `fmt` with `arg`.
 */
static std::string utilFormat(const std::string& fmt, const std::string& arg) {
	std::string result = fmt;
	auto pos = result.find("%s");
	if (pos != std::string::npos)
		result.replace(pos, 2, arg);
	return result;
}

/**
 * Check if there are any available updates.
 * Returns true if an update is available.
 * JS: checkForUpdates()
 *
 * Note: JS reads nw.App.manifest.flavour/guid at runtime (updater.js lines 24-26, 33-35).
 * In C++, constants::FLAVOUR and constants::BUILD_GUID serve the same role — both are
 * set at build time and identify the current installation's update channel and version.
 * The functional behavior is identical.
 */
bool checkForUpdates() {
	try {
		const std::string updateURL = core::view->config.at("updateURL").get<std::string>();
		const std::string manifestURL = utilFormat(updateURL, std::string(constants::FLAVOUR)) + "update.json";
		logging::write(std::format("Checking for updates ({})...", manifestURL));

		nlohmann::json manifest = generics::getJSON(manifestURL);

		if (!manifest.contains("guid") || !manifest["guid"].is_string())
			throw std::runtime_error("Update manifest does not contain a valid build GUID");

		if (!manifest.contains("contents") || !manifest["contents"].is_object())
			throw std::runtime_error("Update manifest does not contain a valid contents list");

		const std::string remoteGuid = manifest["guid"].get<std::string>();
		const std::string localGuid(constants::BUILD_GUID);

		if (remoteGuid != localGuid) {
			updateManifest = std::move(manifest);
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
 * JS: applyUpdate()
 *
 * Deviation from JS: JS applyUpdate/launchUpdater are async with await at each
 * step (updater.js lines 50, 61, 79, 103-104, 119-124). C++ executes the same
 * logical sequence synchronously with blocking calls. This is the standard
 * JS async → C++ synchronous mapping — the step ordering and error handling
 * are identical, only the execution model differs (no event loop yielding).
 */
void applyUpdate() {
	// Collect entries from the manifest contents object.
	struct FileNode {
		std::string file;
		int64_t size;
		std::string hash;
		int64_t compSize;
		int64_t ofs;
	};

	const auto& contents = updateManifest["contents"];
	const int entryCount = static_cast<int>(contents.size());
	core::showLoadingScreen(entryCount, "Verifying local files...");

	logging::write(std::format("Starting update to {}...", updateManifest["guid"].get<std::string>()));

	std::vector<FileNode> requiredFiles;

	int i = 0;
	for (auto it = contents.begin(); it != contents.end(); ++it, ++i) {
		const std::string& file = it.key();
		const auto& meta = it.value();

		core::progressLoadingScreen(std::format("{} / {}", i + 1, entryCount));

		const fs::path localPath = constants::INSTALL_PATH() / file;

		const int64_t metaSize = meta["size"].get<int64_t>();
		const std::string metaHash = meta["hash"].get<std::string>();
		const int64_t metaCompSize = meta["compSize"].get<int64_t>();
		const int64_t metaOfs = meta["ofs"].get<int64_t>();

		try {
			logging::write(std::format("Verifying local file: {}", file));
			const auto fileSize = static_cast<int64_t>(fs::file_size(localPath));

			// If the file size is different, skip hashing and just mark for update.
			if (fileSize != metaSize) {
				logging::write(std::format("Marking {} for update due to size mismatch ({} != {})", file, fileSize, metaSize));
				requiredFiles.push_back({file, metaSize, metaHash, metaCompSize, metaOfs});
				continue;
			}

			// Verify local sha256 hash with remote one.
			logging::write(std::format("Hashing local file {} for verification (size: {} bytes)...", file, fileSize));
			const std::string localHash = generics::getFileHash(localPath, "sha256", "hex");
			logging::write(std::format("Hash calculated for {}: {}", file, localHash));

			if (localHash != metaHash) {
				logging::write(std::format("Marking {} for update due to hash mismatch ({} != {})", file, localHash, metaHash));
				requiredFiles.push_back({file, metaSize, metaHash, metaCompSize, metaOfs});
				continue;
			}

			logging::write(std::format("File {} verified successfully", file));
		} catch (const std::exception& e) {
			// Error thrown, likely due to file not existing.
			logging::write(std::format("Marking {} for update due to local error: {}", file, e.what()));
			requiredFiles.push_back({file, metaSize, metaHash, metaCompSize, metaOfs});
		}
	}

	const double totalCompSize = std::accumulate(
		requiredFiles.begin(), requiredFiles.end(), 0.0,
		[](double total, const FileNode& node) { return total + static_cast<double>(node.compSize); });
	const std::string downloadSize = generics::filesize(totalCompSize);
	logging::write(std::format("{} files ({}) marked for download.", requiredFiles.size(), downloadSize));

	core::showLoadingScreen(static_cast<int>(requiredFiles.size()), "Downloading updates...");

	// Create .update directory here and then check if it exists and is writeable.
	try {
		generics::createDirectory(constants::UPDATE::DIRECTORY());
		if (!generics::directoryIsWritable(constants::UPDATE::DIRECTORY())) {
			throw std::runtime_error(".update directory does not exist or is not writable");
		}
	} catch (const std::exception& e) {
		logging::write(std::format("Failed to create directory for update files: {}", e.what()));
		// Deviation: user-facing text says "wow.export.cpp" per project guidelines.
		core::view->loadingTitle = "wow.export.cpp couldn't create the update directory. Ensure it has write permissions to its folder and restart the app, or update manually.";
		return;
	}

	const std::string updateURL2 = core::view->config.at("updateURL").get<std::string>();
	const std::string remoteEndpoint = utilFormat(updateURL2, std::string(constants::FLAVOUR)) + "update";

	for (size_t j = 0, n = requiredFiles.size(); j < n; ++j) {
		const FileNode& node = requiredFiles[j];
		const fs::path localFile = constants::UPDATE::DIRECTORY() / node.file;
		logging::write(std::format("Downloading {} to {}", node.file, localFile.string()));

		core::progressLoadingScreen(std::format("{} / {} ({})", j + 1, n, downloadSize));
		generics::downloadFile(remoteEndpoint, localFile.string(), node.ofs, node.compSize, true);
	}

	core::view->loadingTitle = "Restarting application...";
	launchUpdater();
}

/**
 * Launch the external updater process and exit.
 * JS: launchUpdater()
 */
static void launchUpdater() {
	// On the rare occurrence that we've updated the updater, the updater
	// cannot update the updater, so instead we update the updater here.
	const fs::path helperApp = constants::INSTALL_PATH() / constants::UPDATE::HELPER();
	const fs::path updatedApp = constants::UPDATE::DIRECTORY() / constants::UPDATE::HELPER();

	try {
		logging::write(std::format("Checking for updater application at {}", updatedApp.string()));
		const bool updaterExists = generics::fileExists(updatedApp);
		logging::write(std::format("Updater exists check: {}", updaterExists));

		if (updaterExists) {
			logging::write(std::format("Renaming updater from {} to {}", updatedApp.string(), helperApp.string()));
			fs::rename(updatedApp, helperApp);
			logging::write("Updater renamed successfully");
		}

#ifdef _WIN32
		const DWORD pid = GetCurrentProcessId();
#else
		const pid_t pid = getpid();
#endif
		logging::write(std::format("Spawning updater process: {} with parent PID {}",
					  helperApp.string(), static_cast<int64_t>(pid)));

		// Launch the updater application.
#ifdef _WIN32
		const std::string cmdLine = std::format("\"{}\" {}", helperApp.string(), pid);

		STARTUPINFOA si{};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};

		BOOL success = CreateProcessA(
			nullptr,
			const_cast<char*>(cmdLine.c_str()),
			nullptr, nullptr,
			FALSE,
			DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
			nullptr, nullptr,
			&si, &pi
		);

		if (!success) {
			throw std::runtime_error(
				std::format("CreateProcess failed with error {}", GetLastError()));
		}

		// Brief delay to let the updater start.
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		logging::write(std::format("Updater spawned successfully (PID: {}), detaching...",
					  static_cast<int64_t>(pi.dwProcessId)));

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
#else
		const std::string pidStr = std::to_string(pid);
		const std::string helperStr = helperApp.string();

		// Use a pipe to detect exec failure in the child process.
		// JS equivalent: child.on('error', ...) handler (updater.js lines 152-155).
		// The write end is set close-on-exec so that a successful execl()
		// closes it automatically (parent reads EOF). On failure, the child
		// writes errno to the pipe so the parent can log the error.
		int pipefd[2];
		if (pipe(pipefd) < 0)
			throw std::runtime_error("pipe() failed");

		// Set close-on-exec on write end.
		fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);

		pid_t child = fork();
		if (child < 0) {
			close(pipefd[0]);
			close(pipefd[1]);
			throw std::runtime_error("fork() failed");
		} else if (child == 0) {
			// Child process — close read end of pipe.
			close(pipefd[0]);

			// Detach into new session.
			setsid();

			// Close standard file descriptors.
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);

			execl(helperStr.c_str(), helperStr.c_str(), pidStr.c_str(), nullptr);
			// If execl returns, it failed — write errno to pipe for parent to log.
			int err = errno;
			(void)::write(pipefd[1], &err, sizeof(err));
			close(pipefd[1]);
			_exit(1);
		}

		// Parent process — close write end of pipe.
		close(pipefd[1]);

		// Brief delay to let the updater start.
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// Check if exec failed in the child via the pipe.
		int execErr = 0;
		ssize_t nread = ::read(pipefd[0], &execErr, sizeof(execErr));
		close(pipefd[0]);

		if (nread > 0) {
			// exec failed — log matching JS child.on('error') handler (updater.js line 153).
			logging::write(std::format("ERROR: Failed to spawn updater: {}", std::strerror(execErr)));
			throw std::runtime_error(std::format("execl() failed: {}", std::strerror(execErr)));
		}

		logging::write(std::format("Updater spawned successfully (PID: {}), detaching...",
					  static_cast<int64_t>(child)));
#endif

		logging::write("Exiting main process to allow update...");
		std::exit(0);
	} catch (const std::exception& e) {
		logging::write(std::format("Failed to restart for update: {}", e.what()));
		// JS: log.write(e) — logs the raw error object (updater.js line 165).
		// util.format(e) on an Error object produces its toString(), which
		// typically includes the error name and message (e.g. "Error: ...").
		// In C++, e.what() is the message; log the full exception representation.
		logging::write(std::string("Error: ") + e.what());
	}
}

} // namespace updater
