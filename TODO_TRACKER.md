# TODO Tracker

> **Progress: 0/3 verified (0%)** — ✅ = Verified, ⬜ = Pending

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
