# TODO Tracker

> **Progress: 0/69 verified (0%)** — ✅ = Verified, ⬜ = Pending

<!-- ─── src/js/modules/font_helpers.cpp ──────────────────────────────────── -->

- [ ] 383. [font_helpers.cpp] `check_glyph_support` uses fundamentally different detection logic from JS
  - **JS Source**: `src/js/modules/font_helpers.js` lines 19–54
  - **Status**: Pending
  - **Details**: The JS `check_glyph_support(ctx, font_family, char)` works by rendering the character twice to an off-screen canvas — once with a fallback font (`32px monospace`) and once with the target font — and comparing the total alpha channel sum of the two renders (lines 38–53). A character is considered supported if the rendered output differs from the fallback. This approach detects whether the *target font* actually has a glyph for the codepoint, even when the font is not loaded into ImGui. The C++ implementation (font_helpers.cpp lines 54–63) instead calls `ImFont::FindGlyphNoFallback()` on an already-loaded ImGui font. These are not equivalent: the JS detects support in *any* font family loaded in the browser, while the C++ detects support only in an *already-loaded ImGui font*. For glyphs that exist in the OS font but were not baked into the ImGui atlas (e.g., because the atlas only baked a subset of codepoints), the C++ will incorrectly report the glyph as not supported. This is an unavoidable architectural difference between browser/DOM and ImGui, but the deviation should be documented.

- [ ] 385. [font_helpers.cpp] `inject_font_face` — missing font load verification equivalent to JS `document.fonts.load` + `document.fonts.check`
  - **JS Source**: `src/js/modules/font_helpers.js` lines 113–133
  - **Status**: Pending
  - **Details**: After injecting the CSS `@font-face` rule, the JS awaits `document.fonts.load('16px "' + font_id + '"')` (line 124) and then checks `document.fonts.check('16px "' + font_id + '"')` (line 125). If the check returns false, the style node is removed from the DOM and an error is thrown (lines 127–130). The C++ implementation calls `io.Fonts->AddFontFromMemoryTTF(...)` and checks for a null return (line 171), but there is no equivalent to the post-load verification step. If ImGui accepts the font data but it is internally corrupt or unusable, the C++ will not detect this and will not clean up, whereas the JS would detect it and throw. This is a minor error-recovery gap.

<!-- ─── src/js/modules/legacy_tab_audio.cpp ──────────────────────────────── -->

- [ ] 388. [legacy_tab_audio.cpp] `export_sounds` passes only message+function to `helper.mark()`, not the full stack trace
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` line 189
  - **Status**: Pending
  - **Details**: JS calls `helper.mark(export_file_name, false, e.message, e.stack)` where `e.stack` is the full JavaScript stack trace string. C++ calls `helper.mark(export_file_name, false, e.what(), build_stack_trace("export_sounds", e))` where `build_stack_trace` (lines 33–35) returns only `"export_sounds: <exception message>"`. The C++ never captures a real C++ stack trace (e.g., via `std::stacktrace` from C++23 or a platform API). The export helper's `mark()` function receives a weaker error context string than the JS version provides to users in the export report.

- [ ] 389. [legacy_tab_audio.cpp] Animated music icon uses `ImDrawList::AddText` raw draw call instead of a native ImGui widget
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 216–216 (template `sound-player-anim` div)
  - **Status**: Pending
  - **Details**: The JS template renders `<div id="sound-player-anim" :style="{ 'animation-play-state': ... }">` — a CSS-animated element. The C++ replacement (lines 440–449) uses `ImGui::GetWindowDrawList()->AddText(iconFont, animSize, pos, ...)` which is a raw `ImDrawList` call. Per CLAUDE.md, `ImDrawList` calls should be reserved exclusively for effects with no native equivalent such as image rotation, multi-colour gradient fills, or custom OpenGL overlays. A pulsating text icon does not fall into any of those categories; `ImGui::Text` with a scaled font (via `ImGui::PushFont`/`ImGui::PopFont` or by setting the font size) would be a closer match using a native widget approach. The current use of `AddText` violates the ImGui rendering guideline.

<!-- ─── src/js/modules/legacy_tab_data.cpp ──────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/legacy_tab_files.cpp ─────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/legacy_tab_fonts.cpp ─────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/legacy_tab_home.cpp ──────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/legacy_tab_textures.cpp ──────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/module_test_a.cpp ──────────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/module_test_b.cpp ──────────────────────────────────── -->

- [ ] 404. [module_test_b.cpp] inputTextResizeCallback uses BufTextLen instead of BufSize
  - **JS Source**: `src/js/modules/module_test_b.js` lines 6 (v-model binding)
  - **Status**: Pending
  - **Details**: The resize callback calls `str->resize(data->BufTextLen)` but the standard ImGui pattern for dynamic-string InputText is to resize to `data->BufSize` (the allocated buffer capacity requested), not `BufTextLen` (current text length). Using `BufTextLen` means the string backing buffer is always exactly the current text length, which may cause the buffer pointer passed to ImGui to become stale on the next character insertion when the string needs to grow. The correct pattern is `str->resize(static_cast<size_t>(data->BufSize))` followed by `data->Buf = str->data()`.

- [ ] 405. [module_test_b.cpp] InputText buf_size argument passes capacity()+1 instead of capacity()+1 with proper reserve
  - **JS Source**: `src/js/modules/module_test_b.js` lines 6 (v-model="message")
  - **Status**: Pending
  - **Details**: The call `ImGui::InputText("##message", message.data(), message.capacity() + 1, ...)` passes `capacity()+1` as the buffer size, but `std::string::capacity()` does not guarantee space for a null terminator beyond capacity bytes — writing `capacity()+1` characters would overflow the string's internal buffer. The standard pattern is to ensure the string is resized to at least 1 byte (so `data()` is valid), reserve extra capacity, and use `capacity()` (not `capacity()+1`) as the size argument, or alternatively use `message.size() + 1` with a fixed reserved capacity. The reserve(256) before the call only happens if `capacity() < 256`, so on a fresh "Hello Thrall" string this is fine, but the `capacity()+1` buffer-size argument is still technically incorrect.

<!-- ─── src/js/modules/screen_settings.cpp ────────────────────────────────── -->

- [ ] 407. [screen_settings.cpp] SectionHeading uses app::theme::getBoldFont() and raw ImDrawList — should use native ImGui widgets
  - **JS Source**: `src/js/modules/screen_settings.js` lines 20–360 (template h1 headings)
  - **Status**: Pending
  - **Details**: `SectionHeading()` calls `app::theme::getBoldFont()` and `ImGui::PushFont`/`ImGui::PopFont` with a raw font pointer from the deprecated `app::theme` API. CLAUDE.md states `app::theme` color constants and `applyTheme()` should be progressively removed and not referenced in new code. The function should use a standard ImGui approach (e.g., `ImGui::SeparatorText()` or `ImGui::TextUnformatted()` with the default font) rather than referencing `app::theme`.

- [ ] 408. [screen_settings.cpp] multiButtonSegment uses raw ImDrawList (AddRectFilled, AddText) instead of native ImGui widgets
  - **JS Source**: `src/js/modules/screen_settings.js` lines 111–115, 178–183, 232–236 (ui-multi-button)
  - **Status**: Pending
  - **Details**: `multiButtonSegment()` renders button backgrounds and text via `ImGui::GetWindowDrawList()->AddRectFilled(...)` and `->AddText(...)`. CLAUDE.md explicitly prohibits raw `ImDrawList` calls for anything a native widget handles, and buttons/text are natively handled by ImGui. The segmented button should be implemented using native `ImGui::Button` or styled `ImGui::Selectable` widgets with `ImGui::SameLine()`, without any `ImDrawList` calls.

- [ ] 409. [screen_settings.cpp] JS handle_apply() checks cfg.exportDirectory.length === 0; C++ checks !cfg.contains() OR .empty()
  - **JS Source**: `src/js/modules/screen_settings.js` lines 426–427
  - **Status**: Pending
  - **Details**: The JS check is `cfg.exportDirectory.length === 0` — it directly accesses the field and would throw a JS error if the field is missing. The C++ uses `!cfg.contains("exportDirectory") || cfg["exportDirectory"].get<std::string>().empty()` which is more defensive and handles a missing key gracefully. This is a minor deviation but makes the C++ more robust than the JS original.

- [ ] 412. [screen_settings.cpp] set_selected_cdn (in screen_source_select) calls config::save() — JS does not
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 78–83
  - **Status**: Pending
  - **Details**: This finding belongs to screen_source_select.cpp (see below), but is noted here because the save call pattern appears in both files. The settings screen's `handle_apply()` correctly saves. No additional issue here beyond the note above.

- [ ] 415. [screen_settings.cpp] CASC Locale MenuButton: JS uses availableLocale.flags object; C++ uses locale_flags::entries
  - **JS Source**: `src/js/modules/screen_settings.js` lines 381–392
  - **Status**: Pending
  - **Details**: The JS `available_locale_keys` computed property iterates `Object.keys(this.$core.view.availableLocale.flags)` and the `selected_locale_key` iterates `Object.entries(this.$core.view.availableLocale.flags)`. The C++ uses `casc::locale_flags::entries` directly (a static array). This is a valid architectural change since the JS `availableLocale.flags` is populated from the same source data. Functionally equivalent if `locale_flags::entries` matches the runtime locale list, but the JS version was dynamic (loaded at runtime), while the C++ version is compile-time static. This may be a deviation if locales need to be filtered at runtime.

- [ ] 416. [screen_settings.cpp] Locale MenuButton wrapper div has width: 150px in JS; C++ does not constrain width
  - **JS Source**: `src/js/modules/screen_settings.js` line 149
  - **Status**: Pending
  - **Details**: The JS wraps the locale MenuButton in `<div style="width: 150px">`. The C++ renders the MenuButton without constraining its width to 150px. This is a minor visual layout deviation.

<!-- ─── src/js/modules/screen_source_select.cpp ───────────────────────────── -->

- [ ] 417. [screen_source_select.cpp] set_selected_cdn calls config::save() which is not present in JS
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 78–83
  - **Status**: Pending
  - **Details**: The JS `set_selected_cdn()` sets `this.$core.view.config.sourceSelectUserRegion = region.tag` but does NOT call `save()`. The C++ explicitly calls `config::save()` after updating the value. This means the CDN region preference is persisted to disk on every CDN region change in C++ but not in the JS original (which relies on the user's explicit Apply/save flow).

- [ ] 418. [screen_source_select.cpp] Build button disabled visual state not applied when isBusy
  - **JS Source**: `src/js/modules/screen_source_select.js` line 61
  - **Status**: Pending
  - **Details**: The JS build buttons use `:class="[..., { disabled: $core.view.isBusy }]"` which applies CSS styling to show the button as visually disabled when busy. The C++ only checks `!core::view->isBusy` before calling `click_source_build()` in the click handler, but does not apply any visual disabled styling (no `ImGui::BeginDisabled()`/`EndDisabled()` around the build buttons). Users receive no visual feedback that the button is inactive while busy.

- [ ] 423. [screen_source_select.cpp] open_legacy_install runs synchronously in C++; JS is async with await
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 169–204
  - **Status**: Pending
  - **Details**: The JS `open_legacy_install()` is `async` and awaits `this.$core.view.mpq.loadInstall()`. The C++ runs `core::view->mpq->loadInstall()` synchronously on the main thread (no background thread or async dispatch). This blocks the main/render thread during legacy MPQ loading, which would freeze the UI. The C++ should use a background thread (like `source_open_thread`) for MPQ loading to match the non-blocking JS behavior.

- [ ] 424. [screen_source_select.cpp] Subtitle text in cards uses raw ImDrawList with manual word-wrap instead of native ImGui::TextWrapped
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 23–26 (source-subtitle divs)
  - **Status**: Pending
  - **Details**: The subtitle text within each source card is rendered via raw `draw->AddText(...)` with manual word-wrap logic using `font->CalcWordWrapPositionA()`. CLAUDE.md prohibits raw `ImDrawList` calls for anything a native widget handles, and `ImGui::TextWrapped()` handles this. The manual word-wrap implementation is error-prone (the `LegacySize` division may produce incorrect results for non-default font sizes) and should be replaced with native `ImGui::TextWrapped()` or `ImGui::PushTextWrapPos()` / `ImGui::TextUnformatted()`.

- [ ] 425. [screen_source_select.cpp] Card title text uses raw ImDrawList AddText instead of native ImGui widget
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 23 (source-title divs)
  - **Status**: Pending
  - **Details**: Card titles are rendered via `draw->AddText(bold_font, title_size, ..., card.title)`. CLAUDE.md says to use native ImGui widgets (`Text`, `Button`, etc.) rather than `AddText` for anything a native widget handles. The entire card rendering (title, subtitle, link text) uses raw `ImDrawList` calls. While the card border (dashed rounded rect) legitimately requires ImDrawList, the text content within should use native ImGui text widgets placed within the card's screen area.

- [ ] 426. [screen_source_select.cpp] ensureSourceTextures uses app::theme::loadSvgTexture — app::theme should be phased out
  - **JS Source**: `src/js/modules/screen_source_select.js` (source icons referenced via CSS background images)
  - **Status**: Pending
  - **Details**: `ensureSourceTextures()` calls `app::theme::loadSvgTexture(...)` to load the SVG icons. CLAUDE.md states `app::theme` color constants and functions should be progressively removed and not referenced in new code. The SVG loading should be performed via a non-theme utility or inlined.

- [ ] 427. [screen_source_select.cpp] build_compact layout uses ImDrawList for all build button rendering
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 61 (build buttons)
  - **Status**: Pending
  - **Details**: Build selection buttons in the build-select view are rendered entirely via raw ImDrawList calls (`AddRectFilled`, `AddText`, `AddImage`, `drawDashedRoundedRect`). CLAUDE.md says native ImGui widgets should be used for button/text rendering. Only the dashed rounded rect outline itself is a legitimate ImDrawList use; the button background, text label, and image should use native `ImGui::ImageButton`, `ImGui::Button`, or `ImGui::Selectable` widgets.

<!-- ─── src/js/modules/tab_audio.cpp ──────────────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/tab_blender.cpp ────────────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/tab_characters.cpp ─────────────────────────────────── -->

- [ ] 446. [tab_characters.cpp] capture_character_thumbnail: C++ renders via GL FBO directly; JS uses canvas + requestAnimationFrame
  - **JS Source**: `src/js/modules/tab_characters.js` lines 1403–1495
  - **Status**: Pending
  - **Details**: The JS function awaits two animation frames via `requestAnimationFrame` before capturing from the canvas. The C++ calls `model_viewer_gl::render_one_frame()` synchronously. The JS version ensures that the renderer has had two frames to settle the camera/animation pose changes. The C++ only renders one frame synchronously. If the model renderer requires multiple frames to settle (e.g. due to interpolated bone poses), the thumbnail may capture an incorrect mid-transition frame. This is a potential visual fidelity deviation.

- [ ] 447. [tab_characters.cpp] export_char_model: PNG/CLIPBOARD path missing "modelsExportPngIncrements" logic
  - **JS Source**: `src/js/modules/tab_characters.js` lines 1742–1776
  - **Status**: Pending
  - **Details**: JS PNG export path (lines 1753–1763) checks `core.view.config.modelsExportPngIncrements` and calls `ExportHelper.getIncrementalFilename(out_file)`. The C++ `export_char_model()` in the PNG/CLIPBOARD path at line ~2467 calls `model_viewer_utils::export_preview(format, *gl_ctx, file_name)` which delegates to a shared utility. It is not clear whether the C++ `export_preview` implements incremental filename support. If it does not, the incremental file naming feature is missing for character PNG export.

- [ ] 448. [tab_characters.cpp] export_char_model: modifier_id not passed to getItemDisplay in export path
  - **JS Source**: `src/js/modules/tab_characters.js` lines 1829, 1885
  - **Status**: Pending
  - **Details**: JS export calls `DBItemModels.getItemDisplay(geom.item_id, char_info?.raceID, char_info?.genderIndex, geom.modifier_id)`. The C++ export at line ~2561 calls `db::caches::DBItemModels::getItemDisplay(geom.item_id, static_cast<int>(char_info->raceID), char_info->genderIndex)` — `modifier_id` (the 4th argument) is not passed in the C++ version. This means item skin/appearance modifier is not applied when getting item display data for export, potentially using the wrong textures for skinned items.

- [ ] 450. [tab_characters.cpp] mounted: JS shows 10 loading steps; C++ shows 8
  - **JS Source**: `src/js/modules/tab_characters.js` line 2706
  - **Status**: Pending
  - **Details**: JS: `this.$core.showLoadingScreen(10)` — 10 loading steps. C++: `core::showLoadingScreen(8)` — 8 loading steps. The JS has separate progress steps for DBItemList and charShaders that the C++ may have consolidated. Minor UI discrepancy.

- [ ] 451. [tab_characters.cpp] mounted: JS initializes DBItemList with progress callback; C++ does not call DBItemList init
  - **JS Source**: `src/js/modules/tab_characters.js` line 2759
  - **Status**: Pending
  - **Details**: JS: `await DBItemList.initialize((msg) => this.$core.progressLoadingScreen(msg))` — initializes a separate `DBItemList` cache with a progress callback. The C++ mounted does not appear to call any equivalent `DBItemList::initialize()` or `DBItemList::ensureInitialized()`. This means item list data used for the item picker may not be loaded, or it is initialized elsewhere. This needs verification.

- [ ] 454. [tab_characters.cpp] render(): color picker popup uses a single shared popup ID "##chr_color_popup" for all options
  - **JS Source**: `src/js/modules/tab_characters.js` lines 2143–2161
  - **Status**: Pending
  - **Details**: In the C++ render loop for customization options, all color options open the same popup `"##chr_color_popup"`. ImGui `BeginPopup` uses string IDs to identify popups, so using the same string across multiple options means the popup opened may render with the wrong option's choices. The popup content uses `option_id` from the surrounding loop scope, which works as long as the loop variable is captured correctly in the closure. However, since ImGui popups are modal/global and the loop has already advanced by the time the popup is rendered, the `option_id` inside the popup body may be incorrect (it will be the last option_id in the loop iteration).

- [ ] 455. [tab_characters.cpp] render(): animation scrubber "start_scrub" / "end_scrub" uses IsItemActivated/IsItemDeactivatedAfterEdit but JS uses mousedown/mouseup
  - **JS Source**: `src/js/modules/tab_characters.js` lines 2360–2372
  - **Status**: Pending
  - **Details**: JS `start_scrub` is triggered on `@mousedown` of the scrubber range input, and `end_scrub` on `@mouseup`. The C++ uses `ImGui::IsItemActivated()` and `ImGui::IsItemDeactivatedAfterEdit()` on the `SliderInt`. These are close equivalents in ImGui but `IsItemActivated` fires on focus, not just mouse-down. The `_was_paused_before_scrub` state variable is stored as a static file-scope variable in C++ but as `this._was_paused_before_scrub` (instance variable) in JS. This is equivalent since there is only one scrubber, but using a static means it persists across tab activations.

- [ ] 456. [tab_characters.cpp] render(): "Get Item Skin Count/Index/Cycle" context menu entry "Next Skin (X/Y)" is missing
  - **JS Source**: `src/js/modules/tab_characters.js` lines 2286–2296
  - **Status**: Pending
  - **Details**: The JS equipment slot context menu includes: `<span v-if="get_item_skin_count(context.node) > 1" @click.self="cycle_item_skin(context.node, 1)">Next Skin ({{ get_item_skin_index(context.node) + 1 }}/{{ get_item_skin_count(context.node) }})</span>`. The C++ context menu (`##equip_ctx`) at line ~3998 does not include a "Next Skin" option. Skin cycling from the context menu is missing.

- [ ] 457. [tab_characters.cpp] render(): skin cycling controls (< / count / >) on equipped item slots are missing
  - **JS Source**: `src/js/modules/tab_characters.js` lines 2286–2290
  - **Status**: Pending
  - **Details**: The JS template shows `<span v-if="get_item_skin_count(slot.id) > 1" class="slot-skin-controls" @click.stop>` with prev/count/next arrows for cycling item skins inline in the equipment list. The C++ equipment list rendering only shows the item name and quality — there are no inline skin cycling controls.

- [ ] 458. [tab_characters.cpp] export_char_model: PNG export missing "view in explorer" callback on success toast
  - **JS Source**: `src/js/modules/tab_characters.js` lines 1762–1763
  - **Status**: Pending
  - **Details**: JS: `core.setToast('success', util.format('successfully exported preview to %s', out_file), { 'view in explorer': () => nw.Shell.openItem(out_dir) }, -1)`. The C++ delegates to `model_viewer_utils::export_preview()` which may or may not include the "view in explorer" action. This needs verification against the `export_preview` implementation.

- [ ] 459. [tab_characters.cpp] render(): "open_items_tab_from_picker" / ItemPickerModal not rendered in C++
  - **JS Source**: `src/js/modules/tab_characters.js` lines 2306, 2539–2543
  - **Status**: Pending
  - **Details**: The JS template ends with `<component :is="$components.ItemPickerModal" v-if="$core.view.chrItemPickerSlot !== null" ...>`. The C++ `render()` does not appear to render any item picker modal (no `ItemPickerModal` equivalent found in the render function). When a user clicks on an empty equipment slot or the "Replace Item" context menu entry, the C++ redirects to `tab_items::setActive()` (navigates away) rather than showing an inline modal picker. The JS shows a modal dialog without leaving the characters tab. This is a significant layout deviation.

- [ ] 460. [tab_characters.cpp] mounted: loading screen progress step count inconsistency (8 in C++ vs 10 in JS)
  - **JS Source**: `src/js/modules/tab_characters.js` line 2706
  - **Status**: Pending
  - **Details**: See finding above — JS shows 10 steps, C++ shows 8. This causes the loading screen to complete at a different visual percentage. Separate from the "DBItemList not initialized" finding.

<!-- ─── src/js/modules/tab_creatures.cpp ──────────────────────────────────── -->

- [ ] 463. [tab_creatures.cpp] Geosets sidebar uses raw ImGui checkboxes instead of Checkboxlist component
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 1108–1114
  - **Status**: Pending
  - **Details**: JS uses `<component :is="$components.Checkboxlist" :items="creatureViewerGeosets">` for the geosets list. The C++ renders individual `ImGui::Checkbox` calls for each geoset item in a loop (line ~2197–2215). Should use `checkboxlist::render()` for consistency, as other tabs (e.g. tab_decor) do. This is an aesthetic deviation but also means the Checkboxlist component behaviour (scrolling, virtualisation) is absent.

- [ ] 464. [tab_creatures.cpp] Equipment checklist sidebar uses raw ImGui checkboxes instead of Checkboxlist component
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 1101–1107
  - **Status**: Pending
  - **Details**: JS uses `<component :is="$components.Checkboxlist" :items="creatureViewerEquipment">`. The C++ renders individual `ImGui::Checkbox` calls for each equipment item (line ~2175–2192). Should use `checkboxlist::render()` for consistency with the JS template.

- [ ] 465. [tab_creatures.cpp] WMO Groups sidebar uses raw ImGui checkboxes instead of Checkboxlist component
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 1120–1125
  - **Status**: Pending
  - **Details**: JS uses `<component :is="$components.Checkboxlist" :items="creatureViewerWMOGroups">` and `<component :is="$components.Checkboxlist" :items="creatureViewerWMOSets">`. The C++ renders individual `ImGui::Checkbox` calls for each item (lines ~2241–2265). Should use `checkboxlist::render()`.

- [ ] 466. [tab_creatures.cpp] Skin list uses ImGui::Selectable instead of Listboxb component
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 1116–1118
  - **Status**: Pending
  - **Details**: JS uses `<component :is="$components.Listboxb" :items="creatureViewerSkins" v-model:selection="creatureViewerSkinsSelection" :single="true">`. The C++ renders a plain `ImGui::Selectable` loop for skins (lines ~2218–2235). The JS `Listboxb` is a scrollable, filterable listbox. The C++ version lacks scrolling/filtering on the skins list.

- [ ] 467. [tab_creatures.cpp] Missing tooltip text on Preview checkboxes in sidebar
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 1055–1080
  - **Status**: Pending
  - **Details**: The JS template includes `title` attributes on each checkbox in the Preview sidebar section (e.g. "Automatically preview a creature when selecting it", "Automatically adjust camera when selecting a new creature", etc.). The C++ sidebar (lines ~2103–2139) does not call `ImGui::SetTooltip()` after most of the Preview checkboxes, missing the tooltip text that was present in the original JS.

- [ ] 468. [tab_creatures.cpp] localeCompare sort vs simple lowercase string sort
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 1169–1173
  - **Status**: Pending
  - **Details**: JS sorts creature entries with `localeCompare()` which is a Unicode-aware, locale-sensitive comparison. The C++ uses simple `<` operator on lowercase strings (line ~1553), which is not locale-aware. For non-ASCII creature names this could produce different sort order.

<!-- ─── src/js/modules/tab_data.cpp ───────────────────────────────────────── -->

- [ ] 470. [tab_data.cpp] Listbox is missing :copydir / copydir binding
  - **JS Source**: `src/js/modules/tab_data.js` lines 97–99
  - **Status**: Pending
  - **Details**: The JS template passes `:copydir="$core.view.config.copyFileDirectories"` to the Listbox for the DB2 list. The C++ `listbox::render()` call (line ~274) does not pass a `copydir` equivalent. The C++ listbox function signature may not have this parameter, but the config value `copyFileDirectories` should still be wired if the listbox supports it.

<!-- ─── src/js/modules/tab_decor.cpp ──────────────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/tab_fonts.cpp ──────────────────────────────────────── -->

- [ ] 481. [tab_fonts.cpp] Font glyph grid renders as character grid in ImGui but JS uses a DOM-based grid element
  - **JS Source**: `src/js/modules/tab_fonts.js` lines 64–66, 153–165
  - **Status**: Pending
  - **Details**: In JS, the glyph grid is a DOM element (`.font-character-grid`) and `detect_glyphs_async(font_id, grid_element, on_glyph_click)` inserts individual character `<div>` elements into the DOM. The C++ (lines ~308–346) renders an ImGui child window with `ImGui::Selectable` cells for each detected codepoint. This is a valid C++ equivalent of the JS DOM approach, but the visual appearance and interaction (hover tooltip, click behaviour, wrapping) may differ from the original. The wrapping logic is manually implemented in C++.

- [ ] 483. [tab_fonts.cpp] Export button uses app::theme::BeginDisabledButton/EndDisabledButton instead of standard ImGui::BeginDisabled
  - **JS Source**: `src/js/modules/tab_fonts.js` line 72
  - **Status**: Pending
  - **Details**: JS renders `<input type="button" :class="{ disabled: isBusy }">`. C++ uses `app::theme::BeginDisabledButton()` / `EndDisabledButton()` (lines ~380–384). According to CLAUDE.md, `app::theme` color constants and `applyTheme()` should be progressively removed. `app::theme::BeginDisabledButton` should be replaced with `ImGui::BeginDisabled(busy)` / `ImGui::EndDisabled()`.

- [ ] 484. [tab_fonts.cpp] fontPreviewText placeholder tooltip only shows on empty InputTextMultiline
  - **JS Source**: `src/js/modules/tab_fonts.js` lines 67–68
  - **Status**: Pending
  - **Details**: In JS, the textarea has a `:placeholder` attribute that shows when the field is empty. In C++ (lines ~372–373), `ImGui::SetItemTooltip()` is used to show the placeholder as a tooltip when the field is empty (`if (view.fontPreviewText.empty())`). The tooltip only appears on hover, not as a persistent placeholder text inside the input box. This is a visual difference from the JS (placeholder text vs hover tooltip).

<!-- ─── src/js/modules/tab_install.cpp ────────────────────────────────────── -->

- [ ] 487. [tab_install.cpp] Missing listbox::renderStatusBar calls — includefilecount status bar absent from both views
  - **JS Source**: `src/js/modules/tab_install.js` lines 165, 184
  - **Status**: Pending
  - **Details**: Both Listbox components in the JS template use `:includefilecount="true"` — one with `unittype="install file"` (main manifest view) and one with `unittype="string"` (string viewer). In the C++ port, `listbox::render()` does not internally render the status bar; it must be called separately via `listbox::renderStatusBar()`. The `tab_install.cpp` render function never calls `listbox::renderStatusBar()` for either listbox. This means the file count status bar ("N install files found.") is entirely absent from the Install Manifest tab in both views. Compare with other tabs that correctly call `listbox::renderStatusBar()` after each listbox (e.g., `tab_audio.cpp` line 553, `tab_fonts.cpp` line 289). The status bar should be added after `ImGui::EndChild()` for each list container, before the tray section.

<!-- ─── src/js/modules/tab_item_sets.cpp ──────────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/tab_items.cpp ──────────────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/tab_maps.cpp ───────────────────────────────────────── -->

- [ ] 496. [tab_maps.cpp] export_map_wmo checks !selected_wdt->worldModelPlacement differently from JS
  - **JS Source**: `src/js/modules/tab_maps.js` lines 640–645
  - **Status**: Pending
  - **Details**: JS checks `if (!selected_wdt || !selected_wdt.worldModelPlacement)` (falsy check). C++ checks `if (!selected_wdt || (!selected_wdt->hasWorldModelPlacement && selected_wdt->worldModel.empty()))`. This is a more permissive condition — if the WDT has a `worldModel` string but no placement, the C++ will proceed while the JS would throw. This deviation means the C++ will attempt to export even when JS would reject it, potentially hitting different errors downstream.

- [ ] 498. [tab_maps.cpp] initialize() posts maps to main thread but JS sets mapViewerMaps directly and hides loading screen after
  - **JS Source**: `src/js/modules/tab_maps.js` lines 938–978
  - **Status**: Pending
  - **Details**: JS sets `this.$core.view.mapViewerMaps = maps` synchronously after the loop, then calls `this.$core.hideLoadingScreen()`. C++ uses `core::postToMainThread` to set `mapViewerMaps` and sets `tab_initialized = true`, but calls `core::hideLoadingScreen()` BEFORE `postToMainThread` returns. This means the loading screen may hide before `mapViewerMaps` is updated, which could cause a brief display of an empty map list if the render loop runs between the hide and the main-thread post.

<!-- ─── src/js/modules/tab_models_legacy.cpp ──────────────────────────────── -->

- [ ] 515. [tab_models_legacy.cpp] PNG/CLIPBOARD export path uses different logic than JS
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 197–221
  - **Status**: Pending
  - **Details**: JS PNG export for legacy models: gets canvas from DOM (`document.getElementById('legacy-model-preview').querySelector('canvas')`), converts to buffer, writes to file with incremental naming support (`modelsExportPngIncrements`), shows success toast with "View in Explorer" link. C++ instead calls `model_viewer_utils::export_preview()` which may have different behaviour. The JS CLIPBOARD path copies as base64 PNG to clipboard with a success toast. C++ calls `model_viewer_utils::export_preview(format, ...)` which likely handles both. The JS also checks `config.modelsExportPngIncrements` for incremental filenames — this config option is not referenced in the C++ path. Missing feature.

- [ ] 516. [tab_models_legacy.cpp] Missing modelsExportPngIncrements check for legacy PNG export
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 207–209
  - **Status**: Pending
  - **Details**: JS legacy PNG export checks `if (core.view.config.modelsExportPngIncrements)` and calls `ExportHelper.getIncrementalFilename(out_file)`. C++ does not implement this for legacy models export.

- [ ] 519. [tab_models_legacy.cpp] JS mounted() calls initialize logic synchronously; C++ runs in mounted() directly (not background thread)
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 499–535
  - **Status**: Pending
  - **Details**: The JS runs all initialization in `mounted()` with async/await on the loading screen. The C++ runs everything in `mounted()` synchronously on the main thread. This blocks the main thread during model list building and creature data loading. It should run on a background thread like other tabs (e.g. tab_maps uses a background thread). The loading screen display may not render correctly if the main thread is blocked.

- [ ] 521. [tab_models_legacy.cpp] step_animation for MDX does not call renderer's step_animation_frame
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 462–471
  - **Status**: Pending
  - **Details**: JS `step_animation`: calls `renderer.step_animation_frame?.(delta)` and reads back `renderer.get_animation_frame?.() || 0` for both M2 and MDX renderers. C++ `step_animation` calls `active_renderer_m2->step_animation_frame(delta)` for M2 but for MDX just sets `view.legacyModelViewerAnimFrame = 0` without calling any renderer method. The MDX renderer's `step_animation_frame` is not called. This means frame stepping does not work for MDX models in C++.

- [ ] 522. [tab_models_legacy.cpp] seek_animation for MDX does not call renderer's set_animation_frame
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 473–480
  - **Status**: Pending
  - **Details**: JS `seek_animation` calls `renderer.set_animation_frame?.(parseInt(frame))` for both M2 and MDX. C++ `seek_animation` calls `active_renderer_m2->set_animation_frame(frame)` for M2 but for MDX only sets `view.legacyModelViewerAnimFrame = frame` without calling the MDX renderer. Animation frame seeking is broken for MDX models.

- [ ] 525. [tab_models_legacy.cpp] export_paths not closed in export_files PNG/CLIPBOARD path
  - **JS Source**: `src/js/modules/tab_models_legacy.js` line 322
  - **Status**: Pending
  - **Details**: JS always calls `export_paths?.close()` at the end of `export_files` regardless of format. In C++ `export_files`, the PNG/CLIPBOARD branch does not open or close `export_paths_writer` (for CLIPBOARD) — actually for PNG it opens one but closes it. For CLIPBOARD no export_paths is opened. For the OBJ/STL/RAW path, `export_paths_writer` is created in the task and closed... it appears there's no `export_paths_writer.close()` call in `pump_legacy_export` at the end. The writer is destroyed when `pending_legacy_export.reset()` is called (destructor). This might not flush properly if FileWriter requires explicit close.

- [ ] 528. [tab_raw.cpp] C++ uses `check.matches` (array) with `startsWith(patterns)` where JS uses `check.match` (single value) with `data.startsWith(check.match)`
  - **JS Source**: `src/js/modules/tab_raw.js` line 63
  - **Status**: Pending
  - **Details**: JS `constants.FILE_IDENTIFIERS` uses a single `match` property per identifier. C++ uses a `matches` array with `match_count`. If `match_count > 1` the C++ checks multiple patterns while JS checks only one. Could over-match relative to JS depending on constant definitions.

- [ ] 529. [tab_raw.cpp] Raw tab listbox is missing `:includefilecount="true"` — JS Listbox component is passed `:includefilecount="true"` but C++ `listbox::render` call does not pass an equivalent parameter
  - **JS Source**: `src/js/modules/tab_raw.js` line 147
  - **Status**: Pending
  - **Details**: The JS template passes `:includefilecount="true"` on the Listbox for tab-raw. The C++ `listbox::render` call (lines 328–354) does not set an equivalent `includefilecount` argument. If `listbox::render` supports this parameter it should be set to `true`.

- [ ] 530. [tab_text.cpp] `pump_text_export` calls `helper.mark(export_file_name, false, e.what())` omitting the stack trace; JS calls `helper.mark(export_file_name, false, e.message, e.stack)` with both message and stack
  - **JS Source**: `src/js/modules/tab_text.js` line 116
  - **Status**: Pending
  - **Details**: The C++ version omits the stack trace argument. The JS passes `e.stack` as a fourth argument to `helper.mark`. Should be passed to match JS fidelity (C++ `ExportHelper::mark` accepts an optional stack trace parameter).

- [ ] 532. [tab_text.cpp] Text tab listbox missing `:includefilecount="true"` — JS passes it but C++ `listbox::render` call does not
  - **JS Source**: `src/js/modules/tab_text.js` line 21
  - **Status**: Pending
  - **Details**: Same issue as tab_raw — JS passes `:includefilecount="true"` which should be reflected in C++ if the parameter is supported.

- [ ] 533. [tab_textures.cpp] `remove_override_textures` action is missing — JS template shows a toast with a "Remove" span calling `remove_override_textures()` when `overrideTextureList.length > 0`; no equivalent action exists in C++
  - **JS Source**: `src/js/modules/tab_textures.js` lines 284–288, 366–368
  - **Status**: Pending
  - **Details**: The JS template shows a progress-styled toast (`<div id="toast" v-if="!$core.view.toast && $core.view.overrideTextureList.length > 0" class="progress">`) with the override texture name and a "Remove" clickable span (`<span @click.self="remove_override_textures">Remove</span>`). The C++ comment (lines 603–606) says this is rendered in `renderAppShell`, but no C++ equivalent of `remove_override_textures()` is exposed or wired up. This functionality appears to be missing.

- [ ] 534. [tab_textures.cpp] Textures listbox missing `:includefilecount="true"` — JS passes it but C++ `listbox::render` call does not
  - **JS Source**: `src/js/modules/tab_textures.js` line 291
  - **Status**: Pending
  - **Details**: Same issue as tab_raw and tab_text.

- [ ] 535. [tab_textures.cpp] `export_textures()` passes a JSON object `{fileName: file}` for drop-handler path but a raw int JSON for single-file path — JS passes raw integer in both cases (`textureExporter.exportFiles([selected_file_data_id])`)
  - **JS Source**: `src/js/modules/tab_textures.js` lines 372–378
  - **Status**: Pending
  - **Details**: JS calls `textureExporter.exportFiles([selected_file_data_id])` where `selected_file_data_id` is a raw integer. C++ single-file path uses `files.push_back(selected_file_data_id)` (raw int as JSON), while the drop-handler path wraps in `{fileName: file}`. The two C++ paths are inconsistent and may not match what `texture_exporter::exportFiles` expects.

- [ ] 536. [tab_videos.cpp] Preview controls use a plain `Button("Export Selected")` instead of a `MenuButton` — JS uses `<MenuButton>` with options from `menuButtonVideos` allowing format selection (MP4/AVI/MP3/SUBTITLES) in the button dropdown
  - **JS Source**: `src/js/modules/tab_videos.js` lines 504–506
  - **Status**: Pending
  - **Details**: C++ export controls (lines 1167–1189) render a plain "Export Selected" button with no format dropdown. The user has no UI way to change the export format within the tab. This is a layout deviation — `menu_button::render` should be used here as in other tabs.

- [ ] 537. [tab_videos.cpp] Videos listbox missing `:includefilecount="true"` — JS passes it but C++ `listbox::render` call does not
  - **JS Source**: `src/js/modules/tab_videos.js` line 479
  - **Status**: Pending
  - **Details**: Same issue as other list tabs.

- [ ] 538. [tab_videos.cpp] `trigger_kino_processing` runs synchronously on the main thread blocking the UI; JS runs it as an async function, yielding between each fetch
  - **JS Source**: `src/js/modules/tab_videos.js` lines 380–464
  - **Status**: Pending
  - **Details**: The C++ version will freeze the UI for the duration of kino processing. JS awaits each fetch asynchronously. Should be moved to a background thread or pumped per-frame.

- [ ] 540. [tab_videos.cpp] `get_mp4_url` is a blocking `while(true)` poll loop running on the calling thread; when called from `export_mp4()` on the main thread this freezes the UI until the MP4 is ready
  - **JS Source**: `src/js/modules/tab_videos.js` lines 347–375
  - **Status**: Pending
  - **Details**: JS uses tail-recursive async calls with `setTimeout` delays. C++ blocks the calling thread. Export should be moved off the main thread.

- [ ] 543. [tab_zones.cpp] Zones listbox missing `:includefilecount="true"` — JS passes it but C++ `listbox_zones::render` call does not
  - **JS Source**: `src/js/modules/tab_zones.js` line 315
  - **Status**: Pending
  - **Details**: Same issue as other list tabs.

- [ ] 544. [tab_zones.cpp] `load_zone_map` runs synchronously on the main thread blocking the UI during tile loading; JS `load_zone_map` is async and yields between tile loads
  - **JS Source**: `src/js/modules/tab_zones.js` lines 275–288
  - **Status**: Pending
  - **Details**: For zones with many tiles, C++ will freeze the UI briefly. Should be moved to a background thread with progress reporting.

- [ ] 551. [audio-helper.cpp] `get_position` calculation differs from JS in how it computes elapsed time.
  - **JS Source**: `src/js/ui/audio-helper.js` lines 115–130
  - **Status**: Pending
  - **Details**: The JS computes `elapsed = context.currentTime - this.start_time` (wall-clock elapsed since `source.start()` was called), then `position = this.start_offset + elapsed`. The C++ uses `ma_sound_get_cursor_in_seconds()`, which returns the cursor relative to the seek point set in the decoder (i.e. it already accounts for `start_offset`). The C++ then adds `start_offset` again: `position = start_offset + cursor`. If `cursor` includes the decoded offset (i.e. reflects absolute playback from byte 0 of the stream), the result is correct; but if `ma_sound_get_cursor_in_seconds` returns elapsed-since-seek, the position is `start_offset` doubled. Needs verification that miniaudio returns an absolute stream cursor rather than elapsed-since-seek.

- [ ] 554. [character-appearance.cpp] `apply_customization_textures` omits `update()` call after each material `reset()`.
  - **JS Source**: `src/js/ui/character-appearance.js` lines 66–69
  - **Status**: Pending
  - **Details**: The JS calls `await chr_material.reset()` followed immediately by `await chr_material.update()` for every material in the reset loop. The C++ only calls `chr_material->reset()` with no subsequent `update()` call. The `update()` call in the JS forces the GPU texture to be refreshed after clearing — skipping it may leave stale texture data on the GPU until the next explicit `upload_textures_to_gpu` call.

- [ ] 555. [character-appearance.cpp] `apply_customization_textures` passes incomplete `chr_model_texture_layer` struct fields to `setTextureTarget`.
  - **JS Source**: `src/js/ui/character-appearance.js` lines 96–108
  - **Status**: Pending
  - **Details**: The JS `setTextureTarget` for the baked NPC case is called with the explicit object `{ BlendMode: 0, TextureType: texture_type, ChrModelTextureTargetID: [0, 0] }` as the fourth argument (the layer descriptor). The C++ passes `{ 0 }` — a single-element initialiser — which likely only sets `BlendMode` to 0 and leaves `TextureType` and `ChrModelTextureTargetID` default-initialised. If `setTextureTarget` uses `TextureType` from the layer argument, this will be wrong.

- [ ] 556. [character-appearance.cpp] `apply_customization_textures` skips entries where `chr_cust_mat->FileDataID` has no value, which has no JS equivalent guard.
  - **JS Source**: `src/js/ui/character-appearance.js` lines 123–130
  - **Status**: Pending
  - **Details**: The C++ checks `if (!chr_cust_mat->FileDataID.has_value()) continue;` before accessing `ChrModelTextureTargetID`. The JS accesses `chr_cust_mat.ChrModelTextureTargetID` directly without checking `FileDataID`. If `FileDataID` is optional in the C++ struct but always present in practice, this may silently skip valid entries that the JS would process.

- [ ] 557. [character-appearance.cpp] The `setTextureTarget` call for customization textures is missing the `BlendMode` from `chr_model_texture_layer`.
  - **JS Source**: `src/js/ui/character-appearance.js` lines 164
  - **Status**: Pending
  - **Details**: The JS calls `chr_material.setTextureTarget(chr_cust_mat, char_component_texture_section, chr_model_material, chr_model_texture_layer, true)` passing the full `chr_model_texture_layer` object (which includes `BlendMode`). The C++ passes `{ 0, 0, 0, static_cast<int>(get_field_int(*chr_model_texture_layer, "BlendMode")) }` as a simplified struct with a fixed layout. This may or may not match the expected fields in the C++ `setTextureTarget` signature — the order and meaning of the initialiser fields must be verified against `CharMaterialRenderer::setTextureTarget`.

- [ ] 573. [cache-collector.cpp] `parse_url` drops the query string from URLs
  - **JS Source**: `src/js/workers/cache-collector.js` lines 19–44
  - **Status**: Pending
  - **Details**: JS `https_request` passes `parsed.pathname + parsed.search` as the request path (line 25), which includes any query-string parameters. The C++ `parse_url` (cache-collector.cpp lines 157–188) only captures the path up to the first `/`, losing everything after `?`. URLs passed to the worker that include query parameters would produce incorrect HTTP requests.
