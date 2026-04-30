# Deviations from JS Source

This file documents all intentional and necessary deviations from the original JavaScript source code. Every deviation from the JS source **must** be recorded here — not in code comments.

Each entry includes the C++ file, the JS source reference, a description of the deviation, and the reason it was necessary.

> **Format:** Entries are grouped by category. Each entry follows this template:
> ```
> ### N. [file.cpp] Short description
> - **JS Source**: `src/js/original-file.js` lines XX–YY
> - **Reason**: Why the deviation was necessary (C++ language constraint, API difference, etc.)
> - **Impact**: What observable behaviour differs from JS
> ```

---

## Removed Features

These features have been deliberately removed from the C++ port with no equivalent.

### R1. "Reload Styling" context menu option
- **JS Source**: `src/app.js` lines 160-164, registered at line 550
- **Reason**: JS hot-reloads CSS `<link>` tags with cache-busting query strings — a dev tool with no C++ equivalent since ImGui has no external stylesheets.
- **Impact**: Context menu has one fewer entry. No user-facing functionality loss (was a dev-only feature).

### R2. tab_help module
- **JS Source**: `src/js/modules/tab_help.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Help tab is absent from the C++ app.

### R3. tab_changelog module
- **JS Source**: `src/js/modules/tab_changelog.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Changelog tab is absent from the C++ app.

### R4. markdown-content component
- **JS Source**: `src/js/components/markdown-content.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port. Only consumed by the removed tab_help and tab_changelog modules.
- **Impact**: No standalone impact.

## Intentional Stubs

These files are intentionally left as no-ops and do not need implementation.

### S1. home-showcase.cpp
- **JS Source**: `src/js/components/home-showcase.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Home tab shows no showcase content.

### S2. tab_home.cpp
- **JS Source**: `src/js/modules/tab_home.js`
- **Reason**: Intentionally blank — content not wanted in the C++ port.
- **Impact**: Home tab layout is empty.

## Async/Sync Conversions

JS async/await patterns converted to synchronous C++ calls. The standard JS-to-C++ mapping is async->sync, but these specific cases have observable timing differences.

### A1. [updater.cpp] Auto-updater removed
- **JS Source**: `src/js/updater.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Application does not check for or apply updates.

### A2. [tiled-png-writer.cpp] write() returns immediately via shared_future
- **JS Source**: `src/js/tiled-png-writer.js` lines 123-125
- **Reason**: JS `await tiledWriter.write(path)` guarantees file is on disk before continuing. C++ launches write on `std::async` and returns a `shared_future<void>` immediately.
- **Impact**: Callers must call `.get()` to observe completion or errors. Race condition if callers don't await.

### A3. [GeosetMapper.cpp] map() async signature dropped
- **JS Source**: `src/js/3D/GeosetMapper.js` lines 79-84
- **Reason**: JS declares `async map()` but body contains no `await`. C++ drops the async wrapper since it's unnecessary.
- **Impact**: None in practice — JS function was already synchronous in behaviour.

## Container Ordering

JS `Map`/`Object` preserve insertion order; C++ `std::unordered_map`/`std::map` do not.

### O1. [tiled-png-writer.cpp] tiles map uses std::map (lexicographic order)
- **JS Source**: `src/js/tiled-png-writer.js` lines 25, 58-59
- **Reason**: JS `Map` iterates in insertion order. C++ `std::map<std::string, Tile>` iterates in lexicographic key order.
- **Impact**: Porter-Duff blend output differs when tiles overlap with partial alpha.

### O2. [xml.cpp] build_object uses std::unordered_map for child grouping
- **JS Source**: `src/js/xml.js` lines 138-153
- **Reason**: JS `Object.entries(groups)` yields insertion order. C++ `std::unordered_map` iterates in hash order.
- **Impact**: JSON output key sequence differs from JS for the same XML input.

### O3. [Shaders.cpp] active_programs is std::unordered_map
- **JS Source**: `src/js/3D/Shaders.js` lines 26, 100-122
- **Reason**: JS `Map` insertion order. C++ `std::unordered_map` hash order.
- **Impact**: reload_all recompile order and log line order differ from JS.

### O4. [WMOExporter.cpp] groupNames ordering may differ
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 738, 1193
- **Reason**: JS `Object.values(wmo.groupNames)` yields insertion order. C++ container may iterate differently.
- **Impact**: Meta JSON `groupNames` array order may differ. (TODO 62)

### O5. [ADTExporter.cpp] foliage JSON key order
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1474-1479
- **Reason**: JS `JSON.stringify` emits the entire DB row in insertion order. C++ enumerates fields via `unordered_map`.
- **Impact**: JSON key ordering differs.

### O6. [mpq-install.cpp] getFilesByExtension and getAllFiles add std::sort
- **JS Source**: `src/js/mpq/mpq-install.js` lines 87-120
- **Reason**: JS returns results in Map insertion order (archive processing order). C++ adds `std::sort` producing alphabetical order.
- **Impact**: Callers that depend on archive-processing order see different results. (TODO 141)

## Data Type / Sentinel Value Differences

JS dynamic types (NaN, undefined, null) don't map 1:1 to C++ static types.

### D1. [subtitles.cpp] parse_sbt_timestamp returns 0 instead of NaN for malformed input
- **JS Source**: `src/js/subtitles.js` lines 8-22
- **Reason**: JS `parseInt` returns `NaN` which propagates through arithmetic. C++ returns `std::nullopt` mapped to 0.
- **Impact**: Malformed timestamps produce 0 in C++ vs NaN in JS.

### D2. [wmv.cpp] parse_legacy returns -1 sentinel instead of NaN
- **JS Source**: `src/js/wmv.js` lines 87-92
- **Reason**: JS `parseInt("abc")` returns NaN. C++ `safe_parse_int` returns `std::nullopt`, mapped to -1 via `value_or(-1)`.
- **Impact**: Callers inspecting the int directly see -1 vs NaN. Practical effect is similar (no choice picked).

### D3. [ShaderProgram.cpp] get_uniform_block_param returns -1 instead of null
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 122-127
- **Reason**: JS returns `null` for `INVALID_INDEX`. C++ returns `-1` (valid GLint).
- **Impact**: Callers cannot distinguish "block missing" from legitimate -1. (TODO 73)

### D4. [ADTExporter.cpp] scale 0 maps to 1.0 instead of preserving 0
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1270
- **Reason**: JS uses `model.scale !== undefined ? model.scale / 1024 : 1`. C++ uses `model.scale != 0.0f` as the gate, conflating absent with zero.
- **Impact**: Explicit scale 0 yields 0/1024=0 in JS but 1.0 in C++.

### D5. [M2Exporter.cpp] data-texture fileDataID not back-patched
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 153-168
- **Reason**: JS back-patches `texture.fileDataID = 'data-' + textureType` (string). C++ `Texture::fileDataID` is `uint32_t`, cannot hold a string.
- **Impact**: Meta JSON `textures[i].fileDataID` differs for data-texture rows. (TODO 50)

### D6. [DBCharacterCustomization.cpp] getFileDataIDByDisplayID maps nullopt to 0
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 128-130
- **Reason**: JS stores raw `undefined` from lookup. C++ uses `.value_or(0)`, converting no-value to 0.
- **Impact**: Downstream callers see `std::optional(0)` instead of `nullopt`. FileDataID 0 is generally invalid so practical impact is low. (TODO 110)

## API Signature Differences

C++ function signatures that differ from the JS originals.

### P1. [M2Exporter.cpp] addURITexture takes BufferWrapper instead of dataURI string
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59-61, 111
- **Reason**: C++ caller pre-decodes; JS stores base64 data URI string and decodes at export time.
- **Impact**: API contract differs. JS regex strip of data-URI prefix is not preserved. (TODO 49)

### P2. [M3Exporter.cpp] addURITexture stores BufferWrapper instead of dataURI string
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49-51
- **Reason**: Same as P1 — C++ uses raw bytes instead of base64 string.
- **Impact**: Consumers reading `dataTextures` receive raw bytes instead of data URI. (TODO 58)

### P3. [ShaderProgram.cpp] set_uniform_3fv/4fv/mat4_array add explicit count parameter
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 204-208, 214-218, 247-251
- **Reason**: JS infers count from `values.length`. C++ `const float*` has no length — callers must pass count.
- **Impact**: Default count=1 if callers don't specify. API contract differs. (TODO 74)

### P4. [UniformBuffer.cpp] set_float_array requires explicit count
- **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 152-158
- **Reason**: Same as P3 — `const float*` has no length.
- **Impact**: Same as P3. (TODO 75)

### P5. [DBItemGeosets.cpp] getDisplayId missing modifier_id parameter
- **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 277-279
- **Reason**: Omission — JS accepts optional `modifier_id`, C++ only accepts `item_id`.
- **Impact**: C++ always uses default modifier resolution. (TODO 112)

### P6. [DBItemModels.cpp] getItemModels missing modifier_id parameter
- **JS Source**: `src/js/db/caches/DBItemModels.js` lines 141-142
- **Reason**: Same omission as P5.
- **Impact**: C++ always uses default modifier resolution. (TODO 113)

## File Path / String Processing

Differences in how file paths and strings are processed.

### F1. [M2LegacyExporter.cpp] matName uses stem() instead of .blp-only removal
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` line 94
- **Reason**: JS `path.basename(texturePath, '.blp')` strips only `.blp`. C++ `stem()` strips any extension.
- **Impact**: Non-.blp paths get different matNames. (TODO 53)

### F2. [WMOLegacyLoader.cpp] getGroup uses rfind (case-insensitive) instead of first-occurrence replace
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` line 138
- **Reason**: JS `String.replace('.wmo', ...)` is case-sensitive, replaces first occurrence. C++ checks last 4 chars case-insensitively.
- **Impact**: Different behaviour for mid-path `.wmo` and uppercase extensions. (TODO 81)

### F3. [WMOLoader.cpp] getGroup uses rfind(".wmo") instead of find(".wmo")
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 75-78
- **Reason**: JS `replace` targets first occurrence. C++ `rfind` targets last occurrence.
- **Impact**: Paths with multiple `.wmo` segments would diverge. (TODO 83)

### F4. [WMOLegacyExporter.cpp] absolute() without normalization vs path.resolve()
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 316-317
- **Reason**: JS `path.resolve` normalizes `..`/`.` segments. C++ `std::filesystem::absolute` does not.
- **Impact**: CSV ModelFile values may contain unresolved relative segments. (TODO 67)

### F5. [WMOLegacyExporter.cpp] ASCII-only tolower instead of Unicode toLowerCase
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 299, 310, 507, 510
- **Reason**: C++ `std::tolower` only handles ASCII bytes. JS `toLowerCase()` is Unicode-aware.
- **Impact**: Non-ASCII characters in paths would cause cache misses / duplicate exports. (TODO 68)

### F6. [tab_audio.cpp] parent_path() returns "" instead of "." for bare filenames
- **JS Source**: `src/js/modules/tab_audio.js` lines 155-158
- **Reason**: Node.js `path.dirname("file.ogg")` returns `"."`. C++ `parent_path()` returns `""`.
- **Impact**: Export paths for bare filenames could get a leading separator. (TODO 121)

### F7. [tab_fonts.cpp] Same parent_path() vs dirname() mismatch
- **JS Source**: `src/js/modules/tab_fonts.js` lines 124-126
- **Reason**: Same as F6.
- **Impact**: Same as F6. (TODO 132)

## Rendering / Canvas Operations

WebGL/Canvas APIs mapped to OpenGL/FBO equivalents.

### G1. [ADTExporter.cpp] FBO + pixel rotation instead of canvas rotate+composite
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 891-1161
- **Reason**: No HTML Canvas in C++. Uses offscreen FBO with index-swap rotation.
- **Impact**: JS canvas bilinear resampling on drawImage vs C++ plain index swap. Slight edge differences.

### G2. [ADTExporter.cpp] stb_image_resize instead of canvas drawImage
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 861-890
- **Reason**: No HTML Canvas. Uses `stbir_resize_uint8_linear`.
- **Impact**: Slightly different interpolation results.

### G3. [GLContext.cpp] ext_float_texture hard-coded to true
- **JS Source**: `src/js/3D/gl/GLContext.js` lines 61-62
- **Reason**: JS probes `gl.getExtension('EXT_color_buffer_float')`. C++ assumes true since float textures are core in GL 3.0+.
- **Impact**: Flag is not actually probed. Could be incorrect on unusual GL implementations. (TODO 69)

### G4. [GLTexture.cpp] Extra flip_y option not in JS
- **JS Source**: `src/js/3D/gl/GLTexture.js` lines 34-50
- **Reason**: Added to compensate for OpenGL vs WebGL coordinate conventions. JS never flips Y.
- **Impact**: Extra code path and option with no JS equivalent. (TODO 70)

### G5. [GLTexture.cpp] set_canvas takes raw pixels instead of HTMLCanvasElement
- **JS Source**: `src/js/3D/gl/GLTexture.js` lines 52-73
- **Reason**: No HTMLCanvasElement in C++. Takes `(pixels, w, h, options)` and forwards to set_rgba.
- **Impact**: Callers must rasterize content to RGBA bytes themselves. (TODO 71)

## UBO / Bone Animation

Bone Uniform Buffer Object creation and binding order differences.

### B1. [M2LegacyRendererGL.cpp] _create_bones_ubo method missing entirely
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 389, 474-477, 1020
- **Reason**: UBO pipeline not yet implemented in C++ for legacy renderer.
- **Impact**: Bone animation will not work until implemented. (TODO 87)

### B2. [M2RendererGL.cpp] Bones UBO created before skeleton is loaded
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 406-439, 567
- **Reason**: C++ creates UBO in `load()` before `_create_skeleton()`. JS creates it in `loadSkin()` after skeleton setup.
- **Impact**: UBO sized for 0 bones. Bone animation may fail. (TODO 89)

### B3. [M3RendererGL.cpp] _create_bones_ubo moved from loadLOD() to load()
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 79-81, 147
- **Reason**: C++ creates UBO once in `load()`. JS creates it per-`loadLOD()` call.
- **Impact**: Switching LOD levels won't recreate the UBO. (TODO 93)

### B4. [MDXRendererGL.cpp] Bone UBO created before skeleton/geometry
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 189-199, 262-265, 291
- **Reason**: C++ creates UBO in `load()` before skeleton. JS creates it inside `_build_geometry` after skeleton.
- **Impact**: UBO always sized for 0 bones. (TODO 99)

### B5. [M2LegacyRendererGL.cpp] Extra u_has_tex_matrix uniforms
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 956-957
- **Reason**: C++ sets `u_has_tex_matrix1/2 = 0` which JS does not set at all.
- **Impact**: Shader may skip texture matrix application. (TODO 88)

### B6. [M3RendererGL.cpp] Extra u_has_tex_matrix uniforms
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 243-244
- **Reason**: Same as B5.
- **Impact**: Same as B5. (TODO 96)

### B7. [M2RendererGL.cpp] Missing u_ambient_color and u_diffuse_color uniforms
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1512-1513
- **Reason**: C++ render() omits both uniform calls.
- **Impact**: Lighting will be incorrect if shader uses these uniforms. (TODO 90)

### B8. [M3RendererGL.cpp] UBO bind(0) not called per draw call
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` line 292
- **Reason**: C++ binds UBO once before loop. JS rebinds per draw call.
- **Impact**: Could break with intervening UBO binds. (TODO 95)

### B9. [MDXRendererGL.cpp] stopAnimation resets ALL bones instead of just bone 0
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 429-431
- **Reason**: JS `set(IDENTITY_MAT4)` with no offset only writes 16 floats (bone 0). C++ loops all bones.
- **Impact**: Different visual result when stopping animation on multi-bone models. (TODO 98)

### B10. [M2RendererGL.cpp] _dispose_skin does not dispose bone UBOs
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1914-1922
- **Reason**: C++ only disposes UBO in final `dispose()`, not in `_dispose_skin()`.
- **Impact**: Old UBOs not cleaned up if loadSkin called multiple times. (TODO 92)

### B11. [M3RendererGL.cpp] UBO disposal missing from _dispose_geometry
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 310-311, 316
- **Reason**: C++ `_dispose_geometry()` has no UBO disposal.
- **Impact**: Same as B10. (TODO 94)

## Exporter Structure Differences

Export pipeline structural deviations.

### E1. [M2Exporter.cpp] exportRaw parent-skeleton bones gated on wrong config flag
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 1098-1163
- **Reason**: C++ places parent-skel bone export inside `modelsExportAnim` block. JS has it outside.
- **Impact**: Bones-only export without animations silently loses parent-skel bone files. (TODO 48)

### E2. [M2Exporter.cpp] cancellation returns partial struct instead of undefined
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 142-143
- **Reason**: C++ returns partial `M2ExportTextureResult`. JS returns `undefined`.
- **Impact**: Callers proceed with empty maps instead of dereferencing undefined. (TODO 51)

### E3. [ADTExporter.cpp] useADTSets hardcoded to false
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1300-1351
- **Reason**: C++ hardcodes `false`. JS evaluates `model & 0x80` (always 0 for JS objects, but should check binary flag).
- **Impact**: Doodad-set-from-ADT branch is dead code.

### E4. [ADTExporter.cpp] Liquid export pushes nullptr for chunks with no instances
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1409-1411
- **Reason**: JS preserves the chunk object (including attributes). C++ pushes nullptr.
- **Impact**: Chunk's `attributes` field is lost.

### E5. [WMOExporter.cpp] groupID=0 treated as "no fileID"
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 1266-1270
- **Reason**: JS `??` operator falls through on 0. C++ treats any in-bounds index as resolved.
- **Impact**: WMOs with 0 in groupIDs lose group-file export coverage. (TODO 61)

### E6. [M2LegacyExporter.cpp] subMesh JSON drops fields beyond hardcoded list
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 206-210
- **Reason**: JS `Object.assign` copies all properties. C++ manually lists 13 fields.
- **Impact**: Additional loader properties silently dropped from meta JSON. (TODO 54)

### E7. [M2LegacyExporter.cpp] textures JSON emits explicit null instead of omitting
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 229-234
- **Reason**: JS `undefined` is omitted by `JSON.stringify`. C++ explicitly writes `null`.
- **Impact**: Output JSON has extra `null` keys that JS would not produce. (TODO 55)

### E8. [WMOLegacyExporter.cpp] CSV float formatting uses std::to_string
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 322-333
- **Reason**: JS `Number.toString` produces shortest round-trip representation. C++ `std::to_string(float)` produces fixed precision with trailing zeros.
- **Impact**: CSV output is byte-different for every float field. (TODO 66)

## Loader / Parser Differences

### L1. [MDXLoader.cpp] parse_ATCH reads KVIS data that JS skips
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` lines 387-412
- **Reason**: JS has a bug where `readUInt32LE(-4)` makes the KVIS branch dead code. C++ deliberately reads KVIS.
- **Impact**: `attachment.visibilityAnim` is populated in C++ but always empty in JS. Intentional improvement. (TODO 76)

### L2. [MDXLoader.cpp] Node-to-nodes registration order differs
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` lines 208-210, 53-56
- **Reason**: JS registers in chunk-appearance order. C++ defers to end of `load()` with fixed iteration order.
- **Impact**: Different node wins on objectId collision. (TODO 77)

### L3. [SKELLoader.cpp] loadAnimsForIndex swallows exceptions
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 308-347
- **Reason**: C++ wraps in try/catch and logs. JS propagates rejection.
- **Impact**: Callers lose error signal, see generic false instead of specific error. (TODO 78)

### L4. [SKELLoader.cpp] loadAnims swallows exceptions
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 407-454
- **Reason**: Same as L3 — continues on error instead of aborting.
- **Impact**: Incomplete animFiles map with only a log line. (TODO 79)

### L5. [WMOLegacyLoader.cpp] getGroup creates child with fileName set
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 144-148
- **Reason**: JS passes `undefined` for fileName on child loaders. C++ sets `fileName = groupPath`.
- **Impact**: Sentinel-style checks on child loaders see different values. (TODO 80)

### L6. [WMOLegacyLoader.cpp] fileID=0 skips listfile lookup
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 22-30
- **Reason**: JS `fileID !== undefined` accepts 0. C++ `fileID != 0` rejects 0.
- **Impact**: Different fileName resolution for fileDataID 0 (not a real WoW asset). (TODO 82)

### L7. [WMOLoader.cpp] Same fileID=0 deviation
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 18-32
- **Reason**: Same as L6.
- **Impact**: Same as L6. (TODO 84)

### L8. [DBCReader.cpp] _read_field reads 8 bytes for Int64/UInt64
- **JS Source**: `src/js/db/DBCReader.js` lines 389-408
- **Reason**: JS has no Int64/UInt64 case — falls through to default (4 bytes). C++ explicitly handles 8-byte reads.
- **Impact**: Record offset misalignment if 64-bit field encountered in DBC. Intentional improvement. (TODO 109)

## Ownership / Lifecycle

### M1. [Shaders.cpp] create_program splits ownership (unique_ptr + raw pointer)
- **JS Source**: `src/js/3D/Shaders.js` lines 56-72
- **Reason**: JS GC keeps the program alive as long as any reference holds it. C++ unique_ptr transfers ownership to caller while raw pointer stays in active_programs.
- **Impact**: Dangling pointer risk if caller's unique_ptr goes out of scope before unregister.

### M2. [Shaders.cpp] create_program installs _unregister_fn callback
- **JS Source**: `src/js/3D/Shaders.js` lines 56-72
- **Reason**: Added to compensate for M1's split ownership. No JS equivalent.
- **Impact**: Destroying unique_ptr auto-deregisters. JS pinned programs survive indefinitely.

### M3. [audio-helper.cpp] onended callback doesn't clean up sound/decoder objects
- **JS Source**: `src/js/ui/audio-helper.js` lines 57-67
- **Reason**: JS nulls source immediately. C++ leaves sound/decoder allocated until next play().
- **Impact**: Resource deviation after natural playback end. (TODO 142)

## Configuration / Manifest

### C1. [updater.cpp] Build-time constants instead of runtime manifest
- **JS Source**: `src/js/updater.js` lines 24-25, 33, 35
- **Reason**: JS reads `nw.App.manifest.flavour`/`guid` at runtime. C++ uses compile-time constants.
- **Impact**: Stale GUID/flavour if manifest changes without rebuild.

## Control Flow / Error Handling

### X1. [ShaderProgram.cpp] _compile deletes successfully-compiled shader on error
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 35-36
- **Reason**: JS leaks the successful shader on compile failure. C++ explicitly deletes both.
- **Impact**: Intentional fix for a JS leak — behavioural deviation in failure path. (TODO 72)

### X2. [M2LegacyExporter.cpp] exportAsOBJ/STL throw explicit error on null skin
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 129, 268
- **Reason**: JS would crash with TypeError on property access. C++ throws descriptive `runtime_error`.
- **Impact**: Different error type/message in failure path. (TODO 57)

### X3. [WMOExporter.cpp] empty-UV branch defensively allocates uv_maps[0]
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 316-321
- **Reason**: JS `splice(-1, 1)` on missing canvas removes last element (latent bug). C++ handles missing entry gracefully.
- **Impact**: Intentional fix — JS would crash, C++ allocates zeros. (TODO 65)

### X4. [context-menu.cpp] Click-to-close checks all mouse buttons
- **JS Source**: `src/js/components/context-menu.js` line 54
- **Reason**: Vue `@click` only fires on left button. C++ checks left, right, and middle.
- **Impact**: Right/middle click closes menu in C++ but not in JS. (TODO 105)

### X5. [item-picker-modal.cpp] open_items_tab erroneously closes modal
- **JS Source**: `src/js/components/item-picker-modal.js` lines 143-145
- **Reason**: JS only emits event. C++ emits event AND calls close_modal().
- **Impact**: "Search in Items Tab" always dismisses modal in C++, not in JS. (TODO 106)

### X6. [item-picker-modal.cpp] Missing click-outside-to-close on backdrop
- **JS Source**: `src/js/components/item-picker-modal.js` line 161
- **Reason**: `BeginPopupModal` doesn't support click-outside-to-close natively.
- **Impact**: Users must use Cancel/Escape/X instead of clicking backdrop. (TODO 107)

### X7. [char-texture-overlay.cpp] remove() handles missing elements differently
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 46-61
- **Reason**: JS `splice(-1, 1)` removes last element on missing. C++ does nothing.
- **Impact**: C++ is arguably more correct but differs from JS. (TODO 144)

### X8. [M2RendererGL.cpp] childSkelLoader only stored when childAnimKeys non-empty
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 697-705
- **Reason**: JS unconditionally stores child skeleton. C++ gates on non-empty anim keys.
- **Impact**: Child skeletons with bones but no .anim entries silently discarded. (TODO 91)

### X9. [MDXRendererGL.cpp] _create_skeleton allocates node_matrices when nodes empty
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 270-273
- **Reason**: JS sets `nodes = null` and returns. C++ clears nodes AND resizes node_matrices to 16 floats.
- **Impact**: Extra allocation with no JS counterpart. (TODO 101)

## UI Text / String Differences

### T1. [legacy_tab_audio.cpp] unittype is "sound" instead of "sound file"
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 204
- **Reason**: Omission.
- **Impact**: Status bar shows "42 sounds found" instead of "42 sound files found". (TODO 114)

### T2. [legacy_tab_audio.cpp] persistscrollkey is "legacy-sounds" instead of "sounds"
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 204
- **Reason**: Renamed, possibly intentionally for disambiguation.
- **Impact**: Scroll positions use different key. (TODO 115)

### T3. [legacy_tab_textures.cpp] persistscrollkey is "legacy-textures" instead of "textures"
- **JS Source**: `src/js/modules/legacy_tab_textures.js` line 117
- **Reason**: Same as T2.
- **Impact**: Scroll positions use different key. (TODO 116)

### T4. [legacy_tab_textures.cpp] Toast button "View Log" instead of "view log"
- **JS Source**: `src/js/modules/legacy_tab_textures.js` line 68
- **Reason**: Casing difference.
- **Impact**: Button text differs. (TODO 117)

### T5. [module_test_b.cpp] isBusy shows boolean text instead of numeric counter
- **JS Source**: `src/js/modules/module_test_b.js` line 9
- **Reason**: C++ renders bool string. JS renders integer counter value.
- **Impact**: Debug display shows "true"/"false" instead of 0/1/2. (TODO 118)

### T6. [tab_characters.cpp] Toast message text differs
- **JS Source**: `src/js/modules/tab_characters.js` line 781
- **Reason**: Different wording and omits file data ID.
- **Impact**: User sees different error message. (TODO 122)

### T7. [tab_fonts.cpp] Missing "Filter fonts..." placeholder text
- **JS Source**: `src/js/modules/tab_fonts.js` line 61
- **Reason**: Uses `InputText` instead of `InputTextWithHint`.
- **Impact**: No placeholder shown in empty filter box. (TODO 131)

## Database / Caching

### DB1. [DBCReader.cpp] loadSchema() has filesystem fallback cache paths
- **JS Source**: `src/js/db/DBCReader.js` lines 176-199
- **Reason**: C++ adds fallback that reads/writes DBD files from filesystem when no CASC source available.
- **Impact**: Extra code paths not in JS. (TODO 108)

### DB2. [db2.cpp] Missing "rows === null" preload guard
- **JS Source**: `src/js/db/db2.js` lines 51-52
- **Reason**: C++ only enforces isLoaded check, not rows-null check.
- **Impact**: Developer-facing error message quality gap. (TODO 104)

### DB3. [DBItemDisplays.cpp] Extra getTexturesByDisplayId function
- **JS Source**: `src/js/db/caches/DBItemDisplays.js` (no counterpart)
- **Reason**: C++ adds API surface not in JS original.
- **Impact**: Extra exported function. (TODO 111)

## Miscellaneous

### Z1. [subtitles.cpp] BOM check uses raw 3-byte UTF-8 check
- **JS Source**: `src/js/subtitles.js` lines 177-178
- **Reason**: JS checks decoded UTF-16 code unit `0xFEFF`. C++ checks literal 3-byte UTF-8 BOM (EF BB BF).
- **Impact**: Functionally equivalent for UTF-8 input.

### Z2. [subtitles.cpp] get_subtitles_vtt exposes extra public symbol
- **JS Source**: `src/js/subtitles.js` lines 172-187
- **Reason**: `get_subtitles_vtt_from_text` is exposed in C++ header but not exported in JS.
- **Impact**: Extra API surface.

### Z3. [Texture.cpp] getTextureFile calls getVirtualFileByID instead of getFile
- **JS Source**: `src/js/3D/Texture.js` lines 32-41
- **Reason**: C++ `CASC::getFile` returns encoding key string, not decoded data. `getVirtualFileByID` returns decoded data.
- **Impact**: Structural deviation; verify BLTE decoding, partial-decrypt defaults, fallback handling match.

### Z4. [CameraControlsGL.cpp] dispose() resets state to STATE_NONE
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 218-221
- **Reason**: Extra safeguard not in JS. JS leaves state untouched on dispose.
- **Impact**: Benign — controller disposed during drag retains state in JS, resets in C++.

### Z5. [GLTFWriter.cpp] Animation channel target uses actual_node_idx instead of nodeIndex+1
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 620-628, 757-765, 887-895
- **Reason**: JS always uses `nodeIndex + 1` which is a bug when bone prefix mode is disabled. C++ uses correct `actual_node_idx`.
- **Impact**: Intentional bug fix. Only manifests when `modelsExportWithBonePrefix=false` AND `modelsExportAnimations=true`. (TODO 102)

### Z6. [JSONWriter.cpp] BigInt serialization replacer not ported
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 40-43
- **Reason**: nlohmann::json handles integers up to uint64_t natively. Values exceeding uint64_t must be pre-converted by callers.
- **Impact**: Shifts serialization responsibility to callers. (TODO 103)

### Z7. [tab_maps.cpp] export_map_wmo_minimap tile compositing ignores draw offsets
- **JS Source**: `src/js/modules/tab_maps.js` lines 723-733
- **Reason**: JS uses `ctx.drawImage` with draw_x/draw_y offsets. C++ samples directly without offsets.
- **Impact**: Tiles drawn at (0,0) instead of correct position. Incorrect WMO minimap exports. (TODO 133)

### Z8. [tab_maps.cpp] load_wmo_minimap_tile uses blp.width instead of scaledWidth
- **JS Source**: `src/js/modules/tab_maps.js` lines 103-104
- **Reason**: C++ uses `blp.width`. JS uses decoded canvas width which may differ from raw BLP width.
- **Impact**: Compositing may sample incorrectly if mipmapped. (TODO 134)

### Z9. [tab_maps.cpp] mounted() defers initialization to render()
- **JS Source**: `src/js/modules/tab_maps.js` lines 1132-1146
- **Reason**: ImGui has no equivalent of Vue `mounted()` lifecycle hook. Lazy-init on first render.
- **Impact**: Timing difference. Brief flash of empty content possible. (TODO 135)

### Z10. [tab_maps.cpp] collect_game_objects uses vector instead of Set
- **JS Source**: `src/js/modules/tab_maps.js` lines 127-144
- **Reason**: `std::vector` has no uniqueness guarantee unlike JS `Set`.
- **Impact**: Duplicate rows possible (unlikely in practice). (TODO 136)

### Z11. [tab_maps.cpp] export_map_wmo_minimap condition differs
- **JS Source**: `src/js/modules/tab_maps.js` lines 695-697
- **Reason**: C++ adds extra `worldModel.empty()` check not in JS.
- **Impact**: C++ may attempt setup_wmo_minimap when JS would not. (TODO 137)

### Z12. [tab_models.cpp] Drop handler wraps files as JSON objects instead of strings
- **JS Source**: `src/js/modules/tab_models.js` lines 587, 213
- **Reason**: C++ wraps paths in `{"fileName": path}` objects. JS passes plain strings.
- **Impact**: Every dropped file fails to export — `file_entry.get<std::string>()` throws on object. (TODO 138)

### Z13. [tab_characters.cpp] Extra re-entry guard in load_character_model
- **JS Source**: `src/js/modules/tab_characters.js` lines 718-720
- **Reason**: C++ adds `if (view.chrModelLoading) return;` guard not in JS.
- **Impact**: Could silently skip loads when another model is already loading. (TODO 123)

### Z14. [tab_characters.cpp] navigate_to_items_for_slot skips checkbox update
- **JS Source**: `src/js/modules/tab_characters.js` lines 2499-2516
- **Reason**: C++ relies on `pendingItemSlotFilter` instead of directly toggling checkboxes.
- **Impact**: Previously loaded Items tab filter may not update. (TODO 124)

### Z15. [tab_creatures.cpp] Missing file_name parameter in create_renderer
- **JS Source**: `src/js/modules/tab_creatures.js` line 652
- **Reason**: C++ passes `file_data_id` but omits `file_name` as 6th argument.
- **Impact**: WMO creatures may fail to load group files. (TODO 125)

### Z16. [tab_creatures.cpp] Missing export_paths and file_manifest
- **JS Source**: `src/js/modules/tab_creatures.js` lines 954-969
- **Reason**: C++ doesn't set `opts.file_manifest` or `opts.export_paths`.
- **Impact**: Standard creature exports won't have paths in export stream. (TODO 126)

### Z17. [tab_creatures.cpp] OBJ/STL uses combined function instead of separate calls
- **JS Source**: `src/js/modules/tab_creatures.js` lines 922-927
- **Reason**: C++ uses single `exportAsOBJ(path, isST=true)` instead of `exportAsSTL()`.
- **Impact**: Structural deviation. Functionally identical if combined function is correct. (TODO 127)

### Z18. [tab_decor.cpp] Extra null-guard on GL context
- **JS Source**: `src/js/modules/tab_decor.js` lines 73-74
- **Reason**: C++ shows error toast and returns early when gl_context is null. JS would pass null through.
- **Impact**: C++ refuses preview without GL context. (TODO 128)

### Z19. [tab_decor.cpp] fitCamera called synchronously
- **JS Source**: `src/js/modules/tab_decor.js` line 112
- **Reason**: JS uses `requestAnimationFrame`. No equivalent in ImGui — called synchronously.
- **Impact**: Timing difference, likely benign. (TODO 129)

### Z20. [tab_decor.cpp] export_decor skips non-string selection entries
- **JS Source**: `src/js/modules/tab_decor.js` lines 497-506
- **Reason**: JS passes non-string entries through. C++ silently skips them.
- **Impact**: Object-type selection entries would be missed. (TODO 130)

### Z21. [tab_blender.cpp] checkLocalVersion log output differs
- **JS Source**: `src/js/modules/tab_blender.js` line 161
- **Reason**: JS renders error object as `[object Object]`. C++ renders error string.
- **Impact**: Log output differs in error case. (TODO 119)

### Z22. [tab_blender.cpp] checkLocalVersion comparison differs in error case
- **JS Source**: `src/js/modules/tab_blender.js` line 163
- **Reason**: JS `NaN > NaN = false` → no update toast. C++ `"1.2.3" > "" = true` → shows update toast.
- **Impact**: C++ prompts for update when installed version can't be read; JS does not. (TODO 120)

### Z23. [M2LegacyRendererGL.cpp] _load_textures calls setSlotFileLegacy instead of setSlotFile
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` line 241
- **Reason**: Wrong function called.
- **Impact**: The two functions may have different behavior. (TODO 85)

### Z24. [M2LegacyRendererGL.cpp] UV2 y-flip omitted
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 358-359
- **Reason**: C++ comment claims "already y-flipped by loader." Needs verification.
- **Impact**: Second UV set textures may appear vertically inverted if loader doesn't pre-flip. (TODO 86)

### Z25. [M2LegacyRendererGL.cpp] materials/textureUnits/boundingBox JSON drops fields
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 244, 248, 250, 254
- **Reason**: C++ rebuilds with hardcoded subset instead of serializing all properties.
- **Impact**: Extra fields from loader dropped from meta JSON. (TODO 56)

### Z26. [MDXRendererGL.cpp] Missing per-draw-call UBO bind
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 758
- **Reason**: C++ binds once before loop. JS rebinds per draw call.
- **Impact**: Likely functionally equivalent but deviates from JS structure. (TODO 100)

### Z27. [char-texture-overlay.cpp] ensureActiveLayerAttached has different semantics
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 63-71
- **Reason**: JS re-appends active layer to DOM. C++ validates membership and may substitute active layer.
- **Impact**: JS preserves active layer identity. C++ may change it. (TODO 145)

### Z28. [audio-helper.cpp] load() doesn't propagate decode failure
- **JS Source**: `src/js/ui/audio-helper.js` lines 31-35
- **Reason**: JS rejects promise on decode failure. C++ continues silently with duration 0.
- **Impact**: Caller has no way to know decoding failed. (TODO 143)

### Z29. [WMOExporter.cpp] formatUnknownFile call signature differs
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 158-160
- **Reason**: JS uses single-arg form. C++ uses two-arg form with separate ID and extension.
- **Impact**: Could drift if helper's two-arg behaviour differs. (TODO 63)

### Z30. [WMOExporter.cpp] exportRaw uses byteLength()==0 instead of "is data set" check
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 1223-1231
- **Reason**: JS checks `this.wmo.data === undefined`. C++ checks `data.byteLength() == 0`.
- **Impact**: Conflates "no data" with "empty data". (TODO 64)

### Z31. [M3Exporter.cpp] OBJ texture-manifest reads value as string
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 145-147
- **Reason**: JS value is object with `matPath` field. C++ value is plain string.
- **Impact**: Will diverge once exportTextures is implemented. (TODO 59)

### Z32. [M3Exporter.cpp] exportAsGLTF reshapes texture map
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 91-92
- **Reason**: C++ stringifies numeric key and wraps in `GLTFTextureEntry` adapter.
- **Impact**: Conversion shape needs verification once M3 texture export is implemented. (TODO 60)

### Z33. [M3RendererGL.cpp] load() execution order differs
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 59-71
- **Reason**: C++ creates bones UBO before default texture. JS creates default texture first.
- **Impact**: Initialization order deviation. (TODO 97)

### Z34. [ADTExporter.cpp] saveRawLayerTexture returns void instead of path
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1164-1190
- **Reason**: JS returns relative path. C++ returns void. Callers ignore return value in JS.
- **Impact**: Cosmetic — no behaviour change unless future caller uses return value.

## Platform Adaptations

Deviations that are inherent to the C++/ImGui platform and apply broadly across the codebase.

### PA1. [generics.cpp] queue() uses polling futures instead of promise .then() chains
- **JS Source**: `src/js/generics.js` — queue() function
- **Reason**: JS uses promise `.then()` chains for zero-latency event-driven dispatch. C++ polls `std::futures` with a 1ms wait to find the first ready one.
- **Impact**: Up to 1ms latency per completion. Same tasks, concurrency limit, and completion order.

### PA2. [generics.cpp] Async file operations converted to synchronous
- **JS Source**: `src/js/generics.js` — createDirectory, fileExists, directoryIsWritable, readFile, deleteDirectory
- **Reason**: JS versions are async (return Promises). C++ uses synchronous `std::filesystem` calls.
- **Impact**: Blocking calls on the calling thread. Standard JS-to-C++ async mapping.

### PA3. [generics.cpp] redraw() uses thread yield instead of requestAnimationFrame
- **JS Source**: `src/js/generics.js` — redraw()
- **Reason**: JS calls `requestAnimationFrame()` twice to defer until the browser paints two frames. C++ has no rAF equivalent in ImGui; uses `std::this_thread::yield()`.
- **Impact**: No synchronization with the render loop. ImGui renders continuously so yield is sufficient.

### PA4. [generics.cpp] batchWork uses sleep_for instead of MessageChannel
- **JS Source**: `src/js/generics.js` — batchWork()
- **Reason**: JS uses MessageChannel to post between batches, yielding to the browser event loop. C++ uses `sleep_for(0)` to yield the thread's timeslice.
- **Impact**: Different yielding mechanism between batches.

### PA5. [core.cpp] showLoadingScreen posts to main thread queue
- **JS Source**: `src/js/core.js` — showLoadingScreen()
- **Reason**: JS sets state synchronously on the current event loop turn. C++ posts to main-thread queue for thread safety.
- **Impact**: One-frame delay before state changes are visible.

### PA6. [core.cpp] progressLoadingScreen has no explicit forced redraw
- **JS Source**: `src/js/core.js` — progressLoadingScreen()
- **Reason**: JS calls `await generics.redraw()` for immediate UI repaint. C++/ImGui repaints every frame automatically.
- **Impact**: No explicit forced redraw — repaint happens on next frame tick.

### PA7. [config.cpp] load() is synchronous
- **JS Source**: `src/js/config.js` — load()
- **Reason**: JS load() is async and awaits file reads. C++ blocks the calling thread during file I/O.
- **Impact**: Blocking call. Standard async-to-sync mapping.

### PA8. [config.cpp] No Vue watcher for auto-save on config mutation
- **JS Source**: `src/js/config.js` — load() registers `core.view.$watch('config', ...)`
- **Reason**: C++ has no Vue-style reactive watcher. UI components that change config must explicitly call `config::save()`.
- **Impact**: Config changes can be silently lost if save() is not called.

### PA9. [log.h/log.cpp] getErrorDump is synchronous
- **JS Source**: `src/js/log.js` lines 102-108
- **Reason**: JS returns a Promise. C++ reads synchronously — during a crash, the event loop may be unavailable.
- **Impact**: Blocking I/O, intentionally more reliable for crash diagnostics.

### PA10. [icon-render.cpp] processQueue is synchronous tail recursion
- **JS Source**: `src/js/icon-render.js`
- **Reason**: JS uses async promise chaining between queue items, yielding to the event loop. C++ processes the entire queue synchronously.
- **Impact**: Large queues may block the calling thread.

### PA11. [blp.cpp] toCanvas()/drawToCanvas() absent — replaced by toPNG()/toBuffer()
- **JS Source**: `src/js/casc/blp.js` — toCanvas(), drawToCanvas()
- **Reason**: No HTML Canvas in C++. Pixel-decoding role replaced by toPNG() and direct pixel buffer writing.
- **Impact**: No Canvas element creation. getDataURL() calls toPNG() then BufferWrapper::getDataURL().

### PA12. [blte-stream-reader.cpp] ReadableStream uses pull/cancel object instead of Web Streams API
- **JS Source**: `src/js/casc/blte-stream-reader.js`
- **Reason**: JS uses Web Streams API with controller.error()/controller.close(). C++ exposes simpler pull/cancel — errors propagate as exceptions, end-of-stream as std::nullopt.
- **Impact**: Different API shape, same streaming semantics.

### PA13. [blte-stream-reader.cpp] streamBlocks uses callback instead of async generator
- **JS Source**: `src/js/casc/blte-stream-reader.js`
- **Reason**: JS uses `async *` generator (yields each block). C++ uses callback — C++20 coroutines are not used in this codebase.
- **Impact**: Inverted control flow. Same ordering and payloads.

### PA14. [vp9-avi-demuxer.cpp] extract_frames uses callback instead of async generator
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js`
- **Reason**: JS is an `async*` generator consumed via `for await`. C++ uses callback since BLTEStreamReader::streamBlocks is already callback-based.
- **Impact**: Inverted control flow. Same frame ordering and FrameInfo payloads.

### PA15. [model-viewer-gl.cpp] Controls split into typed pointers instead of polymorphic
- **JS Source**: `src/js/components/model-viewer-gl.js`
- **Reason**: JS uses a single polymorphic `context.controls` pointer (duck-typing). C++ splits into `controls_orbit` and `controls_character` because C++ has no duck-typing.
- **Impact**: External callers must check the appropriate typed pointer. Unified `controls_update` callback abstracts over both.

### PA16. [model-viewer-gl.cpp] Vue $watch replaced by per-frame polling
- **JS Source**: `src/js/components/model-viewer-gl.js`
- **Reason**: JS uses Vue's `$watch` for immediate reactive callbacks on config changes. C++ records previous values and polls per-frame.
- **Impact**: One-frame latency between config change and callback invocation.

### PA17. [map-viewer.cpp] Tile loading limited per frame instead of async concurrency
- **JS Source**: `src/js/components/map-viewer.js`
- **Reason**: JS loads tiles asynchronously (Promises, up to 4 concurrent). C++ tile loading is synchronous, so caps tiles processed per frame at 3. (TODO 284)
- **Impact**: Same throttling effect, different mechanism.

### PA18. [map-viewer.cpp] tileHasUnexpectedTransparency samples full tile, not clipped portion
- **JS Source**: `src/js/components/map-viewer.js`
- **Reason**: JS clamps sample rectangle to canvas via getImageData(). C++ samples full cached tile pixel data. (TODO 285)
- **Impact**: Different sample positions for partially off-canvas tiles. Acceptable — function is a heuristic.

### PA19. [map-viewer.cpp] GL textures with offset positioning instead of double-buffer blit
- **JS Source**: `src/js/components/map-viewer.js` lines 554-627
- **Reason**: JS uses canvas clearRect/drawImage for double-buffer blit. C++ uses GL textures repositioned by offset. (TODO 287)
- **Impact**: Different rendering mechanism, same visual result.

### PA20. [map-viewer.cpp] Map watcher defers default position to next frame
- **JS Source**: `src/js/components/map-viewer.js` lines 143-150
- **Reason**: JS invokes setToDefaultPosition() immediately (Vue has measured DOM). C++ defers via `initialized = false` until ImGui reports viewport width > 0. (TODO 288)
- **Impact**: One-frame delay before default positioning is applied.

### PA21. [slider.cpp] Native ImGui::SliderFloat replaces custom slider
- **JS Source**: `src/js/components/slider.js` lines 41-99
- **Reason**: JS implements fully custom slider with mousedown/mousemove/mouseup listeners, fill bar, and draggable handle. C++ uses native ImGui::SliderFloat.
- **Impact**: Different appearance per Visual Fidelity rules. Same value-in-[0,1] semantic.

### PA22. [resize-layer.cpp] Float width instead of integer clientWidth
- **JS Source**: `src/js/components/resize-layer.js`
- **Reason**: JS emits `this.$el.clientWidth` (always integer pixels). C++ passes raw float from `ImGui::GetContentRegionAvail().x`.
- **Impact**: Sub-pixel precision preserved. Callers needing integer should cast.

### PA23. [tab_videos.cpp] No video end/error detection for external player
- **JS Source**: `src/js/modules/tab_videos.js` — video.onended, video.onerror
- **Reason**: C++ opens an external player via OS shell. No process lifecycle callbacks available.
- **Impact**: User must click "Stop Video" to reset state. HTTP errors caught by kino_post().

### PA24. [casc-source-local.cpp / casc-source-remote.cpp] tact_keys::waitForLoad() in load()
- **JS Source**: `src/js/casc/casc-source-local.js`, `src/js/casc/casc-source-remote.js`
- **Reason**: JS load() does not call this; JS app loads tact keys earlier in boot. C++ ensures encryption keys are available before decryption.
- **Impact**: Extra synchronization step. Prevents decryption failures.

### PA25. [external-links.cpp] renderLink() is a C++-only addition
- **JS Source**: `src/app.js` — global DOM click handler for `data-external` attributes
- **Reason**: JS uses a global DOM click handler to open elements with `data-external`. ImGui has no DOM, so each call site must invoke renderLink() explicitly.
- **Impact**: Same link-opening behaviour, different invocation pattern.

### PA26. [file-writer.h] writeLine is synchronous under mutex
- **JS Source**: `src/js/file-writer.js`
- **Reason**: JS is async — suspends only on Node stream backpressure. C++ std::ofstream has no backpressure concept.
- **Impact**: Writes never suspend the caller. Data always flushed to OS buffer before returning.

### PA27. [DBComponentTextureFileData.cpp] condition_variable instead of init_promise
- **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js`
- **Reason**: JS caches an `init_promise` for coalescing concurrent callers. C++ uses `is_initializing` + `std::condition_variable`. (TODO 314)
- **Impact**: Same behaviour — single initialization, all callers wait.

### PA28. [DBComponentModelFileData.cpp] condition_variable instead of init_promise
- **JS Source**: `src/js/db/caches/DBComponentModelFileData.js`
- **Reason**: Same as PA27.
- **Impact**: Same behaviour.

## Type System Adaptations

Deviations caused by C++ static typing vs JS dynamic typing.

### TS1. [M3Loader.cpp] ReadBufferAsFormat split into two functions
- **JS Source**: `src/js/3D/loaders/M3Loader.js` lines 239-258
- **Reason**: JS returns a mixed-type Array (floats for F32 formats, uint16s for U16). C++ requires statically typed returns, so split into ReadBufferAsFormat (floats) and ReadBufferAsFormatU16 (uint16s).
- **Impact**: Structurally divergent, functionally equivalent.

### TS2. [M2LegacyLoader.cpp] getSkinList returns empty vector instead of undefined
- **JS Source**: `src/js/3D/loaders/M2LegacyLoader.js`
- **Reason**: For WotLK models, JS returns `undefined` (skins not populated). C++ returns empty `std::vector`. Callers check `.size()` so behaviour is equivalent.
- **Impact**: Callers that need unset-vs-empty distinction would need `std::optional`.

### TS3. [LoaderGenerics.cpp/.h] processStringBlock returns std::map instead of JS object
- **JS Source**: `src/js/3D/loaders/LoaderGenerics.js`
- **Reason**: JS returns a plain object with numeric-string keys (insertion order). C++ returns `std::map<uint32_t, std::string>` (ascending order). Keys are written in monotonically increasing order so both coincide. All callers use offset-key lookups, never iterate.
- **Impact**: No observable effect. If iteration order becomes important, switch to vector-of-pairs.

### TS4. [data-table.cpp] Content-based identity instead of JS reference identity
- **JS Source**: `src/js/components/data-table.js`
- **Reason**: JS uses reference identity for row selection. C++ approximates with content-based identity by joining field values. DB2/DBC rows differ by ID column so collisions are extremely rare.
- **Impact**: Rows with identical content treated as equal, but matches visual outcome.

### TS5. [listboxb.cpp] Index-based selection instead of reference-based
- **JS Source**: `src/js/components/listboxb.js`
- **Reason**: JS stores actual item objects and uses `.indexOf(item)` for reference equality. C++ stores integer indices because there is no equivalent of JS reference-equality for arbitrary items.
- **Impact**: All selection logic works with indices. Same semantic.

### TS6. [core.h] AppState struct deviations from makeNewView()
- **JS Source**: `src/js/core.js` — makeNewView()
- **Reason**: `constants` field omitted (accessed via `constants::` namespace). `availableLocale` omitted (compile-time constant via `casc::locale_flags::entries`). Additional C++/OpenGL fields added (`mpq`, `chrCustRacesPlayable/NPC`, `pendingItemSlotFilter`, GL texture resources).
- **Impact**: Platform adaptations for C++/OpenGL/ImGui. No functional difference.

### TS7. [constants.h] CONFIG.DEFAULT_PATH uses different directory
- **JS Source**: JS uses `INSTALL_PATH/src/default_config.jsonc`
- **Reason**: C++ uses `<install>/data/default_config.jsonc` to match the C++ resource layout.
- **Impact**: Different file path for default config.

### TS8. [GLBWriter.h] bin_buffer stored by reference instead of value
- **JS Source**: `src/js/3D/writers/GLBWriter.js`
- **Reason**: JS uses value semantics. C++ stores by reference — GLBWriter must not outlive the BufferWrapper passed to its constructor.
- **Impact**: Lifetime constraint not present in JS.

### TS9. [WDCReader.h] idFieldIndex is uint16_t defaulting to 0 instead of null
- **JS Source**: `src/js/db/WDCReader.js`
- **Reason**: JS initializes `idField` and `idFieldIndex` to `null`. C++ uses `std::optional<std::string>` for idField but plain `uint16_t` for idFieldIndex.
- **Impact**: 0 is indistinguishable from valid index 0 before loading. Only consulted after loading begins, so uninitialised value is never observed.

### TS10. [UniformBuffer.h] Single private data_ vector instead of three public typed views
- **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 20-22
- **Reason**: JS exposes `view` (DataView), `float_view` (Float32Array), `int_view` (Int32Array) over shared ArrayBuffer. C++ uses single private `std::vector<uint8_t>` with typed setter methods.
- **Impact**: Encapsulation difference. Same bytes reach the GPU.

### TS11. [UniformBuffer.cpp] memcpy (native endian) instead of DataView (explicit little-endian)
- **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 51, 60, 70-71, etc.
- **Reason**: JS writes via DataView with explicit little-endian flag. C++ uses `std::memcpy` which writes in host byte order. Both supported platforms (Windows x64, Linux x64) are little-endian, so layout is identical.
- **Impact**: Platform assumption — porting to big-endian would require byte-swap.

## Bug Fixes / Improvements

Cases where C++ intentionally deviates to fix a JS bug or improve behaviour.

### BF1. [blp.cpp] alphaDepth=4 integer division fixes JS float indexing bug
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS `index / 2` produces a float (e.g. 3/2=1.5), causing `rawData[1.5]` to return `undefined` for odd indices, yielding incorrect alpha of 0. C++ integer division floors correctly.
- **Impact**: Bug fix — alpha values are now correct for odd indices.

### BF2. [blp.cpp] DXT decompression uses `>=` bounds check instead of `===`
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS uses strict equality (`=== pos`) to skip when pos exactly equals rawData length. C++ uses `>=` which also handles pos exceeding rawData size.
- **Impact**: Defensive guard — same behaviour for valid data, safer for corrupt data.

### BF3. [blp.cpp] DXT colour interpolation uses integer division vs JS float+clamp
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS stores float64 values and writes to Uint8ClampedArray (round-half-to-even). C++ uses integer division (truncation). Difference is at most 1 LSB, visually imperceptible.
- **Impact**: Sub-LSB colour differences.

### BF4. [casc-source.cpp] Encoding page termination uses correct page-size bound
- **JS Source**: `src/js/casc/casc-source.js`
- **Reason**: JS has a bug comparing against `pageStart + pagesStart` instead of `pageStart + cKeyPageSize`. C++ uses the correct page-size bound.
- **Impact**: Bug fix — each page terminates at the proper boundary.

### BF5. [tab_raw.cpp] is_dirty set after file identification
- **JS Source**: `src/js/modules/tab_raw.js`
- **Reason**: JS bug — is_dirty was never set to true after identifying files, so compute_raw_files() returned early without refreshing the list. C++ intentionally fixes this.
- **Impact**: Bug fix — list actually updates after file identification.

### BF6. [listboxb.cpp] Arrow-key navigation works at index 0
- **JS Source**: `src/js/components/listboxb.js`
- **Reason**: JS `!this.lastSelectItem` is also true for index 0 (`!0 === true`), silently blocking arrow-key navigation. C++ uses `< 0` against sentinel.
- **Impact**: Bug fix — navigation works correctly at index 0.

### BF7. [listboxb.cpp] Ctrl+C copies item labels instead of [object Object]
- **JS Source**: `src/js/components/listboxb.js`
- **Reason**: JS `this.selection.join('\n')` on item objects produces `[object Object]` per item. C++ copies `items[idx].label`.
- **Impact**: Bug fix — users get useful clipboard content.

### BF8. [buffer.cpp] writeBuffer replicates JS silent error discard
- **JS Source**: `src/js/buffer.js` line 917
- **Reason**: JS creates `new Error(...)` without `throw`, silently discarding. C++ replicates same no-op behaviour for fidelity.
- **Impact**: Same silent behaviour as JS.

### BF9. [BONELoader.cpp] Integer division truncation vs JS float division
- **JS Source**: `src/js/3D/loaders/BONELoader.js`
- **Reason**: JS uses float division; non-multiple-of-64 chunkSize would throw RangeError from `new Array(nonInteger)`. C++ truncates silently. Well-formed BONE files always have chunkSize as multiple of 64.
- **Impact**: No real-world impact.

### BF10. [WDCReader.cpp] BigInt index arithmetic uses uint64_t
- **JS Source**: `src/js/db/WDCReader.js`
- **Reason**: JS uses BigInt (arbitrary precision) for bitpacked index arithmetic. C++ uses uint64_t which could theoretically overflow for pathologically large values. In practice, pallet indices are well below uint32_t bounds.
- **Impact**: No overflow with valid input.

## Extra API Surface

C++ additions with no JS counterpart.

### EA1. [WMOExporter.cpp/.h] loadWMO() and getDoodadSetNames()
- **JS Source**: No counterpart
- **Reason**: C++-only UI-layer convenience methods. JS accesses WMO internals directly.
- **Impact**: Extra public API surface.

### EA2. [WDCReader.cpp] getAllRowsAsync()
- **JS Source**: No counterpart (JS is single-threaded)
- **Reason**: C++-only async wrapper. Returns result by value for thread safety.
- **Impact**: Extra public API surface.

### EA3. [menu-button.h] `upward` parameter
- **JS Source**: `src/js/components/menu-button.js` — props: `['options', 'default', 'disabled', 'dropdown']`
- **Reason**: C++-only extension for popup-above-button positioning needed by some ImGui call sites.
- **Impact**: Extra parameter. The no-upward overload preserves the JS contract.

### EA4. [itemlistbox.h] Dead prop and extra emit noted
- **JS Source**: `src/js/components/itemlistbox.js` lines 19-20
- **Reason**: JS declares `includefilecount` prop (dead code, never used by template). JS declares `emits: ['update:selection', 'equip']` but template also emits `'options'`. C++ mirrors rendered template behaviour.
- **Impact**: Dead JS prop not implemented. Extra emit matches actual JS template behaviour.

### EA5. [blp.h] dataURL cache field omitted
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS sets `this.dataURL = null` as a cache field but `getDataURL()` never writes to it (every call regenerates the PNG). Dead field omitted in C++.
- **Impact**: No functional difference.

### EA6. [blp.cpp] toBuffer returns empty BufferWrapper for unknown encodings
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS has no default case (returns undefined). C++ returns empty BufferWrapper for type safety.
- **Impact**: Callers get empty buffer instead of undefined-equivalent.

## Collation / Sorting

### CO1. [tab_creatures.cpp] ASCII tolower + byte comparison instead of localeCompare
- **JS Source**: `src/js/modules/tab_creatures.js`
- **Reason**: JS uses `localeCompare` (Unicode-aware, locale-sensitive ICU collation). C++ performs byte-wise comparison on lowercased UTF-8. A faithful port would require ICU, not a project dependency.
- **Impact**: May diverge for non-ASCII characters. Creature names are predominantly ASCII.

## Styling

### ST1. [listbox.cpp] Bold font weight not replicated
- **JS Source**: `src/js/components/listbox.js` — CSS `.list-status { font-weight: bold }`
- **Reason**: Only one font is loaded. Per Visual Fidelity rules, exact font weights need not match.
- **Impact**: Status text is not bold.

## Configuration Path

### CP1. [constants.cpp] No macOS UPDATE_ROOT resolution
- **JS Source**: `src/js/constants.js`
- **Reason**: JS on macOS resolves 4 levels up from nw.__dirname for the portable-app root. C++ has no macOS support; ROOT equals INSTALL_PATH on all platforms.
- **Impact**: No macOS support — documented platform constraint.

## User-Facing Text

### UT1. [updater.cpp / STLWriter.cpp / GLTFWriter.cpp] "wow.export.cpp" instead of "wow.export"
- **JS Source**: Various — updater.js, STLWriter.js, GLTFWriter.js
- **Reason**: Per project guidelines, user-facing text says "wow.export.cpp".
- **Impact**: Intentional branding change.
