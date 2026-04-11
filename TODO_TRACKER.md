# TODO Tracker

## Audit Findings

### [constants.cpp] Missing BLENDER namespace constants
- **JS Source**: `src/js/constants.js` lines 20–61
- **Status**: Pending
- **Details**: The entire `BLENDER` namespace is missing from the C++ port. JS defines `BLENDER.DIR` (via `getBlenderBaseDir()` cross-platform function), `BLENDER.ADDON_DIR`, `BLENDER.LOCAL_DIR`, `BLENDER.ADDON_ENTRY`, and `BLENDER.MIN_VER`. The `getBlenderBaseDir()` helper function that returns the platform-specific Blender app-data directory (win32: `%APPDATA%/Blender Foundation/Blender`, linux: `~/.config/blender`) is also missing. These are needed for Blender add-on installation support.

### [constants.cpp] CONTEXT_MENU_ORDER missing 3 entries vs JS source
- **JS Source**: `src/js/constants.js` lines 201–214
- **Status**: Pending
- **Details**: The C++ `CONTEXT_MENU_ORDER` in `src/js/constants.h` lines 191–203 has 9 entries but the JS original has 12. Missing entries: `tab_blender`, `tab_changelog`, `tab_help`. Inline C++ comments say "Removed: module deleted" but the JS source still includes them. Per fidelity rules, either (a) add the entries back to match the JS source, or (b) add a deviation comment in the code explaining why these modules were intentionally removed and document it here.

### [constants.cpp] SHADER_PATH uses different directory structure than JS
- **JS Source**: `src/js/constants.js` line 43
- **Status**: Pending
- **Details**: JS sets `SHADER_PATH` to `path.join(INSTALL_PATH, 'src', 'shaders')`. C++ sets it to `s_data_dir / "shaders"` (i.e., `<install>/data/shaders`). This is a different path. If this is intentional for the C++ port's directory layout, it should have a deviation comment.

### [constants.cpp] CONFIG.DEFAULT_PATH uses different directory than JS
- **JS Source**: `src/js/constants.js` line 95
- **Status**: Pending
- **Details**: JS sets `CONFIG.DEFAULT_PATH` to `path.join(INSTALL_PATH, 'src', 'default_config.jsonc')`. C++ sets it to `s_data_dir / "default_config.jsonc"` (i.e., `<install>/data/default_config.jsonc`). The JS reads the default config from the source directory, while C++ reads from the data directory.

### [constants.cpp] RUNTIME_LOG path differs from JS
- **JS Source**: `src/js/constants.js` line 38
- **Status**: Pending
- **Details**: JS stores the runtime log at `path.join(DATA_PATH, 'runtime.log')`. C++ stores it in a separate `Logs` subdirectory: `s_log_dir / "runtime.log"` where `s_log_dir = s_install_path / "Logs"`. This is a different path structure. Should have a deviation comment if intentional.

### [buffer.cpp] Missing fromCanvas() static method
- **JS Source**: `src/js/buffer.js` lines 89–107
- **Status**: Pending
- **Details**: The JS `BufferWrapper.fromCanvas(canvas, mimeType, quality)` method creates a buffer from an HTML canvas element, with special WebP lossless support via `webp-wasm`. This is browser/NW.js-specific. In the C++ port using Dear ImGui/OpenGL, the equivalent would be reading pixels from an OpenGL framebuffer. This needs a C++ equivalent for screenshot/export functionality, or documentation of how it's handled differently.

### [buffer.cpp] Missing decodeAudio() method
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Status**: Pending
- **Details**: The JS `decodeAudio(context)` method decodes buffer data using the Web Audio API's `AudioContext.decodeAudioData()`. The C++ port uses miniaudio for audio, so this method needs a miniaudio-based equivalent or should be documented as handled elsewhere.

### [config.cpp] save() calls doSave() synchronously instead of deferred
- **JS Source**: `src/js/config.js` lines 83–91
- **Status**: Pending
- **Details**: The JS `save()` uses `setImmediate(doSave)` to defer the actual save operation to the next event loop tick. This prevents blocking the current call stack and allows multiple rapid config changes to be batched. The C++ `save()` calls `doSave()` directly and synchronously, which means each save blocks until disk I/O completes. This could cause UI stuttering if config changes happen frequently. Consider using `core::postToMainThread()` or a background thread to match the deferred semantics.

### [config.cpp] Missing Vue $watch equivalent for automatic config persistence
- **JS Source**: `src/js/config.js` line 60
- **Status**: Pending
- **Details**: The JS `load()` sets up `core.view.$watch('config', () => save(), { deep: true })` which automatically triggers a save whenever any config property changes. The C++ port has a comment noting this is replaced by explicit `save()` calls from the UI layer. This means every UI code path that modifies config must remember to call `config::save()`, which is error-prone. Consider implementing a change-detection mechanism or documenting all call sites that need to invoke save.

### [core.cpp] AppState missing `constants` field
- **JS Source**: `src/js/core.js` line 45
- **Status**: Pending
- **Details**: The JS view state includes `constants: constants` which stores a reference to the constants module. This is used in Vue templates to access constants directly from the view context (e.g., `view.constants.EXPANSIONS`). Since ImGui doesn't use templates, this may not be needed, but it should be documented as an intentional omission.

### [core.cpp] setToast() parameter type differs from JS
- **JS Source**: `src/js/core.js` line 470
- **Status**: Pending
- **Details**: The JS `setToast(toastType, message, options, ttl, closable)` takes a generic `options` object as the third parameter (which can be `null` or contain action buttons). The C++ signature in `src/js/core.h` line 589 uses `const std::vector<ToastAction>& actions` instead. Per fidelity rules, this deviation must either be reverted to accept a more generic type (e.g., `nlohmann::json` or `std::optional<nlohmann::json>`) to match the JS interface, or a deviation comment must be added to the C++ code explaining why the typed vector was chosen and how it differs from the original JS behavior. Verify all call sites are compatible with this interface change.