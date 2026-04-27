# TODO Tracker

> **Progress: 0/148 verified (0%)** — ✅ = Verified, ⬜ = Pending

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

- [ ] 51. [generics.cpp] `requestData()` / `doHttpGetRaw()` missing the JS 60-second request timeout
  - **JS Source**: `src/js/generics.js` lines 197–200
  - **Status**: Pending
  - **Details**: JS calls `req.setTimeout(60000, ...)` and destroys the request on timeout. The C++ `doHttpGetRaw()` path does not apply an explicit timeout to the underlying `httplib` request; a slow or stalled server could block indefinitely.

- [ ] 52. [generics.cpp] `redraw()` uses `std::this_thread::yield()` instead of `requestAnimationFrame`
  - **JS Source**: `src/js/generics.js` lines 266–271
  - **Status**: Pending
  - **Details**: JS calls `requestAnimationFrame()` twice to defer until the next two animation frames, keeping the UI responsive. C++ calls `std::this_thread::yield()` twice, which is OS-scheduler-dependent and does not synchronise with the ImGui render loop.

- [ ] 53. [generics.cpp] `batchWork()` uses `sleep_for(0)` instead of `MessageChannel` scheduling
  - **JS Source**: `src/js/generics.js` lines 423–472
  - **Status**: Pending
  - **Details**: JS uses `MessageChannel` to schedule each batch chunk via the event loop, keeping the UI responsive between chunks. C++ uses `std::this_thread::sleep_for(std::chrono::milliseconds(0))`, which yields to the OS but does not pump the ImGui event loop. Long batch operations may still freeze the UI.

- [ ] 54. [generics.cpp] `queue()` polls futures in a busy loop instead of using event-driven completion
  - **JS Source**: `src/js/generics.js` lines 63–82
  - **Status**: Pending
  - **Details**: JS triggers the next item immediately when a previous one completes (promise `.then()` chain). C++ polls all in-flight `std::future`s in a loop looking for any that are ready, introducing latency and CPU burn between completions.

- [ ] 55. [generics.cpp] `fileExists`, `directoryIsWritable`, `readFile`, `deleteDirectory`, `createDirectory` are synchronous; JS versions are async
  - **JS Source**: `src/js/generics.js` lines 258–411
  - **Status**: Pending
  - **Details**: All five functions are `async` in JS and return Promises. C++ implementations are synchronous and block the calling thread. Callers that relied on the non-blocking behaviour need to be aware.

- [ ] 56. [generics.cpp] `get()` URL-retry log shows incorrect index/total (does not decrement total as JS does)
  - **JS Source**: `src/js/generics.js` lines 40–41
  - **Status**: Pending
  - **Details**: JS logs `[${index}/${url_stack.length + index}]` where `url_stack.length` decreases as URLs are consumed, so the total changes. C++ logs `[${index}/${urls.size()}]` where the total is always fixed. Log output is misleading when multiple URLs are in the list.

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
