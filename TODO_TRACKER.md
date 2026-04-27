# TODO Tracker

> **Progress: 0/323 verified (0%)** — ✅ = Verified, ⬜ = Pending

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
