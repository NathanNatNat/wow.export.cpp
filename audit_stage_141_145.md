<!-- ─── src/js/db/caches/DBNpcEquipment.cpp ─────────────────────────────── -->

- [ ] 1. [DBNpcEquipment.cpp] Missing concurrent-initialization guard (`init_promise`)
  - **JS Source**: `src/js/db/caches/DBNpcEquipment.js` lines 28–60
  - **Status**: Pending
  - **Details**: The JS implementation tracks an `init_promise` so that if `initialize()` is called a second time while the first call is still in progress, the second caller awaits the already-running promise instead of starting a duplicate load. The C++ implementation only checks `is_initialized` (line 58–60), which is set *after* the loop completes. If `initialize()` is called concurrently from multiple threads before the flag is set, the table could be loaded multiple times. The C++ needs either a mutex or a `std::future`/`std::atomic` guard equivalent to `init_promise`.

<!-- ─── src/js/db/caches/DBTextureFileData.cpp ────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/hashing/xxhash64.cpp ──────────────────────────────────────── -->

- [ ] 2. [xxhash64.cpp] `check_glyph_support` note: `update()` memory lazy-init differs but is harmless — actual issue is JS `toUTF8Array` vs C++ raw-byte treatment of `std::string_view`
  - **JS Source**: `src/js/hashing/xxhash64.js` lines 20–39
  - **Status**: Pending
  - **Details**: The JS `update()` method converts a JS string argument via `toUTF8Array(input)`, which performs a full Unicode-aware UTF-16 → UTF-8 transcoding (handling surrogate pairs at lines 33–36). The C++ `update(std::string_view)` overload (xxhash64.cpp line 153–156) reinterprets the string view's bytes directly without any transcoding. In practice C++ string literals and `std::string` values are already UTF-8, so for typical WoW file paths the results are identical. However, if a caller passes a string containing non-ASCII characters that were originally represented as UTF-16 in the JS layer (e.g., Korean or Chinese locale display names), the hash computed by C++ would differ from the hash computed by JS. This is a latent interoperability hazard that should be documented.

- [ ] 3. [xxhash64.cpp] `digest()` — JS BigInt arithmetic applies `& MASK_64` after every multiply/add; C++ relies on `uint64_t` natural overflow
  - **JS Source**: `src/js/hashing/xxhash64.js` lines 201–284
  - **Status**: Pending
  - **Details**: The JS implementation uses native BigInt and explicitly truncates to 64 bits after every operation via `& MASK_64` (e.g., lines 213–239). C++ uses `uint64_t` arithmetic which wraps naturally at 2^64. For the same operations on 64-bit unsigned integers this produces identical bit patterns, so the hashes are functionally equivalent. No code change is needed, but this should be explicitly noted in a comment in xxhash64.cpp to explain the equivalence, as it is a non-obvious deviation from the JS source.

- [ ] 4. [xxhash64.cpp] `update()` large-block loop entry condition differs in form from JS
  - **JS Source**: `src/js/hashing/xxhash64.js` line 161
  - **Status**: Pending
  - **Details**: JS uses `if (p <= bEnd - 32)` (line 161) as the guard before the main 32-byte block loop. C++ uses `if (p + 32 <= bEnd)` (line 113). Both expressions are mathematically equivalent for non-negative values and produce the same result. However, the JS form is the authoritative guard and the C++ form deviates in style from the source. More importantly, the JS condition is `<=` (enters loop when exactly 32 bytes remain), and the C++ condition `p + 32 <= bEnd` also enters when exactly 32 bytes remain. These are identical. The deviation is purely cosmetic but worth noting for line-by-line fidelity audits.

<!-- ─── src/js/modules/font_helpers.cpp ──────────────────────────────────── -->

- [ ] 5. [font_helpers.cpp] `check_glyph_support` uses fundamentally different detection logic from JS
  - **JS Source**: `src/js/modules/font_helpers.js` lines 19–54
  - **Status**: Pending
  - **Details**: The JS `check_glyph_support(ctx, font_family, char)` works by rendering the character twice to an off-screen canvas — once with a fallback font (`32px monospace`) and once with the target font — and comparing the total alpha channel sum of the two renders (lines 38–53). A character is considered supported if the rendered output differs from the fallback. This approach detects whether the *target font* actually has a glyph for the codepoint, even when the font is not loaded into ImGui. The C++ implementation (font_helpers.cpp lines 54–63) instead calls `ImFont::FindGlyphNoFallback()` on an already-loaded ImGui font. These are not equivalent: the JS detects support in *any* font family loaded in the browser, while the C++ detects support only in an *already-loaded ImGui font*. For glyphs that exist in the OS font but were not baked into the ImGui atlas (e.g., because the atlas only baked a subset of codepoints), the C++ will incorrectly report the glyph as not supported. This is an unavoidable architectural difference between browser/DOM and ImGui, but the deviation should be documented.

- [ ] 6. [font_helpers.cpp] `detect_glyphs_async` signature differs from JS — `grid_element` parameter replaced with `GlyphDetectionState&`
  - **JS Source**: `src/js/modules/font_helpers.js` lines 56–106
  - **Status**: Pending
  - **Details**: JS signature is `detect_glyphs_async(font_family, grid_element, on_glyph_click, on_complete)`. The second parameter `grid_element` is a DOM element that the function populates with `<span>` cells for each detected glyph. In the C++ port, this DOM manipulation is replaced by storing detected codepoints into `GlyphDetectionState::detected_codepoints` and deferring UI rendering to the caller. The JS builds the UI incrementally inside the async batch loop (creating DOM nodes immediately when each glyph is found). The C++ collects all codepoints first, then lets the caller iterate `detected_codepoints` during the ImGui render loop. This means the C++ port does not populate the glyph grid incrementally during detection — the grid only appears after `state.complete == true`, rather than growing batch-by-batch as in JS. Callers that rely on incremental grid population will see a different UX.

- [ ] 7. [font_helpers.cpp] `inject_font_face` — missing font load verification equivalent to JS `document.fonts.load` + `document.fonts.check`
  - **JS Source**: `src/js/modules/font_helpers.js` lines 113–133
  - **Status**: Pending
  - **Details**: After injecting the CSS `@font-face` rule, the JS awaits `document.fonts.load('16px "' + font_id + '"')` (line 124) and then checks `document.fonts.check('16px "' + font_id + '"')` (line 125). If the check returns false, the style node is removed from the DOM and an error is thrown (lines 127–130). The C++ implementation calls `io.Fonts->AddFontFromMemoryTTF(...)` and checks for a null return (line 171), but there is no equivalent to the post-load verification step. If ImGui accepts the font data but it is internally corrupt or unusable, the C++ will not detect this and will not clean up, whereas the JS would detect it and throw. This is a minor error-recovery gap.

- [ ] 8. [font_helpers.cpp] `inject_font_face` — JS accepts `log` and `on_error` callback parameters that have no C++ equivalent
  - **JS Source**: `src/js/modules/font_helpers.js` lines 113–133
  - **Status**: Pending
  - **Details**: JS signature is `inject_font_face(font_id, blob_data, log, on_error)`. The `on_error` parameter is never called in the JS body (errors are thrown), so its absence in C++ does not cause a behavioral difference. The `log` parameter is also unused in the JS body (logging is not called inside `inject_font_face` in the JS). The C++ signature correctly omits both unused parameters. This is not a bug but is documented for completeness.

- [ ] 9. [font_helpers.cpp] `get_detection_canvas()` helper and `glyph_detection_canvas`/`glyph_detection_ctx` state have no C++ equivalent
  - **JS Source**: `src/js/modules/font_helpers.js` lines 15–28
  - **Status**: Pending
  - **Details**: The JS maintains a singleton off-screen canvas (`glyph_detection_canvas`) and its 2D context (`glyph_detection_ctx`) for pixel-level glyph detection. The `get_detection_canvas()` helper lazily creates this canvas on first use. The C++ omits this entirely because it replaced the canvas-based detection with an ImGui atlas lookup. This is an expected consequence of the `check_glyph_support` deviation documented in finding 5, but the absence of any canvas state is explicitly noted here for completeness.

<!-- ─── src/js/modules/legacy_tab_audio.cpp ──────────────────────────────── -->

- [ ] 10. [legacy_tab_audio.cpp] `export_sounds` passes only message+function to `helper.mark()`, not the full stack trace
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` line 189
  - **Status**: Pending
  - **Details**: JS calls `helper.mark(export_file_name, false, e.message, e.stack)` where `e.stack` is the full JavaScript stack trace string. C++ calls `helper.mark(export_file_name, false, e.what(), build_stack_trace("export_sounds", e))` where `build_stack_trace` (lines 33–35) returns only `"export_sounds: <exception message>"`. The C++ never captures a real C++ stack trace (e.g., via `std::stacktrace` from C++23 or a platform API). The export helper's `mark()` function receives a weaker error context string than the JS version provides to users in the export report.

- [ ] 11. [legacy_tab_audio.cpp] Animated music icon uses `ImDrawList::AddText` raw draw call instead of a native ImGui widget
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 216–216 (template `sound-player-anim` div)
  - **Status**: Pending
  - **Details**: The JS template renders `<div id="sound-player-anim" :style="{ 'animation-play-state': ... }">` — a CSS-animated element. The C++ replacement (lines 440–449) uses `ImGui::GetWindowDrawList()->AddText(iconFont, animSize, pos, ...)` which is a raw `ImDrawList` call. Per CLAUDE.md, `ImDrawList` calls should be reserved exclusively for effects with no native equivalent such as image rotation, multi-colour gradient fills, or custom OpenGL overlays. A pulsating text icon does not fall into any of those categories; `ImGui::Text` with a scaled font (via `ImGui::PushFont`/`ImGui::PopFont` or by setting the font size) would be a closer match using a native widget approach. The current use of `AddText` violates the ImGui rendering guideline.

- [ ] 12. [legacy_tab_audio.cpp] `start_seek_loop` vs JS — C++ does not pass `core` parameter to `update_seek()`
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 32–35
  - **Status**: Pending
  - **Details**: This is a structural note rather than a bug. JS `update_seek(core)` and `start_seek_loop(core)` receive the `core` object as a parameter because the JS module functions are stateless closures. C++ functions access `core::view` via a global, so the parameter is omitted. This is the correct adaptation for C++. No fix needed, but documented for completeness.

- [ ] 13. [legacy_tab_audio.cpp] `load_sound_list` condition differs: JS checks `!core.view.isBusy` (falsy), C++ checks `view.isBusy > 0`
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` line 124
  - **Status**: Pending
  - **Details**: JS guard is `if (core.view.listfileSounds.length === 0 && !core.view.isBusy)`. C++ guard is `if (!view.listfileSounds.empty() || view.isBusy > 0) return;` (lines 169–170), which is the logical negation of the JS entry condition. If `isBusy` is an integer counter (which it is based on `BusyLock` usage), `isBusy > 0` correctly mirrors JS's `!isBusy` (falsy when 0). However, if `isBusy` could theoretically be negative (e.g., due to a lock mismatch), JS would still proceed but C++ would not. This is a minor robustness note and not a real bug under normal operation.

- [ ] 14. [legacy_tab_audio.cpp] Sound player UI renders seek/title/duration on one `ImGui::Text` line rather than in three separate labelled spans
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 219–221 (template)
  - **Status**: Pending
  - **Details**: JS template renders three separate `<span>` elements: one for seek time (left-aligned), one for the track title (centre, with class `title`), and one for duration (right-aligned), all within a flex-row `#sound-player-info` div. The C++ renders all three concatenated in a single `ImGui::Text("%s  %s  %s", ...)` call (line 460). The layout is compressed onto one line with no alignment differentiation between the three fields. The JS gives the title a `class="title"` style (typically larger/bolder text and centred within the flex row). The C++ does not replicate the three-column flex alignment or the title styling. This is a UI layout deviation.
