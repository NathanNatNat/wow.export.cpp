# TODO Tracker

> **Progress: 1/462 verified (0%)** — ✅ = Verified, ⬜ = Pending

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

- [ ] 45. [modules.cpp] Dynamic component registry and hot-reload proxy behavior is not ported
- **JS Source**: `src/js/modules.js` lines 6–24, 62–109, 270–275
- **Status**: Pending
- **Details**: JS exposes `COMPONENTS`, `COMPONENT_PATH_MAP`, `component_cache`, and `component_registry` proxy with dynamic require-cache invalidation; C++ replaces this with a static/no-op `register_components()` path.

- [ ] 46. [modules.cpp] `wrap_module` computed helper injection differs from JS
- **JS Source**: `src/js/modules.js` lines 200–207
- **Status**: Pending
- **Details**: JS injects `$modules`, `$core`, and `$components` via `module_def.computed`; C++ does not provide equivalent computed helper bindings.

- [ ] 47. [modules.cpp] `activated` lifecycle wrapping logic is missing
- **JS Source**: `src/js/modules.js` lines 244–251
- **Status**: Pending
- **Details**: JS wraps `activated` so `initialize()` is retried before calling original `activated`; C++ only wraps `initialize` and does not implement equivalent `activated` wrapper behavior.

- [ ] 48. [modules.cpp] `initialize(core_instance)` bootstrap flow differs from JS module wiring
- **JS Source**: `src/js/modules.js` lines 277–287
- **Status**: Pending
- **Details**: JS stores `core_instance`, assigns `manager = module.exports`, and wraps every entry in `MODULES` with `Vue.markRaw`; C++ `initialize()` takes no core parameter and builds a static function-pointer registry instead.

- [ ] 49. [modules.cpp] `set_active` assigns different active-module payload to view state
- **JS Source**: `src/js/modules.js` lines 254–267, 293–303
- **Status**: Pending
- **Details**: JS sets `core.view.activeModule` to the wrapped module proxy (including proxy getters like `__name`/`setActive`/`reload`); C++ writes a minimal JSON object with `__name` only.

- [ ] 50. [modules.cpp] Dev hot-reload no longer refreshes module/component code from disk
- **JS Source**: `src/js/modules.js` lines 337–355, 395–401
- **Status**: Pending
- **Details**: JS clears `require.cache` and re-requires modules/components during reload; C++ only resets internal flags and re-wraps existing in-memory module definitions.

- [ ] 51. [modules.cpp] `set_active` eagerly initializes modules unlike JS activation flow
- **JS Source**: `src/js/modules.js` lines 244–251, 293–303
- **Status**: Pending
- **Details**: JS `set_active` only switches `core.view.activeModule`; first-time initialization is triggered by wrapped `activated()`. C++ calls `initialize()` directly inside `set_active`, changing lifecycle timing/side effects.

- [ ] 52. [modules.cpp] `modContextMenuOptions` payload omits JS `action` object data
- **JS Source**: `src/js/modules.js` lines 162–167, 176–198, 289–291
- **Status**: Pending
- **Details**: JS writes option objects containing `action` (or `{ handler, dev_only }` for static options) into `core.view.modContextMenuOptions`; C++ serializes only `id/label/icon/dev_only` to view state, dropping JS action payload shape.

- [ ] 53. [modules.cpp] Unknown nav/context entries lose JS insertion-order behavior
- **JS Source**: `src/js/modules.js` lines 112–114, 139–157, 176–195
- **Status**: Pending
- **Details**: JS builds arrays from `Map` insertion order before stable sort (entries not in order arrays keep insertion order). C++ stores entries in `std::map`, so unordered items are pre-sorted by key, changing final order semantics.

- [ ] 54. [MultiMap.cpp] MultiMap logic is not ported in the `.cpp` sibling translation unit
- **JS Source**: `src/js/MultiMap.js` lines 6–32
- **Status**: Pending
- **Details**: The JS sibling contains the full `MultiMap extends Map` implementation, but `src/js/MultiMap.cpp` only includes `MultiMap.h` and comments; line-by-line implementation parity is not present in the `.cpp` file itself.

- [ ] 55. [MultiMap.cpp] Public API model differs from JS `Map` subclass contract
- **JS Source**: `src/js/MultiMap.js` lines 6, 20–28, 32
- **Status**: Pending
- **Details**: JS exports an actual `Map` subclass with standard `Map` behavior/interop, while C++ exposes a template wrapper (header implementation) returning `std::variant` pointers and not `Map`-equivalent runtime semantics.

- [ ] 56. [png-writer.cpp] `write()` call contract differs from JS async behavior
- **JS Source**: `src/js/png-writer.js` lines 243–249
- **Status**: Pending
- **Details**: JS `write(file)` is `async` and returns/awaits `this.getBuffer().writeToFile(file)`; C++ `PNGWriter::write(...)` is synchronous `void`.

- [ ] 57. [stb-impl.cpp] Required sibling JS source file is missing, blocking parity verification
- **JS Source**: `src/js/stb-impl.js` lines N/A (file missing)
- **Status**: Blocked
- **Details**: `src/js/stb-impl.cpp` exists, but `src/js/stb-impl.js` is absent, so line-by-line comparison against an original JS sibling cannot be completed.

- [ ] 58. [subtitles.cpp] `get_subtitles_vtt` API and data-loading path differ from JS
- **JS Source**: `src/js/subtitles.js` lines 172–175
- **Status**: Pending
- **Details**: JS `get_subtitles_vtt(casc, file_data_id, format)` loads file data internally via CASC; C++ takes preloaded subtitle text and format only.

- [ ] 59. [subtitles.cpp] BOM stripping behavior differs from original JS
- **JS Source**: `src/js/subtitles.js` lines 176–178
- **Status**: Pending
- **Details**: JS removes leading UTF-16 BOM codepoint (`0xFEFF`) via `charCodeAt`; C++ strips UTF-8 byte-order mark bytes (`EF BB BF`) instead.

- [ ] 60. [subtitles.cpp] Invalid SBT timestamp parsing semantics differ from JS `parseInt` behavior
- **JS Source**: `src/js/subtitles.js` lines 13–20
- **Status**: Pending
- **Details**: JS uses `parseInt(...)` and can propagate `NaN` for malformed timestamp segments, while C++ digit-filter parsing can still produce numeric output from mixed/invalid strings.

- [ ] 61. [tiled-png-writer.cpp] `write()` contract is synchronous instead of JS Promise-based async
- **JS Source**: `src/js/tiled-png-writer.js` lines 123–125
- **Status**: Pending
- **Details**: JS exposes `async write(file)` and returns `await this.getBuffer().writeToFile(file)`, while C++ `TiledPNGWriter::write(...)` is `void` and synchronous.

- [ ] 62. [updater.cpp] Update manifest flavour/guid source differs from JS runtime manifest
- **JS Source**: `src/js/updater.js` lines 24–26, 33–35, 113
- **Status**: Pending
- **Details**: JS reads `nw.App.manifest.flavour/guid` at runtime, while C++ uses `constants::FLAVOUR` and `constants::BUILD_GUID`, changing update target selection/comparison behavior.

- [ ] 63. [updater.cpp] Async update flow is flattened into synchronous calls
- **JS Source**: `src/js/updater.js` lines 50, 61, 79, 103–104, 119–124
- **Status**: Pending
- **Details**: JS `applyUpdate`/`launchUpdater` are async and await progress/hash/download/process-launch steps; C++ runs these paths synchronously with blocking calls and no Promise-equivalent sequencing.

- [ ] 64. [updater.cpp] Launch failure logging omits JS error-object log line
- **JS Source**: `src/js/updater.js` lines 163–166
- **Status**: Pending
- **Details**: JS catch block logs both formatted message and the raw error object (`log.write(e)`), while C++ only logs the formatted message text (`e.what()`).

- [ ] 65. [wowhead.cpp] Parse result field name differs from JS API (`class` vs `player_class`)
- **JS Source**: `src/js/wowhead.js` lines 172, 226
- **Status**: Pending
- **Details**: JS returns `class` in parsed output objects; C++ stores this value in `ParseResult::player_class`, changing the exported result shape.

- [ ] 66. [xml.cpp] End-of-input handling can dereference past bounds unlike JS parser semantics
- **JS Source**: `src/js/xml.js` lines 25–29, 39, 89, 97
- **Status**: Pending
- **Details**: JS safely reads `xml[pos]` as `undefined` at end-of-input, but C++ reads `xml[pos]` in `parse_attributes()`/`parse_node()` without guarding `pos < xml.size()` at several checks, which can trigger out-of-bounds access on malformed/truncated XML.

- [ ] 67. [Skin.cpp] `load()` API timing differs from JS Promise-based async flow
- **JS Source**: `src/js/3D/Skin.js` lines 20–23, 96–100
- **Status**: Pending
- **Details**: JS exposes `async load()` and awaits CASC file retrieval (`await core.view.casc.getFile(...)`), while C++ `Skin::load()` is synchronous and throws directly, changing caller timing/error-propagation semantics.

- [ ] 68. [Texture.cpp] `getTextureFile()` return contract differs from JS async/null behavior
- **JS Source**: `src/js/3D/Texture.js` lines 32–41
- **Status**: Pending
- **Details**: JS returns a Promise from `async getTextureFile()` and yields `null` when unset; C++ returns `std::optional<BufferWrapper>` synchronously, changing both async behavior and API shape.

- [ ] 69. [WMOShaderMapper.cpp] Pixel shader enum naming deviates from JS export contract
- **JS Source**: `src/js/3D/WMOShaderMapper.js` lines 35, 90, 94
- **Status**: Pending
- **Details**: JS exports `WMOPixelShader.MapObjParallax`, while C++ renames this constant to `MapObjParallax_PS`; numeric mapping is preserved but exported identifier parity differs from the original module.

- [ ] 70. [CameraControlsGL.cpp] Event listener lifecycle from JS `init()/dispose()` is not ported equivalently
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 198–221
- **Status**: Pending
- **Details**: JS `init()` registers DOM/document listeners and `dispose()` removes document listeners, but C++ relies on externally forwarded events and only resets state, changing ownership/lifecycle semantics.

- [ ] 71. [CameraControlsGL.cpp] Input default-handling behavior differs from JS browser event flow
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 223–248, 257–262, 309–329
- **Status**: Pending
- **Details**: JS calls `preventDefault()` (and wheel `stopPropagation()`), while C++ handler methods have no equivalent suppression path, so browser-default behavior parity is not represented.

- [ ] 72. [CameraControlsGL.cpp] Mouse-down focus fallback differs from JS
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` line 226
- **Status**: Pending
- **Details**: JS falls back to `window.focus()` when `dom_element.focus` is unavailable; C++ only invokes `dom_element.focus` if present and has no fallback focus path.

- [ ] 73. [CharacterCameraControlsGL.cpp] DOM/document listener registration-removal flow differs from JS
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 27–35, 42–43, 51–52, 122–129, 170–175
- **Status**: Pending
- **Details**: JS stores handler refs, registers mousedown/wheel/contextmenu listeners in constructor, dynamically attaches/removes document mousemove/mouseup listeners, and removes listeners in `dispose()`; C++ omits this lifecycle and depends on caller-forwarded events.

- [ ] 74. [CharacterCameraControlsGL.cpp] Event suppression parity is missing in mouse handlers
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 45, 54, 68, 114, 133–135
- **Status**: Pending
- **Details**: JS calls `preventDefault()` during rotate/pan interactions and both `preventDefault()`/`stopPropagation()` on wheel; C++ has no equivalent event suppression behavior.

- [ ] 75. [ADTExporter.cpp] `calculateUVBounds` skips chunks when `vertices` is empty, unlike JS truthiness check
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 267–268
- **Status**: Pending
- **Details**: JS only skips when `chunk`/`chunk.vertices` is missing; an empty typed array is still truthy and processing continues. C++ adds `chunk.vertices.empty()` as an additional skip condition, changing edge-case behavior.

- [ ] 76. [ADTExporter.cpp] Export API flow is synchronous instead of JS Promise-based `async export()`
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 309–367
- **Status**: Pending
- **Details**: JS `export()` is asynchronous and yields between CASC/file operations; C++ `exportTile()` performs the flow synchronously, changing timing/cancellation behavior relative to the original async path.

- [ ] 77. [CharacterExporter.cpp] `get_item_id_for_slot` does not preserve JS falsy fallback semantics
- **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 342–345
- **Status**: Pending
- **Details**: JS uses `a || b || null`, so a slot `item_id` of `0` falls through to collection/null. C++ returns the first found `item_id` directly (including `0`), which differs for falsy-ID edge cases.

- [ ] 78. [M2Exporter.cpp] `addURITexture` input contract differs from JS (data URI string vs decoded PNG buffer)
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59–61, 111–112
- **Status**: Pending
- **Details**: JS stores a data-URI string and decodes it inside `exportTextures()`. C++ `addURITexture` accepts `BufferWrapper` PNG bytes directly, changing caller-facing behavior and where decoding occurs.

- [ ] 79. [M2Exporter.cpp] Equipment UV2 export guard differs from JS truthy check
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 568
- **Status**: Pending
- **Details**: JS exports UV2 when `config.modelsExportUV2 && uv2` (empty arrays are truthy). C++ requires `!uv2.empty()`, so empty-but-present UV2 buffers are not exported.

- [ ] 80. [M2LegacyExporter.cpp] Skin texture override condition differs when `skinTextures` is an empty array
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 65–70, 176–181, 220–225
- **Status**: Pending
- **Details**: JS checks `this.skinTextures` truthiness (empty array is truthy) and may overwrite to `undefined`, then skip texture. C++ requires `!skinTextures.empty()`, so it keeps original texture paths in that edge case.

- [ ] 81. [M2LegacyExporter.cpp] Export API flow is synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 39, 123, 262, 299
- **Status**: Pending
- **Details**: JS export methods (`exportTextures`, `exportAsOBJ`, `exportAsSTL`, `exportRaw`) are async and yield during I/O. C++ runs these paths synchronously, altering timing/cancellation behavior versus JS.

- [ ] 82. [M3Exporter.cpp] `addURITexture` input contract differs from JS (data URI string vs decoded PNG buffer)
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49–50
- **Status**: Pending
- **Details**: JS stores raw data-URI strings in `dataTextures`; C++ stores `BufferWrapper` PNG bytes, changing caller contract and data normalization stage.

- [ ] 83. [M3Exporter.cpp] UV2 export condition checks non-empty instead of JS defined-ness
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 88–89, 141–142
- **Status**: Pending
- **Details**: JS exports UV1 whenever it is defined (`!== undefined`), including empty arrays. C++ requires `!m3->uv1.empty()`, which changes behavior for defined-but-empty UV sets.

- [ ] 84. [WMOExporter.cpp] Export methods are synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 62, 219, 360, 739, 841, 1179
- **Status**: Pending
- **Details**: JS uses async export methods (`exportTextures`, `exportAsGLTF`, `exportAsOBJ`, `exportAsSTL`, `exportGroupsAsSeparateOBJ`, `exportRaw`) with awaited CASC/file operations, while C++ executes these paths synchronously.

- [ ] 85. [WMOLegacyExporter.cpp] Export methods are synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 47, 130, 392, 478
- **Status**: Pending
- **Details**: JS legacy WMO export methods are async and await texture/model I/O; C++ methods (`exportTextures`, `exportAsOBJ`, `exportAsSTL`, `exportRaw`) are synchronous, changing timing/cancellation semantics.

- [ ] 86. [GLContext.cpp] Context creation and capability detection behavior differs from JS canvas/WebGL2 path
- **JS Source**: `src/js/3D/gl/GLContext.js` lines 29–41, 55–63
- **Status**: Pending
- **Details**: JS creates the context with per-call options and throws `WebGL2 not supported` if context acquisition fails; it also conditionally enables anisotropy/float-texture extension flags. C++ assumes an already-created GL context and sets some capability flags via desktop-core assumptions instead of matching JS extension-availability behavior.

- [ ] 87. [ANIMLoader.cpp] `load` API is synchronous instead of JS async Promise-based method
- **JS Source**: `src/js/3D/loaders/ANIMLoader.js` line 25
- **Status**: Pending
- **Details**: JS exposes `async load(isChunked = true)` while C++ exposes synchronous `void load(bool isChunked)`, changing API timing/await semantics.

- [ ] 88. [BONELoader.cpp] `load` API is synchronous instead of JS async Promise-based method
- **JS Source**: `src/js/3D/loaders/BONELoader.js` line 24
- **Status**: Pending
- **Details**: JS exposes `async load()` while C++ exposes synchronous `void load()`, changing API timing/await semantics.

- [ ] 89. [M2LegacyLoader.cpp] `load`/`getSkin` APIs are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/M2LegacyLoader.js` lines 34, 819
- **Status**: Pending
- **Details**: JS exposes `async load()` and `async getSkin(index)` while C++ exposes synchronous `void load()` and `LegacyM2Skin& getSkin(int)`, changing await/timing behavior.

- [ ] 90. [M2Loader.cpp] Primary loader methods are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 37, 67, 87, 146, 332
- **Status**: Pending
- **Details**: JS exposes async `load`, `getSkin`, `loadAnims`, `loadAnimsForIndex`, and `parseChunk_MD21`; C++ ports these as synchronous methods, altering call/await semantics.

- [ ] 91. [M2Loader.cpp] `loadAnims` error propagation differs from JS
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 87–138
- **Status**: Pending
- **Details**: JS `loadAnims` does not catch loader/CASC failures (Promise rejects). C++ wraps per-entry loads in `try/catch` and continues, swallowing errors and changing failure behavior.

- [ ] 92. [M2Loader.cpp] Model-name null stripping differs from original JS behavior
- **JS Source**: `src/js/3D/loaders/M2Loader.js` line 792
- **Status**: Pending
- **Details**: JS calls `fileName.replace('\0', '')` (single replacement call result not reassigned), while C++ removes all null bytes in-place; resulting `name` values differ.

- [ ] 93. [M3Loader.cpp] Loader methods are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/M3Loader.js` lines 67, 104, 269, 277, 299, 315
- **Status**: Pending
- **Details**: JS exposes async `load`, `parseChunk_M3DT`, and async sub-chunk parsers; C++ ports these paths as synchronous calls, changing API timing/await semantics.

- [ ] 94. [MDXLoader.cpp] `load` API is synchronous instead of JS async Promise-based method
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` line 28
- **Status**: Pending
- **Details**: JS exposes `async load()` while C++ exposes synchronous `void load()`, changing await/timing behavior.

- [ ] 95. [SKELLoader.cpp] Loader animation APIs are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 36, 308, 407
- **Status**: Pending
- **Details**: JS exposes async `load`, `loadAnimsForIndex`, and `loadAnims`; C++ ports all three as synchronous methods, altering call/await behavior.

- [ ] 96. [SKELLoader.cpp] Animation-load failure handling differs from JS
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 332–344, 438–448
- **Status**: Pending
- **Details**: JS does not catch ANIM/CASC load failures in `loadAnimsForIndex`/`loadAnims` (Promise rejects). C++ catches exceptions, logs, and returns/continues, changing failure propagation.

- [ ] 97. [WDTLoader.cpp] `MWMO` string null handling differs from JS
- **JS Source**: `src/js/3D/loaders/WDTLoader.js` line 86
- **Status**: Pending
- **Details**: JS uses `.replace('\0', '')` (first match only), while C++ removes all `'\0'` bytes from the string, producing different `worldModel` values in edge cases.

- [ ] 98. [WMOLegacyLoader.cpp] `load`/internal load helpers/`getGroup` are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 33, 54, 86, 116
- **Status**: Pending
- **Details**: JS uses async `load`, `_load_alpha_format`, `_load_standard_format`, and `getGroup`; C++ ports these paths synchronously, changing await/timing behavior.

- [ ] 99. [WMOLegacyLoader.cpp] Group-loader initialization differs from JS in `getGroup`
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 146–149
- **Status**: Pending
- **Details**: JS creates group loaders with `fileID` undefined and explicitly seeds `group.version = this.version` before `await group.load()`. C++ does not pre-seed `version`, changing legacy group parse assumptions.

- [ ] 100. [WMOLoader.cpp] `load`/`getGroup` APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 37, 64
- **Status**: Pending
- **Details**: JS exposes async `load()` and `getGroup(index)` while C++ ports both as synchronous methods, changing await/timing behavior.

- [ ] 101. [WMOLoader.cpp] `getGroup` omits JS filename-based fallback when `groupIDs` are missing
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 75–79
- **Status**: Pending
- **Details**: JS loads by `groupIDs[index]` when present, otherwise falls back to `getFileByName(this.fileName.replace(...))`; C++ hard-requires `groupIDs` and throws out-of-range instead of performing the filename fallback.

- [ ] 102. [CharMaterialRenderer.cpp] Core renderer methods are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 49, 105, 114, 170, 189, 231, 282
- **Status**: Pending
- **Details**: JS defines `init`, `reset`, `setTextureTarget`, `loadTexture`, `loadTextureFromBLP`, `compileShaders`, and `update` as async/await flows. C++ ports these methods synchronously, changing timing/error-propagation behavior expected by async call sites.

- [ ] 103. [M2LegacyRendererGL.cpp] Loader/skin/animation entrypoints are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 183, 210, 252, 294, 455
- **Status**: Pending
- **Details**: JS exposes async `load`, `_load_textures`, `applyCreatureSkin`, `loadSkin`, and `playAnimation`; C++ ports these execution paths as synchronous methods, altering await behavior and scheduling.

- [ ] 104. [M2RendererGL.cpp] Multiple texture/skeleton/animation methods are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 362, 401, 431, 587, 663, 1336, 1371, 1399, 1424
- **Status**: Pending
- **Details**: JS keeps `load`, `_load_textures`, `loadSkin`, `_create_skeleton`, `playAnimation`, `overrideTextureType*`, and `applyReplaceableTextures` asynchronous; C++ ports them synchronously, changing promise timing and exception propagation behavior.

- [ ] 105. [M2RendererGL.cpp] Shader time uniform start-point differs from JS `performance.now()` baseline
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` line 1224
- **Status**: Pending
- **Details**: JS feeds `u_time` from `performance.now() * 0.001` (seconds since page load). C++ computes time from a static timestamp initialized on first render call, shifting animation phase baseline relative to JS.

- [ ] 106. [M3RendererGL.cpp] Load APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 56, 76
- **Status**: Pending
- **Details**: JS defines async `load` and `loadLOD`; C++ ports both as synchronous calls, changing await/timing semantics.

- [ ] 107. [MDXRendererGL.cpp] Load and texture/animation paths are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 174, 200, 407
- **Status**: Pending
- **Details**: JS uses async `load`, `_load_textures`, and `playAnimation`; C++ ports these paths synchronously, changing asynchronous control flow and failure timing.

- [ ] 108. [MDXRendererGL.cpp] Skeleton node flattening changes JS undefined/NaN behavior for `objectId`
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 256–264
- **Status**: Pending
- **Details**: JS compares raw `nodes[i].objectId` and can propagate undefined/NaN semantics. C++ uses `std::optional<int>` checks and skips undefined IDs, which changes edge-case matrix-index behavior from JS.

- [ ] 109. [WMOLegacyRendererGL.cpp] Load/update-set paths are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 77, 104, 168, 270, 353
- **Status**: Pending
- **Details**: JS exposes async `load`, `_load_textures`, `_load_groups`, `loadDoodadSet`, and `updateSets`; C++ ports these paths as synchronous methods, altering Promise scheduling and error propagation behavior.

- [ ] 110. [WMOLegacyRendererGL.cpp] Doodad-set iteration adds bounds guard not present in JS
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 287–289
- **Status**: Pending
- **Details**: JS directly accesses `wmo.doodads[firstIndex + i]` without a pre-check. C++ introduces explicit range guarding/continue behavior, changing edge-case handling when doodad counts/indices are inconsistent.

- [ ] 111. [WMOLegacyRendererGL.cpp] Vue watcher-based reactive updates are replaced with render-time polling
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 88–93, 519–521
- **Status**: Pending
- **Details**: JS wires `$watch` callbacks and unregisters them in `dispose`. C++ removes watcher registration and uses per-frame state polling, which changes update trigger timing and reactivity semantics.

- [ ] 112. [WMORendererGL.cpp] Load/update-set paths are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 81, 119, 206, 353, 434
- **Status**: Pending
- **Details**: JS defines async `load`, `_load_textures`, `_load_groups`, `loadDoodadSet`, and `updateSets`; C++ ports these methods synchronously, changing await/timing behavior.

- [ ] 113. [WMORendererGL.cpp] Reactive view binding/watcher lifecycle differs from JS
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 101–107, 637–639
- **Status**: Pending
- **Details**: JS stores `groupArray`/`setArray` by reference in `core.view` and updates via Vue `$watch` callbacks with explicit unregister in `dispose`. C++ copies arrays into view state and replaces watcher callbacks with polling logic, changing reactivity/update timing semantics.

- [ ] 114. [CSVWriter.cpp] `.cpp`/`.js` sibling contents are swapped, leaving `.cpp` as unconverted JavaScript
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 1–86
- **Status**: Pending
- **Details**: `CSVWriter.cpp` currently contains JavaScript (`require`, `class`, `module.exports`) while `CSVWriter.js` contains C++ (`#include`, `CSVWriter::...`). This violates expected source pairing and leaves the `.cpp` translation unit unconverted.

- [ ] 115. [GLTFWriter.cpp] Export entrypoint is synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 194–1504
- **Status**: Pending
- **Details**: JS defines `async write(overwrite, format)` and awaits filesystem/export operations throughout. C++ exposes `void write(...)` and executes all I/O synchronously, changing call timing/error propagation semantics for callers expecting Promise behavior.

- [ ] 116. [JSONWriter.cpp] `write()` is synchronous and BigInt-stringify behavior differs from JS
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 33–43
- **Status**: Pending
- **Details**: JS uses `async write()` and a `JSON.stringify` replacer that converts `bigint` values to strings. C++ `write()` is synchronous and writes `nlohmann::json::dump()` directly, which changes both async semantics and JS BigInt serialization parity.

- [ ] 117. [MTLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 41–68
- **Status**: Pending
- **Details**: JS awaits file existence checks, directory creation, and line writes in `async write()`. C++ performs the same work synchronously, so behavior differs for call sites that rely on async completion semantics.

- [ ] 118. [OBJWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 129–225
- **Status**: Pending
- **Details**: JS implements asynchronous writes (`await writer.writeLine(...)` and async filesystem calls). C++ `write()` is synchronous, which changes ordering and error propagation relative to the original Promise API.

- [ ] 119. [SQLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 210–229
- **Status**: Pending
- **Details**: JS `async write()` awaits file checks, directory creation, and output writes. C++ performs the same operations synchronously, diverging from JS caller-visible async behavior.

- [ ] 120. [SQLWriter.cpp] Empty-string SQL value handling differs from JS null/undefined checks
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 66–76
- **Status**: Pending
- **Details**: JS returns `NULL` only for `null`/`undefined`; an empty string serializes to `''`. C++ maps `value.empty()` to `NULL`, so genuine empty-string field values are emitted as SQL `NULL`, changing exported data.

- [ ] 121. [STLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/STLWriter.js` lines 131–249
- **Status**: Pending
- **Details**: JS writer path is asynchronous and awaited by callers. C++ `write()` runs synchronously, changing API timing semantics compared to the original implementation.

- [ ] 122. [blp.cpp] Canvas rendering APIs (`toCanvas`/`drawToCanvas`) are not ported
- **JS Source**: `src/js/casc/blp.js` lines 95, 103–117, 221–234
- **Status**: Pending
- **Details**: JS exposes canvas-based rendering and uses `toCanvas(...).toDataURL()` in `getDataURL()`. C++ removes canvas APIs and routes `getDataURL()` through PNG encoding, which changes available surface API and rendering path behavior.

- [ ] 123. [blp.cpp] WebP/PNG save methods are synchronous instead of JS async Promise APIs
- **JS Source**: `src/js/casc/blp.js` lines 146–194
- **Status**: Pending
- **Details**: JS implements `async saveToPNG`, `async toWebP`, and `async saveToWebP`. C++ equivalents are synchronous, changing completion/error semantics for consumers expecting Promise-based behavior.

- [ ] 124. [blp.cpp] 4-bit alpha nibble indexing behavior differs from original JS
- **JS Source**: `src/js/casc/blp.js` lines 286–299
- **Status**: Pending
- **Details**: JS uses `this.rawData[this.scaledLength + (index / 2)]` (floating index for odd values), while C++ uses integer division. This intentionally fixes a JS bug but still deviates from original runtime behavior.

- [ ] 125. [blp.cpp] DXT block overrun guard differs from JS equality check
- **JS Source**: `src/js/casc/blp.js` lines 323–324
- **Status**: Pending
- **Details**: JS skips only when `this.rawData.length === pos`. C++ skips when `pos >= rawData_.size()`, adding defensive handling for overrun states and altering edge-case decode behavior.

- [ ] 126. [blp.cpp] `toBuffer()` fallback differs for unknown encodings
- **JS Source**: `src/js/casc/blp.js` lines 242–250
- **Status**: Pending
- **Details**: JS has no default branch and therefore returns `undefined` for unsupported encodings. C++ returns an empty `BufferWrapper`, changing caller-observed fallback behavior.

- [ ] 127. [blte-reader.cpp] `decodeAudio(context)` API from JS is missing
- **JS Source**: `src/js/casc/blte-reader.js` lines 337–340
- **Status**: Pending
- **Details**: JS exposes `async decodeAudio(context)` after block processing. C++ removes this method entirely, so the sibling port is missing a public API/code path present in the original module.

- [ ] 128. [blte-reader.cpp] `getDataURL()` no longer honors pre-populated `dataURL` short-circuit
- **JS Source**: `src/js/casc/blte-reader.js` lines 346–353
- **Status**: Pending
- **Details**: JS returns existing `this.dataURL` without forcing `processAllBlocks()`. C++ always processes blocks before delegating to `BufferWrapper::getDataURL()`, changing caching/override behavior.

- [ ] 129. [blte-stream-reader.cpp] Block retrieval/decode flow is synchronous instead of JS async
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 54–118
- **Status**: Pending
- **Details**: JS defines `async getBlock` and `async _decodeBlock` and awaits async `blockFetcher`. C++ changes these paths to synchronous calls, altering control flow and error timing.

- [ ] 130. [blte-stream-reader.cpp] `createReadableStream()` Web Streams API path is missing
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 168–193
- **Status**: Pending
- **Details**: JS provides `createReadableStream()` for progressive consumption and cancellation behavior. C++ has no equivalent method, leaving the stream-based event handler/code path unported.

- [ ] 131. [blte-stream-reader.cpp] `streamBlocks` and `createBlobURL` behavior differs from JS
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 199–218
- **Status**: Pending
- **Details**: JS uses an async generator for `streamBlocks()` and returns an object URL from `createBlobURL()` via `BlobPolyfill/URLPolyfill`. C++ uses eager callback iteration and returns concatenated raw bytes (`BufferWrapper`) instead of a blob URL string.

- [ ] 132. [build-cache.cpp] Build cache APIs are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/casc/build-cache.js` lines 49–152, 174–257
- **Status**: Pending
- **Details**: JS uses async methods (`init/getFile/storeFile/saveCacheIntegrity/saveManifest`) and async event handlers with awaited I/O; C++ runs equivalent flows synchronously, changing timing/error propagation behavior.

- [ ] 133. [build-cache.cpp] Cache cleanup size subtraction behavior differs from JS
- **JS Source**: `src/js/casc/build-cache.js` lines 247–254
- **Status**: Pending
- **Details**: JS always performs `deleteSize -= manifestSize` (can go negative with Number); C++ adds an unsigned underflow guard before subtraction, changing edge-case cache-size accounting semantics.

- [ ] 134. [casc-source-local.cpp] Local CASC public/file-loading methods are synchronous instead of JS async methods
- **JS Source**: `src/js/casc/casc-source-local.js` lines 42–517
- **Status**: Pending
- **Details**: JS methods (`init/getFile/getFileStream/load/loadConfigs/loadIndexes/parseIndex/loadEncoding/loadRoot/initializeRemoteCASC/getDataFileWithRemoteFallback/getDataFile/_ensureFileInCache/getFileEncodingInfo`) are Promise-based; C++ equivalents are synchronous.

- [ ] 135. [casc-source-local.cpp] Remote CASC initialization region fallback differs from JS behavior
- **JS Source**: `src/js/casc/casc-source-local.js` lines 324–332
- **Status**: Pending
- **Details**: JS directly constructs `new CASCRemote(core.view.selectedCDNRegion.tag)`; C++ silently falls back to `constants::PATCH::DEFAULT_REGION` when `selectedCDNRegion.tag` is missing, altering failure/selection behavior.

- [ ] 136. [casc-source-remote.cpp] Remote CASC lifecycle and data-access methods are synchronous instead of JS async methods
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 37–556
- **Status**: Pending
- **Details**: JS uses async Promise-based control flow for initialization/config/archive loading and file access (`init/getVersionConfig/getConfig/getCDNConfig/getFile/getFileStream/preload/load/loadEncoding/loadRoot/loadArchives/loadServerConfig/parseArchiveIndex/getDataFile/getDataFilePartial/loadConfigs/resolveCDNHost/_ensureFileInCache/getFileEncodingInfo`); C++ is synchronous.

- [ ] 137. [casc-source-remote.cpp] HTTP error detail from remote config requests differs from JS
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 75–79, 98–102, 121
- **Status**: Pending
- **Details**: JS includes HTTP status codes in thrown messages for `getConfig/getCDNConfig`; C++ checks only for empty `generics::get()` payload and throws generic HTTP error strings without the JS status payload.

- [ ] 138. [casc-source.cpp] `getFileByName` no longer forwards to subclass file-reader path like JS
- **JS Source**: `src/js/casc/casc-source.js` lines 169–191
- **Status**: Pending
- **Details**: JS `getFileByName(...)` forwards all read flags to polymorphic `this.getFile(...)` on subclass implementations; C++ `CASC::getFileByName(...)` returns only the encoding key from base `getFile`, changing behavior and API contract.

- [ ] 139. [casc-source.cpp] Base CASC APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/casc/casc-source.js` lines 70–275, 312–444
- **Status**: Pending
- **Details**: JS methods (`getInstallManifest/getFile/getFileEncodingInfo/getFileByName/getVirtualFileByID/getVirtualFileByName/prepareListfile/prepareDBDManifest/loadListfile/parseRootFile/parseEncodingFile`) are Promise-based; C++ equivalents are synchronous.

- [ ] 140. [cdn-resolver.cpp] Resolver API and internal host-resolution flow are synchronous instead of JS async Promise flow
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 43–116, 143–217
- **Status**: Pending
- **Details**: JS `getBestHost/getRankedHosts/_resolveRegionProduct/_resolveHosts` are async and await Promise pipelines; C++ resolves through blocking waits/futures and synchronous return APIs.

- [ ] 141. [content-flags.cpp] Sibling `.cpp` translation unit does not contain line-by-line ported JS constant exports
- **JS Source**: `src/js/casc/content-flags.js` lines 4–15
- **Status**: Pending
- **Details**: JS sibling exports all content-flag constants in the module body, while `content-flags.cpp` only includes the header and comments; parity exists only via header constants, not in the sibling `.cpp` implementation.

- [ ] 142. [db2.cpp] `getRelationRows` preload-guard error semantics differ from JS proxy behavior
- **JS Source**: `src/js/casc/db2.js` lines 45–53
- **Status**: Pending
- **Details**: JS proxy throws explicit errors if `getRelationRows` is called before parse/preload; C++ delegates to `WDCReader::getRelationRows()` behavior (commented as returning empty), so the JS error path is not preserved.

- [ ] 143. [db2.cpp] JS async proxy call model is replaced with synchronous parse-once wrappers
- **JS Source**: `src/js/casc/db2.js` lines 58–67, 75–92
- **Status**: Pending
- **Details**: JS wraps reader methods in async proxy handlers and memoized parse promises; C++ uses synchronous `std::call_once` parse guards with direct `WDCReader&` access, changing call timing and API behavior.

- [ ] 144. [dbd-manifest.cpp] JS truthiness filter for manifest entries is not preserved for object/array values
- **JS Source**: `src/js/casc/dbd-manifest.js` lines 30–34
- **Status**: Pending
- **Details**: JS `if (entry.tableName && entry.db2FileDataID)` treats objects/arrays as truthy; C++ truthiness logic only handles string/number/bool and treats other JSON value types as false, diverging from JS semantics.

- [ ] 145. [export-helper.cpp] `getIncrementalFilename` is synchronous instead of JS async Promise API
- **JS Source**: `src/js/casc/export-helper.js` lines 97–114
- **Status**: Pending
- **Details**: JS exposes `static async getIncrementalFilename(...)` and awaits `generics.fileExists`; C++ implementation is synchronous, changing timing/error behavior expected by Promise-style callers.

- [ ] 146. [export-helper.cpp] Export failure stack-trace output target differs from JS
- **JS Source**: `src/js/casc/export-helper.js` lines 284–288
- **Status**: Pending
- **Details**: JS writes stack traces with `console.log(stackTrace)` in `mark(...)`; C++ routes stack trace strings through `logging::write(...)`, changing where detailed error output appears.

- [ ] 147. [listfile.cpp] Public listfile APIs are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/casc/listfile.js` lines 478–500, 603–620, 710–756
- **Status**: Pending
- **Details**: JS exposes async `preload`, `prepareListfile`, `loadUnknownTextures`, `loadUnknownModels`, `loadUnknowns`, and `renderListfile`; C++ ports these as synchronous/blocking methods.

- [ ] 148. [listfile.cpp] Shared preload promise semantics differ from JS
- **JS Source**: `src/js/casc/listfile.js` lines 478–500
- **Status**: Pending
- **Details**: JS stores and returns `preload_promise` so all callers can await the same Promise result; C++ uses `void` entrypoints with internal future state and does not expose equivalent awaitable API behavior.

- [ ] 149. [listfile.cpp] `applyPreload` return contract differs from JS
- **JS Source**: `src/js/casc/listfile.js` lines 528–532, 591–601
- **Status**: Pending
- **Details**: JS returns `0` in fallback/no-match paths and otherwise returns `undefined`; C++ changes this API to `void`, removing JS return-value semantics.

- [ ] 150. [listfile.cpp] `getByID` not-found sentinel differs from JS
- **JS Source**: `src/js/casc/listfile.js` lines 778–794
- **Status**: Pending
- **Details**: JS returns `undefined` when ID lookup fails; C++ returns empty string, changing not-found representation and call-site semantics.

- [ ] 151. [listfile.cpp] `getFilteredEntries` search contract differs from JS
- **JS Source**: `src/js/casc/listfile.js` lines 832–857
- **Status**: Pending
- **Details**: JS auto-detects regex via `search instanceof RegExp` and propagates regex errors; C++ requires explicit `is_regex` flag and swallows invalid regex by returning empty results.

- [ ] 152. [listfile.cpp] Binary preload list ordering can differ from JS Map insertion order
- **JS Source**: `src/js/casc/listfile.js` lines 563–588
- **Status**: Pending
- **Details**: JS iterates `Map` keys in insertion order when building filtered preloaded lists; C++ iterates `std::unordered_map` in non-deterministic hash order, which can reorder displayed list entries.

- [ ] 153. [locale-flags.cpp] Sibling `.cpp` translation unit does not contain line-by-line JS exports
- **JS Source**: `src/js/casc/locale-flags.js` lines 4–40
- **Status**: Pending
- **Details**: JS sibling exports `flags` and `names` objects in module body, while `locale-flags.cpp` only includes the header and comments; parity lives in header constants rather than `.cpp` implementation.

- [ ] 154. [realmlist.cpp] `load` API is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/casc/realmlist.js` lines 36–65
- **Status**: Pending
- **Details**: JS exposes `async load()` with awaited cache/network I/O; C++ ports `load()` as synchronous blocking flow, changing timing/error propagation semantics.

- [ ] 155. [realmlist.cpp] `realmListURL` coercion semantics differ from JS `String(...)` behavior
- **JS Source**: `src/js/casc/realmlist.js` lines 39–42
- **Status**: Pending
- **Details**: JS converts any value with `String(core.view.config.realmListURL)` (including `undefined`), while C++ treats missing/null as empty and throws `Missing/malformed realmListURL`, changing edge-case behavior.

- [ ] 156. [realmlist.cpp] Remote non-OK handling/logging path differs from JS response contract
- **JS Source**: `src/js/casc/realmlist.js` lines 51–63
- **Status**: Pending
- **Details**: JS branches on `res.ok` and logs `Failed to retrieve ... (${res.status})` for non-OK responses; C++ uses byte-returning `generics::get()` and exception-based failure handling, removing explicit JS `res.ok/res.status` behavior.

- [ ] 157. [tact-keys.cpp] Tact key lifecycle APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/casc/tact-keys.js` lines 65–137
- **Status**: Pending
- **Details**: JS `load`, `save`, and `doSave` are Promise-based/async; C++ ports to synchronous methods and immediate file I/O.

- [ ] 158. [tact-keys.cpp] Save scheduling differs from JS `setImmediate` batching behavior
- **JS Source**: `src/js/casc/tact-keys.js` lines 122–135
- **Status**: Pending
- **Details**: JS coalesces multiple save requests into a next-tick `setImmediate(doSave)` write; C++ runs `doSave()` immediately, changing batching/timing semantics.

- [ ] 159. [tact-keys.cpp] Remote update error contract differs from JS HTTP status error path
- **JS Source**: `src/js/casc/tact-keys.js` lines 89–93
- **Status**: Pending
- **Details**: JS throws `Unable to update tactKeys, HTTP ${res.status}` when response is non-OK; C++ throws a generic `Unable to update tactKeys: ...` message from caught exceptions without preserving JS status-based error contract.

- [ ] 160. [vp9-avi-demuxer.cpp] Parsing/extraction flow is synchronous callback-based instead of JS async APIs
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js` lines 22–23, 83–126
- **Status**: Pending
- **Details**: JS exposes `async parse_header()` and `async* extract_frames()` generator semantics; C++ ports these to synchronous methods with callback iteration, changing consumption and scheduling behavior.

- [ ] 161. [checkboxlist.cpp] Component lifecycle/event model differs from JS mounted/unmount listener flow
- **JS Source**: `src/js/components/checkboxlist.js` lines 28–51, 122–134
- **Status**: Pending
- **Details**: JS registers/removes document-level mouse listeners and a `ResizeObserver`; C++ emulates behavior via per-frame ImGui polling and internal state, not equivalent listener lifecycle semantics.

- [ ] 162. [checkboxlist.cpp] Scroll bound edge-case behavior differs for zero scrollbar range
- **JS Source**: `src/js/components/checkboxlist.js` lines 102–106
- **Status**: Pending
- **Details**: JS sets `scrollRel = this.scroll / max` (allowing `Infinity/NaN` when `max === 0`); C++ clamps to `0.0f` when range is zero, changing parity in that edge case.

- [ ] 163. [checkboxlist.cpp] Scrollbar height behavior differs from original CSS
- **JS Source**: `src/js/components/checkboxlist.js` lines 93–94; `src/app.css` lines 1097–1103
- **Status**: Pending
- **Details**: JS/CSS uses `.scroller` with fixed `height: 45px` and resize math based on that DOM height; C++ computes a dynamic proportional thumb height with `std::max(20.0f, ...)`, producing different visual size/scroll behavior.

- [ ] 164. [checkboxlist.cpp] Scrollbar default styling differs from CSS reference
- **JS Source**: `src/app.css` lines 1106–1114, 1116–1117
- **Status**: Pending
- **Details**: CSS default scrollbar inner color/border uses `var(--border)` and hover uses `var(--font-highlight)`; C++ uses `FONT_PRIMARY` for default thumb color, causing visual mismatch against reference styling.


- [ ] 165. [combobox.cpp] Blur-close timing is frame-based instead of JS 200ms timeout
- **JS Source**: `src/js/components/combobox.js` lines 67–72
- **Status**: Pending
- **Details**: JS uses `setTimeout(..., 200)` for blur-close timing, but C++ uses a fixed 12-frame countdown. The effective delay changes with frame rate, so dropdown close behavior differs from JS.

- [ ] 166. [combobox.cpp] Dropdown menu is rendered in normal layout flow instead of absolute popup overlay
- **JS Source**: `src/js/components/combobox.js` lines 87–93
- **Status**: Pending
- **Details**: JS renders `<ul>` as an absolutely positioned popup under the input. C++ renders the dropdown as an ImGui child region in normal flow, which can alter layout/overlap behavior and visual parity.

- [x] 167. [context-menu.cpp] Invisible hover buffer zone (`.context-menu-zone`) is not ported
- **JS Source**: `src/js/components/context-menu.js` lines 54–56
- **Status**: Verified
- **Details**: JS includes a dedicated `.context-menu-zone` element to extend hover bounds around the menu. C++ explicitly omits it, which can change close-on-mouseleave behavior near menu edges.

- [ ] 168. [data-table.cpp] Filter icon click no longer focuses the data-table filter input
- **JS Source**: `src/js/components/data-table.js` lines 742–760
- **Status**: Pending
- **Details**: JS appends `column:` filter text and then focuses `#data-table-filter-input` with cursor placement. C++ only emits the new filter string and leaves focus behavior unimplemented.

- [ ] 169. [data-table.cpp] Empty-string numeric sorting semantics differ from JS `Number(...)`
- **JS Source**: `src/js/components/data-table.js` lines 179–193
- **Status**: Pending
- **Details**: JS treats `''` as numeric (`Number('') === 0`) during sort. C++ `tryParseNumber` rejects empty strings, so those values sort as text instead of numeric values.

- [ ] 170. [data-table.cpp] Header sort/filter icons are custom-drawn instead of CSS/SVG assets
- **JS Source**: `src/js/components/data-table.js` lines 988–1003
- **Status**: Pending
- **Details**: JS uses CSS icon classes backed by `fa-icons` images for exact visuals. C++ draws procedural triangles/lines, producing non-identical icon appearance versus the JS UI.

- [ ] 171. [data-table.cpp] Native scroll-to-custom-scroll sync path from JS is missing
- **JS Source**: `src/js/components/data-table.js` lines 51–57, 513–527
- **Status**: Pending
- **Details**: JS listens for native root scroll events and synchronizes custom scrollbar state. C++ omits this path entirely, so behavior differs from the original scroll synchronization model.

- [ ] 172. [file-field.cpp] Directory dialog trigger moved from input focus to separate browse button
- **JS Source**: `src/js/components/file-field.js` lines 34–40, 46
- **Status**: Pending
- **Details**: JS opens the directory picker when the text field receives focus. C++ opens the dialog only from a dedicated `...` button, changing interaction flow and UI behavior.

- [ ] 173. [file-field.cpp] Same-directory reselection behavior differs from JS file input reset logic
- **JS Source**: `src/js/components/file-field.js` lines 35–38
- **Status**: Pending
- **Details**: JS clears the hidden file input value before click so selecting the same directory re-triggers change emission. C++ dialog path does not mirror this reset contract.

- [ ] 174. [home-showcase.cpp] Showcase card/video/background layer rendering is not ported
- **JS Source**: `src/js/components/home-showcase.js` lines 15–29, 32–42, 55–57
- **Status**: Pending
- **Details**: JS builds a layered CSS showcase card with optional autoplay video and title overlay. C++ replaces this with plain text/buttons and no equivalent visual composition.

- [ ] 175. [home-showcase.cpp] `background_style` computed output is missing from C++ render path
- **JS Source**: `src/js/components/home-showcase.js` lines 15–29, 55–57
- **Status**: Pending
- **Details**: JS computes `backgroundImage/backgroundSize/backgroundPosition/backgroundRepeat` from base + showcase layers. C++ does not apply these style layers, so showcase visuals diverge.

- [ ] 176. [home-showcase.cpp] Feedback action wiring differs from JS `data-kb-link` behavior
- **JS Source**: `src/js/components/home-showcase.js` lines 39–41
- **Status**: Pending
- **Details**: JS emits a KB-link anchor (`data-kb-link="KB011"`) handled by the app link system. C++ directly calls `ExternalLinks::open("KB011")`, which is not the same contract/path as the original markup-driven behavior.

- [ ] 177. [itemlistbox.cpp] Selection model changed from item-object references to item ID integers
- **JS Source**: `src/js/components/itemlistbox.js` lines 117–129, 271–315
- **Status**: Pending
- **Details**: JS selection stores full item objects and compares by object identity (`includes/indexOf(item)`). C++ stores numeric IDs, changing selection semantics and update payload shape.

- [ ] 178. [itemlistbox.cpp] Item action controls are rendered as ImGui buttons instead of list item links
- **JS Source**: `src/js/components/itemlistbox.js` lines 336–339
- **Status**: Pending
- **Details**: JS renders actions as `<ul class="item-buttons"><li>...</li></ul>` with CSS styling. C++ uses `SmallButton` widgets, producing different visual and interaction behavior.

- [ ] 179. [listbox.cpp] Keep-alive lifecycle listener behavior (`activated`/`deactivated`) is missing
- **JS Source**: `src/js/components/listbox.js` lines 97–113
- **Status**: Pending
- **Details**: JS conditionally registers/unregisters paste and keydown listeners on keep-alive activation state. C++ has no equivalent lifecycle gating, so keyboard/paste handling differs when component activation changes.

- [ ] 180. [listbox.cpp] Context menu emit payload omits original JS mouse event object
- **JS Source**: `src/js/components/listbox.js` lines 493–497
- **Status**: Pending
- **Details**: JS emits `{ item, selection, event }` including the full event object. C++ emits only simplified coordinates/fields, which drops event data expected by the original contract.

- [ ] 181. [listbox.cpp] Multi-subfield span structure from `item.split('\31')` is flattened
- **JS Source**: `src/js/components/listbox.js` lines 506–508
- **Status**: Pending
- **Details**: JS renders each subfield in separate `<span class="sub sub-N">` elements. C++ concatenates subfields into one display string, removing per-subfield structure and styling parity.

- [ ] 182. [listboxb.cpp] Selection payload changed from item values to row indices
- **JS Source**: `src/js/components/listboxb.js` lines 226–273
- **Status**: Pending
- **Details**: JS selection stores selected item values directly. C++ stores selected indices (`std::vector<int>`), which changes emitted selection data and behavior when item ordering changes.

- [ ] 183. [listboxb.cpp] Selection highlighting logic uses index identity instead of value identity
- **JS Source**: `src/js/components/listboxb.js` lines 281–283
- **Status**: Pending
- **Details**: JS checks `selection.includes(item)` by item value/object. C++ checks index membership, so highlight/selection parity diverges when values repeat or list contents are reordered.

- [ ] 184. [map-viewer.cpp] Tile image drawing path is still unimplemented
- **JS Source**: `src/js/components/map-viewer.js` lines 380–402, 1111–1113
- **Status**: Pending
- **Details**: JS draws loaded tiles to canvas via `putImageData(...)` on main/double-buffer contexts. C++ caches tile pixels but does not upload/draw them, so only overlays render and map tiles are not visually equivalent.

- [ ] 185. [map-viewer.cpp] Tile loading flow is synchronous instead of JS Promise-based async queueing
- **JS Source**: `src/js/components/map-viewer.js` lines 192–197, 380–414
- **Status**: Pending
- **Details**: JS tile loader is async (`loader(...).then(...)`) with Promise completion timing. C++ calls loader synchronously in `loadTile(...)`, changing queue timing and behavior during panning/zoom updates.

- [ ] 186. [markdown-content.cpp] Inline image markdown is not rendered as images
- **JS Source**: `src/js/components/markdown-content.js` lines 208–216, 251–253
- **Status**: Pending
- **Details**: JS converts `![alt](src)` to `<img ...>` in `v-html` output. C++ renders image segments as placeholder text (`[Image: ...]`), causing functional and visual mismatch.

- [ ] 187. [markdown-content.cpp] Inline bold/italic formatting behavior differs from JS HTML rendering
- **JS Source**: `src/js/components/markdown-content.js` lines 219–224
- **Status**: Pending
- **Details**: JS emits `<strong>`/`<em>` markup; C++ substitutes color-only text rendering because ImGui has no inline bold/italic in this path, so typography does not match original styling.

- [ ] 188. [menu-button.cpp] Click emit payload drops the original event object
- **JS Source**: `src/js/components/menu-button.js` lines 45–50
- **Status**: Pending
- **Details**: JS emits `click` with the DOM event argument (`this.$emit('click', e)`). C++ exposes `onClick()` with no event payload, changing callback contract.

- [ ] 189. [menu-button.cpp] Context-menu close behavior differs from original component flow
- **JS Source**: `src/js/components/menu-button.js` lines 75–80; `src/js/components/context-menu.js` line 54
- **Status**: Pending
- **Details**: JS menu closes via context-menu `@close` events (mouseleave/click behavior). C++ popup primarily closes on click-outside checks and does not mirror the same close trigger semantics.

- [ ] 190. [model-viewer-gl.cpp] Character-mode reactive watchers are replaced with render-time polling
- **JS Source**: `src/js/components/model-viewer-gl.js` lines 469–473
- **Status**: Pending
- **Details**: JS registers Vue `$watch` handlers for `chrUse3DCamera`, `chrRenderShadow`, and `chrModelLoading`. C++ polls these values each frame in `renderWidget`, changing update/lifecycle behavior.

- [ ] 191. [model-viewer-gl.cpp] Active renderer contract is narrowed from JS duck-typed renderer to `M2RendererGL`
- **JS Source**: `src/js/components/model-viewer-gl.js` lines 223–226, 304–307, 409–416
- **Status**: Pending
- **Details**: JS uses optional/duck-typed renderer checks (`getActiveRenderer?.()` + method checks). C++ hard-types active renderer as `M2RendererGL*`, reducing parity with the original renderer-agnostic method contract.

- [ ] 192. [resize-layer.cpp] ResizeObserver lifecycle is replaced by per-frame width polling
- **JS Source**: `src/js/components/resize-layer.js` lines 12–15, 21–23
- **Status**: Pending
- **Details**: JS emits resize through `ResizeObserver` mount/unmount lifecycle. C++ emits when measured width changes during render, so behavior is tied to render frames instead of observer callbacks.

- [ ] 193. [slider.cpp] Document-level mouse listener lifecycle from JS is not ported directly
- **JS Source**: `src/js/components/slider.js` lines 23–29, 35–38
- **Status**: Pending
- **Details**: JS installs/removes global `mousemove`/`mouseup` listeners in `mounted`/`beforeUnmount`. C++ handles drag state via ImGui per-frame input polling and has no equivalent listener registration lifecycle.

- [ ] 194. [CompressionType.cpp] Sibling `.cpp` translation unit does not contain line-by-line JS exports
- **JS Source**: `src/js/db/CompressionType.js` lines 1–8
- **Status**: Pending
- **Details**: JS sibling exports compression constants in module body, while `CompressionType.cpp` only includes the header and comments; implementation parity is header-only, not in the `.cpp` sibling.

- [ ] 195. [DBCReader.cpp] Public load path is synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/db/DBCReader.js` lines 162–209, 244–279
- **Status**: Pending
- **Details**: JS `loadSchema()`/`parse()` are async and awaited. C++ ports these flows synchronously, changing timing and error propagation semantics.

- [ ] 196. [DBCReader.cpp] DBD cache backend behavior differs from JS CASC cache API
- **JS Source**: `src/js/db/DBCReader.js` lines 177–195
- **Status**: Pending
- **Details**: JS reads/writes DBD cache via `core.view.casc?.cache.getFile/storeFile`. C++ uses direct filesystem cache paths and bypasses the JS cache interface behavior.

- [ ] 197. [DBDParser.cpp] Empty-chunk parsing behavior differs from original JS control flow
- **JS Source**: `src/js/db/DBDParser.js` lines 215–223, 237–320
- **Status**: Pending
- **Details**: JS `parseChunk` path can process empty chunks produced by blank-line splits, while C++ returns early on `chunk.empty()`, changing edge-case entry creation behavior.

- [ ] 198. [FieldType.cpp] Sibling `.cpp` translation unit does not contain line-by-line JS exports
- **JS Source**: `src/js/db/FieldType.js` lines 1–14
- **Status**: Pending
- **Details**: JS sibling exports field-type symbols in module body, while `FieldType.cpp` only includes the header and comments; parity is provided in header enums, not in `.cpp` implementation.

- [ ] 199. [WDCReader.cpp] Public schema/data loading APIs are synchronous instead of JS async Promise flow
- **JS Source**: `src/js/db/WDCReader.js` lines 240–277, 303–564
- **Status**: Pending
- **Details**: JS `loadSchema(...)` and `parse()` are async and await cache/network/CASC operations; C++ ports both as blocking synchronous methods, changing timing and error propagation behavior.

- [ ] 200. [WDCReader.cpp] DBD cache access path bypasses JS CASC cache API contract
- **JS Source**: `src/js/db/WDCReader.js` lines 251–266
- **Status**: Pending
- **Details**: JS uses `casc.cache.getFile/storeFile` for DBD cache reads/writes; C++ directly reads/writes filesystem paths under `constants::CACHE::DIR_DBD()`, changing cache backend behavior and integration points.

- [ ] 201. [WDCReader.cpp] Row collection ordering/identity semantics differ from JS Map behavior
- **JS Source**: `src/js/db/WDCReader.js` lines 149–193, 200–208
- **Status**: Pending
- **Details**: JS stores rows in `Map` preserving insertion order and returns the same cached map object after `preload()`. C++ uses `std::map` (key-sorted order) and returns map copies, changing iteration order and object identity/mutability semantics.

- [ ] 202. [WDCReader.cpp] Numeric input coercion from JS `parseInt(...)` is not preserved
- **JS Source**: `src/js/db/WDCReader.js` lines 119–120, 220
- **Status**: Pending
- **Details**: JS coerces `recordID` and `foreignKeyValue` with `parseInt(...)` in `getRow`/`getRelationRows`; C++ requires already-typed integers, so string/loose inputs no longer follow JS coercion behavior.

- [ ] 203. [DBCharacterCustomization.cpp] Initialization flow is synchronous and drops JS shared-promise waiting behavior
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 37–46
- **Status**: Pending
- **Details**: JS `ensureInitialized` awaits a shared `init_promise` so concurrent callers wait for completion; C++ uses `is_initializing` early-return logic and can return before initialization completes.

- [ ] 204. [DBCharacterCustomization.cpp] Getter not-found contracts differ from JS `Map.get(...)` undefined behavior
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 212–253
- **Status**: Pending
- **Details**: JS getters return `undefined` when keys are missing; C++ ports several getters to `0`, `nullptr`, or `std::nullopt`, changing not-found sentinel behavior expected by JS-style callers.

- [ ] 205. [DBComponentModelFileData.cpp] Initialization API is synchronous and does not preserve JS promise-sharing semantics
- **JS Source**: `src/js/db/caches/DBComponentModelFileData.js` lines 18–43
- **Status**: Pending
- **Details**: JS `initialize()` is async and returns shared `init_promise` while loading; C++ initialization is synchronous with `is_initializing` early return, so concurrent calls can return before data is ready.

- [ ] 206. [DBComponentTextureFileData.cpp] Initialization API is synchronous and does not preserve JS promise-sharing semantics
- **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js` lines 17–41
- **Status**: Pending
- **Details**: JS `initialize()` is async and returns shared `init_promise`; C++ uses synchronous loading with boolean reentry guards, changing completion/wait behavior for concurrent callers.

- [ ] 207. [DBCreatureDisplayExtra.cpp] Initialization flow is synchronous and does not mirror JS awaited init promise
- **JS Source**: `src/js/db/caches/DBCreatureDisplayExtra.js` lines 15–24, 26–53
- **Status**: Pending
- **Details**: JS `ensureInitialized()` awaits `_initialize()` via `init_promise`; C++ makes `_initialize()` synchronous with `is_initializing` early return, so callers may proceed without JS-equivalent awaited completion semantics.

- [ ] 208. [DBCreatureList.cpp] Public load API is synchronous instead of JS Promise-based async loading
- **JS Source**: `src/js/db/caches/DBCreatureList.js` lines 12–43
- **Status**: Pending
- **Details**: JS `initialize_creature_list` is async and awaits DB2 row retrieval; C++ ports this path as synchronous, changing timing and error-propagation behavior.

- [ ] 209. [DBCreatures.cpp] `initializeCreatureData` is synchronous instead of JS async data-loading flow
- **JS Source**: `src/js/db/caches/DBCreatures.js` lines 17–73
- **Status**: Pending
- **Details**: JS implementation awaits multiple `getAllRows()` calls and exposes Promise timing; C++ performs blocking synchronous table loads, diverging from JS async behavior contract.

- [ ] 210. [DBCreaturesLegacy.cpp] Model path normalization misses JS `.mdl` to `.m2` conversion
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 45–47
- **Status**: Pending
- **Details**: JS normalizes both `.mdl` and `.mdx` model extensions to `.m2` during model map build; C++ `normalizePath` converts only `.mdx`, so `.mdl` model rows resolve differently.

- [ ] 211. [DBCreaturesLegacy.cpp] Legacy load API is synchronous instead of JS Promise-based async parse flow
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 19–112
- **Status**: Pending
- **Details**: JS uses async initialization and awaits DBC parsing operations; C++ performs synchronous parsing and loading, altering timing/error behavior relative to original Promise flow.

- [ ] 212. [DBCreaturesLegacy.cpp] Exception logging omits JS stack trace output behavior
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 108–111
- **Status**: Pending
- **Details**: JS logs both error message and stack (`log.write('%o', e.stack)`); C++ logs only `e.what()` and drops stack trace output.

- [ ] 213. [DBDecor.cpp] Decor cache initialization is synchronous instead of JS async table load
- **JS Source**: `src/js/db/caches/DBDecor.js` lines 15–40
- **Status**: Pending
- **Details**: JS `initializeDecorData` is async and awaits DB2 reads; C++ uses a synchronous blocking initializer, changing API timing behavior.

- [ ] 214. [DBDecorCategories.cpp] Category cache loading is synchronous and unordered-container iteration differs from JS Map/Set ordering
- **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 10–56
- **Status**: Pending
- **Details**: JS uses async initialization plus `Map`/`Set` insertion order iteration; C++ ports to synchronous initialization with `std::unordered_map`/`std::unordered_set`, which can change iteration ordering and timing semantics.

- [ ] 215. [DBGuildTabard.cpp] Sibling `.cpp` file is still unconverted JavaScript and appears swapped with `.js`
- **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 1–133
- **Status**: Pending
- **Details**: `DBGuildTabard.cpp` contains JS module code (`require`, `module.exports`, async functions), while the sibling `DBGuildTabard.js` contains C++ code, so the `.cpp` translation unit is not actually a C++ line-by-line port of the JS source.

- [ ] 216. [DBItemCharTextures.cpp] Initialization flow is synchronous and drops JS shared-promise semantics
- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 34–88
- **Status**: Pending
- **Details**: JS uses `init_promise` and async `initialize/ensureInitialized` so concurrent callers await the same in-flight work; C++ uses synchronous initialization with no promise-sharing behavior.

- [ ] 217. [DBItemCharTextures.cpp] Race/gender texture selection fallback differs from JS behavior
- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 129–135
- **Status**: Pending
- **Details**: JS pushes `bestFileDataID` as returned (can be `undefined` when no match), but C++ falls back to the first entry (`value_or((*file_data_ids)[0])`), changing file-data selection behavior.

- [ ] 218. [DBItemDisplays.cpp] Item display cache initialization is synchronous instead of JS async flow
- **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 18–53
- **Status**: Pending
- **Details**: JS `initializeItemDisplays` is Promise-based and awaits DB2/cache calls; C++ ports this path as synchronous blocking logic.

- [ ] 219. [DBItemGeosets.cpp] Initialization lifecycle is synchronous and omits JS `init_promise` contract
- **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 154–220
- **Status**: Pending
- **Details**: JS uses async initialization with `init_promise` deduplication; C++ uses a synchronous one-shot initializer and cannot preserve awaitable initialization semantics.

- [ ] 220. [DBItemGeosets.cpp] Equipped-items input coercion differs from JS `Object.entries` + `parseInt` behavior
- **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 251–259, 339–345
- **Status**: Pending
- **Details**: JS accepts plain objects keyed by strings and parses slot IDs with `parseInt`; C++ requires `std::unordered_map<int, uint32_t>` inputs, removing JS key-coercion behavior.

- [ ] 221. [DBItemModels.cpp] Item model cache initialization is synchronous instead of JS Promise-based flow
- **JS Source**: `src/js/db/caches/DBItemModels.js` lines 22–103
- **Status**: Pending
- **Details**: JS uses async `initialize` with shared `init_promise` and awaited dependent caches; C++ performs the entire load synchronously with no async/promise contract.

- [ ] 222. [DBItems.cpp] Item cache initialization is synchronous and does not preserve JS shared `init_promise`
- **JS Source**: `src/js/db/caches/DBItems.js` lines 14–59
- **Status**: Pending
- **Details**: JS deduplicates concurrent initialization via `init_promise` and async functions; C++ uses synchronous initialization and lacks equivalent awaitable behavior.

- [ ] 223. [DBModelFileData.cpp] Model mapping loader is synchronous instead of JS async API
- **JS Source**: `src/js/db/caches/DBModelFileData.js` lines 17–35
- **Status**: Pending
- **Details**: JS exposes `initializeModelFileData` as an async Promise-based loader; C++ implementation is synchronous blocking code.

- [ ] 224. [DBNpcEquipment.cpp] NPC equipment cache initialization is synchronous and drops JS `init_promise`
- **JS Source**: `src/js/db/caches/DBNpcEquipment.js` lines 30–66
- **Status**: Pending
- **Details**: JS uses async initialization with in-flight promise reuse; C++ initialization is synchronous and does not retain the JS async concurrency contract.

- [ ] 225. [DBTextureFileData.cpp] Texture mapping loader/ensure APIs are synchronous instead of JS async APIs
- **JS Source**: `src/js/db/caches/DBTextureFileData.js` lines 16–52
- **Status**: Pending
- **Details**: JS defines async `initializeTextureFileData` and `ensureInitialized`; C++ ports both as synchronous methods.

- [ ] 226. [DBTextureFileData.cpp] UsageType remap path remains a TODO placeholder in C++ port
- **JS Source**: `src/js/db/caches/DBTextureFileData.js` line 24
- **Status**: Pending
- **Details**: C++ retains the same `TODO` comment (`Need to remap this to support other UsageTypes`) and still skips non-zero `UsageType`, leaving this path explicitly unfinished.

- [ ] 227. [xxhash64.cpp] Public API contract differs from JS callable-export behavior
- **JS Source**: `src/js/hashing/xxhash64.js` lines 64–75, 286–288
- **Status**: Pending
- **Details**: JS exports a callable function that doubles as constructor/state prototype (`module.exports = XXH64`), while C++ exposes a class/static-method API only, changing the original module’s call surface semantics.

- [ ] 228. [font_helpers.cpp] `detect_glyphs_async` no longer implements JS DOM/callback contract
- **JS Source**: `src/js/modules/font_helpers.js` lines 56–106
- **Status**: Pending
- **Details**: JS clears and repopulates `grid_element`, wires per-glyph click handlers, and invokes `on_complete`; C++ only accumulates codepoints in `GlyphDetectionState` and drops the DOM/callback path from the original module API.

- [ ] 229. [font_helpers.cpp] Active detection cancellation semantics differ from JS global `active_detection`
- **JS Source**: `src/js/modules/font_helpers.js` lines 17, 57–63
- **Status**: Pending
- **Details**: JS tracks a module-level `active_detection` and cancels prior runs automatically, while C++ relies on caller-owned state and does not preserve the same global cancellation behavior across concurrent detections.

- [ ] 230. [font_helpers.cpp] `inject_font_face` return type/behavior differs from JS blob-URL + `document.fonts` flow
- **JS Source**: `src/js/modules/font_helpers.js` lines 113–133
- **Status**: Pending
- **Details**: JS injects `@font-face`, waits for `document.fonts.load/check`, and returns a URL string; C++ adds the font directly to ImGui atlas and returns `ImFont*`, dropping JS URL lifecycle and decode verification behavior.

- [ ] 231. [legacy_tab_audio.cpp] Playback UI visuals diverge from JS template/CSS
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 201–241
- **Status**: Pending
- **Details**: JS renders `#sound-player-anim`, CSS-styled play button state classes, and component sliders, while C++ replaces this with ImGui text/buttons/checkboxes and a custom icon pulse, so layout/styling is not pixel-identical.

- [ ] 232. [legacy_tab_audio.cpp] Seek-loop scheduling differs from JS `requestAnimationFrame` lifecycle
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 19–42
- **Status**: Pending
- **Details**: JS drives seek updates with `requestAnimationFrame` and explicit cancellation IDs; C++ updates via render-loop polling with `seek_loop_active`, changing timing and loop lifecycle semantics.

- [ ] 233. [legacy_tab_data.cpp] Export format menu omits JS SQL/DBC options
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 172–176, 222–231
- **Status**: Pending
- **Details**: JS menu exposes `CSV`, `SQL`, and `DBC` export actions, but C++ `legacy_data_opts` only includes `Export as CSV`, making SQL/DBC exports unavailable through the settings menu path.

- [ ] 234. [legacy_tab_data.cpp] `copy_cell` empty-string handling differs from JS
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 215–220
- **Status**: Pending
- **Details**: JS copies any non-null/undefined value (including empty string), while C++ returns early on `value.empty()`, so empty-cell clipboard behavior is not equivalent.

- [ ] 235. [legacy_tab_files.cpp] Listbox context menu includes extra FileDataID actions absent in JS
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 76–80
- **Status**: Pending
- **Details**: JS legacy-files menu only provides copy file path, copy export path, and open export directory; C++ conditionally adds listfile-format and fileDataID entries, changing context-menu behavior.

- [ ] 236. [legacy_tab_fonts.cpp] Preview text is not rendered with the selected font family
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 78, 165–169
- **Status**: Pending
- **Details**: JS binds textarea `fontFamily` to `fontPreviewFontFamily`, but C++ renders `InputTextMultiline` without switching to the loaded `ImFont`, so font preview output does not use the selected legacy font.

- [ ] 237. [legacy_tab_fonts.cpp] Font loading contract differs from JS URL-based `loaded_fonts` cache
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 18–41
- **Status**: Pending
- **Details**: JS caches `font_id -> blob URL` and reuses CSS font-family identifiers; C++ caches `font_id -> void*` ImGui font pointers, changing the original module’s data model and font-resource lifecycle behavior.

- [ ] 238. [legacy_tab_home.cpp] Legacy home tab template is replaced by shared `tab_home` layout
- **JS Source**: `src/js/modules/legacy_tab_home.js` lines 2–23
- **Status**: Pending
- **Details**: JS defines a dedicated legacy-home template structure (`#legacy-tab-home`, changelog HTML block, and external-link button rows), while C++ delegates to `tab_home::renderHomeLayout()`, so the legacy tab is not a line-by-line equivalent render path.

- [ ] 239. [legacy_tab_textures.cpp] Listbox context menu render path from JS template is missing
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 118–122, 147–161
- **Status**: Pending
- **Details**: JS mounts a `ContextMenu` component for texture list selections, but C++ never calls `context_menu::render(...)` in `render()`, leaving the expected right-click action menu unrendered.

- [ ] 240. [legacy_tab_textures.cpp] Channel toggle visuals/interaction differ from JS channel list UI
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 130–135
- **Status**: Pending
- **Details**: JS uses styled `<li>` channel pills with selected classes (`R/G/B/A`), while C++ uses standard ImGui checkboxes, producing non-identical visuals and interaction behavior.

- [ ] 241. [module_test_a.cpp] Module UI template structure differs from JS component markup
- **JS Source**: `src/js/modules/module_test_a.js` lines 2–8
- **Status**: Pending
- **Details**: JS renders a structured HTML block (`.module-test-a`, `<h2>`, `<p>`, buttons) styled via CSS, while C++ uses plain ImGui text/button primitives without equivalent DOM/CSS layout fidelity.

- [ ] 242. [module_test_b.cpp] Busy-state text formatting differs from JS boolean rendering
- **JS Source**: `src/js/modules/module_test_b.js` line 8
- **Status**: Pending
- **Details**: JS displays `Busy State` using Vue boolean/string rendering, while C++ prints `core::view->isBusy` with `%d`, emitting numeric values rather than matching the original presentation contract.

- [ ] 243. [screen_settings.cpp] Settings descriptions/help text from JS template are largely omitted
- **JS Source**: `src/js/modules/screen_settings.js` lines 24–353
- **Status**: Pending
- **Details**: JS includes extensive per-setting explanatory `<p>` text and warning copy, but C++ mostly renders condensed headings/controls; this is a substantial visual/content mismatch versus the original settings screen.

- [ ] 244. [screen_settings.cpp] Cache/listfile interval labels changed from days to hours
- **JS Source**: `src/js/modules/screen_settings.js` lines 271–274, 329–332
- **Status**: Pending
- **Details**: JS labels `cacheExpiry` and `listfileCacheRefresh` as day-based values, while C++ headings explicitly state hours (`Cache Expiry (hours)`, `Listfile Update Frequency (hours)`), changing user-facing semantics.

- [ ] 245. [screen_settings.cpp] Multi-button style groups are replaced with radio/checkbox controls
- **JS Source**: `src/js/modules/screen_settings.js` lines 111–115, 178–183, 232–236
- **Status**: Pending
- **Details**: JS uses `.ui-multi-button` grouped toggles for path format, export metadata, and copy mode; C++ replaces these with ImGui radio buttons/checkboxes, causing visible layout and styling deviations.

- [ ] 246. [screen_source_select.cpp] Source selection load flow is no longer Promise-based like JS
- **JS Source**: `src/js/modules/screen_source_select.js` lines 85–140, 142–167, 169–204, 267–287
- **Status**: Pending
- **Details**: JS source-open and build-load paths are async/await methods; C++ replaces these with synchronous calls and background-thread posting, changing timing/error propagation behavior versus the original module flow.

- [ ] 247. [screen_source_select.cpp] Hidden directory input reset/click flow is replaced with direct native dialog calls
- **JS Source**: `src/js/modules/screen_source_select.js` lines 252–258, 289–295, 326–337
- **Status**: Pending
- **Details**: JS creates persistent `<input nwdirectory>` selectors and resets `.value` before click to preserve reselection behavior; C++ calls `file_field::openDirectoryDialog()` directly and does not preserve the original selector-reset path.

- [ ] 248. [screen_source_select.cpp] CASC initialization failure toast omits JS support action
- **JS Source**: `src/js/modules/screen_source_select.js` lines 134–137
- **Status**: Pending
- **Details**: JS error toast includes both `View Log` and `Visit Support Discord` actions; C++ keeps only `View Log`, removing one original recovery handler.

- [ ] 249. [tab_audio.cpp] Audio quick-filter list path is missing from listbox wiring
- **JS Source**: `src/js/modules/tab_audio.js` line 190
- **Status**: Pending
- **Details**: JS passes `$core.view.audioQuickFilters` into the sound listbox. C++ passes an empty quickfilter list (`{}`), so the original quick-filter behavior is not available.

- [ ] 250. [tab_audio.cpp] `unload_track` no longer revokes preview URL data like JS
- **JS Source**: `src/js/modules/tab_audio.js` lines 95–97
- **Status**: Pending
- **Details**: JS explicitly calls `file_data?.revokeDataURL()` and clears `file_data`; C++ has no equivalent revoke path, changing cleanup behavior for previewed track resources.

- [ ] 251. [tab_audio.cpp] Sound player visuals differ from the JS template/CSS implementation
- **JS Source**: `src/js/modules/tab_audio.js` lines 203–228
- **Status**: Pending
- **Details**: JS renders `#sound-player-anim`, custom component sliders, and CSS-styled play-state button classes; C++ uses plain ImGui button/slider widgets and a different animated icon presentation, so visuals are not identical.

- [ ] 252. [tab_blender.cpp] Blender version gating semantics differ from JS string comparison behavior
- **JS Source**: `src/js/modules/tab_blender.js` lines 91, 139
- **Status**: Pending
- **Details**: JS compares versions as strings (`version >= MIN_VER`, `blender_version < MIN_VER`), while C++ parses with `std::stod` and compares numerically, changing edge-case ordering behavior.

- [ ] 253. [tab_blender.cpp] Blender install screen layout is not a pixel-equivalent port of JS markup
- **JS Source**: `src/js/modules/tab_blender.js` lines 59–69
- **Status**: Pending
- **Details**: JS uses structured `#blender-info`/`#blender-info-buttons` markup with CSS-defined spacing/styling; C++ replaces it with simple ImGui text/separator/buttons, producing visual/layout mismatch.

- [ ] 254. [tab_changelog.cpp] Changelog path resolution logic differs from JS two-path contract
- **JS Source**: `src/js/modules/tab_changelog.js` lines 14–16
- **Status**: Pending
- **Details**: JS uses `BUILD_RELEASE ? './src/CHANGELOG.md' : '../../CHANGELOG.md'`; C++ adds a third fallback (`CHANGELOG.md`) and different path probing order, changing source resolution behavior.

- [ ] 255. [tab_characters.cpp] Saved-character thumbnail card rendering is replaced by a placeholder button path
- **JS Source**: `src/js/modules/tab_characters.js` lines 1928–1934
- **Status**: Pending
- **Details**: JS renders thumbnail backgrounds and overlay action buttons in `.saved-character-card`; C++ renders a generic button with a `// thumbnail placeholder` path, so card visuals and thumbnail behavior diverge.

- [ ] 256. [tab_characters.cpp] Main-screen quick-save flow skips JS thumbnail capture step
- **JS Source**: `src/js/modules/tab_characters.js` lines 1973, 2328–2333
- **Status**: Pending
- **Details**: JS quick-save button routes through `open_save_prompt()` which captures `chrPendingThumbnail` before prompting; C++ main `Save` button only toggles prompt state and does not capture a fresh thumbnail first.

- [ ] 257. [tab_characters.cpp] Outside-click handlers for import/color popups from JS mounted lifecycle are missing
- **JS Source**: `src/js/modules/tab_characters.js` lines 2668–2685
- **Status**: Pending
- **Details**: JS registers a document click listener to close color pickers and floating import panels when clicking elsewhere; C++ has no equivalent mounted/unmounted document-listener path, changing panel-dismiss behavior.

- [ ] 258. [tab_creatures.cpp] Creature list context actions are not equivalent to JS copy-name/copy-ID menu
- **JS Source**: `src/js/modules/tab_creatures.js` lines 983–986, 1179–1203
- **Status**: Pending
- **Details**: JS creature list context menu exposes only `Copy name(s)` and `Copy ID(s)` handlers; C++ delegates to generic `listbox_context::handle_context_menu(...)`, changing the context action contract from the original creature-specific menu.

- [ ] 259. [tab_data.cpp] Data-table cell copy stringification differs from JS `String(value)` behavior
- **JS Source**: `src/js/modules/tab_data.js` lines 172–177
- **Status**: Pending
- **Details**: JS copies with `String(value)`, while C++ uses `value.dump()`; for string JSON values this includes JSON quoting/escaping, changing clipboard output.

- [ ] 260. [tab_data.cpp] DB2 load error toast omits JS `View Log` action callback
- **JS Source**: `src/js/modules/tab_data.js` lines 80–82
- **Status**: Pending
- **Details**: JS error toast includes `{'View Log': () => log.openRuntimeLog()}`; C++ error toast uses empty actions, removing the original recovery handler.

- [ ] 261. [tab_decor.cpp] PNG/CLIPBOARD export branch does not short-circuit like JS
- **JS Source**: `src/js/modules/tab_decor.js` lines 129–140
- **Status**: Pending
- **Details**: JS returns immediately after preview export for PNG/CLIPBOARD; C++ closes the export stream but continues into full model export logic, changing export behavior for these formats.

- [ ] 262. [tab_decor.cpp] Decor list context menu open/interaction path differs from JS ContextMenu component flow
- **JS Source**: `src/js/modules/tab_decor.js` lines 234–237
- **Status**: Pending
- **Details**: JS renders a dedicated ContextMenu node for listbox selections (`Copy name(s)` / `Copy file data ID(s)`); C++ uses a manual popup path without equivalent Vue component lifecycle/wiring, deviating from original interaction flow.

- [ ] 263. [tab_fonts.cpp] Font preview textarea is not rendered with the selected loaded font family
- **JS Source**: `src/js/modules/tab_fonts.js` lines 67, 159–163
- **Status**: Pending
- **Details**: JS binds preview textarea style `fontFamily` to the loaded font id; C++ updates `fontPreviewFontFamily` state but renders `InputTextMultiline` without switching ImGui font, so preview text does not reflect selected font family.

- [ ] 264. [tab_fonts.cpp] Loaded font cache contract differs from JS URL-based font-face lifecycle
- **JS Source**: `src/js/modules/tab_fonts.js` lines 10, 30–32
- **Status**: Pending
- **Details**: JS caches `font_id -> blob URL` from `inject_font_face`, preserving URL/font-face lifecycle; C++ caches `font_id -> void*` ImGui font pointer, changing resource model and API behavior.

- [ ] 265. [tab_help.cpp] Search filtering no longer uses JS 300ms debounce behavior
- **JS Source**: `src/js/modules/tab_help.js` lines 145–149, 153–157
- **Status**: Pending
- **Details**: JS applies article filtering via `setTimeout(..., 300)` debounce on `search_query`; C++ filters immediately on each input change, changing responsiveness and update timing.

- [ ] 266. [tab_help.cpp] Help article list presentation differs from JS title/tag/KB layout
- **JS Source**: `src/js/modules/tab_help.js` lines 115–121
- **Status**: Pending
- **Details**: JS renders per-item title and a separate tags row with KB badge styling; C++ combines content into selectable labels and tooltip tags, so article list visuals/structure are not identical.

- [ ] 267. [tab_changelog.cpp] Changelog screen typography/layout diverges from JS `#changelog` template styling
- **JS Source**: `src/js/modules/tab_changelog.js` lines 31–35
- **Status**: Pending
- **Details**: JS uses dedicated `#changelog`/`#changelog-text` template structure and CSS styling; C++ renders plain ImGui title/separator/button layout, causing visible UI differences.

- [ ] 268. [tab_home.cpp] Home showcase content is replaced with custom nav-card UI instead of the JS `HomeShowcase` component
- **JS Source**: `src/js/modules/tab_home.js` lines 4–5
- **Status**: Pending
- **Details**: JS renders `<HomeShowcase />`; C++ replaces that section with a custom navigation-card grid (`renderNavCard`/`renderHomeLayout`), changing the original home-tab content and visuals.

- [ ] 269. [tab_home.cpp] `whatsNewHTML` is rendered as plain text instead of HTML content
- **JS Source**: `src/js/modules/tab_home.js` line 6
- **Status**: Pending
- **Details**: JS uses `v-html="$core.view.whatsNewHTML"` to render formatted markup; C++ calls `ImGui::TextWrapped` on the raw string, so HTML formatting/links are not rendered.

- [ ] 270. [tab_install.cpp] Install listbox copy/paste options are hardcoded instead of using JS config-driven behavior
- **JS Source**: `src/js/modules/tab_install.js` lines 165, 184
- **Status**: Pending
- **Details**: JS listbox wiring uses `$core.view.config.copyMode`, `pasteSelection`, and `removePathSpacesCopy`; C++ passes `CopyMode::Default` with `pasteselection=false` and `copytrimwhitespace=false`, changing list interaction behavior.

- [ ] 271. [tab_install.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS renders `Regex Enabled` with `:title="$core.view.regexTooltip"`; C++ renders plain text without the tooltip contract, changing UI affordance.

- [ ] 272. [tab_item_sets.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_item_sets.js` line 82
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ shows plain `Regex Enabled` text without tooltip behavior.

- [ ] 273. [tab_items.cpp] Wowhead item handler is stubbed out
- **JS Source**: `src/js/modules/tab_items.js` lines 322–324
- **Status**: Pending
- **Details**: JS calls `ExternalLinks.wowHead_viewItem(item_id)` from the context action; C++ `view_on_wowhead(...)` immediately returns and does nothing.

- [ ] 274. [tab_items.cpp] Item sidebar checklist interaction/layout diverges from JS clickable row design
- **JS Source**: `src/js/modules/tab_items.js` lines 254–266
- **Status**: Pending
- **Details**: JS uses `.sidebar-checklist-item` rows with selected-state styling and row-level click toggling; C++ renders plain ImGui checkboxes, changing sidebar visuals and interaction feel.

- [ ] 275. [tab_items.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_items.js` line 248
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 276. [tab_maps.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_maps.js` line 302
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 277. [tab_models.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_models.js` line 296
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 278. [tab_models_legacy.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 340
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 279. [tab_raw.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_raw.js` line 158
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 280. [tab_text.cpp] Text preview failure toast omits JS `View Log` action callback
- **JS Source**: `src/js/modules/tab_text.js` lines 138–139
- **Status**: Pending
- **Details**: JS preview failure toast provides `{ 'View Log': () => log.openRuntimeLog() }`; C++ passes empty toast actions, removing the original recovery handler.

- [ ] 281. [tab_text.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_text.js` line 31
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 282. [tab_textures.cpp] Baked NPC texture apply path stores a file data ID instead of the JS BLP object
- **JS Source**: `src/js/modules/tab_textures.js` lines 423–427
- **Status**: Pending
- **Details**: JS loads the selected texture file and stores a `BLPFile` instance in `chrCustBakedNPCTexture`; C++ stores only the resolved file data ID, changing downstream data shape/behavior.

- [ ] 283. [tab_textures.cpp] Baked NPC texture failure toast omits JS `view log` action callback
- **JS Source**: `src/js/modules/tab_textures.js` lines 430–431
- **Status**: Pending
- **Details**: JS error toast includes `{ 'view log': () => log.openRuntimeLog() }`; C++ error toast has no action handlers, removing the original troubleshooting entry point.

- [ ] 284. [tab_textures.cpp] Texture channel controls are rendered as checkboxes instead of JS channel chips
- **JS Source**: `src/js/modules/tab_textures.js` lines 306–311
- **Status**: Pending
- **Details**: JS uses styled `li` channel chips (`R/G/B/A`) with selected-state classes; C++ renders standard ImGui checkboxes, causing visible control-style differences.

- [ ] 285. [tab_videos.cpp] Video preview playback is opened externally instead of using an embedded player
- **JS Source**: `src/js/modules/tab_videos.js` lines 219–276, 493
- **Status**: Pending
- **Details**: JS renders and controls an in-tab `<video>` element with `onended`/`onerror` and subtitle track attachment, while C++ opens the stream URL in an external handler and shows status text in the preview area.

- [ ] 286. [tab_videos.cpp] Video export format selector from MenuButton is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 505, 559–571
- **Status**: Pending
- **Details**: JS uses a `MenuButton` bound to `config.exportVideoFormat` and dispatches format-specific export via selection; C++ renders a single `Export Selected` button with no in-UI format picker.

- [ ] 287. [tab_videos.cpp] Kino processing toast omits the explicit Cancel action payload
- **JS Source**: `src/js/modules/tab_videos.js` lines 394–400
- **Status**: Pending
- **Details**: JS updates progress toast with `{ 'Cancel': cancel_processing }`; C++ calls `setToast(..., {}, ...)`, removing the explicit cancel action binding from the toast configuration.

- [ ] 288. [tab_videos.cpp] Dev-mode kino processing trigger export is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 467–469
- **Status**: Pending
- **Details**: JS exposes `window.trigger_kino_processing = trigger_kino_processing` in non-release mode; C++ has no equivalent debug export hook.

- [ ] 289. [tab_videos.cpp] Corrupted AVI fallback does not force CASC fallback fetch path
- **JS Source**: `src/js/modules/tab_videos.js` line 697
- **Status**: Pending
- **Details**: JS retries corrupted cinematic reads with `getFileByName(file_name, false, false, true, true)` to force fallback behavior; C++ retries `getVirtualFileByName(file_name)` with normal arguments.

- [ ] 290. [tab_zones.cpp] Default phase filtering excludes non-zero phases unlike JS
- **JS Source**: `src/js/modules/tab_zones.js` lines 78–79
- **Status**: Pending
- **Details**: JS includes all `UiMapXMapArt` links when `phase_id === null`; C++ filters to `PhaseID == 0` when no phase is selected.

- [ ] 291. [tab_zones.cpp] UiMapArtStyleLayer lookup key differs from JS relation logic
- **JS Source**: `src/js/modules/tab_zones.js` lines 88–90
- **Status**: Pending
- **Details**: JS resolves style layers by matching `UiMapArtStyleID` to `art_entry.UiMapArtStyleID`; C++ matches `UiMapArtID` to the linked art ID, changing style-layer association behavior.

- [ ] 292. [tab_zones.cpp] Base tile relation lookup uses layer-row ID instead of UiMapArt ID
- **JS Source**: `src/js/modules/tab_zones.js` lines 120–121
- **Status**: Pending
- **Details**: JS fetches `UiMapArtTile` relation rows with `art_style.ID` from the UiMapArt entry; C++ stores `CombinedArtStyle::id` as the UiMapArtStyleLayer row ID and uses that in `getRelationRows`, altering tile resolution.

- [ ] 293. [tab_zones.cpp] Base map tile OffsetX/OffsetY offsets are ignored
- **JS Source**: `src/js/modules/tab_zones.js` lines 181–182
- **Status**: Pending
- **Details**: JS applies `tile.OffsetX`/`tile.OffsetY` when placing map tiles; C++ calculates tile position from row/column and tile dimensions only.

- [ ] 294. [tab_zones.cpp] Zone listbox copy/paste trim options are hardcoded instead of config-bound
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: JS binds `copymode`, `pasteselection`, and `copytrimwhitespace` to config values; C++ hardcodes `CopyMode::Default`, `pasteselection=false`, and `copytrimwhitespace=false`.

- [ ] 295. [tab_zones.cpp] Phase selector placement differs from JS preview overlay layout
- **JS Source**: `src/js/modules/tab_zones.js` lines 341–349
- **Status**: Pending
- **Details**: JS renders the phase dropdown in a `preview-dropdown-overlay` inside the preview background; C++ renders phase selection in the bottom control bar.

- [ ] 296. [audio-helper.cpp] AudioPlayer::load does not return decoded buffer like JS
- **JS Source**: `src/js/ui/audio-helper.js` lines 31–35
- **Status**: Pending
- **Details**: JS `load()` returns the decoded `AudioBuffer`; C++ `AudioPlayer::load(...)` returns `void`, changing function contract.

- [ ] 297. [audio-helper.cpp] End-of-playback callback is polling-driven instead of event-driven
- **JS Source**: `src/js/ui/audio-helper.js` lines 57–67, 115–127
- **Status**: Pending
- **Details**: JS triggers completion via `source.onended`; C++ checks `ma_sound_at_end()` inside `get_position()` and invokes `on_ended` there, requiring polling and adding side effects to position queries.

- [ ] 298. [char-texture-overlay.cpp] Overlay button visibility updater is a no-op instead of internally toggling visibility
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 23–31
- **Status**: Pending
- **Details**: JS toggles `#chr-overlay-btn` display between `flex` and `none` inside `update_button_visibility()`, while C++ leaves `update_button_visibility()` empty and relies on external rendering checks.

- [ ] 299. [char-texture-overlay.cpp] Active-layer reattach flow is stubbed out
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 63–70
- **Status**: Pending
- **Details**: JS schedules `process.nextTick()` to re-append the active canvas after tab changes; C++ `ensureActiveLayerAttached()` is intentionally a no-op, so no equivalent reattach path runs.

- [ ] 300. [data-exporter.cpp] SQL null handling differs for empty-string values
- **JS Source**: `src/js/ui/data-exporter.js` lines 181–187
- **Status**: Pending
- **Details**: JS forwards actual empty strings as values and only maps `null`/`undefined` to SQL null; C++ passes only `std::string` data and routes empty strings through SQLWriter’s null sentinel path, changing empty-string semantics.

- [ ] 301. [data-exporter.cpp] Export failure records omit stack traces from helper marks
- **JS Source**: `src/js/ui/data-exporter.js` lines 68–71, 124–127, 196–199, 246–249
- **Status**: Pending
- **Details**: JS passes both `e.message` and `e.stack` to `helper.mark(...)`; C++ passes `e.what()` and `std::nullopt`, dropping stack details from failure metadata.

- [ ] 302. [model-viewer-utils.cpp] Clipboard preview export copies base64 text instead of PNG image data
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 299–303
- **Status**: Pending
- **Details**: JS writes PNG binary payload to the clipboard (`clipboard.set(..., 'png', true)`), while C++ uses `ImGui::SetClipboardText(buf.toBase64().c_str())`, resulting in text clipboard content rather than image clipboard content.

- [ ] 303. [model-viewer-utils.cpp] Animation selection guard treats empty string as null/undefined
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 251–253
- **Status**: Pending
- **Details**: JS exits only for `null`/`undefined`; C++ exits for `selected_animation_id.empty()`, which changes behavior for explicit empty-string IDs.

- [ ] 304. [model-viewer-utils.cpp] WMO renderer/export constructor inputs differ from JS filename-based path
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 208, 405
- **Status**: Pending
- **Details**: JS constructs `WMORendererGL`/`WMOExporter` with `file_name`; C++ uses `file_data_id`-based constructors and ignores the filename parameter in these paths.

- [ ] 305. [model-viewer-utils.cpp] View-state proxy is hardcoded to three prefixes instead of dynamic property resolution
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 503–528
- **Status**: Pending
- **Details**: JS proxy resolves fields dynamically via `core.view[prefix + ...]`; C++ only maps `"model"`, `"decor"`, and `"creature"` with explicit branches, removing generic-prefix behavior.

- [ ] 306. [app.cpp] Crash screen heading text differs from original JS
- **JS Source**: `src/app.js` line 70 / `src/index.html` line 70
- **Status**: Pending
- **Details**: JS crash screen displays `"Oh no! The kākāpō has exploded..."` in an `<h1>` tag. C++ (line 353) displays `"wow.export.cpp has crashed!"` instead. While the naming convention allows user-facing text to say "wow.export.cpp", the original personality/flavor text ("kākāpō has exploded") is lost entirely.

- [ ] 307. [app.cpp] Crash screen missing logo background element
- **JS Source**: `src/app.js` lines 37–39
- **Status**: Pending
- **Details**: JS crash() appends a `#logo-background` div to the body after replacing the markup. C++ renderCrashScreen() does not render any background logo watermark, deviating from the original visual appearance.

- [ ] 308. [app.cpp] Crash screen version/flavour/build color differs from JS CSS
- **JS Source**: `src/app.js` lines 44–47 / `src/app.css` `#crash-screen-versions span`
- **Status**: Pending
- **Details**: JS CSS styles version spans with `color: var(--border)` (#6c757d). C++ renderCrashScreen() (lines 360–364) uses default text color (FONT_PRIMARY) for version/flavour/build text, not the muted FONT_FADED / BORDER color.

- [ ] 309. [app.cpp] Crash screen error text styling does not match JS CSS
- **JS Source**: `src/app.js` lines 49–51 / `src/app.css` `#crash-screen-text`
- **Status**: Pending
- **Details**: JS CSS uses `font-weight: normal; font-size: 20px; margin: 20px 0` for the error text container, with `#crash-screen-text-code { font-weight: bold; margin-right: 5px }`. C++ uses `TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), ...)` for the error code (red color not in JS CSS) and default size text. The font size, spacing, and color all differ.

- [ ] 310. [app.cpp] ScaleAllSizes(1.5f) has no JS equivalent and alters all UI metrics
- **JS Source**: N/A (no equivalent in `src/app.js`)
- **Status**: Pending
- **Details**: C++ line 2630 calls `ImGui::GetStyle().ScaleAllSizes(1.5f)` which multiplies ALL ImGui style metrics (padding, spacing, rounding, scrollbar size, etc.) by 1.5. This has no JS counterpart and will make the UI 50% larger than the CSS-defined values in app.h/app.css. Combined with `FontGlobalScale = 1.5f / dpiScale` (line 2879), the net effect is a 1.5x magnification that doesn't exist in the original app.

- [ ] 311. [app.cpp] handleContextMenuClick accesses opt.handler instead of opt.action?.handler
- **JS Source**: `src/app.js` lines 318–322
- **Status**: Pending
- **Details**: JS `handleContextMenuClick(opt)` accesses `opt.action?.handler` (a nested property on the action object). C++ (line 1710) accesses `opt.handler` directly. This is a structural mapping difference — if the C++ ContextMenuOption struct stores the handler at the top level rather than nested under an `action` sub-object, callers constructing these options must account for the difference.

- [ ] 312. [app.cpp] click() disabled check uses bool parameter instead of DOM class check
- **JS Source**: `src/app.js` lines 369–371
- **Status**: Pending
- **Details**: JS `click(tag, event, ...params)` checks `event.target.classList.contains('disabled')` to determine if the element is disabled. C++ (line 1740) takes a `bool disabled` parameter. While functionally similar in ImGui context, the JS checks the actual DOM state of the clicked element (which could be stale or dynamically applied), while C++ requires the caller to explicitly pass the disabled flag.

- [ ] 313. [app.cpp] JS source_select.setActive() called twice; C++ only calls once
- **JS Source**: `src/app.js` lines 696, 719
- **Status**: Pending
- **Details**: JS calls `modules.source_select.setActive()` both inside the updater flow (line 696, after update check completes) AND unconditionally at line 719. C++ only calls `modules::setActive("source_select")` once at line 2781, and the updater flow is not ported. When the updater is ported, the C++ must replicate both call sites.

- [ ] 314. [app.cpp] whats-new.html path resolution differs from JS
- **JS Source**: `src/app.js` lines 710–711
- **Status**: Pending
- **Details**: JS reads from `'./src/whats-new.html'` (relative to the NW.js app root). C++ reads from `constants::DATA_DIR() / "whats-new.html"` (line 2765). The resolved paths may differ depending on how DATA_DIR is configured vs the NW.js working directory.

- [ ] 315. [app.cpp] Vue error handler uses 'ERR_VUE' error code; C++ render catch uses 'ERR_RENDER'
- **JS Source**: `src/app.js` line 514
- **Status**: Pending
- **Details**: JS sets `app.config.errorHandler = err => crash('ERR_VUE', err.message)` to catch Vue rendering errors. C++ catches exceptions in the render loop (line 1129) with `crash("ERR_RENDER", e.what())`. The error code string differs — JS uses `ERR_VUE`, C++ uses `ERR_RENDER`.

- [ ] 316. [app.cpp] JS `data-kb-link` click handler for help articles not ported
- **JS Source**: `src/app.js` lines 116–131
- **Status**: Pending
- **Details**: JS has a global click event listener that intercepts clicks on `[data-kb-link]` elements to open help articles via `modules.tab_help.open_article(kb_id)`, and clicks on `[data-external]` elements to open external links. In ImGui there are no DOM elements, so this click delegation pattern cannot exist as-is, but the equivalent functionality (help article deep-linking and external link handling) must be implemented wherever those links appear in the C++ UI.

- [ ] 317. [app.cpp] JS `showDevTools()` for debug builds has no C++ equivalent
- **JS Source**: `src/app.js` lines 76–78
- **Status**: Pending
- **Details**: JS calls `win.showDevTools()` for non-release builds to open the Chrome DevTools inspector. C++ has no equivalent debug tool launcher. This is an expected platform difference but should be documented.

- [ ] 318. [blob.cpp] BlobPolyfill::slice() treats end=0 differently from JS
- **JS Source**: `src/js/blob.js` lines 262–265
- **Status**: Pending
- **Details**: JS uses `end || this._buffer.length`, where `||` treats 0 as falsy and defaults to the full buffer length. C++ uses `std::optional<std::size_t>` with `value_or(_buffer.size())`, so passing `end=0` gives a 0-length slice in C++ but the full buffer in JS. This is a behavioral difference for the edge case where `end` is explicitly 0.

- [ ] 319. [buffer.cpp] setCapacity(secure=false) always zero-initializes unlike JS Buffer.allocUnsafe
- **JS Source**: `src/js/buffer.js` lines 1021–1029
- **Status**: Pending
- **Details**: JS `setCapacity` uses `Buffer.allocUnsafe(capacity)` when `secure=false`, leaving expanded memory uninitialized. C++ uses `std::vector<uint8_t>(capacity, 0)` which always zero-initializes. This mirrors the existing alloc() difference (TODO 6) but applies to setCapacity specifically. Performance-only difference with no functional impact.

- [ ] 320. [buffer.cpp] startsWith(array) reads entries from sequential positions instead of all from offset 0
- **JS Source**: `src/js/buffer.js` lines 592–604
- **Status**: Pending
- **Details**: Both JS and C++ `startsWith(array)` call `seek(0)` once before the loop, then read each alternative entry sequentially. After reading the first entry (e.g., 3 bytes), the offset advances, so the second entry is checked at offset 3, not 0. This is a faithful port of the JS logic, but the likely intent is to check if the buffer starts with ANY of the alternatives (all from offset 0). In practice, FILE_IDENTIFIERS matching likely calls the single-string overload per match, so the array overload may not be triggered for multi-match identifiers.

- [ ] 321. [config.cpp] doSave() array comparison uses value equality instead of JS reference equality
- **JS Source**: `src/js/config.js` lines 98–104
- **Status**: Pending
- **Details**: JS `defaultConfig[key] === value` uses strict reference equality — since `copyConfig` clones arrays with `value.slice(0)`, array config values are always different instances from defaults, so arrays are always persisted to the user config file. C++ `defaultConfig[it.key()] == it.value()` uses nlohmann::json value equality, so arrays matching defaults are skipped. This means the C++ user config file will be smaller (omitting default-matching arrays), while JS always includes them. On reload, missing arrays are filled from defaults, so this is a write-path-only difference.

- [ ] 322. [config.cpp] save() std::async discarded future blocks in destructor making save synchronous
- **JS Source**: `src/js/config.js` lines 83–91
- **Status**: Pending
- **Details**: C++ `save()` calls `std::async(std::launch::async, doSave)` but discards the returned `std::future`. Per the C++ standard, a `std::future` from `std::async` blocks in its destructor until the task completes. This makes the save effectively synchronous (blocking the caller), defeating the intended deferred behavior that JS achieves with `setImmediate(doSave)`. Complements existing TODO 13 which notes the scheduling mechanism differs but does not mention the blocking destructor.

- [ ] 323. [constants.cpp] RUNTIME_LOG placed in separate Logs/ subdirectory instead of DATA_PATH root
- **JS Source**: `src/js/constants.js` line 38
- **Status**: Pending
- **Details**: JS sets `RUNTIME_LOG: path.join(DATA_PATH, 'runtime.log')` — the log file sits directly in the user data directory. C++ creates a separate `Logs/` subdirectory under the install path (`s_log_dir = s_install_path / "Logs"`) and places `runtime.log` inside it. This is a path deviation beyond the general DATA_PATH difference (TODO 16) — the directory structure gains an extra nesting level.

- [ ] 324. [constants.cpp] Legacy directory migration code in init() has no JS equivalent
- **JS Source**: `src/js/constants.js` (no equivalent)
- **Status**: Pending
- **Details**: C++ `constants::init()` lines 123–155 contain migration logic to rename legacy `config/` or `persistence/` directories to `data/`, and `casc/` to `cache/`. This code has no counterpart in the JS source and is a C++-specific addition to handle evolving directory naming across C++ port versions. While harmless, it is additional logic that deviates from the JS source structure.

- [ ] 325. [constants.cpp] Explicit create_directories() for data and log dirs not present in JS
- **JS Source**: `src/js/constants.js` (no equivalent)
- **Status**: Pending
- **Details**: C++ `constants::init()` lines 141–142 call `fs::create_directories(s_data_dir)` and `fs::create_directories(s_log_dir)` to ensure directories exist before any module writes to them. JS relies on NW.js to automatically create `nw.App.dataPath`. This is a necessary C++ adaptation but represents additional logic not in the JS source.

- [ ] 326. [core.cpp] isDev uses NDEBUG instead of JS BUILD_RELEASE flag
- **JS Source**: `src/js/core.js` line 33
- **Status**: Pending
- **Details**: JS sets `isDev: !BUILD_RELEASE` using a specific build-pipeline flag. C++ uses `#ifdef NDEBUG` (core.h lines 260–264) which is a standard C++ debug macro tied to CMake build types. The semantics may differ: `NDEBUG` is defined for both Release and RelWithDebInfo configurations, while `BUILD_RELEASE` is a custom flag that may only be set for true production builds. A custom `BUILD_RELEASE` CMake definition would be more faithful.

- [ ] 327. [core.cpp] openInExplorer() is a C++ addition with no direct JS equivalent
- **JS Source**: `src/js/core.js` line 485 (`nw.Shell.openItem()`)
- **Status**: Pending
- **Details**: C++ core.cpp lines 415–426 defines `openInExplorer(const std::string& path)` as a standalone utility function declared in core.h line 647. The JS source only uses `nw.Shell.openItem(core.view.config.exportDirectory)` inline in `openExportDirectory()`. While `openInExplorer` is a necessary platform adaptation (ShellExecuteW / xdg-open), its existence as a separately exported function is additional API surface not present in the JS module exports.

- [ ] 328. [external-links.h] Windows open() uses naive wstring conversion instead of proper MultiByteToWideChar
- **JS Source**: `src/js/external-links.js` lines 31–35
- **Status**: Pending
- **Details**: `ExternalLinks::open()` in external-links.h line 75 converts URL to wstring via `std::wstring(url.begin(), url.end())`, a naive char-by-char copy that only works for ASCII characters. This is inconsistent with `core::openInExplorer()` (core.cpp lines 418–420) which properly uses `MultiByteToWideChar(CP_UTF8, ...)` for correct UTF-8 to UTF-16 conversion. While URLs are typically ASCII, this is a correctness bug for any URL containing non-ASCII bytes.

- [ ] 329. [external-links.h] wowHead_viewItem() hardcodes URL string instead of using WOWHEAD_ITEM constant
- **JS Source**: `src/js/external-links.js` lines 24, 42–43
- **Status**: Pending
- **Details**: JS defines `const WOWHEAD_ITEM = 'https://www.wowhead.com/item=%d'` and uses it via `util.format(WOWHEAD_ITEM, itemID)`. C++ defines `WOWHEAD_ITEM` as `"https://www.wowhead.com/item={}"` (external-links.h line 48) but `wowHead_viewItem()` (line 89) hardcodes `std::format("https://www.wowhead.com/item={}", itemID)` instead of using the constant. The constant is effectively dead code.

- [ ] 330. [external-links.h] renderLink() missing CSS a:hover visual effects (color change and underline)
- **JS Source**: `src/app.css` lines 93–101 (a tag styling, a:hover)
- **Status**: Pending
- **Details**: `ExternalLinks::renderLink()` (external-links.h lines 107–117) only changes the cursor to a hand on hover. The original CSS defines `a:hover { color: var(--font-highlight); text-decoration: underline; }` which means links should change to pure white (#ffffff) and show an underline on hover. The C++ renderLink() is missing both the hover color change and the underline decoration, causing a visual fidelity difference from the original JS app.

- [ ] 331. [generics.cpp] get() timeout semantics differ from JS 30-second AbortSignal
- **JS Source**: `src/js/generics.js` lines 23–31
- **Status**: Pending
- **Details**: JS uses `AbortSignal.timeout(30000)` which enforces a hard 30-second total timeout for the entire fetch operation. C++ `doHttpGet()` (generics.cpp lines 123–124) sets `set_connection_timeout(30)` and `set_read_timeout(60)` separately, allowing up to 90 seconds total (30s to connect + 60s to read). This means C++ requests can take up to 3x longer than the JS equivalent before timing out.

- [ ] 332. [generics.cpp] queue() initial batch size off-by-one compared to JS
- **JS Source**: `src/js/generics.js` lines 63–83
- **Status**: Pending
- **Details**: JS initializes `free = limit` and `complete = -1`, then calls `check()` which increments both (`complete++; free++`), resulting in `free = limit + 1` items launched in the first batch. C++ (generics.cpp lines 399–433) launches exactly `limit` items in the first batch. This off-by-one means the JS version processes `limit + 1` items concurrently on the initial dispatch, while C++ processes exactly `limit`.

- [ ] 333. [generics.cpp] fileExists() checks existence only, not accessibility like JS fsp.access
- **JS Source**: `src/js/generics.js` lines 343–350
- **Status**: Pending
- **Details**: JS uses `await fsp.access(file)` which checks that the file both exists AND is accessible to the current process (read permission). C++ uses `std::filesystem::exists(file)` (generics.cpp line 710) which only checks existence, not accessibility. A file that exists but has restrictive permissions (e.g., mode 000) would return `true` in C++ but `false` in JS, potentially causing downstream errors when attempting to read/open such files.

- [ ] 334. [generics.cpp] computeFileHash() has malformed duplicate doc comment block
- **JS Source**: `src/js/generics.js` lines 328–336
- **Status**: Pending
- **Details**: generics.cpp lines 251–258 contain a broken doc comment where a new `/**` block starts (line 254) before the previous `/**` block (line 252) is properly closed. The first comment says "Compute hash of a file using streaming I/O." and the second says "Compute hash of a file using streaming mbedTLS MD API." This is a documentation-only issue that does not affect compilation but makes the comment block malformed.

- [ ] 335. [generics.cpp] downloadFile() error logging loses full error object details
- **JS Source**: `src/js/generics.js` lines 243–244
- **Status**: Pending
- **Details**: JS logs the full error object with `log.write(error)` (line 244) which includes the error message, stack trace, and any additional properties. C++ (generics.cpp line 595) only logs `error.what()` message string via `logging::write(std::format("Failed to download from {}: {}", currentUrl, error.what()))`. Stack trace and other diagnostic information available in the original JS error are lost.

- [ ] 336. [generics.cpp] requestData() is publicly declared but is a private/unexported function in JS
- **JS Source**: `src/js/generics.js` lines 145–205, 484–502
- **Status**: Pending
- **Details**: JS `requestData()` is a file-scoped function that is NOT included in `module.exports` (lines 484–502). C++ declares `requestData()` in `generics.h` line 86 as a public function in the `generics` namespace, making it part of the public API. While this doesn't break functionality, it exposes an internal implementation detail that JS keeps private, and external callers could depend on this function when they shouldn't.

- [ ] 337. [gpu-info.cpp] Caps logging condition differs from JS — C++ uses `max_tex_size > 0` while JS always logs caps
- **JS Source**: `src/js/gpu-info.js` lines 348–349
- **Status**: Pending
- **Details**: JS checks `if (webgl.caps)` (line 348) before logging capabilities. Since `caps` is always assigned as an object `{}` (line 25), this condition is always truthy when the WebGL context exists. C++ (gpu-info.cpp line 539) checks `if (gl->caps.max_tex_size > 0)` which would skip caps logging if `max_tex_size` happened to be 0. While unlikely in practice with real GPUs, this is a behavioral deviation from JS which unconditionally logs caps when the GL context is available.

- [ ] 338. [icon-render.h] Truncated doc comment — sentence fragment on line 17
- **JS Source**: `src/js/icon-render.js` lines 1–109
- **Status**: Pending
- **Details**: The namespace doc comment in `icon-render.h` lines 14–18 contains a sentence fragment: `" * in a cache. The queue mechanism with priority ordering is preserved."` on line 17. The preceding text (lines 14–15) describes the JS CSS-based approach but the transition to the C++ approach is missing — "in a cache." is an orphaned fragment, likely left over from an incomplete edit. The full comment should describe that in C++, icons are loaded as BLP files, decoded to RGBA pixel data, and stored as GL textures in a cache.

- [ ] 339. [log.cpp] timeEnd() signature loses JS variadic parameter support
- **JS Source**: `src/js/log.js` lines 64–66
- **Status**: Pending
- **Details**: JS `timeEnd(label, ...params)` passes additional variadic parameters through to `write()`: `write(label + ' (%dms)', ...params, (Date.now() - markTimer))` (line 65). C++ `timeEnd(std::string_view label)` (log.cpp line 179) only accepts a single label string and appends the elapsed time. Any caller that passes extra format arguments to `timeEnd` in JS would lose those values in C++. This is a separate concern from entry 40 (which covers `write()` itself) because `timeEnd` is a distinct exported API with its own parameter contract.

- [ ] 340. [log.cpp] getErrorDump() is synchronous in C++ vs async in JS
- **JS Source**: `src/js/log.js` lines 102–108
- **Status**: Pending
- **Details**: JS declares `getErrorDump = async () => { return await fs.promises.readFile(constants.RUNTIME_LOG, 'utf8'); }` (line 102–104), making it an async function that returns a Promise. C++ `getErrorDump()` (log.cpp lines 208–220) reads the file synchronously with `std::ifstream` and returns `std::string` directly. While the C++ approach is arguably more appropriate for crash-time diagnostics (where the event loop may be unavailable), it changes the function's execution model from non-blocking to blocking I/O.

- [ ] 341. [mmap.cpp] C++ map() explicitly rejects empty files which JS wrapper does not handle
- **JS Source**: `src/js/mmap.js` lines 20–23
- **Status**: Pending
- **Details**: C++ `MmapObject::map()` explicitly checks for `size == 0` and returns false with `lastError = "File is empty"` (mmap.cpp lines 113–118 on Windows, lines 162–167 on Linux). The JS wrapper in `mmap.js` has no such guard — it delegates entirely to `new mmap_native.MmapObject()` and the native addon's `map()` method. Whether the native `.node` addon rejects empty files is unknown from the JS source alone, making this a potential behavioral divergence where C++ would fail on empty files while JS might succeed (mapping zero bytes).

- [ ] 342. [modules.cpp] `display_label` in `wrap_module` error messages shows module key instead of nav button label
- **JS Source**: `src/js/modules.js` lines 208–213
- **Status**: Pending
- **Details**: In JS `wrap_module`, `display_label` starts as `module_name` but is updated to `label` inside the `registerNavButton` callback (line 213: `display_label = label`). The captured `display_label` is then used in the wrapped `initialize` error messages (line 236–237), producing user-friendly text like "Failed to initialize Maps tab". In C++ `wrap_module` (modules.cpp line 194), `display_label` is set to `mod.name` and never updated from the nav button registration, so error messages show the module key (e.g., "Failed to initialize tab_maps tab") instead of the display label (e.g., "Failed to initialize Maps tab").

- [ ] 343. [updater.cpp] Linux fork+exec failure path has no error logging unlike JS child.on('error') handler
- **JS Source**: `src/js/updater.js` lines 150–155
- **Status**: Pending
- **Details**: JS `launchUpdater()` attaches a `child.on('error')` handler (line 152–155) that logs `'ERROR: Failed to spawn updater: %s'` when `cp.spawn()` fails asynchronously. In C++ on Linux (updater.cpp lines 260–274), if `fork()` succeeds but `execl()` fails in the child process, the child simply calls `_exit(1)` with no logging whatsoever. On Windows, `CreateProcessA` failure is caught synchronously and logged via the catch block, but on Linux the exec-failure error path is completely silent. This means a missing or non-executable updater binary on Linux produces no diagnostic output.

- [ ] 344. [wmv.cpp] safe_parse_int returns 0 for fully non-numeric strings while JS parseInt returns NaN
- **JS Source**: `src/js/wmv.js` lines 44, 57–58, 87–91
- **Status**: Pending
- **Details**: JS `parseInt('abc')` returns `NaN`, which propagates through the parsed result (e.g., in v1 `legacy_values` or v2 `customizations`/`equipment`). C++ `safe_parse_int()` (wmv.cpp lines 26–43) catches `std::stoi` exceptions for non-numeric strings and returns `std::nullopt`, which callers convert to 0 via `value_or(0)`. For .chr files with non-numeric `@_value` attributes, JS would store `NaN` while C++ stores `0`, potentially causing different downstream behavior in character customization or equipment application.

- [ ] 345. [Shaders.cpp] C++ adds automatic _unregister_fn callback on ShaderProgram not present in JS
- **JS Source**: `src/js/3D/Shaders.js` lines 56–72
- **Status**: Pending
- **Details**: C++ `create_program()` (Shaders.cpp lines 79–83) installs a static `_unregister_fn` callback on `gl::ShaderProgram` that automatically calls `shaders::unregister()` when a ShaderProgram is destroyed. JS has no equivalent auto-cleanup mechanism — callers must explicitly call `unregister(program)` (Shaders.js line 78–86). This means in C++, a program is automatically removed from `active_programs` on destruction, while in JS a disposed program remains in the set until manually unregistered. This changes `reload_all()` behavior: JS could attempt to recompile stale programs that were not explicitly unregistered, while C++ never encounters this scenario.

- [ ] 346. [ADTExporter.cpp] Scale factor check `!= 0` instead of `!== undefined` changes behavior for scale=0
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1270
- **Status**: Pending
- **Details**: JS checks `model.scale !== undefined ? model.scale / 1024 : 1`. C++ (ADTExporter.cpp ~line 1521) checks `model.scale != 0 ? model.scale / 1024.0f : 1.0f`. In JS, a `scale` of `0` would produce `0 / 1024 = 0` (a valid zero-scale value). In C++, a `scale` of `0` triggers the else branch and returns `1.0f`. This is a behavioral difference — a model with scale=0 would be invisible in JS but normal-sized in C++, affecting M2 doodad CSV export and placement transforms.

- [ ] 347. [ADTExporter.cpp] GL index buffer uses GL_UNSIGNED_INT (uint32) instead of JS GL_UNSIGNED_SHORT (uint16)
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1117–1118
- **Status**: Pending
- **Details**: JS creates `new Uint16Array(indices)` and uses `gl.UNSIGNED_SHORT` for the index element buffer when rendering alpha map tiles. C++ (ADTExporter.cpp ~lines 1327–1328) uses `sizeof(uint32_t)` and `GL_UNSIGNED_INT`. For the 16×16×145 = 37120 vertex terrain grid the indices fit in uint16, so both work, but the GPU draws with different index types. This is a minor fidelity deviation in the GL pipeline even though the visual output is identical.

- [ ] 348. [ADTExporter.cpp] Liquid JSON serialization uses explicit fields instead of JS spread operator
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1428–1438
- **Status**: Pending
- **Details**: JS uses `{ ...chunk, instances: enhancedInstances }` and `{ ...instance, worldPosition, terrainChunkPosition }` which copies *all* fields from the chunk/instance objects via the spread operator. C++ (ADTExporter.cpp ~lines 1744–1780) manually enumerates specific fields for JSON serialization. If the ADTLoader's liquid chunk or instance structs have any additional fields not listed in the C++ serialization, those fields would appear in the JS JSON output but be missing in the C++ output. This is a fragile pattern that could silently omit data if new fields are added to the loader structs.

- [ ] 349. [ADTExporter.cpp] STB_IMAGE_RESIZE_IMPLEMENTATION defined at file scope risks ODR violation
- **JS Source**: N/A (C++ build concern)
- **Status**: Pending
- **Details**: ADTExporter.cpp (line 10) defines `#define STB_IMAGE_RESIZE_IMPLEMENTATION` before including `<stb_image_resize2.h>`. If any other translation unit in the project also defines this macro, the linker will encounter duplicate symbol definitions (ODR violation). STB implementation macros should typically be isolated in a single dedicated .cpp file (like stb-impl.cpp already exists for stb_image/stb_image_write) to avoid this risk.

- [ ] 350. [CharacterExporter.cpp] remap_bone_indices truncates remap_table.size() to uint8_t causing incorrect comparison
- **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 126–138
- **Status**: Pending
- **Details**: C++ `remap_bone_indices()` (CharacterExporter.cpp line 147) compares `original_idx < static_cast<uint8_t>(remap_table.size())`. If `remap_table` has 256 or more entries, `static_cast<uint8_t>(256)` wraps to `0`, making the comparison `original_idx < 0` always false for unsigned types — no indices would be remapped at all. For tables with 257–511 entries, the truncated size wraps to small values, skipping valid remap entries for higher indices. JS has no such issue since `original_idx < remap_table.length` uses normal number comparison. The fix should be `static_cast<size_t>(original_idx) < remap_table.size()` or simply removing the cast.

- [ ] 351. [M2Exporter.cpp] Data textures silently dropped from GLTF/GLB texture maps and buffers
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 357–366
- **Status**: Pending
- **Details**: In JS, `textureMap` is a `Map` with mixed key types — numeric fileDataIDs and string keys like `"data-5"` for data textures (canvas-composited textures). These are passed directly to `gltf.setTextureMap()`. In C++ (M2Exporter.cpp ~lines 610–636), the string-keyed `textureMap` is converted to a `uint32_t`-keyed `gltfTexMap` via `std::stoul()`. Keys like `"data-5"` fail parsing and are silently dropped in the `catch (...)` block. The same happens for `texture_buffers` in GLB mode. This means data textures (canvas-composited textures for character models) are lost in GLTF/GLB exports — meshes will reference material names that have no corresponding texture entry.

- [ ] 352. [M2Exporter.cpp] uint16_t loop variable for triangle iteration risks overflow/infinite loop
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 375, 496, 638, 701, 850, 936
- **Status**: Pending
- **Details**: All triangle iteration loops in M2Exporter.cpp use `for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)`. If `mesh.triangleCount` is 65535, incrementing `vI` from 65534 to 65535 works, but then `vI++` wraps to 0, causing an infinite loop. If `triangleCount` exceeds 65535 (stored as uint32_t in the struct), the loop would also be incorrect since `uint16_t` can never reach the termination condition. JS uses `let vI` which is a double-precision float with no overflow. Should use `uint32_t` for the loop variable.

- [ ] 353. [M2Exporter.cpp] addURITexture accepts BufferWrapper instead of data URI string
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59–61
- **Status**: Pending
- **Details**: JS `addURITexture(out, dataURI)` stores a raw base64 data URI string, which is decoded later in `exportTextures()` via `BufferWrapper.fromBase64(dataTexture.replace(...))`. C++ `addURITexture(uint32_t textureType, BufferWrapper pngData)` accepts already-decoded PNG data, shifting the decoding responsibility to the caller. This is a contract change that alters the interface boundary — callers must now pre-decode the data URI before passing it. While not a bug if all callers are adapted, it changes the API surface compared to the original JS.

- [ ] 354. [M2Exporter.cpp] JSON submesh serialization uses fixed field enumeration instead of JS Object.assign
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 794
- **Status**: Pending
- **Details**: JS uses `Object.assign({ enabled: subMeshEnabled }, skin.subMeshes[i])` which dynamically copies *all* properties from the submesh object. C++ (M2Exporter.cpp ~lines 1111–1126) manually enumerates a fixed set of properties (submeshID, level, vertexStart, vertexCount, triangleStart, triangleCount, boneCount, boneStart, boneInfluences, centerBoneIndex, centerPosition, sortCenterPosition, sortRadius). If the Skin's SubMesh struct gains new fields, they would automatically appear in JS JSON output but would be missing in C++ JSON output. This is a fragile pattern that could silently omit metadata.

- [ ] 355. [M2Exporter.cpp] Data texture file manifest entries get fileDataID=0 instead of string key
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 748
- **Status**: Pending
- **Details**: In JS, `texFileDataID` for data textures is the string key `"data-X"`, which gets stored as-is in the file manifest. In C++ (~line 1059), `std::stoul(texKey)` fails for `"data-X"` keys and `texID` defaults to 0 in the `catch (...)` block. This means data textures in the file manifest will have `fileDataID = 0` instead of a meaningful identifier, losing the ability to correlate manifest entries with specific data texture types.

- [ ] 356. [M2Exporter.cpp] formatUnknownFile call signature differs from JS
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 194
- **Status**: Pending
- **Details**: JS calls `listfile.formatUnknownFile(texFile)` where `texFile` is a string like `"12345.png"`. C++ (~line 410) calls `casc::listfile::formatUnknownFile(texFileDataID, raw ? ".blp" : ".png")` passing the numeric ID and extension separately. The C++ call passes `raw ? ".blp" : ".png"` but this code appears in the `!raw` branch (line 406 checks `!raw`), so the `raw` ternary would always evaluate to `.png`. While not necessarily a bug (depends on `formatUnknownFile` implementation), the call signature divergence means the output filename format may differ.

- [ ] 357. [M2LegacyExporter.cpp] uint16_t loop variable for triangle iteration risks overflow
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 164, 289
- **Status**: Pending
- **Details**: Same issue as M2Exporter: triangle iteration loops in M2LegacyExporter.cpp (exportAsOBJ ~line 212, exportAsSTL ~line 401) use `for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)`. If `mesh.triangleCount` reaches or exceeds 65535, `uint16_t` overflow causes an infinite loop or incorrect iteration. JS uses `let vI` with no overflow limit. Should use `uint32_t` for the loop variable.

- [ ] 358. [M3Exporter.cpp] addURITexture accepts BufferWrapper instead of data URI string
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49–51
- **Status**: Pending
- **Details**: Same issue as M2Exporter: JS `addURITexture(out, dataURI)` stores a raw base64 data URI string keyed by output path. C++ `addURITexture(const std::string& out, BufferWrapper pngData)` accepts already-decoded PNG data. This is an API contract change that shifts decoding responsibility to the caller.

- [ ] 359. [M3Exporter.cpp] exportTextures returns map<uint32_t, string> instead of JS Map with mixed key types
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 62–65
- **Status**: Pending
- **Details**: While the JS `exportTextures()` currently returns an empty Map (texture export not yet implemented), the C++ return type `std::map<uint32_t, std::string>` constrains future implementation to numeric-only keys. If M3 texture export is later implemented following M2Exporter's pattern (which uses string keys like `"data-X"` for data textures), the uint32_t key type would need to change. The JS Map supports mixed key types natively. This is a forward-compatibility concern rather than a current bug.

- [ ] 360. [WMOExporter.cpp] uint16_t loop variable for face iteration risks overflow/infinite loop (4 locations)
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 385, 551, 862, 1004 (batch.numFaces iteration)
- **Status**: Pending
- **Details**: Four face iteration loops in WMOExporter.cpp (lines 484, 643, 1077, 1202) use `for (uint16_t fi = 0; fi < batch.numFaces; fi++)`. If `batch.numFaces` reaches or exceeds 65535, the `uint16_t` loop variable wraps to 0, causing an infinite loop or incorrect iteration. JS uses `let i` which is a double-precision float with no overflow at these magnitudes. Should use `uint32_t` for the loop variable. Same issue as entry 352 (M2Exporter) and entry 357 (M2LegacyExporter).

- [ ] 361. [WMOExporter.cpp] Constructor takes explicit casc::CASC* parameter not present in JS
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 34–36
- **Status**: Pending
- **Details**: JS constructor is `constructor(data, fileID)` and obtains CASC source internally via `core.view.casc`. C++ constructor is `WMOExporter(BufferWrapper data, uint32_t fileDataID, casc::CASC* casc)` with explicit casc pointer. Additionally, `fileDataID` is constrained to `uint32_t` while JS accepts `string|number` for `fileID`. This is an API deviation — callers must pass the correct CASC instance and cannot pass string file paths.

- [ ] 362. [WMOExporter.cpp] Extra loadWMO() and getDoodadSetNames() accessor methods not in JS
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 34–36
- **Status**: Pending
- **Details**: C++ adds `loadWMO()` (line 1698) and `getDoodadSetNames()` (line 1702) methods that do not exist in the JS WMOExporter class. In JS, `this.wmo` is a public property accessed directly by callers. In C++, `wmo` is a private `std::unique_ptr<WMOLoader>`, so these accessor methods were added to expose the loader. This is a necessary C++ adaptation but changes the public API surface.

- [ ] 363. [WMOLegacyExporter.cpp] uint16_t loop variable for face iteration risks overflow/infinite loop (2 locations)
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 202, 425 (batch.numFaces iteration)
- **Status**: Pending
- **Details**: Two face iteration loops in WMOLegacyExporter.cpp (lines 288, 603) use `for (uint16_t i = 0; i < batch.numFaces; i++)`. If `batch.numFaces` reaches or exceeds 65535, the `uint16_t` loop variable wraps to 0, causing an infinite loop or incorrect iteration. JS uses `let i` with no overflow risk. Should use `uint32_t` for the loop variable. Same issue as entries 352, 357, 360.

- [ ] 364. [ShaderProgram.cpp] `_compile` method handles partial shader failure differently from JS
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 29–36
- **Status**: Pending
- **Details**: When one shader compiles successfully but the other fails, JS `_compile` (line 35-36) simply returns without deleting the successfully compiled shader, leaking a WebGL resource until garbage collection. C++ `_compile` (lines 30-35) correctly deletes both shaders on partial failure (`if (vert_shader) glDeleteShader(vert_shader); if (frag_shader) glDeleteShader(frag_shader);`). This is a behavioral improvement over JS but is technically a deviation from the original logic. Note: JS `recompile()` (line 248-256) does handle this correctly — only `_compile` has the issue.

- [ ] 365. [ShaderProgram.cpp] `set_uniform_3fv`/`set_uniform_4fv`/`set_uniform_mat4_array` have extra count parameter
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 187–234
- **Status**: Pending
- **Details**: JS `set_uniform_3fv(name, value)` calls `gl.uniform3fv(loc, value)` where the WebGL2 API infers the count from the typed array length. C++ `set_uniform_3fv(name, value, count=1)` takes an explicit `count` parameter (defaulting to 1) because OpenGL's `glUniform3fv(loc, count, value)` requires it. Same applies to `set_uniform_4fv` and `set_uniform_mat4_array`. While single-value calls work identically (count defaults to 1), callers passing arrays of multiple vec3/vec4/mat4 values must specify the correct count in C++. This is a necessary C++ adaptation but changes the API contract.

- [ ] 366. [M2Generics.cpp] Error message text differs in useAnims branch ("Unhandled" vs "Unknown")
- **JS Source**: `src/js/3D/loaders/M2Generics.js` lines 78, 101
- **Status**: Pending
- **Details**: JS `read_m2_array_array` has two separate switch blocks — the useAnims branch (line 78) throws `"Unhandled data type: ${dataType}"` while the non-useAnims branch (line 101) throws `"Unknown data type: ${dataType}"`. C++ collapses both branches into a single `read_value()` helper that always throws `"Unknown data type: "` for both paths. The error message for the useAnims branch differs from the original JS.

- [ ] 367. [M2Loader.cpp] `loadAnimsForIndex()` catch block logs fileDataID instead of animation.id
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 199–201
- **Status**: Pending
- **Details**: JS catch block logs `"Failed to load .anim file for animation " + animation.id + ": " + e.message`, identifying the animation by its `id` field. C++ catch block logs `"Failed to load .anim file (fileDataID={}): {}"` using the CASC `fileDataID` and `e.what()`. The logged identifier differs — JS reports the animation's logical `id`, C++ reports the file data ID. Message format also differs.

- [ ] 368. [M2Loader.cpp] `parseChunk_SFID` guard check uses `viewCount == 0` instead of undefined-check
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 272–273
- **Status**: Pending
- **Details**: JS checks `if (this.viewCount === undefined)` — true only when MD21 hasn't been parsed yet (property never assigned). C++ checks `if (this->viewCount == 0)`. Since the C++ member is default-initialized to 0, this would throw even AFTER MD21 sets viewCount to a legitimate 0 (a model with no views). The JS would NOT throw in that case because viewCount would be defined as 0 (`0 !== undefined`). The guard should track whether MD21 has been parsed (e.g., using a bool flag), not check for `viewCount == 0`.

- [ ] 369. [M2Loader.cpp] `parseChunk_TXID` guard check uses `textures.empty()` instead of undefined-check
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 290–291
- **Status**: Pending
- **Details**: JS checks `if (this.textures === undefined)` — true only when MD21 hasn't been parsed (textures array never created). C++ checks `if (this->textures.empty())`. If MD21 parses 0 textures, JS would NOT throw (textures is a defined empty array), but C++ WOULD throw. Same semantic issue as the SFID check — should track MD21 parse state rather than vector emptiness.

- [ ] 370. [MDXLoader.cpp] ATCH handler fixes JS `readUInt32LE(-4)` bug without TODO_TRACKER documentation
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` line 404
- **Status**: Pending
- **Details**: JS ATCH handler has `this.data.readUInt32LE(-4)` which is a bug — `BufferWrapper._readInt` passes `_checkBounds(-16)` (always passes since remainingBytes >= 0 > -16), but `new Array(-4)` throws a `RangeError`. C++ correctly fixes this by using a saved `attachmentSize` variable. The fix has a code comment but per project conventions, deviations from the original JS should also be tracked in TODO_TRACKER.md.

- [ ] 371. [MDXLoader.cpp] Node registration deferred to post-parsing (structural deviation)
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` lines 208–209
- **Status**: Pending
- **Details**: In JS, `_read_node()` immediately assigns `this.nodes[node.objectId] = node` (line 209). In C++, this is deferred to `load()` because objects are moved into their final vectors after `_read_node` returns, invalidating any earlier pointers. This is correctly documented with a code comment and is functionally equivalent — all 9 node-bearing types (bones, helpers, attachments, eventObjects, hitTestShapes, particleEmitters, particleEmitters2, lights, ribbonEmitters) are properly registered. This is a structural deviation that should be tracked.

- [ ] 372. [SKELLoader.cpp] Extra bounds check in `loadAnimsForIndex()` not present in JS
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 308–312
- **Status**: Pending
- **Details**: C++ adds `if (animation_index >= this->animations.size()) return false;` that does not exist in JS. In JS, accessing an out-of-bounds index on `this.animations` returns `undefined`, and `animation.flags` would throw a TypeError. C++ silently returns false instead of throwing, changing error behavior.

- [ ] 373. [SKELLoader.cpp] `skeletonBoneData` existence check uses `.empty()` instead of `!== undefined`
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 335–338, 441–444
- **Status**: Pending
- **Details**: JS checks `loader.skeletonBoneData !== undefined` — the property only exists if a SKID chunk was parsed. C++ checks `!loader->skeletonBoneData.empty()`. If ANIMLoader ever sets `skeletonBoneData` to a valid but empty buffer, JS would use it (property exists), but C++ would skip it (empty). This is a potential semantic difference depending on ANIMLoader behavior.

- [ ] 374. [SKELLoader.cpp] `loadAnims()` doesn't guard against missing `animFileIDs` like `loadAnimsForIndex()` does
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 319, 425
- **Status**: Pending
- **Details**: JS `loadAnimsForIndex()` has `if (!this.animFileIDs) return false;` (line 319) to guard against undefined `animFileIDs`. However, JS `loadAnims()` does NOT have this guard — it directly iterates `this.animFileIDs` (line 425), which would throw a TypeError if undefined. In C++, `animFileIDs` is always a default-constructed empty vector, so the for-loop is a no-op. The C++ is more robust but produces different behavior (graceful no-op vs JS crash).

- [ ] 375. [WDTLoader.cpp] `worldModelPlacement`/`worldModel`/MPHD fields not optional — cannot distinguish "chunk absent" from "chunk with zeros"
- **JS Source**: `src/js/3D/loaders/WDTLoader.js` lines 52–103
- **Status**: Pending
- **Details**: In JS, `this.worldModelPlacement` is only assigned when MODF is encountered. If MODF is absent, the property is `undefined` and `if (wdt.worldModelPlacement)` is false. In C++, `WDTWorldModelPlacement worldModelPlacement` is always default-constructed with zeroed fields, making it impossible to distinguish "MODF absent" from "MODF with zeros." Same for `worldModel` (always empty string vs. JS `undefined`) and MPHD fields (always 0 vs. JS `undefined`). Consider `std::optional<T>` for these fields.

- [ ] 376. [WMOLegacyLoader.cpp] MOGP `flags` field renamed to `groupFlags`, diverging from JS property name
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` line 453
- **Status**: Pending
- **Details**: JS MOGP handler stores `this.flags = data.readUInt32LE()`. C++ stores this as `this->groupFlags` (header line 124, MOGP parser line 527) because the C++ class already has `uint16_t flags` from MOHD. Any downstream JS-ported code accessing `group.flags` for MOGP flags must use `group.groupFlags` in C++, which is a naming deviation that could cause porting bugs.

- [ ] 377. [WMOLegacyLoader.cpp] `getGroup` empty-check differs for `groupCount == 0` edge case
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 117–118
- **Status**: Pending
- **Details**: JS checks `if (!this.groups)` — tests whether the `groups` property was ever set (by MOHD handler). An empty JS array `new Array(0)` is truthy, so `!this.groups` is false when `groupCount == 0` — `getGroup` proceeds to the index check. C++ uses `if (this->groups.empty())` which returns true for `groupCount == 0`, incorrectly throwing the exception. A separate bool flag (e.g., `groupsInitialized`) would replicate JS semantics more faithfully.

- [ ] 378. [WMOLoader.cpp] `getGroup()` passes `groupFileID` to child constructor instead of no fileID
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` line 80
- **Status**: Pending
- **Details**: JS creates group `WMOLoader` with `undefined` as fileID: `new WMOLoader(data, undefined, this.renderingOnly)`. The group's `fileDataID` and `fileName` are intentionally unset. C++ passes the actual `groupFileID`, triggering an unnecessary `casc::listfile::getByID()` lookup in the constructor and setting `fileDataID`/`fileName` on the group. The constructor should use fileID=0 (C++ sentinel for "undefined") to match JS.

- [ ] 379. [WMOLoader.cpp] MOGP `flags` field renamed to `groupFlags`, diverging from JS property name
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` line 361
- **Status**: Pending
- **Details**: JS MOGP handler stores `this.flags = data.readUInt32LE()`. C++ stores this as `this->groupFlags` (header line 218, MOGP parser line 426) because the C++ class already has `uint16_t flags` from MOHD. Any downstream code porting JS that accesses `wmoGroup.flags` for MOGP flags must use `groupFlags` in C++. This naming deviation matches the same issue found in WMOLegacyLoader.cpp (entry 376).

- [ ] 380. [WMOLoader.cpp] `hasLiquid` boolean is a C++ addition not present in JS
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 328–338
- **Status**: Pending
- **Details**: JS simply assigns `this.liquid = { ... }` in the MLIQ handler. Consumer code checks `if (this.liquid)` for existence. In C++, the `WMOLiquid liquid` member is always default-constructed, so a `bool hasLiquid = false` flag (header line 209) was added and set to `true` in `parse_MLIQ`. This is a reasonable C++ adaptation, but all downstream JS code that checks `if (this.liquid)` must be ported to check `if (this.hasLiquid)` instead — all consumers need verification.

- [ ] 381. [WMOLoader.cpp] MOPR filler skip uses `data.move(4)` but per wowdev.wiki entry is 8 bytes total
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 208–216
- **Status**: Pending
- **Details**: MOPR entry count is calculated as `chunkSize / 8` (8 bytes per entry). Fields read: `portalIndex(2) + groupIndex(2) + side(2)` = 6 bytes, then `data.move(4)` skips 4 more = 10 bytes per entry. Per wowdev.wiki, `SMOPortalRef` has a 2-byte `filler` (uint16_t), making entries 8 bytes. `data.move(2)` would be correct, not `data.move(4)`. Both JS and C++ match (C++ faithfully ports the JS), but both overread by 2 bytes per entry. The outer `data.seek(nextChunkPos)` corrects the position so parsing doesn't break, but this is a latent bug in both codebases.

- [ ] 382. [CharMaterialRenderer.cpp] `getCanvas()` method missing — JS returns `this.glCanvas` for external use
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 55–59
- **Status**: Pending
- **Details**: JS `getCanvas()` returns the canvas element so external code can access the rendered character material texture. C++ has no equivalent method. Any code that calls `getCanvas()` will fail.

- [ ] 383. [CharMaterialRenderer.cpp] `update()` draw call placement differs — C++ draws inside blend-mode conditional instead of after it
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 382–417
- **Status**: Pending
- **Details**: JS draws ONCE per layer at line 417 (`this.gl.drawArrays(this.gl.TRIANGLES, 0, 6)`) OUTSIDE the blend-mode 4/6/7 if block. C++ has the draw call INSIDE the if block (for blend modes 4/6/7) at line ~534 AND inside the else block at line ~543. This means the draw happens in both branches but the pre-draw setup is different, which could lead to incorrect rendering for certain blend modes.

- [ ] 384. [CharMaterialRenderer.cpp] `setTextureTarget()` signature completely changed — JS takes full objects, C++ takes individual scalar parameters
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 114–144
- **Status**: Pending
- **Details**: JS signature is `setTextureTarget(chrCustomizationMaterial, charComponentTextureSection, chrModelMaterial, chrModelTextureLayer, useAlpha, blpOverride)` receiving full objects. C++ takes individual fields: `setTextureTarget(chrModelTextureTargetID, fileDataID, sectionX, sectionY, sectionWidth, sectionHeight, materialTextureType, materialWidth, materialHeight, textureLayerBlendMode, useAlpha, blpOverride)`. If JS objects contain additional fields used downstream, C++ will lose them.

- [ ] 385. [CharMaterialRenderer.cpp] `clearCanvas()` binds/unbinds FBO in C++ but JS does not
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 218–225
- **Status**: Pending
- **Details**: JS `clearCanvas()` operates on the current WebGL framebuffer (the canvas) without explicit bind/unbind. C++ explicitly binds `fbo_` before clearing and unbinds after. This is architecturally correct for desktop GL but represents a behavioral difference if called while another FBO is bound.

- [ ] 386. [CharMaterialRenderer.cpp] `dispose()` missing WebGL context loss equivalent
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` line 160
- **Status**: Pending
- **Details**: JS calls `gl.getExtension('WEBGL_lose_context').loseContext()` to invalidate all WebGL resources at once. C++ manually deletes each GL resource individually (FBO, textures, depth buffer, VAO). The C++ approach is correct for desktop GL but the order and completeness of cleanup should be verified.

- [ ] 387. [GridRenderer.cpp] GLSL shader version differs — C++ uses `#version 460 core`, JS uses `#version 300 es`
- **JS Source**: `src/js/3D/renderers/GridRenderer.js` lines 9–35
- **Status**: Pending
- **Details**: C++ vertex/fragment shaders use `#version 460 core` (desktop OpenGL 4.6). JS uses `#version 300 es` with `precision highp float;` (WebGL 2.0/OpenGL ES 3.0). The shader logic is identical but the version/precision declarations differ. This is an expected platform adaptation but should be documented.

- [ ] 388. [M2LegacyRendererGL.cpp] Reactive watchers not set up — `geosetWatcher` and `wireframeWatcher` completely missing
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 196–197
- **Status**: Pending
- **Details**: JS `load()` sets up Vue reactive watchers: `this.geosetWatcher = core.view.$watch(this.geosetKey, () => this.updateGeosets(), { deep: true })` and `this.wireframeWatcher = core.view.$watch('config.modelViewerWireframe', () => {}, { deep: true })`. C++ has an empty `if (reactive) {}` block at lines 216–217. Geoset changes from the UI will not trigger `updateGeosets()`. No polling replacement exists.

- [ ] 389. [M2LegacyRendererGL.cpp] `dispose()` missing watcher cleanup calls
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 1038–1039
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.geosetWatcher?.()` and `this.wireframeWatcher?.()` to unregister Vue watchers. C++ `dispose()` has no equivalent cleanup because watchers were never set up. If a polling mechanism is later added, cleanup must be added here too.

- [ ] 390. [M2LegacyRendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` line 926
- **Status**: Pending
- **Details**: JS sets `u_time` to `performance.now() * 0.001` (absolute time from page load in seconds). C++ uses a static `std::chrono::steady_clock` start time and computes elapsed seconds from first render call. Both produce monotonically increasing values suitable for shader animations, but absolute values will differ, potentially affecting time-dependent shader effects.

- [ ] 391. [M2LegacyRendererGL.cpp] Track data property names differ from JS — uses `flatValues`/`nestedTimestamps` instead of `values`/`timestamps`
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 581–609
- **Status**: Pending
- **Details**: JS accesses bone animation data as `bone.translation.values`, `bone.translation.timestamps`, `bone.translation.timestamps[anim_idx]`, `bone.translation.values[anim_idx]`. C++ accesses `bone.translation.flatValues`, `bone.translation.flatTimestamps`, `bone.translation.nestedTimestamps[anim_idx]`, `bone.translation.nestedValues[anim_idx]`. This implies the M2LegacyLoader stores data in a different structure, which must match the renderer's expectations.

- [ ] 392. [M2LegacyRendererGL.cpp] `loadSkin()` geoset assignment to `core.view` uses JSON serialization instead of direct assignment
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 431–432
- **Status**: Pending
- **Details**: JS directly assigns `core.view[this.geosetKey] = this.geosetArray` and passes `this.geosetArray` to `GeosetMapper.map()`. C++ builds nlohmann::json objects manually from `geosetArray` entries (lines 490–498) and constructs a separate `mapper_geosets` vector for GeosetMapper (lines 500–507), then updates labels back. This indirect approach may not synchronize correctly if core.view expects the raw array reference.

- [ ] 393. [M2LegacyRendererGL.cpp] `setSlotFile` called as `setSlotFileLegacy` — function name differs from JS
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` line 226
- **Status**: Pending
- **Details**: JS calls `textureRibbon.setSlotFile(ribbonSlot, fileName, this.syncID)`. C++ calls `texture_ribbon::setSlotFileLegacy(ribbonSlot, fileName, syncID)` at line 255. The C++ function has a different name, which may indicate it has different behavior or was renamed for disambiguation.

- [ ] 394. [M2RendererGL.cpp] Reactive watchers not set up — `geosetWatcher`, `wireframeWatcher`, `bonesWatcher` completely missing
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 381–383
- **Status**: Pending
- **Details**: JS `load()` sets up three Vue watchers for geoset, wireframe, and bone visibility. C++ has empty braces at lines 495–496 with no watcher registration. UI changes to geosets won't trigger `updateGeosets()` automatically.

- [ ] 395. [M2RendererGL.cpp] `dispose()` missing watcher cleanup — no `geosetWatcher`, `wireframeWatcher`, `bonesWatcher` unregister
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1629–1633
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.geosetWatcher?.()`, `this.wireframeWatcher?.()`, `this.bonesWatcher?.()` to unregister watchers. C++ has no equivalent cleanup because watchers were never created.

- [ ] 396. [M2RendererGL.cpp] Bone matrix upload uses SSBO instead of uniform array
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1228–1231
- **Status**: Pending
- **Details**: JS uploads bone matrices via `gl.uniformMatrix4fv(loc, false, this.bone_matrices)` (uniform array). C++ uses Shader Storage Buffer Objects (SSBOs) at lines 1482–1495 (`glBindBuffer(GL_SHADER_STORAGE_BUFFER, ...)`, `glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ...)`). This is a valid modern OpenGL optimization but requires the shader to declare an SSBO binding instead of a uniform array. If the shader still expects a uniform array, bone animation will not work.

- [ ] 397. [M2RendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` line 1224
- **Status**: Pending
- **Details**: Same issue as M2LegacyRendererGL — JS uses `performance.now() * 0.001`, C++ uses elapsed time from first render call (lines 1477–1480).

- [ ] 398. [M2RendererGL.cpp] `overrideTextureTypeWithCanvas()` takes raw pixel data instead of canvas element
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1371–1390
- **Status**: Pending
- **Details**: JS takes `HTMLCanvasElement canvas` and calls `gl_tex.set_canvas(canvas, {...})`. C++ takes `const uint8_t* pixels, int width, int height` and calls `gl_tex->set_canvas(pixels, width, height, opts)`. This is an expected platform adaptation but the interface change means all callers must provide raw pixel data instead of a canvas reference.

- [ ] 399. [M3RendererGL.cpp] `getBoundingBox()` missing vertex array empty check
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 174–175
- **Status**: Pending
- **Details**: JS checks `if (!this.m3 || !this.m3.vertices) return null`. C++ only checks `if (!m3) return std::nullopt` at line 198–199 without checking if vertices array is empty. If m3 is loaded but vertices array is empty, C++ will attempt bounding box calculation on empty data.

- [ ] 400. [M3RendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` line 214
- **Status**: Pending
- **Details**: Same issue as M2RendererGL/M2LegacyRendererGL — C++ uses `std::chrono::steady_clock` elapsed time (lines 242–246) instead of `performance.now() * 0.001`.

- [ ] 401. [MDXRendererGL.cpp] Reactive watchers not set up — `geosetWatcher` and `wireframeWatcher` completely missing
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 187–188
- **Status**: Pending
- **Details**: JS `load()` sets up Vue watchers: `this.geosetWatcher = core.view.$watch(this.geosetKey, () => this.updateGeosets(), { deep: true })` and `this.wireframeWatcher = core.view.$watch('config.modelViewerWireframe', () => {}, { deep: true })`. C++ completely omits these watchers. Comment at lines 228–229 states "polling is handled in render()." but no polling code exists.

- [ ] 402. [MDXRendererGL.cpp] `dispose()` missing watcher cleanup calls
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 780–781
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.geosetWatcher?.()` and `this.wireframeWatcher?.()`. C++ has no equivalent cleanup because watchers were never created.

- [ ] 403. [MDXRendererGL.cpp] `_create_skeleton()` doesn't initialize `node_matrices` to identity when nodes are empty
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 252
- **Status**: Pending
- **Details**: JS sets `this.node_matrices = new Float32Array(16)` which creates a zero-filled 16-element array (single identity-sized buffer). C++ does `node_matrices.resize(16)` at line 313 which leaves elements uninitialized. Should zero-initialize or set to identity to match JS behavior.

- [ ] 404. [MDXRendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 681
- **Status**: Pending
- **Details**: Same issue as other renderers — C++ uses elapsed time from first render call instead of `performance.now() * 0.001`.

- [ ] 405. [MDXRendererGL.cpp] Interpolation constants `INTERP_NONE/LINEAR/HERMITE/BEZIER` defined but never used in either JS or C++
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 27–30
- **Status**: Pending
- **Details**: Both files define `INTERP_NONE=0`, `INTERP_LINEAR=1`, `INTERP_HERMITE=2`, `INTERP_BEZIER=3` but neither uses them. The `_sample_vec3()` and `_sample_quat()` methods only implement linear interpolation (lerp/slerp), never checking interpolation type. Hermite and Bezier interpolation are not implemented in either codebase.

- [ ] 406. [MDXRendererGL.cpp] `_build_geometry()` VAO setup passes 5 params instead of 6 — JS passes `null` as 6th parameter
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 368
- **Status**: Pending
- **Details**: JS calls `vao.setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo, null)` with 6 parameters (last is null for index buffer). C++ calls `vao->setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo)` with only 5 parameters. The 6th parameter (index/element buffer) is missing in C++.

- [ ] 407. [ShadowPlaneRenderer.cpp] GLSL shader version differs — C++ uses `#version 460 core`, JS uses `#version 300 es`
- **JS Source**: `src/js/3D/renderers/ShadowPlaneRenderer.js` lines 9–40
- **Status**: Pending
- **Details**: Same issue as GridRenderer — C++ uses desktop GL 4.6 shaders, JS uses WebGL 2.0 shaders. Logic is identical but version/precision declarations differ. Expected platform adaptation.

- [ ] 408. [WMOLegacyRendererGL.cpp] Reactive watchers replaced with polling — `groupWatcher`, `setWatcher`, `wireframeWatcher` not created
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 91–93
- **Status**: Pending
- **Details**: JS sets up three Vue watchers in `load()`. C++ replaces these with manual per-frame polling in `render()` (lines 517–551), comparing current state against `prev_group_checked`/`prev_set_checked` arrays. This is functionally equivalent but architecturally different — watchers are event-driven, polling is frame-driven with potential one-frame delay.

- [ ] 409. [WMOLegacyRendererGL.cpp] Texture wrap flag logic potentially inverted
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 146–147
- **Status**: Pending
- **Details**: JS sets `wrap_s = (material.flags & 0x40) ? gl.CLAMP_TO_EDGE : gl.REPEAT` and `wrap_t = (material.flags & 0x80) ? gl.CLAMP_TO_EDGE : gl.REPEAT`. C++ creates `BLPTextureFlags` with `wrap_s = !(material.flags & 0x40)` at line 184–185. The boolean negation may invert the wrap behavior — if `true` maps to CLAMP in the BLPTextureFlags API, then `!(flags & 0x40)` produces the opposite of what JS does. Need to verify the BLPTextureFlags API to confirm.

- [ ] 410. [WMORendererGL.cpp] Reactive watchers replaced with polling — `groupWatcher`, `setWatcher`, `wireframeWatcher` not created
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 105–107
- **Status**: Pending
- **Details**: Same approach as WMOLegacyRendererGL — JS uses Vue watchers, C++ uses per-frame polling in `render()` (lines 643–676). Architecturally different but functionally equivalent with potential one-frame delay.

- [ ] 411. [WMORendererGL.cpp] `_load_textures()` `isClassic` check differs — JS tests `!!wmo.textureNames` (truthiness), C++ tests `!wmo->textureNames.empty()`
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` line 126
- **Status**: Pending
- **Details**: JS `!!wmo.textureNames` is true if the property exists and is truthy (even an empty array `[]` is truthy). C++ `!wmo->textureNames.empty()` is only true if the map has entries. If a WMO has the texture names chunk but it's empty, JS enters classic mode but C++ does not. Comment at C++ line 140–143 acknowledges this.

- [ ] 412. [WMORendererGL.cpp] `get_wmo_groups_view()`/`get_wmo_sets_view()` accessor methods don't exist in JS — C++ addition for multi-viewer support
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 64–65, 103–104
- **Status**: Pending
- **Details**: JS uses `view[this.wmoGroupKey]` and `view[this.wmoSetKey]` for dynamic property access. C++ implements `get_wmo_groups_view()` and `get_wmo_sets_view()` methods (lines 60–69) that return references to the appropriate core::view member based on the key string, supporting `modelViewerWMOGroups`, `creatureViewerWMOGroups`, and `decorViewerWMOGroups`. This is a valid C++ adaptation of JS's dynamic property access.

- [ ] 413. [CSVWriter.cpp] `addField()` overloaded — JS uses variadic `...fields`, C++ has two overloads (single string and vector)
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 25–27
- **Status**: Pending
- **Details**: JS `addField(...fields)` accepts any number of arguments via rest parameters and pushes all. C++ provides `addField(const std::string&)` for single fields and `addField(const std::vector<std::string>&)` for multiple. Callers must adapt to one of these two signatures instead of passing multiple individual arguments.

- [ ] 414. [CSVWriter.cpp] `escapeCSVField()` handles `null`/`undefined` differently — JS converts via `.toString()`, C++ returns empty for empty string
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 42–51
- **Status**: Pending
- **Details**: JS `escapeCSVField()` handles `null`/`undefined` by returning empty string (line 43–44), then calls `value.toString()` for other types. C++ only accepts `const std::string&` and returns empty for empty strings (line 28–29). JS could receive numbers/booleans and stringify them; C++ requires pre-conversion to string by the caller.

- [ ] 415. [CSVWriter.cpp] `write()` default parameter differs — JS defaults `overwrite = true`, C++ has no default
- **JS Source**: `src/js/3D/writers/CSVWriter.js` line 57
- **Status**: Pending
- **Details**: JS `async write(overwrite = true)` defaults to overwriting. C++ `void write(bool overwrite)` has no default value. Callers must always explicitly pass the overwrite flag in C++.

- [ ] 416. [GLBWriter.cpp] GLB JSON chunk padding fills with NUL (0x00) instead of spaces (0x20) as required by the glTF 2.0 spec
- **JS Source**: `src/js/3D/writers/GLBWriter.js` lines 20–28
- **Status**: Pending
- **Details**: The glTF 2.0 spec requires that the JSON chunk be padded with trailing space characters (0x20) to maintain 4-byte alignment. C++ `BufferWrapper::alloc(size, true)` zero-fills the buffer, so JSON padding bytes are 0x00. JS `Buffer.alloc(size)` also zero-fills, so JS has the same issue. However, this should be documented as a potential spec compliance issue for both versions.

- [ ] 417. [GLBWriter.cpp] Binary chunk padding uses zero bytes, matching JS behavior correctly
- **JS Source**: `src/js/3D/writers/GLBWriter.js` lines 29–36
- **Status**: Pending
- **Details**: Both JS and C++ use zero bytes (0x00) for BIN chunk padding. The glTF 2.0 spec requires BIN chunks to be padded with NUL (0x00), so this is correct. No issue here, verified as correct.

- [ ] 418. [GLTFWriter.cpp] `add_scene_node` returns size_t index in C++ but the JS function returns the node object itself
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 276–280
- **Status**: Pending
- **Details**: JS `add_scene_node` returns the pushed node object reference (used for `skeleton.children.push()` later). C++ returns the index (size_t) instead, and uses index-based access to modify nodes later. This is functionally equivalent but bone parent lookup uses `bone_lookup_map[bone.parentBone]` to store the node index in C++ vs. storing the node object reference in JS. This difference means C++ accesses `nodes[parent_node_idx]` while JS mutates the object directly.

- [ ] 419. [GLTFWriter.cpp] `add_buffered_accessor` lambda omits `target` from bufferView when `buffer_target < 0` in C++, JS passes `undefined` which is serialized differently
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 282–296
- **Status**: Pending
- **Details**: JS `add_buffered_accessor` always includes `target: buffer_target` in the bufferView. When `buffer_target` is `undefined`, JSON.stringify omits the key entirely. C++ explicitly checks `if (buffer_target >= 0)` before adding the target key. This produces identical JSON output since JS `undefined` values are omitted by JSON.stringify, matching C++ not adding the key at all. Functionally equivalent.

- [ ] 420. [GLTFWriter.cpp] Animation channel target node uses `actual_node_idx` (variable per prefix setting) but JS always uses `nodeIndex + 1`
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 620–628, 757–765, 887–895
- **Status**: Pending
- **Details**: In JS, animation channel target node is always `nodeIndex + 1` regardless of prefix setting. In C++, `actual_node_idx` is used, which varies based on `usePrefix`. When `usePrefix` is true, C++ sets `actual_node_idx = nodes.size()` after pushing prefix_node (so it points to the real bone node, matching JS `nodeIndex + 1`). When `usePrefix` is false, `actual_node_idx = nodes.size()` before pushing the node, so it points to the same node. The JS code always does `nodeIndex + 1` which is only correct when prefix nodes exist. C++ correctly handles both cases. This is a JS bug that C++ fixes intentionally.

- [ ] 421. [GLTFWriter.cpp] `bone_lookup_map` stores index-to-index mapping using `std::map<int, size_t>` instead of JS Map storing index-to-object
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` line 464
- **Status**: Pending
- **Details**: JS `bone_lookup_map.set(bi, node)` stores the node object, which is then mutated later when children are added. C++ `bone_lookup_map[bi] = actual_node_idx` stores the index into the `nodes` array, and children are added via `nodes[parent_node_idx]["children"]`. This is functionally equivalent — JS mutates the object reference in the map and C++ indexes into the JSON array.

- [ ] 422. [GLTFWriter.cpp] Mesh primitive always includes `material` property in JS even when `materialMap.get()` returns `undefined`, C++ conditionally omits it
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1110–1119
- **Status**: Pending
- **Details**: JS always sets `material: materialMap.get(mesh.matName)` in the primitive, even if the material isn't found (result is `undefined`, which gets stripped by JSON.stringify). C++ uses `auto mat_it = materialMap.find(mesh.matName)` and only sets `primitive["material"]` if found. The final JSON output is identical since JS undefined is omitted, but the approach differs.

- [ ] 423. [GLTFWriter.cpp] Equipment mesh primitive always includes `material` in JS; C++ conditionally includes it
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1404–1411
- **Status**: Pending
- **Details**: Same pattern as entry 422 but for equipment meshes. JS sets `material: materialMap.get(mesh.matName)` which may be `undefined`. C++ checks `eq_mat_it != materialMap.end()` before setting material. Functionally equivalent in JSON output.

- [ ] 424. [GLTFWriter.cpp] `addTextureBuffer` method does not exist in JS — C++ addition
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` (no equivalent)
- **Status**: Pending
- **Details**: C++ adds `addTextureBuffer(uint32_t fileDataID, BufferWrapper buffer)` method (lines 113–115) which has no JS counterpart. JS only has `setTextureBuffers()` to set the entire map at once. The C++ addition allows incrementally adding individual texture buffers, which changes the API surface.

- [ ] 425. [GLTFWriter.cpp] Animation buffer name extraction in glb mode uses `rfind('_')` to extract `anim_idx`, but JS uses `split('_')` to get index at position 3
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1468–1470
- **Status**: Pending
- **Details**: JS extracts animation index from bufferView name via `name_parts = bufferView.name.split('_'); anim_idx = name_parts[3]`. C++ uses `bv_name.rfind('_')` and then `std::stoi(bv_name.substr(last_underscore + 1))` to get the animation index. For names like `TRANS_TIMESTAMPS_0_1`, JS gets `name_parts[3] = "1"`, C++ gets substring after last underscore = `"1"`. These produce the same result. However, for `SCALE_TIMESTAMPS_0_1`, both work the same. Functionally equivalent.

- [ ] 426. [GLTFWriter.cpp] `skeleton` variable in JS is a node object reference, C++ is a node index
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 344–347, 449
- **Status**: Pending
- **Details**: JS `const skeleton = add_scene_node({name: ..., children: []})` returns the actual node object. Later, `skeleton.children.push(nodeIndex)` mutates it directly. C++ `size_t skeleton_idx = add_scene_node(...)` gets an index, and later accesses `nodes[skeleton_idx]["children"].push_back(...)`. Functionally equivalent.

- [ ] 427. [GLTFWriter.cpp] `usePrefix` is read inside the bone loop instead of outside like JS
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 460, 466
- **Status**: Pending
- **Details**: JS checks `core.view.config.modelsExportWithBonePrefix` outside the bone loop at line 460 (const is evaluated once). C++ reads `core::view->config.value("modelsExportWithBonePrefix", false)` inside the loop at line 470, which re-reads the config for every bone. Since config shouldn't change during export, this is functionally equivalent but slightly less efficient.

- [ ] 428. [JSONWriter.cpp] `write()` uses `dump(1, '\t')` for pretty-printing; JS uses `JSON.stringify(data, null, '\t')`
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 37–42
- **Status**: Pending
- **Details**: Both produce tab-indented JSON, but nlohmann `dump(1, '\t')` uses indent width of 1 with tab character, while JS `JSON.stringify` with `'\t'` uses tab for each indent level. The output should be identical for well-formed JSON.

- [ ] 429. [JSONWriter.cpp] `write()` default parameter correctly matches JS `overwrite = true`
- **JS Source**: `src/js/3D/writers/JSONWriter.js` line 30
- **Status**: Pending
- **Details**: Both JS and C++ default `overwrite` to `true`. Verified as correct.

- [ ] 430. [MTLWriter.cpp] `material.name` extraction uses `std::filesystem::path(name).stem().string()` but JS uses `path.basename(name, path.extname(name))`
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 35–37
- **Status**: Pending
- **Details**: C++ line 30 uses `std::filesystem::path(name).stem().string()` to extract the filename without extension. JS uses `path.basename(name, path.extname(name))`. These should produce identical results for typical filenames. However, if `name` contains multiple dots (e.g., `texture.v2.png`), `stem()` returns `texture.v2` while `basename('texture.v2.png', '.png')` also returns `texture.v2`. Functionally equivalent.

- [ ] 431. [MTLWriter.cpp] MTL file uses `map_Kd` texture directive correctly matching JS
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 38–39
- **Status**: Pending
- **Details**: Both JS and C++ write `map_Kd <file>` for diffuse texture mapping in material definitions. Verified as correct.

- [ ] 432. [OBJWriter.cpp] `appendGeometry` UV handling differs — JS uses `Array.isArray`/spread, C++ uses `insert`
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 84–99
- **Status**: Pending
- **Details**: JS `appendGeometry` handles multiple UV arrays and uses `Array.isArray` + spread operator for concatenation. C++ uses `std::vector::insert` for appending. Functionally equivalent.

- [ ] 433. [OBJWriter.cpp] Face output format uses 1-based indexing with `v[i+1]//vn[i+1]` or `v[i+1]/vt[i+1]/vn[i+1]` — matches JS correctly
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 119–142
- **Status**: Pending
- **Details**: Both JS and C++ output 1-based vertex indices in OBJ face format (e.g., `f v//vn v//vn v//vn` when no UVs, `f v/vt/vn v/vt/vn v/vt/vn` when UVs present). Vertex offset is added correctly in both implementations. Verified as correct.

- [ ] 434. [OBJWriter.cpp] Only first UV set is written in OBJ faces; JS `this.uvs[0]` matches C++ `uvs[0]`
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 119, 130–131
- **Status**: Pending
- **Details**: Both JS and C++ check `this.uvs[0]` / `uvs[0]` for the first UV set when determining whether to include UV indices in face output. Only the first UV set is used in OBJ face references. Verified as matching.

- [ ] 435. [SQLWriter.cpp] `addField()` overloaded — JS uses variadic `...fields`, C++ has two overloads (single string and vector)
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 48–49
- **Status**: Pending
- **Details**: JS `addField(...fields)` accepts any number of arguments via rest parameters and pushes all. C++ provides `addField(const std::string&)` for single fields and `addField(const std::vector<std::string>&)` for multiple. Same pattern as CSVWriter entry 413.

- [ ] 436. [SQLWriter.cpp] `generateDDL()` output format differs slightly — C++ builds strings directly, JS uses `lines.join('\n')`
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 141–177
- **Status**: Pending
- **Details**: JS builds an array of `lines` and joins with `\n` at the end. The output includes `DROP TABLE IF EXISTS ...\n\nCREATE TABLE ... (\n<columns>\n);\n\n`. C++ builds the result string directly with `+= "\n"`. The C++ version outputs `DROP TABLE IF EXISTS ...;\n\nCREATE TABLE ... (\n<columns joined with ,\n>\n);\n` which should match. However, JS `lines.push('')` creates an empty element that adds an extra `\n` when joined, and the column_defs are joined separately with `,\n`. The overall output may have subtle whitespace differences in the final string.

- [ ] 437. [SQLWriter.cpp] `toSQL()` format differs — JS uses `lines.join('\n')` with `value_rows.join(',\n') + ';'`, C++ concatenates directly
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 183–204
- **Status**: Pending
- **Details**: JS's `toSQL()` builds lines array and joins with `\n`. Each batch creates `INSERT INTO ... VALUES\n` then `(vals),(vals),...(vals);\n\n`. C++ directly concatenates: `INSERT INTO ... VALUES\n(vals),\n(vals);\n\n`. The difference is that JS joins value_rows with `,\n` (so no leading newline on first row), while C++ adds `,\n` as a separator between rows within the loop. The output format may differ — JS produces `(vals),(vals)\n(vals);` while C++ produces `(vals),\n(vals),\n(vals);\n`. Minor formatting difference in output.

- [ ] 438. [STLWriter.cpp] Header string says `wow.export.cpp` while JS says `wow.export` — intentional branding difference
- **JS Source**: `src/js/3D/writers/STLWriter.js` line 147
- **Status**: Pending
- **Details**: JS: `'Exported using wow.export v' + constants.VERSION`. C++: `"Exported using wow.export.cpp v" + std::string(constants::VERSION)`. This is an intentional branding change per project conventions (user-facing text should say wow.export.cpp). Verified as correct.

- [ ] 439. [STLWriter.cpp] `appendGeometry` simplified — C++ doesn't handle `Float32Array` vs `Array` distinction
- **JS Source**: `src/js/3D/writers/STLWriter.js` lines 66–86
- **Status**: Pending
- **Details**: JS `appendGeometry` checks `Array.isArray(this.verts)` to decide between spread and `Float32Array.from()` for concatenation. C++ always uses `std::vector::insert`, which works correctly regardless. The JS type distinction is a JS-specific concern that doesn't apply to C++. Functionally equivalent.

- [ ] 440. [blp.cpp] `getDataURL()` implementation differs — JS uses `toCanvas().toDataURL()`, C++ uses `toPNG()` then `BufferWrapper::getDataURL()`
- **JS Source**: `src/js/casc/blp.js` lines 94–96
- **Status**: Pending
- **Details**: JS `getDataURL()` creates an HTML canvas, draws the BLP to it, and calls `canvas.toDataURL()` which produces a `data:image/png;base64,...` string. C++ `getDataURL()` calls `toPNG()` to get PNG data, then `BufferWrapper::getDataURL()` to encode it as a data URL. The C++ approach produces the same data URL format but bypasses the canvas entirely. This is a documented deviation (comment at lines 100–103).

- [ ] 441. [blp.cpp] `toCanvas()` and `drawToCanvas()` methods not ported — browser-specific
- **JS Source**: `src/js/casc/blp.js` lines 103–117, 221–234
- **Status**: Pending
- **Details**: JS `toCanvas()` creates an HTML `<canvas>` element and draws the BLP onto it. `drawToCanvas()` takes an existing canvas and draws the BLP pixels using 2D context methods (`createImageData`, `putImageData`). These are browser-specific APIs with no C++ equivalent. The C++ port replaces these with `toPNG()`, `toBuffer()`, and `toUInt8Array()` which provide the same pixel data without canvas.

- [ ] 442. [blp.cpp] `dataURL` property initialized to `null` in JS constructor, C++ uses `std::optional<std::string>`
- **JS Source**: `src/js/casc/blp.js` line 86
- **Status**: Pending
- **Details**: JS sets `this.dataURL = null` in the BLPImage constructor. C++ declares `std::optional<std::string> dataURL` in the header which defaults to `std::nullopt`. The C++ `getDataURL()` method doesn't cache to this field (it relies on `BufferWrapper::getDataURL()` caching instead). The JS `getDataURL()` also doesn't set this field — it returns from `toCanvas().toDataURL()`. The `dataURL` field appears to be unused caching infrastructure in both versions.

- [ ] 443. [blp.cpp] `toWebP()` uses libwebp C API directly instead of JS `webp-wasm` module
- **JS Source**: `src/js/casc/blp.js` lines 157–182
- **Status**: Pending
- **Details**: JS uses `webp-wasm` npm module with `webp.encode(imgData, options)` for WebP encoding. C++ uses libwebp's C API directly (`WebPEncodeLosslessRGBA` / `WebPEncodeRGBA`). The JS `options` object `{ lossless: true }` or `{ quality: N }` maps to C++ separate code paths for quality == 100 (lossless) vs lossy. Functionally equivalent.

- [ ] 444. [blp.cpp] `_getCompressed()` DXT color interpolation uses integer division in C++ which may produce different rounding vs JS floating-point division
- **JS Source**: `src/js/casc/blp.js` lines 341–346
- **Status**: Pending
- **Details**: JS `colours[i + 8] = (c + d) / 2` and `colours[i + 8] = (2 * c + d) / 3` use JS number (float64) division. C++ uses integer division since `colours` is `int[]`. For even sums, `/2` is exact; for odd sums, C++ truncates toward zero while JS keeps the decimal. For `/3`, C++ truncates while JS produces exact fraction. When these float values are eventually stored as uint8 pixel values, JS truncates via typed array assignment while C++ truncates at division. The difference is at most 1 LSB for some pixel values.

- [ ] 445. [blp.cpp] DXT5 alpha interpolation uses `| 0` (bitwise OR zero) in JS to floor, C++ uses integer division which truncates toward zero
- **JS Source**: `src/js/casc/blp.js` lines 389, 395
- **Status**: Pending
- **Details**: JS `colours[i + 1] = (((5 - i) * a0 + i * a1) / 5) | 0` uses `| 0` to convert float to int32 (truncates toward zero). C++ uses plain integer division `alphaColours[i + 1] = ((5 - i) * a0 + i * a1) / 5` which also truncates toward zero (all values are int). These should produce identical results since all operands are non-negative integers.

- [ ] 446. [blte-reader.cpp] `_handleBlock` encrypted block catch-all swallows all non-EncryptionError exceptions silently
- **JS Source**: `src/js/casc/blte-reader.js` lines 203–216
- **Status**: Pending
- **Details**: JS encrypted block handler catches `EncryptionError` specifically and re-throws for other exceptions (the catch block only handles `e instanceof EncryptionError`). C++ has `catch (const EncryptionError&)` for encryption errors, but also has a bare `catch (...)` on line 197–198 that silently swallows all other exceptions. This means C++ silently ignores errors like decompression failures inside encrypted blocks, while JS would propagate them.

- [ ] 447. [blte-reader.cpp] `decodeAudio()` not ported — browser-specific Web Audio API
- **JS Source**: `src/js/casc/blte-reader.js` lines 337–340
- **Status**: Pending
- **Details**: JS `decodeAudio(context)` calls `this.processAllBlocks()` then `super.decodeAudio(context)` using the Web Audio API's `AudioContext.decodeAudioData()`. C++ has a comment (lines 279–281) noting this is browser-specific and uses miniaudio instead. The method is not implemented in C++ BLTEReader.

- [ ] 448. [blte-reader.cpp] `getDataURL()` caching differs — JS checks `this.dataURL` first, C++ always processes blocks
- **JS Source**: `src/js/casc/blte-reader.js` lines 346–353
- **Status**: Pending
- **Details**: JS `getDataURL()` checks `if (!this.dataURL)` first, and only processes blocks if no cached value exists. The `dataURL` property could be set externally. C++ always calls `processAllBlocks()` first, relying on `BufferWrapper::getDataURL()` for internal caching. If an external caller sets `dataURL` before calling `getDataURL()`, JS would return the externally-set value without processing blocks, while C++ always processes blocks first.

- [ ] 449. [blte-reader.cpp] `_decompressBlock` passes two bools to `readBuffer()` in JS but only one in C++
- **JS Source**: `src/js/casc/blte-reader.js` line 242
- **Status**: Pending
- **Details**: JS: `data.readBuffer(blockEnd - data.offset, true, true)` — passes two `true` flags (decompress=true, copy=true). C++ line 220: `data.readBuffer(blockEnd - data.offset(), true)` — passes only one `true` flag (decompress=true). The second flag in JS may control whether the data is copied. If C++'s `readBuffer` implementation doesn't need a copy flag (e.g., always copies), this is functionally equivalent. Otherwise, there could be a difference in memory ownership.

- [ ] 450. [blte-stream-reader.cpp] `createReadableStream()` not ported — Web Streams API specific
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 168–193
- **Status**: Pending
- **Details**: JS `createReadableStream()` returns a `ReadableStream` (Web Streams API) that progressively pulls blocks on demand and supports cancellation. This is browser-specific and has no direct C++ equivalent. The C++ header documents this deviation. The `streamBlocks()` callback-based approach provides similar functionality.

- [ ] 451. [blte-stream-reader.cpp] `streamBlocks()` changed from async generator to synchronous callback
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 199–202
- **Status**: Pending
- **Details**: JS `async *streamBlocks()` is an async generator that yields decoded blocks lazily. Consumers use `for await (const block of reader.streamBlocks())`. C++ `streamBlocks()` takes a callback `std::function<void(BufferWrapper&)>` and iterates eagerly through all blocks, invoking the callback for each. This changes the consumption pattern from lazy to eager.

- [ ] 452. [blte-stream-reader.cpp] `createBlobURL()` returns BufferWrapper instead of string URL
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 208–218
- **Status**: Pending
- **Details**: JS `createBlobURL()` creates a `Blob` with MIME type `'video/x-msvideo'` from all decoded blocks, then returns a URL string via `URLPolyfill.createObjectURL(blob)`. C++ `createBlobURL()` concatenates all decoded blocks into a single `BufferWrapper` and returns it (raw data, no URL, no MIME type). This is a significant API difference — callers expecting a URL string will not work with the C++ version.

- [ ] 453. [blte-stream-reader.cpp] Cache eviction uses `std::deque` for FIFO ordering vs JS `Map` insertion order
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 71–77
- **Status**: Pending
- **Details**: JS uses `Map.keys().next().value` to get the oldest entry (Maps maintain insertion order in JS). C++ uses a separate `std::deque<size_t> cacheOrder` alongside `std::unordered_map` because `std::unordered_map` doesn't maintain insertion order. Functionally equivalent LRU eviction.

- [ ] 454. [blte-stream-reader.cpp] `_decodeBlock` for compressed blocks passes one bool in C++ vs two in JS
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 109–110
- **Status**: Pending
- **Details**: JS: `blockData.readBuffer(blockData.remainingBytes, true, true)` passes two `true` args to `readBuffer`. C++ line 75: `blockData.readBuffer(blockData.remainingBytes(), true)` passes only one. Same issue as entry 449 — the second bool flag for copy behavior may or may not be needed depending on BufferWrapper implementation.

- [ ] 455. [build-cache.cpp] `saveCacheIntegrity()` silently ignores file write failures
- **JS Source**: `src/js/casc/build-cache.js` line 144
- **Status**: Pending
- **Details**: JS `await fsp.writeFile(constants.CACHE.INTEGRITY_FILE, JSON.stringify(cacheIntegrity), 'utf8')` throws if the file cannot be written (e.g., disk full, permission denied). C++ `saveCacheIntegrity()` (line 149–153) checks `ofs.is_open()` and silently does nothing if the file cannot be opened. This means integrity data could be lost without any error or log message in C++, whereas JS would propagate the error to the caller.

- [ ] 456. [casc-source-local.cpp] `getProductList()` handles missing Branch field gracefully instead of throwing like JS
- **JS Source**: `src/js/casc/casc-source-local.js` line 152
- **Status**: Pending
- **Details**: JS `entry.Branch.toUpperCase()` accesses `entry.Branch` directly. If the build info entry has no `Branch` field, JS would access `undefined` and `.toUpperCase()` would throw a TypeError, causing that product to fail to be listed. C++ (lines 223–229) checks `branchIt != entry.end()` first and uses an empty string as the fallback. This means C++ would include the product with empty parentheses in the label (e.g., `"Retail () 10.2.7"`), while JS would crash and omit it (or propagate the error). In practice, the Branch field is always present in `.build.info` files, so this is a theoretical difference.

- [ ] 457. [db2.cpp] Extra `clearCache()` function added that does not exist in original JS module
- **JS Source**: `src/js/casc/db2.js` (entire file)
- **Status**: Pending
- **Details**: C++ `db2::clearCache()` (line 85–87 in db2.cpp, declared in db2.h line 58) clears the entire table cache, releasing all WDCReader instances. The original JS module exports only `db2_proxy` (the Proxy object) with its `.preload` property — there is no `clearCache` or equivalent cleanup function. This is additional API surface not present in the JS source.

- [ ] 458. [version-config.cpp] Extra data fields beyond header count silently discarded instead of creating JS `undefined` key
- **JS Source**: `src/js/casc/version-config.js` lines 26–29
- **Status**: Pending
- **Details**: JS iterates `entryFields.length` (the pipe-split data values) which may exceed `fields.length` (the header field count). When there are more data values than header fields, JS creates node entries with key `"undefined"` (since `fields[i]` is `undefined` for excess indices). C++ (line 88) loops `while (pos <= entry.size() && fi < fields.size())`, stopping at the header field count and silently discarding extra data values. In practice, WoW CASC version configs always have matching field counts, but the edge-case behavior differs.

- [ ] 459. [tact-keys.cpp] `getKey` returns empty string instead of JS `undefined` when key not found
- **JS Source**: `src/js/casc/tact-keys.js` lines 19–21
- **Status**: Pending
- **Details**: JS `getKey` returns `KEY_RING[keyName.toLowerCase()]` which evaluates to `undefined` when the key is not in the object. C++ `getKey` (line 130) returns `{}` (empty string) when not found. Callers that compare the result against `undefined` (JS) vs checking `.empty()` (C++) should be functionally equivalent, but the sentinel value difference could cause issues if any code path checks for empty string vs non-existent key, or if a key legitimately has an empty value (not possible for TACT keys but differs in contract).

- [ ] 460. [checkboxlist.cpp] Missing container border and box-shadow from CSS reference
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-checkboxlist` has `border: 1px solid var(--border)` and `box-shadow: black 0 0 3px 0px` providing a bordered, shadowed container. C++ (line 171) uses `ImGui::BeginChild("##checkboxlist_container", ...)` with `ImGuiChildFlags_None` which does not render a border or shadow, producing a visually different container appearance.

- [ ] 461. [checkboxlist.cpp] Missing default item background and CSS padding values
- **JS Source**: `src/app.css` lines 1081–1086
- **Status**: Pending
- **Details**: CSS sets ALL items to `background: var(--background-dark)` with `padding: 2px 8px`. Even-indexed items override with `background: var(--background-alt)`. C++ (lines 241–244) only draws background for even items (`BG_ALT_U32`) and selected items (`FONT_ALT_U32`); odd non-selected items receive no explicit background, inheriting the ImGui child window background instead of the CSS `--background-dark` color. The `2px 8px` padding is not replicated — ImGui uses its default item spacing.

- [ ] 462. [checkboxlist.cpp] Missing item text left margin from CSS `.item span` rule
- **JS Source**: `src/app.css` lines 1065–1068
- **Status**: Pending
- **Details**: CSS `.ui-checkboxlist .item span` has `margin: 0 0 1px 5px` giving the label text 5px left margin and 1px bottom margin relative to the checkbox. C++ (line 260) uses `ImGui::SameLine()` between the checkbox and selectable label with default ImGui spacing (typically 4px item spacing), which may not exactly match the 5px CSS margin. The 1px bottom margin is also not replicated.
