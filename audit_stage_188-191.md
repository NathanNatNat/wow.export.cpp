## File 188. [model-viewer-utils.cpp]

- model-viewer-utils.cpp: `export_preview` has an extra `export_paths` parameter not present in the JS signature
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 277–308
  - **Status**: Pending
  - **Details**: JS `export_preview` is `(core, format, canvas, export_name, export_subdir = '')`. C++ signature is `export_preview(format, ctx, export_name, export_subdir, export_paths)`. The C++ adds `export_paths` as an explicit parameter (JS uses a locally-opened stream). This changes the call convention and caller interface, though the behaviour is equivalent.

- model-viewer-utils.cpp: `initialize_uv_layers` accepts only `M2RendererGL*` but JS accepts any renderer with a `getUVLayers` method
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 107–118
  - **Status**: Pending
  - **Details**: JS `initialize_uv_layers(state, renderer)` calls `renderer.getUVLayers` and works with M2, M3, or WMO renderers. C++ `initialize_uv_layers(ViewStateProxy&, M2RendererGL*)` only accepts an M2 renderer pointer. M3 and WMO renderers will never populate UV layers in C++ even if they would in JS.

- model-viewer-utils.cpp: `toggle_uv_layer` accepts only `M2RendererGL*` but JS accepts any renderer with `getUVLayers`
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 126–152
  - **Status**: Pending
  - **Details**: Same issue as `initialize_uv_layers`. JS toggles UV overlay for any renderer type; C++ restricts to M2 only, so WMO/M3 UV overlays will not work.

- model-viewer-utils.cpp: `export_preview` captures the GL framebuffer via `glReadPixels` instead of reading from an HTML Canvas
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 280
  - **Status**: Pending
  - **Details**: JS uses `BufferWrapper.fromCanvas(canvas, 'image/png')` to capture from an HTML Canvas. C++ reads from the OpenGL framebuffer with `glReadPixels`. This is a necessary C++-specific adaptation since there is no HTML canvas, but means the captured image depends on what is bound to the current GL framebuffer at call time, not a specific canvas element.

- model-viewer-utils.cpp: `create_view_state` does not take a `core` parameter (JS takes `(core, prefix)`)
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 510–535
  - **Status**: Pending
  - **Details**: JS `create_view_state(core, prefix)` takes an explicit `core` reference. C++ uses the global `core::view`. This is a reasonable adaptation but changes the API signature and couples the function to the global state singleton.

- model-viewer-utils.cpp: `handle_animation_change` calls `renderer->playAnimation(m2_index).get()` (blocking) instead of awaiting asynchronously
  - **JS Source**: `src/js/ui/model-viewer-utils.js` lines 262
  - **Status**: Pending
  - **Details**: JS uses `await renderer.playAnimation(anim_info.m2Index)`. C++ calls `.get()` on the future, blocking the calling thread. If playAnimation is long-running, this will block the render thread. The semantic effect is the same but the async model differs.

## File 189. [texture-exporter.cpp]

- texture-exporter.cpp: `exportFiles` catch block uses `fileName` instead of `markFileName` for error marking
  - **JS Source**: `src/js/ui/texture-exporter.js` lines 177
  - **Status**: Pending
  - **Details**: JS `catch (e)` block calls `helper.mark(markFileName, false, e.message, e.stack)` — `markFileName` may have been rewritten with `.webp`/`.png` extension during the try block. C++ uses `fileName` (unchanged original name) because the updated `markFileName` (which is `exportFileName` in C++) is not accessible in the catch scope. This is a scope difference that changes which name is reported on error when format is WEBP or PNG.

- texture-exporter.cpp: `exportFiles` C++ signature adds explicit `casc` and `mpq` parameters not present in the JS function
  - **JS Source**: `src/js/ui/texture-exporter.js` lines 68
  - **Status**: Pending
  - **Details**: JS `exportFiles(files, isLocal = false, exportID = -1, isMPQ = false)` accesses CASC and MPQ through `core.view`. C++ adds `casc::CASC* casc` and `mpq::MPQInstall* mpq` as explicit parameters. This is a necessary adaptation but changes the API surface.

- texture-exporter.cpp: `exportSingleTexture` only accepts a `casc` parameter but JS version takes only `fileDataID`
  - **JS Source**: `src/js/ui/texture-exporter.js` lines 191–193
  - **Status**: Pending
  - **Details**: JS `exportSingleTexture(fileDataID)` calls `exportFiles([fileDataID], false)`. C++ `exportSingleTexture(uint32_t fileDataID, casc::CASC* casc)` requires an explicit CASC pointer. This is a necessary adaptation due to dependency injection, but the API signature diverges from JS.

## File 190. [texture-ribbon.cpp]

- texture-ribbon.cpp: `clearSlotTextures` and `getSlotTexture` are C++-only additions with no JS counterparts
  - **JS Source**: `src/js/ui/texture-ribbon.js` (no equivalent)
  - **Status**: Pending
  - **Details**: JS `texture-ribbon.js` exports only `{ reset, setSlotFile, setSlotFileLegacy, setSlotSrc, onResize, addSlot }`. The C++ adds `clearSlotTextures()` and `getSlotTexture(int)` for GL texture management. These are C++-specific additions required for ImGui image rendering and are not deviations from JS logic, but they extend the JS interface.

- texture-ribbon.cpp: `reset()` calls `clearSlotTextures()` before clearing the stack, which has no JS equivalent
  - **JS Source**: `src/js/ui/texture-ribbon.js` lines 28–34
  - **Status**: Pending
  - **Details**: JS `reset()` just clears the stack array and resets the page/contextMenu. C++ additionally calls `clearSlotTextures()` to free GL textures. This is a correct C++ RAII extension but diverges structurally.

- texture-ribbon.cpp: `s_slotTextures` and `s_slotSrcCache` module-level maps have no JS counterparts
  - **JS Source**: `src/js/ui/texture-ribbon.js` (no equivalent)
  - **Status**: Pending
  - **Details**: These static maps are C++-only infrastructure for GL texture caching. No functional deviation, but they represent additional state not present in the JS module.

## File 191. [uv-drawer.cpp]

- uv-drawer.cpp: Line rendering uses a hard pixel-width Bresenham-like algorithm instead of the JS Canvas 2D `lineWidth = 0.5` (anti-aliased)
  - **JS Source**: `src/js/ui/uv-drawer.js` lines 22–45
  - **Status**: Pending
  - **Details**: JS uses `ctx.lineWidth = 0.5` and `ctx.stroke()` which produces anti-aliased sub-pixel lines via the 2D canvas. C++ uses integer pixel rasterisation (`drawLinePath` → `plotPixel`) which draws 1-pixel-wide aliased lines. The visual output will differ: JS lines are thinner and anti-aliased, C++ lines are full-pixel and aliased. This is a necessary adaptation since there is no 2D canvas API.

- uv-drawer.cpp: `generateUVLayerPixels` is a C++-only helper function not exported from the JS module
  - **JS Source**: `src/js/ui/uv-drawer.js` (no equivalent)
  - **Status**: Pending
  - **Details**: JS exports only `{ generateUVLayerDataURL }`. C++ adds `generateUVLayerPixels` as a second public function for use by `model-viewer-utils.cpp` to upload UV overlays to GL textures. This is a C++-specific extension with no JS counterpart.

- uv-drawer.cpp: JS `indices` parameter type is `Uint16Array`; C++ equivalent uses `std::vector<uint16_t>` — correct mapping but JS treats out-of-bounds as NaN (silently skipped), C++ adds explicit bounds check
  - **JS Source**: `src/js/ui/uv-drawer.js` lines 29–45
  - **Status**: Pending
  - **Details**: JS accessing `uvCoords[outOfBoundsIndex]` returns `undefined`, and `undefined * textureWidth` yields `NaN`. Drawing to `(NaN, NaN)` is silently ignored by the canvas. C++ explicitly checks `if (idx1 + 1 >= uvCoords.size() || ...)` to skip out-of-bounds triangles. The C++ explicit check is the correct safety mechanism and produces the same observable result (no line drawn).
