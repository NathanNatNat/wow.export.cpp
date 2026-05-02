# TODO Tracker

> **Progress: 0/1 verified (0%)** — ✅ = Verified, ⬜ = Pending

- [ ] 1. [config.cpp] Missing automatic config save watcher — config changes outside settings screen are not persisted
  - **JS Source**: `src/js/config.js` lines 59–60
  - **Status**: Pending
  - **Details**: The JS version sets up `core.view.$watch('config', () => save(), { deep: true })` at the end of `load()`, so any change to any config property automatically triggers a debounced save to disk. The C++ version has no equivalent watcher — `config::save()` is only called explicitly from `screen_settings.cpp` (line 712). This means config changes made elsewhere (e.g. export directory default in app.cpp, CDN region selection via `setSelectedCDN`, or any module that mutates `core::view->config`) will not be persisted to disk between sessions.
