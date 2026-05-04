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

### [external-links.cpp / app.cpp / screen_source_select.cpp] Static external links removed
- **JS Source**: `src/js/external-links.js` lines 12–18 (STATIC_LINKS), `src/app.js` lines 457–458 (getExternalLink), footer link rendering, crash screen buttons; `src/js/modules/screen_source_select.js` (Discord toast actions)
- **Reason**: Deliberately excluded — Website, Discord, Patreon, GitHub, and Issue Tracker links point to the upstream JS project and are not relevant to the C++ port.
- **Impact**: Footer shows only the copyright notice (no clickable links). Crash screen has no "Report Issue" or "Get Help on Discord" buttons. CASC load error toasts have no "Visit Support Discord" action. The `STATIC_LINKS` map, `resolve()`, `renderLink()`, and `getExternalLink()` are removed. Only the Wowhead item link (`wowHead_viewItem`) is retained.

### [updater.cpp / STLWriter.cpp / GLTFWriter.cpp] "wow.export.cpp" instead of "wow.export"
- **JS Source**: Various — updater.js, STLWriter.js, GLTFWriter.js
- **Reason**: Per project guidelines, user-facing text says "wow.export.cpp".
- **Impact**: Intentional branding change.

---

## C++ Language / Architecture Limitations

These deviations exist because a direct port is not possible due to fundamental differences between JavaScript and C++.


### [core.cpp] `progressLoadingScreen()` does not await redraw
- **JS Source**: `src/js/core.js` lines 442–450
- **Reason**: JS uses `await generics.redraw()` to yield control to the event loop and wait for two animation frames before returning, ensuring each progress step is visible. In the C++ ImGui architecture, the main loop owns the render cycle — there is no mechanism to "await" a frame render from within a function called during frame processing. The C++ version posts progress updates to the main thread queue via `postToMainThread`, which applies them on the next frame.
- **Impact**: When multiple progress updates are called in quick succession, intermediate steps may not be individually visible — they batch together and only the last update renders. The loading bar still progresses correctly; only the intermediate text labels may flash by too quickly to read.
