# Copilot Instructions

## Project Overview
- Project: **wow.export.cpp** — a C++ port of the original JavaScript/NW.js application **wow.export** (https://github.com/Kruithne/wow.export).
- The original JS files have been renamed to .cpp. Convert the JavaScript code to C++ IN PLACE.
- DO NOT create extra files except for .h header files that pair with an existing .cpp file.
- The C++ entry point is app.cpp.
- The C++ Version should be 100% functionally identical to the original JavaScript code. The goal is a line-by-line port, with the same code structure and logic, but in C++.
- The C++ Version should be 100% visually identical to the original JavaScript app. Use the same colors, fonts, layout, and styling as defined in app.css and the reference screenshots in UI_REFERENCE.md.
- User-facing text (window title, crash screen, logs, context menus) should say **wow.export.cpp**, not wow.export.

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
- The **original JavaScript/NW.js source code** (wow.export) is at **https://github.com/Kruithne/wow.export** — always refer to it when making changes or reviewing code to ensure fidelity with the original application.
- This C++ port (wow.export.cpp) is at **https://github.com/NathanNatNat/wow.export.cpp**.
- **Reference screenshots** of the original app are in [`UI_REFERENCE.md`](../UI_REFERENCE.md) — always compare against these screenshots when making UI changes to ensure visual fidelity.

## C++ Conventions
- **Naming & formatting** — Mirror the naming conventions and formatting style of the original JS source as closely as C++ allows (e.g., `camelCase` functions/variables, `PascalCase` classes if the JS uses constructor functions). Do not invent a new style.
- **Error handling** — Use whichever C++ error-handling mechanism (exceptions, `std::expected`, error codes, etc.) best preserves identical functionality to the original JS code path.
- **String types** — Use whichever string types (`std::string`, `std::string_view`, `std::wstring`, etc.) best fit the context, as long as functionality is identical to the original JS behavior.
- **Memory management** — Use whichever ownership model (`std::unique_ptr`, `std::shared_ptr`, raw pointers, value types, etc.) best fits the context, as long as functionality is identical and there are no leaks.
- **Async model** — Map JS async/promise patterns to whichever C++ concurrency mechanism (`std::async`, `std::jthread`, coroutines, futures, etc.) best preserves identical behavior.
- **Include style** — Use whichever include ordering and guard style (`#pragma once`, include guards, etc.) best fits the context. Consistency within the codebase is preferred.
- **Testing** — Testing will be added later. No test framework is required at this time.

### JS → C++ Idiom Mapping
When converting JS patterns, use the following C++ equivalents (or the closest fit for the context):

| JS Idiom | C++ Equivalent |
|----------|----------------|
| `null` / `undefined` | `std::nullopt` (`std::optional<T>`), `nullptr` (pointers) |
| Closures / callbacks | Lambdas (`auto fn = [&]() { ... };`) |
| `class` / prototype chains | C++ classes |
| `Buffer` | `std::vector<uint8_t>` |
| `Map` / `Set` | `std::unordered_map` / `std::unordered_set` |
| Template literals | `std::format` (C++23) |
| `JSON.parse` / `JSON.stringify` | `nlohmann::json::parse` / `.dump()` |
| `Array` | `std::vector` |
| `typeof` / `instanceof` checks | `std::holds_alternative`, `dynamic_cast`, or compile-time checks |
| `try`/`catch`/`finally` | `try`/`catch` + RAII for cleanup |
| Spread operator (`...args`) | Parameter packs or `std::initializer_list` |

These are guidelines — use whatever produces functionally identical behavior to the original JS.

### Node.js API → C++ Library Mapping
Map Node.js built-in modules to project dependencies as follows:

| Node.js Module | C++ Replacement |
|----------------|------------------|
| `fs` | `std::filesystem`, standard file I/O |
| `path` | `std::filesystem::path` |
| `http` / `https` | cpp-httplib |
| `zlib` | zlib |
| `crypto` | Platform APIs or a lightweight library (document in TODO_TRACKER.md if needed) |
| `events` (EventEmitter) | Custom callback/signal mechanism preserving identical behavior |
| `child_process` | `std::system`, platform process APIs |
| `os` | `std::filesystem`, platform APIs |
| `url` | Manual parsing or a lightweight utility |

## Fidelity Rules

### General Conversion Fidelity
- Conversions must be fully comprehensive — every line, every function, method, constant, code path, and UI element from the JS source must be ported.
- The C++ conversion MUST be 100% identical in functionality and visual appearance to the original JavaScript code. Nothing may be left as a stub or omitted.
- Always do a thorough comparison against the original JS source when making changes or reviewing code.
- **Existing C++ code is not assumed correct.** Much of the codebase has already been converted, but previous conversions may contain errors, omissions, or deviations from the original JS source. When working on any file, always verify the existing C++ code against the original JS and fix any issues found.
- Deviations from the original JS Source are NOT ACCEPTABLE unless impossible to implement in C++. In such cases, the deviation must be documented with a comment in the code explaining why it was necessary and how it differs from the original JS behavior.
- Things that need to be done should be documented in TODO_TRACKER.md with a reference to the original JS source line(s) that require attention.

### Visual Fidelity
- The C++ app must be **pixel-perfect** compared to the original JavaScript app. Every color, font size, spacing, alignment, icon, and animation must match.
- Always reference [`UI_REFERENCE.md`](../UI_REFERENCE.md) and `app.css` when implementing or modifying any UI element.
- Dear ImGui styling must replicate the original HTML/CSS appearance — use custom styling, colors, and layout to match the original exactly.
- If a visual element cannot be replicated exactly in Dear ImGui, document the limitation in a code comment and in `TODO_TRACKER.md`, and get as close as possible.

### TODO_TRACKER.md Format
Entries in `TODO_TRACKER.md` should follow this format:
```
### [filename.cpp] Brief description
- **JS Source**: `src/js/original-file.js` lines XX–YY
- **Status**: Pending | In Progress | Blocked
- **Details**: What needs to be done and why it could not be completed inline.
```
