# TODO Tracker

> **Progress: 0/7 verified (0%)** — ✅ = Verified, ⬜ = Pending

- [ ] 1. [external-links.cpp] Missing STATIC_LINKS map and `::` prefix resolution in `open()`
  - **JS Source**: `src/js/external-links.js` lines 12–35
  - **Status**: Pending
  - **Details**: JS defines a `STATIC_LINKS` map with 5 entries (`::WEBSITE`, `::DISCORD`, `::PATREON`, `::GITHUB`, `::ISSUE_TRACKER` → URLs). The `open()` function checks if the link starts with `::` and resolves it from the map before opening. The C++ `open()` (external-links.cpp L23) passes the link directly to `ShellExecuteW`/`xdg-open` with no `::` prefix handling, so any call with a `::` prefix would attempt to open a non-URL string.

- [ ] 2. [app.cpp] Missing config auto-save watcher
  - **JS Source**: `src/js/config.js` line 60
  - **Status**: Pending
  - **Details**: JS sets up `core.view.$watch('config', () => save(), { deep: true })` in `config.load()` so that ANY config mutation automatically triggers persistence. The C++ version has no equivalent auto-save mechanism. `config::save()` is exported and called explicitly from `resetToDefault()` and `resetAllToDefault()`, but any other code path that modifies `core::view->config` without calling `config::save()` will lose changes on restart.

- [ ] 3. [buffer.cpp] `alloc()` and `setCapacity()` always zero buffer regardless of `secure` flag
  - **JS Source**: `src/js/buffer.js` lines 54–56 and 1021–1029
  - **Status**: Pending
  - **Details**: JS `alloc(length, secure=true)` uses `Buffer.allocUnsafe(length)` when `secure=false` (uninitialized memory, faster for large buffers) and `Buffer.alloc(length)` when `secure=true` (zeroed). The C++ version (buffer.cpp L377) always uses `std::vector<uint8_t>(length, 0)` regardless of the `secure` flag. Same issue in `setCapacity()` (buffer.cpp L1062). Performance-only impact since callers write into the buffer immediately, but it is a behavioral deviation.

- [ ] 4. [blob.cpp] `slice()` treats `end=0` differently from JS due to falsy-check semantics
  - **JS Source**: `src/js/blob.js` line 263
  - **Status**: Pending
  - **Details**: JS uses `end || this._buffer.length` which treats `end=0` as falsy and defaults to the full buffer length. C++ uses `std::optional<std::size_t>` — passing `0` produces an empty blob instead of a full-buffer slice. The JS behavior is a quirk of its falsy semantics (`0` is falsy), but the C++ must replicate it. Fix: treat `end == 0` the same as `end` being absent.

- [ ] 5. [buffer.cpp] `fromMmap()` copies data instead of zero-copy wrapping
  - **JS Source**: `src/js/buffer.js` lines 123–127
  - **Status**: Pending
  - **Details**: JS wraps the mmap data zero-copy via `Buffer.from(mmapObj.data)` and stores a reference to the mmap object to prevent GC. C++ (buffer.cpp L485) copies the data into a `std::vector<uint8_t>` with `std::vector<uint8_t>(src, src + size)`, defeating the purpose of memory mapping. For large game data files this could significantly increase memory usage and load time.

- [ ] 6. [core.cpp] `progressLoadingScreen()` does not await redraw like JS version
  - **JS Source**: `src/js/core.js` lines 442–450
  - **Status**: Pending
  - **Details**: JS `progressLoadingScreen` is async and calls `await generics.redraw()` which waits two animation frames, ensuring the loading progress text is visible on screen before the caller continues with the next work batch. C++ (core.cpp L316–327) posts the update to the main thread queue via `postToMainThread` and returns immediately. If the caller is on the main thread, multiple progress updates queue up and only render when the main loop resumes, so intermediate progress steps may never be visible.

- [ ] 7. [generics.cpp] `fileExists()` is more restrictive than JS equivalent
  - **JS Source**: `src/js/generics.js` lines 346–353
  - **Status**: Pending
  - **Details**: JS uses `fsp.access(file)` which checks only that the file exists (F_OK). C++ (generics.cpp L673–683) additionally opens the file with `std::ifstream` to verify it is readable. A file that exists but lacks read permission returns `true` in JS but `false` in C++. This could cause the C++ version to re-download cached files that exist but have unusual permissions.
