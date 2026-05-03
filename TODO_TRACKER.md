# TODO Tracker

> **Progress: 0/26 verified (0%)** — ✅ = Verified, ⬜ = Pending

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

- [ ] 8. [log.cpp] Log file opened in truncate mode instead of append mode
  - **JS Source**: `src/js/log.js` line 111
  - **Status**: Pending
  - **Details**: JS opens the runtime log with `fs.createWriteStream(constants.RUNTIME_LOG, { flags: 'a', encoding: 'utf8' })` — append mode. C++ (log.cpp `ensureStreamOpen()`) opens with default `std::ofstream` which is `std::ios::out | std::ios::trunc` — truncate mode. This means C++ wipes the log file on each launch while JS preserves entries from prior sessions.

- [ ] 9. [app.cpp] `setTaskbarProgress` hides progress bar at 100% instead of showing it full
  - **JS Source**: `src/js/app.js` lines 500–503
  - **Status**: Pending
  - **Details**: JS `win.setProgressBar(val)` shows a full progress bar when `val` is 1.0 and only hides it when `val < 0`. C++ (app.cpp `setTaskbarProgress`) uses `if (val < 0 || val >= 1.0)` to set `TBPF_NOPROGRESS`, which hides the taskbar progress at both negative values AND when progress reaches exactly 1.0 (100%). The bar briefly disappears at completion rather than showing full.

- [ ] 10. [modules.cpp] `wrap_module` initialize wrapper lacks JS `finally` semantics for `_tab_initializing` reset
  - **JS Source**: `src/js/modules.js` lines 227–244
  - **Status**: Pending
  - **Details**: JS uses `try { ... } catch { ... } finally { this._tab_initializing = false; }` which guarantees `_tab_initializing` is reset even if the catch block itself throws. C++ (modules.cpp L186–210) places `mod._tab_initializing = false;` after the try-catch block. If any function in the catch handlers throws (e.g. `go_to_landing()` → `set_active()` → `activated()` throwing), the statement is skipped and `_tab_initializing` stays `true` permanently, blocking any future initialization attempts for that module. Fix: use RAII scope guard or restructure to guarantee the reset.

- [ ] 11. [WMOShaderMapper.h] Inline deviation comment and missing DEVIATIONS.md entry for `MapObjParallax_PS` rename
  - **JS Source**: `src/js/3D/WMOShaderMapper.js` line 35
  - **Status**: Pending
  - **Details**: The C++ `WMOPixelShader` enum renames `MapObjParallax` to `MapObjParallax_PS` to avoid a name collision with `WMOVertexShader::MapObjParallax`. This is a legitimate deviation, but: (1) WMOShaderMapper.h lines 47-48 contain an inline comment explaining the rename, which violates the "No deviation comments in code" rule — documentation belongs in DEVIATIONS.md. (2) DEVIATIONS.md has no entry for this rename.

- [ ] 12. [CameraControlsGL.cpp] `on_mouse_down()` always returns `true` even when no button condition matches
  - **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 223–239
  - **Status**: Pending
  - **Details**: JS `on_mouse_down(event)` has no explicit return when no `if` branch is entered (e.g. an unrecognized button), effectively returning `undefined`. The C++ version (CameraControlsGL.cpp L237) unconditionally returns `true` at the end of the function, signaling the event was consumed even when no action was taken. Should return `false` in the fallthrough case.

- [ ] 13. [CharacterCameraControlsGL.cpp] `on_mouse_wheel()` returns `true` when `deltaY == 0`
  - **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 132–164
  - **Status**: Pending
  - **Details**: JS `on_mouse_wheel(e)` executes a bare `return;` (line 148) when `deltaY` is 0, effectively returning `undefined` (not consumed). The C++ version (CharacterCameraControlsGL.cpp L131) returns `true` for this case, incorrectly signaling the event was consumed when nothing happened.

- [ ] 14. [ADTExporter.cpp] Game object `scale == 0` produces 1.0 instead of 0.0
  - **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1270
  - **Status**: Pending
  - **Details**: JS uses `model.scale !== undefined ? model.scale / 1024 : 1` — any defined value including 0 yields `0/1024 = 0.0`. C++ (ADTExporter.cpp L1507) uses `model.scale != 0.0f ? model.scale / 1024.0f : 1.0f`, which treats 0 as undefined and defaults to 1.0. A game object with explicit scale=0 would export with wrong scale.

- [ ] 15. [ADTExporter.cpp] `useADTSets` hardcoded to `false` instead of checking bit 0x80
  - **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1301
  - **Status**: Pending
  - **Details**: JS computes `const useADTSets = model & 0x80;` to check if the WMO model entry has the ADT-doodad-set flag. C++ (ADTExporter.cpp L1560) hardcodes `const bool useADTSets = false;`, completely disabling ADT-level doodad set selection for WMO models. This affects which doodad sets are exported alongside WMO models on map tiles.

- [ ] 16. [WMOExporter.cpp] `exportGroupsAsSeparateOBJ` meta JSON adds extra shader fields not in JS
  - **JS Source**: `src/js/3D/exporters/WMOExporter.js` line 1198
  - **Status**: Pending
  - **Details**: JS `exportGroupsAsSeparateOBJ` writes `json.addProperty('materials', wmo.materials)` which serializes raw materials without `vertexShader`/`pixelShader` fields. The C++ version (WMOExporter.cpp L1532–1544) adds `vertexShader` and `pixelShader` to each material entry via the shader mapper. The shader mapper mutation at JS lines 708–709 only occurs in the `exportAsOBJ` path, which has not executed when `split_groups=true`. The C++ output for split-group meta JSON contains extra fields.

- [ ] 17. [ShaderProgram.cpp] `_compile` cleans up leaked shader on failure, deviating from JS
  - **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 29–55
  - **Status**: Pending
  - **Details**: When one shader compiles successfully and the other fails, JS (lines 35–36) simply returns without cleaning up the successfully compiled shader (a resource leak). C++ (ShaderProgram.cpp L30–36) deletes both shaders before returning. While the C++ behavior is better, it fixes a JS bug rather than reproducing it — per CLAUDE.md rules, JS behavior should be reproduced exactly unless impossible.

- [ ] 18. [ShaderProgram.cpp] `get_uniform_block_param` returns -1 instead of null-equivalent
  - **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 122–127
  - **Status**: Pending
  - **Details**: JS returns `null` when the uniform block index is `INVALID_INDEX`. C++ (ShaderProgram.cpp L115) returns `-1`. The sentinel value differs. In practice callers compare against expected values so this is unlikely to cause issues, but it is a behavioral difference.

- [ ] 19. [VertexArray.cpp] `dispose()` calls `unbind_vao` before deleting VAO, not present in JS
  - **JS Source**: `src/js/3D/gl/VertexArray.js` lines 312–334
  - **Status**: Pending
  - **Details**: C++ `dispose()` (VertexArray.cpp L317) calls `ctx_.unbind_vao(vao)` before deleting the VAO. The JS version directly calls `gl.deleteVertexArray(this.vao)` with no unbind step. This is extra behavior not present in the JS source — likely a defensive measure for desktop OpenGL, but a deviation.

- [ ] 20. [ADTExporter.cpp] Doodad CSV `RotationW` field writes `"0.000000"` instead of empty string for M2 models
  - **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1269
  - **Status**: Pending
  - **Details**: For M2 doodad models, `model.rotation` is a 3-element array (Euler angles). JS `model.Rotation?.[3] ?? model.rotation[3]` evaluates to `undefined` since neither `Rotation[3]` nor `rotation[3]` exists on a 3-element array. The JS CSVWriter converts `undefined` to an empty string `""`. C++ (ADTExporter.cpp L1496–1514) initializes `rotW = 0.0f` and writes `"0.000000"` to the CSV. The output differs: JS produces an empty field, C++ produces `"0.000000"`.

- [ ] 21. [WMOExporter.cpp] CSV float values use `std::to_string()` producing verbose 6-decimal notation instead of JS minimal notation
  - **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 611–619 and 1082–1091
  - **Status**: Pending
  - **Details**: In `exportAsOBJ` (WMOExporter.cpp L783–790) and `exportGroupsAsSeparateOBJ` (L1354–1361), doodad CSV position/rotation/scale float values are written via `std::to_string()`, which produces fixed-point notation with 6 decimal places (e.g. `"1.000000"`, `"0.000000"`). JS passes raw numbers to the CSV writer, which uses JavaScript's `Number.toString()` producing minimal notation (e.g. `"1"`, `"0"`). The WMOLegacyExporter C++ correctly uses `std::format("{:g}", ...)` to match JS. The retail WMOExporter should do the same for byte-identical CSV output.

- [ ] 22. [M2RendererGL.cpp] bones_ubo created before skeleton is loaded with bone_count=0
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 567, 730–733
  - **Status**: Pending
  - **Details**: JS calls `_create_bones_ubo()` from inside `loadSkin()` (line 567) after `_create_skeleton()` (line 491), so the UBO is allocated with the correct bone count and pre-filled with identity matrices. C++ (M2RendererGL.cpp L483) calls `renderer_utils::create_bones_ubo(*shader, ctx, bones_count())` in `load()` before `loadSkin()` is called. At this point `bones_count()` returns 0 because `_create_skeleton()` hasn't executed yet. The UBO itself has full capacity (based on shader block size), but 0 identity matrices are pre-filled. If any render occurs before the first bone matrix update, the UBO contains uninitialized data instead of identity matrices.

- [ ] 23. [M2RendererGL.cpp] Unsafe reinterpret_cast of SKELAttachment to M2Attachment in getAttachmentTransform
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1856–1858
  - **Status**: Pending
  - **Details**: JS `getAttachmentTransform` uses duck typing — `this.skelLoader.getAttachmentById()` returns an object with the same shape as M2 attachments (bone, position). C++ (M2RendererGL.cpp L2087–2088) uses `reinterpret_cast<const M2Attachment*>(skel_att)` to convert a `SKELAttachment*` to `M2Attachment*`. This is undefined behavior if the struct layouts differ. Should use a proper conversion or a common interface/struct.

- [ ] 24. [M3RendererGL.cpp] Double dispose of bones_ubo in dispose()
  - **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 307–329
  - **Status**: Pending
  - **Details**: C++ `dispose()` (M3RendererGL.cpp L323–335) calls `_dispose_geometry()` which disposes `bones_ubo` at lines 310–313, then `dispose()` itself disposes `bones_ubo` again at lines 326–329. The null guard prevents a crash, but this is redundant code not present in JS. JS `dispose()` only calls `_dispose_geometry()` (which handles ubos) then handles `default_texture`. The second dispose in C++ should be removed.

- [ ] 25. [JSONWriter.cpp] Missing BigInt serialization handling in write()
  - **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 40–43
  - **Status**: Pending
  - **Details**: JS `write()` uses `JSON.stringify` with a custom replacer that converts `BigInt` values to strings: `typeof value === 'bigint' ? value.toString() : value`. C++ (JSONWriter.cpp L24) uses `data.dump(1, '\t')` with no equivalent handling. nlohmann::json stores large integers as `int64_t`/`uint64_t` and serializes them as numbers. If any caller stores values that were `BigInt` in JS, the C++ output would contain numeric integers where JS output contained quoted strings. The JS code explicitly handles this case, suggesting BigInt values do occur in practice.

- [ ] 26. [M3RendererGL.cpp] bones_ubo created in load() but destroyed by loadLOD() and never recreated
  - **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 59–71, 79–81, 147
  - **Status**: Pending
  - **Details**: JS calls `_create_bones_ubo()` inside `loadLOD()` at line 147, after `_dispose_geometry()` at line 84, so the UBO is always valid after loading. C++ creates `bones_ubo` in `load()` at line 45, then calls `loadLOD(0)` at line 50. Inside `loadLOD()`, `_dispose_geometry()` at line 64 destroys the bones_ubo (lines 310–313) and never recreates it. After `load()` completes, `bones_ubo.ubo` is null. In `render()`, the `if (bones_ubo.ubo)` check at line 229 fails, so the identity bone matrix is never uploaded or bound. The shader receives `u_bone_count = 1` but has no UBO data, causing undefined rendering for all M3 models. Fix: move the `bones_ubo` creation from `load()` into `loadLOD()` after `_dispose_geometry()`, matching JS placement.
