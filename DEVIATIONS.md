# Deviations from JS Source

This file documents all intentional and necessary deviations from the original JavaScript source code. Every deviation from the JS source **must** be recorded here — not in code comments.

Each entry includes the C++ file, the JS source reference, a description of the deviation, and the reason it was necessary.

> **Format:** Entries are grouped by category. Each entry follows this template:
> ```
> ### [file.cpp] Short description
> - **JS Source**: `src/js/original-file.js` lines XX–YY
> - **Reason**: Why the deviation was necessary (C++ language constraint, API difference, etc.)
> - **Impact**: What observable behaviour differs from JS
> ```

---

## Intentionally Removed Files, Features, Options, etc

These files, features, and options have been deliberately removed from the C++ port with no equivalent.

### "Reload Styling" context menu option
- **JS Source**: `src/app.js` lines 160-164, registered at line 550
- **Reason**: JS hot-reloads CSS `<link>` tags with cache-busting query strings — a dev tool with no C++ equivalent since ImGui has no external stylesheets.
- **Impact**: Context menu has one fewer entry. No user-facing functionality loss (was a dev-only feature).

### tab_help module
- **JS Source**: `src/js/modules/tab_help.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Help tab is absent from the C++ app.

### tab_changelog module
- **JS Source**: `src/js/modules/tab_changelog.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Changelog tab is absent from the C++ app.

### markdown-content component
- **JS Source**: `src/js/components/markdown-content.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port. Only consumed by the removed tab_help and tab_changelog modules.
- **Impact**: No standalone impact.

### home-showcase.cpp (intentional stub)
- **JS Source**: `src/js/components/home-showcase.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Home tab shows no showcase content.

### tab_home.cpp (intentional stub)
- **JS Source**: `src/js/modules/tab_home.js`
- **Reason**: Intentionally blank — content not wanted in the C++ port.
- **Impact**: Home tab layout is empty.

### [updater.cpp] Auto-updater removed
- **JS Source**: `src/js/updater.js`
- **Reason**: Deliberately excluded — not wanted in the C++ port.
- **Impact**: Application does not check for or apply updates.

### [updater.cpp] Build-time constants instead of runtime manifest
- **JS Source**: `src/js/updater.js` lines 24-25, 33, 35
- **Reason**: JS reads `nw.App.manifest.flavour`/`guid` at runtime. C++ uses compile-time constants.
- **Impact**: Stale GUID/flavour if manifest changes without rebuild.

### [updater.cpp / STLWriter.cpp / GLTFWriter.cpp] "wow.export.cpp" instead of "wow.export"
- **JS Source**: Various — updater.js, STLWriter.js, GLTFWriter.js
- **Reason**: Per project guidelines, user-facing text says "wow.export.cpp".
- **Impact**: Intentional branding change.
