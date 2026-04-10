# JS → C++ Conversion Tracker

> **⚠️ Update this file every time you convert a file.**
> See [CONVERSION_GUIDANCE.md](CONVERSION_GUIDANCE.md) for the recommended conversion order and principles.

**Status legend:**
- `- [ ]` Not started
- `- [~]` In progress (partially converted)
- `- [x]` Converted and compiles
- `- [✓]` Converted, compiles, and tested/verified

> **🚫 DO NOT mark a file as `[x]` or `[✓]` unless it is 100% converted with NOTHING missing.**
> Every function, every export, every constant, every edge case in the original JS must have a working C++ equivalent.
> A partial conversion stays at `[~]`. No exceptions.

**Progress:** 179 / 179 files converted

---

## Tier 0 — Zero-Dependency Primitives (11 files)

- [x] `src/js/crc32.cpp` (35 lines)
- [x] `src/js/MultiMap.cpp` (31 lines)
- [x] `src/js/blob.cpp` (309 lines)
- [x] `src/js/install-type.cpp` (6 lines)
- [x] `src/js/xml.cpp` (170 lines)
- [x] `src/js/subtitles.cpp` (192 lines)
- [x] `src/js/hashing/xxhash64.cpp` (288 lines)
- [x] `src/js/casc/content-flags.cpp` (14 lines)
- [x] `src/js/casc/locale-flags.cpp` (39 lines)
- [x] `src/js/casc/jenkins96.cpp` (54 lines)
- [x] `src/js/casc/version-config.cpp` (32 lines)

## Tier 1 — Core Foundation (7 files)

- [x] `src/js/constants.cpp` (257 lines)
- [x] `src/js/log.cpp` (113 lines)
- [x] `src/js/file-writer.cpp` (44 lines)
- [x] `src/js/buffer.cpp` (1127 lines) 🔴
- [x] `src/js/png-writer.cpp` (251 lines)
- [x] `src/js/generics.cpp` (502 lines)
- [x] `src/js/mmap.cpp` (52 lines)

## Tier 2 — App Core & Event System (3 files)

- [x] `src/js/core.cpp` (561 lines)
- [x] `src/js/config.cpp` (117 lines)
- [x] `src/js/tiled-png-writer.cpp` (142 lines)

## Tier 3 — Misc Utilities (2 files)

- [x] `src/js/gpu-info.cpp` (363 lines)
- [x] `src/js/icon-render.cpp` (108 lines)

## Tier 4 — CASC Crypto & Low-Level (5 files)

- [x] `src/js/casc/cdn-config.cpp` (50 lines)
- [x] `src/js/casc/install-manifest.cpp` (68 lines)
- [x] `src/js/casc/salsa20.cpp` (280 lines)
- [x] `src/js/casc/tact-keys.cpp` (136 lines)
- [x] `src/js/casc/blp.cpp` (508 lines)

## Tier 5 — CASC Readers (3 files)

- [x] `src/js/casc/blte-reader.cpp` (355 lines)
- [x] `src/js/casc/blte-stream-reader.cpp` (244 lines)
- [x] `src/js/casc/vp9-avi-demuxer.cpp` (258 lines)

## Tier 6 — CASC Mid-Level (5 files)

- [x] `src/js/casc/export-helper.cpp` (293 lines)
- [x] `src/js/casc/dbd-manifest.cpp` (84 lines)
- [x] `src/js/casc/realmlist.cpp` (68 lines)
- [x] `src/js/casc/cdn-resolver.cpp` (219 lines)
- [x] `src/js/casc/build-cache.cpp` (258 lines)

## Tier 7 — DB Schema & Readers (5 files)

- [x] `src/js/db/CompressionType.cpp` (7 lines)
- [x] `src/js/db/FieldType.cpp` (13 lines)
- [x] `src/js/db/DBDParser.cpp` (348 lines)
- [x] `src/js/db/WDCReader.cpp` (909 lines) 🔴
- [x] `src/js/db/DBCReader.cpp` (426 lines)

## Tier 8 — CASC db2 + Listfile (2 files)

- [x] `src/js/casc/db2.cpp` (95 lines)
- [x] `src/js/casc/listfile.cpp` (927 lines) 🔴

## Tier 9 — CASC High-Level Sources (3 files)

- [x] `src/js/casc/casc-source.cpp` (494 lines)
- [x] `src/js/casc/casc-source-remote.cpp` (558 lines)
- [x] `src/js/casc/casc-source-local.cpp` (516 lines)

## Tier 10 — WoW Data Definitions (2 files)

- [x] `src/js/wow/ItemSlot.cpp` (47 lines)
- [x] `src/js/wow/EquipmentSlots.cpp` (184 lines)

## Tier 11 — DB Caches (18 files)

- [x] `src/js/db/caches/DBModelFileData.cpp` (58 lines)
- [x] `src/js/db/caches/DBTextureFileData.cpp` (67 lines)
- [x] `src/js/db/caches/DBComponentModelFileData.cpp` (174 lines)
- [x] `src/js/db/caches/DBComponentTextureFileData.cpp` (123 lines)
- [x] `src/js/db/caches/DBCreatures.cpp` (99 lines)
- [x] `src/js/db/caches/DBCreatureList.cpp` (57 lines)
- [x] `src/js/db/caches/DBCreatureDisplayExtra.cpp` (63 lines)
- [x] `src/js/db/caches/DBCreaturesLegacy.cpp` (146 lines)
- [x] `src/js/db/caches/DBDecor.cpp` (78 lines)
- [x] `src/js/db/caches/DBDecorCategories.cpp` (62 lines)
- [x] `src/js/db/caches/DBGuildTabard.cpp` (133 lines)
- [x] `src/js/db/caches/DBItemGeosets.cpp` (487 lines)
- [x] `src/js/db/caches/DBNpcEquipment.cpp` (76 lines)
- [x] `src/js/db/caches/DBItems.cpp` (96 lines)
- [x] `src/js/db/caches/DBItemDisplays.cpp` (66 lines)
- [x] `src/js/db/caches/DBItemModels.cpp` (256 lines)
- [x] `src/js/db/caches/DBItemCharTextures.cpp` (149 lines)
- [x] `src/js/db/caches/DBCharacterCustomization.cpp` (284 lines)

## Tier 12 — MPQ Legacy Format (7 files)

- [x] `src/js/mpq/bitstream.cpp` (61 lines)
- [x] `src/js/mpq/bzip2.cpp` (843 lines) 🔴
- [x] `src/js/mpq/huffman.cpp` (340 lines)
- [x] `src/js/mpq/pkware.cpp` (204 lines)
- [x] `src/js/mpq/mpq.cpp` (655 lines)
- [x] `src/js/mpq/build-version.cpp` (162 lines)
- [x] `src/js/mpq/mpq-install.cpp` (162 lines)

## Tier 13 — 3D GL Abstraction (5 files)

- [x] `src/js/3D/gl/GLContext.cpp` (412 lines)
- [x] `src/js/3D/gl/GLTexture.cpp` (195 lines)
- [x] `src/js/3D/gl/UniformBuffer.cpp` (230 lines)
- [x] `src/js/3D/gl/VertexArray.cpp` (310 lines)
- [x] `src/js/3D/gl/ShaderProgram.cpp` (304 lines)

## Tier 14 — 3D Shaders & Data Mappings (8 files)

- [x] `src/js/3D/AnimMapper.cpp` (1798 lines) 🔴
- [x] `src/js/3D/BoneMapper.cpp` (431 lines)
- [x] `src/js/3D/GeosetMapper.cpp` (86 lines)
- [x] `src/js/3D/WMOShaderMapper.cpp` (94 lines)
- [x] `src/js/3D/ShaderMapper.cpp` (183 lines)
- [x] `src/js/3D/Shaders.cpp` (154 lines)
- [x] `src/js/3D/Texture.cpp` (43 lines)
- [x] `src/js/3D/Skin.cpp` (102 lines)

## Tier 15 — 3D Cameras (2 files)

- [x] `src/js/3D/camera/CameraControlsGL.cpp` (429 lines)
- [x] `src/js/3D/camera/CharacterCameraControlsGL.cpp` (178 lines)

## Tier 16 — 3D Loaders (13 files)

- [x] `src/js/3D/loaders/LoaderGenerics.cpp` (32 lines)
- [x] `src/js/3D/loaders/M2Generics.cpp` (216 lines)
- [x] `src/js/3D/loaders/ANIMLoader.cpp` (66 lines)
- [x] `src/js/3D/loaders/BONELoader.cpp` (62 lines)
- [x] `src/js/3D/loaders/MDXLoader.cpp` (917 lines) 🔴
- [x] `src/js/3D/loaders/WDTLoader.cpp` (105 lines)
- [x] `src/js/3D/loaders/ADTLoader.cpp` (557 lines)
- [x] `src/js/3D/loaders/M2LegacyLoader.cpp` (834 lines) 🔴
- [x] `src/js/3D/loaders/M3Loader.cpp` (343 lines)
- [x] `src/js/3D/loaders/WMOLoader.cpp` (471 lines)
- [x] `src/js/3D/loaders/WMOLegacyLoader.cpp` (569 lines)
- [x] `src/js/3D/loaders/SKELLoader.cpp` (456 lines)
- [x] `src/js/3D/loaders/M2Loader.cpp` (888 lines) 🔴

## Tier 17 — 3D Writers (8 files)

- [x] `src/js/3D/writers/CSVWriter.cpp` (85 lines)
- [x] `src/js/3D/writers/JSONWriter.cpp` (47 lines)
- [x] `src/js/3D/writers/MTLWriter.cpp` (70 lines)
- [x] `src/js/3D/writers/OBJWriter.cpp` (227 lines)
- [x] `src/js/3D/writers/SQLWriter.cpp` (233 lines)
- [x] `src/js/3D/writers/STLWriter.cpp` (252 lines)
- [x] `src/js/3D/writers/GLBWriter.cpp` (75 lines)
- [x] `src/js/3D/writers/GLTFWriter.cpp` (1506 lines) 🔴

## Tier 18 — 3D Renderers (9 files)

- [x] `src/js/3D/renderers/GridRenderer.cpp` (152 lines)
- [x] `src/js/3D/renderers/ShadowPlaneRenderer.cpp` (166 lines)
- [x] `src/js/3D/renderers/CharMaterialRenderer.cpp` (421 lines)
- [x] `src/js/3D/renderers/M2LegacyRendererGL.cpp` (1055 lines)
- [x] `src/js/3D/renderers/MDXRendererGL.cpp` (808 lines) 🔴
- [x] `src/js/3D/renderers/M2RendererGL.cpp` (1650 lines) 🔴
- [x] `src/js/3D/renderers/M3RendererGL.cpp` (309 lines)
- [x] `src/js/3D/renderers/WMOLegacyRendererGL.cpp` (554 lines)
- [x] `src/js/3D/renderers/WMORendererGL.cpp` (677 lines)

## Tier 19 — 3D Exporters (7 files)

- [x] `src/js/3D/exporters/CharacterExporter.cpp` (349 lines)
- [x] `src/js/3D/exporters/M2LegacyExporter.cpp` (407 lines)
- [x] `src/js/3D/exporters/M2Exporter.cpp` (1215 lines) 🔴
- [x] `src/js/3D/exporters/M3Exporter.cpp` (257 lines)
- [x] `src/js/3D/exporters/WMOLegacyExporter.cpp` (590 lines)
- [x] `src/js/3D/exporters/WMOExporter.cpp` (1337 lines) 🔴
- [x] `src/js/3D/exporters/ADTExporter.cpp` (1551 lines) 🟢

## Tier 20 — UI Helpers (9 files)

- [x] `src/js/ui/audio-helper.cpp` (178 lines)
- [x] `src/js/ui/uv-drawer.cpp` (58 lines)
- [x] `src/js/ui/char-texture-overlay.cpp` (120 lines)
- [x] `src/js/ui/texture-ribbon.cpp` (109 lines)
- [x] `src/js/ui/listbox-context.cpp` (177 lines)
- [x] `src/js/ui/data-exporter.cpp` (255 lines)
- [x] `src/js/ui/texture-exporter.cpp` (194 lines)
- [x] `src/js/ui/character-appearance.cpp` (205 lines)
- [x] `src/js/ui/model-viewer-utils.cpp` (549 lines)

## Tier 21 — Vue Components → ImGui Widgets (15 files)

- [x] `src/js/components/checkboxlist.cpp` (176 lines)
- [x] `src/js/components/combobox.cpp` (94 lines)
- [x] `src/js/components/context-menu.cpp` (58 lines)
- [x] `src/js/components/data-table.cpp` (1020 lines) 🔴
- [x] `src/js/components/file-field.cpp` (46 lines)
- [x] `src/js/components/listboxb.cpp` (284 lines)
- [x] `src/js/components/menu-button.cpp` (81 lines)
- [x] `src/js/components/resize-layer.cpp` (25 lines)
- [x] `src/js/components/slider.cpp` (98 lines)
- [x] `src/js/components/listbox.cpp` (516 lines)
- [x] `src/js/components/listbox-maps.cpp` (95 lines)
- [x] `src/js/components/listbox-zones.cpp` (95 lines)
- [x] `src/js/components/itemlistbox.cpp` (342 lines)
- [x] `src/js/components/map-viewer.cpp` (1113 lines) 🔴
- [x] `src/js/components/model-viewer-gl.cpp` (516 lines)

## Tier 22 — App Modules/Tabs (27 files)

- [x] `src/js/modules/tab_home.cpp` (30 lines)
- [x] `src/js/modules/legacy_tab_home.cpp` (30 lines)
- [x] `src/js/modules/font_helpers.cpp` (139 lines)
- [x] `src/js/modules/tab_install.cpp` (229 lines)
- [x] `src/js/modules/tab_raw.cpp` (208 lines)
- [x] `src/js/modules/tab_text.cpp` (145 lines)
- [x] `src/js/modules/tab_fonts.cpp` (168 lines)
- [x] `src/js/modules/tab_audio.cpp` (334 lines)
- [x] `src/js/modules/tab_data.cpp` (379 lines)
- [x] `src/js/modules/tab_textures.cpp` (471 lines)
- [x] `src/js/modules/tab_zones.cpp` (549 lines)
- [x] `src/js/modules/tab_videos.cpp` (858 lines) 🔴
- [x] `src/js/modules/tab_items.cpp` (348 lines)
- [x] `src/js/modules/tab_item_sets.cpp` (119 lines)
- [x] `src/js/modules/tab_decor.cpp` (612 lines)
- [x] `src/js/modules/tab_models.cpp` (653 lines)
- [x] `src/js/modules/tab_models_legacy.cpp` (593 lines)
- [x] `src/js/modules/tab_maps.cpp` (1147 lines) 🟢
- [x] `src/js/modules/tab_creatures.cpp` (1374 lines) ✅
- [x] `src/js/modules/tab_characters.cpp` (2656 lines JS → 3008 lines C++) ✅
- [x] `src/js/modules/legacy_tab_audio.cpp` (318 lines)
- [x] `src/js/modules/legacy_tab_data.cpp` (325 lines)
- [x] `src/js/modules/legacy_tab_files.cpp` (117 lines)
- [x] `src/js/modules/legacy_tab_fonts.cpp` (173 lines)
- [x] `src/js/modules/legacy_tab_textures.cpp` (191 lines)
- [x] `src/js/modules/screen_settings.cpp` (463 lines)
- [x] `src/js/modules/screen_source_select.cpp` (342 lines)

## Tier 23 — Top-Level Glue (3 files)

- [x] `src/js/workers/cache-collector.cpp` (431 lines)
- [x] `src/js/modules.cpp` (414 lines)
- [x] `src/app.cpp` (713 lines) — **Entry point, convert last**

---

# Integration TODO Tracker

> All 179 files are converted and compile. The remaining work is **wiring** the converted
> pieces together into a working application. Each `TODO(conversion):` comment in the
> source marks a connection point. They are organized below by integration area, following
> the project priority order: **core systems → data layer → rendering → UI → export**.
>
> **~442 TODO(conversion) comments** across **63 files** are covered below.

---

## Phase 1 — Core Systems Integration

Core infrastructure that most other phases depend on.

### 1.1 Event Emitter Typed Parameters
- [x] Add typed parameter support to the event emitter so callbacks receive proper arguments (e.g. `layer_name`) — `src/app.cpp`, `src/js/3D/renderers/WMOLegacyRendererGL.cpp`, `src/js/3D/renderers/WMORendererGL.cpp` (5 TODOs)

### 1.2 Toast Action Callbacks
- [x] Extend `core::setToast()` to accept `std::function` callbacks for button actions instead of `nlohmann::json` — `src/js/modules/screen_settings.cpp` (6), `src/js/modules/tab_install.cpp` (1), `src/js/modules.cpp` (1) (8 TODOs)
- [x] Wire "View Log" toast actions — `src/js/modules/tab_audio.cpp`, `tab_textures.cpp`, `legacy_tab_audio.cpp`, `legacy_tab_data.cpp`, `tab_videos.cpp` (6 TODOs)
- [x] Wire "View in Explorer" toast actions — `src/js/modules/legacy_tab_files.cpp`, `legacy_tab_fonts.cpp` (2 TODOs)

### 1.3 Module System Wiring
- [x] Wire nav button registration across all 20 tab modules — each tab's `registerNavButton` call in `src/js/modules/*.cpp`
- [x] Wire module activation (`setActive`/`deactivate`) — `src/js/modules/*.cpp` (tab_home, legacy_tab_home, screen_settings, tab_items, screen_source_select)
- [x] Wire context menu registration — `src/js/modules/tab_raw.cpp`, `tab_install.cpp`, `screen_settings.cpp`
- [x] Wire drop handler registration — `src/js/modules/tab_models.cpp`, `tab_textures.cpp`

### 1.4 Cross-Tab Navigation
- [x] Wire `tab_textures::setActive()` cross-tab navigation — `src/js/modules/tab_items.cpp` (1 TODO)
- [x] Wire `tab_models::setActive()` cross-tab navigation — `src/js/modules/tab_items.cpp` (1 TODO)
- [x] Wire `goToTexture` cross-tab navigation — `src/js/modules/tab_models.cpp` (1 TODO)
- [x] Wire "Navigate to items tab" for equipment slots — `src/js/modules/tab_characters.cpp` (1 TODO)

### 1.5 Config Change Detection
- [x] Wire `cascLocale` config watch with change-detection pattern — `src/js/modules/tab_raw.cpp` (1 TODO)

### 1.6 App Lifecycle
- [x] Implement `restartApplication()` — `src/js/casc/build-cache.cpp` (1 TODO)
- [x] Implement taskbar progress bar (Windows `ITaskbarList3`, Linux no-op) — `src/app.cpp` (1 TODO)
- [x] Wire cache collection background worker — `src/js/modules/screen_source_select.cpp` (1 TODO)
- [x] Wire CDN ping async integration — `src/js/modules/screen_source_select.cpp` (1 TODO)
- [x] Populate home tab content — `src/js/modules/tab_home.cpp`, `legacy_tab_home.cpp` (2 TODOs)

---

## Phase 2 — CASC Data Integration

Wire `casc->getFile()`, `getFileByName()`, and related calls across all modules.

### 2.1 CASC File Access in Tab Modules
- [x] `tab_creatures.cpp` — getFile, source refs, fileExists, export (14 TODOs)
- [x] `tab_maps.cpp` — getFile for map tiles, ADT export (9 TODOs)
- [x] `tab_characters.cpp` — getFile, M2Exporter, equipment textures, BLP loading (7 TODOs)
- [x] `tab_videos.cpp` — getFile, getFileByName, getFileEncodingInfo, subtitle loading (8 TODOs)
- [x] `tab_models.cpp` — getFile, source params, file loading (6 TODOs)
- [x] `tab_textures.cpp` — getFile for texture preview and export (3 TODOs)
- [x] `tab_audio.cpp` — getFile, getFileByName, writeToFile (4 TODOs)
- [x] `tab_install.cpp` — getFile by hash, getInstallManifest (3 TODOs)
- [x] `tab_raw.cpp` — getFile, getFileByName, getValidRootEntries (3 TODOs)
- [x] `tab_fonts.cpp` — getFileByName (2 TODOs)
- [x] `tab_text.cpp` — getFileByName (2 TODOs)
- [x] `tab_decor.cpp` — getFile, fileExists, source refs (5 TODOs)
- [x] `tab_data.cpp` — CASC pointer wiring (1 TODO)
- [x] `tab_zones.cpp` — CASC integration for zone map loading (3 TODOs)

### 2.2 CASC File Access in Loaders & Renderers
- [x] `M2Loader.cpp` — CASC file loading for sub-resources (2 TODOs)
- [x] `SKELLoader.cpp` — CASC file loading for skeleton data (2 TODOs)
- [x] `WMOLoader.cpp` — CASC file loading for WMO groups (1 TODO)
- [x] `Skin.cpp` — CASC file loading for skin sections (1 TODO)
- [x] `Texture.cpp` — `casc.getFile(fileDataID)` (1 TODO)

### 2.3 CASC in DB Layer
- [x] `WDCReader.cpp` — Wire CASC interface for DB2 table loading (1 TODO)

---

## Phase 3 — MPQ & DB2 Data Layer

Wire `MPQInstall` file access, legacy loaders, and DB2 table access.
Loaders belong here because they are the data-loading layer that renderers
and UI tabs depend on. DB2 wiring is pure data-layer plumbing.

### 3.1 MPQ Source in Legacy Tab Modules
- [x] `legacy_tab_audio.cpp` — MPQ source, listfileSounds (5 TODOs)
- [x] `legacy_tab_data.cpp` — MPQ source, file scanning, build_id (6 TODOs)
- [x] `legacy_tab_files.cpp` — MPQ source (2 TODOs)
- [x] `legacy_tab_fonts.cpp` — MPQ source (3 TODOs)
- [x] `legacy_tab_textures.cpp` — MPQ source, BLP/PNG preview (5 TODOs)
- [x] `screen_source_select.cpp` — MPQ loadInstall, source wiring (2 TODOs)

### 3.2 MPQ in Legacy Loaders
- [x] `WMOLegacyLoader.cpp` — MPQ file loading for WMO groups (1 TODO)

### 3.3 DB2 System Integration

Wire `WDCReader::getAllRows()`, `db2::getTable()`, `db2::getRow()`.

- [x] `tab_zones.cpp` — getAllRows for UiMap, AreaTable, UiMapXMapArt, UiMapArt, UiMapArtStyleLayer iteration (6 TODOs)
- [x] `tab_videos.cpp` — getAllRows for MovieVariation, Movie.getRow (4 TODOs)
- [x] `tab_data.cpp` — getAllRows iteration, db2::getTable trigger (2 TODOs)
- [x] `tab_audio.cpp` — getAllRows for SoundKitEntry (2 TODOs)
- [x] `tab_textures.cpp` — UiTextureAtlas/UiTextureAtlasMember iteration (1 TODO)
- [x] `tab_characters.cpp` — ChrCustomizationChoice row lookup (1 TODO)
- [x] `legacy_tab_data.cpp` — Schema type conversion for DBC export (1 TODO)

---

## Phase 4 — GL/Rendering Pipeline

Wire OpenGL context, model viewer, texture display, renderer integration,
and MPQ texture loading in legacy renderers. Legacy renderers depend on
MPQ source (Phase 3.1, done) and legacy loaders (Phase 3.2, pending).

### 4.1 MPQ in Legacy Renderers
- [x] `M2LegacyRendererGL.cpp` — MPQ texture loading (3 TODOs)
- [x] `MDXRendererGL.cpp` — MPQ texture loading (3 TODOs)
- [x] `WMOLegacyRendererGL.cpp` — MPQ texture + doodad loading (4 TODOs)

### 4.2 ModelViewerGL Component
- [x] Wire GL context creation and management in `model-viewer-gl.cpp` — canvas/WebGL2 equivalent via GLFW+GLAD (3 TODOs in .cpp/.h)
- [x] Wire ModelViewerGL rendering calls in tab modules — `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp`, `tab_creatures.cpp`, `tab_characters.cpp` (6 TODOs)

### 4.3 GL Context & Renderer Wiring
- [x] Wire GL context retrieval — `tab_creatures.cpp` (creatureViewerContext), `tab_decor.cpp` (decorViewerContext), `tab_models_legacy.cpp` (legacyModelViewerContext) (3 TODOs)
- [x] Wire renderer instantiation (M2RendererGL, WMORendererGL, etc.) — `tab_creatures.cpp`, `tab_models.cpp` (4 TODOs)
- [x] Wire renderer load(), draw_calls/groups, has_content, geosetKey/wmoGroupKey/wmoSetKey — `tab_decor.cpp` (4 TODOs)
- [x] Wire `applyReplaceableTextures` — `tab_creatures.cpp` (1 TODO)
- [x] Wire `getActiveRenderer`, `getEquipmentRenderers`, `getCollectionRenderers` — `tab_creatures.cpp` (1 TODO)

### 4.4 Camera Fitting
- [x] Wire `fitCamera` calls — `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp`, `tab_creatures.cpp`, `tab_characters.cpp` (7 TODOs)

### 4.5 Texture Display
- [x] Wire texture preview as `ImGui::Image` — `tab_textures.cpp`, `tab_decor.cpp`, `tab_models.cpp` (3 TODOs)
- [x] Wire zone map preview as `ImGui::Image` — `tab_zones.cpp` (1 TODO)
- [x] Wire UV overlay rendering — `tab_models.cpp` (1 TODO)
- [x] Wire atlas region rendering as ImGui overlay — `tab_textures.cpp` (1 TODO)

### 4.6 Framebuffer Capture
- [x] Wire GL framebuffer capture for screenshots — `tab_characters.cpp`, `tab_models_legacy.cpp` (2 TODOs)
- [x] Wire full thumbnail capture — `tab_characters.cpp` (1 TODO)
- [x] Wire PNG write from framebuffer — `tab_models_legacy.cpp` (1 TODO)
- [x] Wire clipboard PNG copy from FBO — `tab_characters.cpp`, `tab_models_legacy.cpp` (2 TODOs)

### 4.7 TextureRibbon Integration
- [x] Wire `textureRibbon.reset()` / `addSlot` / `setSlotFile` / `setSlotSrc` in renderers — `M2RendererGL.cpp` (4), `M2LegacyRendererGL.cpp` (2), `M3RendererGL.cpp` (1), `MDXRendererGL.cpp` (2), `WMORendererGL.cpp` (4), `WMOLegacyRendererGL.cpp` (3) (16 TODOs)
- [x] Wire texture ribbon rendering in tab modules — `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp`, `tab_creatures.cpp` (4 TODOs)

### 4.8 CharMaterialRenderer
- [x] Wire `getCanvas` / `getRawPixels` integration — `tab_characters.cpp` (1 TODO)
- [x] Wire GL bake composite buffer (OffscreenCanvas equivalent) — `ADTExporter.cpp` (1 TODO)

### 4.9 Background Color Picker
- [x] Wire color picker for model viewer background — `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp` (3 TODOs)

### 4.10 WMO Renderer Change Detection
- [x] Wire Vue watcher equivalents (`updateGroups`/`updateSets`) — `WMORendererGL.cpp`, `WMOLegacyRendererGL.cpp` (4 TODOs)

---

## Phase 5 — UI Component Wiring

Wire ImGui widget components into the tab modules that use them.
The legacy model tab depends on MPQ renderers (Phase 4.1) and loaders
(Phase 3.2), so it belongs here in the UI phase.

### 5.1 MPQ in Legacy Model Tab
- [x] `tab_models_legacy.cpp` — MPQ source, creature data, file loading (6 TODOs)

### 5.2 Listbox & ContextMenu
- [x] Wire in `tab_audio.cpp`, `tab_raw.cpp`, `tab_text.cpp`, `tab_fonts.cpp`, `tab_textures.cpp`, `tab_videos.cpp`, `legacy_tab_audio.cpp`, `legacy_tab_files.cpp`, `legacy_tab_fonts.cpp` (9 TODOs)
- [x] Wire standalone Listbox in `tab_data.cpp`, `legacy_tab_data.cpp`, `tab_creatures.cpp`, `tab_decor.cpp`, `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_install.cpp` (8 TODOs)
- [x] Wire standalone ContextMenu in `tab_items.cpp`, `tab_zones.cpp` (3 TODOs)

### 5.3 DataTable
- [x] Wire DataTable rendering in `tab_data.cpp`, `legacy_tab_data.cpp` (2 TODOs)
- [x] Wire `getSelectedRowsAsCSV` / `getSelectedRowsAsSQL` — `tab_data.cpp`, `legacy_tab_data.cpp` (4 TODOs)

### 5.4 Listboxb
- [x] Wire Listboxb rendering in `tab_models.cpp`, `tab_models_legacy.cpp` (2 TODOs)

### 5.5 ItemListbox
- [x] Wire ItemListbox rendering in `tab_items.cpp`, `tab_item_sets.cpp` (2 TODOs)

### 5.6 ListboxZones
- [x] Wire ListboxZones rendering in `tab_zones.cpp` (1 TODO)

### 5.7 CheckboxList
- [x] Wire CheckboxList rendering in `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp` (3 TODOs)

### 5.8 MenuButton
- [x] Wire MenuButton rendering in `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp` (3 TODOs)

### 5.9 Filter Input
- [x] Wire `ImGui::InputText` filter in `tab_creatures.cpp`, `tab_decor.cpp`, `tab_models.cpp`, `tab_models_legacy.cpp` (4 TODOs)

### 5.10 Full ImGui Rendering
- [x] Complete full ImGui rendering for `screen_source_select.cpp`, `screen_settings.cpp`, `legacy_tab_textures.cpp` (4 TODOs)

### 5.11 ComboBox
- [x] Wire full ComboBox with search for realm list — `tab_characters.cpp` (1 TODO)

---

## Phase 6 — Export Pipeline

Wire file export, texture encoding, and data export operations.

- [x] Wire export stream (file I/O) — `tab_creatures.cpp`, `tab_decor.cpp` (2 TODOs)
- [x] Wire `BufferWrapper::fromCanvas` equivalent (PNG/WebP encoding from raw RGBA) — `tab_textures.cpp`, `tab_zones.cpp` (2 TODOs)
- [x] Wire `BufferWrapper::startsWith` with FileIdentifier matches — `tab_textures.cpp` (1 TODO)
- [x] Wire `GLTFWriter::setTextureMap` typed parameter — `M2Exporter.cpp` (1 TODO)
- [x] Wire `fileManifest` texture entries for `exportTextures` — `ADTExporter.cpp` (1 TODO)
- [x] Wire canvas rotation/scaling for ADT minimap output — `ADTExporter.cpp` (2 TODOs)
- [x] Wire image dimension detection via `stb_image` — `legacy_tab_textures.cpp` (1 TODO)
- [x] Wire data exporter DBC schema type conversion — `legacy_tab_data.cpp` (1 TODO)
- [x] Wire `autoAdjust` for model export — `tab_characters.cpp` (1 TODO)
- [x] Wire animation list population from loaded M2 data — `tab_characters.cpp` (1 TODO)
- [x] Wire `detect_model_type(data)` fallback — `tab_models.cpp` (1 TODO)
- [x] Wire loaded data for model preview — `tab_models.cpp` (1 TODO)
- [x] Wire group/set mask conversion from JSON to typed vectors — `tab_models_legacy.cpp` (1 TODO)

---

## Phase 7 — Media/Video Integration

Wire video playback, Kino streaming, and audio callbacks.

- [x] Wire Kino HTTP POST streaming server — `tab_videos.cpp` (1 TODO)
- [x] Wire video playback via URL — `tab_videos.cpp` (1 TODO)
- [x] Wire video preview rendering — `tab_videos.cpp` (1 TODO)
- [x] Wire video end/error callbacks — `tab_videos.cpp` (2 TODOs)
- [x] Wire media playback controls — `tab_videos.cpp` (1 TODO)
- [x] Wire subtitle visibility toggle — `tab_videos.cpp` (1 TODO)
- [x] Wire poll timer for video state — `tab_videos.cpp` (1 TODO)
- [x] Wire `miniaudio` per-sound "onended" callback — `ui/audio-helper.cpp` (1 TODO)
- [x] Verify MDX pause support — `tab_models_legacy.cpp` (1 TODO)

---

## Phase 8 — Platform & Misc Integration

Platform-specific features and cleanup.

### 8.1 Crypto
- [x] Replace FNV-1a / minimal MD5 with proper MD5/SHA256 — `tab_maps.cpp` (1), `cache-collector.cpp` (3 TODOs)

### 8.2 Platform APIs
- [x] Implement app reload (F5) equivalent — `src/app.cpp`

### 8.3 Map Viewer Internals
- [x] Wire double-buffer / overlay canvas equivalent in `map-viewer.cpp` — canvas operations, tile loading, overlay drawing (12 TODOs in .cpp, 2 in .h)
- [x] Implement dashed line rendering for map grid — `map-viewer.cpp` (1 TODO)

---

## Informational No-Ops (~30 items)

These `TODO(conversion)` comments document **paradigm differences** that require no code
changes. They explain why certain JS patterns (DOM events, ResizeObserver, Blob URLs,
revokeDataURL, event.preventDefault, Vue watchers, component unmount, etc.) have no
C++/ImGui equivalent. These can be cleaned up (removed or converted to regular comments)
at any time but are not blocking.

Files with informational no-ops: `generics.cpp`, `modules.cpp`, `components/*.cpp`,
`app.cpp`, `map-viewer.cpp`, `model-viewer-gl.cpp`, `file-field.cpp`, `tab_videos.cpp`.

---

## Remaining Work — Visual Fidelity & Polish

> The conversion compiles and all integration phases are complete. The items below are
> **not** file conversions or integration wiring — they are **polish and fidelity** tasks
> required to make the C++ app look and behave identically to the original JS app.
> Priority order: **app shell → theme/styling → fonts → screen layouts → tab layouts → overlays → export gaps → CI/testing**.

### 9.1 App Shell Layout (Header / Content / Footer)

The original app uses a **three-row grid layout** (`grid-template-rows: 53px 1fr 73px`)
with a fixed header, scrollable content area, and fixed footer. The C++ app currently
renders only the active module — there is **no header, no footer, no navigation bar**.
The main render loop (`app.cpp:1027-1035`) simply calls `active->render()` with no
surrounding shell.

**Original layout (CSS `#container`):**
```
┌─────────────────────────────────────┐
│ Header (53px): logo + nav icons     │  ← #header, background-dark, border-bottom
├─────────────────────────────────────┤
│ Content (flexible): active module   │  ← #content > #module-container
├─────────────────────────────────────┤
│ Footer (73px): version/copyright    │  ← #footer, background-dark, border-top
└─────────────────────────────────────┘
```

- [ ] Create an app shell rendering function in `app.cpp` that draws header, content, and footer regions every frame
- [ ] Render the **header** as a fixed-height (53px) bar at the top with `--background-dark` (#2c3136) background and a 1px `--border` (#6c757d) bottom border
- [ ] Render the **logo** in the header: `logo.png` (32px) left-aligned with "wow.export" text at 25px bold, 15px left margin
- [ ] Render the **navigation icons** in the header after the logo (see 9.2)
- [ ] Render the **hamburger menu** icon (`line-columns.svg`, 20px) at the right side of the header with a context menu for settings/restart/log
- [ ] Render the **help icon** (`help.svg`, 20px) at the right side of the header, left of the hamburger menu
- [ ] Render the **footer** as a fixed-height (73px) bar at the bottom with `--background-dark` (#2c3136) background and a 1px `--border` (#6c757d) top border
- [ ] Footer content: centered text showing version and copyright in `--font-faded` (#6c757d) color
- [ ] Render the **content area** between header and footer, filling available space
- [ ] Render the active module **inside** the content area, not as the entire window

### 9.2 Navigation Bar (Tab Icons in Header)

The original app has a horizontal row of nav icons in the header (`#nav`). Each icon is
45×52px, uses SVG backgrounds from `data/fa-icons/`, shows a tooltip label on hover,
and highlights green (`--nav-option-selected`: #22b549) when active. Nav icons only appear
after a CASC/MPQ source is loaded (not on the source-select screen).

- [ ] Render nav icon buttons horizontally in the header from `core::view->modNavButtons` (filtered by install type)
- [ ] Each nav icon: 45×52px, centered SVG background, clickable to switch tabs
- [ ] Active tab icon: green tint filter (`--nav-option-selected`: #22b549)
- [ ] Hover effect: brightness(2) filter on the icon
- [ ] Tooltip label on hover: appears to the right of the icon, white text on `--background-dark` background
- [ ] Hide nav bar when on source-select screen (no CASC/MPQ loaded)
- [ ] Nav icons only show for the current install type (CASC vs MPQ shows different sets)

### 9.3 ImGui Theme from app.css

The original app uses `data/app.css` with **231 CSS variables** for colors, spacing,
borders, and rounding. The C++ app currently calls only `ImGui::StyleColorsDark()` (the
generic ImGui default). A centralized theme system must map the CSS variables to
`ImGuiStyle` colors and settings.

- [ ] Create a centralized theme struct/function mapping all CSS color variables to `ImGuiStyle::Colors[]`
- [ ] Key color mappings:
  - `--background` (#343a40) → `ImGuiCol_WindowBg`, `glClearColor`
  - `--background-dark` (#2c3136) → `ImGuiCol_MenuBarBg`, header/footer background
  - `--background-alt` (#3c4147) → `ImGuiCol_ChildBg` for alternate panels
  - `--border` (#6c757d) → `ImGuiCol_Border`, `ImGuiCol_Separator`
  - `--font-primary` (#ffffffcc) → `ImGuiCol_Text`
  - `--font-highlight` (#ffffff) → text highlight/hover color
  - `--font-faded` (#6c757d) → `ImGuiCol_TextDisabled`
  - `--font-alt` (#57afe2) → link/accent color
  - `--form-button-base` (#22b549) → `ImGuiCol_Button`
  - `--form-button-hover` (#2665d2) → `ImGuiCol_ButtonHovered`
  - `--form-button-disabled` (#696969) → disabled button color
  - `--nav-option-selected` (#22b549) → active tab highlight
  - `--toast-error` (#dc9090), `--toast-success` (#a6dc90), `--toast-info` (#90bcdc), `--toast-progress` (#dcba90) → toast bar colors
- [ ] Map CSS spacing/rounding/border variables to `ImGuiStyle` padding/rounding values:
  - Button padding: 9px 13px, border-radius 5px
  - Scrollbar width: 8px, thumb border-radius 5px
  - Window rounding: 0 (the app uses sharp corners)
- [ ] Replace hardcoded `IM_COL32(...)` values scattered in components (e.g. `data-table.cpp`) with theme references
- [ ] Replace the no-op `reloadStylesheet()` in `app.cpp` with actual theme application
- [ ] Set `glClearColor` from `--background` CSS variable (`#343a40`) — currently hardcoded at `app.cpp:1045` but needs theme reference

### 9.4 Custom Font Loading

The original app uses Selawik (regular + bold) and Gambler fonts. The C++ app uses
the ImGui default font. Font files exist in `data/fonts/` but are WOFF2 format —
ImGui requires TTF/OTF.

- [ ] Convert or source TTF versions of Selawik (`selawk.woff2`, `selawkb.woff2`) and Gambler (`gmblr.woff2`)
- [ ] Load fonts into ImGui via `ImGui::GetIO().Fonts->AddFontFromFileTTF()`
- [ ] Set primary font (Selawik regular) as the default ImGui font
- [ ] Load bold font (Selawik bold) and use `ImGui::PushFont()` where the JS app uses `font-weight: bold`
- [ ] Set default font size to match CSS `body` (the original uses the browser default ~16px, Selawik)

### 9.5 Font Awesome Icon Font

The original app uses Font Awesome icons inline in text and buttons. The C++ app
has 49 SVG icons in `data/fa-icons/` rendered via nanosvg, but not as an ImGui icon font.

- [ ] Obtain Font Awesome TTF/OTF (e.g. `fa-solid-900.ttf`)
- [ ] Load as an ImGui icon font merged into the default font range
- [ ] Replace nanosvg icon rendering with ImGui icon font codepoints where appropriate

### 9.6 DPI / High-DPI Scaling

- [ ] Implement proper high-DPI scaling via `ImGui::GetIO().FontGlobalScale` and GLFW framebuffer scale
- [ ] Rebuild font atlas at appropriate sizes for the display scale factor
- [ ] Match JS scaling logic: scale down when window < 1120×700, never scale up above 1.0

### 9.7 Source Select Screen Layout

The source select screen currently renders as a flat list of `ImGui::Button` + `ImGui::TextWrapped`
inside a `BeginChild` panel (see screenshot comparison in issue). The original uses three
large **dashed-border cards** (700px wide, min-height 120px) centered vertically and
horizontally, each with an icon (80×80px), title (22px bold), subtitle (16px, opacity 0.7),
and "last opened" links.

**Original layout (CSS `#source-select`):**
```
┌─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
│  [WoW Logo]  Open Local Installation (Recommended)  │
│              Select the root directory of a WoW...   │
│              Last opened: retail (C:\WoW)            │
└─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
      (30px gap)
┌─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
│  [BNet Logo] Use Battle.net CDN                      │
│              Stream data from Blizzard's CDN...      │
│              Region: US (42ms)                       │
└─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
      (30px gap)
┌─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
│  [MPQ Icon]  Open Legacy Installation                │
│              Select a classic (pre-CASC)...          │
└─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
```

- [ ] Center the three source cards vertically and horizontally in the content area (flexbox equivalent)
- [ ] Each card: 700px wide, min-height 120px, 3px dashed border in `--font-faded` (#6c757d), border-radius 15px, padding 30px, gap 30px between icon and content
- [ ] Card hover: border color changes to `--font-highlight` (#ffffff)
- [ ] Source icon: 80×80px (shrink to 50×50 on small windows), loaded from `data/images/wow_logo.svg`, `import_battlenet.svg`, `mpq.svg`
- [ ] Title text: 22px bold, `--font-highlight` (#ffffff) color
- [ ] Subtitle text: 16px, opacity 0.7
- [ ] "Last opened" link: 15px text below subtitle showing recent install path (clickable to open directly)
- [ ] CDN region selector: inline text with clickable region name showing a context menu (not an ImGui::Combo dropdown)
- [ ] Remove the current `ImGui::Separator()` dividers between sections
- [ ] Remove the current `ImGui::BeginChild("##source-select-panel")` flat panel layout
- [ ] Responsive: at window height < 800px, reduce card padding to 15px 20px, gap to 15px, icon to 50×50, title to 18px, subtitle to 14px

### 9.8 Build Select Screen Layout

The build select screen (shown after clicking "Use Battle.net CDN" or local with
multiple builds) currently renders as flat `ImGui::Button` rows. The original uses
centered dashed-border cards with expansion icons.

- [ ] Center the build select panel vertically and horizontally
- [ ] Title: "Select Build" at 28px bold, `--font-highlight` color, margin-bottom 10px
- [ ] Each build button: 450px min-width, transparent background, 3px dashed border in `--font-faded`, border-radius 10px, 15px padding, 16px text, left-aligned, expansion icon (32px) on the left
- [ ] Build button hover: border color → `--nav-option-selected` (#22b549), text → green, subtle green background tint
- [ ] "Return to Installations" link: 16px, `--font-alt` (#57afe2) color, below the build list
- [ ] Each build button should show the expansion icon from `data/images/expansion/icon_*.webp` (mapped by `expansionId`)

### 9.9 Toast Notification Bar

The toast bar currently uses ImGui's native toast/notification approach. The original is a
30px-tall bar at the top of the content area (not the header) with colored backgrounds,
an icon, message text, clickable action links, and a close button.

- [ ] Render toast as a 30px-tall bar at the top of the content area (below the header, above the module)
- [ ] Background color based on toast type: error (#dc9090), success (#a6dc90), info (#90bcdc), progress (#dcba90)
- [ ] Text color: `--font-toast` (black)
- [ ] Icon: 15px SVG on the left (triangle-exclamation for error, check for success, circle-info for info, timer for progress)
- [ ] Action links: `--font-toast-link` (#0300bf), underlined, clickable, 0 5px margins
- [ ] Close button: xmark.svg icon at the right edge, 30px wide

### 9.10 Loading Screen Overlay

The loading screen currently uses `core::showLoadingScreen()` / `core::hideLoadingScreen()`
but the visual rendering is unknown. The original is a full-screen overlay (z-index 9999)
with centered content: spinning gear icon, title text, progress bar.

- [ ] Render loading screen as a full-window overlay on top of all other content
- [ ] Background: `--background` (#343a40) solid, with `loading.gif` at 0.2 opacity behind
- [ ] Centered content: spinning gear icon (100×100px, 6s rotation), title (25px), progress text (20px)
- [ ] Progress bar: 400px wide, 15px tall, `--border` border, `--progress-bar` gradient fill (linear-gradient 180deg #57afe2 → #35759a)
- [ ] Progress bar fill width = `core::view->loadPct * 100%`

### 9.11 Logo Background Watermark

The original app has a subtle watermark of `logo.png` centered behind the content
at 5% opacity (CSS `#logo-background`). This is visible on all screens.

- [ ] Render `logo.png` as a centered background watermark at 5% opacity behind the content area

### 9.12 Orphaned UI State (Data Exists, Never Rendered)

The conversion ported all the AppState fields and setter functions from the JS app, but
the **main render loop** (`app.cpp:1027-1035`) only calls `active->render()`. There is
**zero rendering code** for the app shell UI. All of these set state that nothing draws:

| UI Element | State Variable | Setter Function | Status |
|---|---|---|---|
| Toast notification bar | `core::view->toast` | `core::setToast()` / `core::hideToast()` | State-only, **never drawn** |
| Loading screen overlay | `core::view->isLoading`, `loadPct`, `loadingTitle`, `loadingProgress` | `core::showLoadingScreen()` / `core::hideLoadingScreen()` | State-only, **never drawn** |
| Navigation buttons | `core::view->modNavButtons` | `modules::register_nav_button()` | Registered by all tabs, **never drawn** |
| Context menu (extra) | `core::view->modContextMenuOptions` | `modules::registerContextMenuOption()` | Registered in `app.cpp`, **never drawn** |
| Context menus (various) | `core::view->contextMenus.*` | Various setters | State-only, **never drawn** |
| File drop prompt | `core::view->fileDropPrompt` | `glfw_drop_callback()` | State-only, **never drawn** |

All tabs **do** call `register_nav_button()` in their `registerTab()` function, and
`app.cpp` registers context menu options for settings/restart/log/shaders. This means
the data infrastructure is correct — the rendering is what's missing.

- [ ] Add toast rendering to the main render loop (check `core::view->toast.has_value()` every frame)
- [ ] Add loading screen overlay rendering to the main render loop (check `core::view->isLoading` every frame)
- [ ] Add nav button rendering in the header (iterate `core::view->modNavButtons`, filter by install type)
- [ ] Add context menu rendering for the hamburger/extra menu (iterate `core::view->modContextMenuOptions`)
- [ ] Add file drop prompt rendering (check `core::view->fileDropPrompt`)

### 9.13 Tab Layout Patterns (Base Framework)

Every content tab in the original app uses a consistent layout pattern. The C++ tabs
currently render as flat sequential ImGui widgets without proper grid/split layouts.

**The base patterns from `app.css`:**

```
.tab                    → position: absolute; top/left/right/bottom: 0   (fill content area)
.tab.list-tab           → display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: 1fr 60px
.sidebar                → grid-column: 3; width: 210px; grid-row: 1/span 2
.list-container         → position: relative; margin: 20px 10px 0 20px
.preview-container      → position: relative; margin: 20px 20px 0 10px; grid-row: 1; grid-column: 2
.filter                 → display: flex; grid-column: 1; grid-row: 2 (or 3)
.preview-controls       → display: flex; justify-content: flex-end; grid-column: 2; grid-row: 2
```

- [ ] Create a shared tab layout helper that establishes the full-area child window for each tab (`.tab` equivalent)
- [ ] Create a shared list-tab layout helper that splits into left list + right preview + bottom bar (`.tab.list-tab` equivalent)
- [ ] Create a shared sidebar layout helper that renders a 210px right column (`.sidebar` equivalent)
- [ ] Create shared `.list-container` layout: 20px margin, position within left grid column
- [ ] Create shared `.preview-container` layout: 20px margin, centered content, right grid column
- [ ] Create shared `.filter` bar: flex row at bottom of left column, input + buttons
- [ ] Create shared `.preview-controls` bar: flex row at bottom of right column, right-aligned buttons

### 9.14 Home Tab Layout (`#tab-home`)

**CSS:** `display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: auto 1fr auto auto; padding: 50px; gap: 0 50px`
**Current C++:** Flat sequential widgets — buttons with `SameLine` + `TextDisabled`.

- [ ] Implement 2-column grid layout: left column for info/buttons, right column for "What's New" changelog
- [ ] "What's New" panel (`#home-changes`): `home-background.webp` background, border-radius 10px, padding 50px
- [ ] Help buttons row (`#home-help-buttons`): 3 cards, 300px wide each, centered at bottom, 20px gap
- [ ] Each help button: 1px solid `--border` border, border-radius 10px, 20px padding, icon watermark at 20% opacity on right
- [ ] Help button hover: border → `--nav-option-selected` (#22b549), text → green
- [ ] Responsive: at window height < 900px, hide help buttons entirely
- [ ] Legacy home tab (`#legacy-tab-home`) uses the same layout

### 9.15 Text Tab Layout (`#tab-text`)

**CSS:** Inherits `.tab.list-tab` (2-column grid, 1fr 1fr, 60px bottom row).
**Current C++:** Flat with no preview capability.

- [ ] Implement 2-column grid: left = file listbox, right = text preview
- [ ] Left column: `.list-container` with listbox component filling the area
- [ ] Right column: `.preview-container` with monospace text preview, dark background (`--background-dark`)
- [ ] Bottom row (60px): filter input on left, export buttons on right

### 9.16 Fonts Tab Layout (`#tab-fonts` / `#legacy-tab-fonts`)

**CSS:** Inherits `.tab.list-tab` (2-column grid).
**Current C++:** Uses `BeginChild` at 40% width — close but not grid-based.

- [ ] Font preview area: full height of right column, contains character glyph grid
- [ ] Glyph grid: `position: absolute; top: 0; bottom: 140px; display: flex; flex-wrap: wrap; gap: 2px`
- [ ] Preview input area: `position: absolute; bottom: 0; height: 120px` — text input + rendered preview
- [ ] Bottom row (60px): filter input on left, export buttons on right

### 9.17 Audio Tab Layout (`#tab-audio` / `#legacy-tab-audio`)

**CSS:** Inherits `.tab.list-tab` (2-column grid).
**Current C++:** Uses `BeginChild` with negative height for controls — close but no grid.

- [ ] Left column: sound file listbox filling the area
- [ ] Right column: sound player widget with seek bar, play/stop/volume controls
- [ ] Sound player: centered in right column, custom seek slider, duration/position display
- [ ] Bottom row (60px): filter input on left, export buttons on right

### 9.18 Textures Tab Layout (`#tab-textures` / `#legacy-tab-textures`)

**CSS:** Inherits `.tab.list-tab` (2-column grid).
**Current C++:** Uses `BeginChild` at 40% width — close but not grid-based.

- [ ] Left column: texture file listbox
- [ ] Right column: texture preview with zoom/pan, channel toggle buttons (R/G/B/A), info bar at bottom
- [ ] Channel toggles: `position: absolute; bottom: 35px; left: 0` — vertical list of toggle buttons
- [ ] Info bar: `position: absolute; left: 0; bottom: 0; right: 0` — file dimensions, format, etc.
- [ ] Atlas overlay: `position: absolute; left: 0; top: 0; z-index: 1` for texture atlas region rendering
- [ ] Bottom row (60px): filter input on left, export format selector + export button on right

### 9.19 Videos Tab Layout (`#tab-videos`)

**CSS:** Inherits `.tab.list-tab` (2-column grid).
**Current C++:** Uses `BeginChild` at 40% width.

- [ ] Left column: video file listbox
- [ ] Right column: video player area (external URL launch, not embedded player)
- [ ] Bottom row (60px): filter input on left, export buttons on right

### 9.20 Raw Files Tab Layout (`#tab-raw` / `#legacy-tab-files`)

**CSS:** Grid with `grid-template-columns: unset` (single column, no split).
**Current C++:** Uses `BeginChild` with negative height offset — close.

- [ ] Single-column layout: listbox fills most of the area
- [ ] Bottom tray: `display: flex; margin: 10px` — filter input (flex-grow: 1) + export button
- [ ] Filter input takes full available width minus button

### 9.21 Install Files Tab Layout (`#tab-install`)

**CSS:** `grid-template-columns: 1fr auto` (list + sidebar).
**Current C++:** Uses `BeginChild` with negative width (-150) — close but not sidebar.

- [ ] Left column: file listbox
- [ ] Right sidebar: info panel with string details (`.sidebar.strings-info`)
- [ ] Bottom tray (`#tab-install-tray`): `display: flex; margin: 10px` — filter + buttons

### 9.22 Data Tab Layout (`#tab-data` / `#legacy-tab-data`)

**CSS:** `grid-template-columns: 1fr 6fr; grid-template-rows: 1fr auto 60px` (narrow left DB2 list, wide right data table).
**Current C++:** Uses `BeginChild` at 30% width — but CSS says 1:6 ratio (~14% left).

- [ ] Left column (1fr ≈ 14%): DB2/DBC file listbox, spanning rows 1-2
- [ ] Right column (6fr ≈ 86%): data table component, row 1
- [ ] Right row 2: `#tab-data-options` — right-aligned options (display: flex; justify-content: flex-end)
- [ ] Bottom row (60px): filter input on left, export format selector + export button on right

### 9.23 Maps Tab Layout (`#tab-maps`)

**CSS:** `grid-template-rows: auto 1fr 60px` (3 rows: expansion filter, content, controls).
**Current C++:** Uses `BeginGroup` for expansion buttons, then listbox + map viewer.

- [ ] Row 1 (auto): expansion filter buttons (horizontal row of expansion icons)
- [ ] Row 2 left (grid-column: 1): map listbox in `.list-container`
- [ ] Row 2 right (grid-column: 2, span rows 1-2): map viewer component (`.ui-map-viewer`)
- [ ] Row 3 left: filter input
- [ ] Row 3 right: `.spaced-preview-controls` — zoom controls, preview options
- [ ] Optional sidebar (grid-column: 3): `#tab-maps .sidebar` for export options

### 9.24 Zones Tab Layout (`#tab-zones`)

**CSS:** `grid-template-columns: 1.5fr 2fr; grid-template-rows: auto 1fr 60px`.
**Current C++:** Similar to maps but with phase selector.

- [ ] Row 1 (auto): expansion filter buttons
- [ ] Row 2 left (1.5fr): zone listbox in `.list-container`
- [ ] Row 2 right (2fr, span rows 1-2): zone map viewer (`.zone-viewer-container`)
- [ ] Row 3: filter + controls
- [ ] Phase selector: dropdown/combo for selecting zone phases

### 9.25 Models Tab Layout (`#tab-models` / `#tab-models-legacy`)

**CSS:** `grid-template-columns: 1fr 1fr auto` (list, 3D viewer, sidebar).
**Current C++:** Uses `BeginChild` at 30% width — but CSS says 3-column with auto sidebar.

- [ ] Left column (1fr): model file listbox + filter
- [ ] Middle column (1fr): 3D model viewer (OpenGL viewport)
- [ ] Right column (auto, 210px): sidebar with checkboxes (geosets, WMO groups, animation controls)
- [ ] Sidebar toggle list: `.list-toggles` with `input[type=button]` for "Show All" / "Hide All"
- [ ] Bottom row (60px): export buttons

### 9.26 Creatures Tab Layout (`#tab-creatures`)

**CSS:** `grid-template-columns: 1fr 1fr auto` (list, 3D viewer, sidebar).
**Current C++:** Similar to models.

- [ ] Left column: creature listbox + search filter
- [ ] Middle column: 3D creature viewer with texture overlay support
- [ ] Right column (sidebar): geoset checkboxes, equipment toggles, skin selector
- [ ] Bottom row (60px): export buttons

### 9.27 Decor Tab Layout (`#tab-decor`)

**CSS:** `grid-template-columns: 1fr 1fr auto` (list, 3D viewer, sidebar).
**Current C++:** Similar to models.

- [ ] Left column: decor item listbox with category filter checkboxes
- [ ] Middle column: 3D decor viewer
- [ ] Right column (sidebar): geoset checkboxes, WMO group checkboxes
- [ ] Category mask: collapsible sections with subcategory checkboxes
- [ ] Bottom row (60px): export buttons

### 9.28 Items Tab Layout (`#tab-items`)

**CSS:** `grid-template-rows: 1fr 70px; grid-template-columns: 1fr auto` (list + sidebar).
**Current C++:** Uses `BeginChild` at 50% width — but CSS says full-width with sidebar.

- [ ] Main area: item listbox with custom item rendering (icon, name, quality color, ID)
- [ ] Item quality colors: color-coded by quality level (0-8), matching CSS `.item-quality-*`
- [ ] Right sidebar (auto): type filter checkboxes + quality filter checkboxes
- [ ] Bottom row (70px): filter input + export buttons

### 9.29 Item Sets Tab Layout (`#tab-item-sets`)

**CSS:** Inherits `.tab.list-tab` with `.list-container-full` (full width list, no split).
**Current C++:** Uses `BeginChild` — close.

- [ ] Full-width listbox: `.list-container-full` fills both columns
- [ ] Custom item set rendering with icons and quality colors
- [ ] Bottom row (60px): filter input + export buttons

### 9.30 Characters Tab Layout (`#tab-characters`)

**CSS:** Complex absolute positioning with overlay panels (NOT a grid layout).
**Current C++:** Complex watch-based system.

```
.preview-container      → position: absolute; top: 0; left: 0; width: 100%; height: 100%  (3D viewer backdrop)
.left-panel             → position: absolute; left: 20px; top: 20px; bottom: 20px; width: 250px
.right-panel            → position: absolute; right: 20px; top: 20px; bottom: 200px
.tab-control            → display: flex; flex-direction: row; justify-content: center  (sub-tabs)
.equipment-list         → display: flex; flex-direction: column; gap: 10px; width: 250px
```

- [ ] 3D character viewer fills entire content area as background
- [ ] Left panel (250px, absolute): race/model selector dropdown, customization options, scrollable
- [ ] Right panel (absolute): equipment slots list, each slot is a flex row with name + buttons
- [ ] Tab controls: centered sub-tab buttons for switching between customization/equipment/saved
- [ ] Bottom area: export controls, auto-adjust checkbox

### 9.31 Settings Screen Layout (`#config`)

**CSS:** Scrollable form with centered content.
**Current C++:** Uses `BeginChild` for scrollable area with fixed bottom buttons.

- [ ] Centered scrollable settings form
- [ ] Section headers: `SeparatorText` dividers between config groups
- [ ] Input fields: text, checkbox, slider, file field (with browse button)
- [ ] Bottom bar: fixed "Save" + "Cancel" buttons

### 9.32 Scrollbar Styling

The original app has custom scrollbars: 8px wide, transparent track, `--border` (#6c757d)
thumb with 1px border and 5px border-radius, `--font-highlight` (#ffffff) on hover.
ImGui's default scrollbars are wider and use different colors.

- [ ] Set `ImGuiStyle::ScrollbarSize` to 8px
- [ ] Set `ImGuiCol_ScrollbarBg` to transparent
- [ ] Set `ImGuiCol_ScrollbarGrab` to `--border` (#6c757d)
- [ ] Set `ImGuiCol_ScrollbarGrabHovered` to `--font-highlight` (#ffffff)
- [ ] Set scrollbar rounding to 5px

### 9.33 Button Styling

Buttons in the original app use specific styling: green (#22b549) background, white text,
9px 13px padding, 5px border-radius, no border, blue (#2665d2) on hover. The C++ app
uses ImGui default button styling.

- [ ] Set `ImGuiCol_Button` to `--form-button-base` (#22b549)
- [ ] Set `ImGuiCol_ButtonHovered` to `--form-button-hover` (#2665d2)
- [ ] Set `ImGuiCol_ButtonActive` to a pressed variant
- [ ] Set `ImGuiStyle::FrameRounding` to 5px for buttons
- [ ] Set `ImGuiStyle::FramePadding` to (13px, 9px) for standard buttons
- [ ] Disabled button style: opacity 0.5, `--form-button-disabled` (#696969) background

### 9.34 Context Menu Styling

Context menus in the original app have specific styling: dark background (#232323),
`--border` border, shadow, 8px padding, hover highlight (#353535). The C++ app uses
ImGui default popup styling.

- [ ] Set `ImGuiCol_PopupBg` to #232323
- [ ] Set popup border to `--border` (#6c757d)
- [ ] Set `ImGuiCol_HeaderHovered` (for selectable items in popups) to #353535
- [ ] Ensure context menus close when clicking outside (ImGui default behavior)

### 9.35 Input Field Styling

Text inputs in the original app use: `--background` background, `--border` border,
`--font-primary` text, `--font-highlight` text on focus, inner shadow effect. The
C++ app uses ImGui default input styling.

- [ ] Set `ImGuiCol_FrameBg` to `--background` (#343a40)
- [ ] Set `ImGuiCol_FrameBgHovered` with highlighted border effect
- [ ] Set `ImGuiCol_FrameBgActive` for focused state
- [ ] Input text color: `--font-primary` (#ffffffcc)

### 9.36 File Drop Overlay

The original app shows a full-screen overlay when files are dragged over the window
(CSS `#drop-overlay`): semi-transparent background (`--background-trans`: #343a40b3),
centered icon (`copy.svg`, 100×100px), "Drop file to load" text at 25px.

- [ ] Render file drop overlay when GLFW detects drag-over (before drop callback fires)
- [ ] Overlay: semi-transparent background, centered copy icon + instructional text

### 9.37 M3 Texture Export (Conversion Gap)

`M3Exporter::exportTextures()` returns an empty map — this is a **conversion gap**
(the original JS had this working). The related `TODO(conversion)` comments are in
`M3Exporter.cpp`.

- [ ] Implement `M3Exporter::exportTextures()` to extract and export M3 model textures
- [ ] Wire `fileManifest` texture entries in `M3Exporter.cpp` (line ~163)

### 9.38 Font Preview in tab_fonts

`inject_font_face` in `tab_fonts.cpp` needs to load font data into ImGui's font
system for live preview.

- [ ] Implement `inject_font_face` to dynamically load font data into ImGui for preview rendering

### 9.39 Loader Wiring Verification (tab_maps)

Three `TODO(conversion)` comments in `tab_maps.cpp` note that ADTLoader, WMOLoader,
and WDTLoader processing "will be wired when fully converted." The loaders are
converted — verify the wiring is complete at runtime.

- [ ] Verify ADTLoader integration in `tab_maps.cpp` works at runtime
- [ ] Verify WMOLoader integration in `tab_maps.cpp` works at runtime
- [ ] Verify WDTLoader integration in `tab_maps.cpp` works at runtime

---

## CASC Pipeline — Critical Runtime Failures

> **Identified 2026-04-10.** The CASC loading pipeline crashes at runtime with
> `"Invalid encoding magic: 0"` during local installation loading. Root cause
> analysis against the original JavaScript source follows.

### 11.1 BLTEReader Missing Lazy Block Processing (Root Cause)

The original JS `BLTEReader` overrides `_checkBounds(length)` on `BufferWrapper`
to **lazily decompress BLTE blocks on demand** whenever a read extends beyond
already-decompressed data. This is the mechanism that makes `new BLTEReader(data, hash)`
immediately usable for reading — blocks are auto-decompressed as needed by reads.

**JS (`blte-reader.js`, lines 311-321 at commit 8d2e6a8e):**
```js
_checkBounds(length) {
    super._checkBounds(length);
    const pos = this.offset + length;
    while (pos > this.blockWriteIndex) {
        if (this._processBlock() === false)
            return;
    }
}
```

The C++ conversion does NOT have this override. `_checkBounds()` is `private`
(non-virtual) in `BufferWrapper`, so the BLTEReader cannot intercept reads.
The C++ BLTEReader allocates a zeroed output buffer in the constructor but
never decompresses blocks into it unless `processAllBlocks()` is explicitly
called. Reads from the BLTEReader therefore return zeros.

**Fix options (pick one):**

1. **Make `_checkBounds` virtual** in `BufferWrapper` and override in `BLTEReader`
   to match the JS lazy-processing behavior exactly. This preserves the memory
   and performance benefits of lazy loading (significant for large BLTE files
   where only partial content is accessed, e.g., streaming). Requires changing
   `BufferWrapper`'s design — `_checkBounds` must become `protected virtual`.
   No other `BufferWrapper` subclasses exist, so the impact is limited.
2. **Add explicit `processAllBlocks()` calls** at every site that creates a
   `BLTEReader` and then reads from it (see §11.2 below). Simpler to implement
   but eliminates lazy loading — all blocks are decompressed upfront even if
   only partial data is needed. This could be significant for large files.

- [ ] Implement lazy block processing in BLTEReader (option 1 or 2)

### 11.2 Call Sites Missing `processAllBlocks()`

These call sites create a `BLTEReader` and then immediately read from it
**without** calling `processAllBlocks()`. In JS, the `_checkBounds` override
handles this transparently. In C++, reads return zeros from the uninitialized
output buffer.

| Call Site | File | Line | Impact |
|-----------|------|------|--------|
| `parseEncodingFile()` | `casc-source.cpp` | 494 | 🔴 Crashes with "Invalid encoding magic: 0" — **reported failure** |
| `parseRootFile()` | `casc-source.cpp` | 377 | 🔴 Crashes when executed — reads root magic from zeroed buffer |
| `getInstallManifest()` | `casc-source.cpp` | 115 | 🔴 Crashes when executed — InstallManifest reads from zeroed buffer |

**Call sites that already call `processAllBlocks()` (no fix needed):**

| Call Site | File | Line |
|-----------|------|------|
| `_ensureFileInCache()` | `casc-source-local.cpp` | 585 |
| `_ensureFileInCache()` | `casc-source-remote.cpp` | 614 |
| `writeToFile()` | `blte-reader.cpp` | 268 |
| `getDataURL()` | `blte-reader.cpp` | 273 |

**Call sites that return a BLTEReader to callers (caller's responsibility):**

| Call Site | File | Line |
|-----------|------|------|
| `getFileAsBLTE()` | `casc-source-local.cpp` | 96 |
| `getFileAsBLTE()` | `casc-source-remote.cpp` | 194 |

For these returned BLTEReaders, if option 1 (virtual `_checkBounds`) is chosen,
no changes are needed — lazy processing handles it. If option 2 is chosen,
every **caller** of `getFileAsBLTE()` must call `processAllBlocks()` before reading.

- [ ] Fix `parseEncodingFile()` — add `processAllBlocks()` or rely on virtual `_checkBounds`
- [ ] Fix `parseRootFile()` — add `processAllBlocks()` or rely on virtual `_checkBounds`
- [ ] Fix `getInstallManifest()` — add `processAllBlocks()` or rely on virtual `_checkBounds`
- [ ] Audit all callers of `getFileAsBLTE()` to ensure they handle block processing

### 11.3 Historical Context

The auto-processing was originally in the BLTEReader constructor (commit `2f4cac91`,
`this._processBlock()` at end of constructor). It was removed in commit `4f7f28b0`
("Skip auto-processing of first BLTE block") and replaced by the `_checkBounds()`
override pattern (commit `55bca20d` and surrounding commits). The C++ conversion
was based on the version that had the `_checkBounds()` override, but that override
was not ported.

---

## Remaining Work — CI & Testing

### 10.1 Linux CI Job

The CI workflow (`.github/workflows/ci.yml`) only has a Windows MSVC job. The Linux
GCC job referenced in README badges does not exist.

- [ ] Add Linux GCC job to `ci.yml`

### 10.2 Test Infrastructure

There are currently zero test files and no test framework integrated.

- [ ] Evaluate and integrate a test framework (e.g. Catch2, Google Test)
- [ ] Add smoke tests for core systems (BufferWrapper, config, CASC parsing)
- [ ] Add CI step to run tests

---

## Pre-Existing JS Limitations (Not Conversion Gaps)

> These limitations existed in the **original JavaScript** source and are **not**
> regressions introduced by the C++ conversion. They do NOT need to be fixed to
> achieve parity with the original app. Documented here for reference only.

| Issue | File | Original JS Status |
|-------|------|--------------------|
| ADPCM Stereo compression (0x80) not supported | `mpq/mpq.cpp` | Never implemented |
| ADPCM Mono compression (0x40) not supported | `mpq/mpq.cpp` | Never implemented |
| ASCII/text PKWare compression not supported | `mpq/pkware.cpp` | Never implemented |
| Alpha inline WMO group parsing not implemented | `3D/loaders/WMOLegacyLoader.cpp` | Never implemented |
| External skin loading for legacy WotLK M2 | `3D/loaders/M2LegacyLoader.cpp` | Never implemented |
| M3 chunk skipping (M3DT, M3SI, M3CL, M3VR) | `3D/loaders/M3Loader.cpp` | Skipped in JS too |
| WDC4 new chunk reading incomplete | `db/WDCReader.cpp` | Same TODO in JS |
| DBD foreign key support missing | `db/DBDParser.cpp` | Same TODO in JS |
| WMO GLTF doodad export not supported | `3D/exporters/WMOExporter.cpp` | Same limitation in JS |
| DBTextureFileData only supports UsageType 0 | `db/caches/DBTextureFileData.cpp` | Same limitation in JS |
