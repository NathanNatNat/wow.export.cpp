/*!
	wow.export.cpp (https://github.com/NathanNatNat/wow.export.cpp)
	Authors: Kruithne <kruithne@gmail.com> (original JS), C++ port
	License: MIT
*/

// C++ port of src/updater/updater.js
// This is a standalone executable spawned by the main application to apply
// updates after the main process exits. It copies files from the .update
// directory into the install directory and re-launches the main app.
//
// NOTE: The updater is currently DISABLED (not built by default). Set the
// CMake option WOW_EXPORT_BUILD_UPDATER=ON to build it.

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

// ── Platform detection ─────────────────────────────────────────
#if defined(_WIN32)
	#define PLATFORM_WINDOWS 1
	#include <windows.h>
	#include <tlhelp32.h>
#elif defined(__linux__)
	#define PLATFORM_LINUX 1
	#include <csignal>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/wait.h>
#else
	#error "Unsupported platform. Only Windows x64 and Linux x64 are supported."
#endif

// ── Constants ──────────────────────────────────────────────────
// Defines the maximum amount of retries to update a file.
// JS source: src/updater/updater.js lines 18–19
static constexpr int MAX_LOCK_TRIES = 30;

// ── Log output collection ──────────────────────────────────────
// JS source: src/updater/updater.js line 13
static std::vector<std::string> log_output;

// ── get_timestamp() ────────────────────────────────────────────
// JS source: src/updater/updater.js lines 21–29
// Creates a Date object and formats a HH:MM:SS timestamp string.
static std::string get_timestamp() {
	auto now = std::chrono::system_clock::now();
	auto time_t_now = std::chrono::system_clock::to_time_t(now);
	std::tm tm_now{};

#if PLATFORM_WINDOWS
	localtime_s(&tm_now, &time_t_now);
#else
	localtime_r(&time_t_now, &tm_now);
#endif

	return std::format("{:02d}:{:02d}:{:02d}",
		tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
}

// ── log() ──────────────────────────────────────────────────────
// JS source: src/updater/updater.js lines 31–35
// Formats a timestamped message, pushes it to log_output, and prints to console.
static void log(const std::string& message) {
	std::string out = "[" + get_timestamp() + "] " + message;
	log_output.push_back(out);
	std::cout << out << std::endl;
}

// ── collect_files() ────────────────────────────────────────────
// JS source: src/updater/updater.js lines 37–48
// Recursively walks a directory, collecting all file paths into the output vector.
static void collect_files(const fs::path& dir, std::vector<fs::path>& out) {
	for (const auto& entry : fs::recursive_directory_iterator(dir)) {
		if (entry.is_regular_file())
			out.push_back(entry.path());
	}
}

// ── delete_directory() ─────────────────────────────────────────
// JS source: src/updater/updater.js lines 50–67
// Recursively deletes a directory and all of its contents.
static void delete_directory(const fs::path& dir) {
	if (fs::exists(dir)) {
		std::error_code ec;
		fs::remove_all(dir, ec);
		if (ec)
			log(std::format("WARN: Failed to remove directory {}: {}", dir.string(), ec.message()));
	}
}

// ── file_exists() ──────────────────────────────────────────────
// JS source: src/updater/updater.js lines 69–76
// Checks if a file exists.
static bool file_exists(const fs::path& file) {
	std::error_code ec;
	return fs::exists(file, ec);
}

// ── is_file_locked() ──────────────────────────────────────────
// JS source: src/updater/updater.js lines 78–85
// Checks if a file is write-locked. Returns true if locked, false if writable.
static bool is_file_locked(const fs::path& file) {
#if PLATFORM_WINDOWS
	// Attempt to open the file for writing. If it fails, the file is locked.
	HANDLE hFile = CreateFileW(
		file.wstring().c_str(),
		GENERIC_WRITE,
		0,             // no sharing — exclusive access
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (hFile == INVALID_HANDLE_VALUE)
		return true; // locked

	CloseHandle(hFile);
	return false;
#else
	// On Linux, check if the file is writable via access().
	// This mirrors the JS fsp.access(file, fs.constants.W_OK) behavior.
	return access(file.string().c_str(), W_OK) != 0;
#endif
}

// ── get_executable_path() ──────────────────────────────────────
// Returns the path of the currently running executable.
static fs::path get_executable_path() {
#if PLATFORM_WINDOWS
	wchar_t buf[MAX_PATH];
	DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		throw std::runtime_error("Failed to get executable path");
	return fs::path(buf);
#elif PLATFORM_LINUX
	return fs::read_symlink("/proc/self/exe");
#endif
}

// ── is_process_running() ───────────────────────────────────────
// Checks if a process with the given PID is still running.
// JS uses process.kill(pid, 0) which sends signal 0 — an existence check.
static bool is_process_running(int pid) {
#if PLATFORM_WINDOWS
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
	if (hProcess == nullptr)
		return false;

	DWORD exitCode = 0;
	if (GetExitCodeProcess(hProcess, &exitCode)) {
		CloseHandle(hProcess);
		return exitCode == STILL_ACTIVE;
	}

	CloseHandle(hProcess);
	return false;
#else
	// kill(pid, 0) — signal 0 doesn't kill, just checks existence.
	return kill(static_cast<pid_t>(pid), 0) == 0;
#endif
}

// ── spawn_detached() ───────────────────────────────────────────
// Spawns a process in a detached manner (fire-and-forget).
static void spawn_detached(const fs::path& executable) {
#if PLATFORM_WINDOWS
	STARTUPINFOW si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = nullptr;
	si.hStdOutput = nullptr;
	si.hStdError = nullptr;

	PROCESS_INFORMATION pi{};

	std::wstring cmd = L"\"" + executable.wstring() + L"\"";
	std::wstring working_dir = executable.parent_path().wstring();

	BOOL ok = CreateProcessW(
		nullptr,
		cmd.data(),
		nullptr,
		nullptr,
		FALSE,
		CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,
		nullptr,
		working_dir.c_str(),
		&si,
		&pi
	);

	if (ok) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
#else
	pid_t child = fork();
	if (child == 0) {
		// Child process: detach from parent
		setsid();

		// Close standard file descriptors
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		std::string exe_str = executable.string();
		execl(exe_str.c_str(), exe_str.c_str(), nullptr);
		_exit(1); // execl failed
	}
	// Parent: don't wait for child — it's detached
#endif
}

// ── run_command() ──────────────────────────────────────────────
// Runs a command and waits for it to complete. Returns the exit code.
static int run_command(const std::vector<std::string>& args) {
	if (args.empty())
		return -1;

	std::string command;
	for (const auto& arg : args) {
		if (!command.empty())
			command += ' ';
		command += arg;
	}

	return std::system(command.c_str());
}

// ── main() entry point ────────────────────────────────────────
// JS source: src/updater/updater.js lines 87–197
// Orchestrates the full update process.
int main(int argc, char* argv[]) {
	try {
		log("Updater has started.");

		// Parse the parent PID from command-line arguments.
		// JS source: line 92 — const pid = Number(argv[0]);
		int pid = -1;
		if (argc > 1) {
			try {
				pid = std::stoi(argv[1]);
			} catch (...) {
				pid = -1;
			}
		}

		if (pid > 0) {
			// Wait for the parent process (PID) to terminate.
			// JS source: lines 95–110
			log(std::format("Waiting for parent process {} to terminate...", pid));

			while (is_process_running(pid)) {
				// Introduce a small delay between checks (500ms).
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}

			log(std::format("Parent process {} has terminated.", pid));
		} else {
			log("WARN: No parent PID was given to the updater.");
		}

		// Send an OS-specific termination command.
		// JS source: lines 117–129
		std::vector<std::string> command;
#if PLATFORM_WINDOWS
		command = {"taskkill", "/f", "/im", "wow.export.cpp.exe"};
#else
		command = {"pkill", "-f", "wow.export.cpp"};
#endif

		{
			std::string cmd_str;
			for (const auto& c : command) {
				if (!cmd_str.empty()) cmd_str += ' ';
				cmd_str += c;
			}

#if PLATFORM_WINDOWS
			log(std::format("Sending auxiliary termination command (win32) {}", cmd_str));
#else
			log(std::format("Sending auxiliary termination command (linux) {}", cmd_str));
#endif
		}

		run_command(command);

		// Determine install and update directories.
		// JS source: lines 131–132
		fs::path install_dir = get_executable_path().parent_path();
		fs::path update_dir = install_dir / ".update";

		log(std::format("Install directory: {}", install_dir.string()));
		log(std::format("Update directory: {}", update_dir.string()));

		// Apply update files.
		// JS source: lines 137–169
		if (file_exists(update_dir)) {
			std::vector<fs::path> update_files;
			collect_files(update_dir, update_files);

			for (const auto& file : update_files) {
				fs::path relative_path = fs::relative(file, update_dir);
				fs::path write_path = install_dir / relative_path;

				log(std::format("Applying update file {}", write_path.string()));

				try {
					bool locked = file_exists(write_path) && is_file_locked(write_path);
					int tries = 0;

					while (locked) {
						tries++;

						if (tries >= MAX_LOCK_TRIES)
							throw std::runtime_error("File was locked, MAX_LOCK_TRIES exceeded.");

						std::this_thread::sleep_for(std::chrono::seconds(1));
						locked = is_file_locked(write_path);
					}

					// Create parent directories as needed.
					// JS source: line 159
					fs::create_directories(write_path.parent_path());

					// Copy the file.
					// JS source: lines 160–162
					std::error_code ec;
					fs::copy_file(file, write_path,
						fs::copy_options::overwrite_existing, ec);

					if (ec)
						log(std::format("WARN: Failed to write update file due to system error: {}", ec.message()));

				} catch (const std::exception& e) {
					log(std::format("WARN: {}", e.what()));
				}
			}
		} else {
			log("WARN: Update directory does not exist. No update to apply.");
		}

		// Re-launch the main application binary.
		// JS source: lines 171–181
#if PLATFORM_WINDOWS
		std::string binary = "wow.export.cpp.exe";
#else
		std::string binary = "wow.export.cpp";
#endif

		log(std::format("Re-launching main process {} ({})", binary,
#if PLATFORM_WINDOWS
			"win32"
#else
			"linux"
#endif
		));

		fs::path binary_path = install_dir / binary;
		if (file_exists(binary_path)) {
			spawn_detached(binary_path);
			log("wow.export.cpp has been updated, have fun!");
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}

		// Delete the .update directory.
		// JS source: lines 183–184
		log("Removing update files...");
		delete_directory(update_dir);

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		log(e.what());
	}

	// Finally block: write log to file.
	// JS source: lines 188–195
	try {
		auto now = std::chrono::system_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()).count();

		std::string log_name = std::format("{}-update.log", ms);
		fs::path log_path = fs::current_path() / "logs";

		fs::create_directories(log_path);

		std::ofstream log_file(log_path / log_name);
		if (log_file.is_open()) {
			for (size_t i = 0; i < log_output.size(); ++i) {
				if (i > 0)
					log_file << '\n';
				log_file << log_output[i];
			}
			log_file.close();
		}
	} catch (...) {
		// Best-effort log writing — don't fail the updater for this.
	}

	return 0;
}
