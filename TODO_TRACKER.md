# TODO Tracker

> **Progress: 22/186 verified (12%)** — ✅ = Verified, ⬜ = Pending

## Upstream Sync — port from wow.export JS @ d0d847f5

- [x] 1. [M2RendererGL.cpp] Port extensive upstream M2 renderer changes
  - **JS Source**: `src/js/3D/renderers/M2RendererGL.js`
  - **Status**: Verified
  - **Details**: Large batch of upstream commits rewrote the M2 renderer. Port: tex_matrices animation (469f3e2a); bone/tex_matrix UBO rework (a1689641); draw-call sorting by texunit prio/layer (201a7e0e); global_seq_times handling (f20a995c); tex_mat_idx/tex_matrices use (4bb373ab); always emit uv2/texcoord2 (4cc75854); blend mode mapping fix (09589e16); submesh colour animation/render (85264d30); lighting from render flag (8d40c107); conditional character model support (3b5aed51); texture weight transparency + skip zero-alpha draw calls (1cec313b); null guard for tex_matrices (85371a6e); UV2 Y-flip for OpenGL origin consistency (189fb5a2); division-by-zero guard on duplicate keyframe timestamps (0ac68161); fix redundant guard clause in _animate_track (4ea0c757); binary search for keyframe lookup (b2a70489); pre-allocate scratch matrices (93782b4c); wireframe via dedicated line index buffers (1715ee16); WMO doodad zero-initialized texture weight fix (52fea2af); MAX_BONES reduction and clamp (d9d48d8a).

- [ ] 2. [WMORendererGL.cpp] Port upstream WMO renderer changes
  - **JS Source**: `src/js/3D/renderers/WMORendererGL.js`
  - **Status**: Pending
  - **Details**: Commits to port: implement WMO shader changes — shader selection/dispatch overhaul (e27b4d63); fix semi-transparent textures via blend mode alpha control (590c2bb3); fix missing WMO rendering flags (3c68fefb); WMO UV improvements (7d57f471 — shared with loaders); wireframe via dedicated line index buffers (1715ee16).

- [ ] 3. [MDXRendererGL.cpp] Port upstream MDX renderer changes
  - **JS Source**: `src/js/3D/renderers/MDXRendererGL.js`
  - **Status**: Pending
  - **Details**: Commits to port: dispose bone UBOs to fix GPU memory leak (c962fdcb); move bone UBO creation outside geoset loop to avoid duplicate allocations (7a35d7e2); bone/tex_matrix UBO rework (a1689641 — shared); tex_mat_idx/tex_matrices (4bb373ab — shared); pre-allocate scratch matrices (93782b4c — shared); MAX_BONES reduction/clamp (d9d48d8a — shared); WMO UV improvements partial (7d57f471); wireframe via dedicated line index buffers (1715ee16).

- [ ] 4. [M2LegacyRendererGL.cpp / M3RendererGL.cpp / WMOLegacyRendererGL.cpp] Port shared upstream renderer changes to legacy renderers
  - **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js`, `M3RendererGL.js`, `WMOLegacyRendererGL.js`
  - **Status**: Pending
  - **Details**: Commits to port across legacy renderers: UV2 Y-flip for OpenGL origin consistency (189fb5a2 — M2Legacy + M3); tex_mat_idx/tex_matrices (4bb373ab — M2Legacy, M3, MDX); always emit uv2/texcoord2 (4cc75854 — M2Legacy, M3); bone UBO rework (a1689641 — M2Legacy, M3, MDX); pre-allocate scratch matrices (93782b4c — M2Legacy, MDX); wireframe via line index buffers (1715ee16 — all four); create_bones_ubo shared helper extraction (92f6e7a4 — see entry 5).

- [ ] 5. [renderer_utils.cpp / renderer_utils.h] Create new renderer_utils translation unit
  - **JS Source**: `src/js/3D/renderers/renderer_utils.js` (new file upstream)
  - **Status**: Pending
  - **Details**: Upstream commit 92f6e7a4 extracted a shared `create_bones_ubo` helper used by M2Renderer, M2LegacyRenderer, M3Renderer, and MDXRenderer into a new `renderer_utils.js`. Port as a new `renderer_utils.cpp`/`renderer_utils.h` pair and update all four renderers to call it instead of inlining UBO setup.

- [ ] 6. [GLContext.cpp / ShaderProgram.cpp / UniformBuffer.cpp / VertexArray.cpp] Port upstream GL subsystem fixes
  - **JS Source**: `src/js/3D/gl/GLContext.js`, `ShaderProgram.js`, `UniformBuffer.js`, `VertexArray.js`
  - **Status**: Pending
  - **Details**: Commits to port: fix blendmode.add in GLContext (f9e8f606); fix get_uniform_block_param passing name instead of index to getActiveUniformBlockParameter in ShaderProgram (aea65174); UniformBuffer + ShaderProgram rework for bones/tex_matrices UBO layout (a1689641 — shared with renderer entries); VertexArray: always emit uv2/texcoord2 attribute (4cc75854); VertexArray: generate dedicated line index buffers for wireframe (1715ee16).

- [ ] 7. [M2Loader.cpp / M2LegacyLoader.cpp / M3Loader.cpp / SKELLoader.cpp / WMOLoader.cpp / WMOLegacyLoader.cpp] Port upstream loader changes
  - **JS Source**: `src/js/3D/loaders/M2Loader.js`, `M2LegacyLoader.js`, `M3Loader.js`, `SKELLoader.js`, `WMOLoader.js`, `WMOLegacyLoader.js`
  - **Status**: Pending
  - **Details**: Commits to port: fix globalloops stored as 32-bit integers not 16-bit (9b403861 — M2Loader + SKELLoader); WMO UV improvements affecting UV data in WMOLoader, WMOLegacyLoader, M2LegacyLoader, M2Loader, M3Loader (7d57f471).

- [ ] 8. [WMOExporter.cpp / WMOLegacyExporter.cpp / OBJWriter.cpp] Port upstream WMO exporter changes
  - **JS Source**: `src/js/3D/exporters/WMOExporter.js`, `WMOLegacyExporter.js`, `src/js/3D/writers/OBJWriter.js`
  - **Status**: Pending
  - **Details**: Commits to port: implement WMO shader 20 OBJ/Blender export support (4ccde240 — WMOExporter + OBJWriter); improve WMO S20 OBJ exporting (a1b9d984 — WMOExporter); fix legacy WMO export skipping texture at MOTX offset 0 (73668717 — WMOLegacyExporter).

- [ ] 9. [M2Exporter.cpp / CharacterExporter.cpp / GLTFWriter.cpp] Port upstream M2/Character exporter and writer changes
  - **JS Source**: `src/js/3D/exporters/M2Exporter.js`, `CharacterExporter.js`, `src/js/3D/writers/GLTFWriter.js`
  - **Status**: Pending
  - **Details**: Commits to port: fix UV flipping for OBJ exports in M2Exporter (1022cd8d); fix item model exports missing material textures — requires new DBItemDisplayInfoModelMatRes cache (a71487d6); GLTFWriter UV/texcoord changes from WMO UV improvements batch (7d57f471); CharacterExporter item variant support (8fcce02e — coordinate with entry 11).

- [ ] 10. [tab_characters.cpp] Port extensive upstream character tab changes
  - **JS Source**: `src/js/modules/tab_characters.js`
  - **Status**: Pending
  - **Details**: Commits to port: apply guild crest from Battle.net character import (0b6923bf); apply item appearance modifiers from Battle.net character import (909aad0d); item variant support for character customization (8fcce02e); conditional character model support (3b5aed51); fix Earthen race import using Race_related fallback (512cb4e9); fix crash when copying item IDs in character tab (7f8d4f87); add support for independent shoulderpads (377aea87); streamline item equipping workflow (d0d847f5 — see entry 11 for new supporting files).

- [ ] 11. [tab_items.cpp / tab_item_sets.cpp / equip-item.cpp / item-picker-modal.cpp] Port item equipping rework and new files
  - **JS Source**: `src/js/modules/tab_items.js`, `tab_item_sets.js`, `src/js/wow/equip-item.js` (new), `src/js/components/item-picker-modal.js` (new)
  - **Status**: Pending
  - **Details**: Upstream commit d0d847f5 reworked item equipping, extracting logic into a new `equip-item.js` module and a new `item-picker-modal.js` UI component. Port these as new .cpp/.h pairs. Also port: item variant support changes to tab_items + tab_item_sets (8fcce02e); independent shoulderpad support in tab_items (377aea87).

- [ ] 12. [DB cache files] Port upstream DB cache additions and changes
  - **JS Source**: `src/js/db/caches/DBItemDisplays.js`, `DBItemGeosets.js`, `DBItemModels.js`, `DBItemCharTextures.js`, `DBCharacterCustomization.js`, `DBCreatures.js`, `DBItemDisplayInfoModelMatRes.js` (new), `DBItemList.js` (new)
  - **Status**: Pending
  - **Details**: New cache files to create as .cpp/.h pairs: `DBItemDisplayInfoModelMatRes` (addf146c, a71487d6 — maps ItemDisplayInfo model material resources); `DBItemList` (d0d847f5 — item list for the picker modal). Existing caches to update: DBItemDisplays — get textures from displayid (addf146c); DBItemGeosets — item variant + shoulderpad support (8fcce02e, 377aea87); DBItemModels — item variant + shoulderpad + displayid texture changes (8fcce02e, 377aea87, addf146c); DBItemCharTextures — item variant support (8fcce02e); DBCharacterCustomization + DBCreatures — conditional character model support (3b5aed51).

- [ ] 13. [listfile.cpp / dbd-manifest.cpp] Port upstream CASC subsystem fixes
  - **JS Source**: `src/js/casc/listfile.js`, `src/js/casc/dbd-manifest.js`
  - **Status**: Pending
  - **Details**: Commits to port: validate binary listfile index/data consistency on load and raise an error on mismatch (afa5d762); fix unnamed files missing from raw client files list (fba07dda); ensure DBD manifest is preloaded when prepareManifest is called early (a7425f7c). Note: several other CASC files also changed upstream (blp.js, blte-reader.js, build-cache.js, cdn-config.js, content-flags.js, export-helper.js, install-manifest.js, jenkins96.js, locale-flags.js, salsa20.js, tact-keys.js, version-config.js) — diff each against the corresponding C++ file and port any logic changes found.

- [ ] 14. [core.cpp / file-writer.cpp / generics.cpp] Port upstream core and utility fixes
  - **JS Source**: `src/js/core.js`, `src/js/file-writer.js`, `src/js/generics.js`
  - **Status**: Pending
  - **Details**: Commits to port: guard against last_export being a directory instead of a file in core + file-writer (3d8af3fc); fix downloadFile using synchronous chmod callback instead of async promise in generics (57fbdf05 — Linux-applicable); mask file-type bits from stored permissions in generics (67ecc5b8). Commits 6918ae93, 398a9f68, fe0be525, 20e899b2 touch updater.js, constants.js, and mmap.js for macOS-specific fixes — review each diff for any Windows/Linux-applicable logic before skipping.

- [ ] 15. [gpu-info.cpp] Port Windows GPU info fix — WMIC replaced with PowerShell CIM
  - **JS Source**: `src/js/gpu-info.js`
  - **Status**: Pending
  - **Details**: Commit 5c11b474 replaced the WMIC command with PowerShell CIM (`Get-CimInstance Win32_VideoController`) for querying GPU info on Windows, fixing broken GPU detection after WMIC was removed in Windows 11 24H2. Verify which Win32 API or shell command our C++ gpu-info implementation uses and apply the equivalent fix if needed.

- [ ] 16. [data-table.cpp / tab_data.cpp / legacy_tab_data.cpp] Port data table copy/export active-filter fix
  - **JS Source**: `src/js/components/data-table.js`, `src/js/modules/tab_data.js`, `src/js/modules/legacy_tab_data.js`
  - **Status**: Pending
  - **Details**: Commit 42afe166 fixed copy/export in the data table operating on the full unfiltered dataset instead of only the visible filtered rows. Port this fix to the C++ data-table component and both tab_data modules.

- [ ] 17. [tab_models.cpp / tab_creatures.cpp / model-viewer-utils.cpp] Port posed OBJ/STL export feature
  - **JS Source**: `src/js/modules/tab_models.js`, `src/js/modules/tab_creatures.js`, `src/js/ui/model-viewer-utils.js`
  - **Status**: Pending
  - **Details**: Commit 7dfca145 added posed (current animation pose) OBJ/STL export to both the models and creatures tabs, with shared logic in model-viewer-utils.js. Port the new export menu items/buttons in tab_models and tab_creatures, and the corresponding posed-export logic in model-viewer-utils.

- [ ] 18. [ShaderMapper.cpp / Skin.cpp / GeosetMapper.cpp / Texture.cpp] Port upstream 3D subsystem fixes
  - **JS Source**: `src/js/3D/ShaderMapper.js`, `src/js/3D/Skin.js`, `src/js/3D/GeosetMapper.js`, `src/js/3D/Texture.js`
  - **Status**: Pending
  - **Details**: Commits to port: fix vertex shader mapping in ShaderMapper (e23e20ff); change bone priority field from unsigned to signed char in Skin (740bf892). GeosetMapper.js and Texture.js also changed upstream — diff each against the corresponding C++ file and port any logic changes.

- [ ] 19. [modules.cpp / mmap.cpp / misc files] Port remaining upstream miscellaneous changes
  - **JS Source**: `src/js/modules.js`, `src/js/mmap.js`, `src/js/MultiMap.js`, `src/js/buffer.js`, `src/js/wow/ItemSlot.js`, `src/js/wow/EquipmentSlots.js`, `src/js/ui/texture-ribbon.js`, `src/js/3D/writers/CSVWriter.js`, `src/js/3D/writers/JSONWriter.js`, `src/js/3D/writers/MTLWriter.js`
  - **Status**: Pending
  - **Details**: Remaining files changed upstream not covered by other entries. Key items: modules.js registers the new item-picker-modal component (d0d847f5); mmap.js deduplicates INSTALL_PATH (5203828a); EquipmentSlots.js adds independent shoulderpad slots (377aea87). Diff CSVWriter, JSONWriter, MTLWriter, texture-ribbon, MultiMap, buffer, and ItemSlot against their C++ counterparts and port any logic changes found.


## Critical — crashes, data corruption, and completely broken core features

- [x] 20. [map-viewer.cpp] Tile image drawing path is still unimplemented
- **JS Source**: `src/js/components/map-viewer.js` lines 380–402, 1111–1113
- **Status**: Verified
- **Details**: Implemented. `loadTile()` now uploads RGBA pixel data as a GL texture (`glGenTextures`/`glTexImage2D`) and stores the handle in `s_state.tileTextures`. A new `renderTiles()` function iterates the rendered set and draws each tile via `ImDrawList::AddImage()`, clipped to the viewport. Called before `renderOverlay()` so overlays appear on top. GL textures are deleted on tile eviction (`renderWithDoubleBuffer` cleanup), full redraw, and `clearTileState()`.

- [x] 21. [map-viewer.cpp] Tile rendering to canvas via GL textures not implemented — tiles cached in memory but not displayed
- **JS Source**: `src/js/components/map-viewer.js` lines 350–500 (loadTile, renderWithDoubleBuffer)
- **Status**: Verified
- **Details**: Resolved together with entry 20. `MapViewerPersistedState` gained a `tileTextures: unordered_map<int, uint32_t>` member. `loadTile` uploads each tile to a GL texture. `renderTiles` draws all rendered tiles each frame via `AddImage`. Proper GL lifetime management: textures deleted in `clearTileState`, `renderFullRedraw`, and per-tile eviction in `renderWithDoubleBuffer`.

- [x] 22. [tab_install.cpp] CASC getFile replaced with low-level two-step call, losing BLTE decoding
- **JS Source**: `src/js/modules/tab_install.js` lines 73–74
- **Status**: Verified
- **Details**: JS calls core.view.casc.getFile() which returns a BLTEReader for BLTE block decompression. C++ calls getEncodingKeyForContentKey then _ensureFileInCache then BufferWrapper::readFile, skipping BLTE decompression entirely. Exported files may be corrupt.

- [x] 23. [tab_install.cpp] processAllBlocks() call missing in view_strings_impl
- **JS Source**: `src/js/modules/tab_install.js` lines 103–105
- **Status**: Verified
- **Details**: JS calls data.processAllBlocks() after getFile() to force all BLTE blocks decompressed before extract_strings. C++ skips this step because it uses plain BufferWrapper instead of BLTEReader.

- [x] 24. [tab_videos.cpp] Video preview playback is opened externally instead of using an embedded player
- **JS Source**: `src/js/modules/tab_videos.js` lines 219–276, 493
- **Status**: Verified
- **Details**: JS renders and controls an in-tab `<video>` element with `onended`/`onerror` and subtitle track attachment, while C++ opens the stream URL in an external handler and shows status text in the preview area.

- [x] 25. [tab_videos.cpp] Video export format selector from MenuButton is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 505, 559–571
- **Status**: Verified
- **Details**: JS uses a `MenuButton` bound to `config.exportVideoFormat` and dispatches format-specific export via selection; C++ renders a single `Export Selected` button with no in-UI format picker.

- [x] 26. [tab_videos.cpp] MenuButton export format dropdown completely missing
- **JS Source**: `src/js/modules/tab_videos.js` line 505
- **Status**: Verified
- **Details**: JS uses `<MenuButton :options="menuButtonVideos" :default="config.exportVideoFormat" @change="..." @click="export_selected">` which renders a dropdown to pick MP4/AVI/MP3/SUBTITLES and triggers export. C++ renders a plain `ImGui::Button("Export Selected")` with no format selector. Users cannot change the export format from this tab.

- [x] 27. [tab_videos.cpp] Video preview is text-only, not an embedded player
- **JS Source**: `src/js/modules/tab_videos.js` line 493
- **Status**: Verified
- **Details**: JS renders a `<video>` element with full controls, autoplay, subtitles overlay via `<track>`. C++ opens the URL in the system's external media player (`core::openInExplorer(url)`) and shows plain text. No inline playback, no controls, no subtitle overlay in the app window.

- [x] 28. [legacy_tab_data.cpp] DBC filename extraction uses `std::filesystem::path` which won't split backslash-delimited MPQ paths on Linux
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 33–36
- **Status**: Verified
- **Details**: JS uses `full_path.split('\\')` to extract the DBC filename from backslash-delimited MPQ paths. C++ uses `std::filesystem::path(full_path).filename()` (line 81). On Linux, `std::filesystem::path` treats `\` as a regular character, not a separator, so `filename()` would return the entire path string instead of just the filename. This would cause the table name extraction to fail for MPQ paths like `DBFilesClient\Achievement.dbc`.

- [x] 29. [tab_raw.cpp] export_raw_files uses getVirtualFileByName and drops partialDecrypt=true
- **JS Source**: `src/js/modules/tab_raw.js` line 123
- **Status**: Verified
- **Details**: JS calls core.view.casc.getFileByName(file_name, true) passing partialDecrypt=true. C++ calls getVirtualFileByName(file_name) without partialDecrypt parameter, silently dropping partial decryption capability for encrypted files.

- [x] 30. [tab_maps.cpp] load_wmo_minimap_tile ignores drawX/drawY and scaleX/scaleY positioning
- **JS Source**: `src/js/modules/tab_maps.js` lines 107–112
- **Status**: Verified
- **Details**: JS draws each tile at its specific offset (tile.drawX * output_scale, tile.drawY * output_scale) with scaled dimensions. C++ ignores drawX, drawY, scaleX, scaleY entirely — stretching all tiles to fill the full cell. Multi-tile compositing within a grid cell is completely broken.

- [x] 31. [tab_maps.cpp] WDT file load is outside try-catch block
- **JS Source**: `src/js/modules/tab_maps.js` lines 433–434
- **Status**: Verified
- **Details**: In JS, getFileByName(wdt_path) is inside the try block. In C++, getVirtualFileByName(wdt_path) is BEFORE the try block. If WDT file doesn't exist, exception propagates uncaught.

- [x] 32. [tab_maps.cpp] Missing optional chaining for export_paths
- **JS Source**: `src/js/modules/tab_maps.js` lines 752–853
- **Status**: Verified
- **Details**: JS uses optional chaining export_paths?.writeLine() and export_paths?.close(). C++ calls directly without null checks. If openLastExportStream returns invalid object, C++ will crash.

- [x] 33. [tab_zones.cpp] Base tile relation lookup uses layer-row ID instead of UiMapArt ID
- **JS Source**: `src/js/modules/tab_zones.js` lines 120–121
- **Status**: Verified
- **Details**: JS fetches `UiMapArtTile` relation rows with `art_style.ID` from the UiMapArt entry; C++ stores `CombinedArtStyle::id` as the UiMapArtStyleLayer row ID and uses that in `getRelationRows`, altering tile resolution.

- [x] 34. [tab_zones.cpp] UiMapArtStyleLayer join uses wrong field name
- **JS Source**: `src/js/modules/tab_zones.js` lines 88–91
- **Status**: Verified
- **Details**: JS joins `art_style_layer.UiMapArtStyleID === art_entry.UiMapArtStyleID`. C++ joins on `layer_row["UiMapArtID"]` — a completely different field name. The C++ looks for "UiMapArtID" in UiMapArtStyleLayer table, but JS matches on "UiMapArtStyleID" from both tables. This produces wrong rows or no rows.

- [x] 35. [tab_zones.cpp] CombinedArtStyle.id stores wrong ID (layer ID vs art ID)
- **JS Source**: `src/js/modules/tab_zones.js` lines 94–101
- **Status**: Verified
- **Details**: JS `combined_style` includes `...art_entry` (spread), so `combined_style.ID` = the UiMapArt row ID (`art_id`). C++ sets `style.id = static_cast<int>(layer_id)` which is the UiMapArtStyleLayer table key. This wrong ID propagates to `getRelationRows()` calls for UiMapArtTile and WorldMapOverlay.

- [x] 36. [tab_zones.cpp] C++ adds ALL matching style layers; JS keeps only LAST
- **JS Source**: `src/js/modules/tab_zones.js` lines 86–91
- **Status**: Verified
- **Details**: JS declares `let style_layer;` then overwrites in a loop, keeping only the last match. C++ `push_back`s every matching row into `art_styles`. This creates duplicate/extra entries causing redundant or incorrect rendering.

- [x] 37. [tab_zones.cpp] Unsafe Windows wstring conversion corrupts multi-byte UTF-8 paths
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Verified
- **Details**: `std::wstring wpath(dir.begin(), dir.end())` does byte-by-byte copy which corrupts multi-byte UTF-8 paths. Should use `MultiByteToWideChar` or equivalent.

- [x] 38. [tab_zones.cpp] Linux shell command injection risk in openInExplorer
- **JS Source**: `src/js/modules/tab_zones.js` line 393
- **Status**: Verified
- **Details**: `"xdg-open \"" + dir + "\" &"` passed to `std::system()`. If `dir` contains shell metacharacters, this is exploitable. JS uses `nw.Shell.openItem` which is safe.

- [x] 39. [tab_creatures.cpp] has_content check and toast/camera logic scoped incorrectly
- **JS Source**: `src/js/modules/tab_creatures.js` lines 713–722
- **Status**: Verified
- **Details**: In JS, the has_content check, hideToast, and fitCamera are outside the if/else running for both character and standard models. In C++ this block is inside the else (standard-model only). For character-model creatures, the loading toast is never dismissed.

- [x] 40. [tab_creatures.cpp] Collection model geoset logic has three bugs
- **JS Source**: `src/js/modules/tab_creatures.js` lines 421–429
- **Status**: Verified
- **Details**: (1) JS calls hideAllGeosets() before applying - C++ never does. (2) JS uses mapping.group_index for lookup - C++ uses coll_idx. (3) JS uses mapping.char_geoset for setGeosetGroupDisplay - C++ uses mapping.group_index.

- [ ] 41. [tab_decor.cpp] PNG/CLIPBOARD export branch does not short-circuit like JS
- **JS Source**: `src/js/modules/tab_decor.js` lines 129–140
- **Status**: Pending
- **Details**: JS returns immediately after preview export for PNG/CLIPBOARD; C++ closes the export stream but continues into full model export logic, changing export behavior for these formats.

- [ ] 42. [tab_decor.cpp] Missing return after PNG/CLIPBOARD export branch falls through
- **JS Source**: `src/js/modules/tab_decor.js` lines 138–140
- **Status**: Pending
- **Details**: JS does return after PNG/CLIPBOARD block. C++ has no return so execution falls through to the ExportHelper loop redundantly exporting all selected entries as models.

- [ ] 43. [tab_decor.cpp] create_renderer receives file_data_id instead of file_name
- **JS Source**: `src/js/modules/tab_decor.js` line 85
- **Status**: Pending
- **Details**: JS passes file_name (string) as 5th argument to create_renderer. C++ passes file_data_id (uint32_t). Parameter type mismatch.

- [ ] 44. [tab_decor.cpp] getActiveRenderer() only returns M2 renderer not any active renderer
- **JS Source**: `src/js/modules/tab_decor.js` line 611
- **Status**: Pending
- **Details**: JS getActiveRenderer returns the single active_renderer which could be M2, WMO, or M3. C++ returns active_renderer_result.m2.get() returning nullptr when active model is WMO or M3.


## High — significant functional bugs and major behavioral differences

- [ ] 45. [legacy_tab_audio.cpp] `load_track` checks `player.get_duration() <= 0` vs JS `!player.buffer`
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 100–101
- **Status**: Pending
- **Details**: JS `play_track` checks `!player.buffer` to determine if a track needs loading. C++ checks `player.get_duration() <= 0` (line 136). If a loaded track has zero duration (e.g., corrupt file that loads but has 0-length), C++ would re-load while JS would not. The check semantics are subtly different.

- [ ] 46. [tab_videos.cpp] Corrupted AVI fallback does not force CASC fallback fetch path
- **JS Source**: `src/js/modules/tab_videos.js` line 697
- **Status**: Pending
- **Details**: JS retries corrupted cinematic reads with `getFileByName(file_name, false, false, true, true)` to force fallback behavior; C++ retries `getVirtualFileByName(file_name)` with normal arguments.

- [ ] 47. [tab_videos.cpp] AVI export corruption fallback is a no-op
- **JS Source**: `src/js/modules/tab_videos.js` line 697
- **Status**: Pending
- **Details**: JS calls `getFileByName(file_name, false, false, true, true)` with extra params (forceFallback). C++ calls `getVirtualFileByName(file_name)` identically to the first attempt, with a comment admitting `// Note: C++ getVirtualFileByName doesn't support forceFallback; retry normally.` The corruption recovery path will always fail the same way twice.

- [ ] 48. [tab_videos.cpp] No onended/onerror callbacks for video playback
- **JS Source**: `src/js/modules/tab_videos.js` lines 263–275
- **Status**: Pending
- **Details**: JS attaches `video.onended` (resets `is_streaming`/`videoPlayerState`) and `video.onerror` (shows error toast). C++ delegates to external player and has neither callback — `is_streaming` and `videoPlayerState` are never automatically reset when playback finishes; user must manually click "Stop Video."

- [ ] 49. [tab_videos.cpp] stop_video does not join/stop background thread
- **JS Source**: `src/js/modules/tab_videos.js` lines 27–57
- **Status**: Pending
- **Details**: JS clears `setTimeout` handle which fully cancels pending work. C++ sets `poll_cancelled = true` but does not `reset()` or join `stream_worker_thread`. The thread may still be running and post results after stop. Only `stream_video` joins it before a new stream.

- [ ] 50. [tab_videos.cpp] MP4 download HTTP error check missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 631–633
- **Status**: Pending
- **Details**: JS explicitly checks `if (!response.ok)` and marks the file with 'Failed to download MP4: ' + response.status. C++ uses `generics::get(*mp4_url)` with no status check — if the server returns a non-200 status, behavior depends on `generics::get()` implementation.

- [ ] 51. [tab_data.cpp] Data-table cell copy stringification differs from JS `String(value)` behavior
- **JS Source**: `src/js/modules/tab_data.js` lines 172–177
- **Status**: Pending
- **Details**: JS copies with `String(value)`, while C++ uses `value.dump()`; for string JSON values this includes JSON quoting/escaping, changing clipboard output.

- [ ] 52. [tab_data.cpp] Context menu node not cleared on close
- **JS Source**: `src/js/modules/tab_data.js` line 107
- **Status**: Pending
- **Details**: JS resets nodeDataTable to null on close. C++ never resets it so the condition stays true permanently.

- [ ] 53. [tab_data.cpp] copy_cell uses value.dump() producing JSON-quoted strings
- **JS Source**: `src/js/modules/tab_data.js` lines 172–177
- **Status**: Pending
- **Details**: JS uses String(value) producing unquoted output. C++ uses value.dump() adding extra quotes for strings.

- [ ] 54. [tab_data.cpp] Selection watcher prevents retry after failed load
- **JS Source**: `src/js/modules/tab_data.js` lines 371–377
- **Status**: Pending
- **Details**: JS compares selected_file which is not updated on failure allowing retry. C++ compares prev_selection_last updated unconditionally preventing retry.

- [ ] 55. [legacy_tab_data.cpp] `copy_cell` empty-string handling differs from JS
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 215–220
- **Status**: Pending
- **Details**: JS copies any non-null/undefined value (including empty string), while C++ returns early on `value.empty()`, so empty-cell clipboard behavior is not equivalent.

- [ ] 56. [tab_raw.cpp] detect_raw_files manually sets is_dirty=true — deviates from JS
- **JS Source**: `src/js/modules/tab_raw.js` lines 75–76
- **Status**: Pending
- **Details**: JS calls listfile.ingestIdentifiedFiles then compute_raw_files without setting is_dirty. Since is_dirty was false, JS would return early (apparent JS bug). C++ adds is_dirty=true to fix this, which is arguably correct but deviates from original JS behavior.

- [ ] 57. [tab_maps.cpp] Hand-rolled MD5 instead of mbedTLS
- **JS Source**: `src/js/modules/tab_maps.js` line 914
- **Status**: Pending
- **Details**: C++ implements a full RFC 1321 MD5 from scratch instead of using mbedTLS MD API (mbedtls/md.h) as specified in project conventions.

- [ ] 58. [tab_maps.cpp] load_map_tile uses nearest-neighbor scaling instead of bilinear interpolation
- **JS Source**: `src/js/modules/tab_maps.js` lines 62–71
- **Status**: Pending
- **Details**: JS Canvas 2D drawImage performs bilinear interpolation when scaling. C++ uses nearest-neighbor sampling (integer coordinate snapping), making scaled minimap tiles look blockier/pixelated.

- [ ] 59. [tab_maps.cpp] load_map_tile uses blp.width instead of blp.scaledWidth
- **JS Source**: `src/js/modules/tab_maps.js` line 62
- **Status**: Pending
- **Details**: JS computes scale as size / blp.scaledWidth. C++ uses blp.width. If the BLP has a scaledWidth differing from raw width (e.g. mipmaps), the scaling factor will be wrong.

- [ ] 60. [tab_maps.cpp] export_map_wmo_minimap uses max-alpha instead of source-over compositing
- **JS Source**: `src/js/modules/tab_maps.js` lines 721–733
- **Status**: Pending
- **Details**: JS Canvas 2D drawImage uses Porter-Duff source-over compositing. C++ export computes alpha as max(dst_alpha, src_alpha) instead of correct source-over formula.

- [ ] 61. [tab_maps.cpp] mapViewerHasWorldModel check differs from JS
- **JS Source**: `src/js/modules/tab_maps.js` lines 438–439
- **Status**: Pending
- **Details**: JS checks if (wdt.worldModelPlacement) — any non-null object is truthy. C++ checks if (worldModelPlacement.id != 0), which misses placement with id=0. Also affects has_global_wmo and export_map_wmo checks.

- [ ] 62. [tab_maps.cpp] collect_game_objects returns vector instead of Set
- **JS Source**: `src/js/modules/tab_maps.js` lines 146–157
- **Status**: Pending
- **Details**: JS returns a Set guaranteeing uniqueness. C++ returns std::vector<ADTGameObject> which can contain duplicates.

- [ ] 63. [tab_maps.cpp] Selection watch may miss intermediate changes
- **JS Source**: `src/js/modules/tab_maps.js` lines 1135–1143
- **Status**: Pending
- **Details**: JS Vue $watch triggers on any reactive change. C++ only compares the first element string between frames. If selection changes and reverts within same frame, or changes to different item with same first entry, C++ misses it.

- [ ] 64. [tab_zones.cpp] Default phase filtering excludes non-zero phases unlike JS
- **JS Source**: `src/js/modules/tab_zones.js` lines 78–79
- **Status**: Pending
- **Details**: JS includes all `UiMapXMapArt` links when `phase_id === null`; C++ filters to `PhaseID == 0` when no phase is selected.

- [ ] 65. [tab_zones.cpp] UiMapArtStyleLayer lookup key differs from JS relation logic
- **JS Source**: `src/js/modules/tab_zones.js` lines 88–90
- **Status**: Pending
- **Details**: JS resolves style layers by matching `UiMapArtStyleID` to `art_entry.UiMapArtStyleID`; C++ matches `UiMapArtID` to the linked art ID, changing style-layer association behavior.

- [ ] 66. [tab_zones.cpp] Base map tile OffsetX/OffsetY offsets are ignored
- **JS Source**: `src/js/modules/tab_zones.js` lines 181–182
- **Status**: Pending
- **Details**: JS applies `tile.OffsetX`/`tile.OffsetY` when placing map tiles; C++ calculates tile position from row/column and tile dimensions only.

- [ ] 67. [tab_zones.cpp] Phase filter logic differs when phase_id is null
- **JS Source**: `src/js/modules/tab_zones.js` line 78
- **Status**: Pending
- **Details**: JS: `if (phase_id === null || link_entry.PhaseID === phase_id)` — when phase_id is null, ALL entries are included. C++: when `phase_id` is nullopt, only entries with `row_phase == 0` are included. C++ omits non-default phases when no phase is specified, while JS shows all.

- [ ] 68. [tab_zones.cpp] Missing tile OffsetX/OffsetY in render_map_tiles
- **JS Source**: `src/js/modules/tab_zones.js` lines 181–182
- **Status**: Pending
- **Details**: JS: `final_x = pixel_x + (tile.OffsetX || 0); final_y = pixel_y + (tile.OffsetY || 0)`. C++ only uses `pixel_x = col * tile_width; pixel_y = row * tile_height` with no offset. Tiles with non-zero offsets will be mispositioned.

- [ ] 69. [tab_zones.cpp] Tile layer rendering architecture differs from JS
- **JS Source**: `src/js/modules/tab_zones.js` lines 126–152
- **Status**: Pending
- **Details**: JS groups ALL tiles for an art_style by their LayerIndex, then renders each group in sorted order. C++ calls `render_map_tiles(art_style, art_style.layer_index, ...)` which filters tiles to only those matching the single layer_index. Combined with the duplicate style layers issue, rendering pipeline differs significantly.

- [ ] 70. [tab_zones.cpp] parse_zone_entry doesn't throw on bad input
- **JS Source**: `src/js/modules/tab_zones.js` lines 17–18
- **Status**: Pending
- **Details**: JS throws `new Error('unexpected zone entry')` on regex mismatch. C++ returns an empty `ZoneDisplayInfo{}` with `id=0`. Callers add `zone.id > 0` guards, but error propagation differs.

- [ ] 71. [tab_zones.cpp] UiMap row existence not validated
- **JS Source**: `src/js/modules/tab_zones.js` lines 67–71
- **Status**: Pending
- **Details**: JS checks `if (!map_data)` and throws `'UiMap entry not found'`. C++ fetches the row but casts to void: `(void)ui_map_row_opt;` — never checks the result or throws.

- [ ] 72. [tab_zones.cpp] Pixel buffer not cleared at start of render when first layer is non-zero
- **JS Source**: `src/js/modules/tab_zones.js` line 59
- **Status**: Pending
- **Details**: JS calls `ctx.clearRect(0, 0, canvas.width, canvas.height)` at the start. C++ only allocates/clears the pixel buffer inside the `if (art_style.layer_index == 0)` block. If the first art_style has layer_index != 0, stale pixel data remains.

- [ ] 73. [tab_items.cpp] std::set ordering differs from JS Set insertion order
- **JS Source**: `src/js/modules/tab_items.js` lines 85–127
- **Status**: Pending
- **Details**: view_item_models and view_item_textures use JS Set which preserves insertion order. C++ uses std::set<std::string> which sorts lexicographically. Should use std::vector with uniqueness check.

- [ ] 74. [tab_items.cpp] itemViewerShowAll second loop superseded by upstream DBItemList refactor
- **JS Source**: `src/js/modules/tab_items.js`
- **Status**: Pending
- **Details**: The original bug (C++ using DBItems::getItemById instead of Item constructor reading item_row.Display_lang) is now moot — upstream commit d0d847f5 removed the entire second loop and replaced item loading with DBItemList.initialize() / DBItemList.loadShowAllItems(). The C++ implementation needs to be redone to match the new DBItemList-based approach. See entry 11 for the full item equipping rework scope.

- [ ] 75. [tab_item_sets.cpp] fieldToUint32Vec does not handle single-value fields
- **JS Source**: `src/js/modules/tab_item_sets.js` line 38
- **Status**: Pending
- **Details**: JS set_row.ItemID is expected to be an array with .filter(id => id !== 0). C++ fieldToUint32Vec only handles vector variants. If a DB2 reader returns a single scalar, the function returns an empty vector, silently dropping data.

- [ ] 76. [tab_creatures.cpp] Scrubber IsItemActivated() called before SliderInt checks wrong widget
- **JS Source**: `src/js/modules/tab_creatures.js` lines 1035–1038
- **Status**: Pending
- **Details**: C++ calls IsItemActivated() before SliderInt() renders. IsItemActivated checks the last widget (Step-Right button) not the slider. start_scrub() will never fire correctly.

- [ ] 77. [tab_creatures.cpp] Missing export_paths.writeLine calls in multiple export paths
- **JS Source**: `src/js/modules/tab_creatures.js` lines 792, 929 (approx — shifted by upstream changes)
- **Status**: Pending
- **Details**: JS writes to export_paths for RAW character-model export (line 792) and non-RAW character-model export (line 929). C++ omits all writeLine calls and does not pass export_paths to export_model. Scope expanded by upstream commit 7dfca145 (entry 17) which added a posed export path via modelViewerUtils.export_preview — verify that path also handles export_paths correctly (currently returns early at line 747 via export_paths?.close() without writing).

- [ ] 78. [tab_creatures.cpp] GLTF format.toLowerCase() not applied
- **JS Source**: `src/js/modules/tab_creatures.js` line 921
- **Status**: Pending
- **Details**: JS passes format.toLowerCase() to exportAsGLTF. C++ passes format as-is (uppercase). If the exporter is case-sensitive output may differ.

- [ ] 79. [tab_decor.cpp] Context menu popup may never open
- **JS Source**: `src/js/modules/tab_decor.js` lines 233–237
- **Status**: Pending
- **Details**: The DecorListboxContextMenu popup requires ImGui::OpenPopup to be called. The handle_listbox_context callback does not open this popup. The popup rendering code will never trigger.

- [ ] 80. [app.h / app.cpp] Remove `applyTheme()` and all CSS-color constants; replace with `ImGui::StyleColorsDark()`
  - **JS Source**: `src/app.css` (CSS variables — all `:root` color definitions)
  - **Status**: In Progress
  - **Details**: `applyTheme()` call replaced with `ImGui::StyleColorsDark()` (done in prior session). All CSS-color push/pop calls and color constant aliases removed from `app.cpp`. The `app::theme` namespace color constants in `app.h` remain and are still referenced by entries 631–636 files. Full removal of the constants from `app.h` is blocked on entries 631–636.


## Medium — behavioral deviations, missing features, and config bindings

- [ ] 81. [map-viewer.cpp] Box-select-mode active color uses NAV_SELECTED (#22B549) instead of CSS #5fdb65
- **JS Source**: `src/app.css` line 1348–1349
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .info span.active` uses `color: #5fdb65` (RGB 95, 219, 101). C++ line 1178 uses `app::theme::NAV_SELECTED` which maps to `BUTTON_BASE` = `(0.133, 0.710, 0.286)` = RGB(34, 181, 73). The active highlight color is noticeably different — should be a dedicated color constant matching #5fdb65.

- [ ] 82. [map-viewer.cpp] Map-viewer info bar text lacks CSS `text-shadow: black 0 0 6px`
- **JS Source**: `src/app.css` lines 1326–1328
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer div { text-shadow: black 0 0 6px; }` applies a text shadow to all divs inside the map viewer, including the info bar and hover-info. C++ renders these with plain `ImGui::TextUnformatted` (lines 1166–1189) with no text shadow effect. ImGui does not natively support text shadows, so a manual shadow would need to be rendered (draw text offset in black, then draw normal text on top).

- [ ] 83. [map-viewer.cpp] Map-viewer info bar spans missing CSS `margin: 0 10px` horizontal spacing
- **JS Source**: `src/app.css` lines 1345–1346
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .info span { margin: 0 10px; }` gives each info label 10px left/right margin. C++ uses `ImGui::SameLine()` between items (lines 1167–1175) with default spacing. The default ImGui item spacing is typically ~8px which may not exactly match the 20px total gap (10px + 10px) between spans.

- [ ] 84. [map-viewer.cpp] Map-viewer checkerboard background pattern not implemented
- **JS Source**: `src/app.css` lines 1300–1306
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer` has a complex checkerboard background using `background-image: linear-gradient(45deg, ...)` with `--trans-check-a` and `--trans-check-b` colors, `background-size: 30px 30px`, and `background-position: 0 0, 15px 15px`. C++ `renderWidget` (line 1148) uses `ImGui::BeginChild` with no custom background drawing — the checkerboard transparency pattern is missing entirely. The JS version shows a checkerboard behind transparent map tiles.

- [ ] 85. [map-viewer.cpp] Map-viewer hover-info positioned at top via ImGui layout instead of CSS `bottom: 3px; left: 3px`
- **JS Source**: `src/app.css` lines 1330–1333
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .hover-info` is positioned `bottom: 3px; left: 3px` (bottom-left corner of the map viewer). C++ renders hover-info at line 1187–1189 via `ImGui::SameLine()` after the info bar spans, placing it inline at the top. It should be rendered at the bottom-left of the map viewer container using `ImGui::SetCursorPos` or overlay drawing.

- [ ] 86. [map-viewer.cpp] Box-select-mode cursor not changed to crosshair
- **JS Source**: `src/app.css` lines 1351–1353
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer.box-select-mode { cursor: crosshair; }` changes the cursor to a crosshair when box-select mode is active. C++ does not call `ImGui::SetMouseCursor(ImGuiMouseCursor_Hand)` or any cursor change when `state.isBoxSelectMode` is true. ImGui does not have a native crosshair cursor, but this should at least document the visual difference.

- [ ] 87. [map-viewer.cpp] `handleTileInteraction` emits selection changes via mutable reference instead of `$emit('update:selection')`
- **JS Source**: `src/js/components/map-viewer.js` lines 846–874
- **Status**: Pending
- **Details**: JS `handleTileInteraction` modifies `this.selection` array directly (splice/push) and Vue reactivity propagates changes via `v-model`. C++ modifies the `selection` vector reference directly (lines 845–848). However, JS Select All (line 812) uses `this.$emit('update:selection', newSelection)` with a new array — the C++ equivalent calls `onSelectionChanged(newSelection)` callback in `handleKeyPress` (line 780) and `finalizeBoxSelection` (line 941). This means `handleTileInteraction` mutates in-place but Select All and box-select create new arrays — potential inconsistency in selection update patterns.

- [ ] 88. [map-viewer.cpp] Map margin `20px 20px 0 10px` from CSS not applied
- **JS Source**: `src/app.css` line 1307
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer { margin: 20px 20px 0 10px; }` adds specific margins around the map viewer. C++ `renderWidget` uses `ImGui::GetContentRegionAvail()` for sizing (line 1144) without adding any padding/margin equivalent. The parent layout is responsible for this in ImGui, but if not handled there, the map viewer will be flush against adjacent elements.

- [ ] 89. [tab_install.cpp] Install listbox copy/paste options are hardcoded instead of using JS config-driven behavior
- **JS Source**: `src/js/modules/tab_install.js` lines 165, 184
- **Status**: Pending
- **Details**: JS listbox wiring uses `$core.view.config.copyMode`, `pasteSelection`, and `removePathSpacesCopy`; C++ passes `CopyMode::Default` with `pasteselection=false` and `copytrimwhitespace=false`, changing list interaction behavior.

- [ ] 90. [tab_install.cpp] First listbox missing copyMode from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode" to the main install Listbox. C++ hardcodes listbox::CopyMode::Default instead of reading from view.config.

- [ ] 91. [tab_install.cpp] First listbox missing pasteSelection from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :pasteselection="$core.view.config.pasteSelection". C++ hardcodes false instead of reading from view.config.

- [ ] 92. [tab_install.cpp] First listbox missing copytrimwhitespace from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :copytrimwhitespace="$core.view.config.removePathSpacesCopy". C++ hardcodes false, disabling the remove-path-spaces-on-copy feature.

- [ ] 93. [tab_install.cpp] Second listbox (strings) missing copyMode from config
- **JS Source**: `src/js/modules/tab_install.js` line 184
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode" to the strings Listbox. C++ hardcodes listbox::CopyMode::Default.

- [ ] 94. [legacy_tab_audio.cpp] Seek-loop scheduling differs from JS `requestAnimationFrame` lifecycle
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 19–42
- **Status**: Pending
- **Details**: JS drives seek updates with `requestAnimationFrame` and explicit cancellation IDs; C++ updates via render-loop polling with `seek_loop_active`, changing timing and loop lifecycle semantics.

- [ ] 95. [legacy_tab_audio.cpp] Context menu adds FileDataID-related items not present in JS legacy audio template
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 205–209
- **Status**: Pending
- **Details**: JS context menu has 3 items: "Copy file path(s)", "Copy export path(s)", "Open export directory". C++ adds conditional "Copy file path(s) (listfile format)" and "Copy file data ID(s)" when `hasFileDataIDs` is true (lines 399–402). Legacy MPQ files don't have FileDataIDs, so these extra menu items are incorrect for the legacy audio tab.

- [ ] 96. [legacy_tab_audio.cpp] Loop/Autoplay checkboxes placed in preview container instead of preview-controls div
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 231–239
- **Status**: Pending
- **Details**: JS places Loop/Autoplay checkboxes and Export button together in the `preview-controls` div. C++ places Loop/Autoplay in the `PreviewContainer` section (lines 479–487) and Export in `PreviewControls` (lines 492–498). This changes the visual layout — checkboxes are above the export button area instead of beside it.

- [ ] 97. [tab_videos.cpp] Kino processing toast omits the explicit Cancel action payload
- **JS Source**: `src/js/modules/tab_videos.js` lines 394–400
- **Status**: Pending
- **Details**: JS updates progress toast with `{ 'Cancel': cancel_processing }`; C++ calls `setToast(..., {}, ...)`, removing the explicit cancel action binding from the toast configuration.

- [ ] 98. [tab_videos.cpp] Cancel button missing from kino_processing toast
- **JS Source**: `src/js/modules/tab_videos.js` line 399
- **Status**: Pending
- **Details**: JS passes `{ 'Cancel': cancel_processing }` as toast buttons. C++ passes `{}` — no Cancel button is shown during batch processing, leaving users with no way to cancel.

- [ ] 99. [tab_videos.cpp] Spurious "Connecting to video server..." toast not in JS
- **JS Source**: `src/js/modules/tab_videos.js` (none)
- **Status**: Pending
- **Details**: JS shows no toast before the initial HTTP request — it only shows "Video is being processed..." on 202 status. C++ always shows a "Connecting to video server..." progress toast before the request, which is not in the original.

- [ ] 100. [tab_videos.cpp] Filter input buffer capped at 255 chars
- **JS Source**: `src/js/modules/tab_videos.js` line 490
- **Status**: Pending
- **Details**: JS `v-model` has no character limit. C++ uses `char filter_buf[256]` which truncates filter input at 255 characters.

- [ ] 101. [tab_videos.cpp] kino_post hardcodes hostname and path instead of using constant
- **JS Source**: `src/js/modules/tab_videos.js` lines 137, 349, 431
- **Status**: Pending
- **Details**: JS uses `constants.KINO.API_URL` dynamically via `fetch()`. C++ hardcodes `httplib::SSLClient cli("www.kruithne.net")` and `.Post("/wow.export/v2/get_video", ...)` instead of parsing the constant. If the constant changes, C++ won't reflect it.

- [ ] 102. [tab_videos.cpp] Subtitle loading uses different API path than JS
- **JS Source**: `src/js/modules/tab_videos.js` lines 226–230
- **Status**: Pending
- **Details**: JS calls `subtitles.get_subtitles_vtt(core_ref.view.casc, subtitle_info.file_data_id, subtitle_info.format)` which fetches+converts internally. C++ manually fetches via `casc->getVirtualFileByID()`, reads as string, then calls `subtitles::get_subtitles_vtt(raw_subtitle_text, fmt)`. Different function signature — caller now responsible for fetching.

- [ ] 103. [tab_videos.cpp] MP4 download may lack User-Agent header
- **JS Source**: `src/js/modules/tab_videos.js` line 628
- **Status**: Pending
- **Details**: JS explicitly sets `'User-Agent': constants.USER_AGENT` for the MP4 download fetch. C++ uses `generics::get(*mp4_url)` which may or may not set User-Agent, depending on that function's implementation.

- [ ] 104. [tab_data.cpp] DB2 load error toast omits JS `View Log` action callback
- **JS Source**: `src/js/modules/tab_data.js` lines 80–82
- **Status**: Pending
- **Details**: JS error toast includes `{'View Log': () => log.openRuntimeLog()}`; C++ error toast uses empty actions, removing the original recovery handler.

- [ ] 105. [tab_data.cpp] Listbox pasteselection and copytrimwhitespace hardcoded false
- **JS Source**: `src/js/modules/tab_data.js` lines 98–99
- **Status**: Pending
- **Details**: JS binds pasteselection to config.pasteSelection and copytrimwhitespace to config.removePathSpacesCopy. C++ hardcodes both to false.

- [ ] 106. [tab_data.cpp] load_table error toast missing View Log action button
- **JS Source**: `src/js/modules/tab_data.js` line 80
- **Status**: Pending
- **Details**: JS includes View Log action. C++ passes empty map.

- [ ] 107. [tab_data.cpp] Context menu labels are static instead of dynamic row count
- **JS Source**: `src/js/modules/tab_data.js` lines 108–110
- **Status**: Pending
- **Details**: JS renders "Copy N rows as CSV" with actual selectedCount and pluralization. C++ uses static labels losing the count.

- [ ] 108. [tab_data.cpp] Missing Regex Enabled indicators in both filter bars
- **JS Source**: `src/js/modules/tab_data.js` lines 102–103, 129–130
- **Status**: Pending
- **Details**: JS shows Regex Enabled div in both the DB2 filter bar and data table tray filter. C++ has no regex indicators.

- [ ] 109. [legacy_tab_data.cpp] Export format menu omits JS SQL/DBC options
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 172–176, 222–231
- **Status**: Pending
- **Details**: JS menu exposes `CSV`, `SQL`, and `DBC` export actions, but C++ `legacy_data_opts` only includes `Export as CSV`, making SQL/DBC exports unavailable through the settings menu path.

- [ ] 110. [legacy_tab_data.cpp] Context menu uses `ImGui::BeginPopupContextItem` vs JS ContextMenu component
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 139–143
- **Status**: Pending
- **Details**: JS uses the custom `ContextMenu` component with slot-based content rendering and a close event. C++ uses native `ImGui::BeginPopupContextItem` (line 368) which has different popup behavior, positioning, and styling compared to the custom ContextMenu component used elsewhere in the app.

- [ ] 111. [legacy_tab_files.cpp] Listbox context menu includes extra FileDataID actions absent in JS
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 76–80
- **Status**: Pending
- **Details**: JS legacy-files menu only provides copy file path, copy export path, and open export directory; C++ conditionally adds listfile-format and fileDataID entries, changing context-menu behavior.

- [ ] 112. [legacy_tab_files.cpp] Layout doesn't use `app::layout` helpers — uses raw `ImGui::BeginChild`
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 72–89
- **Status**: Pending
- **Details**: Other legacy tabs (audio, fonts, textures, data) use `app::layout::BeginTab/EndTab`, `CalcListTabRegions`, `BeginListContainer`, etc. for consistent layout. `legacy_tab_files.cpp` uses raw `ImGui::BeginChild("legacy-files-list-container", ...)` (line 124) without the layout system. This will produce inconsistent sizing and positioning compared to sibling legacy tabs.

- [ ] 113. [legacy_tab_files.cpp] Tray layout structure differs from JS
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 82–88
- **Status**: Pending
- **Details**: JS wraps the filter and export button in a `#tab-legacy-files-tray` div with its own layout (likely flex row). C++ renders filter input, then `ImGui::SameLine()`, then the export button (lines 206–216). The proportions and alignment of filter vs button may not match the JS CSS-defined tray layout.

- [ ] 114. [tab_zones.cpp] Zone listbox copy/paste trim options are hardcoded instead of config-bound
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: JS binds `copymode`, `pasteselection`, and `copytrimwhitespace` to config values; C++ hardcodes `CopyMode::Default`, `pasteselection=false`, and `copytrimwhitespace=false`.

- [ ] 115. [tab_zones.cpp] Phase selector placement differs from JS preview overlay layout
- **JS Source**: `src/js/modules/tab_zones.js` lines 341–349
- **Status**: Pending
- **Details**: JS renders the phase dropdown in a `preview-dropdown-overlay` inside the preview background; C++ renders phase selection in the bottom control bar.

- [ ] 116. [tab_zones.cpp] Listbox copyMode hardcoded instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ passes `listbox::CopyMode::Default` instead of reading from `view.config["copyMode"]`.

- [ ] 117. [tab_zones.cpp] Listbox pasteSelection hardcoded false instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ hardcodes `false` instead of reading `view.config["pasteSelection"]`.

- [ ] 118. [tab_zones.cpp] Listbox copytrimwhitespace hardcoded false instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ hardcodes `false` instead of reading `view.config["removePathSpacesCopy"]`.

- [ ] 119. [tab_zones.cpp] Phase dropdown placed in control bar instead of preview overlay
- **JS Source**: `src/js/modules/tab_zones.js` lines 341–347
- **Status**: Pending
- **Details**: JS puts the phase `<select>` inside `preview-dropdown-overlay` div overlaid on the zone canvas. C++ places the `ImGui::BeginCombo` in the bottom controls bar alongside checkboxes/button. This is a layout difference.

- [ ] 120. [tab_zones.cpp] EXPANSION_NAMES static vector is dead code
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: `EXPANSION_NAMES` vector is defined but never referenced. The actual expansion rendering uses `constants::EXPANSIONS`. Should be removed.

- [ ] 121. [tab_zones.cpp] ZoneDisplayInfo vs ZoneEntry naming mismatch with header
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: The header declares `ZoneEntry` struct but the cpp defines a separate `ZoneDisplayInfo` struct for `parse_zone_entry`. The header's `ZoneEntry` appears unused.

- [ ] 122. [tab_items.cpp] Wowhead item handler is stubbed out
- **JS Source**: `src/js/modules/tab_items.js` lines 322–324
- **Status**: Pending
- **Details**: JS calls `ExternalLinks.wowHead_viewItem(item_id)` from the context action; C++ `view_on_wowhead(...)` immediately returns and does nothing.

- [ ] 123. [tab_items.cpp] Item sidebar checklist interaction/layout diverges from JS clickable row design
- **JS Source**: `src/js/modules/tab_items.js` lines 254–266
- **Status**: Pending
- **Details**: JS uses `.sidebar-checklist-item` rows with selected-state styling and row-level click toggling; C++ renders plain ImGui checkboxes, changing sidebar visuals and interaction feel.

- [ ] 124. [tab_items.cpp] view_on_wowhead is stubbed — does nothing
- **JS Source**: `src/js/modules/tab_items.js` lines 322–324
- **Status**: Pending
- **Details**: JS calls ExternalLinks.wowHead_viewItem(item_id) which opens a Wowhead URL. C++ function is { return; } — a no-op. external_links.h already provides wowHead_viewItem().

- [ ] 125. [tab_items.cpp] copy_to_clipboard bypasses core.view.copyToClipboard
- **JS Source**: `src/js/modules/tab_items.js` lines 318–320
- **Status**: Pending
- **Details**: JS calls this.$core.view.copyToClipboard(value) which may have additional behavior (e.g. toast notification). C++ calls ImGui::SetClipboardText() directly, skipping view layer.

- [ ] 126. [tab_items.cpp] Filter input buffer limited to 256 bytes
- **JS Source**: `src/js/modules/tab_items.js` line 249
- **Status**: Pending
- **Details**: JS input has no character limit. C++ uses char filter_buf[256] with std::strncpy, silently truncating beyond 255 characters.

- [ ] 127. [tab_item_sets.cpp] apply_filter converts ItemSet structs to JSON objects unnecessarily
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 67–69
- **Status**: Pending
- **Details**: JS simply assigns the array of ItemSet objects directly. C++ iterates every ItemSet, constructs nlohmann::json objects, and pushes them. render() then converts JSON back into ItemEntry structs every frame — double-conversion overhead.

- [ ] 128. [tab_item_sets.cpp] render() re-creates item_entries vector from JSON every frame
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 76–86
- **Status**: Pending
- **Details**: C++ render allocates a vector, loops over all JSON items, copies fields into ItemEntry structs, and pushes — every frame. JS template binds directly to existing objects with no per-frame allocation.

- [ ] 129. [tab_item_sets.cpp] Regex-enabled text and filter input lack proper layout container
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 81–84
- **Status**: Pending
- **Details**: JS wraps regex info and filter input inside div class="filter" providing inline layout. C++ renders them sequentially without SameLine() or horizontal group, causing "Regex Enabled" to appear above the filter input instead of beside it.

- [ ] 130. [tab_creatures.cpp] Creature list context actions are not equivalent to JS copy-name/copy-ID menu
- **JS Source**: `src/js/modules/tab_creatures.js` lines 983–986, 1179–1203
- **Status**: Pending
- **Details**: JS creature list context menu exposes only `Copy name(s)` and `Copy ID(s)` handlers; C++ delegates to generic `listbox_context::handle_context_menu(...)`, changing the context action contract from the original creature-specific menu.

- [ ] 131. [tab_creatures.cpp] Error toast for model load missing View Log action button
- **JS Source**: `src/js/modules/tab_creatures.js` line 728
- **Status**: Pending
- **Details**: JS passes View Log action. C++ passes empty map. User cannot open runtime log from this error.

- [ ] 132. [tab_creatures.cpp] path.basename behavior not replicated in skin name
- **JS Source**: `src/js/modules/tab_creatures.js` line 668
- **Status**: Pending
- **Details**: Node.js path.basename produces trailing dot. C++ strips full .m2 extension producing no dot. Skin name stripping matches different substrings producing different display labels.

- [ ] 133. [tab_creatures.cpp] Missing Regex Enabled indicator in filter bar
- **JS Source**: `src/js/modules/tab_creatures.js` line 989
- **Status**: Pending
- **Details**: JS shows Regex Enabled div with tooltip when regex filters are active. C++ filter bar has no indicator.

- [ ] 134. [tab_creatures.cpp] Listbox context menu Copy names and Copy IDs not rendered in UI
- **JS Source**: `src/js/modules/tab_creatures.js` lines 983–986
- **Status**: Pending
- **Details**: JS renders ContextMenu with Copy names and Copy IDs options. C++ does not render an ImGui context menu popup. The functions exist but are never invoked from the UI.

- [ ] 135. [tab_creatures.cpp] Sorting uses byte comparison instead of locale-aware localeCompare
- **JS Source**: `src/js/modules/tab_creatures.js` line 1161
- **Status**: Pending
- **Details**: JS uses localeCompare. C++ uses name_a < name_b. Creatures with diacritics may sort differently.

- [ ] 136. [tab_decor.cpp] Decor list context menu open/interaction path differs from JS ContextMenu component flow
- **JS Source**: `src/js/modules/tab_decor.js` lines 234–237
- **Status**: Pending
- **Details**: JS renders a dedicated ContextMenu node for listbox selections (`Copy name(s)` / `Copy file data ID(s)`); C++ uses a manual popup path without equivalent Vue component lifecycle/wiring, deviating from original interaction flow.

- [ ] 137. [tab_decor.cpp] Error toast for preview_decor missing View Log action button
- **JS Source**: `src/js/modules/tab_decor.js` line 119
- **Status**: Pending
- **Details**: JS includes View Log action. C++ passes empty map.

- [ ] 138. [tab_decor.cpp] Sorting uses byte comparison instead of locale-aware localeCompare
- **JS Source**: `src/js/modules/tab_decor.js` lines 401–405
- **Status**: Pending
- **Details**: JS uses localeCompare. C++ uses name_a < name_b after tolower. Different sort for non-ASCII names.

- [ ] 139. [tab_decor.cpp] Missing scrub pause/resume behavior on animation slider
- **JS Source**: `src/js/modules/tab_decor.js` lines 546–561
- **Status**: Pending
- **Details**: JS start_scrub saves paused state and pauses animation while dragging. C++ SliderInt has no mouse-down/up event handling.

- [ ] 140. [tab_decor.cpp] Missing Regex Enabled indicator in filter bar
- **JS Source**: `src/js/modules/tab_decor.js` line 240
- **Status**: Pending
- **Details**: JS shows Regex Enabled div above filter input. C++ filter bar has no such indicator.

- [ ] 141. [tab_decor.cpp] Category group header click-to-toggle-all not implemented
- **JS Source**: `src/js/modules/tab_decor.js` line 301
- **Status**: Pending
- **Details**: JS clicking category name toggles all subcategories on/off. C++ uses TreeNodeEx which only expands/collapses. The toggle function exists but is never called from render.

- [ ] 142. [tab_decor.cpp] WMO Groups and Doodad Sets use manual checkbox loop instead of CheckboxList
- **JS Source**: `src/js/modules/tab_decor.js` lines 363–369
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for both. C++ uses manual ImGui::Checkbox loops instead of checkboxlist::render(). Inconsistent and may cause visual differences.


## Low — aesthetic differences, missing tooltips, and error stack traces

- [ ] 143. [tab_install.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS renders `Regex Enabled` with `:title="$core.view.regexTooltip"`; C++ renders plain text without the tooltip contract, changing UI affordance.

- [ ] 144. [tab_install.cpp] export_install_files missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_install.js` line 78
- **Status**: Pending
- **Details**: JS calls helper.mark(file_name, false, e.message, e.stack) passing both message and stack trace. C++ passes only e.what(), omitting the stack trace parameter.

- [ ] 145. [tab_install.cpp] Strings sidebar missing CSS styling equivalents
- **JS Source**: `src/js/modules/tab_install.js` lines 194–197
- **Status**: Pending
- **Details**: CSS defines .strings-header font-size 14px opacity 0.7, .strings-filename font-size 12px word-break: break-all, 5px gap. C++ uses default font sizes and ImGui::Spacing() which may not match.

- [ ] 146. [tab_install.cpp] Input placeholder text not rendered
- **JS Source**: `src/js/modules/tab_install.js` lines 170, 189
- **Status**: Pending
- **Details**: JS filter inputs have placeholder="Filter install files..." and "Filter strings...". C++ uses plain ImGui::InputText without hint/placeholder.

- [ ] 147. [tab_install.cpp] Regex tooltip not rendered
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS "Regex Enabled" div has :title="$core.view.regexTooltip" showing tooltip on hover. C++ has no tooltip implementation.

- [ ] 148. [tab_textures.cpp] export_texture_atlas_regions missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_textures.js` line 261
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 149. [tab_audio.cpp] export_sounds missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_audio.js` line 175
- **Status**: Pending
- **Details**: JS passes e.message and e.stack to helper.mark(). C++ only passes e.what().

- [ ] 150. [legacy_tab_audio.cpp] Playback UI visuals diverge from JS template/CSS
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 201–241
- **Status**: Pending
- **Details**: JS renders `#sound-player-anim`, CSS-styled play button state classes, and component sliders, while C++ replaces this with ImGui text/buttons/checkboxes and a custom icon pulse, so layout/styling is not pixel-identical.

- [ ] 151. [legacy_tab_audio.cpp] Sound player info combines seek/title/duration into single Text call vs JS 3 separate spans
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 218–222
- **Status**: Pending
- **Details**: JS renders sound player info as 3 separate `<span>` elements in a flex container: seek formatted time, title (with CSS class "title"), and duration formatted time. C++ combines them into a single `ImGui::Text("%s  %s  %s", ...)` call (line 453). The title won't have distinct styling, and the layout/alignment will differ from the JS flex row.

- [ ] 152. [legacy_tab_audio.cpp] Play/Pause uses text toggle ("Play"/"Pause") vs JS CSS class-based visual toggle
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 225
- **Status**: Pending
- **Details**: JS uses `<input type="button" :class="{ isPlaying: !soundPlayerState }">` — the button appearance changes via CSS class (likely showing a play/pause icon). C++ uses `ImGui::Button(view.soundPlayerState ? "Pause" : "Play")` with text labels. The visual appearance differs significantly from the original icon-based toggle.

- [ ] 153. [legacy_tab_audio.cpp] Volume slider is ImGui::SliderFloat with format string vs JS custom Slider component
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 226
- **Status**: Pending
- **Details**: JS uses a custom `<Slider>` component with id "slider-volume" for volume control. C++ uses `ImGui::SliderFloat` with format "Vol: %.0f%%" (line 474). The custom JS Slider has its own visual styling defined in CSS; the ImGui slider will look different (default ImGui styling vs themed slider).

- [ ] 154. [legacy_tab_audio.cpp] `export_sounds` `helper.mark` doesn't pass error stack trace
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 189
- **Status**: Pending
- **Details**: JS calls `helper.mark(export_file_name, false, e.message, e.stack)` with 4 arguments including the stack trace. C++ calls `helper.mark(export_file_name, false, e.what())` with only 3 arguments (line 239). Error stack information is lost in C++ export failure reports.

- [ ] 155. [tab_videos.cpp] Dev-mode kino processing trigger export is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 467–469
- **Status**: Pending
- **Details**: JS exposes `window.trigger_kino_processing = trigger_kino_processing` in non-release mode; C++ has no equivalent debug export hook.

- [ ] 156. [tab_videos.cpp] All helper.mark error calls missing stack trace argument
- **JS Source**: `src/js/modules/tab_videos.js` lines 642, 690, 702, 763, 822
- **Status**: Pending
- **Details**: JS passes `(file, false, e.message, e.stack)` (4 args) for error marking. C++ passes only `(file, false, e.what())` (3 args). Stack traces are lost in every export error path (MP4, AVI×2, MP3, Subtitles).

- [ ] 157. [tab_videos.cpp] stream_video outer catch missing stack trace log
- **JS Source**: `src/js/modules/tab_videos.js` line 214
- **Status**: Pending
- **Details**: JS logs `log.write(e.stack)` separately from the error message. C++ only logs `e.what()`.

- [ ] 158. [tab_videos.cpp] Regex tooltip missing on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_videos.js` line 489
- **Status**: Pending
- **Details**: JS shows `:title="$core.view.regexTooltip"` tooltip on the "Regex Enabled" text. C++ renders `ImGui::TextUnformatted("Regex Enabled")` with no tooltip.

- [ ] 159. [tab_videos.cpp] Dev-mode trigger_kino_processing not exposed in C++
- **JS Source**: `src/js/modules/tab_videos.js` lines 468–469
- **Status**: Pending
- **Details**: JS exposes `window.trigger_kino_processing = trigger_kino_processing` when `!BUILD_RELEASE`. C++ has only a comment. No equivalent debug hook exists.

- [ ] 160. [tab_fonts.cpp] export_fonts missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_fonts.js` line 141
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

- [ ] 161. [tab_data.cpp] helper.mark() calls missing stack trace argument
- **JS Source**: `src/js/modules/tab_data.js` lines 250, 314, 358
- **Status**: Pending
- **Details**: JS passes both e.message and e.stack. C++ only passes e.what().

- [ ] 162. [legacy_tab_data.cpp] Missing regex info display in DBC filter bar
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 134–135
- **Status**: Pending
- **Details**: JS has `<div class="regex-info" v-if="$core.view.config.regexFilters">Regex Enabled</div>` in the DBC listbox filter section. C++ DBC filter bar (lines 309–316) does not show the regex enabled indicator.

- [ ] 163. [tab_raw.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_raw.js` line 158
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 164. [tab_raw.cpp] export_raw_files error mark missing stack trace argument
- **JS Source**: `src/js/modules/tab_raw.js` line 128
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 165. [tab_raw.cpp] Missing placeholder text on filter input
- **JS Source**: `src/js/modules/tab_raw.js` line 159
- **Status**: Pending
- **Details**: JS has placeholder="Filter raw files...". C++ uses ImGui::InputText with no hint text.

- [ ] 166. [tab_raw.cpp] Missing tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_raw.js` line 158
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ has no tooltip.

- [ ] 167. [tab_maps.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_maps.js` line 302
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 168. [tab_maps.cpp] Missing e.stack in all helper.mark error calls
- **JS Source**: `src/js/modules/tab_maps.js` lines 680, 759, 815, 848, 931, 1122
- **Status**: Pending
- **Details**: JS passes both e.message and e.stack to helper.mark. C++ only passes e.what(), omitting stack trace. Affects 6 export functions.

- [ ] 169. [tab_maps.cpp] Missing regex tooltip
- **JS Source**: `src/js/modules/tab_maps.js` line 302
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" on regex-info div. C++ renders ImGui::TextDisabled("Regex Enabled") without any tooltip.

- [ ] 170. [tab_maps.cpp] Sidebar headers use SeparatorText instead of styled span
- **JS Source**: `src/js/modules/tab_maps.js` lines 313, 342, 352, 354
- **Status**: Pending
- **Details**: JS uses <span class="header"> which renders as bold text. C++ uses ImGui::SeparatorText() which draws a horizontal separator line with text — visually different.

- [ ] 171. [tab_zones.cpp] export_zone_map helper.mark missing stack trace
- **JS Source**: `src/js/modules/tab_zones.js` line 491
- **Status**: Pending
- **Details**: JS: `helper.mark(zone_entry, false, e.message, e.stack)` — passes both message and stack. C++: `helper.mark(..., false, e.what())` — only passes message, no stack trace.

- [ ] 172. [tab_zones.cpp] Missing regex tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_zones.js` line 325
- **Status**: Pending
- **Details**: JS shows a tooltip on "Regex Enabled" via `:title="$core.view.regexTooltip"`. C++ renders `ImGui::TextUnformatted("Regex Enabled")` with no tooltip.

- [ ] 173. [tab_zones.cpp] Missing per-tile position logging in render_map_tiles
- **JS Source**: `src/js/modules/tab_zones.js` lines 184–185
- **Status**: Pending
- **Details**: JS logs `'rendering tile FileDataID %d at position (%d,%d) -> (%d,%d) [Layer %d]'` for each tile. C++ has no per-tile log.

- [ ] 174. [tab_zones.cpp] Missing "no tiles found" log for art style
- **JS Source**: `src/js/modules/tab_zones.js` lines 121–123
- **Status**: Pending
- **Details**: JS logs `'no tiles found for UiMapArt ID %d'` and `continue`s. C++ has no equivalent check/log.

- [ ] 175. [tab_zones.cpp] Missing "no overlays found" log
- **JS Source**: `src/js/modules/tab_zones.js` lines 212–214
- **Status**: Pending
- **Details**: JS logs `'no WorldMapOverlay entries found for UiMapArt ID %d'` when overlays array is empty. C++ has no such log.

- [ ] 176. [tab_zones.cpp] Missing "no overlay tiles" log per overlay
- **JS Source**: `src/js/modules/tab_zones.js` lines 219–222
- **Status**: Pending
- **Details**: JS logs `'no tiles found for WorldMapOverlay ID %d'` and `continue`s for empty tile sets. C++ calls `render_overlay_tiles` regardless.

- [ ] 177. [tab_items.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_items.js` line 248
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 178. [tab_items.cpp] Regex tooltip not rendered
- **JS Source**: `src/js/modules/tab_items.js` line 248
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" on "Regex Enabled" div. C++ renders ImGui::TextUnformatted("Regex Enabled") without any tooltip.

- [ ] 179. [tab_items.cpp] Sidebar headers use SeparatorText instead of styled header span
- **JS Source**: `src/js/modules/tab_items.js` lines 252, 262
- **Status**: Pending
- **Details**: JS uses <span class="header">Item Types</span> which renders as styled header text. C++ uses ImGui::SeparatorText() which draws a horizontal separator line with text — visually different.

- [ ] 180. [tab_items.cpp] Sidebar checklist items lack .selected class visual feedback
- **JS Source**: `src/js/modules/tab_items.js` lines 254–257
- **Status**: Pending
- **Details**: JS adds :class="{ selected: item.checked }" to checklist items. CSS gives .sidebar-checklist-item.selected a background of rgba(255,255,255,0.05). C++ uses plain ImGui::Checkbox with no highlight.

- [ ] 181. [tab_items.cpp] Quality color applied only to CheckMark, not to label text
- **JS Source**: `src/js/modules/tab_items.js` lines 264–265
- **Status**: Pending
- **Details**: CSS accent-color applies quality color to the checkbox. C++ pushes ImGuiCol_CheckMark only coloring the checkmark glyph. The checkbox background/frame and label text are unaffected.

- [ ] 182. [tab_item_sets.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_item_sets.js` line 82
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ shows plain `Regex Enabled` text without tooltip behavior.

- [ ] 183. [tab_item_sets.cpp] Missing filter input placeholder text
- **JS Source**: `src/js/modules/tab_item_sets.js` line 83
- **Status**: Pending
- **Details**: JS uses placeholder="Filter item sets..." on the text input. C++ uses ImGui::InputText without hint/placeholder text.

- [ ] 184. [tab_item_sets.cpp] Missing regex tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_item_sets.js` line 82
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ just renders ImGui::TextUnformatted("Regex Enabled") with no tooltip.

- [ ] 185. [tab_decor.cpp] helper.mark on failure missing stack trace parameter
- **JS Source**: `src/js/modules/tab_decor.js` line 184
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

- [ ] 186. [tab_decor.cpp] Missing tooltips on all sidebar Preview and Export checkboxes
- **JS Source**: `src/js/modules/tab_decor.js` lines 314–354
- **Status**: Pending
- **Details**: JS has title attributes for every checkbox. C++ has no tooltip calls for any of them.
