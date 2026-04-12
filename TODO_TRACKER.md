# TODO Tracker

## Completely Unconverted Files (Raw JavaScript)

### 1. [updater.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/updater.js` lines 1–169
- **Status**: Pending
- **Details**: All 169 lines are verbatim JavaScript. Missing C++ conversion of `checkForUpdates()`, `applyUpdate()`, `launchUpdater()`, module state, and exports. No header file exists.

### 2. [wmv.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/wmv.js` lines 1–177
- **Status**: Pending
- **Details**: All 177 lines are verbatim JavaScript. Missing C++ conversion of `wmv_parse()`, `wmv_parse_v2()`, `wmv_parse_v1()`, `extract_race_gender_from_path()`. No header file exists.

### 3. [wowhead.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/wowhead.js` lines 1–245
- **Status**: Pending
- **Details**: All 245 lines are verbatim JavaScript. Missing C++ conversion of `decode()`, `decompress_zeros()`, `extract_hash_from_url()`, `wowhead_parse_hash()`, `parse_v15()`, `parse_legacy()`, `wowhead_parse()`, plus `charset` and `WOWHEAD_SLOT_TO_SLOT_ID` constants. No header file exists.

---

## Root-Level Files

### 4. [log.cpp] Use-after-move bug in write()
- **JS Source**: `src/js/log.js` lines 75–76
- **Status**: Pending
- **Details**: `line` is used after `std::move` into pool at C++ line 122/130 — undefined behavior producing garbage on debug print.

### 5. [log.cpp] write() lacks variadic formatting
- **JS Source**: `src/js/log.js` lines 75–76
- **Status**: Pending
- **Details**: JS `write()` uses `util.format()` for variadic formatting. C++ `write()` takes a single `string_view` — no format support.

### 6. [log.cpp] timeEnd() drops extra format params
- **JS Source**: `src/js/log.js` lines 60–62
- **Status**: Pending
- **Details**: JS `timeEnd()` forwards extra params to `write()` for formatting. C++ version drops them.

### 7. [log.cpp] drainPool is synchronous and may never flush
- **JS Source**: `src/js/log.js` lines 32–48
- **Status**: Pending
- **Details**: C++ `drainPool` is synchronous and only called on next `write()`. If writes stop, pool may never flush. Also, on drain failure, entry stays in pool (JS removes unconditionally).

### 8. [core.cpp] Toast auto-expiry never fires
- **JS Source**: `src/js/core.js` lines 193–194
- **Status**: Pending
- **Details**: `toastExpiry` is set but never polled — toast never auto-hides. JS uses `setTimeout` for auto-expiry.

### 9. [core.cpp] Shell injection in openInExplorer() on Linux
- **JS Source**: `src/js/core.js` (N/A — platform-specific)
- **Status**: Pending
- **Details**: Linux implementation uses `std::system("xdg-open \"" + path + "\"")` at C++ line 407–408, which is vulnerable to shell injection via crafted paths.

### 10. [core.cpp] Missing constants reference on AppState
- **JS Source**: `src/js/core.js` line 46
- **Status**: Pending
- **Details**: JS assigns `constants` reference to app state. C++ does not.

### 11. [core.cpp] Toast options/actions semantic mismatch
- **JS Source**: `src/js/core.js` lines 183–191
- **Status**: Pending
- **Details**: Toast `options` parameter has different semantics vs JS `actions` at C++ lines 372–389.

### 12. [generics.cpp] get() returns vector instead of Response-like object
- **JS Source**: `src/js/generics.js` lines 14–42
- **Status**: Pending
- **Details**: JS `get()` returns a Response-like object with headers/status. C++ returns `vector<uint8_t>` — API contract changed at C++ lines 176–211.

### 13. [generics.cpp] requestData() missing download progress logging
- **JS Source**: `src/js/generics.js` lines 123–138
- **Status**: Pending
- **Details**: JS logs download progress at 25% intervals. C++ version at lines 342–347 omits this.

### 14. [generics.cpp] requestData() missing redirect logging
- **JS Source**: `src/js/generics.js` lines 116–117
- **Status**: Pending
- **Details**: JS logs redirects. C++ has no redirect logging.

### 15. [generics.cpp] queue() concurrency semantics differ
- **JS Source**: `src/js/generics.js` lines 51–65
- **Status**: Pending
- **Details**: JS launches `limit+1` tasks initially. C++ version at lines 220–244 has different concurrency behavior.

### 16. [generics.cpp] batchWork() runs synchronously, blocks UI thread
- **JS Source**: `src/js/generics.js` lines 227–267
- **Status**: Pending
- **Details**: JS yields via MessageChannel between batches. C++ at lines 585–623 runs synchronously, blocking the UI thread.

### 17. [generics.cpp] getJSON() error message omits HTTP status
- **JS Source**: `src/js/generics.js` lines 94–95
- **Status**: Pending
- **Details**: JS includes HTTP status/statusText in error message. C++ at line 284 omits it.

### 18. [modules.cpp] 5 modules missing from registration
- **JS Source**: `src/js/modules.js` lines 32–33, 53–55
- **Status**: Pending
- **Details**: Missing modules: `module_test_a`, `module_test_b`, `tab_help`, `tab_blender`, `tab_changelog` at C++ lines 386–390.

### 19. [modules.cpp] activated lifecycle hook missing retry-on-failure
- **JS Source**: `src/js/modules.js` lines 188–193
- **Status**: Pending
- **Details**: JS `activated` hook retries failed module loading. C++ at lines 462–465 does not.

### 20. [modules.cpp] update_context_menu_options serializes handler to JSON
- **JS Source**: `src/js/modules.js` line 140
- **Status**: Pending
- **Details**: C++ serializes context menu options to JSON at lines 156–166, losing the `handler` function reference.

### 21. [icon-render.cpp] processQueue() core CASC+BLP logic commented out
- **JS Source**: `src/js/icon-render.js` lines 49–59
- **Status**: Pending
- **Details**: The core logic that processes the icon render queue (CASC file loading and BLP decoding) is entirely commented out at C++ lines 204–223.

### 22. [file-writer.cpp] Backpressure/drain mechanism is no-op
- **JS Source**: `src/js/file-writer.js` lines 25–36
- **Status**: Pending
- **Details**: C++ `_drain()` at lines 23–35 just sets `blocked=false`. JS uses event-driven `stream.once('drain')` for backpressure.

---

## CASC Files

### 23. [casc-source-local.cpp] getFile() renamed to getFileAsBLTE() — breaks virtual dispatch
- **JS Source**: `src/js/casc/casc-source-local.js` lines 63–70
- **Status**: Pending
- **Details**: C++ renames `getFile()` to `getFileAsBLTE()` at lines 88–97, breaking the virtual dispatch chain from the base class.

### 24. [casc-source-local.cpp] load() missing core.view.casc = this assignment
- **JS Source**: `src/js/casc/casc-source-local.js` line 179
- **Status**: Pending
- **Details**: C++ `load()` at lines 229–249 does not assign `core.view.casc = this`, which is needed for other code to reference the active CASC source.

### 25. [casc-source-remote.cpp] getFile() renamed to getFileAsBLTE() — breaks virtual dispatch
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 134–164
- **Status**: Pending
- **Details**: Same issue as local source — C++ renames `getFile()` to `getFileAsBLTE()` at lines 158–193, breaking polymorphism.

### 26. [casc-source-remote.cpp] getFileAsBLTE() checks byteLength == 0 instead of null
- **JS Source**: `src/js/casc/casc-source-remote.js` line 154
- **Status**: Pending
- **Details**: C++ checks `data.byteLength() == 0` at line 181 instead of JS `data === null`. Different semantic — empty data vs no data.

### 27. [casc-source-remote.cpp] load() missing core.view.casc = this assignment
- **JS Source**: `src/js/casc/casc-source-remote.js` line 290
- **Status**: Pending
- **Details**: C++ `load()` at lines 339–351 does not assign `core.view.casc = this`.

### 28. [casc-source-remote.cpp] getConfig() checks res.empty() instead of !res.ok
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 77–78
- **Status**: Pending
- **Details**: C++ at lines 97–98 checks `res.empty()` instead of HTTP status code `!res.ok`.

### 29. [casc-source.cpp] getFileByName() doesn't forward extra params
- **JS Source**: `src/js/casc/casc-source.js` line 190
- **Status**: Pending
- **Details**: C++ at line 236 doesn't forward extra params (partialDecrypt, etc.) to subclass `getFile()`.

### 30. [casc-source.cpp] WMO filter uses hardcoded true instead of constant
- **JS Source**: `src/js/casc/casc-source.js` line 301
- **Status**: Pending
- **Details**: C++ at line 364 uses hardcoded `true` instead of `constants.LISTFILE_MODEL_FILTER` regex.

### 31. [casc-source.cpp] Constructor watches generic config-change event
- **JS Source**: `src/js/casc/casc-source.js` lines 31–38
- **Status**: Pending
- **Details**: C++ at lines 30–56 watches generic "config-change" event vs JS watching specific `cascLocale` key change.

### 32. [build-cache.cpp] getFile() rejects immediately instead of awaiting integrity load
- **JS Source**: `src/js/casc/build-cache.js`
- **Status**: Pending
- **Details**: C++ `getFile()` rejects immediately instead of awaiting the integrity load first as JS does.

### 33. [cdn-resolver.cpp] HTTP status check missing in _resolveRegionProduct
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 152–153
- **Status**: Pending
- **Details**: C++ at lines 172–174 missing explicit HTTP status (`res.ok`) check.

### 34. [cdn-resolver.cpp] startPreResolution missing default parameter
- **JS Source**: `src/js/casc/cdn-resolver.js` line 32
- **Status**: Pending
- **Details**: C++ at line 194 missing default parameter `product = 'wow'`.

### 35. [db2.cpp] Lazy-parse Proxy behavior lost
- **JS Source**: `src/js/casc/db2.js` lines 37–73
- **Status**: Pending
- **Details**: JS uses Proxy to lazy-call `parse()` on first property access. C++ `getTable` at lines 24–36 never calls `parse()` — lazy-parse behavior completely lost.

### 36. [db2.cpp] preload_proxy guard missing
- **JS Source**: `src/js/casc/db2.js` lines 46–53
- **Status**: Pending
- **Details**: JS `preload_proxy` guard throws if not preloaded. C++ has no equivalent guard.

### 37. [listfile.cpp] prepareListfile() doesn't wait for preload
- **JS Source**: `src/js/casc/listfile.js` lines 489–499
- **Status**: Pending
- **Details**: C++ at lines 649–661 doesn't wait for preload to complete, breaking the async contract.

### 38. [listfile.cpp] preload() doesn't deduplicate concurrent callers
- **JS Source**: `src/js/casc/listfile.js` lines 478–487
- **Status**: Pending
- **Details**: JS deduplicates concurrent calls via shared promise. C++ at lines 637–647 doesn't share/return the same future.

### 39. [listfile.cpp] getByID() returns empty string instead of undefined
- **JS Source**: `src/js/casc/listfile.js` lines 778–794
- **Status**: Pending
- **Details**: JS returns `undefined` for missing IDs. C++ at lines 709–728 returns empty string — different semantic.

### 40. [tact-keys.cpp] setImmediate batching replaced with synchronous save
- **JS Source**: `src/js/casc/tact-keys.js` lines 122–127
- **Status**: Pending
- **Details**: JS uses `setImmediate` to batch saves. C++ at lines 76–82 saves synchronously on every `addKey` call — performance regression.

### 41. [tact-keys.cpp] Missing HTTP status check in load()
- **JS Source**: `src/js/casc/tact-keys.js` lines 89–91
- **Status**: Pending
- **Details**: C++ at lines 191–193 doesn't check HTTP status.

### 42. [realmlist.cpp] Missing HTTP status check
- **JS Source**: `src/js/casc/realmlist.js` lines 53–59
- **Status**: Pending
- **Details**: C++ at lines 85–86 missing explicit HTTP status (`res.ok`) check.

### 43. [vp9-avi-demuxer.cpp] find_chunk off-by-one error
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js` line 69
- **Status**: Pending
- **Details**: C++ at line 71 checks one more position than JS, creating an off-by-one error.

### 44. [dbd-manifest.cpp] prepareManifest() return type mismatch
- **JS Source**: `src/js/casc/dbd-manifest.js` lines 50–56
- **Status**: Pending
- **Details**: JS returns `true`. C++ at lines 79–85 returns `void`.

### 45. [blte-reader.cpp] Missing _checkBounds override for lazy block processing
- **JS Source**: `src/js/casc/blte-reader.js` lines 311–321
- **Status**: Pending
- **Details**: JS BLTEReader overrides `_checkBounds()` to lazily decompress BLTE blocks on demand during reads. C++ BLTEReader is missing this override — `_checkBounds` is private non-virtual in BufferWrapper. This causes reads from zeroed memory at call sites in parseEncodingFile, parseRootFile, and getInstallManifest.

---

## Components Files

### 46. [checkboxlist.cpp] mounted()/destroyed() lifecycle hooks are empty stubs
- **JS Source**: `src/js/components/checkboxlist.js`
- **Status**: Pending
- **Details**: C++ `mounted()` at lines 27–28 and `destroyed()` at lines 32–33 are empty — missing ResizeObserver registration, global mouse listener setup, and cleanup.

### 47. [checkboxlist.cpp] propagateClick method is commented out
- **JS Source**: `src/js/components/checkboxlist.js`
- **Status**: Pending
- **Details**: C++ at lines 128, 235–238 has `propagateClick` commented out. ImGui Selectable approximates it but JS clicks child checkbox input with different event propagation.

### 48. [checkboxlist.cpp] scrollIndex doesn't guard against negative results
- **JS Source**: `src/js/components/checkboxlist.js`
- **Status**: Pending
- **Details**: C++ at lines 48–50 doesn't guard against negative results when `items.size() < slotCount`. JS `Array.slice()` handles negatives differently.

---

## Known Issues from Repository Memories

### 49. [casc-source-remote.cpp] Data race in parseArchiveIndex
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 453–529
- **Status**: Pending
- **Details**: 50 concurrent threads write to unprotected `std::unordered_map archives` and call BuildCache methods without mutex. JS `queue()` is single-threaded (Promise concurrency), but C++ `queue()` uses real `std::async` threads.

### 50. [blte-reader.cpp] Three call sites create BLTEReader without processAllBlocks()
- **JS Source**: `src/js/casc/blte-reader.js` lines 311–321
- **Status**: Pending
- **Details**: Call sites in `casc-source.cpp` (parseEncodingFile line 494, parseRootFile line 377, getInstallManifest line 115) create BLTEReader and read without calling `processAllBlocks()`, causing reads from zeroed memory due to missing `_checkBounds` override (see entry 45).

---

## Files Not Yet Audited

The following file groups were not audited due to time constraints and need future review:

### 51. [components/] 16 remaining component files not audited
- **JS Source**: `src/js/components/` (combobox, context-menu, data-table, file-field, home-showcase, itemlistbox, listbox-maps, listbox-zones, listbox, listboxb, map-viewer, markdown-content, menu-button, model-viewer-gl, resize-layer, slider)
- **Status**: Pending
- **Details**: These 16 component files need full comparison against their JS originals.

### 52. [modules/] All module files not audited
- **JS Source**: `src/js/modules/` (font_helpers, legacy_tab_audio, legacy_tab_data, legacy_tab_files, legacy_tab_fonts, legacy_tab_home, legacy_tab_textures, module_test_a, module_test_b, screen_settings, screen_source_select, tab_audio, tab_blender, tab_changelog, tab_characters, tab_creatures, tab_data, tab_decor, tab_fonts, tab_help, tab_home, tab_install, tab_item_sets, tab_items, tab_maps, tab_models, tab_models_legacy, tab_raw, tab_text, tab_textures, tab_videos, tab_zones)
- **Status**: Pending
- **Details**: All 32 module files need full comparison against their JS originals.

### 53. [3D/] All 3D files not audited
- **JS Source**: `src/js/3D/` (AnimMapper, BoneMapper, GeosetMapper, ShaderMapper, Shaders, Skin, Texture, WMOShaderMapper, camera/*, exporters/*, gl/*, loaders/*, renderers/*, writers/*)
- **Status**: Pending
- **Details**: All 52 files in the 3D directory tree need full comparison against their JS originals.

### 54. [db/] All database files not audited
- **JS Source**: `src/js/db/` (CompressionType, DBCReader, DBDParser, FieldType, WDCReader, caches/*)
- **Status**: Pending
- **Details**: All 22 files in the db directory tree need full comparison against their JS originals.

### 55. [ui/] All UI helper files not audited
- **JS Source**: `src/js/ui/` (audio-helper, char-texture-overlay, character-appearance, data-exporter, listbox-context, model-viewer-utils, texture-exporter, texture-ribbon, uv-drawer)
- **Status**: Pending
- **Details**: All 9 UI helper files need full comparison against their JS originals.

### 56. [workers/] cache-collector.cpp not audited
- **JS Source**: `src/js/workers/cache-collector.js`
- **Status**: Pending
- **Details**: Worker file needs full comparison against JS original.

### 57. [mpq/] All MPQ files not audited
- **JS Source**: `src/js/mpq/` (bitstream, build-version, bzip2, huffman, mpq-install, mpq, pkware)
- **Status**: Pending
- **Details**: All 7 MPQ files need full comparison against their JS originals.

### 58. [hashing/] xxhash64.cpp not audited
- **JS Source**: `src/js/hashing/xxhash64.js`
- **Status**: Pending
- **Details**: Hash implementation file needs full comparison against JS original.

### 59. [wow/] EquipmentSlots.cpp and ItemSlot.cpp not audited
- **JS Source**: `src/js/wow/` (EquipmentSlots, ItemSlot)
- **Status**: Pending
- **Details**: Both WoW data files need full comparison against their JS originals.

### 1. [buffer.cpp] Missing fromCanvas() method
- **JS Source**: `src/js/buffer.js` lines 89–106
- **Status**: Pending
- **Details**: JS `BufferWrapper.fromCanvas(canvas, mimeType, quality)` converts an HTML canvas to a buffer via `canvas.toBlob()`. This relies on browser Canvas/Blob APIs that have no direct C++ equivalent. Needs a replacement using the project's image I/O libraries (stb_image_write, libwebp) if canvas-like rendering is ever needed.

### 2. [buffer.cpp] Missing decodeAudio() method
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Status**: Pending
- **Details**: JS `decodeAudio(context)` uses the browser `AudioContext.decodeAudioData()` API to decode audio from a buffer. C++ should use miniaudio for equivalent functionality when audio decoding from buffers is required.

### 3. [blte-reader.cpp] Missing decodeAudio() override
- **JS Source**: `src/js/casc/blte-reader.js` lines 337–340
- **Status**: Pending
- **Details**: JS `BLTEReader.decodeAudio(context)` calls `processAllBlocks()` before delegating to `super.decodeAudio(context)`. This override ensures all BLTE blocks are decoded before audio data is consumed. Blocked until `BufferWrapper.decodeAudio()` is implemented (TODO #2).

### 4. [blte-stream-reader.cpp] Missing createReadableStream() method
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 168–193
- **Status**: Pending
- **Details**: JS `createReadableStream()` creates a browser `ReadableStream` for progressive block consumption with pull-based semantics. No direct C++ equivalent exists. May be implemented as a C++ iterator/generator pattern or callback-based streaming API if needed by consumers.

### 5. [blp.cpp] Missing toCanvas() and drawToCanvas() methods
- **JS Source**: `src/js/casc/blp.js` lines 103–117, 221–234
- **Status**: Pending
- **Details**: JS `toCanvas()` creates an HTML canvas and `drawToCanvas()` renders BLP data onto it using the browser Canvas 2D API. These are browser-specific methods with no direct C++ equivalent. Rendering to Dear ImGui textures is handled separately. Not needed unless direct canvas-style output is required.
