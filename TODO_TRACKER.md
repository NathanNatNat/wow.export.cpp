# TODO Tracker

> **Progress: 1/883 verified (0%)** — ✅ = Verified, ⬜ = Pending

- [ ] 1. [app.cpp] Auto-updater flow from app.js is not ported

- [ ] 2. [app.cpp] Drag enter/leave overlay behavior differs from JS

- [ ] 3. [app.cpp] Crash screen heading text differs from original JS

- [ ] 4. [app.cpp] Crash screen missing logo background element

- [ ] 5. [app.cpp] Crash screen version/flavour/build color differs from JS CSS

- [ ] 6. [app.cpp] Crash screen error text styling does not match JS CSS

- [ ] 7. [app.cpp] ScaleAllSizes(1.5f) has no JS equivalent and alters all UI metrics

- [ ] 8. [app.cpp] handleContextMenuClick accesses opt.handler instead of opt.action?.handler

- [ ] 9. [app.cpp] click() disabled check uses bool parameter instead of DOM class check

- [ ] 10. [app.cpp] JS source_select.setActive() called twice; C++ only calls once

- [ ] 11. [app.cpp] whats-new.html path resolution differs from JS

- [ ] 12. [app.cpp] Vue error handler uses 'ERR_VUE' error code; C++ render catch uses 'ERR_RENDER'

- [ ] 13. [app.cpp] JS `data-kb-link` click handler for help articles not ported

- [ ] 14. [app.cpp] JS `showDevTools()` for debug builds has no C++ equivalent

- [ ] 15. [core.cpp] Loading screen updates are deferred to a main-thread queue instead of immediate state writes

- [ ] 16. [core.cpp] progressLoadingScreen no longer awaits a forced redraw

- [ ] 17. [core.cpp] Toast payload shape differs from JS `options` object contract

- [ ] 18. [core.cpp] isDev uses NDEBUG instead of JS BUILD_RELEASE flag

- [ ] 19. [core.cpp] openInExplorer() is a C++ addition with no direct JS equivalent

- [ ] 20. [config.cpp] Deep config watcher auto-save path from Vue is not equivalent

- [ ] 21. [config.cpp] save scheduling differs from setImmediate event-loop behavior

- [ ] 22. [config.cpp] doSave write failure behavior differs

- [ ] 23. [config.cpp] EPERM detection logic differs from JS exception code check

- [ ] 24. [config.cpp] doSave() array comparison uses value equality instead of JS reference equality

- [ ] 25. [config.cpp] save() std::async discarded future blocks in destructor making save synchronous

- [ ] 26. [constants.h/constants.cpp] Version/flavour/build constants are compile-time values

- [ ] 27. [constants.cpp] DATA path root differs from JS nw.App.dataPath behavior

- [ ] 28. [constants.cpp] Cache directory constant changed from casc/ to cache/

- [ ] 29. [constants.cpp] Shader/default-config paths differ from JS layout

- [ ] 30. [constants.cpp] Updater helper extension mapping differs from JS

- [ ] 31. [constants.cpp] Blender base-dir platform mapping omits JS darwin path

- [ ] 32. [constants.cpp] RUNTIME_LOG placed in separate Logs/ subdirectory instead of DATA_PATH root

- [ ] 33. [constants.cpp] Legacy directory migration code in init() has no JS equivalent

- [ ] 34. [constants.cpp] Explicit create_directories() for data and log dirs not present in JS

- [ ] 35. [log.cpp] `write()` API contract differs from JS variadic util.format behavior

- [ ] 36. [log.cpp] Pool drain scheduling differs from JS `drain` event + `process.nextTick`

- [ ] 37. [log.cpp] Log stream initialization timing differs from JS module-load behavior

- [ ] 38. [log.cpp] timeEnd() signature loses JS variadic parameter support

- [ ] 39. [log.cpp] getErrorDump() is synchronous in C++ vs async in JS

- [ ] 40. [updater.cpp] Update manifest flavour/guid source differs from JS runtime manifest

- [ ] 41. [updater.cpp] Async update flow is flattened into synchronous calls

- [ ] 42. [updater.cpp] Launch failure logging omits JS error-object log line

- [ ] 43. [updater.cpp] Linux fork+exec failure path has no error logging unlike JS child.on('error') handler

- [ ] 44. [screen_source_select.cpp] Source selection load flow is no longer Promise-based like JS

- [ ] 45. [screen_source_select.cpp] Hidden directory input reset/click flow is replaced with direct native dialog calls

- [ ] 46. [screen_source_select.cpp] CASC initialization failure toast omits JS support action

- [ ] 47. [screen_source_select.cpp] Missing "Visit Support Discord" toast action button

- [ ] 48. [screen_source_select.cpp] Hardcoded CDN URL format instead of using constants::PATCH::HOST

- [ ] 49. [screen_source_select.cpp] CDN ping intermediate update batched instead of per-ping progressive

- [ ] 50. [screen_settings.cpp] Settings descriptions/help text from JS template are largely omitted

- [ ] 51. [screen_settings.cpp] Cache/listfile interval labels changed from days to hours

- [ ] 52. [screen_settings.cpp] Multi-button style groups are replaced with radio/checkbox controls

- [ ] 53. [screen_settings.cpp] "Manually Clear Cache" heading missing "(Requires Restart)" from JS

- [ ] 54. [screen_settings.cpp] WebP Quality uses `SliderInt` vs JS `<input type="number">`

- [ ] 55. [screen_settings.cpp] Config buttons not visually disabled when busy

- [ ] 56. [screen_settings.cpp] Encryption key inputs don't enforce `maxlength` from JS

- [ ] 57. [screen_settings.cpp] Listfile Source heading missing "(Legacy)" suffix

- [ ] 58. [screen_settings.cpp] Locale dropdown shows full locale names vs JS short locale keys

- [ ] 59. [modules.cpp] Dynamic component registry and hot-reload proxy behavior is not ported

- [ ] 60. [modules.cpp] `wrap_module` computed helper injection differs from JS

- [ ] 61. [modules.cpp] `activated` lifecycle wrapping logic is missing

- [ ] 62. [modules.cpp] `initialize(core_instance)` bootstrap flow differs from JS module wiring

- [ ] 63. [modules.cpp] `set_active` assigns different active-module payload to view state

- [ ] 64. [modules.cpp] Dev hot-reload no longer refreshes module/component code from disk

- [ ] 65. [modules.cpp] `set_active` eagerly initializes modules unlike JS activation flow

- [ ] 66. [modules.cpp] `modContextMenuOptions` payload omits JS `action` object data

- [ ] 67. [modules.cpp] Unknown nav/context entries lose JS insertion-order behavior

- [ ] 68. [modules.cpp] `display_label` in `wrap_module` error messages shows module key instead of nav button label

- [ ] 69. [module_test_b.cpp] Busy-state text formatting differs from JS boolean rendering

- [ ] 70. [module_test_b.cpp] Message char buffer limited to 255 chars vs JS unlimited string

- [ ] 71. [module_test_b.cpp] Uses `logging::write` vs JS `console.log`

- [ ] 72. [module_test_a.cpp] Module UI template structure differs from JS component markup

- [ ] 73. [module_test_a.cpp] Counter is static (persists across mount/unmount) vs JS instance data (resets)

- [ ] 74. [module_test_a.cpp] Uses `logging::write` vs JS `console.log`

- [ ] 75. [blob.cpp] Blob stream semantics differ (async pull stream vs eager callback loop)

- [ ] 76. [blob.cpp] URLPolyfill.createObjectURL native fallback path is missing

- [ ] 77. [blob.cpp] URLPolyfill.revokeObjectURL native revoke path is missing

- [ ] 78. [blob.cpp] BlobPolyfill::slice() treats end=0 differently from JS

- [ ] 79. [buffer.cpp] alloc(false) behavior differs from JS allocUnsafe

- [ ] 80. [buffer.cpp] fromCanvas API/behavior is not directly ported

- [ ] 81. [buffer.cpp] readString encoding parameter does not affect behavior

- [ ] 82. [buffer.cpp] decodeAudio(context) method is missing

- [ ] 83. [buffer.cpp] getDataURL creates data URLs instead of blob URLs

- [ ] 84. [buffer.cpp] readBuffer wrap parameter is split into separate APIs

- [ ] 85. [buffer.cpp] setCapacity(secure=false) always zero-initializes unlike JS Buffer.allocUnsafe

- [ ] 86. [buffer.cpp] startsWith(array) reads entries from sequential positions instead of all from offset 0

- [ ] 87. [generics.cpp] Exported get() API shape differs from JS fetch-style response contract

- [ ] 88. [generics.cpp] get() fallback completion behavior differs when all URLs return non-ok responses

- [ ] 89. [generics.cpp] requestData status/redirect handling differs from JS manual 3xx flow

- [ ] 90. [generics.cpp] redraw() is a no-op instead of double requestAnimationFrame scheduling

- [ ] 91. [generics.cpp] batchWork scheduling model differs from MessageChannel event-loop batching

- [ ] 92. [generics.cpp] get() timeout semantics differ from JS 30-second AbortSignal

- [ ] 93. [generics.cpp] queue() initial batch size off-by-one compared to JS

- [ ] 94. [generics.cpp] fileExists() checks existence only, not accessibility like JS fsp.access

- [ ] 95. [generics.cpp] computeFileHash() has malformed duplicate doc comment block

- [ ] 96. [generics.cpp] downloadFile() error logging loses full error object details

- [ ] 97. [generics.cpp] requestData() is publicly declared but is a private/unexported function in JS

- [ ] 98. [mmap.cpp] Module architecture differs from JS wrapper around `mmap.node`

- [ ] 99. [mmap.cpp] Virtual-file ownership semantics differ from JS object-lifetime model

- [ ] 100. [mmap.cpp] C++ map() explicitly rejects empty files which JS wrapper does not handle

- [ ] 101. [xml.cpp] End-of-input handling can dereference past bounds unlike JS parser semantics

- [ ] 102. [subtitles.cpp] `get_subtitles_vtt` API and data-loading path differ from JS

- [ ] 103. [subtitles.cpp] BOM stripping behavior differs from original JS

- [ ] 104. [subtitles.cpp] Invalid SBT timestamp parsing semantics differ from JS `parseInt` behavior

- [ ] 105. [wmv.cpp] safe_parse_int returns 0 for fully non-numeric strings while JS parseInt returns NaN

- [ ] 106. [external-links.h] Windows open() uses naive wstring conversion instead of proper MultiByteToWideChar

- [ ] 107. [external-links.h] wowHead_viewItem() hardcodes URL string instead of using WOWHEAD_ITEM constant

- [ ] 108. [external-links.h] renderLink() missing CSS a:hover visual effects (color change and underline)

- [ ] 109. [external-links.cpp] JS logic is not implemented in the .cpp translation unit

- [ ] 110. [gpu-info.cpp] macOS GPU info path from JS is missing in C++

- [ ] 111. [gpu-info.cpp] WebGL debug renderer detection logic differs from JS extension-gated behavior

- [ ] 112. [gpu-info.cpp] `exec_cmd` timeout behavior is not equivalent on Windows

- [ ] 113. [gpu-info.cpp] Extension category normalization diverges from JS WebGL formatting

- [ ] 114. [gpu-info.cpp] Caps logging condition differs from JS — C++ uses `max_tex_size > 0` while JS always logs caps

- [ ] 115. [font_helpers.cpp] `detect_glyphs_async` no longer implements JS DOM/callback contract

- [ ] 116. [font_helpers.cpp] Active detection cancellation semantics differ from JS global `active_detection`

- [ ] 117. [font_helpers.cpp] `inject_font_face` return type/behavior differs from JS blob-URL + `document.fonts` flow

- [ ] 118. [font_helpers.cpp] `check_glyph_support` uses fundamentally different detection algorithm

- [ ] 119. [font_helpers.cpp] `inject_font_face` is synchronous and returns `void*` vs JS async returning blob URL string

- [ ] 120. [icon-render.cpp] Icon load pipeline is still stubbed and never replaces placeholders

- [ ] 121. [icon-render.cpp] Queue execution model differs from JS async recursive processing

- [ ] 122. [icon-render.cpp] Dynamic stylesheet/CSS-rule icon path is replaced with non-equivalent texture cache flow

- [ ] 123. [icon-render.h] Truncated doc comment — sentence fragment on line 17

- [ ] 124. [stb-impl.cpp] Required sibling JS source file is missing, blocking parity verification

- [ ] 125. [png-writer.cpp] `write()` call contract differs from JS async behavior

- [ ] 126. [tiled-png-writer.cpp] `write()` contract is synchronous instead of JS Promise-based async

- [ ] 127. [file-writer.cpp] writeLine backpressure/await behavior differs from JS stream semantics

- [ ] 128. [file-writer.cpp] Closed-stream writes are silently ignored unlike JS stream-end behavior

- [ ] 129. [audio-helper.cpp] AudioPlayer::load does not return decoded buffer like JS

- [ ] 130. [audio-helper.cpp] End-of-playback callback is polling-driven instead of event-driven

- [ ] 131. [audio-helper.cpp] get_position() has side effects not present in JS

- [ ] 132. [audio-helper.cpp] detectFileType accepts raw bytes instead of BufferWrapper

- [ ] 133. [audio-helper.cpp] play() checks !engine in addition to empty data

- [ ] 134. [wowhead.cpp] Parse result field name differs from JS API (`class` vs `player_class`)

- [ ] 135. [xxhash64.cpp] Public API contract differs from JS callable-export behavior

- [ ] 136. [casc-source.cpp] `getFileByName` no longer forwards to subclass file-reader path like JS

- [ ] 137. [casc-source.cpp] Base CASC APIs are synchronous instead of JS async methods

- [ ] 138. [casc-source-remote.cpp] Remote CASC lifecycle and data-access methods are synchronous instead of JS async methods

- [ ] 139. [casc-source-remote.cpp] HTTP error detail from remote config requests differs from JS

- [ ] 140. [casc-source-local.cpp] Local CASC public/file-loading methods are synchronous instead of JS async methods

- [ ] 141. [casc-source-local.cpp] Remote CASC initialization region fallback differs from JS behavior

- [ ] 142. [casc-source-local.cpp] `getProductList()` handles missing Branch field gracefully instead of throwing like JS

- [ ] 143. [cdn-resolver.cpp] Resolver API and internal host-resolution flow are synchronous instead of JS async Promise flow

- [ ] 144. [version-config.cpp] Extra data fields beyond header count silently discarded instead of creating JS `undefined` key

- [ ] 145. [realmlist.cpp] `load` API is synchronous instead of JS Promise-based async method

- [ ] 146. [realmlist.cpp] `realmListURL` coercion semantics differ from JS `String(...)` behavior

- [ ] 147. [realmlist.cpp] Remote non-OK handling/logging path differs from JS response contract

- [ ] 148. [blte-reader.cpp] `decodeAudio(context)` API from JS is missing

- [ ] 149. [blte-reader.cpp] `getDataURL()` no longer honors pre-populated `dataURL` short-circuit

- [ ] 150. [blte-reader.cpp] `_handleBlock` encrypted block catch-all swallows all non-EncryptionError exceptions silently

- [ ] 151. [blte-reader.cpp] `decodeAudio()` not ported — browser-specific Web Audio API

- [ ] 152. [blte-reader.cpp] `getDataURL()` caching differs — JS checks `this.dataURL` first, C++ always processes blocks

- [ ] 153. [blte-reader.cpp] `_decompressBlock` passes two bools to `readBuffer()` in JS but only one in C++

- [ ] 154. [blte-stream-reader.cpp] Block retrieval/decode flow is synchronous instead of JS async

- [ ] 155. [blte-stream-reader.cpp] `createReadableStream()` Web Streams API path is missing

- [ ] 156. [blte-stream-reader.cpp] `streamBlocks` and `createBlobURL` behavior differs from JS

- [ ] 157. [blte-stream-reader.cpp] `createReadableStream()` not ported — Web Streams API specific

- [ ] 158. [blte-stream-reader.cpp] `streamBlocks()` changed from async generator to synchronous callback

- [ ] 159. [blte-stream-reader.cpp] `createBlobURL()` returns BufferWrapper instead of string URL

- [ ] 160. [blte-stream-reader.cpp] Cache eviction uses `std::deque` for FIFO ordering vs JS `Map` insertion order

- [ ] 161. [blte-stream-reader.cpp] `_decodeBlock` for compressed blocks passes one bool in C++ vs two in JS

- [ ] 162. [build-cache.cpp] Build cache APIs are synchronous instead of JS Promise-based async methods

- [ ] 163. [build-cache.cpp] Cache cleanup size subtraction behavior differs from JS

- [ ] 164. [build-cache.cpp] `saveCacheIntegrity()` silently ignores file write failures

- [ ] 165. [cache-collector.cpp] Hand-rolled MD5 and SHA256 instead of using mbedTLS

- [ ] 166. [cache-collector.cpp] upload_chunks converts binary multipart body through std::string

- [ ] 167. [cache-collector.cpp] random_hex uses std::mt19937 instead of cryptographically secure random

- [ ] 168. [listfile.cpp] Public listfile APIs are synchronous instead of JS Promise-based async methods

- [ ] 169. [listfile.cpp] Shared preload promise semantics differ from JS

- [ ] 170. [listfile.cpp] `applyPreload` return contract differs from JS

- [ ] 171. [listfile.cpp] `getByID` not-found sentinel differs from JS

- [ ] 172. [listfile.cpp] `getFilteredEntries` search contract differs from JS

- [ ] 173. [listfile.cpp] Binary preload list ordering can differ from JS Map insertion order

- [ ] 174. [tact-keys.cpp] Tact key lifecycle APIs are synchronous instead of JS async methods

- [ ] 175. [tact-keys.cpp] Save scheduling differs from JS `setImmediate` batching behavior

- [ ] 176. [tact-keys.cpp] Remote update error contract differs from JS HTTP status error path

- [ ] 177. [tact-keys.cpp] `getKey` returns empty string instead of JS `undefined` when key not found

- [ ] 178. [content-flags.cpp] Sibling `.cpp` translation unit does not contain line-by-line ported JS constant exports

- [ ] 179. [locale-flags.cpp] Sibling `.cpp` translation unit does not contain line-by-line JS exports

- [ ] 180. [mpq.cpp] console.error replaced by logging::write for decompression errors

- [ ] 181. [mpq-install.cpp] _scan_mpq_files sorts entire accumulated vector at every recursion depth

- [ ] 182. [mpq-install.cpp] Archive push ordering differs from JS

- [ ] 183. [bzip2.cpp] Dead code branch in StrangeCRC::update for negative index

- [ ] 184. [bzip2.cpp] Missing default parameters in updateBuffer

- [ ] 185. [CompressionType.cpp] Sibling `.cpp` translation unit does not contain line-by-line JS exports

- [ ] 186. [CompressionType.cpp] Fully ported — no issues found

- [ ] 187. [db2.cpp] `getRelationRows` preload-guard error semantics differ from JS proxy behavior

- [ ] 188. [db2.cpp] JS async proxy call model is replaced with synchronous parse-once wrappers

- [ ] 189. [db2.cpp] Extra `clearCache()` function added that does not exist in original JS module

- [ ] 190. [dbd-manifest.cpp] JS truthiness filter for manifest entries is not preserved for object/array values

- [ ] 191. [DBDParser.cpp] Empty-chunk parsing behavior differs from original JS control flow

- [ ] 192. [DBDParser.cpp] `PATTERN_BUILD_ID` regex uses `.` instead of `\.` for literal dots

- [ ] 193. [DBDParser.cpp] `isBuildInRange` comparison logic has known bug matching JS — compares each component independently

- [ ] 194. [DBDParser.cpp] `isValidFor` checks empty layoutHash against `layoutHashes` set — could match unintended entries

- [ ] 195. [DBCReader.cpp] Public load path is synchronous instead of JS Promise-based async flow

- [ ] 196. [DBCReader.cpp] DBD cache backend behavior differs from JS CASC cache API

- [ ] 197. [DBCReader.cpp] `loadSchema` is synchronous in C++ but `async` in JS — caching uses filesystem instead of CASC cache

- [ ] 198. [DBCReader.cpp] `parse` is synchronous in C++ but `async` in JS

- [ ] 199. [DBCReader.cpp] `getRow` returns `std::optional<DataRecord>` but JS returns the row directly or `undefined`

- [ ] 200. [DBCReader.cpp] `getAllRows` returns `std::map` (sorted by key) but JS returns `Map` (insertion order)

- [ ] 201. [DBCReader.cpp] Int64/UInt64 field types read as 32-bit values — JS also reads 32-bit for DBC

- [ ] 202. [WDCReader.cpp] Public schema/data loading APIs are synchronous instead of JS async Promise flow

- [ ] 203. [WDCReader.cpp] DBD cache access path bypasses JS CASC cache API contract

- [ ] 204. [WDCReader.cpp] Row collection ordering/identity semantics differ from JS Map behavior

- [ ] 205. [WDCReader.cpp] Numeric input coercion from JS `parseInt(...)` is not preserved

- [ ] 206. [WDCReader.cpp] `idField` initialized to empty string instead of JS `null` — divergent sentinel before first record read

- [ ] 207. [WDCReader.cpp] BigInt arbitrary-precision bit-shift operations vs C++ fixed-width uint64_t — potential UB for large fieldSizeBits

- [ ] 208. [FieldType.cpp] Sibling `.cpp` translation unit does not contain line-by-line JS exports

- [ ] 209. [FieldType.cpp] Fully ported — no issues found

- [ ] 210. [DBCharacterCustomization.cpp] Initialization flow is synchronous and drops JS shared-promise waiting behavior

- [ ] 211. [DBCharacterCustomization.cpp] Getter not-found contracts differ from JS `Map.get(...)` undefined behavior

- [ ] 212. [DBCharacterCustomization.cpp] `chr_cust_mat_map` stores FileDataID=0 for absent tfd_map entries; JS stores `undefined`

- [ ] 213. [DBCharacterCustomization.cpp] Race/model/option maps use `unordered_map` with no ordering guarantee; JS `Map` preserves insertion order

- [ ] 214. [DBComponentModelFileData.cpp] Initialization API is synchronous and does not preserve JS promise-sharing semantics

- [ ] 215. [DBComponentTextureFileData.cpp] Initialization API is synchronous and does not preserve JS promise-sharing semantics

- [ ] 216. [DBCreatureDisplayExtra.cpp] Initialization flow is synchronous and does not mirror JS awaited init promise

- [ ] 217. [DBCreatureList.cpp] Public load API is synchronous instead of JS Promise-based async loading

- [ ] 218. [DBCreatureList.cpp] `get_all_creatures()` returns `unordered_map` (no order) vs JS `Map` (insertion order)

- [ ] 219. [DBCreatures.cpp] `initializeCreatureData` is synchronous instead of JS async data-loading flow

- [ ] 220. [DBCreatures.cpp] `creatureDisplays` entries are value copies, not shared references with `creatureDisplayInfoMap`

- [ ] 221. [DBCreaturesLegacy.cpp] Model path normalization misses JS `.mdl` to `.m2` conversion

- [ ] 222. [DBCreaturesLegacy.cpp] Legacy load API is synchronous instead of JS Promise-based async parse flow

- [ ] 223. [DBCreaturesLegacy.cpp] Exception logging omits JS stack trace output behavior

- [ ] 224. [DBCreaturesLegacy.cpp] `creatureDisplays` uses `unordered_map` (no order) vs JS `Map` (insertion order)

- [ ] 225. [DBDecor.cpp] Decor cache initialization is synchronous instead of JS async table load

- [ ] 226. [DBDecor.cpp] `decorItems` unordered_map iteration order differs from JS `Map` insertion order

- [ ] 227. [DBDecorCategories.cpp] Category cache loading is synchronous and unordered-container iteration differs from JS Map/Set ordering

- [ ] 228. [DBGuildTabard.cpp] Sibling `.cpp` file is still unconverted JavaScript and appears swapped with `.js`

- [ ] 229. [DBGuildTabard.cpp] Color maps use `unordered_map` — iteration order differs from JS `Map` for UI color pickers

- [ ] 230. [DBItemCharTextures.cpp] Initialization flow is synchronous and drops JS shared-promise semantics

- [ ] 231. [DBItemCharTextures.cpp] Race/gender texture selection fallback differs from JS behavior

- [ ] 232. [DBItemCharTextures.cpp] `value_or` fallback in `getTexturesByDisplayId` is redundant (prior entry 217 is inaccurate)

- [ ] 233. [DBItemDisplays.cpp] Item display cache initialization is synchronous instead of JS async flow

- [ ] 234. [DBItemDisplays.cpp] `ItemDisplay::textures` is a deep copy per entry, not a shared reference as in JS

- [ ] 235. [DBItemGeosets.cpp] Initialization lifecycle is synchronous and omits JS `init_promise` contract

- [ ] 236. [DBItemGeosets.cpp] Equipped-items input coercion differs from JS `Object.entries` + `parseInt` behavior

- [ ] 237. [DBItemModels.cpp] Item model cache initialization is synchronous instead of JS Promise-based flow

- [ ] 238. [DBItems.cpp] Item cache initialization is synchronous and does not preserve JS shared `init_promise`

- [ ] 239. [DBItems.cpp] `items_by_id` uses `unordered_map` (hash order) vs JS `Map` (insertion order)

- [ ] 240. [DBModelFileData.cpp] Model mapping loader is synchronous instead of JS async API

- [ ] 241. [DBNpcEquipment.cpp] NPC equipment cache initialization is synchronous and drops JS `init_promise`

- [ ] 242. [DBNpcEquipment.cpp] Inner equipment slot map uses `unordered_map` vs JS `Map` (insertion order)

- [ ] 243. [DBTextureFileData.cpp] Texture mapping loader/ensure APIs are synchronous instead of JS async APIs

- [ ] 244. [DBTextureFileData.cpp] UsageType remap path remains a TODO placeholder in C++ port

- [x] 245. [context-menu.cpp] Invisible hover buffer zone (`.context-menu-zone`) is not ported

- [ ] 246. [context-menu.cpp] JS uses `window.innerHeight/innerWidth` but C++ uses `io.DisplaySize` which may differ with multi-viewport

- [ ] 247. [context-menu.cpp] JS uses `clientMouseX`/`clientMouseY` from global mousemove listener but C++ uses `io.MousePos` at time of activation

- [ ] 248. [context-menu.cpp] `mounted()` initial reposition is not ported

- [ ] 249. [context-menu.cpp] CSS `span` items use `padding: 8px`, `border-bottom: 1px solid var(--border)`, `text-overflow: ellipsis` — not enforced by C++ rendering

- [ ] 250. [context-menu.cpp] CSS `span:hover` background `#353535` with `cursor: pointer` not enforced on menu items

- [ ] 251. [menu-button.cpp] Click emit payload drops the original event object

- [ ] 252. [menu-button.cpp] Context-menu close behavior differs from original component flow

- [ ] 253. [menu-button.cpp] Arrow width 20px instead of CSS 29px

- [ ] 254. [menu-button.cpp] Arrow uses `ICON_FA_CARET_DOWN` text instead of CSS `caret-down.svg` background image

- [ ] 255. [menu-button.cpp] Arrow missing left border `border-left: 1px solid rgba(255, 255, 255, 0.32)`

- [ ] 256. [menu-button.cpp] Dropdown menu uses ImGui popup window instead of CSS-styled `<ul>` with `--form-button-menu` background

- [ ] 257. [menu-button.cpp] Dropdown menu items missing hover `background: var(--form-button-menu-hover)`

- [ ] 258. [menu-button.cpp] Menu close behavior uses `IsMouseClicked(0)` outside check instead of JS `@close` mouse-leave

- [ ] 259. [combobox.cpp] Blur-close timing is frame-based instead of JS 200ms timeout

- [ ] 260. [combobox.cpp] Dropdown menu is rendered in normal layout flow instead of absolute popup overlay

- [ ] 261. [combobox.cpp] Dropdown `z-index: 5` and `position: absolute; top: 100%` not replicated

- [ ] 262. [combobox.cpp] Dropdown list does not have `list-style: none` equivalent — no bullet points but uses Selectable

- [ ] 263. [combobox.cpp] Missing `box-shadow: black 0 0 3px 0` on dropdown

- [ ] 264. [combobox.cpp] `filteredSource` uses `startsWith` (JS) but C++ uses `find(...) == 0` which is functionally equivalent but `std::string::starts_with` is available in C++20/23

- [ ] 265. [slider.cpp] Document-level mouse listener lifecycle from JS is not ported directly

- [ ] 266. [slider.cpp] Slider fill color uses `SLIDER_FILL_U32` but CSS uses `var(--font-alt)` (#57afe2)

- [ ] 267. [slider.cpp] Slider track background uses `SLIDER_TRACK_U32` but CSS uses `var(--background-dark)` (#2c3136)

- [ ] 268. [slider.cpp] Handle position uses `left: (modelValue * 100)%` without `translateX(-50%)` centering

- [ ] 269. [checkboxlist.cpp] Component lifecycle/event model differs from JS mounted/unmount listener flow

- [ ] 270. [checkboxlist.cpp] Scroll bound edge-case behavior differs for zero scrollbar range

- [ ] 271. [checkboxlist.cpp] Scrollbar height behavior differs from original CSS

- [ ] 272. [checkboxlist.cpp] Scrollbar default styling differs from CSS reference


- [ ] 273. [checkboxlist.cpp] Missing container border and box-shadow from CSS reference

- [ ] 274. [checkboxlist.cpp] Missing default item background and CSS padding values

- [ ] 275. [checkboxlist.cpp] Missing item text left margin from CSS `.item span` rule

- [ ] 276. [listbox.cpp] Keep-alive lifecycle listener behavior (`activated`/`deactivated`) is missing

- [ ] 277. [listbox.cpp] Context menu emit payload omits original JS mouse event object

- [ ] 278. [listbox.cpp] Multi-subfield span structure from `item.split('\31')` is flattened

- [ ] 279. [listbox.cpp] `wheelMouse` uses `core.view.config.scrollSpeed` from JS but C++ reads from `core::view->config`

- [ ] 280. [listbox.cpp] `handlePaste` creates new selection instead of clearing existing and pushing entries

- [ ] 281. [listbox.cpp] `activeQuickFilter` toggle logic matches JS but CSS pattern regex differs

- [ ] 282. [listbox.cpp] Missing container `border`, `box-shadow`, and `background` from CSS `.ui-listbox`

- [ ] 283. [listbox.cpp] `.item:hover` uses `var(--font-alt) !important` in CSS but C++ hover effect may differ

- [ ] 284. [listbox.cpp] `contextmenu` event not emitted in base listbox JS but C++ has `onContextMenu` support

- [ ] 285. [listbox.cpp] Scroller thumb color uses `FONT_PRIMARY_U32` / `FONT_HIGHLIGHT_U32` but CSS uses `var(--border)` / `var(--font-highlight)`

- [ ] 286. [listbox.cpp] Quick filter links use `ImGui::SmallButton` but CSS uses `<a>` tags with specific styling

- [ ] 287. [listbox.cpp] `includefilecount` prop exists in JS but C++ doesn't use it — counter is always shown when `unittype` is non-empty

- [ ] 288. [listbox.cpp] `activated()` / `deactivated()` Vue lifecycle hooks for keep-alive not ported

- [ ] 289. [listboxb.cpp] Selection payload changed from item values to row indices

- [ ] 290. [listboxb.cpp] Selection highlighting logic uses index identity instead of value identity

- [ ] 291. [listboxb.cpp] JS `selection` stores item objects but C++ stores item indices

- [ ] 292. [listboxb.cpp] JS `handleKey` Ctrl+C copies `this.selection.join('\n')` (object labels) but C++ copies `items[idx].label`

- [ ] 293. [listboxb.cpp] Alternating row color parity may not match CSS `:nth-child(even)` due to 0-indexed `startIdx`

- [ ] 294. [listboxb.cpp] Missing container `border`, `box-shadow` from CSS `.ui-listbox`

- [ ] 295. [listboxb.cpp] Scroller thumb uses `TEXT_ACTIVE_U32` / `TEXT_IDLE_U32` but CSS uses `var(--border)` / `var(--font-highlight)`

- [ ] 296. [listboxb.cpp] Row width uses `availSize.x - 10.0f` but CSS doesn't subtract 10px

- [ ] 297. [listboxb.cpp] `displayItems` computed as `items.slice(scrollIndex, scrollIndex + slotCount)` — C++ iterates directly with indices

- [ ] 298. [listbox-context.cpp] handle_context_menu signature changed from data object to direct selection array

- [ ] 299. [listbox-context.cpp] get_export_directory returns empty string instead of null

- [ ] 300. [listbox-maps.cpp] Missing `recalculateBounds()` call after resetting scroll on expansion filter change

- [ ] 301. [listbox-maps.cpp] JS `filteredItems` has inline text filtering + selection pruning, C++ delegates to base listbox

- [ ] 302. [listbox-zones.cpp] Missing `recalculateBounds()` call after resetting scroll on expansion filter change

- [ ] 303. [listbox-zones.cpp] Identical implementation to listbox-maps.cpp — shared code could be refactored

- [ ] 304. [itemlistbox.cpp] Selection model changed from item-object references to item ID integers

- [ ] 305. [itemlistbox.cpp] Item action controls are rendered as ImGui buttons instead of list item links

- [ ] 306. [itemlistbox.cpp] JS `emits: ['update:selection', 'equip']` but C++ header declares `onOptions` callback

- [ ] 307. [itemlistbox.cpp] `handleKey` copies `displayName` but JS copies `displayName` via `e.displayName` from selection objects

- [ ] 308. [itemlistbox.cpp] Item row height uses `46px` but JS CSS uses `height: 26px` in `.ui-listbox .item`

- [ ] 309. [itemlistbox.cpp] Missing container `border`, `box-shadow`, and `background` from CSS `.ui-listbox`

- [ ] 310. [itemlistbox.cpp] Quality 7 and 8 both map to Heirloom color but CSS may define separate classes

- [ ] 311. [itemlistbox.cpp] `.item-icon` rendering not implemented — icon placeholder only

- [ ] 312. [data-table.cpp] Filter icon click no longer focuses the data-table filter input

- [ ] 313. [data-table.cpp] Empty-string numeric sorting semantics differ from JS `Number(...)`

- [ ] 314. [data-table.cpp] Header sort/filter icons are custom-drawn instead of CSS/SVG assets

- [ ] 315. [data-table.cpp] Native scroll-to-custom-scroll sync path from JS is missing

- [ ] 316. [data-table.cpp] JS sort uses `Array.prototype.sort()` (unstable in some engines) but C++ uses `std::stable_sort`

- [ ] 317. [data-table.cpp] JS `localeCompare()` for string sorting vs C++ `std::string::compare()` after toLower

- [ ] 318. [data-table.cpp] `preventMiddleMousePan` from JS is not ported

- [ ] 319. [data-table.cpp] `syncScrollPosition` is intentionally omitted but JS uses it to sync native+custom scroll

- [ ] 320. [data-table.cpp] `list-status` text uses `ImGui::TextDisabled` but CSS `.list-status` may have different styling

- [ ] 321. [data-table.cpp] Missing CSS container `border: 1px solid var(--border)` and `box-shadow: black 0 0 3px 0px`

- [ ] 322. [data-table.cpp] Row alternating colors use `BG_ALT_U32` / `BG_DARK_U32` but CSS uses `--background-dark` (default) and `--background-alt` (even)

- [ ] 323. [data-table.cpp] Cell padding is `5px` in C++ but CSS uses `padding: 5px 10px` for `td`

- [ ] 324. [data-table.cpp] Missing `Number(val)` equivalence check in `escape_value` — JS `isNaN(val)` checks the original value type

- [ ] 325. [data-table.cpp] `formatWithThousandsSep` needs to match JS `toLocaleString()` thousands separator

- [ ] 326. [data-table.cpp] Header height hardcoded to `40px` but CSS uses `padding: 10px` top/bottom on `th`

- [ ] 327. [data-table.cpp] `handleFilterIconClick` not fully visible but CSS filter icon uses specific SVG styling

- [ ] 328. [data-table.cpp] Sort icon CSS uses SVG background images but C++ draws triangles

- [ ] 329. [file-field.cpp] Directory dialog trigger moved from input focus to separate browse button

- [ ] 330. [file-field.cpp] Same-directory reselection behavior differs from JS file input reset logic

- [ ] 331. [file-field.cpp] JS opens dialog on `@focus` but C++ opens on button click

- [ ] 332. [file-field.cpp] JS `mounted()` creates hidden `<input type="file" nwdirectory>` element — C++ uses portable-file-dialogs

- [ ] 333. [file-field.cpp] JS `openDialog()` clears file input value before opening — C++ does not clear state

- [ ] 334. [file-field.cpp] Missing placeholder rendering position uses hardcoded offsets

- [ ] 335. [file-field.cpp] Extra `openFileDialog()` and `saveFileDialog()` functions not in JS source

- [ ] 336. [resize-layer.cpp] ResizeObserver lifecycle is replaced by per-frame width polling

- [ ] 337. [resize-layer.cpp] Fully ported — no issues found

- [ ] 338. [markdown-content.cpp] Inline image markdown is not rendered as images

- [ ] 339. [markdown-content.cpp] Inline bold/italic formatting behavior differs from JS HTML rendering

- [ ] 340. [markdown-content.cpp] CSS base font-size 20px not applied — ImGui uses default ~14px font

- [ ] 341. [markdown-content.cpp] Link color uses `FONT_ALT` (#57afe2 blue) instead of CSS `--font-highlight` (#ffffff white)

- [ ] 342. [markdown-content.cpp] Links rendered without underline decoration

- [ ] 343. [markdown-content.cpp] Inline code missing background `rgba(0,0,0,0.3)` with `padding: 2px 6px` and `border-radius: 3px`

- [ ] 344. [markdown-content.cpp] Inline code color `(0.9, 0.7, 0.5)` has no CSS basis — CSS uses monospace font, not a special color

- [ ] 345. [markdown-content.cpp] Bold text rendered with white color instead of bold font weight

- [ ] 346. [markdown-content.cpp] Italic text rendered with dim blue `(0.8, 0.8, 0.9)` instead of italic font style

- [ ] 347. [markdown-content.cpp] h1 header missing bottom separator matching CSS `border-bottom`

- [ ] 348. [markdown-content.cpp] Scrollbar thumb uses hardcoded gray colors instead of CSS `var(--border)` and `var(--font-highlight)`

- [ ] 349. [markdown-content.cpp] Scrollbar thumb missing `border: 1px solid var(--border)` and `border-radius: 5px` from CSS

- [ ] 350. [markdown-content.cpp] Scrollbar track `right: 3px` positioning and `opacity: 0.7` not matched

- [ ] 351. [markdown-content.cpp] `parseInline` processes text before HTML escaping, while JS escapes first then applies regex

- [ ] 352. [markdown-content.cpp] List items use `•` bullet with 16px indent instead of CSS `padding-left: 2em` with disc marker

- [ ] 353. [texture-ribbon.cpp] Additional GL texture management functions not present in JS

- [ ] 354. [home-showcase.cpp] Showcase card/video/background layer rendering is not ported

- [ ] 355. [home-showcase.cpp] `background_style` computed output is missing from C++ render path

- [ ] 356. [home-showcase.cpp] Feedback action wiring differs from JS `data-kb-link` behavior

- [ ] 357. [home-showcase.cpp] `build_background_style()` function not ported — no background image layers

- [ ] 358. [home-showcase.cpp] `BASE_LAYERS` stored as single `ShowcaseLayer` instead of array

- [ ] 359. [home-showcase.cpp] Video playback (`<video>` element) not implemented

- [ ] 360. [home-showcase.cpp] `computed: background_style()` not ported

- [ ] 361. [home-showcase.cpp] Title text says "Made with wow.export.cpp" but JS says "Made with wow.export"

- [ ] 362. [home-showcase.cpp] Showcase rendering is plain text instead of styled grid layout

- [ ] 363. [home-showcase.cpp] Feedback link opens "KB011" directly instead of resolving via `data-kb-link` attribute

- [ ] 364. [home-showcase.cpp] `.showcase-title` CSS uses Gambler font at 40px — not replicated

- [ ] 365. [map-viewer.cpp] Tile image drawing path is still unimplemented

- [ ] 366. [map-viewer.cpp] Tile loading flow is synchronous instead of JS Promise-based async queueing

- [ ] 367. [map-viewer.cpp] Box-select-mode active color uses NAV_SELECTED (#22B549) instead of CSS #5fdb65

- [ ] 368. [map-viewer.cpp] Map-viewer info bar text lacks CSS `text-shadow: black 0 0 6px`

- [ ] 369. [map-viewer.cpp] Map-viewer info bar spans missing CSS `margin: 0 10px` horizontal spacing

- [ ] 370. [map-viewer.cpp] Map-viewer checkerboard background pattern not implemented

- [ ] 371. [map-viewer.cpp] Map-viewer hover-info positioned at top via ImGui layout instead of CSS `bottom: 3px; left: 3px`

- [ ] 372. [map-viewer.cpp] Box-select-mode cursor not changed to crosshair

- [ ] 373. [map-viewer.cpp] Tile rendering to canvas via GL textures not implemented — tiles cached in memory but not displayed

- [ ] 374. [map-viewer.cpp] `handleTileInteraction` emits selection changes via mutable reference instead of `$emit('update:selection')`

- [ ] 375. [map-viewer.cpp] Map margin `20px 20px 0 10px` from CSS not applied

- [ ] 376. [tab_home.cpp] Home showcase content is replaced with custom nav-card UI instead of the JS `HomeShowcase` component

- [ ] 377. [tab_home.cpp] `whatsNewHTML` is rendered as plain text instead of HTML content

- [ ] 378. [tab_home.cpp] "wow.export vX.X.X" title text is an invention not in original JS

- [ ] 379. [tab_home.cpp] HomeShowcase component entirely replaced with navigation cards

- [ ] 380. [tab_home.cpp] URLs hardcoded instead of using external-links system

- [ ] 381. [tab_home.cpp] whatsNewHTML rendered as plain text instead of HTML

- [ ] 382. [tab_home.cpp] #home-changes vertical padding differs from CSS

- [ ] 383. [tab_home.cpp] Background image "cover" mode not implemented correctly

- [ ] 384. [tab_home.cpp] Help button icon size, position, and rotation differ from CSS

- [ ] 385. [tab_home.cpp] Missing hover transition animations on help buttons

- [ ] 386. [tab_home.cpp] Help button subtitle uses FONT_FADED instead of inheriting parent color

- [ ] 387. [tab_home.cpp] #home-help-buttons grid-column full-width not fully replicated

- [ ] 388. [tab_home.cpp] #home-changes container query responsive font sizes not implemented

- [ ] 389. [tab_home.cpp] openInExplorer used for URL opening instead of external-links open

- [ ] 390. [tab_home.cpp] No cleanup/destruction of OpenGL textures

- [ ] 391. [legacy_tab_home.cpp] Legacy home tab template is replaced by shared `tab_home` layout

- [ ] 392. [legacy_tab_home.cpp] External link help buttons (Discord, GitHub, Patreon) not rendered

- [ ] 393. [tab_changelog.cpp] Changelog path resolution logic differs from JS two-path contract

- [ ] 394. [tab_changelog.cpp] Changelog screen typography/layout diverges from JS `#changelog` template styling

- [ ] 395. [tab_changelog.cpp] Heading rendered as plain ImGui::Text instead of styled h1

- [ ] 396. [tab_help.cpp] Search filtering no longer uses JS 300ms debounce behavior

- [ ] 397. [tab_help.cpp] Help article list presentation differs from JS title/tag/KB layout

- [ ] 398. [tab_help.cpp] Missing 300ms debounce on search filter

- [ ] 399. [tab_help.cpp] Article list layout differs tags shown in tooltip instead of inline

- [ ] 400. [tab_help.cpp] load_help_docs is synchronous instead of async

- [ ] 401. [tab_blender.cpp] Blender version gating semantics differ from JS string comparison behavior

- [ ] 402. [tab_blender.cpp] Blender install screen layout is not a pixel-equivalent port of JS markup

- [ ] 403. [tab_blender.cpp] get_blender_installations uses regex_match instead of regex_search

- [ ] 404. [tab_blender.cpp] Blender version comparison uses numeric instead of string comparison

- [ ] 405. [tab_blender.cpp] start_automatic_install and checkLocalVersion are synchronous

- [ ] 406. [tab_install.cpp] Install listbox copy/paste options are hardcoded instead of using JS config-driven behavior

- [ ] 407. [tab_install.cpp] Regex indicator tooltip metadata from JS template is missing

- [ ] 408. [tab_install.cpp] Async operations converted to synchronous calls

- [ ] 409. [tab_install.cpp] CASC getFile replaced with low-level two-step call, losing BLTE decoding

- [ ] 410. [tab_install.cpp] processAllBlocks() call missing in view_strings_impl

- [ ] 411. [tab_install.cpp] export_install_files missing stack trace in error mark

- [ ] 412. [tab_install.cpp] First listbox missing copyMode from config

- [ ] 413. [tab_install.cpp] First listbox missing pasteSelection from config

- [ ] 414. [tab_install.cpp] First listbox missing copytrimwhitespace from config

- [ ] 415. [tab_install.cpp] Second listbox (strings) missing copyMode from config

- [ ] 416. [tab_install.cpp] First listbox unittype should be "install file" not "file"

- [ ] 417. [tab_install.cpp] Strings listbox nocopy incorrectly set to true

- [ ] 418. [tab_install.cpp] Strings sidebar missing CSS styling equivalents

- [ ] 419. [tab_install.cpp] Input placeholder text not rendered

- [ ] 420. [tab_install.cpp] Regex tooltip not rendered

- [ ] 421. [tab_install.cpp] extract_strings and update_install_listfile exposed in header but should be file-local

- [ ] 422. [tab_models.cpp] Regex indicator tooltip metadata from JS template is missing

- [ ] 423. [tab_models.cpp] preview_model and export_files are synchronous instead of async

- [ ] 424. [tab_models.cpp] create_renderer last parameter is file_data_id instead of file_name

- [ ] 425. [tab_models.cpp] M3 has_content hardcoded to true instead of checking draw_calls/groups

- [ ] 426. [tab_models.cpp] Missing "View Log" button in generic error toast

- [ ] 427. [tab_models.cpp] path.basename strips differently than C++ extension removal

- [ ] 428. [tab_models.cpp] Drop handler prompt lambda missing count parameter

- [ ] 429. [tab_models.cpp] Model quick filters not passed to listbox

- [ ] 430. [tab_models.cpp] Missing "Regex Enabled" indicator in filter bar

- [ ] 431. [tab_models.cpp] Sidebar checkboxes missing tooltip text

- [ ] 432. [tab_models.cpp] WMO Groups uses raw checkboxes instead of CheckboxList component

- [ ] 433. [tab_models.cpp] WMO Doodad Sets uses raw checkboxes instead of CheckboxList component

- [ ] 434. [tab_models.cpp] getActiveRenderer() only returns M2, not polymorphic like JS

- [ ] 435. [tab_models.cpp] enableM2Skins config default may differ

- [ ] 436. [tab_models.cpp] helper.mark on failure missing stack trace parameter

- [ ] 437. [tab_models.cpp] MenuButton missing "upward" class styling

- [ ] 438. [tab_models.cpp] Texture ribbon slot click behavior differs from JS

- [ ] 439. [tab_models_legacy.cpp] Regex indicator tooltip metadata from JS template is missing

- [ ] 440. [tab_models_legacy.cpp] WMOLegacyRendererGL constructor passes 0 instead of file_name

- [ ] 441. [tab_models_legacy.cpp] Missing "View Log" button in preview_model error toast

- [ ] 442. [tab_models_legacy.cpp] Missing requestAnimationFrame deferral for fitCamera

- [ ] 443. [tab_models_legacy.cpp] PNG/CLIPBOARD export_paths stream not written for PNG exports

- [ ] 444. [tab_models_legacy.cpp] helper.mark on failure missing stack trace argument

- [ ] 445. [tab_models_legacy.cpp] Listbox missing quickfilters from view

- [ ] 446. [tab_models_legacy.cpp] Listbox missing copyMode config binding

- [ ] 447. [tab_models_legacy.cpp] Listbox missing pasteSelection config binding

- [ ] 448. [tab_models_legacy.cpp] Listbox missing copytrimwhitespace config binding

- [ ] 449. [tab_models_legacy.cpp] Missing "Regex Enabled" indicator in filter bar

- [ ] 450. [tab_models_legacy.cpp] Filter input missing placeholder text

- [ ] 451. [tab_models_legacy.cpp] All sidebar checkboxes missing tooltip text

- [ ] 452. [tab_models_legacy.cpp] step/seek/start_scrub/end_scrub only handle M2, not MDX

- [ ] 453. [tab_models_legacy.cpp] WMO Groups rendered with raw Checkbox instead of Checkboxlist component

- [ ] 454. [tab_models_legacy.cpp] Doodad Sets rendered with raw Checkbox instead of Checkboxlist component

- [ ] 455. [tab_models_legacy.cpp] getActiveRenderer() only returns M2, not active renderer

- [ ] 456. [tab_models_legacy.cpp] preview_model and export_files are synchronous instead of async

- [ ] 457. [tab_models_legacy.cpp] MenuButton missing "upward" class/direction

- [ ] 458. [tab_textures.cpp] Baked NPC texture apply path stores a file data ID instead of the JS BLP object

- [ ] 459. [tab_textures.cpp] Baked NPC texture failure toast omits JS `view log` action callback

- [ ] 460. [tab_textures.cpp] Texture channel controls are rendered as checkboxes instead of JS channel chips

- [ ] 461. [tab_textures.cpp] Listbox override texture list not forwarded

- [ ] 462. [tab_textures.cpp] MenuButton replaced with plain Button — no format dropdown

- [ ] 463. [tab_textures.cpp] apply_baked_npc_texture skips CASC file load and BLP creation

- [ ] 464. [tab_textures.cpp] Missing "View Log" action button on baked NPC texture error toast

- [ ] 465. [tab_textures.cpp] Atlas overlay regions not cleared when atlas_id found but entry missing

- [ ] 466. [tab_textures.cpp] Drop handler prompt omits file count

- [ ] 467. [tab_textures.cpp] Atlas region tooltip positioning not implemented

- [ ] 468. [tab_textures.cpp] Filter input missing placeholder text

- [ ] 469. [tab_textures.cpp] Regex tooltip text missing

- [ ] 470. [tab_textures.cpp] export_texture_atlas_regions missing stack trace in error mark

- [ ] 471. [tab_textures.cpp] All async operations are synchronous — blocks UI thread

- [ ] 472. [tab_textures.cpp] Channel mask toggles rendered as checkboxes instead of styled inline buttons

- [ ] 473. [tab_textures.cpp] Preview image max dimensions not clamped to texture dimensions

- [ ] 474. [legacy_tab_textures.cpp] Listbox context menu render path from JS template is missing

- [ ] 475. [legacy_tab_textures.cpp] Channel toggle visuals/interaction differ from JS channel list UI

- [ ] 476. [legacy_tab_textures.cpp] PNG/JPG preview info shows lowercase extension vs JS uppercase

- [ ] 477. [legacy_tab_textures.cpp] Listbox hardcodes `pasteselection` and `copytrimwhitespace` to false vs JS config values

- [ ] 478. [legacy_tab_textures.cpp] Listbox hardcodes `CopyMode::Default` vs JS `$core.view.config.copyMode`

- [ ] 479. [legacy_tab_textures.cpp] Channel checkboxes always visible vs JS conditional on `texturePreviewURL`

- [ ] 480. [tab_audio.cpp] Audio quick-filter list path is missing from listbox wiring

- [ ] 481. [tab_audio.cpp] `unload_track` no longer revokes preview URL data like JS

- [ ] 482. [tab_audio.cpp] Sound player visuals differ from the JS template/CSS implementation

- [ ] 483. [tab_audio.cpp] play_track uses get_duration() <= 0 instead of checking buffer existence

- [ ] 484. [tab_audio.cpp] Missing audioQuickFilters prop on Listbox

- [ ] 485. [tab_audio.cpp] unittype is "sound" instead of "sound file"

- [ ] 486. [tab_audio.cpp] export_sounds missing stack trace in error mark

- [ ] 487. [tab_audio.cpp] load_track play_track and export_sounds are synchronous blocking main thread

- [ ] 488. [legacy_tab_audio.cpp] Playback UI visuals diverge from JS template/CSS

- [ ] 489. [legacy_tab_audio.cpp] Seek-loop scheduling differs from JS `requestAnimationFrame` lifecycle

- [ ] 490. [legacy_tab_audio.cpp] Context menu adds FileDataID-related items not present in JS legacy audio template

- [ ] 491. [legacy_tab_audio.cpp] Sound player info combines seek/title/duration into single Text call vs JS 3 separate spans

- [ ] 492. [legacy_tab_audio.cpp] Play/Pause uses text toggle ("Play"/"Pause") vs JS CSS class-based visual toggle

- [ ] 493. [legacy_tab_audio.cpp] Volume slider is ImGui::SliderFloat with format string vs JS custom Slider component

- [ ] 494. [legacy_tab_audio.cpp] Loop/Autoplay checkboxes placed in preview container instead of preview-controls div

- [ ] 495. [legacy_tab_audio.cpp] `load_track` checks `player.get_duration() <= 0` vs JS `!player.buffer`

- [ ] 496. [legacy_tab_audio.cpp] `export_sounds` `helper.mark` doesn't pass error stack trace

- [ ] 497. [legacy_tab_audio.cpp] Filter input missing placeholder text "Filter sound files..."

- [ ] 498. [tab_videos.cpp] Video preview playback is opened externally instead of using an embedded player

- [ ] 499. [tab_videos.cpp] Video export format selector from MenuButton is missing

- [ ] 500. [tab_videos.cpp] Kino processing toast omits the explicit Cancel action payload

- [ ] 501. [tab_videos.cpp] Dev-mode kino processing trigger export is missing

- [ ] 502. [tab_videos.cpp] Corrupted AVI fallback does not force CASC fallback fetch path

- [ ] 503. [tab_videos.cpp] MenuButton export format dropdown completely missing

- [ ] 504. [tab_videos.cpp] AVI export corruption fallback is a no-op

- [ ] 505. [tab_videos.cpp] Video preview is text-only, not an embedded player

- [ ] 506. [tab_videos.cpp] No onended/onerror callbacks for video playback

- [ ] 507. [tab_videos.cpp] build_payload runs on main thread, blocking UI

- [ ] 508. [tab_videos.cpp] stop_video does not join/stop background thread

- [ ] 509. [tab_videos.cpp] MP4 download HTTP error check missing

- [ ] 510. [tab_videos.cpp] All helper.mark error calls missing stack trace argument

- [ ] 511. [tab_videos.cpp] stream_video outer catch missing stack trace log

- [ ] 512. [tab_videos.cpp] Cancel button missing from kino_processing toast

- [ ] 513. [tab_videos.cpp] Regex tooltip missing on "Regex Enabled" text

- [ ] 514. [tab_videos.cpp] Spurious "Connecting to video server..." toast not in JS

- [ ] 515. [tab_videos.cpp] "View Log" button text capitalization differs from JS

- [ ] 516. [tab_videos.cpp] Filter input buffer capped at 255 chars

- [ ] 517. [tab_videos.cpp] kino_post hardcodes hostname and path instead of using constant

- [ ] 518. [tab_videos.cpp] Subtitle loading uses different API path than JS

- [ ] 519. [tab_videos.cpp] MP4 download may lack User-Agent header

- [ ] 520. [tab_videos.cpp] Dead variable prev_selection_first never read

- [ ] 521. [tab_videos.cpp] Dev-mode trigger_kino_processing not exposed in C++

- [ ] 522. [tab_text.cpp] Text preview failure toast omits JS `View Log` action callback

- [ ] 523. [tab_text.cpp] Regex indicator tooltip metadata from JS template is missing

- [ ] 524. [tab_text.cpp] getFileByName vs getVirtualFileByName in preview and export

- [ ] 525. [tab_text.cpp] readString() encoding parameter missing

- [ ] 526. [tab_text.cpp] Missing 'View Log' button in error toast

- [ ] 527. [tab_text.cpp] Regex tooltip missing on "Regex Enabled" text

- [ ] 528. [tab_text.cpp] Filter input missing placeholder text

- [ ] 529. [tab_text.cpp] export_text is synchronous instead of async

- [ ] 530. [tab_text.cpp] export_text error handler missing stack trace parameter

- [ ] 531. [tab_text.cpp] Text preview child window padding differs from CSS

- [ ] 532. [tab_fonts.cpp] Font preview textarea is not rendered with the selected loaded font family

- [ ] 533. [tab_fonts.cpp] Loaded font cache contract differs from JS URL-based font-face lifecycle

- [ ] 534. [tab_fonts.cpp] Font preview textarea does not render in the loaded font

- [ ] 535. [tab_fonts.cpp] Missing data.processAllBlocks() call in load_font

- [ ] 536. [tab_fonts.cpp] export_fonts missing stack trace in error mark

- [ ] 537. [tab_fonts.cpp] load_font and export_fonts are synchronous blocking main thread

- [ ] 538. [legacy_tab_fonts.cpp] Preview text is not rendered with the selected font family

- [ ] 539. [legacy_tab_fonts.cpp] Font loading contract differs from JS URL-based `loaded_fonts` cache

- [ ] 540. [legacy_tab_fonts.cpp] Glyph cells rendered in default ImGui font, not the selected font family

- [ ] 541. [legacy_tab_fonts.cpp] Font preview placeholder shown as tooltip vs JS textarea placeholder

- [ ] 542. [legacy_tab_fonts.cpp] Glyph cell size hardcoded 24x24 may not match JS CSS

- [ ] 543. [tab_data.cpp] Data-table cell copy stringification differs from JS `String(value)` behavior

- [ ] 544. [tab_data.cpp] DB2 load error toast omits JS `View Log` action callback

- [ ] 545. [tab_data.cpp] Listbox single parameter is true should be false (multi-select broken)

- [ ] 546. [tab_data.cpp] Listbox nocopy is false should be true

- [ ] 547. [tab_data.cpp] Listbox unittype is "table" should be "db2 file"

- [ ] 548. [tab_data.cpp] Listbox pasteselection and copytrimwhitespace hardcoded false

- [ ] 549. [tab_data.cpp] load_table error toast missing View Log action button

- [ ] 550. [tab_data.cpp] Context menu labels are static instead of dynamic row count

- [ ] 551. [tab_data.cpp] Context menu node not cleared on close

- [ ] 552. [tab_data.cpp] copy_cell uses value.dump() producing JSON-quoted strings

- [ ] 553. [tab_data.cpp] Selection watcher prevents retry after failed load

- [ ] 554. [tab_data.cpp] Missing Regex Enabled indicators in both filter bars

- [ ] 555. [tab_data.cpp] helper.mark() calls missing stack trace argument

- [ ] 556. [legacy_tab_data.cpp] Export format menu omits JS SQL/DBC options

- [ ] 557. [legacy_tab_data.cpp] `copy_cell` empty-string handling differs from JS

- [ ] 558. [legacy_tab_data.cpp] DBC filename extraction uses `std::filesystem::path` which won't split backslash-delimited MPQ paths on Linux

- [ ] 559. [legacy_tab_data.cpp] Listbox `unittype` is "table" vs JS "dbc file"

- [ ] 560. [legacy_tab_data.cpp] Listbox `nocopy` is `false` vs JS `:nocopy="true"`

- [ ] 561. [legacy_tab_data.cpp] Missing regex info display in DBC filter bar

- [ ] 562. [legacy_tab_data.cpp] Context menu uses `ImGui::BeginPopupContextItem` vs JS ContextMenu component

- [ ] 563. [tab_raw.cpp] Regex indicator tooltip metadata from JS template is missing

- [ ] 564. [tab_raw.cpp] export_raw_files uses getVirtualFileByName and drops partialDecrypt=true

- [ ] 565. [tab_raw.cpp] export_raw_files error mark missing stack trace argument

- [ ] 566. [tab_raw.cpp] parent_path() returns "" not "." for bare filenames

- [ ] 567. [tab_raw.cpp] Missing placeholder text on filter input

- [ ] 568. [tab_raw.cpp] Missing tooltip on "Regex Enabled" text

- [ ] 569. [tab_raw.cpp] All async functions converted to synchronous — blocks UI thread

- [ ] 570. [tab_raw.cpp] detect_raw_files manually sets is_dirty=true — deviates from JS

- [ ] 571. [legacy_tab_files.cpp] Listbox context menu includes extra FileDataID actions absent in JS

- [ ] 572. [legacy_tab_files.cpp] Layout doesn't use `app::layout` helpers — uses raw `ImGui::BeginChild`

- [ ] 573. [legacy_tab_files.cpp] Filter input missing placeholder text "Filter files..."

- [ ] 574. [legacy_tab_files.cpp] Tray layout structure differs from JS

- [ ] 575. [tab_maps.cpp] Regex indicator tooltip metadata from JS template is missing

- [ ] 576. [tab_maps.cpp] Hand-rolled MD5 instead of mbedTLS

- [ ] 577. [tab_maps.cpp] load_map_tile uses nearest-neighbor scaling instead of bilinear interpolation

- [ ] 578. [tab_maps.cpp] load_map_tile uses blp.width instead of blp.scaledWidth

- [ ] 579. [tab_maps.cpp] load_wmo_minimap_tile ignores drawX/drawY and scaleX/scaleY positioning

- [ ] 580. [tab_maps.cpp] export_map_wmo_minimap uses max-alpha instead of source-over compositing

- [ ] 581. [tab_maps.cpp] WDT file load is outside try-catch block

- [ ] 582. [tab_maps.cpp] mapViewerHasWorldModel check differs from JS

- [ ] 583. [tab_maps.cpp] Missing e.stack in all helper.mark error calls

- [ ] 584. [tab_maps.cpp] All async functions converted to synchronous — UI will block

- [ ] 585. [tab_maps.cpp] Missing optional chaining for export_paths

- [ ] 586. [tab_maps.cpp] Missing "Filter maps..." placeholder text

- [ ] 587. [tab_maps.cpp] Missing regex tooltip

- [ ] 588. [tab_maps.cpp] Sidebar headers use SeparatorText instead of styled span

- [ ] 589. [tab_maps.cpp] collect_game_objects returns vector instead of Set

- [ ] 590. [tab_maps.cpp] Selection watch may miss intermediate changes

- [ ] 591. [tab_zones.cpp] Default phase filtering excludes non-zero phases unlike JS

- [ ] 592. [tab_zones.cpp] UiMapArtStyleLayer lookup key differs from JS relation logic

- [ ] 593. [tab_zones.cpp] Base tile relation lookup uses layer-row ID instead of UiMapArt ID

- [ ] 594. [tab_zones.cpp] Base map tile OffsetX/OffsetY offsets are ignored

- [ ] 595. [tab_zones.cpp] Zone listbox copy/paste trim options are hardcoded instead of config-bound

- [ ] 596. [tab_zones.cpp] Phase selector placement differs from JS preview overlay layout

- [ ] 597. [tab_zones.cpp] UiMapArtStyleLayer join uses wrong field name

- [ ] 598. [tab_zones.cpp] CombinedArtStyle.id stores wrong ID (layer ID vs art ID)

- [ ] 599. [tab_zones.cpp] C++ adds ALL matching style layers; JS keeps only LAST

- [ ] 600. [tab_zones.cpp] Phase filter logic differs when phase_id is null

- [ ] 601. [tab_zones.cpp] Missing tile OffsetX/OffsetY in render_map_tiles

- [ ] 602. [tab_zones.cpp] Tile layer rendering architecture differs from JS

- [ ] 603. [tab_zones.cpp] parse_zone_entry doesn't throw on bad input

- [ ] 604. [tab_zones.cpp] UiMap row existence not validated

- [ ] 605. [tab_zones.cpp] Pixel buffer not cleared at start of render when first layer is non-zero

- [ ] 606. [tab_zones.cpp] Listbox copyMode hardcoded instead of from config

- [ ] 607. [tab_zones.cpp] Listbox pasteSelection hardcoded false instead of from config

- [ ] 608. [tab_zones.cpp] Listbox copytrimwhitespace hardcoded false instead of from config

- [ ] 609. [tab_zones.cpp] export_zone_map helper.mark missing stack trace

- [ ] 610. [tab_zones.cpp] Phase dropdown placed in control bar instead of preview overlay

- [ ] 611. [tab_zones.cpp] Missing regex tooltip on "Regex Enabled" text

- [ ] 612. [tab_zones.cpp] EXPANSION_NAMES static vector is dead code

- [ ] 613. [tab_zones.cpp] ZoneDisplayInfo vs ZoneEntry naming mismatch with header

- [ ] 614. [tab_zones.cpp] Missing per-tile position logging in render_map_tiles

- [ ] 615. [tab_zones.cpp] Missing "no tiles found" log for art style

- [ ] 616. [tab_zones.cpp] Missing "no overlays found" log

- [ ] 617. [tab_zones.cpp] Missing "no overlay tiles" log per overlay

- [ ] 618. [tab_zones.cpp] Unsafe Windows wstring conversion corrupts multi-byte UTF-8 paths

- [ ] 619. [tab_zones.cpp] Linux shell command injection risk in openInExplorer

- [ ] 620. [tab_items.cpp] Wowhead item handler is stubbed out

- [ ] 621. [tab_items.cpp] Item sidebar checklist interaction/layout diverges from JS clickable row design

- [ ] 622. [tab_items.cpp] Regex indicator tooltip metadata from JS template is missing

- [ ] 623. [tab_items.cpp] view_on_wowhead is stubbed — does nothing

- [ ] 624. [tab_items.cpp] copy_to_clipboard bypasses core.view.copyToClipboard

- [ ] 625. [tab_items.cpp] std::set ordering differs from JS Set insertion order

- [ ] 626. [tab_items.cpp] Second loop (itemViewerShowAll) retrieves item name from wrong source

- [ ] 627. [tab_items.cpp] Regex tooltip not rendered

- [ ] 628. [tab_items.cpp] Sidebar headers use SeparatorText instead of styled header span

- [ ] 629. [tab_items.cpp] Sidebar checklist items lack .selected class visual feedback

- [ ] 630. [tab_items.cpp] All async operations converted to synchronous — UI may block

- [ ] 631. [tab_items.cpp] Quality color applied only to CheckMark, not to label text

- [ ] 632. [tab_items.cpp] Filter input buffer limited to 256 bytes

- [ ] 633. [tab_item_sets.cpp] Regex indicator tooltip metadata from JS template is missing

- [ ] 634. [tab_item_sets.cpp] Missing filter input placeholder text

- [ ] 635. [tab_item_sets.cpp] Missing regex tooltip on "Regex Enabled" text

- [ ] 636. [tab_item_sets.cpp] Async initialization converted to synchronous blocking calls

- [ ] 637. [tab_item_sets.cpp] apply_filter converts ItemSet structs to JSON objects unnecessarily

- [ ] 638. [tab_item_sets.cpp] render() re-creates item_entries vector from JSON every frame

- [ ] 639. [tab_item_sets.cpp] Regex-enabled text and filter input lack proper layout container

- [ ] 640. [tab_item_sets.cpp] fieldToUint32Vec does not handle single-value fields

- [ ] 641. [tab_characters.cpp] Saved-character thumbnail card rendering is replaced by a placeholder button path

- [ ] 642. [tab_characters.cpp] Main-screen quick-save flow skips JS thumbnail capture step

- [ ] 643. [tab_characters.cpp] Outside-click handlers for import/color popups from JS mounted lifecycle are missing

- [ ] 644. [tab_characters.cpp] import_wmv_character() is completely stubbed

- [ ] 645. [tab_characters.cpp] import_wowhead_character() is completely stubbed

- [ ] 646. [tab_characters.cpp] Missing texture application on attachment equipment models

- [ ] 647. [tab_characters.cpp] Missing texture application on collection equipment models

- [ ] 648. [tab_characters.cpp] Missing geoset visibility for collection models

- [ ] 649. [tab_characters.cpp] OBJ/STL export missing chr_materials URI textures geoset mask and pose application

- [ ] 650. [tab_characters.cpp] OBJ/STL/GLTF export missing CharacterExporter equipment models

- [ ] 651. [tab_characters.cpp] load_character_model always sets animation to none instead of auto-selecting stand

- [ ] 652. [tab_characters.cpp] load_character_model missing on_model_rotate callback

- [ ] 653. [tab_characters.cpp] import_character does not lowercase character name in URL

- [ ] 654. [tab_characters.cpp] import_character error handling uses string search instead of HTTP status

- [ ] 655. [tab_characters.cpp] import_json_character save-to-my-characters preserves guild_tabard (JS does not)

- [ ] 656. [tab_characters.cpp] Missing getEquipmentRenderers and getCollectionRenderers callbacks on viewer context

- [ ] 657. [tab_characters.cpp] Equipment slot items missing quality color styling

- [ ] 658. [tab_characters.cpp] navigate_to_items_for_slot missing type mask filtering

- [ ] 659. [tab_characters.cpp] Saved characters grid missing thumbnail rendering

- [ ] 660. [tab_characters.cpp] Texture preview panel is placeholder text

- [ ] 661. [tab_characters.cpp] Color picker uses ImGui Tooltip instead of positioned popup

- [ ] 662. [tab_characters.cpp] Missing document click handler for dismissing panels

- [ ] 663. [tab_characters.cpp] Missing unmounted() cleanup

- [ ] 664. [tab_creatures.cpp] Creature list context actions are not equivalent to JS copy-name/copy-ID menu

- [ ] 665. [tab_creatures.cpp] has_content check and toast/camera logic scoped incorrectly

- [ ] 666. [tab_creatures.cpp] Collection model geoset logic has three bugs

- [ ] 667. [tab_creatures.cpp] Scrubber IsItemActivated() called before SliderInt checks wrong widget

- [ ] 668. [tab_creatures.cpp] Missing export_paths.writeLine calls in multiple export paths

- [ ] 669. [tab_creatures.cpp] GLTF format.toLowerCase() not applied

- [ ] 670. [tab_creatures.cpp] Error toast for model load missing View Log action button

- [ ] 671. [tab_creatures.cpp] path.basename behavior not replicated in skin name

- [ ] 672. [tab_creatures.cpp] Missing Regex Enabled indicator in filter bar

- [ ] 673. [tab_creatures.cpp] Listbox context menu Copy names and Copy IDs not rendered in UI

- [ ] 674. [tab_creatures.cpp] Sorting uses byte comparison instead of locale-aware localeCompare

- [ ] 675. [tab_decor.cpp] PNG/CLIPBOARD export branch does not short-circuit like JS

- [ ] 676. [tab_decor.cpp] Decor list context menu open/interaction path differs from JS ContextMenu component flow

- [ ] 677. [tab_decor.cpp] Missing return after PNG/CLIPBOARD export branch falls through

- [ ] 678. [tab_decor.cpp] create_renderer receives file_data_id instead of file_name

- [ ] 679. [tab_decor.cpp] getActiveRenderer() only returns M2 renderer not any active renderer

- [ ] 680. [tab_decor.cpp] Error toast for preview_decor missing View Log action button

- [ ] 681. [tab_decor.cpp] helper.mark on failure missing stack trace parameter

- [ ] 682. [tab_decor.cpp] Sorting uses byte comparison instead of locale-aware localeCompare

- [ ] 683. [tab_decor.cpp] Missing scrub pause/resume behavior on animation slider

- [ ] 684. [tab_decor.cpp] Missing Regex Enabled indicator in filter bar

- [ ] 685. [tab_decor.cpp] Missing tooltips on all sidebar Preview and Export checkboxes

- [ ] 686. [tab_decor.cpp] Category group header click-to-toggle-all not implemented

- [ ] 687. [tab_decor.cpp] WMO Groups and Doodad Sets use manual checkbox loop instead of CheckboxList

- [ ] 688. [tab_decor.cpp] Context menu popup may never open

- [ ] 689. [blp.cpp] Canvas rendering APIs (`toCanvas`/`drawToCanvas`) are not ported

- [ ] 690. [blp.cpp] WebP/PNG save methods are synchronous instead of JS async Promise APIs

- [ ] 691. [blp.cpp] 4-bit alpha nibble indexing behavior differs from original JS

- [ ] 692. [blp.cpp] DXT block overrun guard differs from JS equality check

- [ ] 693. [blp.cpp] `toBuffer()` fallback differs for unknown encodings

- [ ] 694. [blp.cpp] `getDataURL()` implementation differs — JS uses `toCanvas().toDataURL()`, C++ uses `toPNG()` then `BufferWrapper::getDataURL()`

- [ ] 695. [blp.cpp] `toCanvas()` and `drawToCanvas()` methods not ported — browser-specific

- [ ] 696. [blp.cpp] `dataURL` property initialized to `null` in JS constructor, C++ uses `std::optional<std::string>`

- [ ] 697. [blp.cpp] `toWebP()` uses libwebp C API directly instead of JS `webp-wasm` module

- [ ] 698. [blp.cpp] `_getCompressed()` DXT color interpolation uses integer division in C++ which may produce different rounding vs JS floating-point division

- [ ] 699. [blp.cpp] DXT5 alpha interpolation uses `| 0` (bitwise OR zero) in JS to floor, C++ uses integer division which truncates toward zero

- [ ] 700. [char-texture-overlay.cpp] Overlay button visibility updater is a no-op instead of internally toggling visibility

- [ ] 701. [char-texture-overlay.cpp] Active-layer reattach flow is stubbed out

- [ ] 702. [char-texture-overlay.cpp] Missing event registrations for tab switch and overlay navigation

- [ ] 703. [char-texture-overlay.cpp] getLayerCount/nextOverlay/prevOverlay are C++ additions not in JS module exports

- [ ] 704. [character-appearance.cpp] apply_customization_textures init() ordering differs from JS

- [ ] 705. [character-appearance.cpp] setTextureTarget API decomposed from composite objects to scalar parameters

- [ ] 706. [GLContext.cpp] Context creation and capability detection behavior differs from JS canvas/WebGL2 path

- [ ] 707. [ShaderProgram.cpp] `_compile` method handles partial shader failure differently from JS

- [ ] 708. [ShaderProgram.cpp] `set_uniform_3fv`/`set_uniform_4fv`/`set_uniform_mat4_array` have extra count parameter

- [ ] 709. [Shaders.cpp] C++ adds automatic _unregister_fn callback on ShaderProgram not present in JS

- [ ] 710. [Texture.cpp] `getTextureFile()` return contract differs from JS async/null behavior

- [ ] 711. [Skin.cpp] `load()` API timing differs from JS Promise-based async flow

- [ ] 712. [GridRenderer.cpp] GLSL shader version differs — C++ uses `#version 460 core`, JS uses `#version 300 es`

- [ ] 713. [ShadowPlaneRenderer.cpp] GLSL shader version differs — C++ uses `#version 460 core`, JS uses `#version 300 es`

- [ ] 714. [CameraControlsGL.cpp] Event listener lifecycle from JS `init()/dispose()` is not ported equivalently

- [ ] 715. [CameraControlsGL.cpp] Input default-handling behavior differs from JS browser event flow

- [ ] 716. [CameraControlsGL.cpp] Mouse-down focus fallback differs from JS

- [ ] 717. [CharacterCameraControlsGL.cpp] DOM/document listener registration-removal flow differs from JS

- [ ] 718. [CharacterCameraControlsGL.cpp] Event suppression parity is missing in mouse handlers

- [ ] 719. [uv-drawer.cpp] Output format changed from data URL string to raw RGBA pixels

- [ ] 720. [uv-drawer.cpp] Line rendering algorithm differs from Canvas 2D API

- [ ] 721. [model-viewer-gl.cpp] Character-mode reactive watchers are replaced with render-time polling

- [ ] 722. [model-viewer-gl.cpp] Active renderer contract is narrowed from JS duck-typed renderer to `M2RendererGL`

- [ ] 723. [model-viewer-gl.cpp] JS `controls.update()` called unconditionally but C++ splits into orbit/char controls with null checks

- [ ] 724. [model-viewer-gl.cpp] JS `context.controls = this.controls` stores single reference; C++ splits into `controls_orbit` and `controls_character`

- [ ] 725. [model-viewer-gl.cpp] JS `activeRenderer.animation_paused` is a property but C++ uses `is_animation_paused()` method

- [ ] 726. [model-viewer-gl.cpp] JS `beforeUnmount` cleans up watcher array but C++ has no equivalent watcher disposal

- [ ] 727. [model-viewer-gl.cpp] JS `window.addEventListener('resize', this.onResize)` but C++ FBO resize is implicit via ImGui layout

- [ ] 728. [model-viewer-gl.cpp] `handle_input` only processes events when `IsItemHovered` — JS events are document-level

- [ ] 729. [model-viewer-utils.cpp] Clipboard preview export copies base64 text instead of PNG image data

- [ ] 730. [model-viewer-utils.cpp] Animation selection guard treats empty string as null/undefined

- [ ] 731. [model-viewer-utils.cpp] WMO renderer/export constructor inputs differ from JS filename-based path

- [ ] 732. [model-viewer-utils.cpp] View-state proxy is hardcoded to three prefixes instead of dynamic property resolution

- [ ] 733. [model-viewer-utils.cpp] export_preview CLIPBOARD copies base64 text instead of PNG image data

- [ ] 734. [model-viewer-utils.cpp] WMO renderer and exporter pass file_data_id instead of file_name

- [ ] 735. [model-viewer-utils.cpp] create_view_state only supports 3 hardcoded prefixes

- [ ] 736. [MultiMap.cpp] MultiMap logic is not ported in the `.cpp` sibling translation unit

- [ ] 737. [MultiMap.cpp] Public API model differs from JS `Map` subclass contract

- [ ] 738. [M2Loader.cpp] Primary loader methods are synchronous instead of JS Promise-based async methods

- [ ] 739. [M2Loader.cpp] `loadAnims` error propagation differs from JS

- [ ] 740. [M2Loader.cpp] Model-name null stripping differs from original JS behavior

- [ ] 741. [M2Loader.cpp] `loadAnimsForIndex()` catch block logs fileDataID instead of animation.id

- [ ] 742. [M2Loader.cpp] `parseChunk_SFID` guard check uses `viewCount == 0` instead of undefined-check

- [ ] 743. [M2Loader.cpp] `parseChunk_TXID` guard check uses `textures.empty()` instead of undefined-check

- [ ] 744. [M2LegacyLoader.cpp] `load`/`getSkin` APIs are synchronous instead of JS Promise-based async methods

- [ ] 745. [M2Generics.cpp] Error message text differs in useAnims branch ("Unhandled" vs "Unknown")

- [ ] 746. [M3Loader.cpp] Loader methods are synchronous instead of JS Promise-based async methods

- [ ] 747. [MDXLoader.cpp] `load` API is synchronous instead of JS async Promise-based method

- [ ] 748. [MDXLoader.cpp] ATCH handler fixes JS `readUInt32LE(-4)` bug without TODO_TRACKER documentation

- [ ] 749. [MDXLoader.cpp] Node registration deferred to post-parsing (structural deviation)

- [ ] 750. [SKELLoader.cpp] Loader animation APIs are synchronous instead of JS Promise-based async methods

- [ ] 751. [SKELLoader.cpp] Animation-load failure handling differs from JS

- [ ] 752. [SKELLoader.cpp] Extra bounds check in `loadAnimsForIndex()` not present in JS

- [ ] 753. [SKELLoader.cpp] `skeletonBoneData` existence check uses `.empty()` instead of `!== undefined`

- [ ] 754. [SKELLoader.cpp] `loadAnims()` doesn't guard against missing `animFileIDs` like `loadAnimsForIndex()` does

- [ ] 755. [BONELoader.cpp] `load` API is synchronous instead of JS async Promise-based method

- [ ] 756. [ANIMLoader.cpp] `load` API is synchronous instead of JS async Promise-based method

- [ ] 757. [WMOLoader.cpp] `load`/`getGroup` APIs are synchronous instead of JS async methods

- [ ] 758. [WMOLoader.cpp] `getGroup` omits JS filename-based fallback when `groupIDs` are missing

- [ ] 759. [WMOLoader.cpp] `getGroup()` passes `groupFileID` to child constructor instead of no fileID

- [ ] 760. [WMOLoader.cpp] MOGP `flags` field renamed to `groupFlags`, diverging from JS property name

- [ ] 761. [WMOLoader.cpp] `hasLiquid` boolean is a C++ addition not present in JS

- [ ] 762. [WMOLoader.cpp] MOPR filler skip uses `data.move(4)` but per wowdev.wiki entry is 8 bytes total

- [ ] 763. [WMOLegacyLoader.cpp] `load`/internal load helpers/`getGroup` are synchronous instead of JS async methods

- [ ] 764. [WMOLegacyLoader.cpp] Group-loader initialization differs from JS in `getGroup`

- [ ] 765. [WMOLegacyLoader.cpp] MOGP `flags` field renamed to `groupFlags`, diverging from JS property name

- [ ] 766. [WMOLegacyLoader.cpp] `getGroup` empty-check differs for `groupCount == 0` edge case

- [ ] 767. [WDTLoader.cpp] `MWMO` string null handling differs from JS

- [ ] 768. [WDTLoader.cpp] `worldModelPlacement`/`worldModel`/MPHD fields not optional — cannot distinguish "chunk absent" from "chunk with zeros"

- [ ] 769. [ADTExporter.cpp] `calculateUVBounds` skips chunks when `vertices` is empty, unlike JS truthiness check

- [ ] 770. [ADTExporter.cpp] Export API flow is synchronous instead of JS Promise-based `async export()`

- [ ] 771. [ADTExporter.cpp] Scale factor check `!= 0` instead of `!== undefined` changes behavior for scale=0

- [ ] 772. [ADTExporter.cpp] GL index buffer uses GL_UNSIGNED_INT (uint32) instead of JS GL_UNSIGNED_SHORT (uint16)

- [ ] 773. [ADTExporter.cpp] Liquid JSON serialization uses explicit fields instead of JS spread operator

- [ ] 774. [ADTExporter.cpp] STB_IMAGE_RESIZE_IMPLEMENTATION defined at file scope risks ODR violation

- [ ] 775. [WMOShaderMapper.cpp] Pixel shader enum naming deviates from JS export contract

- [ ] 776. [CharMaterialRenderer.cpp] Core renderer methods are synchronous instead of JS Promise-based async methods

- [ ] 777. [CharMaterialRenderer.cpp] `getCanvas()` method missing — JS returns `this.glCanvas` for external use

- [ ] 778. [CharMaterialRenderer.cpp] `update()` draw call placement differs — C++ draws inside blend-mode conditional instead of after it

- [ ] 779. [CharMaterialRenderer.cpp] `setTextureTarget()` signature completely changed — JS takes full objects, C++ takes individual scalar parameters

- [ ] 780. [CharMaterialRenderer.cpp] `clearCanvas()` binds/unbinds FBO in C++ but JS does not

- [ ] 781. [CharMaterialRenderer.cpp] `dispose()` missing WebGL context loss equivalent

- [ ] 782. [CharacterExporter.cpp] `get_item_id_for_slot` does not preserve JS falsy fallback semantics

- [ ] 783. [CharacterExporter.cpp] remap_bone_indices truncates remap_table.size() to uint8_t causing incorrect comparison

- [ ] 784. [M2RendererGL.cpp] Multiple texture/skeleton/animation methods are synchronous instead of JS async methods

- [ ] 785. [M2RendererGL.cpp] Shader time uniform start-point differs from JS `performance.now()` baseline

- [ ] 786. [M2RendererGL.cpp] Reactive watchers not set up — `geosetWatcher`, `wireframeWatcher`, `bonesWatcher` completely missing

- [ ] 787. [M2RendererGL.cpp] `dispose()` missing watcher cleanup — no `geosetWatcher`, `wireframeWatcher`, `bonesWatcher` unregister

- [ ] 788. [M2RendererGL.cpp] Bone matrix upload uses SSBO instead of uniform array

- [ ] 789. [M2RendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`

- [ ] 790. [M2RendererGL.cpp] `overrideTextureTypeWithCanvas()` takes raw pixel data instead of canvas element

- [ ] 791. [M2LegacyRendererGL.cpp] Loader/skin/animation entrypoints are synchronous instead of JS async methods

- [ ] 792. [M2LegacyRendererGL.cpp] Reactive watchers not set up — `geosetWatcher` and `wireframeWatcher` completely missing

- [ ] 793. [M2LegacyRendererGL.cpp] `dispose()` missing watcher cleanup calls

- [ ] 794. [M2LegacyRendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`

- [ ] 795. [M2LegacyRendererGL.cpp] Track data property names differ from JS — uses `flatValues`/`nestedTimestamps` instead of `values`/`timestamps`

- [ ] 796. [M2LegacyRendererGL.cpp] `loadSkin()` geoset assignment to `core.view` uses JSON serialization instead of direct assignment

- [ ] 797. [M2LegacyRendererGL.cpp] `setSlotFile` called as `setSlotFileLegacy` — function name differs from JS

- [ ] 798. [M3RendererGL.cpp] Load APIs are synchronous instead of JS async methods

- [ ] 799. [M3RendererGL.cpp] `getBoundingBox()` missing vertex array empty check

- [ ] 800. [M3RendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`

- [ ] 801. [MDXRendererGL.cpp] Load and texture/animation paths are synchronous instead of JS async methods

- [ ] 802. [MDXRendererGL.cpp] Skeleton node flattening changes JS undefined/NaN behavior for `objectId`

- [ ] 803. [MDXRendererGL.cpp] Reactive watchers not set up — `geosetWatcher` and `wireframeWatcher` completely missing

- [ ] 804. [MDXRendererGL.cpp] `dispose()` missing watcher cleanup calls

- [ ] 805. [MDXRendererGL.cpp] `_create_skeleton()` doesn't initialize `node_matrices` to identity when nodes are empty

- [ ] 806. [MDXRendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`

- [ ] 807. [MDXRendererGL.cpp] Interpolation constants `INTERP_NONE/LINEAR/HERMITE/BEZIER` defined but never used in either JS or C++

- [ ] 808. [MDXRendererGL.cpp] `_build_geometry()` VAO setup passes 5 params instead of 6 — JS passes `null` as 6th parameter

- [ ] 809. [WMORendererGL.cpp] Load/update-set paths are synchronous instead of JS async methods

- [ ] 810. [WMORendererGL.cpp] Reactive view binding/watcher lifecycle differs from JS

- [ ] 811. [WMORendererGL.cpp] Reactive watchers replaced with polling — `groupWatcher`, `setWatcher`, `wireframeWatcher` not created

- [ ] 812. [WMORendererGL.cpp] `_load_textures()` `isClassic` check differs — JS tests `!!wmo.textureNames` (truthiness), C++ tests `!wmo->textureNames.empty()`

- [ ] 813. [WMORendererGL.cpp] `get_wmo_groups_view()`/`get_wmo_sets_view()` accessor methods don't exist in JS — C++ addition for multi-viewer support

- [ ] 814. [WMOLegacyRendererGL.cpp] Load/update-set paths are synchronous instead of JS async methods

- [ ] 815. [WMOLegacyRendererGL.cpp] Doodad-set iteration adds bounds guard not present in JS

- [ ] 816. [WMOLegacyRendererGL.cpp] Vue watcher-based reactive updates are replaced with render-time polling

- [ ] 817. [WMOLegacyRendererGL.cpp] Reactive watchers replaced with polling — `groupWatcher`, `setWatcher`, `wireframeWatcher` not created

- [ ] 818. [WMOLegacyRendererGL.cpp] Texture wrap flag logic potentially inverted

- [ ] 819. [data-exporter.cpp] SQL null handling differs for empty-string values

- [ ] 820. [data-exporter.cpp] Export failure records omit stack traces from helper marks

- [ ] 821. [data-exporter.cpp] exportDataTable defaults exportDBFormat to "CSV" while JS has no default

- [ ] 822. [export-helper.cpp] `getIncrementalFilename` is synchronous instead of JS async Promise API

- [ ] 823. [export-helper.cpp] Export failure stack-trace output target differs from JS

- [ ] 824. [texture-exporter.cpp] overwriteFiles config default is true in C++ vs undefined (falsy) in JS

- [ ] 825. [texture-exporter.cpp] Added .jpeg extension handling not present in JS

- [ ] 826. [texture-exporter.cpp] markFileName declared outside try-catch, fixing JS let-scoping bug

- [ ] 827. [M2Exporter.cpp] `addURITexture` input contract differs from JS (data URI string vs decoded PNG buffer)

- [ ] 828. [M2Exporter.cpp] Equipment UV2 export guard differs from JS truthy check

- [ ] 829. [M2Exporter.cpp] Data textures silently dropped from GLTF/GLB texture maps and buffers

- [ ] 830. [M2Exporter.cpp] uint16_t loop variable for triangle iteration risks overflow/infinite loop

- [ ] 831. [M2Exporter.cpp] addURITexture accepts BufferWrapper instead of data URI string

- [ ] 832. [M2Exporter.cpp] JSON submesh serialization uses fixed field enumeration instead of JS Object.assign

- [ ] 833. [M2Exporter.cpp] Data texture file manifest entries get fileDataID=0 instead of string key

- [ ] 834. [M2Exporter.cpp] formatUnknownFile call signature differs from JS

- [ ] 835. [M2LegacyExporter.cpp] Skin texture override condition differs when `skinTextures` is an empty array

- [ ] 836. [M2LegacyExporter.cpp] Export API flow is synchronous instead of JS Promise-based async methods

- [ ] 837. [M2LegacyExporter.cpp] uint16_t loop variable for triangle iteration risks overflow

- [ ] 838. [M3Exporter.cpp] `addURITexture` input contract differs from JS (data URI string vs decoded PNG buffer)

- [ ] 839. [M3Exporter.cpp] UV2 export condition checks non-empty instead of JS defined-ness

- [ ] 840. [M3Exporter.cpp] addURITexture accepts BufferWrapper instead of data URI string

- [ ] 841. [M3Exporter.cpp] exportTextures returns map<uint32_t, string> instead of JS Map with mixed key types

- [ ] 842. [WMOExporter.cpp] Export methods are synchronous instead of JS Promise-based async flow

- [ ] 843. [WMOExporter.cpp] uint16_t loop variable for face iteration risks overflow/infinite loop (4 locations)

- [ ] 844. [WMOExporter.cpp] Constructor takes explicit casc::CASC* parameter not present in JS

- [ ] 845. [WMOExporter.cpp] Extra loadWMO() and getDoodadSetNames() accessor methods not in JS

- [ ] 846. [WMOLegacyExporter.cpp] Export methods are synchronous instead of JS Promise-based async flow

- [ ] 847. [WMOLegacyExporter.cpp] uint16_t loop variable for face iteration risks overflow/infinite loop (2 locations)

- [ ] 848. [vp9-avi-demuxer.cpp] Parsing/extraction flow is synchronous callback-based instead of JS async APIs

- [ ] 849. [OBJWriter.cpp] `write()` is synchronous instead of JS Promise-based async method

- [ ] 850. [OBJWriter.cpp] `appendGeometry` UV handling differs — JS uses `Array.isArray`/spread, C++ uses `insert`

- [ ] 851. [OBJWriter.cpp] Face output format uses 1-based indexing with `v[i+1]//vn[i+1]` or `v[i+1]/vt[i+1]/vn[i+1]` — matches JS correctly

- [ ] 852. [OBJWriter.cpp] Only first UV set is written in OBJ faces; JS `this.uvs[0]` matches C++ `uvs[0]`

- [ ] 853. [MTLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method

- [ ] 854. [MTLWriter.cpp] `material.name` extraction uses `std::filesystem::path(name).stem().string()` but JS uses `path.basename(name, path.extname(name))`

- [ ] 855. [MTLWriter.cpp] MTL file uses `map_Kd` texture directive correctly matching JS

- [ ] 856. [GLTFWriter.cpp] Export entrypoint is synchronous instead of JS Promise-based async flow

- [ ] 857. [GLTFWriter.cpp] `add_scene_node` returns size_t index in C++ but the JS function returns the node object itself

- [ ] 858. [GLTFWriter.cpp] `add_buffered_accessor` lambda omits `target` from bufferView when `buffer_target < 0` in C++, JS passes `undefined` which is serialized differently

- [ ] 859. [GLTFWriter.cpp] Animation channel target node uses `actual_node_idx` (variable per prefix setting) but JS always uses `nodeIndex + 1`

- [ ] 860. [GLTFWriter.cpp] `bone_lookup_map` stores index-to-index mapping using `std::map<int, size_t>` instead of JS Map storing index-to-object

- [ ] 861. [GLTFWriter.cpp] Mesh primitive always includes `material` property in JS even when `materialMap.get()` returns `undefined`, C++ conditionally omits it

- [ ] 862. [GLTFWriter.cpp] Equipment mesh primitive always includes `material` in JS; C++ conditionally includes it

- [ ] 863. [GLTFWriter.cpp] `addTextureBuffer` method does not exist in JS — C++ addition

- [ ] 864. [GLTFWriter.cpp] Animation buffer name extraction in glb mode uses `rfind('_')` to extract `anim_idx`, but JS uses `split('_')` to get index at position 3

- [ ] 865. [GLTFWriter.cpp] `skeleton` variable in JS is a node object reference, C++ is a node index

- [ ] 866. [GLTFWriter.cpp] `usePrefix` is read inside the bone loop instead of outside like JS

- [ ] 867. [GLBWriter.cpp] GLB JSON chunk padding fills with NUL (0x00) instead of spaces (0x20) as required by the glTF 2.0 spec

- [ ] 868. [GLBWriter.cpp] Binary chunk padding uses zero bytes, matching JS behavior correctly

- [ ] 869. [JSONWriter.cpp] `write()` is synchronous and BigInt-stringify behavior differs from JS

- [ ] 870. [JSONWriter.cpp] `write()` uses `dump(1, '\t')` for pretty-printing; JS uses `JSON.stringify(data, null, '\t')`

- [ ] 871. [JSONWriter.cpp] `write()` default parameter correctly matches JS `overwrite = true`

- [ ] 872. [CSVWriter.cpp] `.cpp`/`.js` sibling contents are swapped, leaving `.cpp` as unconverted JavaScript

- [ ] 873. [CSVWriter.cpp] `addField()` overloaded — JS uses variadic `...fields`, C++ has two overloads (single string and vector)

- [ ] 874. [CSVWriter.cpp] `escapeCSVField()` handles `null`/`undefined` differently — JS converts via `.toString()`, C++ returns empty for empty string

- [ ] 875. [CSVWriter.cpp] `write()` default parameter differs — JS defaults `overwrite = true`, C++ has no default

- [ ] 876. [SQLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method

- [ ] 877. [SQLWriter.cpp] Empty-string SQL value handling differs from JS null/undefined checks

- [ ] 878. [SQLWriter.cpp] `addField()` overloaded — JS uses variadic `...fields`, C++ has two overloads (single string and vector)

- [ ] 879. [SQLWriter.cpp] `generateDDL()` output format differs slightly — C++ builds strings directly, JS uses `lines.join('\n')`

- [ ] 880. [SQLWriter.cpp] `toSQL()` format differs — JS uses `lines.join('\n')` with `value_rows.join(',\n') + ';'`, C++ concatenates directly

- [ ] 881. [STLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method

- [ ] 882. [STLWriter.cpp] Header string says `wow.export.cpp` while JS says `wow.export` — intentional branding difference

- [ ] 883. [STLWriter.cpp] `appendGeometry` simplified — C++ doesn't handle `Float32Array` vs `Array` distinction
