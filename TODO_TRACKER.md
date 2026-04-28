# TODO Tracker

> **Progress: 6/580 verified (1%)** — ✅ = Verified, ⬜ = Pending

<!-- ─── src/app.cpp ─────────────────────────────────────────────────────────── -->

- [ ] 1. [app.cpp] `goToTexture()` function missing from C++ port
  - **JS Source**: `src/app.js` lines 418–434
  - **Status**: Pending
  - **Details**: The `goToTexture(fileDataId)` method is entirely absent. It switches to the textures tab, clears the current selection, and applies a file-ID regex filter. Must be added to app.cpp and wired to every call site.

- [ ] 2. [app.cpp] `whatsNewHTML` content not loaded on startup
  - **JS Source**: `src/app.js` lines 708–716
  - **Status**: Pending
  - **Details**: JS reads `whats-new.html` and stores its content in `core.view.whatsNewHTML` for display in the home tab. C++ does nothing here. Blocked on tab_home being a stub (see CLAUDE.md intentional stubs), but the load path itself is absent.

- [ ] 3. [app.cpp] `handleContextMenuClick` missing `.action` wrapper / optional-chaining logic
  - **JS Source**: `src/app.js` lines 318–323
  - **Status**: Pending
  - **Details**: JS checks `opt.action?.handler` (optional chaining through an `action` wrapper object). C++ checks `opt.handler` directly, missing the intermediate `.action` property layer.

- [ ] 4. [app.cpp] `click()` checks DOM classList for disabled state; C++ uses a boolean parameter instead
  - **JS Source**: `src/app.js` lines 369–372
  - **Status**: Pending
  - **Details**: JS inspects `event.target.classList.contains('disabled')` at the call site. C++ receives a pre-computed boolean. Ensure all callers pass the correct value and the semantics are preserved end-to-end.

- [ ] 5. [app.cpp] Drag-enter / drag-leave `fileDropPrompt` overlay not implementable in GLFW
  - **JS Source**: `src/app.js` lines 589–657
  - **Status**: Pending
  - **Details**: JS shows a `fileDropPrompt` overlay while a file is dragged over the window using `window.ondragenter`, `window.ondragleave`, and a `dropStack` counter. GLFW provides no drag-over events, so only the actual `glfw_drop_callback` (drop) is implemented. The visual drag-over prompt is missing. Document this in a code comment and track here.

- [ ] 6. [app.cpp] Vue reactive `AppState` init replaced with static struct; `created` lifecycle hook absent
  - **JS Source**: `src/app.js` lines 142–147
  - **Status**: Pending
  - **Details**: JS uses `Vue.createApp({ data: () => core.makeNewView(), created() { core.view = this; } })`. C++ creates a static `AppState` and assigns it to `core::view` before modules register. Any code relying on Vue's reactivity lifecycle (`mounted`, `created` ordering) is structurally different.

<!-- ─── src/js/blob.cpp ──────────────────────────────────────────────────────── -->

- [ ] 7. [blob.cpp] `arrayBuffer()` and `text()` are synchronous; JS versions return Promises
  - **JS Source**: `src/js/blob.js` lines 254–260
  - **Status**: Pending
  - **Details**: JS returns `Promise.resolve(...)` for both methods. C++ returns values synchronously. Callers that use `.then()` / `await` must be aware of this semantic change.

- [ ] 8. [blob.cpp] `stream()` returns synchronous callback-based `BlobReadableStream`; JS returns async `ReadableStream`
  - **JS Source**: `src/js/blob.js` lines 271–288
  - **Status**: Pending
  - **Details**: JS `stream()` yields 512 KB chunks via an async pull() on a ReadableStream. C++ returns a `BlobReadableStream` with synchronous pull(). Control flow is fundamentally different. Documented in C++ comments but callers need adapting.

- [ ] 9. [blob.cpp] `stringEncode()` does a byte copy instead of UTF-16→UTF-8 transcoding
  - **JS Source**: `src/js/blob.js` lines 42–95
  - **Status**: Pending
  - **Details**: JS performs full UTF-16→UTF-8 encoding with surrogate-pair handling and dynamic buffer resizing. C++ assumes the input `std::string` is already UTF-8 and copies bytes directly. Will produce incorrect output if the input is not UTF-8.

- [ ] 10. [blob.cpp] `stringDecode()` outputs UTF-8; JS outputs UTF-16 with surrogate pairs for codepoints > U+FFFF
  - **JS Source**: `src/js/blob.js` lines 97–167
  - **Status**: Pending
  - **Details**: JS decodes to a UTF-16 codeUnit array and generates surrogate pairs for codepoints > 0xFFFF before joining to a string. C++ decodes and emits UTF-8 bytes directly. The U+FFFD replacement character is preserved but the output encoding is fundamentally different.

- [ ] 11. [blob.cpp] `BlobPolyfill.isPolyfill` static property missing
  - **JS Source**: `src/js/blob.js` line 291
  - **Status**: Pending
  - **Details**: JS sets `BlobPolyfill.isPolyfill = true` to let callers distinguish the polyfill from a native Blob at runtime. No equivalent exposed in C++.

- [ ] 12. [blob.cpp] `URLPolyfill.createObjectURL` lacks fallback to native `URL.createObjectURL` for non-polyfill blobs
  - **JS Source**: `src/js/blob.js` lines 294–300
  - **Status**: Pending
  - **Details**: JS falls back to the native `URL.createObjectURL(blob)` when the argument is not a `BlobPolyfill` instance. C++ has no fallback path; non-BlobPolyfill arguments are not supported.

- [ ] 13. [blob.cpp] `slice()` end-value semantics differ: JS treats any falsy `end` as full length; C++ conflates `0` with "missing"
  - **JS Source**: `src/js/blob.js` lines 262–265
  - **Status**: Pending
  - **Details**: JS uses `end || this._buffer.length`, treating `0`, `null`, `undefined`, and `false` all as "use full length". C++ checks `!end.has_value() || end.value() == 0`, so an explicit `0` is treated the same as an absent value. `slice(0, 0)` should return an empty blob but currently returns a full-length copy.

<!-- ─── src/js/buffer.cpp ────────────────────────────────────────────────────── -->

- [ ] 14. [buffer.cpp] `fromCanvas()` takes raw RGBA pixels instead of a canvas object
  - **JS Source**: `src/js/buffer.js` lines 89–107
  - **Status**: Pending
  - **Details**: JS `fromCanvas(canvas)` accepts an `HTMLCanvasElement` or `OffscreenCanvas` and calls `canvas.getImageData()`. C++ `fromCanvas(uint8_t* rgba, ...)` requires pre-extracted pixel data. All callers must convert before calling.

- [ ] 15. [buffer.cpp] `decodeAudio()` signature and implementation completely replaced (WebAudio → miniaudio)
  - **JS Source**: `src/js/buffer.js` lines 981–983
  - **Status**: Pending
  - **Details**: JS `decodeAudio(context)` takes an `AudioContext` and returns a `Promise` via the Web Audio API. C++ `decodeAudio()` takes no parameters and returns a `DecodedAudioData` struct using miniaudio. All call sites must be adapted.

- [ ] 16. [buffer.cpp] `readFile()` is synchronous; JS version is async
  - **JS Source**: `src/js/buffer.js` lines 113–115
  - **Status**: Pending
  - **Details**: JS returns `Promise<BufferWrapper>`. C++ returns `BufferWrapper` synchronously, blocking the calling thread.

- [ ] 17. [buffer.cpp] `writeToFile()` is synchronous; JS version is async
  - **JS Source**: `src/js/buffer.js` lines 935–938
  - **Status**: Pending
  - **Details**: JS uses `fs.promises` and `await`. C++ uses `std::ofstream` synchronously, blocking the calling thread.

- [ ] 18. [buffer.cpp] `internalArrayBuffer` getter returns raw `uint8_t*` pointer; JS returns `ArrayBuffer` object
  - **JS Source**: `src/js/buffer.js` lines 174–176
  - **Status**: Pending
  - **Details**: JS `get internalArrayBuffer()` returns `this._buf.buffer` (an `ArrayBuffer` object). C++ `internalArrayBuffer()` returns `_buf.data()` (a `uint8_t*`). Type semantics are completely different.

- [ ] 19. [buffer.cpp] `setCapacity()` fills "unsafe" buffers with random bytes; JS leaves them uninitialised
  - **JS Source**: `src/js/buffer.js` lines 1021–1029
  - **Status**: Pending
  - **Details**: JS `Buffer.allocUnsafe()` leaves garbage data intentionally for performance. C++ calls `fill_unsafe_bytes()` which adds random bytes. This is a behavioural difference when callers rely on the buffer being overwritten before use.

- [ ] 20. [buffer.cpp] `readBuffer()` with `wrap=false` returns `std::variant` instead of a raw `Buffer`
  - **JS Source**: `src/js/buffer.js` lines 532–542
  - **Status**: Pending
  - **Details**: JS returns either a `BufferWrapper` or a raw `Buffer` based on the `wrap` parameter (dynamic typing). C++ returns `std::variant<BufferWrapper, std::vector<uint8_t>>`. Call sites need `std::get` or `std::visit`.

<!-- ─── src/js/config.cpp ────────────────────────────────────────────────────── -->

- [ ] 21. [config.cpp] `load()` is synchronous; JS version is async
  - **JS Source**: `src/js/config.js` lines 37–61
  - **Status**: Pending
  - **Details**: JS `load()` is `async` and `await`s all file reads. C++ is synchronous, blocking the calling thread during file I/O.

- [ ] 22. [config.cpp] Vue `$watch` auto-save removed; UI layer must call `config::save()` explicitly
  - **JS Source**: `src/js/config.js` line 60
  - **Status**: Pending
  - **Details**: JS registers `core.view.$watch('config', () => save(), { deep: true })` so any config mutation triggers a save automatically. C++ has no equivalent watcher; if a UI component changes config without calling `config::save()`, the change is silently lost.

- [ ] 23. [config.cpp] EPERM detection uses error-message string search; JS uses structured `e.code === 'EPERM'`
  - **JS Source**: `src/js/config.js` lines 44–46
  - **Status**: Pending
  - **Details**: JS checks the error `code` property directly. C++ searches `what()` for substrings `"permission"` and `"EPERM"`. More fragile — locale-dependent messages or new error wording would silently break detection.

<!-- ─── src/js/constants.cpp ─────────────────────────────────────────────────── -->

- [ ] 24. [constants.cpp] `MAX_RECENT_LOCAL` constant missing (JS value: `3`)
  - **JS Source**: `src/js/constants.js` line 46
  - **Status**: Pending
  - **Details**: No equivalent in `constants.h` or `constants.cpp`. Used to cap the number of recent local paths stored.

- [ ] 25. [constants.cpp] `GAME` object constants missing (MAP_SIZE, MAP_SIZE_SQ, MAP_COORD_BASE, TILE_SIZE, MAP_OFFSET)
  - **JS Source**: `src/js/constants.js` lines 70–76
  - **Status**: Pending
  - **Details**: `MAP_SIZE=64`, `MAP_SIZE_SQ=4096`, `MAP_COORD_BASE=51200/3`, `TILE_SIZE=(51200/3)/32`, `MAP_OFFSET=17066`. All absent from C++ constants.

- [ ] 26. [constants.cpp] `CACHE` string constants missing (SIZE_UPDATE_DELAY, BUILD_MANIFEST, BUILD_LISTFILE, BUILD_ENCODING, BUILD_ROOT, LISTFILE_DATA, SUBMIT_URL, FINALIZE_URL)
  - **JS Source**: `src/js/constants.js` lines 78–98
  - **Status**: Pending
  - **Details**: `SIZE_UPDATE_DELAY=5000`, `BUILD_MANIFEST='manifest.json'`, `BUILD_LISTFILE='listfile'`, `BUILD_ENCODING='encoding'`, `BUILD_ROOT='root'`, `LISTFILE_DATA='listfile.txt'`, plus `SUBMIT_URL` and `FINALIZE_URL` (kruithne.net endpoints). All absent from C++ constants.

- [ ] 27. [constants.cpp] `BLENDER` namespace missing `ADDON_DIR`, `ADDON_ENTRY`, `MIN_VER`
  - **JS Source**: `src/js/constants.js` lines 61–67
  - **Status**: Pending
  - **Details**: `ADDON_DIR='scripts/addons/io_scene_wowobj'`, `ADDON_ENTRY='__init__.py'`, `MIN_VER=2.8`. Only `DIR` and `LOCAL_DIR` are present in C++.

- [ ] 28. [constants.cpp] `MAGIC` object missing (M3DT, MD21, MD20 magic numbers)
  - **JS Source**: `src/js/constants.js` lines 157–161
  - **Status**: Pending
  - **Details**: `M3DT=0x5444334D`, `MD21=0x3132444D`, `MD20=0x3032444D`. All absent from C++ constants.

- [ ] 29. [constants.cpp] `PRODUCTS` array missing (11 WoW product definitions)
  - **JS Source**: `src/js/constants.js` lines 114–126
  - **Status**: Pending
  - **Details**: Array of 11 objects with `product`, `title`, and `tag` properties covering Retail, PTR, PTR2, Beta, Classic, etc. Entirely absent from C++ constants.

- [ ] 30. [constants.cpp] `PATCH` object missing (CDN regions, host URLs, SERVER_CONFIG, VERSION_CONFIG)
  - **JS Source**: `src/js/constants.js` lines 128–141
  - **Status**: Pending
  - **Details**: `REGIONS` array with `eu`, `us`, `kr`, `tw`, `cn`; `DEFAULT_REGION='us'`; `HOST`; `HOST_CHINA`; `SERVER_CONFIG='/cdns'`; `VERSION_CONFIG='/versions'`. All absent from C++ constants.

- [ ] 31. [constants.cpp] `BUILD` object missing (MANIFEST=`'.build.info'`, DATA_DIR=`'Data'`)
  - **JS Source**: `src/js/constants.js` lines 143–146
  - **Status**: Pending
  - **Details**: Both string constants absent from C++ constants.

- [ ] 32. [constants.cpp] `TIME` object missing (`DAY=86400000`)
  - **JS Source**: `src/js/constants.js` lines 148–150
  - **Status**: Pending
  - **Details**: `TIME.DAY` (milliseconds in one day) absent from C++ constants.

- [ ] 33. [constants.cpp] `KINO` object missing (`API_URL`, `POLL_INTERVAL=20000`)
  - **JS Source**: `src/js/constants.js` lines 152–155
  - **Status**: Pending
  - **Details**: Used for video/kino feature polling. Both constants absent from C++ constants.

- [ ] 34. [constants.cpp] `NAV_BUTTON_ORDER` array missing (21 tab/navigation names)
  - **JS Source**: `src/js/constants.js` lines 184–205
  - **Status**: Pending
  - **Details**: Ordered list of 21 navigation button names controlling tab order. Entirely absent from C++ constants.

- [ ] 35. [constants.cpp] `CONTEXT_MENU_ORDER` array missing (12 context-menu item names)
  - **JS Source**: `src/js/constants.js` lines 207–221
  - **Status**: Pending
  - **Details**: Ordered list of 12 context-menu item names. Entirely absent from C++ constants.

- [ ] 36. [constants.cpp] `FONT_PREVIEW_QUOTES` array missing (14 WoW in-game quotes)
  - **JS Source**: `src/js/constants.js` lines 223–238
  - **Status**: Pending
  - **Details**: 14 WoW quotes used to preview fonts. Entirely absent from C++ constants.

- [ ] 37. [constants.cpp] `EXPANSIONS` array missing (13 WoW expansion objects)
  - **JS Source**: `src/js/constants.js` lines 240–254
  - **Status**: Pending
  - **Details**: Array of 13 objects with `id`, `name`, and `shortName` for Classic through The Last Titan. Entirely absent from C++ constants.

- [ ] 38. [constants.cpp] `VERSION` not exported as a standalone constant
  - **JS Source**: `src/js/constants.js` line 52
  - **Status**: Pending
  - **Details**: JS exports `VERSION` directly (from `nw.App.manifest.version`). C++ only uses the version string internally to build `USER_AGENT`; it is not exposed as `constants::VERSION()`.

- [ ] 39. [constants.cpp] `UPDATE.ROOT` not provided as an exported constant
  - **JS Source**: `src/js/constants.js` lines 13–22, 105–109
  - **Status**: Pending
  - **Details**: JS computes `UPDATE_ROOT` based on platform and exports it as `UPDATE.ROOT`. C++ computes `UPDATE.DIRECTORY` and `UPDATE.HELPER` but omits `ROOT` entirely.

<!-- ─── src/js/core.cpp ──────────────────────────────────────────────────────── -->

- [ ] 40. [core.cpp] `makeNewView()` does not initialise `isDev` field
  - **JS Source**: `src/js/core.js` lines 33–44
  - **Status**: Pending
  - **Details**: JS sets `isDev: !BUILD_RELEASE`. C++ `makeNewView()` does not set `isDev` at all; the field will be default-constructed (likely `false`), which may be incorrect in debug builds.

- [ ] 41. [core.cpp] `makeNewView()` does not initialise `regexTooltip` string
  - **JS Source**: `src/js/core.js` line 280
  - **Status**: Pending
  - **Details**: JS initialises `regexTooltip` with a detailed help string shown in the UI. C++ leaves this field default-constructed (empty string).

- [ ] 42. [core.cpp] `makeNewView()` does not initialise help-related properties (`helpArticles`, `helpFilteredArticles`, `helpSelectedArticle`, `helpSearchQuery`)
  - **JS Source**: `src/js/core.js` lines 381–383
  - **Status**: Pending
  - **Details**: Four help-tab properties left at default-constructed values. Note: `tab_help` is removed (see CLAUDE.md), but these fields may still be referenced elsewhere.

- [ ] 43. [core.cpp] `openLastExportStream()` silently ignores all filesystem errors; JS only suppresses ENOENT
  - **JS Source**: `src/js/core.js` lines 394–408
  - **Status**: Pending
  - **Details**: JS catches `stat()` errors, re-throws anything that is not `ENOENT`, and logs unexpected errors. C++ uses `std::error_code ec` but never checks it or logs non-ENOENT failures — all errors are silently swallowed.

<!-- ─── src/js/external-links.cpp ───────────────────────────────────────────── -->

- [ ] 44. [external-links.cpp] Shell command injection vulnerability on Unix (`xdg-open` URL concatenation)
  - **JS Source**: `src/js/external-links.js` line 35
  - **Status**: Pending
  - **Details**: C++ Unix path (line 48) constructs `"xdg-open \"" + url + "\" &"` via string concatenation. A URL containing `"` or `` ` `` or `$()` can inject arbitrary shell commands. JS uses `nw.Shell.openExternal()` which is safe. Fix by using `execvp`/`posix_spawn` directly, or by sanitising the URL before shell expansion.

- [ ] 45. [external-links.cpp] `renderLink()` function has no JS equivalent — ImGui-specific addition not derived from JS source
  - **JS Source**: N/A
  - **Status**: Pending
  - **Details**: `renderLink()` (lines 57–75) renders a clickable ImGui hyperlink with hover styling and cursor change. It has no counterpart in the JS file. This is additional functionality — verify it is needed and document it in a comment.

<!-- ─── src/js/file-writer.cpp ──────────────────────────────────────────────── -->

- [ ] 46. [file-writer.cpp] `writeLine()` always blocks on every write; JS only blocks on actual backpressure
  - **JS Source**: `src/js/file-writer.js` lines 34–43
  - **Status**: Pending
  - **Details**: JS checks the return value of `stream.write()` and only sets `blocked=true` when it returns falsy (backpressure signalled). C++ unconditionally sets `blocked=true` and launches an async task on every single write, serialising all writes and incurring thread-overhead even when the stream is ready. Performance characteristic is fundamentally worse than JS.

- [ ] 47. [file-writer.cpp] `_drain()` does not resolve the pending write future; backpressure unblocking is broken
  - **JS Source**: `src/js/file-writer.js` lines 45–48
  - **Status**: Pending
  - **Details**: JS `_drain()` calls `this.resolver?.()` to unblock the awaiting `writeLine()` caller. C++ `_drain()` only sets `blocked = false` and never signals the future/promise. Any caller awaiting a blocked write will never be unblocked via the drain path — the future is only resolved when the async task in the lambda completes (line 68), not when the stream drains.

- [ ] 48. [file-writer.cpp] Async write lambda captures `this` without lifetime guard — potential use-after-free
  - **JS Source**: N/A
  - **Status**: Pending
  - **Details**: The lambda at lines 55–68 captures `this` (raw pointer) and runs on a background thread. If the `FileWriter` is destroyed before the async task completes, `this` is a dangling pointer and the lambda's dereference is UB. Fix by holding a `shared_ptr` to `this` or joining/detaching the future in the destructor before destruction.

- [ ] 49. [file-writer.cpp] Constructor removes directory on any stream-open failure, not just `EISDIR`
  - **JS Source**: `src/js/file-writer.js` lines 14–24
  - **Status**: Pending
  - **Details**: JS wraps stream creation in try/catch and checks `e.code === 'EISDIR'` before calling `fs.rmSync()`. C++ calls `std::filesystem::is_directory()` after any open failure — if the file simply doesn't exist or there is a permission error, C++ may still attempt a (no-op) directory remove, masking the real error.

- [ ] 50. [file-writer.cpp] Second stream open after directory removal is not checked for success
  - **JS Source**: `src/js/file-writer.js` lines 18–20
  - **Status**: Pending
  - **Details**: JS re-throws if the second `createWriteStream()` also fails. C++ calls `stream.open(file)` a second time but never checks the stream state afterwards; a second failure is silent.

<!-- ─── src/js/generics.cpp ──────────────────────────────────────────────────── -->

- [x] 51. [generics.cpp] `requestData()` / `doHttpGetRaw()` missing the JS 60-second request timeout
  - **JS Source**: `src/js/generics.js` lines 197–200
  - **Status**: Verified
  - **Details**: Confirmed implemented. `doHttpGetRaw()` calls `set_connection_timeout(60)` and `set_read_timeout(60)` on the httplib client, matching JS `req.setTimeout(60000, ...)`. Documented in the function's doc block.

- [x] 52. [generics.cpp] `redraw()` uses `std::this_thread::yield()` instead of `requestAnimationFrame`
  - **JS Source**: `src/js/generics.js` lines 266–271
  - **Status**: Verified
  - **Details**: Acceptable deviation documented in `redraw()` doc block. No C++ equivalent of rAF exists in an ImGui app; `yield()` is the closest available primitive. The ImGui main loop renders continuously so the deviation has no practical impact.

- [x] 53. [generics.cpp] `batchWork()` uses `sleep_for(0)` instead of `MessageChannel` scheduling
  - **JS Source**: `src/js/generics.js` lines 423–472
  - **Status**: Verified
  - **Details**: Acceptable deviation documented in `batchWork()` doc block. C++ has no browser event loop to pump; `sleep_for(0)` yields the timeslice to allow the ImGui render thread to run between batches.

- [x] 54. [generics.cpp] `queue()` polls futures in a busy loop instead of using event-driven completion
  - **JS Source**: `src/js/generics.js` lines 63–82
  - **Status**: Verified
  - **Details**: Acceptable deviation documented in `queue()` doc block. JS promise `.then()` chains are event-driven; C++ polling with 1 ms wait introduces minor latency but is functionally equivalent.

- [x] 55. [generics.cpp] `fileExists`, `directoryIsWritable`, `readFile`, `deleteDirectory`, `createDirectory` are synchronous; JS versions are async
  - **JS Source**: `src/js/generics.js` lines 258–411
  - **Status**: Verified
  - **Details**: Acceptable deviation documented with a `Deviation (TODO 55)` note in each function's doc block. C++ is synchronous by design; callers are aware.

- [x] 56. [generics.cpp] `get()` URL-retry log shows incorrect index/total (does not decrement total as JS does)
  - **JS Source**: `src/js/generics.js` lines 40–41
  - **Status**: Verified
  - **Details**: Confirmed correct. JS computes `url_stack.length + index` after `shift()` removes the current URL; this always equals the original array size, identical to `urls.size()`. A comment in `get()` documents this equivalence.

<!-- ─── src/js/gpu-info.cpp ───────────────────────────────────────────────────── -->

- [ ] 57. [gpu-info.cpp] Viewport capability string uses comma separator; JS uses 'x'
  - **JS Source**: `src/js/gpu-info.js` lines 294–295
  - **Status**: Pending
  - **Details**: JS formats viewport dimensions with `Array.join('x')`, e.g. `"1024x768"`. C++ (line 502 of gpu-info.cpp) concatenates with a comma, e.g. `"1024,768"`. The capability log string differs from JS output.

- [ ] 58. [gpu-info.cpp] macOS VRAM regex match not trimmed
  - **JS Source**: `src/js/gpu-info.js` line 198
  - **Status**: Pending
  - **Details**: JS calls `.trim()` on the regex match result before returning the macOS VRAM string. C++ (line 362) returns the raw match without trimming. Trailing whitespace may appear in the GPU info log on macOS.

<!-- ─── src/js/icon-render.cpp ───────────────────────────────────────────────── -->

- [ ] 59. [icon-render.cpp] `processQueue()` is synchronous; JS implementation is an async promise chain
  - **JS Source**: `src/js/icon-render.js` lines 48–65
  - **Status**: Pending
  - **Details**: JS calls `.then().catch().finally(() => processQueue())`, returning to the event loop between each icon load so other I/O can interleave. C++ (lines 251–288) processes the queue via synchronous tail recursion, blocking the calling thread until the entire queue is drained. Large queues may freeze the UI.

<!-- ─── src/js/install-type.cpp ──────────────────────────────────────────────── -->

- [ ] 60. [install-type.cpp] C++ file is an empty stub; verify install-type.h defines MPQ and CASC with correct values
  - **JS Source**: `src/js/install-type.js` lines 1–6
  - **Status**: Pending
  - **Details**: JS defines `InstallType = { MPQ: 1 << 0, CASC: 1 << 1 }`. The C++ .cpp file contains only a comment stating the enum is in the header. Verify that install-type.h declares `MPQ = 1 << 0` and `CASC = 1 << 1` with identical numeric values.

<!-- ─── src/js/log.cpp ────────────────────────────────────────────────────────── -->

- [ ] 61. [log.cpp] `write()` signature changed from variadic `...parameters` + `util.format()` to pre-formatted `std::string_view`
  - **JS Source**: `src/js/log.js` lines 78–95
  - **Status**: Pending
  - **Details**: JS `write(...parameters)` calls `util.format(...parameters)` internally, so callers may pass a format string and arguments. C++ `write(std::string_view message)` requires a fully pre-formatted string. All C++ callers must format manually; any caller expecting internal format substitution will silently get the raw format string.

- [ ] 62. [log.cpp] `getErrorDump()` is synchronous; JS version is `async` returning `Promise<string>`
  - **JS Source**: `src/js/log.js` lines 102–108
  - **Status**: Pending
  - **Details**: JS marks the function `async` and callers use `await`. C++ (lines 259–270) returns `std::string` synchronously. The deviation is acknowledged in a C++ comment, but callers that relied on the async contract must be adapted.

- [ ] 63. [log.cpp] Stdout logging is unconditional; JS only logs to console when `!BUILD_RELEASE`
  - **JS Source**: `src/js/log.js` lines 93–94
  - **Status**: Pending
  - **Details**: JS guards `console.log(line)` behind `if (!BUILD_RELEASE)`. C++ always calls `std::fputs(line.c_str(), stdout)`. Release builds should suppress stdout output to match JS behaviour.

<!-- ─── src/js/modules.cpp ────────────────────────────────────────────────────── -->

- [ ] 64. [modules.cpp] `reload_module()` component reload is a no-op; JS invalidates require cache and re-requires the module
  - **JS Source**: `src/js/modules.js` lines 321–362
  - **Status**: Pending
  - **Details**: JS deletes the module from `require.cache` and re-requires it so a fresh copy is used. C++ (lines 583–619) has a comment stating "Components are statically linked — this is a no-op." Hot-reloading of module code is not possible without a dynamic module system.

- [ ] 65. [modules.cpp] Module `initialize()` wrapper is synchronous; JS wraps as an `async` function
  - **JS Source**: `src/js/modules.js` lines 224–254
  - **Status**: Pending
  - **Details**: JS wraps each module's `initialize` with an `async` function and calls `await original_initialize.call(this)`, so module init can perform async I/O before the app proceeds. C++ (lines 226–246) calls the function pointer synchronously. Modules that need async startup must block on internal thread primitives instead.

<!-- ─── src/js/MultiMap.cpp ───────────────────────────────────────────────────── -->

- [ ] 66. [MultiMap.cpp] C++ file is a stub; verify MultiMap.h template replicates JS `set()` collision semantics
  - **JS Source**: `src/js/MultiMap.js` lines 19–29
  - **Status**: Pending
  - **Details**: JS `set()` wraps values in arrays on key collision: if the existing value is already an Array it pushes, otherwise it converts the single value to `[existing, new]`. The C++ .cpp file contains only a comment deferring to the header. Verify MultiMap.h implements exactly the same collision behaviour.

<!-- ─── src/js/png-writer.cpp ─────────────────────────────────────────────────── -->

- [ ] 67. [png-writer.cpp] `write()` uses a detached thread + `std::shared_future`; JS uses `async/await`
  - **JS Source**: `src/js/png-writer.js` lines 247–249
  - **Status**: Pending
  - **Details**: JS marks `write()` as `async` and directly `await`s `getBuffer().writeToFile(file)`. C++ (lines 283–298) spawns a detached thread, sets a promise inside it, and returns a `shared_future`. Callers must call `.get()` to block until completion. Unhandled exceptions in the detached thread will terminate the process if the future is abandoned.

<!-- ─── src/js/subtitles.cpp ──────────────────────────────────────────────────── -->

- [ ] 68. [subtitles.cpp] BOM detection mismatch: C++ strips UTF-8 BOM (3 bytes), JS strips UTF-16 BOM (U+FEFF char)
  - **JS Source**: `src/js/subtitles.js` lines 177–178
  - **Status**: Pending
  - **Details**: JS checks `text.charCodeAt(0) === 0xFEFF` (the UTF-16/UCS-2 BOM as a single code point). C++ (line 341) checks for the 3-byte UTF-8 BOM `\xEF\xBB\xBF`. WoW subtitle files are almost certainly UTF-8 so the C++ check is likely more correct, but the mismatch should be verified against real files.

- [ ] 69. [subtitles.cpp] `SUBTITLE_FORMAT` numeric values must match in C++ enum (SRT=118, SBT=7)
  - **JS Source**: `src/js/subtitles.js` lines 3–6
  - **Status**: Pending
  - **Details**: JS defines `SUBTITLE_FORMAT = { SRT: 118, SBT: 7 }`. C++ uses a `SubtitleFormat` enum declared in the header. Verify the C++ enum values are exactly 118 for SRT and 7 for SBT, as these are wire-format identifiers read from CASC files.

- [ ] 70. [subtitles.cpp] `get_subtitles_vtt()` calls `casc->getVirtualFileByID()`; JS calls `casc.getFile()`
  - **JS Source**: `src/js/subtitles.js` line 173
  - **Status**: Pending
  - **Details**: JS uses `await casc.getFile(file_data_id)`. C++ (line 358) calls `casc->getVirtualFileByID(file_data_id)`. The method names differ — verify both resolve the same data for a given file data ID and return an equivalent buffer/stream object.

<!-- ─── src/js/tiled-png-writer.cpp ──────────────────────────────────────────── -->

- [ ] 71. [tiled-png-writer.cpp] `write()` uses detached thread; exceptions silently lost if caller discards future
  - **JS Source**: `src/js/tiled-png-writer.js` lines 123–125
  - **Status**: Pending
  - **Details**: JS `write()` is `async` and propagates write errors to the awaiting caller. C++ spawns a detached `std::thread` and returns a `std::shared_future<void>`. If the caller discards the future, any exception thrown by `buffer.writeToFile` is silently lost. Callers must call `.get()` to observe errors. Verify all call sites retain the future.

- [ ] 72. [tiled-png-writer.cpp] `std::map` lexicographic order differs from JS `Map` insertion order — breaks Porter-Duff alpha blending for overlapping tiles
  - **JS Source**: `src/js/tiled-png-writer.js` lines 25, 58–59
  - **Status**: Pending
  - **Details**: JS `this.tiles` is a `Map()` (insertion-ordered). C++ uses `std::map<std::string, Tile>` (lexicographic key order on `"tileX,tileY"` strings). For fully-opaque pixels the order is irrelevant, but for semi-transparent overlapping tiles the Porter-Duff "over" composite is order-dependent and the two implementations may produce different pixel outputs. The existing header comment notes the deviation but omits the alpha-blending caveat.

- [ ] 73. [tiled-png-writer.cpp] `getStats()` `expectedTiles` field uses `uint32_t` — can overflow on large images
  - **JS Source**: `src/js/tiled-png-writer.js` lines 131–140
  - **Status**: Pending
  - **Details**: JS computes `this.tileCols * this.tileRows` as a 64-bit Number. C++ computes the same product as `uint32_t * uint32_t`, which overflows silently for large tile grids (e.g., 65536×65536 tiles). `Stats::expectedTiles` should be `size_t` or `uint64_t`.

<!-- ─── src/js/updater.cpp ───────────────────────────────────────────────────── -->

- [ ] 74. [updater.cpp] `BUILD_GUID()` generates a random UUID each run instead of a fixed build GUID — update check almost always triggers
  - **JS Source**: `src/js/updater.js` lines 24, 33–36
  - **Status**: Pending
  - **Details**: JS reads `nw.App.manifest.guid` — a fixed GUID baked into the NW.js manifest at build time, identical across all runs of the same build. C++ `constants::BUILD_GUID()` generates a new UUID v4 via `std::random_device` at every startup. Because the local GUID changes on every launch, the comparison `remoteGuid != localGuid` will almost always be true, causing `checkForUpdates()` to falsely report an update is available on every run.

- [ ] 75. [updater.cpp] `applyUpdate()` omits `permissions` from `downloadFile` call; `FileNode` struct has no `permissions` field
  - **JS Source**: `src/js/updater.js` lines 59–64, 120
  - **Status**: Pending
  - **Details**: JS passes `node.meta.permissions` as the sixth argument to `generics.downloadFile()` so downloaded files can be `chmod`'d to the correct mode. C++ `FileNode` never reads `permissions` from the update manifest and calls `generics::downloadFile` with only five arguments, defaulting to mode `0600`. On Linux, executable update files (including the updater binary) will be written as non-executable, causing `launchUpdater()` to fail.

- [ ] 76. [updater.cpp] `utilFormat()` replaces only the first `%s`; Node.js `util.format` replaces all occurrences
  - **JS Source**: `src/js/updater.js` lines 25, 113
  - **Status**: Pending
  - **Details**: `utilFormat()` uses a single `find` + `replace` and returns early, leaving any subsequent `%s` as a literal. The current `updateURL` format string has only one `%s` so there is no current bug, but if the URL pattern ever gains a second placeholder the function will silently produce a malformed URL.

- [ ] 77. [updater.cpp] `applyUpdate()` is fully synchronous, blocking the render thread for the entire update process
  - **JS Source**: `src/js/updater.js` lines 50–125
  - **Status**: Pending
  - **Details**: JS `applyUpdate()` is `async` and yields at every `await`, keeping the UI responsive. C++ runs all file-stat, hash, directory-create, and HTTP-download operations synchronously on the calling thread. The deviation is documented in C++ source comments, but the UI will freeze for the full duration of verification and download if called on the render thread.

- [ ] 78. [updater.cpp] `launchUpdater()` calls `std::exit(0)`, skipping destructors for stack objects
  - **JS Source**: `src/js/updater.js` lines 161–162
  - **Status**: Pending
  - **Details**: JS calls `process.exit()` and the NW.js runtime performs an orderly shutdown. C++ calls `std::exit(0)`, which invokes `atexit` handlers and destroys static-duration objects but skips destructors for all objects on the call stack above `launchUpdater()`. Logging handles, open file streams, and OpenGL contexts on the stack will not be properly closed.

<!-- ─── src/js/wmv.cpp ────────────────────────────────────────────────────────── -->

- [ ] 79. [wmv.cpp] `get_legacy_value` comment conflates null-path and non-numeric-string path without explaining the `-1` / unsigned wrap-around sentinel
  - **JS Source**: `src/js/wmv.js` lines 87–91
  - **Status**: Pending
  - **Details**: JS `parseInt(val ?? '0')` returns `NaN` for non-numeric strings; C++ returns `-1` as a sentinel. The downstream consumer in `tab_characters.cpp` guards with `static_cast<size_t>(value) < choices->size()`, so `-1` wraps to `SIZE_MAX` and is always out-of-range — matching JS `choices[NaN] === undefined`. The functional outcome is correct, but the in-code comments conflate the null path and the non-numeric-string path without documenting the wrap-around mechanism.

- [ ] 80. [wmv.cpp] `ParseResultV1`/`ParseResultV2` as `std::variant` — all callers must dispatch via `std::visit` or `std::get<>`
  - **JS Source**: `src/js/wmv.js` lines 69–75, 113–120
  - **Status**: Pending
  - **Details**: JS `wmv_parse` returns plain objects with different shapes (V1 has `legacy_values`, V2 has `customizations`). C++ models this as `std::variant<ParseResultV1, ParseResultV2>`. Any new caller site must use `std::visit` or `std::get<>` for dispatch; direct member access without dispatch is a compile error. Verify all existing call sites are correct.

<!-- ─── src/js/wowhead.cpp ───────────────────────────────────────────────────── -->

- [ ] 81. [wowhead.cpp] `wowhead_parse_hash` accesses `hash[0]` without an internal empty-string guard
  - **JS Source**: `src/js/wowhead.js` line 64
  - **Status**: Pending
  - **Details**: `wowhead_parse_hash` is only called from `wowhead_parse`, which throws before calling it if the hash is empty — the external guard is sufficient. However the function itself has no internal assertion or check, diverging from defensive C++ practice. The JS function has the same implicit reliance on the caller.

<!-- ─── src/js/xml.cpp ────────────────────────────────────────────────────────── -->

- [ ] 82. [xml.cpp] `build_object` uses `std::unordered_map` where JS uses insertion-order `Object.entries` — child property order may differ
  - **JS Source**: `src/js/xml.js` lines 148–153
  - **Status**: Pending
  - **Details**: JS `Object.entries(groups)` iterates child-tag groups in insertion order (the order distinct tag names first appeared). C++ `std::unordered_map` iterates in unspecified hash order. For WoW data files consumed by key name this does not break correctness, but callers that iterate the resulting JSON object may see keys in a different sequence than the JS version produces.

- [ ] 83. [xml.cpp] `parse_attributes` and `parse_node` contain extra out-of-bounds guards not present in the JS source
  - **JS Source**: `src/js/xml.js` lines 39–54, 60–98
  - **Status**: Pending
  - **Details**: The JS closures do not check `pos >= xml.length` after every `pos++` or `skip_whitespace()` call. C++ adds several extra `if (pos >= xml.size()) break/return` guards at these points, making C++ more robust on malformed/truncated XML. These are defensive additions absent from the JS source; they do not alter behaviour for well-formed input.

- [ ] 84. [xml.cpp] `build_object` null guard present in JS but omitted in C++ — dead code in both versions, no functional difference
  - **JS Source**: `src/js/xml.js` lines 129–131
  - **Status**: Pending
  - **Details**: JS `build_object` begins with `if (!node) return {}`. In practice all call sites check for a valid node before calling, so the guard is dead code. C++ accepts `const Node&` and uses `Node::valid` to ensure only valid nodes are processed, making the guard correctly absent. No functional difference.

- [ ] 85. [xml.cpp] Text-content nodes cause an infinite loop in the children parse loop — shared limitation in JS and C++
  - **JS Source**: `src/js/xml.js` lines 102–115
  - **Status**: Pending
  - **Details**: If XML contains bare text between element tags (e.g., `<root>hello</root>`), `parse_node` returns null/invalid without advancing `pos`, and the enclosing `while` loop spins indefinitely. This is a shared limitation in both JS and C++. WoW data XML files do not use text content, so this is an acceptable shared assumption.

<!-- ─── src/js/3D/AnimMapper.cpp ──────────────────────────────────────────────── -->

- [ ] 86. [AnimMapper.cpp] `get_anim_name` bounds check syntactically differs from JS `in` operator but is semantically equivalent for integer inputs
  - **JS Source**: `src/js/3D/AnimMapper.js` lines 1792–1797
  - **Status**: Pending
  - **Details**: JS uses `anim_id in ANIM_NAMES` (property-existence check on a dense array). C++ uses `anim_id >= 0 && static_cast<size_t>(anim_id) < ANIM_NAMES.size()`. The only theoretical divergence is a non-integer `anim_id` (e.g., `1.5`): JS returns false, C++ truncates to index 1. `anim_id` is always an integer in practice (read from binary WoW data), so there is no real concern.

- [ ] 87. [AnimMapper.cpp] JS uses `module.exports` (CommonJS), not an ES module `export`; C++ free-function export is correct regardless
  - **JS Source**: `src/js/3D/AnimMapper.js` line 1799
  - **Status**: Pending
  - **Details**: `AnimMapper.js` ends with `module.exports = { get_anim_name };` (CommonJS), not an ES module export. The C++ free function in `AnimMapper.h` correctly replaces either module system. No action needed; noted for accuracy.

<!-- ─── src/js/3D/ShaderMapper.cpp ────────────────────────────────────────────── -->

- [ ] 88. [ShaderMapper.cpp] SHADER_ARRAY index 35 has wrong pixel shader name — `"Combiners_Mod_Mod"` should be `"Combiners_Mod_Mod_Depth"`
  - **JS Source**: `src/js/3D/ShaderMapper.js` line 45
  - **Status**: Pending
  - **Details**: The last entry in SHADER_ARRAY (index 35) has `PS = "Combiners_Mod_Mod"` in C++ (`ShaderMapper.cpp` line 60) but should be `"Combiners_Mod_Mod_Depth"` per the JS source. The `_Depth` suffix is missing, causing incorrect pixel shader lookups for any M2 material whose shader ID maps to index 35 via the `0x8000` flag path.

- [ ] 89. [ShaderMapper.cpp] All four shader functions missing a terminal `return` after the non-0x8000 branch — undefined behaviour
  - **JS Source**: `src/js/3D/ShaderMapper.js` lines 52–90, 95–141, 146–161, 166–181
  - **Status**: Pending
  - **Details**: `getVertexShader`, `getPixelShader`, `getHullShader`, and `getDomainShader` are all declared returning `std::optional<std::string>`. In the non-0x8000 branch the if/else tree covers all practical code paths and always returns, but there is no final `return std::nullopt;` after the outer else block closes. Falling off the end of a non-void function is undefined behaviour in C++. The JS equivalents have an explicit return in every branch.

<!-- ─── src/js/3D/Shaders.cpp ──────────────────────────────────────────────────── -->

- [ ] 90. [Shaders.cpp] `create_program` exposes a dangling-pointer hazard — `active_programs` stores raw pointers but caller owns the `unique_ptr`
  - **JS Source**: `src/js/3D/Shaders.js` lines 56–72
  - **Status**: Pending
  - **Details**: JS `active_programs` is a `Map<name, Set<ShaderProgram>>` holding strong references that keep programs alive. C++ `active_programs` stores raw `gl::ShaderProgram*` pointers while `create_program` returns `std::unique_ptr<gl::ShaderProgram>` transferring ownership to the caller. If the caller destroys the `unique_ptr` without first calling `dispose()`, the raw pointer in `active_programs` becomes dangling; any subsequent `reload_all()` will dereference it (undefined behaviour). Should store `shared_ptr` in both `active_programs` and the return value, or enforce that callers call `dispose()` before destroying the returned `unique_ptr`.

- [ ] 91. [Shaders.cpp] `_unregister_fn` set lazily inside `create_program` — programs constructed by any other path silently skip unregistration
  - **JS Source**: `src/js/3D/Shaders.js` lines 56–72
  - **Status**: Pending
  - **Details**: JS resolves the circular dependency via a lazy `require('../Shaders')` call inside `dispose()`, which always works regardless of construction path. C++ sets `gl::ShaderProgram::_unregister_fn` only on the first call to `shaders::create_program()`. A `ShaderProgram` constructed via any other path will have `_unregister_fn` unset and `dispose()` will silently skip unregistration. The extra `_unregister_fn` guard in `dispose()` masks this omission.

- [ ] 92. [Shaders.cpp] `unregister` null-pointer guard has no JS equivalent — silently suppresses programming errors
  - **JS Source**: `src/js/3D/Shaders.js` lines 78–86
  - **Status**: Pending
  - **Details**: JS `unregister(program)` receives the full `ShaderProgram` object and has no null guard. C++ `unregister(gl::ShaderProgram* program)` adds `if (!program) return;` which silently suppresses a programming error in the caller. The C++ should assert or throw on a null input rather than silently returning, to match JS behaviour.

- [ ] 93. [Shaders.cpp] `reload_all` iterates `active_programs` while `get_source` may insert into `source_cache` — safe now, but iterator invalidation risk if code changes
  - **JS Source**: `src/js/3D/Shaders.js` lines 91–122
  - **Status**: Pending
  - **Details**: `reload_all()` iterates `active_programs` with a range-for and calls `get_source(name)` inside the loop. `get_source` may insert into `source_cache` (a separate map) but never touches `active_programs`, so there is no iterator invalidation currently. Future modifications that cause `get_source` to touch `active_programs` would introduce undefined behaviour.

- [ ] 94. [Shaders.cpp] `SHADER_MANIFEST` uses `std::unordered_map` — insertion order not preserved; JS object has guaranteed ES2015 insertion order
  - **JS Source**: `src/js/3D/Shaders.js` lines 13–19
  - **Status**: Pending
  - **Details**: JS `SHADER_MANIFEST` is a plain object literal with guaranteed ES2015+ insertion-order property enumeration. C++ uses `const std::unordered_map` with unspecified iteration order. The manifest is currently only accessed by key lookup (never iterated), so there is no observable difference. If future code iterates the manifest expecting a specific order, behaviour will differ from JS.

<!-- ─── src/js/3D/WMOShaderMapper.cpp ──────────────────────────────────────────── -->

- [ ] 95. [WMOShaderMapper.cpp] `WMOPixelShader::MapObjParallax` renamed to `MapObjParallax_PS` without explanatory comment
  - **JS Source**: `src/js/3D/WMOShaderMapper.js` line 35
  - **Status**: Pending
  - **Details**: JS defines `WMOPixelShader.MapObjParallax = 19`. C++ renames it `MapObjParallax_PS = 19` because `WMOVertexShader` already defines `MapObjParallax = 8` in the same namespace and the names would collide. The numeric value and all map usages are correct, but neither `WMOShaderMapper.h` nor `.cpp` contains a comment explaining why the name differs from the JS original. A future developer searching for `MapObjParallax` in the pixel shader enum will not find it.

<!-- ─── src/js/3D/camera/CameraControlsGL.cpp ──────────────────────────────────── -->

- [ ] 96. [CameraControlsGL.cpp] `camera.up` fallback `|| [0, 1, 0]` not mirrored in constructor or `pan()`
  - **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 180, 356
  - **Status**: Pending
  - **Details**: JS computes `const cam_up = camera.up || [0, 1, 0]` in both the constructor and `pan()`. C++ uses `camera.up` directly with no fallback. Since `CameraGL::up` is always initialized to `{0, 1, 0}`, the fallback is never triggered and the behavior is functionally equivalent. The defensive JS pattern is not mirrored — worth a comment explaining the omission.

- [ ] 97. [CameraControlsGL.cpp] `on_mouse_down()` returns `bool` (always `true`) instead of `void`
  - **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 223–240
  - **Status**: Pending
  - **Details**: JS `on_mouse_down` is implicitly void. C++ returns `bool` and always returns `true`, regardless of which button was pressed. This is a deliberate GLFW adaptation to allow callers to suppress further event processing. The code comment (line 259) explains this. Noted as a deviation from the JS signature.

- [ ] 98. [CameraControlsGL.cpp] quaternion dot-product fallback `|| [0, 0, 0, 1]` not mirrored in `update()`
  - **JS Source**: `src/js/3D/camera/CameraControlsGL.js` line 417
  - **Status**: Pending
  - **Details**: JS uses `this.camera.quaternion || [0, 0, 0, 1]` as the right-hand operand in `quat_dot`. C++ uses `camera.quaternion` directly. Since `CameraGL::quaternion` is always initialized to `{0, 0, 0, 1}`, behavior is equivalent. Defensive null-guard not mirrored.

<!-- ─── src/js/3D/camera/CharacterCameraControlsGL.cpp ──────────────────────────── -->

- [ ] 99. [CharacterCameraControlsGL.cpp] `on_mouse_wheel()` returns `true` on deltaY==0 early-exit path instead of `void`
  - **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 132–148
  - **Status**: Pending
  - **Details**: JS `on_mouse_wheel` is void and uses `return;` for the deltaY==0 early exit. C++ returns `true` on that path (and on all other paths), which correctly matches JS's unconditional `preventDefault`/`stopPropagation`. Intentional GLFW adaptation; noted as a deviation from the JS signature.

- [ ] 100. [CharacterCameraControlsGL.cpp] `dispose()` does not remove mousedown/wheel listeners from `dom_element`
  - **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 170–175
  - **Status**: Pending
  - **Details**: JS `dispose()` removes four listeners: `dom_element` mousedown and wheel, plus `document` mousemove and mouseup. C++ `dispose()` only sets `is_rotating = false` and `is_panning = false`. The mousedown and wheel handlers remain active if the caller keeps forwarding events. Acceptable GLFW adaptation (documented in source comments), but dispose() is semantically incomplete compared to JS.

- [ ] 101. [CharacterCameraControlsGL.cpp] `update()` empty body missing `// no-op for compatibility` comment
  - **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 166–168
  - **Status**: Pending
  - **Details**: JS `update()` has an explanatory comment `// no-op for compatibility`. C++ `update()` is an empty body with no comment. Minor documentation omission; no functional impact.

<!-- ─── src/js/3D/exporters/ADTExporter.cpp ────────────────────────────────────── -->

- [ ] 102. [ADTExporter.cpp] `calculateUVBounds` missing `chunk.vertices.empty()` guard
  - **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 266–268
  - **Status**: Pending
  - **Details**: JS has an early-continue guard `if (!chunk || !chunk.vertices) continue`. C++ checks array bounds (`ci >= rootAdt.chunks.size()`) but does not check whether `chunk.vertices` is empty. In practice ADT chunks always have 145 vertices, but the C++ should also check `chunk.vertices.empty()` to match the JS guard exactly.

- [ ] 103. [ADTExporter.cpp] Minimap scaling uses `stbir_resize_uint8_linear` instead of canvas bilinear — slight visual difference
  - **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 873–887
  - **Status**: Pending
  - **Details**: JS scales the minimap BLP using a 2D canvas context (`ctx.scale(scale, scale); ctx.drawImage(canvas, 0, 0)`), which applies browser bilinear/bicubic interpolation. C++ uses `stbir_resize_uint8_linear` (stb_image_resize2 linear resampling). Output pixel quality will differ slightly from JS; not an error but a visual deviation.

- [ ] 104. [ADTExporter.cpp] `liquidChunks` null-chunk serialization differs — JS passthrough vs C++ empty-instances check
  - **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1409–1412
  - **Status**: Pending
  - **Details**: JS `liquid.map()` returns the chunk verbatim (including `null`) when `!chunk || !chunk.instances`, producing JSON `null` entries. C++ emits `nullptr` (JSON `null`) only when `chunk.instances.empty()`. If ADTLoader ever produces a truly null chunk (not just one with empty instances), the C++ cannot distinguish it since `LiquidChunk` is a value type.

- [ ] 105. [ADTExporter.cpp] `liquidChunks` data spread — explicit field enumeration misses future `LiquidInstance`/`LiquidChunk` fields
  - **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1428–1438
  - **Status**: Pending
  - **Details**: JS uses spread syntax `{ ...instance, worldPosition, terrainChunkPosition }` which automatically includes all fields including any added in the future. C++ explicitly enumerates each field by name. If new fields are added to `LiquidInstance`, `LiquidVertexData`, `LiquidChunk`, or `LiquidAttributes`, they will not automatically appear in the JSON output. A code comment acknowledges this limitation.

- [ ] 106. [ADTExporter.cpp] `foliageJSON` `std::visit` serialization must handle all WDC field variant types correctly
  - **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1475–1476
  - **Status**: Pending
  - **Details**: JS assigns `foliageJSON.data = groundEffectTexture` (replaces the writer's root object with the full DB2 row), so all fields are serialized via `JSON.stringify`. C++ iterates via `std::visit` and calls `addProperty` for each variant-typed value. If `JSONWriter::addProperty` for any WDC variant type does not produce identical JSON to `JSON.stringify` (e.g., integer vs float representation), subtle differences could appear in the foliage output.

<!-- ─── src/js/3D/exporters/CharacterExporter.cpp ──────────────────────────────── -->

- [ ] 107. [CharacterExporter.cpp] `EquipmentRendererEntry.attachment_id` is `uint32_t` instead of `std::optional<uint32_t>` — attachment slot 0 indistinguishable from "no attachment"
  - **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 200–204
  - **Status**: Pending
  - **Details**: In JS, `attachment_id` can be `undefined` for collection-style renderers. C++ `EquipmentRendererEntry.attachment_id` is `uint32_t` (always has a value, defaulting to 0). `_process_equipment_renderer` takes `std::optional<uint32_t> attachment_id` and the condition `!is_collection_style && attachment_id.has_value() && apply_pose` is always true when `attachment_id == 0` (since `has_value()` is always true for non-optional). If attachment ID 0 is a valid slot, C++ incorrectly tries to apply an attachment transform instead of using raw geometry. Fix: make `EquipmentRendererEntry.attachment_id` an `std::optional<uint32_t>`.

<!-- ─── src/js/3D/exporters/M2Exporter.cpp ─────────────────────────────────────── -->

- [ ] 108. [M2Exporter.cpp] `validTextures` map uses all-string keys instead of JS mixed numeric/string keys
  - **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 91, 127, 170, 234
  - **Status**: Pending
  - **Details**: JS `validTextures` is a `Map` where regular texture keys are numeric file data IDs and data texture keys are strings like `"data-N"`. C++ converts all numeric keys to strings via `std::to_string(texFileDataID)` and stores in `std::map<std::string, M2TextureExportInfo>`. Internally consistent (downstream consumers also use `std::to_string`), but deviates from the JS type model. External consumers expecting numeric keys for regular textures will be broken.

- [ ] 109. [M2Exporter.cpp] `exportTextures` data texture backward-patch missing — `texture.fileDataID` not updated to `"data-N"` string
  - **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 150–168
  - **Status**: Pending
  - **Details**: When a texture slot is a data-texture type, JS backward-patches `texture.fileDataID = 'data-' + textureType` on the texture object. C++ leaves `texture.fileDataID` unchanged at the original numeric value. Code that later reads `texture.fileDataID` expecting it to reflect the data-texture replacement will see the wrong value.

- [ ] 110. [M2Exporter.cpp] `textureIndex` not reset to 0 after `dataTextures` loop — reads wrong `textureTypes` indices — bug
  - **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 103, 139–140
  - **Status**: Pending
  - **Details**: JS explicitly resets `textureIndex = 0` (line 139) after the `dataTextures` loop before the main `m2.textures` loop. C++ increments `textureIndex` during the `dataTextures` loop but never resets it. If any data textures were exported, the first `m2->textureTypes[textureIndex]` read in the main loop reads from the wrong offset — the first N texture types are skipped, producing incorrect texture-type assignments.

- [ ] 111. [M2Exporter.cpp] `getSkin(0)` null check uses `getSkinList().empty()` instead of checking returned pointer — potential null dereference
  - **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 421–422
  - **Status**: Pending
  - **Details**: JS does `const skin = await m2.getSkin(0); if (!skin) return`. C++ checks `if (equipM2.getSkinList().empty()) return` then unconditionally dereferences `*equipM2.getSkin(0).get()`. If `getSkinList()` is non-empty but `getSkin(0)` still returns null (e.g., skin file load failure), the C++ will dereference a null pointer. This pattern appears in `_addEquipmentToGLTF`, `_exportEquipmentToOBJ`, and `_exportEquipmentToSTL`.

<!-- ─── src/js/3D/exporters/M3Exporter.cpp ─────────────────────────────────────── -->

- [ ] 112. [M3Exporter.cpp] UV2 export guard `!m3->uv1.empty()` differs from JS `!== undefined` — empty-but-present array not exported
  - **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 88, 141
  - **Status**: Pending
  - **Details**: JS checks `this.m3.uv1 !== undefined` in both `exportAsGLTF` and `exportAsOBJ`. C++ checks `!m3->uv1.empty()`. If `uv1` is present but empty, JS exports it (empty data) while C++ skips it entirely. Affects both GLTF and OBJ UV2 channels when `uv1` exists as an empty array.

- [ ] 113. [M3Exporter.cpp] `exportRaw` writes from exporter's `data` member instead of `m3->data` as in JS
  - **JS Source**: `src/js/3D/exporters/M3Exporter.js` line 247
  - **Status**: Pending
  - **Details**: JS: `await this.m3.data.writeToFile(out)` — accesses raw data via the M3Loader's `data` property. C++: `data.writeToFile(out)` — writes from the exporter's own `data` member. Since the M3Loader is constructed from this same buffer, the content is equivalent, but the access path diverges from JS structurally.

<!-- ─── src/js/3D/exporters/WMOExporter.cpp ─────────────────────────────────────── -->

- [ ] 114. [WMOExporter.cpp] `exportAsOBJ` meta JSON groups array missing `colors2` field
  - **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 693–698
  - **Status**: Pending
  - **Details**: JS meta group entries include `colors2: group.colors2`. C++ group JSON objects (lines 876–919) omit `colors2`. The `liquid` field is included (via `grp->hasLiquid`). `colors2` is missing from the meta output, causing downstream tools that read the meta to see incomplete group data.

- [ ] 115. [WMOExporter.cpp] `exportAsOBJ` meta JSON pixelShader 20 texture list missing `flags3` entry
  - **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 710–720
  - **Status**: Pending
  - **Details**: For pixelShader 20, JS pushes `color3, flags3, runtimeData[0..N]`. C++ pushes `color3, runtimeData[0..N]` without `flags3`. The `flags3` entry is skipped, causing the material texture list to have one fewer entry than expected by the JS-side shader for pixelShader 20.

- [ ] 116. [WMOExporter.cpp] `exportGroupsAsSeparateOBJ` meta groups array missing `colors2` field
  - **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 1139–1159
  - **Status**: Pending
  - **Details**: Same issue as entry 114 but in the split-OBJ code path. JS meta group objects include `colors2: group.colors2`; C++ split-OBJ path omits it.

- [ ] 117. [WMOExporter.cpp] `exportGroupsAsSeparateOBJ` meta materials missing `vertexShader`/`pixelShader` fields
  - **JS Source**: `src/js/3D/exporters/WMOExporter.js` line 1196
  - **Status**: Pending
  - **Details**: JS split-OBJ meta serializes `wmo.materials` with `vertexShader`/`pixelShader` mutated in during the texture loop. C++ split-OBJ path serializes via `materialToJson()` without appending shader fields (unlike combined-OBJ which does add them via WMOShaderMapper). Split-OBJ C++ meta materials are missing shader identifiers.

- [ ] 118. [WMOExporter.cpp] `loadWMO()` and `getDoodadSetNames()` methods added in C++ have no JS counterparts
  - **JS Source**: `src/js/3D/exporters/WMOExporter.js` (entire file)
  - **Status**: Pending
  - **Details**: C++ adds two public methods (`loadWMO()` and `getDoodadSetNames()`) not present in the JS class. These are UI-layer convenience methods. They represent additions to the class interface beyond the JS original; document as intentional C++ extensions.

<!-- ─── src/js/3D/exporters/WMOLegacyExporter.cpp ──────────────────────────────── -->

- [ ] 119. [WMOLegacyExporter.cpp] `exportRaw` group filename uses `rfind(".wmo")` (last occurrence) vs JS `replace` (first occurrence)
  - **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` line 548
  - **Status**: Pending
  - **Details**: JS: `this.filePath.replace('.wmo', '_NNN.wmo')` — `String.prototype.replace` with a string argument replaces the first occurrence only. C++ uses `rfind(".wmo")` which finds the last occurrence. For paths where `.wmo` appears only once the result is identical, but paths with `.wmo` in a directory name would produce different results.

<!-- ─── src/js/3D/gl/GLContext.cpp ──────────────────────────────────────────────── -->

- [ ] 120. [GLContext.cpp] `invalidate_cache()` method has no JS counterpart — C++-only extension undocumented at the class level
  - **JS Source**: `src/js/3D/gl/GLContext.js` (entire file)
  - **Status**: Pending
  - **Details**: The JS `GLContext` has no `invalidate_cache` method. C++ adds it to handle Dear ImGui's OpenGL3 backend mutating GL state behind the cache. The method has an inline comment explaining the ImGui state issue but the class-level header does not document it as a C++ extension. Add a note to `GLContext.h`.

- [ ] 121. [GLContext.cpp] `unbind_vao()` method has no JS counterpart — C++-only extension
  - **JS Source**: `src/js/3D/gl/GLContext.js` (entire file)
  - **Status**: Pending
  - **Details**: The JS `GLContext` has no `unbind_vao` method. C++ adds it as a helper used by `VertexArray::dispose()` to keep the VAO state cache consistent. Undocumented as a C++ extension.

<!-- ─── src/js/3D/gl/GLTexture.cpp ──────────────────────────────────────────────── -->

- [ ] 122. [GLTexture.cpp] `set_rgba` `flip_y` defaults to `true` — behavioural deviation; JS never flips Y
  - **JS Source**: `src/js/3D/gl/GLTexture.js` lines 34–50
  - **Status**: Pending
  - **Details**: JS `set_rgba` calls `gl.texImage2D(...)` directly without any Y-flip; the WebGL `UNPACK_FLIP_Y_WEBGL` parameter is never set in this file. C++ `TextureOptions` defaults `flip_y = true` and `set_rgba` manually flips pixel rows before upload when `flip_y` is true. All callers using the default `TextureOptions{}` will receive a vertically flipped texture compared to the JS version. Callers must explicitly pass `flip_y = false` to match JS behaviour.

- [ ] 123. [GLTexture.cpp] `set_canvas` structurally different signature; inherits `flip_y=true` deviation from `set_rgba`
  - **JS Source**: `src/js/3D/gl/GLTexture.js` lines 57–73
  - **Status**: Pending
  - **Details**: JS `set_canvas(canvas, options)` takes an HTMLCanvasElement as pixel source. C++ `set_canvas(const uint8_t* pixels, int w, int h, ...)` takes raw pixel data explicitly. Both apply wrap/filter/mipmaps and delegate to `set_rgba`. The `flip_y=true` default from entry 122 also applies to `set_canvas` callers using the default options.

<!-- ─── src/js/3D/gl/ShaderProgram.cpp ──────────────────────────────────────────── -->

- [ ] 124. [ShaderProgram.cpp] `get_uniform_block_param` returns `GLint` for all `pname`s; JS returns mixed types including `Uint32Array` for array pnames
  - **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 122–127
  - **Status**: Pending
  - **Details**: JS `getActiveUniformBlockParameter` can return a `Uint32Array` for `GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES`. C++ uses `glGetActiveUniformBlockiv` and always returns a single `GLint`. For scalar pnames this is equivalent; for `GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES` (array-returning pname) the C++ cannot return the full array. If any code queries this pname, results will be incomplete.

- [ ] 125. [ShaderProgram.cpp] `set_uniform_3fv`/`set_uniform_4fv`/`set_uniform_mat4_array` require explicit `count`; JS infers from array length
  - **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 204–218, 247–251
  - **Status**: Pending
  - **Details**: JS passes Float32Arrays directly to `gl.uniform3fv`/`gl.uniform4fv`/`gl.uniformMatrix4fv`, which infer element count. C++ requires an explicit `GLsizei count` parameter (defaulting to 1). Callers that pass arrays of more than one vector/matrix must supply the correct count explicitly; failing to do so will silently upload only the first element.

<!-- ─── src/js/3D/gl/UniformBuffer.cpp ──────────────────────────────────────────── -->

- [ ] 126. [UniformBuffer.cpp] JS public `view`/`float_view`/`int_view` properties are private in C++ — external direct-access callers would fail
  - **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 20–22
  - **Status**: Pending
  - **Details**: JS stores `this.view` (DataView), `this.float_view` (Float32Array), `this.int_view` (Int32Array) as public properties. C++ keeps only a private `data_` vector; access is through typed methods (`set_float`, `set_int`, `set_mat4`, etc.). Any external code ported from JS that uses `uniformBuffer.float_view[i] = x` directly will not compile. Verify all call sites use the C++ API methods.

- [ ] 127. [UniformBuffer.cpp] `set_float`/`set_int` use `std::memcpy` (native-endian) instead of explicit little-endian writes
  - **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 51–53, 60–62
  - **Status**: Pending
  - **Details**: JS uses `view.setFloat32(offset, value, true)` / `view.setInt32(offset, value, true)` — explicit little-endian. C++ uses `std::memcpy` which writes in the host's native byte order. On Windows x64 and Linux x64 this is also little-endian, so behavior matches. This is a platform assumption; document it with a comment for portability awareness.

<!-- ─── src/js/3D/gl/VertexArray.cpp ─────────────────────────────────────────────── -->

- [ ] 128. [VertexArray.cpp] `set_wireframe_index_buffer` lacks `uint32_t` overload — cannot handle 32-bit index wireframes
  - **JS Source**: `src/js/3D/gl/VertexArray.js` lines 86–99
  - **Status**: Pending
  - **Details**: JS `set_wireframe_index_buffer(data, usage)` accepts both `Uint16Array` and `Uint32Array`. C++ only has a `uint16_t*` overload. If the main index buffer uses `GL_UNSIGNED_INT` (32-bit), the wireframe buffer cannot be set without index truncation. A `uint32_t*` overload is needed for large meshes.

- [ ] 129. [VertexArray.cpp] `triangles_to_lines` always returns `std::vector<uint16_t>` — silently truncates 32-bit indices for large meshes — bug
  - **JS Source**: `src/js/3D/gl/VertexArray.js` lines 343–352
  - **Status**: Pending
  - **Details**: JS: `const lines = new indices.constructor(indices.length * 2)` — if input is `Uint32Array`, output is also `Uint32Array`. C++ always returns `std::vector<uint16_t>`. For meshes with vertex counts > 65535, 32-bit indices are truncated to 16 bits, corrupting the wireframe geometry. A `uint32_t` variant (or template overload) is needed.

- [ ] 130. [VertexArray.cpp] `setup_m2_vertex_format` comment states texcoord2 is y-flipped but no code flip is performed — misleading
  - **JS Source**: `src/js/3D/gl/VertexArray.js` line 167
  - **Status**: Pending
  - **Details**: C++ adds `// texcoord2: vec2 at offset 40 (y-flipped for OpenGL bottom-left origin)` — the JS comment only says `// texcoord2: vec2 at offset 40`. The actual `glVertexAttribPointer` call is identical. If a y-flip is required, it must be applied in the shader or in the data before upload; the comment implies a flip that does not exist in the code, which is misleading.

<!-- ─── src/js/3D/loaders/ADTLoader.cpp ──────────────────────────────────────────── -->

- [ ] 131. [ADTLoader.cpp] `handle_root_MH2O` `std::sort` on `std::set`-derived vector is redundant
  - **JS Source**: `src/js/3D/loaders/ADTLoader.js` lines 124–271
  - **Status**: Pending
  - **Details**: Offsets are collected in a `std::set<uint32_t>`, which maintains sorted order. The vector converted from this set is then explicitly `std::sort`ed, which is a no-op on an already-sorted sequence. The redundant sort is harmless but should be removed for clarity.

- [ ] 132. [ADTLoader.cpp] `handle_texmcnk_MCAL` null-WDT guard falls to 2048-byte path instead of throwing — diverges from JS error semantics
  - **JS Source**: `src/js/3D/loaders/ADTLoader.js` line 467
  - **Status**: Pending
  - **Details**: JS accesses `root.flags` directly with no null check (root is always provided). C++ adds `wdt && (wdt->flags & ...)` which silently falls through to the uncompressed 2048-byte path if `wdt` is null. JS would throw a TypeError on null access. The C++ null guard prevents a crash but silently misreads alpha data for callers that forget to provide a WDT.

<!-- ─── src/js/3D/loaders/BONELoader.cpp ──────────────────────────────────────────── -->

- [ ] 133. [BONELoader.cpp] `parse_chunk_bomt` integer division truncates for non-multiple-of-64 `chunkSize`; JS floating-point division would throw
  - **JS Source**: `src/js/3D/loaders/BONELoader.js` line 53
  - **Status**: Pending
  - **Details**: JS computes `amount = (chunkSize / 16) / 4` with floating-point division. If `chunkSize` is not a multiple of 64, `new Array(nonIntegerAmount)` throws a RangeError. C++ uses `uint32_t` division, which truncates silently and reads fewer matrices than expected from a malformed file. Well-formed BONE files always have sizes that are multiples of 64 so this is not a real-world concern.

<!-- ─── src/js/3D/loaders/LoaderGenerics.cpp ─────────────────────────────────────── -->

- [ ] 134. [LoaderGenerics.cpp] `ReadStringBlock` returns ordered `std::map<uint32_t, std::string>` — iteration order differs from JS unordered plain object
  - **JS Source**: `src/js/3D/loaders/LoaderGenerics.js` lines 12–31
  - **Status**: Pending
  - **Details**: JS returns a plain object `{}` with numeric-string keys (insertion order). C++ returns `std::map<uint32_t, std::string>` (ascending key order). All callers access entries by offset key lookup (never iterating), so the ordering difference has no observable effect. Noted for completeness.

<!-- ─── src/js/3D/loaders/M2Generics.cpp ────────────────────────────────────────── -->

- [ ] 135. [M2Generics.cpp] `read_m2_array_array_internal` unknown type: C++ returns seek-0 before throw; JS throws immediately
  - **JS Source**: `src/js/3D/loaders/M2Generics.js` lines 51–79
  - **Status**: Pending
  - **Details**: `value_size()` returns 0 for unknown data types. In the `useAnims` branch, `animBuf->seek(subArrOfs + j * 0)` always seeks to `subArrOfs` regardless of `j`, corrupting the seek position before `read_value` eventually throws. JS throws `"Unhandled data type: ..."` immediately without any seek. The error condition is still reached in C++, but with a corrupted buffer position.

- [ ] 136. [M2Generics.cpp] `read_m2_array_array` passes `animFiles` map by value — unnecessary copy on every call
  - **JS Source**: `src/js/3D/loaders/M2Generics.js` lines 126–127
  - **Status**: Pending
  - **Details**: JS passes the `animFiles` map by reference. C++ `read_m2_array_array` accepts `std::map<uint32_t, BufferWrapper*> animFiles` by value, copying the map on every call. The map is not mutated inside, so pass-by-const-reference would be correct and more efficient.

<!-- ─── src/js/3D/loaders/M2LegacyLoader.cpp ────────────────────────────────────── -->

- [ ] 137. [M2LegacyLoader.cpp] `getSkinList()` returns empty vector for WotLK models; JS returns `undefined`
  - **JS Source**: `src/js/3D/loaders/M2LegacyLoader.js` lines 827–829
  - **Status**: Pending
  - **Details**: For WotLK models, JS `getSkinList()` returns `this.skins` which was never assigned — i.e., `undefined`. C++ always returns a reference to `this->skins`, which is an empty `std::vector<LegacyM2Skin>`. Callers that distinguish `undefined` from an empty array would behave differently. In practice `getSkinList` for WotLK is not expected to be used, but the semantic difference is worth noting.

<!-- ─── src/js/3D/loaders/M3Loader.cpp ───────────────────────────────────────────── -->

- [ ] 138. [M3Loader.cpp] `ReadBufferAsFormat` split into two typed functions vs JS single mixed-type function — structural divergence
  - **JS Source**: `src/js/3D/loaders/M3Loader.js` lines 239–258
  - **Status**: Pending
  - **Details**: JS `ReadBufferAsFormat` returns an `Array` of either floats or uint16s depending on format. C++ splits into `ReadBufferAsFormat` (floats only, throws for `1U16`) and `ReadBufferAsFormatU16` (uint16 only). Call sites are updated consistently. Functionally equivalent but structurally diverges from the JS single-function pattern.

<!-- ─── src/js/3D/loaders/MDXLoader.cpp ──────────────────────────────────────────── -->

- [ ] 139. [MDXLoader.cpp] Deferred node registration after all container `push_back` calls — necessary C++ adaptation, undocumented
  - **JS Source**: `src/js/3D/loaders/MDXLoader.js` lines 208–210
  - **Status**: Pending
  - **Details**: JS `_read_node` immediately assigns `this.nodes[node.objectId] = node` (a reference) before returning. C++ defers building `this->nodes` to after all container vectors (bones, helpers, attachments, etc.) are fully populated, to avoid pointer invalidation from `push_back`. This is the correct approach but is not documented with a comment in the code.

- [ ] 140. [MDXLoader.cpp] `ATCH` handler JS `readUInt32LE(-4)` bug intentionally diverged from JS
  - **JS Source**: `src/js/3D/loaders/MDXLoader.js` line 404
  - **Status**: Pending
  - **Details**: JS line 404: `if (this.data.offset < startPos + this.data.readUInt32LE(-4))` uses a negative offset which is a JS bug (would throw at runtime). C++ correctly uses `startPos + attachmentSize` (the size read at the start of the entry) and has a comment acknowledging this fix. This intentional divergence from the buggy JS source is correct; noted for audit completeness.

<!-- ─── src/js/3D/loaders/SKELLoader.cpp ─────────────────────────────────────────── -->

- [ ] 141. [SKELLoader.cpp] `loadAnimsForIndex`/`loadAnims` async→sync — blocks calling thread during I/O
  - **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 308–347, 407–454
  - **Status**: Pending
  - **Details**: JS uses `async`/`await` for CASC file fetches and loader calls, yielding the event loop. C++ calls `core::view->casc->getVirtualFileByID` and `loader->load(true)` synchronously. The calling thread blocks for the full duration of each file I/O operation. Callers on the UI thread will freeze the application during skeleton animation loading.

- [ ] 142. [SKELLoader.cpp] `loadAnims` `try` block indentation misaligned — cosmetic
  - **JS Source**: N/A
  - **Status**: Pending
  - **Details**: In `SKELLoader.cpp` the `try {` block inside `loadAnims` is indented at a different level from its surrounding `if` block. Does not affect correctness but should be fixed for readability.

<!-- ─── src/js/3D/loaders/WMOLegacyLoader.cpp ────────────────────────────────────── -->

- [ ] 143. [WMOLegacyLoader.cpp] `MOGP` group flags stored in `groupFlags`; JS uses same `flags` property name — consumers must use correct C++ field
  - **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` line 453
  - **Status**: Pending
  - **Details**: JS stores MOGP group flags in `this.flags` — the same property name as the MOHD root flags. C++ declares `uint16_t flags` (MOHD) and `uint32_t groupFlags` (MOGP) as separate members. Any consumer code ported from JS that accesses `.flags` on a group-level WMOLegacyLoader expecting MOGP flags will silently read the root flags instead. All consumers should be verified to use `groupFlags` where MOGP group flags are needed.

- [ ] 144. [WMOLegacyLoader.cpp] `hasLiquid` boolean flag added in C++ — necessary adaptation, no JS counterpart
  - **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 421–431
  - **Status**: Pending
  - **Details**: JS consumers check `if (this.liquid)` (truthy check — struct always truthy in C++). C++ adds `hasLiquid = true` set during `parse_MLIQ` so consumers can test `hasLiquid` instead. This is a necessary C++ adaptation; documented here for completeness.

<!-- ─── src/js/3D/loaders/WMOLoader.cpp ──────────────────────────────────────────── -->

- [ ] 145. [WMOLoader.cpp] `getGroup` guard `groups.empty()` incorrectly throws for valid zero-group WMOs — bug
  - **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 65–66
  - **Status**: Pending
  - **Details**: JS throws if `!this.groups` (groups was never assigned, i.e., MOHD not parsed). An empty JS array is truthy, so `![]` is false — zero-group WMOs pass the guard. C++ uses `this->groups.empty()` which is true both when MOHD was never parsed AND when `groupCount == 0`. A valid zero-group WMO would erroneously trigger "Attempted to obtain group from a root WMO." Fix: add `bool groupsInitialized = false` (set by `parse_MOHD`) and use `!groupsInitialized` as the guard, matching `WMOLegacyLoader.cpp`.

- [ ] 146. [WMOLoader.cpp] `MOGP` group flags stored in `groupFlags`; JS uses same `flags` property — consumers must use correct C++ field
  - **JS Source**: `src/js/3D/loaders/WMOLoader.js` line 361
  - **Status**: Pending
  - **Details**: Same issue as entry 143 for `WMOLegacyLoader`. JS `this.flags` is used for both MOHD root flags and MOGP group flags. C++ declares `uint16_t flags` (MOHD) and `uint32_t groupFlags` (MOGP). `WMOLoader.h` acknowledges this with a comment ("flags already declared above as uint16_t; group flags reuse this as uint32_t"). Verify all consumers use `groupFlags` for MOGP group flags.

- [ ] 147. [WMOLoader.cpp] `hasLiquid` boolean flag added in C++ — necessary adaptation, no JS counterpart
  - **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 327–337
  - **Status**: Pending
  - **Details**: Same as entry 144 for `WMOLegacyLoader`. JS checks `if (this.liquid)` (truthy); C++ adds `hasLiquid = true` in `parse_MLIQ`. Necessary C++ adaptation.

- [ ] 148. [WMOLoader.cpp] `hasMotxChunk` boolean flag added in C++ — necessary adaptation, no JS counterpart
  - **JS Source**: `src/js/3D/loaders/WMOLoader.js` line 131
  - **Status**: Pending
  - **Details**: JS `exportTextures` checks `!!wmo.textureNames` (truthy — `{}` is truthy even when empty). C++ cannot use truthiness on a `std::map`, so `hasMotxChunk = true` is set in `parse_MOTX` (even for empty MOTX chunks) to replicate JS truthy semantics. Necessary C++ adaptation; documented in a code comment.

<!-- ─── src/js/3D/renderers/CharMaterialRenderer.cpp ──────────────────────────────── -->

- [ ] 149. [CharMaterialRenderer.cpp] `setTextureTarget()` does not call `update()` after adding a texture target — bug
  - **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` line 143
  - **Status**: Pending
  - **Details**: JS `setTextureTarget()` ends with `await this.update()`. C++ `setTextureTarget()` adds the texture target and returns without calling `update()`. Adding a texture target will not trigger an FBO redraw until the next explicit `update()` call.

- [ ] 150. [CharMaterialRenderer.cpp] `init()` is synchronous in C++; JS original is `async`
  - **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` line 41
  - **Status**: Pending
  - **Details**: JS `async init()` returns a Promise. C++ `init()` is `void`. Necessary adaptation (no JS Promise system in C++). Callers must not await a return value.

- [ ] 151. [CharMaterialRenderer.cpp] `getCanvas()` returns `GLuint` FBO texture handle; JS returns HTML canvas element
  - **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js`
  - **Status**: Pending
  - **Details**: Necessary adaptation — C++ replaces the HTML canvas with an FBO texture. `getCanvas()` returns the `fbo_texture_` GLuint handle instead of an HTML canvas element.

<!-- ─── src/js/3D/renderers/GridRenderer.cpp ──────────────────────────────── -->

- [ ] 152. [GridRenderer.cpp] Shader GLSL version changed from `#version 300 es` (WebGL2) to `#version 430 core` (desktop OpenGL 4.3)
  - **JS Source**: `src/js/3D/renderers/GridRenderer.js`
  - **Status**: Pending
  - **Details**: Necessary adaptation (desktop OpenGL 4.3 core profile). No functional impact on rendering behaviour.

<!-- ─── src/js/3D/renderers/M2LegacyRendererGL.cpp ──────────────────────────────── -->

- [ ] 153. [M2LegacyRendererGL.cpp] Bone skinning is a documented stub — `u_bone_count` always set to 0
  - **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js`
  - **Status**: Pending
  - **Details**: JS `_create_bones_ubo()` builds a live UBO and `render()` binds and uploads bone data per draw call. C++ `render()` sets `shader->set_uniform_1i("u_bone_count", 0)` with a TODO comment. Legacy M2 models will render without bone skinning — character poses and animations will be incorrect until the animation system is fixed.

- [ ] 154. [M2LegacyRendererGL.cpp] `loadSkin()` does not initialize any GPU bone buffer — stub
  - **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js`
  - **Status**: Pending
  - **Details**: JS `_create_bones_ubo()` is called from `loadSkin()` (via `load()`). C++ `loadSkin()` stores `std::vector<float> bone_matrices` in CPU memory only — no SSBO or UBO is created. Related to entry 153.

- [ ] 155. [M2LegacyRendererGL.cpp] `texture_ribbon::setSlotFileLegacy()` used where JS uses `textureRibbon.setSlotFile()`
  - **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js`
  - **Status**: Pending
  - **Details**: JS uses `textureRibbon.setSlotFile(ribbonSlot, fileName, syncID)` for all texture types. C++ uses `texture_ribbon::setSlotFileLegacy()` for file-path-based slots. If `setSlotFileLegacy()` has different behaviour, ribbon display may differ. Requires verification.

- [ ] 156. [M2LegacyRendererGL.cpp] `current_animation` uses sentinel value `-1`; JS uses `null`
  - **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js`
  - **Status**: Pending
  - **Details**: Necessary adaptation. JS `this.current_animation = null`; C++ uses `-1` as sentinel. Functionally equivalent.

<!-- ─── src/js/3D/renderers/M2RendererGL.cpp ──────────────────────────────── -->

- [ ] 157. [M2RendererGL.cpp] UBO (WebGL2 Uniform Buffer Object) replaced by SSBO for bone matrices
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js`
  - **Status**: Pending
  - **Details**: Necessary architectural adaptation — desktop OpenGL 4.3 SSBO preferred over UBO for large bone arrays. JS uses `create_bones_ubo()` and uploads via `ubo.ubo.upload_range()`; C++ uses `bone_ssbo` (GLuint) with `glBufferData`. Functionally equivalent.

- [ ] 158. [M2RendererGL.cpp] `stopAnimation()` uses `-1` sentinels instead of JS `null`
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js`
  - **Status**: Pending
  - **Details**: JS sets `this.current_animation = null`, `this.anim_index = null`, `this.anim_source = null`. C++ uses `-1` sentinels. Necessary adaptation; functionally equivalent.

- [ ] 159. [M2RendererGL.cpp] `_animate_track()` split into typed overloads
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js`
  - **Status**: Pending
  - **Details**: JS has single `_animate_track(animblock, def, lerpfunc)` with a generic lerp callback. C++ splits into `_animate_track_vec4()` and `_animate_track_scalar()`. Necessary adaptation (C++ requires typed function signatures); functionally equivalent.

- [ ] 160. [M2RendererGL.cpp] `find_keyframe()` free function in JS mapped to `_find_time_index()` static member in C++
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js`
  - **Status**: Pending
  - **Details**: Necessary adaptation; functionally equivalent.

<!-- ─── src/js/3D/renderers/M3RendererGL.cpp ──────────────────────────────── -->

- [ ] 161. [M3RendererGL.cpp] UBO replaced by SSBO for bone matrices
  - **JS Source**: `src/js/3D/renderers/M3RendererGL.js`
  - **Status**: Pending
  - **Details**: Same architectural change as M2RendererGL (entry 157). JS `_create_bones_ubo()` with `bone_count = 1`; C++ uses `bone_ssbo` SSBO. Necessary adaptation; functionally equivalent.

- [ ] 162. [M3RendererGL.cpp] JS `loadLOD()` calls `_create_bones_ubo()`; C++ `loadLOD()` does not
  - **JS Source**: `src/js/3D/renderers/M3RendererGL.js` line 147
  - **Status**: Pending
  - **Details**: In JS, `loadLOD()` calls `this._create_bones_ubo()`. In C++, the SSBO is created once in `load()` — `loadLOD()` does not call any SSBO creation. Necessary adaptation (SSBO lifecycle differs from UBO lifecycle). Functionally equivalent if SSBO is always created before first LOD render.

- [ ] 163. [M3RendererGL.cpp] `getBoundingBox()` returns `std::nullopt` when `m3` is null; JS returns `{min:[Inf...], max:[-Inf...]}` for empty vertices
  - **JS Source**: `src/js/3D/renderers/M3RendererGL.js`
  - **Status**: Pending
  - **Details**: Documented deviation in a code comment. Callers treat both as "no bounding box". Functionally equivalent for current use cases.

- [ ] 164. [M3RendererGL.cpp] `dispose()` deletes `bone_ssbo` with `glDeleteBuffers`; JS disposes UBOs with `for (const ubo of this.ubos) ubo.ubo.dispose()`
  - **JS Source**: `src/js/3D/renderers/M3RendererGL.js`
  - **Status**: Pending
  - **Details**: Necessary adaptation. Functionally equivalent resource cleanup.

<!-- ─── src/js/3D/renderers/MDXRendererGL.cpp ──────────────────────────────── -->

- [ ] 165. [MDXRendererGL.cpp] UV v-coordinate is flipped in `_build_geometry()` — not present in JS source
  - **JS Source**: `src/js/3D/renderers/MDXRendererGL.js`
  - **Status**: Pending
  - **Details**: JS stores raw UVs from `geoset.tVertices[0]` directly with no flip. C++ applies `flippedUvs[i * 2 + 1] = 1.0f - uvs[i * 2 + 1]` (v-coordinate Y-axis flip). This operation does not exist in the JS source and is likely a porting bug — UV-mapped textures may appear vertically mirrored.

- [ ] 166. [MDXRendererGL.cpp] UBO replaced by SSBO for node matrices
  - **JS Source**: `src/js/3D/renderers/MDXRendererGL.js`
  - **Status**: Pending
  - **Details**: Same architectural change as M2RendererGL (entry 157). Necessary adaptation; functionally equivalent.

- [ ] 167. [MDXRendererGL.cpp] `stopAnimation()` resets ALL node matrices to identity; JS only resets first matrix
  - **JS Source**: `src/js/3D/renderers/MDXRendererGL.js`
  - **Status**: Pending
  - **Details**: JS `this.node_matrices.set(IDENTITY_MAT4)` sets only the first 16 floats. C++ iterates all matrices and resets each to identity. C++ behaviour is arguably more correct but deviates from JS. May be intentional.

- [ ] 168. [MDXRendererGL.cpp] Vue `$watch` reactive watchers replaced by per-frame polling
  - **JS Source**: `src/js/3D/renderers/MDXRendererGL.js`
  - **Status**: Pending
  - **Details**: Necessary adaptation (no reactive framework in C++). Polling in `render()` with `prev_geoset_checked` / `prev_wireframe` comparisons. Functionally equivalent.

- [ ] 169. [MDXRendererGL.cpp] `_scratch_calculated` is a JS `Set`; C++ uses `std::set<int>`
  - **JS Source**: `src/js/3D/renderers/MDXRendererGL.js`
  - **Status**: Pending
  - **Details**: Necessary adaptation. Functionally equivalent.

<!-- ─── src/js/3D/renderers/renderer_utils.cpp ──────────────────────────────── -->

- [ ] 170. [renderer_utils.cpp] Complete API redesign: JS `create_bones_ubo()` returns `Float32Array`; C++ `create_bones_ssbo()` returns `GLuint`
  - **JS Source**: `src/js/3D/renderers/renderer_utils.js` lines 10–23
  - **Status**: Pending
  - **Details**: JS `create_bones_ubo(shader, gl, ctx, ubos, bone_count)` binds a uniform block, creates a UniformBuffer, pushes to `ubos` array, and returns a Float32Array view. C++ `renderer_utils::create_bones_ssbo(bone_count)` creates an SSBO, fills with identity matrices, and returns a GLuint. Necessary architectural adaptation (WebGL2 UBO → desktop OpenGL SSBO).

- [ ] 171. [renderer_utils.cpp] `renderer_utils::create_bones_ssbo()` is dead code — no callers in current codebase
  - **JS Source**: `src/js/3D/renderers/renderer_utils.js` lines 10–23
  - **Status**: Pending
  - **Details**: All renderers (M2RendererGL, M3RendererGL, MDXRendererGL) create their own SSBOs directly via `glGenBuffers`. `create_bones_ssbo` is declared and defined but has no call sites. Should be removed or used consistently.

<!-- ─── src/js/3D/renderers/ShadowPlaneRenderer.cpp ──────────────────────────────── -->

- [ ] 172. [ShadowPlaneRenderer.cpp] Shader GLSL version changed from `#version 300 es` to `#version 430 core`
  - **JS Source**: `src/js/3D/renderers/ShadowPlaneRenderer.js`
  - **Status**: Pending
  - **Details**: Necessary adaptation (desktop OpenGL 4.3 core profile). No functional impact.

<!-- ─── src/js/3D/renderers/WMOLegacyRendererGL.cpp ──────────────────────────────── -->

- [ ] 173. [WMOLegacyRendererGL.cpp] Texture loading uses `set_blp()` — may not pass `generate_mipmaps = true`
  - **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js`
  - **Status**: Pending
  - **Details**: JS explicitly calls `gl_tex.set_rgba(pixels, w, h, { ..., generate_mipmaps: true })`. C++ calls `gl_tex->set_blp(blp, blp_flags)`. If `set_blp()` does not internally set `generate_mipmaps = true`, WMO legacy textures will lack mipmaps, causing rendering quality differences at distance. Verify `set_blp()` flags.

- [ ] 174. [WMOLegacyRendererGL.cpp] `updateSets()` is synchronous in C++; JS original is `async`
  - **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js`
  - **Status**: Pending
  - **Details**: JS `async updateSets()` defers doodad set loading. C++ `updateSets()` is synchronous, potentially causing a frame stall during doodad loading on the render thread.

- [ ] 175. [WMOLegacyRendererGL.cpp] `texture_ribbon::setSlotFileLegacy()` used where JS uses `textureRibbon.setSlotFile()`
  - **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js`
  - **Status**: Pending
  - **Details**: Same concern as entry 155 for M2LegacyRendererGL. Requires verification of `setSlotFileLegacy` behaviour relative to `setSlotFile`.

<!-- ─── src/js/3D/renderers/WMORendererGL.cpp ──────────────────────────────── -->

- [ ] 176. [WMORendererGL.cpp] `load()` and all sub-methods synchronous in C++; JS uses `async/await` throughout
  - **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 81–111
  - **Status**: Pending
  - **Details**: JS `load()`, `_load_textures()`, `_load_groups()`, `loadDoodadSet()`, and `updateSets()` are all async. C++ versions are synchronous. Doodad set loading runs on the render thread and may cause frame stalls for large WMOs.

- [ ] 177. [WMORendererGL.cpp] Vue `$watch` replaced by per-frame polling for groups, sets, and wireframe
  - **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 105–107
  - **Status**: Pending
  - **Details**: Necessary adaptation (no reactive framework in C++). Polling in `render()` via `prev_group_checked` / `prev_set_checked` vectors. Functionally equivalent.

- [ ] 178. [WMORendererGL.cpp] C++ constructor adds `fileName`-based overload not present in JS
  - **JS Source**: `src/js/3D/renderers/WMORendererGL.js`
  - **Status**: Pending
  - **Details**: JS constructor takes `(data, fileID, gl_context, useRibbon)`. C++ adds a second constructor taking `const std::string& fileName`. This is a C++ extension — not a deviation from JS semantics (both resolve to the same loading path).

- [ ] 179. [WMORendererGL.cpp] `isClassic` detection: JS uses `!!wmo.textureNames`; C++ uses `wmo->hasMotxChunk`
  - **JS Source**: `src/js/3D/renderers/WMORendererGL.js` line 126
  - **Status**: Pending
  - **Details**: JS checks `const isClassic = !!wmo.textureNames` (truthy for non-null object). C++ checks `const bool isClassic = wmo->hasMotxChunk`. Edge cases where `hasMotxChunk` is not set correctly in `WMOLoader::parse_MOTX()` would cause classic WMO texture name resolution to fail. Verify `WMOLoader::hasMotxChunk` semantics match JS truthy behaviour (see also entry 148).

- [ ] 180. [WMORendererGL.cpp] `dispose()` explicit clear differs from JS `splice(0)` array-reference semantics
  - **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 666–667
  - **Status**: Pending
  - **Details**: JS `this.groupArray.splice(0)` / `this.setArray.splice(0)` clears the shared array reference. C++ explicitly calls `.clear()` on both local and view arrays. Necessary adaptation (C++ value semantics vs JS reference semantics); functionally equivalent.

<!-- ─── src/js/3D/writers/CSVWriter.cpp ──────────────────────────────── -->

- [ ] 181. [CSVWriter.cpp] `escapeCSVField` does not handle null/undefined — uses `empty()` instead
  - **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 42–51
  - **Status**: Pending
  - **Details**: JS explicitly checks `value === null || value === undefined` and returns `''`. C++ checks `value.empty()` only (handles empty string, not a null-like sentinel). Since C++ `addRow` uses `unordered_map<string, string>`, callers must pre-convert nulls to `""`. The differing check is undocumented.

- [ ] 182. [CSVWriter.cpp] `addField` variadic split undocumented
  - **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 24–27
  - **Status**: Pending
  - **Details**: JS `addField(...fields)` accepts variadic arguments. C++ splits into `addField(const std::string&)` and `addField(const std::vector<std::string>&)`. Functional adaptation without a documentation comment.

- [ ] 183. [CSVWriter.cpp] `write()` is synchronous; JS original is `async`
  - **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 57–83
  - **Status**: Pending
  - **Details**: JS `async write(overwrite = true)` returns a Promise. C++ is synchronous and blocks the calling thread. Deviation undocumented in the file.

<!-- ─── src/js/3D/writers/GLBWriter.cpp ──────────────────────────────── -->

- [ ] 184. [GLBWriter.cpp] `bin_buffer` stored as reference without lifetime documentation
  - **JS Source**: `src/js/3D/writers/GLBWriter.js` lines 22–25
  - **Status**: Pending
  - **Details**: JS stores `bin_buffer` by value semantics. C++ stores it as `BufferWrapper&` (reference), meaning the GLBWriter cannot outlive the passed-in buffer. The lifetime constraint is not documented with a comment.

<!-- ─── src/js/3D/writers/GLTFWriter.cpp ──────────────────────────────── -->

- [ ] 185. [GLTFWriter.cpp] Spurious UV Y-axis flip applied to main model UVs — not present in JS
  - **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1026–1050
  - **Status**: Pending
  - **Details**: C++ `write()` flips all main model UV arrays on the Y axis: `uv[i + 1] = (uv[i + 1] - 1) * -1`. This operation does NOT exist in the JS source. JS writes UV arrays directly without any flip. This is a porting bug that produces different UV coordinates in exported GLTF/GLB files.

- [ ] 186. [GLTFWriter.cpp] Spurious UV Y-axis flip applied to equipment UVs — not present in JS
  - **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1219–1257
  - **Status**: Pending
  - **Details**: Same issue as entry 185 — C++ flips equipment UV arrays on the Y axis. JS source writes `equip.uv` directly without any flip. Produces incorrect UV coordinates for exported equipment meshes.

- [ ] 187. [GLTFWriter.cpp] `asset.generator` uses `"wow.export.cpp"` instead of `"wow.export"` — intentional deviation
  - **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 208–212
  - **Status**: Pending
  - **Details**: JS builds `asset.generator` as `util.format('wow.export v%s %s [%s]', ...)`. C++ uses `std::format("wow.export.cpp v{} {} [{}]", ...)`. Per CLAUDE.md this is intentional, but it is a functional change in exported GLTF metadata that third-party tools may inspect.

- [ ] 188. [GLTFWriter.cpp] `write()` is synchronous; JS original is `async`
  - **JS Source**: `src/js/3D/writers/GLTFWriter.js` line 194
  - **Status**: Pending
  - **Details**: JS `async write(overwrite = true, format = 'gltf')` uses `await` throughout. C++ is synchronous. Deviation undocumented in the file.

- [ ] 189. [GLTFWriter.cpp] `textures` map uses `std::map` (alphabetical order) instead of JS `Map` (insertion order)
  - **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 86–100
  - **Status**: Pending
  - **Details**: JS `Map` for `textures` preserves insertion order. C++ `std::map<std::string, GLTFTextureEntry>` is alphabetically ordered. Iteration order determines `materialIndex` assignment — C++ may assign different indices from JS, producing different GLTF output when textures are not inserted in alphabetical order.

- [ ] 190. [GLTFWriter.cpp] `texture_buffer_views` drops `fileDataID` tracking
  - **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 909–919
  - **Status**: Pending
  - **Details**: JS `texture_buffer_views` is `[{fileDataID, buffer: ...}]`. C++ `struct TextureBufferView { BufferWrapper buffer; }` omits `fileDataID`. Currently `fileDataID` is not used after push (images are found by iteration index), so this is functionally correct but diverges from the JS structure.

- [ ] 191. [GLTFWriter.cpp] `animationBufferMap` uses `std::map` (alphabetical order) vs JS `Map` (insertion order)
  - **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 304, 404
  - **Status**: Pending
  - **Details**: JS `Map` preserves insertion order for `animationBufferMap`. C++ `std::map<std::string, BufferWrapper>` sorts alphabetically by key. In GLB mode, animation buffers are packed in iteration order. Keys like `"10-0"` and `"2-0"` have different alphabetical vs insertion order, potentially producing different byte offsets in GLB animation data.

- [ ] 192. [GLTFWriter.cpp] `animation_buffer_lookup_map` uses `std::map` instead of JS `Map`
  - **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 351, 407–416
  - **Status**: Pending
  - **Details**: Same ordering issue as entry 191. C++ `std::map<std::string, int>` vs JS `Map`. Affects lookup correctness only when key ordering matters; functionally correct for present usage.

<!-- ─── src/js/3D/writers/JSONWriter.cpp ──────────────────────────────── -->

- [ ] 193. [JSONWriter.cpp] `write()` is synchronous; JS original is `async`
  - **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 33–45
  - **Status**: Pending
  - **Details**: JS `async write(overwrite = true)` returns a Promise. C++ is synchronous. Deviation undocumented.

- [ ] 194. [JSONWriter.cpp] BigInt serialization comment is inaccurate
  - **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 40–43
  - **Status**: Pending
  - **Details**: JS uses a custom replacer to handle BigInt serialization. C++ comment claims "nlohmann::json handles all types natively including large integers; no special BigInt serialization needed." However nlohmann/json does not handle arbitrary-precision integers beyond `UINT64_MAX`. If callers store values that were JS BigInts, pre-conversion to strings is required. Comment is misleading.

<!-- ─── src/js/3D/writers/MTLWriter.cpp ──────────────────────────────── -->

- [ ] 195. [MTLWriter.cpp] `isEmpty()` is a method; JS defines it as a getter property
  - **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 32–34
  - **Status**: Pending
  - **Details**: JS `get isEmpty()` is a getter property. C++ `bool isEmpty() const` is a regular method. Necessary adaptation (no property getters in C++). Callers must use `isEmpty()` with parentheses. Functionally equivalent.

- [ ] 196. [MTLWriter.cpp] `write()` is synchronous; JS original is `async`
  - **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 41–67
  - **Status**: Pending
  - **Details**: JS `async write(overwrite = true)` returns a Promise. C++ is synchronous. Deviation undocumented.

- [ ] 197. [MTLWriter.cpp] `path.resolve` vs `std::filesystem::weakly_canonical` for absolute path resolution
  - **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 60–63
  - **Status**: Pending
  - **Details**: JS `path.resolve(mtlDir, materialFile)` is a pure string operation. C++ `std::filesystem::weakly_canonical(mtlDir / materialFile).string()` touches the filesystem to resolve symlinks and on Windows converts forward slashes to backslashes, potentially producing Windows-style paths in the MTL output unlike the JS version which uses forward slashes.

<!-- ─── src/js/3D/writers/OBJWriter.cpp ──────────────────────────────── -->

- [ ] 198. [OBJWriter.cpp] `float_to_string` uses `%.17g` — produces more digits than JS `Number.toString()`
  - **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 170, 179, 188, 211
  - **Status**: Pending
  - **Details**: JS uses `Number.prototype.toString()` (Grisu/Dragon4 — shortest decimal that round-trips). C++ uses `snprintf(buf, size, "%.17g", ...)` always outputting up to 17 significant digits (e.g., `0.1` → `"0.10000000000000001"`). Produces larger OBJ files with longer number strings.

- [ ] 199. [OBJWriter.cpp] `write()` is synchronous; JS original is `async`
  - **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 138–243
  - **Status**: Pending
  - **Details**: JS `async write(overwrite = true)`. C++ is synchronous. Deviation undocumented.

<!-- ─── src/js/3D/writers/SQLWriter.cpp ──────────────────────────────── -->

- [ ] 200. [SQLWriter.cpp] `addField` variadic split undocumented
  - **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 47–49
  - **Status**: Pending
  - **Details**: Same pattern as CSVWriter (entry 182). JS `addField(...fields)` is variadic; C++ splits into two overloads without a documentation comment.

- [ ] 201. [SQLWriter.cpp] `write()` is synchronous; JS original is `async`
  - **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 210–230
  - **Status**: Pending
  - **Details**: JS `async write(overwrite = true)`. C++ is synchronous. Undocumented.

- [ ] 202. [SQLWriter.cpp] `generateDDL()` trailing blank line differs from JS output
  - **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 141–177
  - **Status**: Pending
  - **Details**: JS `generateDDL()` pushes an empty string at the end, joining with `'\n'` to produce a trailing `\n\n` (double newline after `);`). C++ output ends with a single `\n`. This is a minor formatting difference in generated DDL output.

- [ ] 203. [SQLWriter.cpp] `escapeSQLValue` uses `strtod` for numeric detection; JS uses `isNaN()`
  - **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 65–77
  - **Status**: Pending
  - **Details**: JS `!isNaN(value) && str.trim() !== ''` is the numeric check. C++ uses `strtod` on the trimmed string. Both correctly reject whitespace-only strings. Hex strings like `"0xFF"` may be handled differently depending on platform `strtod` behaviour. Noted in a code comment; functionally equivalent for expected manifest data.

<!-- ─── src/js/3D/writers/STLWriter.cpp ──────────────────────────────── -->

- [ ] 204. [STLWriter.cpp] STL binary header uses `"wow.export.cpp"` instead of `"wow.export"` — intentional
  - **JS Source**: `src/js/3D/writers/STLWriter.js` line 147
  - **Status**: Pending
  - **Details**: JS writes `'Exported using wow.export v' + constants.VERSION` into the 80-byte STL header. C++ writes `"Exported using wow.export.cpp v" + constants::VERSION`. Per CLAUDE.md this is intentional but is a functional change in the exported binary file.

- [ ] 205. [STLWriter.cpp] `write()` is synchronous; JS original is `async`
  - **JS Source**: `src/js/3D/writers/STLWriter.js` lines 131–249
  - **Status**: Pending
  - **Details**: JS `async write(overwrite = true)`. C++ is synchronous. Undocumented.

- [ ] 206. [STLWriter.cpp] `calculate_normal` is `public` in C++; should be `private`
  - **JS Source**: `src/js/3D/writers/STLWriter.js` lines 101–125
  - **Status**: Pending
  - **Details**: `calculate_normal` is an internal helper called only from `write()`. C++ header declares it `public`. Should be `private` to match JS intent (instance method not intended for external use).

<!-- ─── src/js/casc/blp.cpp ──────────────────────────────── -->

- [ ] 207. [blp.cpp] Missing `toCanvas()` and `drawToCanvas()` methods — undocumented deviation
  - **JS Source**: `src/js/casc/blp.js` lines 103–234
  - **Status**: Pending
  - **Details**: JS `BLPImage` has `toCanvas(mask, mipmap)` and `drawToCanvas(canvas, mipmap, mask)` for browser canvas rendering. These browser-specific methods cannot be ported. The C++ deviation comment only explains `getDataURL()`, not the missing canvas methods themselves. The absence should be documented in TODO_TRACKER.

- [ ] 208. [blp.cpp] `getDataURL()` never writes to `dataURL` cache field — always recomputes
  - **JS Source**: `src/js/casc/blp.js` lines 94–96
  - **Status**: Pending
  - **Details**: JS `getDataURL()` calls `this.toCanvas(mask, mipmap).toDataURL()` and callers can cache in `this.dataURL`. C++ has a `dataURL` optional member in `blp.h` but `getDataURL()` never writes to it. Every call regenerates the PNG unconditionally. The cache field exists but is never populated.

- [ ] 209. [blp.cpp] `_getCompressed`: uses `>=` instead of strict `===` for rawData length check
  - **JS Source**: `src/js/casc/blp.js` line 323
  - **Status**: Pending
  - **Details**: JS: `if (this.rawData.length === pos) continue;`. C++: `if (static_cast<size_t>(pos) >= rawData_.size()) continue;`. C++ is more defensive (handles `pos > size`), whereas JS uses strict equality. Documented in a code comment. Intentional and safe deviation.

- [ ] 210. [blp.cpp] DXT5 alpha block uses `alphaColours[8]` array; JS uses shadowed `colours` variable
  - **JS Source**: `src/js/casc/blp.js` lines 383–395
  - **Status**: Pending
  - **Details**: JS re-declares `let colours = []` inside the DXT5 block, shadowing the outer `colours` array. C++ uses a separate `int alphaColours[8]` array with a more explicit name. Functionally correct adaptation.

- [ ] 211. [blp.cpp] DXT5 alpha-interpolation truncation comment is incomplete
  - **JS Source**: `src/js/casc/blp.js` lines 388–395
  - **Status**: Pending
  - **Details**: JS uses `| 0` (bitwise-OR truncation) for alpha interpolation. C++ uses integer division (also truncates, equivalent). The existing deviation comment covers DXT colour truncation but does not explicitly mention the DXT5 alpha case. No functional issue — the comment should be extended for completeness.

<!-- ─── src/js/casc/blte-reader.cpp ──────────────────────────────── -->

- [ ] 212. [blte-reader.cpp] `getDataURL()` comment documents inverted condition — potentially confusing
  - **JS Source**: `src/js/casc/blte-reader.js` lines 346–353
  - **Status**: Pending
  - **Details**: C++ comment says `// JS: if (!this.dataURL)` but the code checks `hasDataURL()` to return early (same semantics). The comment documents the JS negative condition while the C++ code uses the positive form. No functional bug but may confuse readers.

- [ ] 213. [blte-reader.cpp] `_processBlock()` return value adaptation
  - **JS Source**: `src/js/casc/blte-reader.js` lines 163–192
  - **Status**: Pending
  - **Details**: JS `_processBlock()` returns `false` when no more blocks remain, `undefined` otherwise. C++ returns `false` when no blocks remain, `true` otherwise. Correct adaptation (C++ requires explicit return type). Functionally equivalent for the `_checkBounds` loop.

- [ ] 214. [blte-reader.cpp] `_writeBufferBLTE` implementation diverges from JS raw buffer copy pattern
  - **JS Source**: `src/js/casc/blte-reader.js` lines 302–305
  - **Status**: Pending
  - **Details**: JS: `buf.raw.copy(this._buf, this._ofs, buf.offset, blockEnd)`. C++: `std::memcpy(raw().data() + offset(), buf.raw().data() + buf.offset(), length)` where `length = blockEnd - buf.offset()`. Semantics are equivalent (copy `blockEnd - buf.offset` bytes). Correct adaptation.

<!-- ─── src/js/casc/blte-stream-reader.cpp ──────────────────────────────── -->

- [ ] 215. [blte-stream-reader.cpp] `streamBlocks()` uses callback pattern; JS original is `async *` generator
  - **JS Source**: `src/js/casc/blte-stream-reader.js` lines 199–202
  - **Status**: Pending
  - **Details**: JS `async *streamBlocks()` yields each decoded block. C++ uses `streamBlocks(callback)` with a callback parameter. Callers cannot `break` early or use range-for semantics. Necessary C++ adaptation (C++20 coroutines not used here). Undocumented in TODO_TRACKER.

- [ ] 216. [blte-stream-reader.cpp] `createReadableStream()` missing error/close signalling from Web Streams API
  - **JS Source**: `src/js/casc/blte-stream-reader.js` lines 168–193
  - **Status**: Pending
  - **Details**: JS `ReadableStream` uses `controller.enqueue()`, `controller.close()`, and `controller.error(e)`. C++ `ReadableStream` only provides `pull()` returning `std::optional<std::vector<uint8_t>>` and `cancel()`. The error and close signals from the JS API are not replicated, so callers relying on those signals need adaptation.

- [ ] 217. [blte-stream-reader.cpp] Block cache eviction uses `std::deque` for insertion order; JS `Map` iteration order is insertion order
  - **JS Source**: `src/js/casc/blte-stream-reader.js` lines 72–74
  - **Status**: Pending
  - **Details**: JS evicts via `this.blockCache.keys().next().value` using Map insertion order. C++ uses `std::unordered_map` (no insertion order) plus a `std::deque<size_t> cacheOrder` to maintain insertion order. Necessary and functionally correct adaptation.

- [ ] 218. [blte-stream-reader.cpp] `_decodeBlock` catches only `EncryptionError`; JS rethrows all non-EncryptionError exceptions
  - **JS Source**: `src/js/casc/blte-stream-reader.js` lines 92–104
  - **Status**: Pending
  - **Details**: JS catch block checks `if (e instanceof EncryptionError)` then has `throw e` for other errors. C++ catches `const EncryptionError&` only — other exceptions propagate naturally. Correct C++ idiom; functionally equivalent.

<!-- ─── src/js/casc/build-cache.cpp ──────────────────────────────── -->

- [ ] 219. [build-cache.cpp] `storeFile()` runtime type check replaced by compile-time type guarantee
  - **JS Source**: `src/js/casc/build-cache.js` lines 119–120
  - **Status**: Pending
  - **Details**: JS throws if `!(data instanceof BufferWrapper)`. C++ omits this check since the type is guaranteed at compile time. Necessary and correct adaptation.

- [ ] 220. [build-cache.cpp] `storeFile()` calls `data.writeToFile(filePath)` vs JS `fsp.writeFile(filePath, data.raw)`
  - **JS Source**: `src/js/casc/build-cache.js` line 134
  - **Status**: Pending
  - **Details**: JS writes `data.raw` (raw Node.js Buffer) directly. C++ calls `data.writeToFile(filePath)`. Functionally equivalent if `writeToFile` writes the entire raw buffer. No issue in practice.

- [ ] 221. [build-cache.cpp] Module IIFE initialization uses `cacheIntegrityReady` event in JS; C++ uses `condition_variable`
  - **JS Source**: `src/js/casc/build-cache.js` lines 156–171
  - **Status**: Pending
  - **Details**: JS IIFE loads cache integrity then emits `'cache-integrity-ready'`. C++ uses `cacheIntegrityLoaded` flag + `cacheIntegrityCV` condition variable. C++ still emits `"cache-integrity-ready"` after signalling the CV for event-based waiter compatibility. Necessary adaptation.

- [ ] 222. [build-cache.cpp] `casc-source-changed` cleanup: JS uses `!isNaN(manifest.lastAccess)`; C++ uses `json.contains() && is_number()`
  - **JS Source**: `src/js/casc/build-cache.js` line 230
  - **Status**: Pending
  - **Details**: JS `!isNaN(manifest.lastAccess)` returns true for any numeric value. C++ uses `.contains("lastAccess") && .is_number()`. Semantically equivalent. No functional issue.

- [ ] 223. [build-cache.cpp] `manifestSize` uses `std::string::size()` vs JS `Buffer.byteLength(str, 'utf8')`
  - **JS Source**: `src/js/casc/build-cache.js` line 228
  - **Status**: Pending
  - **Details**: JS `Buffer.byteLength(manifestRaw, 'utf8')` returns the UTF-8 byte count. C++ `manifestRaw.size()` returns the character count of `std::string`. For ASCII-only JSON manifest data (hex strings and numbers) these are identical. No functional issue in practice.

- [ ] 224. [build-cache.cpp] `cacheExpiry` config reading: JS uses `Number(value) || 0`; C++ handles string and number types separately
  - **JS Source**: `src/js/casc/build-cache.js` line 200
  - **Status**: Pending
  - **Details**: JS `Number(core.view.config.cacheExpiry) || 0` handles both string and number in one coercion. C++ explicitly checks JSON type and calls `std::stoll()` for strings, falling back to 0 on exception. Functionally equivalent but more verbose.

<!-- ─── src/js/casc/casc-source-local.cpp ──────────────────────────────── -->

- [ ] 225. [casc-source-local.cpp] `init()` is synchronous in C++; JS is `async`
  - **JS Source**: `src/js/casc/casc-source-local.js` lines 42–52
  - **Status**: Pending
  - **Details**: JS `init()` is async with Promise-based initialization. C++ handles async via `initAsync()`. Main `init()` is synchronous. Necessary adaptation.

- [ ] 226. [casc-source-local.cpp] `getProductList()` branch/version formatting — minor implementation difference
  - **JS Source**: `src/js/casc/casc-source-local.js` lines 148–158
  - **Status**: Pending
  - **Details**: JS accesses `entry.Branch.toUpperCase()` and `entry.Version` as direct Map properties then calls `util.format`. C++ uses `.find()` on `unordered_map` and `std::format`. JS and C++ produce the same formatted output. No functional deviation.

- [ ] 227. [casc-source-local.cpp] `load()` calls `tact_keys::waitForLoad()` — JS `load()` does not
  - **JS Source**: `src/js/casc/casc-source-local.js` lines 165–186
  - **Status**: Pending
  - **Details**: C++ `load()` calls `tact_keys::waitForLoad()` before proceeding. The JS counterpart does not. This undocumented deviation ensures encryption keys are available before decryption but diverges from JS behaviour.

- [ ] 228. [casc-source-local.cpp] `loadEncoding()` encoding key extraction
  - **JS Source**: `src/js/casc/casc-source-local.js` lines 292–299
  - **Status**: Pending
  - **Details**: JS: `const encKeys = this.buildConfig.encoding.split(' '); ... getDataFileWithRemoteFallback(encKeys[1])` — uses index 1 of space-split array. C++ finds the first space and takes everything after it, equivalent to `split(' ')[1]` for a two-element split. Functionally equivalent.

- [ ] 229. [casc-source-local.cpp] `initializeRemoteCASC()`: `this->remote` assigned before `preload()` completes — partial state on failure
  - **JS Source**: `src/js/casc/casc-source-local.js` lines 324–332
  - **Status**: Pending
  - **Details**: JS assigns `this.remote = remote` only AFTER `await remote.preload()` completes successfully. C++ assigns `this->remote = std::make_unique<CASCRemote>(...)` before calling `remote->preload()`. If `preload()` throws, `this->remote` is already set to a partially-initialized state, which may cause re-initialization to skip on a subsequent call.

- [ ] 230. [casc-source-local.cpp] `_ensureFileInCache()` casts `BLTEReader` to `BufferWrapper` via move before `storeFile`
  - **JS Source**: `src/js/casc/casc-source-local.js` lines 466–470
  - **Status**: Pending
  - **Details**: JS passes `BLTEReader` (extends `BufferWrapper`) directly to `storeFile`. C++ must `static_cast<BufferWrapper&>(blte)` then move before passing to `storeFile(BufferWrapper&)`. Requires all blocks to be processed first. Necessary adaptation.

<!-- ─── src/js/casc/casc-source-remote.cpp ──────────────────────────────── -->

- [ ] 231. [casc-source-remote.cpp] `init()` uses `Promise.allSettled` in JS; C++ uses futures that may differ for fulfilled-but-no-region-match case
  - **JS Source**: `src/js/casc/casc-source-remote.js` lines 47–55
  - **Status**: Pending
  - **Details**: JS `Promise.allSettled` pushes `undefined` for a fulfilled-but-no-matching-region result. C++ pushes an empty map `{}` for the same case. The `entry.empty()` check in `getProductList()` handles this. Functionally equivalent.

- [ ] 232. [casc-source-remote.cpp] `getFile()` null check: JS checks `data === null`; C++ checks `data.byteLength() == 0`
  - **JS Source**: `src/js/casc/casc-source-remote.js` lines 148–156
  - **Status**: Pending
  - **Details**: JS `generics.downloadFile` returns `null` on failure; C++ `generics::downloadFile` returns an empty `BufferWrapper`. If `downloadFile` can return a non-empty but invalid buffer on failure, this check would differ. Depends on `generics::downloadFile` behaviour.

- [ ] 233. [casc-source-remote.cpp] `parseArchiveIndex()` reads `offset` and `size` in reversed order — critical bug
  - **JS Source**: `src/js/casc/casc-source-remote.js` line 427
  - **Status**: Pending
  - **Details**: JS line 427: `{ key, size: data.readInt32BE(), offset: data.readInt32BE() }` — reads size first, then offset. C++ `ArchiveEntry` struct has fields `{ key, offset, size }` and the initializer `ArchiveEntry{ key, data.readInt32BE(), data.readInt32BE() }` reads into `offset` first, `size` second. This reversal is a critical bug: `offset` will contain the binary `size` field value and vice versa. Archive file lookups will use wrong offsets and sizes.

- [ ] 234. [casc-source-remote.cpp] `loadArchives()` total size: JS `.reduce()`; C++ `std::stoll()` iteration
  - **JS Source**: `src/js/casc/casc-source-remote.js` lines 375–376
  - **Status**: Pending
  - **Details**: JS `archivesIndexSize.split(' ').reduce((sum, e) => sum + Number(e), 0)`. C++ iterates tokens adding with `std::stoll()`. Functionally equivalent.

- [ ] 235. [casc-source-remote.cpp] `load()` calls `tact_keys::waitForLoad()` — JS `load()` does not
  - **JS Source**: `src/js/casc/casc-source-remote.js` lines 283–296
  - **Status**: Pending
  - **Details**: Same undocumented deviation as entry 227 for `CASCLocal`. C++ `load()` calls `tact_keys::waitForLoad()` before `core::showLoadingScreen(12)`. JS does not.

- [ ] 236. [casc-source-remote.cpp] `_ensureFileInCache()` casts `BLTEReader` to `BufferWrapper` via move
  - **JS Source**: `src/js/casc/casc-source-remote.js` lines 513–517
  - **Status**: Pending
  - **Details**: Same pattern as entry 230 for `CASCLocal`. Necessary adaptation.

<!-- ─── src/js/casc/casc-source.cpp ──────────────────────────────── -->

- [ ] 237. [casc-source.cpp] `parseEncodingFile()` inner loop uses `cKeyPageSize` where JS uses `pagesStart` — C++ fixes JS bug
  - **JS Source**: `src/js/casc/casc-source.js` line 470
  - **Status**: Pending
  - **Details**: JS line 470: `while (encoding.offset < (pageStart + pagesStart))` — a JS bug (should be `cKeyPageSize`, not `pagesStart`). C++ correctly uses `cKeyPageSize`. This is a case where C++ is more correct than the JS original. Documented as a known bug-fix deviation.

- [ ] 238. [casc-source.cpp] `getFileByName()` calls `getFileAsBLTE()` instead of virtual `getFile()`
  - **JS Source**: `src/js/casc/casc-source.js` line 190
  - **Status**: Pending
  - **Details**: JS `this.getFile(...)` dispatches polymorphically to subclass override. C++ `getFileAsBLTE()` is the correct virtual dispatch equivalent. Necessary and correct adaptation.

- [ ] 239. [casc-source.cpp] `loadListfile()` passes `unordered_set<uint32_t>` of IDs; JS passes full `Map` of root entries
  - **JS Source**: `src/js/casc/casc-source.js` line 284
  - **Status**: Pending
  - **Details**: JS `listfile.applyPreload(this.rootEntries)` passes the entire Map of `fileDataID → content entries`. C++ builds `std::unordered_set<uint32_t>` of just the fileDataIDs and passes that. Verify against `listfile.cpp` that `listfile::applyPreload` only needs the IDs — if it accesses content values the implementations diverge.

- [ ] 240. [casc-source.cpp] CASC constructor uses `$watch` with `immediate:true` in JS; C++ reads config once and registers event
  - **JS Source**: `src/js/casc/casc-source.js` lines 31–38
  - **Status**: Pending
  - **Details**: JS `core.view.$watch('config.cascLocale', handler, { immediate: true })` fires immediately and on each change to `cascLocale`. C++ reads the current config once in the constructor then registers a `"config-change"` event handler. The `immediate: true` is replicated by the initial read. However the C++ `"config-change"` event fires for any config change (not just `cascLocale`), so locale is re-read more often than necessary.

- [ ] 241. [casc-source.cpp] `getValidRootEntries()` iterates key-value pairs; JS uses `entry.keys()` (keys only)
  - **JS Source**: `src/js/casc/casc-source.js` lines 51–53
  - **Status**: Pending
  - **Details**: JS `for (const rootTypeIdx of entry.keys())` iterates only keys. C++ `for (const auto& [rootTypeIdx, _key] : entry)` iterates pairs but discards the value. Functionally equivalent.

- [ ] 242. [casc-source.cpp] `rootEntries` uses `std::unordered_map` (no insertion order); JS uses `Map` (insertion order preserved)
  - **JS Source**: `src/js/casc/casc-source.js` line 27
  - **Status**: Pending
  - **Details**: JS `this.rootEntries = new Map()` preserves insertion order. C++ `std::unordered_map<uint32_t, std::unordered_map<size_t, std::string>>` does not. For current `getFile()` and `getValidRootEntries()` uses (key lookups), ordering does not matter. No functional issue for present use cases.

- [ ] 243. [casc-source.cpp] `getFileAsBLTE()` base implementation throws at runtime instead of being pure virtual
  - **JS Source**: `src/js/casc/casc-source.js`
  - **Status**: Pending
  - **Details**: C++ `CASC::getFileAsBLTE()` throws `std::runtime_error` if called on the base class but is not declared `= 0` (pure virtual), meaning subclasses are not forced to override it at compile time. Making it pure virtual would be safer and match the JS polymorphic intent.

<!-- ─── src/js/casc/cdn-config.cpp ──────────────────────────────── -->

- [ ] 244. [cdn-config.cpp] Comment-skip check applied to untrimmed line — minor note
  - **JS Source**: `src/js/casc/cdn-config.js` lines 40–41
  - **Status**: Pending
  - **Details**: JS `line.startsWith('#')` is called on the original untrimmed line. C++ mirrors this with `lineView.front() == '#'`. The empty-line check uses `trim(lineView).empty()` in C++, matching JS `line.trim().length === 0`. Both checks are logically equivalent. No functional issue; minor code-style note.

- [ ] 245. [cdn-config.cpp] `std::regex` usage is a performance concern vs. JS V8 regex
  - **JS Source**: `src/js/casc/cdn-config.js` lines 6, 43–45
  - **Status**: Pending
  - **Details**: C++ uses `std::regex` (compiled once as a static) to match `KEY_VAR_PATTERN`. JS uses a V8-compiled regex which is significantly faster. `std::regex` can be much slower on large config files. The logic is correct — this is a necessary C++ adaptation but may impact performance.

<!-- ─── src/js/casc/cdn-resolver.cpp ──────────────────────────────── -->

- [ ] 246. [cdn-resolver.cpp] `backgroundFutures` vector not mutex-protected against concurrent `startPreResolution` calls — data race
  - **JS Source**: `src/js/casc/cdn-resolver.js` lines 32–35
  - **Status**: Pending
  - **Details**: `backgroundFutures` vector is accessed in `startPreResolution()` without holding `cacheMutex`. Concurrent calls from multiple threads could cause a data race on the vector's erase/push_back. `cacheMutex` only guards `resolutionCache` and `failedHosts`. A separate mutex or extended `cacheMutex` coverage is needed.

- [ ] 247. [cdn-resolver.cpp] `getBestHostAsync` / `getRankedHostsAsync` have no JS counterpart — C++-only additions
  - **JS Source**: `src/js/casc/cdn-resolver.js` lines 43–73, 83–116
  - **Status**: Pending
  - **Details**: Two C++-only functions `getBestHostAsync` and `getRankedHostsAsync` wrap the synchronous functions in `std::async`. These are not direct ports of any JS method. They should be documented as C++-only additions or removed if unused.

- [ ] 248. [cdn-resolver.cpp] `_resolveRegionProduct` calls `getBestHost` synchronously; JS `await`s it as a Promise
  - **JS Source**: `src/js/casc/cdn-resolver.js` line 162
  - **Status**: Pending
  - **Details**: JS `await this.getBestHost(region, serverConfig)`. C++ calls `getBestHost` synchronously inside a `std::async` background thread. C++ `getBestHost` may internally fire another `std::async` and block on `existingFuture.get()`, resulting in a nested async-inside-async pattern that allocates an extra thread. Necessary adaptation.

<!-- ─── src/js/casc/content-flags.cpp ──────────────────────────────── -->

- [ ] 249. [content-flags.cpp] Static `exports` array in `.cpp` file is unused dead code
  - **JS Source**: `src/js/casc/content-flags.js` lines 4–14
  - **Status**: Pending
  - **Details**: The `.cpp` file defines a `[[maybe_unused]] static constexpr` array of `{name, value}` pairs mirroring the JS `module.exports` shape. This array is never referenced anywhere in the codebase. Actual constants are correctly declared in `content-flags.h`. The array should be removed or replaced with a runtime lookup function if dynamic name→value lookup is needed.

<!-- ─── src/js/casc/db2.cpp ──────────────────────────────── -->

- [ ] 250. [db2.cpp] `getTable()` eagerly parses on every call; JS Proxy only parses on first method invocation
  - **JS Source**: `src/js/casc/db2.js` lines 58–67
  - **Status**: Pending
  - **Details**: JS `db2.SomeTable` returns a Proxy without parsing; parsing begins only when a method is called. C++ `getTable()` calls `entry_ptr->ensureParsed()` immediately before returning. Even unused table references trigger parsing. `std::call_once` ensures parse runs only once but it is initiated earlier than in JS.

- [ ] 251. [db2.cpp] JS `preload_proxy` returns an `async` function (Promise); C++ `preloadTable()` is synchronous/blocking
  - **JS Source**: `src/js/casc/db2.js` lines 10–35
  - **Status**: Pending
  - **Details**: JS `db2.preload.SomeTable` returns `async () => { ... }` — calling it returns a `Promise<wrapper>`. C++ `preloadTable()` blocks until parse + preload complete. Callers requiring async preloading must manually wrap `preloadTable()` in `std::async`. Necessary adaptation.

- [ ] 252. [db2.cpp] JS `create_wrapper` enforces `getRelationRows` requires preload; C++ delegates to WDCReader
  - **JS Source**: `src/js/casc/db2.js` lines 46–55
  - **Status**: Pending
  - **Details**: JS Proxy intercepts `getRelationRows` and throws with a detailed message if not preloaded. C++ code comment states "WDCReader::getRelationRows() enforces the same preload requirement." Verify against `WDCReader.cpp` that the error messages match. If they differ, callers relying on error text for diagnostics will see different output.

- [ ] 253. [db2.cpp] `CacheEntry::ensureParsed` / `ensurePreloaded` `once_flag` interaction — note only
  - **JS Source**: `src/js/casc/db2.js` lines 19–23
  - **Status**: Pending
  - **Details**: `ensurePreloaded()` calls `ensureParsed()` inside its `call_once` lambda. The interaction of two separate `once_flag`s is correct. This note documents the pattern for future maintainers — no bug exists.

<!-- ─── src/js/casc/dbd-manifest.cpp ──────────────────────────────── -->

- [ ] 254. [dbd-manifest.cpp] `prepareManifest()` is synchronous/blocking; JS original is `async` returning `Promise<boolean>`
  - **JS Source**: `src/js/casc/dbd-manifest.js` lines 50–59
  - **Status**: Pending
  - **Details**: JS `async prepareManifest()` is awaited by callers. C++ returns `bool` synchronously, blocking the calling thread until `preload_promise->wait()` completes. Callers requiring non-blocking behaviour must run on a background thread. Necessary adaptation documented in the file.

- [ ] 255. [dbd-manifest.cpp] `jsonTruthy()` helper is over-engineered for the actual manifest data format
  - **JS Source**: `src/js/casc/dbd-manifest.js` line 31
  - **Status**: Pending
  - **Details**: JS `if (entry.tableName && entry.db2FileDataID)` is a simple truthy check. C++ `jsonTruthy()` replicates full JS truthiness semantics for float/int/bool/string/object. Manifest data always has string `tableName` and integer `db2FileDataID`. The boolean/float/object branches are dead code adding unnecessary complexity. A simpler check would be more correct for the expected data format.

- [ ] 256. [dbd-manifest.cpp] `table_to_id` and `id_to_table` use `int` — may overflow for large FileDataIDs
  - **JS Source**: `src/js/casc/dbd-manifest.js` lines 32–34
  - **Status**: Pending
  - **Details**: Maps `table_to_id` and `id_to_table` use `int` as the key/value type. WoW FileDataIDs are 32-bit unsigned integers. IDs above 2,147,483,647 (0x7FFFFFFF) would overflow a signed `int`. Should use `uint32_t` or `int64_t` to match JS number semantics.

- [ ] 257. [dbd-manifest.cpp] Pre-existing `TODO 191` and `TODO 192` comments already implemented in code — stale
  - **JS Source**: `src/js/casc/dbd-manifest.js` lines 10–13
  - **Status**: Pending
  - **Details**: C++ file contains `// TODO 191:` and `// TODO 192:` comments describing threading concerns. The code already implements `std::atomic<bool>` (for TODO 191) and `std::call_once` (for TODO 192) as the solutions. The TODO comments say what needs to be done but the code already does it. They are stale and should be removed or converted to completion notes.

<!-- ─── src/js/casc/export-helper.cpp ──────────────────────────────── -->

- [ ] 258. [export-helper.cpp] `mark()` logs stack trace to app log file instead of stdout like JS
  - **JS Source**: `src/js/casc/export-helper.js` lines 286–287
  - **Status**: Pending
  - **Details**: JS uses `console.log(stackTrace)` which goes to stdout/NW.js devtools, bypassing the log system. C++ uses `logging::write(stackTrace.value())`, sending it to the log file. Callers relying on stdout for stack trace output will not see it in C++.

<!-- ─── src/js/casc/listfile.cpp ──────────────────────────────────────── -->

- [ ] 259. [listfile.cpp] `listfile_check_cache_expiry` may throw on malformed config value
  - **JS Source**: `src/js/casc/listfile.js` lines 65–66
  - **Status**: Pending
  - **Details**: JS uses `Number(core.view.config.listfileCacheRefresh) || 0`, coercing non-numeric/null values to 0. C++ uses `config["listfileCacheRefresh"].get<int64_t>()` which throws `nlohmann::json::type_error` on malformed values. Should guard with try/catch or use `.value()` with a fallback.

- [ ] 260. [listfile.cpp] `getFilenamesByExtension` binary mode does not guard against missing map key
  - **JS Source**: `src/js/casc/listfile.js` lines 661–680
  - **Status**: Pending
  - **Details**: In the binary mode path, `binary_id_to_pf_index.find(fileDataID)` is called without checking if the iterator equals `end()`. Dereferencing a past-end iterator is undefined behavior. JS uses `Map.get()` which safely returns `undefined` on miss.

- [ ] 261. [listfile.cpp] TOCTOU race in `prepareListfileAsync()` — `is_preloaded` checked outside mutex
  - **JS Source**: `src/js/casc/listfile.js` lines 498–507
  - **Status**: Pending
  - **Details**: `is_preloaded` is read without holding the mutex, then the mutex is acquired for the `preload_future` check. On a multi-core system another thread could set `is_preloaded` between the two checks. JS is single-threaded and has no equivalent race.

<!-- ─── src/js/casc/locale-flags.cpp ─────────────────────────────────── -->

- [ ] 262. [locale-flags.cpp] `flags_exports` and `names_exports` static arrays in .cpp are dead code
  - **JS Source**: `src/js/casc/locale-flags.js` lines 4–40
  - **Status**: Pending
  - **Details**: Two `[[maybe_unused]]` static constexpr arrays mirror JS `module.exports.flags` and `module.exports.names` but are never used. All actual usage goes through the `entries` array in the header. These duplicate data already in `locale-flags.h` and should be removed.

<!-- ─── src/js/casc/realmlist.cpp ─────────────────────────────────────── -->

- [ ] 263. [realmlist.cpp] Missing/malformed `realmListURL` config silently produces "undefined" URL instead of throwing
  - **JS Source**: `src/js/casc/realmlist.js` lines 39–41
  - **Status**: Pending
  - **Details**: JS converts the config value via `String()` then has an explicit throw for malformed values. C++ `jsStringCoerce` returns the string "undefined" for null and proceeds silently, using an invalid URL. Should validate the config value and throw/log an error if missing.

- [ ] 264. [realmlist.cpp] `jsStringCoerce` helper is over-engineered for a single config URL lookup
  - **JS Source**: `src/js/casc/realmlist.js` line 39
  - **Status**: Pending
  - **Details**: JS simply calls `String(...)`. C++ implements full JS-style string coercion (null, bool, number, float, array, object), most of which is dead code for this single use case. A simpler string extraction with a null/empty check would be more readable and faithful.

<!-- ─── src/js/casc/tact-keys.cpp ─────────────────────────────────────── -->

- [ ] 265. [tact-keys.cpp] Stale TODO 205 comment in `loadAsync()` describes already-correct logic
  - **JS Source**: `src/js/casc/tact-keys.js` lines 99–101
  - **Status**: Pending
  - **Details**: The TODO 205 comment claims the line-splitting logic needs to match JS `split(' ')` with `parts.length !== 2`. The implementation already correctly handles this. The comment is misleading and should be removed.

<!-- ─── src/js/casc/vp9-avi-demuxer.cpp ──────────────────────────────── -->

- [ ] 266. [vp9-avi-demuxer.cpp] Stale TODO 207 comment in `find_chunk()` describes already-correct loop bound
  - **JS Source**: `src/js/casc/vp9-avi-demuxer.js` line 69
  - **Status**: Pending
  - **Details**: TODO 207 claims `i < data.length - 4` needs fixing. C++ already uses `i + 4 < data.size()`, which is mathematically identical. The comment should be removed.

- [ ] 267. [vp9-avi-demuxer.cpp] `extract_frames()` async generator converted to synchronous callback — deviation undocumented
  - **JS Source**: `src/js/casc/vp9-avi-demuxer.js` lines 83–126
  - **Status**: Pending
  - **Details**: JS uses `async*` generator; callers use `for await`. C++ uses a synchronous `std::function<void(const FrameInfo&)>` callback. This is a necessary adaptation but differs structurally. The deviation should be documented with a comment in the source.

- [ ] 268. [vp9-avi-demuxer.cpp] `find_chunk()` `const char fourcc[4]` parameter decays to pointer — unsafe signature
  - **JS Source**: `src/js/casc/vp9-avi-demuxer.js` lines 67–77
  - **Status**: Pending
  - **Details**: The array parameter decays to `const char*` with no length information. Calling with a string shorter than 4 chars reads past the end. Should use `std::string_view` or `const char (&)[5]` for a null-terminated literal.

<!-- ─── src/js/components/checkboxlist.cpp ───────────────────────────── -->

- [ ] 269. [checkboxlist.cpp] `scrollIndex()` can return negative value before caller clamps it
  - **JS Source**: `src/js/components/checkboxlist.js` lines 67–69
  - **Status**: Pending
  - **Details**: JS computed property never goes negative because `scrollRel` is clamped to [0,1]. C++ `scrollIndex()` can return a negative rounded value when `items.size() < state.slotCount`. The caller guards with `std::max(0, idx)` but the function's own contract differs from the JS property.

- [ ] 270. [checkboxlist.cpp] Scrollbar thumb rendered with raw `ImDrawList::AddRectFilled` (CLAUDE.md violation)
  - **JS Source**: `src/js/components/checkboxlist.js` lines 170–171
  - **Status**: Pending
  - **Details**: Per CLAUDE.md, `ImDrawList` calls should be reserved for effects with no native equivalent. The custom scrollbar thumb is rendered entirely with `AddRectFilled` calls. A native ImGui child window scrollbar or styled region should be used instead.

- [ ] 271. [checkboxlist.cpp] Checked items do not receive a highlighted row background
  - **JS Source**: `src/js/components/checkboxlist.js` line 172
  - **Status**: Pending
  - **Details**: JS applies a `selected` CSS class to each item row when `item.checked` is true, giving a visual highlight. The C++ render loop does not change the row background for checked items — only the checkbox widget itself reflects the checked state.

<!-- ─── src/js/components/combobox.cpp ────────────────────────────────── -->

- [ ] 272. [combobox.cpp] Extra `PushStyleVar(FramePadding, {15,10})` for dropdown items not in JS source
  - **JS Source**: `src/js/components/combobox.js` lines 91–92
  - **Status**: Pending
  - **Details**: The JS component defines no explicit padding for dropdown `<li>` items. C++ applies `ImGuiStyleVar_FramePadding = {15, 10}` which is a CSS approximation not derived from the JS component definition.

- [ ] 273. [combobox.cpp] Hard-coded `PushStyleColor` calls for dropdown hover colors violate CLAUDE.md
  - **JS Source**: `src/js/components/combobox.js` lines 87–93
  - **Status**: Pending
  - **Details**: Lines 212–214 push hard-coded RGBA color `{0.208, 0.208, 0.208, 1.0}` for `ImGuiCol_HeaderHovered` and `ImGuiCol_HeaderActive`. Per CLAUDE.md, `PushStyleColor` calls solely to match CSS values should not be added and should be progressively removed.

<!-- ─── src/js/components/context-menu.cpp ──────────────────────────── -->

- [ ] 274. [context-menu.cpp] JS repositions via `$nextTick` (deferred); C++ repositions synchronously
  - **JS Source**: `src/js/components/context-menu.js` lines 40–44
  - **Status**: Pending
  - **Details**: JS uses `this.$nextTick(() => this.reposition())` to defer until after DOM update. C++ calls `reposition(state)` synchronously when the node becomes active. In practice no visible difference, but the timing differs structurally.

- [ ] 275. [context-menu.cpp] Close-on-click only fires for left mouse button; JS `@click` fires for all buttons
  - **JS Source**: `src/js/components/context-menu.js` line 54
  - **Status**: Pending
  - **Details**: C++ checks only `ImGui::IsMouseClicked(0)` (left button). JS `@click` fires for any mouse button by default, which would also close the menu on right/middle click.

- [ ] 276. [context-menu.cpp] `context-menu-zone` hover buffer uses fixed 20px padding on all sides; JS CSS is asymmetric
  - **JS Source**: `src/js/components/context-menu.js` lines 54–57
  - **Status**: Pending
  - **Details**: The JS hover zone extends -20px from top/left/right with a different downward extent. C++ inflates by `contextMenuZonePadding = 20.0f` uniformly on all sides. The geometry may not match the JS CSS positioning exactly.

<!-- ─── src/js/components/data-table.cpp ─────────────────────────────── -->

- [ ] 277. [data-table.cpp] Selection tracks integer indices instead of row object references — fundamental semantic difference
  - **JS Source**: `src/js/components/data-table.js` lines 129–159, 836–868
  - **Status**: Pending
  - **Details**: JS `selection` stores actual row objects (reference identity). `filteredItems`, `handleKey`, and `selectRow` all use object identity checks. C++ stores integer indices into `sortedItems`. After a filter change, an index may refer to a different row than the JS would track, breaking multi-select semantics across filter changes.

- [ ] 278. [data-table.cpp] String sort uses byte comparison (`std::string::compare`) instead of locale-aware `localeCompare`
  - **JS Source**: `src/js/components/data-table.js` lines 193–197
  - **Status**: Pending
  - **Details**: JS sort calls `aStr.localeCompare(bStr)` which is locale-aware. C++ uses `aStr.compare(bStr)` — a simple lexicographic byte comparison. For Unicode content or locale-specific sort order the results differ.

- [ ] 279. [data-table.cpp] `wheelMouse` cannot call `e.preventDefault()` for horizontal scroll — event propagates
  - **JS Source**: `src/js/components/data-table.js` line 650
  - **Status**: Pending
  - **Details**: JS calls `e.preventDefault()` when handling horizontal scroll (shift+wheel) to prevent browser page scroll. C++ cannot prevent the underlying GLFW/OS scroll event from propagating.

- [ ] 280. [data-table.cpp] `resize()` called every frame; JS only calls on ResizeObserver events
  - **JS Source**: `src/js/components/data-table.js` lines 436–444
  - **Status**: Pending
  - **Details**: C++ calls `resize(...)` every frame unconditionally. JS fires only when the ResizeObserver detects a layout change. Calling every frame may cause subtle scroll position drift from floating-point accumulation.

- [ ] 281. [data-table.cpp] Header and body rendered with raw `ImDrawList` calls instead of `ImGui::BeginTable` (CLAUDE.md violation)
  - **JS Source**: `src/js/components/data-table.js` lines 976–1020
  - **Status**: Pending
  - **Details**: The entire table header and body are rendered via `drawList->AddRectFilled`, `drawList->AddText`, `drawList->AddRect`, etc. Per CLAUDE.md, native `ImGui::BeginTable`/`TableNextRow`/`TableSetColumnIndex` widgets must be used for table rendering.

- [ ] 282. [data-table.cpp] Selection lookup is O(n) per rendered row; JS uses O(1) `Set`
  - **JS Source**: `src/js/components/data-table.js` lines 120–122
  - **Status**: Pending
  - **Details**: JS computes `new Set(this.selection)` for O(1) `has()` lookups in the template. C++ uses `std::find` on a `std::vector<int>` for each rendered row — O(n) per row, O(n^2) total. Affects performance on large tables.

<!-- ─── src/js/components/file-field.cpp ─────────────────────────────── -->

- [ ] 283. [file-field.cpp] JS opens dialog on text field focus; C++ adds a separate `...` button (UI deviation)
  - **JS Source**: `src/js/components/file-field.js` lines 34–39, 46
  - **Status**: Pending
  - **Details**: JS template uses `<input @focus="openDialog">` — the file dialog opens automatically when the input is focused. C++ adds a separate `...` button that must be clicked to open the dialog. The JS has no such button. This changes the user interaction model and visual layout significantly.

- [ ] 284. [file-field.cpp] Extra utility functions `openDirectoryDialog`, `openFileDialog`, `saveFileDialog` beyond JS component scope
  - **JS Source**: `src/js/components/file-field.js` (entire file)
  - **Status**: Pending
  - **Details**: The JS component only exposes `openDialog()`. C++ additionally exposes three standalone utility functions with no JS counterparts in this file. These are C++ additions that extend the API surface beyond the original component.

<!-- ─── src/js/components/home-showcase.cpp ──────────────────────────── -->

- [ ] 285. [home-showcase.cpp] Stub comment says index "chosen randomly at startup" but `HomeShowcaseState.index` defaults to -1
  - **JS Source**: `src/js/components/home-showcase.js` lines 12–13
  - **Status**: Pending
  - **Details**: JS `data()` initializes `index: get_random_index()`. C++ `HomeShowcaseState` initializes `index = -1`. The stub comment is inaccurate — callers must explicitly randomize `state.index` before rendering. The comment should note this requirement.

<!-- ─── src/js/components/item-picker-modal.cpp ──────────────────────── -->

- [ ] 286. [item-picker-modal.cpp] Missing `is_loading`/`load_error` state and `DBItemList.initialize()` async lazy-load
  - **JS Source**: `src/js/components/item-picker-modal.js` lines 37–38, 107–119
  - **Status**: Pending
  - **Details**: JS tracks `is_loading` and `load_error` flags and calls `await DBItemList.initialize()` when `slot_id` changes and `all_items` is empty. C++ reads `tab_items::getAllItems()` every frame with no lazy load trigger and never renders the "Failed to load items." error branch.

- [ ] 287. [item-picker-modal.cpp] Filter text not trimmed before search — JS trims, C++ does not
  - **JS Source**: `src/js/components/item-picker-modal.js` line 65
  - **Status**: Pending
  - **Details**: JS `filtered_items` trims before lowercasing: `this.filter_text.trim().toLowerCase()`. C++ `rebuild_filtered` lowercases `s_filter_buf` directly. A filter of "  sword  " would not match "sword" in C++ but would in JS.

- [ ] 288. [item-picker-modal.cpp] Item rows display `item->name` instead of `displayName`
  - **JS Source**: `src/js/components/item-picker-modal.js` line 173
  - **Status**: Pending
  - **Details**: JS template renders `{{ item.name }}` which is the display name field. C++ Selectable uses `item->name` (the raw DB name). The filter correctly searches `item->displayName` but displays `item->name`. These fields may differ; display should use `displayName`.

- [ ] 289. [item-picker-modal.cpp] Icon image not rendered per item row in Selectable list
  - **JS Source**: `src/js/components/item-picker-modal.js` lines 171–172
  - **Status**: Pending
  - **Details**: JS template renders `<div :class="['item-icon', 'icon-' + item.icon]">` for each row. C++ preloads icons via `icon_render::loadIcon()` but does not render them (no `ImGui::Image`) alongside the item name. The icon column is missing from the layout.

- [ ] 290. [item-picker-modal.cpp] `open_items_tab` directly calls `modules::set_active` instead of emitting an event
  - **JS Source**: `src/js/components/item-picker-modal.js` lines 143–145
  - **Status**: Pending
  - **Details**: JS emits `'open-items-tab'` and lets the parent handle tab switching. C++ calls `modules::set_active("tab_items")` directly inside the modal, coupling the component to the module system.

- [ ] 291. [item-picker-modal.cpp] Escape key only closes when popup has ImGui focus; JS handler is global
  - **JS Source**: `src/js/components/item-picker-modal.js` lines 138–140, 148–151
  - **Status**: Pending
  - **Details**: JS mounts a `keydown` listener on `document` that fires for Escape at any time. C++ checks `ImGui::IsKeyPressed(ImGuiKey_Escape)` inside the popup window, so Escape only works when that window has ImGui focus.

- [ ] 292. [item-picker-modal.cpp] Overlay backdrop dimming not rendered
  - **JS Source**: `src/js/components/item-picker-modal.js` line 161
  - **Status**: Pending
  - **Details**: JS wraps the modal in `<div class="item-picker-overlay">` which provides a full-screen semi-transparent backdrop. C++ uses `ImGui::BeginPopupModal` which does not draw a dimming overlay. The visual backdrop effect is absent.

<!-- ─── src/js/components/itemlistbox.cpp ───────────────────────────── -->

- [ ] 293. [itemlistbox.cpp] Extra `onOptions` callback and "Options" button not present in JS source
  - **JS Source**: `src/js/components/itemlistbox.js` line 20
  - **Status**: Pending
  - **Details**: JS `emits` is `['update:selection', 'equip']` — no `'options'` event. C++ declares `onOptions` callback and renders an "Options" button. This extra button and callback are not in the JS source.

- [ ] 294. [itemlistbox.cpp] Item slot height hardcoded at 46px; JS `resize()` uses 26px
  - **JS Source**: `src/js/components/itemlistbox.js` line 155
  - **Status**: Pending
  - **Details**: JS `resize()` uses `Math.floor(clientHeight / 26)` (26px slot height, matching base listbox). C++ uses 46px for `itemHeightVal` and the slot count divisor. The actual ImGui rendered row height matches neither, causing incorrect slot count calculations.

- [ ] 295. [itemlistbox.cpp] Font scaling via `ImGui::GetFont()->Scale` mutation is fragile and not thread-safe
  - **JS Source**: `src/js/components/itemlistbox.js` line 335
  - **Status**: Pending
  - **Details**: C++ modifies `ImGui::GetFont()->Scale` directly on the shared font object before calling `ImGui::PushFont(ImGui::GetFont())`. Mutating the shared font affects all subsequent text until restored and is not thread-safe. The correct approach is to push a separately-sized font or accept uniform font size.

- [ ] 296. [itemlistbox.cpp] `includefilecount` prop declared in JS but not implemented in C++
  - **JS Source**: `src/js/components/itemlistbox.js` line 19
  - **Status**: Pending
  - **Details**: JS `props` includes `'includefilecount'` which conditionally shows the file counter alongside `unittype`. C++ `render()` does not include this parameter; `unittype` alone drives the status line, and the `includefilecount` gate is absent.

- [ ] 297. [itemlistbox.cpp] Selectable width subtracts hardcoded 120px magic number for button space
  - **JS Source**: `src/js/components/itemlistbox.js` lines 333–338
  - **Status**: Pending
  - **Details**: C++ gives Selectable `ImVec2(availSize.x - 120.0f, 0.0f)` as a fixed offset for Equip/Options buttons. JS layout is CSS-driven and auto-sizes. The hardcoded value may not match actual button widths.

<!-- ─── src/js/components/listbox-maps.cpp ───────────────────────────── -->

- [ ] 298. [listbox-maps.cpp] `expansionFilter` watch does not call `recalculateBounds()` — `persistscrollkey` not cleared on filter change
  - **JS Source**: `src/js/components/listbox-maps.js` lines 27–30
  - **Status**: Pending
  - **Details**: JS watcher calls `this.recalculateBounds()` after zeroing scroll. C++ only resets `state.base.scroll` and `state.base.scrollRel` without calling `recalculateBounds`. When a `persistscrollkey` is set, the stored scroll position is not cleared in storage when the filter changes.

<!-- ─── src/js/components/listbox-zones.cpp ─────────────────────────── -->

- [ ] 299. [listbox-zones.cpp] `expansionFilter` watch does not call `recalculateBounds()` — `persistscrollkey` not cleared on filter change
  - **JS Source**: `src/js/components/listbox-zones.js` lines 27–30
  - **Status**: Pending
  - **Details**: Same deviation as listbox-maps.cpp entry 298: C++ does not call `recalculateBounds()` after zeroing scroll on filter change, so `persistscrollkey` position is not cleared in storage.

<!-- ─── src/js/components/listbox.cpp ────────────────────────────────── -->

- [ ] 300. [listbox.cpp] `renderStatusBar` uses raw `ImDrawList::AddRectFilled` for background (CLAUDE.md violation)
  - **JS Source**: `src/js/components/listbox.js` lines 510–514
  - **Status**: Pending
  - **Details**: The status bar background is drawn with `AddRectFilled`. Per CLAUDE.md, raw `ImDrawList` calls should not be used for things a native widget can handle. A child window or styled ImGui region should be used instead.

- [ ] 301. [listbox.cpp] Quick filter hover underline not implemented; JS applies CSS `text-decoration: underline` on hover
  - **JS Source**: `src/js/components/listbox.js` lines 512–513
  - **Status**: Pending
  - **Details**: JS renders quick filters as `<a>` elements with CSS hover underline. C++ uses `ImGui::Selectable` styled with `PushStyleColor` to simulate link appearance but does not draw an underline on hover.

- [ ] 302. [listbox.cpp] Status bar bold font not implemented; comment says bold but uses regular font
  - **JS Source**: `src/js/components/listbox.js` line 510
  - **Status**: Pending
  - **Details**: CSS `.list-status` has `font-weight: bold`. C++ `renderStatusBar` comments "Render bold status text" but calls `ImGui::TextUnformatted` without pushing a bold font variant.

<!-- ─── src/js/components/listboxb.cpp ───────────────────────────────── -->

- [ ] 303. [listboxb.cpp] Ctrl+C copies object references in JS (produces `[object Object]`); C++ copies labels — C++ is more correct but deviates
  - **JS Source**: `src/js/components/listboxb.js` lines 179–181
  - **Status**: Pending
  - **Details**: JS `this.selection.join('\n')` on item objects produces `[object Object]` per item unless items have a custom `toString()`. C++ looks up each selected index and copies `items[idx].label`. The C++ behavior is more useful but diverges from the literal JS behavior.

- [ ] 304. [listboxb.cpp] `lastSelectItem` sentinel -1 in C++ vs `null` in JS — JS bug blocks navigation at index 0
  - **JS Source**: `src/js/components/listboxb.js` lines 175–177
  - **Status**: Pending
  - **Details**: JS checks `if (!this.lastSelectItem)` which is also true for index 0 (`!0 === true`), blocking arrow-key navigation when item 0 was last selected. C++ checks `state.lastSelectItem < 0`, which correctly allows navigation when item 0 is selected. C++ behavior is more correct but intentionally deviates from the JS.

- [ ] 305. [listboxb.cpp] Fundamental model difference: C++ stores item indices; JS stores item objects in selection and `lastSelectItem`
  - **JS Source**: `src/js/components/listboxb.js` lines 192–215, 230–270
  - **Status**: Pending
  - **Details**: JS stores actual item objects in `this.selection` and uses `.indexOf(item)` for reference equality. C++ stores integer indices and uses `std::find`. Shift-range selection, `selectItem`, and all callers of the C++ API must use indices rather than item references, breaking the JS component contract.

<!-- ─── src/js/components/map-viewer.cpp ─────────────────────────────── -->

- [ ] 306. [map-viewer.cpp] `checkTileQueue` uses per-frame tile cap (3) instead of JS concurrency limit (up to 4 in-flight)
  - **JS Source**: `src/js/components/map-viewer.js` lines 192–215
  - **Status**: Pending
  - **Details**: JS drains the queue while `activeTileRequests < maxConcurrentTiles` (up to 4 concurrent async requests). C++ uses `MAX_TILES_PER_FRAME=3` per synchronous frame, never gating on in-flight request counts. The semantics of tile loading throughput differ from the JS.

- [ ] 307. [map-viewer.cpp] `tileHasUnexpectedTransparency` samples full tile instead of canvas-clamped sub-region
  - **JS Source**: `src/js/components/map-viewer.js` lines 287–331
  - **Status**: Pending
  - **Details**: JS calls `ctx.getImageData(left, top, width, height)` clamped to the canvas rectangle, checking only the on-screen portion. C++ reads full cached tile pixel data from `tilePixelCache[index]`. A tile partially off-canvas may return incorrect transparency results in C++.

- [ ] 308. [map-viewer.cpp] `setToDefaultPosition` does not internally call `render()` as JS does
  - **JS Source**: `src/js/components/map-viewer.js` lines 421–462
  - **Status**: Pending
  - **Details**: JS `setToDefaultPosition` calls `this.render()` in all code paths including the fallback. C++ never calls `render()` inside `setToDefaultPosition`; callers must invoke `render()` separately. Direct callers that rely on implicit render will not get a render.

- [ ] 309. [map-viewer.cpp] Double-buffer blit-back step not implemented (GL structural difference)
  - **JS Source**: `src/js/components/map-viewer.js` lines 554–627
  - **Status**: Pending
  - **Details**: JS lines 624–626 clear the main canvas and blit the double-buffer onto it. C++ acknowledges this in a comment but does not implement the pixel-level copy; GL textures are repositioned by offset instead. The double-buffer blit behavior is not replicated.

- [ ] 310. [map-viewer.cpp] Map watcher defers `setToDefaultPosition` by one frame (ImGui adaptation)
  - **JS Source**: `src/js/components/map-viewer.js` lines 143–150
  - **Status**: Pending
  - **Details**: JS `map` watcher calls `setToDefaultPosition()` immediately. C++ sets `state.initialized = false` and defers until `viewportWidth > 0.0f && viewportHeight > 0.0f` in the next `renderWidget` call. There is a one-frame delay before the default position is applied.

<!-- ─── src/js/components/menu-button.cpp ───────────────────────────── -->

- [ ] 311. [menu-button.cpp] `openMenu` toggle-disable guard logic missing — clicking again while open does not close
  - **JS Source**: `src/js/components/menu-button.js` lines 37–40
  - **Status**: Pending
  - **Details**: JS `openMenu` does `this.open = !this.open && !this.disabled`, toggling the menu closed if already open. C++ always calls `ImGui::OpenPopup()` on click (a no-op if popup is already open), losing the toggle-close behavior.

- [ ] 312. [menu-button.cpp] `upward` prop not present in JS source — C++ addition with no JS counterpart
  - **JS Source**: `src/js/components/menu-button.js` (entire file)
  - **Status**: Pending
  - **Details**: JS `props` has `['options', 'default', 'disabled', 'dropdown']` — no `upward`. C++ `render()` takes `bool upward` to control popup direction. This is a C++ addition that deviates from the JS component interface.

- [ ] 313. [menu-button.cpp] Arrow triangle rendered with raw `ImDrawList::AddTriangleFilled` (CLAUDE.md violation)
  - **JS Source**: `src/js/components/menu-button.js` lines 75–82
  - **Status**: Pending
  - **Details**: The JS arrow is a CSS-styled `<div class="arrow">`. C++ uses `ImDrawList::AddTriangleFilled` in `drawArrowButton`. Per CLAUDE.md, `ImGui::ArrowButton` or a text character should be used instead of raw ImDrawList for a standard arrow.

- [ ] 314. [menu-button.cpp] Option separator lines use raw `ImDrawList::AddLine` instead of `ImGui::Separator()`
  - **JS Source**: `src/js/components/menu-button.js` lines 78–80
  - **Status**: Pending
  - **Details**: C++ draws separator lines between popup options via `drawList->AddLine(...)`. Per CLAUDE.md, `ImGui::Separator()` should be preferred over raw ImDrawList calls for separator rendering.

<!-- ─── src/js/components/model-viewer-gl.cpp ──────────────────────── -->

- [ ] 315. [model-viewer-gl.cpp] Extra auto-rotation branch via `context.setActiveModelTransform` has no JS counterpart
  - **JS Source**: `src/js/components/model-viewer-gl.js` lines 239–247
  - **Status**: Pending
  - **Details**: JS auto-rotation only goes through `activeRenderer.setTransform`. C++ has an additional `else if` branch that uses `context.setActiveModelTransform` when `activeRenderer` is null. This extension is not present in the JS source.

- [ ] 316. [model-viewer-gl.cpp] Vue reactive watchers replaced by frame-polling — one-frame latency on config changes
  - **JS Source**: `src/js/components/model-viewer-gl.js` lines 468–474
  - **Status**: Pending
  - **Details**: JS uses `core.view.$watch('config.chrUse3DCamera', ...)` for immediate reactive callbacks. C++ polls each bool each frame via `poll_watchers`, comparing previous vs current value, introducing one-frame latency.

- [ ] 317. [model-viewer-gl.cpp] `context.controls` split into two typed C++ pointers breaks JS API contract for external callers
  - **JS Source**: `src/js/components/model-viewer-gl.js` line 395
  - **Status**: Pending
  - **Details**: JS sets `this.context.controls = this.controls` after `recreate_controls`. External JS code calls `this.context.controls.someMethod()`. C++ callers must know to use `context.controls_orbit` vs `context.controls_character`. This is a breaking interface change relative to JS, documented in the header but not in code comments.

<!-- ─── src/js/components/resize-layer.cpp ──────────────────────────── -->

- [ ] 318. [resize-layer.cpp] Resize callback receives float width from ImGui instead of integer `clientWidth` like JS
  - **JS Source**: `src/js/components/resize-layer.js` lines 13–15
  - **Status**: Pending
  - **Details**: JS `clientWidth` is always an integer (pixels, rounded down). C++ passes `currentWidth` (float from `GetContentRegionAvail().x`) to the callback. Callers expecting an integer may round differently.

<!-- ─── src/js/components/slider.cpp ─────────────────────────────────── -->

- [ ] 319. [slider.cpp] Entire custom drag/click slider replaced by `ImGui::SliderFloat` — structural deviation undocumented in source
  - **JS Source**: `src/js/components/slider.js` lines 41–99
  - **Status**: Pending
  - **Details**: JS implements a fully custom slider with mousedown/mousemove/mouseup document listeners, a fill bar, and a handle element. C++ replaces all of this with `ImGui::SliderFloat`. Per CLAUDE.md Visual Fidelity rules the appearance need not match exactly, but the structural deviation should be documented with a comment in the source file.

- [ ] 320. [slider.cpp] `SliderState` parameter in `render()` is unused/vestigial
  - **JS Source**: `src/js/components/slider.js` (entire file)
  - **Status**: Pending
  - **Details**: C++ `render()` takes `SliderState& /*state*/` but never uses it. The JS `isScrolling` reactive state is managed internally by `ImGui::SliderFloat`. The `SliderState` struct and parameter are vestigial dead code.

<!-- ─── src/js/db/DBCReader.cpp ───────────────────────────────────────── -->

- [ ] 321. [DBCReader.cpp] `loadSchema()` bypasses CASC BuildCache; should use `getActiveCascCache()` like `WDCReader.cpp`
  - **JS Source**: `src/js/db/DBCReader.js` lines 177–194
  - **Status**: Pending
  - **Details**: JS `loadSchema()` uses `core.view.casc?.cache` (a `BuildCache` object) with `cache.getFile()` / `cache.storeFile()` for DBD caching with null-safe fallback. C++ always uses `std::filesystem` directly. `WDCReader.cpp` correctly implements `getActiveCascCache()` with proper fallback — `DBCReader.cpp` should use the same pattern.

- [ ] 322. [DBCReader.cpp] `_read_field()` missing `FieldType::Int64` and `FieldType::UInt64` switch cases
  - **JS Source**: `src/js/db/DBCReader.js` lines 390–408
  - **Status**: Pending
  - **Details**: `convert_dbd_to_schema_type` can return `FieldType::Int64` and `FieldType::UInt64` for 64-bit fields. Neither JS nor C++ handles these in `_read_field` — both fall through to `readUInt32LE()` default, silently truncating 64-bit values. The cases should be handled or explicitly documented.

- [ ] 323. [DBCReader.cpp] `_read_field_array()` silently drops fields of unexpected variant type
  - **JS Source**: `src/js/db/DBCReader.js` lines 417–423
  - **Status**: Pending
  - **Details**: In `_read_record`, when the post-processing loop encounters a `vector<FieldValue>` whose first element holds none of the four checked types, the field is silently omitted from `out`. JS always stores the array regardless. An `else` fallback should store an empty or raw vector to match JS behavior.

<!-- ─── src/js/db/DBDParser.cpp ─────────────────────────────── -->

- [ ] 324. [DBDParser.cpp] Column name `?` replacement removes only the last `?` instead of all `?` characters
  - **JS Source**: `src/js/db/DBDParser.js` line 339
  - **Status**: Pending
  - **Details**: JS uses `match[3].replace('?', '')` which replaces **all** occurrences of `?` in the column name string. The C++ only removes the trailing `?` with `if (!columnName.empty() && columnName.back() == '?') columnName.pop_back();`. If a column name contains a `?` anywhere other than the end (e.g. `"Field?Test?"`), the JS produces `"FieldTest"` while C++ produces `"Field?Test"`. In practice DBD column names only have a trailing `?`, but the logic is not a faithful port.

- [ ] 325. [DBDParser.cpp] Annotation parsing uses substring search instead of exact token match
  - **JS Source**: `src/js/db/DBDParser.js` lines 286–294
  - **Status**: Pending
  - **Details**: JS splits annotations string by comma and uses `Array.includes()` for exact token matching: `fieldMatch[2].split(',').includes('id')`. The C++ uses `annotations.find("id") != std::string::npos`, which is a substring search. If a future annotation contained `"id"` as a substring (e.g. `"validid"`), the C++ would incorrectly set `field.isID = true`. The same applies to `"noninline"` and `"relation"`. The current known annotation values (`id`, `noninline`, `relation`) do not overlap as substrings, so there is no current breakage, but this is a deviation from the JS semantics.

- [ ] 326. [DBDParser.cpp] Empty chunks passed to `parseChunk()` are silently skipped in C++ but generate empty `DBDEntry` objects in JS
  - **JS Source**: `src/js/db/DBDParser.js` lines 216–221, 238–242
  - **Status**: Pending
  - **Details**: In the JS `parse()` loop, `parseChunk(chunk)` is called every time an empty line is encountered, even when `chunk` is still an empty array (e.g. consecutive blank lines at the start of the file). When `chunk` is `[]`, `chunk[0]` is `undefined`, so `parseChunk` falls into the `else` branch and pushes an empty `DBDEntry` with no fields into `this.entries`. The C++ guards with `if (!chunk.empty() && chunk[0] == "COLUMNS")` and only processes non-empty chunks. In the else branch, `entries.push_back(std::move(entry))` is only reached when `chunk` is non-empty. The empty `DBDEntry` objects in JS are harmless (they never match anything in `isValidFor()`), so there is no functional impact, but the structural behavior differs.

<!-- ─── src/js/db/FieldType.cpp ─────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/db/WDCReader.cpp ─────────────────────────────── -->

- [ ] 327. [WDCReader.cpp] `getRelationRows()` incorrectly requires preload; JS does not
  - **JS Source**: `src/js/db/WDCReader.js` lines 216–234
  - **Status**: Pending
  - **Details**: The JS `getRelationRows()` only requires the table to be loaded (`isLoaded`). It reads each record directly via `_readRecord()` without requiring `preload()`. The C++ version at lines 305–309 throws a `std::runtime_error` if `rows` is not preloaded: `"Table must be preloaded before calling getRelationRows. Use db2.preload.<TableName>() first."` This is a behavioral deviation — callers that call `getRelationRows()` without first calling `preload()` will work in JS but throw in C++. The JS comment `Required for getRelationRows() to work properly` appears to be advisory; it does not enforce it. The C++ should remove the preload requirement and instead call `_readRecord()` directly for each record ID in the lookup, matching JS behavior.

- [ ] 328. [WDCReader.cpp] `getAllRowsAsync()` returns `std::map` by value instead of by reference, inconsistent with `getAllRows()` which returns a const reference
  - **JS Source**: `src/js/db/WDCReader.js` lines 141–193
  - **Status**: Pending
  - **Details**: The synchronous `getAllRows()` returns `const std::map<uint32_t, DataRecord>&` — a reference to either `rows` or `transientRows`. The async version `getAllRowsAsync()` at line 1297–1299 returns `std::future<std::map<uint32_t, DataRecord>>` (by value, making a copy). While making a copy is safe, there is a subtle threading issue: if `preload()` has not been called, `getAllRows()` fills `transientRows` (a member variable) and returns a reference to it. A concurrent call to `getAllRowsAsync()` could race with `getAllRows()` filling `transientRows` from another thread. The JS original is single-threaded so this risk does not exist in JS. The async methods are C++-only additions, but the `transientRows` approach is not thread-safe.

- [ ] 329. [WDCReader.cpp] `idFieldIndex` is initialized to `0` instead of `null`; `idField` is `optional<string>` instead of `null` — minor behavioral difference in unloaded state
  - **JS Source**: `src/js/db/WDCReader.js` lines 79–80
  - **Status**: Pending
  - **Details**: The JS constructor sets `this.idField = null` and `this.idFieldIndex = null`. The C++ has `idFieldIndex = 0` (uint16_t) and `idField` as `std::optional<std::string>` (empty). The value `0` for `idFieldIndex` before loading is indistinguishable from a valid index of `0`, whereas JS `null` is clearly uninitialized. The `getIDIndex()` method in both JS and C++ guards with `isLoaded`, so callers should not access these before loading. However, the C++ `_readRecordFromSection` references `idFieldIndex` at line 1269 without checking `isLoaded` (it is called internally after loading starts), so the uninitialized `0` is never used incorrectly in practice. This is a low-severity deviation.

- [ ] 330. [WDCReader.cpp] BitpackedIndexedArray index arithmetic uses integer multiplication instead of BigInt, potential overflow for large `bitpackedValue`
  - **JS Source**: `src/js/db/WDCReader.js` lines 820–822
  - **Status**: Pending
  - **Details**: The JS computes the pallet data index as `bitpackedValue * BigInt(recordFieldInfo.fieldCompressionPacking[2]) + BigInt(i)` using BigInt arithmetic which is arbitrary-precision and cannot overflow. The C++ computes `static_cast<size_t>(bitpackedValue * arrSize + i)` at line 1149 where `bitpackedValue` is `uint64_t` and `arrSize` is `uint32_t`. The multiplication `bitpackedValue * arrSize` is done in uint64_t, which could theoretically overflow for pathologically large values, though in practice DB2 pallet data indices are small. This is a theoretical deviation.

<!-- ─── src/js/db/caches/DBCharacterCustomization.cpp ─────────────────────────────── -->

- [ ] 331. [DBCharacterCustomization.cpp] `chr_cust_mat_map` is keyed by `materialID` (the lookup key) instead of `mat_row.ID` (the row's own ID field)
  - **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` line 102
  - **Status**: Pending
  - **Details**: JS keys `chr_cust_mat_map` by `mat_row.ID`, the row's own `ID` field from the `ChrCustomizationMaterial` DB2 table: `chr_cust_mat_map.set(mat_row.ID, {...})`. The C++ uses `materialID` (the value from `ChrCustomizationMaterialID` in the element row): `chr_cust_mat_map[materialID] = mat_info`. Since the row is retrieved via `getRow(materialID)`, the row's own `ID` should equal `materialID` for direct (non-copy-table) rows. However, if the row was accessed via the copy table (JS `getRow` sets `tempCopy.ID = recordID` which would be the destination copy ID), the row's `ID` field would differ from the original source `materialID`. This is an edge case but represents a semantic deviation from the JS source.

- [ ] 332. [DBCharacterCustomization.cpp] `ensureInitialized()` uses a synchronous blocking pattern with mutex instead of the JS async promise-caching pattern
  - **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 39–48
  - **Status**: Pending
  - **Details**: The JS uses a promise-caching pattern: if `init_promise` is set, it `await`s the same promise, ensuring all concurrent async callers share a single initialization pass. The C++ uses `is_initializing` with a `std::condition_variable` wait. Both achieve the same goal of single-initialization with concurrent waiter support. The C++ also runs `_initialize()` synchronously on the calling thread (blocking), whereas JS runs it asynchronously. This is an intentional C++ adaptation since `std::async`/`std::future` equivalents are not used here. The functional behavior is preserved. No fix required, but worth noting as a structural deviation.

<!-- ─── src/js/db/caches/DBComponentModelFileData.cpp ─────────────────────────────── -->

- [ ] 333. [DBComponentModelFileData.cpp] `initialize()` does not implement the promise-deduplication pattern from JS — async concurrent callers would each block instead of sharing one init
  - **JS Source**: `src/js/db/caches/DBComponentModelFileData.js` lines 18–43
  - **Status**: Pending
  - **Details**: The JS `initialize()` caches an `init_promise` and returns it to all concurrent callers, ensuring the initialization IIFE runs only once even under concurrent async calls. The C++ `initialize()` uses a mutex and `is_initializing` flag with a `std::condition_variable` to prevent double-initialization and wait for completion. This is functionally equivalent for the concurrency case (second caller blocks until initialization completes). The structure is different but the observable behavior is preserved. Low-severity deviation.

- [ ] 334. [DBComponentModelFileData.cpp] `initializeAsync()` is a C++-only addition with no JS counterpart
  - **JS Source**: `src/js/db/caches/DBComponentModelFileData.js` (no equivalent)
  - **Status**: Pending
  - **Details**: The C++ exposes `initializeAsync()` which wraps `initialize()` in `std::async`. This has no equivalent in the JS source. The JS `initialize()` itself is already async (returns a Promise). This is an intentional addition for the C++ async model and is not a bug, but it is an addition beyond the JS API surface.
<!-- ─── src/js/db/caches/DBComponentTextureFileData.cpp ─────────────────────────────── -->

- [ ] 335. [DBComponentTextureFileData.cpp] `getTextureForRaceGender` wraps all race/gender-specific loops inside `if (race_id.has_value() && gender_index.has_value())`, diverging from JS logic
  - **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js` lines 59–87
  - **Status**: Pending
  - **Details**: In JS, `getTextureForRaceGender` always executes the exact-match loop, the race+any-gender loop, the fallback-race loop, and the race=0+specific-gender loop regardless of whether `race_id`/`gender_index` are null. A null comparison like `info.raceID === null` simply never matches, so those loops silently produce no result. In C++, when either `race_id` or `gender_index` is `std::nullopt`, all four loops are skipped entirely and execution jumps directly to the race=0 (any race) loop. The functional outcome is the same (null never matches), but the structure deviates from the original JS — a future change to any of those inner loops could inadvertently rely on the JS always-execute behaviour. The C++ guard should either be removed (relying on the nullopt comparison never matching) or the code should be clearly documented as a deliberate structural deviation.

- [ ] 336. [DBComponentTextureFileData.cpp] `initialize` uses mutex/condvar concurrency guard, but JS uses a single `init_promise` singleton guard
  - **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js` lines 17–41
  - **Status**: Pending
  - **Details**: The JS `initialize` uses `init_promise` to coalesce concurrent callers onto a single in-flight Promise. The C++ implementation uses `is_initializing` + `std::condition_variable` to serialize concurrent calls: the first caller runs initialization while subsequent callers block waiting on the condvar. This is a structural deviation, but the behaviour is functionally equivalent — only one initialization runs and all concurrent callers receive the result. Documented here for completeness; it is an acceptable C++ adaptation.

- [ ] 337. [DBComponentTextureFileData.cpp] `db2.ComponentTextureFileData.getAllRows()` replaced by `casc::db2::preloadTable("ComponentTextureFileData").getAllRows()` — table name must match exactly
  - **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js` line 27
  - **Status**: Pending
  - **Details**: Minor: the JS accesses `db2.ComponentTextureFileData` (a named property). The C++ calls `casc::db2::preloadTable("ComponentTextureFileData")`. The string key must match the actual DB2 table name. If `preloadTable` is case-sensitive this is fine; just flagging for completeness.

<!-- ─── src/js/db/caches/DBCreatureDisplayExtra.cpp ─────────────────────────────── -->

- [ ] 338. [DBCreatureDisplayExtra.cpp] JS `_initialize` iterates `CreatureDisplayInfoOption` rows via `.values()` (ignoring the row ID key), but C++ iterates via `[_optId, row]` key-value pairs and discards the key with `(void)_optId`
  - **JS Source**: `src/js/db/caches/DBCreatureDisplayExtra.js` lines 39–48
  - **Status**: Pending
  - **Details**: The JS iterates `(await db2.CreatureDisplayInfoOption.getAllRows()).values()` — it only uses the row value, not the key. The C++ iterates `const auto& [_optId, row]` and silences the unused key with `(void)_optId`. The result is functionally identical, but it is worth noting since `_optId` is the row's own DB ID; the JS code explicitly discards it. No functional bug here.

- [ ] 339. [DBCreatureDisplayExtra.cpp] JS `get_extra` returns `undefined` when not found; C++ returns `nullptr`
  - **JS Source**: `src/js/db/caches/DBCreatureDisplayExtra.js` line 55
  - **Status**: Pending
  - **Details**: JS: `const get_extra = (id) => extra_map.get(id)` — returns `undefined` when the key is absent. C++ returns `nullptr` (a pointer). All call sites must check for `nullptr` rather than truthiness. This is a necessary C++ adaptation; callers must be verified to handle `nullptr` correctly.

- [ ] 340. [DBCreatureDisplayExtra.cpp] JS `get_customization_choices` returns a new empty array `[]` (via nullish coalescing `?? []`) when not found; C++ returns a `const&` to a static empty vector
  - **JS Source**: `src/js/db/caches/DBCreatureDisplayExtra.js` line 57
  - **Status**: Pending
  - **Details**: JS: `option_map.get(extra_id) ?? []` — each call for a missing key yields a fresh empty array. C++ returns a const reference to a static `empty_options` vector. Callers that hold onto the returned reference after a `reset()` or re-initialization would observe a stale reference; however, since there is no `reset()` function in this module, this is practically safe. The interface diverges (reference vs. value) and callers must not store the returned reference.

<!-- ─── src/js/db/caches/DBCreatureList.cpp ─────────────────────────────── -->

- [ ] 341. [DBCreatureList.cpp] JS `get_all_creatures` returns a `Map` (keyed by creature ID); C++ returns a `const std::vector<CreatureEntry>&`
  - **JS Source**: `src/js/db/caches/DBCreatureList.js` lines 45–47
  - **Status**: Pending
  - **Details**: In JS, `creatures` is a `Map` and `get_all_creatures()` returns that Map directly. Callers iterate it with `for (const [id, entry] of creatures)`. In C++, `creatures` is a `std::vector<CreatureEntry>` and `get_all_creatures()` returns a const reference to it. Callers must iterate with range-for over the vector. This is a structural change that all call sites must account for — any caller that uses Map-style iteration (by key) will break if not updated accordingly.

- [ ] 342. [DBCreatureList.cpp] JS `get_creature_by_id` returns `undefined` when not found; C++ returns `nullptr`
  - **JS Source**: `src/js/db/caches/DBCreatureList.js` lines 49–51
  - **Status**: Pending
  - **Details**: JS: `creatures.get(id)` — returns `undefined` when absent. C++ returns `nullptr`. All callers must check for `nullptr`. Acceptable C++ adaptation, but callers must be verified.

- [ ] 343. [DBCreatureList.h] Header documents `get_all_creatures` as returning "reference to the creature map" but it actually returns a vector
  - **JS Source**: `src/js/db/caches/DBCreatureList.js` lines 45–47
  - **Status**: Pending
  - **Details**: The comment in `DBCreatureList.h` line 32 says "Reference to the creature map." but the function returns `const std::vector<CreatureEntry>&`. The comment is stale and misleading, though not a functional defect.

<!-- ─── src/js/db/caches/DBCreatures.cpp ─────────────────────────────── -->

- [ ] 344. [DBCreatures.cpp] Indentation inconsistency inside `initializeCreatureData`: the outer `try` block and the `creatureGeosetMap`/`modelIDToDisplayInfoMap` section use different indent levels
  - **JS Source**: `src/js/db/caches/DBCreatures.js` lines 18–76
  - **Status**: Pending
  - **Details**: Lines 71–82 inside `initializeCreatureData` are indented with one less tab level than the surrounding `try` block (lines 70–135). This is a cosmetic/style issue but it could mask structural logic errors when reviewing the code. Not a functional bug.

- [ ] 345. [DBCreatures.cpp] JS `initializeCreatureData` checks `isInitialized` at the start and returns early; C++ uses a mutex guard instead
  - **JS Source**: `src/js/db/caches/DBCreatures.js` lines 18–20
  - **Status**: Pending
  - **Details**: The JS function begins with `if (isInitialized) return;`. The C++ version uses a mutex+condvar pattern (shared with other DB cache files) that handles concurrent callers. This is a structural difference but functionally equivalent and is an acceptable C++ adaptation.

- [ ] 346. [DBCreatures.cpp] JS `extraGeosets` is only present on `display` objects that need it (property does not exist otherwise); C++ uses `std::optional<std::vector<uint32_t>>` which defaults to `std::nullopt` — minor structural deviation
  - **JS Source**: `src/js/db/caches/DBCreatures.js` lines 60–63
  - **Status**: Pending
  - **Details**: In JS, `display.extraGeosets` is only assigned if `modelIDHasExtraGeosets` is true — otherwise the property does not exist on the object at all. In C++, `extraGeosets` is a member of `CreatureDisplayInfo` and defaults to `std::nullopt`. The check `display.extraGeosets.has_value()` (or checking for `std::nullopt`) is therefore the correct C++ equivalent of `display.extraGeosets !== undefined`. All callers must use `.has_value()` rather than a truthiness check. This is an acceptable C++ adaptation.

- [ ] 347. [DBCreatures.cpp] JS stores all `creatureDisplays`, `creatureDisplayInfoMap`, `displayIDToFileDataID`, `modelDataIDToFileDataID` as `Map`; C++ uses `std::unordered_map` with `std::reference_wrapper` for `creatureDisplays`
  - **JS Source**: `src/js/db/caches/DBCreatures.js` lines 9–12
  - **Status**: Pending
  - **Details**: The C++ `creatureDisplays` map stores `std::vector<std::reference_wrapper<const CreatureDisplayInfo>>` referencing entries in `creatureDisplayInfoMap`. This means `creatureDisplayInfoMap` must not be rehashed or cleared after population, otherwise the references become dangling. Since `initializeCreatureData` fills both maps in one pass and they are static globals, this is safe at runtime — but it is a fragile design that does not exist in the JS version (which stores direct object references). Documented for future maintainability.

<!-- ─── src/js/db/caches/DBCreaturesLegacy.cpp ─────────────────────────────── -->

- [ ] 348. [DBCreaturesLegacy.cpp] JS `initializeCreatureData` takes an `mpq` object and calls `mpq.getFile(path)`; C++ takes a `std::function<std::vector<uint8_t>(const std::string&)>` callback
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 19–112
  - **Status**: Pending
  - **Details**: The JS interface is `initializeCreatureData(mpq, build_id)` where `mpq` is an MPQInstall object with a `getFile` method. The C++ interface is `initializeCreatureData(std::function<...> getFile, build_id)` — the MPQ object is abstracted away as a callback. This is a valid C++ adaptation (the MPQ type doesn't exist directly in C++), but all call sites must pass a lambda wrapping the MPQ getFile call.

- [ ] 349. [DBCreaturesLegacy.cpp] JS `getCreatureDisplaysByPath` only converts `.mdx` to `.m2` (not `.mdl`); C++ `getCreatureDisplaysByPath` converts both `.mdl` and `.mdx` to `.m2`
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 119–132
  - **Status**: Pending
  - **Details**: In JS, `getCreatureDisplaysByPath` normalizes the incoming path with `normalized.replace(/\.mdx$/i, '.m2')` — only `.mdx`. The JS `initializeCreatureData` function also normalizes to `.m2` for both `.mdl` and `.mdx` when building the `model_id_to_path` map. However at lookup time, if a caller passes a `.mdl` path, JS would NOT convert it to `.m2` and would find no match. In C++, both `normalizePath` (used in both init and lookup) convert `.mdl` and `.mdx`, so a `.mdl` path at lookup time would match. This is a behavioural divergence from the JS — the C++ is more permissive in `getCreatureDisplaysByPath`. Whether this is intentional or a bug needs review.

- [ ] 350. [DBCreaturesLegacy.cpp] JS `getCreatureDisplaysByPath` returns `undefined` when not found; C++ returns `nullptr`
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 119–132
  - **Status**: Pending
  - **Details**: JS: `creatureDisplays.get(normalized)` — returns `undefined` when absent. C++ returns `nullptr`. All callers must check for `nullptr`. Acceptable C++ adaptation.

- [ ] 351. [DBCreaturesLegacy.cpp] JS error handler logs `e.stack` in addition to `e.message`; C++ only logs `e.what()` and has a comment noting the omission
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 108–111
  - **Status**: Pending
  - **Details**: JS logs both `e.message` and `e.stack` on error. C++ only logs `e.what()` and acknowledges the omission with a comment: "Note: JS also logs e.stack here, but C++ exceptions do not carry stack traces by default". This is an acceptable limitation (C++ has no standard stack-trace mechanism), but it means less diagnostic information on error. The comment is present so this is documented.

- [ ] 352. [DBCreaturesLegacy.cpp] JS `model_path` field lookup is `row.ModelName || row.ModelPath || row.field_2` (short-circuit OR); C++ uses sequential `if`-chain with separate `empty()` checks
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 42–48
  - **Status**: Pending
  - **Details**: In JS, `row.ModelName || row.ModelPath || row.field_2` returns the first truthy value, falling through only on empty string, `0`, `null`, or `undefined`. The C++ code checks `model_path.empty()` after each field read, which only falls through on empty string. If a field exists and has a non-string value (e.g. a number 0 returned as a string "0"), JS would skip it while C++ would not (since "0" is non-empty). This is a minor edge-case discrepancy for unusual DBC schemas, but is practically safe since these fields are always strings.

- [ ] 353. [DBCreaturesLegacy.cpp] JS `model_id` lookup falls back to `row.field_1` via nullish coalescing `?? row.field_1`; C++ uses sequential `model_id_found` flag with `row.find("field_1")` fallback
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` line 69
  - **Status**: Pending
  - **Details**: In JS: `const model_id = row.ModelID ?? row.field_1` — this falls back to `field_1` only when `ModelID` is `null` or `undefined`. The C++ uses a `model_id_found` flag and falls back to `"field_1"` only when `"ModelID"` is not found in the row map. The semantic is slightly different: in JS, a `ModelID` that exists but equals `0` would NOT fall back to `field_1` (since `0 ?? x` = `0`). In C++, if `"ModelID"` is present in the row and evaluates to `0`, `model_id` becomes 0 (correct). Both paths then proceed to look up `model_id_to_path.get(model_id)` / `model_id_to_path.find(model_id)`. Functionally equivalent in the expected data range.
<!-- ─── src/js/db/caches/DBDecor.cpp ─────────────────────────────── -->

- [ ] 354. [DBDecor.cpp] `initializeDecorData` is synchronous; JS original is async with `await`
  - **JS Source**: `src/js/db/caches/DBDecor.js` lines 15–40
  - **Status**: Pending
  - **Details**: The JS `initializeDecorData` is declared `async` and uses `await db2.HouseDecor.getAllRows()`. The C++ version is a plain synchronous function calling `casc::db2::preloadTable("HouseDecor").getAllRows()`. This is a deliberate architectural change (all DB2 loading is synchronous in C++), but it should be verified that callers have been updated to not `await` the result.

- [ ] 355. [DBDecor.cpp] Log format uses `std::format` `{}` instead of JS `%d` printf-style
  - **JS Source**: `src/js/db/caches/DBDecor.js` line 38
  - **Status**: Pending
  - **Details**: JS logs `'Loaded %d house decor items', decorItems.size` using a variadic log signature. C++ logs `std::format("Loaded {} house decor items", decorItems.size())`. While the output text is equivalent, the calling convention differs — `logging::write` receives a pre-formatted string in C++ rather than a format+args pair. This is consistent with the rest of the C++ codebase but is a deviation from the JS API.

<!-- ─── src/js/db/caches/DBDecorCategories.cpp ───────────────────── -->

- [ ] 356. [DBDecorCategories.cpp] `get_subcategories_for_decor` returns `nullptr` instead of JS `null`; return type should convey "null or set"
  - **JS Source**: `src/js/db/caches/DBDecorCategories.js` line 55
  - **Status**: Pending
  - **Details**: JS returns `decor_subcategory_map.get(decor_id) ?? null`. In C++ the function returns `const std::unordered_set<uint32_t>*` and returns `nullptr` when not found. This is an acceptable C++ idiom, but callers must be checked to handle `nullptr` correctly since JS callers would receive `null` (not a JS Set), which they test with `if (!result)`.

- [ ] 357. [DBDecorCategories.cpp] `initialize_categories` is synchronous; JS original is async
  - **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 10–51
  - **Status**: Pending
  - **Details**: The JS `initialize_categories` is `async` and uses `await` on all three `getAllRows()` calls. The C++ version is synchronous. Same architectural note as finding 1 — callers must be confirmed to not `await`.

- [ ] 358. [DBDecorCategories.cpp] `Name_lang` field accessed via `row.at()` without guard, may throw if field is absent
  - **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 17–19, 25–29
  - **Status**: Pending
  - **Details**: In `initialize_categories`, both the categories and subcategories loops call `fieldToString(row.at("Name_lang"))` using `at()` which throws `std::out_of_range` if the key is missing. The JS code accesses `row.Name_lang` with optional chaining fallback (`row.Name_lang || ...`), meaning a missing field is handled gracefully. The C++ should use `row.find("Name_lang")` with a fallback (as `DBDecor.cpp` does for the same field) to avoid potential crashes.

- [ ] 359. [DBDecorCategories.cpp] `DecorXDecorSubcategory` loop: `decor_id_found` check does not replicate JS `undefined` check correctly when `HouseDecorID` is present but zero
  - **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 33–47
  - **Status**: Pending
  - **Details**: JS checks `if (decor_id === undefined || sub_id === undefined) continue;`. A value of `0` would NOT skip the record. In C++, the code tracks whether the field was found at all (`decor_id_found`), meaning `decor_id = 0` from a present field is kept — this matches JS. However, there is no equivalent guard for `sub_id == 0`: JS skips only if `sub_id === undefined`, not if it is 0. The C++ code also does not skip when `sub_id == 0`, so for the sub_id side this is correct. The logic is functionally equivalent, but the comment on line 98–99 is slightly misleading.

<!-- ─── src/js/db/caches/DBGuildTabard.cpp ────────────────────────── -->

- [ ] 360. [DBGuildTabard.cpp] Missing `init_promise` deduplication: concurrent calls to `initialize()` may run the body multiple times
  - **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 36–89
  - **Status**: Pending
  - **Details**: The JS `initialize` uses `init_promise` to ensure that if the async function is called concurrently before `is_initialized` is set, all callers await the same in-flight promise rather than starting duplicate work. The C++ `initialize()` and `ensureInitialized()` are synchronous and single-threaded in this context, but if they are ever called from multiple threads the `is_initialized` check is not protected by a mutex. This is a potential thread-safety gap. The JS deduplication pattern should either be replicated or documented as not needed.

- [ ] 361. [DBGuildTabard.cpp] `GuildColorBackground/Border/Emblem` getAllRows use `.entries()` in JS vs structured binding in C++
  - **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 75–82
  - **Status**: Pending
  - **Details**: The JS loads background/border/emblem colors using `(await db2.GuildColorBackground.getAllRows()).entries()` which yields `[id, row]` pairs using the Map's id as key. The C++ uses structured bindings `for (const auto& [id, row] : ...)` which should also yield the table's row ID as `id`. This is functionally equivalent, but should be confirmed that `casc::db2::preloadTable(...).getAllRows()` returns pairs with the DB2 record ID as the first element (matching the JS Map key from `getAllRows()`).

- [ ] 362. [DBGuildTabard.cpp] `ColorRGB` struct stores `uint32_t` for r/g/b but JS stores plain numbers (likely 0–255 byte values)
  - **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 75–82
  - **Status**: Pending
  - **Details**: The JS stores `{ r: row.Red, g: row.Green, b: row.Blue }` where these are raw DB2 field values. C++ uses `uint32_t` for each channel. The DB2 color fields are typically 8-bit byte values (0–255). Using `uint32_t` is wider than necessary but does not cause data loss for values in 0–255 range. However, callers that expect byte-range RGB should be aware. This is a minor type mismatch.

- [ ] 363. [DBGuildTabard.cpp] `initialize` is synchronous; JS original is async with init_promise deduplication
  - **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 36–90
  - **Status**: Pending
  - **Details**: The JS `initialize` is `async` and wraps its body in an inner IIFE assigned to `init_promise`, then returns `init_promise`. The C++ version is a plain synchronous function. Callers must be verified to not `await` the result.

<!-- ─── src/js/db/caches/DBItemCharTextures.cpp ──────────────────── -->

- [ ] 364. [DBItemCharTextures.cpp] Missing `init_promise` deduplication for concurrent initialization calls
  - **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 34–93
  - **Status**: Pending
  - **Details**: The JS `initialize` uses an `init_promise` variable so that concurrent async callers await the same in-flight initialization. The C++ `initialize()` is synchronous and has no equivalent deduplication or mutex guard. Same concern as finding 7.

- [ ] 365. [DBItemCharTextures.cpp] `resolve_display_id` uses `std::map<uint32_t, uint32_t>` sorted iteration but JS sorts an array of keys; behavior is equivalent only if key type and sort order match
  - **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 106–119
  - **Status**: Pending
  - **Details**: JS does `[...modifiers.keys()].sort((a, b) => a - b)` to get the lowest modifier key. C++ stores modifiers in `std::map<uint32_t, uint32_t>` which is sorted in ascending order, so `modifiers.begin()` yields the lowest key. Since `uint32_t` sorts the same as numeric ascending this is functionally equivalent. However the C++ uses `uint32_t` keys while JS Map keys are numbers (signed). If any modifier IDs are negative in the DB2 data, `uint32_t` would wrap them, changing sort order. Recommend verifying modifier IDs are always non-negative.

- [ ] 366. [DBItemCharTextures.cpp] `DBComponentTextureFileData::initialize()` called synchronously; JS awaits it
  - **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 44–45
  - **Status**: Pending
  - **Details**: JS calls `await DBTextureFileData.ensureInitialized()` and `await DBComponentTextureFileData.initialize()`. The C++ calls them synchronously. This is consistent with the general architectural shift but should be verified that these dependencies are always initialized before `DBItemCharTextures::initialize()` is called.

- [ ] 367. [DBItemCharTextures.cpp] `getTexturesByDisplayId` default parameter for `race_id` and `gender_index` is `null` in JS but `-1` in C++
  - **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 155, 130
  - **Status**: Pending
  - **Details**: JS declares `get_item_textures = (item_id, race_id = null, gender_index = null, modifier_id)` and `get_textures_by_display_id = (display_id, race_id = null, gender_index = null)`. C++ uses `-1` as the sentinel for "no preference". The C++ code then maps `race_id >= 0` to `std::optional<uint32_t>` and otherwise passes `std::nullopt`. This correctly maps `-1` (C++ "null") to `null` (JS). Functionally equivalent provided callers always pass `-1` or a valid non-negative race/gender ID, never `0` intending "no preference" (since `0` would be passed as a real value).

<!-- ─── src/js/db/caches/DBItemDisplayInfoModelMatRes.cpp ──────────── -->

- [ ] 368. [DBItemDisplayInfoModelMatRes.cpp] `fieldToUint32` does not handle `float` variant unlike other files in this directory
  - **JS Source**: `src/js/db/caches/DBItemDisplayInfoModelMatRes.js` lines 26–28
  - **Status**: Pending
  - **Details**: The `fieldToUint32` helper in `DBItemDisplayInfoModelMatRes.cpp` only handles `int64_t` and `uint64_t` variants of `db::FieldValue`, returning 0 for all others including `float`. Other files in the same directory (e.g. `DBDecor.cpp`, `DBGuildTabard.cpp`, `DBDecorCategories.cpp`) include a `float` branch: `if (auto* p = std::get_if<float>(&val)) return static_cast<uint32_t>(*p);`. If `ItemDisplayInfoID` or `MaterialResourcesID` are stored as floats in the DB2 variant, this helper would silently return 0, causing data to be dropped without any diagnostic.

- [ ] 369. [DBItemDisplayInfoModelMatRes.cpp] `initialize` is synchronous; JS original (`initializeIDIMMR`) is async
  - **JS Source**: `src/js/db/caches/DBItemDisplayInfoModelMatRes.js` lines 15–40
  - **Status**: Pending
  - **Details**: The JS `initializeIDIMMR` is `async` and uses `await DBTextureFileData.ensureInitialized()` and `await db2.ItemDisplayInfoModelMatRes.getAllRows()`. The C++ version is a plain synchronous function. Same architectural note as findings 1, 4, and 10.

- [ ] 370. [DBItemDisplayInfoModelMatRes.cpp] `ensureInitialized` does not guard against concurrent calls (no `init_promise` equivalent)
  - **JS Source**: `src/js/db/caches/DBItemDisplayInfoModelMatRes.js` lines 42–45
  - **Status**: Pending
  - **Details**: The JS `ensure_initialized` is `async` and simply calls `await initializeIDIMMR()` which itself has no concurrent deduplication. The C++ equivalent is synchronous with a simple `is_initialized` guard — same level of protection. No additional concern beyond finding 16.
<!-- ─── src/js/db/caches/DBItemDisplays.cpp ─────────────────────────────── -->

- [ ] 371. [DBItemDisplays.cpp] Extra function `getTexturesByDisplayId` has no JS counterpart in DBItemDisplays.js
  - **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 66–68 (module.exports)
  - **Status**: Pending
  - **Details**: The JS module only exports `initializeItemDisplays` and `getItemDisplaysByFileDataID`. The C++ adds a third function `getTexturesByDisplayId` (defined at line 117–119 of DBItemDisplays.cpp, declared in DBItemDisplays.h line 35) that delegates to `DBItemDisplayInfoModelMatRes::getItemDisplayIdTextureFileIds`. This function does not exist in the JS source and represents an undocumented addition with no JS counterpart.

- [ ] 372. [DBItemDisplays.cpp] `getItemDisplayIdTextureFileIds` is called outside the per-modelFileDataID loop, changing the skip behaviour
  - **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 39–49
  - **Status**: Pending
  - **Details**: In the JS, `DBItemDisplayInfoModelMatRes.getItemDisplayIdTextureFileIds(itemDisplayInfoID)` is called inside the `for (const modelFileDataID of modelFileDataIDs)` loop, and a `continue` there skips only that single iteration. In the C++ (lines 83–93), the call is hoisted outside the loop and an early `continue` exits the entire display row if the result is null. In practice the result is the same for all iterations of the inner loop (same displayInfoID → same texture IDs), so the final map content is identical. However, the control-flow structure deviates from the JS original.

<!-- ─── src/js/db/caches/DBItemGeosets.cpp ─────────────────────────────── -->

- [ ] 373. [DBItemGeosets.cpp] `init_promise` concurrent-initialization guard is not ported
  - **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 20, 162–163, 225–226
  - **Status**: Pending
  - **Details**: The JS `initialize()` function stores an in-flight promise in `init_promise` and returns it immediately on re-entry to prevent double-initialization when called concurrently from multiple async contexts. The C++ `initialize()` (line 145) only checks `is_initialized` before proceeding. If C++ ever calls `initialize()` from multiple threads simultaneously before the flag is set, both could run the full initialization body. The JS pattern also resets `init_promise = null` after completion. While this may not cause issues in the current single-threaded call sites, it is a structural deviation from the JS original.

- [ ] 374. [DBItemGeosets.cpp] `ensure_initialized` / `ensureInitialized` missing async semantics — structural deviation
  - **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 231–234
  - **Status**: Pending
  - **Details**: The JS `ensure_initialized` is `async` and awaits `initialize()`, which itself returns the shared `init_promise`. The C++ `ensureInitialized()` (line 224) is synchronous. This is a known and acceptable adaptation for C++, but it should be noted as a deviation from the JS async model.

<!-- ─── src/js/db/caches/DBItemModels.cpp ─────────────────────────────── -->

- [ ] 375. [DBItemModels.cpp] `init_promise` concurrent-initialization guard is not ported
  - **JS Source**: `src/js/db/caches/DBItemModels.js` lines 19, 27–28, 102–103
  - **Status**: Pending
  - **Details**: Same pattern as DBItemGeosets: the JS `initialize()` uses `init_promise` to guard against concurrent calls and sets it to `null` after completion. The C++ `initialize()` (line 132) only checks `is_initialized`. The structural deviation is the same as finding 3 above.

- [ ] 376. [DBItemModels.cpp] `ensure_initialized` / `ensureInitialized` missing async semantics — structural deviation
  - **JS Source**: `src/js/db/caches/DBItemModels.js` lines 108–111
  - **Status**: Pending
  - **Details**: The JS `ensure_initialized` is `async` and awaits `initialize()`. The C++ `ensureInitialized()` (line 219) is synchronous. Acceptable C++ adaptation but a noted structural deviation.

<!-- ─── src/js/db/caches/DBItems.cpp ─────────────────────────────── -->

- [ ] 377. [DBItems.cpp] `init_promise` concurrent-initialization guard is not ported
  - **JS Source**: `src/js/db/caches/DBItems.js` lines 16, 18–19, 50–51
  - **Status**: Pending
  - **Details**: Same pattern as findings 3 and 5: the JS `initialize_items()` uses an `init_promise` to coalesce concurrent async callers and resets it to `null` after completion. The C++ `initialize()` (line 42) only checks `is_initialized_flag`. Structural deviation from the JS original.

- [ ] 378. [DBItems.cpp] `Display_lang` empty-string case not handled as a fallback for the item name
  - **JS Source**: `src/js/db/caches/DBItems.js` lines 40–41
  - **Status**: Pending
  - **Details**: The JS uses `item_row.Display_lang ?? 'Unknown item #' + item_id`. The nullish-coalescing operator `??` triggers on `null` or `undefined` but not on an empty string `""`. The C++ (lines 65–69) uses `item_row.find("Display_lang")` — if the field is present but the variant holds a non-string type (e.g., `int64_t`), `fieldToString` returns `""` and the empty string is silently stored as the name without applying the fallback. If the DB2 field is present but empty or zero-typed, the C++ will silently store an empty name instead of the `"Unknown item #N"` fallback, whereas the JS would use the fallback for `null`/`undefined` values. The check should also apply the fallback when `fieldToString` returns an empty string.

<!-- ─── src/js/db/caches/DBModelFileData.cpp ─────────────────────────────── -->
<!-- No issues found -->
<!-- ─── src/js/db/caches/DBNpcEquipment.cpp ─────────────────────────────── -->

- [ ] 379. [DBNpcEquipment.cpp] Missing concurrent-initialization guard (`init_promise`)
  - **JS Source**: `src/js/db/caches/DBNpcEquipment.js` lines 28–60
  - **Status**: Pending
  - **Details**: The JS implementation tracks an `init_promise` so that if `initialize()` is called a second time while the first call is still in progress, the second caller awaits the already-running promise instead of starting a duplicate load. The C++ implementation only checks `is_initialized` (line 58–60), which is set *after* the loop completes. If `initialize()` is called concurrently from multiple threads before the flag is set, the table could be loaded multiple times. The C++ needs either a mutex or a `std::future`/`std::atomic` guard equivalent to `init_promise`.

<!-- ─── src/js/db/caches/DBTextureFileData.cpp ────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/hashing/xxhash64.cpp ──────────────────────────────────────── -->

- [ ] 380. [xxhash64.cpp] `check_glyph_support` note: `update()` memory lazy-init differs but is harmless — actual issue is JS `toUTF8Array` vs C++ raw-byte treatment of `std::string_view`
  - **JS Source**: `src/js/hashing/xxhash64.js` lines 20–39
  - **Status**: Pending
  - **Details**: The JS `update()` method converts a JS string argument via `toUTF8Array(input)`, which performs a full Unicode-aware UTF-16 → UTF-8 transcoding (handling surrogate pairs at lines 33–36). The C++ `update(std::string_view)` overload (xxhash64.cpp line 153–156) reinterprets the string view's bytes directly without any transcoding. In practice C++ string literals and `std::string` values are already UTF-8, so for typical WoW file paths the results are identical. However, if a caller passes a string containing non-ASCII characters that were originally represented as UTF-16 in the JS layer (e.g., Korean or Chinese locale display names), the hash computed by C++ would differ from the hash computed by JS. This is a latent interoperability hazard that should be documented.

- [ ] 381. [xxhash64.cpp] `digest()` — JS BigInt arithmetic applies `& MASK_64` after every multiply/add; C++ relies on `uint64_t` natural overflow
  - **JS Source**: `src/js/hashing/xxhash64.js` lines 201–284
  - **Status**: Pending
  - **Details**: The JS implementation uses native BigInt and explicitly truncates to 64 bits after every operation via `& MASK_64` (e.g., lines 213–239). C++ uses `uint64_t` arithmetic which wraps naturally at 2^64. For the same operations on 64-bit unsigned integers this produces identical bit patterns, so the hashes are functionally equivalent. No code change is needed, but this should be explicitly noted in a comment in xxhash64.cpp to explain the equivalence, as it is a non-obvious deviation from the JS source.

- [ ] 382. [xxhash64.cpp] `update()` large-block loop entry condition differs in form from JS
  - **JS Source**: `src/js/hashing/xxhash64.js` line 161
  - **Status**: Pending
  - **Details**: JS uses `if (p <= bEnd - 32)` (line 161) as the guard before the main 32-byte block loop. C++ uses `if (p + 32 <= bEnd)` (line 113). Both expressions are mathematically equivalent for non-negative values and produce the same result. However, the JS form is the authoritative guard and the C++ form deviates in style from the source. More importantly, the JS condition is `<=` (enters loop when exactly 32 bytes remain), and the C++ condition `p + 32 <= bEnd` also enters when exactly 32 bytes remain. These are identical. The deviation is purely cosmetic but worth noting for line-by-line fidelity audits.

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

- [ ] 393. [legacy_tab_data.cpp] DBC listbox passes non-empty unittype despite JS using `:includefilecount="false"`
  - **JS Source**: `src/js/modules/legacy_tab_data.js` line 131
  - **Status**: Pending
  - **Details**: The JS Listbox prop `:includefilecount="false"` disables the file counter. The C++ `listbox::render` call passes `"dbc file"` as `unittype`, which enables the file count display in the C++ implementation. To match the JS behaviour the unittype should be `""` (empty string) so that `listbox::renderStatusBar` shows nothing. The existing `renderStatusBar("table", {}, listbox_dbc_state)` call on the DBC list also renders an unwanted status bar not present in the JS template.

- [ ] 394. [legacy_tab_data.cpp] DBC listbox passes `persistscrollkey="dbc"` but JS template has no `persistscrollkey`
  - **JS Source**: `src/js/modules/legacy_tab_data.js` line 131
  - **Status**: Pending
  - **Details**: The JS `<Listbox>` for the DBC list does not set the `persistscrollkey` prop. The C++ call passes `"dbc"` as the persist scroll key. This causes scroll position to be saved/restored across tab switches when it should not be. The argument should be `""` (empty string) to disable scroll persistence.

- [ ] 395. [legacy_tab_data.cpp] Options checkboxes are missing hover tooltips present in the JS template
  - **JS Source**: `src/js/modules/legacy_tab_data.js` lines 146–157
  - **Status**: Pending
  - **Details**: Three checkbox labels in the `tab-data-options` section carry `title` attributes in the JS template, which render as browser hover tooltips:
    - "Copy Header" → `title="Include header row when copying"`
    - "Create Table" → `title="Include DROP/CREATE TABLE statements"`
    - "Export all rows" → `title="Export all rows"`
    The C++ renders these checkboxes (`ImGui::Checkbox`) without any `ImGui::IsItemHovered()` + `ImGui::SetTooltip()` call after each one. Each checkbox should have a corresponding tooltip.

<!-- ─── src/js/modules/legacy_tab_files.cpp ─────────────────────────────── -->

- [ ] 396. [legacy_tab_files.cpp] "Regex Enabled" label is missing hover tooltip and `SameLine` before filter input
  - **JS Source**: `src/js/modules/legacy_tab_files.js` line 84
  - **Status**: Pending
  - **Details**: The JS template places `<div class="regex-info" :title="$core.view.regexTooltip">Regex Enabled</div>` and the filter `<input>` together inline. The C++ renders `ImGui::TextUnformatted("Regex Enabled")` without calling `ImGui::IsItemHovered()` + `ImGui::SetTooltip(view.regexTooltip.c_str())` and without `ImGui::SameLine()` afterwards, so the filter input falls onto a new line. Both the tooltip and the `SameLine()` call are required to match the JS layout and behaviour.

- [ ] 397. [legacy_tab_files.cpp] Missing `renderStatusBar` call — file count not displayed despite JS using `:includefilecount="true"`
  - **JS Source**: `src/js/modules/legacy_tab_files.js` line 75
  - **Status**: Pending
  - **Details**: The JS Listbox prop `:includefilecount="true"` enables a visible file counter below the list. The C++ call correctly passes `unittype = "file"` to `listbox::render`, but no `listbox::renderStatusBar(...)` call is made and no `BeginStatusBar`/`EndStatusBar` region is opened. As a result the file count is never shown. A `BeginStatusBar` / `renderStatusBar("file", {}, listbox_state)` / `EndStatusBar` block must be added between `EndListContainer` and `BeginFilterBar` to match JS behaviour.

<!-- ─── src/js/modules/legacy_tab_fonts.cpp ─────────────────────────────── -->

- [ ] 398. [legacy_tab_fonts.cpp] Filter input uses `ImGui::InputText` instead of `ImGui::InputTextWithHint` — placeholder text missing
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 72
  - **Status**: Pending
  - **Details**: The JS template has `<input type="text" placeholder="Filter fonts..."/>`. The C++ filter bar uses `ImGui::InputText("##FilterFonts", ...)` which has no placeholder. It should be `ImGui::InputTextWithHint("##FilterFonts", "Filter fonts...", ...)` to display the placeholder hint when the field is empty, matching the JS behaviour.

- [ ] 399. [legacy_tab_fonts.cpp] "Regex Enabled" label is missing hover tooltip and `SameLine` before filter input
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 71
  - **Status**: Pending
  - **Details**: The JS template has `<div class="regex-info" :title="$core.view.regexTooltip">Regex Enabled</div>` inline with the filter input. The C++ (lines 248–255) renders `ImGui::TextUnformatted("Regex Enabled")` without `ImGui::IsItemHovered()` + `ImGui::SetTooltip(view.regexTooltip.c_str())` and without `ImGui::SameLine()`, so the filter input is placed on a new line. Both the tooltip and `SameLine()` are required.

- [ ] 400. [legacy_tab_fonts.cpp] Context menu contains extra items not present in the JS template
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 64–68
  - **Status**: Pending
  - **Details**: The JS context menu for the fonts listbox has exactly three items: "Copy file path(s)", "Copy export path(s)", "Open export directory". The C++ context menu lambda (lines 217–235) adds two additional items guarded by `hasFileDataIDs`: "Copy file path(s) (listfile format)" and "Copy file data ID(s)". These items do not exist in the JS template and should be removed. Because MPQ-sourced font files never have file data IDs, `hasFileDataIDs` will always be false, so they never appear in practice — but the code is still a deviation from the JS source.

- [ ] 401. [legacy_tab_fonts.cpp] Export button uses deprecated `app::theme::BeginDisabledButton`/`EndDisabledButton` instead of `ImGui::BeginDisabled`/`EndDisabled`
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 83
  - **Status**: Pending
  - **Details**: Lines 345–348 in legacy_tab_fonts.cpp use `app::theme::BeginDisabledButton()` and `app::theme::EndDisabledButton()` to disable the Export button when busy. Per CLAUDE.md, `app::theme` APIs should be progressively removed and not used in new code. The correct approach is `if (busy) ImGui::BeginDisabled(); ... if (busy) ImGui::EndDisabled();` as used in the files and textures tabs.

<!-- ─── src/js/modules/legacy_tab_home.cpp ──────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/legacy_tab_textures.cpp ──────────────────────────── -->

- [ ] 402. [legacy_tab_textures.cpp] "Regex Enabled" label is missing `SameLine` before filter input
  - **JS Source**: `src/js/modules/legacy_tab_textures.js` line 125
  - **Status**: Pending
  - **Details**: The JS template renders the regex info div and the filter input together inline inside `<div class="filter">`. The C++ filter bar (lines 347–358) renders `ImGui::TextUnformatted("Regex Enabled")` with an `IsItemHovered` + `SetTooltip` (correct) but does NOT call `ImGui::SameLine()` afterwards. This means the filter input is placed on a new line below "Regex Enabled" instead of on the same line, deviating from the JS layout. An `ImGui::SameLine()` call must be added after the closing brace of the `if (view.config.value("regexFilters", false))` block, before `ImGui::SetNextItemWidth`.

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

- [ ] 436. [tab_audio.cpp] Filter bar: JS shows regexTooltip on the "Regex Enabled" text; C++ omits tooltip
  - **JS Source**: `src/js/modules/tab_audio.js` lines 199–202
  - **Status**: Pending
  - **Details**: The JS template: `<div class="regex-info" v-if="..." :title="$core.view.regexTooltip">Regex Enabled</div>`. The C++ renders `ImGui::TextUnformatted("Regex Enabled")` with no tooltip. The `regexTooltip` view property tooltip is missing.

- [ ] 437. [tab_audio.cpp] Filter input placeholder "Filter sound files..." is missing from C++
  - **JS Source**: `src/js/modules/tab_audio.js` line 201
  - **Status**: Pending
  - **Details**: The JS `<input type="text" ... placeholder="Filter sound files..."/>`. The C++ `InputText("##FilterSounds", ...)` has no hint/placeholder text. ImGui does support `ImGui::InputTextWithHint()` for placeholder text.


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


<!-- ─── src/js/modules/tab_models.cpp ─────────────────────────────────────── -->

- [ ] 505. [tab_models.cpp] active_skins unified Map split into two separate maps (creature vs item)
  - **JS Source**: `src/js/modules/tab_models.js` lines 18, 52–58, 136
  - **Status**: Pending
  - **Details**: JS uses a single `active_skins` Map that stores display info regardless of source (creature or item). C++ splits into `active_skins_creature` and `active_skins_item`. The JS `get_model_displays` merges creature and item displays into one array and all are stored in the same map. The C++ split is functional but requires care in the watch handler to try both maps. The current C++ `handle_skins_selection_change` does try both maps. Minor structural deviation.

- [ ] 506. [tab_models.cpp] JS skin_name uses path.basename stripping .blp; C++ reimplements basename logic manually
  - **JS Source**: `src/js/modules/tab_models.js` lines 107–128
  - **Status**: Pending
  - **Details**: JS uses `path.basename(skin_name, '.blp')` and `path.basename(model_name, 'm2')`. C++ manually strips path components and extensions. The C++ strips `.m2` by checking if the string ends with `"m2"` (2 chars) — but JS strips extension `'m2'` with `path.basename(model_name, 'm2')` which removes the `m2` suffix even if there's no dot before it. The C++ checks `model_name.compare(model_name.size() - 2, 2, "m2") == 0` which would match `somefilem2` (no dot). The intent is to strip `.m2` and `m2` both. Minor edge case difference.

- [ ] 507. [tab_models.cpp] JS clean_skin_name uses replace() for first occurrence only; C++ uses find+erase
  - **JS Source**: `src/js/modules/tab_models.js` line 119
  - **Status**: Pending
  - **Details**: JS: `clean_skin_name = skin_name.replace(model_name, '').replace('_', '')`. `String.replace()` replaces only the first occurrence. C++ uses `clean_skin_name.find(model_name)` + `erase` and then `clean_skin_name.find('_')` + `erase`. This matches first-occurrence-only behaviour. Appears equivalent.

- [ ] 508. [tab_models.cpp] skin extraGeosets joined differently: JS uses .join(','), C++ builds manually
  - **JS Source**: `src/js/modules/tab_models.js` lines 127–128
  - **Status**: Pending
  - **Details**: JS: `skin_name += display.extraGeosets.join(',')`. C++ builds manually with a loop and comma separator. Functionally equivalent.

- [ ] 509. [tab_models.cpp] Missing "Show Bones" checkbox in JS — C++ adds it
  - **JS Source**: `src/js/modules/tab_models.js` lines 357–399
  - **Status**: Pending
  - **Details**: The JS template sidebar does NOT include a "Show Bones" checkbox. The C++ adds `ImGui::Checkbox("Show Bones", ...)` (line 1499). This is an addition not present in the original JS. Per fidelity rules this should not exist unless it was intentionally added as a C++-specific enhancement. However it controls `config.modelViewerShowBones` which may be used in the renderer, so it may be a genuine new feature. This should be documented.

- [ ] 510. [tab_models.cpp] JS isBusy is a boolean; C++ treats isBusy as integer (> 0)
  - **JS Source**: `src/js/modules/tab_models.js` line 355
  - **Status**: Pending
  - **Details**: JS passes `:disabled="$core.view.isBusy"` (boolean). C++ uses `view.isBusy > 0` (integer count). This is an architectural difference in the busy-lock system. It doesn't affect functionality here but is a structural deviation.

- [ ] 511. [tab_models.cpp] JS selectionModels watch checks `!this._tab_initialized`; C++ checks `is_initialized`
  - **JS Source**: `src/js/modules/tab_models.js` lines 639–649
  - **Status**: Pending
  - **Details**: JS guards with `if (!this._tab_initialized) return;`. C++ uses `if (is_initialized && ...)`. The JS `_tab_initialized` flag is never explicitly set to true in the JS shown — it appears to be set elsewhere. C++ uses `is_initialized` which is set to true in the `postToMainThread` lambda at the end of `initialize()`. Functionally equivalent in effect.

- [ ] 512. [tab_models.cpp] Context menu uses BeginPopup/MenuItem instead of ContextMenu component — different dismissal behaviour
  - **JS Source**: `src/js/modules/tab_models.js` lines 288–294
  - **Status**: Pending
  - **Details**: The JS uses the shared ContextMenu component with `@close="$core.view.contextMenus.nodeListbox = null"` to dismiss when clicked outside. The C++ uses `ImGui::BeginPopup("ModelsListboxContextMenu")` which is opened via `ImGui::OpenPopup` in the listbox callback. This diverges from the shared `context-menu.h` component used by other tabs. The popup auto-dismisses on click-outside in ImGui, so functionally similar, but the exact trigger mechanism differs.

- [ ] 513. [tab_models.cpp] JS modelsAutoPreview auto-preview fires only when _tab_initialized; C++ fires on any is_initialized true
  - **JS Source**: `src/js/modules/tab_models.js` line 641
  - **Status**: Pending
  - **Details**: In JS the `selectionModels` watch exits early if `!this._tab_initialized`. This flag is not set in the shown JS. In C++ `is_initialized` is set true in `initialize()`. If `selectionModels` changes during loading, the C++ may attempt preview while the JS would skip it. Minor edge case.

- [ ] 514. [tab_models.cpp] export_files does not call helper.finish() after finish_pending_export_task in certain paths
  - **JS Source**: `src/js/modules/tab_models.js` lines 199–276
  - **Status**: Pending
  - **Details**: In JS `export_files`, after the loop `helper.finish()` is always called before `export_paths?.close()`. In C++ `pump_export_task()`, when the task is cancelled, `finish_pending_export_task()` is called WITHOUT calling `helper.finish()`. JS always calls `helper.finish()` regardless of cancellation state. This may cause the export helper to not properly mark the operation as completed when cancelled.


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

- [ ] 558. [model-viewer-utils.cpp] `export_preview` has an extra `export_paths` parameter not present in the JS signature
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 277–308
  - **Status**: Pending
  - **Details**: JS `export_preview` is `(core, format, canvas, export_name, export_subdir = '')`. C++ signature is `export_preview(format, ctx, export_name, export_subdir, export_paths)`. The C++ adds `export_paths` as an explicit parameter (JS uses a locally-opened stream). This changes the call convention and caller interface, though the behaviour is equivalent.

- [ ] 559. [model-viewer-utils.cpp] `initialize_uv_layers` accepts only `M2RendererGL*` but JS accepts any renderer with a `getUVLayers` method
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 107–118
  - **Status**: Pending
  - **Details**: JS `initialize_uv_layers(state, renderer)` calls `renderer.getUVLayers` and works with M2, M3, or WMO renderers. C++ `initialize_uv_layers(ViewStateProxy&, M2RendererGL*)` only accepts an M2 renderer pointer. M3 and WMO renderers will never populate UV layers in C++ even if they would in JS.

- [ ] 560. [model-viewer-utils.cpp] `toggle_uv_layer` accepts only `M2RendererGL*` but JS accepts any renderer with `getUVLayers`
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 126–152
  - **Status**: Pending
  - **Details**: Same issue as `initialize_uv_layers`. JS toggles UV overlay for any renderer type; C++ restricts to M2 only, so WMO/M3 UV overlays will not work.

- [ ] 561. [model-viewer-utils.cpp] `export_preview` captures the GL framebuffer via `glReadPixels` instead of reading from an HTML Canvas
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 280
  - **Status**: Pending
  - **Details**: JS uses `BufferWrapper.fromCanvas(canvas, 'image/png')` to capture from an HTML Canvas. C++ reads from the OpenGL framebuffer with `glReadPixels`. This is a necessary C++-specific adaptation since there is no HTML canvas, but means the captured image depends on what is bound to the current GL framebuffer at call time, not a specific canvas element.

- [ ] 562. [model-viewer-utils.cpp] `create_view_state` does not take a `core` parameter (JS takes `(core, prefix)`)
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 510–535
  - **Status**: Pending
  - **Details**: JS `create_view_state(core, prefix)` takes an explicit `core` reference. C++ uses the global `core::view`. This is a reasonable adaptation but changes the API signature and couples the function to the global state singleton.

- [ ] 563. [model-viewer-utils.cpp] `handle_animation_change` calls `renderer->playAnimation(m2_index).get()` (blocking) instead of awaiting asynchronously
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 262
  - **Status**: Pending
  - **Details**: JS uses `await renderer.playAnimation(anim_info.m2Index)`. C++ calls `.get()` on the future, blocking the calling thread. If playAnimation is long-running, this will block the render thread. The semantic effect is the same but the async model differs.

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
