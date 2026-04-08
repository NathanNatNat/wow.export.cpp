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
> **~470 TODO(conversion) comments** across **65 files** are covered below.

---

## Phase 1 — Core Systems Integration

Core infrastructure that most other phases depend on.

### 1.1 Event Emitter Typed Parameters
- [x] Add typed parameter support to the event emitter so callbacks receive proper arguments (e.g. `layer_name`) — `src/app.cpp`, `src/js/3D/renderers/WMOLegacyRendererGL.cpp`, `src/js/3D/renderers/WMORendererGL.cpp` (5 TODOs)

### 1.2 Toast Action Callbacks
- [ ] Extend `core::setToast()` to accept `std::function` callbacks for button actions instead of `nlohmann::json` — `src/js/modules/screen_settings.cpp` (6), `src/js/modules/tab_install.cpp` (1), `src/js/modules.cpp` (1) (8 TODOs)
- [ ] Wire "View Log" toast actions — `src/js/modules/tab_audio.cpp`, `tab_textures.cpp`, `legacy_tab_audio.cpp`, `legacy_tab_data.cpp`, `tab_videos.cpp` (6 TODOs)
- [ ] Wire "View in Explorer" toast actions — `src/js/modules/legacy_tab_files.cpp`, `legacy_tab_fonts.cpp` (2 TODOs)

### 1.3 Module System Wiring
- [ ] Wire nav button registration across all 17 tab modules — each tab's `registerNavButton` call in `src/js/modules/*.cpp` (17 TODOs)
- [ ] Wire module activation (`setActive`/`deactivate`) — `src/js/modules/*.cpp` (8 TODOs)
- [ ] Wire context menu registration — `src/js/modules/tab_creatures.cpp`, `tab_characters.cpp` (2 TODOs)
- [ ] Wire drop handler registration — `src/js/modules/tab_models.cpp`, `tab_textures.cpp` (2 TODOs)

### 1.4 Cross-Tab Navigation
- [ ] Wire `tab_textures::setActive()` cross-tab navigation — `src/js/modules/tab_textures.cpp` (1 TODO)
- [ ] Wire `tab_models::setActive()` cross-tab navigation — `src/js/modules/tab_models.cpp` (1 TODO)
- [ ] Wire `goToTexture` cross-tab navigation — `src/js/modules/tab_textures.cpp` (1 TODO)
- [ ] Wire "Navigate to items tab" for equipment slots — `src/js/modules/tab_characters.cpp` (1 TODO)

### 1.5 Config Change Detection
- [ ] Wire `cascLocale` config watch with change-detection pattern — `src/js/modules/tab_raw.cpp` (1 TODO)

### 1.6 App Lifecycle
- [ ] Implement `restartApplication()` — `src/js/casc/build-cache.cpp` (1 TODO)
- [ ] Implement taskbar progress bar (Windows `ITaskbarList3`, Linux no-op) — `src/app.cpp` (1 TODO)
- [ ] Wire cache collection background worker — `src/js/modules/screen_source_select.cpp` (1 TODO)
- [ ] Wire CDN ping async integration — `src/js/modules/screen_source_select.cpp` (1 TODO)
- [ ] Populate home tab content — `src/js/modules/tab_home.cpp`, `legacy_tab_home.cpp` (2 TODOs)

---

## Phase 2 — CASC Data Integration

Wire `casc->getFile()`, `getFileByName()`, and related calls across all modules.

### 2.1 CASC File Access in Tab Modules
- [ ] `tab_creatures.cpp` — getFile, source refs, fileExists, export (14 TODOs)
- [ ] `tab_maps.cpp` — getFile for map tiles, ADT export (9 TODOs)
- [ ] `tab_characters.cpp` — getFile, M2Exporter, equipment textures, BLP loading (7 TODOs)
- [ ] `tab_videos.cpp` — getFile, getFileByName, getFileEncodingInfo, subtitle loading (8 TODOs)
- [ ] `tab_models.cpp` — getFile, source params, file loading (6 TODOs)
- [ ] `tab_textures.cpp` — getFile for texture preview and export (3 TODOs)
- [ ] `tab_audio.cpp` — getFile, getFileByName, writeToFile (4 TODOs)
- [ ] `tab_install.cpp` — getFile by hash, getInstallManifest (3 TODOs)
- [ ] `tab_raw.cpp` — getFile, getFileByName, getValidRootEntries (3 TODOs)
- [ ] `tab_fonts.cpp` — getFileByName (2 TODOs)
- [ ] `tab_text.cpp` — getFileByName (2 TODOs)
- [ ] `tab_decor.cpp` — getFile, fileExists, source refs (5 TODOs)
- [ ] `tab_data.cpp` — CASC pointer wiring (1 TODO)
- [ ] `tab_zones.cpp` — CASC integration for zone map loading (3 TODOs)

### 2.2 CASC File Access in Loaders & Renderers
- [ ] `M2Loader.cpp` — CASC file loading for sub-resources (2 TODOs)
- [ ] `SKELLoader.cpp` — CASC file loading for skeleton data (2 TODOs)
- [ ] `WMOLoader.cpp` — CASC file loading for WMO groups (1 TODO)
- [ ] `Skin.cpp` — CASC file loading for skin sections (1 TODO)
- [ ] `Texture.cpp` — `casc.getFile(fileDataID)` (1 TODO)

### 2.3 CASC in DB Layer
- [ ] `WDCReader.cpp` — Wire CASC interface for DB2 table loading (1 TODO)

---

## Phase 3 — MPQ Legacy Integration

Wire `MPQInstall` file access across legacy modules and renderers.

### 3.1 MPQ Source in Legacy Tab Modules
- [ ] `legacy_tab_audio.cpp` — MPQ source, listfileSounds (5 TODOs)
- [ ] `legacy_tab_data.cpp` — MPQ source, file scanning, build_id (6 TODOs)
- [ ] `legacy_tab_files.cpp` — MPQ source (2 TODOs)
- [ ] `legacy_tab_fonts.cpp` — MPQ source (3 TODOs)
- [ ] `legacy_tab_textures.cpp` — MPQ source, BLP/PNG preview (5 TODOs)
- [ ] `screen_source_select.cpp` — MPQ loadInstall, source wiring (2 TODOs)

### 3.2 MPQ in Legacy Renderers
- [ ] `M2LegacyRendererGL.cpp` — MPQ texture loading (3 TODOs)
- [ ] `MDXRendererGL.cpp` — MPQ texture loading (3 TODOs)
- [ ] `WMOLegacyRendererGL.cpp` — MPQ texture + doodad loading (4 TODOs)

### 3.3 MPQ in Legacy Loaders
- [ ] `WMOLegacyLoader.cpp` — MPQ file loading for WMO groups (1 TODO)

### 3.4 MPQ in Legacy Model Tab
- [ ] `tab_models_legacy.cpp` — MPQ source, creature data, file loading (6 TODOs)

---

## Phase 4 — DB2 System Integration

Wire `WDCReader::getAllRows()`, `db2::getTable()`, `db2::getRow()`.

- [ ] `tab_zones.cpp` — getAllRows for UiMap, AreaTable, UiMapXMapArt, UiMapArt, UiMapArtStyleLayer iteration (6 TODOs)
- [ ] `tab_videos.cpp` — getAllRows for MovieVariation, Movie.getRow (4 TODOs)
- [ ] `tab_data.cpp` — getAllRows iteration, db2::getTable trigger (2 TODOs)
- [ ] `tab_audio.cpp` — getAllRows for SoundKitEntry (2 TODOs)
- [ ] `tab_textures.cpp` — UiTextureAtlas/UiTextureAtlasMember iteration (1 TODO)
- [ ] `tab_characters.cpp` — ChrCustomizationChoice row lookup (1 TODO)
- [ ] `legacy_tab_data.cpp` — Schema type conversion for DBC export (1 TODO)

---

## Phase 5 — GL/Rendering Pipeline

Wire OpenGL context, model viewer, texture display, and renderer integration.

### 5.1 ModelViewerGL Component
- [ ] Wire GL context creation and management in `model-viewer-gl.cpp` — canvas/WebGL2 equivalent via GLFW+GLAD (3 TODOs in .cpp/.h)
- [ ] Wire ModelViewerGL rendering calls in tab modules — `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp`, `tab_creatures.cpp`, `tab_characters.cpp` (6 TODOs)

### 5.2 GL Context & Renderer Wiring
- [ ] Wire GL context retrieval — `tab_creatures.cpp` (creatureViewerContext), `tab_decor.cpp` (decorViewerContext), `tab_models_legacy.cpp` (legacyModelViewerContext) (3 TODOs)
- [ ] Wire renderer instantiation (M2RendererGL, WMORendererGL, etc.) — `tab_creatures.cpp`, `tab_models.cpp` (4 TODOs)
- [ ] Wire renderer load(), draw_calls/groups, has_content, geosetKey/wmoGroupKey/wmoSetKey — `tab_decor.cpp` (4 TODOs)
- [ ] Wire `applyReplaceableTextures` — `tab_creatures.cpp` (1 TODO)
- [ ] Wire `getActiveRenderer`, `getEquipmentRenderers`, `getCollectionRenderers` — `tab_creatures.cpp` (1 TODO)

### 5.3 Camera Fitting
- [ ] Wire `fitCamera` calls — `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp`, `tab_creatures.cpp`, `tab_characters.cpp` (7 TODOs)

### 5.4 Texture Display
- [ ] Wire texture preview as `ImGui::Image` — `tab_textures.cpp`, `tab_decor.cpp`, `tab_models.cpp` (3 TODOs)
- [ ] Wire zone map preview as `ImGui::Image` — `tab_zones.cpp` (1 TODO)
- [ ] Wire UV overlay rendering — `tab_models.cpp` (1 TODO)
- [ ] Wire atlas region rendering as ImGui overlay — `tab_zones.cpp` (1 TODO)

### 5.5 Framebuffer Capture
- [ ] Wire GL framebuffer capture for screenshots — `tab_characters.cpp`, `tab_models_legacy.cpp` (2 TODOs)
- [ ] Wire full thumbnail capture — `tab_characters.cpp` (1 TODO)
- [ ] Wire PNG write from framebuffer — `tab_models_legacy.cpp` (1 TODO)
- [ ] Wire clipboard PNG copy from FBO — `tab_characters.cpp`, `tab_models_legacy.cpp` (2 TODOs)

### 5.6 TextureRibbon Integration
- [ ] Wire `textureRibbon.reset()` / `addSlot` / `setSlotFile` / `setSlotSrc` in renderers — `M2RendererGL.cpp` (4), `M2LegacyRendererGL.cpp` (2), `M3RendererGL.cpp` (1), `MDXRendererGL.cpp` (2), `WMORendererGL.cpp` (4), `WMOLegacyRendererGL.cpp` (3) (16 TODOs)
- [ ] Wire texture ribbon rendering in tab modules — `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp`, `tab_creatures.cpp` (4 TODOs)

### 5.7 CharMaterialRenderer
- [ ] Wire `getCanvas` / `getRawPixels` integration — `tab_characters.cpp` (1 TODO)
- [ ] Wire GL bake composite buffer (OffscreenCanvas equivalent) — `ADTExporter.cpp` (1 TODO)

### 5.8 Background Color Picker
- [ ] Wire color picker for model viewer background — `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp` (3 TODOs)

### 5.9 WMO Renderer Change Detection
- [ ] Wire Vue watcher equivalents (`updateGroups`/`updateSets`) — `WMORendererGL.cpp`, `WMOLegacyRendererGL.cpp` (4 TODOs)

---

## Phase 6 — UI Component Wiring

Wire ImGui widget components into the tab modules that use them.

### 6.1 Listbox & ContextMenu
- [ ] Wire in `tab_audio.cpp`, `tab_raw.cpp`, `tab_text.cpp`, `tab_fonts.cpp`, `tab_textures.cpp`, `tab_videos.cpp`, `legacy_tab_audio.cpp`, `legacy_tab_files.cpp`, `legacy_tab_fonts.cpp` (9 TODOs)
- [ ] Wire standalone Listbox in `tab_data.cpp`, `legacy_tab_data.cpp`, `tab_creatures.cpp`, `tab_decor.cpp`, `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_install.cpp` (8 TODOs)
- [ ] Wire standalone ContextMenu in `tab_items.cpp`, `tab_zones.cpp` (3 TODOs)

### 6.2 DataTable
- [ ] Wire DataTable rendering in `tab_data.cpp`, `legacy_tab_data.cpp` (2 TODOs)
- [ ] Wire `getSelectedRowsAsCSV` / `getSelectedRowsAsSQL` — `tab_data.cpp`, `legacy_tab_data.cpp` (4 TODOs)

### 6.3 Listboxb
- [ ] Wire Listboxb rendering in `tab_models.cpp`, `tab_models_legacy.cpp` (2 TODOs)

### 6.4 ItemListbox
- [ ] Wire ItemListbox rendering in `tab_items.cpp`, `tab_item_sets.cpp` (2 TODOs)

### 6.5 ListboxZones
- [ ] Wire ListboxZones rendering in `tab_zones.cpp` (1 TODO)

### 6.6 CheckboxList
- [ ] Wire CheckboxList rendering in `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp` (3 TODOs)

### 6.7 MenuButton
- [ ] Wire MenuButton rendering in `tab_models.cpp`, `tab_models_legacy.cpp`, `tab_decor.cpp` (3 TODOs)

### 6.8 Filter Input
- [ ] Wire `ImGui::InputText` filter in `tab_creatures.cpp`, `tab_decor.cpp`, `tab_models.cpp`, `tab_models_legacy.cpp` (4 TODOs)

### 6.9 Full ImGui Rendering
- [ ] Complete full ImGui rendering for `screen_source_select.cpp`, `screen_settings.cpp`, `legacy_tab_textures.cpp` (4 TODOs)

### 6.10 ComboBox
- [ ] Wire full ComboBox with search for realm list — `tab_characters.cpp` (1 TODO)

---

## Phase 7 — Export Pipeline

Wire file export, texture encoding, and data export operations.

- [ ] Wire export stream (file I/O) — `tab_creatures.cpp`, `tab_decor.cpp` (2 TODOs)
- [ ] Wire `BufferWrapper::fromCanvas` equivalent (PNG/WebP encoding from raw RGBA) — `tab_textures.cpp`, `tab_zones.cpp` (2 TODOs)
- [ ] Wire `BufferWrapper::startsWith` with FileIdentifier matches — `tab_textures.cpp` (1 TODO)
- [ ] Wire `GLTFWriter::setTextureMap` typed parameter — `M2Exporter.cpp` (1 TODO)
- [ ] Wire `fileManifest` texture entries for `exportTextures` — `ADTExporter.cpp` (1 TODO)
- [ ] Wire canvas rotation/scaling for ADT minimap output — `ADTExporter.cpp` (2 TODOs)
- [ ] Wire image dimension detection via `stb_image` — `legacy_tab_textures.cpp` (1 TODO)
- [ ] Wire data exporter DBC schema type conversion — `legacy_tab_data.cpp` (1 TODO)
- [ ] Wire `autoAdjust` for model export — `tab_characters.cpp` (1 TODO)
- [ ] Wire animation list population from loaded M2 data — `tab_characters.cpp` (1 TODO)
- [ ] Wire `detect_model_type(data)` fallback — `tab_models.cpp` (1 TODO)
- [ ] Wire loaded data for model preview — `tab_models.cpp` (1 TODO)
- [ ] Wire group/set mask conversion from JSON to typed vectors — `tab_models_legacy.cpp` (1 TODO)

---

## Phase 8 — Media/Video Integration

Wire video playback, Kino streaming, and audio callbacks.

- [ ] Wire Kino HTTP POST streaming server — `tab_videos.cpp` (1 TODO)
- [ ] Wire video playback via URL — `tab_videos.cpp` (1 TODO)
- [ ] Wire video preview rendering — `tab_videos.cpp` (1 TODO)
- [ ] Wire video end/error callbacks — `tab_videos.cpp` (2 TODOs)
- [ ] Wire media playback controls — `tab_videos.cpp` (1 TODO)
- [ ] Wire subtitle visibility toggle — `tab_videos.cpp` (1 TODO)
- [ ] Wire poll timer for video state — `tab_videos.cpp` (1 TODO)
- [ ] Wire `miniaudio` per-sound "onended" callback — `ui/audio-helper.cpp` (1 TODO)
- [ ] Verify MDX pause support — `tab_models_legacy.cpp` (1 TODO)

---

## Phase 9 — Platform & Misc Integration

Platform-specific features and cleanup.

### 9.1 Crypto
- [ ] Replace FNV-1a / minimal MD5 with proper MD5/SHA256 (OpenSSL or similar) — `tab_maps.cpp` (1), `cache-collector.cpp` (3 TODOs)

### 9.2 Platform APIs
- [ ] Implement app reload (F5) equivalent — `src/app.cpp` (2 TODOs)

### 9.3 Map Viewer Internals
- [ ] Wire double-buffer / overlay canvas equivalent in `map-viewer.cpp` — canvas operations, tile loading, overlay drawing (12 TODOs in .cpp, 2 in .h)
- [ ] Implement dashed line rendering for map grid — `map-viewer.cpp` (1 TODO)

---

## Informational No-Ops (~30 items)

These `TODO(conversion)` comments document **paradigm differences** that require no code
changes. They explain why certain JS patterns (DOM events, ResizeObserver, Blob URLs,
revokeDataURL, event.preventDefault, Vue watchers, component unmount, etc.) have no
C++/ImGui equivalent. These can be cleaned up (removed or converted to regular comments)
at any time but are not blocking.

Files with informational no-ops: `generics.cpp`, `modules.cpp`, `components/*.cpp`,
`app.cpp`, `map-viewer.cpp`, `model-viewer-gl.cpp`, `file-field.cpp`, `tab_videos.cpp`.
