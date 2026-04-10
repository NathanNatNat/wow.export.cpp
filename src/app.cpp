/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "js/constants.h"
#include "js/generics.h"
// const updater = require('./js/updater'); // Removed: updater module deleted
#include "js/core.h"
#include "js/casc/listfile.h"
#include "js/casc/dbd-manifest.h"
#include "js/casc/cdn-resolver.h"
#include "js/log.h"
#include "js/config.h"
#include "js/casc/tact-keys.h"
#include "js/casc/export-helper.h"
// const ExternalLinks = require('./js/external-links'); // Removed: external-links module deleted
#include "js/ui/texture-ribbon.h"
#include "js/3D/Shaders.h"
#include "js/gpu-info.h"
#include "js/modules.h"
#include "js/modules/tab_textures.h"

// BUILD_RELEASE will be set by the bundler during production builds allowing us
// to discern a production build. For debugging builds, process.env.BUILD_RELEASE
// will be undefined. Any code that only runs when BUILD_RELEASE is false will
// be removed as dead-code during compile.
#ifdef NDEBUG
static constexpr bool BUILD_RELEASE = true;
#else
static constexpr bool BUILD_RELEASE = false;
#endif

// check for --disable-auto-update flag
static bool DISABLE_AUTO_UPDATE = false;

/**
 * crash() is used to inform the user that the application has exploded.
 * It is purposely global and primitive as we have no idea what state
 * the application will be in when it is called.
 * @param {string} errorCode
 * @param {string} errorText
 */
static bool isCrashed = false;
static std::string crashErrorCode;
static std::string crashErrorText;
static std::string crashLogDump;

static void crash(const std::string& errorCode, const std::string& errorText) {
	// Prevent a never-ending cycle of depression.
	if (isCrashed)
		return;

	isCrashed = true;

	// TODO(conversion): In JS, the crash function replaces the entire document
	// with a crash screen showing version/flavour/build, error code/text, and
	// log dump. In C++/ImGui, we store the crash state and render it in the
	// main loop as a full-window crash overlay.

	crashErrorCode = errorCode;
	crashErrorText = errorText;

	// getErrorDump is set as a global function by the log module.
	// This is used to get the contents of the runtime log without depending on the module.
	crashLogDump = getErrorDump();

	// If we can, emit a global event to the application informing of the crash.
	if (core::view)
		core::events.emit("crash");
}

// ── Crash screen rendering (ImGui equivalent of the <noscript> crash markup) ──

static void renderCrashScreen() {
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::Begin("##CrashScreen", nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("wow.export has crashed!");
	ImGui::Separator();

	// Show build version/flavour/ID.
	ImGui::Text("Version: v%s", std::string(constants::VERSION).c_str());

	// Display our error code/text.
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error Code: %s", crashErrorCode.c_str());
	ImGui::TextWrapped("Error Message: %s", crashErrorText.c_str());

	// Show log dump if available.
	if (!crashLogDump.empty()) {
		ImGui::Spacing();
		ImGui::Text("Runtime Log:");
		ImGui::BeginChild("##CrashLog", ImVec2(0, 0), ImGuiChildFlags_Borders);
		ImGui::TextUnformatted(crashLogDump.c_str(), crashLogDump.c_str() + crashLogDump.size());
		ImGui::EndChild();
	}

	ImGui::End();
}

// ── Platform-specific helpers for diagnostic logging ─────────────

static std::string getPlatformName() {
#ifdef _WIN32
	return "win32";
#elif defined(__linux__)
	return "linux";
#else
	return "unknown";
#endif
}

static std::string getArchName() {
#if defined(__x86_64__) || defined(_M_X64)
	return "x64";
#elif defined(__i386__) || defined(_M_IX86)
	return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
	return "arm64";
#else
	return "unknown";
#endif
}

static std::string getCPUModel() {
#ifdef _WIN32
	// Read from registry or environment
	char buf[256] = {};
	DWORD bufSize = sizeof(buf);
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr,
			reinterpret_cast<LPBYTE>(buf), &bufSize);
		RegCloseKey(hKey);
	}
	std::string model(buf);
	if (model.empty())
		model = "Unknown CPU";
	return model;
#elif defined(__linux__)
	std::ifstream cpuinfo("/proc/cpuinfo");
	std::string line;
	while (std::getline(cpuinfo, line)) {
		if (line.find("model name") == 0) {
			auto pos = line.find(':');
			if (pos != std::string::npos) {
				std::string model = line.substr(pos + 1);
				// Trim leading whitespace
				auto start = model.find_first_not_of(" \t");
				if (start != std::string::npos)
					model = model.substr(start);
				return model;
			}
		}
	}
	return "Unknown CPU";
#else
	return "Unknown CPU";
#endif
}

static int getCPUCoreCount() {
	int count = static_cast<int>(std::thread::hardware_concurrency());
	return count > 0 ? count : 1;
}

static uint64_t getTotalMemory() {
#ifdef _WIN32
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullTotalPhys;
#elif defined(__linux__)
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (pages > 0 && page_size > 0)
		return static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size);
	return 0;
#else
	return 0;
#endif
}

static uint64_t getFreeMemory() {
#ifdef _WIN32
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullAvailPhys;
#elif defined(__linux__)
	long pages = sysconf(_SC_AVPHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (pages > 0 && page_size > 0)
		return static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size);
	return 0;
#else
	return 0;
#endif
}

static std::filesystem::path getHomeDir() {
#ifdef _WIN32
	const char* userProfile = std::getenv("USERPROFILE");
	if (userProfile)
		return std::filesystem::path(userProfile);
	const char* homeDrive = std::getenv("HOMEDRIVE");
	const char* homePath = std::getenv("HOMEPATH");
	if (homeDrive && homePath)
		return std::filesystem::path(std::string(homeDrive) + homePath);
	return std::filesystem::path("C:\\");
#else
	const char* home = std::getenv("HOME");
	if (home)
		return std::filesystem::path(home);
	return std::filesystem::path("/tmp");
#endif
}

// ── GLFW drop callback for file drag/drop ────────────────────────

static void glfw_drop_callback(GLFWwindow* /*window*/, int count, const char** paths) {
	if (count <= 0 || !paths)
		return;

	if (!core::view)
		return;

	core::view->fileDropPrompt = nullptr;

	const DropHandler* handler = core::getDropHandler(paths[0]);
	if (handler) {
		// Since dataTransfer.files is a FileList, we need to iterate it the old fashioned way.
		std::vector<std::string> include;
		for (int i = 0; i < count; ++i) {
			std::string check = paths[i];
			std::string lower;
			lower.reserve(check.size());
			for (char c : check)
				lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

			bool match = false;
			for (const auto& ext : handler->ext) {
				if (lower.size() >= ext.size() &&
					lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0) {
					match = true;
					break;
				}
			}

			if (match)
				include.push_back(paths[i]);
		}

		if (!include.empty() && handler->process)
			handler->process(include[0]);
	}
}

// ── Cache size file management ───────────────────────────────────

static int64_t prevCacheSize = -1;
static std::chrono::steady_clock::time_point cacheSizeUpdateScheduledAt;
static bool cacheSizeUpdatePending = false;

static void loadCacheSize() {
	// Load cachesize, a file used to track the overall size of the cache directory
	// without having to calculate the real size before showing to users. Fast and reliable.
	try {
		std::ifstream in(constants::CACHE::SIZE());
		if (in.is_open()) {
			std::string data;
			std::getline(in, data);
			int64_t val = 0;
			try { val = std::stoll(data); } catch (...) { val = 0; }
			core::view->cacheSize = val;
		}
	} catch (...) {
		// Ignore errors — cacheSize stays at 0.
	}

	// Record initial value to prevent needless file write.
	prevCacheSize = core::view->cacheSize;
}

static void checkCacheSizeUpdate() {
	if (!core::view)
		return;

	// Create a watcher programmatically *after* assigning the initial value
	// to prevent a needless file write by triggering itself during init.
	if (core::view->cacheSize != prevCacheSize) {
		prevCacheSize = core::view->cacheSize;

		// We buffer this call by SIZE_UPDATE_DELAY so that we're not writing
		// to the file constantly during heavy cache usage. Postponing until
		// next tick would not help due to async and potential IO/net delay.
		cacheSizeUpdateScheduledAt = std::chrono::steady_clock::now();
		cacheSizeUpdatePending = true;
	}

	if (cacheSizeUpdatePending) {
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - cacheSizeUpdateScheduledAt).count();
		if (elapsed >= constants::CACHE::SIZE_UPDATE_DELAY) {
			cacheSizeUpdatePending = false;
			try {
				std::ofstream out(constants::CACHE::SIZE());
				out << core::view->cacheSize;
			} catch (...) {
				// Ignore write errors.
			}
		}
	}
}

// ── Vue methods / computed / watch equivalents ───────────────────
// In C++/ImGui, these are called directly from the modules or from
// the main loop. They are defined here to preserve the JS structure.

namespace app {

/**
 * Opens the runtime application log from the application data directory.
 */
static void openRuntimeLog() {
	logging::openRuntimeLog();
}

/**
 * Reloads all stylesheets in the document.
 */
static void reloadStylesheet() {
	// TODO(conversion): In ImGui, there are no CSS stylesheets. This is a no-op.
	// The JS version reloads <link> tags; ImGui styling is done via ImGuiStyle.
}

/**
 * Reload the currently active module.
 */
static void reloadActiveModule() {
	modules::reloadActiveModule();
}

/**
 * Reload all loaded modules.
 */
static void reloadAllModules() {
	modules::reloadAllModules();
}

/**
 * Mark all WMO groups to the given state.
 * @param {boolean} state
 */
static void setAllWMOGroups(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->modelViewerWMOGroups)
		node["checked"] = state;
}

/**
 * Mark all decor geosets to the given state.
 * @param {boolean} state
 */
static void setAllDecorGeosets(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->decorViewerGeosets)
		node["checked"] = state;
}

/**
 * Mark all decor WMO groups to the given state.
 * @param {boolean} state
 */
static void setAllDecorWMOGroups(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->decorViewerWMOGroups)
		node["checked"] = state;
}

/**
 * Mark all creature geosets to the given state.
 * @param {boolean} state
 */
static void setAllCreatureGeosets(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->creatureViewerGeosets)
		node["checked"] = state;
}

/**
 * Mark all creature equipment toggles to the given state.
 * @param {boolean} state
 */
static void setAllCreatureEquipment(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->creatureViewerEquipment)
		node["checked"] = state;
}

/**
 * Mark all creature WMO groups to the given state.
 * @param {boolean} state
 */
static void setAllCreatureWMOGroups(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->creatureViewerWMOGroups)
		node["checked"] = state;
}

/**
 * Toggle UV layer for the main model viewer.
 * @param {string} layerName
 */
static void toggleUVLayer(const std::string& layerName) {
	// In JS: core.events.emit('toggle-uv-layer', layerName);
	core::events.emit("toggle-uv-layer", layerName);
}

/**
 * Mark all geosets to the given state.
 * @param {boolean} state
 * @param {object} geosets
 */
static void setAllGeosets(bool state, std::vector<nlohmann::json>& geosets) {
	for (auto& node : geosets)
		node["checked"] = state;
}

/**
 * Mark all decor category subcategories to the given state.
 * @param {boolean} state
 */
static void setAllDecorCategories(bool state) {
	if (!core::view)
		return;
	for (auto& entry : core::view->decorCategoryMask)
		entry["checked"] = state;
}

/**
 * Mark all subcategories within a category group to the given state.
 * @param {number} category_id
 * @param {boolean} state
 */
static void setDecorCategoryGroup(int category_id, bool state) {
	if (!core::view)
		return;
	for (auto& entry : core::view->decorCategoryMask) {
		if (entry.contains("categoryID") && entry["categoryID"].get<int>() == category_id)
			entry["checked"] = state;
	}
}

/**
 * Mark all item types to the given state.
 * @param {boolean} state
 */
static void setAllItemTypes(bool /*state*/) {
	// TODO(conversion): itemViewerTypeMask is std::vector<int>, not JSON with checked field.
	// The JS version sets entry.checked for each entry. In C++, the mask is handled
	// differently by the item viewer module.
}

/**
 * Mark all item qualities to the given state.
 * @param {boolean} state
 */
static void setAllItemQualities(bool /*state*/) {
	// TODO(conversion): itemViewerQualityMask is std::vector<int>, not JSON with checked field.
	// The JS version sets entry.checked for each entry. In C++, the mask is handled
	// differently by the item viewer module.
}

/**
 * Return a tag for a given product.
 * @param {string} product
 */
static std::string getProductTag(std::string_view product) {
	auto it = std::find_if(constants::PRODUCTS.begin(), constants::PRODUCTS.end(),
		[&](const constants::Product& e) { return e.product == product; });
	return it != constants::PRODUCTS.end() ? std::string(it->tag) : std::string("Unknown");
}

static void setActiveModule(const std::string& module_name) {
	modules::setActive(module_name);
}

static void handleContextMenuClick(const modules::ContextMenuOption& opt) {
	if (opt.handler)
		opt.handler();
	else
		modules::setActive(opt.id);
}

/**
 * Invoked when a toast option is clicked.
 * The tag is passed to our global event emitter.
 * @param {string} tag
 */
static void handleToastOptionClick(const std::function<void()>& func) {
	if (core::view)
		core::view->toast.reset();

	if (func)
		func();
}

/**
 * Invoked when a user cancels a model override filter.
 */
static void removeOverrideModels() {
	if (!core::view)
		return;
	core::view->overrideModelList.clear();
	core::view->overrideModelName.clear();
}

/**
 * Invoked when a user cancels a texture override filter.
 */
static void removeOverrideTextures() {
	if (!core::view)
		return;
	core::view->overrideTextureList.clear();
	core::view->overrideTextureName.clear();
}

/**
 * Invoked when the user manually selects a CDN region.
 * @param {object} region
 */
static void setSelectedCDN(const nlohmann::json& region) {
	if (!core::view)
		return;
	core::view->selectedCDNRegion = region;
	core::view->lockCDNRegion = true;
	core::view->config["sourceSelectUserRegion"] = region.value("tag", "");
	casc::cdn_resolver::startPreResolution(region.value("tag", ""));
}

/**
 * Emit an event using the global event emitter.
 * @param {string} tag
 * @param {object} event
 */
static void click(const std::string& tag) {
	// TODO(conversion): In JS, this checks event.target.classList.contains('disabled')
	// before emitting. In ImGui, disabled state is handled differently by each widget.
	core::events.emit("click-" + tag);
}

/**
 * Pass-through function to emit events from reactive markup.
 * @param {string} tag
 * @param  {...any} params
 */
static void emit(const std::string& tag) {
	core::events.emit(tag);
}

/**
 * Hide the toast bar.
 * @param {boolean} userCancel
 */
static void hideToast(bool userCancel = false) {
	core::hideToast(userCancel);
}

/**
 * Restart the application.
 * JS equivalent: chrome.runtime.reload() — reloads the NW.js app.
 * C++ equivalent: re-exec the current process binary.
 */
void restartApplication() {
#ifdef _WIN32
	wchar_t exe_path[MAX_PATH];
	DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
	if (len > 0 && len < MAX_PATH) {
		STARTUPINFOW si{};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		if (CreateProcessW(exe_path, GetCommandLineW(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
	}
	std::exit(0);
#else
	char exe_path[PATH_MAX];
	ssize_t path_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
	if (path_len > 0) {
		exe_path[path_len] = '\0';
		execl(exe_path, exe_path, nullptr);
	}
	// execl only returns on error — fall through to exit.
	std::exit(0);
#endif
}

/**
 * Invoked when the texture ribbon element on the model viewer
 * fires a resize event.
 */
static void onTextureRibbonResize(int width) {
	texture_ribbon::onResize(width);
}

/**
 * Copy given data as text to the system clipboard.
 * @param {string} data
 */
static void copyToClipboard(const std::string& data) {
	ImGui::SetClipboardText(data.c_str());
}

/**
 * Get the external export path for a given file.
 * @param {string} file
 * @returns {string}
 */
static std::string getExportPath(const std::string& file) {
	return casc::ExportHelper::getExportPath(file);
}

// Removed: getExternalLink() — external-links module deleted
static void getExternalLink() {
	return; // Removed: external-links module deleted
}

// ── Computed property equivalents ────────────────────────────────

/**
 * Return the formatted duration of the selected track on the sound player.
 */
static std::string soundPlayerDurationFormatted() {
	if (!core::view)
		return "00:00";
	return generics::formatPlaybackSeconds(core::view->soundPlayerDuration);
}

/**
 * Return the formatted current seek of the selected track on the sound player.
 */
static std::string soundPlayerSeekFormatted() {
	if (!core::view)
		return "00:00";
	return generics::formatPlaybackSeconds(core::view->soundPlayerSeek * core::view->soundPlayerDuration);
}

/**
 * Returns the maximum amount of pages needed for the texture ribbon.
 * @returns {number}
 */
static int textureRibbonMaxPages() {
	if (!core::view || core::view->textureRibbonSlotCount <= 0)
		return 0;
	return static_cast<int>(std::ceil(
		static_cast<double>(core::view->textureRibbonStack.size()) /
		core::view->textureRibbonSlotCount));
}

/**
 * Returns the texture ribbon stack array subject to paging.
 * @returns {Array}
 */
static std::vector<nlohmann::json> textureRibbonDisplay() {
	if (!core::view)
		return {};
	int startIndex = core::view->textureRibbonPage * core::view->textureRibbonSlotCount;
	int endIndex = startIndex + core::view->textureRibbonSlotCount;
	auto& stack = core::view->textureRibbonStack;
	if (startIndex >= static_cast<int>(stack.size()))
		return {};
	endIndex = (std::min)(endIndex, static_cast<int>(stack.size()));
	return std::vector<nlohmann::json>(
		stack.begin() + startIndex,
		stack.begin() + endIndex);
}

} // namespace app

// ── Watch equivalents (change detection in the main loop) ────────

static double prevLoadPct = -1;
static void* prevCasc = nullptr;
static nlohmann::json prevActiveModule;

#ifdef _WIN32
static ITaskbarList3* s_taskbar = nullptr;
static bool s_com_initialized = false;

static void initTaskbarProgress() {
	if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
		s_com_initialized = true;
		CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
			IID_ITaskbarList3, reinterpret_cast<void**>(&s_taskbar));
		if (s_taskbar)
			s_taskbar->HrInit();
	}
}

static void setTaskbarProgress(GLFWwindow* window, double val) {
	if (!s_taskbar)
		return;

	HWND hwnd = glfwGetWin32Window(window);
	if (!hwnd)
		return;

	if (val < 0 || val >= 1.0) {
		s_taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
	} else {
		s_taskbar->SetProgressState(hwnd, TBPF_NORMAL);
		s_taskbar->SetProgressValue(hwnd, static_cast<ULONGLONG>(val * 10000), 10000);
	}
}
#endif

static void checkWatchers(GLFWwindow* window) {
	if (!core::view)
		return;

	/**
	 * Invoked when the active loading percentage is changed.
	 * @param {float} val
	 */
	if (core::view->loadPct != prevLoadPct) {
		prevLoadPct = core::view->loadPct;
		// JS: win.setProgressBar(val) sets taskbar progress.
#ifdef _WIN32
		setTaskbarProgress(window, prevLoadPct);
#else
		// Linux: no standard taskbar progress API; no-op.
		(void)window;
#endif
	}

	/**
	 * Invoked when the core CASC instance is changed.
	 */
	void* currentCasc = static_cast<void*>(core::view->casc);
	if (currentCasc != prevCasc) {
		prevCasc = currentCasc;
		core::events.emit("casc-source-changed");
	}

	// watch activeModule and close context menus when it changes
	if (core::view->activeModule != prevActiveModule) {
		prevActiveModule = core::view->activeModule;
		auto& contextMenus = core::view->contextMenus;
		contextMenus.stateNavExtra = false;
		contextMenus.stateModelExport = false;
		contextMenus.stateCDNRegion = false;
		contextMenus.nodeTextureRibbon = nullptr;
		contextMenus.nodeItem = nullptr;
		contextMenus.nodeDataTable = nullptr;
		contextMenus.nodeListbox = nullptr;
		contextMenus.nodeMap = nullptr;
		contextMenus.nodeZone = nullptr;
	}
}

// ── Main entry point ─────────────────────────────────────────────

int main(int argc, char* argv[]) {
	// check for --disable-auto-update flag
	for (int i = 1; i < argc; ++i) {
		if (std::string_view(argv[i]) == "--disable-auto-update")
			DISABLE_AUTO_UPDATE = true;
	}

	// Initialize runtime paths (INSTALL_PATH, DATA_DIR, LOG_DIR, etc.)
	constants::init();

	// Initialize logging stream.
	logging::init();

	// ── GLFW / OpenGL / ImGui initialization ─────────────────────

	if (!glfwInit()) {
		crash("ERR_GLFW_INIT", "Failed to initialize GLFW");
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

	// Append the application version to the title bar.
	std::string windowTitle = std::format("wow.export v{}", constants::VERSION);
	GLFWwindow* window = glfwCreateWindow(1280, 720, windowTitle.c_str(), nullptr, nullptr);
	if (!window) {
		crash("ERR_WINDOW_CREATE", "Failed to create GLFW window");
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // VSync

#ifdef _WIN32
	// Initialize Windows taskbar progress (ITaskbarList3).
	initTaskbarProgress();
#endif

	// Load OpenGL function pointers via GLAD2.
	if (!gladLoadGL(glfwGetProcAddress)) {
		crash("ERR_GLAD_INIT", "Failed to initialize OpenGL loader (GLAD2)");
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	// Prevent files from being dropped onto the window. These are over-written
	// later but we disable here to prevent them working if init fails.
	// (GLFW does not have ondragover; drop callback is set later.)

	// Force all links to open in the users default application.
	// TODO(conversion): In ImGui, external links are opened via platform shell commands
	// rather than DOM click events. Each module handles this directly.
	// ExternalLinks.open(externalElement.getAttribute('data-external')); // Removed: external-links module deleted

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup Dear ImGui style (will be refined to match app.css)
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	// ── Application state initialization ─────────────────────────

	// Initialize Vue equivalent: create AppState and assign to core::view.
	static AppState appState = core::makeNewView();
	core::view = &appState;

	// Interlink error handling for Vue.
	// TODO(conversion): In JS, app.config.errorHandler catches Vue render errors.
	// In C++, exceptions during render are caught by the main loop try/catch.

	modules::register_components();

	// dynamic interface scaling for smaller displays
	// TODO(conversion): In ImGui, scaling is handled via ImGui::GetIO().FontGlobalScale
	// and framebuffer DPI. The JS version scales a DOM container; the equivalent
	// in ImGui is adjusting the global font scale based on window size.
	constexpr int SCALE_THRESHOLD_W = 1120;
	constexpr int SCALE_THRESHOLD_H = 700;

	modules::initialize();

	// register static context menu options
	modules::registerContextMenuOption("runtime-log", "Open Runtime Log", "timeline.svg", []() { logging::openRuntimeLog(); });
	modules::registerContextMenuOption("restart", "Restart wow.export", "arrow-rotate-left.svg", []() { app::restartApplication(); });
	modules::registerContextMenuOption("reload-style", "Reload Styling", "palette.svg", []() { app::reloadStylesheet(); }, true);
	modules::registerContextMenuOption("reload-shaders", "Reload Shaders", "cube.svg", []() { shaders::reload_all(); }, true);
	modules::registerContextMenuOption("reload-active", "Reload Active Module", "gear.svg", []() { modules::reloadActiveModule(); }, true);
	modules::registerContextMenuOption("reload-all", "Reload All Modules", "gear.svg", []() { modules::reloadAllModules(); }, true);

	// Log some basic information for potential diagnostics.
	logging::write(std::format("wow.export has started v{}", constants::VERSION));
	logging::write(std::format("Host {} ({}), CPU {} ({} cores), Memory {} / {}",
		getPlatformName(), getArchName(), getCPUModel(), getCPUCoreCount(),
		generics::filesize(static_cast<double>(getFreeMemory())),
		generics::filesize(static_cast<double>(getTotalMemory()))));
	logging::write(std::format("INSTALL_PATH {} DATA_DIR {} LOG_DIR {}",
		constants::INSTALL_PATH().string(),
		constants::DATA_DIR().string(),
		constants::LOG_DIR().string()));

	// log gpu info async to avoid blocking startup
	gpu_info::log_gpu_info();

	// Load configuration.
	config::load();

	// Set-up default export directory if none configured.
	if (core::view->config.contains("exportDirectory") &&
		core::view->config["exportDirectory"].get<std::string>().empty()) {
		std::string defaultExportDir = (getHomeDir() / "wow.export").string();
		core::view->config["exportDirectory"] = defaultExportDir;
		logging::write(std::format("No export directory set, setting to {}", defaultExportDir));
	} else if (!core::view->config.contains("exportDirectory")) {
		std::string defaultExportDir = (getHomeDir() / "wow.export").string();
		core::view->config["exportDirectory"] = defaultExportDir;
		logging::write(std::format("No export directory set, setting to {}", defaultExportDir));
	}

	casc::listfile::preload();
	casc::dbd_manifest::preload();

	// Set-up proper drag/drop handlers.
	glfwSetDropCallback(window, glfw_drop_callback);

	//window.ondragover = e => { e.preventDefault(); return false; };
	//window.ondrop = e => { e.preventDefault(); return false; };

	loadCacheSize();

	// Load/update BLTE decryption keys.
	casc::tact_keys::load();

	// Removed: auto-update check — updater module deleted
	// if (BUILD_RELEASE && !DISABLE_AUTO_UPDATE) {
	// 	core::showLoadingScreen(1, "Checking for updates...");
	// 	updater::checkForUpdates().then(updateAvailable => {
	// 		if (updateAvailable) {
	// 			updater::applyUpdate();
	// 		} else {
	// 			core::hideLoadingScreen();
	// 			modules::setActive("source_select");
	// 		}
	// 	});
	// }

	// Load what's new HTML on app start
	// Removed: whats-new.html is no longer used — home page is a blank placeholder.
	// (async () => {
	// 	try {
	// 		const whats_new_path = BUILD_RELEASE ? './src/whats-new.html' : './src/whats-new.html';
	// 		const html = await fsp.readFile(whats_new_path, 'utf8');
	// 		core.view.whatsNewHTML = html;
	// 	} catch (e) {
	// 		log.write('failed to load whats-new.html: %o', e);
	// 	}
	// })();

	// Set source select as the currently active interface screen.
	modules::setActive("source_select");

	// Initialize watch state trackers to current values.
	prevLoadPct = core::view->loadPct;
	prevCasc = static_cast<void*>(core::view->casc);
	prevActiveModule = core::view->activeModule;

	// ── Main loop ────────────────────────────────────────────────

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Debugging reloader.
		if (!BUILD_RELEASE) {
			if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {
				// In NW.js, F5 reloads the app via chrome.runtime.reload().
				// C++ equivalent: re-exec the current process binary.
				app::restartApplication();
			}
		}

		// dynamic interface scaling for smaller displays
		{
			int win_w, win_h;
			glfwGetWindowSize(window, &win_w, &win_h);
			float scale_w = win_w < SCALE_THRESHOLD_W ? static_cast<float>(win_w) / SCALE_THRESHOLD_W : 1.0f;
			float scale_h = win_h < SCALE_THRESHOLD_H ? static_cast<float>(win_h) / SCALE_THRESHOLD_H : 1.0f;
			float scale = (std::min)(scale_w, scale_h);
			io.FontGlobalScale = scale;
		}

		// Check watchers (loadPct, casc, activeModule)
		checkWatchers(window);

		// Check cache size update timer
		checkCacheSizeUpdate();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		try {
			if (isCrashed) {
				renderCrashScreen();
			} else {
				// Render the active module
				modules::ModuleDef* active = modules::getActive();
				if (active && active->render)
					active->render();
			}
		} catch (const std::exception& e) {
			crash("ERR_RENDER", e.what());
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.204f, 0.227f, 0.251f, 1.0f); // --background: #343a40
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// ── Cleanup ──────────────────────────────────────────────────

#ifdef _WIN32
	if (s_taskbar) {
		s_taskbar->Release();
		s_taskbar = nullptr;
	}
	if (s_com_initialized)
		CoUninitialize();
#endif

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}