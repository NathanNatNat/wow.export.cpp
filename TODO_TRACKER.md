# TODO Tracker

> **Progress: 0/66 verified (0%)** — ✅ = Verified, ⬜ = Pending

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

- [ ] 27. [M2RendererGL.cpp] `applyExternalBoneMatrices` bounds check compares float-offset against bone-count
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js` line 1345
  - **Status**: Pending
  - **Details**: JS checks `if (char_offset + 16 <= char_bone_matrices.length)` where `char_offset = char_idx * 16` (float index) and `.length` is the total float count (e.g., 1600 for 100 bones). C++ (M2RendererGL.cpp L1554) checks `if (char_offset + 16 <= matrix_count)` where `matrix_count` is passed as `vector.size() / 16` (bone count, e.g., 100) from call sites at model-viewer-gl.cpp L507/L539 and CharacterExporter.cpp L313. This compares a float-offset (e.g., 1584 for bone 99) against a bone count (100), so `1584 + 16 <= 100` is always false for any bone index > ~5. All external bone matrix copies beyond the first few bones silently fail, breaking collection model rigging. Fix: callers should pass `char_bone_matrices.size()` (float count) instead of `char_bone_matrices.size() / 16`.

- [ ] 28. [M2RendererGL.cpp] `submesh_colors` initialized to 1.0 instead of 0.0
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js` line 423
  - **Status**: Pending
  - **Details**: JS initializes `this.submesh_colors = new Float32Array(this.m2.colors.length * 4)` which zero-fills. C++ (M2RendererGL.cpp L495) uses `submesh_colors.assign(m2->colors.size() * 4, 1.0f)` which fills with 1.0. Before any animation plays, submeshes with a valid `color_idx` will have RGBA=(0,0,0,0) in JS (invisible, skipped by `alpha <= 0` check) but RGBA=(1,1,1,1) in C++ (fully visible). This is a visual difference in the initial render state before `playAnimation` or `stopAnimation` is called. Fix: change to `submesh_colors.assign(m2->colors.size() * 4, 0.0f)`.

- [ ] 29. [file-writer.cpp] FileWriter opens in text mode, producing `\r\n` line endings on Windows
  - **JS Source**: `src/js/file-writer.js` line 38
  - **Status**: Pending
  - **Details**: C++ `FileWriter` (file-writer.cpp L12) opens with `std::ios::out | std::ios::trunc` (no `std::ios::binary`). On Windows, `std::ofstream` in text mode translates every `\n` to `\r\n`. JS `fs.createWriteStream` does NOT do line-ending translation — `line + '\n'` always produces a literal `\n` (0x0A) on all platforms. This affects all files written via FileWriter: CSV (CSVWriter), SQL (SQLWriter), JSON (JSONWriter), OBJ (OBJWriter), and MTL (MTLWriter). Every exported file from these writers will have `\r\n` on Windows in C++ vs `\n` in JS. Fix: add `std::ios::binary` to the open flags at file-writer.cpp L12 and L18.

- [ ] 30. [blp.cpp] `_getAlpha` case 4 (alphaDepth=4) uses integer division instead of reproducing JS float-division bug
  - **JS Source**: `src/js/casc/blp.js` line 294
  - **Status**: Pending
  - **Details**: JS `_getAlpha` case 4 reads `this.rawData[this.scaledLength + (index / 2)]`. In JS, `index / 2` for odd indices produces a float (e.g. `3/2 = 1.5`). Accessing a JS Array with a float index returns `undefined`, and `undefined & 0xF0` evaluates to `0`. So for odd-indexed pixels with alphaDepth=4, JS returns alpha=0. C++ (blp.cpp L224) uses `rawData_[scaledLength_ + (index / 2)]` where integer division truncates (e.g. `3/2 = 1`), reading the correct byte and returning the actual high-nibble alpha value. The JS code has a bug (should use `Math.floor(index / 2)` like case 1 does), but per CLAUDE.md the C++ must reproduce the JS behavior. For odd pixels with alphaDepth=4, JS produces alpha=0 while C++ produces the correct alpha. Fix: for odd indices, return 0 to match JS, or use a float division that truncates the array index to reproduce the `undefined` access.

- [ ] 31. [item-picker-modal.cpp] Missing item database initialization when items are not yet loaded
  - **JS Source**: `src/js/components/item-picker-modal.js` lines 107–119
  - **Status**: Pending
  - **Details**: JS `slot_id` watch handler checks `this.all_items.length === 0` and if true, calls `await DBItemList.initialize()` to trigger asynchronous loading of the item database, setting `is_loading`/`load_error`/`items_loaded` appropriately. C++ `render()` (item-picker-modal.cpp L183–192) checks `tab_items::getAllItems()` and sets `s_is_loading = true` if null, but never triggers any initialization or loading. If the item database hasn't been loaded elsewhere before the modal opens, C++ displays "Loading items..." indefinitely while JS would actively load the data. Fix: call the item database initialization (equivalent to `DBItemList.initialize()`) from `open()` or `render()` when items are not yet available.

- [ ] 32. [tact-keys.cpp] Cache iteration aborts on first non-string JSON value instead of skipping individually
  - **JS Source**: `src/js/casc/tact-keys.js` lines 73–80
  - **Status**: Pending
  - **Details**: JS iterates `Object.entries(tactKeys)` and calls `validateKeyPair(keyName, key)` for each entry. If `key` is a non-string (e.g. a number), `key.length` is `undefined`, validation fails, the entry is skipped with a log message, and iteration continues to the next entry. C++ (tact-keys.cpp L204) calls `it.value().get<std::string>()` which throws `nlohmann::json::type_error` for non-string values. The exception is caught by the outer `catch (...)` at L217, aborting the entire cache loading loop. Keys processed before the non-string entry are kept, but all subsequent entries (even valid ones) are never loaded. Fix: wrap the individual `get<std::string>()` call in a try-catch within the loop body and continue on failure, matching JS per-entry skip behavior.

- [ ] 33. [itemlistbox.cpp] `handleKey` arrow-down with filtered-out `lastSelectItem` does nothing instead of selecting first item
  - **JS Source**: `src/js/components/itemlistbox.js` lines 237–240
  - **Status**: Pending
  - **Details**: When a previously selected item is removed by filtering, JS `this.filteredItems.indexOf(this.lastSelectItem)` returns -1. For arrow-down, `nextIndex = -1 + 1 = 0`, and `this.filteredItems[0]` returns the first visible item, which is then selected. C++ (itemlistbox.cpp L269–271) has an early-return guard `if (lastSelectIndex < 0) return;` that prevents any action when the last-selected item is no longer in the filtered list. This means arrow-down does nothing in C++ while JS selects the first item. The JS behavior is a side effect of `indexOf` returning -1, but per CLAUDE.md rules the C++ must reproduce it. Fix: remove the `if (lastSelectIndex < 0) return;` guard, or special-case arrow-down when lastSelectIndex is -1 to select index 0.

- [ ] 34. [WDCReader.cpp] `getRelationRows` has extra preload precondition not present in JS
  - **JS Source**: `src/js/db/WDCReader.js` lines 216–234
  - **Status**: Pending
  - **Details**: JS `getRelationRows` only checks `this.isLoaded` before reading records via `this._readRecord(recordID)`. C++ (WDCReader.cpp L305–306) adds an extra guard `if (!rows.has_value()) throw "Table must be preloaded..."` that the JS does not have. JS callers can call `getRelationRows` without first calling `preload()` — the function will lazily read records. C++ callers that skip `preload()` will get an exception. Fix: remove the `rows.has_value()` check and read records via `_readRecord` like JS does.

- [ ] 35. [map-viewer.cpp] `render()` missing `this.map === null` early return guard
  - **JS Source**: `src/js/components/map-viewer.js` lines 490–491
  - **Status**: Pending
  - **Details**: JS `render()` starts with `if (this.map === null) return;` to skip all rendering when no map is selected. C++ `render()` (map-viewer.cpp L394–438) has no equivalent guard. While `renderWidget()` gates `renderTiles`/`renderOverlay` behind `if (mapId != -1)` at L1200, internal callers like `setToDefaultPosition`, `handleMouseMove`, and `setMapPosition` call `render()` directly without the guard, allowing tile queueing/loading to proceed with no active map. Fix: add `if (mapId == -1) return;` or equivalent at the start of `render()`.

- [ ] 36. [menu-button.cpp] Dropdown re-click does not toggle menu closed like JS
  - **JS Source**: `src/js/components/menu-button.js` lines 38–39
  - **Status**: Pending
  - **Details**: JS `openMenu` uses `this.open = !this.open && !this.disabled` — clicking the dropdown button while the menu is already open sets `open = false` (since `!this.open` is `false`), closing the menu. C++ (menu-button.cpp L119–121) checks `if (!disabled && !popupOpen) ImGui::OpenPopup(...)` — when the popup IS already open, it does nothing (no open call), but when the user clicks the button, ImGui closes the popup (click outside) and then immediately reopens it via the button click handler. The net effect is the menu stays open instead of toggling closed. Fix: add a toggle check — if popup is already open, call `ImGui::CloseCurrentPopup()` instead of opening.

- [ ] 37. [model-viewer-gl.cpp] `chrUse3DCamera` config fallback default is `true` but should be `false`
  - **JS Source**: `src/js/components/model-viewer-gl.js` line 375; `src/default_config.jsonc` line 81
  - **Status**: Pending
  - **Details**: JS accesses `core.view.config.chrUse3DCamera` directly — when the key is missing, this returns `undefined` (falsy). `default_config.jsonc` explicitly sets it to `false`. C++ (model-viewer-gl.cpp L565, L625, L627) uses `core::view->config.value("chrUse3DCamera", true)`, providing `true` as the fallback. If the config JSON doesn't contain the key, C++ enables 3D camera by default while JS does not. Fix: change the fallback from `true` to `false` in all three locations.

- [ ] 38. [model-viewer-gl.cpp] `chrRenderShadow` config fallback default is `false` but should be `true`
  - **JS Source**: `src/js/components/model-viewer-gl.js` line 403; `src/default_config.jsonc` line 80
  - **Status**: Pending
  - **Details**: `default_config.jsonc` sets `chrRenderShadow` to `true`. C++ (model-viewer-gl.cpp L612, L631, L633) uses `core::view->config.value("chrRenderShadow", false)`, providing `false` as the fallback. If the config key is missing, C++ hides the character shadow by default while JS shows it. Fix: change the fallback from `false` to `true` in all three locations.

- [ ] 39. [model-viewer-gl.cpp] Background color config fallbacks use `#343a40` instead of `#000000`
  - **JS Source**: `src/js/components/model-viewer-gl.js` lines 255–256; `src/default_config.jsonc` lines 64, 83
  - **Status**: Pending
  - **Details**: `default_config.jsonc` sets both `modelViewerBackgroundColor` and `chrBackgroundColor` to `"#000000"`. C++ (model-viewer-gl.cpp L418–419) uses `"#343a40"` as the fallback for both keys. If the config JSON is missing these keys, C++ uses a dark grey background instead of black. Fix: change both fallbacks from `"#343a40"` to `"#000000"`.

- [ ] 40. [legacy_tab_textures.cpp] `export_textures` passes `isLocal=true` instead of `false`
  - **JS Source**: `src/js/modules/legacy_tab_textures.js` line 170
  - **Status**: Pending
  - **Details**: JS calls `textureExporter.exportFiles(selected, false, -1, true)` — `isLocal=false`, `exportID=-1`, `isMPQ=true`. C++ (legacy_tab_textures.cpp L417) calls `texture_exporter::exportFiles(selected, nullptr, core::view->mpq.get(), true, -1)` — `isLocal=true`, `exportID=-1`. The `true` for `isLocal` causes three problems: (1) line 209 forces `overwriteFiles=true` regardless of user config, (2) line 231 skips `exportNamedFiles` renaming, (3) line 241 uses the raw MPQ filename as the export path without prepending the configured export directory. Fix: change `true` to `false` at legacy_tab_textures.cpp L417.

- [ ] 41. [screen_source_select.cpp] CDN auto-selection for fastest region never works
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 219–249
  - **Status**: Pending
  - **Details**: JS sets `selectedCDNRegion` to the same `node` object reference that lives in the `regions` array. When pings complete and `node.delay = ms` is executed, it mutates the same object. By `Promise.all` time, `selected_region.delay` has a numeric value, so the fastest-region comparison works. C++ (screen_source_select.cpp L530) copies the JSON node into `selectedCDNRegion`. When pings update `regions[index]["delay"]` at L557, `selectedCDNRegion["delay"]` remains `null`. At L584, `selected_region["delay"].is_number()` fails because `delay` is null, so no region ever replaces the default. The "auto-select fastest CDN" feature is completely broken. Fix: when the initial `selected_region.delay` is null, treat any valid region with positive delay as better; or update `selectedCDNRegion["delay"]` when the corresponding region's ping completes.

- [ ] 42. [screen_source_select.cpp] Missing "Visit Support Discord" action in CASC load error toasts
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 134–137
  - **Status**: Pending
  - **Details**: JS CASC load error toast has two actions: `'View Log'` and `'Visit Support Discord': () => ExternalLinks.open('::DISCORD')`. C++ (screen_source_select.cpp L314–317, L338–341) only has `'View Log'` in both the local and remote CASC load error paths. The "Visit Support Discord" button is missing from both. Fix: add `{"Visit Support Discord", []() { external_links::open("::DISCORD"); }}` to both toast action lists.

- [ ] 43. [screen_settings.cpp] JS typo "manfiest" silently corrected to "manifest"
  - **JS Source**: `src/js/modules/screen_settings.js` line 445
  - **Status**: Pending
  - **Details**: JS has `'A valid URL is required for DBD manfiest.'` (typo: "manfiest"). C++ (screen_settings.cpp L705) has `"A valid URL is required for DBD manifest."` (corrected: "manifest"). Per CLAUDE.md rules ("Do not 'fix' perceived JS bugs"), the C++ should reproduce the exact typo. Fix: change "manifest" back to "manfiest" at screen_settings.cpp L705.

- [ ] 44. [legacy_tab_fonts.cpp] `get_font_id` has undefined behavior when hash equals `0x80000000`
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 12–15
  - **Status**: Pending
  - **Details**: JS `get_font_id` computes `hash = ((hash << 5) - hash + charCode) | 0` (signed 32-bit coercion) then returns `'font_legacy_' + Math.abs(hash)`. `Math.abs(-2147483648)` safely returns `2147483648` (float64). C++ (legacy_tab_fonts.cpp L38) does `std::to_string(std::abs(static_cast<int32_t>(hash)))`. If `hash` is exactly `0x80000000`, `static_cast<int32_t>` gives `INT32_MIN` and `std::abs(INT32_MIN)` is undefined behavior in C++ (positive value overflows int32_t). On most implementations it returns `-2147483648`, producing `"font_legacy_-2147483648"` instead of JS's `"font_legacy_2147483648"`. Fix: use `static_cast<int64_t>(static_cast<int32_t>(hash))` before `std::abs`, or special-case `0x80000000`.

- [ ] 45. [screen_settings.cpp] "Add Encryption Key" input buffers cleared unconditionally after handle_tact_key()
  - **JS Source**: `src/js/modules/screen_settings.js` lines 295–297, 401–405
  - **Status**: Pending
  - **Details**: JS `handle_tact_key()` does not clear the `userInputTactKeyName` or `userInputTactKey` view properties after calling `tactKeys.addKey()`. On success or failure, the text inputs retain their values, allowing the user to correct and retry on failure. C++ (screen_settings.cpp L514–515) unconditionally sets `key_name_buf[0] = '\0'` and `key_val_buf[0] = '\0'` after `handle_tact_key()`, clearing both fields regardless of whether the key was successfully added. On failure, the user loses their input and must re-enter both the key name and key value. Fix: only clear the buffers when `addKey` returns true (success), not on failure.

- [ ] 46. [screen_source_select.cpp] Folder dialog titles are generic instead of matching JS descriptive titles
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 329–336
  - **Status**: Pending
  - **Details**: JS sets descriptive directory dialog titles: "Select World of Warcraft Installation" for local CASC (line 330) and "Select Legacy MPQ Installation" for legacy (line 336). C++ (screen_source_select.cpp L599, L668) uses the generic title "Select Directory" for both `pfd::select_folder()` calls. Fix: change local to `pfd::select_folder("Select World of Warcraft Installation")` and legacy to `pfd::select_folder("Select Legacy MPQ Installation")`.

- [ ] 47. [legacy_tab_data.cpp] DBC listbox missing `unittype` and status bar
  - **JS Source**: `src/js/modules/legacy_tab_data.js` line 131
  - **Status**: Pending
  - **Details**: JS DBC listbox sets `unittype="dbc file"` which enables the status bar text showing the number of DBC files found. C++ (legacy_tab_data.cpp L267) passes `""` for the unittype parameter, suppressing the status text. Additionally, unlike every other legacy tab (audio, files, fonts, textures) which all have an explicit `app::layout::BeginStatusBar` / `listbox::renderStatusBar` block, `legacy_tab_data` has none for the DBC listbox (between EndListContainer at L282 and BeginFilterBar at L284). Fix: change unittype from `""` to `"dbc file"` at L267, and add a status bar block between L282 and L284.

- [ ] 48. [legacy_tab_fonts.cpp] `persistscrollkey` is `"legacy-fonts"` instead of `"fonts"`
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 63
  - **Status**: Pending
  - **Details**: JS Listbox sets `persistscrollkey="fonts"` for the font file list. C++ (legacy_tab_fonts.cpp L169) passes `"legacy-fonts"` as the persist scroll key parameter. This means scroll position is stored under a different key than what JS uses, so scroll position would not carry over if persistence is ever shared between the two implementations, and it's a deviation from the JS source. Fix: change `"legacy-fonts"` to `"fonts"` at legacy_tab_fonts.cpp L169.

- [ ] 49. [screen_source_select.cpp] CDN auto-selection loop finds absolute fastest instead of last-below-default
  - **JS Source**: `src/js/modules/screen_source_select.js` lines 239–248
  - **Status**: Pending
  - **Details**: In the CDN fastest-region selection loop (after all pings), JS sets `let selected_region = this.$core.view.selectedCDNRegion` which is a reference to the original default region object. The loop compares each `region.delay < selected_region.delay` but does NOT update `selected_region` — so every region is compared against the original default's delay. If US=100ms, EU=50ms, KR=75ms: EU (50<100) sets selectedCDNRegion=EU, then KR (75<100) overwrites to KR. The last region below the default wins. C++ (screen_source_select.cpp L588) adds `selected_region = region` which updates the local tracker, so it finds the absolute lowest delay (EU=50ms wins). While C++ behavior is arguably better, it deviates from JS. Note: this is currently unreachable due to TODO #41 (delay stays null), but would matter once #41 is fixed. Fix: remove line 588 (`selected_region = region;`) to match JS behavior.

- [ ] 50. [legacy_tab_audio.cpp] Filter bar "Regex Enabled" missing tooltip and SameLine
  - **JS Source**: `src/js/modules/legacy_tab_audio.js` line 212
  - **Status**: Pending
  - **Details**: JS filter bar `<div class="regex-info" v-if="..." :title="$core.view.regexTooltip">Regex Enabled</div>` has a tooltip on hover (`:title`) and is displayed inline with the filter input. C++ (legacy_tab_audio.cpp L388–389) only has `ImGui::TextUnformatted("Regex Enabled")` with no tooltip and no `ImGui::SameLine()`. All three sibling legacy tabs (files L187–192, fonts L215–220, data L285–290) correctly include both `ImGui::SetTooltip` on hover and `ImGui::SameLine()`. Fix: add `if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", view.regexTooltip.c_str()); ImGui::SameLine();` after the TextUnformatted call.

- [ ] 51. [legacy_tab_audio.cpp] `format_time()` uses truncation instead of JS `Math.round` for seconds
  - **JS Source**: `src/js/generics.js` line 484
  - **Status**: Pending
  - **Details**: JS `formatPlaybackSeconds` (generics.js L484) computes seconds as `Math.round(seconds % 60)` which rounds to the nearest integer. C++ (legacy_tab_audio.cpp L243) uses `static_cast<int>(seconds)` which truncates (floors). For 59.7 seconds, JS produces `":60"` while C++ produces `":59"`. The C++ codebase already has a shared `generics::formatPlaybackSeconds()` (generics.cpp L809) that correctly uses `std::round`, but the legacy audio tab uses a local `format_time()` function with truncation instead. Fix: replace the local `format_time()` with calls to `generics::formatPlaybackSeconds()`.

- [ ] 52. [legacy_tab_data.cpp] MenuButton missing `upward=true` parameter
  - **JS Source**: `src/js/modules/legacy_tab_data.js` line 164
  - **Status**: Pending
  - **Details**: JS MenuButton has `class="upward"` which makes the dropdown open upward (important since it's in the bottom tray). C++ (legacy_tab_data.cpp L421–425) calls the `menu_button::render` overload without the `upward` parameter, defaulting to `false` (downward). The full overload with explicit `upward` is available (menu-button.h L55–60). Fix: use the full render overload and pass `true` for the `upward` parameter.

- [ ] 53. [legacy_tab_data.cpp] `load_table` catch block missing stack trace log line
  - **JS Source**: `src/js/modules/legacy_tab_data.js` lines 112–113
  - **Status**: Pending
  - **Details**: JS `load_table` catch block has two log calls: `log.write('Failed to open DBC file: %s', e.message)` and `log.write('%o', e.stack)`. C++ (legacy_tab_data.cpp L207) only logs the error message, omitting the stack trace entirely. While C++ exceptions lack a `.stack` property, the second log line should still be present — either logging a placeholder or using platform-specific stack capture. Fix: add a second `logging::write` call after L207 to log the exception context, matching the JS two-line pattern.

- [ ] 54. [legacy_tab_textures.cpp] Listbox and status bar add quickfilters not present in JS
  - **JS Source**: `src/js/modules/legacy_tab_textures.js` line 117
  - **Status**: Pending
  - **Details**: JS Listbox for the legacy textures tab has no quickfilters prop — no quick filter links appear in the status bar. C++ (legacy_tab_textures.cpp L269) passes `{".blp", ".png", ".jpg"}` as quickfilters to `listbox::render()`, and L309 passes the same to `listbox::renderStatusBar()`. This adds "Quick filter: BLP / PNG / JPG" links to the status bar that are absent from the original JS application. Fix: change both to empty vectors `{}` to match JS.

- [ ] 55. [tab_characters.cpp] Missing DBItemList initialization — item picker may be empty
  - **JS Source**: `src/js/modules/tab_characters.js` line 2759
  - **Status**: Pending
  - **Details**: JS calls `await DBItemList.initialize(...)` in the characters tab's `initialize()` method, ensuring the full item list is available for the item picker modal. The C++ `mounted()` (tab_characters.cpp ~L3967–3994) initializes DBItems, DBItemCharTextures, DBItemGeosets, DBItemModels, DBModelFileData, and DBGuildTabard but never initializes the item list data that `tab_items::getAllItems()` returns. The item picker modal calls `tab_items::getAllItems()` (item-picker-modal.cpp L183) which returns nullptr if the Items tab hasn't been visited yet. Fix: call the equivalent of `tab_items` item list initialization during characters tab setup.

- [ ] 56. [tab_characters.cpp] Left-click on equipped slot does not open context menu
  - **JS Source**: `src/js/modules/tab_characters.js` lines 2489–2497
  - **Status**: Pending
  - **Details**: JS `open_slot_context()` is bound to both `@click` and `@contextmenu.prevent` on equipment slots. For equipped items (item_id truthy), it sets `chrEquipmentSlotContext = slot_id` which opens a context menu. Both left-click and right-click trigger this. C++ (tab_characters.cpp L3869–3877) only opens the context menu popup on right-click (`ImGuiMouseButton_Right`). Left-click on an equipped slot does nothing. Left-click on an empty slot correctly opens the item picker. Fix: add `|| (slot_clicked_left && item)` to the context menu open condition.

- [ ] 57. [tab_maps.cpp] `collect_game_objects()` adds value-based deduplication not in JS
  - **JS Source**: `src/js/modules/tab_maps.js` lines 125–156
  - **Status**: Pending
  - **Details**: JS uses a `Set` which deduplicates by object identity (reference equality). Since each row from `getAllRows()` is a unique object reference, no actual value-based deduplication occurs — all matching objects are included. C++ (tab_maps.cpp L373–416) adds a `GameObjectKey { FileDataID, x, y }` with an `unordered_set` to skip objects sharing the same FileDataID and position. This could cause the C++ version to export fewer game objects than the JS. Fix: remove the `GameObjectKey`/`seen` set and add all matching objects directly to the result vector.

- [ ] 58. [tab_text.cpp] BusyLock during text preview not present in JS
  - **JS Source**: `src/js/modules/tab_text.js` lines 124–143
  - **Status**: Pending
  - **Details**: JS `mounted()` watcher checks `!this.$core.view.isBusy` before starting a preview load, but does NOT increment `isBusy` itself. C++ `preview_text()` (tab_text.cpp L59) creates a `BusyLock` via `core::create_busy_lock()` stored in `PendingTextPreview.busy_lock`, which increments `isBusy` until the preview completes. This prevents the user from clicking "Export Selected" while a text file preview is loading — behavior the JS allows. Fix: remove the `busy_lock` member from `PendingTextPreview` and the `create_busy_lock()` call at L59.

- [ ] 59. [tab_audio.cpp] `format_time()` truncates seconds instead of rounding
  - **JS Source**: `src/js/modules/tab_audio.js` lines 207–209 (computed properties using `generics.formatPlaybackSeconds`)
  - **Status**: Pending
  - **Details**: JS uses `generics.formatPlaybackSeconds()` which calls `Math.round(seconds % 60)` for the seconds component. C++ tab_audio.cpp defines a local `format_time()` (L320–328) that uses `static_cast<int>(seconds)` which truncates instead of rounding. The C++ already has a correct `generics::formatPlaybackSeconds()` (generics.cpp L804) using `std::round`. Fix: replace the local `format_time()` calls with `generics::formatPlaybackSeconds()`.

- [ ] 60. [tab_data.cpp] Status bar unit type is "table" instead of "db2 file"
  - **JS Source**: `src/js/modules/tab_data.js` line 99
  - **Status**: Pending
  - **Details**: JS Listbox has `unittype="db2 file"` which produces a status bar reading "X db2 files found". C++ (tab_data.cpp L284) calls `listbox::renderStatusBar("table", {}, listbox_db2_state)` producing "X tables found". Fix: change `"table"` to `"db2 file"`.

- [ ] 61. [tab_fonts.cpp] Glyph grid not rendered in loaded font
  - **JS Source**: `src/js/modules/tab_fonts.js` line 154 (JS `detect_glyphs_async` sets `cell.style.fontFamily`)
  - **Status**: Pending
  - **Details**: JS renders each glyph cell using the loaded game font (`fontFamily = "font_id", monospace`), so users see the actual glyphs from the game font file. C++ (tab_fonts.cpp L284–313) renders the glyph grid without calling `ImGui::PushFont(loaded_font)`, so glyphs appear in the default ImGui font. Characters not present in the default font render as rectangles. Fix: push the loaded ImFont* before rendering the grid and pop it afterwards.

- [ ] 62. [tab_item_sets.cpp] Missing `BeginTab`/`EndTab` wrapper
  - **JS Source**: `src/js/modules/tab_item_sets.js` line 77 (`<div class="tab" id="tab-item-sets">`)
  - **Status**: Pending
  - **Details**: Every other tab in the C++ codebase uses `app::layout::BeginTab("tab-xxx")` / `app::layout::EndTab()` to wrap content in a properly configured child window with consistent padding and scroll behavior. `tab_item_sets::render()` skips this and uses `ImGui::BeginChild` directly. Fix: wrap render content in `BeginTab`/`EndTab`.

- [ ] 63. [tab_items.cpp] Item cache invalidation based only on vector size
  - **JS Source**: `src/js/modules/tab_items.js` lines 148–153 (reactivity-based updates)
  - **Status**: Pending
  - **Details**: C++ (tab_items.cpp L611) only rebuilds the item string cache when `view.listfileItems.size() != s_item_entries_cache_size`. If a filter change produces a different set of items with the same count, the cache shows stale entries. JS uses Vue reactivity which always re-renders when the array reference changes. Fix: use a generation counter that increments in `apply_filters()` instead of comparing sizes.

- [ ] 64. [tab_text.cpp] Missing `ImGui::SameLine()` between regex label and filter input
  - **JS Source**: `src/js/modules/tab_text.js` lines 30–33
  - **Status**: Pending
  - **Details**: JS renders "Regex Enabled" text and the filter input on the same line (siblings in a flex div). C++ (tab_text.cpp L193–204) renders them without `ImGui::SameLine()` between them, causing the filter input to appear below the regex label when regex is enabled. Other tabs like `tab_raw.cpp` (L383) correctly include `SameLine()`. Fix: add `ImGui::SameLine()` after the regex tooltip check, before `ImGui::SetNextItemWidth`.

- [ ] 65. [tab_fonts.cpp] Missing `ImGui::SameLine()` between regex label and filter input
  - **JS Source**: `src/js/modules/tab_fonts.js` lines 60–62
  - **Status**: Pending
  - **Details**: Same issue as #64 but in tab_fonts.cpp. Lines 268–274 render "Regex Enabled" text and tooltip but do not call `ImGui::SameLine()` before the filter input (L274). Both `tab_audio.cpp` (L529) and `tab_data.cpp` (L292) correctly include `SameLine()` in the same pattern. Fix: add `ImGui::SameLine()` after the tooltip check on L271, before `ImGui::SetNextItemWidth`.

- [ ] 66. [tab_creatures.cpp] Missing stack trace in `helper.mark()` export catch blocks
  - **JS Source**: `src/js/modules/tab_creatures.js` lines 933, 973
  - **Status**: Pending
  - **Details**: Both catch blocks in `export_files` (tab_creatures.cpp L1348 and L1396) pass only `e.what()` to `helper.mark()` without a stack trace: `helper.mark(creature_name, false, e.what())`. JS passes both `e.message` and `e.stack` as separate arguments. Other tabs (tab_decor L289, tab_models L811) correctly pass `build_stack_trace("export_files", e)` as the 4th argument. The tab_creatures file is also missing the `build_stack_trace` helper entirely. Fix: add `build_stack_trace` and update both catch blocks to `helper.mark(creature_name, false, e.what(), build_stack_trace("export_files", e))`.
