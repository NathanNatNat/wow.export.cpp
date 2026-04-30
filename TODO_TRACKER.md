# TODO Tracker

> **Progress: 0/5 verified (0%)** — ✅ = Verified, ⬜ = Pending

- [ ] 141. [mpq-install.cpp] getFilesByExtension and getAllFiles add std::sort not present in JS
  - **JS Source**: `src/js/mpq/mpq-install.js` lines 87–120
  - **Status**: Pending
  - **Details**: JS `getFilesByExtension` (lines 87–97) and `getAllFiles` (lines 99–120) iterate the `listfile` Map and return results in insertion order (archive processing order). The C++ versions (lines 104–120 and 122–144) add `std::sort(results.begin(), results.end())` before returning, which changes the output to alphabetical order. This could affect callers that depend on the original archive-processing order. The underlying `std::unordered_map` already iterates in undefined order (unlike JS Map's insertion order), but the extra sort is an explicit deviation from JS semantics.
- [ ] 142. [audio-helper.cpp] onended callback does not clean up sound/decoder objects
  - **JS Source**: `src/js/ui/audio-helper.js` lines 57–67
  - **Status**: Pending
  - **Details**: JS sets `this.source = null` on natural playback end (line 62), which prevents `stop_source()` from trying to stop an already-ended source. C++ (lines 178–191) sets `is_playing = false` and `start_offset = 0` but does NOT uninit/delete the sound or decoder objects. They remain allocated until the next `play()` call triggers `stop_source()` cleanup. This is a resource deviation — after natural playback ends, sound/decoder remain allocated in C++ while JS nulls the source immediately.
- [ ] 143. [audio-helper.cpp] load() does not propagate decode failure to caller
  - **JS Source**: `src/js/ui/audio-helper.js` lines 31–35
  - **Status**: Pending
  - **Details**: JS `await this.context.decodeAudioData(array_buffer)` rejects the promise if decoding fails, propagating the error to the caller. C++ (lines 117–125) silently continues if `ma_decoder_init_memory` fails — `duration_cache` is set to 0.0 and the raw audio_data is returned. The caller has no way to know decoding failed. Audio data is stored but will fail to play later with no error feedback.
- [ ] 144. [char-texture-overlay.cpp] remove() handles missing elements differently than JS
  - **JS Source**: `src/js/ui/char-texture-overlay.js` lines 46–61
  - **Status**: Pending
  - **Details**: JS `layers.splice(layers.indexOf(canvas), 1)` — when canvas is not in the array, `indexOf` returns -1, and `splice(-1, 1)` removes the last element. C++ (lines 83–85) uses `std::find` and only erases if found — if the textureID is not in layers, nothing is removed. The C++ behavior is arguably more correct, but it is a behavioral deviation from JS: calling `remove()` with a non-existent element silently removes the last layer in JS but does nothing in C++.
- [ ] 145. [char-texture-overlay.cpp] ensureActiveLayerAttached() has different semantics than JS DOM re-attachment
  - **JS Source**: `src/js/ui/char-texture-overlay.js` lines 63–71
  - **Status**: Pending
  - **Details**: JS checks if `active_layer.parentNode !== element` (DOM detachment) and re-appends the active layer to the overlay DOM element. C++ (lines 98–106) checks if `active_layer` is still in the `layers` vector; if not, resets to `layers.back()` or 0. The JS version never changes the active layer — it only ensures it's visually attached. The C++ version can change the active layer to a different one if the current one is no longer in the vector. Different semantics: JS preserves active layer identity, C++ validates membership and may substitute.
