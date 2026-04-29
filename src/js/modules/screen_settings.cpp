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
#include "../config.h"
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
 * Render a settings section heading: bold font at 18px, with 20px top spacing.
 * CSS: #config > div h1 { font-size: 18px; font-weight: bold (browser default for h1) }
 * CSS: #config > div { padding-top: 20px }
 */
static void SectionHeading(const char* label) {
	ImGui::Dummy(ImVec2(0.0f, 20.0f));
	ImFont* bold = app::theme::getBoldFont();
	ImGui::PushFont(bold, 18.0f);
	ImGui::TextUnformatted(label);
	ImGui::PopFont();
}

/**
 * Render a single segment of a .ui-multi-button bar.
 * Uses InvisibleButton for hit detection + manual drawing for correct per-corner rounding.
 * CSS: .ui-multi-button li { background: var(--form-button-base); padding: 10px; display: inline-block }
 * CSS: .ui-multi-button li:hover, .selected { background: var(--form-button-hover) }
 * CSS: first-child { border-radius: 5px on left corners }, last-child { right corners }
 */
static bool multiButtonSegment(int idx, const char* label, bool selected, bool is_first, bool is_last) {
	ImGui::PushID(idx);
	const float pad_x = 10.0f, pad_y = 10.0f;
	ImVec2 label_size = ImGui::CalcTextSize(label);
	ImVec2 size(label_size.x + pad_x * 2.0f, label_size.y + pad_y * 2.0f);

	bool clicked = ImGui::InvisibleButton("##seg", size);
	bool hovered = ImGui::IsItemHovered();

	ImU32 bg = (selected || hovered) ? ImGui::GetColorU32(ImGuiCol_ButtonHovered) : ImGui::GetColorU32(ImGuiCol_Button);

	constexpr float r = 5.0f;
	ImDrawFlags flags = ImDrawFlags_RoundCornersNone;
	if (is_first && is_last)  flags = ImDrawFlags_RoundCornersAll;
	else if (is_first)        flags = ImDrawFlags_RoundCornersLeft;
	else if (is_last)         flags = ImDrawFlags_RoundCornersRight;

	ImVec2 p0 = ImGui::GetItemRectMin();
	ImVec2 p1 = ImGui::GetItemRectMax();
	ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, bg, r, flags);
	ImGui::GetWindowDrawList()->AddText(ImVec2(p0.x + pad_x, p0.y + pad_y),
		ImGui::GetColorU32(ImGuiCol_Text), label);

	ImGui::PopID();
	return clicked;
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

	// CSS: #config-wrapper fills the entire screen. #config has flex: 1 filling available width.
	const float availW = ImGui::GetContentRegionAvail().x;
	ImGui::BeginChild("settings-content", ImVec2(availW, 0));

	// CSS: #config > div { padding: 20px; padding-bottom: 0 }
	// Apply 20px left/right padding by offsetting the scroll child.
	const float pad = 20.0f;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad);
	ImGui::BeginChild("##config-scroll", ImVec2(-pad, -ImGui::GetFrameHeightWithSpacing() * 2));

	// CSS: #config.toastgap { margin-top: 20px } — extra top space when toast is visible.
	if (core::view->toast.has_value())
		ImGui::Dummy(ImVec2(0.0f, 20.0f));

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
		// CSS: input[type=number] { width: 50px } — no spin buttons (CSS hides them).
		int scroll_speed = cfg.value("scrollSpeed", 2);
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::InputScalar("##ScrollSpeed", ImGuiDataType_S32, &scroll_speed, nullptr, nullptr))
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
	// CSS: .ui-multi-button — green segmented toggle buttons (radio: only one selected at a time).
	{
		std::string path_fmt = cfg.value("pathFormat", std::string("win32"));
		const char* opts[] = { "Windows", "POSIX" };
		const char* vals[] = { "win32", "posix" };
		ImGui::PushID("##PathFormat");
		for (int i = 0; i < 2; i++) {
			if (i > 0) ImGui::SameLine(0, 1.0f);
			if (multiButtonSegment(i, opts[i], path_fmt == vals[i], i == 0, i == 1))
				cfg["pathFormat"] = vals[i];
		}
		ImGui::PopID();
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
	// CSS: #config > div .spaced { margin: 10px } — MenuButton has class="spaced" in JS.
	ImGui::Dummy(ImVec2(0.0f, 10.0f));
	{
		auto keys = available_locale_keys();
		std::string current_key = selected_locale_key();

		std::vector<menu_button::MenuOption> locale_options;
		for (const auto& key : keys)
			locale_options.push_back({ key, key });

		// JS: <div style="width: 150px"> wraps the locale MenuButton.
		ImGui::SetNextItemWidth(150.0f);
		menu_button::render("##LocaleMenuButton", locale_options,
			current_key, false, true, locale_menu_state,
			[&](const std::string& val) {
				uint32_t flag = casc::locale_flags::getFlag(val);
				cfg["cascLocale"] = flag;
			},
			nullptr);
	}
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	SectionHeading("WebP Quality");
	ImGui::TextWrapped("Quality setting for WebP exports. Range is 1-100 (100 is lossless)");
	{
		// CSS: input[type=number] { width: 50px } — no spin buttons.
		int val = cfg.value("exportWebPQuality", 75);
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::InputScalar("##WebPQuality", ImGuiDataType_S32, &val, nullptr, nullptr)) {
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
	// CSS: .ui-multi-button — each button independently toggleable (not mutually exclusive).
	{
		bool m2_meta      = cfg.value("exportM2Meta",      false);
		bool wmo_meta     = cfg.value("exportWMOMeta",     false);
		bool blp_meta     = cfg.value("exportBLPMeta",     false);
		bool foliage_meta = cfg.value("exportFoliageMeta", false);

		ImGui::PushID("##ExportMeta");
		if (multiButtonSegment(0, "M2",      m2_meta,      true,  false)) cfg["exportM2Meta"]      = !m2_meta;
		ImGui::SameLine(0, 1.0f);
		if (multiButtonSegment(1, "WMO",     wmo_meta,     false, false)) cfg["exportWMOMeta"]     = !wmo_meta;
		ImGui::SameLine(0, 1.0f);
		if (multiButtonSegment(2, "BLP",     blp_meta,     false, false)) cfg["exportBLPMeta"]     = !blp_meta;
		ImGui::SameLine(0, 1.0f);
		if (multiButtonSegment(3, "Foliage", foliage_meta, false, true))  cfg["exportFoliageMeta"] = !foliage_meta;
		ImGui::PopID();
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
	// CSS: .ui-multi-button — green segmented toggle buttons (radio: only one selected at a time).
	{
		std::string copy_mode = cfg.value("copyMode", std::string("FULL"));
		const char* opts[] = { "Full", "Directory", "FileDataID" };
		const char* vals[] = { "FULL", "DIR", "FID" };
		ImGui::PushID("##CopyMode");
		for (int i = 0; i < 3; i++) {
			if (i > 0) ImGui::SameLine(0, 1.0f);
			if (multiButtonSegment(i, opts[i], copy_mode == vals[i], i == 0, i == 2))
				cfg["copyMode"] = vals[i];
		}
		ImGui::PopID();
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

	SectionHeading("Cache Expiry");
	ImGui::TextWrapped("After how many days of inactivity is cached data deleted. Setting to zero disables cache clean-up (not recommended).");
	{
		// CSS: input[type=number] { width: 50px } — no spin buttons.
		int val = cfg.value("cacheExpiry", 168);
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::InputScalar("##CacheExpiry", ImGuiDataType_S32, &val, nullptr, nullptr))
			cfg["cacheExpiry"] = val;
	}

	SectionHeading("CDN Fallback Hosts");
	ImGui::TextWrapped("Comma-separated list of additional CDN hostnames to try when official CDN servers are unavailable or slow.");
	ImGui::TextWrapped("These are pinged alongside official servers and used based on speed and availability.");
	// CSS: input[type=text].long { width: 600px }
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##CDNFallbackHosts", cdn_fallback_state, cfg, "cdnFallbackHosts");

	SectionHeading("Manually Clear Cache (Requires Restart)");
	ImGui::TextWrapped("While housekeeping on the cache is mostly automatic, sometimes clearing manually can resolve issues.");
	// CSS: #config > div .spaced { margin: 10px } — the button has class="spaced" in JS.
	{
		bool busy = view.isBusy;
		if (busy) ImGui::BeginDisabled();
		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
		std::string btn_label = std::format("Clear Cache ({})", cache_size_formatted());
		if (ImGui::Button(btn_label.c_str()))
			handle_cache_clear();
		if (busy) ImGui::EndDisabled();
	}

	SectionHeading("Encryption Keys");
	ImGui::TextWrapped("Remote URL used to update keys for encrypted files.");
	// JS: <p>Primary <input type="text" class="long" .../></p> — label and input inline on same line.
	ImGui::Text("Primary"); ImGui::SameLine();
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##TactKeysURL", tact_url_state, cfg, "tactKeysURL");
	ImGui::Text("Fallback"); ImGui::SameLine();
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##TactKeysFallbackURL", tact_fallback_url_state, cfg, "tactKeysFallbackURL");

	SectionHeading("Add Encryption Key");
	ImGui::TextWrapped("Manually add a BLTE encryption key.");
	{
		// JS: maxlength="16" / maxlength="32"; placeholder text from JS template.
		// JS width="140" / width="280" — these are HTML attributes applied as pixel widths in NW.js.
		static char key_name_buf[17] = {};
		static char key_val_buf[33]  = {};
		ImGui::SetNextItemWidth(140.0f);
		ImGui::InputTextWithHint("##tactKeyName", "e.g 8F4098E2470FE0C8",         key_name_buf, sizeof(key_name_buf));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(280.0f);
		ImGui::InputTextWithHint("##tactKeyValue", "e.g AA718D1F1A23078D49AD0C606A72F3D5", key_val_buf,  sizeof(key_val_buf));
		ImGui::SameLine();
		if (ImGui::Button("Add##tactkey")) {
			view.userInputTactKeyName = key_name_buf;
			view.userInputTactKey     = key_val_buf;
			handle_tact_key();
			key_name_buf[0] = '\0';
			key_val_buf[0]  = '\0';
		}
	}

	SectionHeading("Realm List Source");
	ImGui::TextWrapped("Remote URL used for retrieving the realm list. (Must use same format)");
	// JS: <p><input type="text" class="long" .../></p> — input alone in paragraph (no inline label).
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##RealmListURL", realm_url_state, cfg, "realmListURL");

	SectionHeading("Character Appearance API Endpoint");
	ImGui::TextWrapped("Remote URL used for retrieving data from the Battle.net Character Appearance API. (Must use same format)");
	ImGui::SetNextItemWidth(600.0f);
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
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##ListfileBinSrc", listfile_bin_state, cfg, "listfileBinarySource");

	SectionHeading("Listfile Source (Legacy)");
	ImGui::TextWrapped("Remote URL or local path used for updating the CASC listfile. (Must use same format)");
	ImGui::Text("Primary"); ImGui::SameLine();
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##ListfileURL", listfile_url_state, cfg, "listfileURL");
	ImGui::Text("Fallback"); ImGui::SameLine();
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##ListfileFallbackURL", listfile_fb_url_state, cfg, "listfileFallbackURL");

	SectionHeading("Listfile Update Frequency");
	ImGui::TextWrapped("How often (in days) the listfile is updated. Set to zero to always re-download the listfile.");
	{
		// CSS: input[type=number] { width: 50px } — no spin buttons.
		int val = cfg.value("listfileCacheRefresh", 168);
		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::InputScalar("##ListfileCacheRefresh", ImGuiDataType_S32, &val, nullptr, nullptr))
			cfg["listfileCacheRefresh"] = val;
	}

	SectionHeading("Data Table Definition Repository");
	ImGui::TextWrapped("Remote URL used to update DBD definitions. (Must use same format)");
	ImGui::Text("Primary"); ImGui::SameLine();
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##DBDURL", dbd_url_state, cfg, "dbdURL");
	ImGui::Text("Fallback"); ImGui::SameLine();
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##DBDFallbackURL", dbd_fb_url_state, cfg, "dbdFallbackURL");

	SectionHeading("DBD Manifest Repository");
	ImGui::TextWrapped("Remote URL used to obtain DBD manifest information. (Must use same format)");
	ImGui::Text("Primary"); ImGui::SameLine();
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##DBDFilenameURL", dbdf_url_state, cfg, "dbdFilenameURL");
	ImGui::Text("Fallback"); ImGui::SameLine();
	ImGui::SetNextItemWidth(600.0f);
	configTextInput("##DBDFilenameFallbackURL", dbdf_fb_url_state, cfg, "dbdFilenameFallbackURL");

	{
		bool val = cfg.value("allowCacheCollection", false);
		SectionHeading("Allow Cache Collection");
		ImGui::TextWrapped("If enabled, wow.export.cpp will anonymously collect cache data from your client for community usage.");
		if (ImGui::Checkbox("Enable##allowCacheCollection", &val))
			cfg["allowCacheCollection"] = val;
	}

	ImGui::EndChild(); // config-scroll

	// CSS: #config-buttons { display: flex; flex-direction: row-reverse; padding: 15px 0;
	//                         border-top: 1px solid var(--border); background: var(--background); }
	ImGui::Spacing();
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImVec2 curPos = ImGui::GetCursorScreenPos();
		float lineW = ImGui::GetContentRegionAvail().x;
		dl->AddLine(ImVec2(curPos.x, curPos.y), ImVec2(curPos.x + lineW, curPos.y), ImGui::GetColorU32(ImGuiCol_Separator));
	}
	ImGui::Dummy(ImVec2(0.0f, 15.0f));

	// CSS: #config-buttons #config-reset { margin-left: 20px } — Reset sits at far left.
	// CSS: #config-buttons input { margin-right: 20px } — Apply/Discard sit at far right with 20px margin.
	bool busy = view.isBusy;
	if (busy) ImGui::BeginDisabled();

	ImGui::SetCursorPosX(20.0f);
	if (ImGui::Button("Reset to Defaults"))
		handle_reset();

	{
		// Position Apply and Discard at the right with 20px right margin.
		const float right_margin = 20.0f;
		const float spacing      = ImGui::GetStyle().ItemSpacing.x;
		const float apply_w      = ImGui::CalcTextSize("Apply").x   + ImGui::GetStyle().FramePadding.x * 2;
		const float discard_w    = ImGui::CalcTextSize("Discard").x + ImGui::GetStyle().FramePadding.x * 2;
		float buttons_start = ImGui::GetContentRegionMax().x - right_margin - discard_w - spacing - apply_w;
		ImGui::SameLine();
		ImGui::SetCursorPosX(buttons_start);
		if (ImGui::Button("Apply"))
			handle_apply();
		ImGui::SameLine();
		if (ImGui::Button("Discard"))
			handle_discard();
	}

	if (busy) ImGui::EndDisabled();

	ImGui::Dummy(ImVec2(0.0f, 15.0f));

	ImGui::EndChild(); // settings-content
}

static void handle_cache_clear() {
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
	config::save();  // Persist to disk — JS equivalent: Vue $watch triggers save() on config change.

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
