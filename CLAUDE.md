# CLAUDE.md

## Project Overview
- Project: **wow.export.cpp** — a C++ port of the original JavaScript/NW.js application **wow.export**.
- Convert the JavaScript code to C++ IN PLACE.
- Prefer keeping code in the corresponding `.cpp`/`.h` pair. New `.cpp` source files are allowed when a JS module cannot faithfully map to an existing file. Do NOT create speculative or utility files that have no direct JS counterpart.
- The C++ entry point is `src/app.cpp`.
- The C++ version must be **100% functionally identical** to the original JavaScript code — a line-by-line port with the same structure and logic, but in C++.
- The C++ version must be **100% visually identical** to the original JavaScript app. Use the same colors, fonts, layout, and styling as defined in `app.css` and the reference screenshots in `UI_REFERENCE.md`.
- User-facing text (window title, crash screen, logs, context menus) should say **wow.export.cpp**, not wow.export.
- The **original JS source files** (**192** in total) are included in the repo under `src/js/` (and `src/installer/`), sitting alongside the C++ files. Always read these local JS files as the authoritative reference when converting or verifying C++ code.

## Platform & Toolchain
- Platforms: **Windows x64** and **Linux x64** ONLY. No macOS.
- Compilers: MSVC (Windows), GCC (Linux).
- Build configurations: Debug, Release, RelWithDebInfo.
- Language standard: **C++23**.
- Build system: CMake with presets (`CMakePresets.json`) for Windows MSVC and Linux GCC.
- Build output: `out/build/<preset-name>/` (e.g. `out/build/windows-msvc-debug/`).
- **Build prerequisites**: Python 3 and Jinja2 (`pip install jinja2`) are required at build time for GLAD2 OpenGL loader generation. CMake finds Python automatically; only Jinja2 needs manual installation.

## Building

Use `scripts/build.ps1` (Windows) or `scripts/build.sh` (Linux). These scripts handle all environment setup and run configure + build in one step.

### Windows (MSVC x64)

Run from the Bash tool (or any shell with `pwsh` on PATH):

```bash
pwsh scripts/build.ps1                   # configure + build (debug)
pwsh scripts/build.ps1 -Clean            # delete build dir, then configure + build
pwsh scripts/build.ps1 -Config release   # release build
```

The script locates Visual Studio automatically via `vswhere.exe`, activates the MSVC x64 environment (`VsDevCmd.bat`), then runs `cmake --preset` + `cmake --build` in a single `cmd /c` chain. No manual environment setup needed.

**CMake presets used:**
- Configure: `windows-msvc-debug`, `windows-msvc-release`, `windows-msvc-relwithdebinfo`
- Build: `windows-debug`, `windows-release`, `windows-relwithdebinfo`
- Build output: `out/build/windows-msvc-<config>/`

**VS Code:** Use `Ctrl+Shift+P → Tasks: Run Build Task` — this runs the same script.

### Linux (GCC x64)

```bash
bash scripts/build.sh              # configure + build (debug)
CLEAN=1 bash scripts/build.sh      # delete build dir, then configure + build
bash scripts/build.sh release      # release build
```

**CMake presets used:**
- Configure: `linux-gcc-debug`, `linux-gcc-release`, `linux-gcc-relwithdebinfo`
- Build: `linux-debug`, `linux-release`, `linux-relwithdebinfo`
- Build output: `out/build/linux-gcc-<config>/`

The build must compile without errors before any task is considered done.

## Dependencies

Most dependencies are git submodules in `extern/`. CMake manages them automatically — do not introduce system-installed libraries.

| Purpose | Library |
|---------|---------|
| Windowing | GLFW |
| UI | Dear ImGui (docking branch; multiviewport enabled, docking disabled for now) |
| Rendering | OpenGL 4.3 core via GLAD2 (sources regenerated every build via `python -m glad` from `extern/glad/`; not pre-committed) |
| Math | GLM |
| JSON | nlohmann/json |
| Logging | spdlog |
| HTTP / HTTPS | cpp-httplib (HTTPS enabled via OpenSSL) |
| TLS / Crypto | OpenSSL (prebuilt x64 DLLs in `extern/openssl-prebuilt/`; provides HTTPS for cpp-httplib and hash APIs via `<openssl/evp.h>`) |
| Compression | zlib |
| Archive I/O | minizip-ng (ZIP read/write — C++ equivalent of JS adm-zip) |
| Image I/O | stb_image / stb_image_write, libwebp, nanosvg |
| Audio | miniaudio |
| File Dialogs | portable-file-dialogs (cross-platform native open/save/folder dialogs) |
| Threading | `std::jthread`, `std::async` (standard library) |

## Reference Sources
- **JS source files** in `src/js/` are the authoritative reference for all conversions. Always open the corresponding `.js` file when working on any `.cpp` file.
- When converting or verifying a C++ file, open the corresponding JS file (e.g. for `src/js/casc/casc-source-remote.cpp`, refer to `src/js/casc/casc-source-remote.js`).
- Upstream JS repo (historical reference only): **https://github.com/Kruithne/wow.export**
- This C++ port: **https://github.com/NathanNatNat/wow.export.cpp**
- **UI reference screenshots**: [`UI_REFERENCE.md`](UI_REFERENCE.md) — always compare against these when making UI changes.

## C++ Conventions
- **Naming & formatting** — Mirror the naming conventions and formatting of the original JS source (e.g. `camelCase` functions/variables, `PascalCase` classes). Do not invent a new style.
- **Error handling** — Use whichever C++ mechanism best preserves identical functionality to the original JS code path.
- **String types** — Use whichever string type (`std::string`, `std::string_view`, `std::wstring`, etc.) best fits the context while matching original JS behavior.
- **Memory management** — Use whichever ownership model best fits the context; no leaks.
- **Async model** — Map JS async/promise patterns to whichever C++ concurrency mechanism best preserves identical behavior.
- **Testing** — No test framework required at this time.
- **ImGui rendering** — Use native ImGui widgets (`Text`, `Button`, `Selectable`, `ProgressBar`, `BeginTable`, `BeginChild`, etc.) for all UI elements. Do **not** use raw `ImDrawList` calls (`AddText`, `AddRectFilled`, `AddLine`, `AddImage`, etc.) for anything a native widget handles. Reserve `ImDrawList` exclusively for effects with no native equivalent: image rotation, multi-colour gradient fills, and custom OpenGL overlays (3D viewports, map tiles). The `app::theme` color constants and `applyTheme()` should be progressively removed; do not reference them in new code.

### JS → C++ Idiom Mapping

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
| `typeof` / `instanceof` | `std::holds_alternative`, `dynamic_cast`, or compile-time checks |
| `try`/`catch`/`finally` | `try`/`catch` + RAII for cleanup |
| Spread operator (`...args`) | Parameter packs or `std::initializer_list` |

### Node.js API → C++ Library Mapping

| Node.js Module | C++ Replacement |
|----------------|-----------------|
| `fs` | `std::filesystem`, standard file I/O |
| `path` | `std::filesystem::path` |
| `http` / `https` | cpp-httplib (with OpenSSL for HTTPS) |
| `zlib` | zlib |
| `crypto` | OpenSSL EVP API (`<openssl/evp.h>` — `EVP_MD_CTX_new`, `EVP_DigestUpdate`, etc.) |
| `events` (EventEmitter) | Custom callback/signal mechanism preserving identical behavior |
| `child_process` | `std::system`, platform process APIs |
| `os` | `std::filesystem`, platform APIs |
| `url` | Manual parsing or a lightweight utility |

## Fidelity Rules

### Protected Files
The following files **must not be deleted, renamed, or moved** under any circumstances unless the user explicitly requests it. Do not delete them as part of "cleanup" commits, do not delete them when their contents appear obsolete, and do not delete them when their stated purpose seems "complete":
- **`audit_tracker.md`** — Historical audit log. Persistent reference, not a transient working file.
- **`DEVIATIONS.md`** — JS-to-C++ deviation log. Must be updated whenever a deviation is introduced or resolved.
- **`TODO_TRACKER.md`** — Active TODO log.
- **`UI_REFERENCE.md`** — UI layout reference.

If you believe one of these files should be removed, **ask first**. Do not act unilaterally.

### Intentional Stubs
The following files are intentionally left as no-op stubs. Do **not** add TODO entries for them. Do **not** implement them unless explicitly asked. See [`DEVIATIONS.md`](DEVIATIONS.md) entries S1–S2 for details on what the JS originals do.
- **`src/js/components/home-showcase.cpp`**
- **`src/js/modules/tab_home.cpp`**

### Removed Files
The following files have been deliberately removed from the C++ build. Do **not** re-add unless explicitly asked. See [`DEVIATIONS.md`](DEVIATIONS.md) entries R2–R4 for details and TODO entry 604 for restoration instructions.
- **`src/js/modules/tab_help.cpp`** / **`tab_help.h`**
- **`src/js/modules/tab_changelog.cpp`** / **`tab_changelog.h`**
- **`src/js/components/markdown-content.cpp`** / **`markdown-content.h`**

### Removed Features
The following features have been deliberately removed. Do **not** re-add unless explicitly asked. See [`DEVIATIONS.md`](DEVIATIONS.md) entry R1 for details.
- **"Reload Styling" context menu option**

### General Conversion Fidelity
- Conversions must be fully comprehensive — every function, method, constant, code path, and UI element from the JS source must be ported.
- The C++ conversion must be functionally and visually identical to the original JavaScript code. Nothing may be left as a permanent stub or silently omitted.
- The intentional stubs listed under **Intentional Stubs** above are exceptions to this rule.
- Always do a thorough comparison against the original JS source when making changes or reviewing code.
- **Existing C++ code is not assumed correct.** Much of the codebase has already been converted, but previous conversions may contain errors, omissions, or deviations from the original JS source. When working on any file, always verify the existing C++ code against the original JS and fix any issues found.
- Deviations from the original JS source are strongly discouraged. Only deviate when a direct port is genuinely impossible in C++. In such cases, document the deviation in [`DEVIATIONS.md`](DEVIATIONS.md) — **not** in code comments. Every deviation must have an entry in `DEVIATIONS.md` with the C++ file, JS source reference, reason, and impact.
- **No deviation comments in code.** Do not add inline comments that describe how the C++ code differs from the JS source, explain why a deviation was necessary, reference JS behaviour, or document JS bugs being fixed/replicated. All such documentation belongs exclusively in [`DEVIATIONS.md`](DEVIATIONS.md). The C++ source should read as clean code with no JS comparison commentary.
- Things that cannot be completed immediately should be documented in `TODO_TRACKER.md`. Do not let documentation overhead block forward progress — implement what you can, then document what remains.

### Visual Fidelity

**Layout and functional fidelity is required. Aesthetic fidelity is not.**

- The C++ app must be **layout-identical** to the original JS app: every panel, tab, column, control, list, filter, button, context menu, and interactive element must be present, positioned correctly, and behave identically.
- The **exact colors, shadows, gradients, fonts, and pixel-level styling** of the JS app do **not** need to be replicated. Use ImGui's default rendering and standard widget appearance throughout. The goal is a clean, natively-styled ImGui application that is easy to theme later — not a CSS replica.
- Do **not** add `PushStyleColor`/`PushStyleVar`/`ImDrawList` calls solely to match specific CSS values from `app.css`. Remove existing ones progressively as files are touched.
- [`UI_REFERENCE.md`](UI_REFERENCE.md) is useful for understanding the **layout structure** (what controls exist, in what order) but should **not** be used as a color or style target.
- If a **layout element** cannot be replicated in Dear ImGui, document the deviation in [`DEVIATIONS.md`](DEVIATIONS.md) and in `TODO_TRACKER.md`.

### TODO_TRACKER.md Format
Entries are **numbered sequentially** and **ordered by number** (no section headers). When adding a new entry, increment from the last existing number and append at the end:
```
- [ ] N. [filename.cpp] Brief description
  - **JS Source**: `src/js/original-file.js` lines XX–YY
  - **Status**: Pending | In Progress | Blocked
  - **Details**: What needs to be done and why it could not be completed inline.
```
Use `- [x]` for completed/verified entries and `- [ ]` for pending ones.

### TODO_TRACKER.md Progress Line
The progress summary at the top of `TODO_TRACKER.md` must always be kept up to date:
```
> **Progress: X/Y verified (Z%)** — ✅ = Verified, ⬜ = Pending
```
- `X` = number of `✅` entries, `Y` = total entries, `Z` = `round(X/Y * 100)`.
- **Update this line whenever you add, remove, or change the status of any entry.**
