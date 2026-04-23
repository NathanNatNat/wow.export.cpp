# TODO Tracker

> **Progress: 6/185 verified (3%)** — ✅ = Verified, ⬜ = Pending


## Data Caches & Database

## UI Components

## Map Viewer

- [ ] 141. [map-viewer.cpp] Tile image drawing path is still unimplemented
- **JS Source**: `src/js/components/map-viewer.js` lines 380–402, 1111–1113
- **Status**: Pending
- **Details**: JS draws loaded tiles to canvas via `putImageData(...)` on main/double-buffer contexts. C++ caches tile pixels but does not upload/draw them, so only overlays render and map tiles are not visually equivalent.

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

## Tab: Blender & Install

- [ ] 182. [tab_install.cpp] Install listbox copy/paste options are hardcoded instead of using JS config-driven behavior
- **JS Source**: `src/js/modules/tab_install.js` lines 165, 184
- **Status**: Pending
- **Details**: JS listbox wiring uses `$core.view.config.copyMode`, `pasteSelection`, and `removePathSpacesCopy`; C++ passes `CopyMode::Default` with `pasteselection=false` and `copytrimwhitespace=false`, changing list interaction behavior.

- [ ] 183. [tab_install.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS renders `Regex Enabled` with `:title="$core.view.regexTooltip"`; C++ renders plain text without the tooltip contract, changing UI affordance.

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

## Tab: Audio

- [ ] 249. [tab_audio.cpp] export_sounds missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_audio.js` line 175
- **Status**: Pending
- **Details**: JS passes e.message and e.stack to helper.mark(). C++ only passes e.what().

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

- [ ] 299. [tab_fonts.cpp] export_fonts missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_fonts.js` line 141
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

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

- [ ] 457. [Shaders.cpp] C++ adds automatic _unregister_fn callback on ShaderProgram not present in JS
- **JS Source**: `src/js/3D/Shaders.js` lines 56–72
- **Status**: Pending
- **Details**: C++ `create_program()` (Shaders.cpp lines 79–83) installs a static `_unregister_fn` callback on `gl::ShaderProgram` that automatically calls `shaders::unregister()` when a ShaderProgram is destroyed. JS has no equivalent auto-cleanup mechanism — callers must explicitly call `unregister(program)` (Shaders.js line 78–86). This means in C++, a program is automatically removed from `active_programs` on destruction, while in JS a disposed program remains in the set until manually unregistered. This changes `reload_all()` behavior: JS could attempt to recompile stale programs that were not explicitly unregistered, while C++ never encounters this scenario.

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

- [x] 597. [casc/casc-source-local.cpp] CASC journal index loading 1.27× slower than JS in release (1074ms vs 848ms for 2M entries)
  - **JS Source**: `src/js/casc/casc-source-local.js` (journal index loading)
  - **Status**: Pending
  - **Details**: Debug: 4029ms. Release: 1074ms vs 848ms JS (1.27×). The gap is marginal and may reflect real algorithmic overhead: likely insufficient pre-allocation of the index map (`reserve()`) or sequential small reads per `.idx` entry rather than bulk-reading each file into memory first. Investigate but low priority given the small gap.

- [x] 598. [casc/encoding.cpp] Encoding table parsing 1.85× slower than JS in release (3811ms vs 2057ms for 2.8M entries)
  - **JS Source**: `src/js/casc/encoding.js`
  - **Status**: Pending
  - **Details**: Debug: 12292ms. Release: 3811ms vs 2057ms JS (1.85×). This is the single remaining significant CASC bottleneck in release. Likely causes: (1) per-entry `std::unordered_map` inserts without `reserve()` — 2.8M insertions with multiple rehash cycles; (2) MD5 keys stored as `std::string` or `std::vector<uint8_t>` rather than `std::array<uint8_t, 16>` with a flat hash — avoids heap allocation per key; (3) file not read into a single buffer before parsing. Fix: `reserve(3'000'000)`, bulk-read the file, use fixed-size key arrays.

- [x] 599. [casc/root.cpp] Root file parsing 1.54× slower than JS in release (1431ms vs 928ms for 1.9M entries)
  - **JS Source**: `src/js/casc/root.js`
  - **Status**: Pending
  - **Details**: Debug: 7222ms. Release: 1431ms vs 928ms JS (1.54×). Similar to entry 598: likely missing `reserve()` on the root map and/or per-entry copies in the hot parse loop. Fix: `reserve()` before filling, load the full root blob into memory and parse in-place, avoid temporaries.

- [x] 601. [casc/tact-keys.cpp / app.cpp] TACTKeys download runs before source_select — should fire async in parallel with CDN resolution
  - **JS Source**: `src/js/casc/tact-keys.js`; `src/js/casc/casc-source-remote.js`
  - **Status**: Pending
  - **Details**: In JS, TACTKeys are fetched async during CDN pre-resolution. In C++ release, TACTKeys download completes at 19:35:37 before `set active module: source_select` (also 19:35:37) — in release this is invisible since the download is fast, but the structure is wrong. In slower network conditions the blocking download would delay the UI appearing. Fix: fire TACTKeys as a background `std::async` task concurrent with CDN pre-resolution, join only when a CASC source is opened.

- [x] 602. [app.cpp / screen_source_select.cpp] "failed to load whats-new.html" error on startup — file missing from C++ build
  - **JS Source**: `src/js/components/whats-new.js`; `src/whats-new.html`
  - **Status**: Pending
  - **Details**: C++ log shows: `failed to load whats-new.html: could not open D:/Repositories/wow.export.cpp\src\whats-new.html`. The `whats-new.html` file was removed from the C++ repo (audited and ported per commit `012b8cde`), but something still attempts to load it at runtime. The `tab_home` stub intentionally omits the whats-new panel (see Intentional Stubs in CLAUDE.md), so this load attempt should be removed or guarded. Locate the call site that tries to open `whats-new.html` and remove it, or suppress the error since the tab_home stub is intentional.

- [x] 603. [CMakeLists.txt] MSVC debug build is 5–12× slower at runtime than release — add `_ITERATOR_DEBUG_LEVEL=0` and `/JMC-` for debug
  - **JS Source**: N/A (build system only)
  - **Status**: Pending
  - **Details**: MSVC debug builds have two major sources of runtime overhead beyond `/Od` (no optimisation) that do not affect debugability at all: (1) `_ITERATOR_DEBUG_LEVEL=2` (default) adds bounds checking and iterator invalidation detection to every STL operation — responsible for most of the CASC loading slowdown (encoding table 1.85×, root file 1.54× even in release hints at some debug-only calls, but in a pure debug build this is the primary cost on all `std::vector`/`std::unordered_map` access); (2) `/JMC` (Just My Code, on by default) adds a function-entry probe to every function for the debugger's "Step Into User Code" button, adding overhead to every function call. Fix: in `CMakeLists.txt`, add `$<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>` via `add_compile_definitions` (applied globally so all compiled targets in the build are consistent — mixing `_ITERATOR_DEBUG_LEVEL` values across translation units in the same binary causes ODR violations), and `/JMC-` via `target_compile_options` for MSVC debug. Full debugability is preserved: breakpoints, step-through, variable inspection, call stack, and watch expressions are all unaffected. The only things lost are runtime STL iterator validation and the "Step Into User Code" filtering — both of which release builds also omit. Expected: debug CASC load time drops from 5–12× to roughly 2–3× of release.

- [ ] 604. [tab_characters.cpp] Loading screen invisible when switching to Characters tab — mounted() runs synchronously on main thread
  - **JS Source**: `src/js/modules/tab_characters.js` lines 2545–2666
  - **Status**: In Progress
  - **Details**: JS `mounted()` is `async` with 8 `await progressLoadingScreen(...)` yield points before each DB cache load (`realmlist.load`, `DBCharacterCustomization`, `DBItems`, `DBItemCharTextures`, `DBItemGeosets`, `DBItemModels`, `DBGuildTabard`, shader setup). In C++, `showLoadingScreen`/`hideLoadingScreen` both use `postToMainThread` internally; when called synchronously on the main thread they queue together and process in the same `drainMainThreadQueue()` call — no render frame fires between them, so the loading screen never appears. Fix: run the heavy DB work (`realmlist::load`, all `ensureInitialized` calls) on a `std::thread`; wrap all `state.*` writes and final setup (`chrModelViewerContext`, `viewer_context`, `view_state`, `anim_methods`, `update_chr_race_list`) in `postToMainThread`; call `hideLoadingScreen` from the background thread so the main thread renders the overlay across multiple frames.

- [ ] 605. [tab_audio.cpp] Loading screen invisible when processing unknown sound files — runs synchronously on main thread
  - **JS Source**: `src/js/modules/tab_audio.js` lines 282–295
  - **Status**: Pending
  - **Details**: JS `mounted()` shows a loading screen (`showLoadingScreen(1)`) while processing unknown sound files, with an `await progressLoadingScreen(...)` yield. In C++, `showLoadingScreen`/`hideLoadingScreen` are called synchronously (lines 357–390), so both queue into the same `drainMainThreadQueue()` drain — the overlay never renders. Fix: run the unknown-file processing on a `std::thread`, wrap the `state.*` mutations in `postToMainThread`, call `hideLoadingScreen` from the background thread.

- [ ] 606. [tab_items.cpp] Loading screen invisible when switching to Items tab — initialize() runs synchronously on main thread
  - **JS Source**: `src/js/modules/tab_items.js` lines 132–135, 278–281
  - **Status**: Pending
  - **Details**: JS `mounted()` calls `showLoadingScreen(2)` and awaits two `progressLoadingScreen` steps (Loading model file data, Loading item data) before `hideLoadingScreen`. C++ `initialize()` calls both synchronously (lines 473–477 in mounted, 240–243 in initialize), so show and hide queue together — loading screen never visible. Fix: run `initialize()` on a `std::thread`; wrap `state.*` writes (`listfileItems` population, viewer context setup) in `postToMainThread`; call `hideLoadingScreen` from background thread. Tab already has `is_initialized` guard.

- [ ] 607. [tab_item_sets.cpp] Loading screen invisible when switching to Item Sets tab — initialize() runs synchronously on main thread
  - **JS Source**: `src/js/modules/tab_item_sets.js` lines 26–34, 90–92
  - **Status**: Pending
  - **Details**: JS `mounted()` calls `showLoadingScreen(3)` and awaits three `progressLoadingScreen` steps (item data, item appearance data, item sets) before `hideLoadingScreen`. C++ calls all synchronously (lines 227–231 in mounted, 96–108 in initialize), so show and hide queue together — overlay never visible. Fix: run `initialize()` on a `std::thread`; wrap `state.*` writes in `postToMainThread`; call `hideLoadingScreen` from background thread. Tab already has `is_initialized` guard.

- [ ] 608. [tab_data.cpp] Loading screen invisible when switching to Data tab — mounted() runs synchronously on main thread
  - **JS Source**: `src/js/modules/tab_data.js` lines 180–183
  - **Status**: Pending
  - **Details**: JS `mounted()` calls `showLoadingScreen(1)` and awaits one `progressLoadingScreen` step (Loading data table manifest) before `hideLoadingScreen`. C++ calls all synchronously (lines 219–222), so show and hide queue together — overlay never visible. Fix: run the manifest load on a `std::thread`; wrap `state.*` writes in `postToMainThread`; call `hideLoadingScreen` from background thread.

- [ ] 609. [tab_textures.cpp] Loading screen invisible during atlas parse and texture list load — runs synchronously on main thread
  - **JS Source**: `src/js/modules/tab_textures.js` lines 107–113, 397–409
  - **Status**: Pending
  - **Details**: JS shows a loading screen during `initialize()` (atlas parsing) and again in `mounted()` (loading texture file data + unknown textures). Both call sequences are synchronous in C++ (lines 348–354 in initialize, 495–507 in mounted), so show/hide queue together in the same `drainMainThreadQueue()` — overlay never renders. Fix: run the heavy atlas and listfile work on a `std::thread`; wrap `state.*` writes in `postToMainThread`; call `hideLoadingScreen` from background thread.

- [ ] 610. [tab_videos.cpp] Loading screen invisible when switching to Videos tab — mounted() runs synchronously on main thread
  - **JS Source**: `src/js/modules/tab_videos.js` lines 536–539
  - **Status**: Pending
  - **Details**: JS `mounted()` calls `showLoadingScreen(1)` and awaits one `progressLoadingScreen` step (Loading video metadata) before `hideLoadingScreen`. C++ calls all synchronously (lines 909–912), so show and hide queue together — overlay never visible. Fix: run the video metadata load on a `std::thread`; wrap `state.*` writes in `postToMainThread`; call `hideLoadingScreen` from background thread.
