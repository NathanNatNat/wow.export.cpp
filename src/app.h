/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <filesystem>
#include <string>
#include <imgui.h>
#include <glad/gl.h>

/**
 * Application-level functions (app.cpp).
 *
 * JS equivalent: Functions defined in the root Vue app (app.js).
 */
namespace app {

/**
 * Restart the application by re-executing the current binary.
 * JS equivalent: restartApplication() which calls chrome.runtime.reload().
 */
void restartApplication();

} // namespace app

/**
 * Centralized ImGui theme mapped from data/app.css CSS variables.
 *
 * Every color constant here corresponds to a CSS custom property in :root.
 * Components should reference these instead of hardcoding IM_COL32 / ImVec4
 * color literals, so that the entire UI stays consistent with app.css.
 */
namespace app::theme {

// ── Color variables from app.css :root ───────────────────────────

// --background: #343a40
inline constexpr ImVec4 BG          = ImVec4(0.204f, 0.227f, 0.251f, 1.0f);
inline constexpr ImU32  BG_U32      = IM_COL32(52, 58, 64, 255);

// --background-trans: #343a40b3 (~70% opacity)
inline constexpr ImVec4 BG_TRANS    = ImVec4(0.204f, 0.227f, 0.251f, 0.702f);
inline constexpr ImU32  BG_TRANS_U32 = IM_COL32(52, 58, 64, 179);

// --background-dark: #2c3136
inline constexpr ImVec4 BG_DARK     = ImVec4(0.173f, 0.192f, 0.212f, 1.0f);
inline constexpr ImU32  BG_DARK_U32 = IM_COL32(44, 49, 54, 255);

// --background-alt: #3c4147
inline constexpr ImVec4 BG_ALT      = ImVec4(0.235f, 0.255f, 0.278f, 1.0f);
inline constexpr ImU32  BG_ALT_U32  = IM_COL32(60, 65, 71, 255);

// --border: #6c757d
inline constexpr ImVec4 BORDER      = ImVec4(0.424f, 0.459f, 0.490f, 1.0f);
inline constexpr ImU32  BORDER_U32  = IM_COL32(108, 117, 125, 255);

// --font-primary: #ffffffcc
inline constexpr ImVec4 FONT_PRIMARY     = ImVec4(1.0f, 1.0f, 1.0f, 0.8f);
inline constexpr ImU32  FONT_PRIMARY_U32 = IM_COL32(255, 255, 255, 204);

// --font-highlight: #ffffff
inline constexpr ImVec4 FONT_HIGHLIGHT     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
inline constexpr ImU32  FONT_HIGHLIGHT_U32 = IM_COL32(255, 255, 255, 255);

// --font-faded: #6c757d
inline constexpr ImVec4 FONT_FADED     = ImVec4(0.424f, 0.459f, 0.490f, 1.0f);
inline constexpr ImU32  FONT_FADED_U32 = IM_COL32(108, 117, 125, 255);

// --font-alt: #57afe2
inline constexpr ImVec4 FONT_ALT     = ImVec4(0.341f, 0.686f, 0.886f, 1.0f);
inline constexpr ImU32  FONT_ALT_U32 = IM_COL32(87, 175, 226, 255);

// --font-alt-highlight: #9ff1a1
inline constexpr ImVec4 FONT_ALT_HIGHLIGHT     = ImVec4(0.624f, 0.945f, 0.631f, 1.0f);
inline constexpr ImU32  FONT_ALT_HIGHLIGHT_U32 = IM_COL32(159, 241, 161, 255);

// --form-button-base: #22b549
inline constexpr ImVec4 BUTTON_BASE     = ImVec4(0.133f, 0.710f, 0.286f, 1.0f);
inline constexpr ImU32  BUTTON_BASE_U32 = IM_COL32(34, 181, 73, 255);

// --form-button-hover: #2665d2
inline constexpr ImVec4 BUTTON_HOVER     = ImVec4(0.149f, 0.396f, 0.824f, 1.0f);
inline constexpr ImU32  BUTTON_HOVER_U32 = IM_COL32(38, 101, 210, 255);

// --form-button-disabled: #696969
inline constexpr ImVec4 BUTTON_DISABLED     = ImVec4(0.412f, 0.412f, 0.412f, 1.0f);
inline constexpr ImU32  BUTTON_DISABLED_U32 = IM_COL32(105, 105, 105, 255);

// --form-button-menu: #389451
inline constexpr ImVec4 BUTTON_MENU     = ImVec4(0.220f, 0.580f, 0.318f, 1.0f);
inline constexpr ImU32  BUTTON_MENU_U32 = IM_COL32(56, 148, 81, 255);

// --form-button-menu-hover: #3e6bb9
inline constexpr ImVec4 BUTTON_MENU_HOVER     = ImVec4(0.243f, 0.420f, 0.725f, 1.0f);
inline constexpr ImU32  BUTTON_MENU_HOVER_U32 = IM_COL32(62, 107, 185, 255);

// --nav-option-selected: #22b549 (same as --form-button-base)
inline constexpr ImVec4 NAV_SELECTED     = BUTTON_BASE;
inline constexpr ImU32  NAV_SELECTED_U32 = BUTTON_BASE_U32;

// --toast-error: #dc9090
inline constexpr ImVec4 TOAST_ERROR     = ImVec4(0.863f, 0.565f, 0.565f, 1.0f);
inline constexpr ImU32  TOAST_ERROR_U32 = IM_COL32(220, 144, 144, 255);

// --toast-success: #a6dc90
inline constexpr ImVec4 TOAST_SUCCESS     = ImVec4(0.651f, 0.863f, 0.565f, 1.0f);
inline constexpr ImU32  TOAST_SUCCESS_U32 = IM_COL32(166, 220, 144, 255);

// --toast-info: #90bcdc
inline constexpr ImVec4 TOAST_INFO     = ImVec4(0.565f, 0.737f, 0.863f, 1.0f);
inline constexpr ImU32  TOAST_INFO_U32 = IM_COL32(144, 188, 220, 255);

// --toast-progress: #dcba90
inline constexpr ImVec4 TOAST_PROGRESS     = ImVec4(0.863f, 0.729f, 0.565f, 1.0f);
inline constexpr ImU32  TOAST_PROGRESS_U32 = IM_COL32(220, 186, 144, 255);

// --font-toast: black
inline constexpr ImU32 FONT_TOAST_U32 = IM_COL32(0, 0, 0, 255);

// --font-toast-link: #0300bf
inline constexpr ImU32 FONT_TOAST_LINK_U32 = IM_COL32(3, 0, 191, 255);

// --trans-check-a: #303030  (transparency checkerboard dark)
inline constexpr ImU32 TRANS_CHECK_A_U32 = IM_COL32(48, 48, 48, 255);

// --trans-check-b: #272727  (transparency checkerboard light)
inline constexpr ImU32 TRANS_CHECK_B_U32 = IM_COL32(39, 39, 39, 255);

// ── Derived / composite colors used by components ────────────────

// Row hover overlay (white, very faint)
inline constexpr ImU32 ROW_HOVER_U32     = IM_COL32(255, 255, 255, 8);

// Selected row highlight (green tint from --nav-option-selected)
inline constexpr ImU32 ROW_SELECTED_U32  = IM_COL32(34, 181, 73, 40);

// Active text (brighter white for hovered items)
inline constexpr ImU32 TEXT_ACTIVE_U32   = IM_COL32(255, 255, 255, 180);

// Inactive/idle text (dimmer white)
inline constexpr ImU32 TEXT_IDLE_U32     = IM_COL32(255, 255, 255, 80);

// Icon/indicator default (semi-transparent white)
inline constexpr ImU32 ICON_DEFAULT_U32  = IM_COL32(255, 255, 255, 128);

// Slider track background
inline constexpr ImU32 SLIDER_TRACK_U32  = IM_COL32(80, 80, 80, 255);

// Slider thumb (normal / hovered)
inline constexpr ImU32 SLIDER_THUMB_U32        = IM_COL32(200, 200, 200, 200);
inline constexpr ImU32 SLIDER_THUMB_ACTIVE_U32 = IM_COL32(255, 255, 255, 220);

// Field placeholder text (subtle white for empty input fields)
inline constexpr ImU32 FIELD_PLACEHOLDER_U32 = IM_COL32(255, 255, 255, 100);

// Dash selection line color (map-viewer)
inline constexpr ImU32 DASH_COLOR_U32    = IM_COL32(255, 255, 255, 230);

// Data-table hover row (brighter gray)
inline constexpr ImU32 TABLE_ROW_HOVER_U32    = IM_COL32(100, 100, 100, 255);
inline constexpr ImU32 TABLE_ROW_SELECTED_U32 = IM_COL32(100, 100, 100, 100);

// --progress-bar gradient: linear-gradient(180deg, #57afe2, #35759a)
inline constexpr ImU32 PROGRESS_BAR_TOP_U32    = IM_COL32(87, 175, 226, 255);  // #57afe2
inline constexpr ImU32 PROGRESS_BAR_BOTTOM_U32 = IM_COL32(53, 117, 154, 255);  // #35759a

// Loading bar background: rgba(0, 0, 0, 0.22)
inline constexpr ImU32 LOADING_BAR_BG_U32 = IM_COL32(0, 0, 0, 56);

// ── glClearColor components for --background: #343a40 ────────────
inline constexpr float BG_CLEAR_R = 0.204f;
inline constexpr float BG_CLEAR_G = 0.227f;
inline constexpr float BG_CLEAR_B = 0.251f;
inline constexpr float BG_CLEAR_A = 1.0f;

// ── Spacing / rounding / sizing from app.css ─────────────────────
inline constexpr float BUTTON_ROUNDING    = 5.0f;     // input[type=button] border-radius: 5px
inline constexpr float SCROLLBAR_SIZE     = 8.0f;     // ::-webkit-scrollbar width: 8px
inline constexpr float SCROLLBAR_ROUNDING = 5.0f;     // scrollbar-thumb border-radius: 5px
inline constexpr float WINDOW_ROUNDING    = 0.0f;     // sharp corners (no CSS border-radius on main panels)
inline constexpr float FRAME_ROUNDING     = 5.0f;     // input field border-radius
inline constexpr float POPUP_ROUNDING     = 0.0f;     // context menus have sharp corners
inline constexpr ImVec2 BUTTON_PADDING    = ImVec2(13.0f, 9.0f); // input[type=button] padding: 9px 13px

/**
 * Apply the app.css theme to the current ImGui context.
 * Sets all ImGuiStyle colors, rounding, padding, and scrollbar sizes
 * to match the CSS variables defined in data/app.css.
 *
 * Called once at startup and again from reloadStylesheet().
 */
void applyTheme();

/**
 * Push disabled-button styling: gray (#696969) background + ImGui::BeginDisabled().
 * CSS: input[type=button].disabled { background-color: var(--form-button-disabled); opacity: 0.5; }
 * Must be paired with EndDisabledButton().
 */
inline void BeginDisabledButton() {
	ImGui::PushStyleColor(ImGuiCol_Button, BUTTON_DISABLED);
	ImGui::BeginDisabled();
}

/**
 * Pop disabled-button styling pushed by BeginDisabledButton().
 */
inline void EndDisabledButton() {
	ImGui::EndDisabled();
	ImGui::PopStyleColor();
}

// ── Font size from CSS ───────────────────────────────────────────
// body { font-family: "Selawik", sans-serif; } — browser default is 16px.
inline constexpr float DEFAULT_FONT_SIZE = 16.0f;

/**
 * Load custom fonts (Selawik regular, Selawik bold, Gambler) into
 * the ImGui font atlas from data/fonts/*.ttf.
 *
 * Fonts are loaded at DEFAULT_FONT_SIZE * dpiScale so that they render
 * crisply on high-DPI displays. The caller is responsible for setting
 * ImGui::GetIO().FontGlobalScale = 1.0f / dpiScale so that logical
 * sizes remain unchanged.
 *
 * Must be called after ImGui::CreateContext() and before the first frame.
 * Sets the Selawik regular font as the default.
 *
 * @param dpiScale  The display content scale from GLFW (1.0 on standard
 *                  displays, 2.0 on Retina/200% scaling, etc.).
 */
void loadFonts(float dpiScale = 1.0f);

/**
 * Rebuild the font atlas for a new DPI scale factor.
 *
 * Clears the existing atlas, reloads all fonts at the new scale, and
 * re-creates the OpenGL font texture. Call this when the window's
 * content scale changes (e.g. moved to a monitor with different DPI).
 *
 * @param dpiScale  The new display content scale from GLFW.
 */
void rebuildFontsForScale(float dpiScale);

/**
 * Return the current DPI scale factor that fonts were loaded at.
 * This is the value most recently passed to loadFonts() or
 * rebuildFontsForScale().
 */
float getDpiScale();

/**
 * Return the bold font (Selawik Bold). Falls back to the default font
 * if the bold font failed to load.
 */
ImFont* getBoldFont();

/**
 * Return the Gambler font. Falls back to the default font
 * if the Gambler font failed to load.
 */
ImFont* getGamblerFont();

/**
 * Return the Font Awesome icon font. Falls back to the default font
 * if the icon font failed to load.
 */
ImFont* getIconFont();

// ── Font Awesome 6 Free Solid icon codepoints ───────────────────
// Glyph range for merging into the default font (covers all FA6 Solid glyphs).
inline constexpr ImWchar ICON_FA_MIN = 0xE005;
inline constexpr ImWchar ICON_FA_MAX = 0xF8FF;

// Individual icon codepoints used in the app, as UTF-8 string literals.
// clang-format off
#define ICON_FA_ARROW_LEFT           "\xef\x81\xa0"  // U+F060
#define ICON_FA_ARROW_RIGHT          "\xef\x81\xa1"  // U+F061
#define ICON_FA_ARROW_ROTATE_LEFT    "\xef\x83\xa2"  // U+F0E2
#define ICON_FA_BAN                  "\xef\x81\x9e"  // U+F05E
#define ICON_FA_BARS                 "\xef\x83\x89"  // U+F0C9 (line-columns / hamburger)
#define ICON_FA_BUG                  "\xef\x86\x88"  // U+F188
#define ICON_FA_CARET_DOWN           "\xef\x83\x97"  // U+F0D7
#define ICON_FA_CHECK                "\xef\x80\x8c"  // U+F00C
#define ICON_FA_CIRCLE_INFO          "\xef\x81\x9a"  // U+F05A
#define ICON_FA_CIRCLE_QUESTION      "\xef\x81\x99"  // U+F059 (help)
#define ICON_FA_CLIPBOARD_LIST       "\xef\x91\xad"  // U+F46D
#define ICON_FA_COPY                 "\xef\x83\x85"  // U+F0C5
#define ICON_FA_CUBE                 "\xef\x86\xb2"  // U+F1B2
#define ICON_FA_DATABASE             "\xef\x87\x80"  // U+F1C0
#define ICON_FA_FILE_EXPORT          "\xef\x95\xae"  // U+F56E (export)
#define ICON_FA_FILE_IMPORT          "\xef\x95\xaf"  // U+F56F (import)
#define ICON_FA_FILE_LINES           "\xef\x85\x9c"  // U+F15C
#define ICON_FA_FILM                 "\xef\x80\x88"  // U+F008
#define ICON_FA_FISH                 "\xef\x95\xb8"  // U+F578
#define ICON_FA_FLOPPY_DISK          "\xef\x83\x87"  // U+F0C7 (save)
#define ICON_FA_FONT                 "\xef\x80\xb1"  // U+F031
#define ICON_FA_GEAR                 "\xef\x80\x93"  // U+F013
#define ICON_FA_HOUSE                "\xef\x80\x95"  // U+F015
#define ICON_FA_IMAGE                "\xef\x80\xbe"  // U+F03E
#define ICON_FA_LIST                 "\xef\x80\xba"  // U+F03A
#define ICON_FA_MAGNIFYING_GLASS     "\xef\x80\x82"  // U+F002 (search)
#define ICON_FA_MAP                  "\xef\x89\xb9"  // U+F279
#define ICON_FA_MUSIC                "\xef\x80\x81"  // U+F001
#define ICON_FA_PALETTE              "\xef\x94\xbf"  // U+F53F
#define ICON_FA_PAUSE                "\xef\x81\x8c"  // U+F04C
#define ICON_FA_PERSON               "\xef\x86\x83"  // U+F183 (person-solid)
#define ICON_FA_PLAY                 "\xef\x81\x8b"  // U+F04B
#define ICON_FA_SORT                 "\xef\x83\x9c"  // U+F0DC
#define ICON_FA_SORT_DOWN            "\xef\x83\x9d"  // U+F0DD
#define ICON_FA_SORT_UP              "\xef\x83\x9e"  // U+F0DE
#define ICON_FA_STOPWATCH            "\xef\x8b\xb2"  // U+F2F2 (timer)
#define ICON_FA_TIMELINE             "\xee\x8a\x9c"  // U+E29C
#define ICON_FA_TRASH                "\xef\x87\xb8"  // U+F1F8
#define ICON_FA_TRIANGLE_EXCLAMATION "\xef\x81\xb1"  // U+F071
#define ICON_FA_VOLUME_HIGH          "\xef\x80\xa8"  // U+F028
#define ICON_FA_XMARK                "\xef\x80\x8d"  // U+F00D
// clang-format on

/**
 * Look up the Font Awesome icon codepoint (UTF-8 string) for a given
 * SVG icon filename (e.g. "gear.svg" → ICON_FA_GEAR).
 * Returns nullptr if the icon has no FA mapping (custom icon).
 */
const char* getIconForFilename(const std::string& svg_filename);

/**
 * Load an SVG file and rasterize it into an OpenGL texture at the given size.
 * Returns the GL texture ID (0 on failure).
 */
GLuint loadSvgTexture(const std::filesystem::path& path, int size);

/**
 * Load a PNG/JPEG image from disk into an OpenGL texture.
 * Returns the GL texture ID (0 on failure).
 */
GLuint loadImageTexture(const std::filesystem::path& path, int* out_w = nullptr, int* out_h = nullptr);

} // namespace app::theme

/**
 * Shared tab layout helpers — CSS grid-like layout for ImGui.
 *
 * Maps the standard layout patterns from app.css (.tab, .tab.list-tab,
 * .sidebar, .list-container, .preview-container, .filter, .preview-controls)
 * into ImGui Begin/End child-window helpers.
 *
 * Usage:
 *   if (app::layout::BeginTab("my-tab")) {
 *       auto regions = app::layout::CalcListTabRegions();
 *       if (app::layout::BeginListContainer("list", regions)) {
 *           // render list...
 *           app::layout::EndListContainer();
 *       }
 *       if (app::layout::BeginPreviewContainer("preview", regions)) {
 *           // render preview...
 *           app::layout::EndPreviewContainer();
 *       }
 *       if (app::layout::BeginFilterBar("filter", regions)) {
 *           // render filter input + buttons...
 *           app::layout::EndFilterBar();
 *       }
 *       if (app::layout::BeginPreviewControls("controls", regions)) {
 *           // render export buttons...
 *           app::layout::EndPreviewControls();
 *       }
 *   }
 *   app::layout::EndTab();
 */
namespace app::layout {

// ── Constants matching app.css layout values ─────────────────────

// .tab.list-tab { grid-template-rows: 1fr 60px }
inline constexpr float FILTER_BAR_HEIGHT  = 60.0f;

// .sidebar { width: 210px }
inline constexpr float SIDEBAR_WIDTH      = 210.0f;

// .list-container { margin: 20px 10px 0 20px }
inline constexpr float LIST_MARGIN_TOP    = 20.0f;
inline constexpr float LIST_MARGIN_RIGHT  = 10.0f;
inline constexpr float LIST_MARGIN_BOTTOM = 0.0f;
inline constexpr float LIST_MARGIN_LEFT   = 20.0f;

// .preview-container { margin: 20px 20px 0 10px }
inline constexpr float PREVIEW_MARGIN_TOP    = 20.0f;
inline constexpr float PREVIEW_MARGIN_RIGHT  = 20.0f;
inline constexpr float PREVIEW_MARGIN_BOTTOM = 0.0f;
inline constexpr float PREVIEW_MARGIN_LEFT   = 10.0f;

// .preview-controls { margin-right: 20px }
inline constexpr float CONTROLS_MARGIN_RIGHT = 20.0f;

// .sidebar { margin-top: 20px; padding-right: 20px }
inline constexpr float SIDEBAR_MARGIN_TOP    = 20.0f;
inline constexpr float SIDEBAR_PADDING_RIGHT = 20.0f;

/**
 * Calculated regions for a list-tab grid layout.
 * Populated by CalcListTabRegions() and consumed by the Begin* helpers.
 */
struct ListTabRegions {
	// Left column: list area (row 1)
	ImVec2 listPos;
	ImVec2 listSize;

	// Right column: preview area (row 1)
	ImVec2 previewPos;
	ImVec2 previewSize;

	// Bottom-left: filter bar (row 2, column 1)
	ImVec2 filterPos;
	ImVec2 filterSize;

	// Bottom-right: preview controls (row 2, column 2)
	ImVec2 controlsPos;
	ImVec2 controlsSize;

	// Optional sidebar (column 3, spanning both rows)
	bool hasSidebar = false;
	ImVec2 sidebarPos;
	ImVec2 sidebarSize;
};

/**
 * Calculate grid regions for a list-tab layout.
 *
 * Divides the current content region into a 2-column grid matching
 * `.tab.list-tab { grid-template-columns: 1fr 1fr; grid-template-rows: 1fr 60px }`.
 * When hasSidebar is true, a 210px third column is reserved on the right.
 *
 * @param hasSidebar  Reserve a 210px sidebar column on the right.
 * @param colRatio    Ratio of left column width to total (default 0.5 = 1fr 1fr).
 * @return Calculated regions (positions relative to the current window).
 */
ListTabRegions CalcListTabRegions(bool hasSidebar = false, float colRatio = 0.5f);

/**
 * Begin a full-area tab child window (`.tab` equivalent).
 *
 * Creates a borderless, no-scroll child window that fills the entire
 * content region, matching `.tab { position: absolute; top/left/right/bottom: 0 }`.
 *
 * Must be paired with EndTab().
 * @return true if the child window is visible (same as ImGui::BeginChild).
 */
bool BeginTab(const char* id);

/**
 * End the full-area tab child window.
 */
void EndTab();

/**
 * Begin the list container region (`.list-container` equivalent).
 *
 * Creates a child window at the calculated list position with
 * margins: 20px top, 10px right, 0 bottom, 20px left.
 *
 * Must be paired with EndListContainer().
 */
bool BeginListContainer(const char* id, const ListTabRegions& regions);
void EndListContainer();

/**
 * Begin the preview container region (`.preview-container` equivalent).
 *
 * Creates a child window at the calculated preview position with
 * margins: 20px top, 20px right, 0 bottom, 10px left.
 *
 * The CSS original uses `display: flex; align-items: center; justify-content: center`
 * to center content. In ImGui's immediate mode, the caller is responsible for
 * centering content within this region (e.g. using SetCursorPos with size math).
 *
 * Must be paired with EndPreviewContainer().
 */
bool BeginPreviewContainer(const char* id, const ListTabRegions& regions);
void EndPreviewContainer();

/**
 * Begin the filter bar region (`.filter` equivalent).
 *
 * Creates a 60px-tall child window at the bottom of the left column.
 * Items are vertically centered within the bar (flex align-items: center).
 *
 * Must be paired with EndFilterBar().
 */
bool BeginFilterBar(const char* id, const ListTabRegions& regions);
void EndFilterBar();

/**
 * Begin the preview controls region (`.preview-controls` equivalent).
 *
 * Creates a 60px-tall child window at the bottom of the right column.
 * Content is vertically centered within the bar.
 *
 * The CSS original uses `justify-content: flex-end` to right-align buttons.
 * In ImGui's immediate mode, the caller is responsible for right-aligning
 * widgets (e.g. `ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - totalW)`).
 *
 * Must be paired with EndPreviewControls().
 */
bool BeginPreviewControls(const char* id, const ListTabRegions& regions);
void EndPreviewControls();

/**
 * Begin the sidebar region (`.sidebar` equivalent).
 *
 * Creates a 210px-wide child window on the right edge spanning both rows.
 * margin-top: 20px, padding-right: 20px.
 *
 * Must be paired with EndSidebar().
 */
bool BeginSidebar(const char* id, const ListTabRegions& regions);
void EndSidebar();

} // namespace app::layout
