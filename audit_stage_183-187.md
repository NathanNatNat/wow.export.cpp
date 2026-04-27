## File 183. [audio-helper.cpp]

- audio-helper.cpp: `set_volume` is a no-op before `init()` and does not persist the value, but the JS does not persist it either — however the C++ `volume` field defaults to `1.0f` at construction, which slightly diverges from JS where there is no stored "pending volume" concept at all.
  - **JS Source**: `src/js/ui/audio-helper.js` lines 136–139
  - **Status**: Pending
  - **Details**: JS `set_volume` is guarded by `if (this.gain)` and writes directly to the gain node; there is no local storage of the volume before `init()`. The C++ early-returns before setting `this->volume` when `!engine`, which matches the JS no-op behaviour, but the constructor initialises `volume = 1.0f` and `play()` passes that pre-stored value to the new sound — meaning a volume set before `init()` is silently ignored, whereas in JS no such "pending value" field exists at all. This is a low-risk deviation but worth noting.

- audio-helper.cpp: `get_position` calculation differs from JS in how it computes elapsed time.
  - **JS Source**: `src/js/ui/audio-helper.js` lines 115–130
  - **Status**: Pending
  - **Details**: The JS computes `elapsed = context.currentTime - this.start_time` (wall-clock elapsed since `source.start()` was called), then `position = this.start_offset + elapsed`. The C++ uses `ma_sound_get_cursor_in_seconds()`, which returns the cursor relative to the seek point set in the decoder (i.e. it already accounts for `start_offset`). The C++ then adds `start_offset` again: `position = start_offset + cursor`. If `cursor` includes the decoded offset (i.e. reflects absolute playback from byte 0 of the stream), the result is correct; but if `ma_sound_get_cursor_in_seconds` returns elapsed-since-seek, the position is `start_offset` doubled. Needs verification that miniaudio returns an absolute stream cursor rather than elapsed-since-seek.

- audio-helper.cpp: `play()` `from_offset` sentinel differs from JS.
  - **JS Source**: `src/js/ui/audio-helper.js` lines 43–72
  - **Status**: Pending
  - **Details**: JS uses `if (from_offset !== undefined)` to decide whether to update `start_offset`. The C++ uses `if (from_offset >= 0.0)` as the sentinel (passing `-1.0` means "don't update offset"). This is a deliberate deviation using a sentinel value instead of `std::optional`, which is acceptable but undocumented. No functional impact for callers who pass a valid offset or the default, but callers who legitimately want to seek to 0.0 seconds must pass `0.0`, which works correctly.

## File 184. [char-texture-overlay.cpp]

- char-texture-overlay.cpp: `ensureActiveLayerAttached` deferred callback has incorrect logic.
  - **JS Source**: `src/js/ui/char-texture-overlay.js` lines 63–71
  - **Status**: Pending
  - **Details**: The JS `ensure_active_layer_attached` uses `process.nextTick` to re-attach the active canvas to the `#chr-texture-preview` DOM element if it is not already a child. It does NOT modify `active_layer`. The C++ deferred callback instead checks if `active_layer` is absent from `layers` and resets it to `layers.back()` or `0`. This is a different semantic: the C++ is doing a consistency repair, not a DOM re-attach. In the ImGui context there is no DOM to attach to, so the JS intent cannot be replicated exactly, but the current C++ logic is wrong in a different way — it would clear a valid `active_layer` if it somehow got removed from the list, rather than doing anything display-related.

## File 185. [character-appearance.cpp]

- character-appearance.cpp: `apply_customization_textures` omits `update()` call after each material `reset()`.
  - **JS Source**: `src/js/ui/character-appearance.js` lines 66–69
  - **Status**: Pending
  - **Details**: The JS calls `await chr_material.reset()` followed immediately by `await chr_material.update()` for every material in the reset loop. The C++ only calls `chr_material->reset()` with no subsequent `update()` call. The `update()` call in the JS forces the GPU texture to be refreshed after clearing — skipping it may leave stale texture data on the GPU until the next explicit `upload_textures_to_gpu` call.

- character-appearance.cpp: `apply_customization_textures` passes incomplete `chr_model_texture_layer` struct fields to `setTextureTarget`.
  - **JS Source**: `src/js/ui/character-appearance.js` lines 96–108
  - **Status**: Pending
  - **Details**: The JS `setTextureTarget` for the baked NPC case is called with the explicit object `{ BlendMode: 0, TextureType: texture_type, ChrModelTextureTargetID: [0, 0] }` as the fourth argument (the layer descriptor). The C++ passes `{ 0 }` — a single-element initialiser — which likely only sets `BlendMode` to 0 and leaves `TextureType` and `ChrModelTextureTargetID` default-initialised. If `setTextureTarget` uses `TextureType` from the layer argument, this will be wrong.

- character-appearance.cpp: `apply_customization_textures` skips entries where `chr_cust_mat->FileDataID` has no value, which has no JS equivalent guard.
  - **JS Source**: `src/js/ui/character-appearance.js` lines 123–130
  - **Status**: Pending
  - **Details**: The C++ checks `if (!chr_cust_mat->FileDataID.has_value()) continue;` before accessing `ChrModelTextureTargetID`. The JS accesses `chr_cust_mat.ChrModelTextureTargetID` directly without checking `FileDataID`. If `FileDataID` is optional in the C++ struct but always present in practice, this may silently skip valid entries that the JS would process.

- character-appearance.cpp: The `setTextureTarget` call for customization textures is missing the `BlendMode` from `chr_model_texture_layer`.
  - **JS Source**: `src/js/ui/character-appearance.js` lines 164
  - **Status**: Pending
  - **Details**: The JS calls `chr_material.setTextureTarget(chr_cust_mat, char_component_texture_section, chr_model_material, chr_model_texture_layer, true)` passing the full `chr_model_texture_layer` object (which includes `BlendMode`). The C++ passes `{ 0, 0, 0, static_cast<int>(get_field_int(*chr_model_texture_layer, "BlendMode")) }` as a simplified struct with a fixed layout. This may or may not match the expected fields in the C++ `setTextureTarget` signature — the order and meaning of the initialiser fields must be verified against `CharMaterialRenderer::setTextureTarget`.

## File 186. [data-exporter.cpp]

No findings.

## File 187. [listbox-context.cpp]

No findings.
