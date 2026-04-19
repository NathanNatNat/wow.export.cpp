# Copilot Instructions

## Project Overview
- Project: **wow.export.cpp** - a C++ port of the original JavaScript/NW.js application **wow.export** (https://github.com/Kruithne/wow.export).
- Convert the JavaScript code to C++ IN PLACE.
- Prefer keeping code in the corresponding `.cpp`/`.h` pair. New `.cpp` source files are allowed when a JS module cannot faithfully map to an existing file (e.g., a JS file that has no corresponding C++ file yet, or a helper module that is clearly separate). Do NOT create speculative or utility files that have no direct JS counterpart.
- The C++ entry point is app.cpp.
- The C++ Version should be 100% functionally identical to the original JavaScript code. The goal is a line-by-line port, with the same code structure and logic, but in C++.
- The C++ Version should be 100% visually identical to the original JavaScript app. Use the same colors, fonts, layout, and styling as defined in app.css and the reference screenshots in UI_REFERENCE.md.
- User-facing text (window title, crash screen, logs, context menus) should say **wow.export.cpp**, not wow.export.
- The **original JS source files** (**192** in total) are included in the repo under `src/js/` (and `src/installer/`), sitting alongside the C++ files. Always read these local JS files as the authoritative reference when converting or verifying C++ code.

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
| HTTP / HTTPS | cpp-httplib (HTTPS enabled via bundled mbedTLS — no system OpenSSL required) |
| TLS / Crypto | mbedTLS v3.6.x LTS (bundled submodule; provides HTTPS for cpp-httplib and MD hash APIs replacing hand-rolled MD5/SHA1/SHA256) |
| Compression | zlib |
| Archive I/O | minizip-ng 4.0.x (bundled submodule; ZIP read/write — C++ equivalent of JS adm-zip) |
| Image I/O | stb_image / stb_image_write, libwebp, nanosvg |
| Audio | miniaudio |
| File Dialogs | portable-file-dialogs 0.1.0 (bundled submodule; cross-platform native open/save/folder dialogs — replaces platform-specific COM/zenity code) |
| Threading | std::jthread, std::async (standard library — no external dependency) |

## Reference Sources
- The **original JavaScript/NW.js source code** (wow.export) is included **directly in this repository** under `src/js/` (and `src/installer/`). These are the authoritative JS source files — always refer to them locally when making changes or reviewing code to ensure fidelity with the original application. The upstream repo is at **https://github.com/Kruithne/wow.export** for historical reference.
- When converting or verifying a C++ file, **open the corresponding JS file from `src/js/`** (e.g., for `src/casc/casc-source-remote.cpp`, refer to `src/js/casc/casc-source-remote.js`). The local copy is the primary reference; the upstream repo at **https://github.com/Kruithne/wow.export** can also be consulted when needed.
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
| `http` / `https` | cpp-httplib (with mbedTLS for HTTPS) |
| `zlib` | zlib |
| `crypto` | mbedTLS MD API (`mbedtls/md.h` — `mbedtls_md_info_from_type`, `mbedtls_md_update`, etc.) |
| `events` (EventEmitter) | Custom callback/signal mechanism preserving identical behavior |
| `child_process` | `std::system`, platform process APIs |
| `os` | `std::filesystem`, platform APIs |
| `url` | Manual parsing or a lightweight utility |

## Fidelity Rules

### General Conversion Fidelity
- Conversions must be fully comprehensive — every function, method, constant, code path, and UI element from the JS source must be ported.
- The C++ conversion must be functionally and visually identical to the original JavaScript code. Nothing may be left as a permanent stub or silently omitted.
- Always do a thorough comparison against the original JS source when making changes or reviewing code.
- **Existing C++ code is not assumed correct.** Much of the codebase has already been converted, but previous conversions may contain errors, omissions, or deviations from the original JS source. When working on any file, always verify the existing C++ code against the original JS and fix any issues found.
- Deviations from the original JS source are strongly discouraged. Only deviate when a direct port is genuinely impossible in C++ (e.g., a JS runtime feature with no C++ equivalent). In such cases, document the deviation with a comment in the code explaining why it was necessary and how it differs from the original JS behavior.
- Things that cannot be completed immediately should be documented in TODO_TRACKER.md with a reference to the original JS source line(s) that require attention. Do not let documentation overhead block forward progress — implement what you can, then document what remains.

### Visual Fidelity
- The C++ app must closely match the original JavaScript app visually. Every color, font size, spacing, alignment, and icon should replicate the original as faithfully as Dear ImGui allows.
- Always reference [`UI_REFERENCE.md`](../UI_REFERENCE.md) and `app.css` when implementing or modifying any UI element.
- Dear ImGui styling must replicate the original HTML/CSS appearance — use custom styling, colors, and layout to match the original as closely as possible.
- If a visual element cannot be replicated exactly in Dear ImGui, document the limitation in a code comment and in `TODO_TRACKER.md`, and get as close as possible.

### TODO_TRACKER.md Format
Entries in `TODO_TRACKER.md` are **numbered sequentially** and **ordered by number** (no section headers or groupings). When adding a new entry, increment from the last existing number and append it at the end. The format is:
```
- [ ] N. [filename.cpp] Brief description
  - **JS Source**: `src/js/original-file.js` lines XX–YY
  - **Status**: Pending | In Progress | Blocked
  - **Details**: What needs to be done and why it could not be completed inline.
```
where `N` is the next sequential number after the last entry in the file. Use `- [x]` (checked) for completed/verified entries and `- [ ]` (unchecked) for pending ones.

### TODO_TRACKER.md Totals
The progress summary line at the top of `TODO_TRACKER.md` must always be kept up to date:
```
> **Progress: X/Y verified (Z%)** — ✅ = Verified, ⬜ = Pending
```
- `X` = number of `✅` entries, `Y` = total entries, `Z` = percentage (`round(X/Y * 100)`).
- **Update this line whenever you add, remove, or change the status of any entry** (e.g., marking ⬜ → ✅ or adding new entries).
