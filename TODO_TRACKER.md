# TODO Tracker

> **Progress: 0/70 verified (0%)** — ✅ = Verified, ⬜ = Pending

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
