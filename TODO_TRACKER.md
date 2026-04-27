# TODO Tracker

> **Progress: 0/85 verified (0%)** — ✅ = Verified, ⬜ = Pending

## Upstream Sync — port from wow.export JS @ d0d847f5

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


## High — significant functional bugs and major behavioral differences

- [ ] 74. [tab_items.cpp] itemViewerShowAll second loop superseded by upstream DBItemList refactor
- **JS Source**: `src/js/modules/tab_items.js`
- **Status**: Blocked
- **Details**: Blocked on entry 11 — DBItemList is not yet implemented in C++ (only exists as `src/js/db/caches/DBItemList.js`). The original bug (C++ using DBItems::getItemById instead of Item constructor reading item_row.Display_lang) is now moot — upstream commit d0d847f5 removed the entire second loop and replaced item loading with DBItemList.initialize() / DBItemList.loadShowAllItems(). Once DBItemList.cpp/.h are implemented (entry 12) and the item equipping rework is ported (entry 11), rework itemViewerShowAll to call DBItemList::initialize() and DBItemList::loadShowAllItems() to match the current JS implementation.


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

- [ ] 127. [tab_item_sets.cpp] apply_filter converts ItemSet structs to JSON objects unnecessarily
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 67–69
- **Status**: Pending
- **Details**: JS simply assigns the array of ItemSet objects directly. C++ iterates every ItemSet, constructs nlohmann::json objects, and pushes them. Eliminating JSON requires changing view.listfileItemSets type in core.h — architectural change deferred.

- [ ] 128. [tab_item_sets.cpp] render() re-creates item_entries vector from JSON every frame
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 76–86
- **Status**: Pending
- **Details**: Cache already exists (size-guarded). Proper fix requires bypassing JSON entirely (see TODO 127).

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

- [ ] 144. [tab_install.cpp] export_install_files missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_install.js` line 78
- **Status**: Pending
- **Details**: JS calls helper.mark(file_name, false, e.message, e.stack) passing both message and stack trace. C++ passes only e.what(), omitting the stack trace parameter.

- [ ] 145. [tab_install.cpp] Strings sidebar missing CSS styling equivalents
- **JS Source**: `src/js/modules/tab_install.js` lines 194–197
- **Status**: Pending
- **Details**: CSS defines .strings-header font-size 14px opacity 0.7, .strings-filename font-size 12px word-break: break-all, 5px gap. C++ uses default font sizes and ImGui::Spacing() which may not match.

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

- [ ] 164. [tab_raw.cpp] export_raw_files error mark missing stack trace argument
- **JS Source**: `src/js/modules/tab_raw.js` line 128
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

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

- [ ] 185. [tab_decor.cpp] helper.mark on failure missing stack trace parameter
- **JS Source**: `src/js/modules/tab_decor.js` line 184
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

- [ ] 186. [tab_decor.cpp] Missing tooltips on all sidebar Preview and Export checkboxes
- **JS Source**: `src/js/modules/tab_decor.js` lines 314–354
- **Status**: Pending
- **Details**: JS has title attributes for every checkbox. C++ has no tooltip calls for any of them.
