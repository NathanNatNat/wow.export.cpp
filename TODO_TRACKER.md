# TODO Tracker

## Audit Findings

### [constants.cpp] Missing BLENDER namespace constants
- **JS Source**: `src/js/constants.js` lines 20–61
- **Status**: Fixed
- **Details**: Added `getBlenderBaseDir()` (cross-platform: Windows `%APPDATA%/Blender Foundation/Blender`, Linux `~/.config/blender`), `BLENDER::DIR()`, `BLENDER::ADDON_DIR`, `BLENDER::LOCAL_DIR()`, `BLENDER::ADDON_ENTRY`, and `BLENDER::MIN_VER` to both `constants.h` and `constants.cpp`.

### [constants.cpp] CONTEXT_MENU_ORDER missing 3 entries vs JS source
- **JS Source**: `src/js/constants.js` lines 201–214
- **Status**: Fixed
- **Details**: Restored `tab_blender`, `tab_changelog`, and `tab_help` entries to `CONTEXT_MENU_ORDER` in `constants.h`. Array size changed from 9 to 12 to match JS source.

### [constants.cpp] SHADER_PATH uses different directory structure than JS
- **JS Source**: `src/js/constants.js` line 43
- **Status**: Fixed (deviation documented)
- **Details**: JS sets `SHADER_PATH` to `path.join(INSTALL_PATH, 'src', 'shaders')`. C++ sets it to `DATA_DIR / "shaders"` (i.e., `<install>/data/shaders`). This is intentional for the C++ port's resource layout. Deviation comment added in `constants.h`.

### [constants.cpp] CONFIG.DEFAULT_PATH uses different directory than JS
- **JS Source**: `src/js/constants.js` line 95
- **Status**: Fixed (deviation documented)
- **Details**: JS sets `CONFIG.DEFAULT_PATH` to `path.join(INSTALL_PATH, 'src', 'default_config.jsonc')`. C++ sets it to `DATA_DIR / "default_config.jsonc"`. Intentional for C++ resource layout. Deviation comment added in `constants.h`.

### [constants.cpp] RUNTIME_LOG path differs from JS
- **JS Source**: `src/js/constants.js` line 38
- **Status**: Fixed (deviation documented)
- **Details**: JS stores runtime log at `path.join(DATA_PATH, 'runtime.log')`. C++ stores it in `LOG_DIR / "runtime.log"` (i.e., `<install>/Logs/runtime.log`). C++ also adds a `LOG_DIR` concept not present in JS. Deviation comments added in `constants.h`.

### [constants.cpp] DATA_PATH renamed to DATA_DIR, different base path
- **JS Source**: `src/js/constants.js` line 16
- **Status**: Fixed (deviation documented)
- **Details**: JS uses `DATA_PATH = nw.App.dataPath` (OS-specific user data directory). C++ renames to `DATA_DIR` and uses `<install>/data/` for a portable, self-contained layout. Deviation comment added in `constants.h`.

### [constants.cpp] Cache directory renamed from `casc/` to `cache/`
- **JS Source**: `src/js/constants.js` line 73
- **Status**: Fixed (deviation documented)
- **Details**: JS uses `DATA_PATH/casc/` as the cache directory. C++ renames it to `cache/` and includes migration code from `casc/` to `cache/` on first run. Deviation comment added in `constants.h`.

### [buffer.cpp] Missing fromCanvas() static method
- **JS Source**: `src/js/buffer.js` lines 89–107
- **Status**: Pending
- **Details**: The JS `BufferWrapper.fromCanvas(canvas, mimeType, quality)` method creates a buffer from an HTML canvas element, with special WebP lossless support via `webp-wasm`. This is browser/NW.js-specific. In the C++ port using Dear ImGui/OpenGL, the equivalent would be reading pixels from an OpenGL framebuffer. This needs a C++ equivalent for screenshot/export functionality, or documentation of how it's handled differently.

### [buffer.cpp] Missing decodeAudio() method
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Status**: Pending
- **Details**: The JS `decodeAudio(context)` method decodes buffer data using the Web Audio API's `AudioContext.decodeAudioData()`. The C++ port uses miniaudio for audio, so this method needs a miniaudio-based equivalent or should be documented as handled elsewhere.

### [config.cpp] save() calls doSave() synchronously instead of deferred
- **JS Source**: `src/js/config.js` lines 83–91
- **Status**: Pending
- **Details**: The JS `save()` uses `setImmediate(doSave)` to defer the actual save operation to the next event loop tick. This prevents blocking the current call stack and allows multiple rapid config changes to be batched. The C++ `save()` calls `doSave()` directly and synchronously, which means each save blocks until disk I/O completes. This could cause UI stuttering if config changes happen frequently. Consider using `core::postToMainThread()` or a background thread to match the deferred semantics.

### [config.cpp] Missing Vue $watch equivalent for automatic config persistence
- **JS Source**: `src/js/config.js` line 60
- **Status**: Pending
- **Details**: The JS `load()` sets up `core.view.$watch('config', () => save(), { deep: true })` which automatically triggers a save whenever any config property changes. The C++ port has a comment noting this is replaced by explicit `save()` calls from the UI layer. This means every UI code path that modifies config must remember to call `config::save()`, which is error-prone. Consider implementing a change-detection mechanism or documenting all call sites that need to invoke save.

### [core.cpp] AppState missing `constants` field
- **JS Source**: `src/js/core.js` line 45
- **Status**: Pending
- **Details**: The JS view state includes `constants: constants` which stores a reference to the constants module. This is used in Vue templates to access constants directly from the view context (e.g., `view.constants.EXPANSIONS`). Since ImGui doesn't use templates, this may not be needed, but it should be documented as an intentional omission.

### [core.cpp] setToast() parameter type differs from JS
- **JS Source**: `src/js/core.js` line 470
- **Status**: Pending
- **Details**: The JS `setToast(toastType, message, options, ttl, closable)` takes a generic `options` object as the third parameter (which can be `null` or contain action buttons). The C++ signature in `src/js/core.h` line 589 uses `const std::vector<ToastAction>& actions` instead. Per fidelity rules, this deviation must either be reverted to accept a more generic type (e.g., `nlohmann::json` or `std::optional<nlohmann::json>`) to match the JS interface, or a deviation comment must be added to the C++ code explaining why the typed vector was chosen and how it differs from the original JS behavior. Verify all call sites are compatible with this interface change.

### [generics.cpp] getJSON() error message differs from JS
- **JS Source**: `src/js/generics.js` lines 101–105
- **Status**: In Progress
- **Details**: The JS `getJSON()` throws `"Unable to request JSON from end-point. HTTP ${res.status} ${res.statusText}"` when the response is not ok. The C++ `getJSON()` now checks for parse failure and throws a generic message but cannot include the HTTP status code/text because `get()` returns raw bytes (the status info is lost). The HTTP status range check in `doHttpGet()` has been fixed from `> 302` to `>= 300` to match JS `res.ok` semantics. To fully match the JS error message, `get()` would need to return status information alongside the body.

### [generics.cpp] requestData() missing download progress logging
- **JS Source**: `src/js/generics.js` lines 130–164
- **Status**: Pending
- **Details**: The JS `requestData()` is a full HTTP implementation with chunked response handling that tracks download progress and logs at 25% thresholds (`"Download progress: %d/%d bytes (%d%%)"`) using the `content-length` header. It also handles 301/302 redirects with explicit logging (`"Got redirect to " + res.headers.location`) and has a 60-second timeout. The C++ `requestData()` (lines 338–343) is a thin wrapper around `doHttpGet()` with just two log lines — all progress tracking, redirect logging, and the content-length-based progress reporting are missing.

### [generics.cpp] queue() off-by-one: launches `limit` concurrent tasks instead of `limit + 1`
- **JS Source**: `src/js/generics.js` lines 57–73
- **Status**: Pending
- **Details**: The JS `queue()` initializes `free = limit; complete = -1;` and the `check()` callback immediately increments both (`complete++; free++`), so the first invocation sets `free = limit + 1` and launches up to `limit + 1` concurrent tasks. The C++ version (lines 221–245) uses `while (futures.size() < limit)` which launches exactly `limit` tasks. Additionally, the C++ waits for `futures.front().get()` (always the oldest task), while the JS resolves whichever Promise completes first via `.then(check)`. The C++ behavior is functionally similar but slightly less concurrent.

### [generics.cpp] get() returns raw bytes instead of Response-like object
- **JS Source**: `src/js/generics.js` lines 18–49
- **Status**: Pending
- **Details**: The JS `get()` returns a `fetch` Response object which callers use for `.json()`, `.ok`, `.status`, etc. The C++ `get()` returns `std::vector<uint8_t>`. While downstream callers (`getJSON`, `downloadFile`) have been adapted, any future code porting that calls `get()` expecting Response-like methods will need adaptation. This is a known architectural difference. Also, the JS `get()` uses `AbortSignal.timeout(30000)` for a 30s request-level timeout; the C++ uses separate connection (30s) and read (60s) timeouts via cpp-httplib, which is a different timeout model.

### [icon-render.cpp] processQueue() is stubbed — BLP loading not implemented
- **JS Source**: `src/js/icon-render.js` lines 46–57
- **Status**: Blocked
- **Details**: The JS `processQueue()` loads a BLP file from CASC (`core.view.casc.getFile(entry.fileDataID)`), decodes it with `BLPFile`, and sets the CSS background-image to the decoded data URL. The C++ `processQueue()` (lines 209–229) has the CASC+BLP loading commented out with a note that "CASC source and BLP decoder (casc/blp.cpp) are unconverted". The actual icon texture creation from BLP data is not implemented — icons will always show the default placeholder. Blocked on CASC and BLP tier conversion.

### [icon-render.cpp] processQueue() processes all entries synchronously instead of one-at-a-time async
- **JS Source**: `src/js/icon-render.js` lines 46–57
- **Status**: Pending
- **Details**: The JS `processQueue()` pops one entry, processes it asynchronously (CASC getFile returns a Promise), and recursively calls itself via `.finally()`. This means each icon is loaded one-at-a-time with yielding between loads. The C++ version (lines 209–229) processes all entries in a `while (!_queue.empty())` synchronous loop. When CASC+BLP loading is implemented, this should be changed to process one entry at a time (or batch with yielding) to avoid blocking the main thread.

### [log.cpp] drainPool() missing re-scheduling for remaining pool entries
- **JS Source**: `src/js/log.js` lines 34–49
- **Status**: Pending
- **Details**: The JS `drainPool()` has a post-loop check: `if (!isClogged && pool.length > 0) process.nextTick(drainPool);` which schedules another drain on the next event loop tick if the pool still has entries after processing `MAX_DRAIN_PER_TICK` items. The C++ `drainPool()` (lines 67–86) lacks this continuation — if the pool has more than 50 entries and the stream recovers, only 50 entries are drained per `write()` call. Remaining entries must wait for the next `write()` invocation. This could cause pool buildup if writes are infrequent after a clog recovery.

### [log.cpp] write() calls drainPool() before checking stream, JS checks stream first
- **JS Source**: `src/js/log.js` lines 79–93
- **Status**: Pending
- **Details**: The JS `write()` first attempts `stream.write(line)` and only pools if clogged. The drain event handler is separate (`stream.on('drain', drainPool)`). The C++ `write()` (lines 100–132) calls `drainPool()` at the top if `isClogged` is true, then attempts to write. This ordering difference means the C++ tries to flush old pool entries before writing the new line, while JS writes the new line first and pools/drains independently. The functional impact is that during clog recovery, C++ drains old entries first (possibly un-clogging) then writes the new line directly, while JS would pool the new line and drain asynchronously. The C++ approach is arguably better for ordering but differs from JS behavior.

### [mmap.cpp] release_virtual_files() outer catch drops error message
- **JS Source**: `src/js/mmap.js` lines 35–47
- **Status**: Fixed
- **Details**: Fixed: Added `std::exception` catch handler with `.what()` message to match JS `e.message` logging.

### [modules.cpp] Missing 5 modules from JS MODULES object
- **JS Source**: `src/js/modules.js` lines 27–59
- **Status**: Pending
- **Details**: The JS `MODULES` object includes `module_test_a`, `module_test_b`, `tab_help`, `tab_blender`, and `tab_changelog`, but none of these have been ported to C++. No corresponding header/cpp files exist in `src/js/modules/`. The C++ `modules.cpp` previously marked these as "Removed: module deleted" which is incorrect — they exist in the JS source. Comments have been corrected to indicate they are pending conversion. Each module needs its `.cpp` and `.h` files created and its `add_module()` registration added to `modules::initialize()`.