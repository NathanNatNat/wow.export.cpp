## File 171. [tab_raw.cpp]

- tab_raw.cpp: `cascLocale` watch deviates from JS — C++ triggers `compute_raw_files()` immediately on locale change, JS only sets `is_dirty = true` in the watcher and relies on the next `compute_raw_files()` call to act on it
  - **JS Source**: `src/js/modules/tab_raw.js` lines 204–206
  - **Status**: Pending
  - **Details**: In JS, `mounted()` watches `config.cascLocale` and only sets `is_dirty = true`. The next call to `compute_raw_files()` (e.g. tab re-mount) picks up the dirty flag. In C++ `render()` detects the locale change and immediately calls `compute_raw_files()`. Minor behavioral difference; C++ behavior is arguably more correct but deviates from JS.

- tab_raw.cpp: `pump_detect_task` processes one file per frame; JS `detect_raw_files` processes all files in a single async for-loop without interleaving frames
  - **JS Source**: `src/js/modules/tab_raw.js` lines 56–88
  - **Status**: Pending
  - **Details**: JS processes all files sequentially within one async execution context. C++ breaks each file into a separate per-frame pump step, spreading detection across many frames. End result is identical but observable timing differs.

- tab_raw.cpp: C++ uses `check.matches` (array) with `startsWith(patterns)` where JS uses `check.match` (single value) with `data.startsWith(check.match)`
  - **JS Source**: `src/js/modules/tab_raw.js` line 63
  - **Status**: Pending
  - **Details**: JS `constants.FILE_IDENTIFIERS` uses a single `match` property per identifier. C++ uses a `matches` array with `match_count`. If `match_count > 1` the C++ checks multiple patterns while JS checks only one. Could over-match relative to JS depending on constant definitions.

- tab_raw.cpp: Raw tab listbox is missing `:includefilecount="true"` — JS Listbox component is passed `:includefilecount="true"` but C++ `listbox::render` call does not pass an equivalent parameter
  - **JS Source**: `src/js/modules/tab_raw.js` line 147
  - **Status**: Pending
  - **Details**: The JS template passes `:includefilecount="true"` on the Listbox for tab-raw. The C++ `listbox::render` call (lines 328–354) does not set an equivalent `includefilecount` argument. If `listbox::render` supports this parameter it should be set to `true`.

## File 172. [tab_text.cpp]

- tab_text.cpp: `pump_text_export` calls `helper.mark(export_file_name, false, e.what())` omitting the stack trace; JS calls `helper.mark(export_file_name, false, e.message, e.stack)` with both message and stack
  - **JS Source**: `src/js/modules/tab_text.js` line 116
  - **Status**: Pending
  - **Details**: The C++ version omits the stack trace argument. The JS passes `e.stack` as a fourth argument to `helper.mark`. Should be passed to match JS fidelity (C++ `ExportHelper::mark` accepts an optional stack trace parameter).

- tab_text.cpp: C++ `render()` adds a status bar (`BeginStatusBar`/`EndStatusBar`) not present in the JS template
  - **JS Source**: `src/js/modules/tab_text.js` lines 18–44
  - **Status**: Pending
  - **Details**: The JS template has no status bar element. The C++ adds one (lines 207–210). Acceptable if status bars are a framework-level addition to all list tabs, but it deviates from the JS template structure.

- tab_text.cpp: Text tab listbox missing `:includefilecount="true"` — JS passes it but C++ `listbox::render` call does not
  - **JS Source**: `src/js/modules/tab_text.js` line 21
  - **Status**: Pending
  - **Details**: Same issue as tab_raw — JS passes `:includefilecount="true"` which should be reflected in C++ if the parameter is supported.

## File 173. [tab_textures.cpp]

- tab_textures.cpp: `remove_override_textures` action is missing — JS template shows a toast with a "Remove" span calling `remove_override_textures()` when `overrideTextureList.length > 0`; no equivalent action exists in C++
  - **JS Source**: `src/js/modules/tab_textures.js` lines 284–288, 366–368
  - **Status**: Pending
  - **Details**: The JS template shows a progress-styled toast (`<div id="toast" v-if="!$core.view.toast && $core.view.overrideTextureList.length > 0" class="progress">`) with the override texture name and a "Remove" clickable span (`<span @click.self="remove_override_textures">Remove</span>`). The C++ comment (lines 603–606) says this is rendered in `renderAppShell`, but no C++ equivalent of `remove_override_textures()` is exposed or wired up. This functionality appears to be missing.

- tab_textures.cpp: Textures listbox missing `:includefilecount="true"` — JS passes it but C++ `listbox::render` call does not
  - **JS Source**: `src/js/modules/tab_textures.js` line 291
  - **Status**: Pending
  - **Details**: Same issue as tab_raw and tab_text.

- tab_textures.cpp: `export_textures()` passes a JSON object `{fileName: file}` for drop-handler path but a raw int JSON for single-file path — JS passes raw integer in both cases (`textureExporter.exportFiles([selected_file_data_id])`)
  - **JS Source**: `src/js/modules/tab_textures.js` lines 372–378
  - **Status**: Pending
  - **Details**: JS calls `textureExporter.exportFiles([selected_file_data_id])` where `selected_file_data_id` is a raw integer. C++ single-file path uses `files.push_back(selected_file_data_id)` (raw int as JSON), while the drop-handler path wraps in `{fileName: file}`. The two C++ paths are inconsistent and may not match what `texture_exporter::exportFiles` expects.

## File 174. [tab_videos.cpp]

- tab_videos.cpp: Preview controls use a plain `Button("Export Selected")` instead of a `MenuButton` — JS uses `<MenuButton>` with options from `menuButtonVideos` allowing format selection (MP4/AVI/MP3/SUBTITLES) in the button dropdown
  - **JS Source**: `src/js/modules/tab_videos.js` lines 504–506
  - **Status**: Pending
  - **Details**: C++ export controls (lines 1167–1189) render a plain "Export Selected" button with no format dropdown. The user has no UI way to change the export format within the tab. This is a layout deviation — `menu_button::render` should be used here as in other tabs.

- tab_videos.cpp: Videos listbox missing `:includefilecount="true"` — JS passes it but C++ `listbox::render` call does not
  - **JS Source**: `src/js/modules/tab_videos.js` line 479
  - **Status**: Pending
  - **Details**: Same issue as other list tabs.

- tab_videos.cpp: `trigger_kino_processing` runs synchronously on the main thread blocking the UI; JS runs it as an async function, yielding between each fetch
  - **JS Source**: `src/js/modules/tab_videos.js` lines 380–464
  - **Status**: Pending
  - **Details**: The C++ version will freeze the UI for the duration of kino processing. JS awaits each fetch asynchronously. Should be moved to a background thread or pumped per-frame.

- tab_videos.cpp: `export_mp4` calls `generics::get(*mp4_url)` without a User-Agent header; JS sends `{ 'User-Agent': constants.USER_AGENT }` in the download fetch
  - **JS Source**: `src/js/modules/tab_videos.js` lines 627–629
  - **Status**: Pending
  - **Details**: The C++ MP4 download omits the User-Agent header that JS sends. `generics::get` should be checked/configured to set User-Agent to match JS behavior and avoid server rejections.

- tab_videos.cpp: `get_mp4_url` is a blocking `while(true)` poll loop running on the calling thread; when called from `export_mp4()` on the main thread this freezes the UI until the MP4 is ready
  - **JS Source**: `src/js/modules/tab_videos.js` lines 347–375
  - **Status**: Pending
  - **Details**: JS uses tail-recursive async calls with `setTimeout` delays. C++ blocks the calling thread. Export should be moved off the main thread.

- tab_videos.cpp: In-app video playback is replaced by opening an external player via `core::openInExplorer(url)` — JS plays video directly in an embedded `<video>` element with subtitle track, play/pause controls, and `onended`/`onerror` callbacks
  - **JS Source**: `src/js/modules/tab_videos.js` lines 219–276
  - **Status**: Pending
  - **Details**: Major intentional deviation documented in code comments (lines 306–311). No in-app video playback exists in C++. The preview area shows "Video opened in external player". Known limitation.

## File 175. [tab_zones.cpp]

- tab_zones.cpp: Zones filter input uses `ImGui::InputText` without a placeholder hint; JS template uses `placeholder="Filter zones..."`
  - **JS Source**: `src/js/modules/tab_zones.js` line 325
  - **Status**: Pending
  - **Details**: C++ line 998 calls `ImGui::InputText("##FilterZones", ...)`. Should be `ImGui::InputTextWithHint("##FilterZones", "Filter zones...", ...)` to match the JS placeholder text.

- tab_zones.cpp: Zones listbox missing `:includefilecount="true"` — JS passes it but C++ `listbox_zones::render` call does not
  - **JS Source**: `src/js/modules/tab_zones.js` line 315
  - **Status**: Pending
  - **Details**: Same issue as other list tabs.

- tab_zones.cpp: `load_zone_map` runs synchronously on the main thread blocking the UI during tile loading; JS `load_zone_map` is async and yields between tile loads
  - **JS Source**: `src/js/modules/tab_zones.js` lines 275–288
  - **Status**: Pending
  - **Details**: For zones with many tiles, C++ will freeze the UI briefly. Should be moved to a background thread with progress reporting.
