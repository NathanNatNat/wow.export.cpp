/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "screen_settings.h"
#include "../modules.h"
#include "tab_characters.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../constants.h"
#include "../casc/tact-keys.h"
#include "../casc/locale-flags.h"
#include "../components/file-field.h"

#include <cstring>
#include <format>
#include <optional>
#include <regex>

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace screen_settings {

// --- File-local state ---

// JS: let default_config = null;
static std::optional<nlohmann::json> default_config;

// FileField widget states for directory pickers.
static file_field::FileFieldState export_dir_state;
static file_field::FileFieldState char_save_dir_state;

// --- Internal functions ---

// JS: const load_default_config = async () => { ... }
static nlohmann::json load_default_config() {
	if (!default_config.has_value()) {
		auto result = generics::readJSON(constants::CONFIG::DEFAULT_PATH(), true);
		default_config = result.value_or(nlohmann::json::object());
	}

	return default_config.value();
}

// JS: computed: is_edit_export_path_concerning()
static bool is_edit_export_path_concerning() {
	const auto& cfg = core::view->configEdit;
	if (!cfg.contains("exportDirectory"))
		return false;

	const std::string& path = cfg["exportDirectory"].get_ref<const std::string&>();
	return path.find(' ') != std::string::npos ||
		   path.find('\t') != std::string::npos;
}

// JS: computed: default_character_path()
static std::string default_character_path() {
	return tab_characters::get_default_characters_dir();
}

// JS: computed: cache_size_formatted()
static std::string cache_size_formatted() {
	return generics::filesize(static_cast<double>(core::view->cacheSize));
}

// JS: computed: available_locale_keys()
// Returns the locale ID strings from locale_flags::entries.
static std::vector<std::string> available_locale_keys() {
	std::vector<std::string> keys;
	for (const auto& entry : casc::locale_flags::entries)
		keys.emplace_back(entry.id);
	return keys;
}

// JS: computed: selected_locale_key()
static std::string selected_locale_key() {
	uint32_t current_locale = core::view->config.value("cascLocale", static_cast<uint32_t>(casc::locale_flags::enUS));
	for (const auto& entry : casc::locale_flags::entries) {
		if (entry.flag == current_locale)
			return std::string(entry.id);
	}
	return "unUN";
}

// JS: methods.go_home()
static void go_home() {
	// JS: this.$modules.go_to_landing();
	modules::go_to_landing();
}

// --- Public functions ---

// JS: register() { this.registerContextMenuOption('Manage Settings', 'gear.svg'); }
void registerScreen() {
	modules::registerContextMenuOption("settings", "Manage Settings", "gear.svg",
		[]() { modules::set_active("settings"); });
}

// JS: mounted()
void mounted() {
	// JS: this.$core.view.configEdit = Object.assign({}, this.$core.view.config);
	core::view->configEdit = core::view->config;

	// Reset FileField states.
	export_dir_state = file_field::FileFieldState{};
	char_save_dir_state = file_field::FileFieldState{};
}

void render() {
	// --- Template rendering ---
	// JS: <div id="config-wrapper">
	// JS: <div id="config" :class="{ toastgap: $core.view.toast !== null }">
	// TODO(conversion): Full ImGui rendering will be implemented when UI integration is complete.

	// JS: Export Directory
	// <h1>Export Directory</h1>
	// <p>Local directory where files will be exported to.</p>
	// <p v-if="is_edit_export_path_concerning" class="concern">Warning: Using an export path with spaces...</p>
	// <FileField v-model="$core.view.configEdit.exportDirectory" :class="{ concern: is_edit_export_path_concerning }">

	// JS: Character Save Directory
	// <h1>Character Save Directory</h1>
	// <p>Local directory where saved characters are stored. Leave empty to use the default location.</p>
	// <FileField v-model="$core.view.configEdit.characterExportPath" :placeholder="default_character_path">

	// JS: Scroll Speed
	// <h1>Scroll Speed</h1>
	// <input type="number" v-model.number="$core.view.configEdit.scrollSpeed"/>

	// JS: Display File Lists in Numerical Order (FileDataID)
	// <input type="checkbox" v-model="$core.view.configEdit.listfileSortByID"/>

	// JS: Find Unknown Files (Requires Restart)
	// <input type="checkbox" v-model="$core.view.configEdit.enableUnknownFiles"/>

	// JS: Load Model Skins (Requires Restart)
	// <input type="checkbox" v-model="$core.view.configEdit.enableM2Skins"/>

	// JS: Include Bone Prefixes
	// <input type="checkbox" v-model="$core.view.configEdit.modelsExportWithBonePrefix"/>

	// JS: Enable Shared Textures (Recommended)
	// <input type="checkbox" v-model="$core.view.configEdit.enableSharedTextures"/>

	// JS: Enable Shared Children (Recommended)
	// <input type="checkbox" v-model="$core.view.configEdit.enableSharedChildren"/>

	// JS: Strip Whitespace From Export Paths
	// <input type="checkbox" v-model="$core.view.configEdit.removePathSpaces"/>

	// JS: Strip Whitespace From Copied Paths
	// <input type="checkbox" v-model="$core.view.configEdit.removePathSpacesCopy"/>

	// JS: Path Separator Format
	// <li :class="{ selected: configEdit.pathFormat == 'win32' }" @click="configEdit.pathFormat = 'win32'">Windows</li>
	// <li :class="{ selected: configEdit.pathFormat == 'posix' }" @click="configEdit.pathFormat = 'posix'">POSIX</li>

	// JS: Use Absolute MTL Texture Paths
	// <input type="checkbox" v-model="$core.view.configEdit.enableAbsoluteMTLPaths"/>

	// JS: Use Absolute glTF Texture Paths
	// <input type="checkbox" v-model="$core.view.configEdit.enableAbsoluteGLTFPaths"/>

	// JS: Use Absolute Model Placement Paths
	// <input type="checkbox" v-model="$core.view.configEdit.enableAbsoluteCSVPaths"/>

	// JS: CASC Locale
	// <MenuButton :dropdown="true" :options="available_locale_keys" :default="selected_locale_key"
	//   @change="$core.view.configEdit.cascLocale = $core.view.availableLocale.flags[$event]">

	// JS: WebP Quality
	// <input type="number" min="1" max="100" v-model.number="$core.view.configEdit.exportWebPQuality"/>

	// JS: Export Model Collision
	// <input type="checkbox" v-model="$core.view.configEdit.modelsExportCollision"/>

	// JS: Export Additional UV Layers
	// <input type="checkbox" v-model="$core.view.configEdit.modelsExportUV2"/>

	// JS: Export Meta Data
	// Multi-button for M2, WMO, BLP, Foliage meta toggles

	// JS: Export M2 Bone Data
	// <input type="checkbox" v-model="$core.view.configEdit.exportM2Bones"/>

	// JS: Always Overwrite Existing Files (Recommended)
	// <input type="checkbox" v-model="$core.view.configEdit.overwriteFiles"/>

	// JS: Name Exported Files
	// <input type="checkbox" v-model="$core.view.configEdit.exportNamedFiles"/>

	// JS: Prevent 3D Preview Overwrites
	// <input type="checkbox" v-model="$core.view.configEdit.modelsExportPngIncrements"/>

	// JS: Regular Expression Filtering (Advanced)
	// <input type="checkbox" v-model="$core.view.configEdit.regexFilters"/>

	// JS: Copy Mode
	// Multi-button: Full / Directory / FileDataID

	// JS: Paste Selection
	// <input type="checkbox" v-model="$core.view.configEdit.pasteSelection"/>

	// JS: Split Large Terrain Maps (Recommended)
	// <input type="checkbox" v-model="$core.view.configEdit.splitLargeTerrainBakes"/>

	// JS: Split Alpha Maps
	// <input type="checkbox" v-model="$core.view.configEdit.splitAlphaMaps"/>

	// JS: Show unknown items
	// <input type="checkbox" v-model="$core.view.configEdit.itemViewerShowAll"/>

	// JS: Cache Expiry
	// <input type="number" v-model.number="$core.view.configEdit.cacheExpiry"/>

	// JS: CDN Fallback Hosts
	// <input type="text" class="long" v-model.trim="$core.view.configEdit.cdnFallbackHosts"/>

	// JS: Manually Clear Cache (Requires Restart)
	// <input type="button" :value="'Clear Cache (' + cache_size_formatted + ')'" @click="handle_cache_clear">

	// JS: Encryption Keys
	// Primary <input type="text" class="long" v-model.trim="$core.view.configEdit.tactKeysURL"/>
	// Fallback <input type="text" class="long" v-model.trim="$core.view.configEdit.tactKeysFallbackURL"/>

	// JS: Add Encryption Key
	// <input type="text" v-model.trim="$core.view.userInputTactKeyName" maxlength="16"/>
	// <input type="text" v-model.trim="$core.view.userInputTactKey" maxlength="32"/>
	// <input type="button" value="Add" @click="handle_tact_key"/>

	// JS: Realm List Source
	// <input type="text" class="long" v-model.trim="$core.view.configEdit.realmListURL"/>

	// JS: Character Appearance API Endpoint
	// <input type="text" class="long" v-model.trim="$core.view.configEdit.armoryURL"/>

	// JS: Use Binary Listfile Format (Requires Restart)
	// <input type="checkbox" v-model="$core.view.configEdit.enableBinaryListfile"/>

	// JS: Listfile Binary Source
	// <input type="text" class="long" v-model.trim="$core.view.configEdit.listfileBinarySource"/>

	// JS: Listfile Source (Legacy)
	// Primary <input type="text" class="long" v-model.trim="$core.view.configEdit.listfileURL"/>
	// Fallback <input type="text" class="long" v-model.trim="$core.view.configEdit.listfileFallbackURL"/>

	// JS: Listfile Update Frequency
	// <input type="number" v-model.number="$core.view.configEdit.listfileCacheRefresh"/>

	// JS: Data Table Definition Repository
	// Primary <input type="text" class="long" v-model.trim="$core.view.configEdit.dbdURL"/>
	// Fallback <input type="text" class="long" v-model.trim="$core.view.configEdit.dbdFallbackURL"/>

	// JS: DBD Manifest Repository
	// Primary <input type="text" class="long" v-model.trim="$core.view.configEdit.dbdFilenameURL"/>
	// Fallback <input type="text" class="long" v-model.trim="$core.view.configEdit.dbdFilenameFallbackURL"/>

	// JS: Allow Cache Collection
	// <input type="checkbox" v-model="$core.view.configEdit.allowCacheCollection"/>

	// JS: <div id="config-buttons">
	//   <input type="button" value="Discard" @click="handle_discard"/>
	//   <input type="button" value="Apply" @click="handle_apply"/>
	//   <input type="button" id="config-reset" value="Reset to Defaults" @click="handle_reset"/>
}

// JS: methods.handle_cache_clear(event)
static void handle_cache_clear() {
	// JS: if (!event.target.classList.contains('disabled'))
	//     this.$core.events.emit('click-cache-clear');
	if (!core::view->isBusy)
		core::events.emit("click-cache-clear");
}

// JS: methods.handle_tact_key()
static void handle_tact_key() {
	if (casc::tact_keys::addKey(core::view->userInputTactKeyName, core::view->userInputTactKey))
		core::setToast("success", "Successfully added decryption key.");
	else
		core::setToast("error", "Invalid encryption key.", {}, -1);
}

// JS: methods.handle_discard()
void handle_discard() {
	if (core::view->isBusy)
		return;

	go_home();
}

// JS: methods.handle_apply()
void handle_apply() {
	if (core::view->isBusy)
		return;

	auto& cfg = core::view->configEdit;
	nlohmann::json defaults = load_default_config();

	// JS: if (cfg.exportDirectory.length === 0)
	if (!cfg.contains("exportDirectory") || cfg["exportDirectory"].get<std::string>().empty()) {
		core::setToast("error", "A valid export directory must be provided", {}, -1);
		return;
	}

	// JS: if (cfg.realmListURL.length === 0 || !cfg.realmListURL.startsWith('http'))
	{
		std::string val = cfg.value("realmListURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			// JS: { 'Use Default': () => cfg.realmListURL = defaults.realmListURL }
			core::setToast("error", "A valid realm list URL or path is required.",
				{ {"Use Default", [&cfg, defaults]() { cfg["realmListURL"] = defaults["realmListURL"]; }} }, -1);
			return;
		}
	}

	// JS: if (cfg.listfileURL.length === 0)
	{
		std::string val = cfg.value("listfileURL", std::string{});
		if (val.empty()) {
			// JS: { 'Use Default': () => cfg.listfileURL = defaults.listfileURL }
			core::setToast("error", "A valid listfile URL or path is required.",
				{ {"Use Default", [&cfg, defaults]() { cfg["listfileURL"] = defaults["listfileURL"]; }} }, -1);
			return;
		}
	}

	// JS: if (cfg.armoryURL.length === 0 || !cfg.armoryURL.startsWith('http'))
	{
		std::string val = cfg.value("armoryURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			// JS: { 'Use Default': () => cfg.armoryURL = defaults.armoryURL }
			core::setToast("error", "A valid URL is required for the Character Appearance API.",
				{ {"Use Default", [&cfg, defaults]() { cfg["armoryURL"] = defaults["armoryURL"]; }} }, -1);
			return;
		}
	}

	// JS: if (cfg.tactKeysURL.length === 0 || !cfg.tactKeysURL.startsWith('http'))
	{
		std::string val = cfg.value("tactKeysURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			// JS: { 'Use Default': () => cfg.tactKeysURL = defaults.tactKeysURL }
			core::setToast("error", "A valid URL is required for encryption key updates.",
				{ {"Use Default", [&cfg, defaults]() { cfg["tactKeysURL"] = defaults["tactKeysURL"]; }} }, -1);
			return;
		}
	}

	// JS: if (cfg.dbdURL.length === 0 || !cfg.dbdURL.startsWith('http'))
	{
		std::string val = cfg.value("dbdURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			// JS: { 'Use Default': () => cfg.dbdURL = defaults.dbdURL }
			core::setToast("error", "A valid URL is required for DBD updates.",
				{ {"Use Default", [&cfg, defaults]() { cfg["dbdURL"] = defaults["dbdURL"]; }} }, -1);
			return;
		}
	}

	// JS: if (cfg.dbdFilenameURL.length === 0 || !cfg.dbdFilenameURL.startsWith('http'))
	{
		std::string val = cfg.value("dbdFilenameURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			// JS: { 'Use Default': () => cfg.dbdFilenameURL = defaults.dbdFilenameURL }
			core::setToast("error", "A valid URL is required for DBD manifest.",
				{ {"Use Default", [&cfg, defaults]() { cfg["dbdFilenameURL"] = defaults["dbdFilenameURL"]; }} }, -1);
			return;
		}
	}

	// JS: this.$core.view.config = cfg;
	core::view->config = cfg;

	go_home();

	// JS: this.$core.setToast('success', 'Changes to your configuration have been saved!');
	core::setToast("success", "Changes to your configuration have been saved!");
}

// JS: methods.handle_reset()
void handle_reset() {
	if (core::view->isBusy)
		return;

	// JS: const defaults = await load_default_config();
	// JS: this.$core.view.configEdit = JSON.parse(JSON.stringify(defaults));
	nlohmann::json defaults = load_default_config();
	core::view->configEdit = nlohmann::json::parse(defaults.dump());
}

} // namespace screen_settings
