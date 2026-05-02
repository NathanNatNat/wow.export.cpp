# TODO Tracker

> **Progress: 0/9 verified (0%)** — ✅ = Verified, ⬜ = Pending

- [ ] 1. [config.cpp] Missing automatic config save watcher — config changes outside settings screen are not persisted
  - **JS Source**: `src/js/config.js` lines 59–60
  - **Status**: Pending
  - **Details**: The JS version sets up `core.view.$watch('config', () => save(), { deep: true })` at the end of `load()`, so any change to any config property automatically triggers a debounced save to disk. The C++ version has no equivalent watcher — `config::save()` is only called explicitly from `screen_settings.cpp` (line 712). This means config changes made elsewhere (e.g. export directory default in app.cpp, CDN region selection via `setSelectedCDN`, or any module that mutates `core::view->config`) will not be persisted to disk between sessions.
- [ ] 2. [core.h] `chrPendingEquipSlot` default value should be optional, not int 0
  - **JS Source**: `src/js/core.js` line 268
  - **Status**: Pending
  - **Details**: The JS initializes `chrPendingEquipSlot` to `null` (meaning "no slot pending"). The C++ version uses `int chrPendingEquipSlot = 0` (core.h:334). Since WoW equipment slot 0 is the Head slot, the C++ code cannot distinguish "no pending slot" from "head slot pending". Should use `std::optional<int>` to match JS null-sentinel behavior, consistent with the adjacent `chrItemPickerSlot` field which already uses `std::optional<int>`.
- [ ] 3. [M2LegacyRendererGL.cpp] Missing bones UBO infrastructure — no _create_bones_ubo, no UBO bind in render
  - **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 474–477, 1020
  - **Status**: Pending
  - **Details**: The JS creates a bone UBO via `create_bones_ubo()` (line 474) and binds it each frame with `this.ubos[0].ubo.bind(0)` (line 1020) before draw calls. The C++ has no `_create_bones_ubo` method, no UBO member, and no UBO binding in `render()`. While the shader currently disables bone skinning (`u_bone_count = 0`), the UBO infrastructure is needed for future animation support. The JS creates and binds the UBO even when bone count is 0.
- [ ] 4. [M2RendererGL.cpp] _create_bones_ubo declared in header but not implemented in .cpp
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 730–733
  - **Status**: Pending
  - **Details**: The header (M2RendererGL.h line 120) declares `_create_bones_ubo()` but no implementation exists in the .cpp file. The UBO is instead created directly in `load()` at line 483. The dangling declaration should either be removed or implemented. Would cause a linker error if any code called the method.
- [ ] 5. [M2RendererGL.cpp] _dispose_skin does not dispose UBOs — potential GPU resource leak on skin reload
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1914–1919
  - **Status**: Pending
  - **Details**: JS `_dispose_skin()` explicitly disposes UBOs (`for (const ubo of this.ubos) ubo.ubo.dispose()`) allowing clean skin reloads. C++ `_dispose_skin()` (lines 2146–2161) does not dispose the bones UBO — it is only disposed in `dispose()`. If `loadSkin` is called again (e.g. switching skins), the old UBO resources would leak.
- [ ] 6. [M3RendererGL.cpp] getBoundingBox missing empty vertices check
  - **JS Source**: `src/js/3D/renderers/M3RendererGL.js` line 195
  - **Status**: Pending
  - **Details**: JS checks `!this.m3 || !this.m3.vertices` and returns null. C++ (line 183) only checks `!m3` but not whether vertices are empty. If `m3` exists but has no vertices, JS returns null but C++ iterates over an empty vector and returns `{infinity, -infinity}` bounding box instead of `std::nullopt`.
- [ ] 7. [M3RendererGL.cpp] _dispose_geometry does not dispose UBOs — potential GPU resource leak on loadLOD
  - **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 307–320
  - **Status**: Pending
  - **Details**: JS `_dispose_geometry()` disposes UBOs (line 311: `for (const ubo of this.ubos) ubo.ubo.dispose()`). C++ `_dispose_geometry()` (lines 306–316) does not dispose the bones UBO. If `loadLOD` is called multiple times, old UBO resources would leak. The UBO is only disposed in `dispose()`.
- [ ] 8. [MDXRendererGL.cpp] Bone UBO created with bone_count=0 before skeleton initialization
  - **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 262–265, 291
  - **Status**: Pending
  - **Details**: JS calls `_create_bones_ubo()` inside `_build_geometry()` (line 291), after `_create_skeleton()` has populated `this.nodes`. This gives the correct `bone_count = this.nodes.length`. C++ calls `create_bones_ubo()` in `load()` (line 190) *before* `_create_skeleton()` (line 194), always passing `bone_count=0`. The UBO is thus not initialized with the correct number of identity matrices for the actual bone count.
- [ ] 9. [GLTFWriter.cpp] Animation channel node target index differs from JS in non-prefix bone mode — needs DEVIATIONS.md entry
  - **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 621, 761, 891
  - **Status**: Pending
  - **Details**: JS animation channels always use `nodeIndex + 1` as the target node, regardless of prefix mode. When `modelsExportWithBonePrefix` is true, `nodeIndex + 1` correctly points to the bone node (prefix is at nodeIndex). When false, `nodeIndex + 1` points one past the bone node — this appears to be a JS bug. C++ (lines 659, 781, 901) uses `actual_node_idx` which correctly targets the bone node in both cases. This is a deliberate deviation that fixes a likely JS bug and should be documented in DEVIATIONS.md.
