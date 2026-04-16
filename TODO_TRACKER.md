# TODO Tracker

> **Progress: 0/21 verified (0%)** — ✅ = Verified, ⬜ = Pending

### 1. [app.cpp] Auto-updater flow from app.js is not ported
- **JS Source**: `src/app.js` lines 691–704
- **Status**: Pending
- **Details**: `updater.checkForUpdates()` / `updater.applyUpdate()` flow is still disabled in C++; only `tab_blender::checkLocalVersion()` runs.

### 2. [app.cpp] Drag enter/leave overlay behavior differs from JS
- **JS Source**: `src/app.js` lines 590–660
- **Status**: Pending
- **Details**: JS uses `ondragenter`/`ondragleave` plus `dropStack` to show and hide the file-drop prompt during drag-over, while C++ only handles drop callbacks.

### 3. [blob.cpp] Blob stream semantics differ (async pull stream vs eager callback loop)
- **JS Source**: `src/js/blob.js` lines 271–288
- **Status**: Pending
- **Details**: JS returns a lazy `ReadableStream` with async `pull()`, but C++ `BlobPolyfill::stream` is synchronous and eager.

### 4. [blob.cpp] URLPolyfill.createObjectURL native fallback path is missing
- **JS Source**: `src/js/blob.js` lines 294–300
- **Status**: Pending
- **Details**: JS falls back to native `URL.createObjectURL(blob)` for non-polyfill blobs; C++ only accepts `BlobPolyfill` and has no fallback path.

### 5. [blob.cpp] URLPolyfill.revokeObjectURL native revoke path is missing
- **JS Source**: `src/js/blob.js` lines 302–306
- **Status**: Pending
- **Details**: JS calls native `URL.revokeObjectURL(url)` for non-`data:` URLs; C++ is effectively a no-op for that case.

### 6. [buffer.cpp] alloc(false) behavior differs from JS allocUnsafe
- **JS Source**: `src/js/buffer.js` lines 54–56
- **Status**: Pending
- **Details**: JS uses `Buffer.allocUnsafe()` when `secure=false`; C++ always zero-initializes via `std::vector<uint8_t>(length)`.

### 7. [buffer.cpp] fromCanvas API/behavior is not directly ported
- **JS Source**: `src/js/buffer.js` lines 89–107
- **Status**: Pending
- **Details**: JS accepts canvas/OffscreenCanvas and browser Blob APIs, while C++ replaces this with `fromPixelData(...)` and different call semantics.

### 8. [buffer.cpp] readString encoding parameter does not affect behavior
- **JS Source**: `src/js/buffer.js` lines 551–553
- **Status**: Pending
- **Details**: JS passes `encoding` into `Buffer.toString(encoding, ...)`; C++ accepts the parameter but ignores it.

### 9. [buffer.cpp] decodeAudio(context) method is missing
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Status**: Pending
- **Details**: JS exposes `decodeAudio(context)` on `BufferWrapper`; C++ intentionally omits this method and relies on other audio paths.

### 10. [buffer.cpp] getDataURL creates data URLs instead of blob URLs
- **JS Source**: `src/js/buffer.js` lines 989–995
- **Status**: Pending
- **Details**: JS uses `URL.createObjectURL(new Blob(...))` (`blob:` URLs), while C++ emits `data:application/octet-stream;base64,...`.

### 11. [buffer.cpp] readBuffer wrap parameter is split into separate APIs
- **JS Source**: `src/js/buffer.js` lines 531–542
- **Status**: Pending
- **Details**: JS has `readBuffer(length, wrap=true, inflate=false)`; C++ replaces this with `readBuffer(...)` and `readBufferRaw(...)`.

### 12. [config.cpp] Deep config watcher auto-save path from Vue is not equivalent
- **JS Source**: `src/js/config.js` lines 60–61
- **Status**: Pending
- **Details**: JS attaches `core.view.$watch('config', () => save(), { deep: true })`; C++ relies on explicit manual `save()` calls from UI code.

### 13. [config.cpp] save scheduling differs from setImmediate event-loop behavior
- **JS Source**: `src/js/config.js` lines 83–90
- **Status**: Pending
- **Details**: JS defers with `setImmediate(doSave)` in the same event-loop model; C++ uses `std::async` thread execution.

### 14. [config.cpp] doSave write failure behavior differs
- **JS Source**: `src/js/config.js` lines 106–108
- **Status**: Pending
- **Details**: JS `await fsp.writeFile(...)` rejects on failure; C++ checks `file.is_open()` and can silently skip writing if open fails.

### 15. [config.cpp] EPERM detection logic differs from JS exception code check
- **JS Source**: `src/js/config.js` lines 43–49
- **Status**: Pending
- **Details**: JS checks `e.code === 'EPERM'`; C++ inspects exception message text for substrings like `EPERM`/`permission`.

### 16. [constants.cpp] DATA path root differs from JS nw.App.dataPath behavior
- **JS Source**: `src/js/constants.js` lines 16, 35–39
- **Status**: Pending
- **Details**: JS uses `nw.App.dataPath`; C++ uses `<install>/data`, changing where runtime/user files resolve.

### 17. [constants.cpp] Cache directory constant changed from casc/ to cache/
- **JS Source**: `src/js/constants.js` lines 73–81
- **Status**: Pending
- **Details**: JS cache namespace uses `DATA_PATH/casc/...`; C++ uses `data/cache/...` with migration logic.

### 18. [constants.cpp] Shader/default-config paths differ from JS layout
- **JS Source**: `src/js/constants.js` lines 43, 95–96
- **Status**: Pending
- **Details**: JS points to `INSTALL_PATH/src/shaders` and `INSTALL_PATH/src/default_config.jsonc`; C++ points to `<install>/data/...`.

### 19. [constants.h/constants.cpp] Version/flavour/build constants are compile-time values
- **JS Source**: `src/js/constants.js` line 46; `src/app.js` lines 44–47
- **Status**: Pending
- **Details**: JS reads these from `nw.App.manifest` at runtime; C++ hardcodes `VERSION`, `FLAVOUR`, and `BUILD_GUID`.

### 20. [constants.cpp] Updater helper extension mapping differs from JS
- **JS Source**: `src/js/constants.js` lines 18, 101
- **Status**: Pending
- **Details**: JS uses `UPDATER_EXT` mapping including `.app` for darwin; C++ only maps Windows/Linux helper naming.

### 21. [constants.cpp] Blender base-dir platform mapping omits JS darwin path
- **JS Source**: `src/js/constants.js` lines 24–32
- **Status**: Pending
- **Details**: JS handles `win32`, `darwin`, and `linux`; C++ implementation omits macOS branch.
