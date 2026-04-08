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
