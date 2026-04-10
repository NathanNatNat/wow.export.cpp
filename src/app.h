/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <imgui.h>

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

// Field border (subtle white outline for combobox/file-field)
inline constexpr ImU32 FIELD_BORDER_U32  = IM_COL32(255, 255, 255, 100);

// Dash selection line color (map-viewer)
inline constexpr ImU32 DASH_COLOR_U32    = IM_COL32(255, 255, 255, 230);

// Data-table hover row (brighter gray)
inline constexpr ImU32 TABLE_ROW_HOVER_U32    = IM_COL32(100, 100, 100, 255);
inline constexpr ImU32 TABLE_ROW_SELECTED_U32 = IM_COL32(100, 100, 100, 100);

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

} // namespace app::theme
