# Copilot Instructions

## Project Overview
- Project: wow.export — conversion from JavaScript/NW.js to Modern C++.
- The original JS files have been renamed to .cpp. Convert the JavaScript code to C++ IN PLACE.
- DO NOT create extra files except for .h header files that pair with an existing .cpp file.
- The C++ entry point is app.cpp.
- The C++ app must be styled to look EXACTLY LIKE the original app. app.css is the reference.

## Platform & Toolchain
- Platforms: Windows x64 and Linux x64 ONLY. No macOS.
- Compilers: MSVC (Windows), GCC (Linux).
- Build configurations: Debug, Release, RelWithDebInfo.
- Language standard: C++23.
- Build system: CMake with presets for Windows MSVC and Linux GCC.
- Build prerequisites: Python 3 and Jinja2 (`pip install jinja2`) are required at build time for GLAD2 OpenGL loader generation.
- CI/CD will be done later.

## Dependencies
All dependencies should be git submodules integrated via CMake where possible.

| Purpose | Library |
|---------|---------|
| Windowing | GLFW |
| UI | Dear ImGui (docking branch, multiviewport support enabled, docking disabled for now) |
| Rendering | OpenGL 4.6 core profile via GLAD2 (regenerated every build, not pre-committed) |
| Math | GLM |
| JSON | nlohmann/json |
| Logging | spdlog |
| HTTP | cpp-httplib |
| Compression | zlib |
| Image I/O | stb_image / stb_image_write, libwebp, nanosvg |
| Audio | miniaudio |
| Threading | std::jthread, std::async (standard library — no external dependency) |

## Reference Sources
- The **original JavaScript/NW.js source code** is at **https://github.com/Kruithne/wow.export** — always refer to it when making changes or reviewing code to ensure fidelity with the original application.
- **Reference screenshots** of the original app are in [`UI_REFERENCE.md`](../UI_REFERENCE.md) — always compare against these screenshots when making UI changes to ensure visual fidelity.

## Fidelity Rules

### General Conversion Fidelity
- Conversions must be fully comprehensive — every line, every function, method, constant, code path, and UI element from the JS source must be ported.
- The C++ conversion MUST be 100% identical in functionality to the original JavaScript code. Nothing may be left as a stub or omitted.
- The C++ conversion should use the same code layout as the JS original (same function order, same logical grouping, same structure) — except where the JS-to-C++ paradigm shift makes this physically impossible (see "Systemic Translations" below).
- Always do a thorough comparison against the original JS source when making changes or reviewing code.

### Conversion-Added Comments
- Any TODO, FIXME, stub, placeholder, or "not yet wired" comment **added during the conversion** (not present in the original JS) must use the `TODO(conversion):` prefix.
- This distinguishes conversion-related work items from pre-existing TODOs in the original JavaScript source.
- Pre-existing JS TODOs/comments must be preserved exactly as they are, without adding the prefix.
- Examples:
  - `// TODO(conversion): CASC file loading will be wired when UI integration is complete.`
  - `// TODO(conversion): textureRibbon is not yet converted; stubbed where referenced.`
  - `// TODO(conversion): In ImGui, this is a no-op since ImGui redraws every frame.`

### Function & Method Naming
- **Function and method names in C++ must match the original JS names exactly.** Keep the same naming as the JS source, regardless of style. Examples:
  - JS `getFileHash()` → C++ `getFileHash()`
  - JS `resetToDefault()` → C++ `resetToDefault()`
  - JS `parseBLTEHeader()` → C++ `parseBLTEHeader()`
  - JS `_compile_shader()` → C++ `_compile_shader()`
- Do NOT rename, convert to snake_case, shorten, or rephrase. The C++ name must be identical, including leading underscore prefixes.
- The only acceptable naming deviations are when the JS name uses a browser/platform-specific concept (e.g., `toCanvas()` in a browser-only context). Any naming deviation must be documented in `AUDIT_TRACKER.md`.

### Extra Functions & Methods
- Do not add extra public functions or methods to C++ classes that do not exist in the JS source.
- C++ accessor/getter methods (like `width()`, `height()`, `size()`) are acceptable when they expose data that JS accesses via public properties.
- Any utility methods, helper functions, or additional API surface not present in the JS source must be documented as ACCEPTABLE deviations in `AUDIT_TRACKER.md` with rationale.

### Deviations
- **Deviations from the JS source are only acceptable if absolutely necessary.** Do NOT make even minor deviations without first checking the JS source and documenting the rationale in `AUDIT_TRACKER.md`.
- Any deviation must be documented with ACCEPTABLE severity, explaining:
  - What the JS source does
  - What the C++ code does differently
  - Why the deviation is necessary
  - How it still meets the project's goals

## Systemic Translations (Not Deviations)

The following structural translations are inherent to the JS→C++ paradigm shift. They are NOT deviations and do NOT need individual AUDIT_TRACKER entries. Each category should be documented ONCE as a blanket entry in `AUDIT_TRACKER.md`:

### Module System
- JS `require('./foo')` / `module.exports` → C++ `#include "foo.h"` with declarations in the corresponding header file.
- Each .cpp file's `module.exports` becomes the public API in its paired .h file.
- Namespace structure should mirror the directory structure (e.g., `src/js/casc/` → `namespace casc`).

### Vue.js → Dear ImGui
- Vue component files (with `template:`, `data()`, `computed:`, `watch:`, `methods:`, `props:`, `emits:`) → ImGui immediate-mode rendering functions.
- `template:` HTML with Vue directives (`v-for`, `v-if`, `v-model`, `:class`, `@click`) → ImGui widget calls (`ImGui::Begin`, `ImGui::Text`, `ImGui::Button`, `ImGui::BeginChild`, etc.).
- `data()` reactive state → C++ struct/class member variables.
- `computed:` properties → inline calculations or cached values recalculated when dependencies change.
- `watch:` handlers → change-detection logic (compare previous vs. current value, fire callback on change).
- `methods:` → C++ member functions with identical names.
- `props:` / `emits:` → function parameters and callbacks.
- The "same code layout" rule applies to the logical ordering of methods and data within a component, NOT to Vue-specific structural elements (template/data/computed/watch blocks).

### Browser & NW.js APIs
- `nw.Shell.openItem(path)` → platform-specific "open file explorer" (`ShellExecuteW` on Windows, `xdg-open` on Linux).
- `nw.Clipboard.get().set(text, 'text')` → GLFW clipboard (`glfwSetClipboardString`).
- `nw.Window.get()` → GLFW window handle.
- `nw.App.manifest.version` → compile-time version constant.
- `document.createElement('canvas')` / WebGL2 → GLFW window + OpenGL 4.6 via GLAD2.
- `window.addEventListener('resize', ...)` → GLFW resize callback.
- `window.devicePixelRatio` → GLFW framebuffer scale.
- `process.platform` → compile-time `#ifdef _WIN32` / `#ifdef __linux__`.
- `process.execPath` / `__dirname` → C++ executable path detection.
- `fs` / `fsp` (Node.js filesystem) → C++ `<filesystem>` and `<fstream>`.
- `crypto` (Node.js) → platform crypto or a lightweight hash library.

### Binary Data
- Node.js `Buffer` / `DataView` → `std::vector<uint8_t>` for owned buffers, `std::span<const uint8_t>` for non-owning views.
- The JS `BufferWrapper` class (buffer.cpp) must be ported as a C++ class with the same `readUInt8()`, `readUInt16LE()`, `readUInt32LE()`, `readInt32LE()`, `readFloatLE()`, `readString()`, `seek()`, `tell()`, `remaining()` API. Use `std::vector<uint8_t>` as the internal storage.
- Endianness helpers: JS uses `readIntLE`/`readIntBE` style — map to explicit endian-aware reads in C++.

### Async / Concurrency
- JS runs on a single-threaded event loop with `async`/`await`. C++ must preserve this execution model for correctness:
  - **Main thread**: All UI rendering (ImGui), state mutations to `core.view` equivalent, and event dispatch. This is the "event loop" equivalent.
  - **Background threads**: Use `std::async` / `std::jthread` for I/O-bound work (CASC downloads, file parsing, cache operations).
  - **Thread safety**: Background threads must NOT directly mutate shared application state. Results from background work must be dispatched back to the main thread (e.g., via a thread-safe task queue that the main loop drains each frame).
- JS `Worker` (worker_threads) → `std::jthread` running a function, communicating via a thread-safe queue instead of `postMessage`.

### Event System
- JS `EventEmitter` (`core.events`) → C++ event dispatcher class with `on(event, callback)`, `emit(event, args...)`, `removeListener()` semantics. This is a single class used globally, not a per-file deviation.

### Other Standard Translations
- JS `Map` → `std::unordered_map` or `std::map`.
- JS `Set` → `std::unordered_set` or `std::set`.
- JS `Array` → `std::vector`.
- JS `Promise` → `std::future` / `std::async`.
- JS `class` → C++ `class`.
- JS `const` / `let` → C++ `const` / local variables.
- JS template literals → `std::format` or `fmt::format`.
- JS regex → `std::regex`.
- JS `JSON.parse` / `JSON.stringify` → `nlohmann::json`.
- JS `console.log` → `spdlog`.
- JS `setTimeout` / `setInterval` → timer logic in the main loop or `std::jthread` with sleep.

## State Management
- The JS app uses Vue's reactive state system centered on `core.view` (an object with 80+ properties defined in core.cpp).
- C++ equivalent: A central `AppState` struct (or equivalent) holding the same fields as `core.view`. Since ImGui is immediate-mode, state changes are reflected automatically on the next frame — no "reactivity" system is needed for rendering.
- For state that triggers side effects (equivalent to Vue `watch:`), implement a simple change-detection pattern: store the previous value, compare each frame or on mutation, and fire the callback when changed.
- `core.view.config` → the config module's data, loaded from JSON via nlohmann/json.
- `core.view.configEdit` → a temporary copy used during settings editing, same as JS.

## UI Styling
- The C++ ImGui app must visually match the original app. Use app.css as the definitive style reference.
- **Reference screenshots** of the original JS app are in [`UI_REFERENCE.md`](../UI_REFERENCE.md). Always compare against these screenshots when making UI changes to ensure visual fidelity.
- Map CSS variables (colors, fonts, shadows) to ImGui style settings:
  - `--background: #343a40` → `ImGuiStyle::Colors[ImGuiCol_WindowBg]`
  - `--font-primary: #ffffffcc` → `ImGuiStyle::Colors[ImGuiCol_Text]`
  - `--nav-option-selected: #22b549` → custom highlight color for selected nav items
  - etc.
- Font loading: Use ImGui's font system with the same fonts from `src/fonts/`.
- Icon rendering: Font Awesome icons (from `src/fa-icons/`) should be loaded as an ImGui icon font.

## Audit Tracker
- Always keep `AUDIT_TRACKER.md` up to date when making code changes.
- If a change resolves, modifies, or affects any audit finding, update the finding's severity and description.
- If a change introduces a new intentional deviation from the JS source, add a new finding with ACCEPTABLE severity.
- **Systemic translations** (listed above) should each be documented as ONE blanket entry, not per-function.
- **Individual deviations** (specific to one file or function) should each get their own entry.

## Work Priority Order
When presenting, suggesting, or tackling work items, always follow this priority:
**core systems → DB/data layer → rendering → UI → export**

Specifically:
1. **Core systems**: constants, config, log, generics, buffer (BufferWrapper), core (AppState/events), file-writer, updater, external-links
2. **DB/data layer**: CASC (casc-source, blte-reader, build-cache, cdn-resolver, listfile, tact-keys, etc.), DB readers (WDCReader, DBDParser, DBCReader), DB caches, MPQ
3. **Rendering**: GL context, shaders, textures, vertex arrays, UBOs, all renderers (M2, WMO, M3, MDX, ADT, grid, shadow plane, char material), cameras, loaders
4. **UI**: Components (listbox, combobox, slider, data-table, map-viewer, model-viewer, etc.), modules/tabs (home, source-select, settings, all content tabs)
5. **Export**: All exporters (M2, WMO, ADT, character, M3), all writers (OBJ, MTL, GLTF, GLB, CSV, SQL, JSON, STL)

## Commit Messages
- Describe what was done. Avoid using "Phase X", "Step Y", or similar structural labeling.
- Good: "Convert BufferWrapper to C++ with std::vector<uint8_t> backing"
- Good: "Port CASC BLTE reader with zlib decompression"
- Bad: "Phase 2 Step 3: Core systems"
