# TODO Tracker

> **Progress: 0/170 verified (0%)** — ✅ = Verified, ⬜ = Pending

<!-- ─── src/js/modules/font_helpers.cpp ──────────────────────────────────── -->

- [ ] 383. [font_helpers.cpp] `check_glyph_support` uses fundamentally different detection logic from JS
  - **JS Source**: `src/js/modules/font_helpers.js` lines 19–54
  - **Status**: Pending
  - **Details**: The JS `check_glyph_support(ctx, font_family, char)` works by rendering the character twice to an off-screen canvas — once with a fallback font (`32px monospace`) and once with the target font — and comparing the total alpha channel sum of the two renders (lines 38–53). A character is considered supported if the rendered output differs from the fallback. This approach detects whether the *target font* actually has a glyph for the codepoint, even when the font is not loaded into ImGui. The C++ implementation (font_helpers.cpp lines 54–63) instead calls `ImFont::FindGlyphNoFallback()` on an already-loaded ImGui font. These are not equivalent: the JS detects support in *any* font family loaded in the browser, while the C++ detects support only in an *already-loaded ImGui font*. For glyphs that exist in the OS font but were not baked into the ImGui atlas (e.g., because the atlas only baked a subset of codepoints), the C++ will incorrectly report the glyph as not supported. This is an unavoidable architectural difference between browser/DOM and ImGui, but the deviation should be documented.

- [ ] 384. [font_helpers.cpp] `detect_glyphs_async` signature differs from JS — `grid_element` parameter replaced with `GlyphDetectionState&`
  - **JS Source**: `src/js/modules/font_helpers.js` lines 56–106
  - **Status**: Pending
  - **Details**: JS signature is `detect_glyphs_async(font_family, grid_element, on_glyph_click, on_complete)`. The second parameter `grid_element` is a DOM element that the function populates with `<span>` cells for each detected glyph. In the C++ port, this DOM manipulation is replaced by storing detected codepoints into `GlyphDetectionState::detected_codepoints` and deferring UI rendering to the caller. The JS builds the UI incrementally inside the async batch loop (creating DOM nodes immediately when each glyph is found). The C++ collects all codepoints first, then lets the caller iterate `detected_codepoints` during the ImGui render loop. This means the C++ port does not populate the glyph grid incrementally during detection — the grid only appears after `state.complete == true`, rather than growing batch-by-batch as in JS. Callers that rely on incremental grid population will see a different UX.

- [ ] 385. [font_helpers.cpp] `inject_font_face` — missing font load verification equivalent to JS `document.fonts.load` + `document.fonts.check`
  - **JS Source**: `src/js/modules/font_helpers.js` lines 113–133
  - **Status**: Pending
  - **Details**: After injecting the CSS `@font-face` rule, the JS awaits `document.fonts.load('16px "' + font_id + '"')` (line 124) and then checks `document.fonts.check('16px "' + font_id + '"')` (line 125). If the check returns false, the style node is removed from the DOM and an error is thrown (lines 127–130). The C++ implementation calls `io.Fonts->AddFontFromMemoryTTF(...)` and checks for a null return (line 171), but there is no equivalent to the post-load verification step. If ImGui accepts the font data but it is internally corrupt or unusable, the C++ will not detect this and will not clean up, whereas the JS would detect it and throw. This is a minor error-recovery gap.

- [ ] 386. [font_helpers.cpp] `inject_font_face` — JS accepts `log` and `on_error` callback parameters that have no C++ equivalent
  - **JS Source**: `src/js/modules/font_helpers.js` lines 113–133
  - **Status**: Pending
  - **Details**: JS signature is `inject_font_face(font_id, blob_data, log, on_error)`. The `on_error` parameter is never called in the JS body (errors are thrown), so its absence in C++ does not cause a behavioral difference. The `log` parameter is also unused in the JS body (logging is not called inside `inject_font_face` in the JS). The C++ signature correctly omits both unused parameters. This is not a bug but is documented for completeness.

- [ ] 387. [font_helpers.cpp] `get_detection_canvas()` helper and `glyph_detection_canvas`/`glyph_detection_ctx` state have no C++ equivalent
  - **JS Source**: `src/js/modules/font_helpers.js` lines 15–28
  - **Status**: Pending
  - **Details**: The JS maintains a singleton off-screen canvas (`glyph_detection_canvas`) and its 2D context (`glyph_detection_ctx`) for pixel-level glyph detection. The `get_detection_canvas()` helper lazily creates this canvas on first use. The C++ omits this entirely because it replaced the canvas-based detection with an ImGui atlas lookup. This is an expected consequence of the `check_glyph_support` deviation documented in finding 5, but the absence of any canvas state is explicitly noted here for completeness.

<!-- ─── src/js/modules/legacy_tab_audio.cpp ──────────────────────────────── -->

- [ ] 388. [legacy_tab_audio.cpp] `export_sounds` passes only message+function to `helper.mark()`, not the full stack trace
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` line 189
  - **Status**: Pending
  - **Details**: JS calls `helper.mark(export_file_name, false, e.message, e.stack)` where `e.stack` is the full JavaScript stack trace string. C++ calls `helper.mark(export_file_name, false, e.what(), build_stack_trace("export_sounds", e))` where `build_stack_trace` (lines 33–35) returns only `"export_sounds: <exception message>"`. The C++ never captures a real C++ stack trace (e.g., via `std::stacktrace` from C++23 or a platform API). The export helper's `mark()` function receives a weaker error context string than the JS version provides to users in the export report.

- [ ] 389. [legacy_tab_audio.cpp] Animated music icon uses `ImDrawList::AddText` raw draw call instead of a native ImGui widget
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 216–216 (template `sound-player-anim` div)
  - **Status**: Pending
  - **Details**: The JS template renders `<div id="sound-player-anim" :style="{ 'animation-play-state': ... }">` — a CSS-animated element. The C++ replacement (lines 440–449) uses `ImGui::GetWindowDrawList()->AddText(iconFont, animSize, pos, ...)` which is a raw `ImDrawList` call. Per CLAUDE.md, `ImDrawList` calls should be reserved exclusively for effects with no native equivalent such as image rotation, multi-colour gradient fills, or custom OpenGL overlays. A pulsating text icon does not fall into any of those categories; `ImGui::Text` with a scaled font (via `ImGui::PushFont`/`ImGui::PopFont` or by setting the font size) would be a closer match using a native widget approach. The current use of `AddText` violates the ImGui rendering guideline.

- [ ] 390. [legacy_tab_audio.cpp] `start_seek_loop` vs JS — C++ does not pass `core` parameter to `update_seek()`
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 32–35
  - **Status**: Pending
  - **Details**: This is a structural note rather than a bug. JS `update_seek(core)` and `start_seek_loop(core)` receive the `core` object as a parameter because the JS module functions are stateless closures. C++ functions access `core::view` via a global, so the parameter is omitted. This is the correct adaptation for C++. No fix needed, but documented for completeness.

- [ ] 391. [legacy_tab_audio.cpp] `load_sound_list` condition differs: JS checks `!core.view.isBusy` (falsy), C++ checks `view.isBusy > 0`
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` line 124
  - **Status**: Pending
  - **Details**: JS guard is `if (core.view.listfileSounds.length === 0 && !core.view.isBusy)`. C++ guard is `if (!view.listfileSounds.empty() || view.isBusy > 0) return;` (lines 169–170), which is the logical negation of the JS entry condition. If `isBusy` is an integer counter (which it is based on `BusyLock` usage), `isBusy > 0` correctly mirrors JS's `!isBusy` (falsy when 0). However, if `isBusy` could theoretically be negative (e.g., due to a lock mismatch), JS would still proceed but C++ would not. This is a minor robustness note and not a real bug under normal operation.

- [ ] 392. [legacy_tab_audio.cpp] Sound player UI renders seek/title/duration on one `ImGui::Text` line rather than in three separate labelled spans
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 219–221 (template)
  - **Status**: Pending
  - **Details**: JS template renders three separate `<span>` elements: one for seek time (left-aligned), one for the track title (centre, with class `title`), and one for duration (right-aligned), all within a flex-row `#sound-player-info` div. The C++ renders all three concatenated in a single `ImGui::Text("%s  %s  %s", ...)` call (line 460). The layout is compressed onto one line with no alignment differentiation between the three fields. The JS gives the title a `class="title"` style (typically larger/bolder text and centred within the flex row). The C++ does not replicate the three-column flex alignment or the title styling. This is a UI layout deviation.
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

- [ ] 403. [module_test_a.cpp] counter reset in mounted() is an addition not present in JS mounted()
  - **JS Source**: `src/js/modules/module_test_a.js` lines 27–29
  - **Status**: Pending
  - **Details**: The JS `mounted()` only calls `console.log('module_test_a mounted')`. The C++ `mounted()` additionally resets `counter = 0`. The justification (data() returns fresh state on each Vue mount) is architecturally valid, but it is a behavioural addition not explicitly present in the JS `mounted()` body. Document as an intentional port decision.


<!-- ─── src/js/modules/module_test_b.cpp ──────────────────────────────────── -->

- [ ] 404. [module_test_b.cpp] inputTextResizeCallback uses BufTextLen instead of BufSize
  - **JS Source**: `src/js/modules/module_test_b.js` lines 6 (v-model binding)
  - **Status**: Pending
  - **Details**: The resize callback calls `str->resize(data->BufTextLen)` but the standard ImGui pattern for dynamic-string InputText is to resize to `data->BufSize` (the allocated buffer capacity requested), not `BufTextLen` (current text length). Using `BufTextLen` means the string backing buffer is always exactly the current text length, which may cause the buffer pointer passed to ImGui to become stale on the next character insertion when the string needs to grow. The correct pattern is `str->resize(static_cast<size_t>(data->BufSize))` followed by `data->Buf = str->data()`.

- [ ] 405. [module_test_b.cpp] InputText buf_size argument passes capacity()+1 instead of capacity()+1 with proper reserve
  - **JS Source**: `src/js/modules/module_test_b.js` lines 6 (v-model="message")
  - **Status**: Pending
  - **Details**: The call `ImGui::InputText("##message", message.data(), message.capacity() + 1, ...)` passes `capacity()+1` as the buffer size, but `std::string::capacity()` does not guarantee space for a null terminator beyond capacity bytes — writing `capacity()+1` characters would overflow the string's internal buffer. The standard pattern is to ensure the string is resized to at least 1 byte (so `data()` is valid), reserve extra capacity, and use `capacity()` (not `capacity()+1`) as the size argument, or alternatively use `message.size() + 1` with a fixed reserved capacity. The reserve(256) before the call only happens if `capacity() < 256`, so on a fresh "Hello Thrall" string this is fine, but the `capacity()+1` buffer-size argument is still technically incorrect.

- [ ] 406. [module_test_b.cpp] counter reset in mounted() is an addition not present in JS mounted() (same as module_test_a)
  - **JS Source**: `src/js/modules/module_test_b.js` lines 36–38
  - **Status**: Pending
  - **Details**: Same pattern as module_test_a: JS `mounted()` only logs. C++ `mounted()` also resets `message = "Hello Thrall"`. The justification (data() returns fresh state on each mount) is valid and mirrors the pattern in module_test_a, but should be documented as intentional.


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

- [ ] 410. [screen_settings.cpp] JS handle_apply() does not call config::save() explicitly; C++ does
  - **JS Source**: `src/js/modules/screen_settings.js` lines 447–449
  - **Status**: Pending
  - **Details**: The JS sets `this.$core.view.config = cfg` and relies on a Vue watcher (`$watch`) to call `save()` reactively. The C++ calls `config::save()` explicitly after setting `core::view->config = cfg`. This is functionally equivalent (the save happens in both cases) but the mechanism differs. This should be documented as an intentional deviation from the JS flow since C++ has no reactive watchers.

- [ ] 411. [screen_settings.cpp] JS handle_apply() validation typo: "DBD manfiest" — C++ correctly spells "DBD manifest"
  - **JS Source**: `src/js/modules/screen_settings.js` line 445
  - **Status**: Pending
  - **Details**: The JS source has a typo in the toast message: `'A valid URL is required for DBD manfiest.'` (missing 'i' in manifest). The C++ corrects this to `"A valid URL is required for DBD manifest."`. This is a minor intentional correction that diverges from the JS source text.

- [ ] 412. [screen_settings.cpp] set_selected_cdn (in screen_source_select) calls config::save() — JS does not
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 78–83
  - **Status**: Pending
  - **Details**: This finding belongs to screen_source_select.cpp (see below), but is noted here because the save call pattern appears in both files. The settings screen's `handle_apply()` correctly saves. No additional issue here beyond the note above.

- [ ] 413. [screen_settings.cpp] JS mounted() uses Object.assign({}, config) for shallow copy; C++ directly assigns
  - **JS Source**: `src/js/modules/screen_settings.js` lines 461–463
  - **Status**: Pending
  - **Details**: The JS uses `Object.assign({}, this.$core.view.config)` which is a shallow copy. The C++ uses `core::view->configEdit = core::view->config` which is a direct assignment of nlohmann::json (deep copy by value). This is correct since nlohmann::json assignment performs a deep copy, so the behavior is identical.

- [ ] 414. [screen_settings.cpp] JS handle_reset uses JSON.parse(JSON.stringify(defaults)) for deep clone; C++ uses .dump()/.parse()
  - **JS Source**: `src/js/modules/screen_settings.js` lines 452–457
  - **Status**: Pending
  - **Details**: The JS `handle_reset()` calls `JSON.parse(JSON.stringify(defaults))` for a deep clone. The C++ uses `nlohmann::json::parse(defaults.dump())` which is the equivalent but slightly less efficient round-trip. This is functionally equivalent and not a bug, but could be simplified to `core::view->configEdit = defaults` since nlohmann::json operator= performs a deep copy.

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

- [ ] 419. [screen_source_select.cpp] init_cdn_pings: JS updates cdnRegions array reactively after each ping; C++ only updates individual region delay
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 227–232
  - **Status**: Pending
  - **Details**: In the JS, after each ping resolves, it does `this.$core.view.cdnRegions = [...regions]` (spread to create a new array reference, triggering Vue reactivity). The C++ posts only `regions[index]["delay"] = delay` to the main thread. Since C++ is not Vue-reactive, this is intentional — the ImGui render loop will pick up the change naturally. No functional issue, but worth noting as a documented deviation.

- [ ] 420. [screen_source_select.cpp] open_local_install: JS passes product to load_install via findIndex on builds directly; C++ iterates builds array
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 151–153
  - **Status**: Pending
  - **Details**: The JS finds the build index via `casc_source.builds.findIndex(build => build.Product === product)`. The C++ iterates `src->builds` with a for loop checking `builds[i].count("Product") && builds[i].at("Product") == product`. The C++ adds an extra `.count("Product")` guard before accessing the field. This is more defensive than the JS but diverges slightly — if a build entry lacks the "Product" key, C++ skips it while JS would return -1 from findIndex (since undefined !== product). Functionally equivalent for well-formed data.

- [ ] 421. [screen_source_select.cpp] JS uses single casc_source variable for both local and remote; C++ uses two separate unique_ptrs
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 13–15
  - **Status**: Pending
  - **Details**: The JS uses a single `casc_source` variable that holds either a `CASCLocal` or `CASCRemote` instance, distinguished by `instanceof` checks. The C++ uses two separate `std::unique_ptr` (`casc_local_source` and `casc_remote_source`) plus an `active_source_type` enum. This is an intentional architectural deviation for type safety. The behavior is equivalent (only one is ever non-null at a time), but the structure differs. Document as intentional.

- [ ] 422. [screen_source_select.cpp] CDN region delay comparison in best-region selection: JS uses null check; C++ uses different null semantics
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 240–247
  - **Status**: Pending
  - **Details**: The JS checks `if (region.delay === null || region.delay < 0) continue` and then `if (region.delay < selected_region.delay)`. The C++ checks `if (!region.contains("delay") || region["delay"].is_null() || !region["delay"].is_number()) continue` and `if (selected_region.contains("delay") && selected_region["delay"].is_number() && delay < selected_region["delay"].get<int64_t>())`. The C++ is more defensive. The JS would crash if `selected_region.delay` is null when comparing (since `null < number` is false in JS due to type coercion). The C++ handles this more correctly but deviates from the JS structure.

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

- [ ] 428. [tab_audio.cpp] load_track async flow differs: JS uses RAII busy lock, C++ defers it outside async task
  - **JS Source**: `src/js/modules/tab_audio.js` lines 47–85
  - **Status**: Pending
  - **Details**: In the JS `load_track`, `using _lock = core.create_busy_lock()` holds the busy lock for the full async call including the CASC read and post-processing. In the C++ implementation, the `BusyLock` is created in `PendingAudioLoad.busy_lock` at dispatch time but is released via `pending_audio_load.reset()` after the async result is processed, which matches the intent. However, if the load is cancelled via `pending_audio_load.reset()` (in `unload_track` or a new `load_track` call), the busy lock is released without the toast being hidden or `soundPlayerDuration` being reset. The JS flow is a single async function where cancellation isn't possible mid-flight; the C++ cancellation path may leave the UI in a stale "progress" state.

- [ ] 429. [tab_audio.cpp] load_track: C++ does not restore file_data object (JS: file_data.revokeDataURL on unload)
  - **JS Source**: `src/js/modules/tab_audio.js` lines 87–97
  - **Status**: Pending
  - **Details**: `unload_track` in JS calls `file_data?.revokeDataURL()` and sets `file_data = null`. The C++ `unload_track` has no equivalent because it stores data in the audio player directly, not in a separate `file_data` reference. This is an acceptable C++ deviation but should be documented. No functional impact since C++ doesn't use object URLs.

- [ ] 430. [tab_audio.cpp] play_track: C++ path differs when no buffer — JS awaits load_track and returns on failure
  - **JS Source**: `src/js/modules/tab_audio.js` lines 99–114
  - **Status**: Pending
  - **Details**: In JS `play_track`, if no buffer, it calls `const loaded = await load_track(core); if (!loaded) return;` — meaning if the load fails, play is not triggered. In C++, `play_track` starts the async load with `auto_play = true`, but if the async load later fails (e.g. encryption error), `auto_play` is already set and the error toast is shown. However, there is no path where `should_play` is used after a failure — the catch block does not call `player.play()`, so this is functionally equivalent, but the code flow relies on `should_play` being checked before calling `player.play()` in the success branch only.

- [ ] 431. [tab_audio.cpp] export_sounds: C++ processes one file per frame (pump), JS processes all files in a loop
  - **JS Source**: `src/js/modules/tab_audio.js` lines 122–180
  - **Status**: Pending
  - **Details**: JS `export_sounds` is a single async function that iterates all files with `for...of`, checking `helper.isCancelled()` each iteration. The C++ implementation uses a per-frame pump (`pump_audio_export`) that processes one file per frame. This is an intentional C++ adaptation for non-blocking rendering. However the JS version calls `export_data = await core.view.casc.getFile(...)` for `.unk_sound` files to detect type, then potentially changes `file_name`, then calls `getFileByName(file_name)` again if the data was already fetched. The C++ version calls `getVirtualFileByName(file_name)` once for the unk_sound file detection and then conditionally again for export — this matches the JS logic correctly.

- [ ] 432. [tab_audio.cpp] soundPlayerTitle display — C++ formats seek/title/duration differently from JS
  - **JS Source**: `src/js/modules/tab_audio.js` lines 207–210
  - **Status**: Pending
  - **Details**: The JS template shows three separate `<span>` elements: `soundPlayerSeekFormatted`, `soundPlayerTitle` (styled with CSS class `title`), and `soundPlayerDurationFormatted`. The C++ renders them all on one `ImGui::Text` line with `%s  %s  %s`. The JS uses `soundPlayerSeekFormatted` and `soundPlayerDurationFormatted` as pre-formatted view properties. The C++ computes `format_time(view.soundPlayerSeek * view.soundPlayerDuration)` inline. The JS properties `soundPlayerSeekFormatted` and `soundPlayerDurationFormatted` are computed properties in the Vue state (not shown in tab_audio.js directly, likely computed in the view model). The C++ inline computation is likely correct but the properties should be verified against the view model.

- [ ] 433. [tab_audio.cpp] Volume slider display format uses "Vol: %.0f%%" but JS uses a plain slider with no format string
  - **JS Source**: `src/js/modules/tab_audio.js` lines 213–215
  - **Status**: Pending
  - **Details**: The JS volume slider (`<component :is="$components.Slider" id="slider-volume" v-model="$core.view.config.soundPlayerVolume">`) has no explicit label. The C++ renders `ImGui::SliderFloat("##VolumeSlider", &vol, 0.0f, 1.0f, "Vol: %.0f%%")`. The `Vol: %.0f%%` format string will display the raw float 0–1 value multiplied by nothing (it won't display as a percentage automatically — `%.0f` on a value of `0.5` gives "Vol: 0%", not "Vol: 50%"). The slider value is in range 0–1, so the format string is incorrect — it would need `(vol * 100)` to display percentage properly.

- [ ] 434. [tab_audio.cpp] mounted: JS initializes soundPlayerVolume/Loop with Vue reactivity; C++ uses change-detection polling
  - **JS Source**: `src/js/modules/tab_audio.js` lines 270–313
  - **Status**: Pending
  - **Details**: The JS `initialize` and `mounted` set up `$watch('config.soundPlayerVolume', ...)` and `$watch('config.soundPlayerLoop', ...)` for reactive updates. The C++ equivalent polls `current_volume != prev_sound_player_volume` each frame in `render()`. This is functionally equivalent but the C++ `mounted()` function is named `mounted()` while the JS name is `initialize()` (called from `mounted()`); the C++ correctly combines them.

- [ ] 435. [tab_audio.cpp] mounted: JS crash handler calls unload_track then player.destroy; C++ crash handler matches
  - **JS Source**: `src/js/modules/tab_audio.js` lines 329–332
  - **Status**: Pending
  - **Details**: The C++ `mounted()` registers the crash event handler correctly. One minor difference: the JS watches `selectionSounds` and checks `!this.$core.view.isBusy` (truthy check), while the C++ checks `view.isBusy == 0`. If `isBusy` can be a non-integer truthy value in JS but is always an integer in C++, these are equivalent. This is fine.

<!-- ─── src/js/modules/tab_blender.cpp ────────────────────────────────────── -->

- [ ] 438. [tab_blender.cpp] Version comparison uses lexicographic string comparison instead of numeric
  - **JS Source**: `src/js/modules/tab_blender.js` lines 91, 139
  - **Status**: Pending
  - **Details**: JS `start_automatic_install`: `if (version >= constants.BLENDER.MIN_VER)` — in JS, comparing a string like `"2.8"` to a number `2.8` triggers type coercion; in practice JS converts the string to a float for comparison. In `checkLocalVersion`: `if (blender_version < constants.BLENDER.MIN_VER)` — same pattern. The C++ code correctly converts to `double` via `std::stod(version)` before comparing, which is the right behavior. This is fine and not a bug.

- [ ] 439. [tab_blender.cpp] parse_manifest_version: JS returns string on success, object on error; C++ uses struct
  - **JS Source**: `src/js/modules/tab_blender.js` lines 12–27
  - **Status**: Pending
  - **Details**: Minor: The C++ uses a `ManifestResult` struct with `version` and `error` fields, which correctly maps the JS duck-typed return. The check `typeof latest_addon_version === 'object'` in JS maps to `!latestAddonVersion.error.empty()` in C++. Functionally equivalent. No issue.

- [ ] 440. [tab_blender.cpp] checkLocalVersion: version comparison is string lexicographic in C++, numeric-coerced in JS
  - **JS Source**: `src/js/modules/tab_blender.js` lines 161–163
  - **Status**: Pending
  - **Details**: JS line 163: `if (latest_addon_version > blender_addon_version)` — both are version strings like `"1.2.3"`. JS does a lexicographic string comparison. C++ does the same: `if (latestAddonVersion.version > blenderAddonVersion.version)`. For semantic versioning this can be incorrect (e.g. `"1.9.0" > "1.10.0"` is true lexicographically but false numerically). However this matches the JS behavior exactly, so it is a faithful port.

- [ ] 441. [tab_blender.cpp] checkLocalVersion: JS logs blender_addon_version even if it has an error; C++ also logs error value
  - **JS Source**: `src/js/modules/tab_blender.js` lines 160–161
  - **Status**: Pending
  - **Details**: JS line 161: `log.write('Latest add-on version: %s, Blender add-on version: %s', latest_addon_version, blender_addon_version)` — the blender_addon_version here could be an error object `{ error: '...' }`. The JS would stringify it as `[object Object]`. The C++ uses `blenderAddonVersion.error.empty() ? blenderAddonVersion.version : blenderAddonVersion.error`, which is slightly more informative than the JS behavior but not strictly identical. Minor deviation, non-impactful.

- [ ] 442. [tab_blender.cpp] render(): title text says "wow.export.cpp Add-on" but JS says "wow.export Add-on"
  - **JS Source**: `src/js/modules/tab_blender.js` line 61
  - **Status**: Pending
  - **Details**: JS template header: `<h1>Installing the wow.export Add-on for Blender 2.8+</h1>`. C++ renders: `ImGui::Text("Installing the wow.export.cpp Add-on for Blender 2.8+")`. Per CLAUDE.md, user-facing text should say "wow.export.cpp", so this is intentionally correct per project rules.

- [ ] 443. [tab_blender.cpp] render() missing: no animation/frame controls overlay as the tab is non-interactive except buttons
  - **JS Source**: `src/js/modules/tab_blender.js` lines 58–70
  - **Status**: Pending
  - **Details**: The JS template has a single `<div id="blender-info">` with a header section and buttons section. The C++ renders the equivalent with ImGui Text, TextWrapped, Separator, and Buttons. Layout is functionally equivalent for what the tab does. No missing interactive controls.


<!-- ─── src/js/modules/tab_characters.cpp ─────────────────────────────────── -->

- [ ] 444. [tab_characters.cpp] reset_module_state: C++ does not clean up watcher_cleanup_funcs (no equivalent)
  - **JS Source**: `src/js/modules/tab_characters.js` lines 254–272
  - **Status**: Pending
  - **Details**: JS `reset_module_state` iterates `watcher_cleanup_funcs` and calls each cleanup function, then clears the array. The C++ has no equivalent `watcher_cleanup_funcs` array because watchers are handled as change-detection polling in `render()`. The lifecycle is different but functionally equivalent since C++ watchers are not heap-allocated. No functional bug, but the JS pattern is documented for completeness.

- [ ] 445. [tab_characters.cpp] load_character: JS also clears chrSavedCharactersScreen = false; C++ does the same — OK
  - **JS Source**: `src/js/modules/tab_characters.js` lines 1359–1401
  - **Status**: Pending
  - **Details**: JS `load_character` sets `chrSavedCharactersScreen = false` and `chrModelLoading = true/false`. The C++ implementation in `load_character()` at line 2021 also sets these. Matches. No issue.

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

- [ ] 449. [tab_characters.cpp] import_json_character: "load directly into viewer" path missing guild_tabard config application
  - **JS Source**: `src/js/modules/tab_characters.js` lines 1656–1675
  - **Status**: Pending
  - **Details**: JS `import_json_character` "load directly into viewer" path (line 1656–1675) does NOT apply guild_tabard config — it only applies equipment, equipment_skins, choices, model_id and race_id. The C++ implementation at line ~2342 applies `guild_tabard` config in the "load directly into viewer" branch. This is a deviation from the JS behavior: the JS does not restore guild tabard when loading a character JSON directly; the C++ does. This is likely a bug in the C++ (extra behavior not present in JS).

- [ ] 450. [tab_characters.cpp] mounted: JS shows 10 loading steps; C++ shows 8
  - **JS Source**: `src/js/modules/tab_characters.js` line 2706
  - **Status**: Pending
  - **Details**: JS: `this.$core.showLoadingScreen(10)` — 10 loading steps. C++: `core::showLoadingScreen(8)` — 8 loading steps. The JS has separate progress steps for DBItemList and charShaders that the C++ may have consolidated. Minor UI discrepancy.

- [ ] 451. [tab_characters.cpp] mounted: JS initializes DBItemList with progress callback; C++ does not call DBItemList init
  - **JS Source**: `src/js/modules/tab_characters.js` line 2759
  - **Status**: Pending
  - **Details**: JS: `await DBItemList.initialize((msg) => this.$core.progressLoadingScreen(msg))` — initializes a separate `DBItemList` cache with a progress callback. The C++ mounted does not appear to call any equivalent `DBItemList::initialize()` or `DBItemList::ensureInitialized()`. This means item list data used for the item picker may not be loaded, or it is initialized elsewhere. This needs verification.

- [ ] 452. [tab_characters.cpp] mounted: C++ sets chrModelViewerContext as a JSON object; JS sets it as a typed object
  - **JS Source**: `src/js/modules/tab_characters.js` lines 2766–2774
  - **Status**: Pending
  - **Details**: JS mounted sets `state.chrModelViewerContext = { gl_context: null, controls: null, useCharacterControls: true, fitCamera: null, getActiveRenderer: () => ..., getEquipmentRenderers: () => ..., getCollectionRenderers: () => ... }`. The C++ sets `state.chrModelViewerContext = nlohmann::json{...}` as a JSON object, which cannot store C++ function references. The actual function callbacks are wired via `viewer_context` (a `model_viewer_gl::Context` struct), not via the JSON object. This is an architectural deviation but appears intentional and necessary.

- [ ] 453. [tab_characters.cpp] render(): outside-click handler to close import panel is incomplete
  - **JS Source**: `src/js/modules/tab_characters.js` lines 2833–2849
  - **Status**: Pending
  - **Details**: The JS click handler closes `characterImportMode` when clicking outside both the import panel AND the BNet/Wowhead buttons. The C++ at lines ~2973–2977 only closes when `!ImGui::IsAnyItemHovered()`, which is a crude approximation. It will close the panel even when clicking on the import panel itself (since `IsAnyItemHovered` checks hovered items, not the panel). The exact JS behavior cannot be fully replicated with ImGui because ImGui doesn't have DOM hit-testing. This is a known ImGui limitation but the current implementation is overly aggressive.

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

- [ ] 461. [tab_creatures.cpp] export_files: mark_name not used as export mark for standard creature export
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 954–971
  - **Status**: Pending
  - **Details**: In the JS, `export_model()` returns `mark_name` and `helper.mark(mark_name, true)` is called with that value. In the C++ `export_files()` (line ~1399), `model_viewer_utils::export_model(opts)` return value is stored in `mark_name`, but `helper.mark(creature_name, true)` is called instead of `helper.mark(mark_name, true)`. This means the mark label always uses `creature_name` instead of the actual exported file path/name returned by `export_model`, deviating from JS behaviour.

- [ ] 462. [tab_creatures.cpp] Missing export_paths.writeLine for PNG/CLIPBOARD export path
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 738–748
  - **Status**: Pending
  - **Details**: In the JS, after the PNG/CLIPBOARD export branch completes, `export_paths?.close()` is called. In the C++ (line ~1110), `export_paths.close()` is **not** called after the early return from the PNG/CLIPBOARD branch. This means the export stream may not be properly closed when using those formats.

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

- [ ] 469. [tab_creatures.cpp] Export: `export_paths` stream not passed to `export_model` opts
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 954–969
  - **Status**: Pending
  - **Details**: In JS, `export_paths` is passed as a property in the `modelViewerUtils.export_model({..., export_paths})` call. In C++, `opts.export_paths = &export_paths` is set (line ~1396), which looks correct. This item is tentatively fine, but the export_paths field should be verified that `export_model` actually uses it for writing lines.


<!-- ─── src/js/modules/tab_data.cpp ───────────────────────────────────────── -->

- [ ] 470. [tab_data.cpp] Listbox is missing :copydir / copydir binding
  - **JS Source**: `src/js/modules/tab_data.js` lines 97–99
  - **Status**: Pending
  - **Details**: The JS template passes `:copydir="$core.view.config.copyFileDirectories"` to the Listbox for the DB2 list. The C++ `listbox::render()` call (line ~274) does not pass a `copydir` equivalent. The C++ listbox function signature may not have this parameter, but the config value `copyFileDirectories` should still be wired if the listbox supports it.

- [ ] 471. [tab_data.cpp] initialize_available_tables function is defined but never called from mounted()
  - **JS Source**: `src/js/modules/tab_data.js` lines 12–21, 179–183
  - **Status**: Pending
  - **Details**: In JS, `initialize()` calls `initialize_available_tables()` inside a loading screen. In C++, `initialize_available_tables()` is defined (line ~69) but `mounted()` (line ~219) does not call it — instead it runs the initialization inline in a `std::thread`. The standalone `initialize_available_tables()` function is effectively dead code. This is not a functional issue but is a code quality concern.

- [ ] 472. [tab_data.cpp] export_csv single-table: rows_to_export uses all_rows directly, not filtered/selected rows
  - **JS Source**: `src/js/modules/tab_data.js` lines 217–229
  - **Status**: Pending
  - **Details**: In the JS `export_csv()`, when exporting a single table in non-export-all mode, `rows_to_export = [...selection]` which is the actual selected row **data** (the selection in the DataTable component is the selected row values, not indices). In the C++ `export_csv()` single-table branch (line ~574), the selection holds row **indices** into the sorted display rows, which are then looked up. This is architecturally different but should produce equivalent results. However, it warrants verification that indices match the displayed rows correctly (sorted + filtered).

- [ ] 473. [tab_data.cpp] active_table local variable not updated after failed load_table
  - **JS Source**: `src/js/modules/tab_data.js` lines 371–377
  - **Status**: Pending
  - **Details**: In JS, `this.active_table = selected_file` is set after `await load_table(...)` — so it's only updated on success (because `selected_file` is only updated on success inside `load_table()`). In C++ `render()` (line ~254), `active_table = selected_file` is set after calling `load_table(last)`. Since `selected_file` is only updated on success in `load_table()`, this matches. No actual issue here — the difference is moot. Marking as informational.


<!-- ─── src/js/modules/tab_decor.cpp ──────────────────────────────────────── -->

- [ ] 474. [tab_decor.cpp] Listbox unittype is "decor item" in C++ but "item" in JS template
  - **JS Source**: `src/js/modules/tab_decor.js` line 233
  - **Status**: Pending
  - **Details**: In JS, the Listbox has `unittype="item"`. In C++ (line ~775) `listbox::render()` is called with `"decor item"` as the unittype. This causes different wording in file-count displays like "X items" vs "X decor items".

- [ ] 475. [tab_decor.cpp] toggle_category_group differs from JS: JS delegates to setDecorCategoryGroup, C++ implements inline
  - **JS Source**: `src/js/modules/tab_decor.js` lines 513–516
  - **Status**: Pending
  - **Details**: In JS, `toggle_category_group(group)` calls `this.$core.view.setDecorCategoryGroup(group.id, !all_checked)` which is a view method that presumably updates the whole group including the Uncategorized synthetic entry. In C++, it directly toggles `sub->checked` on pointers into `category_mask`. This is functionally equivalent for non-Uncategorized groups, but the Uncategorized group's single entry (`[uncategorized]`) is also handled correctly via the pointer. Functionally equivalent but the indirection via `setDecorCategoryGroup` is lost.

- [ ] 476. [tab_decor.cpp] Missing toggle_checklist_item method
  - **JS Source**: `src/js/modules/tab_decor.js` lines 509–511
  - **Status**: Pending
  - **Details**: JS has a `toggle_checklist_item(item)` method that toggles `item.checked`. In the C++ template, subcategory checkboxes are rendered with `ImGui::Checkbox(&sub->checked)` directly (line ~1071), which directly modifies the value. The JS method is effectively inlined, so there is no functional omission — this is an observation that the JS method is not needed in C++ because ImGui::Checkbox modifies the value in place.

- [ ] 477. [tab_decor.cpp] decorAutoPreview auto-preview check missing `isBusy` guard
  - **JS Source**: `src/js/modules/tab_decor.js` lines 582–603
  - **Status**: Pending
  - **Details**: In the JS watch on `selectionDecor`, the guard is `if (!first || this.$core.view.isBusy)`. The C++ `render()` watch implementation (line ~724) checks `view.isBusy == 0` correctly — this is fine. No issue.

- [ ] 478. [tab_decor.cpp] export_files: active_renderer check uses active_file_data_id != 0 vs active_renderer
  - **JS Source**: `src/js/modules/tab_decor.js` lines 129–131
  - **Status**: Pending
  - **Details**: JS checks `if (active_file_data_id)` (truthy check). C++ checks `if (active_file_data_id != 0)` (line ~243). These are equivalent for uint32_t — no issue.

- [ ] 479. [tab_decor.cpp] Missing "Apply pose" export checkbox in sidebar
  - **JS Source**: `src/js/modules/tab_decor.js` — (not present in decor JS template)
  - **Status**: Pending
  - **Details**: The JS decor template does NOT include an "Apply pose" checkbox. The C++ decor sidebar also does NOT include one. This is consistent. No issue.

- [ ] 480. [tab_decor.cpp] Decor export missing `active_renderer` field in ExportModelOptions
  - **JS Source**: `src/js/modules/tab_decor.js` lines 167–180
  - **Status**: Pending
  - **Details**: In JS, `export_model()` is called without an `active_renderer` field. In C++ `export_files()` (line ~290), `opts.active_renderer` is not set (no such line is present). This matches the JS behaviour. No issue.


<!-- ─── src/js/modules/tab_fonts.cpp ──────────────────────────────────────── -->

- [ ] 481. [tab_fonts.cpp] Font glyph grid renders as character grid in ImGui but JS uses a DOM-based grid element
  - **JS Source**: `src/js/modules/tab_fonts.js` lines 64–66, 153–165
  - **Status**: Pending
  - **Details**: In JS, the glyph grid is a DOM element (`.font-character-grid`) and `detect_glyphs_async(font_id, grid_element, on_glyph_click)` inserts individual character `<div>` elements into the DOM. The C++ (lines ~308–346) renders an ImGui child window with `ImGui::Selectable` cells for each detected codepoint. This is a valid C++ equivalent of the JS DOM approach, but the visual appearance and interaction (hover tooltip, click behaviour, wrapping) may differ from the original. The wrapping logic is manually implemented in C++.

- [ ] 482. [tab_fonts.cpp] JS load_font called once per selection change, C++ may re-trigger on every render frame during isBusy
  - **JS Source**: `src/js/modules/tab_fonts.js` lines 156–166
  - **Status**: Pending
  - **Details**: In JS, the `$watch('selectionFonts')` is only triggered when the selection changes. In C++ `render()` (line ~177), the change detection guard compares `first != prev_selection_first` — this correctly fires only once per new selection. The `isBusy == 0` guard in C++ is also present. The C++ approach is functionally equivalent.

- [ ] 483. [tab_fonts.cpp] Export button uses app::theme::BeginDisabledButton/EndDisabledButton instead of standard ImGui::BeginDisabled
  - **JS Source**: `src/js/modules/tab_fonts.js` line 72
  - **Status**: Pending
  - **Details**: JS renders `<input type="button" :class="{ disabled: isBusy }">`. C++ uses `app::theme::BeginDisabledButton()` / `EndDisabledButton()` (lines ~380–384). According to CLAUDE.md, `app::theme` color constants and `applyTheme()` should be progressively removed. `app::theme::BeginDisabledButton` should be replaced with `ImGui::BeginDisabled(busy)` / `ImGui::EndDisabled()`.

- [ ] 484. [tab_fonts.cpp] fontPreviewText placeholder tooltip only shows on empty InputTextMultiline
  - **JS Source**: `src/js/modules/tab_fonts.js` lines 67–68
  - **Status**: Pending
  - **Details**: In JS, the textarea has a `:placeholder` attribute that shows when the field is empty. In C++ (lines ~372–373), `ImGui::SetItemTooltip()` is used to show the placeholder as a tooltip when the field is empty (`if (view.fontPreviewText.empty())`). The tooltip only appears on hover, not as a persistent placeholder text inside the input box. This is a visual difference from the JS (placeholder text vs hover tooltip).

- [ ] 485. [tab_fonts.cpp] Missing context menu in filter bar for Regex tooltip
  - **JS Source**: `src/js/modules/tab_fonts.js` lines 59–61
  - **Status**: Pending
  - **Details**: The JS template includes `<div class="regex-info" v-if="config.regexFilters" :title="regexTooltip">Regex Enabled</div>` in the filter bar. The C++ filter bar (lines ~295–303) renders `ImGui::TextUnformatted("Regex Enabled")` but does NOT call `ImGui::SetTooltip()` with `view.regexTooltip` after it. The tooltip text is missing from the regex-enabled indicator in the font filter bar.

- [ ] 486. [tab_fonts.cpp] export_fonts: JS exports synchronously in a loop, C++ uses async one-per-frame pump
  - **JS Source**: `src/js/modules/tab_fonts.js` lines 102–146
  - **Status**: Pending
  - **Details**: JS processes all selected fonts in a single async loop per click. The C++ uses a `PendingFontExport` queue that processes one file per `render()` frame call (line ~402–453). This is a C++ architectural adaptation to avoid blocking the main thread. Functionally it should produce the same results — files are exported in order, cancellation is checked, helper.finish() is called. The `isCancelled()` check in JS uses `return` to exit the function; C++ uses `reset()` and returns from `pump_font_export()`. These are functionally equivalent. No correctness issue, this deviation is intentional.


<!-- ─── src/js/modules/tab_install.cpp ────────────────────────────────────── -->

- [ ] 487. [tab_install.cpp] Missing listbox::renderStatusBar calls — includefilecount status bar absent from both views
  - **JS Source**: `src/js/modules/tab_install.js` lines 165, 184
  - **Status**: Pending
  - **Details**: Both Listbox components in the JS template use `:includefilecount="true"` — one with `unittype="install file"` (main manifest view) and one with `unittype="string"` (string viewer). In the C++ port, `listbox::render()` does not internally render the status bar; it must be called separately via `listbox::renderStatusBar()`. The `tab_install.cpp` render function never calls `listbox::renderStatusBar()` for either listbox. This means the file count status bar ("N install files found.") is entirely absent from the Install Manifest tab in both views. Compare with other tabs that correctly call `listbox::renderStatusBar()` after each listbox (e.g., `tab_audio.cpp` line 553, `tab_fonts.cpp` line 289). The status bar should be added after `ImGui::EndChild()` for each list container, before the tray section.


<!-- ─── src/js/modules/tab_item_sets.cpp ──────────────────────────────────── -->

- [ ] 488. [tab_item_sets.cpp] equip_set does not clear chrEquippedItemSkins for each equipped slot
  - **JS Source**: `src/js/modules/tab_item_sets.js` lines 99–115
  - **Status**: Pending
  - **Details**: The JS `equip_set` method explicitly deletes the skin override for each equipped slot: `delete this.$core.view.chrEquippedItemSkins[slot_id]` (line 103) for each item equipped. The C++ `equip_set` (tab_item_sets.cpp lines 209–218) only sets `view.chrEquippedItems[std::to_string(slot_id_opt.value())] = item_id` but never clears `chrEquippedItemSkins` for those slots. This means when a set is equipped, any existing skin customization for those slots persists, causing incorrect rendering in the character preview (old skin override takes precedence over the newly equipped set item). The fix is to also erase the slot from `chrEquippedItemSkins` after setting the equipped item, as done in `equip-item.cpp` lines 54–55.


<!-- ─── src/js/modules/tab_items.cpp ──────────────────────────────────────── -->

- [ ] 489. [tab_items.cpp] view_item_models uses inline logic instead of DBItemList.getItemModels
  - **JS Source**: `src/js/modules/tab_items.js` lines 37–45
  - **Status**: Pending
  - **Details**: JS `view_item_models` calls `DBItemList.getItemModels(item)` which returns a ready-made list. The C++ version reimplements the lookup inline (iterating `DBModelFileData::getModelFileDataID`). This is functionally equivalent but any logic differences inside `DBItemList.getItemModels` could cause divergence. The JS version also produces a flat string list (just the `entry [fdid]` format strings), whereas the C++ version adds them in the same way. This is a structural deviation worth noting, but appears functionally equivalent provided `DBModelFileData` mirrors the JS cache.

- [ ] 490. [tab_items.cpp] view_item_textures uses inline logic instead of DBItemList.getItemTextures
  - **JS Source**: `src/js/modules/tab_items.js` lines 47–55
  - **Status**: Pending
  - **Details**: JS `view_item_textures` calls `await DBItemList.getItemTextures(item)`. C++ reimplements it inline. Same concern as view_item_models above.

- [ ] 491. [tab_items.cpp] JS initialize checks config.itemViewerEnabledTypes as array; C++ handles undefined differently
  - **JS Source**: `src/js/modules/tab_items.js` lines 129–137
  - **Status**: Pending
  - **Details**: In JS `initialize`, `enabled_types` may be `undefined` if the config key is absent (`const enabled_types = this.$core.view.config.itemViewerEnabledTypes`). If undefined, the condition `enabled_types.includes(label)` would throw — but the JS initialises this in core/view, so it defaults to an array. The C++ uses `view.config.value("itemViewerEnabledTypes", nlohmann::json::array())` which defaults to an empty array. This is correct, though if the config key is missing the default logic differs slightly: JS checks `enabled_types.includes(label)` (false for empty), C++ checks for membership in the json array (also false). No bug, but worth noting.

- [ ] 492. [tab_items.cpp] JS pendingItemSlotFilter cleared with null; C++ clears with empty string
  - **JS Source**: `src/js/modules/tab_items.js` line 140
  - **Status**: Pending
  - **Details**: JS sets `this.$core.view.pendingItemSlotFilter = null` after using it. C++ sets `view.pendingItemSlotFilter.clear()`. Functionally equivalent since the C++ uses empty string to mean "no filter". The JS checks `if (pending_slot)` which is falsy for null; C++ checks `!pending_slot.empty()`. Consistent.

- [ ] 493. [tab_items.cpp] Quality mask default logic differs: JS checks `enabled_qualities === undefined`, C++ checks null/non-array
  - **JS Source**: `src/js/modules/tab_items.js` lines 142–147
  - **Status**: Pending
  - **Details**: JS: `checked: enabled_qualities === undefined || enabled_qualities.includes(q.id)`. This means if the config key is absent (`undefined`), all qualities are checked. C++ checks `if (enabled_qualities_json.is_null() || !enabled_qualities_json.is_array())` to enable all. The `nlohmann::json::value()` call returns the stored value or a default — but the default used is `nlohmann::json()` (a null json). So if the key is absent, C++ gets a null json and enables all qualities, matching JS behaviour. If the key IS present but empty array, JS would leave all unchecked while C++ would also leave all unchecked. This appears correct.

- [ ] 494. [tab_items.cpp] Context menu is placed after EndTab but before PopStyleVar — style leak
  - **JS Source**: `src/js/modules/tab_items.js` lines 81–87
  - **Status**: Pending
  - **Details**: In `render()`, the `ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ...)` is pushed at line 803 and popped at line 829 (`ImGui::PopStyleVar()`). The context_menu::render call happens AFTER `app::layout::EndTab()`. However there's a matching `PopStyleVar` on line 829 before `EndTab`. This appears correct — just noting the ordering. No bug found here on close review.

- [ ] 495. [tab_items.cpp] Missing DBItemList abstraction layer — items loading is reimplemented directly
  - **JS Source**: `src/js/modules/tab_items.js` lines 1–3 (imports DBItemList)
  - **Status**: Pending
  - **Details**: The JS uses a `DBItemList` module (`../db/caches/DBItemList`) that abstracts item loading and retrieval. The C++ does not have a corresponding `DBItemList` class — all its logic is inlined in `initialize_items()`. This is a structural difference. Functionally it must be verified that all the logic from `DBItemList.initialize()`, `DBItemList.loadShowAllItems()`, `DBItemList.getItems()`, `DBItemList.getItemModels()`, and `DBItemList.getItemTextures()` is covered by the C++ implementation. The initialize_items() function appears to cover the initialize/loadShowAllItems paths, but getItemModels/getItemTextures are inlined in the view helpers.


<!-- ─── src/js/modules/tab_maps.cpp ───────────────────────────────────────── -->

- [ ] 496. [tab_maps.cpp] export_map_wmo checks !selected_wdt->worldModelPlacement differently from JS
  - **JS Source**: `src/js/modules/tab_maps.js` lines 640–645
  - **Status**: Pending
  - **Details**: JS checks `if (!selected_wdt || !selected_wdt.worldModelPlacement)` (falsy check). C++ checks `if (!selected_wdt || (!selected_wdt->hasWorldModelPlacement && selected_wdt->worldModel.empty()))`. This is a more permissive condition — if the WDT has a `worldModel` string but no placement, the C++ will proceed while the JS would throw. This deviation means the C++ will attempt to export even when JS would reject it, potentially hitting different errors downstream.

- [ ] 497. [tab_maps.cpp] export_map_wmo_minimap missing isCancelled check in tile loop
  - **JS Source**: `src/js/modules/tab_maps.js` lines 712–742
  - **Status**: Pending
  - **Details**: JS checks `if (helper.isCancelled()) break;` inside the `for...of tiles_by_coord` loop. The C++ does NOT check `helper.isCancelled()` inside the tile composition loop in `export_map_wmo_minimap`. This means cancellation during WMO minimap export is not honoured in C++ whereas it is in JS.

- [ ] 498. [tab_maps.cpp] initialize() posts maps to main thread but JS sets mapViewerMaps directly and hides loading screen after
  - **JS Source**: `src/js/modules/tab_maps.js` lines 938–978
  - **Status**: Pending
  - **Details**: JS sets `this.$core.view.mapViewerMaps = maps` synchronously after the loop, then calls `this.$core.hideLoadingScreen()`. C++ uses `core::postToMainThread` to set `mapViewerMaps` and sets `tab_initialized = true`, but calls `core::hideLoadingScreen()` BEFORE `postToMainThread` returns. This means the loading screen may hide before `mapViewerMaps` is updated, which could cause a brief display of an empty map list if the render loop runs between the hide and the main-thread post.

- [ ] 499. [tab_maps.cpp] mounted() does not call initialize() — uses lazy init in render()
  - **JS Source**: `src/js/modules/tab_maps.js` lines 1132–1146
  - **Status**: Pending
  - **Details**: JS `mounted()` calls `await this.initialize()`. C++ `mounted()` sets `tab_initialized = false` and the actual initialization is triggered lazily inside `render()`. This is a conscious structural change for threading reasons, and is functionally acceptable, but represents a deviation from the original structure.

- [ ] 500. [tab_maps.cpp] WMO minimap tile export uses nearest-neighbor sampling instead of drawImage (canvas bilinear)
  - **JS Source**: `src/js/modules/tab_maps.js` lines 716–736
  - **Status**: Pending
  - **Details**: In `export_map_wmo_minimap`, JS uses `ctx.drawImage(canvas, ...)` which performs bilinear interpolation. The C++ uses nearest-neighbor integer division `int src_x = (std::min)(static_cast<int>(px * tw / output_tile_size), ...)`. For the WMO minimap tile export specifically (not preview), this is less accurate than canvas bilinear. The preview path (`load_wmo_minimap_tile`) uses proper bilinear blitting, but the export path uses nearest-neighbor. Minor visual quality difference.

- [ ] 501. [tab_maps.cpp] export_selected_map (OBJ) does not cancel loop on isCancelled
  - **JS Source**: `src/js/modules/tab_maps.js` lines 790–820
  - **Status**: Pending
  - **Details**: In the JS OBJ export loop, `if (helper.isCancelled()) break;` is checked at the top of each iteration. The C++ async pump-based approach checks `if (helper.isCancelled())` at the start of `pump_map_export()`, which correctly handles cancellation. This appears fine.

- [ ] 502. [tab_maps.cpp] export_selected_map_as_heightmaps uses ADTExporter instance for extract_height_data_from_tile in JS but C++ extracts map_dir/tile_x/tile_y directly
  - **JS Source**: `src/js/modules/tab_maps.js` lines 1004–1005
  - **Status**: Pending
  - **Details**: JS `extract_height_data_from_tile` receives an `ADTExporter` instance and reads `adt.mapDir`, `adt.tileX`, `adt.tileY`. C++ passes the values directly as parameters. Functionally equivalent.

- [ ] 503. [tab_maps.cpp] Missing `map_dir_lower` assignment when `mapDir` is passed — C++ initializes it but doesn't re-lower on load_map re-entry
  - **JS Source**: `src/js/modules/tab_maps.js` lines 416
  - **Status**: Pending
  - **Details**: C++ `load_map` does `std::transform` to lowercase. The JS also uses `.toLowerCase()`. Both normalize the directory. Appears equivalent.

- [ ] 504. [tab_maps.cpp] fieldToUint32 and related helpers have indentation issues (no indentation)
  - **JS Source**: N/A
  - **Status**: Pending
  - **Details**: Lines 140–189 of tab_maps.cpp — the field helper functions (`fieldToUint32`, `fieldToInt32`, `fieldToFloat`, `fieldToString`, `fieldToFloatVec`) have all their function bodies without proper indentation. The code is syntactically correct but inconsistently formatted compared to the rest of the file. Not a functional issue.


<!-- ─── src/js/modules/tab_models_legacy.cpp ──────────────────────────────── -->

- [ ] 515. [tab_models_legacy.cpp] PNG/CLIPBOARD export path uses different logic than JS
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 197–221
  - **Status**: Pending
  - **Details**: JS PNG export for legacy models: gets canvas from DOM (`document.getElementById('legacy-model-preview').querySelector('canvas')`), converts to buffer, writes to file with incremental naming support (`modelsExportPngIncrements`), shows success toast with "View in Explorer" link. C++ instead calls `model_viewer_utils::export_preview()` which may have different behaviour. The JS CLIPBOARD path copies as base64 PNG to clipboard with a success toast. C++ calls `model_viewer_utils::export_preview(format, ...)` which likely handles both. The JS also checks `config.modelsExportPngIncrements` for incremental filenames — this config option is not referenced in the C++ path. Missing feature.

- [ ] 516. [tab_models_legacy.cpp] Missing modelsExportPngIncrements check for legacy PNG export
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 207–209
  - **Status**: Pending
  - **Details**: JS legacy PNG export checks `if (core.view.config.modelsExportPngIncrements)` and calls `ExportHelper.getIncrementalFilename(out_file)`. C++ does not implement this for legacy models export.

- [ ] 517. [tab_models_legacy.cpp] Legacy texture ribbon has no context menu — correctly matches JS
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 344–348
  - **Status**: Pending
  - **Details**: The JS legacy texture ribbon does NOT include a ContextMenu component (unlike the modern models tab). The C++ correctly omits the context menu for the legacy ribbon. This is correct. (No finding.)

- [ ] 518. [tab_models_legacy.cpp] Animation controls are commented out in both JS and C++ — correctly disabled
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 352–369 (commented block)
  - **Status**: Pending
  - **Details**: Both JS and C++ have the animation dropdown/controls commented out in the preview area. The underlying animation methods (play, step, seek, scrub) are still implemented in C++. This matches the JS. Correct.

- [ ] 519. [tab_models_legacy.cpp] JS mounted() calls initialize logic synchronously; C++ runs in mounted() directly (not background thread)
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 499–535
  - **Status**: Pending
  - **Details**: The JS runs all initialization in `mounted()` with async/await on the loading screen. The C++ runs everything in `mounted()` synchronously on the main thread. This blocks the main thread during model list building and creature data loading. It should run on a background thread like other tabs (e.g. tab_maps uses a background thread). The loading screen display may not render correctly if the main thread is blocked.

- [ ] 520. [tab_models_legacy.cpp] JS WMO Export uses setGroupMask with the raw view array; C++ converts to WMOGroupMaskEntry vector
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 256–259
  - **Status**: Pending
  - **Details**: JS passes `core.view.modelViewerWMOGroups` directly to `exporter.setGroupMask()`. C++ converts it to a `std::vector<WMOGroupMaskEntry>`. This is necessary due to type differences and appears functionally correct, assuming the exporter processes the mask the same way.

- [ ] 521. [tab_models_legacy.cpp] step_animation for MDX does not call renderer's step_animation_frame
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 462–471
  - **Status**: Pending
  - **Details**: JS `step_animation`: calls `renderer.step_animation_frame?.(delta)` and reads back `renderer.get_animation_frame?.() || 0` for both M2 and MDX renderers. C++ `step_animation` calls `active_renderer_m2->step_animation_frame(delta)` for M2 but for MDX just sets `view.legacyModelViewerAnimFrame = 0` without calling any renderer method. The MDX renderer's `step_animation_frame` is not called. This means frame stepping does not work for MDX models in C++.

- [ ] 522. [tab_models_legacy.cpp] seek_animation for MDX does not call renderer's set_animation_frame
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 473–480
  - **Status**: Pending
  - **Details**: JS `seek_animation` calls `renderer.set_animation_frame?.(parseInt(frame))` for both M2 and MDX. C++ `seek_animation` calls `active_renderer_m2->set_animation_frame(frame)` for M2 but for MDX only sets `view.legacyModelViewerAnimFrame = frame` without calling the MDX renderer. Animation frame seeking is broken for MDX models.

- [ ] 523. [tab_models_legacy.cpp] Sidebar missing "Show Bones" checkbox — C++ incorrectly omits it vs modern tab
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 376–399
  - **Status**: Pending
  - **Details**: The JS legacy sidebar does NOT include "Show Bones" checkbox (consistent with modern tab_models.js which also lacks it). C++ legacy sidebar also omits it (correct), but C++ modern tab adds it. Noted for cross-reference with the tab_models finding above.

- [ ] 524. [tab_models_legacy.cpp] Export format only handles OBJ/STL/RAW; GLTF not listed as unsupported
  - **JS Source**: `src/js/modules/tab_models_legacy.js` lines 228, 317
  - **Status**: Pending
  - **Details**: JS handles OBJ/STL/RAW as formats and shows toast for others. C++ `export_files` also handles OBJ/STL/RAW and shows toast for others. However it's not clear what `menuButtonLegacyModels` contains. If GLTF is in the button options, users can select it and get a toast error. This matches JS behaviour.

- [ ] 525. [tab_models_legacy.cpp] export_paths not closed in export_files PNG/CLIPBOARD path
  - **JS Source**: `src/js/modules/tab_models_legacy.js` line 322
  - **Status**: Pending
  - **Details**: JS always calls `export_paths?.close()` at the end of `export_files` regardless of format. In C++ `export_files`, the PNG/CLIPBOARD branch does not open or close `export_paths_writer` (for CLIPBOARD) — actually for PNG it opens one but closes it. For CLIPBOARD no export_paths is opened. For the OBJ/STL/RAW path, `export_paths_writer` is created in the task and closed... it appears there's no `export_paths_writer.close()` call in `pump_legacy_export` at the end. The writer is destroyed when `pending_legacy_export.reset()` is called (destructor). This might not flush properly if FileWriter requires explicit close.

- [ ] 526. [tab_raw.cpp] `cascLocale` watch deviates from JS — C++ triggers `compute_raw_files()` immediately on locale change, JS only sets `is_dirty = true` in the watcher and relies on the next `compute_raw_files()` call to act on it
  - **JS Source**: `src/js/modules/tab_raw.js` lines 204–206
  - **Status**: Pending
  - **Details**: In JS, `mounted()` watches `config.cascLocale` and only sets `is_dirty = true`. The next call to `compute_raw_files()` (e.g. tab re-mount) picks up the dirty flag. In C++ `render()` detects the locale change and immediately calls `compute_raw_files()`. Minor behavioral difference; C++ behavior is arguably more correct but deviates from JS.

- [ ] 527. [tab_raw.cpp] `pump_detect_task` processes one file per frame; JS `detect_raw_files` processes all files in a single async for-loop without interleaving frames
  - **JS Source**: `src/js/modules/tab_raw.js` lines 56–88
  - **Status**: Pending
  - **Details**: JS processes all files sequentially within one async execution context. C++ breaks each file into a separate per-frame pump step, spreading detection across many frames. End result is identical but observable timing differs.

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

- [ ] 531. [tab_text.cpp] C++ `render()` adds a status bar (`BeginStatusBar`/`EndStatusBar`) not present in the JS template
  - **JS Source**: `src/js/modules/tab_text.js` lines 18–44
  - **Status**: Pending
  - **Details**: The JS template has no status bar element. The C++ adds one (lines 207–210). Acceptable if status bars are a framework-level addition to all list tabs, but it deviates from the JS template structure.

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

- [ ] 539. [tab_videos.cpp] `export_mp4` calls `generics::get(*mp4_url)` without a User-Agent header; JS sends `{ 'User-Agent': constants.USER_AGENT }` in the download fetch
  - **JS Source**: `src/js/modules/tab_videos.js` lines 627–629
  - **Status**: Pending
  - **Details**: The C++ MP4 download omits the User-Agent header that JS sends. `generics::get` should be checked/configured to set User-Agent to match JS behavior and avoid server rejections.

- [ ] 540. [tab_videos.cpp] `get_mp4_url` is a blocking `while(true)` poll loop running on the calling thread; when called from `export_mp4()` on the main thread this freezes the UI until the MP4 is ready
  - **JS Source**: `src/js/modules/tab_videos.js` lines 347–375
  - **Status**: Pending
  - **Details**: JS uses tail-recursive async calls with `setTimeout` delays. C++ blocks the calling thread. Export should be moved off the main thread.

- [ ] 541. [tab_videos.cpp] In-app video playback is replaced by opening an external player via `core::openInExplorer(url)` — JS plays video directly in an embedded `<video>` element with subtitle track, play/pause controls, and `onended`/`onerror` callbacks
  - **JS Source**: `src/js/modules/tab_videos.js` lines 219–276
  - **Status**: Pending
  - **Details**: Major intentional deviation documented in code comments (lines 306–311). No in-app video playback exists in C++. The preview area shows "Video opened in external player". Known limitation.

- [ ] 542. [tab_zones.cpp] Zones filter input uses `ImGui::InputText` without a placeholder hint; JS template uses `placeholder="Filter zones..."`
  - **JS Source**: `src/js/modules/tab_zones.js` line 325
  - **Status**: Pending
  - **Details**: C++ line 998 calls `ImGui::InputText("##FilterZones", ...)`. Should be `ImGui::InputTextWithHint("##FilterZones", "Filter zones...", ...)` to match the JS placeholder text.

- [ ] 543. [tab_zones.cpp] Zones listbox missing `:includefilecount="true"` — JS passes it but C++ `listbox_zones::render` call does not
  - **JS Source**: `src/js/modules/tab_zones.js` line 315
  - **Status**: Pending
  - **Details**: Same issue as other list tabs.

- [ ] 544. [tab_zones.cpp] `load_zone_map` runs synchronously on the main thread blocking the UI during tile loading; JS `load_zone_map` is async and yields between tile loads
  - **JS Source**: `src/js/modules/tab_zones.js` lines 275–288
  - **Status**: Pending
  - **Details**: For zones with many tiles, C++ will freeze the UI briefly. Should be moved to a background thread with progress reporting.

- [ ] 545. [build-version.cpp] `find_version_in_buffer` loop boundary differs from JS
  - **JS Source**: `src/js/mpq/build-version.js` lines 55–68
  - **Status**: Pending
  - **Details**: JS loops `while (pos < buf.length - 52)`, stopping before the final 52-byte window. C++ searches via `std::search` up to `buf.end()` and then calls `parse_vs_fixed_file_info` which validates the 52-byte window independently. In practice there is no behavioral difference, but the loop termination condition differs from JS.

- [ ] 546. [build-version.cpp] `DEFAULT_BUILD` constant from JS is not exposed in C++
  - **JS Source**: `src/js/mpq/build-version.js` lines 10, 159–162
  - **Status**: Pending
  - **Details**: JS exports `DEFAULT_BUILD = '1.12.1.5875'`. The C++ implementation does not declare this constant in `build-version.h`; the fallback value is hardcoded inline in `detect_build_version`. Any consumer that needs `DEFAULT_BUILD` by name has no access to it.

- [ ] 547. [bzip2.cpp] `N_ITERS` and `NUM_OVERSHOOT_BYTES` constants are commented out
  - **JS Source**: `src/js/mpq/bzip2.js` lines 51–53
  - **Status**: Pending
  - **Details**: JS defines `const N_ITERS = 4` and `const NUM_OVERSHOOT_BYTES = 20`. The C++ file comments both out. Neither is used in the decompression logic in JS or C++, but they should be present as uncommented constants to match the JS source faithfully.

- [ ] 548. [mpq-install.cpp] `_scan_mpq_files` is not recursive; JS version recurses into subdirectories
  - **JS Source**: `src/js/mpq/mpq-install.js` lines 25–41
  - **Status**: Pending
  - **Details**: JS `_scan_mpq_files` recursively calls itself for subdirectories, finding MPQ files anywhere under the install root. C++ uses `fs::directory_iterator` which is non-recursive and only scans the top-level directory. MPQ files in subdirectories are silently missed. Should use `fs::recursive_directory_iterator` or implement recursive descent mirroring the JS.

- [ ] 549. [mpq-install.cpp] `getFilesByExtension` and `getAllFiles` return results in non-deterministic order
  - **JS Source**: `src/js/mpq/mpq-install.js` lines 87–120
  - **Status**: Pending
  - **Details**: JS iterates a `Map` in deterministic insertion order (archives loaded in sorted path order). C++ iterates an `std::unordered_map` in unspecified order. Callers that depend on a stable ordering will observe different results.

- [ ] 550. [audio-helper.cpp] `set_volume` is a no-op before `init()` and does not persist the value, but the JS does not persist it either — however the C++ `volume` field defaults to `1.0f` at construction, which slightly diverges from JS where there is no stored "pending volume" concept at all.
  - **JS Source**: `src/js/ui/audio-helper.js` lines 136–139
  - **Status**: Pending
  - **Details**: JS `set_volume` is guarded by `if (this.gain)` and writes directly to the gain node; there is no local storage of the volume before `init()`. The C++ early-returns before setting `this->volume` when `!engine`, which matches the JS no-op behaviour, but the constructor initialises `volume = 1.0f` and `play()` passes that pre-stored value to the new sound — meaning a volume set before `init()` is silently ignored, whereas in JS no such "pending value" field exists at all. This is a low-risk deviation but worth noting.

- [ ] 551. [audio-helper.cpp] `get_position` calculation differs from JS in how it computes elapsed time.
  - **JS Source**: `src/js/ui/audio-helper.js` lines 115–130
  - **Status**: Pending
  - **Details**: The JS computes `elapsed = context.currentTime - this.start_time` (wall-clock elapsed since `source.start()` was called), then `position = this.start_offset + elapsed`. The C++ uses `ma_sound_get_cursor_in_seconds()`, which returns the cursor relative to the seek point set in the decoder (i.e. it already accounts for `start_offset`). The C++ then adds `start_offset` again: `position = start_offset + cursor`. If `cursor` includes the decoded offset (i.e. reflects absolute playback from byte 0 of the stream), the result is correct; but if `ma_sound_get_cursor_in_seconds` returns elapsed-since-seek, the position is `start_offset` doubled. Needs verification that miniaudio returns an absolute stream cursor rather than elapsed-since-seek.

- [ ] 552. [audio-helper.cpp] `play()` `from_offset` sentinel differs from JS.
  - **JS Source**: `src/js/ui/audio-helper.js` lines 43–72
  - **Status**: Pending
  - **Details**: JS uses `if (from_offset !== undefined)` to decide whether to update `start_offset`. The C++ uses `if (from_offset >= 0.0)` as the sentinel (passing `-1.0` means "don't update offset"). This is a deliberate deviation using a sentinel value instead of `std::optional`, which is acceptable but undocumented. No functional impact for callers who pass a valid offset or the default, but callers who legitimately want to seek to 0.0 seconds must pass `0.0`, which works correctly.

- [ ] 553. [char-texture-overlay.cpp] `ensureActiveLayerAttached` deferred callback has incorrect logic.
  - **JS Source**: `src/js/ui/char-texture-overlay.js` lines 63–71
  - **Status**: Pending
  - **Details**: The JS `ensure_active_layer_attached` uses `process.nextTick` to re-attach the active canvas to the `#chr-texture-preview` DOM element if it is not already a child. It does NOT modify `active_layer`. The C++ deferred callback instead checks if `active_layer` is absent from `layers` and resets it to `layers.back()` or `0`. This is a different semantic: the C++ is doing a consistency repair, not a DOM re-attach. In the ImGui context there is no DOM to attach to, so the JS intent cannot be replicated exactly, but the current C++ logic is wrong in a different way — it would clear a valid `active_layer` if it somehow got removed from the list, rather than doing anything display-related.

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

- [ ] 564. [texture-exporter.cpp] `exportFiles` catch block uses `fileName` instead of `markFileName` for error marking
  - **JS Source**: `src/js/ui/texture-exporter.js` lines 177
  - **Status**: Pending
  - **Details**: JS `catch (e)` block calls `helper.mark(markFileName, false, e.message, e.stack)` — `markFileName` may have been rewritten with `.webp`/`.png` extension during the try block. C++ uses `fileName` (unchanged original name) because the updated `markFileName` (which is `exportFileName` in C++) is not accessible in the catch scope. This is a scope difference that changes which name is reported on error when format is WEBP or PNG.

- [ ] 565. [texture-exporter.cpp] `exportFiles` C++ signature adds explicit `casc` and `mpq` parameters not present in the JS function
  - **JS Source**: `src/js/ui/texture-exporter.js` lines 68
  - **Status**: Pending
  - **Details**: JS `exportFiles(files, isLocal = false, exportID = -1, isMPQ = false)` accesses CASC and MPQ through `core.view`. C++ adds `casc::CASC* casc` and `mpq::MPQInstall* mpq` as explicit parameters. This is a necessary adaptation but changes the API surface.

- [ ] 566. [texture-exporter.cpp] `exportSingleTexture` only accepts a `casc` parameter but JS version takes only `fileDataID`
  - **JS Source**: `src/js/ui/texture-exporter.js` lines 191–193
  - **Status**: Pending
  - **Details**: JS `exportSingleTexture(fileDataID)` calls `exportFiles([fileDataID], false)`. C++ `exportSingleTexture(uint32_t fileDataID, casc::CASC* casc)` requires an explicit CASC pointer. This is a necessary adaptation due to dependency injection, but the API signature diverges from JS.

- [ ] 567. [texture-ribbon.cpp] `clearSlotTextures` and `getSlotTexture` are C++-only additions with no JS counterparts
  - **JS Source**: `src/js/ui/texture-ribbon.js` (no equivalent)
  - **Status**: Pending
  - **Details**: JS `texture-ribbon.js` exports only `{ reset, setSlotFile, setSlotFileLegacy, setSlotSrc, onResize, addSlot }`. The C++ adds `clearSlotTextures()` and `getSlotTexture(int)` for GL texture management. These are C++-specific additions required for ImGui image rendering and are not deviations from JS logic, but they extend the JS interface.

- [ ] 568. [texture-ribbon.cpp] `reset()` calls `clearSlotTextures()` before clearing the stack, which has no JS equivalent
  - **JS Source**: `src/js/ui/texture-ribbon.js` lines 28–34
  - **Status**: Pending
  - **Details**: JS `reset()` just clears the stack array and resets the page/contextMenu. C++ additionally calls `clearSlotTextures()` to free GL textures. This is a correct C++ RAII extension but diverges structurally.

- [ ] 569. [texture-ribbon.cpp] `s_slotTextures` and `s_slotSrcCache` module-level maps have no JS counterparts
  - **JS Source**: `src/js/ui/texture-ribbon.js` (no equivalent)
  - **Status**: Pending
  - **Details**: These static maps are C++-only infrastructure for GL texture caching. No functional deviation, but they represent additional state not present in the JS module.

- [ ] 570. [uv-drawer.cpp] Line rendering uses a hard pixel-width Bresenham-like algorithm instead of the JS Canvas 2D `lineWidth = 0.5` (anti-aliased)
  - **JS Source**: `src/js/ui/uv-drawer.js` lines 22–45
  - **Status**: Pending
  - **Details**: JS uses `ctx.lineWidth = 0.5` and `ctx.stroke()` which produces anti-aliased sub-pixel lines via the 2D canvas. C++ uses integer pixel rasterisation (`drawLinePath` → `plotPixel`) which draws 1-pixel-wide aliased lines. The visual output will differ: JS lines are thinner and anti-aliased, C++ lines are full-pixel and aliased. This is a necessary adaptation since there is no 2D canvas API.

- [ ] 571. [uv-drawer.cpp] `generateUVLayerPixels` is a C++-only helper function not exported from the JS module
  - **JS Source**: `src/js/ui/uv-drawer.js` (no equivalent)
  - **Status**: Pending
  - **Details**: JS exports only `{ generateUVLayerDataURL }`. C++ adds `generateUVLayerPixels` as a second public function for use by `model-viewer-utils.cpp` to upload UV overlays to GL textures. This is a C++-specific extension with no JS counterpart.

- [ ] 572. [uv-drawer.cpp] JS `indices` parameter type is `Uint16Array`; C++ equivalent uses `std::vector<uint16_t>` — correct mapping but JS treats out-of-bounds as NaN (silently skipped), C++ adds explicit bounds check
  - **JS Source**: `src/js/ui/uv-drawer.js` lines 29–45
  - **Status**: Pending
  - **Details**: JS accessing `uvCoords[outOfBoundsIndex]` returns `undefined`, and `undefined * textureWidth` yields `NaN`. Drawing to `(NaN, NaN)` is silently ignored by the canvas. C++ explicitly checks `if (idx1 + 1 >= uvCoords.size() || ...)` to skip out-of-bounds triangles. The C++ explicit check is the correct safety mechanism and produces the same observable result (no line drawn).

- [ ] 573. [cache-collector.cpp] `parse_url` drops the query string from URLs
  - **JS Source**: `src/js/workers/cache-collector.js` lines 19–44
  - **Status**: Pending
  - **Details**: JS `https_request` passes `parsed.pathname + parsed.search` as the request path (line 25), which includes any query-string parameters. The C++ `parse_url` (cache-collector.cpp lines 157–188) only captures the path up to the first `/`, losing everything after `?`. URLs passed to the worker that include query parameters would produce incorrect HTTP requests.

- [ ] 574. [cache-collector.cpp] No top-level entry point equivalent to `collect().catch(...)`
  - **JS Source**: `src/js/workers/cache-collector.js` line 431
  - **Status**: Pending
  - **Details**: The JS file auto-invokes `collect().catch(err => log(\`fatal: ${err.message}\`))` at module load time. The C++ exposes `collect()` as a library function, so the caller is responsible for invoking it; the top-level invocation and the fatal-error log path are not replicated inside the translation unit itself. This is an architectural difference (worker thread vs library call) but worth documenting for completeness.

- [ ] 575. [cache-collector.cpp] `upload_flavor` payload missing `binary_hashes || {}` fallback
  - **JS Source**: `src/js/workers/cache-collector.js` line 312
  - **Status**: Pending
  - **Details**: JS builds the submit payload with `binary_hashes: result.binary_hashes || {}` (line 312), ensuring an empty object is sent when `binary_hashes` is absent/null. C++ uses `result.binary_hashes` directly (cache-collector.cpp line 571), which relies on `FlavorResult::binary_hashes` always being initialised — currently it is, but the explicit fallback present in JS is not replicated.

- [ ] 576. [equip-item.cpp] Vue reactivity spread assignments omitted — architectural deviation from JS
  - **JS Source**: `src/js/wow/equip-item.js` lines 26–27
  - **Status**: Pending
  - **Details**: JS reassigns `core.view.chrEquippedItems = { ...core.view.chrEquippedItems }` and `core.view.chrEquippedItemSkins = { ...core.view.chrEquippedItemSkins }` after mutation to trigger Vue.js reactive updates (lines 26–27). C++ has no reactive data-binding framework, so these reassignments are omitted. If the C++ view system gains a notification/dirty-flag mechanism for equipped-item changes, this code path will need to be revisited.

- [ ] 577. [equip-item.cpp] "other slot empty" check uses stricter null-only test vs JS falsy check
  - **JS Source**: `src/js/wow/equip-item.js` line 15
  - **Status**: Pending
  - **Details**: JS checks `!core.view.chrEquippedItems[other]` (line 15), which is falsy for `undefined`, `null`, `0`, `false`, and `""`. The C++ equivalent (equip-item.cpp lines 42–45) checks `.is_null()` and `.contains()` only, treating JSON `0`, `false`, or `""` as truthy (slot occupied). In practice this is unlikely to cause a difference since item IDs are always positive integers, but it is a semantic deviation from the JS behaviour.

- [ ] 578. [equip-item.cpp] Function signature differs from JS — takes separate `item_id`/`item_name` instead of an item object
  - **JS Source**: `src/js/wow/equip-item.js` lines 4–5
  - **Status**: Pending
  - **Details**: JS `equip_item(core, item, pending_slot)` receives `item.id` and `item.name` from the item object (lines 5–6, 30). C++ `equip_item(uint32_t item_id, const std::string& item_name, int pending_slot)` accepts them as separate arguments (equip-item.cpp line 21). While functionally equivalent if callers pass the correct values, the signature change must be verified against all call sites to ensure no data is lost or mismatched.

- [ ] 579. [EquipmentSlots.cpp] `filter_name` field added to all `EQUIPMENT_SLOTS` entries — JS only defines it on shoulder entries
  - **JS Source**: `src/js/wow/EquipmentSlots.js` lines 11–27
  - **Status**: Pending
  - **Details**: In the JS `EQUIPMENT_SLOTS` array, only the two shoulder entries carry a `filter_name` property (lines 14–15); all other entries have `filter_name` as `undefined`. The C++ `EquipmentSlotEntry` struct (EquipmentSlots.h line 26) defines `filter_name` for every slot, with non-shoulder slots echoing their display name. The existing C++ usage in `tab_characters.cpp` (lines 3994, 4002) checks `filter_name.empty() ? slot.name : slot.filter_name`, which replicates the JS `filter_name ?? slot.name` pattern correctly. However, any future code that checks whether `filter_name` is *different from* the display name (to identify shoulder-class slots) would behave differently between JS and C++, since non-shoulder slots in JS have no `filter_name` at all while C++ always has it equal to `name`.

- [ ] 580. [EquipmentSlots.cpp] `get_slot_id_for_inventory_type` / `get_slot_id_for_wmv_slot` return `std::nullopt` on miss vs JS `null`
  - **JS Source**: `src/js/wow/EquipmentSlots.js` lines 157–159, 161–163
  - **Status**: Pending
  - **Details**: JS functions return `null` (via `?? null`) when the key is not found. C++ returns `std::optional` with `std::nullopt`. Callers in C++ must use `.has_value()` / `.value()` checks; any caller that was ported expecting a raw int without optional handling would silently fail. This is a standard C++ idiom translation but is flagged because callers must be verified.
