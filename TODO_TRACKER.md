# TODO Tracker

> **Progress: 0/6 verified (0%)** — ✅ = Verified, ⬜ = Pending

## Upstream Sync — port from wow.export JS @ d0d847f5

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
