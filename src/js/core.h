/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <any>
#include <variant>
#include <memory>
#include <mutex>
#include <optional>
#include <chrono>

#include <nlohmann/json.hpp>

#include "file-writer.h"

namespace casc {
	class CASC;
	class BLPImage;
	namespace locale_flags {
		struct LocaleEntry;
	}
}
namespace mpq {
	class MPQInstall;
}

/**
 * EventEmitter — C++ equivalent of Node.js EventEmitter.
 * Provides on(event, callback), emit(event, args...), off(event, callback) semantics.
 */
class EventEmitter {
public:
	using Callback = std::function<void()>;
	using ArgCallback = std::function<void(const std::any&)>;

	/**
	 * Set the maximum number of listeners per event.
	 */
	void setMaxListeners(int max);

	/**
	 * Register an event listener (no arguments).
	 * @param event Event name.
	 * @param callback Callback to invoke.
	 * @returns Listener ID for removal.
	 */
	size_t on(const std::string& event, Callback callback);

	/**
	 * Register an event listener that receives a typed argument.
	 * @param event Event name.
	 * @param callback Callback to invoke with the emitted argument.
	 * @returns Listener ID for removal.
	 */
	size_t on(const std::string& event, ArgCallback callback);

	/**
	 * Register an event listener that fires only once (no arguments).
	 * The listener is automatically removed after the first invocation.
	 * @param event Event name.
	 * @param callback Callback to invoke.
	 * @returns Listener ID for removal.
	 */
	size_t once(const std::string& event, Callback callback);

	/**
	 * Register an event listener that fires only once and receives a typed argument.
	 * The listener is automatically removed after the first invocation.
	 * @param event Event name.
	 * @param callback Callback to invoke with the emitted argument.
	 * @returns Listener ID for removal.
	 */
	size_t once(const std::string& event, ArgCallback callback);

	/**
	 * Remove an event listener by ID.
	 * @param event Event name.
	 * @param id Listener ID returned by on().
	 */
	void off(const std::string& event, size_t id);

	/**
	 * Emit an event, invoking all registered listeners (no arguments).
	 * @param event Event name.
	 */
	void emit(const std::string& event);

	/**
	 * Emit an event with a typed argument, invoking all registered listeners.
	 * ArgCallback listeners receive the argument; Callback listeners ignore it.
	 * @param event Event name.
	 * @param arg Argument to pass to ArgCallback listeners.
	 */
	void emit(const std::string& event, const std::any& arg);

	/**
	 * Remove all listeners for an event.
	 * @param event Event name.
	 */
	void removeAllListeners(const std::string& event);

private:
	struct Listener {
		size_t id;
		std::variant<Callback, ArgCallback> callback;
	};

	size_t addListener(const std::string& event, std::variant<Callback, ArgCallback> cb);
	void emitImpl(const std::string& event, const std::any* arg);

	int maxListeners = 666;
	size_t nextId = 0;
	std::unordered_map<std::string, std::vector<Listener>> listeners;
};

/**
 * Toast notification data.
 */
/**
 * A single toast action button (label + callback).
 */
struct ToastAction {
	std::string label;
	std::function<void()> callback;
};

struct Toast {
	std::string type;      // 'error', 'info', 'success', 'progress'
	std::string message;
	std::vector<ToastAction> actions; // Action buttons with callbacks.
	bool closable = true;
};

/**
 * Menu button option (label + value pair).
 */
struct MenuOption {
	std::string label;
	std::string value;
};

/**
 * Menu button option with integer value.
 */
struct MenuOptionInt {
	std::string label;
	int value;
};

/**
 * Context menu state nodes.
 */
struct ContextMenus {
	nlohmann::json nodeTextureRibbon;  // Context menu node for the texture ribbon.
	nlohmann::json nodeItem;           // Context menu node for the items listfile.
	nlohmann::json nodeDataTable;      // Context menu node for the data table.
	nlohmann::json nodeListbox;        // Context menu node for generic listbox.
	nlohmann::json nodeMap;            // Context menu node for maps listbox.
	nlohmann::json nodeZone;           // Context menu node for zones listbox.
	bool stateNavExtra = false;        // State controller for the extra nav menu.
	bool stateModelExport = false;     // State controller for the model export menu.
	bool stateCDNRegion = false;       // State controller for the CDN region selection menu.

	// JS: app.js lines 556-563 — bool fields → false, json fields → null.
	void resetAll() {
		stateNavExtra = false;
		stateModelExport = false;
		stateCDNRegion = false;
		nodeTextureRibbon = nullptr;
		nodeItem = nullptr;
		nodeDataTable = nullptr;
		nodeListbox = nullptr;
		nodeMap = nullptr;
		nodeZone = nullptr;
	}
};

/**
 * Guild tabard configuration.
 */
struct GuildTabardConfig {
	int background = 0;
	int border_style = 0;
	int border_color = 0;
	int emblem_design = 0;
	int emblem_color = 0;
};

/**
 * Color picker position.
 */
struct ColorPickerPosition {
	int x = 0;
	int y = 0;
};

/**
 * Scroll position state for listbox components.
 */
struct ScrollPosition {
	double scrollRel = 0;
	int scrollIndex = 0;
	int64_t timestamp = 0;
};

/**
 * Drop handler for drag/drop file support.
 */
struct DropHandler {
	std::vector<std::string> ext;
	std::function<std::string(int)> prompt;
	std::function<void(const std::vector<std::string>&)> process;
};

/**
 * AppState — Central application state struct.
 * Since ImGui is immediate-mode, state changes are reflected
 * automatically on the next frame — no "reactivity" system needed.
 *
 * JS equivalent: makeNewView() in core.js
 *
 * Deviations from JS makeNewView():
 * - `constants` field omitted: JS stores `constants: constants` on the view
 *   for Vue template access. In C++, constants are accessed via the
 *   `constants::` namespace directly — no need for a runtime reference.
 * - `availableLocale` field omitted: JS stores `availableLocale: Locale`.
 *   In C++, this is a compile-time constant accessed via
 *   `casc::locale_flags::entries`.
 * - Additional C++/OpenGL-specific fields not in JS:
 *   `mpq` (MPQ install instance, JS uses different install model),
 *   `chrCustRacesPlayable`, `chrCustRacesNPC` (split from chrCustRaces),
 *   `pendingItemSlotFilter` (ImGui UI plumbing),
 *   `zoneMapTexID/Width/Height/Pixels` (OpenGL texture resources),
 *   `*TexID` fields (OpenGL texture handles for preview images).
 *   These are necessary platform adaptations for C++/OpenGL/ImGui.
 */
struct AppState {
	~AppState();                                 // Defined in core.cpp (needs complete mpq::MPQInstall).
	AppState() = default;
	AppState(AppState&&) = default;
	AppState& operator=(AppState&&) = default;
	int installType = 0;                         // Active install type (MPQ or CASC).
	int isBusy = 0;                              // To prevent race-conditions with multiple tasks.
#ifdef NDEBUG
	bool isDev = false;                          // True if in development environment.
#else
	bool isDev = true;
#endif
	bool isLoading = false;                      // Controls whether the loading overlay is visible.
	std::string loadingProgress;                 // Sets the progress text for the loading screen.
	std::string loadingTitle;                    // Sets the title text for the loading screen.
	double loadPct = -1;                         // Controls active loading bar percentage.
	std::optional<Toast> toast;                  // Controls the currently active toast bar.
	nlohmann::json cdnRegions = nlohmann::json::array(); // CDN region data.
	nlohmann::json selectedCDNRegion;            // Active CDN region.
	bool lockCDNRegion = false;                  // If true, do not programmatically alter the selected CDN region.
	nlohmann::json config = nlohmann::json::object();  // Default/user-set configuration.
	nlohmann::json configEdit = nlohmann::json::object();
	nlohmann::json availableLocalBuilds;         // Local builds to display during source select.
	nlohmann::json availableRemoteBuilds;        // Remote builds to display during source select.
	bool sourceSelectShowBuildSelect = false;    // Controls whether build select is shown.
	casc::CASC* casc = nullptr;                  // Active CASC instance.
	std::unique_ptr<mpq::MPQInstall> mpq;        // Active MPQ install instance.
	int64_t cacheSize = 0;                       // Active size of the user cache.
	std::string userInputTactKey;                // Value of manual tact key field.
	std::string userInputTactKeyName;            // Value of manual tact key name field.
	std::string userInputFilterTextures;
	std::string userInputFilterSounds;
	std::string userInputFilterVideos;
	std::string userInputFilterText;
	std::string userInputFilterFonts;
	std::string userInputFilterModels;
	std::string userInputFilterMaps;
	std::string userInputFilterZones;
	std::string userInputFilterItems;
	std::string userInputFilterItemSets;
	std::string userInputFilterDB2s;
	std::string userInputFilterDataTable;
	std::string userInputFilterRaw;
	std::string userInputFilterLegacyModels;
	std::string userInputFilterDecor;
	std::string userInputFilterCreatures;
	nlohmann::json activeModule;                 // Active module component instance.
	std::vector<nlohmann::json> modNavButtons;   // Module-registered navigation buttons.
	std::vector<nlohmann::json> modContextMenuOptions; // Module-registered context menu options.
	std::string userInputFilterInstall;
	std::vector<std::string> modelQuickFilters = {"m2", "m3", "wmo"};
	std::vector<std::string> legacyModelQuickFilters = {"m2", "mdx", "wmo"};
	std::vector<std::string> audioQuickFilters = {"ogg", "mp3", "unk"};
	std::vector<std::string> textQuickFilters = {"lua", "xml", "txt", "sbt", "wtf", "htm", "toc", "xsd", "srt"};
	std::vector<nlohmann::json> selectionTextures;
	std::vector<nlohmann::json> selectionModels;
	std::vector<nlohmann::json> selectionSounds;
	std::vector<nlohmann::json> selectionVideos;
	std::vector<nlohmann::json> selectionText;
	std::vector<nlohmann::json> selectionFonts;
	std::vector<nlohmann::json> selectionMaps;
	std::vector<nlohmann::json> selectionZones;
	std::vector<nlohmann::json> selectionItems;
	std::vector<nlohmann::json> selectionItemSets;
	std::vector<nlohmann::json> selectionDB2s;
	std::vector<nlohmann::json> selectionDataTable;
	std::vector<nlohmann::json> selectionRaw;
	std::vector<nlohmann::json> selectionInstall;
	std::vector<nlohmann::json> selectionLegacyModels;
	std::vector<nlohmann::json> selectionDecor;
	std::vector<nlohmann::json> selectionCreatures;
	bool installStringsView = false;
	std::vector<std::string> installStrings;
	std::string installStringsFileName;
	std::vector<nlohmann::json> selectionInstallStrings;
	std::string userInputFilterInstallStrings;
	std::vector<nlohmann::json> listfileTextures;
	std::vector<nlohmann::json> listfileSounds;
	std::vector<nlohmann::json> listfileVideos;
	std::vector<nlohmann::json> listfileText;
	std::vector<nlohmann::json> listfileFonts;
	std::vector<nlohmann::json> listfileModels;
	std::vector<nlohmann::json> listfileItems;
	std::vector<nlohmann::json> listfileItemSets;
	std::vector<int> itemViewerTypeMask;
	std::vector<int> itemViewerQualityMask;
	std::vector<nlohmann::json> listfileRaw;
	std::vector<nlohmann::json> listfileInstall;
	std::vector<nlohmann::json> listfileLegacyModels;
	std::vector<nlohmann::json> listfileDecor;
	std::vector<nlohmann::json> listfileCreatures;
	std::vector<nlohmann::json> decorCategoryMask;
	std::vector<nlohmann::json> decorCategoryGroups;
	std::vector<nlohmann::json> dbdManifest;
	std::vector<nlohmann::json> installTags;
	std::vector<nlohmann::json> tableBrowserHeaders;
	std::vector<nlohmann::json> tableBrowserRows;
	// availableLocale is the casc::locale_flags::entries array (compile-time constant).
	// JS equivalent: `availableLocale: Locale` on makeNewView().
	// In C++ this is accessed directly via casc::locale_flags::entries instead of
	// being stored as a field. Any JS code using `view.availableLocale` maps to
	// casc::locale_flags::entries in C++.
	nlohmann::json fileDropPrompt;
	std::string whatsNewHTML;
	std::string textViewerSelectedText;
	std::string fontPreviewPlaceholder;
	std::string fontPreviewText;
	std::string fontPreviewFontFamily;
	double soundPlayerSeek = 0;
	bool soundPlayerState = false;
	std::string soundPlayerTitle = "No File Selected";
	double soundPlayerDuration = 0;
	bool videoPlayerState = false;
	nlohmann::json modelViewerContext;
	std::string modelViewerActiveType = "none";
	std::vector<nlohmann::json> modelViewerGeosets;
	std::vector<nlohmann::json> modelViewerSkins;
	std::vector<nlohmann::json> modelViewerSkinsSelection;
	std::vector<nlohmann::json> modelViewerAnims;
	nlohmann::json modelViewerAnimSelection;
	bool modelViewerAnimPaused = false;
	int modelViewerAnimFrame = 0;
	int modelViewerAnimFrameCount = 0;
	std::vector<nlohmann::json> modelViewerWMOGroups;
	std::vector<nlohmann::json> modelViewerWMOSets;
	bool modelViewerAutoAdjust = true;
	nlohmann::json legacyModelViewerContext;
	std::string legacyModelViewerActiveType = "none";
	std::vector<nlohmann::json> legacyModelViewerAnims;
	nlohmann::json legacyModelViewerAnimSelection;
	bool legacyModelViewerAnimPaused = false;
	int legacyModelViewerAnimFrame = 0;
	int legacyModelViewerAnimFrameCount = 0;
	bool legacyModelViewerAutoAdjust = true;
	nlohmann::json creatureViewerContext;
	std::string creatureViewerActiveType = "none";
	std::vector<nlohmann::json> creatureViewerGeosets;
	std::vector<nlohmann::json> creatureViewerSkins;
	std::vector<nlohmann::json> creatureViewerSkinsSelection;
	std::vector<nlohmann::json> creatureViewerWMOGroups;
	std::vector<nlohmann::json> creatureViewerWMOSets;
	bool creatureViewerAutoAdjust = true;
	std::vector<nlohmann::json> creatureViewerAnims;
	nlohmann::json creatureViewerAnimSelection;
	bool creatureViewerAnimPaused = false;
	int creatureViewerAnimFrame = 0;
	int creatureViewerAnimFrameCount = 0;
	std::vector<nlohmann::json> creatureViewerEquipment;
	std::vector<nlohmann::json> creatureViewerUVLayers;
	std::string creatureTexturePreviewURL;
	std::string creatureTexturePreviewUVOverlay;
	int creatureTexturePreviewWidth = 256;
	int creatureTexturePreviewHeight = 256;
	std::string creatureTexturePreviewName;
	uint32_t creatureTexturePreviewTexID = 0;
	uint32_t creatureTexturePreviewUVTexID = 0;
	nlohmann::json decorViewerContext;
	std::string decorViewerActiveType = "none";
	std::vector<nlohmann::json> decorViewerGeosets;
	std::vector<nlohmann::json> decorViewerWMOGroups;
	std::vector<nlohmann::json> decorViewerWMOSets;
	bool decorViewerAutoAdjust = true;
	std::vector<nlohmann::json> decorViewerAnims;
	nlohmann::json decorViewerAnimSelection;
	bool decorViewerAnimPaused = false;
	int decorViewerAnimFrame = 0;
	int decorViewerAnimFrameCount = 0;
	std::vector<nlohmann::json> decorViewerUVLayers;
	std::string decorTexturePreviewURL;
	std::string decorTexturePreviewUVOverlay;
	int decorTexturePreviewWidth = 256;
	int decorTexturePreviewHeight = 256;
	std::string decorTexturePreviewName;
	uint32_t decorTexturePreviewTexID = 0;
	uint32_t decorTexturePreviewUVTexID = 0;
	std::vector<nlohmann::json> legacyModelViewerSkins;
	std::vector<nlohmann::json> legacyModelViewerSkinsSelection;
	std::string legacyModelTexturePreviewURL;
	double modelViewerRotationSpeed = 0;
	std::vector<nlohmann::json> textureRibbonStack;
	int textureRibbonSlotCount = 0;
	int textureRibbonPage = 0;
	std::vector<nlohmann::json> textureAtlasOverlayRegions;
	int textureAtlasOverlayWidth = 0;
	int textureAtlasOverlayHeight = 0;
	// itemViewerTypeMask is above with the other masks
	int modelTexturePreviewWidth = 256;
	int modelTexturePreviewHeight = 256;
	std::string modelTexturePreviewURL;
	std::string modelTexturePreviewName;
	std::string modelTexturePreviewUVOverlay;
	std::vector<nlohmann::json> modelViewerUVLayers;
	uint32_t modelTexturePreviewTexID = 0;
	uint32_t modelTexturePreviewUVTexID = 0;
	int texturePreviewWidth = 256;
	int texturePreviewHeight = 256;
	std::string texturePreviewURL;
	std::string texturePreviewInfo;
	uint32_t texturePreviewTexID = 0;
	std::vector<nlohmann::json> overrideModelList;
	std::string overrideModelName;
	std::vector<nlohmann::json> overrideTextureList;
	std::string overrideTextureName;
	std::string pendingItemSlotFilter;             // Pending item slot filter from character tab.
	int chrPendingEquipSlot = 0;                   // Pending equip slot ID set by character tab navigation (0 = none). JS: chrPendingEquipSlot
	std::vector<nlohmann::json> mapViewerMaps;
	std::vector<nlohmann::json> zoneViewerZones;
	std::vector<nlohmann::json> zonePhases;
	nlohmann::json zonePhaseSelection;
	int selectedZoneExpansionFilter = -1;
	uint32_t zoneMapTexID = 0;
	int zoneMapWidth = 0;
	int zoneMapHeight = 0;
	std::vector<uint8_t> zoneMapPixels;
	bool mapViewerHasWorldModel = false;
	bool mapViewerIsWMOMinimap = false;
	nlohmann::json mapViewerTileLoader;
	nlohmann::json mapViewerSelectedMap;
	nlohmann::json mapViewerSelectedDir;
	nlohmann::json mapViewerChunkMask;
	nlohmann::json mapViewerGridSize;
	std::vector<nlohmann::json> mapViewerSelection;
	int selectedExpansionFilter = -1;
	nlohmann::json chrModelViewerContext;
	std::vector<nlohmann::json> chrModelViewerAnims;
	nlohmann::json chrModelViewerAnimSelection;
	bool chrModelViewerAnimPaused = false;
	int chrModelViewerAnimFrame = 0;
	int chrModelViewerAnimFrameCount = 0;
	std::vector<nlohmann::json> chrCustRaces;
	std::vector<nlohmann::json> chrCustRacesPlayable;
	std::vector<nlohmann::json> chrCustRacesNPC;
	std::vector<nlohmann::json> chrCustRaceSelection;
	std::vector<nlohmann::json> chrCustModels;
	std::vector<nlohmann::json> chrCustModelSelection;
	std::vector<nlohmann::json> chrCustOptions;
	std::vector<nlohmann::json> chrCustOptionSelection;
	std::vector<nlohmann::json> chrCustChoices;
	std::vector<nlohmann::json> chrCustChoiceSelection;
	std::vector<nlohmann::json> chrCustActiveChoices;
	std::vector<nlohmann::json> chrCustGeosets;
	std::string chrCustTab = "models";
	std::string chrCustRightTab = "geosets";
	bool chrModelLoading = false;
	bool chrShowGeosetControl = false;
	std::string chrExportMenu = "export";
	nlohmann::json colorPickerOpenFor;
	ColorPickerPosition colorPickerPosition;
	std::string chrImportChrName;
	std::vector<nlohmann::json> chrImportRegions;
	std::string chrImportSelectedRegion;
	std::vector<nlohmann::json> chrImportRealms;
	nlohmann::json chrImportSelectedRealm;
	bool chrImportLoadVisage = false;
	bool chrImportClassicRealms = false;
	int chrImportChrModelID = 0;
	int chrImportTargetModelID = 0;
	std::vector<nlohmann::json> chrImportChoices;
	std::string chrImportWowheadURL;
	std::string characterImportMode = "none";
	nlohmann::json chrEquippedItems = nlohmann::json::object();
	nlohmann::json chrEquippedItemSkins = nlohmann::json::object();
	GuildTabardConfig chrGuildTabardConfig;
	nlohmann::json chrEquipmentSlotContext;
	std::optional<int> chrItemPickerSlot;
	std::optional<std::string> chrItemPickerFilter;
	bool chrSavedCharactersScreen = false;
	std::vector<nlohmann::json> chrSavedCharacters;
	bool chrSaveCharacterPrompt = false;
	std::string chrSaveCharacterName;
	nlohmann::json chrPendingThumbnail;
	nlohmann::json realmList = nlohmann::json::object();
	bool exportCancelled = false;
	bool isXmas = false; // Set at runtime based on current month.
	std::shared_ptr<casc::BLPImage> chrCustBakedNPCTexture;
	std::string regexTooltip =
		".* - Matches anything\n"
		"(a|b) - Matches either a or b.\n"
		"[a-f] - Matches characters between a-f.\n"
		"[^a-d] - Matches characters that are not between a-d.\n"
		"\\s - Matches whitespace characters.\n"
		"\\d - Matches any digit.\n"
		"a? - Matches zero or one of a.\n"
		"a* - Matches zero or more of a.\n"
		"a+ - Matches one or more of a.\n"
		"a{3} - Matches exactly 3 of a.";
	ContextMenus contextMenus;
	std::vector<MenuOption> menuButtonTextures;
	std::vector<MenuOption> menuButtonMapExport;
	std::vector<MenuOptionInt> menuButtonTextureQuality;
	std::vector<MenuOptionInt> menuButtonHeightmapResolution;
	std::vector<MenuOptionInt> menuButtonHeightmapBitDepth;
	std::vector<MenuOption> menuButtonModels;
	std::vector<MenuOption> menuButtonLegacyModels;
	std::vector<MenuOption> menuButtonDecor;
	std::vector<MenuOption> menuButtonCreatures;
	std::vector<MenuOption> menuButtonCharacterExport;
	std::vector<MenuOption> menuButtonVideos;
	std::vector<MenuOption> menuButtonData;
	std::vector<nlohmann::json> helpArticles;
	std::vector<nlohmann::json> helpFilteredArticles;
	nlohmann::json helpSelectedArticle;
	std::string helpSearchQuery;
};

/**
 * BusyLock — RAII busy lock that increments isBusy on creation
 * and decrements on destruction. Use as a local variable.
 *
 * JS equivalent: create_busy_lock() with Symbol.dispose
 */
class BusyLock {
public:
	explicit BusyLock(AppState& state);
	~BusyLock();

	BusyLock(const BusyLock&) = delete;
	BusyLock& operator=(const BusyLock&) = delete;
	BusyLock(BusyLock&& other) noexcept;
	BusyLock& operator=(BusyLock&& other) noexcept;

private:
	AppState* state;
};

/**
 * Core module — global event dispatcher and application state management.
 *
 * JS equivalent: module.exports = { events, view, makeNewView, create_busy_lock,
 *     showLoadingScreen, progressLoadingScreen, hideLoadingScreen,
 *     setToast, hideToast, openExportDirectory, registerDropHandler,
 *     getDropHandler, openLastExportStream, saveScrollPosition, getScrollPosition }
 */
namespace core {

/// Global event emitter.
extern EventEmitter events;

/// Reference to the main application state (initially nullptr, set after init).
extern AppState* view;

/**
 * Create a new AppState with default values.
 * @returns A fresh AppState instance.
 */
AppState makeNewView();

/**
 * Creates an RAII busy lock that increments isBusy on creation and
 * decrements on destruction.
 * @returns BusyLock that decrements on scope exit.
 */
BusyLock create_busy_lock();

/**
 * Show loading screen with specified number of progress steps.
 * @param segments Number of progress steps.
 * @param title    Loading screen title.
 */
void showLoadingScreen(int segments = 1, const std::string& title = "Loading, please wait...");

/**
 * Advance loading screen progress by one step.
 * @param text Progress text to display.
 */
void progressLoadingScreen(const std::string& text = "");

/**
 * Hide loading screen.
 */
void hideLoadingScreen();

/**
 * Display a toast message.
 * @param toastType 'error', 'info', 'success', 'progress'
 * @param message   Toast message text.
 * @param actions   Optional action buttons (label + callback pairs).
 * @param ttl       Time in milliseconds before removing the toast (-1 = persistent).
 * @param closable  If true, toast can manually be closed.
 */
void setToast(const std::string& toastType, const std::string& message,
              const std::vector<ToastAction>& actions = {}, int ttl = 10000, bool closable = true);

/**
 * Hide the currently active toast prompt.
 * @param userCancel True if the user manually dismissed the toast.
 */
void hideToast(bool userCancel = false);

/**
 * Open user-configured export directory with OS default.
 */
void openExportDirectory();

/**
 * Open a file or directory with the OS default application/explorer.
 * @param path Path to open.
 */
void openInExplorer(const std::string& path);

/**
 * Register a handler for file drops.
 * @param handler Drop handler to register.
 */
void registerDropHandler(DropHandler handler);

/**
 * Get a drop handler for the given file path.
 * @param file File path to match against registered handlers.
 * @returns Pointer to matching handler, or nullptr if none found.
 */
const DropHandler* getDropHandler(const std::string& file);

/**
 * Open a stream to the last export file.
 * @returns FileWriter instance.
 */
FileWriter openLastExportStream();

/**
 * Save scroll position for a listbox with the given key.
 * @param key         Unique identifier for the listbox.
 * @param scrollRel   Relative scroll position (0-1).
 * @param scrollIndex Current scroll index.
 */
void saveScrollPosition(const std::string& key, double scrollRel, int scrollIndex);

/**
 * Get saved scroll position for a listbox with the given key.
 * @param key Unique identifier for the listbox.
 * @returns Saved scroll state, or std::nullopt if not found.
 */
std::optional<ScrollPosition> getScrollPosition(const std::string& key);

/**
 * Post a task to be executed on the main thread.
 * Thread-safe: may be called from any thread.
 * Tasks are drained once per frame by the main loop via drainMainThreadQueue().
 */
void postToMainThread(std::function<void()> task);

/**
 * Drain and execute all tasks posted via postToMainThread().
 * Must be called from the main thread (once per frame in the main loop).
 */
void drainMainThreadQueue();

/**
 * Return a cached std::vector<std::string> built from a JSON vector.
 * Only rebuilds when the source vector's size changes (O(1) check on hot path).
 * Use a per-caller pair of (cache, cached_size) as persistent state.
 *
 * @param arr         Source vector of nlohmann::json string elements.
 * @param cache       Persistent cache vector (modified in-place when stale).
 * @param cached_size Persistent size of last build (initialize to ~size_t(0)).
 * @returns           Const reference to the (possibly freshly rebuilt) cache.
 */
inline const std::vector<std::string>& cached_json_strings(
	const std::vector<nlohmann::json>& arr,
	std::vector<std::string>& cache,
	size_t& cached_size) {
	if (arr.size() != cached_size) {
		cached_size = arr.size();
		cache.clear();
		cache.reserve(cached_size);
		for (const auto& item : arr)
			cache.push_back(item.get<std::string>());
	}
	return cache;
}

} // namespace core
