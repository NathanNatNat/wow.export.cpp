# UI Reference Screenshots

Reference screenshots from the original JavaScript/NW.js wow.export application.
These were captured at **full screen on Windows on a 4K monitor** (3840×2076 — not full 4K due to the Windows taskbar).

When making UI changes to the C++ ImGui conversion, **always compare against these screenshots** to ensure visual fidelity with the original app.

---

## Style Reference

The original app's styling was defined in `src/app.css` (now removed — fully audited and ported). The C++ equivalents live in `src/app.cpp` and `src/app.h`. Key values for quick reference:

| CSS Variable | Value | C++ Constant |
|---|---|---|
| `--background` | `#343a40` | `COLOR_BG` |
| `--background-dark` | `#2c3136` | `COLOR_BG_DARK` |
| `--background-alt` | `#3c4147` | `COLOR_BG_ALT` |
| `--background-trans` | `#343a40b3` | `COLOR_BG_TRANS` |
| `--border` | `#6c757d` | `COLOR_BORDER` |
| `--nav-option-selected` | `#22b549` | `COLOR_NAV_ACTIVE` |
| `--font-primary` | `#ffffffcc` | `FONT_PRIMARY` |
| `--font-highlight` | `#ffffff` | `FONT_HIGHLIGHT` |
| `--font-faded` | `#6c757d` | `FONT_FADED` |
| `--font-alt` | `#57afe2` | `FONT_ALT` |
| `--font-alt-highlight` | `#9ff1a1` | `FONT_ALT_HIGHLIGHT` |
| `--font-toast` | `black` | `FONT_TOAST` |
| `--font-toast-link` | `#0300bf` | `FONT_TOAST_LINK` |
| `--toast-progress` | `#dcba90` | `TOAST_PROGRESS` |
| `--toast-error` | `#dc9090` | `TOAST_ERROR` |
| `--toast-success` | `#a6dc90` | `TOAST_SUCCESS` |
| `--toast-info` | `#90bcdc` | `TOAST_INFO` |
| `--form-button-base` | `#22b549` | `BUTTON_BASE` |
| `--form-button-hover` | `#2665d2` | `BUTTON_HOVER` |
| `--form-button-menu` | `#389451` | `BUTTON_MENU` |
| `--form-button-menu-hover` | `#3e6bb9` | `BUTTON_MENU_HOVER` |
| `--form-button-disabled` | `#696969` | `BUTTON_DISABLED` |
| `--progress-bar` | `linear-gradient(#57afe2, #35759a)` | `PROGRESS_BAR_TOP` / `PROGRESS_BAR_BOTTOM` |
| `--trans-check-a` | `#303030` | `TRANS_CHECK_A` |
| `--trans-check-b` | `#272727` | `TRANS_CHECK_B` |

**Fonts:** Selawik regular + bold (`fonts/selawk.woff2` / `selawkb.woff2`) and Gambler (`fonts/gmblr.woff2`) are loaded at startup in `src/app.cpp`.

**Layout:** Header 53 px · Content area (flex) · Footer 73 px (grid rows defined in `renderAppShell()`).

**Known ImGui limitations** (cannot be replicated exactly; see `TODO_TRACKER.md` for entries):
- CSS `box-shadow` / `drop-shadow` — no native ImGui support; approximated with semi-transparent quads where critical
- CSS `filter: brightness()` / `hue-rotate()` — nav icon active/hover states use direct color tinting instead
- CSS `@keyframes` transitions and `transition:` properties — not applicable; state changes are immediate
- CSS `text-shadow` — not supported in ImGui
- `@media (max-height: N px)` responsive breakpoints — not yet implemented (TODO #593)

---

## Source Select Screen

The initial screen where the user chooses how to open a World of Warcraft installation (local, CDN, or legacy MPQ).

![Source Select Screen](UI_REFERENCE_IMAGES/Source%20Select%20Screen.png)

---

## Build Select Screen

Shown after choosing a source — lists available game builds to explore.

![Build Select Screen](UI_REFERENCE_IMAGES/Build%20Select%20Screen.png)

---

## Loading Screen

Displayed whenever the app is loading resources — opening a CASC/legacy source, loading tab data (models, textures, maps, creatures, characters, items, audio, videos, etc.), and exporting. Any operation that calls `showLoadingScreen()` shows this overlay with a progress bar.

![Loading Screen](UI_REFERENCE_IMAGES/Loading%20Screen.png)

---

## Home Screen

The main landing screen after a build has been loaded, showing navigation options for the various content tabs.

![Home Screen](UI_REFERENCE_IMAGES/Home%20Screen.png)

> **Legacy (MPQ) sources** use the same layout, but show only the navigation buttons available for that game version — not all tabs are present. Refer to the original JS source for which tabs are shown per version.

---

## Models Tab — Nothing Loaded

The models tab after data has loaded, showing the file list, empty 3D preview area, and Preview/Export controls on the right.

![Models Tab — Nothing Loaded](UI_REFERENCE_IMAGES/Models%20Tab%20-%20Nothing%20Loaded.png)

---

## Models Tab — M2 Model Loaded

An M2 model selected and previewed. M3 models render with the same layout.

![Models Tab — M2 Model Loaded](UI_REFERENCE_IMAGES/Models%20Tab%20-%20M2%20Model%20Loaded.png)

---

## Models Tab — WMO Model Loaded

A WMO model selected and previewed.

![Models Tab — WMO Model Loaded](UI_REFERENCE_IMAGES/Models%20Tab%20-%20WMO%20Model%20Loaded.png)

---

## Textures Tab

The textures tab with file list, texture preview, and export controls.

![Textures Tab](UI_REFERENCE_IMAGES/Textures%20Tab.png)

---

## Textures Tab — Atlas Regions Enabled

The textures tab with the "Atlas Regions" option enabled, showing region overlays on the texture preview.

![Textures Tab — Atlas Regions](UI_REFERENCE_IMAGES/Textures%20Tab%20-%20Atlas%20Regions%20Enabled.png)

---

## Characters Tab — Export Panel

The characters tab with character selection options on the left (race, gender, skin color, etc.), 3D preview in the center, and the Export panel selected in the bottom-right. Note: dropdown options are populated from the database.

![Characters Tab — Export](UI_REFERENCE_IMAGES/Characters%20Tab%20-%20Export%20Panel.png)

---

## Characters Tab — Textures Panel

The characters tab with the Textures panel selected in the bottom-right, showing texture file list and export options.

![Characters Tab — Textures](UI_REFERENCE_IMAGES/Characters%20Tab%20-%20Textures%20Panel.png)

---

## Characters Tab — Settings Panel

The characters tab with the Settings panel selected in the bottom-right, showing model display settings.

![Characters Tab — Settings](UI_REFERENCE_IMAGES/Characters%20Tab%20-%20Settings%20Panel.png)

---

## Characters Tab — Custom Geoset Selection

The characters tab with the custom geoset selection panel open on the left, allowing toggling of individual geometry groups on the character model.

![Characters Tab — Custom Geoset Selection](UI_REFERENCE_IMAGES/Characters%20Tab%20-%20Custom%20Geoset%20Selection.png)

---

## Items Tab

The items tab with an item selected, showing the 3D model preview and export controls.

![Items Tab](UI_REFERENCE_IMAGES/Items%20Tab.png)

---

## Item Sets Tab

The item sets tab showing grouped equipment sets with preview and export controls.

![Item Sets Tab](UI_REFERENCE_IMAGES/Item%20Sets%20Tab.png)

---

## Decor Tab

The decor tab with a model loaded, showing 3D preview and export controls.

![Decor Tab](UI_REFERENCE_IMAGES/Decor%20Tab.png)

---

## Creatures Tab

The creatures tab with a creature model loaded, showing 3D preview and export controls.

![Creatures Tab](UI_REFERENCE_IMAGES/Creatures%20Tab.png)

---

## Audio Tab

The audio tab with a sound playing, showing the animated model, playback controls, progress bar, and export options.

![Audio Tab](UI_REFERENCE_IMAGES/Audio%20Tab.png)

---

## Videos Tab

The videos tab with a video playing, showing the video list, playback area, and streaming controls.

![Videos Tab](UI_REFERENCE_IMAGES/Videos%20Tab.png)

---

## Maps Tab

The maps tab with a map section selected, showing the tile map viewer with selected region highlighted.

![Maps Tab - Map Selected](UI_REFERENCE_IMAGES/Maps%20Tab.png)

The maps tab when there is only a WMO (no terrain tiles), showing the WMO-only export view.

![Maps Tab - WMO Only](UI_REFERENCE_IMAGES/Maps%20Tab%20with%20only%20WMO.png)

---

## Zones Tab

The zones tab with a zone map loaded, showing the zone image viewer with navigation and export controls.

![Zones Tab](UI_REFERENCE_IMAGES/Zones%20Tab.png)

---

## Text Tab

The text tab showing the text file list with search/filter and file preview.

![Text Tab](UI_REFERENCE_IMAGES/Text%20Tab.png)

---

## Fonts Tab

The fonts tab with a font file selected, showing the font preview and export controls. Note: this tab uses single-selection only (no multi-select).

![Fonts Tab](UI_REFERENCE_IMAGES/Fonts%20Tab.png)

---

## Data Tab

The data tab with a data table loaded, showing column headers and row data.

![Data Tab - Table Loaded](UI_REFERENCE_IMAGES/Data%20Tab.png)

---

## Options Dropdown

Options Dropdown menu (This image is just to show the drop down menu in the top right, the dropdown can be used in any tab).

![Options Dropdown Menu](UI_REFERENCE_IMAGES/Options%20Dropdown%20Menu.png)

---

## Install Manifest Tab

The "Browse Install Manifest" tab, showing the install manifest file list.

![Install Manifest Tab](UI_REFERENCE_IMAGES/Install%20Manifest%20Tab.png)

---

## Raw Client Files Tab

The "Browse Raw Client Files" tab, showing the raw file browser for exploring CASC data directly.

![Raw Client Files Tab](UI_REFERENCE_IMAGES/Raw%20Client%20Files%20Tab.png)

---

## Settings Screen

The full settings screen, shown across multiple screenshots to capture the entire scrollable page. Settings include export directory, character save directory, scroll speed, file list ordering, unknown files, model skins, bone prefixes, shared textures/children, and more.

![Settings Screen — Top](UI_REFERENCE_IMAGES/Settings%20Screen%201.png)

![Settings Screen — Upper Middle](UI_REFERENCE_IMAGES/Settings%20Screen%202.png)

![Settings Screen — Middle](UI_REFERENCE_IMAGES/Settings%20Screen%203.png)

![Settings Screen — Lower Middle](UI_REFERENCE_IMAGES/Settings%20Screen%204.png)

![Settings Screen — Lower Bottom](UI_REFERENCE_IMAGES/Settings%20Screen%205.png)

![Settings Screen — Bottom](UI_REFERENCE_IMAGES/Settings%20Screen%206.png)
