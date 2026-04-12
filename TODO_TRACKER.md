# TODO Tracker

## Missing C++ Files (No Counterpart Exists)

### 1. [external-links.js] No C++ port exists
- **JS Source**: `src/js/external-links.js` lines 1–end
- **Status**: Pending
- **Details**: Entire file needs to be ported. Contains URL constants for Discord, GitHub, Patreon, website, and issue tracker links. These are referenced by core.js and the app footer.

### 2. [updater.js] No C++ port exists
- **JS Source**: `src/js/updater.js` lines 1–end
- **Status**: Pending
- **Details**: Entire file needs to be ported. Contains auto-update check logic using the GitHub releases API to detect and prompt for new versions.

### 3. [wmv.js] No C++ port exists
- **JS Source**: `src/js/wmv.js` lines 1–end
- **Status**: Pending
- **Details**: Entire file needs to be ported. Contains WoW Model Viewer character import/export integration logic used by tab_characters.

### 4. [wowhead.js] No C++ port exists
- **JS Source**: `src/js/wowhead.js` lines 1–end
- **Status**: Pending
- **Details**: Entire file needs to be ported. Contains Wowhead character import integration used by tab_characters for importing character appearances from Wowhead URLs.

### 5. [home-showcase.js] No C++ port exists
- **JS Source**: `src/js/components/home-showcase.js` lines 1–end
- **Status**: Pending
- **Details**: Entire Vue component needs to be ported to ImGui. Displays a rotating showcase of images on the home tab using data from showcase.json.

### 6. [markdown-content.js] No C++ port exists
- **JS Source**: `src/js/components/markdown-content.js` lines 1–end
- **Status**: Pending
- **Details**: Entire Vue component needs to be ported to ImGui. Renders markdown content used in changelogs and help screens. Handles headers, links, images, bold, italic, code blocks, and lists.

## CASC Files (Audited)

Note: The following CASC files were audited and found to have no substantive issues: content-flags, install-manifest, locale-flags, salsa20. All other CASC files have detailed audit entries below.

### 7. [blte-reader.cpp] Missing _checkBounds() override for lazy block decompression
- **JS Source**: `src/js/casc/blte-reader.js` lines 311–321
- **Status**: Resolved
- **Details**: `_checkBounds()` override is now implemented in blte-reader.cpp (lines 135–143) and declared virtual in BufferWrapper. Lazy block decompression during reads works correctly.

### 8. [casc-source-remote.cpp] Data race in parseArchiveIndex — concurrent writes to unprotected maps
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 130–170
- **Status**: Pending
- **Details**: JS queue() uses Promise-based concurrency which is single-threaded (event loop). C++ queue() uses real std::async threads. 50 concurrent threads write to unprotected std::unordered_map `archives` and call BuildCache methods without mutex. Needs mutex protection or thread-safe container.

### 9. [casc-source.cpp] `getFileByName` drops parameters and does not return decoded data
- **JS Source**: `src/js/casc/casc-source.js` lines 169–195
- **Status**: Pending
- **Details**: In JS, `getFileByName` calls `this.getFile(fileDataID, partialDecrypt, suppressLog, supportFallback, forceFallback)` which polymorphically dispatches to the subclass `getFile` (CASCLocal/CASCRemote) that accepts all 5+ parameters and returns decoded file data (a BLTEReader). In C++, `getFileByName` accepts these parameters but silently drops them, calling only `getFile(fileDataID)` which returns an encoding key string. The C++ subclasses provide `getFileAsBLTE` instead of overriding `getFile`, and there is no `getFileByNameAsBLTE` equivalent. When callers are converted (tab_maps, tab_fonts, tab_audio, tab_videos, tab_text, tab_raw all reference `getFileByName`), this function will not deliver decoded data as expected.

### 10. [casc-source-local.cpp] Missing `core.view.casc = this` assignment in `load()`
- **JS Source**: `src/js/casc/casc-source-local.js` line 179
- **Status**: Pending
- **Details**: The JS `load()` method sets `core.view.casc = this` between `loadRoot()` and `prepareListfile()`. This assignment is missing in the C++ `CASCLocal::load()`. Without it, the rest of the application has no reference to the active CASC source after loading completes locally. The C++ equivalent (e.g., `core::view->casc = this;`) must be added in the same position. Note: the same issue exists in `casc-source-remote.cpp` (JS line 290).

## DB Cache Files (Audited — Issues Below)

### 11. [DBCharacterCustomization.cpp] Missing re-entrancy guard for concurrent initialization
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 37–44
- **Status**: Pending
- **Details**: JS uses `init_promise` so concurrent callers await the same promise. C++ `ensureInitialized()` has no guard against concurrent calls — could initialize multiple times. Needs `std::once_flag` or mutex+condvar.

### 12. [DBCreatureDisplayExtra.cpp] ensureInitialized doesn't properly handle concurrent initialization
- **JS Source**: `src/js/db/caches/DBCreatureDisplayExtra.js` lines 16–23
- **Status**: Pending
- **Details**: JS stores and returns `init_promise` so concurrent callers await the same promise. C++ returns immediately if `is_initializing` is set, meaning a second caller gets uninitialized data. Needs `std::once_flag` or mutex+condvar.

### 13. [DBCreaturesLegacy.cpp] Missing error stack trace logging on catch
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 95–96
- **Status**: Pending
- **Details**: JS catch logs both `e.message` and `e.stack`. C++ only logs `e.what()`, omitting the stack trace log line.

### 14. [DBDecorCategories.cpp] Incorrect skip logic for decor_id == 0 in mapping loop
- **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 37–47
- **Status**: Pending
- **Details**: JS uses `row.HouseDecorID ?? row.DecorID` (nullish coalescing — falls through only if `undefined`, not `0`). C++ treats `0` as missing and falls through to `DecorID`, producing different behavior when `HouseDecorID` is explicitly `0`. The skip condition `decor_id == 0` also differs from JS's `decor_id === undefined`.

### 15. [DBModelFileData.cpp] Missing getFileDataIDs() reset behavior
- **JS Source**: `src/js/db/caches/DBModelFileData.js` lines 42–50
- **Status**: Pending
- **Details**: JS resets `fileDataIDs` to `undefined` after first retrieval to free memory. C++ returns a persistent const reference and never clears the set.

### 16. [DBTextureFileData.cpp] Missing getFileDataIDs() reset behavior
- **JS Source**: `src/js/db/caches/DBTextureFileData.js` lines 52–62
- **Status**: Pending
- **Details**: Same as DBModelFileData — JS resets after first call; C++ never clears.

### 17. [DBItemDisplays.cpp] Missing initializeModelFileData() call
- **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 17–19
- **Status**: Pending
- **Details**: JS calls `await initializeModelFileData()` at the start. C++ calls `DBTextureFileData::ensureInitialized()` but does **not** call `DBModelFileData::initializeModelFileData()`. Model data may not be loaded when needed.

### 18. [DBItems.cpp] Missing classID/subclassID default initialization
- **JS Source**: `src/js/db/caches/DBItems.js` lines 30–55
- **Status**: Pending
- **Details**: Items not in the `Item` table have `classID`/`subclassID` as `undefined` in JS. If C++ struct defaults to 0 instead of using `std::optional`, `isItemBow()` could incorrectly match items with classID=2/subclassID=2.

## UI Helper Files (Audited — Issues Below)

### 19. [texture-exporter.cpp] Clipboard copies base64 text instead of PNG image data
- **JS Source**: `src/js/ui/texture-exporter.js` CLIPBOARD export branch
- **Status**: In Progress
- **Details**: C++ uses `ImGui::SetClipboardText(base64)` (text). JS uses `clipboard.set(png, 'png', true)` (actual image data). Users can't paste-as-image.

### 20. [texture-ribbon.cpp] Missing context menu event handlers and page navigation
- **JS Source**: `src/js/ui/texture-ribbon.js` event handlers
- **Status**: Pending
- **Details**: JS registers handlers for `click-texture-ribbon-next/prev` and context menu interactions. C++ has no `initEvents()` or equivalent; page navigation and context menu logic appear missing.

### 21. [uv-drawer.cpp] Returns raw pixels instead of data URL
- **JS Source**: `src/js/ui/uv-drawer.js` `generateUVLayerDataURL()`
- **Status**: In Progress
- **Details**: JS returns a canvas data URL string. C++ returns raw RGBA pixels. All call sites must handle this difference correctly.

## Component Files (Audit Incomplete — Needs Detailed Review)

### 22. [components/*.cpp] Component files need line-by-line audit
- **JS Source**: `src/js/components/*.js` (all files)
- **Status**: Pending
- **Details**: The following component files were not fully audited: checkboxlist, combobox, context-menu, data-table, file-field, itemlistbox, listbox, listbox-maps, listbox-zones, listboxb, map-viewer, menu-button, model-viewer-gl, resize-layer, slider. Each needs a detailed comparison against the JS original to verify all event handlers, watchers, computed properties, and template rendering are correctly converted.

## Module/Tab Files (Audit Incomplete — Needs Detailed Review)

### 23. [modules/*.cpp] Module and tab files need line-by-line audit
- **JS Source**: `src/js/modules/*.js` (all files)
- **Status**: Pending
- **Details**: The following module/tab files were not fully audited: font_helpers, screen_settings, screen_source_select, tab_audio, tab_characters, tab_creatures, tab_data, tab_decor, tab_fonts, tab_home, tab_install, tab_item_sets, tab_items, tab_maps, tab_models, tab_models_legacy, tab_raw, tab_text, tab_textures, tab_videos, tab_zones, legacy_tab_audio, legacy_tab_data, legacy_tab_files, legacy_tab_fonts, legacy_tab_home, legacy_tab_textures. Each needs a detailed comparison.

## 3D Files (Audit Incomplete — Needs Detailed Review)

### 24. [3D/**/*.cpp] 3D system files need line-by-line audit
- **JS Source**: `src/js/3D/**/*.js` (all files)
- **Status**: Pending
- **Details**: The following 3D files were not fully audited: AnimMapper, BoneMapper, GeosetMapper, ShaderMapper, Shaders, Skin, Texture, WMOShaderMapper, camera/CameraControlsGL, camera/CharacterCameraControlsGL, exporters/ADTExporter, exporters/CharacterExporter, exporters/M2Exporter, exporters/M2LegacyExporter, exporters/M3Exporter, exporters/WMOExporter, exporters/WMOLegacyExporter, gl/GLContext, gl/GLTexture, gl/ShaderProgram, gl/UniformBuffer, gl/VertexArray, loaders/ADTLoader, loaders/ANIMLoader, loaders/BONELoader, loaders/LoaderGenerics, loaders/M2Generics, loaders/M2LegacyLoader, loaders/M2Loader, loaders/M3Loader, loaders/MDXLoader, loaders/SKELLoader, loaders/WDTLoader, loaders/WMOLegacyLoader, loaders/WMOLoader, renderers/CharMaterialRenderer, renderers/GridRenderer, renderers/M2LegacyRendererGL, renderers/M2RendererGL, renderers/M3RendererGL, renderers/MDXRendererGL, renderers/ShadowPlaneRenderer, renderers/WMOLegacyRendererGL, renderers/WMORendererGL, writers/CSVWriter, writers/GLBWriter, writers/GLTFWriter, writers/JSONWriter, writers/MTLWriter, writers/OBJWriter, writers/SQLWriter, writers/STLWriter.

## MPQ, Workers Files (Audit Incomplete — Needs Detailed Review)

### 25. [mpq/*.cpp] MPQ files need line-by-line audit
- **JS Source**: `src/js/mpq/*.js` (all files)
- **Status**: Pending
- **Details**: The following MPQ files were not fully audited: bitstream, build-version, bzip2, huffman, mpq-install, mpq, pkware.

### 26. [workers/cache-collector.cpp] Worker file needs line-by-line audit
- **JS Source**: `src/js/workers/cache-collector.js` lines 1–end
- **Status**: Pending
- **Details**: Needs comparison against JS original for correctness of hash implementations, file scanning logic, and cache eviction behavior.

## Detailed Audit Findings (Core Files)

### 27. [constants.cpp] Missing BLENDER namespace constants
- **JS Source**: `src/js/constants.js` lines 20–61
- **Status**: Fixed
- **Details**: Added `getBlenderBaseDir()` (cross-platform: Windows `%APPDATA%/Blender Foundation/Blender`, Linux `~/.config/blender`), `BLENDER::DIR()`, `BLENDER::ADDON_DIR`, `BLENDER::LOCAL_DIR()`, `BLENDER::ADDON_ENTRY`, and `BLENDER::MIN_VER` to both `constants.h` and `constants.cpp`.

### 28. [constants.cpp] CONTEXT_MENU_ORDER missing 3 entries vs JS source
- **JS Source**: `src/js/constants.js` lines 201–214
- **Status**: Fixed
- **Details**: Restored `tab_blender`, `tab_changelog`, and `tab_help` entries to `CONTEXT_MENU_ORDER` in `constants.h`. Array size changed from 9 to 12 to match JS source.

### 29. [constants.cpp] SHADER_PATH uses different directory structure than JS
- **JS Source**: `src/js/constants.js` line 43
- **Status**: Fixed (deviation documented)
- **Details**: JS sets `SHADER_PATH` to `path.join(INSTALL_PATH, 'src', 'shaders')`. C++ sets it to `DATA_DIR / "shaders"` (i.e., `<install>/data/shaders`). This is intentional for the C++ port's resource layout. Deviation comment added in `constants.h`.

### 30. [constants.cpp] CONFIG.DEFAULT_PATH uses different directory than JS
- **JS Source**: `src/js/constants.js` line 95
- **Status**: Fixed (deviation documented)
- **Details**: JS sets `CONFIG.DEFAULT_PATH` to `path.join(INSTALL_PATH, 'src', 'default_config.jsonc')`. C++ sets it to `DATA_DIR / "default_config.jsonc"`. Intentional for C++ resource layout. Deviation comment added in `constants.h`.

### 31. [constants.cpp] RUNTIME_LOG path differs from JS
- **JS Source**: `src/js/constants.js` line 38
- **Status**: Fixed (deviation documented)
- **Details**: JS stores runtime log at `path.join(DATA_PATH, 'runtime.log')`. C++ stores it in `LOG_DIR / "runtime.log"` (i.e., `<install>/Logs/runtime.log`). C++ also adds a `LOG_DIR` concept not present in JS. Deviation comments added in `constants.h`.

### 32. [constants.cpp] DATA_PATH renamed to DATA_DIR, different base path
- **JS Source**: `src/js/constants.js` line 16
- **Status**: Fixed (deviation documented)
- **Details**: JS uses `DATA_PATH = nw.App.dataPath` (OS-specific user data directory). C++ renames to `DATA_DIR` and uses `<install>/data/` for a portable, self-contained layout. Deviation comment added in `constants.h`.

### 33. [constants.cpp] Cache directory renamed from `casc/` to `cache/`
- **JS Source**: `src/js/constants.js` line 73
- **Status**: Fixed (deviation documented)
- **Details**: JS uses `DATA_PATH/casc/` as the cache directory. C++ renames it to `cache/` and includes migration code from `casc/` to `cache/` on first run. Deviation comment added in `constants.h`.

### 34. [buffer.cpp] Missing fromCanvas() static method
- **JS Source**: `src/js/buffer.js` lines 89–107
- **Status**: Pending
- **Details**: The JS `BufferWrapper.fromCanvas(canvas, mimeType, quality)` method creates a buffer from an HTML canvas element, with special WebP lossless support via `webp-wasm`. This is browser/NW.js-specific. In the C++ port using Dear ImGui/OpenGL, the equivalent would be reading pixels from an OpenGL framebuffer. This needs a C++ equivalent for screenshot/export functionality, or documentation of how it's handled differently.

### 35. [buffer.cpp] Missing decodeAudio() method
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Status**: Pending
- **Details**: The JS `decodeAudio(context)` method decodes buffer data using the Web Audio API's `AudioContext.decodeAudioData()`. The C++ port uses miniaudio for audio, so this method needs a miniaudio-based equivalent or should be documented as handled elsewhere.

### 36. [config.cpp] save() calls doSave() synchronously instead of deferred
- **JS Source**: `src/js/config.js` lines 83–91
- **Status**: Pending
- **Details**: The JS `save()` uses `setImmediate(doSave)` to defer the actual save operation to the next event loop tick. This prevents blocking the current call stack and allows multiple rapid config changes to be batched. The C++ `save()` calls `doSave()` directly and synchronously, which means each save blocks until disk I/O completes. This could cause UI stuttering if config changes happen frequently. Consider using `core::postToMainThread()` or a background thread to match the deferred semantics.

### 37. [config.cpp] Missing Vue $watch equivalent for automatic config persistence
- **JS Source**: `src/js/config.js` line 60
- **Status**: Pending
- **Details**: The JS `load()` sets up `core.view.$watch('config', () => save(), { deep: true })` which automatically triggers a save whenever any config property changes. The C++ port has a comment noting this is replaced by explicit `save()` calls from the UI layer. This means every UI code path that modifies config must remember to call `config::save()`, which is error-prone. Consider implementing a change-detection mechanism or documenting all call sites that need to invoke save.

### 38. [core.cpp] AppState missing `constants` field
- **JS Source**: `src/js/core.js` line 45
- **Status**: Pending
- **Details**: The JS view state includes `constants: constants` which stores a reference to the constants module. This is used in Vue templates to access constants directly from the view context (e.g., `view.constants.EXPANSIONS`). Since ImGui doesn't use templates, this may not be needed, but it should be documented as an intentional omission.

### 39. [core.cpp] setToast() parameter type differs from JS
- **JS Source**: `src/js/core.js` line 470
- **Status**: Pending
- **Details**: The JS `setToast(toastType, message, options, ttl, closable)` takes a generic `options` object as the third parameter (which can be `null` or contain action buttons). The C++ signature in `src/js/core.h` line 589 uses `const std::vector<ToastAction>& actions` instead. Per fidelity rules, this deviation must either be reverted to accept a more generic type (e.g., `nlohmann::json` or `std::optional<nlohmann::json>`) to match the JS interface, or a deviation comment must be added to the C++ code explaining why the typed vector was chosen and how it differs from the original JS behavior. Verify all call sites are compatible with this interface change.

### 40. [generics.cpp] getJSON() error message differs from JS
- **JS Source**: `src/js/generics.js` lines 101–105
- **Status**: In Progress
- **Details**: The JS `getJSON()` throws `"Unable to request JSON from end-point. HTTP ${res.status} ${res.statusText}"` when the response is not ok. The C++ `getJSON()` now checks for parse failure and throws a generic message but cannot include the HTTP status code/text because `get()` returns raw bytes (the status info is lost). The HTTP status range check in `doHttpGet()` has been fixed from `> 302` to `>= 300` to match JS `res.ok` semantics. To fully match the JS error message, `get()` would need to return status information alongside the body.

### 41. [generics.cpp] requestData() missing download progress logging
- **JS Source**: `src/js/generics.js` lines 130–164
- **Status**: Pending
- **Details**: The JS `requestData()` is a full HTTP implementation with chunked response handling that tracks download progress and logs at 25% thresholds (`"Download progress: %d/%d bytes (%d%%)"`) using the `content-length` header. It also handles 301/302 redirects with explicit logging (`"Got redirect to " + res.headers.location`) and has a 60-second timeout. The C++ `requestData()` (lines 338–343) is a thin wrapper around `doHttpGet()` with just two log lines — all progress tracking, redirect logging, and the content-length-based progress reporting are missing.

### 42. [generics.cpp] queue() off-by-one: launches `limit` concurrent tasks instead of `limit + 1`
- **JS Source**: `src/js/generics.js` lines 57–73
- **Status**: Pending
- **Details**: The JS `queue()` initializes `free = limit; complete = -1;` and the `check()` callback immediately increments both (`complete++; free++`), so the first invocation sets `free = limit + 1` and launches up to `limit + 1` concurrent tasks. The C++ version (lines 221–245) uses `while (futures.size() < limit)` which launches exactly `limit` tasks. Additionally, the C++ waits for `futures.front().get()` (always the oldest task), while the JS resolves whichever Promise completes first via `.then(check)`. The C++ behavior is functionally similar but slightly less concurrent.

### 43. [generics.cpp] get() returns raw bytes instead of Response-like object
- **JS Source**: `src/js/generics.js` lines 18–49
- **Status**: Pending
- **Details**: The JS `get()` returns a `fetch` Response object which callers use for `.json()`, `.ok`, `.status`, etc. The C++ `get()` returns `std::vector<uint8_t>`. While downstream callers (`getJSON`, `downloadFile`) have been adapted, any future code porting that calls `get()` expecting Response-like methods will need adaptation. This is a known architectural difference. Also, the JS `get()` uses `AbortSignal.timeout(30000)` for a 30s request-level timeout; the C++ uses separate connection (30s) and read (60s) timeouts via cpp-httplib, which is a different timeout model.

### 44. [icon-render.cpp] processQueue() is stubbed — BLP loading not implemented
- **JS Source**: `src/js/icon-render.js` lines 46–57
- **Status**: Blocked
- **Details**: The JS `processQueue()` loads a BLP file from CASC (`core.view.casc.getFile(entry.fileDataID)`), decodes it with `BLPFile`, and sets the CSS background-image to the decoded data URL. The C++ `processQueue()` (lines 209–229) has the CASC+BLP loading commented out with a note that "CASC source and BLP decoder (casc/blp.cpp) are unconverted". The actual icon texture creation from BLP data is not implemented — icons will always show the default placeholder. Blocked on CASC and BLP tier conversion.

### 45. [icon-render.cpp] processQueue() processes all entries synchronously instead of one-at-a-time async
- **JS Source**: `src/js/icon-render.js` lines 46–57
- **Status**: Pending
- **Details**: The JS `processQueue()` pops one entry, processes it asynchronously (CASC getFile returns a Promise), and recursively calls itself via `.finally()`. This means each icon is loaded one-at-a-time with yielding between loads. The C++ version (lines 209–229) processes all entries in a `while (!_queue.empty())` synchronous loop. When CASC+BLP loading is implemented, this should be changed to process one entry at a time (or batch with yielding) to avoid blocking the main thread.

### 46. [log.cpp] drainPool() missing re-scheduling for remaining pool entries
- **JS Source**: `src/js/log.js` lines 34–49
- **Status**: Pending
- **Details**: The JS `drainPool()` has a post-loop check: `if (!isClogged && pool.length > 0) process.nextTick(drainPool);` which schedules another drain on the next event loop tick if the pool still has entries after processing `MAX_DRAIN_PER_TICK` items. The C++ `drainPool()` (lines 67–86) lacks this continuation — if the pool has more than 50 entries and the stream recovers, only 50 entries are drained per `write()` call. Remaining entries must wait for the next `write()` invocation. This could cause pool buildup if writes are infrequent after a clog recovery.

### 47. [log.cpp] write() calls drainPool() before checking stream, JS checks stream first
- **JS Source**: `src/js/log.js` lines 79–93
- **Status**: Pending
- **Details**: The JS `write()` first attempts `stream.write(line)` and only pools if clogged. The drain event handler is separate (`stream.on('drain', drainPool)`). The C++ `write()` (lines 100–132) calls `drainPool()` at the top if `isClogged` is true, then attempts to write. This ordering difference means the C++ tries to flush old pool entries before writing the new line, while JS writes the new line first and pools/drains independently. The functional impact is that during clog recovery, C++ drains old entries first (possibly un-clogging) then writes the new line directly, while JS would pool the new line and drain asynchronously. The C++ approach is arguably better for ordering but differs from JS behavior.

### 48. [mmap.cpp] release_virtual_files() outer catch drops error message
- **JS Source**: `src/js/mmap.js` lines 35–47
- **Status**: Fixed
- **Details**: Fixed: Added `std::exception` catch handler with `.what()` message to match JS `e.message` logging.

### 49. [modules.cpp] Missing 5 modules from JS MODULES object
- **JS Source**: `src/js/modules.js` lines 27–59
- **Status**: Pending
- **Details**: The JS `MODULES` object includes `module_test_a`, `module_test_b`, `tab_help`, `tab_blender`, and `tab_changelog`, but none of these have been ported to C++. No corresponding header/cpp files exist in `src/js/modules/`. The C++ `modules.cpp` previously marked these as "Removed: module deleted" which is incorrect — they exist in the JS source. Comments have been corrected to indicate they are pending conversion. Each module needs its `.cpp` and `.h` files created and its `add_module()` registration added to `modules::initialize()`.

## Detailed Audit Findings (2026-04-12 Comprehensive Audit)

### 50. [external-links.cpp] Entire file is still raw unconverted JavaScript
- **JS Source**: `src/js/external-links.js` lines 1–end
- **Status**: Pending
- **Details**: The file uses `require('util')`, `const`, `module.exports`, `nw.Shell.openExternal()`. No C++ conversion has been performed. No `.h` header file exists. All URL constants and the `openLink()` function need to be ported to C++.

### 51. [core.cpp] openInExplorer() uses incorrect UTF-8 to UTF-16 conversion on Windows
- **JS Source**: `src/js/core.js` line 465
- **Status**: Pending
- **Details**: The C++ `openInExplorer` on Windows uses `std::wstring(path.begin(), path.end())` which performs incorrect char-by-char widening instead of proper UTF-8→UTF-16 conversion. Unicode paths containing non-ASCII characters will be mangled. Needs `MultiByteToWideChar` or equivalent.

### 52. [gpu-info.cpp] Queries wrong OpenGL uniform constant — values 4× too large
- **JS Source**: `src/js/gpu-info.js` lines 41–42
- **Status**: Pending
- **Details**: C++ queries `GL_MAX_VERTEX_UNIFORM_COMPONENTS` (counts individual floats) instead of JS's `MAX_VERTEX_UNIFORM_VECTORS` (counts vec4s). The reported values will be approximately 4× larger than the JS equivalent. Should use `GL_MAX_VERTEX_UNIFORM_VECTORS` or divide by 4.

### 53. [gpu-info.cpp] exec_cmd() has no timeout — can hang indefinitely
- **JS Source**: `src/js/gpu-info.js` line 67
- **Status**: Pending
- **Details**: JS uses `child_process.execSync` with a 5000ms timeout. C++ uses `popen()` with no timeout mechanism. If the external command hangs, the application will hang indefinitely.

### 54. [gpu-info.cpp] get_gl_info() never returns null on context failure
- **JS Source**: `src/js/gpu-info.js` lines 14–58
- **Status**: Pending
- **Details**: JS returns `null` if WebGL context creation fails. C++ always returns a struct (never null). Callers that check for null/undefined will behave differently.

### 55. [blob.cpp] stringEncode() does not handle UTF-16 surrogate pairs
- **JS Source**: `src/js/blob.js` lines 41–82
- **Status**: Pending
- **Details**: JS `stringEncode` performs full UTF-16→UTF-8 conversion with proper surrogate pair handling. C++ simply reinterprets bytes directly, which will produce incorrect output for any text containing characters outside the Basic Multilingual Plane.

### 56. [blob.cpp] stringDecode() missing UTF-8 error recovery
- **JS Source**: `src/js/blob.js` lines 84–137
- **Status**: Pending
- **Details**: JS `stringDecode` performs full UTF-8→UTF-16 decoding with `0xFFFD` replacement for malformed sequences. C++ skips this error recovery, which could produce different output for malformed UTF-8 data.

### 57. [buffer.cpp] _writeCheck() throws instead of creating silent error like JS
- **JS Source**: `src/js/buffer.js` lines 639–640
- **Status**: Pending
- **Details**: JS creates `new Error(...)` but does NOT throw it — the error is silently created and the write continues. C++ **throws** `std::runtime_error`, which will crash the program on out-of-bounds writes. This is a behavioral difference that could affect callers.

### 58. [buffer.cpp] getDataURL() generates base64 string instead of object URL
- **JS Source**: `src/js/buffer.js` lines 606–611
- **Status**: Pending
- **Details**: JS uses `URL.createObjectURL()` which creates a short blob URL. C++ generates a full base64 data URL string. For very large buffers this produces huge strings that may cause performance issues.

### 59. [buffer.cpp] calculateHash() only supports md5/sha1
- **JS Source**: `src/js/buffer.js` lines 625–627
- **Status**: Pending
- **Details**: JS supports any Node.js hash algorithm (sha256, sha512, etc.) via `crypto.createHash(hash)`. C++ only implements md5 and sha1. If any caller requests sha256 or other algorithms, it will fail.

### 60. [log.cpp] write() accepts single string instead of variadic parameters
- **JS Source**: `src/js/log.js` line 78
- **Status**: Pending
- **Details**: JS `write(...parameters)` accepts variadic args and formats them with `util.format()` (printf-style `%s`, `%d` interpolation). C++ accepts only a single `std::string`. Callers using format strings like `log.write("Loaded %d items", count)` will output literal `%d` instead of the count.

### 61. [log.cpp] timeEnd() missing variadic format parameters
- **JS Source**: `src/js/log.js` lines 64–66
- **Status**: Pending
- **Details**: JS `timeEnd(label, ...params)` passes extra params to `write()` for format interpolation. C++ `timeEnd()` only accepts the label. Calls like `timeEnd("Loaded %s", "textures")` would output literal `%s`.

### 62. [log.cpp] Console output suppression uses NDEBUG instead of BUILD_RELEASE
- **JS Source**: `src/js/log.js` line 93
- **Status**: Pending
- **Details**: JS checks `!BUILD_RELEASE` to decide if console output is enabled. C++ checks `NDEBUG`. These may not map to the same build configurations, potentially suppressing console output in debug builds or showing it in release builds.

### 63. [file-writer.cpp] Encoding parameter is ignored
- **JS Source**: `src/js/file-writer.js` line 14
- **Status**: Pending
- **Details**: JS `FileWriter` constructor accepts and uses an encoding parameter (defaulting to 'utf8'). C++ ignores the encoding entirely — non-UTF-8 encodings would produce different file output.

### 64. [MultiMap.cpp] Missing forEach, keys, values, entries accessors
- **JS Source**: `src/js/MultiMap.js` lines 1–34
- **Status**: Pending
- **Details**: JS `MultiMap` extends `Map` and inherits `forEach`, `keys`, `values`, `entries` accessors. The C++ `MultiMap` class does not expose these — callers needing to iterate all entries or keys will have no API to do so.

### 65. [constants.h] VERSION hardcoded instead of read from manifest
- **JS Source**: `src/js/constants.js` line 46
- **Status**: Pending
- **Details**: JS reads VERSION dynamically from `nw.App.manifest.version`. C++ hardcodes `"0.1.0"`. Version won't update automatically with builds.

### 66. [generics.cpp] get() hardcodes [200] in log instead of actual HTTP status
- **JS Source**: `src/js/generics.js` line 44
- **Status**: Pending
- **Details**: C++ always logs `[200]` for successful HTTP responses instead of the actual HTTP status code. Misleading for 301/302 redirects or other success codes.

### 67. [generics.cpp] batchWork() processes synchronously without yielding
- **JS Source**: `src/js/generics.js` lines 420–469
- **Status**: Pending
- **Details**: JS `batchWork()` uses `MessageChannel` port messaging to yield between batches, allowing the event loop to process other work. C++ processes everything in a tight synchronous loop, blocking the main thread and preventing UI updates during large batch operations.

### 68. [blp.cpp] Missing toCanvas() method
- **JS Source**: `src/js/casc/blp.js` lines ~91–107
- **Status**: Pending
- **Details**: JS `toCanvas()` creates an HTML Canvas element and draws decoded BLP pixel data to it. This is a browser-specific API. The C++ equivalent (creating an OpenGL texture or returning raw pixels for ImGui) needs to be implemented or documented as a deviation.

### 69. [blp.cpp] Missing drawToCanvas() method
- **JS Source**: `src/js/casc/blp.js` lines ~144–160
- **Status**: Pending
- **Details**: JS `drawToCanvas()` draws BLP data onto an existing canvas at specified coordinates. The C++ port needs an equivalent for compositing BLP textures, or this should be documented as a deviation.

### 70. [blp.cpp] getDataURL() uses different encoding path than JS
- **JS Source**: `src/js/casc/blp.js` line ~83–88
- **Status**: Pending
- **Details**: JS uses `canvas.toDataURL()` which produces a PNG data URL via the browser's Canvas API. C++ uses a different PNG encoding path. While functionally similar, there could be subtle differences in the output.

### 71. [blp.cpp] _getCompressed boundary check differs from JS
- **JS Source**: `src/js/casc/blp.js` line 201
- **Status**: Pending
- **Details**: JS checks `if (this.rawData.length === pos)` — skips block only when `pos` **exactly equals** data length. C++ checks `if (static_cast<size_t>(pos) >= rawData_.size())` — skips when `pos >= length`. C++ is safer (prevents out-of-bounds reads) but behaves differently for the `pos > length` case: JS would attempt an out-of-bounds access, C++ would `continue`.

### 72. [blte-reader.cpp] _decompressBlock missing 3rd argument to readBuffer()
- **JS Source**: `src/js/casc/blte-reader.js` line 196
- **Status**: Resolved (Non-issue)
- **Details**: JS `readBuffer(length, wrap, inflate)` has 3 params; C++ `readBuffer(length, inflate)` drops the `wrap` param (always returns BufferWrapper). JS call `readBuffer(len, true, true)` correctly maps to C++ `readBuffer(len, true)` with `inflate=true`. No code change needed.

### 73. [blte-reader.cpp] Missing decodeAudio() method
- **JS Source**: `src/js/casc/blte-reader.js` lines 236–239
- **Status**: Pending
- **Details**: JS `decodeAudio(context)` calls `processAllBlocks()` then delegates to `super.decodeAudio(context)`. This method is completely missing from the C++ port.

### 74. [blte-reader.cpp] getDataURL() missing dataURL cache
- **JS Source**: `src/js/casc/blte-reader.js` lines 244–251
- **Status**: Resolved (Non-issue)
- **Details**: JS checks `if (!this.dataURL)` before processing as an optimization. C++ `BufferWrapper::getDataURL()` already caches via `std::optional<std::string> dataURL`, and `processAllBlocks()` is a no-op when done. Behavior is functionally identical.

### 75. [icon-render.cpp] Extra getIconTexture() function not in JS source
- **JS Source**: N/A
- **Status**: Pending
- **Details**: C++ has a `getIconTexture()` function (lines ~271–276) that does not exist in the original JS source. This appears to be a C++-specific addition and should be documented as a deviation.

## Audit Coverage Notes (2026-04-12)

The following file groups were partially or not fully audited at the line-by-line level and need follow-up audits:

### 76. [casc-source.cpp, casc-source-local.cpp, casc-source-remote.cpp] CASC source files need deeper audit
- **JS Source**: `src/js/casc/casc-source.js`, `casc-source-local.js`, `casc-source-remote.js`
- **Status**: Completed — see entries 89–101
- **Details**: Full line-by-line audit completed. Found 13 issues including broken polymorphic dispatch, missing `core.view.casc` assignments, and HTTP status check differences.

### 77. [build-cache.cpp, cdn-config.cpp, cdn-resolver.cpp] Remaining CASC utility files need audit
- **JS Source**: Corresponding `.js` files in `src/js/casc/`
- **Status**: Completed — see entries 102–111
- **Details**: Full line-by-line audit completed. Found critical issues in build-cache (missing wait-for-integrity), and several issues in cdn-resolver (thread cleanup, architectural deviations).

### 78. [listfile.cpp, export-helper.cpp] CASC data files need audit
- **JS Source**: `src/js/casc/listfile.js`, `src/js/casc/export-helper.js`
- **Status**: Completed — see entries 112–123
- **Details**: Full line-by-line audit completed. Found 12 issues including preload deduplication, type inconsistencies, and missing filter semantics.

### 79. [db/WDCReader.cpp] WDCReader needs detailed audit — highest complexity risk
- **JS Source**: `src/js/db/WDCReader.js` (31KB)
- **Status**: Pending
- **Details**: This is the most complex DB reader file, handling WDC1/WDC2/WDC3 formats with bitpacked field decompression, pallet data, common data, copy tables, and relationship mapping. High risk for conversion errors due to complexity.

### 80. [db/DBCReader.cpp] DBCReader needs detailed audit
- **JS Source**: `src/js/db/DBCReader.js` (11KB)
- **Status**: Pending
- **Details**: Complex parsing with locale count logic, `loadSchema()` with DBD manifest lookup and fallback URLs, `_read_record()` with locstring handling. Needs line-by-line comparison.

### 81. [db/DBDParser.cpp] DBDParser needs detailed audit
- **JS Source**: `src/js/db/DBDParser.js` (8KB)
- **Status**: Pending
- **Details**: Contains 7 regex patterns, `parseBuildID()`, `isBuildInRange()`, `DBDField` class with specific defaults, and `isValidFor()` method. Needs verification that all regex patterns and default values match JS.

### 82. [db/caches/*.cpp] All DB cache files need line-by-line audit
- **JS Source**: `src/js/db/caches/*.js` (17 files)
- **Status**: Pending
- **Details**: Files DBComponentModelFileData, DBComponentTextureFileData, DBCreatureList, DBCreatures, DBCreaturesLegacy, DBDecor, DBGuildTabard, DBItemCharTextures, DBItemGeosets, DBItemModels, DBNpcEquipment need detailed comparison. Each follows a pattern of loading DB2 tables and building lookup maps — need to verify field names, query logic, and initialization patterns.

### 83. [components/*.cpp] All component files need line-by-line audit
- **JS Source**: `src/js/components/*.js` (17 files)
- **Status**: Pending
- **Details**: Component files checkboxlist, combobox, context-menu, data-table, file-field, home-showcase, itemlistbox, listbox, listbox-maps, listbox-zones, listboxb, map-viewer, markdown-content, menu-button, model-viewer-gl, resize-layer, slider need detailed comparison. These are Vue components converted to ImGui — need to verify all event handlers, computed properties, watchers, and template rendering are correctly ported.

### 84. [modules/*.cpp] All module/tab files need line-by-line audit
- **JS Source**: `src/js/modules/*.js` (27 files)
- **Status**: Pending
- **Details**: All module and tab files need detailed comparison: font_helpers, screen_settings, screen_source_select, tab_audio, tab_blender, tab_changelog, tab_characters, tab_creatures, tab_data, tab_decor, tab_fonts, tab_help, tab_home, tab_install, tab_item_sets, tab_items, tab_maps, tab_models, tab_models_legacy, tab_raw, tab_text, tab_textures, tab_videos, tab_zones, legacy_tab_audio, legacy_tab_data, legacy_tab_files, legacy_tab_fonts, legacy_tab_home, legacy_tab_textures, module_test_a, module_test_b.

### 85. [3D/**/*.cpp] All 3D system files need line-by-line audit
- **JS Source**: `src/js/3D/**/*.js` (51 files)
- **Status**: Pending
- **Details**: All 3D files need detailed comparison. This includes: AnimMapper, BoneMapper, GeosetMapper, ShaderMapper, Shaders, Skin, Texture, WMOShaderMapper, camera/* (2), exporters/* (7), gl/* (5), loaders/* (13), renderers/* (9), writers/* (8). These contain complex 3D math, OpenGL rendering, model loading, and file export logic — high risk for conversion errors.

### 86. [mpq/*.cpp] All MPQ files need line-by-line audit
- **JS Source**: `src/js/mpq/*.js` (7 files)
- **Status**: Pending
- **Details**: MPQ files bitstream, build-version, bzip2, huffman, mpq-install, mpq, pkware need detailed comparison. These implement MPQ archive reading with multiple compression algorithms — critical for legacy WoW client support.

### 87. [ui/*.cpp] UI helper files need deeper audit
- **JS Source**: `src/js/ui/*.js` (9 files)
- **Status**: Pending
- **Details**: Files audio-helper, char-texture-overlay, character-appearance, data-exporter, listbox-context, model-viewer-utils need detailed comparison (texture-exporter, texture-ribbon, and uv-drawer already have entries 19–21). These handle UI logic for audio playback, character customization, data export, and model viewing.

### 88. [blte-stream-reader.cpp] Missing createReadableStream() method
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 148–170
- **Status**: Pending
- **Details**: JS `createReadableStream()` returns a `ReadableStream` with `pull` and `cancel` callbacks for progressive block consumption. Not used in the codebase currently, but missing from the C++ port. Needs a C++ equivalent (e.g., a generator/coroutine or pull-based iterator) or documentation as a deviation if ReadableStream semantics are not needed.

## Detailed CASC Audit Findings (2026-04-12)

### 89. [casc-source.cpp] Polymorphic getFile() dispatch broken — subclasses use different method name
- **JS Source**: `src/js/casc/casc-source-local.js` line 63, `src/js/casc/casc-source-remote.js` line 134
- **Status**: Pending
- **Details**: In JS, both `CASCLocal` and `CASCRemote` **override** `CASC.getFile()` (same method name) to accept additional parameters (`partialDecryption`, `suppressLog`, `supportFallback`, `forceFallback`, `contentKey`) and return a `BLTEReader`. The base class `getFileByName()` calls `this.getFile(fileDataID, ...)` which dispatches polymorphically. In C++, the subclasses define a **differently-named** method `getFileAsBLTE()` and do **not** override the base `getFile()`. This means `getFileByName()` always resolves to `CASC::getFile()`, returning only an encoding key string instead of decoded file data.

### 90. [casc-source.cpp] getFileByName() drops additional parameters when calling getFile()
- **JS Source**: `src/js/casc/casc-source.js` line 190
- **Status**: Pending
- **Details**: JS calls `return await this.getFile(fileDataID, partialDecrypt, suppressLog, supportFallback, forceFallback)` forwarding all 5 arguments. C++ calls `return getFile(fileDataID.value())` with only the fileDataID. Even if the polymorphic dispatch were fixed, the extra parameters are not forwarded.

### 91. [casc-source-local.cpp] Missing `core.view.casc = this` assignment in load()
- **JS Source**: `src/js/casc/casc-source-local.js` line 179
- **Status**: Pending
- **Details**: JS `load()` sets `core.view.casc = this` after loading root and before preparing listfiles, making the CASC instance accessible to the rest of the application. C++ omits this assignment.

### 92. [casc-source-remote.cpp] Missing `core.view.casc = this` assignment in load()
- **JS Source**: `src/js/casc/casc-source-remote.js` line 290
- **Status**: Pending
- **Details**: Same as entry 91 but for the remote source. JS sets `core.view.casc = this` at line 290 inside `load()`. C++ omits this.

### 93. [casc-source-remote.cpp] getConfig() checks res.empty() instead of HTTP status
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 73–81
- **Status**: Pending
- **Details**: JS checks `if (!res.ok)` (HTTP response status code) and includes the status code in the error message. C++ checks `if (res.empty())` which only detects completely empty responses. A server returning an HTML error page (non-empty, non-ok) would be treated as valid data in C++ but would throw in JS.

### 94. [casc-source-remote.cpp] getCDNConfig() checks res.empty() instead of HTTP status
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 100–101
- **Status**: Pending
- **Details**: Same pattern as entry 93. JS checks `if (!res.ok)` with status-code error message; C++ checks `if (res.empty())`.

### 95. [casc-source.cpp] getInstallManifest() falls back to raw key instead of propagating undefined
- **JS Source**: `src/js/casc/casc-source.js` line 72
- **Status**: Pending
- **Details**: JS: if `encodingKeys.get()` returns `undefined`, that undefined is used as `installKey`, which would fail downstream. C++ adds a fallback: if the key is not found in `encodingKeys`, it uses `installKeys[0]` as the key directly. This changes error semantics — some cases that would fail in JS might succeed in C++.

### 96. [casc-source-remote.cpp] getFileAsBLTE() null-check differs from JS
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 154–155
- **Status**: Pending
- **Details**: JS checks `if (data === null)` for unarchived files. C++ checks `if (data.byteLength() == 0)`. A valid zero-length file would pass JS's check but fail C++'s check, and a failed download returning a non-null empty buffer would fail differently.

### 97. [casc-source-local.cpp] init() logging less detailed than JS
- **JS Source**: `src/js/casc/casc-source-local.js` line 51
- **Status**: Pending
- **Details**: JS logs the full builds array with `log.write('%o', this.builds)`. C++ only logs the count. Same issue exists in `casc-source-remote.cpp` vs `casc-source-remote.js` line 55.

### 98. [casc-source-local.cpp] loadConfigs() logging omits config details
- **JS Source**: `src/js/casc/casc-source-local.js` lines 212–213
- **Status**: Pending
- **Details**: JS logs `'BuildConfig: %o'` and `'CDNConfig: %o'` dumping full config objects. C++ logs only `"BuildConfig loaded"` and `"CDNConfig loaded"` with no detail. Same in `casc-source-remote.cpp`.

### 99. [casc-source-remote.cpp] loadEncoding() uses build["BuildConfig"] instead of cache.key
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 312, 318
- **Status**: Pending
- **Details**: JS logs `'Encoding for build %s not cached, downloading.', this.cache.key`. C++ uses `build["BuildConfig"]` directly. When cache is shared from CASCLocal preload, `cache.key` references the local build's key, not necessarily `build["BuildConfig"]`.

### 100. [casc-source.cpp] getVirtualFileByID() error message differs from JS
- **JS Source**: `src/js/casc/casc-source.js` line 226
- **Status**: Pending
- **Details**: JS throws `'failed to map file: ' + mmap_obj.lastError` including the OS-level error. C++ throws `'failed to map file: ' + cachedPath` with just the file path, losing the diagnostic reason.

### 101. [casc-source.cpp] loadListfile() passes different type to listfile::applyPreload()
- **JS Source**: `src/js/casc/casc-source.js` lines 284–285
- **Status**: Pending
- **Details**: JS passes the full `rootEntries` Map to `listfile.applyPreload()`. C++ constructs a `std::unordered_set<uint32_t>` containing only the keys and passes that instead. This is functionally equivalent if `applyPreload` only uses key existence checks, but if it accesses the root entry values, the C++ version would break.

### 102. [build-cache.cpp] getFile() rejects immediately instead of waiting for cache integrity
- **JS Source**: `src/js/casc/build-cache.js` lines 71–73
- **Status**: Pending
- **Details**: JS `getFile()` calls `await cacheIntegrityReady()` which waits for the integrity system to be loaded via a Promise. C++ checks `cacheIntegrityLoaded` and immediately returns `std::nullopt` if not ready. Any call arriving before `initBuildCacheSystem()` completes will silently fail in C++ but would succeed in JS after waiting. Needs a condition variable or event wait.

### 103. [build-cache.cpp] storeFile() initializes empty map instead of waiting — potential data loss
- **JS Source**: `src/js/casc/build-cache.js` lines 108–109
- **Status**: Pending
- **Details**: JS `storeFile()` calls `await cacheIntegrityReady()` to wait for integrity data to load. C++, if `cacheIntegrityLoaded` is false, initializes an empty integrity map and continues. This could cause data loss — if `storeFile` is called before `initBuildCacheSystem()` finishes, C++ will overwrite `cacheIntegrity` with an empty object, discarding all previously cached integrity data.

### 104. [build-cache.cpp] getFile() holds global mutex during file I/O and hash computation
- **JS Source**: `src/js/casc/build-cache.js` lines 65–89
- **Status**: Pending
- **Details**: C++ `getFile()` acquires `cacheIntegrityMutex` and holds it while performing disk I/O and computing a SHA-1 hash, blocking all other cache operations. Correct approach: read integrity hash under lock, release, then do I/O and hash comparison without the lock. JS is single-threaded so this isn't an issue there.

### 105. [build-cache.cpp] casc-source-changed handler underflow guard changes accounting
- **JS Source**: `src/js/casc/build-cache.js` line 188
- **Status**: Pending
- **Details**: JS does `deleteSize -= manifestSize` unconditionally, which could produce a negative number (then `cacheSize -= deleteSize` would effectively add). C++ uses `uintmax_t` with underflow guard: `if (deleteSize >= manifestSize) deleteSize -= manifestSize;`. When `deleteSize < manifestSize`, the C++ skips the subtraction, reducing `cacheSize` by the full `deleteSize` instead of `deleteSize - manifestSize`.

### 106. [cdn-resolver.cpp] Architecture change — singleton class to namespace with static state
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 12–22, 180
- **Status**: Pending
- **Details**: JS exports a singleton class instance `new CDNResolver()` with instance fields. C++ uses namespace-scoped static variables. The C++ state can never be reset or replaced (e.g., for testing). Acceptable adaptation but should be documented as a deviation.

### 107. [cdn-resolver.cpp] getBestHost() fixes JS bug with cached promise handling
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 52–54
- **Status**: Pending
- **Details**: In JS `getBestHost`, when a second caller finds a cached entry with an active promise, it does `const result = await cached.promise; return result.host + ...` but `cached.promise` resolves to an **array**, so `result.host` is `undefined`, producing `"undefined/Path/"`. C++ correctly handles this by falling through to `rankedHosts[0].host`. This is a JS bug fix — should be documented as intentional deviation.

### 108. [cdn-resolver.cpp] getRankedHosts() uses future result directly instead of re-reading cache
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 82–86
- **Status**: Pending
- **Details**: JS re-reads the cache after the promise resolves. C++ uses the future's result directly, which is more robust. Behavioral difference but C++ approach is arguably better.

### 109. [cdn-resolver.cpp] _resolveRegionProduct() host computation order reversed
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 129–131
- **Status**: Pending
- **Details**: JS first formats `HOST` with region, then overrides for CN. C++ checks CN first. Functionally equivalent but structure deviates from original.

### 110. [cdn-resolver.cpp] Error logging uses e.what() instead of full error object
- **JS Source**: `src/js/casc/cdn-resolver.js` line 168
- **Status**: Pending
- **Details**: JS logs the full error object. C++ logs only `e.what()`. Minor fidelity difference in log output.

### 111. [cdn-resolver.cpp] Background thread cleanup missing
- **JS Source**: `src/js/casc/cdn-resolver.js` (uses async/await, no threads)
- **Status**: Pending
- **Details**: C++ stores `std::jthread` objects in a static vector but provides no mechanism to clean them up on shutdown. While `std::jthread` auto-joins on destruction, the static vector's destruction order is undefined relative to other statics. Should have an explicit cleanup function.

### 112. [listfile.cpp] listfile_check_cache_expiry TTL=0 check split differently
- **JS Source**: `src/js/casc/listfile.js` line 68
- **Status**: Pending
- **Details**: The TTL=0 check is structured differently and the log message says "out-of-date (> 0)" which is misleading. Behavioral difference in how TTL=0 special case is handled.

### 113. [listfile.cpp] preload() has no promise/future deduplication
- **JS Source**: `src/js/casc/listfile.js` lines 479–488
- **Status**: Pending
- **Details**: JS stores and reuses a single preload promise so concurrent callers await the same operation. C++ has no deduplication — a second caller returns immediately without waiting for the in-progress preload to complete.

### 114. [listfile.cpp] prepareListfile() does not wait for in-progress preload
- **JS Source**: `src/js/casc/listfile.js` lines 490–501
- **Status**: Pending
- **Details**: JS `prepareListfile()` awaits the preload promise. C++ does not actually wait for a preload that is still in progress, potentially using partial/stale data.

### 115. [listfile.cpp] loadUnknownModels() calls wrong initialization function
- **JS Source**: `src/js/casc/listfile.js` line 611
- **Status**: Pending
- **Details**: C++ calls `initializeModelFileData()` instead of `ensureInitialized()`. These may have different semantics (e.g., `ensureInitialized` may be idempotent while `initializeModelFileData` may re-initialize).

### 116. [listfile.cpp] getByID() returns empty string instead of std::optional
- **JS Source**: `src/js/casc/listfile.js` lines 779–795
- **Status**: Pending
- **Details**: JS returns `undefined` when a file ID is not found. C++ returns an empty string. Callers cannot distinguish "file not found" from "file has an empty filename".

### 117. [listfile.cpp] applyPreload() returns void instead of error code
- **JS Source**: `src/js/casc/listfile.js` lines 529, 593
- **Status**: Pending
- **Details**: JS returns `0` on error. C++ returns `void`, so callers cannot detect failure.

### 118. [listfile.cpp] filter_and_format lambda returns wrong type in legacy path
- **JS Source**: `src/js/casc/listfile.js` lines 573–583
- **Status**: Pending
- **Details**: C++ lambda returns `vector<nlohmann::json>` instead of `vector<string>` — type inconsistency with the legacy code path that may cause issues.

### 119. [listfile.cpp] renderListfile() empty vector conflates "all" with "filter to nothing"
- **JS Source**: `src/js/casc/listfile.js` lines 711, 726
- **Status**: Pending
- **Details**: JS uses `undefined` vs an empty array to distinguish "no filter" from "filter to nothing". C++ uses an empty vector for both, which conflates the two cases.

### 120. [listfile.cpp] ExtFilter hardcodes LISTFILE_MODEL_FILTER regex
- **JS Source**: `src/js/casc/listfile.js` line 513
- **Status**: Pending
- **Details**: JS stores the regex in the filter tuple. C++ `ExtFilter` struct hardcodes `LISTFILE_MODEL_FILTER`, which may not match the JS filter definition.

### 121. [export-helper.cpp] mark() sends stack trace to logging::write instead of console
- **JS Source**: `src/js/casc/export-helper.js` lines 253–255
- **Status**: Pending
- **Details**: JS uses `console.log` for the stack trace. C++ sends it to `logging::write`, which goes to the log file instead of the developer console.

### 122. [export-helper.cpp] start() toast cancel passes empty vector instead of null
- **JS Source**: `src/js/casc/export-helper.js` line 161
- **Status**: Pending
- **Details**: JS passes `null` as the toast options parameter. C++ passes `{}` (empty vector). This could affect toast rendering if it distinguishes between "no actions" (null) vs "empty actions array".

### 123. [export-helper.cpp] finish() missing third argument in cancelled toast call
- **JS Source**: `src/js/casc/export-helper.js` line 182
- **Status**: Pending
- **Details**: JS passes three arguments including `null` for options. C++ omits the third argument entirely.

### 124. [blp.cpp] _getAlpha case 4: integer vs float division fixes JS bug
- **JS Source**: `src/js/casc/blp.js` line 243
- **Status**: Pending
- **Details**: In JS, `index / 2` is floating-point division. For odd `index`, this produces a non-integer (e.g. 1.5), which when used as a Uint8Array index returns `undefined`. `undefined & 0xF0 = 0`, so odd-indexed pixels with 4-bit alpha all get alpha 0 in JS. C++ integer division truncates correctly. The C++ **fixes a JS bug** but creates a behavioral difference.

### 125. [db2.cpp] Missing proxy/guard behavior for lazy initialization
- **JS Source**: `src/js/casc/db2.js` (multiple locations)
- **Status**: Pending
- **Details**: JS db2 module uses patterns that provide lazy initialization and guarding behavior. C++ port may be missing equivalent protection, risking premature access to uninitialized data. Needs line-by-line comparison of all 4 guard patterns.

### 126. [jenkins96.cpp] Fallthrough in switch statement matches JS behavior
- **JS Source**: `src/js/casc/jenkins96.js`
- **Status**: Pending
- **Details**: Both JS and C++ have a fallthrough in the Jenkins96 hash computation switch statement. While this matches JS behavior, it should be verified that the C++ fallthrough is intentional and has a `[[fallthrough]]` attribute to suppress compiler warnings.

### 127. [realmlist.cpp] Missing HTTP status check in fetch
- **JS Source**: `src/js/casc/realmlist.js`
- **Status**: Pending
- **Details**: JS checks `res.ok` on the HTTP response. C++ may not check for HTTP error status codes, treating error responses as valid data.

### 128. [tact-keys.cpp] save() timing differs from JS
- **JS Source**: `src/js/casc/tact-keys.js`
- **Status**: Pending
- **Details**: JS defers save using setImmediate/setTimeout pattern. C++ saves synchronously, which could block the calling thread during file I/O.

### 129. [tact-keys.cpp] Missing HTTP status check in fetch
- **JS Source**: `src/js/casc/tact-keys.js`
- **Status**: Pending
- **Details**: Similar to realmlist — JS checks HTTP response status; C++ may not validate the response code.

### 130. [tact-keys.cpp] getKey() return semantics differ
- **JS Source**: `src/js/casc/tact-keys.js`
- **Status**: Pending
- **Details**: JS returns `undefined` when a key is not found. C++ may return an empty string or throw, which changes caller behavior.

### 131. [version-config.cpp] Empty input handling differs
- **JS Source**: `src/js/casc/version-config.js`
- **Status**: Pending
- **Details**: JS behavior on empty/null input to parseVersionConfig or parseBuildConfig may differ from C++. Needs verification of edge cases.

### 132. [vp9-avi-demuxer.cpp] Function returns differ from JS null semantics
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js`
- **Status**: Pending
- **Details**: JS returns `null` on parse failure. C++ may return empty data or throw. Callers checking `=== null` would need different logic.

### 133. [vp9-avi-demuxer.cpp] Logging detail differs from JS
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js`
- **Status**: Pending
- **Details**: JS may include more detailed logging (chunk types, sizes, offsets) during demuxing. C++ logging may be less verbose.

### 134. [dbd-manifest.cpp] prepareManifest() return type and preload state
- **JS Source**: `src/js/casc/dbd-manifest.js`
- **Status**: Pending
- **Details**: JS `prepareManifest()` returns a boolean and uses `is_preloaded` to gate re-entry. C++ needs to match the return type and ensure `is_preloaded` is thread-safe (should be `std::atomic<bool>` if accessed from multiple threads).

---

## Reverted Code Changes (Need Implementation)

The following entries document code changes that were made by a previous agent session but have been reverted per user request. They are documented here as pending work items.

### 135. [wowhead.cpp] Full JS-to-C++ conversion needed
- **JS Source**: `src/js/wowhead.js` (entire file)
- **Status**: Pending
- **Details**: The file currently contains the original JavaScript code. Needs full conversion to C++:
  - `charset` constant → `constexpr std::string_view`
  - `WOWHEAD_SLOT_TO_SLOT_ID` object → `std::unordered_map<int,int>`
  - `decode(str)` → `int decode(std::string_view)`
  - `decompress_zeros(str)` → `std::vector<int> decompress_zeros(std::string_view)`
  - `extract_hash_from_url(url)` → `std::string extract_hash_from_url(std::string_view)`
  - `parse_v15(hash)` → parses v15 wowhead outfit format
  - `parse_legacy(segments)` → parses legacy wowhead outfit format
  - `wowhead_parse_hash(hash)` → dispatches to v15 or legacy parser
  - Public `wowhead_parse(url_or_hash)` → main entry point returning `WowheadResult` struct
  - Create `wowhead.h` header with `WowheadResult` struct and public API
  - Add `wowhead.cpp` to `src/js/CMakeLists.txt`

### 136. [updater.cpp] Full JS-to-C++ conversion needed
- **JS Source**: `src/js/updater.js` (entire file)
- **Status**: Pending
- **Details**: The file currently contains the original JavaScript code. Needs full conversion to C++:
  - `checkForUpdates()` → fetches update manifest JSON, compares GUIDs, returns bool
  - `applyUpdate()` → downloads update files with progress, writes to disk
  - `launchUpdater()` → spawns external updater process (platform-specific: Windows uses `CreateProcess`/`ShellExecute`, Linux uses `fork`/`exec`)
  - Uses `generics::getJSON()`, `generics::downloadFile()`, `core::view`, `log::write()`
  - User-facing text should reference `wow.export.cpp` not `wow.export`
  - Create `updater.h` header with public API declarations
  - Requires `<filesystem>`, `<nlohmann/json.hpp>`, platform headers (`<windows.h>` / `<unistd.h>`)

### 137. [wmv.cpp] Full JS-to-C++ conversion needed
- **JS Source**: `src/js/wmv.js` (entire file)
- **Status**: Pending
- **Details**: The file currently contains the original JavaScript code. Needs full conversion to C++:
  - `wmv_parse(xml_str)` → parses `.chr` file XML, dispatches to v1/v2 parser
  - `wmv_parse_v2(data)` → parses v2 SavedCharacter format, extracts model path, race/gender, equipment slots, customizations
  - `wmv_parse_v1(data)` → parses v1 SavedCharacter format (simpler structure)
  - `extract_race_gender_from_path(path)` → regex extraction from model file path
  - Returns `WmvResult` struct with race, gender, items (slot→displayID), customizations
  - Uses `xml::parse_xml()` and `wow::get_slot_id_for_wmv_slot()`
  - Create `wmv.h` header with `WmvResult` struct and public API
  - Requires `<nlohmann/json.hpp>` for XML data access

### 138. [modules.cpp] Null pointer dereference in go_to_landing()
- **JS Source**: `src/js/modules.js`, `go_to_landing()` function
- **Status**: Pending
- **Details**: `modules::go_to_landing()` accesses `core::view->installType` without checking if `core::view` is null. In JS this is safe because `core.view` is always an object, but in C++ `core::view` can be `nullptr` (e.g. during startup before a source is selected). Fix: add `if (!core::view) return;` guard at the top of `go_to_landing()`.

### 139. [modules.cpp] wrap_module() display_label not updated from nav button
- **JS Source**: `src/js/modules.js`, `wrap_module()` function
- **Status**: Pending
- **Details**: In JS, `display_label` is set inside the `registerNavButton` callback to the nav button's label. In the C++ version, `display_label` stays as the module key name. After calling `mod.registerModule()`, the code should look up the registered nav button label: `auto it = nav_button_map.find(mod.name); if (it != nav_button_map.end()) display_label = it->second.label;` — this ensures error messages reference the user-visible tab name rather than the internal module key.

### 140. [cache-collector.cpp] https_request() hardcodes content-type instead of using caller's headers
- **JS Source**: `src/js/workers/cache-collector.js`, `https_request()` function
- **Status**: Pending
- **Details**: The `https_request()` function hardcodes `"application/octet-stream"` as the content type for POST requests. In JS, the caller controls the content type via `options.headers['Content-Type']`. Fix: extract `Content-Type` from the `headers` map parameter and pass it to `cli.Post()` instead of the hardcoded string.

### 141. [cache-collector.cpp] https_request() silently returns default response on connection error
- **JS Source**: `src/js/workers/cache-collector.js`, `https_request()` function
- **Status**: Pending
- **Details**: When `httplib::Result` is falsy (connection failure, DNS error, timeout), the C++ code returns a default `HttpResponse` with status 0 and empty data. In JS, `req.on('error', reject)` propagates the error as a promise rejection. Fix: throw `std::runtime_error` when `!res` to match JS error propagation behavior. Same issue exists in `json_post()`.
