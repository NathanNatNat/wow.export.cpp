# TODO Tracker

> **Progress: 0/74 verified (0%)** — ✅ = Verified, ⬜ = Pending

- [ ] 1. [app.cpp] Auto-updater flow from app.js is not ported
- **JS Source**: `src/app.js` lines 691–704
- **Details**: `updater.checkForUpdates()` / `updater.applyUpdate()` flow is still disabled in C++; only `tab_blender::checkLocalVersion()` runs.

- [ ] 2. [app.cpp] Drag enter/leave overlay behavior differs from JS
- **JS Source**: `src/app.js` lines 590–660
- **Details**: JS uses `ondragenter`/`ondragleave` plus `dropStack` to show and hide the file-drop prompt during drag-over, while C++ only handles drop callbacks.

- [ ] 3. [blob.cpp] Blob stream semantics differ (async pull stream vs eager callback loop)
- **JS Source**: `src/js/blob.js` lines 271–288
- **Details**: JS returns a lazy `ReadableStream` with async `pull()`, but C++ `BlobPolyfill::stream` is synchronous and eager.

- [ ] 4. [blob.cpp] URLPolyfill.createObjectURL native fallback path is missing
- **JS Source**: `src/js/blob.js` lines 294–300
- **Details**: JS falls back to native `URL.createObjectURL(blob)` for non-polyfill blobs; C++ only accepts `BlobPolyfill` and has no fallback path.

- [ ] 5. [blob.cpp] URLPolyfill.revokeObjectURL native revoke path is missing
- **JS Source**: `src/js/blob.js` lines 302–306
- **Details**: JS calls native `URL.revokeObjectURL(url)` for non-`data:` URLs; C++ is effectively a no-op for that case.

- [ ] 6. [buffer.cpp] alloc(false) behavior differs from JS allocUnsafe
- **JS Source**: `src/js/buffer.js` lines 54–56
- **Details**: JS uses `Buffer.allocUnsafe()` when `secure=false`; C++ always zero-initializes via `std::vector<uint8_t>(length)`.

- [ ] 7. [buffer.cpp] fromCanvas API/behavior is not directly ported
- **JS Source**: `src/js/buffer.js` lines 89–107
- **Details**: JS accepts canvas/OffscreenCanvas and browser Blob APIs, while C++ replaces this with `fromPixelData(...)` and different call semantics.

- [ ] 8. [buffer.cpp] readString encoding parameter does not affect behavior
- **JS Source**: `src/js/buffer.js` lines 551–553
- **Details**: JS passes `encoding` into `Buffer.toString(encoding, ...)`; C++ accepts the parameter but ignores it.

- [ ] 9. [buffer.cpp] decodeAudio(context) method is missing
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Details**: JS exposes `decodeAudio(context)` on `BufferWrapper`; C++ intentionally omits this method and relies on other audio paths.

- [ ] 10. [buffer.cpp] getDataURL creates data URLs instead of blob URLs
- **JS Source**: `src/js/buffer.js` lines 989–995
- **Details**: JS uses `URL.createObjectURL(new Blob(...))` (`blob:` URLs), while C++ emits `data:application/octet-stream;base64,...`.

- [ ] 11. [buffer.cpp] readBuffer wrap parameter is split into separate APIs
- **JS Source**: `src/js/buffer.js` lines 531–542
- **Details**: JS has `readBuffer(length, wrap=true, inflate=false)`; C++ replaces this with `readBuffer(...)` and `readBufferRaw(...)`.

- [ ] 12. [config.cpp] Deep config watcher auto-save path from Vue is not equivalent
- **JS Source**: `src/js/config.js` lines 60–61
- **Details**: JS attaches `core.view.$watch('config', () => save(), { deep: true })`; C++ relies on explicit manual `save()` calls from UI code.

- [ ] 13. [config.cpp] save scheduling differs from setImmediate event-loop behavior
- **JS Source**: `src/js/config.js` lines 83–90
- **Details**: JS defers with `setImmediate(doSave)` in the same event-loop model; C++ uses `std::async` thread execution.

- [ ] 14. [config.cpp] doSave write failure behavior differs
- **JS Source**: `src/js/config.js` lines 106–108
- **Details**: JS `await fsp.writeFile(...)` rejects on failure; C++ checks `file.is_open()` and can silently skip writing if open fails.

- [ ] 15. [config.cpp] EPERM detection logic differs from JS exception code check
- **JS Source**: `src/js/config.js` lines 43–49
- **Details**: JS checks `e.code === 'EPERM'`; C++ inspects exception message text for substrings like `EPERM`/`permission`.

- [ ] 16. [constants.cpp] DATA path root differs from JS nw.App.dataPath behavior
- **JS Source**: `src/js/constants.js` lines 16, 35–39
- **Details**: JS uses `nw.App.dataPath`; C++ uses `<install>/data`, changing where runtime/user files resolve.

- [ ] 17. [constants.cpp] Cache directory constant changed from casc/ to cache/
- **JS Source**: `src/js/constants.js` lines 73–81
- **Details**: JS cache namespace uses `DATA_PATH/casc/...`; C++ uses `data/cache/...` with migration logic.

- [ ] 18. [constants.cpp] Shader/default-config paths differ from JS layout
- **JS Source**: `src/js/constants.js` lines 43, 95–96
- **Details**: JS points to `INSTALL_PATH/src/shaders` and `INSTALL_PATH/src/default_config.jsonc`; C++ points to `<install>/data/...`.

- [ ] 19. [constants.h/constants.cpp] Version/flavour/build constants are compile-time values
- **JS Source**: `src/js/constants.js` line 46; `src/app.js` lines 44–47
- **Details**: JS reads these from `nw.App.manifest` at runtime; C++ hardcodes `VERSION`, `FLAVOUR`, and `BUILD_GUID`.

- [ ] 20. [constants.cpp] Updater helper extension mapping differs from JS
- **JS Source**: `src/js/constants.js` lines 18, 101
- **Details**: JS uses `UPDATER_EXT` mapping including `.app` for darwin; C++ only maps Windows/Linux helper naming.

- [ ] 21. [constants.cpp] Blender base-dir platform mapping omits JS darwin path
- **JS Source**: `src/js/constants.js` lines 24–32
- **Details**: JS handles `win32`, `darwin`, and `linux`; C++ implementation omits macOS branch.

- [ ] 22. [core.cpp] Loading screen updates are deferred to a main-thread queue instead of immediate state writes
- **JS Source**: `src/js/core.js` lines 413–420, 439–443
- **Details**: JS writes `core.view` loading fields synchronously in `showLoadingScreen()`/`hideLoadingScreen()`, while C++ posts these mutations via `postToMainThread()`, introducing frame-delayed behavior differences.

- [ ] 23. [core.cpp] progressLoadingScreen no longer awaits a forced redraw
- **JS Source**: `src/js/core.js` lines 426–434
- **Details**: JS explicitly calls `await generics.redraw()` after updating progress text/percentage; C++ removed the awaited redraw path and only queues state changes.

- [ ] 24. [core.cpp] Toast payload shape differs from JS `options` object contract
- **JS Source**: `src/js/core.js` lines 470–472
- **Details**: JS stores toast data as `{ type, message, options, closable }`, while C++ `setToast(...)` maps the third field to typed toast actions rather than a generic options object.

- [ ] 25. [external-links.cpp] JS logic is not implemented in the .cpp translation unit
- **JS Source**: `src/js/external-links.js` lines 12–44
- **Details**: `external-links.cpp` only includes `external-links.h`; the sibling `.cpp` file does not contain line-by-line equivalents of JS constants/methods (`STATIC_LINKS`, `WOWHEAD_ITEM`, `open`, `wowHead_viewItem`).

- [ ] 26. [file-writer.cpp] writeLine backpressure/await behavior differs from JS stream semantics
- **JS Source**: `src/js/file-writer.js` lines 24–33, 35–38
- **Details**: JS `writeLine()` is async and waits on resolver/drain when `stream.write()` backpressures; C++ writes synchronously and keeps blocked/drain as structural no-ops.

- [ ] 27. [file-writer.cpp] Closed-stream writes are silently ignored unlike JS stream-end behavior
- **JS Source**: `src/js/file-writer.js` lines 24–33, 40–42
- **Details**: C++ guards `writeLine()`/`close()` with `is_open()` and returns early; JS writes to a stream that has been `end()`ed follow Node stream error semantics rather than a silent no-op guard.

- [ ] 28. [generics.cpp] Exported get() API shape differs from JS fetch-style response contract
- **JS Source**: `src/js/generics.js` lines 22–54
- **Details**: JS `get()` returns a fetch `Response` object (`ok`, `status`, `statusText`, body/json methods), while C++ `get()` returns raw bytes and hides response metadata from callers.

- [ ] 29. [generics.cpp] get() fallback completion behavior differs when all URLs return non-ok responses
- **JS Source**: `src/js/generics.js` lines 38–54
- **Details**: JS returns the last non-ok `Response` after exhausting fallback URLs; C++ throws `HTTP <status> <statusText>` instead of returning a non-ok response object.

- [ ] 30. [generics.cpp] requestData status/redirect handling differs from JS manual 3xx flow
- **JS Source**: `src/js/generics.js` lines 159–167
- **Details**: JS manually follows 301/302 and accepts status codes up to 302 in that flow; C++ relies on auto-follow and then enforces a strict 2xx success check in `doHttpGetRaw()`.

- [ ] 31. [generics.cpp] redraw() is a no-op instead of double requestAnimationFrame scheduling
- **JS Source**: `src/js/generics.js` lines 263–268
- **Details**: JS resolves `redraw()` only after two animation frames to force UI repaint ordering; C++ redraw is intentionally empty, changing timing guarantees for callers that rely on redraw completion.

- [ ] 32. [generics.cpp] batchWork scheduling model differs from MessageChannel event-loop batching
- **JS Source**: `src/js/generics.js` lines 420–469
- **Details**: JS slices work via MessageChannel posts between batches; C++ runs batches in a tight loop with `std::this_thread::yield()`, which is not equivalent to browser event-loop task scheduling.

- [ ] 33. [gpu-info.cpp] macOS GPU info path from JS is missing in C++
- **JS Source**: `src/js/gpu-info.js` lines 199–243
- **Details**: JS implements `get_macos_gpu_info()` and a `darwin` branch in `get_platform_gpu_info()`, but C++ only handles Windows/Linux and returns `nullopt` for all other platforms.

- [ ] 34. [gpu-info.cpp] WebGL debug renderer detection logic differs from JS extension-gated behavior
- **JS Source**: `src/js/gpu-info.js` lines 30–34, 340–347
- **Details**: JS only populates vendor/renderer when `WEBGL_debug_renderer_info` is available and otherwise logs `WebGL debug info unavailable`; C++ reads `GL_VENDOR/GL_RENDERER` directly, changing the fallback path and emitted diagnostics.

- [ ] 35. [gpu-info.cpp] `exec_cmd` timeout behavior is not equivalent on Windows
- **JS Source**: `src/js/gpu-info.js` lines 65–73
- **Details**: JS enforces `{ timeout: 5000 }` through `child_process.exec`; C++ only wraps Linux commands with `timeout 5` and does not enforce the same timeout semantics on Windows `_popen`.

- [ ] 36. [gpu-info.cpp] Extension category normalization diverges from JS WebGL formatting
- **JS Source**: `src/js/gpu-info.js` lines 250–303
- **Details**: JS normalizes `WEBGL_/EXT_/OES_` extension names for compact logging, while C++ uses different `GL_ARB_/GL_EXT_/GL_OES_` stripping rules and produces different category labels/content.

- [ ] 37. [icon-render.cpp] Icon load pipeline is still stubbed and never replaces placeholders
- **JS Source**: `src/js/icon-render.js` lines 57–64, 93–106
- **Details**: JS asynchronously loads CASC icon data, decodes BLP, and updates the rendered icon; C++ `processQueue()` contains placeholder comments and does not fetch/decode icon data, so queued icons remain on the default image.

- [ ] 38. [icon-render.cpp] Queue execution model differs from JS async recursive processing
- **JS Source**: `src/js/icon-render.js` lines 48–65, 77–91
- **Details**: JS processes one queue entry per async chain and re-enters via `.finally(() => processQueue())`; C++ drains the queue in a synchronous `while` loop, changing scheduling and starvation behavior.

- [ ] 39. [icon-render.cpp] Dynamic stylesheet/CSS-rule icon path is replaced with non-equivalent texture cache flow
- **JS Source**: `src/js/icon-render.js` lines 14–46, 67–75, 93–109
- **Details**: JS creates `.icon-<id>` CSS rules in a dynamic stylesheet and renders via `background-image`; C++ uses `_registeredIcons/_textureCache` and does not implement stylesheet rule insertion/removal semantics from the JS module.

- [ ] 40. [log.cpp] `write()` API contract differs from JS variadic util.format behavior
- **JS Source**: `src/js/log.js` lines 78–80, 114
- **Details**: JS `write(...parameters)` formats arguments with `util.format`; C++ `write(std::string_view)` accepts only a pre-formatted message, changing caller-facing formatting behavior.

- [ ] 41. [log.cpp] Pool drain scheduling differs from JS `drain` event + `process.nextTick`
- **JS Source**: `src/js/log.js` lines 32–49, 111–112
- **Details**: JS drains pooled logs from stream `drain` events and recursively schedules with `process.nextTick`; C++ uses synchronous flush checks and a `drainPending` flag that only drains on later writes, which can leave queued entries undrained.

- [ ] 42. [log.cpp] Log stream initialization timing differs from JS module-load behavior
- **JS Source**: `src/js/log.js` lines 111–112
- **Details**: JS initializes `stream` at module load and immediately binds `drain`; C++ requires explicit `logging::init()` calls, so behavior differs if writes occur before initialization.

- [ ] 43. [mmap.cpp] Module architecture differs from JS wrapper around `mmap.node`
- **JS Source**: `src/js/mmap.js` lines 8–23, 49–52
- **Details**: JS delegates mapping behavior to native addon object construction (`new mmap_native.MmapObject()`); C++ reimplements mapping logic directly in this module, changing parity with the original JS/native boundary.

- [ ] 44. [mmap.cpp] Virtual-file ownership semantics differ from JS object-lifetime model
- **JS Source**: `src/js/mmap.js` lines 14–24, 30–43
- **Details**: JS tracks objects in a `Set` and only calls `unmap()`/`clear()`; C++ tracks raw pointers in a global set and deletes them in `release_virtual_files()`, introducing different lifetime/aliasing behavior versus JS-managed object references.

### 45. [modules.cpp] Dynamic component registry and hot-reload proxy behavior is not ported
- **JS Source**: `src/js/modules.js` lines 6–24, 62–109, 270–275
- **Status**: Pending
- **Details**: JS exposes `COMPONENTS`, `COMPONENT_PATH_MAP`, `component_cache`, and `component_registry` proxy with dynamic require-cache invalidation; C++ replaces this with a static/no-op `register_components()` path.

### 46. [modules.cpp] `wrap_module` computed helper injection differs from JS
- **JS Source**: `src/js/modules.js` lines 200–207
- **Status**: Pending
- **Details**: JS injects `$modules`, `$core`, and `$components` via `module_def.computed`; C++ does not provide equivalent computed helper bindings.

### 47. [modules.cpp] `activated` lifecycle wrapping logic is missing
- **JS Source**: `src/js/modules.js` lines 244–251
- **Status**: Pending
- **Details**: JS wraps `activated` so `initialize()` is retried before calling original `activated`; C++ only wraps `initialize` and does not implement equivalent `activated` wrapper behavior.

### 48. [modules.cpp] `initialize(core_instance)` bootstrap flow differs from JS module wiring
- **JS Source**: `src/js/modules.js` lines 277–287
- **Status**: Pending
- **Details**: JS stores `core_instance`, assigns `manager = module.exports`, and wraps every entry in `MODULES` with `Vue.markRaw`; C++ `initialize()` takes no core parameter and builds a static function-pointer registry instead.

### 49. [modules.cpp] `set_active` assigns different active-module payload to view state
- **JS Source**: `src/js/modules.js` lines 254–267, 293–303
- **Status**: Pending
- **Details**: JS sets `core.view.activeModule` to the wrapped module proxy (including proxy getters like `__name`/`setActive`/`reload`); C++ writes a minimal JSON object with `__name` only.

### 50. [modules.cpp] Dev hot-reload no longer refreshes module/component code from disk
- **JS Source**: `src/js/modules.js` lines 337–355, 395–401
- **Status**: Pending
- **Details**: JS clears `require.cache` and re-requires modules/components during reload; C++ only resets internal flags and re-wraps existing in-memory module definitions.

### 51. [modules.cpp] `set_active` eagerly initializes modules unlike JS activation flow
- **JS Source**: `src/js/modules.js` lines 244–251, 293–303
- **Status**: Pending
- **Details**: JS `set_active` only switches `core.view.activeModule`; first-time initialization is triggered by wrapped `activated()`. C++ calls `initialize()` directly inside `set_active`, changing lifecycle timing/side effects.

### 52. [modules.cpp] `modContextMenuOptions` payload omits JS `action` object data
- **JS Source**: `src/js/modules.js` lines 162–167, 176–198, 289–291
- **Status**: Pending
- **Details**: JS writes option objects containing `action` (or `{ handler, dev_only }` for static options) into `core.view.modContextMenuOptions`; C++ serializes only `id/label/icon/dev_only` to view state, dropping JS action payload shape.

### 53. [modules.cpp] Unknown nav/context entries lose JS insertion-order behavior
- **JS Source**: `src/js/modules.js` lines 112–114, 139–157, 176–195
- **Status**: Pending
- **Details**: JS builds arrays from `Map` insertion order before stable sort (entries not in order arrays keep insertion order). C++ stores entries in `std::map`, so unordered items are pre-sorted by key, changing final order semantics.

### 54. [MultiMap.cpp] MultiMap logic is not ported in the `.cpp` sibling translation unit
- **JS Source**: `src/js/MultiMap.js` lines 6–32
- **Status**: Pending
- **Details**: The JS sibling contains the full `MultiMap extends Map` implementation, but `src/js/MultiMap.cpp` only includes `MultiMap.h` and comments; line-by-line implementation parity is not present in the `.cpp` file itself.

### 55. [MultiMap.cpp] Public API model differs from JS `Map` subclass contract
- **JS Source**: `src/js/MultiMap.js` lines 6, 20–28, 32
- **Status**: Pending
- **Details**: JS exports an actual `Map` subclass with standard `Map` behavior/interop, while C++ exposes a template wrapper (header implementation) returning `std::variant` pointers and not `Map`-equivalent runtime semantics.

### 56. [png-writer.cpp] `write()` call contract differs from JS async behavior
- **JS Source**: `src/js/png-writer.js` lines 243–249
- **Status**: Pending
- **Details**: JS `write(file)` is `async` and returns/awaits `this.getBuffer().writeToFile(file)`; C++ `PNGWriter::write(...)` is synchronous `void`.

### 57. [stb-impl.cpp] Required sibling JS source file is missing, blocking parity verification
- **JS Source**: `src/js/stb-impl.js` lines N/A (file missing)
- **Status**: Blocked
- **Details**: `src/js/stb-impl.cpp` exists, but `src/js/stb-impl.js` is absent, so line-by-line comparison against an original JS sibling cannot be completed.

### 58. [subtitles.cpp] `get_subtitles_vtt` API and data-loading path differ from JS
- **JS Source**: `src/js/subtitles.js` lines 172–175
- **Status**: Pending
- **Details**: JS `get_subtitles_vtt(casc, file_data_id, format)` loads file data internally via CASC; C++ takes preloaded subtitle text and format only.

### 59. [subtitles.cpp] BOM stripping behavior differs from original JS
- **JS Source**: `src/js/subtitles.js` lines 176–178
- **Status**: Pending
- **Details**: JS removes leading UTF-16 BOM codepoint (`0xFEFF`) via `charCodeAt`; C++ strips UTF-8 byte-order mark bytes (`EF BB BF`) instead.

### 60. [subtitles.cpp] Invalid SBT timestamp parsing semantics differ from JS `parseInt` behavior
- **JS Source**: `src/js/subtitles.js` lines 13–20
- **Status**: Pending
- **Details**: JS uses `parseInt(...)` and can propagate `NaN` for malformed timestamp segments, while C++ digit-filter parsing can still produce numeric output from mixed/invalid strings.

### 61. [tiled-png-writer.cpp] `write()` contract is synchronous instead of JS Promise-based async
- **JS Source**: `src/js/tiled-png-writer.js` lines 123–125
- **Status**: Pending
- **Details**: JS exposes `async write(file)` and returns `await this.getBuffer().writeToFile(file)`, while C++ `TiledPNGWriter::write(...)` is `void` and synchronous.

### 62. [updater.cpp] Update manifest flavour/guid source differs from JS runtime manifest
- **JS Source**: `src/js/updater.js` lines 24–26, 33–35, 113
- **Status**: Pending
- **Details**: JS reads `nw.App.manifest.flavour/guid` at runtime, while C++ uses `constants::FLAVOUR` and `constants::BUILD_GUID`, changing update target selection/comparison behavior.

### 63. [updater.cpp] Async update flow is flattened into synchronous calls
- **JS Source**: `src/js/updater.js` lines 50, 61, 79, 103–104, 119–124
- **Status**: Pending
- **Details**: JS `applyUpdate`/`launchUpdater` are async and await progress/hash/download/process-launch steps; C++ runs these paths synchronously with blocking calls and no Promise-equivalent sequencing.

### 64. [updater.cpp] Launch failure logging omits JS error-object log line
- **JS Source**: `src/js/updater.js` lines 163–166
- **Status**: Pending
- **Details**: JS catch block logs both formatted message and the raw error object (`log.write(e)`), while C++ only logs the formatted message text (`e.what()`).

### 65. [wowhead.cpp] Parse result field name differs from JS API (`class` vs `player_class`)
- **JS Source**: `src/js/wowhead.js` lines 172, 226
- **Status**: Pending
- **Details**: JS returns `class` in parsed output objects; C++ stores this value in `ParseResult::player_class`, changing the exported result shape.

### 66. [xml.cpp] End-of-input handling can dereference past bounds unlike JS parser semantics
- **JS Source**: `src/js/xml.js` lines 25–29, 39, 89, 97
- **Status**: Pending
- **Details**: JS safely reads `xml[pos]` as `undefined` at end-of-input, but C++ reads `xml[pos]` in `parse_attributes()`/`parse_node()` without guarding `pos < xml.size()` at several checks, which can trigger out-of-bounds access on malformed/truncated XML.

### 67. [Skin.cpp] `load()` API timing differs from JS Promise-based async flow
- **JS Source**: `src/js/3D/Skin.js` lines 20–23, 96–100
- **Status**: Pending
- **Details**: JS exposes `async load()` and awaits CASC file retrieval (`await core.view.casc.getFile(...)`), while C++ `Skin::load()` is synchronous and throws directly, changing caller timing/error-propagation semantics.

### 68. [Texture.cpp] `getTextureFile()` return contract differs from JS async/null behavior
- **JS Source**: `src/js/3D/Texture.js` lines 32–41
- **Status**: Pending
- **Details**: JS returns a Promise from `async getTextureFile()` and yields `null` when unset; C++ returns `std::optional<BufferWrapper>` synchronously, changing both async behavior and API shape.

### 69. [WMOShaderMapper.cpp] Pixel shader enum naming deviates from JS export contract
- **JS Source**: `src/js/3D/WMOShaderMapper.js` lines 35, 90, 94
- **Status**: Pending
- **Details**: JS exports `WMOPixelShader.MapObjParallax`, while C++ renames this constant to `MapObjParallax_PS`; numeric mapping is preserved but exported identifier parity differs from the original module.

### 70. [CameraControlsGL.cpp] Event listener lifecycle from JS `init()/dispose()` is not ported equivalently
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 198–221
- **Status**: Pending
- **Details**: JS `init()` registers DOM/document listeners and `dispose()` removes document listeners, but C++ relies on externally forwarded events and only resets state, changing ownership/lifecycle semantics.

### 71. [CameraControlsGL.cpp] Input default-handling behavior differs from JS browser event flow
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 223–248, 257–262, 309–329
- **Status**: Pending
- **Details**: JS calls `preventDefault()` (and wheel `stopPropagation()`), while C++ handler methods have no equivalent suppression path, so browser-default behavior parity is not represented.

### 72. [CameraControlsGL.cpp] Mouse-down focus fallback differs from JS
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` line 226
- **Status**: Pending
- **Details**: JS falls back to `window.focus()` when `dom_element.focus` is unavailable; C++ only invokes `dom_element.focus` if present and has no fallback focus path.

### 73. [CharacterCameraControlsGL.cpp] DOM/document listener registration-removal flow differs from JS
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 27–35, 42–43, 51–52, 122–129, 170–175
- **Status**: Pending
- **Details**: JS stores handler refs, registers mousedown/wheel/contextmenu listeners in constructor, dynamically attaches/removes document mousemove/mouseup listeners, and removes listeners in `dispose()`; C++ omits this lifecycle and depends on caller-forwarded events.

### 74. [CharacterCameraControlsGL.cpp] Event suppression parity is missing in mouse handlers
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 45, 54, 68, 114, 133–135
- **Status**: Pending
- **Details**: JS calls `preventDefault()` during rotate/pan interactions and both `preventDefault()`/`stopPropagation()` on wheel; C++ has no equivalent event suppression behavior.
