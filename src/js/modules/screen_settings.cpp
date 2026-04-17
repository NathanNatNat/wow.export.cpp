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

#include "../../app.h"

namespace screen_settings {

// --- File-local state ---

static std::optional<nlohmann::json> default_config;

// FileField widget states for directory pickers.
static file_field::FileFieldState export_dir_state;
static file_field::FileFieldState char_save_dir_state;
static menu_button::MenuButtonState locale_menu_state;

/**
 * Render a settings section heading as bold text at 18px font size.
 * CSS: #config > div h1 { font-size: 18px; }
 * Replaces SectionHeading() to match the original JS heading style.
 */
static void SectionHeading(const char* label) {
	// CSS: #config > div { padding: 20px; padding-bottom: 0; }
	ImGui::Dummy(ImVec2(0.0f, 20.0f));
	ImGui::SetWindowFontScale(18.0f / app::theme::DEFAULT_FONT_SIZE);
	ImGui::TextUnformatted(label);
	ImGui::SetWindowFontScale(1.0f);
}

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

static nlohmann::json load_default_config() {
	if (!default_config.has_value()) {
		auto result = generics::readJSON(constants::CONFIG::DEFAULT_PATH(), true);
		default_config = result.value_or(nlohmann::json::object());
	}

	return default_config.value();
}

static bool is_edit_export_path_concerning() {
	const auto& cfg = core::view->configEdit;
	if (!cfg.contains("exportDirectory"))
		return false;

	const std::string& path = cfg["exportDirectory"].get_ref<const std::string&>();
	return path.find(' ') != std::string::npos ||
		   path.find('\t') != std::string::npos;
}

static std::string default_character_path() {
	return tab_characters::get_default_characters_dir();
}

static std::string cache_size_formatted() {
	return generics::filesize(static_cast<double>(core::view->cacheSize));
}

// Returns the locale ID strings from locale_flags::entries.
static std::vector<std::string> available_locale_keys() {
	std::vector<std::string> keys;
	for (const auto& entry : casc::locale_flags::entries)
		keys.emplace_back(entry.id);
	return keys;
}

static std::string selected_locale_key() {
	uint32_t current_locale = core::view->config.value("cascLocale", static_cast<uint32_t>(casc::locale_flags::enUS));
	for (const auto& entry : casc::locale_flags::entries) {
		if (entry.flag == current_locale)
			return std::string(entry.id);
	}
	return "unUN";
}

static void go_home() {
	modules::go_to_landing();
}

// --- Public functions ---

void registerScreen() {
	modules::registerContextMenuOption("settings", "Manage Settings", "gear.svg",
		[]() { modules::set_active("settings"); });
}

void mounted() {
	core::view->configEdit = core::view->config;

	// Reset FileField states.
	export_dir_state = file_field::FileFieldState{};
	char_save_dir_state = file_field::FileFieldState{};
}

void render() {
	auto& view = *core::view;
	auto& cfg = view.configEdit;

	// --- Template rendering ---

	// CSS: #config-wrapper fills the entire screen. #config has flex: 1 filling available width.
	const float availW = ImGui::GetContentRegionAvail().x;
	ImGui::BeginChild("settings-content", ImVec2(availW, 0));

	ImGui::BeginChild("##config-scroll", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2));

	SectionHeading("Export Directory");
	ImGui::TextWrapped("Local directory where files will be exported to.");
	if (is_edit_export_path_concerning())
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: Using an export path with spaces may lead to problems in most 3D programs.");
	{
		std::string export_dir = cfg.value("exportDirectory", std::string(""));
		file_field::render("##ExportDir", export_dir, "", export_dir_state,
			[&](const std::string& new_path) { cfg["exportDirectory"] = new_path; });
	}

	SectionHeading("Character Save Directory");
	ImGui::TextWrapped("Local directory where saved characters are stored. Leave empty to use the default location.");
	{
		std::string char_dir = cfg.value("characterExportPath", std::string(""));
		file_field::render("##CharSaveDir", char_dir, default_character_path().c_str(), char_save_dir_state,
			[&](const std::string& new_path) { cfg["characterExportPath"] = new_path; });
	}

	SectionHeading("Scroll Speed");
	ImGui::TextWrapped("How many lines at a time you scroll down in the results view (leave at 0 for default scroll amount)");
	{
		int scroll_speed = cfg.value("scrollSpeed", 2);
		if (ImGui::InputInt("##ScrollSpeed", &scroll_speed))
			cfg["scrollSpeed"] = scroll_speed;
	}

	{
		bool val = cfg.value("listfileSortByID", false);
		SectionHeading("Display File Lists in Numerical Order (FileDataID)");
		ImGui::TextWrapped("If enabled, all file lists will be ordered numerically by file ID, otherwise alphabetically by filename.");
		if (ImGui::Checkbox("Enable##listfileSortByID", &val))
			cfg["listfileSortByID"] = val;
	}

	{
		bool val = cfg.value("enableUnknownFiles", false);
		SectionHeading("Find Unknown Files (Requires Restart)");
		ImGui::TextWrapped("If enabled, wow.export.cpp will use various methods to locate unknown files.");
		if (ImGui::Checkbox("Enable##enableUnknownFiles", &val))
			cfg["enableUnknownFiles"] = val;
	}

	{
		bool val = cfg.value("enableM2Skins", true);
		SectionHeading("Load Model Skins (Requires Restart)");
		ImGui::TextWrapped("If enabled, wow.export.cpp will parse creature and item skins from data tables for M2 models.");
		ImGui::TextWrapped("Disabling this will reduce memory usage and improve loading, but will disable M2 skin functionality.");
		if (ImGui::Checkbox("Enable##enableM2Skins", &val))
			cfg["enableM2Skins"] = val;
	}

	{
		bool val = cfg.value("modelsExportWithBonePrefix", false);
		SectionHeading("Include Bone Prefixes");
		ImGui::TextWrapped("If enabled, wow.export.cpp will Include _p Bone prefixes in model skeleton/armature.");
		ImGui::TextWrapped("Disabling this will break backwards compatibility with previous glTF model and animation exports.");
		if (ImGui::Checkbox("Enable##modelsExportWithBonePrefix", &val))
			cfg["modelsExportWithBonePrefix"] = val;
	}

	{
		bool val = cfg.value("enableSharedTextures", true);
		SectionHeading("Enable Shared Textures (Recommended)");
		ImGui::TextWrapped("If enabled, exported textures will be exported to their own path rather than with their parent.");
		ImGui::TextWrapped("This dramatically reduces disk space used by not duplicating textures.");
		if (ImGui::Checkbox("Enable##enableSharedTextures", &val))
			cfg["enableSharedTextures"] = val;
	}

	{
		bool val = cfg.value("enableSharedChildren", true);
		SectionHeading("Enable Shared Children (Recommended)");
		ImGui::TextWrapped("If enabled, exported models on a WMO/ADT will be exported to their own path rather than with their parent.");
		ImGui::TextWrapped("This dramatically reduces disk space used by not duplicating models.");
		if (ImGui::Checkbox("Enable##enableSharedChildren", &val))
			cfg["enableSharedChildren"] = val;
	}

	{
		bool val = cfg.value("removePathSpaces", false);
		SectionHeading("Strip Whitespace From Export Paths");
		ImGui::TextWrapped("If enabled, whitespace will be removed from exported paths.");
		ImGui::TextWrapped("Enable this option if your 3D software does not support whitespace in paths.");
		if (ImGui::Checkbox("Enable##removePathSpaces", &val))
			cfg["removePathSpaces"] = val;
	}

	{
		bool val = cfg.value("removePathSpacesCopy", false);
		SectionHeading("Strip Whitespace From Copied Paths");
		ImGui::TextWrapped("If enabled, file paths copied from a listbox (CTRL + C) will have whitespace stripped.");
		if (ImGui::Checkbox("Enable##removePathSpacesCopy", &val))
			cfg["removePathSpacesCopy"] = val;
	}

	SectionHeading("Path Separator Format");
	ImGui::TextWrapped("Sets the path separator format used in exported files.");
	// Note: JS uses .ui-multi-button grouped toggles; C++ uses ImGui radio buttons
	// which provide equivalent toggle functionality with a different visual style.
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

	{
		bool val = cfg.value("enableAbsoluteMTLPaths", false);
		SectionHeading("Use Absolute MTL Texture Paths");
		ImGui::TextWrapped("If enabled, MTL files will contain absolute textures paths rather than relative ones.");
		ImGui::TextWrapped("This will cause issues when sharing exported models between computers.");
		ImGui::TextWrapped("Enable this option if you are having issues importing OBJ models with Shared Textures enabled.");
		if (ImGui::Checkbox("Enable##enableAbsoluteMTLPaths", &val))
			cfg["enableAbsoluteMTLPaths"] = val;
	}

	{
		bool val = cfg.value("enableAbsoluteGLTFPaths", false);
		SectionHeading("Use Absolute glTF Texture Paths");
		ImGui::TextWrapped("If enabled, glTF files will contain absolute textures paths rather than relative ones.");
		ImGui::TextWrapped("This will cause issues when sharing exported models between computers.");
		ImGui::TextWrapped("Enable this option if you are having issues importing glTF models with Shared Textures enabled.");
		if (ImGui::Checkbox("Enable##enableAbsoluteGLTFPaths", &val))
			cfg["enableAbsoluteGLTFPaths"] = val;
	}

	{
		bool val = cfg.value("enableAbsoluteCSVPaths", false);
		SectionHeading("Use Absolute Model Placement Paths");
		ImGui::TextWrapped("If enabled, paths inside model placement files (CSV) will contain absolute paths rather than relative ones.");
		ImGui::TextWrapped("This will cause issues when sharing exported models between computers.");
		if (ImGui::Checkbox("Enable##enableAbsoluteCSVPaths", &val))
			cfg["enableAbsoluteCSVPaths"] = val;
	}

	SectionHeading("CASC Locale");
	ImGui::TextWrapped("Which locale to use for file reading. This only affects game files.");
	ImGui::TextWrapped("This should match the locale of your client when using local installations.");
	{
		auto keys = available_locale_keys();
		std::string current_key = selected_locale_key();

		std::vector<menu_button::MenuOption> locale_options;
		for (const auto& key : keys) {
			// JS: available_locale_keys returns { value: e } where e is the short key.
			// The MenuButton displays the short key (e.g. "enUS"), not the full name.
			locale_options.push_back({ key, key });
		}

		menu_button::render("##LocaleMenuButton", locale_options,
			current_key, false, true, locale_menu_state,
			[&](const std::string& val) {
				uint32_t flag = casc::locale_flags::getFlag(val);
				cfg["cascLocale"] = flag;
			},
			nullptr);
	}

	SectionHeading("WebP Quality");
	ImGui::TextWrapped("Quality setting for WebP exports. Range is 1-100 (100 is lossless)");
	{
		// JS: <input type="number" min="1" max="100">
		int val = cfg.value("exportWebPQuality", 75);
		if (ImGui::InputInt("##WebPQuality", &val)) {
			if (val < 1) val = 1;
			if (val > 100) val = 100;
			cfg["exportWebPQuality"] = val;
		}
	}

	{
		bool val = cfg.value("modelsExportCollision", false);
		SectionHeading("Export Model Collision");
		ImGui::TextWrapped("If enabled, M2 models exported as OBJ will also have their collision exported into a .phys.obj file.");
		if (ImGui::Checkbox("Enable##modelsExportCollision", &val))
			cfg["modelsExportCollision"] = val;
	}

	{
		bool val = cfg.value("modelsExportUV2", false);
		SectionHeading("Export Additional UV Layers");
		ImGui::TextWrapped("If enabled, additional UV layers will be exported for M2/WMO models, included as non-standard properties (vt2, vt3, etc) in OBJ files.");
		ImGui::TextWrapped("Use the wow.export Blender add-on to import OBJ models with additional UV layers.");
		if (ImGui::Checkbox("Enable##modelsExportUV2", &val))
			cfg["modelsExportUV2"] = val;
	}

	SectionHeading("Export Meta Data");
	ImGui::TextWrapped("If enabled, verbose data will be exported for enabled formats into relative .json files.");
	// Note: JS uses .ui-multi-button grouped toggles; C++ uses checkboxes on SameLine
	// which provide equivalent toggle functionality with a different visual style.
	{
		bool m2_meta = cfg.value("exportM2Meta", false);
		if (ImGui::Checkbox("M2##meta", &m2_meta))
			cfg["exportM2Meta"] = m2_meta;
		ImGui::SameLine();
		bool wmo_meta = cfg.value("exportWMOMeta", false);
		if (ImGui::Checkbox("WMO##meta", &wmo_meta))
			cfg["exportWMOMeta"] = wmo_meta;
		ImGui::SameLine();
		bool blp_meta = cfg.value("exportBLPMeta", false);
		if (ImGui::Checkbox("BLP##meta", &blp_meta))
			cfg["exportBLPMeta"] = blp_meta;
		ImGui::SameLine();
		bool foliage_meta = cfg.value("exportFoliageMeta", false);
		if (ImGui::Checkbox("Foliage##meta", &foliage_meta))
			cfg["exportFoliageMeta"] = foliage_meta;
	}

	{
		bool val = cfg.value("exportM2Bones", false);
		SectionHeading("Export M2 Bone Data");
		ImGui::TextWrapped("If enabled, bone data will be exported in a relative _bones.json file.");
		if (ImGui::Checkbox("Enable##exportM2Bones", &val))
			cfg["exportM2Bones"] = val;
	}

	{
		bool val = cfg.value("overwriteFiles", true);
		SectionHeading("Always Overwrite Existing Files (Recommended)");
		ImGui::TextWrapped("When exporting, files will always be written to disk even if they exist.");
		ImGui::TextWrapped("Disabling this can speed up exporting, but may lead to issues between versions.");
		if (ImGui::Checkbox("Enable##overwriteFiles", &val))
			cfg["overwriteFiles"] = val;
	}

	{
		bool val = cfg.value("exportNamedFiles", false);
		SectionHeading("Name Exported Files");
		ImGui::TextWrapped("When enabled, files are exported using their listfile names (e.g., \"ability_stealth.png\").");
		ImGui::TextWrapped("When disabled, files are exported using their fileDataID numbers (e.g., \"12345.png\").");
		if (ImGui::Checkbox("Enable##exportNamedFiles", &val))
			cfg["exportNamedFiles"] = val;
	}

	{
		bool val = cfg.value("modelsExportPngIncrements", false);
		SectionHeading("Prevent 3D Preview Overwrites");
		ImGui::TextWrapped("If enabled, 3D preview exports will add increments to prevent overwriting existing files.");
		if (ImGui::Checkbox("Enable##modelsExportPngIncrements", &val))
			cfg["modelsExportPngIncrements"] = val;
	}

	{
		bool val = cfg.value("regexFilters", false);
		SectionHeading("Regular Expression Filtering (Advanced)");
		ImGui::TextWrapped("Allows use of regular expressions in filtering lists.");
		if (ImGui::Checkbox("Enable##regexFilters", &val))
			cfg["regexFilters"] = val;
	}

	SectionHeading("Copy Mode");
	ImGui::TextWrapped("By default, using CTRL + C on a file list will copy the full entry to your clipboard.");
	ImGui::TextWrapped("Setting this to Directory will instead only copy the directory of the given entry.");
	ImGui::TextWrapped("Setting this to FileDataID will instead only copy the FID of the entry (must have FIDs enabled).");
	// Note: JS uses .ui-multi-button grouped toggles; C++ uses radio buttons
	// which provide equivalent toggle functionality with a different visual style.
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

	{
		bool val = cfg.value("pasteSelection", false);
		SectionHeading("Paste Selection");
		ImGui::TextWrapped("If enabled, using CTRL + V on the model list will attempt to select filenames you paste.");
		if (ImGui::Checkbox("Enable##pasteSelection", &val))
			cfg["pasteSelection"] = val;
	}

	{
		bool val = cfg.value("splitLargeTerrainBakes", true);
		SectionHeading("Split Large Terrain Maps (Recommended)");
		ImGui::TextWrapped("If enabled, exporting baked terrain above 8k will be split into smaller files rather than one large file.");
		if (ImGui::Checkbox("Enable##splitLargeTerrainBakes", &val))
			cfg["splitLargeTerrainBakes"] = val;
	}

	{
		bool val = cfg.value("splitAlphaMaps", false);
		SectionHeading("Split Alpha Maps");
		ImGui::TextWrapped("If enabled, terrain alpha maps will be exported as individual images for each ADT chunk.");
		if (ImGui::Checkbox("Enable##splitAlphaMaps", &val))
			cfg["splitAlphaMaps"] = val;
	}

	{
		bool val = cfg.value("itemViewerShowAll", false);
		SectionHeading("Show unknown items");
		ImGui::TextWrapped("When enabled, wow.export.cpp will list all items in the items tab, even those without a name.");
		if (ImGui::Checkbox("Enable##itemViewerShowAll", &val))
			cfg["itemViewerShowAll"] = val;
	}

	// Item 51: JS labels this as "Cache Expiry" with description "After how many days..."
	SectionHeading("Cache Expiry");
	ImGui::TextWrapped("After how many days of inactivity is cached data deleted. Setting to zero disables cache clean-up (not recommended).");
	{
		int val = cfg.value("cacheExpiry", 168);
		if (ImGui::InputInt("##CacheExpiry", &val))
			cfg["cacheExpiry"] = val;
	}

	SectionHeading("CDN Fallback Hosts");
	ImGui::TextWrapped("Comma-separated list of additional CDN hostnames to try when official CDN servers are unavailable or slow.");
	ImGui::TextWrapped("These are pinged alongside official servers and used based on speed and availability.");
	configTextInput("##CDNFallbackHosts", cdn_fallback_state, cfg, "cdnFallbackHosts");

	// Item 53: JS heading is "Manually Clear Cache (Requires Restart)"
	SectionHeading("Manually Clear Cache (Requires Restart)");
	ImGui::TextWrapped("While housekeeping on the cache is mostly automatic, sometimes clearing manually can resolve issues.");
	{
		// JS: :class="{ disabled: $core.view.isBusy }" — visually disable when busy
		bool busy = view.isBusy;
		if (busy) {
			ImGui::BeginDisabled();
		}
		std::string btn_label = std::format("Clear Cache ({})", cache_size_formatted());
		if (ImGui::Button(btn_label.c_str()))
			handle_cache_clear();
		if (busy) {
			ImGui::EndDisabled();
		}
	}

	SectionHeading("Encryption Keys");
	ImGui::TextWrapped("Remote URL used to update keys for encrypted files.");
	ImGui::Text("Primary");
	configTextInput("##TactKeysURL", tact_url_state, cfg, "tactKeysURL");
	ImGui::Text("Fallback");
	configTextInput("##TactKeysFallbackURL", tact_fallback_url_state, cfg, "tactKeysFallbackURL");

	SectionHeading("Add Encryption Key");
	ImGui::TextWrapped("Manually add a BLTE encryption key.");
	{
		// Item 56: JS maxlength="16" for key name, maxlength="32" for key value
		static char key_name_buf[17] = {}; // 16 chars + null terminator
		static char key_val_buf[33] = {};  // 32 chars + null terminator
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

	SectionHeading("Realm List Source");
	ImGui::TextWrapped("Remote URL used for retrieving the realm list. (Must use same format)");
	configTextInput("##RealmListURL", realm_url_state, cfg, "realmListURL");

	SectionHeading("Character Appearance API Endpoint");
	ImGui::TextWrapped("Remote URL used for retrieving data from the Battle.net Character Appearance API. (Must use same format)");
	configTextInput("##ArmoryURL", armory_url_state, cfg, "armoryURL");

	{
		bool val = cfg.value("enableBinaryListfile", true);
		SectionHeading("Use Binary Listfile Format (Requires Restart)");
		ImGui::TextWrapped("If enabled, wow.export.cpp will use the optimized binary listfile format instead of the legacy CSV format.");
		if (ImGui::Checkbox("Enable##enableBinaryListfile", &val))
			cfg["enableBinaryListfile"] = val;
	}

	SectionHeading("Listfile Binary Source");
	ImGui::TextWrapped("Remote URL used for downloading the optimized binary listfile format. (Must use same format)");
	configTextInput("##ListfileBinSrc", listfile_bin_state, cfg, "listfileBinarySource");

	// Item 57: JS heading is "Listfile Source (Legacy)"
	SectionHeading("Listfile Source (Legacy)");
	ImGui::TextWrapped("Remote URL or local path used for updating the CASC listfile. (Must use same format)");
	ImGui::Text("Primary");
	configTextInput("##ListfileURL", listfile_url_state, cfg, "listfileURL");
	ImGui::Text("Fallback");
	configTextInput("##ListfileFallbackURL", listfile_fb_url_state, cfg, "listfileFallbackURL");

	// Item 51: JS heading is "Listfile Update Frequency" with description "How often (in days)..."
	SectionHeading("Listfile Update Frequency");
	ImGui::TextWrapped("How often (in days) the listfile is updated. Set to zero to always re-download the listfile.");
	{
		int val = cfg.value("listfileCacheRefresh", 168);
		if (ImGui::InputInt("##ListfileCacheRefresh", &val))
			cfg["listfileCacheRefresh"] = val;
	}

	SectionHeading("Data Table Definition Repository");
	ImGui::TextWrapped("Remote URL used to update DBD definitions. (Must use same format)");
	ImGui::Text("Primary");
	configTextInput("##DBDURL", dbd_url_state, cfg, "dbdURL");
	ImGui::Text("Fallback");
	configTextInput("##DBDFallbackURL", dbd_fb_url_state, cfg, "dbdFallbackURL");

	SectionHeading("DBD Manifest Repository");
	ImGui::TextWrapped("Remote URL used to obtain DBD manifest information. (Must use same format)");
	ImGui::Text("Primary");
	configTextInput("##DBDFilenameURL", dbdf_url_state, cfg, "dbdFilenameURL");
	ImGui::Text("Fallback");
	configTextInput("##DBDFilenameFallbackURL", dbdf_fb_url_state, cfg, "dbdFilenameFallbackURL");

	{
		bool val = cfg.value("allowCacheCollection", false);
		SectionHeading("Allow Cache Collection");
		ImGui::TextWrapped("If enabled, wow.export.cpp will anonymously collect cache data from your client for community usage.");
		if (ImGui::Checkbox("Enable##allowCacheCollection", &val))
			cfg["allowCacheCollection"] = val;
	}

	ImGui::EndChild(); // config-scroll

	// CSS: #config-buttons { display: flex; flex-direction: row-reverse; padding: 15px 0; border-top: 1px solid var(--border); background: var(--background); }
	// row-reverse puts the first button on the right. Order in HTML: Discard, Apply, Reset.
	// Visually: [Reset to Defaults (far left)] ... [Apply] [Discard (far right)]
	ImGui::Spacing();
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImVec2 curPos = ImGui::GetCursorScreenPos();
		float availW = ImGui::GetContentRegionAvail().x;
		// border-top: 1px solid var(--border)
		dl->AddLine(ImVec2(curPos.x, curPos.y), ImVec2(curPos.x + availW, curPos.y), app::theme::BORDER_U32);
	}
	ImGui::Dummy(ImVec2(0.0f, 15.0f));

	// CSS: row-reverse — "Reset to Defaults" has margin-right: auto (pushes it to the left),
	// "Discard" and "Apply" are on the right side.
	// CSS: #config-buttons #config-reset { margin-right: auto; margin-left: 20px; }
	// Item 55: JS applies :class="{ disabled: $core.view.isBusy }" to all 3 buttons
	bool busy = view.isBusy;
	if (busy)
		ImGui::BeginDisabled();

	if (ImGui::Button("Reset to Defaults"))
		handle_reset();
	ImGui::SameLine();
	// Push remaining buttons to the right.
	const float buttonWidth = ImGui::CalcTextSize("Discard").x + ImGui::CalcTextSize("Apply").x + ImGui::GetStyle().FramePadding.x * 4 + 20.0f * 2;
	ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - buttonWidth);
	if (ImGui::Button("Apply"))
		handle_apply();
	ImGui::SameLine();
	if (ImGui::Button("Discard"))
		handle_discard();

	if (busy)
		ImGui::EndDisabled();

	ImGui::Dummy(ImVec2(0.0f, 15.0f));

	ImGui::EndChild(); // settings-content
}

static void handle_cache_clear() {
	//     this.$core.events.emit('click-cache-clear');
	if (!core::view->isBusy)
		core::events.emit("click-cache-clear");
}

static void handle_tact_key() {
	if (casc::tact_keys::addKey(core::view->userInputTactKeyName, core::view->userInputTactKey))
		core::setToast("success", "Successfully added decryption key.");
	else
		core::setToast("error", "Invalid encryption key.", {}, -1);
}

void handle_discard() {
	if (core::view->isBusy)
		return;

	go_home();
}

void handle_apply() {
	if (core::view->isBusy)
		return;

	auto& cfg = core::view->configEdit;
	nlohmann::json defaults = load_default_config();

	if (!cfg.contains("exportDirectory") || cfg["exportDirectory"].get<std::string>().empty()) {
		core::setToast("error", "A valid export directory must be provided", {}, -1);
		return;
	}

	{
		std::string val = cfg.value("realmListURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			core::setToast("error", "A valid realm list URL or path is required.",
				{ {"Use Default", [&cfg, defaults]() { cfg["realmListURL"] = defaults["realmListURL"]; }} }, -1);
			return;
		}
	}

	{
		std::string val = cfg.value("listfileURL", std::string{});
		if (val.empty()) {
			core::setToast("error", "A valid listfile URL or path is required.",
				{ {"Use Default", [&cfg, defaults]() { cfg["listfileURL"] = defaults["listfileURL"]; }} }, -1);
			return;
		}
	}

	{
		std::string val = cfg.value("armoryURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			core::setToast("error", "A valid URL is required for the Character Appearance API.",
				{ {"Use Default", [&cfg, defaults]() { cfg["armoryURL"] = defaults["armoryURL"]; }} }, -1);
			return;
		}
	}

	{
		std::string val = cfg.value("tactKeysURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			core::setToast("error", "A valid URL is required for encryption key updates.",
				{ {"Use Default", [&cfg, defaults]() { cfg["tactKeysURL"] = defaults["tactKeysURL"]; }} }, -1);
			return;
		}
	}

	{
		std::string val = cfg.value("dbdURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			core::setToast("error", "A valid URL is required for DBD updates.",
				{ {"Use Default", [&cfg, defaults]() { cfg["dbdURL"] = defaults["dbdURL"]; }} }, -1);
			return;
		}
	}

	{
		std::string val = cfg.value("dbdFilenameURL", std::string{});
		if (val.empty() || val.substr(0, 4) != "http") {
			core::setToast("error", "A valid URL is required for DBD manifest.",
				{ {"Use Default", [&cfg, defaults]() { cfg["dbdFilenameURL"] = defaults["dbdFilenameURL"]; }} }, -1);
			return;
		}
	}

	core::view->config = cfg;

	go_home();

	core::setToast("success", "Changes to your configuration have been saved!");
}

void handle_reset() {
	if (core::view->isBusy)
		return;

	nlohmann::json defaults = load_default_config();
	core::view->configEdit = nlohmann::json::parse(defaults.dump());
}

} // namespace screen_settings
