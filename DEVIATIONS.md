# Deviations from JS Source

This file documents all intentional and necessary deviations from the original JavaScript source code. Every deviation from the JS source **must** be recorded here — not in code comments.

Each entry includes the C++ file, the JS source reference, a description of the deviation, and the reason it was necessary.

> **Format:** Entries are grouped by category. Each entry follows this template:
> ```
> ### [file.cpp] Short description
> - **JS Source**: `src/js/original-file.js` lines XX–YY
> - **Reason**: Why the deviation was necessary (C++ language constraint, API difference, etc.)
> - **Impact**: What observable behaviour differs from JS
> ```

---

## Intentionally Removed Files, Features, Options, etc

These files, features, and options have been deliberately removed from the C++ port with no equivalent.

### "Reload Styling" context menu option
- **JS Source**: `src/app.js` lines 160-164, registered at line 550
- **Reason**: JS hot-reloads CSS `<link>` tags with cache-busting query strings — a dev tool with no C++ equivalent since ImGui has no external stylesheets.
- **Impact**: Context menu has one fewer entry. No user-facing functionality loss (was a dev-only feature).

### tab_help module
- **JS Source**: `src/js/modules/tab_help.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Help tab is absent from the C++ app.

### tab_changelog module
- **JS Source**: `src/js/modules/tab_changelog.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Changelog tab is absent from the C++ app.

### markdown-content component
- **JS Source**: `src/js/components/markdown-content.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port. Only consumed by the removed tab_help and tab_changelog modules.
- **Impact**: No standalone impact.

### home-showcase.cpp (intentional stub)
- **JS Source**: `src/js/components/home-showcase.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Home tab shows no showcase content.

### tab_home.cpp (intentional stub)
- **JS Source**: `src/js/modules/tab_home.js`
- **Reason**: Intentionally blank — content not wanted in the C++ port.
- **Impact**: Home tab layout is empty.

### [updater.cpp] Auto-updater removed
- **JS Source**: `src/js/updater.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Application does not check for or apply updates.

### [updater.cpp] Build-time constants instead of runtime manifest
- **JS Source**: `src/js/updater.js` lines 24-25, 33, 35
- **Reason**: JS reads `nw.App.manifest.flavour`/`guid` at runtime. C++ uses compile-time constants.
- **Impact**: Stale GUID/flavour if manifest changes without rebuild.

### [updater.cpp / STLWriter.cpp / GLTFWriter.cpp] "wow.export.cpp" instead of "wow.export"
- **JS Source**: Various — updater.js, STLWriter.js, GLTFWriter.js
- **Reason**: Per project guidelines, user-facing text says "wow.export.cpp".
- **Impact**: Intentional branding change.

## Bug Fixes / Improvements

Cases where C++ intentionally deviates to fix a JS bug or improve behaviour.

### [blp.cpp] alphaDepth=4 integer division fixes JS float indexing bug
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS `index / 2` produces a float (e.g. 3/2=1.5), causing `rawData[1.5]` to return `undefined` for odd indices, yielding incorrect alpha of 0. C++ integer division floors correctly.
- **Impact**: Bug fix — alpha values are now correct for odd indices.

### [blp.cpp] DXT decompression uses `>=` bounds check instead of `===`
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS uses strict equality (`=== pos`) to skip when pos exactly equals rawData length. C++ uses `>=` which also handles pos exceeding rawData size.
- **Impact**: Defensive guard — same behaviour for valid data, safer for corrupt data.

### [blp.cpp] DXT colour interpolation uses integer division vs JS float+clamp
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS stores float64 values and writes to Uint8ClampedArray (round-half-to-even). C++ uses integer division (truncation). Difference is at most 1 LSB, visually imperceptible.
- **Impact**: Sub-LSB colour differences.

### [casc-source.cpp] Encoding page termination uses correct page-size bound
- **JS Source**: `src/js/casc/casc-source.js`
- **Reason**: JS has a bug comparing against `pageStart + pagesStart` instead of `pageStart + cKeyPageSize`. C++ uses the correct page-size bound.
- **Impact**: Bug fix — each page terminates at the proper boundary.

### [tab_raw.cpp] is_dirty set after file identification
- **JS Source**: `src/js/modules/tab_raw.js`
- **Reason**: JS bug — is_dirty was never set to true after identifying files, so compute_raw_files() returned early without refreshing the list. C++ intentionally fixes this.
- **Impact**: Bug fix — list actually updates after file identification.

### [listboxb.cpp] Arrow-key navigation works at index 0
- **JS Source**: `src/js/components/listboxb.js`
- **Reason**: JS `!this.lastSelectItem` is also true for index 0 (`!0 === true`), silently blocking arrow-key navigation. C++ uses `< 0` against sentinel.
- **Impact**: Bug fix — navigation works correctly at index 0.

### [listboxb.cpp] Ctrl+C copies item labels instead of [object Object]
- **JS Source**: `src/js/components/listboxb.js`
- **Reason**: JS `this.selection.join('\n')` on item objects produces `[object Object]` per item. C++ copies `items[idx].label`.
- **Impact**: Bug fix — users get useful clipboard content.

### [buffer.cpp] writeBuffer replicates JS silent error discard
- **JS Source**: `src/js/buffer.js` line 917
- **Reason**: JS creates `new Error(...)` without `throw`, silently discarding. C++ replicates same no-op behaviour for fidelity.
- **Impact**: Same silent behaviour as JS.

### [BONELoader.cpp] Integer division truncation vs JS float division
- **JS Source**: `src/js/3D/loaders/BONELoader.js`
- **Reason**: JS uses float division; non-multiple-of-64 chunkSize would throw RangeError from `new Array(nonInteger)`. C++ truncates silently. Well-formed BONE files always have chunkSize as multiple of 64.
- **Impact**: No real-world impact.

### [WDCReader.cpp] BigInt index arithmetic uses uint64_t
- **JS Source**: `src/js/db/WDCReader.js`
- **Reason**: JS uses BigInt (arbitrary precision) for bitpacked index arithmetic. C++ uses uint64_t which could theoretically overflow for pathologically large values. In practice, pallet indices are well below uint32_t bounds.
- **Impact**: No overflow with valid input.

### [GLTFWriter.cpp] Animation channel target uses actual_node_idx instead of nodeIndex+1
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 620-628, 757-765, 887-895
- **Reason**: JS always uses `nodeIndex + 1` which is a bug when bone prefix mode is disabled. C++ uses correct `actual_node_idx`.
- **Impact**: Intentional bug fix. Only manifests when `modelsExportWithBonePrefix=false` AND `modelsExportAnimations=true`.

## Platform Adaptations

Deviations that are inherent to the C++/ImGui platform and apply broadly across the codebase.

### [generics.cpp] queue() uses polling futures instead of promise .then() chains
- **JS Source**: `src/js/generics.js` — queue() function
- **Reason**: JS uses promise `.then()` chains for zero-latency event-driven dispatch. C++ polls `std::futures` with a 1ms wait to find the first ready one.
- **Impact**: Up to 1ms latency per completion. Same tasks, concurrency limit, and completion order.

### [generics.cpp] Async file operations converted to synchronous
- **JS Source**: `src/js/generics.js` — createDirectory, fileExists, directoryIsWritable, readFile, deleteDirectory
- **Reason**: JS versions are async (return Promises). C++ uses synchronous `std::filesystem` calls.
- **Impact**: Blocking calls on the calling thread. Standard JS-to-C++ async mapping.

### [generics.cpp] redraw() uses thread yield instead of requestAnimationFrame
- **JS Source**: `src/js/generics.js` — redraw()
- **Reason**: JS calls `requestAnimationFrame()` twice to defer until the browser paints two frames. C++ has no rAF equivalent in ImGui; uses `std::this_thread::yield()`.
- **Impact**: No synchronization with the render loop. ImGui renders continuously so yield is sufficient.

### [generics.cpp] batchWork uses sleep_for instead of MessageChannel
- **JS Source**: `src/js/generics.js` — batchWork()
- **Reason**: JS uses MessageChannel to post between batches, yielding to the browser event loop. C++ uses `sleep_for(0)` to yield the thread's timeslice.
- **Impact**: Different yielding mechanism between batches.

### [core.cpp] showLoadingScreen posts to main thread queue
- **JS Source**: `src/js/core.js` — showLoadingScreen()
- **Reason**: JS sets state synchronously on the current event loop turn. C++ posts to main-thread queue for thread safety.
- **Impact**: One-frame delay before state changes are visible.

### [core.cpp] progressLoadingScreen has no explicit forced redraw
- **JS Source**: `src/js/core.js` — progressLoadingScreen()
- **Reason**: JS calls `await generics.redraw()` for immediate UI repaint. C++/ImGui repaints every frame automatically.
- **Impact**: No explicit forced redraw — repaint happens on next frame tick.

### [config.cpp] load() is synchronous
- **JS Source**: `src/js/config.js` — load()
- **Reason**: JS load() is async and awaits file reads. C++ blocks the calling thread during file I/O.
- **Impact**: Blocking call. Standard async-to-sync mapping.

### [config.cpp] No Vue watcher for auto-save on config mutation
- **JS Source**: `src/js/config.js` — load() registers `core.view.$watch('config', ...)`
- **Reason**: C++ has no Vue-style reactive watcher. UI components that change config must explicitly call `config::save()`.
- **Impact**: Config changes can be silently lost if save() is not called.

### [log.h/log.cpp] getErrorDump is synchronous
- **JS Source**: `src/js/log.js` lines 102-108
- **Reason**: JS returns a Promise. C++ reads synchronously — during a crash, the event loop may be unavailable.
- **Impact**: Blocking I/O, intentionally more reliable for crash diagnostics.

### [icon-render.cpp] processQueue is synchronous tail recursion
- **JS Source**: `src/js/icon-render.js`
- **Reason**: JS uses async promise chaining between queue items, yielding to the event loop. C++ processes the entire queue synchronously.
- **Impact**: Large queues may block the calling thread.

### [blp.cpp] toCanvas()/drawToCanvas() absent — replaced by toPNG()/toBuffer()
- **JS Source**: `src/js/casc/blp.js` — toCanvas(), drawToCanvas()
- **Reason**: No HTML Canvas in C++. Pixel-decoding role replaced by toPNG() and direct pixel buffer writing.
- **Impact**: No Canvas element creation. getDataURL() calls toPNG() then BufferWrapper::getDataURL().

### [blte-stream-reader.cpp] ReadableStream uses pull/cancel object instead of Web Streams API
- **JS Source**: `src/js/casc/blte-stream-reader.js`
- **Reason**: JS uses Web Streams API with controller.error()/controller.close(). C++ exposes simpler pull/cancel — errors propagate as exceptions, end-of-stream as std::nullopt.
- **Impact**: Different API shape, same streaming semantics.

### [blte-stream-reader.cpp] streamBlocks uses callback instead of async generator
- **JS Source**: `src/js/casc/blte-stream-reader.js`
- **Reason**: JS uses `async *` generator (yields each block). C++ uses callback — C++20 coroutines are not used in this codebase.
- **Impact**: Inverted control flow. Same ordering and payloads.

### [vp9-avi-demuxer.cpp] extract_frames uses callback instead of async generator
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js`
- **Reason**: JS is an `async*` generator consumed via `for await`. C++ uses callback since BLTEStreamReader::streamBlocks is already callback-based.
- **Impact**: Inverted control flow. Same frame ordering and FrameInfo payloads.

### [model-viewer-gl.cpp] Controls split into typed pointers instead of polymorphic
- **JS Source**: `src/js/components/model-viewer-gl.js`
- **Reason**: JS uses a single polymorphic `context.controls` pointer (duck-typing). C++ splits into `controls_orbit` and `controls_character` because C++ has no duck-typing.
- **Impact**: External callers must check the appropriate typed pointer. Unified `controls_update` callback abstracts over both.

### [model-viewer-gl.cpp] Vue $watch replaced by per-frame polling
- **JS Source**: `src/js/components/model-viewer-gl.js`
- **Reason**: JS uses Vue's `$watch` for immediate reactive callbacks on config changes. C++ records previous values and polls per-frame.
- **Impact**: One-frame latency between config change and callback invocation.

### [map-viewer.cpp] Tile loading limited per frame instead of async concurrency
- **JS Source**: `src/js/components/map-viewer.js`
- **Reason**: JS loads tiles asynchronously (Promises, up to 4 concurrent). C++ tile loading is synchronous, so caps tiles processed per frame at 3. (TODO 284)
- **Impact**: Same throttling effect, different mechanism.

### [map-viewer.cpp] tileHasUnexpectedTransparency samples full tile, not clipped portion
- **JS Source**: `src/js/components/map-viewer.js`
- **Reason**: JS clamps sample rectangle to canvas via getImageData(). C++ samples full cached tile pixel data. (TODO 285)
- **Impact**: Different sample positions for partially off-canvas tiles. Acceptable — function is a heuristic.

### [map-viewer.cpp] GL textures with offset positioning instead of double-buffer blit
- **JS Source**: `src/js/components/map-viewer.js` lines 554-627
- **Reason**: JS uses canvas clearRect/drawImage for double-buffer blit. C++ uses GL textures repositioned by offset. (TODO 287)
- **Impact**: Different rendering mechanism, same visual result.

### [map-viewer.cpp] Map watcher defers default position to next frame
- **JS Source**: `src/js/components/map-viewer.js` lines 143-150
- **Reason**: JS invokes setToDefaultPosition() immediately (Vue has measured DOM). C++ defers via `initialized = false` until ImGui reports viewport width > 0. (TODO 288)
- **Impact**: One-frame delay before default positioning is applied.

### [slider.cpp] Native ImGui::SliderFloat replaces custom slider
- **JS Source**: `src/js/components/slider.js` lines 41-99
- **Reason**: JS implements fully custom slider with mousedown/mousemove/mouseup listeners, fill bar, and draggable handle. C++ uses native ImGui::SliderFloat.
- **Impact**: Different appearance per Visual Fidelity rules. Same value-in-[0,1] semantic.

### [resize-layer.cpp] Float width instead of integer clientWidth
- **JS Source**: `src/js/components/resize-layer.js`
- **Reason**: JS emits `this.$el.clientWidth` (always integer pixels). C++ passes raw float from `ImGui::GetContentRegionAvail().x`.
- **Impact**: Sub-pixel precision preserved. Callers needing integer should cast.

### [tab_videos.cpp] No video end/error detection for external player
- **JS Source**: `src/js/modules/tab_videos.js` — video.onended, video.onerror
- **Reason**: C++ opens an external player via OS shell. No process lifecycle callbacks available.
- **Impact**: User must click "Stop Video" to reset state. HTTP errors caught by kino_post().

### [casc-source-local.cpp / casc-source-remote.cpp] tact_keys::waitForLoad() in load()
- **JS Source**: `src/js/casc/casc-source-local.js`, `src/js/casc/casc-source-remote.js`
- **Reason**: JS load() does not call this; JS app loads tact keys earlier in boot. C++ ensures encryption keys are available before decryption.
- **Impact**: Extra synchronization step. Prevents decryption failures.

### [external-links.cpp] renderLink() is a C++-only addition
- **JS Source**: `src/app.js` — global DOM click handler for `data-external` attributes
- **Reason**: JS uses a global DOM click handler to open elements with `data-external`. ImGui has no DOM, so each call site must invoke renderLink() explicitly.
- **Impact**: Same link-opening behaviour, different invocation pattern.

### [file-writer.h] writeLine is synchronous under mutex
- **JS Source**: `src/js/file-writer.js`
- **Reason**: JS is async — suspends only on Node stream backpressure. C++ std::ofstream has no backpressure concept.
- **Impact**: Writes never suspend the caller. Data always flushed to OS buffer before returning.

### [DBComponentTextureFileData.cpp] condition_variable instead of init_promise
- **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js`
- **Reason**: JS caches an `init_promise` for coalescing concurrent callers. C++ uses `is_initializing` + `std::condition_variable`. (TODO 314)
- **Impact**: Same behaviour — single initialization, all callers wait.

### [DBComponentModelFileData.cpp] condition_variable instead of init_promise
- **JS Source**: `src/js/db/caches/DBComponentModelFileData.js`
- **Reason**: Same as DBComponentTextureFileData entry above.
- **Impact**: Same behaviour.

### [constants.cpp] No macOS UPDATE_ROOT resolution
- **JS Source**: `src/js/constants.js`
- **Reason**: JS on macOS resolves 4 levels up from nw.__dirname for the portable-app root. C++ has no macOS support; ROOT equals INSTALL_PATH on all platforms.
- **Impact**: No macOS support — documented platform constraint.

### [tiled-png-writer.cpp] write() returns immediately via shared_future
- **JS Source**: `src/js/tiled-png-writer.js` lines 123-125
- **Reason**: JS `await tiledWriter.write(path)` guarantees file is on disk before continuing. C++ launches write on `std::async` and returns a `shared_future<void>` immediately.
- **Impact**: Callers must call `.get()` to observe completion or errors. Race condition if callers don't await.

### [GeosetMapper.cpp] map() async signature dropped
- **JS Source**: `src/js/3D/GeosetMapper.js` lines 79-84
- **Reason**: JS declares `async map()` but body contains no `await`. C++ drops the async wrapper since it's unnecessary.
- **Impact**: None in practice — JS function was already synchronous in behaviour.

### [tab_maps.cpp] mounted() defers initialization to render()
- **JS Source**: `src/js/modules/tab_maps.js` lines 1132-1146
- **Reason**: ImGui has no equivalent of Vue `mounted()` lifecycle hook. Lazy-init on first render.
- **Impact**: Timing difference. Brief flash of empty content possible. (TODO 135)

### [char-texture-overlay.cpp] ensureActiveLayerAttached is a no-op (Accepted)
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 63-71
- **Reason**: JS re-appends the active layer canvas to the DOM overlay element if detached. ImGui has no DOM parent/child attachment concept — textures are rendered explicitly, not by being children of a container. The function is a no-op in C++.
- **Impact**: Active layer identity is preserved (matching JS). No membership validation or substitution occurs.

## Type System Adaptations

Deviations caused by C++ static typing vs JS dynamic typing.

### [M3Loader.cpp] ReadBufferAsFormat split into two functions
- **JS Source**: `src/js/3D/loaders/M3Loader.js` lines 239-258
- **Reason**: JS returns a mixed-type Array (floats for F32 formats, uint16s for U16). C++ requires statically typed returns, so split into ReadBufferAsFormat (floats) and ReadBufferAsFormatU16 (uint16s).
- **Impact**: Structurally divergent, functionally equivalent.

### [M2LegacyLoader.cpp] getSkinList returns empty vector instead of undefined
- **JS Source**: `src/js/3D/loaders/M2LegacyLoader.js`
- **Reason**: For WotLK models, JS returns `undefined` (skins not populated). C++ returns empty `std::vector`. Callers check `.size()` so behaviour is equivalent.
- **Impact**: Callers that need unset-vs-empty distinction would need `std::optional`.

### [LoaderGenerics.cpp/.h] processStringBlock returns std::map instead of JS object
- **JS Source**: `src/js/3D/loaders/LoaderGenerics.js`
- **Reason**: JS returns a plain object with numeric-string keys (insertion order). C++ returns `std::map<uint32_t, std::string>` (ascending order). Keys are written in monotonically increasing order so both coincide. All callers use offset-key lookups, never iterate.
- **Impact**: No observable effect. If iteration order becomes important, switch to vector-of-pairs.

### [data-table.cpp] Content-based identity instead of JS reference identity
- **JS Source**: `src/js/components/data-table.js`
- **Reason**: JS uses reference identity for row selection. C++ approximates with content-based identity by joining field values. DB2/DBC rows differ by ID column so collisions are extremely rare.
- **Impact**: Rows with identical content treated as equal, but matches visual outcome.

### [listboxb.cpp] Index-based selection instead of reference-based
- **JS Source**: `src/js/components/listboxb.js`
- **Reason**: JS stores actual item objects and uses `.indexOf(item)` for reference equality. C++ stores integer indices because there is no equivalent of JS reference-equality for arbitrary items.
- **Impact**: All selection logic works with indices. Same semantic.

### [core.h] AppState struct deviations from makeNewView()
- **JS Source**: `src/js/core.js` — makeNewView()
- **Reason**: `constants` field omitted (accessed via `constants::` namespace). `availableLocale` omitted (compile-time constant via `casc::locale_flags::entries`). Additional C++/OpenGL fields added (`mpq`, `chrCustRacesPlayable/NPC`, `pendingItemSlotFilter`, GL texture resources).
- **Impact**: Platform adaptations for C++/OpenGL/ImGui. No functional difference.

### [constants.h] CONFIG.DEFAULT_PATH uses different directory
- **JS Source**: JS uses `INSTALL_PATH/src/default_config.jsonc`
- **Reason**: C++ uses `<install>/data/default_config.jsonc` to match the C++ resource layout.
- **Impact**: Different file path for default config.

### [GLBWriter.h] bin_buffer stored by reference instead of value
- **JS Source**: `src/js/3D/writers/GLBWriter.js`
- **Reason**: JS uses value semantics. C++ stores by reference — GLBWriter must not outlive the BufferWrapper passed to its constructor.
- **Impact**: Lifetime constraint not present in JS.

### [WDCReader.h] idFieldIndex is uint16_t defaulting to 0 instead of null
- **JS Source**: `src/js/db/WDCReader.js`
- **Reason**: JS initializes `idField` and `idFieldIndex` to `null`. C++ uses `std::optional<std::string>` for idField but plain `uint16_t` for idFieldIndex.
- **Impact**: 0 is indistinguishable from valid index 0 before loading. Only consulted after loading begins, so uninitialised value is never observed.

### [UniformBuffer.h] Single private data_ vector instead of three public typed views
- **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 20-22
- **Reason**: JS exposes `view` (DataView), `float_view` (Float32Array), `int_view` (Int32Array) over shared ArrayBuffer. C++ uses single private `std::vector<uint8_t>` with typed setter methods.
- **Impact**: Encapsulation difference. Same bytes reach the GPU.

### [UniformBuffer.cpp] memcpy (native endian) instead of DataView (explicit little-endian)
- **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 51, 60, 70-71, etc.
- **Reason**: JS writes via DataView with explicit little-endian flag. C++ uses `std::memcpy` which writes in host byte order. Both supported platforms (Windows x64, Linux x64) are little-endian, so layout is identical.
- **Impact**: Platform assumption — porting to big-endian would require byte-swap.

### [JSONWriter.cpp] BigInt serialization replacer not ported
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 40-43
- **Reason**: nlohmann::json handles integers up to uint64_t natively. Values exceeding uint64_t must be pre-converted by callers.
- **Impact**: Shifts serialization responsibility to callers.

### [M2LegacyRendererGL.cpp] _load_textures calls setSlotFileLegacy instead of setSlotFile
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` line 241
- **Reason**: C++ splits JS's dynamically-typed `setSlotFile` into two functions: `setSlotFile(uint32_t)` for CASC fileDataIDs and `setSlotFileLegacy(string)` for MPQ file paths. Legacy renderer uses string paths, so `setSlotFileLegacy` is the correct C++ adaptation.
- **Impact**: Functionally identical — same slot data stored.

## Container Ordering

JS `Map`/`Object` preserve insertion order; C++ `std::unordered_map`/`std::map` do not.

### [tiled-png-writer.cpp] tiles map uses std::map (lexicographic order)
- **JS Source**: `src/js/tiled-png-writer.js` lines 25, 58-59
- **Reason**: JS `Map` iterates in insertion order. C++ `std::map<std::string, Tile>` iterates in lexicographic key order.
- **Impact**: Porter-Duff blend output differs when tiles overlap with partial alpha.

### [xml.cpp] build_object uses std::unordered_map for child grouping
- **JS Source**: `src/js/xml.js` lines 138-153
- **Reason**: JS `Object.entries(groups)` yields insertion order. C++ `std::unordered_map` iterates in hash order.
- **Impact**: JSON output key sequence differs from JS for the same XML input.

### [Shaders.cpp] active_programs is std::unordered_map
- **JS Source**: `src/js/3D/Shaders.js` lines 26, 100-122
- **Reason**: JS `Map` insertion order. C++ `std::unordered_map` hash order.
- **Impact**: reload_all recompile order and log line order differ from JS.

### [WMOExporter.cpp] groupNames ordering may differ
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 738, 1193
- **Reason**: JS `Object.values(wmo.groupNames)` yields insertion order. C++ `std::map<uint32_t, std::string>` iterates in ascending key order.
- **Impact**: Meta JSON `groupNames` array order may differ.

### [ADTExporter.cpp] foliage JSON key order
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1474-1479
- **Reason**: JS `JSON.stringify` emits the entire DB row in insertion order. C++ enumerates fields via `unordered_map`.
- **Impact**: JSON key ordering differs.

## Data Type / Sentinel Value Differences

JS dynamic types (NaN, undefined, null) don't map 1:1 to C++ static types.

### [subtitles.cpp] parse_sbt_timestamp returns 0 instead of NaN for malformed input
- **JS Source**: `src/js/subtitles.js` lines 8-22
- **Reason**: JS `parseInt` returns `NaN` which propagates through arithmetic. C++ returns `std::nullopt` mapped to 0.
- **Impact**: Malformed timestamps produce 0 in C++ vs NaN in JS.

### [wmv.cpp] parse_legacy returns -1 sentinel instead of NaN
- **JS Source**: `src/js/wmv.js` lines 87-92
- **Reason**: JS `parseInt("abc")` returns NaN. C++ `safe_parse_int` returns `std::nullopt`, mapped to -1 via `value_or(-1)`.
- **Impact**: Callers inspecting the int directly see -1 vs NaN. Practical effect is similar (no choice picked).

### [ShaderProgram.cpp] get_uniform_block_param returns -1 instead of null
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 122-127
- **Reason**: JS returns `null` for `INVALID_INDEX`. C++ returns `-1` (valid GLint).
- **Impact**: Callers cannot distinguish "block missing" from legitimate -1.

### [ADTExporter.cpp] scale 0 maps to 1.0 instead of preserving 0
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1270
- **Reason**: JS uses `model.scale !== undefined ? model.scale / 1024 : 1`. C++ uses `model.scale != 0.0f` as the gate, conflating absent with zero.
- **Impact**: Explicit scale 0 yields 0/1024=0 in JS but 1.0 in C++.

### [M2Exporter.cpp] data-texture fileDataID not back-patched
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 153-168
- **Reason**: JS back-patches `texture.fileDataID = 'data-' + textureType` (string). C++ `Texture::fileDataID` is `uint32_t`, cannot hold a string.
- **Impact**: Meta JSON `textures[i].fileDataID` differs for data-texture rows.

### [DBCharacterCustomization.cpp] getFileDataIDByDisplayID maps nullopt to 0
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 128-130
- **Reason**: JS stores raw `undefined` from lookup. C++ uses `.value_or(0)`, converting no-value to 0.
- **Impact**: Downstream callers see `std::optional(0)` instead of `nullopt`. FileDataID 0 is generally invalid so practical impact is low.

### [tab_models.cpp] Drop handler wraps files as JSON objects instead of strings
- **JS Source**: `src/js/modules/tab_models.js` lines 587, 213
- **Reason**: C++ wraps paths in `{"fileName": path}` objects. JS passes plain strings.
- **Impact**: Every dropped file fails to export — `file_entry.get<std::string>()` throws on object. (TODO 138)

### [tab_blender.cpp] checkLocalVersion comparison differs in error case
- **JS Source**: `src/js/modules/tab_blender.js` line 163
- **Reason**: JS `NaN > NaN = false` → no update toast. C++ `"1.2.3" > "" = true` → shows update toast.
- **Impact**: C++ prompts for update when installed version can't be read; JS does not. (TODO 120)

## API Signature Differences

C++ function signatures that differ from the JS originals.

### [M2Exporter.cpp] addURITexture takes BufferWrapper instead of dataURI string
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59-61, 111
- **Reason**: C++ caller pre-decodes; JS stores base64 data URI string and decodes at export time.
- **Impact**: API contract differs. JS regex strip of data-URI prefix is not preserved.

### [M3Exporter.cpp] addURITexture stores BufferWrapper instead of dataURI string
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49-51
- **Reason**: Same as M2Exporter entry above — C++ uses raw bytes instead of base64 string.
- **Impact**: Consumers reading `dataTextures` receive raw bytes instead of data URI.

### [ShaderProgram.cpp] set_uniform_3fv/4fv/mat4_array add explicit count parameter
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 204-208, 214-218, 247-251
- **Reason**: JS infers count from `values.length`. C++ `const float*` has no length — callers must pass count.
- **Impact**: Default count=1 if callers don't specify. API contract differs.

### [UniformBuffer.cpp] set_float_array requires explicit count
- **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 152-158
- **Reason**: Same as ShaderProgram entry above — `const float*` has no length.
- **Impact**: Same as ShaderProgram entry above.

### [DBItemGeosets.cpp] getDisplayId missing modifier_id parameter
- **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 277-279
- **Reason**: Omission — JS accepts optional `modifier_id`, C++ only accepts `item_id`.
- **Impact**: C++ always uses default modifier resolution.

### [DBItemModels.cpp] getItemModels missing modifier_id parameter
- **JS Source**: `src/js/db/caches/DBItemModels.js` lines 141-142
- **Reason**: Same omission as DBItemGeosets entry above.
- **Impact**: C++ always uses default modifier resolution.

### [Texture.cpp] getTextureFile calls getVirtualFileByID instead of getFile
- **JS Source**: `src/js/3D/Texture.js` lines 32-41
- **Reason**: C++ `CASC::getFile` returns encoding key string, not decoded data. `getVirtualFileByID` returns decoded data.
- **Impact**: Structural deviation; verify BLTE decoding, partial-decrypt defaults, fallback handling match.

### [tab_creatures.cpp] Missing file_name parameter in create_renderer
- **JS Source**: `src/js/modules/tab_creatures.js` line 652
- **Reason**: C++ passes `file_data_id` but omits `file_name` as 6th argument.
- **Impact**: WMO creatures may fail to load group files. (TODO 125)

## File Path / String Processing

Differences in how file paths, strings, and collation are processed.

### [WMOLegacyExporter.cpp] ASCII-only tolower instead of Unicode toLowerCase
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 299, 310, 507, 510
- **Reason**: C++ `std::tolower` only handles ASCII bytes. JS `toLowerCase()` is Unicode-aware.
- **Impact**: Non-ASCII characters in paths would cause cache misses / duplicate exports. WoW asset paths are ASCII so practical impact is negligible.

### [tab_audio.cpp] parent_path() returns "" instead of "." for bare filenames
- **JS Source**: `src/js/modules/tab_audio.js` lines 155-158
- **Reason**: Node.js `path.dirname("file.ogg")` returns `"."`. C++ `parent_path()` returns `""`. C++ code checks both `dir.empty()` and `dir == "."` to handle both cases.
- **Impact**: None — both code paths produce the same result after the fix.

### [tab_fonts.cpp] Same parent_path() vs dirname() mismatch
- **JS Source**: `src/js/modules/tab_fonts.js` lines 124-126
- **Reason**: Same as tab_audio entry above.
- **Impact**: Same as tab_audio entry above. (TODO 132)

### [tab_creatures.cpp] ASCII tolower + byte comparison instead of localeCompare
- **JS Source**: `src/js/modules/tab_creatures.js`
- **Reason**: JS uses `localeCompare` (Unicode-aware, locale-sensitive ICU collation). C++ performs byte-wise comparison on lowercased UTF-8. A faithful port would require ICU, not a project dependency.
- **Impact**: May diverge for non-ASCII characters. Creature names are predominantly ASCII.

### [subtitles.cpp] BOM check uses raw 3-byte UTF-8 check
- **JS Source**: `src/js/subtitles.js` lines 177-178
- **Reason**: JS checks decoded UTF-16 code unit `0xFEFF`. C++ checks literal 3-byte UTF-8 BOM (EF BB BF).
- **Impact**: Functionally equivalent for UTF-8 input.

## Rendering / Canvas Operations

WebGL/Canvas APIs mapped to OpenGL/FBO equivalents.

### [ADTExporter.cpp] FBO + pixel rotation instead of canvas rotate+composite
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 891-1161
- **Reason**: No HTML Canvas in C++. Uses offscreen FBO with index-swap rotation.
- **Impact**: JS canvas bilinear resampling on drawImage vs C++ plain index swap. Slight edge differences.

### [ADTExporter.cpp] stb_image_resize instead of canvas drawImage
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 861-890
- **Reason**: No HTML Canvas. Uses `stbir_resize_uint8_linear`.
- **Impact**: Slightly different interpolation results.

### [GLContext.cpp] ext_float_texture hard-coded to true
- **JS Source**: `src/js/3D/gl/GLContext.js` lines 61-62
- **Reason**: JS probes `gl.getExtension('EXT_color_buffer_float')`. C++ assumes true since float textures are core in desktop GL 3.0+.
- **Impact**: Flag is not actually probed. Correct for all supported platforms (Windows/Linux x64 with GL 4.3+).

### [GLTexture.cpp] set_canvas takes raw pixels instead of HTMLCanvasElement
- **JS Source**: `src/js/3D/gl/GLTexture.js` lines 52-73
- **Reason**: No HTMLCanvasElement in C++. Takes `(pixels, w, h, options)` and forwards to set_rgba.
- **Impact**: Callers must rasterize content to RGBA bytes themselves.

### [tab_maps.cpp] export_map_wmo_minimap tile compositing ignores draw offsets
- **JS Source**: `src/js/modules/tab_maps.js` lines 723-733
- **Reason**: JS uses `ctx.drawImage` with draw_x/draw_y offsets. C++ samples directly without offsets.
- **Impact**: Tiles drawn at (0,0) instead of correct position. Incorrect WMO minimap exports. (TODO 133)

### [tab_maps.cpp] load_wmo_minimap_tile uses blp.width instead of scaledWidth
- **JS Source**: `src/js/modules/tab_maps.js` lines 103-104
- **Reason**: C++ uses `blp.width`. JS uses decoded canvas width which may differ from raw BLP width.
- **Impact**: Compositing may sample incorrectly if mipmapped. (TODO 134)

## UBO / Bone Animation

Bone Uniform Buffer Object creation and binding order differences.

### [M2LegacyRendererGL.cpp] _create_bones_ubo method missing entirely
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 389, 474-477, 1020
- **Reason**: UBO pipeline not yet implemented in C++ for legacy renderer.
- **Impact**: Bone animation will not work until implemented.

### [M2RendererGL.cpp] Bones UBO created before skeleton is loaded
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 406-439, 567
- **Reason**: C++ creates UBO in `load()` before `_create_skeleton()`. JS creates it in `loadSkin()` after skeleton setup.
- **Impact**: UBO sized for 0 bones. Bone animation may fail.

### [M3RendererGL.cpp] _create_bones_ubo moved from loadLOD() to load()
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 79-81, 147
- **Reason**: C++ creates UBO once in `load()`. JS creates it per-`loadLOD()` call.
- **Impact**: Switching LOD levels won't recreate the UBO.

### [MDXRendererGL.cpp] Bone UBO created before skeleton/geometry
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 189-199, 262-265, 291
- **Reason**: C++ creates UBO in `load()` before skeleton. JS creates it inside `_build_geometry` after skeleton.
- **Impact**: UBO always sized for 0 bones.

### [M3RendererGL.cpp] UBO bind(0) not called per draw call
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` line 292
- **Reason**: C++ binds UBO once before loop. JS rebinds per draw call.
- **Impact**: Could break with intervening UBO binds.

### [M2RendererGL.cpp] _dispose_skin does not dispose bone UBOs
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1914-1922
- **Reason**: C++ only disposes UBO in final `dispose()`, not in `_dispose_skin()`.
- **Impact**: Old UBOs not cleaned up if loadSkin called multiple times.

### [M3RendererGL.cpp] UBO disposal missing from _dispose_geometry
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 310-311, 316
- **Reason**: C++ `_dispose_geometry()` has no UBO disposal.
- **Impact**: Same as _dispose_skin entry above.

### [MDXRendererGL.cpp] Missing per-draw-call UBO bind
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 758
- **Reason**: C++ binds once before loop. JS rebinds per draw call.
- **Impact**: Likely functionally equivalent but deviates from JS structure.

### [M3RendererGL.cpp] load() execution order differs
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 59-71
- **Reason**: C++ creates bones UBO before default texture. JS creates default texture first.
- **Impact**: Initialization order deviation.

## Exporter Structure Differences

Export pipeline structural deviations.

### [M2Exporter.cpp] cancellation returns partial struct instead of undefined
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 142-143
- **Reason**: C++ returns partial `M2ExportTextureResult`. JS returns `undefined`.
- **Impact**: Callers proceed with empty maps instead of dereferencing undefined.

### [M2Exporter.cpp] exportTextures early-return always returns same struct shape
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 95-96
- **Reason**: JS returns `glbMode ? { validTextures, texture_buffers, files_to_cleanup } : validTextures` — shape changes by mode. C++ always returns a single `M2ExportTextureResult` struct.
- **Impact**: Functionally equivalent since all C++ callers access fields through the struct uniformly.

### [ADTExporter.cpp] useADTSets hardcoded to false
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1300-1351
- **Reason**: C++ hardcodes `false`. JS evaluates `model & 0x80` (always 0 for JS objects, but should check binary flag).
- **Impact**: Doodad-set-from-ADT branch is dead code.

### [ADTExporter.cpp] Liquid export pushes nullptr for chunks with no instances
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1409-1411
- **Reason**: JS preserves the chunk object (including attributes). C++ pushes nullptr.
- **Impact**: Chunk's `attributes` field is lost.

### [M2LegacyExporter.cpp] subMesh JSON drops fields beyond hardcoded list
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 206-210
- **Reason**: JS `Object.assign` copies all properties. C++ manually lists 13 fields.
- **Impact**: Additional loader properties silently dropped from meta JSON.

### [tab_creatures.cpp] Missing export_paths and file_manifest
- **JS Source**: `src/js/modules/tab_creatures.js` lines 954-969
- **Reason**: C++ doesn't set `opts.file_manifest` or `opts.export_paths`.
- **Impact**: Standard creature exports won't have paths in export stream. (TODO 126)

### [tab_creatures.cpp] OBJ/STL uses combined function instead of separate calls
- **JS Source**: `src/js/modules/tab_creatures.js` lines 922-927
- **Reason**: C++ uses single `exportAsOBJ(path, isST=true)` instead of `exportAsSTL()`.
- **Impact**: Structural deviation. Functionally identical if combined function is correct. (TODO 127)

### [M2LegacyRendererGL.cpp] materials/textureUnits/boundingBox JSON drops fields
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 244, 248, 250, 254
- **Reason**: C++ rebuilds with hardcoded subset instead of serializing all properties.
- **Impact**: Extra fields from loader dropped from meta JSON.

### [WMOExporter.cpp] formatUnknownFile call signature differs
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 158-160
- **Reason**: JS uses single-arg form. C++ uses two-arg form with separate ID and extension.
- **Impact**: Could drift if helper's two-arg behaviour differs.

### [WMOExporter.cpp] exportRaw uses byteLength()==0 instead of "is data set" check
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 1223-1231
- **Reason**: JS checks `this.wmo.data === undefined`. C++ checks `data.byteLength() == 0`.
- **Impact**: Conflates "no data" with "empty data".

### [M3Exporter.cpp] OBJ texture-manifest reads value as string
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 145-147
- **Reason**: JS value is object with `matPath` field. C++ value is plain string.
- **Impact**: Will diverge once exportTextures is implemented.

### [M3Exporter.cpp] exportAsGLTF reshapes texture map
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 91-92
- **Reason**: C++ stringifies numeric key and wraps in `GLTFTextureEntry` adapter.
- **Impact**: Conversion shape needs verification once M3 texture export is implemented.

### [ADTExporter.cpp] saveRawLayerTexture returns void instead of path
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1164-1190
- **Reason**: JS returns relative path. C++ returns void. Callers ignore return value in JS.
- **Impact**: Cosmetic — no behaviour change unless future caller uses return value.

## Loader / Parser Differences

### [MDXLoader.cpp] parse_ATCH reads KVIS data that JS skips
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` lines 387-412
- **Reason**: JS has a bug where `readUInt32LE(-4)` makes the KVIS branch dead code. C++ deliberately reads KVIS.
- **Impact**: `attachment.visibilityAnim` is populated in C++ but always empty in JS. Intentional improvement.

### [MDXLoader.cpp] Node-to-nodes registration order differs
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` lines 208-210, 53-56
- **Reason**: JS registers in chunk-appearance order. C++ defers to end of `load()` with fixed iteration order.
- **Impact**: Different node wins on objectId collision.

### [WMOLegacyLoader.cpp] fileID=0 skips listfile lookup
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 22-30
- **Reason**: JS `fileID !== undefined` accepts 0. C++ `fileID != 0` rejects 0.
- **Impact**: Different fileName resolution for fileDataID 0 (not a real WoW asset).

### [WMOLoader.cpp] Same fileID=0 deviation
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 18-32
- **Reason**: Same as WMOLegacyLoader entry above.
- **Impact**: Same as WMOLegacyLoader entry above.

### [DBCReader.cpp] _read_field reads 8 bytes for Int64/UInt64
- **JS Source**: `src/js/db/DBCReader.js` lines 389-408
- **Reason**: JS has no Int64/UInt64 case — falls through to default (4 bytes). C++ explicitly handles 8-byte reads.
- **Impact**: Record offset misalignment if 64-bit field encountered in DBC. Intentional improvement. (TODO 109)

## Ownership / Lifecycle

### [Shaders.cpp] create_program splits ownership (unique_ptr + raw pointer)
- **JS Source**: `src/js/3D/Shaders.js` lines 56-72
- **Reason**: JS GC keeps the program alive as long as any reference holds it. C++ unique_ptr transfers ownership to caller while raw pointer stays in active_programs.
- **Impact**: Dangling pointer risk if caller's unique_ptr goes out of scope before unregister.

### [Shaders.cpp] create_program installs _unregister_fn callback
- **JS Source**: `src/js/3D/Shaders.js` lines 56-72
- **Reason**: Added to compensate for the split ownership above. No JS equivalent.
- **Impact**: Destroying unique_ptr auto-deregisters. JS pinned programs survive indefinitely.

## Control Flow / Error Handling

### [ShaderProgram.cpp] _compile deletes successfully-compiled shader on error
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 35-36
- **Reason**: JS leaks the successful shader on compile failure. C++ explicitly deletes both.
- **Impact**: Intentional fix for a JS leak — behavioural deviation in failure path.

### [M2LegacyExporter.cpp] exportAsOBJ/STL throw explicit error on null skin
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 129, 268
- **Reason**: JS would crash with TypeError on property access. C++ throws descriptive `runtime_error`.
- **Impact**: Different error type/message in failure path.

### [WMOExporter.cpp] empty-UV branch defensively allocates uv_maps[0]
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 316-321
- **Reason**: JS `splice(-1, 1)` on missing canvas removes last element (latent bug). C++ handles missing entry gracefully.
- **Impact**: Intentional fix — JS would crash, C++ allocates zeros.

### [char-texture-overlay.cpp] remove() handles missing elements differently (Accepted)
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 46-61
- **Reason**: JS `splice(indexOf(canvas), 1)` with a missing canvas yields `splice(-1, 1)`, which removes the last element — an unintentional JS quirk. C++ uses `std::find` and only erases if found, which is the correct behaviour.
- **Impact**: Removing a non-existent element is a no-op in C++ vs removing the last layer in JS. Accepted as the C++ behaviour is correct.

### [CameraControlsGL.cpp] dispose() resets state to STATE_NONE
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 218-221
- **Reason**: Extra safeguard not in JS. JS leaves state untouched on dispose.
- **Impact**: Benign — controller disposed during drag retains state in JS, resets in C++.

### [tab_maps.cpp] export_map_wmo_minimap condition differs
- **JS Source**: `src/js/modules/tab_maps.js` lines 695-697
- **Reason**: C++ adds extra `worldModel.empty()` check not in JS.
- **Impact**: C++ may attempt setup_wmo_minimap when JS would not. (TODO 137)

### [tab_characters.cpp] Extra re-entry guard in load_character_model
- **JS Source**: `src/js/modules/tab_characters.js` lines 718-720
- **Reason**: C++ adds `if (view.chrModelLoading) return;` guard not in JS.
- **Impact**: Could silently skip loads when another model is already loading. (TODO 123)

### [tab_characters.cpp] navigate_to_items_for_slot skips checkbox update
- **JS Source**: `src/js/modules/tab_characters.js` lines 2499-2516
- **Reason**: C++ relies on `pendingItemSlotFilter` instead of directly toggling checkboxes.
- **Impact**: Previously loaded Items tab filter may not update. (TODO 124)

## UI Text / String Differences

### [legacy_tab_audio.cpp] unittype is "sound" instead of "sound file"
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 204
- **Reason**: Omission.
- **Impact**: Status bar shows "42 sounds found" instead of "42 sound files found". (TODO 114)

### [legacy_tab_audio.cpp] persistscrollkey is "legacy-sounds" instead of "sounds"
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 204
- **Reason**: Renamed, possibly intentionally for disambiguation.
- **Impact**: Scroll positions use different key. (TODO 115)

### [legacy_tab_textures.cpp] persistscrollkey is "legacy-textures" instead of "textures"
- **JS Source**: `src/js/modules/legacy_tab_textures.js` line 117
- **Reason**: Same as legacy_tab_audio entry above.
- **Impact**: Scroll positions use different key. (TODO 116)

### [legacy_tab_textures.cpp] Toast button "View Log" instead of "view log"
- **JS Source**: `src/js/modules/legacy_tab_textures.js` line 68
- **Reason**: Casing difference.
- **Impact**: Button text differs. (TODO 117)

### [module_test_b.cpp] isBusy shows boolean text instead of numeric counter
- **JS Source**: `src/js/modules/module_test_b.js` line 9
- **Reason**: C++ renders bool string. JS renders integer counter value.
- **Impact**: Debug display shows "true"/"false" instead of 0/1/2. (TODO 118)

### [tab_characters.cpp] Toast message text differs
- **JS Source**: `src/js/modules/tab_characters.js` line 781
- **Reason**: Different wording and omits file data ID.
- **Impact**: User sees different error message. (TODO 122)

### [tab_fonts.cpp] Missing "Filter fonts..." placeholder text
- **JS Source**: `src/js/modules/tab_fonts.js` line 61
- **Reason**: Uses `InputText` instead of `InputTextWithHint`.
- **Impact**: No placeholder shown in empty filter box. (TODO 131)

### [tab_blender.cpp] checkLocalVersion() logs error string instead of "[object Object]"
- **JS Source**: `src/js/modules/tab_blender.js` line 161
- **Reason**: JS `%s` on an error object produces `"[object Object]"`. C++ logs the actual error string (e.g. `"file_not_found"`), which is more informative.
- **Impact**: Log output differs but is strictly more useful in C++.

### [listbox.cpp] Bold font weight not replicated
- **JS Source**: `src/js/components/listbox.js` — CSS `.list-status { font-weight: bold }`
- **Reason**: Only one font is loaded. Per Visual Fidelity rules, exact font weights need not match.
- **Impact**: Status text is not bold.

## Extra API Surface

C++ additions with no JS counterpart.

### [WMOExporter.cpp/.h] loadWMO() and getDoodadSetNames()
- **JS Source**: No counterpart
- **Reason**: C++-only UI-layer convenience methods. JS accesses WMO internals directly.
- **Impact**: Extra public API surface.

### [WDCReader.cpp] getAllRowsAsync()
- **JS Source**: No counterpart (JS is single-threaded)
- **Reason**: C++-only async wrapper. Returns result by value for thread safety.
- **Impact**: Extra public API surface.

### [menu-button.h] `upward` parameter
- **JS Source**: `src/js/components/menu-button.js` — props: `['options', 'default', 'disabled', 'dropdown']`
- **Reason**: C++-only extension for popup-above-button positioning needed by some ImGui call sites.
- **Impact**: Extra parameter. The no-upward overload preserves the JS contract.

### [itemlistbox.h] Dead prop and extra emit noted
- **JS Source**: `src/js/components/itemlistbox.js` lines 19-20
- **Reason**: JS declares `includefilecount` prop (dead code, never used by template). JS declares `emits: ['update:selection', 'equip']` but template also emits `'options'`. C++ mirrors rendered template behaviour.
- **Impact**: Dead JS prop not implemented. Extra emit matches actual JS template behaviour.

### [blp.h] dataURL cache field omitted
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS sets `this.dataURL = null` as a cache field but `getDataURL()` never writes to it (every call regenerates the PNG). Dead field omitted in C++.
- **Impact**: No functional difference.

### [blp.cpp] toBuffer returns empty BufferWrapper for unknown encodings
- **JS Source**: `src/js/casc/blp.js`
- **Reason**: JS has no default case (returns undefined). C++ returns empty BufferWrapper for type safety.
- **Impact**: Callers get empty buffer instead of undefined-equivalent.

### [subtitles.cpp] get_subtitles_vtt exposes extra public symbol
- **JS Source**: `src/js/subtitles.js` lines 172-187
- **Reason**: `get_subtitles_vtt_from_text` is exposed in C++ header but not exported in JS.
- **Impact**: Extra API surface.

### [DBCReader.cpp] loadSchema() has filesystem fallback cache paths
- **JS Source**: `src/js/db/DBCReader.js` lines 176-199
- **Reason**: C++ adds fallback that reads/writes DBD files from filesystem when no CASC source available.
- **Impact**: Extra code paths not in JS. (TODO 108)

### [DBItemDisplays.cpp] Extra getTexturesByDisplayId function
- **JS Source**: `src/js/db/caches/DBItemDisplays.js` (no counterpart)
- **Reason**: C++ adds API surface not in JS original.
- **Impact**: Extra exported function. (TODO 111)

## Defensive Guards

### [tab_decor.cpp] Null-guard on GL context before create_renderer
- **JS Source**: `src/js/modules/tab_decor.js` lines 73-74
- **Reason**: JS uses optional chaining (`?.gl_context`) and passes possibly-null to `create_renderer`. C++ `create_renderer` takes `gl::GLContext&` (reference), so null would crash. Guard with error toast is necessary.
- **Impact**: C++ shows error toast and returns early when GL context is unavailable; JS would pass null and potentially create a broken renderer.

### [tab_decor.cpp] fitCamera called synchronously instead of deferred via requestAnimationFrame
- **JS Source**: `src/js/modules/tab_decor.js` line 112
- **Reason**: JS `requestAnimationFrame()` defers to the next browser animation frame. ImGui is immediate-mode — calling synchronously is equivalent since the renderer picks up state changes on the next frame automatically.
- **Impact**: None in practice — timing difference is functionally equivalent in ImGui.

### [tab_decor.cpp] export_decor skips non-string selection entries
- **JS Source**: `src/js/modules/tab_decor.js` lines 497-506
- **Reason**: JS `return entry` passes non-string entries through unchanged. In practice, `selectionDecor` only ever contains formatted display strings; the non-string fallback is dead code. C++ correctly processes string entries and skips the unreachable non-string path.
- **Impact**: None — dead code path in both JS and C++.

### [tab_maps.cpp] collect_game_objects uses std::vector instead of Set
- **JS Source**: `src/js/modules/tab_maps.js` lines 127-144
- **Reason**: JS stores game objects in `Map<number, Set>` where `Set` provides reference-identity uniqueness. C++ uses `std::unordered_map<uint32_t, std::vector<db::DataRecord>>`. `DataRecord` is a `std::map<std::string, FieldValue>` with no `std::hash` specialisation, so `std::unordered_set` cannot be used. JS `Set` deduplicates by reference identity (not value equality), and DB2 rows are iterated once with no duplicates, so the vector is functionally equivalent.
- **Impact**: None in practice — DB2 rows are unique objects from `getAllRows()`.

### [tab_models_legacy.cpp] Missing stack trace log line in preview_model error handler
- **JS Source**: `src/js/modules/tab_models_legacy.js` lines 186-187
- **Reason**: JS logs `e.stack` as a second line after the error message. C++ exceptions do not carry stack traces without a third-party library (e.g. Boost.Stacktrace). Adding such a dependency is not justified for a single log line.
- **Impact**: Slightly less diagnostic information on preview failure in C++. The error message itself is still logged.

## Resolved Deviations

These deviations have been fixed and are retained for historical reference.

### ~~[context-menu.cpp] Click-to-close checks all mouse buttons~~ RESOLVED
- **JS Source**: `src/js/components/context-menu.js` line 54
- **Reason**: Fixed — now only checks left-click, matching Vue `@click` behaviour.

### ~~[item-picker-modal.cpp] open_items_tab erroneously closes modal~~ RESOLVED
- **JS Source**: `src/js/components/item-picker-modal.js` lines 143-145
- **Reason**: Fixed — removed erroneous close_modal() call; now only emits event like JS.

### ~~[item-picker-modal.cpp] Missing click-outside-to-close on backdrop~~ RESOLVED
- **JS Source**: `src/js/components/item-picker-modal.js` line 161
- **Reason**: Fixed — added manual click-outside detection on the backdrop overlay.

### ~~[db2.cpp] Missing "rows === null" preload guard~~ RESOLVED
- **JS Source**: `src/js/db/db2.js` lines 51-52
- **Reason**: Fixed — added rows.has_value() check in WDCReader::getRelationRows().
