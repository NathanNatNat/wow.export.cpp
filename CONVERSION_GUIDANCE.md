# JS → C++ Conversion Guidance

This document describes the recommended order and principles for converting the 188 `.cpp` files (currently JavaScript with `.cpp` extensions) into true C++.

## Build System & CI

### Prerequisites

- **CMake** ≥ 3.20
- **MSVC** (Windows, via Visual Studio 2022) or **GCC** (Linux)
- **Python 3** with `jinja2` (`pip install jinja2`) — required at build time for GLAD2 OpenGL loader generation
- All library dependencies are **git submodules** under `extern/`. No vcpkg.

### Local Build (Windows MSVC)

```powershell
git submodule update --init --recursive
pip install jinja2
cmake -B build -S . -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
```

### Local Build (Linux GCC)

```bash
# Install system packages for GLFW/X11/OpenGL
sudo apt-get install -y libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

git submodule update --init --recursive
pip install jinja2
cmake -B build -S .
cmake --build build -j$(nproc)
```

### CI

CI runs on every push/PR via `.github/workflows/ci.yml`:

- **Platform**: Windows x64 (MSVC, `windows-latest`)
- **Configuration**: Debug
- Checks out all submodules, installs Python + Jinja2, then configures and builds with CMake

## Key Principles

1. **Leaf-first**: Convert files with zero local dependencies first — they can compile standalone.
2. **Build upward**: Each tier only depends on files from earlier tiers, so converted files compile incrementally without stubs.
3. **Test early**: After Tier 2, you can start testing core + config + logging end-to-end.
4. **Big files flagged**: Files over 800 lines are noted with `🔴` — budget extra time for these.
5. **Within a tier**: Order doesn't matter — convert in whatever order you prefer.
6. **UI last**: Components and modules depend on everything else, so they naturally come last.

## ⚠️ Keep the Tracker Updated

**Every time you convert a file, update `CONVERSION_TRACKER.md` immediately.** Use these status markers:

| Marker | Meaning |
|--------|---------|
| `- [ ]` | Not started |
| `- [~]` | In progress (partially converted) |
| `- [x]` | Converted and compiles |
| `- [✓]` | Converted, compiles, and tested/verified |

This is critical for coordination — the tracker is the single source of truth for what's done and what's remaining.

## 🚫 100% Completion Required

**A file MUST be 100% converted with NOTHING missing before it can be marked `[x]` (completed).**

This means:
- Every function in the original JS file has a working C++ equivalent
- Every exported constant, enum, or data structure is present
- Every edge case and error path is handled
- The C++ version compiles and behaves identically to the JS original
- No stubs, no TODOs, no placeholder implementations

**If any part of a file is incomplete, it stays at `[~]` (in progress). No exceptions.**

---

## Conversion Order (23 Tiers)

### Tier 0 — Zero-Dependency Primitives (11 files)
Pure utility/data files with NO local `require()` calls. Convert these first.

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/crc32.cpp` | 35 | _(none)_ |
| `src/js/MultiMap.cpp` | 31 | _(none)_ |
| `src/js/blob.cpp` | 309 | _(none)_ |
| `src/js/install-type.cpp` | 6 | _(none)_ |
| `src/js/xml.cpp` | 170 | _(none)_ |
| `src/js/subtitles.cpp` | 192 | _(none)_ |
| `src/js/hashing/xxhash64.cpp` | 288 | _(none)_ |
| `src/js/casc/content-flags.cpp` | 14 | _(none)_ |
| `src/js/casc/locale-flags.cpp` | 39 | _(none)_ |
| `src/js/casc/jenkins96.cpp` | 54 | _(none)_ |
| `src/js/casc/version-config.cpp` | 32 | _(none)_ |

### Tier 1 — Core Foundation (7 files)
Depend only on Tier 0 files and Node built-ins.

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/constants.cpp` | 257 | _(Node fs, path only)_ |
| `src/js/log.cpp` | 113 | constants |
| `src/js/file-writer.cpp` | 44 | _(Node fs only)_ |
| `src/js/buffer.cpp` | 1127 🔴 | crc32 |
| `src/js/png-writer.cpp` | 251 | buffer |
| `src/js/generics.cpp` | 502 | buffer, constants, log |
| `src/js/mmap.cpp` | 52 | log |

### Tier 2 — App Core & Event System (3 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/core.cpp` | 561 | generics, constants, log, file-writer, locale-flags |
| `src/js/config.cpp` | 117 | constants, generics, core, log |
| `src/js/tiled-png-writer.cpp` | 142 | buffer, png-writer |

### Tier 3 — Misc Utilities (3 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/gpu-info.cpp` | 363 | log, generics |
| `src/js/external-links.cpp` | 44 | _(Node util only)_ |
| `src/js/icon-render.cpp` | 108 | core, blp |

### Tier 4 — CASC Crypto & Low-Level (5 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/casc/cdn-config.cpp` | 50 | _(none)_ |
| `src/js/casc/install-manifest.cpp` | 68 | _(none)_ |
| `src/js/casc/salsa20.cpp` | 280 | buffer |
| `src/js/casc/tact-keys.cpp` | 136 | log, generics, constants, core |
| `src/js/casc/blp.cpp` | 508 | buffer, png-writer |

### Tier 5 — CASC Readers (3 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/casc/blte-reader.cpp` | 355 | buffer, salsa20, tact-keys |
| `src/js/casc/blte-stream-reader.cpp` | 244 | buffer, salsa20, tact-keys, blob |
| `src/js/casc/vp9-avi-demuxer.cpp` | 258 | _(none)_ |

### Tier 6 — CASC Mid-Level (5 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/casc/export-helper.cpp` | 293 | core, log, generics |
| `src/js/casc/dbd-manifest.cpp` | 84 | core, log, generics |
| `src/js/casc/realmlist.cpp` | 68 | core, log, constants, generics |
| `src/js/casc/cdn-resolver.cpp` | 219 | constants, generics, log, core, version-config |
| `src/js/casc/build-cache.cpp` | 258 | log, constants, generics, core, buffer, mmap |

### Tier 7 — DB Schema & Readers (5 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/db/CompressionType.cpp` | 7 | _(none)_ |
| `src/js/db/FieldType.cpp` | 13 | _(none)_ |
| `src/js/db/DBDParser.cpp` | 348 | _(none)_ |
| `src/js/db/WDCReader.cpp` | 909 🔴 | log, core, generics, constants, export-helper, DBDParser, FieldType, CompressionType, buffer |
| `src/js/db/DBCReader.cpp` | 426 | log, core, generics, constants, export-helper, DBDParser, FieldType, buffer, dbd-manifest |

### Tier 8 — CASC db2 + Listfile (2 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/casc/db2.cpp` | 95 | WDCReader |
| `src/js/casc/listfile.cpp` | 927 🔴 | generics, constants, core, log, buffer, export-helper, mmap, xxhash64, DBTextureFileData, DBModelFileData |

### Tier 9 — CASC High-Level Sources (3 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/casc/casc-source.cpp` | 494 | blte-reader, listfile, dbd-manifest, log, core, constants, locale-flags, content-flags, install-manifest, buffer, mmap |
| `src/js/casc/casc-source-remote.cpp` | 558 | casc-source, version-config, cdn-config, build-cache, listfile, blte-reader, blte-stream-reader, cdn-resolver, constants, generics, core, log |
| `src/js/casc/casc-source-local.cpp` | 516 | casc-source, version-config, cdn-config, buffer, build-cache, blte-reader, blte-stream-reader, listfile, core, generics, casc-source-remote, cdn-resolver, constants, log |

### Tier 10 — WoW Data Definitions (2 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/wow/ItemSlot.cpp` | 47 | _(none)_ |
| `src/js/wow/EquipmentSlots.cpp` | 184 | _(none)_ |

### Tier 11 — DB Caches (18 files)
All depend on log + db2. Can be done in any order within this tier.

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/db/caches/DBModelFileData.cpp` | 58 | log, db2 |
| `src/js/db/caches/DBTextureFileData.cpp` | 67 | log, db2 |
| `src/js/db/caches/DBComponentModelFileData.cpp` | 174 | log, db2 |
| `src/js/db/caches/DBComponentTextureFileData.cpp` | 123 | log, db2 |
| `src/js/db/caches/DBCreatures.cpp` | 99 | log, db2 |
| `src/js/db/caches/DBCreatureList.cpp` | 57 | log, db2 |
| `src/js/db/caches/DBCreatureDisplayExtra.cpp` | 63 | log, db2 |
| `src/js/db/caches/DBCreaturesLegacy.cpp` | 146 | log, DBCReader, buffer |
| `src/js/db/caches/DBDecor.cpp` | 78 | log, db2 |
| `src/js/db/caches/DBDecorCategories.cpp` | 62 | log, db2 |
| `src/js/db/caches/DBGuildTabard.cpp` | 133 | log, db2 |
| `src/js/db/caches/DBItemGeosets.cpp` | 487 | log, db2 |
| `src/js/db/caches/DBNpcEquipment.cpp` | 76 | log, db2 |
| `src/js/db/caches/DBItems.cpp` | 96 | log, db2, EquipmentSlots |
| `src/js/db/caches/DBItemDisplays.cpp` | 66 | log, db2, DBModelFileData, DBTextureFileData |
| `src/js/db/caches/DBItemModels.cpp` | 256 | log, db2, DBModelFileData, DBTextureFileData, DBComponentModelFileData |
| `src/js/db/caches/DBItemCharTextures.cpp` | 149 | log, db2, DBTextureFileData, DBComponentTextureFileData |
| `src/js/db/caches/DBCharacterCustomization.cpp` | 284 | log, db2, DBCreatures |

### Tier 12 — MPQ (Legacy Format) (7 files)
Self-contained subsystem for legacy WoW.

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/mpq/bitstream.cpp` | 61 | _(none)_ |
| `src/js/mpq/bzip2.cpp` | 843 🔴 | _(none)_ |
| `src/js/mpq/huffman.cpp` | 340 | bitstream |
| `src/js/mpq/pkware.cpp` | 204 | bitstream |
| `src/js/mpq/mpq.cpp` | 655 | pkware, huffman, bzip2 |
| `src/js/mpq/build-version.cpp` | 162 | log |
| `src/js/mpq/mpq-install.cpp` | 162 | mpq, build-version, log, core |

### Tier 13 — 3D GL Abstraction (5 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/3D/gl/GLContext.cpp` | 412 | _(none)_ |
| `src/js/3D/gl/GLTexture.cpp` | 195 | _(none)_ |
| `src/js/3D/gl/UniformBuffer.cpp` | 230 | _(none)_ |
| `src/js/3D/gl/VertexArray.cpp` | 310 | _(none)_ |
| `src/js/3D/gl/ShaderProgram.cpp` | 304 | log, Shaders (lazy) |

### Tier 14 — 3D Shaders & Data Mappings (8 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/3D/AnimMapper.cpp` | 1798 🔴 | _(none — big lookup table)_ |
| `src/js/3D/BoneMapper.cpp` | 431 | _(none)_ |
| `src/js/3D/GeosetMapper.cpp` | 86 | _(none)_ |
| `src/js/3D/WMOShaderMapper.cpp` | 94 | _(none)_ |
| `src/js/3D/ShaderMapper.cpp` | 183 | log |
| `src/js/3D/Shaders.cpp` | 154 | constants, log, ShaderProgram |
| `src/js/3D/Texture.cpp` | 43 | listfile, core |
| `src/js/3D/Skin.cpp` | 102 | listfile, core |

### Tier 15 — 3D Cameras (2 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/3D/camera/CameraControlsGL.cpp` | 429 | _(none)_ |
| `src/js/3D/camera/CharacterCameraControlsGL.cpp` | 178 | _(none)_ |

### Tier 16 — 3D Loaders (13 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/3D/loaders/LoaderGenerics.cpp` | 32 | _(none)_ |
| `src/js/3D/loaders/M2Generics.cpp` | 216 | _(none)_ |
| `src/js/3D/loaders/ANIMLoader.cpp` | 66 | _(none)_ |
| `src/js/3D/loaders/BONELoader.cpp` | 62 | _(none)_ |
| `src/js/3D/loaders/MDXLoader.cpp` | 917 🔴 | _(none)_ |
| `src/js/3D/loaders/WDTLoader.cpp` | 105 | constants |
| `src/js/3D/loaders/ADTLoader.cpp` | 557 | LoaderGenerics |
| `src/js/3D/loaders/M2LegacyLoader.cpp` | 834 🔴 | Texture |
| `src/js/3D/loaders/M3Loader.cpp` | 343 | constants, log |
| `src/js/3D/loaders/WMOLoader.cpp` | 471 | core, listfile, LoaderGenerics |
| `src/js/3D/loaders/WMOLegacyLoader.cpp` | 569 | core, listfile, LoaderGenerics, buffer |
| `src/js/3D/loaders/SKELLoader.cpp` | 456 | M2Generics, buffer, AnimMapper, log, ANIMLoader, core |
| `src/js/3D/loaders/M2Loader.cpp` | 888 🔴 | Texture, Skin, constants, M2Generics, ANIMLoader, core, buffer, AnimMapper, log |

### Tier 17 — 3D Writers (8 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/3D/writers/CSVWriter.cpp` | 85 | generics, file-writer |
| `src/js/3D/writers/JSONWriter.cpp` | 47 | generics, file-writer |
| `src/js/3D/writers/MTLWriter.cpp` | 70 | generics, file-writer, core |
| `src/js/3D/writers/OBJWriter.cpp` | 227 | constants, generics, file-writer |
| `src/js/3D/writers/SQLWriter.cpp` | 233 | generics, file-writer, FieldType |
| `src/js/3D/writers/STLWriter.cpp` | 252 | constants, generics, buffer |
| `src/js/3D/writers/GLBWriter.cpp` | 75 | buffer |
| `src/js/3D/writers/GLTFWriter.cpp` | 1506 🔴 | core, generics, export-helper, buffer, BoneMapper, AnimMapper, log, GLBWriter |

### Tier 18 — 3D Renderers (9 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/3D/renderers/GridRenderer.cpp` | 152 | ShaderProgram |
| `src/js/3D/renderers/ShadowPlaneRenderer.cpp` | 166 | ShaderProgram |
| `src/js/3D/renderers/CharMaterialRenderer.cpp` | 421 | blp, core, log, listfile, char-texture-overlay, png-writer, Shaders |
| `src/js/3D/renderers/M2LegacyRendererGL.cpp` | 1055 🔴 | core, log, blp, M2LegacyLoader, GeosetMapper, Shaders, VertexArray, GLTexture, texture-ribbon, buffer |
| `src/js/3D/renderers/MDXRendererGL.cpp` | 808 🔴 | core, log, blp, MDXLoader, Shaders, VertexArray, GLTexture, texture-ribbon, buffer |
| `src/js/3D/renderers/M2RendererGL.cpp` | 1650 🔴 | core, log, blp, M2Loader, SKELLoader, GeosetMapper, ShaderMapper, Shaders, GLContext, VertexArray, GLTexture, texture-ribbon |
| `src/js/3D/renderers/M3RendererGL.cpp` | 309 | core, M3Loader, Shaders, GLContext, VertexArray, GLTexture |
| `src/js/3D/renderers/WMOLegacyRendererGL.cpp` | 554 | core, log, blp, WMOLegacyLoader, M2LegacyRendererGL, Shaders, VertexArray, GLTexture, texture-ribbon, buffer |
| `src/js/3D/renderers/WMORendererGL.cpp` | 677 | core, log, constants, blp, Texture, WMOLoader, M2RendererGL, listfile, WMOShaderMapper, Shaders, GLContext, VertexArray, GLTexture, texture-ribbon |

### Tier 19 — 3D Exporters (7 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/3D/exporters/CharacterExporter.cpp` | 349 | log |
| `src/js/3D/exporters/M2LegacyExporter.cpp` | 407 | core, log, generics, M2LegacyLoader, export-helper, JSONWriter, OBJWriter, MTLWriter, STLWriter, blp, buffer, GeosetMapper |
| `src/js/3D/exporters/M2Exporter.cpp` | 1215 🔴 | core, log, generics, listfile, blp, M2Loader, SKELLoader, OBJWriter, MTLWriter, STLWriter, JSONWriter, GLTFWriter, GeosetMapper, export-helper, buffer, EquipmentSlots |
| `src/js/3D/exporters/M3Exporter.cpp` | 257 | core, log, generics, listfile, blp, M3Loader, SKELLoader, OBJWriter, MTLWriter, STLWriter, JSONWriter, GLTFWriter, GeosetMapper, export-helper, buffer |
| `src/js/3D/exporters/WMOLegacyExporter.cpp` | 590 | core, log, generics, WMOLegacyLoader, export-helper, JSONWriter, OBJWriter, MTLWriter, STLWriter, CSVWriter, blp, buffer, M2LegacyExporter |
| `src/js/3D/exporters/WMOExporter.cpp` | 1337 🔴 | core, log, listfile, generics, blp, WMOLoader, OBJWriter, MTLWriter, STLWriter, CSVWriter, GLTFWriter, JSONWriter, export-helper, M2Exporter, M3Exporter, constants, WMOShaderMapper |
| `src/js/3D/exporters/ADTExporter.cpp` | 1551 🔴 | core, constants, generics, listfile, log, buffer, blp, WDTLoader, ADTLoader, Shaders, OBJWriter, MTLWriter, png-writer, db2, export-helper, M2Exporter, WMOExporter, CSVWriter |

### Tier 20 — UI Helpers (9 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/ui/audio-helper.cpp` | 178 | _(none)_ |
| `src/js/ui/uv-drawer.cpp` | 58 | _(none)_ |
| `src/js/ui/char-texture-overlay.cpp` | 120 | core |
| `src/js/ui/texture-ribbon.cpp` | 109 | core, listfile |
| `src/js/ui/listbox-context.cpp` | 177 | core, listfile, export-helper |
| `src/js/ui/data-exporter.cpp` | 255 | core, log, generics, export-helper, CSVWriter, SQLWriter |
| `src/js/ui/texture-exporter.cpp` | 194 | core, log, generics, listfile, blp, buffer, export-helper, JSONWriter |
| `src/js/ui/character-appearance.cpp` | 205 | CharMaterialRenderer, DBCharacterCustomization |
| `src/js/ui/model-viewer-utils.cpp` | 549 | log, buffer, export-helper, listfile, constants, blte-reader, blp, M2RendererGL, M3RendererGL, M2Exporter, M3Exporter, WMORendererGL, WMOExporter, texture-ribbon, uv-drawer, AnimMapper |

### Tier 21 — Vue Components (16 files)

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/components/checkboxlist.cpp` | 176 | _(none)_ |
| `src/js/components/combobox.cpp` | 94 | _(none)_ |
| `src/js/components/context-menu.cpp` | 58 | _(none)_ |
| `src/js/components/data-table.cpp` | 1020 🔴 | _(none)_ |
| `src/js/components/file-field.cpp` | 46 | _(none)_ |
| `src/js/components/listboxb.cpp` | 284 | _(none)_ |
| `src/js/components/markdown-content.cpp` | 255 | _(none)_ |
| `src/js/components/menu-button.cpp` | 81 | _(none)_ |
| `src/js/components/resize-layer.cpp` | 25 | _(none)_ |
| `src/js/components/slider.cpp` | 98 | _(none)_ |
| `src/js/components/listbox.cpp` | 516 | core |
| `src/js/components/listbox-maps.cpp` | 95 | listbox |
| `src/js/components/listbox-zones.cpp` | 95 | listbox |
| `src/js/components/itemlistbox.cpp` | 342 | icon-render |
| `src/js/components/map-viewer.cpp` | 1113 🔴 | core, constants |
| `src/js/components/model-viewer-gl.cpp` | 516 | core, GLContext, CameraControlsGL, CharacterCameraControlsGL, GridRenderer, ShadowPlaneRenderer, EquipmentSlots |

### Tier 22 — App Modules/Tabs (31 files)
The main application screens. These depend on nearly everything above.

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/modules/tab_home.cpp` | 30 | _(none)_ |
| `src/js/modules/legacy_tab_home.cpp` | 30 | _(none)_ |
| `src/js/modules/module_test_a.cpp` | 34 | _(none)_ |
| `src/js/modules/module_test_b.cpp` | 43 | _(none)_ |
| `src/js/modules/tab_changelog.cpp` | 53 | log |
| `src/js/modules/tab_help.cpp` | 174 | log |
| `src/js/modules/font_helpers.cpp` | 139 | constants, blob |
| `src/js/modules/tab_install.cpp` | 229 | log, export-helper, generics, listfile |
| `src/js/modules/tab_raw.cpp` | 208 | log, export-helper, generics, constants, listfile, listbox-context |
| `src/js/modules/tab_text.cpp` | 145 | log, listfile, export-helper, blte-reader, generics, listbox-context, install-type |
| `src/js/modules/tab_fonts.cpp` | 168 | log, listfile, export-helper, generics, listbox-context, install-type, font_helpers |
| `src/js/modules/tab_audio.cpp` | 334 | log, generics, listfile, export-helper, blte-reader, db2, audio-helper, listbox-context, install-type |
| `src/js/modules/tab_data.cpp` | 379 | log, WDCReader, dbd-manifest, data-exporter, export-helper, install-type |
| `src/js/modules/tab_textures.cpp` | 471 | log, listfile, blp, buffer, export-helper, blte-reader, db2, texture-exporter, listbox-context, install-type |
| `src/js/modules/tab_zones.cpp` | 549 | core, log, install-type, db2, blp, buffer, export-helper |
| `src/js/modules/tab_videos.cpp` | 858 🔴 | log, export-helper, blte-reader, generics, listfile, db2, install-type, constants, core, subtitles, listbox-context, blob |
| `src/js/modules/tab_items.cpp` | 348 | log, listfile, MultiMap, external-links, DBModelFileData, DBTextureFileData, db2, ItemSlot, install-type, DBItems, EquipmentSlots |
| `src/js/modules/tab_item_sets.cpp` | 119 | log, db2, DBItems, install-type, EquipmentSlots |
| `src/js/modules/tab_decor.cpp` | 612 | log, export-helper, listfile, blte-reader, install-type, listbox-context, DBDecor, DBModelFileData, DBDecorCategories, texture-ribbon, texture-exporter, model-viewer-utils |
| `src/js/modules/tab_models.cpp` | 653 | log, export-helper, listfile, blte-reader, install-type, listbox-context, DBModelFileData, DBItemDisplays, DBCreatures, texture-ribbon, texture-exporter, model-viewer-utils, buffer |
| `src/js/modules/tab_models_legacy.cpp` | 593 | log, buffer, export-helper, install-type, listbox-context, constants, M2LegacyRendererGL, WMOLegacyRendererGL, MDXRendererGL, M2LegacyExporter, WMOLegacyExporter, texture-ribbon, AnimMapper, DBCreaturesLegacy |
| `src/js/modules/tab_maps.cpp` | 1147 🔴 | core, log, listfile, constants, install-type, db2, blp, WDTLoader, ADTExporter, ADTLoader, export-helper, WMOExporter, WMOLoader, tiled-png-writer, png-writer |
| `src/js/modules/tab_creatures.cpp` | 1374 🔴 | log, export-helper, listfile, blte-reader, install-type, listbox-context, CharMaterialRenderer, blp, M2RendererGL, M2Exporter, DBModelFileData, DBCreatures, DBCreatureList, DBCharacterCustomization, DBCreatureDisplayExtra, DBNpcEquipment, DBItemModels, DBItemGeosets |
| `src/js/modules/tab_characters.cpp` | 2704 🔴 | log, buffer, generics, CharMaterialRenderer, M2RendererGL, M2Exporter, CharacterExporter, db2, export-helper, listfile, realmlist, wmv, wowhead, install-type, char-texture-overlay, png-writer |
| `src/js/modules/legacy_tab_audio.cpp` | 318 | log, generics, export-helper, buffer, audio-helper, listbox-context, install-type |
| `src/js/modules/legacy_tab_data.cpp` | 325 | log, DBCReader, data-exporter, install-type, export-helper, buffer |
| `src/js/modules/legacy_tab_files.cpp` | 117 | log, listbox-context, install-type |
| `src/js/modules/legacy_tab_fonts.cpp` | 173 | log, listbox-context, install-type, font_helpers |
| `src/js/modules/legacy_tab_textures.cpp` | 191 | log, blp, buffer, texture-exporter, listbox-context, install-type |
| `src/js/modules/screen_settings.cpp` | 463 | generics, constants, tact-keys, tab_characters |
| `src/js/modules/screen_source_select.cpp` | 342 | constants, generics, log, external-links, install-type, casc-source-local, casc-source-remote, cdn-resolver, mpq-install |

### Tier 23 — Top-Level Glue (6 files)
Module registration, integrations, and entry point. Convert LAST.

| File | Lines | Local Dependencies |
|------|------:|-------------------|
| `src/js/wowhead.cpp` | 245 | _(none)_ |
| `src/js/wmv.cpp` | 177 | xml, EquipmentSlots |
| `src/js/updater.cpp` | 168 | constants, generics, core, log |
| `src/js/workers/cache-collector.cpp` | 431 | _(Node worker_threads, https, fs, crypto)_ |
| `src/js/modules.cpp` | 414 | log, install-type, constants, _all components_ |
| `src/app.cpp` | 713 | **Entry point — convert last** |

---

## Summary Stats

| Tier | Category | Files | Approx Lines |
|------|----------|------:|------------:|
| 0 | Zero-dep primitives | 11 | ~1,170 |
| 1 | Core foundation | 7 | ~2,346 |
| 2 | App core & event system | 3 | ~820 |
| 3 | Misc utilities | 3 | ~515 |
| 4–5 | CASC crypto & readers | 8 | ~1,899 |
| 6 | CASC mid-level | 5 | ~922 |
| 7 | DB schema & readers | 5 | ~1,703 |
| 8 | CASC db2 + listfile | 2 | ~1,022 |
| 9 | CASC high-level sources | 3 | ~1,568 |
| 10 | WoW data definitions | 2 | ~231 |
| 11 | DB caches | 18 | ~2,574 |
| 12 | MPQ legacy format | 7 | ~2,427 |
| 13–15 | 3D GL + shaders + cameras | 15 | ~5,048 |
| 16 | 3D loaders | 13 | ~6,516 |
| 17 | 3D writers | 8 | ~2,495 |
| 18 | 3D renderers | 9 | ~5,792 |
| 19 | 3D exporters | 7 | ~5,706 |
| 20 | UI helpers | 9 | ~1,845 |
| 21 | Vue components | 16 | ~4,819 |
| 22 | App modules/tabs | 31 | ~12,410 |
| 23 | Top-level glue | 6 | ~2,148 |
| **Total** | | **188** | **~63,685** |

---

## Non-C++ Assets (also need attention)

These files aren't `.cpp` but will need to be handled during or after conversion:

| File | Notes |
|------|-------|
| `src/app.css` | Main stylesheet — will need C++ UI equivalent (ImGui theming, etc.) |
| `src/default_config.jsonc` | Default config — keep as embedded resource or parse at runtime |
| `src/shaders/*.shader` (9 files) | GLSL shaders — keep as-is, load from files or embed as string literals |
| `src/fonts/` (3 files) | Font files — embed or load at runtime |
| `src/images/` | UI images/icons — embed or load at runtime |
| `src/fa-icons/` | SVG icons — embed or load at runtime |

---

## Recommended Prompts for the Conversion Process

Use these prompts when working with Copilot to do the conversion. Copy and adapt as needed.

### Starting a conversion session (beginning of a tier)

```
Convert the following Tier N files to true C++ (they are currently JavaScript with .cpp extensions).
Follow the CONVERSION_GUIDANCE.md in the repo for ordering and principles.
After converting each file, update CONVERSION_TRACKER.md to mark it [x].

Files to convert this session:
- src/js/path/file1.cpp
- src/js/path/file2.cpp
- src/js/path/file3.cpp
```

### Converting a single file

```
Convert src/js/path/filename.cpp from JavaScript to true C++.
- Read the current JS code and understand what it does
- Create a proper C++ header (.h) and implementation (.cpp)
- Replace require() with #include
- Replace module.exports with proper class/namespace exports
- Keep the same logic and behavior
- After converting, mark it [x] in CONVERSION_TRACKER.md
```

### Converting a batch of small related files (recommended for small files <100 lines)

```
Convert these related files from JavaScript to true C++. They are all in the same tier
and can be done together:
- src/js/path/file1.cpp (31 lines)
- src/js/path/file2.cpp (14 lines)
- src/js/path/file3.cpp (39 lines)

Follow CONVERSION_GUIDANCE.md. Update CONVERSION_TRACKER.md after each file.
```

### Mid-conversion check-in

```
Check the CONVERSION_TRACKER.md and tell me:
1. What tier are we currently on?
2. How many files remain in this tier?
3. What files should be converted next?
```

### After completing a tier

```
We've finished Tier N. Please:
1. Verify all Tier N files are marked [x] in CONVERSION_TRACKER.md
2. Update the progress count at the top
3. List any issues or TODOs discovered during conversion
4. Show me what's next in Tier N+1
```

### How many files per prompt?

| File size | Recommendation |
|-----------|---------------|
| **< 50 lines** | Batch 5–10 files per prompt |
| **50–200 lines** | Batch 2–4 files per prompt |
| **200–500 lines** | 1–2 files per prompt |
| **500–800 lines** | 1 file per prompt |
| **800+ lines** (🔴) | 1 file per prompt, possibly split into multiple prompts |

**General rule**: Ask for 1 file at a time for anything complex or >200 lines. Batch small/simple files together to save time. Always verify the conversion compiles before moving on.
