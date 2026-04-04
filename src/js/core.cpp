/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#endif

#include "core.h"
#include "generics.h"
#include "casc/locale-flags.h"
#include "constants.h"
#include "log.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <format>
#include <memory>
#include <thread>

// ─── EventEmitter ────────────────────────────────────────────────

void EventEmitter::setMaxListeners(int max) {
	maxListeners = max;
}

size_t EventEmitter::on(const std::string& event, Callback callback) {
	size_t id = nextId++;
	listeners[event].push_back({ id, std::move(callback) });
	return id;
}

size_t EventEmitter::once(const std::string& event, Callback callback) {
	auto idPtr = std::make_shared<size_t>(0);
	*idPtr = on(event, [this, event, idPtr, cb = std::move(callback)]() {
		off(event, *idPtr);
		cb();
	});
	return *idPtr;
}

void EventEmitter::off(const std::string& event, size_t id) {
	auto it = listeners.find(event);
	if (it == listeners.end())
		return;

	auto& vec = it->second;
	vec.erase(
		std::remove_if(vec.begin(), vec.end(), [id](const Listener& l) { return l.id == id; }),
		vec.end()
	);
}

void EventEmitter::emit(const std::string& event) {
	auto it = listeners.find(event);
	if (it == listeners.end())
		return;

	// Copy the vector in case listeners modify it during iteration.
	auto callbacks = it->second;
	for (const auto& listener : callbacks)
		listener.callback();
}

void EventEmitter::removeAllListeners(const std::string& event) {
	listeners.erase(event);
}

// ─── BusyLock ────────────────────────────────────────────────────

BusyLock::BusyLock(AppState& state)
	: state(&state) {
	this->state->isBusy++;
}

BusyLock::~BusyLock() {
	if (state)
		state->isBusy--;
}

BusyLock::BusyLock(BusyLock&& other) noexcept
	: state(other.state) {
	other.state = nullptr;
}

BusyLock& BusyLock::operator=(BusyLock&& other) noexcept {
	if (this != &other) {
		if (state)
			state->isBusy--;
		state = other.state;
		other.state = nullptr;
	}
	return *this;
}

// ─── core namespace ──────────────────────────────────────────────

namespace core {

// core.events is a global event handler used for dispatching
// events from any point in the system, to any other point.
EventEmitter events;

// The `view` object is used as a reference to the data for the main Vue instance.
AppState* view = nullptr;

// Used by setToast() for TTL toast prompts.
static int toastTimer = -1;
static std::chrono::steady_clock::time_point toastExpiry;

// dropHandlers contains handlers for drag/drop support.
// Each item is an object defining .ext, .prompt() and .process().
static std::vector<DropHandler> dropHandlers;

// scrollPositions stores persistent scroll positions for listbox components
// keyed by persistScrollKey (e.g., "models", "textures", etc.)
static std::unordered_map<std::string, ScrollPosition> scrollPositions;

// internal progress state for loading screen api
static int loading_progress_segments = 1;
static int loading_progress_value = 0;

AppState makeNewView() {
	AppState state;

	// Determine if it's December (month 11 in JS Date, month 12 in std::tm).
	const auto now = std::chrono::system_clock::now();
	const auto time = std::chrono::system_clock::to_time_t(now);
	std::tm tm_buf;
#ifdef _WIN32
	localtime_s(&tm_buf, &time);
#else
	localtime_r(&time, &tm_buf);
#endif
	state.isXmas = (tm_buf.tm_mon == 11); // tm_mon is 0-based, December = 11

	// Initialize menu button arrays matching JS source exactly.
	state.menuButtonTextures = {
		{ "Export as PNG", "PNG" },
		{ "Export as WebP", "WEBP" },
		{ "Export as BLP (Raw)", "BLP" },
		{ "Copy to Clipboard", "CLIPBOARD" }
	};

	state.menuButtonMapExport = {
		{ "Export OBJ", "OBJ" },
		{ "Export PNG", "PNG" },
		{ "Export Raw", "RAW" },
		{ "Export Heightmaps", "HEIGHTMAPS" }
	};

	state.menuButtonTextureQuality = {
		{ "Alpha Maps", -1 },
		{ "None", 0 },
		{ "Minimap (512)", 512 },
		{ "Low (1k)", 1024 },
		{ "Medium (4k)", 4096 },
		{ "High (8k)", 8192 },
		{ "Ultra (16k)", 16384 }
	};

	state.menuButtonHeightmapResolution = {
		{ "64x64", 64 },
		{ "128x128", 128 },
		{ "512x512", 512 },
		{ "1024x1024 (1k)", 1024 },
		{ "2048x2048 (2k)", 2048 },
		{ "4096x4096 (4k)", 4096 },
		{ "Custom", -1 }
	};

	state.menuButtonHeightmapBitDepth = {
		{ "8-bit Depth", 8 },
		{ "16-bit Depth", 16 },
		{ "32-bit Depth", 32 }
	};

	state.menuButtonModels = {
		{ "Export OBJ", "OBJ" },
		{ "Export STL", "STL" },
		{ "Export glTF", "GLTF" },
		{ "Export GLB", "GLB" },
		{ "Export M2 / WMO (Raw)", "RAW" },
		{ "Export PNG (3D Preview)", "PNG" },
		{ "Copy to Clipboard (3D Preview)", "CLIPBOARD" }
	};

	state.menuButtonLegacyModels = {
		{ "Export OBJ", "OBJ" },
		{ "Export STL", "STL" },
		{ "Export Raw", "RAW" },
		{ "Export PNG (3D Preview)", "PNG" },
		{ "Copy to Clipboard (3D Preview)", "CLIPBOARD" }
	};

	state.menuButtonDecor = {
		{ "Export OBJ", "OBJ" },
		{ "Export STL", "STL" },
		{ "Export glTF", "GLTF" },
		{ "Export GLB", "GLB" },
		{ "Export M2 / WMO (Raw)", "RAW" },
		{ "Export PNG (3D Preview)", "PNG" },
		{ "Copy to Clipboard (3D Preview)", "CLIPBOARD" }
	};

	state.menuButtonCreatures = {
		{ "Export OBJ", "OBJ" },
		{ "Export STL", "STL" },
		{ "Export glTF", "GLTF" },
		{ "Export GLB", "GLB" },
		{ "Export M2 / WMO (Raw)", "RAW" },
		{ "Export PNG (3D Preview)", "PNG" },
		{ "Copy to Clipboard (3D Preview)", "CLIPBOARD" }
	};

	state.menuButtonCharacterExport = {
		{ "Export glTF", "GLTF" },
		{ "Export GLB", "GLB" },
		{ "Export OBJ (Posed)", "OBJ" },
		{ "Export STL (Posed)", "STL" },
		{ "Export PNG (3D Preview)", "PNG" },
		{ "Copy to Clipboard (3D Preview)", "CLIPBOARD" }
	};

	state.menuButtonVideos = {
		{ "Export MP4 (Video + Audio)", "MP4" },
		{ "Export AVI (Video Only)", "AVI" },
		{ "Export MP3 (Audio Only)", "MP3" },
		{ "Export Subtitles", "SUBTITLES" }
	};

	state.menuButtonData = {
		{ "Export as CSV", "CSV" },
		{ "Export as SQL", "SQL" },
		{ "Export DB2 (Raw)", "DB2" }
	};

	return state;
}

/**
 * Open a stream to the last export file.
 */
FileWriter openLastExportStream() {
	return FileWriter(constants::LAST_EXPORT(), "utf8");
}

/**
 * Creates an RAII busy lock that increments isBusy on creation and
 * decrements on destruction.
 */
BusyLock create_busy_lock() {
	return BusyLock(*view);
}

/**
 * Show loading screen with specified number of progress steps.
 */
void showLoadingScreen(int segments, const std::string& title) {
	loading_progress_segments = segments;
	loading_progress_value = 0;
	view->loadPct = 0;
	view->loadingTitle = title;
	view->isLoading = true;
	view->isBusy++;
}

/**
 * Advance loading screen progress by one step.
 */
void progressLoadingScreen(const std::string& text) {
	loading_progress_value++;
	view->loadPct = std::min(
		static_cast<double>(loading_progress_value) / loading_progress_segments, 1.0
	);

	if (!text.empty())
		view->loadingProgress = text;

	generics::redraw();
}

/**
 * Hide loading screen.
 */
void hideLoadingScreen() {
	view->loadPct = -1;
	view->isLoading = false;
	view->isBusy--;
}

/**
 * Hide the currently active toast prompt.
 */
void hideToast(bool userCancel) {
	// Cancel outstanding toast expiry timer.
	if (toastTimer > -1) {
		toastTimer = -1;
	}

	view->toast = std::nullopt;

	if (userCancel)
		events.emit("toast-cancelled");
}

/**
 * Display a toast message.
 */
void setToast(const std::string& toastType, const std::string& message,
              const nlohmann::json& options, int ttl, bool closable) {
	Toast toast;
	toast.type = toastType;
	toast.message = message;
	toast.options = options;
	toast.closable = closable;
	view->toast = toast;

	// Remove any outstanding toast timer we may have.
	toastTimer = -1;

	// Create a timer to remove this toast.
	if (ttl > -1) {
		toastTimer = ttl;
		toastExpiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(ttl);
	}
}

/**
 * Open user-configured export directory with OS default.
 */
void openExportDirectory() {
	const std::string exportDir = view->config.value("exportDirectory", "");
#ifdef _WIN32
	const std::wstring wpath(exportDir.begin(), exportDir.end());
	ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	std::string cmd = "xdg-open \"" + exportDir + "\" &";
	std::system(cmd.c_str());
#endif
}

/**
 * Register a handler for file drops.
 */
void registerDropHandler(DropHandler handler) {
	// Ensure the extensions are all lower-case.
	for (auto& ext : handler.ext)
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	dropHandlers.push_back(std::move(handler));
}

/**
 * Get a drop handler for the given file path.
 */
const DropHandler* getDropHandler(const std::string& file) {
	std::string lowerFile = file;
	std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	for (const auto& handler : dropHandlers) {
		for (const auto& ext : handler.ext) {
			if (lowerFile.size() >= ext.size() &&
				lowerFile.compare(lowerFile.size() - ext.size(), ext.size(), ext) == 0)
				return &handler;
		}
	}

	return nullptr;
}

/**
 * Save scroll position for a listbox with the given key.
 */
void saveScrollPosition(const std::string& key, double scrollRel, int scrollIndex) {
	if (key.empty())
		return;

	const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()
	).count();

	scrollPositions[key] = {
		.scrollRel = scrollRel,
		.scrollIndex = scrollIndex,
		.timestamp = now
	};
}

/**
 * Get saved scroll position for a listbox with the given key.
 */
std::optional<ScrollPosition> getScrollPosition(const std::string& key) {
	if (key.empty())
		return std::nullopt;

	auto it = scrollPositions.find(key);
	if (it == scrollPositions.end())
		return std::nullopt;

	return it->second;
}

} // namespace core