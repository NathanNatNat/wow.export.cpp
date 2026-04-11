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
#include "../components/menu-button.h"

#include <algorithm>
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
static menu_button::MenuButtonState locale_menu_state;

// --- Forward declarations ---
static void handle_cache_clear();
static void handle_tact_key();
void handle_discard();
void handle_apply();
void handle_reset();

/**
 * Helper for config-bound text inputs.
 * Uses a static buffer + prev-value tracker to avoid overwriting user input each frame.
 * Only syncs the buffer from config when the config value changes externally.
 */
struct ConfigTextState {
	char buf[1024] = {};
	std::string prev;
	bool initialized = false;
};

static bool configTextInput(const char* id, ConfigTextState& state, nlohmann::json& cfg, const char* key) {
	std::string current = cfg.value(key, std::string(""));
	if (!state.initialized || current != state.prev) {
		std::strncpy(state.buf, current.c_str(), sizeof(state.buf) - 1);
		state.buf[sizeof(state.buf) - 1] = '\0';
		state.prev = current;
		state.initialized = true;
	}
	if (ImGui::InputText(id, state.buf, sizeof(state.buf))) {
		cfg[key] = std::string(state.buf);
		state.prev = std::string(state.buf);
		return true;
	}
	return false;
}

// Config text states for each text input in settings.
static ConfigTextState cdn_fallback_state;
static ConfigTextState tact_url_state;
static ConfigTextState tact_fallback_url_state;
static ConfigTextState realm_url_state;
static ConfigTextState armory_url_state;
static ConfigTextState listfile_bin_state;
static ConfigTextState listfile_url_state;
static ConfigTextState listfile_fb_url_state;
static ConfigTextState dbd_url_state;
static ConfigTextState dbd_fb_url_state;
static ConfigTextState dbdf_url_state;
static ConfigTextState dbdf_fb_url_state;

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
	auto& view = *core::view;
	auto& cfg = view.configEdit;

	// --- Template rendering ---
	// JS: <div id="config-wrapper">
	// JS: <div id="config" :class="{ toastgap: $core.view.toast !== null }">

	// CSS: .tab#tab-settings .settings-content { max-width: 800px; margin: 0 auto; }
	const float availW = ImGui::GetContentRegionAvail().x;
	const float maxW = 800.0f;
	const float contentW = std::min(availW, maxW);
	const float indent = (availW - contentW) * 0.5f;
	if (indent > 0.0f)
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
	ImGui::BeginChild("settings-content", ImVec2(contentW, 0));

	ImGui::BeginChild("##config-scroll", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2));

	// JS: Export Directory
	ImGui::SeparatorText("Export Directory");
	ImGui::TextWrapped("Local directory where files will be exported to.");
	if (is_edit_export_path_concerning())
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: Using an export path with spaces may cause issues in some applications.");
	{
		std::string export_dir = cfg.value("exportDirectory", std::string(""));
		file_field::render("##ExportDir", export_dir, "", export_dir_state,
			[&](const std::string& new_path) { cfg["exportDirectory"] = new_path; });
	}

	// JS: Character Save Directory
	ImGui::SeparatorText("Character Save Directory");
	ImGui::TextWrapped("Local directory where saved characters are stored. Leave empty to use the default location.");
	{
		std::string char_dir = cfg.value("characterExportPath", std::string(""));
		file_field::render("##CharSaveDir", char_dir, default_character_path().c_str(), char_save_dir_state,
			[&](const std::string& new_path) { cfg["characterExportPath"] = new_path; });
	}

	// JS: Scroll Speed
	ImGui::SeparatorText("Scroll Speed");
	{
		int scroll_speed = cfg.value("scrollSpeed", 2);
		if (ImGui::InputInt("##ScrollSpeed", &scroll_speed))
			cfg["scrollSpeed"] = scroll_speed;
	}

	// JS: Display File Lists in Numerical Order (FileDataID)
	{
		bool val = cfg.value("listfileSortByID", false);
		if (ImGui::Checkbox("Display File Lists in Numerical Order (FileDataID)", &val))
			cfg["listfileSortByID"] = val;
	}

	// JS: Find Unknown Files (Requires Restart)
	{
		bool val = cfg.value("enableUnknownFiles", false);
		if (ImGui::Checkbox("Find Unknown Files (Requires Restart)", &val))
			cfg["enableUnknownFiles"] = val;
	}

	// JS: Load Model Skins (Requires Restart)
	{
		bool val = cfg.value("enableM2Skins", true);
		if (ImGui::Checkbox("Load Model Skins (Requires Restart)", &val))
			cfg["enableM2Skins"] = val;
	}

	// JS: Include Bone Prefixes
	{
		bool val = cfg.value("modelsExportWithBonePrefix", false);
		if (ImGui::Checkbox("Include Bone Prefixes", &val))
			cfg["modelsExportWithBonePrefix"] = val;
	}

	// JS: Enable Shared Textures (Recommended)
	{
		bool val = cfg.value("enableSharedTextures", true);
		if (ImGui::Checkbox("Enable Shared Textures (Recommended)", &val))
			cfg["enableSharedTextures"] = val;
	}

	// JS: Enable Shared Children (Recommended)
	{
		bool val = cfg.value("enableSharedChildren", true);
		if (ImGui::Checkbox("Enable Shared Children (Recommended)", &val))
			cfg["enableSharedChildren"] = val;
	}

	// JS: Strip Whitespace From Export Paths
	{
		bool val = cfg.value("removePathSpaces", false);
		if (ImGui::Checkbox("Strip Whitespace From Export Paths", &val))
			cfg["removePathSpaces"] = val;
	}

	// JS: Strip Whitespace From Copied Paths
	{
		bool val = cfg.value("removePathSpacesCopy", false);
		if (ImGui::Checkbox("Strip Whitespace From Copied Paths", &val))
			cfg["removePathSpacesCopy"] = val;
	}

	// JS: Path Separator Format
	ImGui::SeparatorText("Path Separator Format");
	{
		std::string path_fmt = cfg.value("pathFormat", std::string("win32"));
		bool is_win = (path_fmt == "win32");
		bool is_posix = (path_fmt == "posix");
		if (ImGui::RadioButton("Windows", is_win))
			cfg["pathFormat"] = "win32";
		ImGui::SameLine();
		if (ImGui::RadioButton("POSIX", is_posix))
			cfg["pathFormat"] = "posix";
	}

	// JS: Use Absolute MTL Texture Paths
	{
		bool val = cfg.value("enableAbsoluteMTLPaths", false);
		if (ImGui::Checkbox("Use Absolute MTL Texture Paths", &val))
			cfg["enableAbsoluteMTLPaths"] = val;
	}

	// JS: Use Absolute glTF Texture Paths
	{
		bool val = cfg.value("enableAbsoluteGLTFPaths", false);
		if (ImGui::Checkbox("Use Absolute glTF Texture Paths", &val))
			cfg["enableAbsoluteGLTFPaths"] = val;
	}

	// JS: Use Absolute Model Placement Paths
	{
		bool val = cfg.value("enableAbsoluteCSVPaths", false);
		if (ImGui::Checkbox("Use Absolute Model Placement Paths", &val))
			cfg["enableAbsoluteCSVPaths"] = val;
	}

	// JS: CASC Locale
	ImGui::SeparatorText("CASC Locale");
	{
		auto keys = available_locale_keys();
		std::string current_key = selected_locale_key();

		std::vector<menu_button::MenuOption> locale_options;
		for (const auto& key : keys) {
			auto name = casc::locale_flags::getName(key);
			locale_options.push_back({ std::string(name), key });
		}

		menu_button::render("##LocaleMenuButton", locale_options,
			current_key, false, true, locale_menu_state,
			[&](const std::string& val) {
				uint32_t flag = casc::locale_flags::getFlag(val);
				cfg["cascLocale"] = flag;
			},
			nullptr);
	}

	// JS: WebP Quality
	ImGui::SeparatorText("WebP Quality");
	{
		int val = cfg.value("exportWebPQuality", 75);
		if (ImGui::SliderInt("##WebPQuality", &val, 1, 100))
			cfg["exportWebPQuality"] = val;
	}

	// JS: Export Model Collision
	{
		bool val = cfg.value("modelsExportCollision", false);
		if (ImGui::Checkbox("Export Model Collision", &val))
			cfg["modelsExportCollision"] = val;
	}

	// JS: Export Additional UV Layers
	{
		bool val = cfg.value("modelsExportUV2", false);
		if (ImGui::Checkbox("Export Additional UV Layers", &val))
			cfg["modelsExportUV2"] = val;
	}

	// JS: Export Meta Data
	ImGui::SeparatorText("Export Meta Data");
	{
		bool m2_meta = cfg.value("exportM2Meta", false);
		if (ImGui::Checkbox("M2 Meta##meta", &m2_meta))
			cfg["exportM2Meta"] = m2_meta;
		ImGui::SameLine();
		bool wmo_meta = cfg.value("exportWMOMeta", false);
		if (ImGui::Checkbox("WMO Meta##meta", &wmo_meta))
			cfg["exportWMOMeta"] = wmo_meta;
		ImGui::SameLine();
		bool blp_meta = cfg.value("exportBLPMeta", false);
		if (ImGui::Checkbox("BLP Meta##meta", &blp_meta))
			cfg["exportBLPMeta"] = blp_meta;
		ImGui::SameLine();
		bool foliage_meta = cfg.value("exportFoliageMeta", false);
		if (ImGui::Checkbox("Foliage Meta##meta", &foliage_meta))
			cfg["exportFoliageMeta"] = foliage_meta;
	}

	// JS: Export M2 Bone Data
	{
		bool val = cfg.value("exportM2Bones", false);
		if (ImGui::Checkbox("Export M2 Bone Data", &val))
			cfg["exportM2Bones"] = val;
	}

	// JS: Always Overwrite Existing Files (Recommended)
	{
		bool val = cfg.value("overwriteFiles", true);
		if (ImGui::Checkbox("Always Overwrite Existing Files (Recommended)", &val))
			cfg["overwriteFiles"] = val;
	}

	// JS: Name Exported Files
	{
		bool val = cfg.value("exportNamedFiles", false);
		if (ImGui::Checkbox("Name Exported Files", &val))
			cfg["exportNamedFiles"] = val;
	}

	// JS: Prevent 3D Preview Overwrites
	{
		bool val = cfg.value("modelsExportPngIncrements", false);
		if (ImGui::Checkbox("Prevent 3D Preview Overwrites", &val))
			cfg["modelsExportPngIncrements"] = val;
	}

	// JS: Regular Expression Filtering (Advanced)
	{
		bool val = cfg.value("regexFilters", false);
		if (ImGui::Checkbox("Regular Expression Filtering (Advanced)", &val))
			cfg["regexFilters"] = val;
	}

	// JS: Copy Mode
	ImGui::SeparatorText("Copy Mode");
	{
		std::string copy_mode = cfg.value("copyMode", std::string("FULL"));
		if (ImGui::RadioButton("Full##copymode", copy_mode == "FULL"))
			cfg["copyMode"] = "FULL";
		ImGui::SameLine();
		if (ImGui::RadioButton("Directory##copymode", copy_mode == "DIR"))
			cfg["copyMode"] = "DIR";
		ImGui::SameLine();
		if (ImGui::RadioButton("FileDataID##copymode", copy_mode == "FID"))
			cfg["copyMode"] = "FID";
	}

	// JS: Paste Selection
	{
		bool val = cfg.value("pasteSelection", false);
		if (ImGui::Checkbox("Paste Selection", &val))
			cfg["pasteSelection"] = val;
	}

	// JS: Split Large Terrain Maps (Recommended)
	{
		bool val = cfg.value("splitLargeTerrainBakes", true);
		if (ImGui::Checkbox("Split Large Terrain Maps (Recommended)", &val))
			cfg["splitLargeTerrainBakes"] = val;
	}

	// JS: Split Alpha Maps
	{
		bool val = cfg.value("splitAlphaMaps", false);
		if (ImGui::Checkbox("Split Alpha Maps", &val))
			cfg["splitAlphaMaps"] = val;
	}

	// JS: Show unknown items
	{
		bool val = cfg.value("itemViewerShowAll", false);
		if (ImGui::Checkbox("Show Unknown Items", &val))
			cfg["itemViewerShowAll"] = val;
	}

	// JS: Cache Expiry
	ImGui::SeparatorText("Cache Expiry (hours)");
	{
		int val = cfg.value("cacheExpiry", 168);
		if (ImGui::InputInt("##CacheExpiry", &val))
			cfg["cacheExpiry"] = val;
	}

	// JS: CDN Fallback Hosts
	ImGui::SeparatorText("CDN Fallback Hosts");
	configTextInput("##CDNFallbackHosts", cdn_fallback_state, cfg, "cdnFallbackHosts");

	// JS: Manually Clear Cache (Requires Restart)
	{
		std::string btn_label = std::format("Clear Cache ({})", cache_size_formatted());
		if (ImGui::Button(btn_label.c_str()))
			handle_cache_clear();
	}

	// JS: Encryption Keys
	ImGui::SeparatorText("Encryption Keys");
	ImGui::Text("Primary TACT Keys URL");
	configTextInput("##TactKeysURL", tact_url_state, cfg, "tactKeysURL");
	ImGui::Text("Fallback TACT Keys URL");
	configTextInput("##TactKeysFallbackURL", tact_fallback_url_state, cfg, "tactKeysFallbackURL");

	// JS: Add Encryption Key
	ImGui::Text("Add Encryption Key");
	{
		static char key_name_buf[32] = {};
		static char key_val_buf[64] = {};
		ImGui::InputText("Key Name (16 hex)##addkey", key_name_buf, sizeof(key_name_buf));
		ImGui::InputText("Key Value (32 hex)##addkey", key_val_buf, sizeof(key_val_buf));
		if (ImGui::Button("Add##tactkey")) {
			view.userInputTactKeyName = key_name_buf;
			view.userInputTactKey = key_val_buf;
			handle_tact_key();
			key_name_buf[0] = '\0';
			key_val_buf[0] = '\0';
		}
	}

	// JS: Realm List Source
	ImGui::SeparatorText("Realm List Source");
	configTextInput("##RealmListURL", realm_url_state, cfg, "realmListURL");

	// JS: Character Appearance API Endpoint
	ImGui::SeparatorText("Character Appearance API Endpoint");
	configTextInput("##ArmoryURL", armory_url_state, cfg, "armoryURL");

	// JS: Use Binary Listfile Format (Requires Restart)
	{
		bool val = cfg.value("enableBinaryListfile", true);
		if (ImGui::Checkbox("Use Binary Listfile Format (Requires Restart)", &val))
			cfg["enableBinaryListfile"] = val;
	}

	// JS: Listfile Binary Source
	ImGui::SeparatorText("Listfile Binary Source");
	configTextInput("##ListfileBinSrc", listfile_bin_state, cfg, "listfileBinarySource");

	// JS: Listfile Source (Legacy)
	ImGui::SeparatorText("Listfile Source");
	ImGui::Text("Primary");
	configTextInput("##ListfileURL", listfile_url_state, cfg, "listfileURL");
	ImGui::Text("Fallback");
	configTextInput("##ListfileFallbackURL", listfile_fb_url_state, cfg, "listfileFallbackURL");

	// JS: Listfile Update Frequency
	ImGui::SeparatorText("Listfile Update Frequency (hours)");
	{
		int val = cfg.value("listfileCacheRefresh", 168);
		if (ImGui::InputInt("##ListfileCacheRefresh", &val))
			cfg["listfileCacheRefresh"] = val;
	}

	// JS: Data Table Definition Repository
	ImGui::SeparatorText("Data Table Definition Repository");
	ImGui::Text("Primary");
	configTextInput("##DBDURL", dbd_url_state, cfg, "dbdURL");
	ImGui::Text("Fallback");
	configTextInput("##DBDFallbackURL", dbd_fb_url_state, cfg, "dbdFallbackURL");

	// JS: DBD Manifest Repository
	ImGui::SeparatorText("DBD Manifest Repository");
	ImGui::Text("Primary");
	configTextInput("##DBDFilenameURL", dbdf_url_state, cfg, "dbdFilenameURL");
	ImGui::Text("Fallback");
	configTextInput("##DBDFilenameFallbackURL", dbdf_fb_url_state, cfg, "dbdFilenameFallbackURL");

	// JS: Allow Cache Collection
	{
		bool val = cfg.value("allowCacheCollection", false);
		if (ImGui::Checkbox("Allow Cache Collection", &val))
			cfg["allowCacheCollection"] = val;
	}

	ImGui::EndChild(); // config-scroll

	// JS: <div id="config-buttons">
	//   <input type="button" value="Discard" @click="handle_discard"/>
	//   <input type="button" value="Apply" @click="handle_apply"/>
	//   <input type="button" id="config-reset" value="Reset to Defaults" @click="handle_reset"/>
	ImGui::Separator();
	if (ImGui::Button("Discard"))
		handle_discard();
	ImGui::SameLine();
	if (ImGui::Button("Apply"))
		handle_apply();
	ImGui::SameLine();
	if (ImGui::Button("Reset to Defaults"))
		handle_reset();

	ImGui::EndChild(); // settings-content
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
