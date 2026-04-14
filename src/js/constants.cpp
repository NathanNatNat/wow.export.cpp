/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

// This file defines constants used throughout the application.
#include "constants.h"

#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <climits>
#endif

namespace constants {

namespace fs = std::filesystem;


static fs::path getExecutablePath() {
#ifdef _WIN32
	wchar_t buf[MAX_PATH];
	DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		return fs::current_path();
	return fs::path(std::wstring_view(buf, len));
#else
	char buf[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (len == -1)
		return fs::current_path();
	buf[len] = '\0';
	return fs::path(buf);
#endif
}


static fs::path getBlenderBaseDir() {
#ifdef _WIN32
	const char* appdata = std::getenv("APPDATA");
	if (appdata)
		return fs::path(appdata) / "Blender Foundation" / "Blender";
	return {};
#else
	// macOS case intentionally omitted — project scope is Windows and Linux only.
	// JS equivalent: case 'darwin': return path.join(home_dir, 'Library', 'Application Support', 'Blender');
	const char* home = std::getenv("HOME");
	if (home)
		return fs::path(home) / ".config" / "blender";
	return {};
#endif
}


namespace {
	fs::path s_install_path;
	fs::path s_data_dir;
	fs::path s_log_dir;
	fs::path s_runtime_log;
	fs::path s_last_export;
	fs::path s_shader_path;

	fs::path s_cache_dir;
	fs::path s_cache_size;
	fs::path s_cache_integrity_file;
	fs::path s_cache_dir_builds;
	fs::path s_cache_dir_indexes;
	fs::path s_cache_dir_data;
	fs::path s_cache_dir_dbd;
	fs::path s_cache_dir_listfile;
	fs::path s_cache_tact_keys;
	fs::path s_cache_realmlist;
	fs::path s_cache_state_file;

	fs::path s_config_default_path;
	fs::path s_config_user_path;

	fs::path s_update_directory;
	std::string s_update_helper;

	fs::path s_blender_dir;
	fs::path s_blender_local_dir;

	std::string s_user_agent;
}


void init() {
	// on macOS, process.execPath points to the renderer helper binary deep inside
	// the framework, not the app root. use __dirname (app.nw/src/) instead.
	// macOS is not supported in the C++ port — only Windows and Linux.
	s_install_path = getExecutablePath().parent_path();

	// Deviation from JS: JS uses `DATA_PATH = nw.App.dataPath` which is the
	// OS-specific user-data directory (e.g., %LOCALAPPDATA% on Windows,
	// ~/.config on Linux). C++ uses `<install>/data` — a subdirectory of the
	// install path. This is a deliberate platform adaptation: the C++ port
	// bundles resources (shaders, config, fonts) in data/ alongside the
	// executable, whereas NW.js stores user data separately from install data.
	s_data_dir = s_install_path / "data";

	// Deviation from JS: JS stores runtime.log in DATA_PATH directly.
	// C++ uses a separate Logs/ directory. This does not affect functionality
	// — only the log file location.
	s_log_dir = s_install_path / "Logs";

	// Deviation from JS: Legacy directory migration logic below does not exist
	// in the JS source. This is C++-specific code to handle migration from
	// earlier C++ port directory layouts (config/ → persistence/ → data/).
	// This is a safe addition — it only runs once on first launch after upgrade
	// and does not affect any JS-equivalent behavior.
	const fs::path legacyDirs[] = {
		s_install_path / "config",      // Original name
		s_install_path / "persistence"  // Previous rename
	};
	try {
		if (!fs::exists(s_data_dir)) {
			for (const auto& legacyDir : legacyDirs) {
				if (fs::exists(legacyDir)) {
					fs::rename(legacyDir, s_data_dir);
					break;
				}
			}
		}
	} catch (...) {
	}

	// Ensure data and log directories exist before any module attempts to
	// write to them (e.g. log.cpp creates a stream at require-time).
	fs::create_directories(s_data_dir);
	fs::create_directories(s_log_dir);

	// Deviation from JS: Legacy casc/ → cache/ migration does not exist in JS.
	// JS uses `path.join(DATA_PATH, 'casc')` for the cache directory name.
	// C++ renames it to `cache/` with a migration from legacy `casc/` on first
	// run. This is a C++-specific change; the directory content and structure
	// are identical. See also constants.h CACHE namespace.
	const auto legacyCascDir = s_data_dir / "casc";
	const auto newCacheDir = s_data_dir / "cache";
	try {
		if (fs::exists(legacyCascDir) && !fs::exists(newCacheDir))
			fs::rename(legacyCascDir, newCacheDir);
	} catch (...) {
	}

	// Compute derived paths.
	// Deviation from JS: JS RUNTIME_LOG is `path.join(DATA_PATH, 'runtime.log')`.
	// C++ uses LOG_DIR/runtime.log (separate Logs/ directory).
	s_runtime_log = s_log_dir / "runtime.log";
	s_last_export = s_data_dir / "last_export";

	// Deviation from JS: JS SHADER_PATH is `path.join(INSTALL_PATH, 'src', 'shaders')`.
	// C++ uses `<install>/data/shaders` because resources are bundled in the
	// data/ directory in the C++ port (POST_BUILD copy step in CMakeLists.txt).
	s_shader_path = s_data_dir / "shaders";

	s_cache_dir = s_data_dir / "cache";
	s_cache_size = s_cache_dir / "cachesize";
	s_cache_integrity_file = s_cache_dir / "cacheintegrity";
	s_cache_dir_builds = s_cache_dir / "builds";
	s_cache_dir_indexes = s_cache_dir / "indices";
	s_cache_dir_data = s_cache_dir / "data";
	s_cache_dir_dbd = s_cache_dir / "dbd";
	s_cache_dir_listfile = s_cache_dir / "listfile";
	s_cache_tact_keys = s_data_dir / "tact.json";
	s_cache_realmlist = s_data_dir / "realmlist.json";
	s_cache_state_file = s_data_dir / "cache_state.json";

	// Deviation from JS: CONFIG.DEFAULT_PATH is `path.join(INSTALL_PATH, 'src', 'default_config.jsonc')`.
	// C++ uses `<install>/data/default_config.jsonc` — same resource layout as SHADER_PATH.
	s_config_default_path = s_data_dir / "default_config.jsonc";
	s_config_user_path = s_data_dir / "config.json";

	s_update_directory = s_install_path / ".update";
#ifdef _WIN32
	s_update_helper = "updater.exe";
#else
	s_update_helper = "updater";
#endif

	s_blender_dir = getBlenderBaseDir();
	s_blender_local_dir = s_install_path / "addon" / "io_scene_wowobj";

	s_user_agent = std::string("wow.export (") + std::string(VERSION) + ")";
}


const fs::path& INSTALL_PATH() { return s_install_path; }
const fs::path& DATA_DIR() { return s_data_dir; }
const fs::path& LOG_DIR() { return s_log_dir; }
const fs::path& RUNTIME_LOG() { return s_runtime_log; }
const fs::path& LAST_EXPORT() { return s_last_export; }
const fs::path& SHADER_PATH() { return s_shader_path; }

namespace CACHE {
	const fs::path& DIR() { return s_cache_dir; }
	const fs::path& SIZE() { return s_cache_size; }
	const fs::path& INTEGRITY_FILE() { return s_cache_integrity_file; }
	const fs::path& DIR_BUILDS() { return s_cache_dir_builds; }
	const fs::path& DIR_INDEXES() { return s_cache_dir_indexes; }
	const fs::path& DIR_DATA() { return s_cache_dir_data; }
	const fs::path& DIR_DBD() { return s_cache_dir_dbd; }
	const fs::path& DIR_LISTFILE() { return s_cache_dir_listfile; }
	const fs::path& TACT_KEYS() { return s_cache_tact_keys; }
	const fs::path& REALMLIST() { return s_cache_realmlist; }
	const fs::path& STATE_FILE() { return s_cache_state_file; }
}

namespace CONFIG {
	const fs::path& DEFAULT_PATH() { return s_config_default_path; }
	const fs::path& USER_PATH() { return s_config_user_path; }
}

namespace BLENDER {
	const fs::path& DIR() { return s_blender_dir; }
	const fs::path& LOCAL_DIR() { return s_blender_local_dir; }
}

namespace UPDATE {
	const fs::path& DIRECTORY() { return s_update_directory; }
	const std::string& HELPER() { return s_update_helper; }
}

const std::string& USER_AGENT() { return s_user_agent; }

const std::regex& LISTFILE_MODEL_FILTER() {
	static const std::regex re(R"((_\d\d\d_)|(_\d\d\d.wmo$)|(lod\d.wmo$))");
	return re;
}


using namespace std::string_view_literals;

const std::array<FileIdentifier, 17> FILE_IDENTIFIERS = {{
	{ {{"OggS"sv}}, 1, ".ogg" },
	{ {{"ID3"sv, "\xFF\xFB"sv, "\xFF\xF3"sv, "\xFF\xF2"sv}}, 4, ".mp3" },
	{ {{"AFM2"sv}}, 1, ".anim" },
	{ {{"AFSA"sv}}, 1, ".anim" },
	{ {{"AFSB"sv}}, 1, ".anim" },
	{ {{"BLP2"sv}}, 1, ".blp" },
	{ {{"MD20"sv}}, 1, ".m2" },
	{ {{"MD21"sv}}, 1, ".m2" },
	{ {{"M3DT"sv}}, 1, ".m3" },
	{ {{"SKIN"sv}}, 1, ".skin" },
	// String concatenation required: \x00B would parse 'B' as hex digit (0x0B) in C++.
	{ {{"\x01\x00\x00\x00""BIDA"sv}}, 1, ".bone" },
	{ {{"SYHP\x02\x00\x00\x00"sv}}, 1, ".phys" },
	{ {{"HSXG"sv}}, 1, ".bls" },
	{ {{"RVXT"sv}}, 1, ".tex" },
	{ {{"RIFF"sv}}, 1, ".avi" },
	{ {{"WDC3"sv}}, 1, ".db2" },
	{ {{"WDC4"sv}}, 1, ".db2" }
}};

} // namespace constants