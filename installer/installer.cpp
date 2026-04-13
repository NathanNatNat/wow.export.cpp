/*!
	wow.export.cpp (https://github.com/NathanNatNat/wow.export.cpp)
	Authors: Kruithne <kruithne@gmail.com> (original JS), C++ port
	License: MIT
*/

// C++ port of src/installer/installer.js
// This is a standalone console application that extracts a data.pak archive
// into a platform-specific install directory and creates desktop shortcuts.

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <zlib.h>

namespace fs = std::filesystem;

// ── Platform detection (TODO #44) ──────────────────────────────
#if defined(_WIN32)
	#define PLATFORM_WINDOWS 1
	#include <windows.h>
	#include <shlobj.h>
#elif defined(__linux__)
	#define PLATFORM_LINUX 1
	#include <unistd.h>
	#include <sys/stat.h>
	#include <pwd.h>
#else
	#error "Unsupported platform. Only Windows x64 and Linux x64 are supported."
#endif

// ── Helper: get home directory ─────────────────────────────────
static std::string get_home_dir() {
#if PLATFORM_WINDOWS
	char* localAppData = std::getenv("USERPROFILE");
	if (localAppData)
		return std::string(localAppData);
	throw std::runtime_error("Could not determine home directory");
#elif PLATFORM_LINUX
	const char* home = std::getenv("HOME");
	if (home)
		return std::string(home);

	struct passwd* pw = getpwuid(getuid());
	if (pw)
		return std::string(pw->pw_dir);

	throw std::runtime_error("Could not determine home directory");
#endif
}

// ── wait_for_exit() (TODO #35) ─────────────────────────────────
// Flushes output and prompts user to press ENTER before exiting.
static void wait_for_exit() {
	std::cout.flush();
	std::cerr.flush();

	std::cout << "Press ENTER to exit...";
	std::cout.flush();
	std::cin.get();
}

// ── get_install_path() (TODO #36) ──────────────────────────────
// Returns platform-specific install directory.
static fs::path get_install_path() {
#if PLATFORM_WINDOWS
	const char* localAppData = std::getenv("LOCALAPPDATA");
	if (!localAppData)
		throw std::runtime_error("Could not determine LOCALAPPDATA directory");
	return fs::path(localAppData) / "wow.export";
#elif PLATFORM_LINUX
	return fs::path(get_home_dir()) / ".local" / "share" / "wow.export";
#endif
}

// ── get_executable_name() (TODO #37) ───────────────────────────
// Returns the platform-specific executable name.
// Per naming conventions, uses wow.export.cpp / wow.export.cpp.exe.
static std::string get_executable_name() {
#if PLATFORM_WINDOWS
	return "wow.export.cpp.exe";
#elif PLATFORM_LINUX
	return "wow.export.cpp";
#endif
}

// ── get_icon_path() (TODO #38) ─────────────────────────────────
// Returns the platform-specific icon path for shortcuts.
static fs::path get_icon_path(const fs::path& install_path) {
#if PLATFORM_WINDOWS
	return install_path / "res" / "icon.png";
#elif PLATFORM_LINUX
	return install_path / "res" / "icon.png";
#endif
}

// ── get_installer_dir() (TODO #40) ─────────────────────────────
// Returns the directory containing the installer executable itself.
static fs::path get_installer_dir() {
#if PLATFORM_WINDOWS
	char buf[MAX_PATH];
	DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		throw std::runtime_error("Failed to get installer executable path");
	return fs::path(buf).parent_path();
#elif PLATFORM_LINUX
	auto exe_path = fs::read_symlink("/proc/self/exe");
	return exe_path.parent_path();
#endif
}

// ── validate_data_pak() (TODO #41) ─────────────────────────────
// Checks that data.pak and data.pak.json exist next to the installer.
static void validate_data_pak() {
	fs::path installer_dir = get_installer_dir();
	fs::path manifest_path = installer_dir / "data.pak.json";
	fs::path data_path = installer_dir / "data.pak";

	if (!fs::exists(manifest_path) || !fs::exists(data_path)) {
		throw std::runtime_error(
			"Could not find data.pak and data.pak.json next to the installer.\n\n"
			"Please extract the entire archive before running the installer.\n"
			"Expected location: " + installer_dir.string()
		);
	}
}

// ── extract_data_pak() (TODO #42) ──────────────────────────────
// Core installation function: reads manifest, decompresses and extracts files.
static void extract_data_pak(const fs::path& install_path) {
	fs::path installer_dir = get_installer_dir();
	fs::path manifest_path = installer_dir / "data.pak.json";
	fs::path data_path = installer_dir / "data.pak";

	std::cout << "Reading installation manifest..." << std::endl;

	// Read and parse manifest
	std::ifstream manifest_file(manifest_path);
	if (!manifest_file.is_open())
		throw std::runtime_error("Failed to open manifest: " + manifest_path.string());

	nlohmann::json manifest;
	manifest_file >> manifest;
	manifest_file.close();

	// Read binary data.pak
	std::ifstream data_file(data_path, std::ios::binary | std::ios::ate);
	if (!data_file.is_open())
		throw std::runtime_error("Failed to open data.pak: " + data_path.string());

	std::streamsize data_size = data_file.tellg();
	data_file.seekg(0, std::ios::beg);

	std::vector<uint8_t> data(static_cast<size_t>(data_size));
	if (!data_file.read(reinterpret_cast<char*>(data.data()), data_size))
		throw std::runtime_error("Failed to read data.pak");
	data_file.close();

	const auto& contents = manifest["contents"];
	size_t total = contents.size();

	std::cout << "Extracting " << total << " files..." << std::endl;

	size_t extracted = 0;
	for (auto it = contents.begin(); it != contents.end(); ++it) {
		const std::string relative_path = it.key();
		const auto& entry = it.value();

		size_t offset = entry["ofs"].get<size_t>();
		size_t comp_size = entry["compSize"].get<size_t>();

		fs::path target_path = install_path / relative_path;

		// Create parent directories
		fs::create_directories(target_path.parent_path());

		// Extract compressed slice
		const uint8_t* compressed = data.data() + offset;

		// Decompress with zlib inflate
		// First, determine decompressed size by using a sufficiently large buffer
		// or by iterating. We'll use zlib's inflate in a loop.
		z_stream strm{};
		strm.next_in = const_cast<Bytef*>(compressed);
		strm.avail_in = static_cast<uInt>(comp_size);

		int ret = inflateInit(&strm);
		if (ret != Z_OK)
			throw std::runtime_error("inflateInit failed for: " + relative_path);

		std::vector<uint8_t> decompressed;
		const size_t CHUNK = 65536;
		std::vector<uint8_t> out_buf(CHUNK);

		do {
			strm.next_out = out_buf.data();
			strm.avail_out = static_cast<uInt>(CHUNK);

			ret = inflate(&strm, Z_NO_FLUSH);
			if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
				inflateEnd(&strm);
				throw std::runtime_error("inflate failed for: " + relative_path);
			}

			size_t have = CHUNK - strm.avail_out;
			decompressed.insert(decompressed.end(), out_buf.data(), out_buf.data() + have);
		} while (ret != Z_STREAM_END);

		inflateEnd(&strm);

		// Write decompressed file
		std::ofstream out_file(target_path, std::ios::binary);
		if (!out_file.is_open())
			throw std::runtime_error("Failed to write: " + target_path.string());

		out_file.write(reinterpret_cast<const char*>(decompressed.data()),
		               static_cast<std::streamsize>(decompressed.size()));
		out_file.close();

		extracted++;
		std::cout << "  [" << extracted << "/" << total << "] " << relative_path << std::endl;
	}

	std::cout << "Extracted " << extracted << " files successfully" << std::endl;

	// Set executable permissions on Linux
#if PLATFORM_LINUX
	fs::path exec_path = install_path / get_executable_name();
	fs::permissions(exec_path,
		fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
		fs::perms::others_read | fs::perms::others_exec);

	// Also chmod the updater
	fs::path updater_path = install_path / "updater";
	try {
		fs::permissions(updater_path,
			fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
			fs::perms::others_read | fs::perms::others_exec);
	} catch (...) {
		// updater may not exist
	}
#endif
}

// ── create_windows_shortcut() (TODO #39) ───────────────────────
#if PLATFORM_WINDOWS
static void create_windows_shortcut(const fs::path& exec_path) {
	std::string home = get_home_dir();
	fs::path desktop = fs::path(home) / "Desktop";
	fs::path shortcut_path = desktop / "wow.export.cpp.lnk";
	fs::path working_dir = exec_path.parent_path();

	// Escape backslashes for PowerShell
	auto escape_backslashes = [](std::string s) {
		std::string result;
		for (char c : s) {
			if (c == '\\')
				result += "\\\\";
			else
				result += c;
		}
		return result;
	};

	std::string ps_script =
		"$WshShell = New-Object -ComObject WScript.Shell; "
		"$Shortcut = $WshShell.CreateShortcut(\"" + escape_backslashes(shortcut_path.string()) + "\"); "
		"$Shortcut.TargetPath = \"" + escape_backslashes(exec_path.string()) + "\"; "
		"$Shortcut.WorkingDirectory = \"" + escape_backslashes(working_dir.string()) + "\"; "
		"$Shortcut.Description = \"Export Toolkit for World of Warcraft\"; "
		"$Shortcut.Save()";

	std::string command = "powershell -Command \"" + ps_script + "\"";
	int ret = std::system(command.c_str());

	if (ret == 0)
		std::cout << "Created desktop shortcut: " << shortcut_path.string() << std::endl;
	else
		std::cout << "WARN: Failed to create desktop shortcut" << std::endl;
}
#endif

// ── create_linux_shortcut() (TODO #39) ─────────────────────────
#if PLATFORM_LINUX
static void create_linux_shortcut(const fs::path& exec_path, const fs::path& icon_path) {
	fs::path applications_dir = fs::path(get_home_dir()) / ".local" / "share" / "applications";
	fs::create_directories(applications_dir);

	fs::path desktop_file = applications_dir / "wow-export.desktop";
	std::string content =
		"[Desktop Entry]\n"
		"Name=wow.export.cpp\n"
		"Comment=Export Toolkit for World of Warcraft\n"
		"Exec=\"" + exec_path.string() + "\"\n"
		"Icon=" + icon_path.string() + "\n"
		"Terminal=false\n"
		"Type=Application\n"
		"Categories=Utility;Game;\n";

	std::ofstream file(desktop_file);
	if (!file.is_open())
		throw std::runtime_error("Failed to write desktop entry: " + desktop_file.string());

	file << content;
	file.close();

	fs::permissions(desktop_file,
		fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
		fs::perms::others_read | fs::perms::others_exec);

	std::cout << "Created desktop entry: " << desktop_file.string() << std::endl;
}
#endif

// ── create_desktop_shortcut() (TODO #39) ───────────────────────
static void create_desktop_shortcut(const fs::path& install_path) {
	fs::path exec_path = install_path / get_executable_name();

#if PLATFORM_WINDOWS
	create_windows_shortcut(exec_path);
#elif PLATFORM_LINUX
	create_linux_shortcut(exec_path, get_icon_path(install_path));
#endif
}

// ── main() entry point (TODO #43) ──────────────────────────────
int main() {
	try {
		validate_data_pak();

		std::cout << std::endl;
		std::cout << "                                                      _   " << std::endl;
		std::cout << " __      _______      _______  ___ __   ___  _ __| |_ " << std::endl;
		std::cout << R"( \ \ /\ / / _ \ \ /\ / / _ \ \/ / '_ \ / _ \| '__| __|)" << std::endl;
		std::cout << R"(  \ V  V / (_) \ V  V /  __/>  <| |_) | (_) | |  | |_ )" << std::endl;
		std::cout << R"(   \_/\_/ \___/ \_/\_(_)___/_/\_\ .__/ \___/|_|   \__|)" << std::endl;
		std::cout << "                                |_|          .cpp     " << std::endl;
		std::cout << std::endl;

		fs::path install_path = get_install_path();
		std::cout << "Install location: " << install_path.string() << std::endl;
		std::cout << std::endl;

		fs::create_directories(install_path);

		extract_data_pak(install_path);

		std::cout << std::endl;
		std::cout << "Creating shortcuts..." << std::endl;
		create_desktop_shortcut(install_path);

		std::cout << std::endl;
		std::cout << "===========================================" << std::endl;
		std::cout << "  Installation complete!" << std::endl;
		std::cout << "===========================================" << std::endl;
		std::cout << std::endl;
		std::cout << "You can now launch wow.export.cpp from your desktop." << std::endl;
		std::cout << std::endl;

		wait_for_exit();
	} catch (const std::exception& e) {
		std::cerr << std::endl;
		std::cerr << "Installation failed: " << e.what() << std::endl;
		std::cerr << std::endl;

		wait_for_exit();
		return 1;
	}

	return 0;
}
