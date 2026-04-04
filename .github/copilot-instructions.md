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

## Fidelity Rules

### General Conversion Fidelity
- Conversions must be fully comprehensive — every line, every function, method, constant, code path, and UI element from the JS source must be ported.
- The C++ conversion MUST be 100% identical in functionality to the original JavaScript code. Nothing may be left as a stub or omitted.
- The C++ conversion should use the same code layout as the JS original (same function order, same logical grouping, same structure) — except where the JS-to-C++ paradigm shift makes this physically impossible (see "Systemic Translations" below).
- Always do a thorough comparison against the original JS source when making changes or reviewing code.

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
