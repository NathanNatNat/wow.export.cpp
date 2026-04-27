# TODO Tracker

> **Progress: 0/16 verified (0%)** — ✅ = Verified, ⬜ = Pending

## Upstream Sync — port from wow.export JS @ d0d847f5

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

- [ ] 20. [M2RendererGL.cpp / WMORendererGL.cpp / ADTExporter.cpp / CharMaterialRenderer.cpp] Update inline shader lighting model and ADT blend-mode logic
  - **JS Source**: `src/shaders/m2.vertex.shader`, `src/shaders/m2.fragment.shader`, `src/shaders/wmo.vertex.shader`, `src/shaders/wmo.fragment.shader`, `src/shaders/adt.fragment.shader`, `src/shaders/char.fragment.shader`, `src/shaders/char.vertex.shader`
  - **Status**: Pending
  - **Details**: Upstream synced shaders (2025-04-27) contain non-trivial logic changes beyond the WebGL2 header migration. Port the relevant logic to the corresponding C++ inline GLSL 4.3 shaders: (1) Lighting model simplified across all shaders — `u_ambient_color`/`u_diffuse_color` uniforms removed, replaced with hardcoded `ambient_strength = 0.3`; light direction sign flipped from `normalize(-u_light_dir)` to `normalize(u_light_dir)`. (2) M2 vertex shader: `u_has_tex_matrix1`/`u_has_tex_matrix2` integer flag uniforms removed — tex matrices now applied unconditionally. (3) ADT fragment shader: `tex2` now samples `v_texcoord2` instead of `v_texcoord`; added per-blend-mode `final_opacity` tracking; added alpha test (`discard` if opacity < 0.904 for blend mode 1); blend mode mix arg changed from `v_color2.a` to `v_color.a` in one case; env texture emissive calculation now uses a zero-initialized local instead of tex1. Keep SSBO for bone matrices in M2 (SSBO upstream → UBO was a WebGL2 constraint; SSBO is correct for desktop OpenGL 4.3).

- [ ] 21. [config.cpp / tab_models.cpp] Add `modelsExportApplyPose` config option
  - **JS Source**: `src/default_config.jsonc`, `src/js/modules/tab_models.js`
  - **Status**: Pending
  - **Details**: Upstream added `"modelsExportApplyPose": true` to `default_config.jsonc` alongside the existing `chrExportApplyPose`. This is the config key that controls whether model exports apply the current animation pose (OBJ/STL posed export, entry 17). Add the key to the C++ config schema/defaults and wire it to the models export logic in `tab_models.cpp`.
