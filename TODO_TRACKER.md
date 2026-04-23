# TODO Tracker

> **Progress: 0/193 verified (0%)** — ✅ = Verified, ⬜ = Pending

## Data Caches & Database

- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 19–112
- **Status**: Pending
- **Details**: JS uses async initialization and awaits DBC parsing operations; C++ performs synchronous parsing and loading, altering timing/error behavior relative to original Promise flow.

- **JS Source**: `src/js/db/caches/DBDecor.js` lines 15–40
- **Status**: Pending
- **Details**: JS `initializeDecorData` is async and awaits DB2 reads; C++ uses a synchronous blocking initializer, changing API timing behavior.

- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 34–88
- **Status**: Pending
- **Details**: JS uses `init_promise` and async `initialize/ensureInitialized` so concurrent callers await the same in-flight work; C++ uses synchronous initialization with no promise-sharing behavior.

- **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 18–53
- **Status**: Pending
- **Details**: JS `initializeItemDisplays` is Promise-based and awaits DB2/cache calls; C++ ports this path as synchronous blocking logic.

- **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 154–220
- **Status**: Pending
- **Details**: JS uses async initialization with `init_promise` deduplication; C++ uses a synchronous one-shot initializer and cannot preserve awaitable initialization semantics.

- **JS Source**: `src/js/db/caches/DBItemModels.js` lines 22–103
- **Status**: Pending
- **Details**: JS uses async `initialize` with shared `init_promise` and awaited dependent caches; C++ performs the entire load synchronously with no async/promise contract.

- **JS Source**: `src/js/db/caches/DBItems.js` lines 14–59
- **Status**: Pending
- **Details**: JS deduplicates concurrent initialization via `init_promise` and async functions; C++ uses synchronous initialization and lacks equivalent awaitable behavior.

- **JS Source**: `src/js/db/caches/DBModelFileData.js` lines 17–35
- **Status**: Pending
- **Details**: JS exposes `initializeModelFileData` as an async Promise-based loader; C++ implementation is synchronous blocking code.

- **JS Source**: `src/js/db/caches/DBNpcEquipment.js` lines 30–66
- **Status**: Pending
- **Details**: JS uses async initialization with in-flight promise reuse; C++ initialization is synchronous and does not retain the JS async concurrency contract.

- **JS Source**: `src/js/db/caches/DBTextureFileData.js` lines 16–52
- **Status**: Pending
- **Details**: JS defines async `initializeTextureFileData` and `ensureInitialized`; C++ ports both as synchronous methods.

## UI Components

- **JS Source**: `src/js/components/data-table.js` lines 950–958
- **Status**: Pending
- **Details**: JS `escape_value` checks `if (val === null || val === undefined) return 'NULL'` then `if (!isNaN(val) && str.trim() !== '') return str`. The `!isNaN(val)` check tests if the ORIGINAL value (not string) is numeric. C++ (lines 874–879) uses `tryParseNumber(val, num)` which parses the string representation. In JS, `!isNaN(Number(""))` is `false` (Number("") is 0 but isNaN(0) is false, so !isNaN is true) — wait, `Number("")` is `0` and `isNaN(0)` is `false`, so `!isNaN(Number(""))` is `true`. But the JS also checks `str.trim() !== ''` to exclude empty strings. C++ `tryParseNumber` checks `pos == s.size()` which would fail for empty string since `stod("")` throws. So both handle empty strings correctly (treated as non-numeric). Functionally equivalent.

- **JS Source**: `src/js/components/data-table.js` template lines 1018–1019
- **Status**: Pending
- **Details**: JS uses `filteredItems.length.toLocaleString()` and `rows.length.toLocaleString()` which formats numbers with locale-appropriate thousands separators. C++ uses `formatWithThousandsSep()` (line 1374–1377). The C++ function should produce comma-separated thousands (e.g., "1,234") to match the default English locale used in most WoW installations. If the function uses a different separator or format, the status text would differ visually.

- **JS Source**: `src/app.css` lines 1163–1168
- **Status**: Pending
- **Details**: C++ (line 971) hardcodes `const float headerHeight = 40.0f` with comment "padding 10px top/bottom + ~20px text". CSS `.ui-datatable table tr th` has `padding: 10px` (all sides). The actual header height depends on the font size — CSS text is typically ~14px default, so 10px + 14px + 10px = 34px, not 40px. If the CSS font size is different or the browser computes differently, the hardcoded 40px may not match.

- **JS Source**: `src/app.css` lines 1176–1191
- **Status**: Pending
- **Details**: CSS `.filter-icon` uses a background SVG image (`background-image: url(./fa-icons/funnel.svg)`) with `background-size: contain; width: 18px; height: 14px`. C++ (lines 1101–1113) draws a custom triangle + rectangle shape as a funnel icon approximation. The custom drawing may not match the SVG icon's exact shape and proportions. The CSS also specifies `opacity: 0.5` default and `opacity: 1.0` on hover, while C++ uses `ICON_DEFAULT_U32` and `FONT_HIGHLIGHT_U32` colors.

- **JS Source**: `src/app.css` lines 1198–1224
- **Status**: Pending
- **Details**: CSS `.sort-icon` uses `background-image: url(./fa-icons/sort.svg)` for the default state, with `.sort-icon-up` and `.sort-icon-down` using different SVG files. The icons have `width: 12px; height: 18px; opacity: 0.5`. C++ (lines 1119–1157) draws triangle shapes to approximate sort icons. The triangle approximation may not match the SVG icon's exact appearance — SVGs typically have more refined shapes with anti-aliasing.

- **JS Source**: `src/js/components/file-field.js` lines 34–40, 46
- **Status**: Pending
- **Details**: JS opens the directory picker when the text field receives focus. C++ opens the dialog only from a dedicated `...` button, changing interaction flow and UI behavior.

- **JS Source**: `src/js/components/file-field.js` lines 35–38
- **Status**: Pending
- **Details**: JS clears the hidden file input value before click so selecting the same directory re-triggers change emission. C++ dialog path does not mirror this reset contract.

- **JS Source**: `src/js/components/file-field.js` lines 33–40, template line 46
- **Status**: Pending
- **Details**: JS template uses `@focus="openDialog"` on the text input — when the input receives focus, the directory picker opens immediately. C++ (lines 128–132) uses a separate "..." button next to the input to trigger the dialog. The JS behavior is: clicking the text field opens the dialog, and the field never actually receives text focus for editing. C++ allows direct text editing in the field AND has a browse button. This is a significant UX difference — in JS, the field is effectively read-only (clicking always opens picker), while in C++ it's editable with an optional browse button.

- **JS Source**: `src/js/components/file-field.js` lines 14–23
- **Status**: Pending
- **Details**: JS creates a hidden file input with `nwdirectory` attribute (NW.js specific for directory selection), listens for `change` event, and emits the selected value. C++ uses `pfd::select_folder()` which opens a native folder dialog. The underlying mechanism differs (NW.js DOM file input vs. native OS dialog) but the user-facing behavior should be equivalent — both present a directory picker. C++ implementation correctly replaces the NW.js-specific API.

- **JS Source**: `src/js/components/file-field.js` lines 34–39
- **Status**: Pending
- **Details**: JS `openDialog()` sets `this.fileSelector.value = ''` before calling `click()` and then calls `this.$el.blur()`. This ensures the `change` event fires even if the user selects the same directory again. C++ `openDialog()` (line 74–81) calls `openDirectoryDialog()` directly without any pre-clear. Since `pfd::select_folder()` returns the result directly (not via an event), re-selecting the same directory works fine — the result is always returned. The `blur()` call is unnecessary in C++ since the dialog is modal.

- **JS Source**: `src/js/components/file-field.js` template line 46
- **Status**: Pending
- **Details**: C++ (lines 120–126) renders placeholder text with `ImVec2(textPos.x + 4.0f, textPos.y + 2.0f)` using hardcoded offsets. The JS template uses the browser's native placeholder rendering via `:placeholder="placeholder"` which automatically positions and styles the placeholder text. The C++ offsets may not match the actual input text baseline, causing misalignment.

- **JS Source**: `src/js/components/file-field.js` (entire file)
- **Status**: Pending
- **Details**: C++ adds `openFileDialog()` (lines 44–53) and `saveFileDialog()` (lines 60–73) as public utility functions. The JS file-field component only provides directory selection via `nwdirectory`. These extra functions may be useful for other parts of the C++ app but are not present in the original JS component source. They are additional API surface.

- **JS Source**: `src/js/components/resize-layer.js` lines 12–15, 21–23
- **Status**: Pending
- **Details**: JS emits resize through `ResizeObserver` mount/unmount lifecycle. C++ emits when measured width changes during render, so behavior is tied to render frames instead of observer callbacks.

- **JS Source**: `src/js/components/resize-layer.js` lines 1–26
- **Status**: Pending
- **Details**: The resize-layer component is a simple wrapper that emits a 'resize' event when the element width changes. JS uses `ResizeObserver` and `beforeUnmount` cleanup. C++ uses `ImGui::GetContentRegionAvail().x` polling each frame and compares against previous width. The conversion is functionally complete and correct. No deviations found.

## Markdown & Content Rendering

- [ ] 115. [markdown-content.cpp] Inline image markdown is not rendered as images
- **JS Source**: `src/js/components/markdown-content.js` lines 208–216, 251–253
- **Status**: Pending
- **Details**: JS converts `![alt](src)` to `<img ...>` in `v-html` output. C++ renders image segments as placeholder text (`[Image: ...]`), causing functional and visual mismatch.

- [ ] 116. [markdown-content.cpp] Inline bold/italic formatting behavior differs from JS HTML rendering
- **JS Source**: `src/js/components/markdown-content.js` lines 219–224
- **Status**: Pending
- **Details**: JS emits `<strong>`/`<em>` markup; C++ substitutes color-only text rendering because ImGui has no inline bold/italic in this path, so typography does not match original styling.

- [ ] 117. [markdown-content.cpp] CSS base font-size 20px not applied — ImGui uses default ~14px font
- **JS Source**: `src/app.css` lines 236–243
- **Status**: Pending
- **Details**: CSS `.markdown-content { font-size: 20px; }` sets the base font size for all markdown content to 20px. C++ `render()` does not call `ImGui::SetWindowFontScale()` to scale up to 20px equivalent. The default ImGui font is ~13-14px, so all markdown text renders significantly smaller than the JS version. Header scales (1.8em, 1.5em, 1.2em) are applied correctly relative to the current font scale, but since the base is wrong, all absolute sizes are too small.

- [ ] 118. [markdown-content.cpp] Link color uses `FONT_ALT` (#57afe2 blue) instead of CSS `--font-highlight` (#ffffff white)
- **JS Source**: `src/app.css` lines 311–314
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner a { color: var(--font-highlight); text-decoration: underline; }` uses pure white (#ffffff) for links with an underline. C++ `renderInlineSegments` line 254 uses `app::theme::FONT_ALT` (blue #57afe2) for links. The link color should be `FONT_HIGHLIGHT` (white) to match the CSS.

- [ ] 119. [markdown-content.cpp] Links rendered without underline decoration
- **JS Source**: `src/app.css` lines 311–314
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner a { text-decoration: underline; }` renders links with an underline. C++ `renderInlineSegments` (lines 253–267) renders link text without any underline decoration. ImGui does not natively support underlined text, but a manual underline can be drawn using `ImGui::GetWindowDrawList()->AddLine()` beneath the text.

- [ ] 120. [markdown-content.cpp] Inline code missing background `rgba(0,0,0,0.3)` with `padding: 2px 6px` and `border-radius: 3px`
- **JS Source**: `src/app.css` lines 283–288
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner code` has a dark background, padding, and rounded corners. C++ `renderInlineSegments` (lines 248–251) only changes the text color to `(0.9, 0.7, 0.5)` for inline code, without any background rectangle. The background color should be `IM_COL32(0, 0, 0, 77)` with a rounded rect behind the text.

- [ ] 121. [markdown-content.cpp] Inline code color `(0.9, 0.7, 0.5)` has no CSS basis — CSS uses monospace font, not a special color
- **JS Source**: `src/app.css` lines 283–289
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner code` uses `font-family: monospace` but inherits the parent text color (white). C++ applies `ImVec4(0.9f, 0.7f, 0.5f, 1.0f)` (orange-ish) which has no equivalent in the CSS. The text color should remain the same as surrounding text; only the font family should change (which ImGui can't easily do for inline segments).

- [ ] 122. [markdown-content.cpp] Bold text rendered with white color instead of bold font weight
- **JS Source**: `src/app.css` lines 303–305
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner strong { font-weight: bold; }` makes bold text heavier. C++ `renderInlineSegments` (lines 237–240) pushes white color `(1,1,1,1)` for bold, which is the same as normal text on a dark background. The visual difference from normal text is imperceptible. ImGui doesn't support inline bold without loading a separate bold font face and switching to it.

- [ ] 123. [markdown-content.cpp] Italic text rendered with dim blue `(0.8, 0.8, 0.9)` instead of italic font style
- **JS Source**: `src/app.css` lines 307–309
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner em { font-style: italic; }` renders text in italic. C++ `renderInlineSegments` (lines 243–246) uses a dimmed blue-white color `(0.8, 0.8, 0.9)` which has no CSS basis. ImGui doesn't support italic text natively, but the color chosen doesn't match any CSS variable.

- [ ] 124. [markdown-content.cpp] h1 header missing bottom separator matching CSS `border-bottom`
- **JS Source**: `src/app.css` lines 249–253
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner h1` has `margin: 0` but standard HTML h1 elements typically render with a bottom margin. C++ adds `ImGui::Separator()` after h1 (line 347) which draws a full-width horizontal line. The JS HTML rendering does not add a separator after h1 — the `<h1>` tag simply has larger text. The `ImGui::Separator()` is not present in the CSS and creates a visual difference.

- [ ] 125. [markdown-content.cpp] Scrollbar thumb uses hardcoded gray colors instead of CSS `var(--border)` and `var(--font-highlight)`
- **JS Source**: `src/app.css` lines 322–346
- **Status**: Pending
- **Details**: CSS `.vscroller > div` uses `background: var(--border)` (#6c757d) default and `background: var(--font-highlight)` (#ffffff) on hover/active. C++ uses hardcoded `IM_COL32(120, 120, 120, 150)` default and `IM_COL32(180, 180, 180, 200)` for active (lines 457–459). Neither color matches the CSS variables. Default should be `BORDER_U32` and active should be `FONT_HIGHLIGHT_U32`.

- [ ] 126. [markdown-content.cpp] Scrollbar thumb missing `border: 1px solid var(--border)` and `border-radius: 5px` from CSS
- **JS Source**: `src/app.css` lines 332–341
- **Status**: Pending
- **Details**: CSS `.vscroller > div` has `border: 1px solid var(--border)` and `border-radius: 5px`. C++ draws the thumb with `AddRectFilled` with 4px rounding (line 461) but no border outline. Should add `AddRect` with `BORDER_U32` and use 5px rounding.

- [ ] 127. [markdown-content.cpp] Scrollbar track `right: 3px` positioning and `opacity: 0.7` not matched
- **JS Source**: `src/app.css` lines 322–330
- **Status**: Pending
- **Details**: CSS `.vscroller { right: 3px; opacity: 0.7; }` positions the scrollbar 3px from the right edge with 70% opacity. C++ positions the scrollbar at `containerSize.x - scrollbar_width - 2.0f` (line 451, using 2px instead of 3px) and uses full opacity for the thumb colors. The alpha values in the thumb colors partially compensate but don't match the 70% opacity overlay.

- [ ] 128. [markdown-content.cpp] `parseInline` processes text before HTML escaping, while JS escapes first then applies regex
- **JS Source**: `src/js/components/markdown-content.js` lines 204–237
- **Status**: Pending
- **Details**: JS `parseInline` calls `this.escapeHtml(text)` first (line 205) before applying regex replacements for bold, italic, code, links, and images. This means the regex patterns match against HTML-escaped text (e.g., `&amp;` instead of `&`). C++ `parseInline` processes raw text directly without escaping. While ImGui doesn't need HTML escaping, if markdown content contains `&`, `<`, or `>` characters, the parsing behavior could differ because the JS regex operates on escaped text while C++ operates on raw text.

- [ ] 129. [markdown-content.cpp] List items use `•` bullet with 16px indent instead of CSS `padding-left: 2em` with disc marker
- **JS Source**: `src/app.css` lines 272–276
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner ul { padding-left: 2em; list-style-type: disc; }` uses standard disc bullets with 2em left padding. At 20px base font, 2em = 40px. C++ uses `ImGui::Indent(16.0f)` with a manual `•` character (lines 369–373). 16px vs 40px indent is a significant visual difference, and the bullet character may render differently than the CSS disc marker.

## Map Viewer

- [ ] 141. [map-viewer.cpp] Tile image drawing path is still unimplemented
- **JS Source**: `src/js/components/map-viewer.js` lines 380–402, 1111–1113
- **Status**: Pending
- **Details**: JS draws loaded tiles to canvas via `putImageData(...)` on main/double-buffer contexts. C++ caches tile pixels but does not upload/draw them, so only overlays render and map tiles are not visually equivalent.

- **JS Source**: `src/js/components/map-viewer.js` lines 192–197, 380–414
- **Status**: Pending
- **Details**: JS tile loader is async (`loader(...).then(...)`) with Promise completion timing. C++ calls loader synchronously in `loadTile(...)`, changing queue timing and behavior during panning/zoom updates.

- [ ] 143. [map-viewer.cpp] Box-select-mode active color uses NAV_SELECTED (#22B549) instead of CSS #5fdb65
- **JS Source**: `src/app.css` line 1348–1349
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .info span.active` uses `color: #5fdb65` (RGB 95, 219, 101). C++ line 1178 uses `app::theme::NAV_SELECTED` which maps to `BUTTON_BASE` = `(0.133, 0.710, 0.286)` = RGB(34, 181, 73). The active highlight color is noticeably different — should be a dedicated color constant matching #5fdb65.

- [ ] 144. [map-viewer.cpp] Map-viewer info bar text lacks CSS `text-shadow: black 0 0 6px`
- **JS Source**: `src/app.css` lines 1326–1328
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer div { text-shadow: black 0 0 6px; }` applies a text shadow to all divs inside the map viewer, including the info bar and hover-info. C++ renders these with plain `ImGui::TextUnformatted` (lines 1166–1189) with no text shadow effect. ImGui does not natively support text shadows, so a manual shadow would need to be rendered (draw text offset in black, then draw normal text on top).

- [ ] 145. [map-viewer.cpp] Map-viewer info bar spans missing CSS `margin: 0 10px` horizontal spacing
- **JS Source**: `src/app.css` lines 1345–1346
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .info span { margin: 0 10px; }` gives each info label 10px left/right margin. C++ uses `ImGui::SameLine()` between items (lines 1167–1175) with default spacing. The default ImGui item spacing is typically ~8px which may not exactly match the 20px total gap (10px + 10px) between spans.

- [ ] 146. [map-viewer.cpp] Map-viewer checkerboard background pattern not implemented
- **JS Source**: `src/app.css` lines 1300–1306
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer` has a complex checkerboard background using `background-image: linear-gradient(45deg, ...)` with `--trans-check-a` and `--trans-check-b` colors, `background-size: 30px 30px`, and `background-position: 0 0, 15px 15px`. C++ `renderWidget` (line 1148) uses `ImGui::BeginChild` with no custom background drawing — the checkerboard transparency pattern is missing entirely. The JS version shows a checkerboard behind transparent map tiles.

- [ ] 147. [map-viewer.cpp] Map-viewer hover-info positioned at top via ImGui layout instead of CSS `bottom: 3px; left: 3px`
- **JS Source**: `src/app.css` lines 1330–1333
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .hover-info` is positioned `bottom: 3px; left: 3px` (bottom-left corner of the map viewer). C++ renders hover-info at line 1187–1189 via `ImGui::SameLine()` after the info bar spans, placing it inline at the top. It should be rendered at the bottom-left of the map viewer container using `ImGui::SetCursorPos` or overlay drawing.

- [ ] 148. [map-viewer.cpp] Box-select-mode cursor not changed to crosshair
- **JS Source**: `src/app.css` lines 1351–1353
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer.box-select-mode { cursor: crosshair; }` changes the cursor to a crosshair when box-select mode is active. C++ does not call `ImGui::SetMouseCursor(ImGuiMouseCursor_Hand)` or any cursor change when `state.isBoxSelectMode` is true. ImGui does not have a native crosshair cursor, but this should at least document the visual difference.

- [ ] 149. [map-viewer.cpp] Tile rendering to canvas via GL textures not implemented — tiles cached in memory but not displayed
- **JS Source**: `src/js/components/map-viewer.js` lines 350–500 (loadTile, renderWithDoubleBuffer)
- **Status**: Pending
- **Details**: The JS version draws tiles to a `<canvas>` via `context.putImageData()` and `context.drawImage()`. C++ caches tile pixel data in `s_state.tilePixelCache` (map-viewer.cpp line 81) but never uploads it as OpenGL textures or renders it to the screen. The TODO comment at line 1194–1198 acknowledges this. The map overlay (selection highlights, hover) draws over empty space. This is a critical functional gap — the map tiles are invisible.

- [ ] 150. [map-viewer.cpp] `handleTileInteraction` emits selection changes via mutable reference instead of `$emit('update:selection')`
- **JS Source**: `src/js/components/map-viewer.js` lines 846–874
- **Status**: Pending
- **Details**: JS `handleTileInteraction` modifies `this.selection` array directly (splice/push) and Vue reactivity propagates changes via `v-model`. C++ modifies the `selection` vector reference directly (lines 845–848). However, JS Select All (line 812) uses `this.$emit('update:selection', newSelection)` with a new array — the C++ equivalent calls `onSelectionChanged(newSelection)` callback in `handleKeyPress` (line 780) and `finalizeBoxSelection` (line 941). This means `handleTileInteraction` mutates in-place but Select All and box-select create new arrays — potential inconsistency in selection update patterns.

- [ ] 151. [map-viewer.cpp] Map margin `20px 20px 0 10px` from CSS not applied
- **JS Source**: `src/app.css` line 1307
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer { margin: 20px 20px 0 10px; }` adds specific margins around the map viewer. C++ `renderWidget` uses `ImGui::GetContentRegionAvail()` for sizing (line 1144) without adding any padding/margin equivalent. The parent layout is responsible for this in ImGui, but if not handled there, the map viewer will be flush against adjacent elements.

## Tab: Changelog, Help, Blender & Install

- **JS Source**: `src/js/modules/tab_changelog.js` lines 14–16
- **Status**: Pending
- **Details**: JS uses `BUILD_RELEASE ? './src/CHANGELOG.md' : '../../CHANGELOG.md'`; C++ adds a third fallback (`CHANGELOG.md`) and different path probing order, changing source resolution behavior.

- **JS Source**: `src/js/modules/tab_changelog.js` lines 31–35
- **Status**: Pending
- **Details**: JS uses dedicated `#changelog`/`#changelog-text` template structure and CSS styling; C++ renders plain ImGui title/separator/button layout, causing visible UI differences.

- **JS Source**: `src/js/modules/tab_changelog.js` line 32
- **Status**: Pending
- **Details**: JS uses h1 tag which renders as a large bold heading per CSS. C++ uses ImGui::Text with default font size and weight.

- **JS Source**: `src/js/modules/tab_help.js` lines 145–149, 153–157
- **Status**: Pending
- **Details**: JS applies article filtering via `setTimeout(..., 300)` debounce on `search_query`; C++ filters immediately on each input change, changing responsiveness and update timing.

- **JS Source**: `src/js/modules/tab_help.js` lines 115–121
- **Status**: Pending
- **Details**: JS renders per-item title and a separate tags row with KB badge styling; C++ combines content into selectable labels and tooltip tags, so article list visuals/structure are not identical.

- **JS Source**: `src/js/modules/tab_help.js` lines 145–149
- **Status**: Pending
- **Details**: JS implements a 300ms setTimeout debounce before filtering. C++ calls update_filter() immediately on every keystroke.

- [ ] 175. [tab_help.cpp] Article list layout differs tags shown in tooltip instead of inline
- **JS Source**: `src/js/modules/tab_help.js` lines 115–121
- **Status**: Pending
- **Details**: JS renders each article with visible title and tags divs inline. C++ renders KB_ID and title as a single Selectable with tags only in hover tooltip. Tags and KB ID badge are not visually inline.

- **JS Source**: `src/js/modules/tab_help.js` line 8
- **Status**: Pending
- **Details**: JS load_help_docs is async. C++ is synchronous blocking the main thread during file reads.

- **JS Source**: `src/js/modules/tab_blender.js` lines 91, 139
- **Status**: Pending
- **Details**: JS compares versions as strings (`version >= MIN_VER`, `blender_version < MIN_VER`), while C++ parses with `std::stod` and compares numerically, changing edge-case ordering behavior.

- **JS Source**: `src/js/modules/tab_blender.js` lines 59–69
- **Status**: Pending
- **Details**: JS uses structured `#blender-info`/`#blender-info-buttons` markup with CSS-defined spacing/styling; C++ replaces it with simple ImGui text/separator/buttons, producing visual/layout mismatch.

- **JS Source**: `src/js/modules/tab_blender.js` line 39
- **Status**: Pending
- **Details**: JS match() performs a search anywhere in string. C++ uses std::regex_match which requires the ENTIRE string to match. Directory names like blender-2.83 would match in JS but fail in C++.

- **JS Source**: `src/js/modules/tab_blender.js` lines 81, 127
- **Status**: Pending
- **Details**: Both JS functions are async with await. C++ implementations are synchronous, blocking the render thread.

- [ ] 182. [tab_install.cpp] Install listbox copy/paste options are hardcoded instead of using JS config-driven behavior
- **JS Source**: `src/js/modules/tab_install.js` lines 165, 184
- **Status**: Pending
- **Details**: JS listbox wiring uses `$core.view.config.copyMode`, `pasteSelection`, and `removePathSpacesCopy`; C++ passes `CopyMode::Default` with `pasteselection=false` and `copytrimwhitespace=false`, changing list interaction behavior.

- [ ] 183. [tab_install.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS renders `Regex Enabled` with `:title="$core.view.regexTooltip"`; C++ renders plain text without the tooltip contract, changing UI affordance.

- **JS Source**: `src/js/modules/tab_install.js` lines 52–146
- **Status**: Pending
- **Details**: JS export_install_files(), view_strings(), and export_strings() are all async functions that await CASC file I/O. C++ versions are fully synchronous, blocking the UI thread.

- [ ] 185. [tab_install.cpp] CASC getFile replaced with low-level two-step call, losing BLTE decoding
- **JS Source**: `src/js/modules/tab_install.js` lines 73–74
- **Status**: Pending
- **Details**: JS calls core.view.casc.getFile() which returns a BLTEReader for BLTE block decompression. C++ calls getEncodingKeyForContentKey then _ensureFileInCache then BufferWrapper::readFile, skipping BLTE decompression entirely. Exported files may be corrupt.

- [ ] 186. [tab_install.cpp] processAllBlocks() call missing in view_strings_impl
- **JS Source**: `src/js/modules/tab_install.js` lines 103–105
- **Status**: Pending
- **Details**: JS calls data.processAllBlocks() after getFile() to force all BLTE blocks decompressed before extract_strings. C++ skips this step because it uses plain BufferWrapper instead of BLTEReader.

- [ ] 187. [tab_install.cpp] export_install_files missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_install.js` line 78
- **Status**: Pending
- **Details**: JS calls helper.mark(file_name, false, e.message, e.stack) passing both message and stack trace. C++ passes only e.what(), omitting the stack trace parameter.

- [ ] 188. [tab_install.cpp] First listbox missing copyMode from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode" to the main install Listbox. C++ hardcodes listbox::CopyMode::Default instead of reading from view.config.

- [ ] 189. [tab_install.cpp] First listbox missing pasteSelection from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :pasteselection="$core.view.config.pasteSelection". C++ hardcodes false instead of reading from view.config.

- [ ] 190. [tab_install.cpp] First listbox missing copytrimwhitespace from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :copytrimwhitespace="$core.view.config.removePathSpacesCopy". C++ hardcodes false, disabling the remove-path-spaces-on-copy feature.

- [ ] 191. [tab_install.cpp] Second listbox (strings) missing copyMode from config
- **JS Source**: `src/js/modules/tab_install.js` line 184
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode" to the strings Listbox. C++ hardcodes listbox::CopyMode::Default.

- [ ] 194. [tab_install.cpp] Strings sidebar missing CSS styling equivalents
- **JS Source**: `src/js/modules/tab_install.js` lines 194–197
- **Status**: Pending
- **Details**: CSS defines .strings-header font-size 14px opacity 0.7, .strings-filename font-size 12px word-break: break-all, 5px gap. C++ uses default font sizes and ImGui::Spacing() which may not match.

- [ ] 195. [tab_install.cpp] Input placeholder text not rendered
- **JS Source**: `src/js/modules/tab_install.js` lines 170, 189
- **Status**: Pending
- **Details**: JS filter inputs have placeholder="Filter install files..." and "Filter strings...". C++ uses plain ImGui::InputText without hint/placeholder.

- [ ] 196. [tab_install.cpp] Regex tooltip not rendered
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS "Regex Enabled" div has :title="$core.view.regexTooltip" showing tooltip on hover. C++ has no tooltip implementation.

## Tab: Models

## Tab: Textures

- [ ] 233. [tab_textures.cpp] export_texture_atlas_regions missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_textures.js` line 261
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- **JS Source**: `src/js/modules/tab_textures.js` lines 23–413
- **Status**: Pending
- **Details**: JS preview_texture_by_id, load_texture_atlas_data, reload_texture_atlas_data, export_texture_atlas_regions, export_textures, initialize, and apply_baked_npc_texture are all async. C++ equivalents all run synchronously on UI thread.

## Tab: Audio

- [ ] 249. [tab_audio.cpp] export_sounds missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_audio.js` line 175
- **Status**: Pending
- **Details**: JS passes e.message and e.stack to helper.mark(). C++ only passes e.what().

- **JS Source**: `src/js/modules/tab_audio.js` lines 47, 99, 122
- **Status**: Pending
- **Details**: All three JS functions are async with await. C++ implementations are synchronous, blocking the render thread.

- [ ] 251. [legacy_tab_audio.cpp] Playback UI visuals diverge from JS template/CSS
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 201–241
- **Status**: Pending
- **Details**: JS renders `#sound-player-anim`, CSS-styled play button state classes, and component sliders, while C++ replaces this with ImGui text/buttons/checkboxes and a custom icon pulse, so layout/styling is not pixel-identical.

- [ ] 252. [legacy_tab_audio.cpp] Seek-loop scheduling differs from JS `requestAnimationFrame` lifecycle
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 19–42
- **Status**: Pending
- **Details**: JS drives seek updates with `requestAnimationFrame` and explicit cancellation IDs; C++ updates via render-loop polling with `seek_loop_active`, changing timing and loop lifecycle semantics.

- [ ] 253. [legacy_tab_audio.cpp] Context menu adds FileDataID-related items not present in JS legacy audio template
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 205–209
- **Status**: Pending
- **Details**: JS context menu has 3 items: "Copy file path(s)", "Copy export path(s)", "Open export directory". C++ adds conditional "Copy file path(s) (listfile format)" and "Copy file data ID(s)" when `hasFileDataIDs` is true (lines 399–402). Legacy MPQ files don't have FileDataIDs, so these extra menu items are incorrect for the legacy audio tab.

- [ ] 254. [legacy_tab_audio.cpp] Sound player info combines seek/title/duration into single Text call vs JS 3 separate spans
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 218–222
- **Status**: Pending
- **Details**: JS renders sound player info as 3 separate `<span>` elements in a flex container: seek formatted time, title (with CSS class "title"), and duration formatted time. C++ combines them into a single `ImGui::Text("%s  %s  %s", ...)` call (line 453). The title won't have distinct styling, and the layout/alignment will differ from the JS flex row.

- [ ] 255. [legacy_tab_audio.cpp] Play/Pause uses text toggle ("Play"/"Pause") vs JS CSS class-based visual toggle
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 225
- **Status**: Pending
- **Details**: JS uses `<input type="button" :class="{ isPlaying: !soundPlayerState }">` — the button appearance changes via CSS class (likely showing a play/pause icon). C++ uses `ImGui::Button(view.soundPlayerState ? "Pause" : "Play")` with text labels. The visual appearance differs significantly from the original icon-based toggle.

- [ ] 256. [legacy_tab_audio.cpp] Volume slider is ImGui::SliderFloat with format string vs JS custom Slider component
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 226
- **Status**: Pending
- **Details**: JS uses a custom `<Slider>` component with id "slider-volume" for volume control. C++ uses `ImGui::SliderFloat` with format "Vol: %.0f%%" (line 474). The custom JS Slider has its own visual styling defined in CSS; the ImGui slider will look different (default ImGui styling vs themed slider).

- [ ] 257. [legacy_tab_audio.cpp] Loop/Autoplay checkboxes placed in preview container instead of preview-controls div
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 231–239
- **Status**: Pending
- **Details**: JS places Loop/Autoplay checkboxes and Export button together in the `preview-controls` div. C++ places Loop/Autoplay in the `PreviewContainer` section (lines 479–487) and Export in `PreviewControls` (lines 492–498). This changes the visual layout — checkboxes are above the export button area instead of beside it.

- [ ] 258. [legacy_tab_audio.cpp] `load_track` checks `player.get_duration() <= 0` vs JS `!player.buffer`
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 100–101
- **Status**: Pending
- **Details**: JS `play_track` checks `!player.buffer` to determine if a track needs loading. C++ checks `player.get_duration() <= 0` (line 136). If a loaded track has zero duration (e.g., corrupt file that loads but has 0-length), C++ would re-load while JS would not. The check semantics are subtly different.

- [ ] 259. [legacy_tab_audio.cpp] `export_sounds` `helper.mark` doesn't pass error stack trace
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 189
- **Status**: Pending
- **Details**: JS calls `helper.mark(export_file_name, false, e.message, e.stack)` with 4 arguments including the stack trace. C++ calls `helper.mark(export_file_name, false, e.what())` with only 3 arguments (line 239). Error stack information is lost in C++ export failure reports.

## Tab: Video

- [ ] 261. [tab_videos.cpp] Video preview playback is opened externally instead of using an embedded player
- **JS Source**: `src/js/modules/tab_videos.js` lines 219–276, 493
- **Status**: Pending
- **Details**: JS renders and controls an in-tab `<video>` element with `onended`/`onerror` and subtitle track attachment, while C++ opens the stream URL in an external handler and shows status text in the preview area.

- [ ] 262. [tab_videos.cpp] Video export format selector from MenuButton is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 505, 559–571
- **Status**: Pending
- **Details**: JS uses a `MenuButton` bound to `config.exportVideoFormat` and dispatches format-specific export via selection; C++ renders a single `Export Selected` button with no in-UI format picker.

- [ ] 263. [tab_videos.cpp] Kino processing toast omits the explicit Cancel action payload
- **JS Source**: `src/js/modules/tab_videos.js` lines 394–400
- **Status**: Pending
- **Details**: JS updates progress toast with `{ 'Cancel': cancel_processing }`; C++ calls `setToast(..., {}, ...)`, removing the explicit cancel action binding from the toast configuration.

- [ ] 264. [tab_videos.cpp] Dev-mode kino processing trigger export is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 467–469
- **Status**: Pending
- **Details**: JS exposes `window.trigger_kino_processing = trigger_kino_processing` in non-release mode; C++ has no equivalent debug export hook.

- [ ] 265. [tab_videos.cpp] Corrupted AVI fallback does not force CASC fallback fetch path
- **JS Source**: `src/js/modules/tab_videos.js` line 697
- **Status**: Pending
- **Details**: JS retries corrupted cinematic reads with `getFileByName(file_name, false, false, true, true)` to force fallback behavior; C++ retries `getVirtualFileByName(file_name)` with normal arguments.

- [ ] 266. [tab_videos.cpp] MenuButton export format dropdown completely missing
- **JS Source**: `src/js/modules/tab_videos.js` line 505
- **Status**: Pending
- **Details**: JS uses `<MenuButton :options="menuButtonVideos" :default="config.exportVideoFormat" @change="..." @click="export_selected">` which renders a dropdown to pick MP4/AVI/MP3/SUBTITLES and triggers export. C++ renders a plain `ImGui::Button("Export Selected")` with no format selector. Users cannot change the export format from this tab.

- [ ] 267. [tab_videos.cpp] AVI export corruption fallback is a no-op
- **JS Source**: `src/js/modules/tab_videos.js` line 697
- **Status**: Pending
- **Details**: JS calls `getFileByName(file_name, false, false, true, true)` with extra params (forceFallback). C++ calls `getVirtualFileByName(file_name)` identically to the first attempt, with a comment admitting `// Note: C++ getVirtualFileByName doesn't support forceFallback; retry normally.` The corruption recovery path will always fail the same way twice.

- [ ] 268. [tab_videos.cpp] Video preview is text-only, not an embedded player
- **JS Source**: `src/js/modules/tab_videos.js` line 493
- **Status**: Pending
- **Details**: JS renders a `<video>` element with full controls, autoplay, subtitles overlay via `<track>`. C++ opens the URL in the system's external media player (`core::openInExplorer(url)`) and shows plain text. No inline playback, no controls, no subtitle overlay in the app window.

- [ ] 269. [tab_videos.cpp] No onended/onerror callbacks for video playback
- **JS Source**: `src/js/modules/tab_videos.js` lines 263–275
- **Status**: Pending
- **Details**: JS attaches `video.onended` (resets `is_streaming`/`videoPlayerState`) and `video.onerror` (shows error toast). C++ delegates to external player and has neither callback — `is_streaming` and `videoPlayerState` are never automatically reset when playback finishes; user must manually click "Stop Video."

- **JS Source**: `src/js/modules/tab_videos.js` line 125
- **Status**: Pending
- **Details**: JS `build_payload` is `async`/`await` (non-blocking). In C++, it's called synchronously on the main thread before launching the background thread. DB2 queries + CASC lookups could freeze the UI.

- [ ] 271. [tab_videos.cpp] stop_video does not join/stop background thread
- **JS Source**: `src/js/modules/tab_videos.js` lines 27–57
- **Status**: Pending
- **Details**: JS clears `setTimeout` handle which fully cancels pending work. C++ sets `poll_cancelled = true` but does not `reset()` or join `stream_worker_thread`. The thread may still be running and post results after stop. Only `stream_video` joins it before a new stream.

- [ ] 272. [tab_videos.cpp] MP4 download HTTP error check missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 631–633
- **Status**: Pending
- **Details**: JS explicitly checks `if (!response.ok)` and marks the file with 'Failed to download MP4: ' + response.status. C++ uses `generics::get(*mp4_url)` with no status check — if the server returns a non-200 status, behavior depends on `generics::get()` implementation.

- [ ] 273. [tab_videos.cpp] All helper.mark error calls missing stack trace argument
- **JS Source**: `src/js/modules/tab_videos.js` lines 642, 690, 702, 763, 822
- **Status**: Pending
- **Details**: JS passes `(file, false, e.message, e.stack)` (4 args) for error marking. C++ passes only `(file, false, e.what())` (3 args). Stack traces are lost in every export error path (MP4, AVI×2, MP3, Subtitles).

- [ ] 274. [tab_videos.cpp] stream_video outer catch missing stack trace log
- **JS Source**: `src/js/modules/tab_videos.js` line 214
- **Status**: Pending
- **Details**: JS logs `log.write(e.stack)` separately from the error message. C++ only logs `e.what()`.

- [ ] 275. [tab_videos.cpp] Cancel button missing from kino_processing toast
- **JS Source**: `src/js/modules/tab_videos.js` line 399
- **Status**: Pending
- **Details**: JS passes `{ 'Cancel': cancel_processing }` as toast buttons. C++ passes `{}` — no Cancel button is shown during batch processing, leaving users with no way to cancel.

- [ ] 276. [tab_videos.cpp] Regex tooltip missing on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_videos.js` line 489
- **Status**: Pending
- **Details**: JS shows `:title="$core.view.regexTooltip"` tooltip on the "Regex Enabled" text. C++ renders `ImGui::TextUnformatted("Regex Enabled")` with no tooltip.

- [ ] 277. [tab_videos.cpp] Spurious "Connecting to video server..." toast not in JS
- **JS Source**: `src/js/modules/tab_videos.js` (none)
- **Status**: Pending
- **Details**: JS shows no toast before the initial HTTP request — it only shows "Video is being processed..." on 202 status. C++ always shows a "Connecting to video server..." progress toast before the request, which is not in the original.

- [ ] 279. [tab_videos.cpp] Filter input buffer capped at 255 chars
- **JS Source**: `src/js/modules/tab_videos.js` line 490
- **Status**: Pending
- **Details**: JS `v-model` has no character limit. C++ uses `char filter_buf[256]` which truncates filter input at 255 characters.

- [ ] 280. [tab_videos.cpp] kino_post hardcodes hostname and path instead of using constant
- **JS Source**: `src/js/modules/tab_videos.js` lines 137, 349, 431
- **Status**: Pending
- **Details**: JS uses `constants.KINO.API_URL` dynamically via `fetch()`. C++ hardcodes `httplib::SSLClient cli("www.kruithne.net")` and `.Post("/wow.export/v2/get_video", ...)` instead of parsing the constant. If the constant changes, C++ won't reflect it.

- [ ] 281. [tab_videos.cpp] Subtitle loading uses different API path than JS
- **JS Source**: `src/js/modules/tab_videos.js` lines 226–230
- **Status**: Pending
- **Details**: JS calls `subtitles.get_subtitles_vtt(core_ref.view.casc, subtitle_info.file_data_id, subtitle_info.format)` which fetches+converts internally. C++ manually fetches via `casc->getVirtualFileByID()`, reads as string, then calls `subtitles::get_subtitles_vtt(raw_subtitle_text, fmt)`. Different function signature — caller now responsible for fetching.

- [ ] 282. [tab_videos.cpp] MP4 download may lack User-Agent header
- **JS Source**: `src/js/modules/tab_videos.js` line 628
- **Status**: Pending
- **Details**: JS explicitly sets `'User-Agent': constants.USER_AGENT` for the MP4 download fetch. C++ uses `generics::get(*mp4_url)` which may or may not set User-Agent, depending on that function's implementation.

- [ ] 284. [tab_videos.cpp] Dev-mode trigger_kino_processing not exposed in C++
- **JS Source**: `src/js/modules/tab_videos.js` lines 468–469
- **Status**: Pending
- **Details**: JS exposes `window.trigger_kino_processing = trigger_kino_processing` when `!BUILD_RELEASE`. C++ has only a comment. No equivalent debug hook exists.

## Tab: Text & Fonts

- **JS Source**: `src/js/modules/tab_text.js` lines 77–121
- **Status**: Pending
- **Details**: JS export_text is async with await for generics.fileExists(), casc.getFileByName(), and data.writeToFile(). C++ runs entirely synchronously, freezing UI during multi-file export.

- [ ] 299. [tab_fonts.cpp] export_fonts missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_fonts.js` line 141
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

- **JS Source**: `src/js/modules/tab_fonts.js` lines 16, 102
- **Status**: Pending
- **Details**: Both JS functions are async. C++ implementations are synchronous blocking the render thread.

## Tab: Data

- [ ] 306. [tab_data.cpp] Data-table cell copy stringification differs from JS `String(value)` behavior
- **JS Source**: `src/js/modules/tab_data.js` lines 172–177
- **Status**: Pending
- **Details**: JS copies with `String(value)`, while C++ uses `value.dump()`; for string JSON values this includes JSON quoting/escaping, changing clipboard output.

- [ ] 307. [tab_data.cpp] DB2 load error toast omits JS `View Log` action callback
- **JS Source**: `src/js/modules/tab_data.js` lines 80–82
- **Status**: Pending
- **Details**: JS error toast includes `{'View Log': () => log.openRuntimeLog()}`; C++ error toast uses empty actions, removing the original recovery handler.

- [ ] 311. [tab_data.cpp] Listbox pasteselection and copytrimwhitespace hardcoded false
- **JS Source**: `src/js/modules/tab_data.js` lines 98–99
- **Status**: Pending
- **Details**: JS binds pasteselection to config.pasteSelection and copytrimwhitespace to config.removePathSpacesCopy. C++ hardcodes both to false.

- [ ] 312. [tab_data.cpp] load_table error toast missing View Log action button
- **JS Source**: `src/js/modules/tab_data.js` line 80
- **Status**: Pending
- **Details**: JS includes View Log action. C++ passes empty map.

- [ ] 313. [tab_data.cpp] Context menu labels are static instead of dynamic row count
- **JS Source**: `src/js/modules/tab_data.js` lines 108–110
- **Status**: Pending
- **Details**: JS renders "Copy N rows as CSV" with actual selectedCount and pluralization. C++ uses static labels losing the count.

- [ ] 314. [tab_data.cpp] Context menu node not cleared on close
- **JS Source**: `src/js/modules/tab_data.js` line 107
- **Status**: Pending
- **Details**: JS resets nodeDataTable to null on close. C++ never resets it so the condition stays true permanently.

- [ ] 315. [tab_data.cpp] copy_cell uses value.dump() producing JSON-quoted strings
- **JS Source**: `src/js/modules/tab_data.js` lines 172–177
- **Status**: Pending
- **Details**: JS uses String(value) producing unquoted output. C++ uses value.dump() adding extra quotes for strings.

- [ ] 316. [tab_data.cpp] Selection watcher prevents retry after failed load
- **JS Source**: `src/js/modules/tab_data.js` lines 371–377
- **Status**: Pending
- **Details**: JS compares selected_file which is not updated on failure allowing retry. C++ compares prev_selection_last updated unconditionally preventing retry.

- [ ] 317. [tab_data.cpp] Missing Regex Enabled indicators in both filter bars
- **JS Source**: `src/js/modules/tab_data.js` lines 102–103, 129–130
- **Status**: Pending
- **Details**: JS shows Regex Enabled div in both the DB2 filter bar and data table tray filter. C++ has no regex indicators.

- [ ] 318. [tab_data.cpp] helper.mark() calls missing stack trace argument
- **JS Source**: `src/js/modules/tab_data.js` lines 250, 314, 358
- **Status**: Pending
- **Details**: JS passes both e.message and e.stack. C++ only passes e.what().

- [ ] 319. [legacy_tab_data.cpp] Export format menu omits JS SQL/DBC options
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 172–176, 222–231
- **Status**: Pending
- **Details**: JS menu exposes `CSV`, `SQL`, and `DBC` export actions, but C++ `legacy_data_opts` only includes `Export as CSV`, making SQL/DBC exports unavailable through the settings menu path.

- [ ] 320. [legacy_tab_data.cpp] `copy_cell` empty-string handling differs from JS
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 215–220
- **Status**: Pending
- **Details**: JS copies any non-null/undefined value (including empty string), while C++ returns early on `value.empty()`, so empty-cell clipboard behavior is not equivalent.

- [ ] 321. [legacy_tab_data.cpp] DBC filename extraction uses `std::filesystem::path` which won't split backslash-delimited MPQ paths on Linux
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 33–36
- **Status**: Pending
- **Details**: JS uses `full_path.split('\\')` to extract the DBC filename from backslash-delimited MPQ paths. C++ uses `std::filesystem::path(full_path).filename()` (line 81). On Linux, `std::filesystem::path` treats `\` as a regular character, not a separator, so `filename()` would return the entire path string instead of just the filename. This would cause the table name extraction to fail for MPQ paths like `DBFilesClient\Achievement.dbc`.

- [ ] 324. [legacy_tab_data.cpp] Missing regex info display in DBC filter bar
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 134–135
- **Status**: Pending
- **Details**: JS has `<div class="regex-info" v-if="$core.view.config.regexFilters">Regex Enabled</div>` in the DBC listbox filter section. C++ DBC filter bar (lines 309–316) does not show the regex enabled indicator.

- [ ] 325. [legacy_tab_data.cpp] Context menu uses `ImGui::BeginPopupContextItem` vs JS ContextMenu component
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 139–143
- **Status**: Pending
- **Details**: JS uses the custom `ContextMenu` component with slot-based content rendering and a close event. C++ uses native `ImGui::BeginPopupContextItem` (line 368) which has different popup behavior, positioning, and styling compared to the custom ContextMenu component used elsewhere in the app.

## Tab: Raw Files & Legacy Files

- [ ] 326. [tab_raw.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_raw.js` line 158
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 327. [tab_raw.cpp] export_raw_files uses getVirtualFileByName and drops partialDecrypt=true
- **JS Source**: `src/js/modules/tab_raw.js` line 123
- **Status**: Pending
- **Details**: JS calls core.view.casc.getFileByName(file_name, true) passing partialDecrypt=true. C++ calls getVirtualFileByName(file_name) without partialDecrypt parameter, silently dropping partial decryption capability for encrypted files.

- [ ] 328. [tab_raw.cpp] export_raw_files error mark missing stack trace argument
- **JS Source**: `src/js/modules/tab_raw.js` line 128
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 330. [tab_raw.cpp] Missing placeholder text on filter input
- **JS Source**: `src/js/modules/tab_raw.js` line 159
- **Status**: Pending
- **Details**: JS has placeholder="Filter raw files...". C++ uses ImGui::InputText with no hint text.

- [ ] 331. [tab_raw.cpp] Missing tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_raw.js` line 158
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ has no tooltip.

- **JS Source**: `src/js/modules/tab_raw.js` lines 12, 31, 91
- **Status**: Pending
- **Details**: JS compute_raw_files, detect_raw_files, and export_raw_files are all async. C++ versions are synchronous, blocking render thread during CASC I/O and disk operations.

- [ ] 333. [tab_raw.cpp] detect_raw_files manually sets is_dirty=true — deviates from JS
- **JS Source**: `src/js/modules/tab_raw.js` lines 75–76
- **Status**: Pending
- **Details**: JS calls listfile.ingestIdentifiedFiles then compute_raw_files without setting is_dirty. Since is_dirty was false, JS would return early (apparent JS bug). C++ adds is_dirty=true to fix this, which is arguably correct but deviates from original JS behavior.

- [ ] 334. [legacy_tab_files.cpp] Listbox context menu includes extra FileDataID actions absent in JS
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 76–80
- **Status**: Pending
- **Details**: JS legacy-files menu only provides copy file path, copy export path, and open export directory; C++ conditionally adds listfile-format and fileDataID entries, changing context-menu behavior.

- [ ] 335. [legacy_tab_files.cpp] Layout doesn't use `app::layout` helpers — uses raw `ImGui::BeginChild`
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 72–89
- **Status**: Pending
- **Details**: Other legacy tabs (audio, fonts, textures, data) use `app::layout::BeginTab/EndTab`, `CalcListTabRegions`, `BeginListContainer`, etc. for consistent layout. `legacy_tab_files.cpp` uses raw `ImGui::BeginChild("legacy-files-list-container", ...)` (line 124) without the layout system. This will produce inconsistent sizing and positioning compared to sibling legacy tabs.

- [ ] 337. [legacy_tab_files.cpp] Tray layout structure differs from JS
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 82–88
- **Status**: Pending
- **Details**: JS wraps the filter and export button in a `#tab-legacy-files-tray` div with its own layout (likely flex row). C++ renders filter input, then `ImGui::SameLine()`, then the export button (lines 206–216). The proportions and alignment of filter vs button may not match the JS CSS-defined tray layout.

## Tab: Maps

- [ ] 338. [tab_maps.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_maps.js` line 302
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 339. [tab_maps.cpp] Hand-rolled MD5 instead of mbedTLS
- **JS Source**: `src/js/modules/tab_maps.js` line 914
- **Status**: Pending
- **Details**: C++ implements a full RFC 1321 MD5 from scratch instead of using mbedTLS MD API (mbedtls/md.h) as specified in project conventions.

- [ ] 340. [tab_maps.cpp] load_map_tile uses nearest-neighbor scaling instead of bilinear interpolation
- **JS Source**: `src/js/modules/tab_maps.js` lines 62–71
- **Status**: Pending
- **Details**: JS Canvas 2D drawImage performs bilinear interpolation when scaling. C++ uses nearest-neighbor sampling (integer coordinate snapping), making scaled minimap tiles look blockier/pixelated.

- [ ] 341. [tab_maps.cpp] load_map_tile uses blp.width instead of blp.scaledWidth
- **JS Source**: `src/js/modules/tab_maps.js` line 62
- **Status**: Pending
- **Details**: JS computes scale as size / blp.scaledWidth. C++ uses blp.width. If the BLP has a scaledWidth differing from raw width (e.g. mipmaps), the scaling factor will be wrong.

- [ ] 342. [tab_maps.cpp] load_wmo_minimap_tile ignores drawX/drawY and scaleX/scaleY positioning
- **JS Source**: `src/js/modules/tab_maps.js` lines 107–112
- **Status**: Pending
- **Details**: JS draws each tile at its specific offset (tile.drawX * output_scale, tile.drawY * output_scale) with scaled dimensions. C++ ignores drawX, drawY, scaleX, scaleY entirely — stretching all tiles to fill the full cell. Multi-tile compositing within a grid cell is completely broken.

- [ ] 343. [tab_maps.cpp] export_map_wmo_minimap uses max-alpha instead of source-over compositing
- **JS Source**: `src/js/modules/tab_maps.js` lines 721–733
- **Status**: Pending
- **Details**: JS Canvas 2D drawImage uses Porter-Duff source-over compositing. C++ export computes alpha as max(dst_alpha, src_alpha) instead of correct source-over formula.

- [ ] 344. [tab_maps.cpp] WDT file load is outside try-catch block
- **JS Source**: `src/js/modules/tab_maps.js` lines 433–434
- **Status**: Pending
- **Details**: In JS, getFileByName(wdt_path) is inside the try block. In C++, getVirtualFileByName(wdt_path) is BEFORE the try block. If WDT file doesn't exist, exception propagates uncaught.

- [ ] 345. [tab_maps.cpp] mapViewerHasWorldModel check differs from JS
- **JS Source**: `src/js/modules/tab_maps.js` lines 438–439
- **Status**: Pending
- **Details**: JS checks if (wdt.worldModelPlacement) — any non-null object is truthy. C++ checks if (worldModelPlacement.id != 0), which misses placement with id=0. Also affects has_global_wmo and export_map_wmo checks.

- [ ] 346. [tab_maps.cpp] Missing e.stack in all helper.mark error calls
- **JS Source**: `src/js/modules/tab_maps.js` lines 680, 759, 815, 848, 931, 1122
- **Status**: Pending
- **Details**: JS passes both e.message and e.stack to helper.mark. C++ only passes e.what(), omitting stack trace. Affects 6 export functions.

- **JS Source**: `src/js/modules/tab_maps.js` lines 49–980
- **Status**: Pending
- **Details**: Every async function (load_map_tile, load_wmo_minimap_tile, collect_game_objects, extract_height_data_from_tile, load_map, setup_wmo_minimap, all export functions, initialize) is synchronous C++. Long exports freeze the UI.

- [ ] 348. [tab_maps.cpp] Missing optional chaining for export_paths
- **JS Source**: `src/js/modules/tab_maps.js` lines 752–853
- **Status**: Pending
- **Details**: JS uses optional chaining export_paths?.writeLine() and export_paths?.close(). C++ calls directly without null checks. If openLastExportStream returns invalid object, C++ will crash.

- [ ] 350. [tab_maps.cpp] Missing regex tooltip
- **JS Source**: `src/js/modules/tab_maps.js` line 302
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" on regex-info div. C++ renders ImGui::TextDisabled("Regex Enabled") without any tooltip.

- [ ] 351. [tab_maps.cpp] Sidebar headers use SeparatorText instead of styled span
- **JS Source**: `src/js/modules/tab_maps.js` lines 313, 342, 352, 354
- **Status**: Pending
- **Details**: JS uses <span class="header"> which renders as bold text. C++ uses ImGui::SeparatorText() which draws a horizontal separator line with text — visually different.

- [ ] 352. [tab_maps.cpp] collect_game_objects returns vector instead of Set
- **JS Source**: `src/js/modules/tab_maps.js` lines 146–157
- **Status**: Pending
- **Details**: JS returns a Set guaranteeing uniqueness. C++ returns std::vector<ADTGameObject> which can contain duplicates.

- [ ] 353. [tab_maps.cpp] Selection watch may miss intermediate changes
- **JS Source**: `src/js/modules/tab_maps.js` lines 1135–1143
- **Status**: Pending
- **Details**: JS Vue $watch triggers on any reactive change. C++ only compares the first element string between frames. If selection changes and reverts within same frame, or changes to different item with same first entry, C++ misses it.

## Tab: Zones

- [ ] 354. [tab_zones.cpp] Default phase filtering excludes non-zero phases unlike JS
- **JS Source**: `src/js/modules/tab_zones.js` lines 78–79
- **Status**: Pending
- **Details**: JS includes all `UiMapXMapArt` links when `phase_id === null`; C++ filters to `PhaseID == 0` when no phase is selected.

- [ ] 355. [tab_zones.cpp] UiMapArtStyleLayer lookup key differs from JS relation logic
- **JS Source**: `src/js/modules/tab_zones.js` lines 88–90
- **Status**: Pending
- **Details**: JS resolves style layers by matching `UiMapArtStyleID` to `art_entry.UiMapArtStyleID`; C++ matches `UiMapArtID` to the linked art ID, changing style-layer association behavior.

- [ ] 356. [tab_zones.cpp] Base tile relation lookup uses layer-row ID instead of UiMapArt ID
- **JS Source**: `src/js/modules/tab_zones.js` lines 120–121
- **Status**: Pending
- **Details**: JS fetches `UiMapArtTile` relation rows with `art_style.ID` from the UiMapArt entry; C++ stores `CombinedArtStyle::id` as the UiMapArtStyleLayer row ID and uses that in `getRelationRows`, altering tile resolution.

- [ ] 357. [tab_zones.cpp] Base map tile OffsetX/OffsetY offsets are ignored
- **JS Source**: `src/js/modules/tab_zones.js` lines 181–182
- **Status**: Pending
- **Details**: JS applies `tile.OffsetX`/`tile.OffsetY` when placing map tiles; C++ calculates tile position from row/column and tile dimensions only.

- [ ] 358. [tab_zones.cpp] Zone listbox copy/paste trim options are hardcoded instead of config-bound
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: JS binds `copymode`, `pasteselection`, and `copytrimwhitespace` to config values; C++ hardcodes `CopyMode::Default`, `pasteselection=false`, and `copytrimwhitespace=false`.

- [ ] 359. [tab_zones.cpp] Phase selector placement differs from JS preview overlay layout
- **JS Source**: `src/js/modules/tab_zones.js` lines 341–349
- **Status**: Pending
- **Details**: JS renders the phase dropdown in a `preview-dropdown-overlay` inside the preview background; C++ renders phase selection in the bottom control bar.

- [ ] 360. [tab_zones.cpp] UiMapArtStyleLayer join uses wrong field name
- **JS Source**: `src/js/modules/tab_zones.js` lines 88–91
- **Status**: Pending
- **Details**: JS joins `art_style_layer.UiMapArtStyleID === art_entry.UiMapArtStyleID`. C++ joins on `layer_row["UiMapArtID"]` — a completely different field name. The C++ looks for "UiMapArtID" in UiMapArtStyleLayer table, but JS matches on "UiMapArtStyleID" from both tables. This produces wrong rows or no rows.

- [ ] 361. [tab_zones.cpp] CombinedArtStyle.id stores wrong ID (layer ID vs art ID)
- **JS Source**: `src/js/modules/tab_zones.js` lines 94–101
- **Status**: Pending
- **Details**: JS `combined_style` includes `...art_entry` (spread), so `combined_style.ID` = the UiMapArt row ID (`art_id`). C++ sets `style.id = static_cast<int>(layer_id)` which is the UiMapArtStyleLayer table key. This wrong ID propagates to `getRelationRows()` calls for UiMapArtTile and WorldMapOverlay.

- [ ] 362. [tab_zones.cpp] C++ adds ALL matching style layers; JS keeps only LAST
- **JS Source**: `src/js/modules/tab_zones.js` lines 86–91
- **Status**: Pending
- **Details**: JS declares `let style_layer;` then overwrites in a loop, keeping only the last match. C++ `push_back`s every matching row into `art_styles`. This creates duplicate/extra entries causing redundant or incorrect rendering.

- [ ] 363. [tab_zones.cpp] Phase filter logic differs when phase_id is null
- **JS Source**: `src/js/modules/tab_zones.js` line 78
- **Status**: Pending
- **Details**: JS: `if (phase_id === null || link_entry.PhaseID === phase_id)` — when phase_id is null, ALL entries are included. C++: when `phase_id` is nullopt, only entries with `row_phase == 0` are included. C++ omits non-default phases when no phase is specified, while JS shows all.

- [ ] 364. [tab_zones.cpp] Missing tile OffsetX/OffsetY in render_map_tiles
- **JS Source**: `src/js/modules/tab_zones.js` lines 181–182
- **Status**: Pending
- **Details**: JS: `final_x = pixel_x + (tile.OffsetX || 0); final_y = pixel_y + (tile.OffsetY || 0)`. C++ only uses `pixel_x = col * tile_width; pixel_y = row * tile_height` with no offset. Tiles with non-zero offsets will be mispositioned.

- [ ] 365. [tab_zones.cpp] Tile layer rendering architecture differs from JS
- **JS Source**: `src/js/modules/tab_zones.js` lines 126–152
- **Status**: Pending
- **Details**: JS groups ALL tiles for an art_style by their LayerIndex, then renders each group in sorted order. C++ calls `render_map_tiles(art_style, art_style.layer_index, ...)` which filters tiles to only those matching the single layer_index. Combined with the duplicate style layers issue, rendering pipeline differs significantly.

- [ ] 366. [tab_zones.cpp] parse_zone_entry doesn't throw on bad input
- **JS Source**: `src/js/modules/tab_zones.js` lines 17–18
- **Status**: Pending
- **Details**: JS throws `new Error('unexpected zone entry')` on regex mismatch. C++ returns an empty `ZoneDisplayInfo{}` with `id=0`. Callers add `zone.id > 0` guards, but error propagation differs.

- [ ] 367. [tab_zones.cpp] UiMap row existence not validated
- **JS Source**: `src/js/modules/tab_zones.js` lines 67–71
- **Status**: Pending
- **Details**: JS checks `if (!map_data)` and throws `'UiMap entry not found'`. C++ fetches the row but casts to void: `(void)ui_map_row_opt;` — never checks the result or throws.

- [ ] 368. [tab_zones.cpp] Pixel buffer not cleared at start of render when first layer is non-zero
- **JS Source**: `src/js/modules/tab_zones.js` line 59
- **Status**: Pending
- **Details**: JS calls `ctx.clearRect(0, 0, canvas.width, canvas.height)` at the start. C++ only allocates/clears the pixel buffer inside the `if (art_style.layer_index == 0)` block. If the first art_style has layer_index != 0, stale pixel data remains.

- [ ] 369. [tab_zones.cpp] Listbox copyMode hardcoded instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ passes `listbox::CopyMode::Default` instead of reading from `view.config["copyMode"]`.

- [ ] 370. [tab_zones.cpp] Listbox pasteSelection hardcoded false instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ hardcodes `false` instead of reading `view.config["pasteSelection"]`.

- [ ] 371. [tab_zones.cpp] Listbox copytrimwhitespace hardcoded false instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ hardcodes `false` instead of reading `view.config["removePathSpacesCopy"]`.

- [ ] 372. [tab_zones.cpp] export_zone_map helper.mark missing stack trace
- **JS Source**: `src/js/modules/tab_zones.js` line 491
- **Status**: Pending
- **Details**: JS: `helper.mark(zone_entry, false, e.message, e.stack)` — passes both message and stack. C++: `helper.mark(..., false, e.what())` — only passes message, no stack trace.

- [ ] 373. [tab_zones.cpp] Phase dropdown placed in control bar instead of preview overlay
- **JS Source**: `src/js/modules/tab_zones.js` lines 341–347
- **Status**: Pending
- **Details**: JS puts the phase `<select>` inside `preview-dropdown-overlay` div overlaid on the zone canvas. C++ places the `ImGui::BeginCombo` in the bottom controls bar alongside checkboxes/button. This is a layout difference.

- [ ] 374. [tab_zones.cpp] Missing regex tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_zones.js` line 325
- **Status**: Pending
- **Details**: JS shows a tooltip on "Regex Enabled" via `:title="$core.view.regexTooltip"`. C++ renders `ImGui::TextUnformatted("Regex Enabled")` with no tooltip.

- [ ] 375. [tab_zones.cpp] EXPANSION_NAMES static vector is dead code
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: `EXPANSION_NAMES` vector is defined but never referenced. The actual expansion rendering uses `constants::EXPANSIONS`. Should be removed.

- [ ] 376. [tab_zones.cpp] ZoneDisplayInfo vs ZoneEntry naming mismatch with header
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: The header declares `ZoneEntry` struct but the cpp defines a separate `ZoneDisplayInfo` struct for `parse_zone_entry`. The header's `ZoneEntry` appears unused.

- [ ] 377. [tab_zones.cpp] Missing per-tile position logging in render_map_tiles
- **JS Source**: `src/js/modules/tab_zones.js` lines 184–185
- **Status**: Pending
- **Details**: JS logs `'rendering tile FileDataID %d at position (%d,%d) -> (%d,%d) [Layer %d]'` for each tile. C++ has no per-tile log.

- [ ] 378. [tab_zones.cpp] Missing "no tiles found" log for art style
- **JS Source**: `src/js/modules/tab_zones.js` lines 121–123
- **Status**: Pending
- **Details**: JS logs `'no tiles found for UiMapArt ID %d'` and `continue`s. C++ has no equivalent check/log.

- [ ] 379. [tab_zones.cpp] Missing "no overlays found" log
- **JS Source**: `src/js/modules/tab_zones.js` lines 212–214
- **Status**: Pending
- **Details**: JS logs `'no WorldMapOverlay entries found for UiMapArt ID %d'` when overlays array is empty. C++ has no such log.

- [ ] 380. [tab_zones.cpp] Missing "no overlay tiles" log per overlay
- **JS Source**: `src/js/modules/tab_zones.js` lines 219–222
- **Status**: Pending
- **Details**: JS logs `'no tiles found for WorldMapOverlay ID %d'` and `continue`s for empty tile sets. C++ calls `render_overlay_tiles` regardless.

- [ ] 381. [tab_zones.cpp] Unsafe Windows wstring conversion corrupts multi-byte UTF-8 paths
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: `std::wstring wpath(dir.begin(), dir.end())` does byte-by-byte copy which corrupts multi-byte UTF-8 paths. Should use `MultiByteToWideChar` or equivalent.

- [ ] 382. [tab_zones.cpp] Linux shell command injection risk in openInExplorer
- **JS Source**: `src/js/modules/tab_zones.js` line 393
- **Status**: Pending
- **Details**: `"xdg-open \"" + dir + "\" &"` passed to `std::system()`. If `dir` contains shell metacharacters, this is exploitable. JS uses `nw.Shell.openItem` which is safe.

## Tab: Items & Item Sets

- [ ] 383. [tab_items.cpp] Wowhead item handler is stubbed out
- **JS Source**: `src/js/modules/tab_items.js` lines 322–324
- **Status**: Pending
- **Details**: JS calls `ExternalLinks.wowHead_viewItem(item_id)` from the context action; C++ `view_on_wowhead(...)` immediately returns and does nothing.

- [ ] 384. [tab_items.cpp] Item sidebar checklist interaction/layout diverges from JS clickable row design
- **JS Source**: `src/js/modules/tab_items.js` lines 254–266
- **Status**: Pending
- **Details**: JS uses `.sidebar-checklist-item` rows with selected-state styling and row-level click toggling; C++ renders plain ImGui checkboxes, changing sidebar visuals and interaction feel.

- [ ] 385. [tab_items.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_items.js` line 248
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 386. [tab_items.cpp] view_on_wowhead is stubbed — does nothing
- **JS Source**: `src/js/modules/tab_items.js` lines 322–324
- **Status**: Pending
- **Details**: JS calls ExternalLinks.wowHead_viewItem(item_id) which opens a Wowhead URL. C++ function is { return; } — a no-op. external_links.h already provides wowHead_viewItem().

- [ ] 387. [tab_items.cpp] copy_to_clipboard bypasses core.view.copyToClipboard
- **JS Source**: `src/js/modules/tab_items.js` lines 318–320
- **Status**: Pending
- **Details**: JS calls this.$core.view.copyToClipboard(value) which may have additional behavior (e.g. toast notification). C++ calls ImGui::SetClipboardText() directly, skipping view layer.

- [ ] 388. [tab_items.cpp] std::set ordering differs from JS Set insertion order
- **JS Source**: `src/js/modules/tab_items.js` lines 85–127
- **Status**: Pending
- **Details**: view_item_models and view_item_textures use JS Set which preserves insertion order. C++ uses std::set<std::string> which sorts lexicographically. Should use std::vector with uniqueness check.

- [ ] 389. [tab_items.cpp] Second loop (itemViewerShowAll) retrieves item name from wrong source
- **JS Source**: `src/js/modules/tab_items.js` lines 181–211
- **Status**: Pending
- **Details**: JS constructs new Item(item_id, item_row, null, null, null) where Item constructor reads item_row.Display_lang. C++ looks up name via DBItems::getItemById(item_id), a different data source.

- [ ] 390. [tab_items.cpp] Regex tooltip not rendered
- **JS Source**: `src/js/modules/tab_items.js` line 248
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" on "Regex Enabled" div. C++ renders ImGui::TextUnformatted("Regex Enabled") without any tooltip.

- [ ] 391. [tab_items.cpp] Sidebar headers use SeparatorText instead of styled header span
- **JS Source**: `src/js/modules/tab_items.js` lines 252, 262
- **Status**: Pending
- **Details**: JS uses <span class="header">Item Types</span> which renders as styled header text. C++ uses ImGui::SeparatorText() which draws a horizontal separator line with text — visually different.

- [ ] 392. [tab_items.cpp] Sidebar checklist items lack .selected class visual feedback
- **JS Source**: `src/js/modules/tab_items.js` lines 254–257
- **Status**: Pending
- **Details**: JS adds :class="{ selected: item.checked }" to checklist items. CSS gives .sidebar-checklist-item.selected a background of rgba(255,255,255,0.05). C++ uses plain ImGui::Checkbox with no highlight.

- **JS Source**: `src/js/modules/tab_items.js` lines 104, 129, 277, 345–346
- **Status**: Pending
- **Details**: JS initialize_items, view_item_textures, and the mounted initialize flow are all async with await. C++ converts all to synchronous blocking calls, freezing the ImGui render loop.

- [ ] 394. [tab_items.cpp] Quality color applied only to CheckMark, not to label text
- **JS Source**: `src/js/modules/tab_items.js` lines 264–265
- **Status**: Pending
- **Details**: CSS accent-color applies quality color to the checkbox. C++ pushes ImGuiCol_CheckMark only coloring the checkmark glyph. The checkbox background/frame and label text are unaffected.

- [ ] 395. [tab_items.cpp] Filter input buffer limited to 256 bytes
- **JS Source**: `src/js/modules/tab_items.js` line 249
- **Status**: Pending
- **Details**: JS input has no character limit. C++ uses char filter_buf[256] with std::strncpy, silently truncating beyond 255 characters.

- [ ] 396. [tab_item_sets.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_item_sets.js` line 82
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ shows plain `Regex Enabled` text without tooltip behavior.

- [ ] 397. [tab_item_sets.cpp] Missing filter input placeholder text
- **JS Source**: `src/js/modules/tab_item_sets.js` line 83
- **Status**: Pending
- **Details**: JS uses placeholder="Filter item sets..." on the text input. C++ uses ImGui::InputText without hint/placeholder text.

- [ ] 398. [tab_item_sets.cpp] Missing regex tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_item_sets.js` line 82
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ just renders ImGui::TextUnformatted("Regex Enabled") with no tooltip.

- **JS Source**: `src/js/modules/tab_item_sets.js` lines 23–65
- **Status**: Pending
- **Details**: JS initialize_item_sets is async with await for progressLoadingScreen(), DBItems.ensureInitialized(), db2 getAllRows(). C++ calls all synchronously, blocking the UI thread and preventing loading screen updates.

- [ ] 400. [tab_item_sets.cpp] apply_filter converts ItemSet structs to JSON objects unnecessarily
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 67–69
- **Status**: Pending
- **Details**: JS simply assigns the array of ItemSet objects directly. C++ iterates every ItemSet, constructs nlohmann::json objects, and pushes them. render() then converts JSON back into ItemEntry structs every frame — double-conversion overhead.

- [ ] 401. [tab_item_sets.cpp] render() re-creates item_entries vector from JSON every frame
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 76–86
- **Status**: Pending
- **Details**: C++ render allocates a vector, loops over all JSON items, copies fields into ItemEntry structs, and pushes — every frame. JS template binds directly to existing objects with no per-frame allocation.

- [ ] 402. [tab_item_sets.cpp] Regex-enabled text and filter input lack proper layout container
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 81–84
- **Status**: Pending
- **Details**: JS wraps regex info and filter input inside div class="filter" providing inline layout. C++ renders them sequentially without SameLine() or horizontal group, causing "Regex Enabled" to appear above the filter input instead of beside it.

- [ ] 403. [tab_item_sets.cpp] fieldToUint32Vec does not handle single-value fields
- **JS Source**: `src/js/modules/tab_item_sets.js` line 38
- **Status**: Pending
- **Details**: JS set_row.ItemID is expected to be an array with .filter(id => id !== 0). C++ fieldToUint32Vec only handles vector variants. If a DB2 reader returns a single scalar, the function returns an empty vector, silently dropping data.

## Tab: Characters

## Tab: Creatures & Decor

- [ ] 427. [tab_creatures.cpp] Creature list context actions are not equivalent to JS copy-name/copy-ID menu
- **JS Source**: `src/js/modules/tab_creatures.js` lines 983–986, 1179–1203
- **Status**: Pending
- **Details**: JS creature list context menu exposes only `Copy name(s)` and `Copy ID(s)` handlers; C++ delegates to generic `listbox_context::handle_context_menu(...)`, changing the context action contract from the original creature-specific menu.

- [ ] 428. [tab_creatures.cpp] has_content check and toast/camera logic scoped incorrectly
- **JS Source**: `src/js/modules/tab_creatures.js` lines 713–722
- **Status**: Pending
- **Details**: In JS, the has_content check, hideToast, and fitCamera are outside the if/else running for both character and standard models. In C++ this block is inside the else (standard-model only). For character-model creatures, the loading toast is never dismissed.

- [ ] 429. [tab_creatures.cpp] Collection model geoset logic has three bugs
- **JS Source**: `src/js/modules/tab_creatures.js` lines 421–429
- **Status**: Pending
- **Details**: (1) JS calls hideAllGeosets() before applying - C++ never does. (2) JS uses mapping.group_index for lookup - C++ uses coll_idx. (3) JS uses mapping.char_geoset for setGeosetGroupDisplay - C++ uses mapping.group_index.

- [ ] 430. [tab_creatures.cpp] Scrubber IsItemActivated() called before SliderInt checks wrong widget
- **JS Source**: `src/js/modules/tab_creatures.js` lines 1035–1038
- **Status**: Pending
- **Details**: C++ calls IsItemActivated() before SliderInt() renders. IsItemActivated checks the last widget (Step-Right button) not the slider. start_scrub() will never fire correctly.

- [ ] 431. [tab_creatures.cpp] Missing export_paths.writeLine calls in multiple export paths
- **JS Source**: `src/js/modules/tab_creatures.js` lines 747, 792, 923
- **Status**: Pending
- **Details**: JS writes to export_paths for RAW and non-RAW character-model export and standard export. C++ omits all writeLine calls and does not pass export_paths to export_model.

- [ ] 432. [tab_creatures.cpp] GLTF format.toLowerCase() not applied
- **JS Source**: `src/js/modules/tab_creatures.js` line 921
- **Status**: Pending
- **Details**: JS passes format.toLowerCase() to exportAsGLTF. C++ passes format as-is (uppercase). If the exporter is case-sensitive output may differ.

- [ ] 433. [tab_creatures.cpp] Error toast for model load missing View Log action button
- **JS Source**: `src/js/modules/tab_creatures.js` line 728
- **Status**: Pending
- **Details**: JS passes View Log action. C++ passes empty map. User cannot open runtime log from this error.

- [ ] 434. [tab_creatures.cpp] path.basename behavior not replicated in skin name
- **JS Source**: `src/js/modules/tab_creatures.js` line 668
- **Status**: Pending
- **Details**: Node.js path.basename produces trailing dot. C++ strips full .m2 extension producing no dot. Skin name stripping matches different substrings producing different display labels.

- [ ] 435. [tab_creatures.cpp] Missing Regex Enabled indicator in filter bar
- **JS Source**: `src/js/modules/tab_creatures.js` line 989
- **Status**: Pending
- **Details**: JS shows Regex Enabled div with tooltip when regex filters are active. C++ filter bar has no indicator.

- [ ] 436. [tab_creatures.cpp] Listbox context menu Copy names and Copy IDs not rendered in UI
- **JS Source**: `src/js/modules/tab_creatures.js` lines 983–986
- **Status**: Pending
- **Details**: JS renders ContextMenu with Copy names and Copy IDs options. C++ does not render an ImGui context menu popup. The functions exist but are never invoked from the UI.

- [ ] 437. [tab_creatures.cpp] Sorting uses byte comparison instead of locale-aware localeCompare
- **JS Source**: `src/js/modules/tab_creatures.js` line 1161
- **Status**: Pending
- **Details**: JS uses localeCompare. C++ uses name_a < name_b. Creatures with diacritics may sort differently.

- [ ] 438. [tab_decor.cpp] PNG/CLIPBOARD export branch does not short-circuit like JS
- **JS Source**: `src/js/modules/tab_decor.js` lines 129–140
- **Status**: Pending
- **Details**: JS returns immediately after preview export for PNG/CLIPBOARD; C++ closes the export stream but continues into full model export logic, changing export behavior for these formats.

- [ ] 439. [tab_decor.cpp] Decor list context menu open/interaction path differs from JS ContextMenu component flow
- **JS Source**: `src/js/modules/tab_decor.js` lines 234–237
- **Status**: Pending
- **Details**: JS renders a dedicated ContextMenu node for listbox selections (`Copy name(s)` / `Copy file data ID(s)`); C++ uses a manual popup path without equivalent Vue component lifecycle/wiring, deviating from original interaction flow.

- [ ] 440. [tab_decor.cpp] Missing return after PNG/CLIPBOARD export branch falls through
- **JS Source**: `src/js/modules/tab_decor.js` lines 138–140
- **Status**: Pending
- **Details**: JS does return after PNG/CLIPBOARD block. C++ has no return so execution falls through to the ExportHelper loop redundantly exporting all selected entries as models.

- [ ] 441. [tab_decor.cpp] create_renderer receives file_data_id instead of file_name
- **JS Source**: `src/js/modules/tab_decor.js` line 85
- **Status**: Pending
- **Details**: JS passes file_name (string) as 5th argument to create_renderer. C++ passes file_data_id (uint32_t). Parameter type mismatch.

- [ ] 442. [tab_decor.cpp] getActiveRenderer() only returns M2 renderer not any active renderer
- **JS Source**: `src/js/modules/tab_decor.js` line 611
- **Status**: Pending
- **Details**: JS getActiveRenderer returns the single active_renderer which could be M2, WMO, or M3. C++ returns active_renderer_result.m2.get() returning nullptr when active model is WMO or M3.

- [ ] 443. [tab_decor.cpp] Error toast for preview_decor missing View Log action button
- **JS Source**: `src/js/modules/tab_decor.js` line 119
- **Status**: Pending
- **Details**: JS includes View Log action. C++ passes empty map.

- [ ] 444. [tab_decor.cpp] helper.mark on failure missing stack trace parameter
- **JS Source**: `src/js/modules/tab_decor.js` line 184
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

- [ ] 445. [tab_decor.cpp] Sorting uses byte comparison instead of locale-aware localeCompare
- **JS Source**: `src/js/modules/tab_decor.js` lines 401–405
- **Status**: Pending
- **Details**: JS uses localeCompare. C++ uses name_a < name_b after tolower. Different sort for non-ASCII names.

- [ ] 446. [tab_decor.cpp] Missing scrub pause/resume behavior on animation slider
- **JS Source**: `src/js/modules/tab_decor.js` lines 546–561
- **Status**: Pending
- **Details**: JS start_scrub saves paused state and pauses animation while dragging. C++ SliderInt has no mouse-down/up event handling.

- [ ] 447. [tab_decor.cpp] Missing Regex Enabled indicator in filter bar
- **JS Source**: `src/js/modules/tab_decor.js` line 240
- **Status**: Pending
- **Details**: JS shows Regex Enabled div above filter input. C++ filter bar has no such indicator.

- [ ] 448. [tab_decor.cpp] Missing tooltips on all sidebar Preview and Export checkboxes
- **JS Source**: `src/js/modules/tab_decor.js` lines 314–354
- **Status**: Pending
- **Details**: JS has title attributes for every checkbox. C++ has no tooltip calls for any of them.

- [ ] 449. [tab_decor.cpp] Category group header click-to-toggle-all not implemented
- **JS Source**: `src/js/modules/tab_decor.js` line 301
- **Status**: Pending
- **Details**: JS clicking category name toggles all subcategories on/off. C++ uses TreeNodeEx which only expands/collapses. The toggle function exists but is never called from render.

- [ ] 450. [tab_decor.cpp] WMO Groups and Doodad Sets use manual checkbox loop instead of CheckboxList
- **JS Source**: `src/js/modules/tab_decor.js` lines 363–369
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for both. C++ uses manual ImGui::Checkbox loops instead of checkboxlist::render(). Inconsistent and may cause visual differences.

- [ ] 451. [tab_decor.cpp] Context menu popup may never open
- **JS Source**: `src/js/modules/tab_decor.js` lines 233–237
- **Status**: Pending
- **Details**: The DecorListboxContextMenu popup requires ImGui::OpenPopup to be called. The handle_listbox_context callback does not open this popup. The popup rendering code will never trigger.

## 3D Engine, File Loaders & Core Systems

- **JS Source**: `src/js/casc/blp.js` lines 146–194
- **Status**: Pending
- **Details**: JS implements `async saveToPNG`, `async toWebP`, and `async saveToWebP`. C++ equivalents are synchronous, changing completion/error semantics for consumers expecting Promise-based behavior.

- **JS Source**: `src/js/casc/blp.js` lines 242–250
- **Status**: Pending
- **Details**: JS has no default branch and therefore returns `undefined` for unsupported encodings. C++ returns an empty `BufferWrapper`, changing caller-observed fallback behavior.

- **JS Source**: `src/js/casc/blp.js` lines 103–117, 221–234
- **Status**: Pending
- **Details**: JS `toCanvas()` creates an HTML `<canvas>` element and draws the BLP onto it. `drawToCanvas()` takes an existing canvas and draws the BLP pixels using 2D context methods (`createImageData`, `putImageData`). These are browser-specific APIs with no C++ equivalent. The C++ port replaces these with `toPNG()`, `toBuffer()`, and `toUInt8Array()` which provide the same pixel data without canvas.

- **JS Source**: `src/js/casc/blp.js` line 86
- **Status**: Pending
- **Details**: JS sets `this.dataURL = null` in the BLPImage constructor. C++ declares `std::optional<std::string> dataURL` in the header which defaults to `std::nullopt`. The C++ `getDataURL()` method doesn't cache to this field (it relies on `BufferWrapper::getDataURL()` caching instead). The JS `getDataURL()` also doesn't set this field — it returns from `toCanvas().toDataURL()`. The `dataURL` field appears to be unused caching infrastructure in both versions.

- **JS Source**: `src/js/casc/blp.js` lines 157–182
- **Status**: Pending
- **Details**: JS uses `webp-wasm` npm module with `webp.encode(imgData, options)` for WebP encoding. C++ uses libwebp's C API directly (`WebPEncodeLosslessRGBA` / `WebPEncodeRGBA`). The JS `options` object `{ lossless: true }` or `{ quality: N }` maps to C++ separate code paths for quality == 100 (lossless) vs lossy. Functionally equivalent.

- [ ] 457. [Shaders.cpp] C++ adds automatic _unregister_fn callback on ShaderProgram not present in JS
- **JS Source**: `src/js/3D/Shaders.js` lines 56–72
- **Status**: Pending
- **Details**: C++ `create_program()` (Shaders.cpp lines 79–83) installs a static `_unregister_fn` callback on `gl::ShaderProgram` that automatically calls `shaders::unregister()` when a ShaderProgram is destroyed. JS has no equivalent auto-cleanup mechanism — callers must explicitly call `unregister(program)` (Shaders.js line 78–86). This means in C++, a program is automatically removed from `active_programs` on destruction, while in JS a disposed program remains in the set until manually unregistered. This changes `reload_all()` behavior: JS could attempt to recompile stale programs that were not explicitly unregistered, while C++ never encounters this scenario.

- **JS Source**: `src/js/3D/Texture.js` lines 32–41
- **Status**: Pending
- **Details**: JS returns a Promise from `async getTextureFile()` and yields `null` when unset; C++ returns `std::optional<BufferWrapper>` synchronously, changing both async behavior and API shape.

- **JS Source**: `src/js/3D/Skin.js` lines 20–23, 96–100
- **Status**: Pending
- **Details**: JS exposes `async load()` and awaits CASC file retrieval (`await core.view.casc.getFile(...)`), while C++ `Skin::load()` is synchronous and throws directly, changing caller timing/error-propagation semantics.

- **JS Source**: `src/js/MultiMap.js` lines 6–32
- **Status**: Pending
- **Details**: The JS sibling contains the full `MultiMap extends Map` implementation, but `src/js/MultiMap.cpp` only includes `MultiMap.h` and comments; line-by-line implementation parity is not present in the `.cpp` file itself.

- **JS Source**: `src/js/MultiMap.js` lines 6, 20–28, 32
- **Status**: Pending
- **Details**: JS exports an actual `Map` subclass with standard `Map` behavior/interop, while C++ exposes a template wrapper (header implementation) returning `std::variant` pointers and not `Map`-equivalent runtime semantics.

- **JS Source**: `src/js/3D/loaders/M3Loader.js` lines 67, 104, 269, 277, 299, 315
- **Status**: Pending
- **Details**: JS exposes async `load`, `parseChunk_M3DT`, and async sub-chunk parsers; C++ ports these paths as synchronous calls, changing API timing/await semantics.

- **JS Source**: `src/js/3D/loaders/MDXLoader.js` line 28
- **Status**: Pending
- **Details**: JS exposes `async load()` while C++ exposes synchronous `void load()`, changing await/timing behavior.

- **JS Source**: `src/js/3D/loaders/MDXLoader.js` line 404
- **Status**: Pending
- **Details**: JS ATCH handler has `this.data.readUInt32LE(-4)` which is a bug — `BufferWrapper._readInt` passes `_checkBounds(-16)` (always passes since remainingBytes >= 0 > -16), but `new Array(-4)` throws a `RangeError`. C++ correctly fixes this by using a saved `attachmentSize` variable. The fix has a code comment but per project conventions, deviations from the original JS should also be tracked in TODO_TRACKER.md.

- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 36, 308, 407
- **Status**: Pending
- **Details**: JS exposes async `load`, `loadAnimsForIndex`, and `loadAnims`; C++ ports all three as synchronous methods, altering call/await behavior.

- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 332–344, 438–448
- **Status**: Pending
- **Details**: JS does not catch ANIM/CASC load failures in `loadAnimsForIndex`/`loadAnims` (Promise rejects). C++ catches exceptions, logs, and returns/continues, changing failure propagation.

- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 308–312
- **Status**: Pending
- **Details**: C++ adds `if (animation_index >= this->animations.size()) return false;` that does not exist in JS. In JS, accessing an out-of-bounds index on `this.animations` returns `undefined`, and `animation.flags` would throw a TypeError. C++ silently returns false instead of throwing, changing error behavior.

- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 335–338, 441–444
- **Status**: Pending
- **Details**: JS checks `loader.skeletonBoneData !== undefined` — the property only exists if a SKID chunk was parsed. C++ checks `!loader->skeletonBoneData.empty()`. If ANIMLoader ever sets `skeletonBoneData` to a valid but empty buffer, JS would use it (property exists), but C++ would skip it (empty). This is a potential semantic difference depending on ANIMLoader behavior.

- **JS Source**: `src/js/3D/loaders/BONELoader.js` line 24
- **Status**: Pending
- **Details**: JS exposes `async load()` while C++ exposes synchronous `void load()`, changing API timing/await semantics.

- **JS Source**: `src/js/3D/loaders/ANIMLoader.js` line 25
- **Status**: Pending
- **Details**: JS exposes `async load(isChunked = true)` while C++ exposes synchronous `void load(bool isChunked)`, changing API timing/await semantics.

- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 37, 64
- **Status**: Pending
- **Details**: JS exposes async `load()` and `getGroup(index)` while C++ ports both as synchronous methods, changing await/timing behavior.

- **JS Source**: `src/js/3D/loaders/WMOLoader.js` line 361
- **Status**: Pending
- **Details**: JS MOGP handler stores `this.flags = data.readUInt32LE()`. C++ stores this as `this->groupFlags` (header line 218, MOGP parser line 426) because the C++ class already has `uint16_t flags` from MOHD. Any downstream code porting JS that accesses `wmoGroup.flags` for MOGP flags must use `groupFlags` in C++. This naming deviation matches the same issue found in WMOLegacyLoader.cpp (entry 376).

- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 328–338
- **Status**: Pending
- **Details**: JS simply assigns `this.liquid = { ... }` in the MLIQ handler. Consumer code checks `if (this.liquid)` for existence. In C++, the `WMOLiquid liquid` member is always default-constructed, so a `bool hasLiquid = false` flag (header line 209) was added and set to `true` in `parse_MLIQ`. This is a reasonable C++ adaptation, but all downstream JS code that checks `if (this.liquid)` must be ported to check `if (this.hasLiquid)` instead — all consumers need verification.

- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 33, 54, 86, 116
- **Status**: Pending
- **Details**: JS uses async `load`, `_load_alpha_format`, `_load_standard_format`, and `getGroup`; C++ ports these paths synchronously, changing await/timing behavior.

- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 146–149
- **Status**: Pending
- **Details**: JS creates group loaders with `fileID` undefined and explicitly seeds `group.version = this.version` before `await group.load()`. C++ does not pre-seed `version`, changing legacy group parse assumptions.

- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` line 453
- **Status**: Pending
- **Details**: JS MOGP handler stores `this.flags = data.readUInt32LE()`. C++ stores this as `this->groupFlags` (header line 124, MOGP parser line 527) because the C++ class already has `uint16_t flags` from MOHD. Any downstream JS-ported code accessing `group.flags` for MOGP flags must use `group.groupFlags` in C++, which is a naming deviation that could cause porting bugs.

- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 117–118
- **Status**: Pending
- **Details**: JS checks `if (!this.groups)` — tests whether the `groups` property was ever set (by MOHD handler). An empty JS array `new Array(0)` is truthy, so `!this.groups` is false when `groupCount == 0` — `getGroup` proceeds to the index check. C++ uses `if (this->groups.empty())` which returns true for `groupCount == 0`, incorrectly throwing the exception. A separate bool flag (e.g., `groupsInitialized`) would replicate JS semantics more faithfully.

- **JS Source**: `src/js/3D/loaders/WDTLoader.js` line 86
- **Status**: Pending
- **Details**: JS uses `.replace('\0', '')` (first match only), while C++ removes all `'\0'` bytes from the string, producing different `worldModel` values in edge cases.

- **JS Source**: `src/js/3D/loaders/WDTLoader.js` lines 52–103
- **Status**: Pending
- **Details**: In JS, `this.worldModelPlacement` is only assigned when MODF is encountered. If MODF is absent, the property is `undefined` and `if (wdt.worldModelPlacement)` is false. In C++, `WDTWorldModelPlacement worldModelPlacement` is always default-constructed with zeroed fields, making it impossible to distinguish "MODF absent" from "MODF with zeros." Same for `worldModel` (always empty string vs. JS `undefined`) and MPHD fields (always 0 vs. JS `undefined`). Consider `std::optional<T>` for these fields.

- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 309–367
- **Status**: Pending
- **Details**: JS `export()` is asynchronous and yields between CASC/file operations; C++ `exportTile()` performs the flow synchronously, changing timing/cancellation behavior relative to the original async path.

- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 49, 105, 114, 170, 189, 231, 282
- **Status**: Pending
- **Details**: JS defines `init`, `reset`, `setTextureTarget`, `loadTexture`, `loadTextureFromBLP`, `compileShaders`, and `update` as async/await flows. C++ ports these methods synchronously, changing timing/error-propagation behavior expected by async call sites.

- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 56, 76
- **Status**: Pending
- **Details**: JS defines async `load` and `loadLOD`; C++ ports both as synchronous calls, changing await/timing semantics.

- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 174–175
- **Status**: Pending
- **Details**: JS checks `if (!this.m3 || !this.m3.vertices) return null`. C++ only checks `if (!m3) return std::nullopt` at line 198–199 without checking if vertices array is empty. If m3 is loaded but vertices array is empty, C++ will attempt bounding box calculation on empty data.

- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 174, 200, 407
- **Status**: Pending
- **Details**: JS uses async `load`, `_load_textures`, and `playAnimation`; C++ ports these paths synchronously, changing asynchronous control flow and failure timing.

- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 256–264
- **Status**: Pending
- **Details**: JS compares raw `nodes[i].objectId` and can propagate undefined/NaN semantics. C++ uses `std::optional<int>` checks and skips undefined IDs, which changes edge-case matrix-index behavior from JS.

- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 187–188
- **Status**: Pending
- **Details**: JS `load()` sets up Vue watchers: `this.geosetWatcher = core.view.$watch(this.geosetKey, () => this.updateGeosets(), { deep: true })` and `this.wireframeWatcher = core.view.$watch('config.modelViewerWireframe', () => {}, { deep: true })`. C++ completely omits these watchers. Comment at lines 228–229 states "polling is handled in render()." but no polling code exists.

- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 780–781
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.geosetWatcher?.()` and `this.wireframeWatcher?.()`. C++ has no equivalent cleanup because watchers were never created.

- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 252
- **Status**: Pending
- **Details**: JS sets `this.node_matrices = new Float32Array(16)` which creates a zero-filled 16-element array (single identity-sized buffer). C++ does `node_matrices.resize(16)` at line 313 which leaves elements uninitialized. Should zero-initialize or set to identity to match JS behavior.

- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 681
- **Status**: Pending
- **Details**: Same issue as other renderers — C++ uses elapsed time from first render call instead of `performance.now() * 0.001`.

- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 27–30
- **Status**: Pending
- **Details**: Both files define `INTERP_NONE=0`, `INTERP_LINEAR=1`, `INTERP_HERMITE=2`, `INTERP_BEZIER=3` but neither uses them. The `_sample_vec3()` and `_sample_quat()` methods only implement linear interpolation (lerp/slerp), never checking interpolation type. Hermite and Bezier interpolation are not implemented in either codebase.

- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 368
- **Status**: Pending
- **Details**: JS calls `vao.setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo, null)` with 6 parameters (last is null for index buffer). C++ calls `vao->setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo)` with only 5 parameters. The 6th parameter (index/element buffer) is missing in C++.

- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 81, 119, 206, 353, 434
- **Status**: Pending
- **Details**: JS defines async `load`, `_load_textures`, `_load_groups`, `loadDoodadSet`, and `updateSets`; C++ ports these methods synchronously, changing await/timing behavior.

- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 101–107, 637–639
- **Status**: Pending
- **Details**: JS stores `groupArray`/`setArray` by reference in `core.view` and updates via Vue `$watch` callbacks with explicit unregister in `dispose`. C++ copies arrays into view state and replaces watcher callbacks with polling logic, changing reactivity/update timing semantics.

- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 105–107
- **Status**: Pending
- **Details**: Same approach as WMOLegacyRendererGL — JS uses Vue watchers, C++ uses per-frame polling in `render()` (lines 643–676). Architecturally different but functionally equivalent with potential one-frame delay.

- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` line 126
- **Status**: Pending
- **Details**: JS `!!wmo.textureNames` is true if the property exists and is truthy (even an empty array `[]` is truthy). C++ `!wmo->textureNames.empty()` is only true if the map has entries. If a WMO has the texture names chunk but it's empty, JS enters classic mode but C++ does not. Comment at C++ line 140–143 acknowledges this.

- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 64–65, 103–104
- **Status**: Pending
- **Details**: JS uses `view[this.wmoGroupKey]` and `view[this.wmoSetKey]` for dynamic property access. C++ implements `get_wmo_groups_view()` and `get_wmo_sets_view()` methods (lines 60–69) that return references to the appropriate core::view member based on the key string, supporting `modelViewerWMOGroups`, `creatureViewerWMOGroups`, and `decorViewerWMOGroups`. This is a valid C++ adaptation of JS's dynamic property access.

- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 77, 104, 168, 270, 353
- **Status**: Pending
- **Details**: JS exposes async `load`, `_load_textures`, `_load_groups`, `loadDoodadSet`, and `updateSets`; C++ ports these paths as synchronous methods, altering Promise scheduling and error propagation behavior.

- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 287–289
- **Status**: Pending
- **Details**: JS directly accesses `wmo.doodads[firstIndex + i]` without a pre-check. C++ introduces explicit range guarding/continue behavior, changing edge-case handling when doodad counts/indices are inconsistent.

- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 88–93, 519–521
- **Status**: Pending
- **Details**: JS wires `$watch` callbacks and unregisters them in `dispose`. C++ removes watcher registration and uses per-frame state polling, which changes update trigger timing and reactivity semantics.

- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 91–93
- **Status**: Pending
- **Details**: JS sets up three Vue watchers in `load()`. C++ replaces these with manual per-frame polling in `render()` (lines 517–551), comparing current state against `prev_group_checked`/`prev_set_checked` arrays. This is functionally equivalent but architecturally different — watchers are event-driven, polling is frame-driven with potential one-frame delay.

- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 146–147
- **Status**: Pending
- **Details**: JS sets `wrap_s = (material.flags & 0x40) ? gl.CLAMP_TO_EDGE : gl.REPEAT` and `wrap_t = (material.flags & 0x80) ? gl.CLAMP_TO_EDGE : gl.REPEAT`. C++ creates `BLPTextureFlags` with `wrap_s = !(material.flags & 0x40)` at line 184–185. The boolean negation may invert the wrap behavior — if `true` maps to CLAMP in the BLPTextureFlags API, then `!(flags & 0x40)` produces the opposite of what JS does. Need to verify the BLPTextureFlags API to confirm.

- **JS Source**: `src/js/casc/export-helper.js` lines 97–114
- **Status**: Pending
- **Details**: JS exposes `static async getIncrementalFilename(...)` and awaits `generics.fileExists`; C++ implementation is synchronous, changing timing/error behavior expected by Promise-style callers.

- **JS Source**: `src/js/casc/export-helper.js` lines 284–288
- **Status**: Pending
- **Details**: JS writes stack traces with `console.log(stackTrace)` in `mark(...)`; C++ routes stack trace strings through `logging::write(...)`, changing where detailed error output appears.

- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59–61, 111–112
- **Status**: Pending
- **Details**: JS stores a data-URI string and decodes it inside `exportTextures()`. C++ `addURITexture` accepts `BufferWrapper` PNG bytes directly, changing caller-facing behavior and where decoding occurs.

- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 568
- **Status**: Pending
- **Details**: JS exports UV2 when `config.modelsExportUV2 && uv2` (empty arrays are truthy). C++ requires `!uv2.empty()`, so empty-but-present UV2 buffers are not exported.

- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59–61
- **Status**: Pending
- **Details**: JS `addURITexture(out, dataURI)` stores a raw base64 data URI string, which is decoded later in `exportTextures()` via `BufferWrapper.fromBase64(dataTexture.replace(...))`. C++ `addURITexture(uint32_t textureType, BufferWrapper pngData)` accepts already-decoded PNG data, shifting the decoding responsibility to the caller. This is a contract change that alters the interface boundary — callers must now pre-decode the data URI before passing it. While not a bug if all callers are adapted, it changes the API surface compared to the original JS.

- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 794
- **Status**: Pending
- **Details**: JS uses `Object.assign({ enabled: subMeshEnabled }, skin.subMeshes[i])` which dynamically copies *all* properties from the submesh object. C++ (M2Exporter.cpp ~lines 1111–1126) manually enumerates a fixed set of properties (submeshID, level, vertexStart, vertexCount, triangleStart, triangleCount, boneCount, boneStart, boneInfluences, centerBoneIndex, centerPosition, sortCenterPosition, sortRadius). If the Skin's SubMesh struct gains new fields, they would automatically appear in JS JSON output but would be missing in C++ JSON output. This is a fragile pattern that could silently omit metadata.

- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 748
- **Status**: Pending
- **Details**: In JS, `texFileDataID` for data textures is the string key `"data-X"`, which gets stored as-is in the file manifest. In C++ (~line 1059), `std::stoul(texKey)` fails for `"data-X"` keys and `texID` defaults to 0 in the `catch (...)` block. This means data textures in the file manifest will have `fileDataID = 0` instead of a meaningful identifier, losing the ability to correlate manifest entries with specific data texture types.

- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 194
- **Status**: Pending
- **Details**: JS calls `listfile.formatUnknownFile(texFile)` where `texFile` is a string like `"12345.png"`. C++ (~line 410) calls `casc::listfile::formatUnknownFile(texFileDataID, raw ? ".blp" : ".png")` passing the numeric ID and extension separately. The C++ call passes `raw ? ".blp" : ".png"` but this code appears in the `!raw` branch (line 406 checks `!raw`), so the `raw` ternary would always evaluate to `.png`. While not necessarily a bug (depends on `formatUnknownFile` implementation), the call signature divergence means the output filename format may differ.

- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 39, 123, 262, 299
- **Status**: Pending
- **Details**: JS export methods (`exportTextures`, `exportAsOBJ`, `exportAsSTL`, `exportRaw`) are async and yield during I/O. C++ runs these paths synchronously, altering timing/cancellation behavior versus JS.

- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49–50
- **Status**: Pending
- **Details**: JS stores raw data-URI strings in `dataTextures`; C++ stores `BufferWrapper` PNG bytes, changing caller contract and data normalization stage.

- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 88–89, 141–142
- **Status**: Pending
- **Details**: JS exports UV1 whenever it is defined (`!== undefined`), including empty arrays. C++ requires `!m3->uv1.empty()`, which changes behavior for defined-but-empty UV sets.

- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49–51
- **Status**: Pending
- **Details**: Same issue as M2Exporter: JS `addURITexture(out, dataURI)` stores a raw base64 data URI string keyed by output path. C++ `addURITexture(const std::string& out, BufferWrapper pngData)` accepts already-decoded PNG data. This is an API contract change that shifts decoding responsibility to the caller.

- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 62, 219, 360, 739, 841, 1179
- **Status**: Pending
- **Details**: JS uses async export methods (`exportTextures`, `exportAsGLTF`, `exportAsOBJ`, `exportAsSTL`, `exportGroupsAsSeparateOBJ`, `exportRaw`) with awaited CASC/file operations, while C++ executes these paths synchronously.

- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 34–36
- **Status**: Pending
- **Details**: JS constructor is `constructor(data, fileID)` and obtains CASC source internally via `core.view.casc`. C++ constructor is `WMOExporter(BufferWrapper data, uint32_t fileDataID, casc::CASC* casc)` with explicit casc pointer. Additionally, `fileDataID` is constrained to `uint32_t` while JS accepts `string|number` for `fileID`. This is an API deviation — callers must pass the correct CASC instance and cannot pass string file paths.

- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 47, 130, 392, 478
- **Status**: Pending
- **Details**: JS legacy WMO export methods are async and await texture/model I/O; C++ methods (`exportTextures`, `exportAsOBJ`, `exportAsSTL`, `exportRaw`) are synchronous, changing timing/cancellation semantics.

- **JS Source**: `src/js/casc/vp9-avi-demuxer.js` lines 22–23, 83–126
- **Status**: Pending
- **Details**: JS exposes `async parse_header()` and `async* extract_frames()` generator semantics; C++ ports these to synchronous methods with callback iteration, changing consumption and scheduling behavior.

- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 129–225
- **Status**: Pending
- **Details**: JS implements asynchronous writes (`await writer.writeLine(...)` and async filesystem calls). C++ `write()` is synchronous, which changes ordering and error propagation relative to the original Promise API.

- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 84–99
- **Status**: Pending
- **Details**: JS `appendGeometry` handles multiple UV arrays and uses `Array.isArray` + spread operator for concatenation. C++ uses `std::vector::insert` for appending. Functionally equivalent.

- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 119–142
- **Status**: Pending
- **Details**: Both JS and C++ output 1-based vertex indices in OBJ face format (e.g., `f v//vn v//vn v//vn` when no UVs, `f v/vt/vn v/vt/vn v/vt/vn` when UVs present). Vertex offset is added correctly in both implementations. Verified as correct.

- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 41–68
- **Status**: Pending
- **Details**: JS awaits file existence checks, directory creation, and line writes in `async write()`. C++ performs the same work synchronously, so behavior differs for call sites that rely on async completion semantics.

- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 35–37
- **Status**: Pending
- **Details**: C++ line 30 uses `std::filesystem::path(name).stem().string()` to extract the filename without extension. JS uses `path.basename(name, path.extname(name))`. These should produce identical results for typical filenames. However, if `name` contains multiple dots (e.g., `texture.v2.png`), `stem()` returns `texture.v2` while `basename('texture.v2.png', '.png')` also returns `texture.v2`. Functionally equivalent.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 194–1504
- **Status**: Pending
- **Details**: JS defines `async write(overwrite, format)` and awaits filesystem/export operations throughout. C++ exposes `void write(...)` and executes all I/O synchronously, changing call timing/error propagation semantics for callers expecting Promise behavior.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 276–280
- **Status**: Pending
- **Details**: JS `add_scene_node` returns the pushed node object reference (used for `skeleton.children.push()` later). C++ returns the index (size_t) instead, and uses index-based access to modify nodes later. This is functionally equivalent but bone parent lookup uses `bone_lookup_map[bone.parentBone]` to store the node index in C++ vs. storing the node object reference in JS. This difference means C++ accesses `nodes[parent_node_idx]` while JS mutates the object directly.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 282–296
- **Status**: Pending
- **Details**: JS `add_buffered_accessor` always includes `target: buffer_target` in the bufferView. When `buffer_target` is `undefined`, JSON.stringify omits the key entirely. C++ explicitly checks `if (buffer_target >= 0)` before adding the target key. This produces identical JSON output since JS `undefined` values are omitted by JSON.stringify, matching C++ not adding the key at all. Functionally equivalent.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 620–628, 757–765, 887–895
- **Status**: Pending
- **Details**: In JS, animation channel target node is always `nodeIndex + 1` regardless of prefix setting. In C++, `actual_node_idx` is used, which varies based on `usePrefix`. When `usePrefix` is true, C++ sets `actual_node_idx = nodes.size()` after pushing prefix_node (so it points to the real bone node, matching JS `nodeIndex + 1`). When `usePrefix` is false, `actual_node_idx = nodes.size()` before pushing the node, so it points to the same node. The JS code always does `nodeIndex + 1` which is only correct when prefix nodes exist. C++ correctly handles both cases. This is a JS bug that C++ fixes intentionally.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` line 464
- **Status**: Pending
- **Details**: JS `bone_lookup_map.set(bi, node)` stores the node object, which is then mutated later when children are added. C++ `bone_lookup_map[bi] = actual_node_idx` stores the index into the `nodes` array, and children are added via `nodes[parent_node_idx]["children"]`. This is functionally equivalent — JS mutates the object reference in the map and C++ indexes into the JSON array.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1110–1119
- **Status**: Pending
- **Details**: JS always sets `material: materialMap.get(mesh.matName)` in the primitive, even if the material isn't found (result is `undefined`, which gets stripped by JSON.stringify). C++ uses `auto mat_it = materialMap.find(mesh.matName)` and only sets `primitive["material"]` if found. The final JSON output is identical since JS undefined is omitted, but the approach differs.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1404–1411
- **Status**: Pending
- **Details**: Same pattern as entry 422 but for equipment meshes. JS sets `material: materialMap.get(mesh.matName)` which may be `undefined`. C++ checks `eq_mat_it != materialMap.end()` before setting material. Functionally equivalent in JSON output.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` (no equivalent)
- **Status**: Pending
- **Details**: C++ adds `addTextureBuffer(uint32_t fileDataID, BufferWrapper buffer)` method (lines 113–115) which has no JS counterpart. JS only has `setTextureBuffers()` to set the entire map at once. The C++ addition allows incrementally adding individual texture buffers, which changes the API surface.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1468–1470
- **Status**: Pending
- **Details**: JS extracts animation index from bufferView name via `name_parts = bufferView.name.split('_'); anim_idx = name_parts[3]`. C++ uses `bv_name.rfind('_')` and then `std::stoi(bv_name.substr(last_underscore + 1))` to get the animation index. For names like `TRANS_TIMESTAMPS_0_1`, JS gets `name_parts[3] = "1"`, C++ gets substring after last underscore = `"1"`. These produce the same result. However, for `SCALE_TIMESTAMPS_0_1`, both work the same. Functionally equivalent.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 344–347, 449
- **Status**: Pending
- **Details**: JS `const skeleton = add_scene_node({name: ..., children: []})` returns the actual node object. Later, `skeleton.children.push(nodeIndex)` mutates it directly. C++ `size_t skeleton_idx = add_scene_node(...)` gets an index, and later accesses `nodes[skeleton_idx]["children"].push_back(...)`. Functionally equivalent.

- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 460, 466
- **Status**: Pending
- **Details**: JS checks `core.view.config.modelsExportWithBonePrefix` outside the bone loop at line 460 (const is evaluated once). C++ reads `core::view->config.value("modelsExportWithBonePrefix", false)` inside the loop at line 470, which re-reads the config for every bone. Since config shouldn't change during export, this is functionally equivalent but slightly less efficient.

- **JS Source**: `src/js/3D/writers/GLBWriter.js` lines 20–28
- **Status**: Pending
- **Details**: The glTF 2.0 spec requires that the JSON chunk be padded with trailing space characters (0x20) to maintain 4-byte alignment. C++ `BufferWrapper::alloc(size, true)` zero-fills the buffer, so JSON padding bytes are 0x00. JS `Buffer.alloc(size)` also zero-fills, so JS has the same issue. However, this should be documented as a potential spec compliance issue for both versions.

- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 33–43
- **Status**: Pending
- **Details**: JS uses `async write()` and a `JSON.stringify` replacer that converts `bigint` values to strings. C++ `write()` is synchronous and writes `nlohmann::json::dump()` directly, which changes both async semantics and JS BigInt serialization parity.

- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 37–42
- **Status**: Pending
- **Details**: Both produce tab-indented JSON, but nlohmann `dump(1, '\t')` uses indent width of 1 with tab character, while JS `JSON.stringify` with `'\t'` uses tab for each indent level. The output should be identical for well-formed JSON.

- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 25–27
- **Status**: Pending
- **Details**: JS `addField(...fields)` accepts any number of arguments via rest parameters and pushes all. C++ provides `addField(const std::string&)` for single fields and `addField(const std::vector<std::string>&)` for multiple. Callers must adapt to one of these two signatures instead of passing multiple individual arguments.

- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 42–51
- **Status**: Pending
- **Details**: JS `escapeCSVField()` handles `null`/`undefined` by returning empty string (line 43–44), then calls `value.toString()` for other types. C++ only accepts `const std::string&` and returns empty for empty strings (line 28–29). JS could receive numbers/booleans and stringify them; C++ requires pre-conversion to string by the caller.

- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 210–229
- **Status**: Pending
- **Details**: JS `async write()` awaits file checks, directory creation, and output writes. C++ performs the same operations synchronously, diverging from JS caller-visible async behavior.

- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 66–76
- **Status**: Pending
- **Details**: JS returns `NULL` only for `null`/`undefined`; an empty string serializes to `''`. C++ maps `value.empty()` to `NULL`, so genuine empty-string field values are emitted as SQL `NULL`, changing exported data.

- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 48–49
- **Status**: Pending
- **Details**: JS `addField(...fields)` accepts any number of arguments via rest parameters and pushes all. C++ provides `addField(const std::string&)` for single fields and `addField(const std::vector<std::string>&)` for multiple. Same pattern as CSVWriter entry 413.

- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 141–177
- **Status**: Pending
- **Details**: JS builds an array of `lines` and joins with `\n` at the end. The output includes `DROP TABLE IF EXISTS ...\n\nCREATE TABLE ... (\n<columns>\n);\n\n`. C++ builds the result string directly with `+= "\n"`. The C++ version outputs `DROP TABLE IF EXISTS ...;\n\nCREATE TABLE ... (\n<columns joined with ,\n>\n);\n` which should match. However, JS `lines.push('')` creates an empty element that adds an extra `\n` when joined, and the column_defs are joined separately with `,\n`. The overall output may have subtle whitespace differences in the final string.

- **JS Source**: `src/js/3D/writers/STLWriter.js` lines 131–249
- **Status**: Pending
- **Details**: JS writer path is asynchronous and awaited by callers. C++ `write()` runs synchronously, changing API timing semantics compared to the original implementation.

- **JS Source**: `src/js/3D/writers/STLWriter.js` line 147
- **Status**: Pending
- **Details**: JS: `'Exported using wow.export v' + constants.VERSION`. C++: `"Exported using wow.export.cpp v" + std::string(constants::VERSION)`. This is an intentional branding change per project conventions (user-facing text should say wow.export.cpp). Verified as correct.

- **JS Source**: `src/js/3D/writers/STLWriter.js` lines 66–86
- **Status**: Pending
- **Details**: JS `appendGeometry` checks `Array.isArray(this.verts)` to decide between spread and `Float32Array.from()` for concatenation. C++ always uses `std::vector::insert`, which works correctly regardless. The JS type distinction is a JS-specific concern that doesn't apply to C++. Functionally equivalent.

- [ ] 591. [app.cpp] Header `shadowed` class (box-shadow when toast active) not implemented
  - **JS Source**: `src/index.html` line 11; `src/app.css` lines 212–214
  - **Status**: Pending
  - **Details**: HTML applies `:class="{ shadowed: toast !== null }"` to `#header`. CSS: `#container #header.shadowed { box-shadow: var(--widget-shadow); }` where `--widget-shadow: black 0 0 5px`. When a toast is visible the header should cast a downward black blur shadow. Dear ImGui has no native box-shadow; approximate by drawing a few semi-transparent filled rectangles below the header bottom border when `core::view->toast` is non-null.

- [ ] 592. [tab_characters.cpp] Import/export button brand colors not implemented
  - **JS Source**: `src/app.css` lines 3053–3112
  - **Status**: Pending
  - **Details**: CSS defines specific background-color overrides for character import/export buttons: `.character-bnet-button` (#148eff blue), `.character-wmv-button` (#d22c1e red), `.character-wowhead-button` (#e02020 red), `.character-save-button` (#5865f2 purple). These brand colors do not appear anywhere in the C++ code — the buttons likely render with the default green `--form-button-base` instead. Fix: define the four brand colors as constants and apply them when rendering the respective buttons via `ImGui::PushStyleColor(ImGuiCol_Button, ...)`.

- [ ] 593. [screen_source_select.cpp, screen_build_select.cpp] CSS media-query responsive breakpoints not implemented
  - **JS Source**: `src/app.css` lines 3736–3790
  - **Status**: Pending
  - **Details**: Three `@media (max-height: N px)` breakpoints adjust layout at small screen heights: (1) `max-height: 899px` — hide `#home-help-buttons`; (2) `max-height: 799px` — compress `#source-select` (icons shrink from 80px→50px, reduced gaps and font sizes); (3) `max-height: 849px` — change `#build-select .build-select-buttons` from column to wrapping row. C++ uses fixed layouts and ignores viewport height. Fix: read `ImGui::GetMainViewport()->WorkSize.y` and switch layouts at the same pixel thresholds.

- [ ] 594. [tab_audio.cpp] Sound player audiobox sprite animation not implemented
  - **JS Source**: `src/app.css` lines 2513–2537
  - **Status**: Pending
  - **Details**: CSS animates `#sound-player-anim` as a horizontal sprite strip: `@keyframes sound-audiobox-anim { to { background-position-x: -4326px; } }` at `0.7s steps(14, end) infinite`, playing/paused based on state. The sprite image is `images/audiobox.png` (309×397px per frame, 14 frames = 4326px total width). C++ renders the audio tab without this animated audiobox graphic. Fix: load `audiobox.png` as a texture, compute the current frame from elapsed time (frame = floor(elapsed / 0.7s * 14) % 14 when playing), and render the correct UV sub-region (frame * 309 / total_width → (frame+1) * 309 / total_width).

- [ ] 597. [casc/casc-source-local.cpp] CASC journal index loading 1.27× slower than JS in release (1074ms vs 848ms for 2M entries)
  - **JS Source**: `src/js/casc/casc-source-local.js` (journal index loading)
  - **Status**: Pending
  - **Details**: Debug: 4029ms. Release: 1074ms vs 848ms JS (1.27×). The gap is marginal and may reflect real algorithmic overhead: likely insufficient pre-allocation of the index map (`reserve()`) or sequential small reads per `.idx` entry rather than bulk-reading each file into memory first. Investigate but low priority given the small gap.

- [ ] 598. [casc/encoding.cpp] Encoding table parsing 1.85× slower than JS in release (3811ms vs 2057ms for 2.8M entries)
  - **JS Source**: `src/js/casc/encoding.js`
  - **Status**: Pending
  - **Details**: Debug: 12292ms. Release: 3811ms vs 2057ms JS (1.85×). This is the single remaining significant CASC bottleneck in release. Likely causes: (1) per-entry `std::unordered_map` inserts without `reserve()` — 2.8M insertions with multiple rehash cycles; (2) MD5 keys stored as `std::string` or `std::vector<uint8_t>` rather than `std::array<uint8_t, 16>` with a flat hash — avoids heap allocation per key; (3) file not read into a single buffer before parsing. Fix: `reserve(3'000'000)`, bulk-read the file, use fixed-size key arrays.

- [ ] 599. [casc/root.cpp] Root file parsing 1.54× slower than JS in release (1431ms vs 928ms for 1.9M entries)
  - **JS Source**: `src/js/casc/root.js`
  - **Status**: Pending
  - **Details**: Debug: 7222ms. Release: 1431ms vs 928ms JS (1.54×). Similar to entry 598: likely missing `reserve()` on the root map and/or per-entry copies in the hot parse loop. Fix: `reserve()` before filling, load the full root blob into memory and parse in-place, avoid temporaries.

- [ ] 601. [casc/tact-keys.cpp / app.cpp] TACTKeys download runs before source_select — should fire async in parallel with CDN resolution
  - **JS Source**: `src/js/casc/tact-keys.js`; `src/js/casc/casc-source-remote.js`
  - **Status**: Pending
  - **Details**: In JS, TACTKeys are fetched async during CDN pre-resolution. In C++ release, TACTKeys download completes at 19:35:37 before `set active module: source_select` (also 19:35:37) — in release this is invisible since the download is fast, but the structure is wrong. In slower network conditions the blocking download would delay the UI appearing. Fix: fire TACTKeys as a background `std::async` task concurrent with CDN pre-resolution, join only when a CASC source is opened.

- [ ] 602. [app.cpp / screen_source_select.cpp] "failed to load whats-new.html" error on startup — file missing from C++ build
  - **JS Source**: `src/js/components/whats-new.js`; `src/whats-new.html`
  - **Status**: Pending
  - **Details**: C++ log shows: `failed to load whats-new.html: could not open D:/Repositories/wow.export.cpp\src\whats-new.html`. The `whats-new.html` file was removed from the C++ repo (audited and ported per commit `012b8cde`), but something still attempts to load it at runtime. The `tab_home` stub intentionally omits the whats-new panel (see Intentional Stubs in CLAUDE.md), so this load attempt should be removed or guarded. Locate the call site that tries to open `whats-new.html` and remove it, or suppress the error since the tab_home stub is intentional.

- [ ] 603. [CMakeLists.txt] MSVC debug build is 5–12× slower at runtime than release — add `_ITERATOR_DEBUG_LEVEL=0` and `/JMC-` for debug
  - **JS Source**: N/A (build system only)
  - **Status**: Pending
  - **Details**: MSVC debug builds have two major sources of runtime overhead beyond `/Od` (no optimisation) that do not affect debugability at all: (1) `_ITERATOR_DEBUG_LEVEL=2` (default) adds bounds checking and iterator invalidation detection to every STL operation — responsible for most of the CASC loading slowdown (encoding table 1.85×, root file 1.54× even in release hints at some debug-only calls, but in a pure debug build this is the primary cost on all `std::vector`/`std::unordered_map` access); (2) `/JMC` (Just My Code, on by default) adds a function-entry probe to every function for the debugger's "Step Into User Code" button, adding overhead to every function call. Fix: in `CMakeLists.txt`, add `$<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>` via `add_compile_definitions` (applied globally so all compiled targets in the build are consistent — mixing `_ITERATOR_DEBUG_LEVEL` values across translation units in the same binary causes ODR violations), and `/JMC-` via `target_compile_options` for MSVC debug. Full debugability is preserved: breakpoints, step-through, variable inspection, call stack, and watch expressions are all unaffected. The only things lost are runtime STL iterator validation and the "Step Into User Code" filtering — both of which release builds also omit. Expected: debug CASC load time drops from 5–12× to roughly 2–3× of release.

