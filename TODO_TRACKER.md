# TODO Tracker

> **Progress: 0/2 verified (0%)** — ✅ = Verified, ⬜ = Pending

- [ ] 1. [config.cpp] Missing automatic config save watcher — config changes outside settings screen are not persisted
  - **JS Source**: `src/js/config.js` lines 59–60
  - **Status**: Pending
  - **Details**: The JS version sets up `core.view.$watch('config', () => save(), { deep: true })` at the end of `load()`, so any change to any config property automatically triggers a debounced save to disk. The C++ version has no equivalent watcher — `config::save()` is only called explicitly from `screen_settings.cpp` (line 712). This means config changes made elsewhere (e.g. export directory default in app.cpp, CDN region selection via `setSelectedCDN`, or any module that mutates `core::view->config`) will not be persisted to disk between sessions.
- [ ] 2. [core.h] `chrPendingEquipSlot` default value should be optional, not int 0
  - **JS Source**: `src/js/core.js` line 268
  - **Status**: Pending
  - **Details**: The JS initializes `chrPendingEquipSlot` to `null` (meaning "no slot pending"). The C++ version uses `int chrPendingEquipSlot = 0` (core.h:334). Since WoW equipment slot 0 is the Head slot, the C++ code cannot distinguish "no pending slot" from "head slot pending". Should use `std::optional<int>` to match JS null-sentinel behavior, consistent with the adjacent `chrItemPickerSlot` field which already uses `std::optional<int>`.
