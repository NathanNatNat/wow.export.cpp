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
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: Using an export path with spaces may cause issues in some applications.");
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
	{
		int scroll_speed = cfg.value("scrollSpeed", 2);
		if (ImGui::InputInt("##ScrollSpeed", &scroll_speed))
			cfg["scrollSpeed"] = scroll_speed;
	}

	{
		bool val = cfg.value("listfileSortByID", false);
		if (ImGui::Checkbox("Display File Lists in Numerical Order (FileDataID)", &val))
			cfg["listfileSortByID"] = val;
	}

	{
		bool val = cfg.value("enableUnknownFiles", false);
		if (ImGui::Checkbox("Find Unknown Files (Requires Restart)", &val))
			cfg["enableUnknownFiles"] = val;
	}

	{
		bool val = cfg.value("enableM2Skins", true);
		if (ImGui::Checkbox("Load Model Skins (Requires Restart)", &val))
			cfg["enableM2Skins"] = val;
	}

	{
		bool val = cfg.value("modelsExportWithBonePrefix", false);
		if (ImGui::Checkbox("Include Bone Prefixes", &val))
			cfg["modelsExportWithBonePrefix"] = val;
	}

	{
		bool val = cfg.value("enableSharedTextures", true);
		if (ImGui::Checkbox("Enable Shared Textures (Recommended)", &val))
			cfg["enableSharedTextures"] = val;
	}

	{
		bool val = cfg.value("enableSharedChildren", true);
		if (ImGui::Checkbox("Enable Shared Children (Recommended)", &val))
			cfg["enableSharedChildren"] = val;
	}

	{
		bool val = cfg.value("removePathSpaces", false);
		if (ImGui::Checkbox("Strip Whitespace From Export Paths", &val))
			cfg["removePathSpaces"] = val;
	}

	{
		bool val = cfg.value("removePathSpacesCopy", false);
		if (ImGui::Checkbox("Strip Whitespace From Copied Paths", &val))
			cfg["removePathSpacesCopy"] = val;
	}

	SectionHeading("Path Separator Format");
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
		if (ImGui::Checkbox("Use Absolute MTL Texture Paths", &val))
			cfg["enableAbsoluteMTLPaths"] = val;
	}

	{
		bool val = cfg.value("enableAbsoluteGLTFPaths", false);
		if (ImGui::Checkbox("Use Absolute glTF Texture Paths", &val))
			cfg["enableAbsoluteGLTFPaths"] = val;
	}

	{
		bool val = cfg.value("enableAbsoluteCSVPaths", false);
		if (ImGui::Checkbox("Use Absolute Model Placement Paths", &val))
			cfg["enableAbsoluteCSVPaths"] = val;
	}

	SectionHeading("CASC Locale");
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

	SectionHeading("WebP Quality");
	{
		int val = cfg.value("exportWebPQuality", 75);
		if (ImGui::SliderInt("##WebPQuality", &val, 1, 100))
			cfg["exportWebPQuality"] = val;
	}

	{
		bool val = cfg.value("modelsExportCollision", false);
		if (ImGui::Checkbox("Export Model Collision", &val))
			cfg["modelsExportCollision"] = val;
	}

	{
		bool val = cfg.value("modelsExportUV2", false);
		if (ImGui::Checkbox("Export Additional UV Layers", &val))
			cfg["modelsExportUV2"] = val;
	}

	SectionHeading("Export Meta Data");
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

	{
		bool val = cfg.value("exportM2Bones", false);
		if (ImGui::Checkbox("Export M2 Bone Data", &val))
			cfg["exportM2Bones"] = val;
	}

	{
		bool val = cfg.value("overwriteFiles", true);
		if (ImGui::Checkbox("Always Overwrite Existing Files (Recommended)", &val))
			cfg["overwriteFiles"] = val;
	}

	{
		bool val = cfg.value("exportNamedFiles", false);
		if (ImGui::Checkbox("Name Exported Files", &val))
			cfg["exportNamedFiles"] = val;
	}

	{
		bool val = cfg.value("modelsExportPngIncrements", false);
		if (ImGui::Checkbox("Prevent 3D Preview Overwrites", &val))
			cfg["modelsExportPngIncrements"] = val;
	}

	{
		bool val = cfg.value("regexFilters", false);
		if (ImGui::Checkbox("Regular Expression Filtering (Advanced)", &val))
			cfg["regexFilters"] = val;
	}

	SectionHeading("Copy Mode");
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
		if (ImGui::Checkbox("Paste Selection", &val))
			cfg["pasteSelection"] = val;
	}

	{
		bool val = cfg.value("splitLargeTerrainBakes", true);
		if (ImGui::Checkbox("Split Large Terrain Maps (Recommended)", &val))
			cfg["splitLargeTerrainBakes"] = val;
	}

	{
		bool val = cfg.value("splitAlphaMaps", false);
		if (ImGui::Checkbox("Split Alpha Maps", &val))
			cfg["splitAlphaMaps"] = val;
	}

	{
		bool val = cfg.value("itemViewerShowAll", false);
		if (ImGui::Checkbox("Show Unknown Items", &val))
			cfg["itemViewerShowAll"] = val;
	}

	SectionHeading("Cache Expiry (hours)");
	{
		int val = cfg.value("cacheExpiry", 168);
		if (ImGui::InputInt("##CacheExpiry", &val))
			cfg["cacheExpiry"] = val;
	}

	SectionHeading("CDN Fallback Hosts");
	configTextInput("##CDNFallbackHosts", cdn_fallback_state, cfg, "cdnFallbackHosts");

	{
		std::string btn_label = std::format("Clear Cache ({})", cache_size_formatted());
		if (ImGui::Button(btn_label.c_str()))
			handle_cache_clear();
	}

	SectionHeading("Encryption Keys");
	ImGui::Text("Primary TACT Keys URL");
	configTextInput("##TactKeysURL", tact_url_state, cfg, "tactKeysURL");
	ImGui::Text("Fallback TACT Keys URL");
	configTextInput("##TactKeysFallbackURL", tact_fallback_url_state, cfg, "tactKeysFallbackURL");

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

	SectionHeading("Realm List Source");
	configTextInput("##RealmListURL", realm_url_state, cfg, "realmListURL");

	SectionHeading("Character Appearance API Endpoint");
	configTextInput("##ArmoryURL", armory_url_state, cfg, "armoryURL");

	{
		bool val = cfg.value("enableBinaryListfile", true);
		if (ImGui::Checkbox("Use Binary Listfile Format (Requires Restart)", &val))
			cfg["enableBinaryListfile"] = val;
	}

	SectionHeading("Listfile Binary Source");
	configTextInput("##ListfileBinSrc", listfile_bin_state, cfg, "listfileBinarySource");

	SectionHeading("Listfile Source");
	ImGui::Text("Primary");
	configTextInput("##ListfileURL", listfile_url_state, cfg, "listfileURL");
	ImGui::Text("Fallback");
	configTextInput("##ListfileFallbackURL", listfile_fb_url_state, cfg, "listfileFallbackURL");

	SectionHeading("Listfile Update Frequency (hours)");
	{
		int val = cfg.value("listfileCacheRefresh", 168);
		if (ImGui::InputInt("##ListfileCacheRefresh", &val))
			cfg["listfileCacheRefresh"] = val;
	}

	SectionHeading("Data Table Definition Repository");
	ImGui::Text("Primary");
	configTextInput("##DBDURL", dbd_url_state, cfg, "dbdURL");
	ImGui::Text("Fallback");
	configTextInput("##DBDFallbackURL", dbd_fb_url_state, cfg, "dbdFallbackURL");

	SectionHeading("DBD Manifest Repository");
	ImGui::Text("Primary");
	configTextInput("##DBDFilenameURL", dbdf_url_state, cfg, "dbdFilenameURL");
	ImGui::Text("Fallback");
	configTextInput("##DBDFilenameFallbackURL", dbdf_fb_url_state, cfg, "dbdFilenameFallbackURL");

	{
		bool val = cfg.value("allowCacheCollection", false);
		if (ImGui::Checkbox("Allow Cache Collection", &val))
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
