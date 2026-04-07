/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>
License: MIT
 */

#include "tab_zones.h"
#include "../log.h"
#include "../core.h"
#include "../generics.h"
#include "../buffer.h"
#include "../casc/export-helper.h"
#include "../casc/listfile.h"
#include "../casc/casc-source.h"
#include "../casc/blte-reader.h"
#include "../casc/blp.h"
#include "../casc/db2.h"
#include "../ui/texture-exporter.h"
#include "../install-type.h"

#include <cstring>
#include <format>
#include <filesystem>
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <set>
#include <regex>
#include <cmath>

#include <imgui.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

namespace tab_zones {

// --- File-local structures ---

// JS: parse_zone_entry result: { id, zone_name, area_name }
// The zone entry format is: expansion_id \x19 [zone_id] \x19 area_name \x19 (zone_name)
struct ZoneDisplayInfo {
int id = 0;
std::string zone_name;
std::string area_name;
int expansion = -1;
};

// Art style layer combining UiMapArt + UiMapArtStyleLayer
struct CombinedArtStyle {
int id = 0;
int ui_map_art_style_id = 0;
int layer_index = 0;
int layer_width = 0;
int layer_height = 0;
int tile_width = 0;
int tile_height = 0;
};

// --- File-local state ---

// JS: const EXPANSION_NAMES = [...];
static const std::vector<std::string> EXPANSION_NAMES = {
	"Classic",
	"The Burning Crusade",
	"Wrath of the Lich King",
	"Cataclysm",
	"Mists of Pandaria",
	"Warlords of Draenor",
	"Legion",
	"Battle for Azeroth",
	"Shadowlands",
	"Dragonflight",
	"The War Within"
};

// JS: let selected_zone_id = null;
static std::optional<int> selected_zone_id;

// JS: let selected_phase_id = null;
static std::optional<int> selected_phase_id;

// Change-detection.
static std::string prev_selection_first;
static bool prev_show_zone_base_map = true;
static bool prev_show_zone_overlays = true;

// --- Internal functions ---

// JS: const parse_zone_entry = (entry) => { ... }
static ZoneDisplayInfo parse_zone_entry(const std::string& entry) {
// Format: expansion_id \x19 [zone_id] \x19 area_name \x19 (zone_name)
std::regex re(R"((\d+)\x19\[(\d+)\]\x19([^\x19]+)\x19\(([^)]+)\))");
std::smatch match;
if (!std::regex_match(entry, match, re))
return {};

ZoneDisplayInfo info;
info.expansion = std::stoi(match[1].str());
info.id = std::stoi(match[2].str());
info.area_name = match[3].str();
info.zone_name = match[4].str();
return info;
}

// JS: const get_zone_ui_map_id = async (zone_id) => { ... }
static std::optional<int> get_zone_ui_map_id(int zone_id) {
// JS: for (const assignment of (await db2.UiMapAssignment.getAllRows()).values()) {
//     if (assignment.AreaID === zone_id) return assignment.UiMapID; }
auto& ui_map_assignment = casc::db2::getTable("UiMapAssignment");
// TODO(conversion): WDCReader getAllRows iteration will be wired when DB2 system is fully integrated.
// Placeholder: When wired, iterate rows and check AreaID == zone_id.
(void)ui_map_assignment;
return std::nullopt;
}

// JS: const get_zone_phases = async (zone_id) => { ... }
static std::vector<ZonePhase> get_zone_phases(int zone_id) {
const auto ui_map_id = get_zone_ui_map_id(zone_id);
if (!ui_map_id.has_value())
return {};

std::vector<ZonePhase> phases;
std::set<int> seen_phases;

// JS: for (const link_entry of (await db2.UiMapXMapArt.getAllRows()).values()) { ... }
auto& ui_map_x_map_art = casc::db2::getTable("UiMapXMapArt");
// TODO(conversion): WDCReader getAllRows iteration will be wired when DB2 system is fully integrated.
// Placeholder: When wired, iterate UiMapXMapArt rows for matching UiMapID.
(void)ui_map_x_map_art;

std::sort(phases.begin(), phases.end(), [](const ZonePhase& a, const ZonePhase& b) {
return a.id < b.id;
});

return phases;
}

// JS: const render_zone_to_canvas = async (canvas, zone_id, phase_id, set_canvas_size, skip_zone_check) => { ... }
static ZoneMapInfo render_zone_to_canvas(int zone_id, std::optional<int> phase_id = std::nullopt, bool skip_zone_check = false) {
const auto ui_map_id = get_zone_ui_map_id(zone_id);
if (!ui_map_id.has_value()) {
logging::write(std::format("no UiMap found for zone ID {}", zone_id));
throw std::runtime_error("no map data available for this zone");
}

// JS: const map_data = await db2.UiMap.getRow(ui_map_id);
auto& ui_map = casc::db2::getTable("UiMap");
(void)ui_map;
// TODO(conversion): db2 getRow will be wired when DB2 system is fully integrated.

// JS: Collect linked art IDs from UiMapXMapArt
std::vector<int> linked_art_ids;
auto& ui_map_x_map_art = casc::db2::getTable("UiMapXMapArt");
(void)ui_map_x_map_art;
// TODO(conversion): Iterate UiMapXMapArt rows to collect art IDs matching ui_map_id and phase_id.

// JS: Collect combined art styles from UiMapArt + UiMapArtStyleLayer
std::vector<CombinedArtStyle> art_styles;
auto& ui_map_art = casc::db2::getTable("UiMapArt");
auto& ui_map_art_style_layer = casc::db2::getTable("UiMapArtStyleLayer");
(void)ui_map_art;
(void)ui_map_art_style_layer;
// TODO(conversion): For each linked_art_id, look up UiMapArt row, then find matching UiMapArtStyleLayer.

if (art_styles.empty()) {
logging::write(std::format("no art styles found for UiMap ID {} (phase {})",
*ui_map_id, phase_id.value_or(-1)));
throw std::runtime_error("no art styles found for map");
}

logging::write(std::format("found {} art styles for UiMap ID {} (phase {})",
art_styles.size(), *ui_map_id, phase_id.value_or(-1)));

// Sort by LayerIndex.
std::sort(art_styles.begin(), art_styles.end(), [](const CombinedArtStyle& a, const CombinedArtStyle& b) {
return a.layer_index < b.layer_index;
});

int map_width = 0, map_height = 0;

for (const auto& art_style : art_styles) {
// JS: const all_tiles = await db2.UiMapArtTile.getRelationRows(art_style.ID);
auto& ui_map_art_tile = casc::db2::getTable("UiMapArtTile");
(void)ui_map_art_tile;
// TODO(conversion): Get tiles by relation, group by layer, render each tile from CASC.

if (art_style.layer_index == 0) {
map_width = art_style.layer_width;
map_height = art_style.layer_height;
}

// JS: render base map tiles if config.showZoneBaseMap
if (core::view->config.value("showZoneBaseMap", true)) {
// TODO(conversion): render_map_tiles equivalent — load BLP tiles from CASC, composite onto texture.
}

// JS: render overlays if config.showZoneOverlays
if (core::view->config.value("showZoneOverlays", true)) {
// TODO(conversion): render_world_map_overlays equivalent — load WorldMapOverlay + tiles.
}
}

logging::write(std::format("successfully rendered zone map for zone ID {} (UiMap ID {})",
zone_id, *ui_map_id));

return { map_width, map_height, *ui_map_id };
}

// JS: const render_map_tiles = async (ctx, tiles, art_style, layer_index, expected_zone_id, skip_zone_check) => { ... }
// TODO(conversion): render_map_tiles will be implemented when GL texture compositing is available.
// It sorts tiles by RowIndex/ColIndex, computes pixel positions, loads BLP from CASC, and draws to canvas.

// JS: const render_world_map_overlays = async (ctx, art_style, expected_zone_id, skip_zone_check) => { ... }
// TODO(conversion): render_world_map_overlays will be implemented when GL texture compositing is available.
// It queries WorldMapOverlay + WorldMapOverlayTile DB2 tables and renders overlay tiles at offset positions.

// JS: const render_overlay_tiles = async (ctx, tiles, overlay, art_style, expected_zone_id, skip_zone_check) => { ... }
// TODO(conversion): render_overlay_tiles will be implemented when GL texture compositing is available.

// JS: const load_zone_map = async (zone_id, phase_id) => { ... }
static void load_zone_map(int zone_id, std::optional<int> phase_id = std::nullopt) {
try {
const auto result = render_zone_to_canvas(zone_id, phase_id);
// JS: canvas rendering is done in render_zone_to_canvas.
// TODO(conversion): The resulting composite texture will be stored for ImGui::Image display.
(void)result;
} catch (const std::exception& e) {
logging::write(std::format("failed to render zone map: {}", e.what()));
core::setToast("error", std::format("Failed to load map data: {}", e.what()));
}
}

// --- Public API ---

void registerTab() {
// JS: this.registerNavButton('Zones', 'mountain-castle.svg', InstallType.CASC);
// TODO(conversion): Nav button registration will be wired when the module system is integrated.
}

void mounted() {
auto& view = *core::view;

// JS: await this.initialize();
core::showLoadingScreen(3);

core::progressLoadingScreen("Loading map tiles...");
casc::db2::preloadTable("UiMapArtTile");

core::progressLoadingScreen("Loading map overlays...");
casc::db2::preloadTable("WorldMapOverlay");
casc::db2::preloadTable("WorldMapOverlayTile");

core::progressLoadingScreen("Loading zone data...");

// JS: const expansion_map = new Map();
// JS: for (const [id, entry] of await db2.Map.getAllRows())
//         expansion_map.set(id, entry.ExpansionID);
std::unordered_map<int, int> expansion_map;
auto& map_table = casc::db2::getTable("Map");
// TODO(conversion): WDCReader getAllRows iteration will be wired when DB2 system is fully integrated.
(void)map_table;

logging::write(std::format("loaded {} maps for expansion mapping", expansion_map.size()));

// JS: const available_zones = new Set();
// JS: for (const entry of (await db2.UiMapAssignment.getAllRows()).values())
//         available_zones.add(entry.AreaID);
std::set<int> available_zones;
auto& ui_map_assignment = casc::db2::getTable("UiMapAssignment");
// TODO(conversion): WDCReader getAllRows iteration will be wired when DB2 system is fully integrated.
(void)ui_map_assignment;

logging::write(std::format("loaded {} zones from UiMapAssignment", available_zones.size()));

// JS: const table = db2.AreaTable;
// JS: for (const [id, entry] of await table.getAllRows()) { ... }
auto& area_table = casc::db2::getTable("AreaTable");
(void)area_table;

std::vector<std::string> zone_entries;
// TODO(conversion): When wired, iterate AreaTable rows:
// for (const auto& [id, row] : area_table.getAllRows()) {
//     if (!available_zones.contains(id)) continue;
//     int expansion_id = expansion_map.count(row.ContinentID) ? expansion_map[row.ContinentID] : 0;
//     zone_entries.push_back(std::format("{}\x19[{}]\x19{}\x19({})",
//         expansion_id, id, row.AreaName_lang, row.ZoneName));
// }

view.zoneViewerZones.clear();
for (const auto& entry : zone_entries)
view.zoneViewerZones.push_back(entry);

logging::write(std::format("loaded {} zones from AreaTable", zone_entries.size()));

core::hideLoadingScreen();

// Store initial config values for change-detection.
prev_show_zone_base_map = view.config.value("showZoneBaseMap", true);
prev_show_zone_overlays = view.config.value("showZoneOverlays", true);
}

void render() {
auto& view = *core::view;

// --- Change-detection for selection (equivalent to watch on selectionZones) ---
if (!view.selectionZones.empty()) {
const std::string first = view.selectionZones[0].get<std::string>();
if (view.isBusy == 0 && !first.empty() && first != prev_selection_first) {
const auto zone = parse_zone_entry(first);
if (zone.id > 0 && (!selected_zone_id.has_value() || *selected_zone_id != zone.id)) {
selected_zone_id = zone.id;
logging::write(std::format("selected zone: {} ({})", zone.zone_name, zone.id));

const auto phases = get_zone_phases(zone.id);

view.zonePhases.clear();
for (const auto& phase : phases) {
nlohmann::json p;
p["id"] = phase.id;
p["label"] = phase.label;
view.zonePhases.push_back(std::move(p));
}

if (!phases.empty()) {
selected_phase_id = phases[0].id;
view.zonePhaseSelection = phases[0].id;
} else {
selected_phase_id = std::nullopt;
view.zonePhaseSelection = nullptr;
}

load_zone_map(zone.id, selected_phase_id);
}

prev_selection_first = first;
}
}

// --- Change-detection for zonePhaseSelection ---
if (!view.zonePhaseSelection.is_null() && view.zonePhaseSelection.is_number()) {
int new_phase = view.zonePhaseSelection.get<int>();
if (selected_zone_id.has_value() && view.isBusy == 0 &&
(!selected_phase_id.has_value() || *selected_phase_id != new_phase)) {
selected_phase_id = new_phase;
logging::write(std::format("zone phase changed to {}, reloading zone {}",
new_phase, *selected_zone_id));
load_zone_map(*selected_zone_id, selected_phase_id);
}
}

// --- Change-detection for config.showZoneBaseMap ---
const bool current_show_base_map = view.config.value("showZoneBaseMap", true);
if (current_show_base_map != prev_show_zone_base_map) {
if (selected_zone_id.has_value() && view.isBusy == 0) {
logging::write(std::format("zone base map setting changed, reloading zone {}",
*selected_zone_id));
load_zone_map(*selected_zone_id, selected_phase_id);
}
prev_show_zone_base_map = current_show_base_map;
}

// --- Change-detection for config.showZoneOverlays ---
const bool current_show_overlays = view.config.value("showZoneOverlays", true);
if (current_show_overlays != prev_show_zone_overlays) {
if (selected_zone_id.has_value() && view.isBusy == 0) {
logging::write(std::format("zone overlay setting changed, reloading zone {}",
*selected_zone_id));
load_zone_map(*selected_zone_id, selected_phase_id);
}
prev_show_zone_overlays = current_show_overlays;
}

// --- Template rendering ---

// Expansion filter buttons.
// JS: <div class="expansion-buttons">
//     <button class="expansion-button show-all" ...>
//     <button v-for="expansion in $core.view.constants.EXPANSIONS" ...>
if (ImGui::SmallButton("All"))
view.selectedZoneExpansionFilter = -1;
ImGui::SameLine();

// TODO(conversion): Expansion icons are background-image CSS vars in JS; here we use text buttons.
// When icon font or texture icons are available, replace with icon buttons.
for (int i = 0; i < static_cast<int>(EXPANSION_NAMES.size()); i++) {
const bool selected = (view.selectedZoneExpansionFilter == i);
if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
if (ImGui::SmallButton(std::format("E{}", i).c_str()))
view.selectedZoneExpansionFilter = i;
if (selected) ImGui::PopStyleColor();
ImGui::SameLine();
}
ImGui::NewLine();

// Zone listbox.
// JS: <component :is="$components.ListboxZones" ... >
ImGui::BeginChild("zones-list-container", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, -ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Borders);
// TODO(conversion): ListboxZones component rendering will be wired when integration is complete.
ImGui::Text("Zones: %zu", view.zoneViewerZones.size());
ImGui::EndChild();

// Context menu.
// JS: <ContextMenu :node="contextMenus.nodeZone" ...>
//   copy_zone_names, copy_area_names, copy_zone_ids, copy_zone_export_path, open_zone_export_directory
// TODO(conversion): ContextMenu component rendering will be wired when integration is complete.

// Filter.
// JS: <div class="filter">
if (view.config.value("regexFilters", false))
ImGui::TextUnformatted("Regex Enabled");

char filter_buf[256] = {};
std::strncpy(filter_buf, view.userInputFilterZones.c_str(), sizeof(filter_buf) - 1);
if (ImGui::InputText("##FilterZones", filter_buf, sizeof(filter_buf)))
view.userInputFilterZones = filter_buf;

ImGui::SameLine();

// Right panel: export controls + zone preview.
ImGui::BeginGroup();

// Export controls.
// JS: <div class="zone-export-controls">
bool show_base_map = view.config.value("showZoneBaseMap", true);
if (ImGui::Checkbox("Show Base Map", &show_base_map))
view.config["showZoneBaseMap"] = show_base_map;

ImGui::SameLine();

bool show_overlays = view.config.value("showZoneOverlays", true);
if (ImGui::Checkbox("Show Overlays", &show_overlays))
view.config["showZoneOverlays"] = show_overlays;

ImGui::SameLine();

const bool busy = view.isBusy > 0;
const bool no_selection = view.selectionZones.empty();
if (busy || no_selection) ImGui::BeginDisabled();
if (ImGui::Button("Export Map"))
export_zone_map();
if (busy || no_selection) ImGui::EndDisabled();

// Phase dropdown.
// JS: <select v-model="$core.view.zonePhaseSelection">
if (view.zonePhases.size() > 1) {
std::string current_label = "Default";
if (view.zonePhaseSelection.is_number()) {
int current_id = view.zonePhaseSelection.get<int>();
for (const auto& phase : view.zonePhases) {
if (phase.value("id", -99) == current_id) {
current_label = phase.value("label", std::string("Unknown"));
break;
}
}
}

if (ImGui::BeginCombo("Phase", current_label.c_str())) {
for (const auto& phase : view.zonePhases) {
const std::string label = phase.value("label", std::string("Unknown"));
const int phase_id = phase.value("id", -99);
const bool is_selected = view.zonePhaseSelection.is_number() &&
view.zonePhaseSelection.get<int>() == phase_id;
if (ImGui::Selectable(label.c_str(), is_selected))
view.zonePhaseSelection = phase_id;
if (is_selected)
ImGui::SetItemDefaultFocus();
}
ImGui::EndCombo();
}
}

// Zone map preview (canvas).
// JS: <div class="zone-viewer-container preview-container">
//     <canvas id="zone-canvas"></canvas>
ImGui::BeginChild("zone-canvas-area", ImVec2(0, 0), ImGuiChildFlags_Borders);
// TODO(conversion): Zone map preview will be rendered as an ImGui::Image when GL texture compositing is integrated.
// The JS version renders BLP tiles onto an HTML canvas; the C++ version will composite onto an offscreen FBO.
ImGui::TextUnformatted("Select a zone to preview its map");
ImGui::EndChild();

ImGui::EndGroup();
}

// --- Context menu methods ---

// JS: methods.handle_zone_context(data)
// TODO(conversion): Context menu handling will be wired when ContextMenu component is integrated.
// The methods below implement the clipboard operations identically to JS.

// JS: methods.copy_zone_names(selection)
static void copy_zone_names(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
const auto zone = parse_zone_entry(entry);
if (zone.id > 0) {
if (!result.empty())
result += '\n';
result += zone.zone_name;
}
}
ImGui::SetClipboardText(result.c_str());
}

// JS: methods.copy_area_names(selection)
static void copy_area_names(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
const auto zone = parse_zone_entry(entry);
if (zone.id > 0) {
if (!result.empty())
result += '\n';
result += zone.area_name;
}
}
ImGui::SetClipboardText(result.c_str());
}

// JS: methods.copy_zone_ids(selection)
static void copy_zone_ids(const std::vector<std::string>& selection) {
std::string result;
for (const auto& entry : selection) {
const auto zone = parse_zone_entry(entry);
if (zone.id > 0) {
if (!result.empty())
result += '\n';
result += std::to_string(zone.id);
}
}
ImGui::SetClipboardText(result.c_str());
}

// JS: methods.copy_zone_export_path()
static void copy_zone_export_path() {
const std::string dir = casc::ExportHelper::getExportPath("zones");
ImGui::SetClipboardText(dir.c_str());
}

// JS: methods.open_zone_export_directory()
static void open_zone_export_directory() {
	const std::string dir = casc::ExportHelper::getExportPath("zones");
#ifdef _WIN32
	const std::wstring wpath(dir.begin(), dir.end());
	ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
	std::string cmd = "xdg-open \"" + dir + "\" &";
	std::system(cmd.c_str());
#endif
}

void export_zone_map() {
auto& view = *core::view;
const auto& user_selection = view.selectionZones;
if (user_selection.empty()) {
core::setToast("info", "You didn't select any zones to export; you should do that first.");
return;
}

casc::ExportHelper helper(static_cast<int>(user_selection.size()), "zone");
helper.start();

const std::string format = view.config.value("exportTextureFormat", std::string("PNG"));
const std::string ext = format == "WEBP" ? ".webp" : ".png";

for (const auto& zone_entry : user_selection) {
if (helper.isCancelled())
return;

try {
const auto zone = parse_zone_entry(zone_entry.get<std::string>());

logging::write(std::format("exporting zone map: {} ({}) phase {}",
zone.zone_name, zone.id, selected_phase_id.value_or(-1)));

const auto map_info = render_zone_to_canvas(zone.id, selected_phase_id, true);

if (map_info.width == 0 || map_info.height == 0) {
logging::write(std::format("no map data available for zone {}, skipping", zone.id));
helper.mark(std::format("Zone_{}", zone.id), false, "No map data available");
continue;
}

// JS: Sanitize names for filenames.
auto sanitize = [](const std::string& s) -> std::string {
std::string result;
for (char c : s) {
if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == ' ')
result += c;
}
// Replace whitespace runs with underscore.
std::string final_result;
bool last_was_space = false;
for (char c : result) {
if (c == ' ') {
if (!last_was_space)
final_result += '_';
last_was_space = true;
} else {
final_result += c;
last_was_space = false;
}
}
return final_result;
};

const std::string normalized_zone_name = sanitize(zone.zone_name);
const std::string normalized_area_name = sanitize(zone.area_name);
const std::string phase_suffix = (selected_phase_id.has_value() && *selected_phase_id != 0)
? std::format("_Phase{}", *selected_phase_id) : "";

namespace fs = std::filesystem;
const std::string filename = std::format("Zone_{}_{}_{}{}{}",
zone.id, normalized_zone_name, normalized_area_name, phase_suffix, ext);
const std::string export_path = casc::ExportHelper::getExportPath(
(fs::path("zones") / filename).string());

logging::write(std::format("exporting zone map at full resolution ({}x{}): {}",
map_info.width, map_info.height, filename));

// JS: const buf = await BufferWrapper.fromCanvas(export_canvas, mime_type, quality);
// JS: await buf.writeToFile(export_path);
// TODO(conversion): BufferWrapper fromCanvas equivalent (PNG/WebP encoding) will be wired.

helper.mark((fs::path("zones") / filename).string(), true);

logging::write(std::format("successfully exported zone map to: {}", export_path));
} catch (const std::exception& e) {
logging::write(std::format("failed to export zone map: {}", e.what()));
helper.mark(zone_entry.get<std::string>(), false, e.what());
}
}

helper.finish();
}


} // namespace tab_zones
