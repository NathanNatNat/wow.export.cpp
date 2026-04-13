# TODO Tracker

## app.cpp / app.h Audit

### 1. [app.cpp] Crash screen missing original UI elements and buttons
- **JS Source**: `src/app.js` lines 24–61, `src/index.html` `<noscript>` block
- **Status**: Verified
- **Details**: Added FLAVOUR and BUILD_GUID constants to `constants.h`. Rewrote `renderCrashScreen()` to display version/flavour/build-guid fields, four action buttons (Report Issue → issue tracker URL, Get Help on Discord → Discord URL, Copy Log to Clipboard → ImGui clipboard, Restart Application → `app::restartApplication()`), and replaced `TextUnformatted` with `InputTextMultiline` (ReadOnly) for a selectable/copyable log textarea. Heading "wow.export.cpp has crashed!" retained per naming convention.

### 2. [app.cpp] Missing global crash handlers (unhandledRejection / uncaughtException equivalents)
- **JS Source**: `src/app.js` lines 72–73
- **Status**: Verified
- **Details**: Added `std::set_terminate(terminateHandler)` to catch uncaught C++ exceptions (equivalent to `process.on('uncaughtException', ...)`), signal handlers for SIGSEGV/SIGABRT/SIGFPE/SIGILL via `fatalSignalHandler`, and on Windows `SetUnhandledExceptionFilter(unhandledSEHFilter)` for SEH exceptions. All handlers route to `crash()` with appropriate error codes. The main loop's try/catch was expanded to cover all per-frame logic (drainMainThreadQueue, checkWatchers, checkCacheSizeUpdate, DPI scaling, and rendering) instead of only the render call, and a catch-all `catch(...)` clause was added for non-std exceptions. Handlers are registered early in `main()` right after `logging::init()`.

### 3. [app.cpp] Missing `goToTexture(fileDataID)` method
- **JS Source**: `src/app.js` lines 418–434
- **Status**: Verified
- **Details**: The `goToTexture(fileDataID)` method is fully implemented in `tab_textures::goToTexture()` (src/js/modules/tab_textures.cpp lines 807–823). The function: (1) calls `modules::setActive("tab_textures")`, (2) calls `previewTextureByID(fileDataID)`, (3) clears `view->selectionTextures`, and (4) sets `userInputFilterTextures` with regex escaping when `regexFilters` is enabled. This matches the JS behavior exactly. The function is declared in `tab_textures.h` and called from `tab_models.cpp`. In the C++ port, this method belongs in the `tab_textures` namespace rather than on the Vue app, which is the correct C++ architecture.

### 4. [app.cpp] `click()` method does not check for disabled state
- **JS Source**: `src/app.js` lines 369–372
- **Status**: Verified
- **Details**: Added a `disabled` parameter (default false) to `click()` that skips the emit when true, mirroring the JS `if (!event.target.classList.contains('disabled'))` check. In ImGui, disabled state is handled by `BeginDisabled()/EndDisabled()` so the check is implicit for standard buttons, but the parameter provides API fidelity. Also added an overload `click(tag, disabled, std::any arg)` that forwards a typed argument to the event emitter, matching the JS `...params` pass-through.

### 5. [app.cpp] `emit()` method does not pass variadic parameters
- **JS Source**: `src/app.js` lines 379–381
- **Status**: Verified
- **Details**: Added an overload `emit(tag, std::any arg)` that forwards a typed argument through to `core::events.emit(tag, arg)`, matching the JS `emit(tag, ...params)` pass-through. The C++ EventEmitter supports both no-arg and single-arg emit; the `std::any` parameter covers the common case of passing a single typed value.

### 6. [app.cpp] `setAllItemTypes()` and `setAllItemQualities()` are stubbed out
- **JS Source**: `src/app.js` lines 291–303
- **Status**: Verified
- **Details**: Implemented `setAllItemTypes(bool state)` and `setAllItemQualities(bool state)` by delegating to `tab_items::setAllItemTypes()` and `tab_items::setAllItemQualities()`. The mask data (TypeMaskEntry/QualityMaskEntry structs with `.checked` bool) lives in tab_items.cpp as file-local state. The new public functions in tab_items.h/cpp iterate these entries and set `entry.checked = state`, exactly matching the JS behavior of `for (const entry of this.itemViewerTypeMask) entry.checked = state`.

### 7. [app.cpp] `getExternalLink()` returns void instead of ExternalLinks module
- **JS Source**: `src/app.js` lines 457–459
- **Status**: Verified
- **Details**: Created `src/js/external-links.h` — a header-only C++ port of the JS ExternalLinks class. Provides: `ExternalLinks::STATIC_LINKS` (map of `::WEBSITE`, `::DISCORD`, `::PATREON`, `::GITHUB`, `::ISSUE_TRACKER` to URLs), `ExternalLinks::resolve(link)` (resolves `::` prefixed identifiers to URLs), `ExternalLinks::open(link)` (resolves + opens via ShellExecuteW on Windows / xdg-open on Linux), and `ExternalLinks::wowHead_viewItem(itemID)` (opens Wowhead item page). Updated `getExternalLink()` in app.cpp to return a const reference to the STATIC_LINKS map. Callers throughout the codebase can now use `ExternalLinks::open("::DISCORD")` directly.

### 8. [app.cpp] Drag-and-drop missing `ondragenter` / `ondragleave` handlers — prompt never shown before drop
- **JS Source**: `src/app.js` lines 589–660
- **Status**: Verified
- **Details**: This is a known GLFW platform limitation. GLFW only provides a drop callback — there are no drag-enter or drag-leave callbacks. The JS uses `ondragenter` (shows file drop prompt with count via a `dropStack` counter), `ondragleave` (hides prompt, decrements `dropStack`), and `ondrop` (processes files). In the C++ port, the file drop prompt overlay cannot be shown proactively during drag-over. Implementing drag-enter/leave would require platform-specific native APIs (Win32 OLE drag-and-drop, X11/Wayland protocols), which is beyond the scope of a direct GLFW-based port. Added detailed documentation comment in app.cpp explaining the limitation. The drop handler itself (ondrop equivalent) is fully functional.

### 9. [app.cpp] Drop handler processes only the first file instead of all matching files
- **JS Source**: `src/app.js` lines 626–647
- **Status**: Verified
- **Details**: Changed `DropHandler::process` signature in core.h from `std::function<void(const std::string&)>` to `std::function<void(const std::vector<std::string>&)>`. Updated `glfw_drop_callback` to pass the entire `include` vector to `handler->process(include)` instead of just `include[0]`. Updated both registered drop handlers: tab_textures.cpp now builds a vector of JSON entries from all files and calls `texture_exporter::exportFiles()`, and tab_models.cpp does the same with `export_files()`. Also removed the incorrect error toast for unrecognized files (the JS ondrop handler silently returns false when no handler matches).

### 10. [app.cpp] Auto-updater logic is commented out
- **JS Source**: `src/app.js` lines 688–701
- **Status**: Verified
- **Details**: Replaced the commented-out auto-updater block with proper documentation. The C++ updater module (`src/js/updater.js`) has not been ported yet — it requires HTTP download, file extraction, and process spawning infrastructure. Added clear documentation comments referencing the original JS lines and the expected behavior: `if (BUILD_RELEASE && !DISABLE_AUTO_UPDATE)` → check for updates → apply or skip to source_select. The fallback behavior (skip to source_select) is already implemented. Also documented the missing whats-new.html loading. These remain as future work items requiring the updater module to be ported.

### 11. [app.cpp] What's-new HTML loading is commented out
- **JS Source**: `src/app.js` lines 707–716
- **Status**: Pending
- **Details**: The JS loads `whats-new.html` on app start and assigns it to `core.view.whatsNewHTML`. The C++ has this commented out (lines 2364–2373). The whatsNewHTML field is never populated.

### 12. [app.cpp] Missing Blender add-on version check at startup
- **JS Source**: `src/app.js` lines 699, 704
- **Status**: Pending
- **Details**: The JS calls `modules.tab_blender.checkLocalVersion()` after the update check (both when an update is not available and in debug mode). The C++ does not call any Blender add-on version check at startup.

### 13. [app.cpp] Extra 'Settings' context menu option not present in original JS
- **JS Source**: `src/app.js` lines 548–553
- **Status**: Pending
- **Details**: The C++ registers a "Settings" context menu option (line 2303) that navigates to the settings module. The original JS app.js does not register this option — it only registers: runtime-log, restart, reload-style (dev), reload-shaders (dev), reload-active (dev), and reload-all (dev). If Settings is registered by a module in the JS, it should be registered by that module in C++ too, not hardcoded in app.cpp.

### 14. [app.cpp] Hardcoded config toggles in hamburger menu not present in original JS app.js
- **JS Source**: `src/index.html` context-menu template, `src/app.js` lines 547–553
- **Status**: Pending
- **Details**: The C++ hamburger context menu (lines 518–531) hardcodes three config toggle menu items: "Show File Data IDs", "Enable Shared Textures", and "Show Unknown Files". These are not present in the JS app.js. The JS context menu iterates `modContextMenuOptions` which are registered by individual modules. These config toggles may be implemented as module-level context menu options in the original JS and should be registered through the module system rather than hardcoded in the app shell.

### 15. [app.cpp] activeModule watcher context menu clearing uses hardcoded field list
- **JS Source**: `src/app.js` lines 556–564
- **Status**: Pending
- **Details**: The JS activeModule watcher dynamically iterates ALL entries in `core.view.contextMenus` using `Object.entries()`, setting boolean `true` values to `false` and non-false values to `null`. The C++ `checkWatchers()` (lines 2032–2044) hardcodes specific field names (stateNavExtra, stateModelExport, stateCDNRegion, nodeTextureRibbon, nodeItem, nodeDataTable, nodeListbox, nodeMap, nodeZone). This means any new context menu fields added later won't be automatically cleared, deviating from the JS's dynamic approach.

### 16. [app.cpp] Help icon click does not navigate to tab_help
- **JS Source**: `src/index.html` line `<div id="nav-help" v-if="!isBusy" @click="setActiveModule('tab_help')"></div>`
- **Status**: Pending
- **Details**: In the original JS, clicking the help icon (`#nav-help`) calls `setActiveModule('tab_help')` to navigate to the help tab. The C++ help icon (lines 546–563) renders the icon and shows a tooltip on hover, but has no click handler to navigate to the help tab. `ImGui::IsItemClicked()` is never checked for the help button.

### 17. [app.cpp] Missing `data-kb-link` click handler for knowledge-base article links
- **JS Source**: `src/app.js` lines 116–131
- **Status**: Pending
- **Details**: The JS registers a document-level click handler that intercepts clicks on elements with `data-kb-link` attributes and calls `modules.tab_help.open_article(kb_id)`. The C++ has no equivalent mechanism for handling knowledge-base links embedded in the UI.

### 18. [app.cpp] Missing model override toast bar (persistent toast for filtered models/textures)
- **JS Source**: `src/index.html` lines with `v-if="!toast && activeModule && activeModule.__name === 'tab_models' && overrideModelList.length > 0"`
- **Status**: Pending
- **Details**: The original JS renders a secondary persistent toast bar when viewing the models tab with an active model override filter. It shows "Filtering models for item: {overrideModelName}" with a "Remove" action and close button. The C++ `renderAppShell()` does not render this secondary toast at all — only the primary `core::view->toast` toast is rendered.

### 19. [app.cpp] Missing taskbar progress reset at startup
- **JS Source**: `src/app.js` line 105
- **Status**: Pending
- **Details**: The JS calls `win.setProgressBar(-1)` at startup to reset any stuck taskbar progress from a previous session. The C++ does not reset the Windows taskbar progress at startup. On Windows, `initTaskbarProgress()` creates the ITaskbarList3 interface but does not explicitly clear any pre-existing progress state.

### 20. [app.cpp] Missing Vue error handler equivalent
- **JS Source**: `src/app.js` line 514
- **Status**: Pending
- **Details**: The JS sets `app.config.errorHandler = err => crash('ERR_VUE', err.message)` to catch Vue rendering errors and route them to the crash screen. The C++ has a try/catch around `active->render()` in the main content area (line 800–804) which catches `std::exception` and calls crash, but the active module render is only one of many places errors could occur. The main loop also has a try/catch (lines 2441–2451) but it only wraps `renderAppShell()`, not `checkWatchers()` or other per-frame logic.

### 21. [app.cpp] Footer links use hardcoded URLs instead of ExternalLinks module
- **JS Source**: `src/index.html` footer with `data-external="::WEBSITE"` etc., `src/app.js` lines 125–131
- **Status**: Pending
- **Details**: The JS uses named constants (`::WEBSITE`, `::DISCORD`, `::PATREON`, `::GITHUB`) resolved through the `ExternalLinks` module. The C++ footer (lines 607–612) hardcodes the URLs directly. While the URLs themselves may be correct, this doesn't match the JS architecture where URLs are centralized in the ExternalLinks module. If any URL changes, the C++ would need to update the hardcoded values rather than a single module.

### 22. [app.cpp] `handleToastOptionClick` resets toast via `core::view->toast.reset()` instead of `core::hideToast()`
- **JS Source**: `src/app.js` lines 330–335
- **Status**: Pending
- **Details**: The JS `handleToastOptionClick` sets `this.toast = null` directly on the Vue state. The C++ version (lines 308–314) calls `core::view->toast.reset()`. However, the separate `hideToast()` function (line 320–322) routes through `core::hideToast(userCancel)` which may have different behavior (e.g., emitting events, checking toast.closable). The toast action handler should directly null the toast (matching JS), which the C++ does correctly with `reset()`. But the behavior should be verified against `core::hideToast` to ensure the direct reset doesn't skip side effects that `core::hideToast` would handle.

### 23. [app.cpp] Missing `window.ondragover` prevention during drag-and-drop
- **JS Source**: `src/app.js` lines 112–113, 659–660
- **Status**: Pending
- **Details**: The JS sets `window.ondragover` and `window.ondrop` to `e => { e.preventDefault(); return false; }` early in startup to prevent default browser drag behavior, then later overrides these with proper handlers. GLFW's drag-and-drop model is inherently different (opt-in via callback), but the intent to prevent unwanted default behavior should be noted for completeness.

### 24. [app.cpp] Dynamic interface scaling uses FontGlobalScale instead of CSS transform
- **JS Source**: `src/app.js` lines 519–543
- **Status**: Pending
- **Details**: The JS scales the `#container` element using CSS `transform: scale(scale_w, scale_h)` with explicit width/height overrides when the window is smaller than 1120×700. The C++ (lines 2407–2428) uses `io.FontGlobalScale = windowScale / dpiScale` which scales text uniformly but may not scale non-text elements (custom-drawn shapes, spacing constants, icon sizes) proportionally. This could result in layout differences at small window sizes where text scales down but hardcoded pixel dimensions for headers (53px), footers (73px), nav icons (45×52px), etc. do not scale.

### 25. [app.cpp] Missing `window.on('close')` handler for clean exit
- **JS Source**: `src/app.js` lines 107–108
- **Status**: Pending
- **Details**: The JS registers `win.on('close', () => process.exit())` in release builds to ensure clean exit when the window is closed. The C++ relies on the GLFW main loop `glfwWindowShouldClose()` exiting naturally and running cleanup code, which is the standard GLFW pattern and is functionally equivalent. However, there is no `glfwSetWindowCloseCallback` to handle abrupt close scenarios or confirm exit. Low priority — the GLFW event loop pattern is idiomatic C++.

### 26. [app.cpp] `handleContextMenuClick` signature differs from JS
- **JS Source**: `src/app.js` lines 318–323
- **Status**: Pending
- **Details**: The JS `handleContextMenuClick(opt)` checks `opt.action?.handler` (optional chaining — checks if `opt.action` exists and has a `handler` property). The C++ version (lines 1380–1385) checks `opt.handler` directly. This suggests the C++ `ContextMenuOption` struct may have a different shape than the JS object. In JS, the handler is nested under `opt.action.handler`; in C++, it's directly on `opt.handler`. This could cause the handler to not be found or called incorrectly if the data structures diverge.

### 27. [app.cpp] F5 debug reload fires every frame while key is held
- **JS Source**: `src/app.js` lines 64–69
- **Status**: Pending
- **Details**: The JS registers a `window.addEventListener('keyup', ...)` handler that fires exactly once when the F5 key is released. The C++ (lines 2393–2397) polls `glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS` every frame in the main loop. This means holding F5 in the C++ port will call `app::restartApplication()` on every frame for the duration of the key press, rather than triggering a single restart on key release. This should either use a one-shot edge detection (track previous key state) or use a GLFW key callback.

### 28. [app.cpp] Drop handler shows error toast for unrecognized files; JS does nothing on drop
- **JS Source**: `src/app.js` lines 626–647
- **Status**: Pending
- **Details**: In the JS, the `ondrop` handler (line 626–647) checks for a handler and, if found, processes the matching files. If no handler is found, it simply returns false — no error is displayed to the user. The error message "That file cannot be converted." is only shown during the drag-enter phase (line 619), not on drop. The C++ `glfw_drop_callback` (line 1150) calls `core::setToast("error", "That file cannot be converted.", {}, 3000)` when no handler matches, showing an error toast on drop. This is a behavioral deviation — the JS silently ignores unrecognized drops, the C++ shows an error toast.

### 29. [app.cpp] Startup log message missing flavour and build guid fields
- **JS Source**: `src/app.js` line 569
- **Status**: Pending
- **Details**: The JS logs `'wow.export has started v%s %s [%s]'` with three fields: `manifest.version`, `manifest.flavour`, and `manifest.guid`. The C++ (line 2310) logs `"wow.export.cpp has started v{}"` with only `constants::VERSION`. The flavour and build guid fields are entirely missing from the startup diagnostic log line. These fields help identify the build configuration and should be included.

### 30. [app.cpp] Startup log path fields differ from JS (DATA_DIR/LOG_DIR instead of DATA_PATH)
- **JS Source**: `src/app.js` line 571
- **Status**: Pending
- **Details**: The JS logs `'INSTALL_PATH %s DATA_PATH %s'` with two path fields: `constants.INSTALL_PATH` and `constants.DATA_PATH`. The C++ (lines 2315–2318) logs `"INSTALL_PATH {} DATA_DIR {} LOG_DIR {}"` with three different path fields: `constants::INSTALL_PATH()`, `constants::DATA_DIR()`, and `constants::LOG_DIR()`. The field name `DATA_DIR` replaces `DATA_PATH`, and `LOG_DIR` is added as an extra. This could cause confusion when comparing logs between the JS and C++ versions for diagnostics.

### 31. [app.cpp] Dynamic interface scaling uses uniform min(w,h) instead of independent x/y scaling
- **JS Source**: `src/app.js` lines 519–543
- **Status**: Pending
- **Details**: The JS computes `scale_w` and `scale_h` independently and applies them as `transform: scale(${scale_w}, ${scale_h})` — an anisotropic (non-uniform) CSS transform. If the window is 800×900, the result is `scale(0.71, 1.0)` — only the horizontal axis shrinks. The C++ (lines 2416–2427) computes `windowScale = min(scale_w, scale_h)` and applies this single value uniformly via `io.FontGlobalScale`. With the same 800×900 window, both axes would scale to 0.71, making the UI smaller than it should be vertically. This differs from the JS's independent x/y scaling behavior. (Related to entry 24 which covers the FontGlobalScale vs CSS transform difference more broadly, but this specific uniform-vs-independent scaling is a distinct behavioral deviation.)

### 32. [app.cpp] Missing global `data-external` click handler for opening external links
- **JS Source**: `src/app.js` lines 115–131
- **Status**: Pending
- **Details**: The JS registers a document-level `click` event listener that intercepts clicks on any element with a `data-external` attribute and opens the URL via `ExternalLinks.open(...)`. This is a global mechanism used by any UI component that needs to open external links — not just the footer. The C++ port only handles external link clicks explicitly in the footer (lines 607–641), with hardcoded URLs. Any other part of the UI (e.g., crash screen report/Discord buttons, settings links, module-specific links) that would have used `data-external` attributes in the JS will not have working link-opening behavior in C++. A centralized external link system is needed to match the JS architecture.

### 33. [app.cpp] Missing texture override persistent toast bar
- **JS Source**: `src/app.js` lines 348–351, corresponding HTML template
- **Status**: Pending
- **Details**: Similar to the model override toast bar documented in entry 18, the JS app renders a persistent toast bar when `overrideTextureList.length > 0`, showing "Filtering textures for item: {overrideTextureName}" with a "Remove" action. The C++ `renderAppShell()` does not render this texture override toast bar — only the model override bar was mentioned in entry 18, and neither bar is actually rendered in the C++ port. The `removeOverrideTextures()` function exists in C++ (lines 1400–1404) but has no corresponding UI trigger in the app shell.

## src/installer/ Audit

### 34. [installer.cpp] Entire installer.js is unconverted — no installer.cpp exists
- **JS Source**: `src/installer/installer.js` lines 1–261
- **Status**: Pending
- **Details**: The `src/installer/` directory contains only the original `installer.js` (261 lines, 14 functions). No `installer.cpp` or `installer.h` file exists. The entire installer program needs to be ported to C++. The installer is a standalone console application (not part of the main GUI app) that extracts a `data.pak` archive into a platform-specific install directory and creates desktop shortcuts. All functions and logic listed in entries 35–44 below are completely missing.

### 35. [installer.cpp] Missing `wait_for_exit()` function
- **JS Source**: `src/installer/installer.js` lines 17–28
- **Status**: Pending
- **Details**: The JS `wait_for_exit()` function flushes stdout/stderr, creates a readline interface on stdin, and prompts the user with "Press ENTER to exit..." before resolving. This is used at the end of both successful and failed installations to keep the console window open so the user can read the output. No C++ equivalent exists.

### 36. [installer.cpp] Missing `get_install_path()` function
- **JS Source**: `src/installer/installer.js` lines 32–46
- **Status**: Pending
- **Details**: The JS `get_install_path()` returns platform-specific install directories: `%LOCALAPPDATA%/wow.export` on Windows, `~/.local/share/wow.export` on Linux, and `~/Library/Application Support/wow.export` on macOS. No C++ equivalent exists. Note: per project conventions, only Windows x64 and Linux x64 are targeted — the macOS case can be omitted or stubbed.

### 37. [installer.cpp] Missing `get_executable_name()` function
- **JS Source**: `src/installer/installer.js` lines 48–59
- **Status**: Pending
- **Details**: The JS `get_executable_name()` returns the platform-specific executable name: `wow.export.exe` on Windows, `wow.export` on Linux (and a macOS variant). No C++ equivalent exists. Note: per naming conventions, the C++ port should use `wow.export.cpp.exe` / `wow.export.cpp` as the executable name.

### 38. [installer.cpp] Missing `get_icon_path()` function
- **JS Source**: `src/installer/installer.js` lines 61–72
- **Status**: Pending
- **Details**: The JS `get_icon_path(install_path)` returns the platform-specific icon path used for shortcuts: `<install_path>/res/icon.png` on Windows and Linux. No C++ equivalent exists.

### 39. [installer.cpp] Missing `create_desktop_shortcut()` and platform-specific shortcut functions
- **JS Source**: `src/installer/installer.js` lines 74–151
- **Status**: Pending
- **Details**: The JS provides four shortcut-creation functions: (1) `create_desktop_shortcut(install_path)` (lines 74–90) dispatches to the platform-specific function, (2) `create_windows_shortcut(exec_path)` (lines 92–113) uses PowerShell to create a `.lnk` file on the Desktop via WScript.Shell COM, (3) `create_macos_shortcut(install_path)` (lines 115–131) creates a symlink in `/Applications`, and (4) `create_linux_shortcut(exec_path, icon_path)` (lines 133–151) writes a `.desktop` file to `~/.local/share/applications/` with appropriate metadata and chmod 755. None of these have C++ equivalents. The macOS function can be omitted per project platform requirements.

### 40. [installer.cpp] Missing `get_installer_dir()` function
- **JS Source**: `src/installer/installer.js` lines 153–155
- **Status**: Pending
- **Details**: The JS `get_installer_dir()` returns the directory containing the installer executable itself, using `path.dirname(path.resolve(process.execPath))`. No C++ equivalent exists. This is needed by `validate_data_pak()` and `extract_data_pak()` to locate the `data.pak` and `data.pak.json` files that ship alongside the installer.

### 41. [installer.cpp] Missing `validate_data_pak()` function
- **JS Source**: `src/installer/installer.js` lines 157–169
- **Status**: Pending
- **Details**: The JS `validate_data_pak()` checks that both `data.pak` and `data.pak.json` exist in the installer directory. If either is missing, it throws an error instructing the user to extract the entire archive before running the installer. No C++ equivalent exists.

### 42. [installer.cpp] Missing `extract_data_pak()` function
- **JS Source**: `src/installer/installer.js` lines 171–216
- **Status**: Pending
- **Details**: The JS `extract_data_pak(install_path)` is the core installation function. It: (1) reads and parses `data.pak.json` as a manifest, (2) reads the binary `data.pak` file, (3) iterates over all entries in `manifest.contents`, (4) for each entry, creates parent directories recursively, extracts the compressed slice from `data.pak` using offset and compSize, decompresses it with zlib inflate, and writes the decompressed file to the install path, (5) logs progress as `[N/total] relative_path`, and (6) on non-Windows platforms, sets executable permissions (chmod 755) on the main executable and the updater binary. No C++ equivalent exists. The C++ version should use zlib for decompression and `nlohmann::json` for manifest parsing, per project dependencies.

### 43. [installer.cpp] Missing main entry point and installation flow
- **JS Source**: `src/installer/installer.js` lines 218–260
- **Status**: Pending
- **Details**: The JS main IIFE (immediately invoked async function) orchestrates the full installation: (1) calls `validate_data_pak()`, (2) prints an ASCII art banner for "wow.export", (3) logs the install location, (4) creates the install directory recursively, (5) calls `extract_data_pak()`, (6) calls `create_desktop_shortcut()`, (7) prints a success banner, and (8) calls `wait_for_exit()`. On error, it prints the failure message and exits with code 1 after waiting. No C++ `main()` function exists for the installer. The ASCII art banner should say `wow.export.cpp` per naming conventions, and the success message should reference `wow.export.cpp` rather than `wow.export`.

### 44. [installer.cpp] Missing PLATFORM constant and platform detection
- **JS Source**: `src/installer/installer.js` line 30
- **Status**: Pending
- **Details**: The JS uses `const PLATFORM = process.platform` to detect the runtime platform (`'win32'`, `'linux'`, `'darwin'`), which is then used by `get_install_path()`, `get_executable_name()`, `get_icon_path()`, `create_desktop_shortcut()`, and `extract_data_pak()` for platform-specific logic. No C++ equivalent exists. The C++ version should use preprocessor macros (`_WIN32`, `__linux__`) or `std::filesystem` capabilities to determine the platform at compile time.

## src/updater Audit

### 45. [updater — no .cpp file exists] Entire standalone updater application is unconverted
- **JS Source**: `src/updater/updater.js` lines 1–197
- **Status**: Pending
- **Details**: The `src/updater/` directory contains only `updater.js` — the standalone updater helper application that is spawned by the main app to apply updates after the main process exits. There is no corresponding `.cpp` file anywhere in the repository. The entire standalone updater application needs to be ported to C++. This is a separate executable from the main app, with its own entry point. All functions and code paths described in entries 46–52 below are part of this unconverted file.

### 46. [updater — no .cpp file exists] Missing `get_timestamp()` function
- **JS Source**: `src/updater/updater.js` lines 21–29
- **Status**: Pending
- **Details**: The JS `get_timestamp()` function creates a `Date` object and formats a `HH:MM:SS` timestamp string using `util.format()` with zero-padded hours, minutes, and seconds. No C++ equivalent exists. The C++ version should use `<chrono>` and `std::format` to produce the same formatted timestamp.

### 47. [updater — no .cpp file exists] Missing `log()` function and `log_output` array
- **JS Source**: `src/updater/updater.js` lines 13, 31–35
- **Status**: Pending
- **Details**: The JS defines a module-level `log_output` array (line 13) and a `log(message, ...params)` function (lines 31–35) that: (1) formats a timestamped message using `get_timestamp()` and `util.format()`, (2) pushes it to the `log_output` array for later writing to a log file, and (3) prints it to the console via `console.log()`. No C++ equivalent exists. The C++ version should use `spdlog` and/or `std::vector<std::string>` to collect log lines for the final log file write.

### 48. [updater — no .cpp file exists] Missing `collect_files()` function
- **JS Source**: `src/updater/updater.js` lines 37–48
- **Status**: Pending
- **Details**: The JS `collect_files(dir, out)` recursively walks a directory using `fsp.readdir()` with `withFileTypes: true`, collecting all file paths (non-directories) into the `out` array. No C++ equivalent exists. The C++ version should use `std::filesystem::recursive_directory_iterator` to achieve the same recursive file collection.

### 49. [updater — no .cpp file exists] Missing `delete_directory()` function
- **JS Source**: `src/updater/updater.js` lines 50–67
- **Status**: Pending
- **Details**: The JS `delete_directory(dir)` synchronously checks if a directory exists, iterates its entries, recursively deletes files/symlinks via `fs.unlinkSync()` and subdirectories via recursion, then removes the directory itself with `fs.rmdirSync()`. No C++ equivalent exists. The C++ version should use `std::filesystem::remove_all()` for equivalent behavior.

### 50. [updater — no .cpp file exists] Missing `file_exists()` and `is_file_locked()` helper functions
- **JS Source**: `src/updater/updater.js` lines 69–85
- **Status**: Pending
- **Details**: The JS defines two async helper functions: (1) `file_exists(file)` (lines 69–76) checks if a file exists using `fsp.access(file, fs.constants.F_OK)`, returning true/false; (2) `is_file_locked(file)` (lines 78–85) checks if a file is write-locked using `fsp.access(file, fs.constants.W_OK)`, returning true if locked, false if writable. No C++ equivalents exist. The C++ version should use `std::filesystem::exists()` and platform-specific file locking checks.

### 51. [updater — no .cpp file exists] Missing `MAX_LOCK_TRIES` constant
- **JS Source**: `src/updater/updater.js` lines 18–19
- **Status**: Pending
- **Details**: The JS defines `const MAX_LOCK_TRIES = 30` which limits the number of retry attempts when waiting for a locked file to become writable during the update process. No C++ equivalent exists.

### 52. [updater — no .cpp file exists] Missing main entry point and update orchestration logic
- **JS Source**: `src/updater/updater.js` lines 87–197
- **Status**: Pending
- **Details**: The JS main IIFE (immediately invoked async function) orchestrates the full update process: (1) parses the parent PID from `process.argv` (line 93), (2) waits for the parent process to terminate by polling `process.kill(pid, 0)` with 500ms delays (lines 95–110), (3) sends an auxiliary OS-specific termination command — `taskkill /f /im wow.export.exe` on Windows, `pkill -f wow.export` on Linux/macOS (lines 117–129), (4) determines the install and update directories relative to the executable path (lines 131–132), (5) iterates all files in the `.update` directory and copies each to the install directory, with file-lock retry logic up to `MAX_LOCK_TRIES` with 1-second delays (lines 137–166), (6) creates parent directories as needed via `fsp.mkdir(…, { recursive: true })` (line 159), (7) re-launches the main application binary — `wow.export.exe` on Windows, `wow.export` on Linux — as a detached child process (lines 171–181), (8) deletes the `.update` directory after completion (line 184), and (9) in the `finally` block, writes all accumulated log lines to a timestamped log file in `./logs/` (lines 188–193) and calls `process.exit()`. No C++ equivalent exists. The C++ version should be a separate executable with its own `main()`, using `std::filesystem` for file operations, platform APIs for process management, and the naming convention `wow.export.cpp` / `wow.export.cpp.exe` for user-facing binary names and termination commands.

## src/js/ Top-Level Audit

### 53. [blob.cpp] stringEncode() missing full UTF-8 encoding logic
- **JS Source**: `src/js/blob.js` lines 42–95
- **Status**: Pending
- **Details**: The C++ `stringEncode()` (lines 57–61) is a trivial byte copy that assumes the input is already UTF-8. The JS version performs full UTF-8 encoding from a UTF-16 JavaScript string, handling surrogate pairs (0xD800–0xDBFF), multi-byte UTF-8 sequences (2/3/4-byte), dynamic buffer resizing, and skipping lone surrogates. The entire encoding logic from JS lines 42–95 is absent.

### 54. [blob.cpp] stringDecode() missing full UTF-8 decoding logic
- **JS Source**: `src/js/blob.js` lines 97–167
- **Status**: Pending
- **Details**: The C++ `stringDecode()` (lines 63–65) is a trivial byte reinterpretation. The JS version performs full UTF-8 decoding with multi-byte sequence detection (1–4 bytes), validation of continuation bytes (0xC0 mask checks), surrogate pair generation for code points > 0xFFFF, replacement character (0xFFFD) for invalid sequences, and batched String.fromCharCode conversion. The entire 70-line UTF-8 decoder from the JS is omitted.

### 55. [blob.cpp] Missing textEncode/textDecode conditional selection
- **JS Source**: `src/js/blob.js` lines 169–173
- **Status**: Pending
- **Details**: The JS `textEncode` and `textDecode` constants conditionally select between native TextEncoder/TextDecoder and the polyfill stringEncode/stringDecode functions. This conditional selection is not ported. The C++ directly uses its simplified stringEncode/stringDecode. While reasonable for C++ (no TextEncoder API), it should be documented since the original polyfill functions contained significant UTF-8 logic.

### 56. [blob.cpp] Missing bufferClone() function
- **JS Source**: `src/js/blob.js` lines 175–182
- **Status**: Pending
- **Details**: The JS `bufferClone()` function creates a byte-by-byte copy of an ArrayBuffer. While C++ vector copies serve the same purpose via BlobPart constructors, the function itself and its specific cloning logic for DataView and ArrayBuffer inputs (used in the BlobPolyfill constructor at lines 235–236) is replaced by implicit vector copying without an explicit equivalent function.

### 57. [blob.cpp] stream() method has sync/eager semantics instead of async/lazy
- **JS Source**: `src/js/blob.js` lines 271–288
- **Status**: Pending
- **Details**: The JS version returns a ReadableStream object with an async `pull()` method that uses `arrayBuffer()` (which returns a Promise). The stream is pull-based and lazy — the consumer controls the pace. The C++ version (lines 161–170) uses a synchronous callback that iterates all chunks eagerly in a blocking while loop. While both deliver the same data in 512KB chunks, the async/lazy vs sync/eager execution model differs.

### 58. [blob.cpp] URLPolyfill missing fallback path and revokeObjectURL is a no-op
- **JS Source**: `src/js/blob.js` lines 293–307
- **Status**: Pending
- **Details**: The JS `URLPolyfill.createObjectURL()` (lines 294–299) has a fallback path: if the blob is NOT an instance of BlobPolyfill, it falls back to the native `URL.createObjectURL(blob)`. The C++ version (lines 186–188) only accepts `const BlobPolyfill&` and has no fallback. The JS `revokeObjectURL()` (lines 302–306) calls `URL.revokeObjectURL(url)` for non-data URLs; the C++ version (lines 194–198) is a complete no-op, meaning non-data URLs would leak.

### 59. [buffer.cpp] Missing fromCanvas() static method
- **JS Source**: `src/js/buffer.js` lines 89–107
- **Status**: Pending
- **Details**: The static `fromCanvas()` method is completely missing from the C++ port. This async method converts an HTMLCanvasElement or OffscreenCanvas to a BufferWrapper, with special handling for lossless WebP encoding (using webp-wasm) when quality=100, and standard browser blob conversion for other formats.

### 60. [buffer.cpp] Missing decodeAudio() method
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Status**: Pending
- **Details**: The `decodeAudio()` method is completely missing. The JS method decodes the buffer's ArrayBuffer using Web Audio API AudioContext.decodeAudioData(). Not ported and not declared in buffer.h.

### 61. [buffer.cpp] readString() drops encoding parameter
- **JS Source**: `src/js/buffer.js` lines 551–561
- **Status**: Pending
- **Details**: The JS `readString()` accepts an optional `encoding` parameter (default 'utf8') that is passed to `Buffer.toString(encoding, ...)`. The C++ version drops the encoding parameter entirely. All string reads are effectively raw byte copies. If callers pass a non-utf8 encoding (e.g., 'ascii', 'latin1', 'hex'), the C++ behavior will silently differ. The encoding parameter is also missing from readNullTerminatedString, startsWith, readJSON, and readLines.

### 62. [buffer.cpp] getDataURL() produces data: URLs instead of blob: URLs
- **JS Source**: `src/js/buffer.js` lines 989–995
- **Status**: Pending
- **Details**: The JS creates a Blob from `this.internalArrayBuffer` and uses `URL.createObjectURL(blob)` to produce a `blob:` URL. The C++ produces a `data:` URL with inline base64 encoding. The format is completely different (`blob:...` vs `data:application/octet-stream;base64,...`). For large buffers the data URL will be very large, and any code checking URL format/prefix will break.

### 63. [buffer.cpp] revokeDataURL() does not match JS blob URL lifecycle
- **JS Source**: `src/js/buffer.js` lines 1000–1005
- **Status**: Pending
- **Details**: The JS calls `URL.revokeObjectURL()` to free the blob URL resource, then sets `this.dataURL = undefined`. Since C++ getDataURL() creates data: URLs (not blob: URLs), revoking is a no-op, representing a behavioral change from the JS blob URL lifecycle management.

### 64. [buffer.cpp] writeBuffer() JS bug not replicated — C++ throws instead of silently discarding Error
- **JS Source**: `src/js/buffer.js` lines 899–928
- **Status**: Pending
- **Details**: In the raw Buffer branch (lines 913–918), the JS has a bug: `new Error(...)` without `throw` (line 917). The C++ span-based overload correctly throws via `throw std::runtime_error(...)`. While arguably an improvement, it changes behavior: the JS silently creates (and discards) an Error object, while C++ throws an exception. Code relying on the JS non-throwing behavior would break.

### 65. [buffer.cpp] alloc() always zero-initializes regardless of secure parameter
- **JS Source**: `src/js/buffer.js` lines 54–56
- **Status**: Pending
- **Details**: JS uses `Buffer.alloc(length)` for secure (zeroed) and `Buffer.allocUnsafe(length)` for non-secure (uninitialized). The C++ always creates a zero-initialized vector via `std::vector<uint8_t>(length)` regardless of the `secure` parameter. This is a performance difference — JS `allocUnsafe` intentionally avoids zeroing for speed.

### 66. [buffer.cpp] calculateHash() only supports md5 and sha1
- **JS Source**: `src/js/buffer.js` lines 1036–1038
- **Status**: Pending
- **Details**: The JS uses Node's crypto module which supports all hash algorithms (md5, sha1, sha256, sha384, sha512, etc.) and all encodings. The C++ only supports 'md5' and 'sha1' hash algorithms and only 'hex' and 'base64' encodings, throwing for anything else. This limitation is not documented.

### 67. [buffer.cpp] unmapSource() uses _buf.size() which may be incorrect after setCapacity()
- **JS Source**: `src/js/buffer.js` lines 123–127, 1065
- **Status**: Pending
- **Details**: The JS `fromMmap()` stores the mmap object itself which has an `unmap()` method. The C++ `fromMmap()` copies mapped data into a vector and stores the raw void pointer. The C++ `unmapSource()` uses `_buf.size()` as the mapping size for `munmap()`, which will be incorrect if `setCapacity()` was ever called after `fromMmap()`, potentially causing undefined behavior.

### 68. [buffer.cpp] readBuffer() API split differs from JS
- **JS Source**: `src/js/buffer.js` lines 531–543
- **Status**: Pending
- **Details**: The JS `readBuffer()` has a `wrap` parameter controlling whether the result is a BufferWrapper (wrap=true) or raw Buffer (wrap=false). The C++ splits this into two separate methods: `readBuffer()` (returns BufferWrapper) and `readBufferRaw()` (returns vector). Any JS caller using `readBuffer(length, false)` must be changed to `readBufferRaw(length)`.

### 69. [buffer.h] BufferWrapper has virtual methods not present in JS
- **JS Source**: `src/js/buffer.js` lines 47–1128
- **Status**: Pending
- **Details**: The C++ BufferWrapper declares `virtual ~BufferWrapper()` and `virtual void _checkBounds()`. The JS BufferWrapper class has no virtual methods or inheritance. Making _checkBounds virtual and the destructor virtual adds vtable overhead. The header says "Virtual so that BLTEReader can override to lazily decompress blocks" — this is C++-specific design with no equivalent in the JS source.

### 70. [config.cpp] save() is synchronous instead of deferred
- **JS Source**: `src/js/config.js` lines 83–91
- **Status**: Pending
- **Details**: `save()` calls `doSave()` synchronously, but JS uses `setImmediate(doSave)` which defers execution to the next event loop tick. This changes timing behavior — the C++ version blocks the caller with synchronous file I/O, while JS defers it to prevent blocking UI rendering.

### 71. [config.cpp] Toast message says "wow.export" instead of "wow.export.cpp"
- **JS Source**: `src/js/config.js` line 46
- **Status**: Pending
- **Details**: Toast message reads `"Restart wow.export using \"Run as Administrator\"."` Per project naming conventions, user-facing text should say `"wow.export.cpp"` not `"wow.export"`.

### 72. [config.cpp] doSave() is synchronous instead of async
- **JS Source**: `src/js/config.js` lines 96–116
- **Status**: Pending
- **Details**: `doSave()` in JS is `async` — it uses `await fsp.writeFile(...)` for non-blocking file I/O. The C++ performs synchronous `std::ofstream` writes. This may block the UI thread if called from the main thread.

### 73. [config.cpp] Missing Vue $watch for auto-save on config changes
- **JS Source**: `src/js/config.js` line 60
- **Status**: Pending
- **Details**: JS sets up `core.view.$watch('config', () => save(), { deep: true })` to auto-save on any config change. The C++ has a comment saying this is replaced by explicit `save()` calls from the UI layer. Config changes made programmatically (not through UI) will NOT auto-persist, which is a functional deviation.

### 74. [constants.cpp] DATA_PATH uses install-relative path instead of OS user-data directory
- **JS Source**: `src/js/constants.js` line 16
- **Status**: Pending
- **Details**: `DATA_PATH` in JS is `nw.App.dataPath` — an OS-specific user-data directory (e.g., `%LOCALAPPDATA%` on Windows, `~/.config` on Linux). C++ uses `s_install_path / "data"` (a subdirectory of the install path). This affects portability and multi-user scenarios.

### 75. [constants.cpp] RUNTIME_LOG in different directory than JS
- **JS Source**: `src/js/constants.js` line 38
- **Status**: Pending
- **Details**: `RUNTIME_LOG` in JS is `path.join(DATA_PATH, 'runtime.log')`. C++ is `s_log_dir / "runtime.log"` where `s_log_dir = s_install_path / "Logs"` — a separate `Logs/` directory that doesn't exist in JS. The log file ends up in a completely different location.

### 76. [constants.cpp] SHADER_PATH uses different subdirectory structure
- **JS Source**: `src/js/constants.js` line 43
- **Status**: Pending
- **Details**: `SHADER_PATH` in JS is `path.join(INSTALL_PATH, 'src', 'shaders')`. C++ is `s_data_dir / "shaders"` (i.e. `<install>/data/shaders`). Different subdirectory structure.

### 77. [constants.cpp] CONFIG.DEFAULT_PATH uses different subdirectory
- **JS Source**: `src/js/constants.js` line 95
- **Status**: Pending
- **Details**: `CONFIG.DEFAULT_PATH` in JS is `path.join(INSTALL_PATH, 'src', 'default_config.jsonc')`. C++ is `s_data_dir / "default_config.jsonc"` (i.e. `<install>/data/default_config.jsonc`).

### 78. [constants.cpp] CACHE.DIR renamed from "casc" to "cache"
- **JS Source**: `src/js/constants.js` line 72
- **Status**: Pending
- **Details**: `CACHE.DIR` in JS is `path.join(DATA_PATH, 'casc')`. C++ renames it to `s_data_dir / "cache"` with a migration from legacy `casc/`. This changes the cache directory name, which could break any external tools referencing the `casc` directory.

### 79. [constants.cpp] init() contains legacy directory migration logic not in JS
- **JS Source**: `src/js/constants.js` lines 97–103
- **Status**: Pending
- **Details**: C++ `init()` contains legacy directory migration logic (renaming `config/` → `persistence/` → `data/`, and `casc/` → `cache/`) that does not exist in the JS source at all. This is entirely new functionality not present in the original application.

### 80. [constants.h] VERSION is hardcoded instead of read from manifest
- **JS Source**: `src/js/constants.js` line 46
- **Status**: Pending
- **Details**: `VERSION` is hardcoded as `"0.1.0"` in C++. JS reads it dynamically from `nw.App.manifest.version`. There is no mechanism to keep the C++ version string in sync with build/release processes.

### 81. [constants.h] getBlenderBaseDir() missing macOS case without documentation
- **JS Source**: `src/js/constants.js` lines 20–33
- **Status**: Pending
- **Details**: `getBlenderBaseDir()` in JS handles three platforms: `win32`, `darwin` (macOS), and `linux`/default. The C++ only handles `_WIN32` and else (Linux). The macOS case is intentionally skipped per project scope, but there is no code comment in the .cpp file documenting this omission.

### 82. [core.cpp] Toast TTL auto-dismiss is non-functional
- **JS Source**: `src/js/core.js` lines 470–479
- **Status**: Pending
- **Details**: `setToast()` in JS stores the timer ID from `setTimeout(hideToast, ttl)` in `toastTimer`, and `clearTimeout(toastTimer)` cancels pending auto-dismiss. In C++, `setToast()` stores `toastExpiry` as a time point, but there is NO polling mechanism to check if `toastExpiry` has passed and call `hideToast()`. The `toastTimer` and `toastExpiry` static variables are only accessible within `core.cpp`. Toasts with a TTL will never auto-dismiss.

### 83. [core.cpp] hideToast() timer cancellation is a no-op
- **JS Source**: `src/js/core.js` lines 449–460
- **Status**: Pending
- **Details**: `hideToast()` in JS calls `clearTimeout(toastTimer)` to cancel the pending timeout. C++ only sets `toastTimer = -1` — since there's no actual timer/timeout mechanism, this is a no-op. Combined with entry 82, the entire toast TTL system is non-functional.

### 84. [core.cpp] showLoadingScreen() has one-frame delay due to postToMainThread
- **JS Source**: `src/js/core.js` lines 413–420
- **Status**: Pending
- **Details**: In JS, `showLoadingScreen()` sets state synchronously (same event loop turn). C++ posts state changes to a main-thread queue via `postToMainThread()`, meaning the loading screen won't appear until the next frame when `drainMainThreadQueue()` runs. This introduces a one-frame delay.

### 85. [core.cpp] progressLoadingScreen() lacks forced redraw
- **JS Source**: `src/js/core.js` lines 426–434
- **Status**: Pending
- **Details**: `progressLoadingScreen()` in JS calls `await generics.redraw()` which forces a UI repaint/yield so the progress is visible immediately. The C++ posts via `postToMainThread()` with no equivalent forced redraw. Progress updates may not be visible until the background work completes and the main thread drains the queue.

### 86. [core.cpp] openExportDirectory() uses non-JS openInExplorer() function
- **JS Source**: `src/js/core.js` line 484–486
- **Status**: Pending
- **Details**: JS calls `nw.Shell.openItem(core.view.config.exportDirectory)` directly. C++ calls `openInExplorer()` which is an extra function not present in the JS source. Additionally, the Windows implementation uses `std::wstring wpath(path.begin(), path.end())` which is incorrect for non-ASCII (UTF-8) paths — it copies bytes as if they were UTF-16 code units. Should use `MultiByteToWideChar`.

### 87. [core.h] AppState contains many fields not in JS makeNewView()
- **JS Source**: `src/js/core.js` lines 30–381
- **Status**: Pending
- **Details**: C++ `AppState` contains fields not present in the JS `makeNewView()`: `mpq`, `chrCustRacesPlayable`, `chrCustRacesNPC`, `pendingItemSlotFilter`, `zoneMapTexID`, `zoneMapWidth`, `zoneMapHeight`, `zoneMapPixels`, various texture preview Tex IDs. While most are C++/OpenGL-specific additions, they represent deviations from the JS source.

### 88. [core.h] Missing constants field on AppState
- **JS Source**: `src/js/core.js` line 44–45
- **Status**: Pending
- **Details**: JS `makeNewView()` includes `constants: constants` as a field on the view object, making the constants module available through the view. C++ omits this (accessed via `constants::` namespace), changing how templates/components access constants.

### 89. [core.h] Missing availableLocale field on AppState
- **JS Source**: `src/js/core.js` line 120
- **Status**: Pending
- **Details**: JS `makeNewView()` includes `availableLocale: Locale` as a field on the view. C++ only has a comment saying it's a compile-time constant. Any JS code accessing `view.availableLocale` would not find it on the C++ `AppState`.

### 90. [external-links.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/external-links.js` lines 1–45
- **Status**: Pending
- **Details**: The .cpp file is entirely unconverted — it contains raw JavaScript (`const util = require('util')`, `module.exports`, `nw.Shell.openExternal`, etc.). It is a byte-for-byte copy of the .js file. No header file (external-links.h) exists. Nothing has been ported: the STATIC_LINKS map (5 entries), the WOWHEAD_ITEM constant, and the ExternalLinks class with `open()` and `wowHead_viewItem()` static methods are all still JavaScript.

### 91. [file-writer.cpp] Backpressure/drain mechanism is non-functional
- **JS Source**: `src/js/file-writer.js` lines 24–33
- **Status**: Pending
- **Details**: In JS, if blocked is true, `writeLine()` awaits a promise that is resolved by `_drain()` when the stream's 'drain' event fires. In C++, if blocked is true, `_drain()` is called synchronously (immediately setting blocked=false) and the write proceeds unconditionally. The C++ never actually waits or handles backpressure. The mechanism should either be removed (since std::ofstream doesn't need backpressure) or corrected.

### 92. [file-writer.cpp] blocked flag set on I/O error instead of backpressure
- **JS Source**: `src/js/file-writer.js` lines 28–32
- **Status**: Pending
- **Details**: In JS, `stream.write()` returning false indicates backpressure (normal flow control, not an error). In C++, `stream.fail()` indicates an actual I/O error (disk full, permissions). C++ sets blocked=true on I/O failure and then calls `stream.clear()` which resets error flags but doesn't fix the underlying problem, potentially causing silent data loss.

### 93. [generics.cpp] get() returns raw bytes instead of Response-like object
- **JS Source**: `src/js/generics.js` lines 22–54
- **Status**: Pending
- **Details**: The JS `get()` returns a fetch Response object (with .ok, .status, .statusText, .json(), .text(), etc.). The C++ returns `std::vector<uint8_t>` (raw body bytes only). External callers cannot inspect HTTP status codes or headers. Additionally, in JS, non-ok responses don't throw — they proceed to the next URL. In C++, `doHttpGet()` throws on any non-2xx status, changing control flow.

### 94. [generics.cpp] get() always logs [200] status regardless of actual status
- **JS Source**: `src/js/generics.js` line 44
- **Status**: Pending
- **Details**: The JS logs the real response status: `get -> [${index++}][${res.status}] ${url}` which could be 200, 201, 206, etc. The C++ hardcodes `[200]` regardless of actual status.

### 95. [generics.cpp] getJSON() error message omits HTTP status details
- **JS Source**: `src/js/generics.js` lines 116–122
- **Status**: Pending
- **Details**: JS throws "Unable to request JSON from end-point. HTTP ${res.status} ${res.statusText}". C++ throws only "Unable to request JSON from end-point" without status details.

### 96. [generics.cpp] requestData() missing download progress logging and redirect logging
- **JS Source**: `src/js/generics.js` lines 145–205
- **Status**: Pending
- **Details**: The JS logs "Starting download: N bytes expected" (content-length), "Download progress: X/Y bytes (Z%)" at 25% thresholds, and "Got redirect to " + location for 301/302 redirects. The C++ only logs the initial request and "Download complete" — all intermediate progress and redirect logging is absent.

### 97. [generics.cpp] queue() waits FIFO instead of as-available
- **JS Source**: `src/js/generics.js` lines 63–83
- **Status**: Pending
- **Details**: The JS uses promise-based concurrency where ANY completed task immediately triggers the next one via `check()` callback chained with `.then()`. The C++ always waits on `futures.front().get()` — it only checks the FIRST future, meaning if later tasks complete before earlier ones, the C++ won't start new work until the front finishes. This reduces effective parallelism.

### 98. [generics.cpp] readJSON() EPERM detection only checks owner permission bits
- **JS Source**: `src/js/generics.js` lines 130–143
- **Status**: Pending
- **Details**: The JS checks `e.code === 'EPERM'` which is an OS-level permission error from the effective process access. The C++ checks owner_read permission bits via `std::filesystem::status()`, which misses group/other/ACL/SELinux denials. A file unreadable due to group permissions would incorrectly return `std::nullopt`.

### 99. [generics.cpp] directoryIsWritable() only checks owner_write permission bit
- **JS Source**: `src/js/generics.js` lines 356–363
- **Status**: Pending
- **Details**: The JS uses `fs.constants.W_OK` with `fsp.access()` which checks effective access permissions of the calling process (including group, other, and ACL). The C++ only checks `std::filesystem::perms::owner_write`. A directory writable via group permissions would return true in JS but false in C++.

### 100. [generics.cpp] batchWork() does not yield between batches
- **JS Source**: `src/js/generics.js` lines 420–469
- **Status**: Pending
- **Details**: The JS uses MessageChannel scheduling to yield to the browser event loop between batches, enabling UI updates mid-processing. The C++ runs all batches synchronously in a tight loop with no yielding. The C++ also adds `core::postToMainThread()` calls to update loadingProgress that do not exist in the JS source.

### 101. [generics.cpp] getFileHash() loads entire file into memory instead of streaming
- **JS Source**: `src/js/generics.js` lines 329–337
- **Status**: Pending
- **Details**: JS streams the file using `fs.createReadStream()` and pipes chunks to the hash, which is memory-efficient. C++ loads the entire file via `BufferWrapper::readFile()`, then hashes it. For large WoW game files this could cause excessive memory usage.

### 102. [gpu-info.cpp] exec_cmd() missing 5000ms timeout
- **JS Source**: `src/js/gpu-info.js` lines 65–74
- **Status**: Pending
- **Details**: The JS uses `exec(cmd, { timeout: 5000 }, ...)` which kills the child process after 5 seconds. The C++ popen-based implementation has no timeout mechanism, so a hung command (e.g. nvidia-smi on a misconfigured system) will block indefinitely.

### 103. [gpu-info.cpp] GL capability queries use component counts instead of vector counts
- **JS Source**: `src/js/gpu-info.js` lines 41–42
- **Status**: Pending
- **Details**: JS queries `gl.MAX_VERTEX_UNIFORM_VECTORS` and `gl.MAX_FRAGMENT_UNIFORM_VECTORS` (WebGL vector-based counts). C++ queries `GL_MAX_VERTEX_UNIFORM_COMPONENTS` and `GL_MAX_FRAGMENT_UNIFORM_COMPONENTS` (component-based counts). Components = Vectors × 4, so logged values will be ~4× larger than JS output. OpenGL 4.1+ does define the vector variants for compatibility.

### 104. [gpu-info.cpp] get_gl_info() cannot indicate "GL unavailable" state
- **JS Source**: `src/js/gpu-info.js` lines 14–58
- **Status**: Pending
- **Details**: The JS `get_webgl_info()` can return null (no GL context), `{ error: msg }` (exception), or a full result object. The C++ `get_gl_info()` always returns a populated GLInfo struct — there is no way to distinguish "GL context not available" from "GL context available but no debug info." This merges the "GPU: WebGL unavailable" code path with "GL debug info unavailable."

### 105. [gpu-info.cpp] format_extensions() uses different prefix-stripping logic
- **JS Source**: `src/js/gpu-info.js` lines 250–303
- **Status**: Pending
- **Details**: The JS strips WebGL-specific prefixes (e.g. "WEBGL_compressed_texture_", "WEBGL_", "EXT_"). The C++ strips OpenGL-specific prefixes (e.g. "GL_ARB_", "GL_EXT_", "GL_OES_"). While adapting for OpenGL is reasonable, the stripping structure also differs: JS uses `string.replace()` (replaces first occurrence anywhere) while C++ uses `find() + substr()` (only strips matching prefixes). This could produce different output for certain extension names.

### 106. [gpu-info.cpp] get_macos_gpu_info() entirely omitted
- **JS Source**: `src/js/gpu-info.js` lines 203–226
- **Status**: Pending
- **Details**: The JS contains a full macOS implementation using `system_profiler SPDisplaysDataType`. While the project specifies no macOS support, this represents a deviation from the JS source that should be documented.

### 107. [icon-render.cpp] processQueue() is completely stubbed out
- **JS Source**: `src/js/icon-render.js` lines 48–65
- **Status**: Pending
- **Details**: The function body contains only a comment placeholder ("CASC source and BLP decoder are unconverted") and an empty try/catch. The actual functionality — calling `core.view.casc.getFile(entry.fileDataID)`, constructing a BLPFile, and calling `blp.getDataURL(0b0111)` to create the icon image — is entirely missing. No BLP decoding, no texture creation, no CASC file retrieval occurs.

### 108. [icon-render.cpp] processQueue() changed from async recursive to synchronous loop
- **JS Source**: `src/js/icon-render.js` lines 48–65
- **Status**: Pending
- **Details**: The JS processes one item at a time via promise chaining (`.then/.catch/.finally(() => processQueue())`), returning to the event loop between items. The C++ uses a synchronous while loop that would block the main thread if the BLP loading were implemented.

### 109. [icon-render.cpp] getIconTexture() is an extra function not in JS
- **JS Source**: `src/js/icon-render.js` (not in original)
- **Status**: Pending
- **Details**: `getIconTexture(uint32_t fileDataID)` does not exist in the JS source. While needed for the C++ OpenGL-based rendering approach (replacing CSS background-image), it represents an API addition beyond the original module interface which only exported `{ loadIcon }`.

### 110. [icon-render.cpp] STB_IMAGE_IMPLEMENTATION defined in wrong file
- **JS Source**: `src/js/icon-render.js` lines 1–109
- **Status**: Pending
- **Details**: `STB_IMAGE_IMPLEMENTATION` is `#defined` in icon-render.cpp (line 17). This macro causes stb_image to emit all function definitions. If any other translation unit also defines it before including stb_image.h, it will cause linker errors. This macro should be defined in exactly one dedicated .cpp file, not in a module like icon-render.cpp.

### 111. [log.cpp] write() does not support variadic printf-style formatting
- **JS Source**: `src/js/log.js` lines 78–95
- **Status**: Pending
- **Details**: The JS `write(...parameters)` uses `util.format(...parameters)` to support calls like `write('GPU: %s (%s)', renderer, vendor)`. The C++ `write(std::string_view message)` only accepts a pre-formatted string, requiring all callers to use `std::format()` externally. This changes the API contract.

### 112. [log.cpp] timeEnd() does not support variadic format parameters
- **JS Source**: `src/js/log.js` lines 64–66
- **Status**: Pending
- **Details**: The JS `timeEnd(label, ...params)` constructs `write(label + ' (%dms)', ...params, elapsed)`, allowing callers to pass format arguments. The C++ only takes a label and appends elapsed time. Format specifiers in the label appear as literal text.

### 113. [log.cpp] Bug: line moved before debug output, prints empty string
- **JS Source**: `src/js/log.js` lines 78–95
- **Status**: Pending
- **Details**: When the stream is clogged and `pool.size() < MAX_LOG_POOL`, line is moved via `pool.push_back(std::move(line))` on line 122. After the move, line is in an unspecified (typically empty) state. The subsequent `std::fputs(line.c_str(), stdout)` on line 131 will print an empty string instead of the log message.

### 114. [log.cpp] drainPool() infinite retry on permanently failed stream
- **JS Source**: `src/js/log.js` lines 32–49
- **Status**: Pending
- **Details**: In JS, `pool.shift()` always removes the item before writing — Node.js `stream.write()` returning false means backpressure but the data IS buffered and will be written. In C++, if `stream.fail()` occurs, the item stays in the pool. A permanently failed stream would retry the same item forever.

### 115. [log.cpp] drainPool() missing "schedule another drain" logic
- **JS Source**: `src/js/log.js` lines 46–48
- **Status**: Pending
- **Details**: The JS calls `process.nextTick(drainPool)` after the while loop if `!isClogged && pool.length > 0`, ensuring remaining pooled items are drained even without new write() calls. The C++ only drains when `write()` is called, so if more than MAX_DRAIN_PER_TICK (50) items are pooled and no new writes occur, remaining items stay in the pool indefinitely.

### 116. [log.cpp] timeEnd() uses fixed 64-byte buffer that may truncate
- **JS Source**: `src/js/log.js` lines 64–66
- **Status**: Pending
- **Details**: `timeEnd()` uses a fixed `char buf[64]` for snprintf output. If the label string is longer than ~50 characters, the output will be silently truncated. The JS version has no such length limitation.

### 117. [mmap.cpp] release_virtual_files() does not protect against exceptions from delete
- **JS Source**: `src/js/mmap.js` lines 30–47
- **Status**: Pending
- **Details**: In the C++ release loop, `delete mmap_obj` (line 243) is called inside the loop. If `delete` throws, the remaining objects won't be cleaned up. The JS version calls only `unmap()` and lets GC handle destruction, ensuring all objects in the set are always attempted. The C++ should catch exceptions around `delete` for parity with the JS's "swallows errors" contract.

### 118. [modules.cpp] Missing module_test_a and module_test_b registrations
- **JS Source**: `src/js/modules.js` lines 27–28
- **Status**: Pending
- **Details**: `module_test_a` and `module_test_b` are defined in the JS MODULES object but are completely absent from the C++ `initialize()` function. The corresponding .cpp files exist but no headers exist and they are not registered.

### 119. [modules.cpp] Missing tab_help, tab_blender, tab_changelog module registrations
- **JS Source**: `src/js/modules.js` lines 48–50
- **Status**: Pending
- **Details**: `tab_help`, `tab_blender`, and `tab_changelog` are defined in the JS MODULES object but the C++ `initialize()` function has only placeholder comments saying "has not been created yet. It needs to be ported." The .cpp files exist but no .h headers exist and they are not registered.

### 120. [modules.cpp] wrap_module() does not provide register_context to modules
- **JS Source**: `src/js/modules.js` lines 208–218
- **Status**: Pending
- **Details**: The JS `wrap_module()` provides a `register_context` object with `registerNavButton()` and `registerContextMenuOption()` to each module's `register()` function. `registerNavButton()` captures the label into `display_label` for error messages. The C++ calls `mod.registerModule()` directly without a register_context, so `display_label` is never updated and error messages always show the internal module key instead of the user-facing label.

### 121. [modules.cpp] Module initialize() called synchronously instead of async
- **JS Source**: `src/js/modules.js` lines 225, 231–232
- **Status**: Pending
- **Details**: The JS initialize wrapper is an async function using `await original_initialize.call(this)`. The C++ calls `original_initialize()` synchronously. Since JS module initialize() functions use await, the C++ error handling will not catch errors from asynchronous operations that happen after the initial synchronous return.

### 122. [modules.cpp] _tab_initializing reset not protected by RAII/finally equivalent
- **JS Source**: `src/js/modules.js` lines 239–241
- **Status**: Pending
- **Details**: The JS uses try/catch/finally to ensure `_tab_initializing` is always reset to false. The C++ places `mod._tab_initializing = false` after the catch block but outside any RAII guard. If the catch block throws, `_tab_initializing` won't be reset, causing the module to be permanently stuck in the "initializing" state.

### 123. [modules.cpp] Missing activated() lifecycle hook wrapping
- **JS Source**: `src/js/modules.js` lines 244–251
- **Status**: Pending
- **Details**: The JS `wrap_module()` wraps the module's `activated()` lifecycle hook to retry initialization if not yet initialized. The C++ does not implement any `activated()` equivalent. Instead, `set_active()` calls `initialize()` on every activation, but the JS `activated()` also calls the `original_activated` callback if present, which the C++ does not preserve.

### 124. [modules.cpp] Missing Proxy-based setActive() and reload() on module objects
- **JS Source**: `src/js/modules.js` lines 254–267
- **Status**: Pending
- **Details**: The JS `wrap_module()` returns a Proxy that intercepts property access to provide `__name`, `setActive()`, and `reload()` virtual properties. The C++ ModuleDef struct has no equivalent. Any code calling `module.setActive()` or `module.reload()` on a module object will not work in C++.

### 125. [modules.cpp] register_context_menu_option() does not support dev_only
- **JS Source**: `src/js/modules.js` lines 162–165, 289–291
- **Status**: Pending
- **Details**: The JS `register_static_context_menu_option()` wraps the action in `{ handler: action, dev_only }` and passes it to `register_context_menu_option()`. The C++ internal `register_context_menu_option()` creates a ContextMenuOption without setting dev_only, so options registered through it always have dev_only=false.

### 126. [png-writer.cpp] write() is synchronous instead of async
- **JS Source**: `src/js/png-writer.js` lines 247–249
- **Status**: Pending
- **Details**: The JS `write()` is `async` and returns a promise from `this.getBuffer().writeToFile(file)`. The C++ `write()` is synchronous and returns void. Callers expecting async behavior (error handling via promise rejection, non-blocking I/O) will behave differently.

### 127. [subtitles.cpp] get_subtitles_vtt() signature differs — CASC loading removed
- **JS Source**: `src/js/subtitles.js` lines 172–187
- **Status**: Pending
- **Details**: The JS is an async function taking `(casc, file_data_id, format)` that calls `await casc.getFile(file_data_id)` to load data. The C++ takes `(std::string_view text, SubtitleFormat format)` — CASC file loading is removed entirely and delegated to the caller, significantly changing the function signature and responsibilities.

### 128. [subtitles.h] Internal functions exposed as public API
- **JS Source**: `src/js/subtitles.js` lines 189–192
- **Status**: Pending
- **Details**: The JS exports only `SUBTITLE_FORMAT` and `get_subtitles_vtt`. The C++ header additionally exposes `parse_sbt_timestamp`, `format_srt_timestamp`, `format_vtt_timestamp`, `parse_srt_timestamp`, `sbt_to_srt`, and `srt_to_vtt` as public API. The JS keeps these as module-private. These internal functions could be made file-local to match JS encapsulation.

### 129. [tiled-png-writer.cpp] Tile iteration order differs from JS Map insertion order
- **JS Source**: `src/js/tiled-png-writer.js` lines 38–46, 52–62
- **Status**: Pending
- **Details**: The C++ uses `std::map<std::string, Tile>` (sorted by key) whereas the JS uses `Map` (preserves insertion order). When tiles overlap, alpha blending in `_writeTileToPixelData` means iteration order affects final pixel values. JS iterates in insertion order; C++ iterates in lexicographic key order. For overlapping tiles, this produces different blending results. Should use a container that preserves insertion order.

### 130. [tiled-png-writer.cpp] write() is synchronous instead of async
- **JS Source**: `src/js/tiled-png-writer.js` lines 123–125
- **Status**: Pending
- **Details**: The JS `write()` is `async` and returns the result of `this.getBuffer().writeToFile(file)`. The C++ `write()` returns void. Same issue as entry 126 for png-writer.cpp.

### 131. [updater.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/updater.js` lines 1–169
- **Status**: Pending
- **Details**: The entire .cpp file is still raw JavaScript — it is a verbatim copy of updater.js with zero C++ conversion. All three functions (`checkForUpdates`, `applyUpdate`, `launchUpdater`), the module-level `updateManifest` variable, and the `module.exports` line are unconverted. There is no corresponding .h header file. Every line needs to be ported to C++.

### 132. [wmv.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/wmv.js` lines 1–177
- **Status**: Pending
- **Details**: The entire .cpp file is still raw JavaScript — it is a verbatim copy of wmv.js with zero C++ conversion. All four functions (`wmv_parse`, `wmv_parse_v2`, `wmv_parse_v1`, `extract_race_gender_from_path`), the `race_map` constant, and the `module.exports` line are unconverted. No .h header file exists.

### 133. [wowhead.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/wowhead.js` lines 1–245
- **Status**: Pending
- **Details**: The entire .cpp file is still raw JavaScript — it is a verbatim copy of wowhead.js with zero C++ conversion. All seven functions (`decode`, `decompress_zeros`, `extract_hash_from_url`, `wowhead_parse_hash`, `parse_v15`, `parse_legacy`, `wowhead_parse`), both constants (`charset`, `WOWHEAD_SLOT_TO_SLOT_ID`), and the `module.exports` line are unconverted. No .h header file exists.

## src/js/workers/ Audit

### 134. [cache-collector.cpp] `https_request()` hardcodes POST content type to `application/octet-stream`
- **JS Source**: `src/js/workers/cache-collector.js` lines 19–44
- **Status**: Pending
- **Details**: The JS `https_request(url, options, body)` passes headers from `options.headers` directly to the request, including whatever `Content-Type` the caller specified. The C++ `https_request()` (lines 416–456) passes the caller's headers to httplib but then also provides a hardcoded `"application/octet-stream"` as the content-type argument to `cli.Post()` (line 434), which overrides any `Content-Type` already set in the headers map. If `https_request()` were called with `Content-Type: multipart/form-data` (as the JS `upload_chunks` does on line 103), the wrong content type would be sent. The function should use the Content-Type from the headers parameter, not a hardcoded value.

### 135. [cache-collector.cpp] `json_post()` and `upload_chunks()` duplicate HTTP client logic instead of calling `https_request()`
- **JS Source**: `src/js/workers/cache-collector.js` lines 46–65 (`json_post`), 93–111 (`upload_chunks`)
- **Status**: Pending
- **Details**: In the JS, `json_post` (line 48) calls `https_request()` to perform the HTTP POST, and `upload_chunks` (line 100) also calls `https_request()`. In the C++, both `json_post()` (lines 458–495) and `upload_chunks()` (lines 530–564) create their own httplib `SSLClient`/`Client` instances, set timeouts, and call `cli.Post()` directly — completely bypassing the `https_request()` function. This means any future fix or change to `https_request()` (e.g., adding retry logic, proxy support, or certificate handling) would not apply to `json_post()` or `upload_chunks()`. The three functions should share a single HTTP request path as the JS does.

### 136. [cache-collector.cpp] `scan_wdb()` uses case-insensitive `.wdb` extension check instead of case-sensitive
- **JS Source**: `src/js/workers/cache-collector.js` lines 201–203
- **Status**: Pending
- **Details**: The JS `scan_wdb` (line 202) uses `file.endsWith('.wdb')` which is a case-sensitive check — only files with a lowercase `.wdb` extension are matched. The C++ `scan_wdb()` (line 685) uses `iends_with(file_name, ".wdb")` which is case-insensitive, meaning it would also match `.WDB`, `.Wdb`, etc. While this is unlikely to cause issues in practice (WoW cache files always use lowercase extensions), it is a behavioral deviation from the original JS.

### 137. [cache-collector.cpp] HTTP client timeouts are hardcoded with no JS equivalent
- **JS Source**: `src/js/workers/cache-collector.js` lines 19–44
- **Status**: Pending
- **Details**: The C++ code sets `set_connection_timeout(30)` and `set_read_timeout(60)` on all httplib clients (e.g., lines 431–432, 472–473, 551–552). The JS code uses Node.js `https.request()` with no explicit timeout configuration, relying on Node.js defaults. While adding timeouts is arguably an improvement, it is a behavioral deviation — very large file uploads or slow connections could time out in the C++ version but succeed in the JS version.

### 138. [cache-collector.cpp] `https_request()` does not propagate error on connection failure
- **JS Source**: `src/js/workers/cache-collector.js` lines 37–38
- **Status**: Pending
- **Details**: The JS `https_request` (line 37) has `req.on('error', reject)` which rejects the promise (throws) on connection errors, DNS failures, TLS errors, etc. The C++ `https_request()` (lines 448–455) checks `if (res)` but when the httplib result is falsy (connection failed), it returns a default `HttpResponse` with `status=0, ok=false, data=""` instead of throwing an exception. Callers that check `res.ok` will see a non-OK response, but they lose the specific error information. The JS behavior would cause `upload_chunks` to throw `'upload chunk failed: ...'` with the underlying error, whereas the C++ would throw `'upload chunk failed: 0'` (line 562) with no descriptive error.

### 139. [cache-collector.cpp] `json_post()` does not propagate error on connection failure
- **JS Source**: `src/js/workers/cache-collector.js` lines 46–65
- **Status**: Pending
- **Details**: Same issue as #138 but for `json_post()`. The JS `json_post` calls `https_request` which rejects on connection error, causing the error to propagate to callers. The C++ `json_post()` (lines 482–493) returns a default `JsonPostResponse` with `status=0, ok=false` when the httplib result is falsy, silently swallowing the connection error. In `upload_flavor()`, this means a connection failure during submit would log `"submit failed (0) for ..."` rather than propagating the actual error.

## src/js/ui/ Audit

### 140. [audio-helper.cpp] `load()` returns void instead of the decoded audio buffer
- **JS Source**: `src/js/ui/audio-helper.js` lines 31–35
- **Status**: Pending
- **Details**: The JS `async load(array_buffer)` returns `this.buffer` (the decoded `AudioBuffer`). The C++ `void load(...)` (lines 108–125) returns `void`. Any caller relying on the return value of `load()` would break. The JS function is async and returns the buffer; C++ is synchronous and returns nothing.

### 141. [audio-helper.cpp] `on_ended` callback requires polling `get_position()` instead of firing automatically
- **JS Source**: `src/js/ui/audio-helper.js` lines 57–67
- **Status**: Pending
- **Details**: JS registers `source.onended` callback directly on the `AudioBufferSourceNode`, which the browser fires automatically when playback reaches the end. The C++ version (lines 172–174, 232–242) documents that miniaudio has no per-sound `onended` callback. Instead, `on_ended` is fired from inside `get_position()` by polling `ma_sound_at_end()`. If consumers never call `get_position()`, `on_ended` never fires. This is a necessary deviation due to miniaudio's API, but callers must be aware they must poll `get_position()` periodically.

### 142. [audio-helper.cpp] `get_position()` has side effects not present in JS
- **JS Source**: `src/js/ui/audio-helper.js` lines 115–130
- **Status**: Pending
- **Details**: JS `get_position()` is a pure getter with no side effects. The C++ version (lines 223–251) detects natural end-of-playback via `ma_sound_at_end()`. When detected, it sets `is_playing = false`, `start_offset = 0`, calls `stop_source()`, fires `on_ended()`, and returns 0. In JS, `onended` is asynchronous and separate from `get_position()`. In C++, calling `get_position()` can mutate state and trigger callbacks.

### 143. [audio-helper.cpp] `get_position()` returns 0 at natural end instead of ~duration
- **JS Source**: `src/js/ui/audio-helper.js` line 126
- **Status**: Pending
- **Details**: When playback reaches the end naturally, JS returns `Math.min(position, this.buffer.duration)` (approximately the full duration). The `onended` fires separately/asynchronously. The C++ version (line 241) immediately returns `0` after resetting state. UI progress bars or position displays may flash to 0 instead of showing the full duration momentarily.

### 144. [audio-helper.cpp] `set_volume()` remembers value before `init()` — JS doesn't
- **JS Source**: `src/js/ui/audio-helper.js` lines 136–139
- **Status**: Pending
- **Details**: JS does `if (this.gain) this.gain.gain.value = value;` — if `init()` hasn't been called, this is a no-op and the volume is not remembered. C++ (lines 257–262) always stores `volume = value`, then applies to sound if it exists. Volume is remembered and applied on next `play()` (line 170). C++ behavior is arguably better but differs from JS.

### 145. [char-texture-overlay.cpp] `initEvents()` is dead code — never called
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 74–108
- **Status**: Pending
- **Details**: In JS, events are registered at module load time (automatic on `require()`). In C++ (lines 94–106), three event handlers are wrapped in `initEvents()`, but it is never called anywhere in the codebase. `tab_characters.cpp` directly calls `nextOverlay()`, `prevOverlay()`, and `ensureActiveLayerAttached()`, bypassing the event system entirely. Either `initEvents()` should be called at startup or removed.

### 146. [character-appearance.cpp] `get_field_int` default of 512 for Width/Height differs from JS undefined behavior
- **JS Source**: `src/js/ui/character-appearance.js` lines 91, 141, 150
- **Status**: Pending
- **Details**: The C++ helper `get_field_int` (lines 22–34) uses a default value of `512` for Width/Height fields. In JS, missing fields would produce `undefined`/`NaN`, which is a different failure mode. If a DataRecord is missing Width/Height, JS would get `undefined` (NaN in arithmetic), while C++ silently uses 512.

### 147. [character-appearance.cpp] Bit-shift `1 << sectionType` uses 64-bit in C++ vs 32-bit in JS
- **JS Source**: `src/js/ui/character-appearance.js` line 154
- **Status**: Pending
- **Details**: JS uses `(1 << section_type)` which is a 32-bit operation. C++ (line 246) uses `(1LL << section_type)` which is a 64-bit operation. For `section_type >= 32`, the results differ. C++ is more correct but produces different results than JS for high values.

### 148. [data-exporter.cpp] `mark()` calls missing stack trace parameter
- **JS Source**: `src/js/ui/data-exporter.js` lines 70, 126, 198, 248
- **Status**: Pending
- **Details**: JS calls `helper.mark(fileName, false, e.message, e.stack)` — passes both error message AND stack trace (4 args). C++ (lines 95, 161, 247, 306) calls `h->mark(fileName, false, e.what())` — passes only the error message (3 args), losing diagnostic stack trace information.

### 149. [data-exporter.cpp] `exportRawDB2` missing null check on fileData
- **JS Source**: `src/js/ui/data-exporter.js` lines 115–116
- **Status**: Pending
- **Details**: JS has `if (!fileData) throw new Error('Failed to retrieve DB2 file from CASC');` — an explicit null guard before calling `writeToFile`. C++ (line 153) does `BufferWrapper fileData = casc->getVirtualFileByID(fileDataID, true); fileData.writeToFile(...)` with no check. If the call returns an empty/invalid BufferWrapper, it proceeds to `writeToFile` which could silently write nothing or crash.

### 150. [data-exporter.cpp] `overwriteFiles` config default value may be inverted
- **JS Source**: `src/js/ui/data-exporter.js` lines 45, 109, 169, 229
- **Status**: Pending
- **Details**: JS accesses `core.view.config.overwriteFiles` directly — if undefined, evaluates to `undefined` (falsy), meaning `!overwriteFiles` = `true`, so it checks for existing files and skips. C++ (lines 71, 148, 218, 284) uses `core::view->config.value("overwriteFiles", true)` — default is `true`, so if the key is missing, `!true` = `false`, and it would always overwrite. The default behavior is inverted if the config key is absent.

### 151. [data-exporter.cpp] `exportDataTableSQL` null value handling differs
- **JS Source**: `src/js/ui/data-exporter.js` line 185
- **Status**: Pending
- **Details**: JS does `rowObject[headers[i]] = value !== null && value !== undefined ? value : null;` — preserves JS `null` for null/undefined values and preserves original type (number, string, etc.) for non-null. The C++ version (line 235) converts everything to `std::string`, using empty string for missing values. Comment says "empty string maps to NULL in SQLWriter" but this depends on SQLWriter treating empty strings as NULL correctly. All values are strings, so numeric values may be quoted differently in SQL output.

### 152. [data-exporter.cpp] `exportRawDBC` no error checking on file write
- **JS Source**: `src/js/ui/data-exporter.js` line 240
- **Status**: Pending
- **Details**: JS `await fsp.writeFile(exportPath, Buffer.from(raw_data));` rejects on I/O errors. C++ (lines 296–298) does `std::ofstream ofs(...); ofs.write(...); ofs.close();` with no check on `ofs.good()`, `ofs.fail()`, or `ofs.is_open()`. A write failure would be silently ignored.

### 153. [listbox-context.cpp] `get_listfile_entries` fileDataID == 0 treated differently
- **JS Source**: `src/js/ui/listbox-context.js` line 41
- **Status**: Pending
- **Details**: JS uses `fileDataID ? \`${filePath};${fileDataID}\` : filePath` — JS truthiness means `0` is falsy, so fileDataID of `0` would output just `filePath` (no `;0`). C++ (line 75) uses `parsed.fileDataID.has_value()` — `std::optional<uint32_t>(0)` has a value, so it would output `filePath;0`. Behavioral divergence for the edge case of fileDataID == 0.

### 154. [listbox-context.cpp] `open_export_directory` naive UTF-8 to wide string conversion (Windows)
- **JS Source**: `src/js/ui/listbox-context.js` line 126
- **Status**: Pending
- **Details**: JS `nw.Shell.openItem(dir)` handles encoding correctly via NW.js. C++ (line 194) does `std::wstring wpath(dir.begin(), dir.end())` — naively copies byte-by-byte from `char` to `wchar_t`. This only works for ASCII paths. Any non-ASCII characters (accented characters, CJK) will produce a garbled wide string. Should use `MultiByteToWideChar(CP_UTF8, ...)` or equivalent.

### 155. [listbox-context.cpp] `open_export_directory` command injection risk (Linux)
- **JS Source**: `src/js/ui/listbox-context.js` line 126
- **Status**: Pending
- **Details**: JS uses `nw.Shell.openItem(dir)` — a safe API. C++ (lines 197–198) does `std::string cmd = "xdg-open \"" + dir + "\" &"; std::system(cmd.c_str());` — if `dir` contains shell metacharacters, this is a command injection vector. Should use `fork()/exec()` to avoid shell interpretation.

### 156. [model-viewer-utils.cpp] `create_renderer()` passes file_data_id (uint32_t) instead of file_name (string) for WMO
- **JS Source**: `src/js/ui/model-viewer-utils.js` line 208
- **Status**: Pending
- **Details**: JS does `new WMORendererGL(data, file_name, gl_context, show_textures)` — passes the file name string. C++ (line 343) does `std::make_unique<WMORendererGL>(data, file_data_id, ctx, show_textures)` — passes the numeric file data ID instead. The JS WMO renderer receives the file name string; the C++ passes a numeric ID.

### 157. [model-viewer-utils.cpp] `export_preview()` clipboard copy sets text instead of PNG image
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 300–301
- **Status**: Pending
- **Details**: JS uses `nw.Clipboard.get()` then `clipboard.set(buf.toBase64(), 'png', true)` to copy an actual PNG image to the system clipboard. C++ (line 482) uses `ImGui::SetClipboardText(buf.toBase64().c_str())` which copies base64 text to the clipboard, not a PNG image. Users cannot paste the image into other apps. Needs a platform-specific clipboard image API.

### 158. [model-viewer-utils.cpp] `handle_animation_change()` missing `playAnimation` capability check
- **JS Source**: `src/js/ui/model-viewer-utils.js` line 243
- **Status**: Pending
- **Details**: JS checks `if (!renderer || !renderer.playAnimation)` — verifies renderer has `playAnimation` method. C++ (line 395) only checks `if (!renderer)`. If the renderer type doesn't support animations (e.g., WMO), JS would bail out but C++ would proceed to call it.

### 159. [model-viewer-utils.cpp] `handle_animation_change()` empty string vs null/undefined check
- **JS Source**: `src/js/ui/model-viewer-utils.js` line 251
- **Status**: Pending
- **Details**: JS checks `if (selected_animation_id === null || selected_animation_id === undefined)` — an empty string `""` would pass through and reach the find logic. C++ (line 403) checks `if (selected_animation_id.empty())` — an empty string returns early. Different behavior for the empty string case.

### 160. [model-viewer-utils.cpp] `create_view_state()` only supports 3 hardcoded prefixes
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 503–528
- **Status**: Pending
- **Details**: JS is fully generic — uses `core.view[prefix + 'TexturePreviewURL']` dynamic property access that works for ANY prefix. C++ (lines 720–772) only supports 3 hardcoded prefixes: `"model"`, `"decor"`, `"creature"`. Any other prefix returns all-null pointers, silently failing.

### 161. [model-viewer-utils.cpp] `export_model()` WMO non-RAW constructor passes file_data_id instead of file_name
- **JS Source**: `src/js/ui/model-viewer-utils.js` line 405
- **Status**: Pending
- **Details**: JS does `new WMOExporter(data, file_name)` — passes the file_name string for non-RAW WMO exports. C++ (line 673) does `WMOExporter exporter(data, file_data_id, casc)` — passes file_data_id number and an extra casc parameter. The JS uses the file name; C++ uses file_data_id.

### 162. [model-viewer-utils.cpp] `export_preview()` export_paths not null-checked
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 283–296
- **Status**: Pending
- **Details**: JS uses optional chaining `export_paths?.writeLine(...)` and `export_paths?.close()` because `core.openLastExportStream()` can return `null`. C++ (lines 460–475) calls `exportPaths.writeLine(...)` and `exportPaths.close()` unconditionally. If the stream is invalid/empty, this could crash or behave differently.

### 163. [model-viewer-utils.cpp] `extract_animations()` missing `Math.floor` on animation.id
- **JS Source**: `src/js/ui/model-viewer-utils.js` line 223
- **Status**: Pending
- **Details**: JS uses `Math.floor(animation.id)` in id string and label. C++ (lines 361, 363, 374, 376) uses `std::to_string(animation.id)` with no floor/truncation. If `animation.id` can be a float in the data format, C++ won't truncate it, producing different id/label strings.

### 164. [texture-exporter.cpp] Clipboard copy sets text instead of PNG image
- **JS Source**: `src/js/ui/texture-exporter.js` lines 86–87
- **Status**: Pending
- **Details**: JS uses `nw.Clipboard.get()` then `clipboard.set(png.toBase64(), 'png', true)` to copy an actual PNG image to the system clipboard. C++ (line 115) uses `ImGui::SetClipboardText(png.toBase64().c_str())` which copies base64 text to the clipboard, not a PNG image. Users cannot paste the image into other apps.

### 165. [texture-exporter.cpp] Missing null guard for exportPaths
- **JS Source**: `src/js/ui/texture-exporter.js` lines 148, 152, 157, 165, 182
- **Status**: Pending
- **Details**: JS uses `exportPaths?.writeLine(...)` and `exportPaths?.close()` (optional chaining) because `core.openLastExportStream()` can return `null`. C++ (lines 198, 202, 209, 218, 239) calls `exportPaths.writeLine(...)` and `exportPaths.close()` unconditionally. If the stream is invalid/empty, this could crash.

### 166. [texture-exporter.cpp] Case-insensitive `.blp` check only covers two cases
- **JS Source**: `src/js/ui/texture-exporter.js` line 117
- **Status**: Pending
- **Details**: JS uses `fileName.toLowerCase().endsWith('.blp')` for fully case-insensitive matching. C++ (lines 153–155) only checks `== ".blp" || == ".BLP"`, missing mixed-case variants like `.Blp`, `.bLp`, etc.

### 167. [texture-exporter.cpp] File extension detection hardcodes 4-char suffix
- **JS Source**: `src/js/ui/texture-exporter.js` line 143
- **Status**: Pending
- **Details**: JS uses `fileName.slice(fileName.lastIndexOf('.')).toLowerCase()` for proper extension parsing — finds last `.`, any length, lowercased. C++ (lines 188–192) hardcodes 4-char suffix `fileName.substr(fileName.size() - 4)` then only checks `.png`/`.PNG` and `.jpg`/`.JPG`. Misses extensions not exactly 4 chars (e.g., `.jpeg`), mixed-case variants, and files with no extension or short names.

### 168. [texture-exporter.cpp] `mark()` calls missing stack trace parameter
- **JS Source**: `src/js/ui/texture-exporter.js` line 177
- **Status**: Pending
- **Details**: JS calls `helper.mark(markFileName, false, e.message, e.stack)` passing 4 args including stack trace. C++ (line 232) calls `helper.mark(markFileName, false, e.what())` passing 3 args, no stack trace. C++ `std::exception` has no `.stack` equivalent.

### 169. [uv-drawer.cpp] Line width 1px instead of JS 0.5px
- **JS Source**: `src/js/ui/uv-drawer.js` line 24
- **Status**: Pending
- **Details**: JS sets `ctx.lineWidth = 0.5` — the Canvas API renders these as semi-transparent 1px lines via anti-aliasing. C++ uses Bresenham's line algorithm (lines 18–48) which always draws fully opaque 1px lines — visually thicker and more prominent than the original. This is a visual fidelity difference inherent to the rasterization approach.

### 170. [uv-drawer.cpp] Anti-aliased (JS) vs aliased (C++) line rendering
- **JS Source**: `src/js/ui/uv-drawer.js` lines 41–44
- **Status**: Pending
- **Details**: JS `ctx.moveTo()`/`ctx.lineTo()` uses the Canvas 2D API which provides anti-aliased line rendering by default. C++ uses Bresenham's algorithm (lines 18–48) which produces aliased (staircase) lines with no smoothing. UV wireframes will appear more jagged in the C++ version.

### 171. [uv-drawer.cpp] Sub-pixel precision lost — float coords truncated to int
- **JS Source**: `src/js/ui/uv-drawer.js` lines 34–39
- **Status**: Pending
- **Details**: JS keeps UV coordinates as floating-point values and passes them directly to `moveTo()`/`lineTo()` which handles sub-pixel positioning natively. C++ (lines 71–76) truncates UV coordinates to `int` via `static_cast<int>()`. Lines may be offset by up to 1 pixel vs the JS version. Using `std::round()` or `std::lround()` would better match Canvas behavior.

### 172. [uv-drawer.cpp] No bounds checking on `uvCoords` array access
- **JS Source**: `src/js/ui/uv-drawer.js` lines 34–39
- **Status**: Pending
- **Details**: JS accessing `uvCoords[idx]` out-of-bounds returns `undefined` → `NaN`, producing no visible lines (safe, just garbage rendering). C++ (lines 71–76) accessing `uvCoords[idx]` out-of-bounds via `operator[]` is undefined behavior (potential crash/corruption). No guard exists for this case.

### 173. [build-version.cpp] `find_version_in_buffer` search range off-by-4
- **JS Source**: `src/js/mpq/build-version.js` lines 51–69
- **Status**: Pending
- **Details**: JS `buf.indexOf(sig_bytes, pos)` searches the entire remaining buffer from `pos` onward and can find signatures at any position; `parse_vs_fixed_file_info` then validates that at least 52 bytes remain from the match position. C++ (lines 81–100) uses `std::search(buf.begin() + pos, buf.end() - 52, sig_bytes.begin(), sig_bytes.end())`, which limits the search endpoint to `buf.end() - 52`. For a 4-byte signature pattern, `std::search` can only find matches starting at positions up to `buf.end() - 56`. This misses valid signature positions `buf.size() - 55` through `buf.size() - 52` where `parse_vs_fixed_file_info` would succeed (since `offset + 52 <= buf.size()` holds for all four). The search endpoint should be `buf.end() - 48` to allow matches up to position `buf.size() - 52`.

### 174. [mpq.cpp] `extractFileByBlockIndex` TODO: encrypted file support incomplete
- **JS Source**: `src/js/mpq/mpq.js` lines 600–601
- **Status**: Pending
- **Details**: Both JS (`return null; // todo: MPQ_FILE_FIX_KEY?`) and C++ (line 698, `return std::nullopt; // todo: MPQ_FILE_FIX_KEY?`) return null/nullopt for encrypted files in `extractFileByBlockIndex` without attempting decryption. The full `extractFile` method handles encrypted files via filename-based key derivation, but `extractFileByBlockIndex` lacks the filename needed for key computation. The TODO comment indicates this is acknowledged unfinished work, consistent between both versions.

### 175. [mpq.cpp] `inflateData` uses `spdlog::error` instead of project logging module
- **JS Source**: `src/js/mpq/mpq.js` line 422
- **Status**: Pending
- **Details**: JS uses `console.error('decompression error:', e)` for error logging in the zlib decompression catch block. C++ (line 511) uses `spdlog::error("decompression error: {}", e.what())` directly instead of the project's `logging::write` function that is used consistently throughout the rest of the codebase (including elsewhere in mpq.cpp via the logging header). This is inconsistent with the project's logging convention.

### 176. [module_test_a.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/modules/module_test_a.js` lines 1–34
- **Status**: Pending
- **Details**: `module_test_a.cpp` is byte-identical to `module_test_a.js`. The file contains `module.exports`, a Vue template string, and JavaScript `data()` / `methods` — it is pure JavaScript with no C++ conversion. The file needs to be fully converted to C++ or removed if the test module is not needed in the C++ port.

### 177. [module_test_b.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/modules/module_test_b.js` lines 1–43
- **Status**: Pending
- **Details**: `module_test_b.cpp` is byte-identical to `module_test_b.js`. The file contains `module.exports`, a Vue template string, and JavaScript `data()` / `methods` / `mounted()` — it is pure JavaScript with no C++ conversion. The file needs to be fully converted to C++ or removed if the test module is not needed in the C++ port.

### 178. [tab_blender.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/modules/tab_blender.js` lines 1–171
- **Status**: Pending
- **Details**: `tab_blender.cpp` is byte-identical to `tab_blender.js` (171 lines). The file contains `require()` calls, `nw.Shell.openItem()`, `async/await`, `module.exports`, a Vue template, and JavaScript methods. No C++ conversion has been performed. The entire module — including Blender addon download logic, version checking, and the settings UI — needs to be ported to C++.

### 179. [tab_changelog.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/modules/tab_changelog.js` lines 1–53
- **Status**: Pending
- **Details**: `tab_changelog.cpp` is byte-identical to `tab_changelog.js` (53 lines). The file contains `require('fs').promises`, `require('../log')`, `async/await`, `module.exports`, and a Vue template with `MarkdownContent` component. No C++ conversion has been performed. The changelog file reading and markdown rendering need to be ported to C++.

### 180. [tab_help.cpp] Entire file is unconverted JavaScript
- **JS Source**: `src/js/modules/tab_help.js` lines 1–174
- **Status**: Pending
- **Details**: `tab_help.cpp` is byte-identical to `tab_help.js` (174 lines). The file contains `require('fs').promises`, `require('path')`, `module.exports`, `async mounted()`, a Vue template with `MarkdownContent` component, and JavaScript DOM manipulation. No C++ conversion has been performed. The help article loading, navigation, and markdown rendering need to be ported to C++.

### 181. [tab_decor.cpp] Model export functionality is stubbed out
- **JS Source**: `src/js/modules/tab_decor.js` lines 167–175 (`export_model` call inside export loop)
- **Status**: Pending
- **Details**: In C++ (lines 273–291), the model export code inside the per-file export loop is entirely commented out. The `ExportModelOptions` struct population and `export_model` call are replaced with `(void)data; (void)model_type; (void)file_name; (void)export_path; (void)is_active;` followed by `helper.mark(decor_name, true);`. This means decor export reports success without actually writing any model files. The JS version (line 167) calls `modelViewerUtils.export_model(...)` with full options including geoset masks, WMO group masks, and WMO set masks.

### 182. [tab_audio.cpp] Residual commented-out JavaScript reference code
- **JS Source**: `src/js/modules/tab_audio.js` lines 56–59, 73, 94–97
- **Status**: Pending
- **Details**: C++ lines 90–92 contain `// file_data = await core.view.casc.getFile(selected_file_data_id);`, line 208 contains `// export_data = await core.view.casc.getFileByName(file_name);`, and lines 465–467 contain commented-out Vue template spans (`// <span>{{ $core.view.soundPlayerSeekFormatted }}</span>` etc.). These are leftover JavaScript reference comments from the conversion process. While the C++ implementation following each comment is correct, the residual JS code should be cleaned up for clarity.

### 183. [screen_settings.cpp] Residual commented-out Vue template code
- **JS Source**: `src/js/modules/screen_settings.js` lines ~460–462 (template buttons)
- **Status**: Pending
- **Details**: C++ lines 482–484 contain commented-out Vue template code: `// <input type="button" value="Discard" @click="handle_discard"/>`, `// <input type="button" value="Apply" @click="handle_apply"/>`, `// <input type="button" id="config-reset" value="Reset to Defaults" @click="handle_reset"/>`. The ImGui equivalents are properly implemented on lines 486–493, but the residual HTML/Vue template comments should be removed.

### 184. [DBDParser.cpp] TODO placeholder: foreign key support not implemented
- **JS Source**: `src/js/db/DBDParser.js` line 342
- **Status**: Pending
- **Details**: C++ line 379 contains `// TODO: Support foreign key support.` carried over from the original JS source (line 342). The `parseColumnChunk` method reads the column foreign key match group (`<TableName::ColumnName>`) but discards it (C++ line 372 has the commented-out capture `//const std::string columnForeignKey = match[2].str();`). This matches the JS behavior exactly — the JS also captures but discards the foreign key (JS line 338). Both C++ and JS are missing this feature identically.

### 185. [WDCReader.cpp] TODO placeholder: string vs locstring not differentiated
- **JS Source**: `src/js/db/WDCReader.js` line 42
- **Status**: Pending
- **Details**: C++ line 89 contains `// TODO: Handle string separate to locstring in the event we need it.` carried over from the original JS source (line 42). The `convertDBDToSchemaType` function treats both `"string"` and `"locstring"` DBD types identically, mapping them both to `FieldType::String`. This matches the JS behavior exactly — the JS also maps both to `FieldType.String` (JS line 43). If localized string handling is ever needed, both would need to be updated.

### 186. [WDCReader.cpp] TODO placeholder: WDC4 chunk data not fully read
- **JS Source**: `src/js/db/WDCReader.js` line 429
- **Status**: Pending
- **Details**: C++ line 497 contains `// New WDC4 chunk: TODO read` carried over from the original JS source (line 429). For WDC versions > 3 (WDC4, WDC5), both JS and C++ read and skip over this chunk data without parsing it (`data.move(entryCount * 4)` in JS; `dataRef.move(static_cast<int64_t>(entryCount) * 4)` in C++). The chunk's purpose is not documented, and the data is discarded. This matches the JS behavior exactly.

## src/js/db/caches Audit

### 187. [DBCharacterCustomization.cpp] tfd_map uses record ID instead of explicit FileDataID field
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` line 55
- **Status**: Pending
- **Details**: JS line 55 reads `tfd_row.FileDataID` explicitly from the TextureFileData row and stores it as the map value: `tfd_map.set(tfd_row.MaterialResourcesID, tfd_row.FileDataID)`. C++ line 97 instead uses the row's map key `_id` as the value: `tfd_map[matResID] = _id;`, never reading the `"FileDataID"` field from the row data. For the TextureFileData table the record ID happens to be the FileDataID, so this is correct in practice, but it is an implicit assumption that deviates from the explicit field access in the original JS.

### 188. [DBCharacterCustomization.cpp] Missing concurrent-initialization guard (init_promise pattern)
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 37–46, 208
- **Status**: Pending
- **Details**: JS `ensureInitialized` (lines 37–46) stores and returns `init_promise` so that concurrent async callers await the same initialization promise, preventing double-initialization. After completion, line 208 sets `init_promise = null`. C++ `ensureInitialized` (lines 294–299) simply calls `_initialize()` synchronously with no re-entrancy guard, mutex, or flag to prevent concurrent calls. While acceptable for single-threaded use, this deviates from the JS pattern which explicitly guards against concurrent initialization.

### 189. [DBCreatures.cpp] getFileDataIDByDisplayID returns 0 instead of undefined equivalent
- **JS Source**: `src/js/db/caches/DBCreatures.js` line 88
- **Status**: Pending
- **Details**: JS `getFileDataIDByDisplayID` (line 88) returns `displayIDToFileDataID.get(displayID)`, which yields `undefined` when the key is not found. C++ (line 143) returns `0` when the key is not found. This means callers cannot distinguish "not found" from "found with FileDataID=0". A `std::optional<uint32_t>` return type (as used elsewhere in the codebase, e.g., `get_chr_model_id` in DBCharacterCustomization) would be a more faithful port.

### 190. [DBCreatures.cpp] extraGeosets always present on struct vs conditionally added in JS
- **JS Source**: `src/js/db/caches/DBCreatures.js` lines 57–61
- **Status**: Pending
- **Details**: JS only adds the `extraGeosets` property to a display object when `modelIDHasExtraGeosets` is true (lines 57–61). When false, `display.extraGeosets` is `undefined`, allowing callers to check `if (display.extraGeosets)` to determine if extra geosets apply. In C++ (`DBCreatures.h` line 19), `std::vector<uint32_t> extraGeosets` is always present (default-constructed to empty). Callers must use `.empty()` instead, which cannot distinguish "model has no extra geosets configured" from "model supports extra geosets but this display has none" (JS line 58: `display.extraGeosets = Array()`).

### 191. [DBCreaturesLegacy.cpp] model_id fallback uses == 0 instead of JS ?? (nullish coalescing)
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` line 69
- **Status**: Pending
- **Details**: JS line 69 uses `row.ModelID ?? row.field_1` — the `??` operator only falls through on `null`/`undefined`, keeping `0` as a valid model ID. C++ lines 137–145 use `if (model_id == 0)` to trigger the fallback, which incorrectly treats a valid model_id of `0` as missing and falls through to `field_1`. This is a semantic difference in nullish coalescing translation.

### 192. [DBCreaturesLegacy.cpp] normalizePath converts .mdl to .m2 beyond JS behavior
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` line 129
- **Status**: Pending
- **Details**: JS `getCreatureDisplaysByPath` (line 129) only normalizes `.mdx` to `.m2`: `normalized.replace(/\.mdx$/i, '.m2')`. C++ `normalizePath` (line 55) additionally converts `.mdl` to `.m2`: `if (ext == ".mdl" || ext == ".mdx")`. This adds behavior not present in the original JS and could match model paths that the JS would not match.

### 193. [DBCreaturesLegacy.cpp] Missing stack trace log in error handler
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 109–110
- **Status**: Pending
- **Details**: JS error handler logs both the error message (line 109: `log.write('Failed to load legacy creature data: %s', e.message)`) and the stack trace (line 110: `log.write('%o', e.stack)`). C++ (line 214) only logs `e.what()` and omits the stack trace. C++ exceptions do not carry stack traces by default, but the omission should be documented.

### 194. [DBCreaturesLegacy.cpp] Texture fallback uses .empty() instead of JS ?? semantics
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 76–78
- **Status**: Pending
- **Details**: JS lines 76–78 use `??` for texture fallback chains: `row.TextureVariation?.[0] ?? row.Skin1 ?? row.field_6 ?? ''`. The `??` operator only falls through on `null`/`undefined`, keeping an empty string `""` as a valid value. C++ lines 165–188 use `.empty()` to trigger fallbacks, which also falls through on empty strings. If a texture field has an intentional empty string value, JS would keep it while C++ would try the next fallback.

### 195. [DBDecor.cpp] Uses row.at() for mandatory fields which throws on missing keys
- **JS Source**: `src/js/db/caches/DBDecor.js` lines 22–34
- **Status**: Pending
- **Details**: JS accesses row fields via property access (`row.ModelFileDataID`, `row.Name_lang`) which returns `undefined` gracefully when a field is absent. C++ lines 47 and 54 use `row.at("ModelFileDataID")` and `row.at("Name_lang")` which throw `std::out_of_range` if the field does not exist. Other fields on lines 59–72 correctly use `row.find()` with fallback. This inconsistency means C++ will crash on rows with missing mandatory fields that JS would handle gracefully.

### 196. [DBDecorCategories.cpp] decor_id fallback uses == 0 instead of JS ?? (nullish coalescing)
- **JS Source**: `src/js/db/caches/DBDecorCategories.js` line 34
- **Status**: Pending
- **Details**: JS line 34 uses `row.HouseDecorID ?? row.DecorID` — the `??` operator only falls through on `null`/`undefined`, keeping `0` as a valid decor ID. C++ line 82 uses `if (decor_id == 0)` to trigger the fallback to `DecorID`, which incorrectly treats a valid HouseDecorID of `0` as missing.

### 197. [DBDecorCategories.cpp] Skip condition uses == 0 instead of JS === undefined
- **JS Source**: `src/js/db/caches/DBDecorCategories.js` line 37
- **Status**: Pending
- **Details**: JS line 37 uses `if (decor_id === undefined || sub_id === undefined)` to skip entries where fields are genuinely absent. C++ line 93 uses `if (decor_id == 0)` which also skips entries where the decor_id is legitimately `0`. This is a semantic difference — JS allows `0` as a valid ID, C++ does not.

### 198. [DBItemCharTextures.cpp] Passes 0 instead of null for race_id/gender_index "no preference"
- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 98, 122, 131
- **Status**: Pending
- **Details**: JS `get_textures_by_display_id` defaults `race_id` and `gender_index` to `null` (lines 98, 122). When `null` is passed to `getTextureForRaceGender`, all race/gender comparisons (`info.raceID === null`) fail, causing the function to skip to generic "any race" fallbacks. C++ lines 118–119 convert the `-1` sentinel (used for "no preference") to `0` via `(race_id >= 0) ? static_cast<uint32_t>(race_id) : 0`, then passes `0` to `getTextureForRaceGender`. With `race_id=0`, the function matches "exact race=0 + gender=0" entries first (male-specific any-race), whereas JS with `null` would skip all race/gender matching and fall through to generic "any raceID=0" entries regardless of gender. This produces different texture selection results.

### 199. [DBItemModels.cpp] Missing filter(Boolean) equivalent in getItemModels
- **JS Source**: `src/js/db/caches/DBItemModels.js` line 120
- **Status**: Pending
- **Details**: JS line 120 uses `data.modelOptions.map(opts => opts[0]).filter(Boolean)` which removes all falsy values from the result, including `0` and `undefined`. C++ lines 229–231 only skip entries where `opts` is empty (`if (!opts.empty()) temp_models.push_back(opts[0])`), but does not filter out entries where `opts[0] == 0`. If any `modelOptions` sub-array has a first element of `0`, JS would exclude it but C++ would include it.

### 200. [DBItemModels.cpp] getItemModels uses thread_local static vector instead of returning new value
- **JS Source**: `src/js/db/caches/DBItemModels.js` line 120
- **Status**: Pending
- **Details**: JS line 120 returns a newly-allocated array each call. C++ lines 227–236 return a pointer to a `thread_local std::vector` that is cleared and reused on each call. The caller must consume the result before calling `getItemModels` again on the same thread, or the data is silently overwritten. This is a semantic deviation — JS always returns an independent value, while C++ returns a pointer to shared mutable storage.

### 201. [DBItems.cpp] name.empty() replaces empty strings unlike JS ?? which preserves them
- **JS Source**: `src/js/db/caches/DBItems.js` line 40
- **Status**: Pending
- **Details**: JS line 40 uses `item_row.Display_lang ?? 'Unknown item #' + item_id` — the `??` operator only substitutes on `null`/`undefined`, keeping an empty string `""` as the item name. C++ line 64 uses `name.empty() ? std::format("Unknown item #{}", item_id) : std::move(name)` which replaces empty strings with the fallback text. Items with an intentionally empty `Display_lang` would show `""` in JS but `"Unknown item #N"` in C++.

### 202. [DBItems.cpp] row.at("Display_lang") throws on missing field unlike JS graceful undefined
- **JS Source**: `src/js/db/caches/DBItems.js` line 40
- **Status**: Pending
- **Details**: JS line 40 accesses `item_row.Display_lang` which returns `undefined` gracefully when the field is absent, then `??` substitutes the fallback. C++ line 63 uses `item_row.at("Display_lang")` which throws `std::out_of_range` if the `Display_lang` field does not exist in the row map. If any ItemSparse row lacks a `Display_lang` field, C++ crashes; JS handles it gracefully.

## src/js/components/ Audit

### 203. [checkboxlist.cpp] `resize()` called every frame instead of only on layout change
- **JS Source**: `src/js/components/checkboxlist.js` lines 28–38
- **Status**: Pending
- **Details**: In JS, `resize()` is called only when the `ResizeObserver` fires (actual DOM resize). In C++ (line 149), `resize()` is called unconditionally every frame inside `render()`. This continuously overwrites `state.scroll` with `(containerHeight - scrollerHeight) * scrollRel`, potentially preventing user-initiated scroll from persisting between frames.

### 204. [checkboxlist.cpp] `itemWeight` returns 0.0f instead of JS Infinity when list is empty
- **JS Source**: `src/js/components/checkboxlist.js` line 83
- **Status**: Pending
- **Details**: JS returns `1 / this.items.length` which yields `Infinity` when `items.length === 0`. C++ (lines 56–58) returns `0.0f` when items is empty. Downstream arithmetic in `wheelMouse` will behave differently with an empty list.

### 205. [checkboxlist.cpp] `recalculateBounds` division-by-zero guard diverges from JS
- **JS Source**: `src/js/components/checkboxlist.js` line 105
- **Status**: Pending
- **Details**: JS sets `this.scrollRel = this.scroll / max` which produces `Infinity`/`NaN` when `max === 0`. C++ (line 77) guards with `(max > 0.0f) ? (state.scroll / max) : 0.0f`. Added safety guard changes behavior when container and scroller heights are equal.

### 206. [checkboxlist.cpp] `wheelMouse` uses hardcoded itemHeight instead of querying actual child height
- **JS Source**: `src/js/components/checkboxlist.js` lines 142–145
- **Status**: Pending
- **Details**: JS dynamically queries `this.$el.querySelector('.item').clientHeight` to compute `scrollCount`. C++ (line 143) uses hardcoded `const float itemHeight = 26.0f`. If the actual rendered item height ever differs from 26px (due to styling, DPI, or font size), the JS would adapt while the C++ would not.

### 207. [checkboxlist.cpp] Custom scrollbar styling does not replicate CSS `.scroller` appearance
- **JS Source**: `src/js/components/checkboxlist.js` line 171
- **Status**: Pending
- **Details**: C++ (lines 179–201) uses hardcoded 8px-wide scrollbar, 4.0f corner rounding, TEXT_ACTIVE_U32/TEXT_IDLE_U32 colors. No track background is drawn. The original CSS `.scroller` likely specifies different width, colors, and may include a track/background. The `using` class (hover/active state styling) is reduced to just a color toggle.

### 208. [checkboxlist.cpp] Alternate row background uses ROW_HOVER_U32 instead of even-row color
- **JS Source**: `src/js/components/checkboxlist.js` line 172
- **Status**: Pending
- **Details**: JS uses CSS `:nth-child(even)` styling applied via external stylesheet. C++ (lines 214–218) uses `app::theme::ROW_HOVER_U32`. The constant name suggests it's a hover color, not an alternate-row background color. The original CSS nth-child(even) would use a distinct background color.

### 209. [combobox.cpp] `watchValue` does not call `selectOption` / does not emit `onChange`
- **JS Source**: `src/js/components/combobox.js` lines 19–24
- **Status**: Pending
- **Details**: JS Watch calls `this.selectOption(this.source.find(...))`, which may emit `update:value` (e.g., if the item isn't found, it emits null). C++ (lines 86–110) `watchValue` manually sets `currentText` and `isActive` but never calls `selectOption` or invokes `onChange`. When value changes externally and the matching source item is not found, JS emits null back via `selectOption`; C++ silently clears `currentText` without notifying the caller.

### 210. [combobox.cpp] `mounted()` initialization not explicitly ported
- **JS Source**: `src/js/components/combobox.js` lines 27–31
- **Status**: Pending
- **Details**: JS `mounted()` calls `selectOption` if `value !== null`, else clears text. C++ relies on `watchValue` being called each frame. If the initial value is non-null, `watchValue` will fire but won't call `onChange` (see #209).

### 211. [combobox.cpp] `onBlur` 200ms delay replaced with hover detection
- **JS Source**: `src/js/components/combobox.js` lines 68–71
- **Status**: Pending
- **Details**: JS uses `setTimeout(() => { this.isActive = false; }, 200)` to delay deactivation by 200ms, allowing dropdown clicks to register. C++ (lines 194–196) uses `!inputActive && !ImGui::IsAnyItemHovered()` which may close the dropdown prematurely or fail to close it in edge cases. `IsAnyItemHovered()` checks all ImGui items, not just dropdown items.

### 212. [combobox.cpp] Dropdown max height hardcoded to 200px
- **JS Source**: `src/js/components/combobox.js` lines 87–93
- **Status**: Pending
- **Details**: The JS `<ul>` has no explicit max-height in the template; it's controlled entirely by CSS. C++ (line 175) hardcodes `std::min(..., 200.0f)` as max dropdown height. If the CSS specifies a different max height, this won't match.

### 213. [combobox.cpp] `InputText` buffer management is fragile / potential UB
- **JS Source**: `src/js/components/combobox.js` line 89
- **Status**: Pending
- **Details**: C++ (line 128) uses `ImGui::InputText("##input", &state.currentText[0], state.currentText.capacity() + 1, ...)`. When `state.currentText` is empty, `&state.currentText[0]` may be undefined behavior. Modern ImGui provides `ImGui::InputText` overloads that accept `std::string*` directly (via `imgui_stdlib.h`), which would be safer.

### 214. [combobox.cpp] Placeholder text rendered manually with hardcoded offsets
- **JS Source**: `src/js/components/combobox.js` line 89
- **Status**: Pending
- **Details**: JS uses standard HTML `placeholder` attribute. C++ (lines 144–151) draws placeholder via `AddText` with hardcoded offsets `+4.0f, +2.0f`. The positioning offset may not match the actual input text position across different DPI/font settings.

### 215. [context-menu.cpp] `@mouseleave` close behavior replaced with click-outside
- **JS Source**: `src/js/components/context-menu.js` line 54
- **Status**: Pending
- **Details**: JS menu closes immediately when the mouse leaves the menu div (`@mouseleave="$emit('close')"`). C++ (lines 94–98) only closes when clicking outside the window (`!ImGui::IsWindowHovered(...) && ImGui::IsMouseClicked(0)`). This is a significant behavioral difference — JS: hover-out closes; C++: click-outside closes.

### 216. [context-menu.cpp] `@click` on container div not fully ported
- **JS Source**: `src/js/components/context-menu.js` line 54
- **Status**: Pending
- **Details**: In JS, any click anywhere inside the `<div class="context-menu">` emits `close`. C++ (lines 100–102) says "handled by individual Selectable items" but no general click handler exists. If `contentCallback` renders non-Selectable elements (text, separators), clicking them won't close the menu in C++.

### 217. [context-menu.cpp] `context-menu-zone` div not implemented
- **JS Source**: `src/js/components/context-menu.js` line 55
- **Status**: Pending
- **Details**: JS has a `<div class="context-menu-zone"></div>` — an invisible zone that extends the hover area between the trigger and menu, preventing premature close. C++ (lines 85–86) acknowledges this in a comment but does not implement it.

### 218. [context-menu.cpp] Window ID collision for multiple instances
- **JS Source**: `src/js/components/context-menu.js` — each Vue component instance is independent
- **Status**: Pending
- **Details**: C++ (line 84) uses `ImGui::Begin("##context_menu_popup", ...)`. The `PushID(id)` on line 61 does NOT scope `Begin()` window names. If `render()` is called with different `id` values for multiple context menus, they all create/fight over the same window `"##context_menu_popup"`.

### 219. [data-table.cpp] `moveMouse` missing `manuallyResizedColumns` update during resize drag
- **JS Source**: `src/js/components/data-table.js` lines 572–576
- **Status**: Pending
- **Details**: In JS, both `columnWidths[i]` and `manuallyResizedColumns[columnName]` are updated during the drag inside the `requestAnimationFrame` callback. C++ (lines 453–462) only updates `state.columnWidths`; `state.manuallyResizedColumns` is not updated until `stopMouse`. During the drag, the column is resized visually but not recorded as "manually resized," which could cause `calculateColumnWidths` to overwrite the in-progress width if headers change mid-drag.

### 220. [data-table.cpp] `syncScrollPosition` completely omitted
- **JS Source**: `src/js/components/data-table.js` lines 515–527
- **Status**: Pending
- **Details**: JS syncs the custom scrollbar position with the native scroll position (handles native scroll events on the root div). C++ (line 422) has a comment saying "is fully managed by our custom scrollbar logic" but the function body is entirely absent.

### 221. [data-table.cpp] `ContextMenuEvent` struct missing mouse position data
- **JS Source**: `src/js/components/data-table.js` lines 883–889
- **Status**: Pending
- **Details**: JS emits `{ rowIndex, columnIndex, cellValue, selectedCount, event }` — the mouse `event` object is included, which consumers use to position the context menu. C++ `ContextMenuEvent` (data-table.h lines 38–43) has `rowIndex`, `columnIndex`, `cellValue`, `selectedCount` but no mouse position data. Consumers cannot determine where to display the context menu.

### 222. [data-table.cpp] Status text missing locale/thousands-separator formatting
- **JS Source**: `src/js/components/data-table.js` lines 1018–1019
- **Status**: Pending
- **Details**: JS uses `.toLocaleString()` to format numbers with thousands separators (e.g., "1,234,567"). C++ (lines 1320–1323) uses `std::to_string()` which produces "1234567" without separators.

### 223. [data-table.cpp] `escape_value` in `getSelectedRowsAsSQL` treats empty string as NULL
- **JS Source**: `src/js/components/data-table.js` lines 950–951
- **Status**: Pending
- **Details**: JS only returns `'NULL'` for `null` and `undefined`; an empty string `""` would be escaped as `''`. C++ (lines 826–827) `if (val.empty()) return "NULL";` treats empty strings as SQL NULL, which differs from JS behavior.

### 224. [data-table.cpp] Row selected/hover color constants appear swapped
- **JS Source**: `src/js/components/data-table.js` line 1011
- **Status**: Pending
- **Details**: C++ (line 1187) uses `app::theme::TABLE_ROW_HOVER_U32` for selected rows; hover effect on non-selected rows at line 1200 uses `app::theme::TABLE_ROW_SELECTED_U32`. The constant names are semantically swapped.

### 225. [data-table.cpp] `sortedItems` uses unstable sort
- **JS Source**: `src/js/components/data-table.js` line 170
- **Status**: Pending
- **Details**: Modern JS engines use stable sort (TimSort). C++ (line 294) uses `std::sort(...)` which is not guaranteed stable (typically introsort). Should use `std::stable_sort` to match JS behavior for rows with equal sort keys.

### 226. [data-table.cpp] `handleKey` focus check semantic difference
- **JS Source**: `src/js/components/data-table.js` line 778
- **Status**: Pending
- **Details**: JS checks `if (document.activeElement !== document.body) return;` — only intercepts keys when nothing is focused. C++ (line 663) checks `if (ImGui::IsAnyItemActive()) return;`. These are conceptually similar but `IsAnyItemActive` may behave differently when a child window is focused but no item is active.

### 227. [data-table.cpp] Rows watcher change detection may miss in-place mutations
- **JS Source**: `src/js/components/data-table.js` lines 316–324
- **Status**: Pending
- **Details**: JS Vue reactivity watches the `rows` prop reference; any change to the array triggers the handler. C++ (lines 897–904) checks `rows.size()` and `rows.data()` pointer. This can miss changes if rows are mutated in-place without changing size or base pointer (e.g., editing cell content).

### 228. [file-field.cpp] Extra `openFileDialog()` and `saveFileDialog()` functions not in JS
- **JS Source**: `src/js/components/file-field.js`
- **Status**: Pending
- **Details**: JS only has directory picker functionality (input with `nwdirectory`). C++ (lines 106–300) adds `openFileDialog()` (~95 lines) and `saveFileDialog()` (~100 lines) with file type filters, default directory. These ~195 lines of code have no JS equivalent and are entirely new functionality.

### 229. [file-field.cpp] Dialog opens on button click instead of on input focus
- **JS Source**: `src/js/components/file-field.js` line 46
- **Status**: Pending
- **Details**: JS renders a single `<input type="text">` that opens the dialog on focus (`@focus="openDialog"`). C++ (lines 317–361) renders an `ImGui::InputText` + a separate "..." browse button. The dialog opens on button click, not on input focus. This changes user interaction flow.

### 230. [file-field.cpp] Missing `$el.blur()` equivalent after dialog
- **JS Source**: `src/js/components/file-field.js` line 39
- **Status**: Pending
- **Details**: JS calls `this.$el.blur()` to blur the input after opening the dialog. C++ (lines 301–308) has no equivalent `ImGui::ClearActiveID()` or similar blur call after dialog selection.

### 231. [home-showcase.cpp] ENTIRE FILE IS UNCONVERTED JAVASCRIPT
- **JS Source**: `src/js/components/home-showcase.js` lines 1–65
- **Status**: Pending
- **Details**: The `.cpp` file contains identical JavaScript code — `const showcases = require(...)`, `module.exports = { ... }`, Vue template syntax, etc. Zero conversion has been done. All elements need porting: `require('../showcase.json')` parsing, `BASE_LAYERS` constant array, `get_random_index()`, `build_background_style()`, Vue `data()`/`computed`/`methods`, HTML template with `<h1>`, `<a>`, `<video>`, `<span>`, click handlers, external link handling, and video playback. No `.h` header file exists either.

### 232. [itemlistbox.cpp] Item height hardcoded to 26px vs dynamic DOM query in JS
- **JS Source**: `src/js/components/itemlistbox.js` lines 201–209
- **Status**: Pending
- **Details**: JS queries `this.$refs.root.querySelector('.item').clientHeight` dynamically. C++ (lines 125–135, 435) hardcodes `itemHeightVal` to `26.0f`. Since itemlistbox items include 32px icons plus padding/margins, the actual rendered row height is likely larger than 26px, meaning scroll-wheel step size would differ.

### 233. [itemlistbox.cpp] `itemWeight` returns 0.0f instead of JS Infinity when list is empty
- **JS Source**: `src/js/components/itemlistbox.js` lines 143–145
- **Status**: Pending
- **Details**: JS returns `1 / this.filteredItems.length` which yields `Infinity` when length is 0. C++ (lines 68–72) returns `0.0f` when empty. This affects scroll calculations when the list is empty.

### 234. [itemlistbox.cpp] `recalculateBounds` division-by-zero protection differs from JS
- **JS Source**: `src/js/components/itemlistbox.js` lines 162–166
- **Status**: Pending
- **Details**: JS sets `scrollRel = scroll / max` with no guard, producing `NaN`/`Infinity` when `max == 0`. C++ (lines 87–91) guards with `(max > 0.0f) ? (state.scroll / max) : 0.0f`. Defensive but deviates from JS behavior.

### 235. [itemlistbox.cpp] Item ID `<span>` loses separate CSS styling when rendered inline
- **JS Source**: `src/js/components/itemlistbox.js` line 335
- **Status**: Pending
- **Details**: JS wraps the item ID in `<span class="item-id">({{ item.id }})</span>` which likely has distinct styling (e.g., different opacity or color). C++ (line 559) renders `item.name + " (" + std::to_string(item.id) + ")"` as one string, losing per-sub-field styling.

### 236. [itemlistbox.cpp] Quality color values unverified against CSS
- **JS Source**: `src/js/components/itemlistbox.js` line 334
- **Status**: Pending
- **Details**: JS uses CSS classes `.item-quality-0` through `.item-quality-7` via `:class="'item-quality-' + item.quality"`. C++ (lines 389–401) hardcodes `ImVec4` color values. These should be verified against the actual CSS color definitions in `app.css`.

### 237. [itemlistbox.cpp] Odd rows get explicit BG_DARK instead of transparent
- **JS Source**: `src/js/components/itemlistbox.js` template
- **Status**: Pending
- **Details**: JS only sets background on even rows (via CSS `:nth-child(even)`); odd rows are transparent (inherit parent background). C++ (lines 526–529) explicitly sets `BG_DARK_U32` on odd rows. If `BG_DARK_U32` doesn't match the parent container's background color, this could produce a visible difference.

### 238. [itemlistbox.cpp] Selectable width hardcoded to `availSize.x - 120.0f`
- **JS Source**: `src/js/components/itemlistbox.js` lines 333–340
- **Status**: Pending
- **Details**: JS uses CSS flex to let the item name fill available space dynamically. C++ (line 563) hardcodes `ImVec2(availSize.x - 120.0f, 0.0f)` for reserved button width, which may not match the actual button width and could cause misalignment or clipping.

### 239. [listbox-maps.cpp] Missing `recalculateBounds()` call on expansion filter change
- **JS Source**: `src/js/components/listbox-maps.js` lines 27–31
- **Status**: Pending
- **Details**: JS calls `this.recalculateBounds()` after resetting scroll. C++ (lines 91–95) only resets `scroll` and `scrollRel` to 0, no `recalculateBounds`. The JS `recalculateBounds()` also saves scroll position via `core.saveScrollPosition` when `persistscrollkey` is set. Scroll position persistence is not saved when the expansion filter changes.

### 240. [listbox-maps.cpp] Expansion filtering applied before override resolution (order of operations differs)
- **JS Source**: `src/js/components/listbox-maps.js` lines 44–45
- **Status**: Pending
- **Details**: In JS, `this.itemList` resolves to `this.override?.length > 0 ? this.override : this.items` first, then expansion filtering is applied to the resolved list. In C++, expansion filtering is applied to raw `items` before passing to `listbox::render()` which may apply its own override logic. If `overrideItems` is provided, the C++ would discard the expansion-filtered items entirely and use overrideItems. Behavioral difference when both override and expansion filter are active.

### 241. [listbox-zones.cpp] Missing `recalculateBounds()` call on expansion filter change
- **JS Source**: `src/js/components/listbox-zones.js` lines 27–31
- **Status**: Pending
- **Details**: Same issue as #239 for listbox-maps. JS calls `this.recalculateBounds()` after resetting scroll. C++ (lines 91–95) only resets scroll values without calling recalculateBounds. Scroll position persistence is not saved.

### 242. [listbox-zones.cpp] Expansion filtering applied before override resolution (order of operations differs)
- **JS Source**: `src/js/components/listbox-zones.js` lines 44–45
- **Status**: Pending
- **Details**: Same issue as #240 for listbox-maps. Override resolution order differs from JS when both override and expansion filter are active.

### 243. [listbox.cpp] Missing `\31` sub-field rendering with distinct CSS classes
- **JS Source**: `src/js/components/listbox.js` line 507
- **Status**: Pending
- **Details**: JS renders each sub-field (split by `\31`) with its own CSS class (`sub-0`, `sub-1`, etc.) and `data-item` attribute, allowing different styling per sub-field. C++ (lines 731–745) concatenates all sub-fields into a single `displayText` string separated by spaces, then renders as one `ImGui::Selectable`, losing per-sub-field styling.

### 244. [listbox.cpp] Missing `update:filter` emit
- **JS Source**: `src/js/components/listbox.js` line 41
- **Status**: Pending
- **Details**: JS declares `emits: ['update:selection', 'update:filter', 'contextmenu']`. C++ has no callback parameter or mechanism for `update:filter`. If any parent component relies on this event, it will not receive updates.

### 245. [listbox.cpp] Scroll position restoration skips recalculation
- **JS Source**: `src/js/components/listbox.js` lines 84–88, 150–157
- **Status**: Pending
- **Details**: JS scroll position restoration sets `scrollRel`, then computes `this.scroll = (root.clientHeight - scroller.clientHeight) * scrollRel` and calls `recalculateBounds()`. C++ (lines 615–621) only sets `state.scrollRel` from saved state without recalculating `state.scroll` or calling `recalculateBounds()`.

### 246. [listbox.cpp] `activated` / `deactivated` lifecycle (keep-alive) not ported
- **JS Source**: `src/js/components/listbox.js` lines 97–113
- **Status**: Pending
- **Details**: JS `activated()` adds paste and keydown listeners; `deactivated()` removes them, modeling Vue keep-alive component activation. C++ has no equivalent activation/deactivation mechanism. Paste and keyboard input are always processed every frame when the component is rendered. If this listbox is inside a tab-switching container, keyboard/paste events may fire when the component is not the active tab.

### 247. [listbox.cpp] `handleKey` not scoped to `document.body` active element check
- **JS Source**: `src/js/components/listbox.js` line 346
- **Status**: Pending
- **Details**: JS checks `if (document.activeElement !== document.body) return;` — only intercepts when nothing is focused. C++ (line 295) checks `if (ImGui::IsAnyItemActive()) return;`, which is conceptually similar but not identical. `IsAnyItemActive()` returns true only when an item is being interacted with, while `document.activeElement !== document.body` returns true when ANY DOM element has focus.

### 248. [listbox.cpp] `filteredItems` recomputed every frame instead of cached
- **JS Source**: `src/js/components/listbox.js` line 193
- **Status**: Pending
- **Details**: JS `filteredItems` is a Vue computed property, cached until dependencies change. C++ (lines 610–612) calls `computeFilteredItems()` every frame unconditionally, even when nothing has changed. For large item lists, recomputing the filter every frame is expensive.

### 249. [listbox.cpp] Quick filter active color hardcoded
- **JS Source**: `src/js/components/listbox.js` line 513
- **Status**: Pending
- **Details**: JS active quick filter gets class `active`, styled by CSS. C++ (line 798) hardcodes `ImVec4(0.13f, 0.71f, 0.29f, 1.0f)` for the active color. Should reference a theme constant from `app.css` rather than a hardcoded value.

### 250. [listboxb.cpp] `handleKey` Ctrl+C copies `[object Object]` in JS vs `.label` in C++
- **JS Source**: `src/js/components/listboxb.js` line 181
- **Status**: Pending
- **Details**: JS `this.selection.join('\n')` on item objects would produce `[object Object]`. C++ (lines 169–175) copies `items[idx].label` for each selected index, producing meaningful text. The C++ version handles this better but deviates from JS behavior.

### 251. [listboxb.cpp] Selection model differs: JS uses item references, C++ uses indices
- **JS Source**: `src/js/components/listboxb.js` lines 14, 230–273
- **Status**: Pending
- **Details**: JS `selection` prop contains item references (objects). `indexOf(item)` searches for the item object. C++ (listboxb.h line 52, listboxb.cpp lines 221–272) uses `std::vector<int>` (indices). If items are reordered or the list changes, index-based selection will point to wrong items. The JS version is reference-based and survives reordering.

### 252. [listboxb.cpp] Alternating row pattern shifts during scrolling
- **JS Source**: `src/js/components/listboxb.js` line 281
- **Status**: Pending
- **Details**: JS rows get alternating background via CSS `:nth-child(even)` based on DOM child position (stable). C++ (lines 373–376) uses `((i - startIdx) % 2 == 0)` based on offset from `startIdx`. As scrolling changes `startIdx`, the even/odd pattern can flip, whereas in JS the pattern is always relative to DOM position.

### 253. [map-viewer.cpp] Missing double-buffer pixel blitting — no canvas shift on pan
- **JS Source**: `src/js/components/map-viewer.js` lines 82–83, 555–627
- **Status**: Pending
- **Details**: JS creates an offscreen canvas element, copies the main canvas to it with offset via `doubleCtx.drawImage(canvas, deltaX, deltaY)`, then copies back. C++ (lines 452–527) tracks the technique structurally but has NO actual pixel-level double-buffer blitting. `deltaX`/`deltaY` are computed but marked `[[maybe_unused]]`. Already-rendered tiles are NOT shifted by the pan delta.

### 254. [map-viewer.cpp] `loadTile` is synchronous in C++, async in JS
- **JS Source**: `src/js/components/map-viewer.js` lines 387–414
- **Status**: Pending
- **Details**: JS tile loading is async (Promise-based) with multiple tiles loading concurrently, managed by `maxConcurrentTiles` and `activeTileRequests`. C++ (lines 279–326) calls the loader synchronously/blocking. The concurrency tracking (`activeTileRequests++`/`--`) happens in the same function call, making it meaningless. Synchronous loading will block the UI thread.

### 255. [map-viewer.cpp] No actual tile texture rendering in `renderWidget`
- **JS Source**: `src/js/components/map-viewer.js` lines 1101–1112
- **Status**: Pending
- **Details**: JS draws tiles via `context.putImageData()` to a canvas. C++ `renderWidget()` (lines 1108–1246) renders info text, an invisible button for interaction, and the overlay, but there is NO code to draw tile textures from `tilePixelCache` to the screen. The map tiles are never actually displayed — the overlay draws selection/hover highlights over empty space. Critical missing functionality.

### 256. [map-viewer.cpp] Overlay color values may differ from JS hardcoded RGBA
- **JS Source**: `src/js/components/map-viewer.js` lines 744, 750, 755
- **Status**: Pending
- **Details**: JS uses hardcoded `rgba(159, 241, 161, 0.5)` for selection and `rgba(87, 175, 226, 0.5)` for hover/box-select. C++ (lines 656, 665) uses `app::theme::FONT_ALT_HIGHLIGHT_U32` and `app::theme::FONT_ALT_U32` with alpha override. The theme constants may not match the hardcoded JS RGBA values. Should be verified against `app.css`.

### 257. [map-viewer.cpp] `handleTileInteraction` does not call `onSelectionChanged` callback
- **JS Source**: `src/js/components/map-viewer.js` lines 846–874
- **Status**: Pending
- **Details**: JS directly mutates `this.selection` which triggers Vue reactivity. C++ (lines 805–839) directly mutates the `selection` vector passed by reference but does NOT call `onSelectionChanged`. The callback is only invoked for box-select and Ctrl+A/D but never for shift-click tile selection. If the caller relies on the callback to know about selection changes, shift-click selection will silently change the vector without notification.

### 258. [map-viewer.cpp] `mapPositionFromClientPoint` may use wrong origin position
- **JS Source**: `src/js/components/map-viewer.js` lines 991–1011
- **Status**: Pending
- **Details**: JS uses `viewport.getBoundingClientRect()` and `canvas.width/height` for coordinate conversion. C++ (lines 969–994) uses `ImGui::GetCursorScreenPos()` for `contentOrigin`. After `InvisibleButton` is drawn, the cursor has advanced, so `GetCursorScreenPos()` may return the wrong position depending on when it's called during the frame.

### 259. [markdown-content.cpp] ENTIRE FILE IS UNCONVERTED JAVASCRIPT
- **JS Source**: `src/js/components/markdown-content.js` lines 1–255
- **Status**: Pending
- **Details**: The `.cpp` file is a byte-for-byte copy of the `.js` file. It contains `module.exports`, Vue lifecycle hooks (`mounted`, `beforeUnmount`), `this.$refs`, `this.$emit`, `document.addEventListener`, `ResizeObserver`, `requestAnimationFrame`, JavaScript template literals, etc. None of this is valid C++. All functionality needs porting: `htmlContent` computed property, `parseMarkdown()` (lines 143–202), `parseInline()` (lines 204–237), `escapeHtml()` (lines 239–248), scrollbar logic, resize observation, and Vue template with `v-html` / `:style` / `@wheel` / `@mousedown` bindings. No `.h` header file exists either.

### 260. [menu-button.cpp] Popup window ID collision for multiple instances
- **JS Source**: `src/js/components/menu-button.js` lines 78–80
- **Status**: Pending
- **Details**: The popup window uses hardcoded ID `"##menu_button_popup"` (line 140). If multiple `menu_button::render()` instances exist in the same frame, they will share the same ImGui window. Should incorporate the widget `id` parameter into the popup ID.

### 261. [menu-button.cpp] Arrow button uses text "v" instead of CSS-styled chevron
- **JS Source**: `src/js/components/menu-button.js` line 77
- **Status**: Pending
- **Details**: JS renders a `<div class="arrow">` styled via CSS (likely with a triangle/chevron icon). C++ (line 116) uses a text button with literal character `"v"`. This will not match the original visual appearance.

### 262. [menu-button.cpp] CSS class states `disabled`, `dropdown`, `open` not replicated
- **JS Source**: `src/js/components/menu-button.js` line 75
- **Status**: Pending
- **Details**: JS applies CSS classes `disabled`, `dropdown`, `open` on the container `<div>`, driving visual styling (hover effects, borders, etc.). C++ relies entirely on ImGui's default disabled styling and has no equivalent for `dropdown` or `open` visual states.

### 263. [menu-button.cpp] Context-menu component replaced with raw ImGui::Begin window
- **JS Source**: `src/js/components/menu-button.js` lines 78–80
- **Status**: Pending
- **Details**: JS uses a `<context-menu>` child component with `@close` event binding. C++ (lines 127–155) replaces this with a raw `ImGui::Begin` window with NoMove/NoTitleBar flags. The visual and behavioral fidelity (focus handling, z-ordering, click-outside dismiss) may differ from the JS context-menu component.

### 264. [menu-button.cpp] `selectedObj` uses index instead of object reference
- **JS Source**: `src/js/components/menu-button.js` line 59
- **Status**: Pending
- **Details**: JS stores the option object itself (`this.selectedObj ?? this.defaultObj`), immune to array reordering. C++ uses `selectedIndex >= 0` check (lines 40–44). If `options` are reordered between frames, the selected index may point to a different option than intended.

### 265. [model-viewer-gl.cpp] `fit_camera_for_character` signature diverges — updates dual controls
- **JS Source**: `src/js/components/model-viewer-gl.js` line 186
- **Status**: Pending
- **Details**: JS takes a single `controls` parameter (duck-typed). C++ (lines 191–218) splits into `CameraControlsGL* orbit_controls` and `CharacterCameraControlsGL* char_controls`, updating both if both are non-null. JS only updates the single passed-in controls object.

### 266. [model-viewer-gl.cpp] `render_scene` rotation guard differs from JS
- **JS Source**: `src/js/components/model-viewer-gl.js` line 240
- **Status**: Pending
- **Details**: JS: `if (rotation_speed !== 0 && activeRenderer && activeRenderer.setTransform && !this.use_character_controls)`. C++ drops the `activeRenderer` and `activeRenderer.setTransform` guards from the outer condition. It also applies rotation via a `context.setActiveModelTransform` fallback path (lines 427–433) that has no JS equivalent, allowing rotation even without an M2 renderer.

### 267. [model-viewer-gl.cpp] Hand grip check missing `activeRenderer.setHandGrip` guard
- **JS Source**: `src/js/components/model-viewer-gl.js` line 277
- **Status**: Pending
- **Details**: JS: `if (activeRenderer && equipment_renderers && activeRenderer.setHandGrip)`. C++ (line 481) omits the `activeRenderer.setHandGrip` existence check, only checking `if (activeRenderer && equipment_renderers)`.

### 268. [model-viewer-gl.cpp] Animation update missing `activeRenderer.updateAnimation` guard
- **JS Source**: `src/js/components/model-viewer-gl.js` line 224
- **Status**: Pending
- **Details**: JS: `if (activeRenderer && activeRenderer.updateAnimation)`. C++ (lines 399–400) only checks `if (activeRenderer)` and unconditionally calls `updateAnimation(deltaTime)`, also omitting the `get_animation_frame` existence check (JS line 229).

### 269. [model-viewer-gl.cpp] `window.devicePixelRatio` not accounted for in FBO sizing
- **JS Source**: `src/js/components/model-viewer-gl.js` lines 482–483
- **Status**: Pending
- **Details**: JS multiplies canvas dimensions by `window.devicePixelRatio` for HiDPI rendering. C++ comment says "ImGui handles DPI internally" but does not apply any DPI scaling to FBO size (lines 778–779). On HiDPI displays, the 3D rendering may appear at lower resolution.

### 270. [model-viewer-gl.cpp] GLContext created without WebGL options equivalent
- **JS Source**: `src/js/components/model-viewer-gl.js` lines 435–438
- **Status**: Pending
- **Details**: JS: `new GLContext(canvas, { antialias: true, alpha: true, preserveDrawingBuffer: true })`. C++ (line 684) creates `gl::GLContext()` with no arguments. The WebGL context options (antialias, alpha, preserveDrawingBuffer) are not passed or configured. While some don't directly apply to desktop GL, `alpha` and multisampling equivalents should be addressed.

### 271. [model-viewer-gl.cpp] `context.controls` split into dual typed pointers
- **JS Source**: `src/js/components/model-viewer-gl.js` line 395
- **Status**: Pending
- **Details**: JS: `this.context.controls = this.controls;` — one untyped reference. C++ maintains separate `context.controls_orbit` and `context.controls_character` pointers (lines 624–636). Parent code accessing `context.controls` in JS must use the appropriate typed pointer in C++. This structural change affects all consumers of the context object.

### 272. [model-viewer-gl.cpp] Extra C++ fallback paths not in JS (`renderActiveModel`, `getActiveBoundingBox`, `setActiveModelTransform`)
- **JS Source**: `src/js/components/model-viewer-gl.js`
- **Status**: Pending
- **Details**: C++ adds `context.renderActiveModel` (lines 525–527), `context.getActiveBoundingBox` (model-viewer-gl.h line 159), and `context.setActiveModelTransform` (model-viewer-gl.h line 167) as fallback paths for non-M2 renderers. JS has no equivalent — it only operates on M2 `activeRenderer`. These are new functionality not present in the original.

### 273. [resize-layer.cpp] Floating-point comparison for width change detection
- **JS Source**: `src/js/components/resize-layer.js` line 13
- **Status**: Pending
- **Details**: JS `ResizeObserver` uses integer `clientWidth` making exact comparison safe. C++ (line 33) compares `currentWidth != state.prevWidth` on floats from `ImGui::GetContentRegionAvail().x`. Floating-point `!=` comparison is fragile due to IEEE rounding. Should use an epsilon or cast to int.

### 274. [resize-layer.cpp] No wrapping container element equivalent
- **JS Source**: `src/js/components/resize-layer.js` line 25
- **Status**: Pending
- **Details**: JS template is `<div><slot></slot></div>`, a wrapper div observed by the `ResizeObserver`. C++ (lines 24–44) has no `ImGui::BeginGroup()`/`EndGroup()` or `BeginChild()`/`EndChild()` wrapping. Width is measured from the parent's content region, not a dedicated wrapper. If content changes the layout, the measured width may not correspond to what the JS wrapper div would report.

### 275. [slider.cpp] Fill bar spans only middle 40% instead of full height
- **JS Source**: CSS `app.css` lines 1267–1274
- **Status**: Pending
- **Details**: CSS `.ui-slider .fill` has `top: 0; bottom: 0;` filling the entire 20px container height. C++ (lines 120–121) draws fill rect from `sliderHeight * 0.3f` to `sliderHeight * 0.7f` (only middle 40%). Visual fidelity error.

### 276. [slider.cpp] Track background spans only middle 40% instead of full height
- **JS Source**: CSS `app.css` lines 1259–1266
- **Status**: Pending
- **Details**: CSS `.ui-slider` is a full 20px-tall box with background and border. C++ (lines 112–114) draws only a narrow stripe in the center. Both the background area and the border are missing.

### 277. [slider.cpp] Track color is wrong — `(80,80,80)` instead of CSS `#2c3136` `(44,49,54)`
- **JS Source**: CSS `app.css` line 1264
- **Status**: Pending
- **Details**: CSS `.ui-slider` background is `var(--background-dark)` = `#2c3136` = `RGB(44, 49, 54)`. C++ `SLIDER_TRACK_U32 = IM_COL32(80, 80, 80, 255)` (app.h line 117) is `RGB(80, 80, 80)`. Significant color mismatch.

### 278. [slider.cpp] Fill color is wrong — green `(34,181,73)` instead of blue `#57afe2` `(87,175,226)`
- **JS Source**: CSS `app.css` line 1273
- **Status**: Pending
- **Details**: CSS `.fill` background is `var(--font-alt)` = `#57afe2` = `RGB(87, 175, 226)` (blue). C++ uses `BUTTON_BASE_U32 = IM_COL32(34, 181, 73, 255)` (green). Major color mismatch.

### 279. [slider.cpp] Handle colors are wrong — default and hover
- **JS Source**: CSS `app.css` lines 1283, 1287–1288
- **Status**: Pending
- **Details**: CSS `.handle` background is `var(--border)` = `#6c757d` = `RGB(108, 117, 125)`. CSS `.handle:hover` is `var(--font-alt)` = `#57afe2` = `RGB(87, 175, 226)`. C++ `SLIDER_THUMB_U32 = IM_COL32(200, 200, 200, 200)` and `SLIDER_THUMB_ACTIVE_U32 = IM_COL32(255, 255, 255, 220)`. Both default and hover handle colors are wrong.

### 280. [slider.cpp] Handle height is 20px instead of CSS 28px — no vertical overhang
- **JS Source**: CSS `app.css` line 1278
- **Status**: Pending
- **Details**: CSS `.handle` height is `28px`, deliberately taller than the 20px container, vertically centered via `transform: translateY(-50%)`. C++ handle (lines 89, 129) equals `sliderHeight` (20px) with no overhang or centering.

### 281. [slider.cpp] Handle horizontal positioning center-aligned instead of left-edge-aligned
- **JS Source**: `src/js/components/slider.js` line 97
- **Status**: Pending
- **Details**: JS handle `left` is `(modelValue * 100) + '%'` — the handle's left edge is at the value position with no `translateX`. C++ (line 127) centers the handle on the value point: `handleX = winPos.x + fillWidth - handleWidth * 0.5f`. At `value=1.0`, JS handle left edge is at 100% while C++ handle center is at 100%, creating positioning difference.

### 282. [slider.cpp] Handle box-shadow, slider border, and slider box-shadow all missing
- **JS Source**: CSS `app.css` lines 1263, 1265, 1282
- **Status**: Pending
- **Details**: CSS `.handle` has `box-shadow: black 0 0 8px`, `.ui-slider` has `border: 1px solid var(--border)` and `box-shadow: black 0 0 1px`. None of these are rendered in the C++ version. Only `AddRectFilled` is used; no border (`AddRect`) or shadow effects exist.

### 283. [slider.cpp] Slider margin missing — no 4px vertical spacing
- **JS Source**: CSS `app.css` line 1262
- **Status**: Pending
- **Details**: CSS `.ui-slider` has `margin: 4px 0`. C++ (lines 86–93) has no spacing or margin before or after the slider.

### 284. [slider.cpp] Track click fires on mousedown instead of click event
- **JS Source**: `src/js/components/slider.js` line 95
- **Status**: Pending
- **Details**: JS `@click="handleClick"` fires on `click` event (mousedown + mouseup on same element). C++ (line 145) uses `ImGui::IsMouseClicked(0)` which fires on the frame the mouse button is pressed down. If the user presses on the track then drags away before releasing, JS does not fire but C++ would have already jumped the value.

### 285. [slider.cpp] Handle hover state persists during entire drag
- **JS Source**: `src/js/components/slider.js` line 97, CSS `app.css` line 1287
- **Status**: Pending
- **Details**: JS handle hover style only applies via CSS `:hover` pseudo-class. When dragging and the cursor leaves the handle, the hover style is lost. C++ (line 130) `handleHovered` is true if hovered OR `state.isScrolling`, so the active color persists during the entire drag even when the cursor is far from the handle.

### 286. [slider.cpp] Missing `cursor: pointer` on handle hover
- **JS Source**: CSS `app.css` line 1284
- **Status**: Pending
- **Details**: CSS `.handle` has `cursor: pointer`. The mouse cursor should change to a pointer when hovering the handle. ImGui supports `ImGui::SetMouseCursor(ImGuiMouseCursor_Hand)` but this is not called in slider.cpp.

## src/js/casc/ Audit

### 287. [blp.cpp] Missing `toCanvas()` method
- **JS Source**: `src/js/casc/blp.js` lines 103–117
- **Status**: Pending
- **Details**: JS `toCanvas(mask, mipmap)` creates an HTML `<canvas>` element at the proper mipmap-scaled dimensions and draws the BLP onto it. This method is entirely absent from C++. It is browser-specific (uses `document.createElement('canvas')`), so it cannot be directly ported but should have a documented equivalent or TODO for an alternative rendering approach.

### 288. [blp.cpp] Missing `drawToCanvas()` method
- **JS Source**: `src/js/casc/blp.js` lines 221–234
- **Status**: Pending
- **Details**: JS `drawToCanvas(canvas, mipmap, mask)` draws BLP pixel data onto an existing HTML canvas via a 2D context. Entirely absent from C++. Browser-specific — uses `canvas.getContext('2d')`, `createImageData`, `putImageData`. Needs a C++ equivalent for rendering BLP data to a texture or framebuffer.

### 289. [blp.cpp] `getDataURL()` implementation differs from JS
- **JS Source**: `src/js/casc/blp.js` lines 94–96
- **Status**: Pending
- **Details**: JS calls `this.toCanvas(mask, mipmap).toDataURL()`, which generates a data URL from an HTML canvas. C++ calls `toPNG(mask, mipmap).getDataURL()`. The output should be equivalent (both produce a PNG-encoded data URL), but the approach differs. This adaptation is undocumented.

### 290. [blp.h] Missing `dataURL` field
- **JS Source**: `src/js/casc/blp.js` line 85
- **Status**: Pending
- **Details**: JS sets `this.dataURL = null` in the constructor. This field is not declared in the C++ class. While it appears unused within `BLPImage` itself, it is a public property that external code could reference.

### 291. [blp.cpp] `_getCompressed()` boundary check uses `>=` instead of `===`
- **JS Source**: `src/js/casc/blp.js` line 323
- **Status**: Pending
- **Details**: JS uses strict equality (`if (this.rawData.length === pos)`), meaning only when `pos` exactly equals length does it skip. C++ (line 256) uses `>=` (`if (static_cast<size_t>(pos) >= rawData_.size())`), which is strictly safer but technically a behavioral deviation for the case where `pos > length`.

### 292. [blp.cpp] `_getAlpha()` case 4 — C++ integer division fixes JS bug but changes behavior
- **JS Source**: `src/js/casc/blp.js` line 294
- **Status**: Pending
- **Details**: In JS, `index / 2` produces a float (e.g., `3 / 2 = 1.5`). Accessing `rawData[1.5]` on a Uint8Array returns `undefined`, and `undefined & 0x0F` evaluates to `0`, producing incorrect alpha for all odd-indexed pixels. In C++, `index / 2` is integer division (floors automatically), so it correctly reads the byte containing the 4-bit alpha nibble. C++ accidentally fixes a JS bug. Should be documented with a comment explaining the intentional deviation.

### 293. [blp.cpp] `toBuffer()` extra `default` case not in JS
- **JS Source**: `src/js/casc/blp.js` lines 242–250
- **Status**: Pending
- **Details**: JS switch has cases 1, 2, 3 only — returns `undefined` for other encodings. C++ (lines 190–198) adds `default: return BufferWrapper();` which returns an empty buffer. Minor behavioral deviation — in JS the caller would get `undefined`, while in C++ it gets an empty buffer object.

### 294. [blte-reader.cpp] Missing `decodeAudio()` method
- **JS Source**: `src/js/casc/blte-reader.js` lines 337–340
- **Status**: Pending
- **Details**: JS `decodeAudio(context)` calls `this.processAllBlocks()` then `super.decodeAudio(context)`. This method is entirely absent from C++. It uses a Web Audio `AudioContext` which is browser-specific, but the `processAllBlocks()` call before delegating to the parent is important for ensuring blocks are processed before audio decoding.

### 295. [blte-reader.cpp] `getDataURL()` missing cached `dataURL` check
- **JS Source**: `src/js/casc/blte-reader.js` lines 346–353
- **Status**: Pending
- **Details**: JS checks `if (!this.dataURL)` before processing blocks, and returns the cached `this.dataURL` if already set. C++ (lines 282–285) always calls `processAllBlocks()` then `BufferWrapper::getDataURL()` with no caching check. The JS allows `dataURL` to be set externally without processing any blocks, which C++ does not support.

### 296. [blte-reader.cpp] `_handleBlock()` encrypted case uses `move()` instead of direct `_ofs +=`
- **JS Source**: `src/js/casc/blte-reader.js` line 211
- **Status**: Pending
- **Details**: JS directly modifies `this._ofs += this.blocks[index].DecompSize`. C++ (line 192) calls `move(static_cast<int64_t>(blocks[index].DecompSize))`. If `move()` has any bounds checking or side effects that `_ofs +=` does not, behavior could differ. Should verify `move()` semantics match direct offset addition.

### 297. [blte-stream-reader.cpp] Missing `createReadableStream()` method
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 168–193
- **Status**: Pending
- **Details**: JS creates a Web Streams API `ReadableStream` with `pull()` and `cancel()` callbacks for progressive block consumption. Entirely absent from C++. This is browser-specific, but the progressive streaming pattern could be replicated with C++ iterators or coroutines.

### 298. [blte-stream-reader.cpp] `streamBlocks()` async generator replaced with synchronous callback
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 199–202
- **Status**: Pending
- **Details**: JS uses `async *streamBlocks()` (async generator, yields `BufferWrapper`) allowing the caller to consume blocks lazily with `for await...of`. C++ (lines 128–133) uses `void streamBlocks(const std::function<void(BufferWrapper&)>& callback)` which iterates all blocks eagerly and synchronously. The caller cannot pause/resume iteration.

### 299. [blte-stream-reader.cpp] `createBlobURL()` returns BufferWrapper instead of URL string
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 208–218
- **Status**: Pending
- **Details**: JS creates a `Blob` with MIME type `'video/x-msvideo'` from decoded block chunks, then creates an object URL for direct use in `<video>` elements. C++ (lines 135–142) just concatenates all blocks into one `BufferWrapper` and returns raw data. The return type, semantics, and MIME type metadata are all different. The method name `createBlobURL` is misleading in C++ since no URL is created.

### 300. [blte-stream-reader.cpp] All async methods converted to synchronous
- **JS Source**: `src/js/casc/blte-stream-reader.js` line 38 and throughout
- **Status**: Pending
- **Details**: The JS `blockFetcher` is async (returns a Promise), allowing non-blocking I/O for fetching block data from remote sources. C++ (`blte-stream-reader.h` line 35) uses `std::function<BufferWrapper(size_t)>` (synchronous). Methods like `getBlock()`, `_decodeBlock()`, `streamBlocks()`, and `createBlobURL()` are all `async` in JS but synchronous in C++, blocking the calling thread during block fetches.

### 301. [build-cache.cpp] `getFile()` rejects immediately instead of waiting for cache integrity
- **JS Source**: `src/js/casc/build-cache.js` lines 77–79
- **Status**: Pending
- **Details**: In JS, if `cacheIntegrity` is not yet loaded, `getFile()` calls `await cacheIntegrityReady()`, which creates a Promise that resolves once the `'cache-integrity-ready'` event fires. The function waits for integrity to become available. In C++ (lines 81–85), the function checks `cacheIntegrityLoaded` and, if false, immediately logs a rejection message and returns `std::nullopt`. Files requested before initialization completes will fail in C++ but succeed in JS.

### 302. [build-cache.cpp] `storeFile()` initializes empty integrity instead of waiting
- **JS Source**: `src/js/casc/build-cache.js` lines 128–129
- **Status**: Pending
- **Details**: JS `await`s `cacheIntegrityReady()` to ensure integrity is loaded before writing. C++ (lines 129–134) checks `cacheIntegrityLoaded`, and if false, initializes an empty integrity map and proceeds. This silently discards any previously-cached integrity data that was still loading and creates a divergent code path.

### 303. [build-cache.cpp] Underflow guard on cache size subtraction deviates from JS
- **JS Source**: `src/js/casc/build-cache.js` line 252
- **Status**: Pending
- **Details**: JS `deleteSize -= manifestSize;` has no guard — `deleteSize` is a JS number (double) and can go negative. C++ uses `uintmax_t` (unsigned), so `if (deleteSize >= manifestSize) deleteSize -= manifestSize;` is added to prevent wraparound. Reasonable defensive fix but a deviation.

### 304. [casc-source.cpp] `getInstallManifest()` fallback differs when encoding key not found
- **JS Source**: `src/js/casc/casc-source.js` line 72
- **Status**: Pending
- **Details**: In JS, if `encodingKeys.get(installKeys[0])` returns `undefined`, `installKey` becomes `undefined`, which would propagate downstream. In C++ (lines 103–111), if the encoding key is not found, it falls back to using the raw key (`installKeys[0]`) instead of propagating the failure. This silently changes behavior for the key-not-found case.

### 305. [casc-source.cpp] `getFileByName()` drops parameters, breaking subclass dispatch
- **JS Source**: `src/js/casc/casc-source.js` line 190
- **Status**: Pending
- **Details**: In JS, `this.getFile()` dispatches polymorphically to `CASCLocal.getFile()` or `CASCRemote.getFile()`, accepting extra parameters (`partialDecrypt`, `suppressLog`, `supportFallback`, `forceFallback`). In C++ (line 236), `getFile()` is called with only `fileDataID`, dropping all extra parameters. CASCLocal doesn't override `getFile()` (renamed to `getFileAsBLTE()`), so polymorphic dispatch is broken.

### 306. [casc-source.cpp] `getVirtualFileByID()` missing readonly mmap protection and wrong error message
- **JS Source**: `src/js/casc/casc-source.js` line 225
- **Status**: Pending
- **Details**: (a) JS passes `{ protection: 'readonly' }` to map the file read-only; C++ (lines 274–275) calls `map()` with no protection argument. (b) JS error message includes `mmap_obj.lastError` (OS error details); C++ error message includes `cachedPath` (just the file path), losing diagnostic information.

### 307. [casc-source.cpp] `getModelFormats()` uses boolean instead of explicit filter constant
- **JS Source**: `src/js/casc/casc-source.js` line 301
- **Status**: Pending
- **Details**: In JS, `modelExt.push(['.wmo', constants.LISTFILE_MODEL_FILTER])` passes the actual regex filter. In C++ (line 364), `ExtFilter` struct stores `has_exclusion = true` as a boolean flag. Consumers separately call `constants::LISTFILE_MODEL_FILTER()`. Functionally equivalent for current use case but creates an indirect coupling.

### 308. [casc-source-local.cpp] `getFile()` renamed to `getFileAsBLTE()`, breaking polymorphism
- **JS Source**: `src/js/casc/casc-source-local.js` lines 63–70
- **Status**: Pending
- **Details**: In JS, `CASCLocal.getFile()` overrides the base `CASC.getFile()` and returns a `BLTEReader`. In C++ it is renamed to `getFileAsBLTE()`, so `CASC::getFile()` (which returns a raw encoding key string) is never overridden. Any code calling `getFile()` polymorphically on a CASCLocal instance gets the base behavior (encoding key) instead of decoded file data. Directly impacts `getFileByName` dispatch (see #305).

### 309. [casc-source-local.cpp] `load()` missing `core.view.casc = this` assignment
- **JS Source**: `src/js/casc/casc-source-local.js` line 179
- **Status**: Pending
- **Details**: In JS, `core.view.casc = this;` is set inside `load()`, immediately after `loadRoot()` but before `prepareListfile()`, `prepareDBDManifest()`, and `loadListfile()`. In C++ (lines 229–249), this assignment is done externally after `load()` returns, meaning `core::view->casc` is null/stale during those methods. If any code during those steps references `core::view->casc`, it would fail in C++ but work in JS.

### 310. [casc-source-local.cpp] `init()` logs count instead of full builds object
- **JS Source**: `src/js/casc/casc-source-local.js` line 51
- **Status**: Pending
- **Details**: JS logs the full builds array via `log.write('%o', this.builds)`. C++ (line 76) only logs `"Local builds found: " + std::to_string(builds.size())`. Full build details are lost for debugging.

### 311. [casc-source-local.cpp] `loadConfigs()` simplified log messages
- **JS Source**: `src/js/casc/casc-source-local.js` lines 212–213
- **Status**: Pending
- **Details**: JS dumps the full config objects via `log.write('BuildConfig: %o', this.buildConfig)`. C++ (lines 290–291) only logs `"BuildConfig loaded"` and `"CDNConfig loaded"`. Diagnostic information lost.

### 312. [casc-source-local.cpp] `load()` log message missing build details
- **JS Source**: `src/js/casc/casc-source-local.js` line 167
- **Status**: Pending
- **Details**: JS logs `'Loading local CASC build: %o', this.build` with the full build object. C++ (line 231) only logs `"Loading local CASC build"`.

### 313. [casc-source-local.cpp] `initializeRemoteCASC()` silently skips preload on invalid build index
- **JS Source**: `src/js/casc/casc-source-local.js` lines 328–330
- **Status**: Pending
- **Details**: In JS, `builds.findIndex(...)` returns `-1` if not found, and `preload(-1, ...)` is called with an invalid index (likely causing an error). In C++ (lines 442–443), `if (buildIndex >= 0)` guards against the -1 case, silently skipping preload. The remote instance is set up but unpreloaded, so subsequent operations may fail silently.

### 314. [casc-source-remote.cpp] `std::format` used with `%s` placeholder — host URL formatting is broken
- **JS Source**: `src/js/casc/casc-source-remote.js` line 39
- **Status**: Pending
- **Details**: `constants::PATCH::HOST` is `"https://%s.version.battle.net/"` (printf-style). `std::format` (line 43) requires `{}` placeholders, not `%s`. This will NOT substitute the region, producing the literal string `https://%s.version.battle.net/` or throwing `std::format_error`. Compare with `cdn-resolver.cpp:164-168` which correctly does manual `%s` replacement.

### 315. [casc-source-remote.cpp] `getConfig()` checks `res.empty()` instead of HTTP status
- **JS Source**: `src/js/casc/casc-source-remote.js` line 77
- **Status**: Pending
- **Details**: JS checks `if (!res.ok)` on the HTTP response status flag, including the HTTP status code in the error message. C++ (line 97) only checks `if (res.empty())`, which would not catch non-200 HTTP responses that have a body. Also, the error message omits the HTTP status code.

### 316. [casc-source-remote.cpp] `getCDNConfig()` same `res.empty()` vs `!res.ok` mismatch
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 100–101
- **Status**: Pending
- **Details**: Same issue as #315. JS throws with HTTP status code on `!res.ok`. C++ (lines 123–124) only checks `res.empty()` and omits the status code from the error.

### 317. [casc-source-remote.cpp] `load()` missing `core.view.casc = this` assignment
- **JS Source**: `src/js/casc/casc-source-remote.js` line 290
- **Status**: Pending
- **Details**: After `loadRoot()` and before `prepareListfile()`, the JS assigns `core.view.casc = this`. This is critical for other parts of the application to access the CASC source. The C++ port (lines 339–351) completely omits this assignment.

### 318. [casc-source-remote.cpp] `preload()` log message missing build details
- **JS Source**: `src/js/casc/casc-source-remote.js` line 264
- **Status**: Pending
- **Details**: JS logs `'Preloading remote CASC build: %o', this.build` with the full build object. C++ (line 319) only logs `"Preloading remote CASC build"`.

### 319. [casc-source-remote.cpp] `loadEncoding()` uses `build["BuildConfig"]` instead of `cache.key`
- **JS Source**: `src/js/casc/casc-source-remote.js` line 314
- **Status**: Pending
- **Details**: JS uses `this.cache.key` (the BuildCache's key property) in log messages. C++ (line 375) uses `build["BuildConfig"]` instead. While they are likely the same value, using the wrong source could diverge if the cache key is transformed differently.

### 320. [casc-source-remote.cpp] `loadServerConfig()` log missing server config details
- **JS Source**: `src/js/casc/casc-source-remote.js` line 388
- **Status**: Pending
- **Details**: JS logs `log.write('%o', serverConfigs)` with the full server config array. C++ (line 475) only logs `"Server configs loaded: " + std::to_string(serverConfigs.size())`.

### 321. [casc-source-remote.cpp] `loadConfigs()` config objects not logged
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 463–464
- **Status**: Pending
- **Details**: JS logs full config objects via `log.write('CDNConfig: %o', this.cdnConfig)`. C++ (lines 571–572) only logs `"CDNConfig loaded"` and `"BuildConfig loaded"`.

### 322. [casc-source-remote.cpp] `getDataFilePartial()` passes `""` instead of `null` for output path
- **JS Source**: `src/js/casc/casc-source-remote.js` line 448
- **Status**: Pending
- **Details**: JS passes `null` as the output-file parameter to `downloadFile()` meaning "don't write to disk". C++ (line 556) passes empty string `""`. Depending on how `downloadFile` handles these, behavior could differ.

### 323. [casc-source-remote.cpp] `init()` log doesn't dump builds array
- **JS Source**: `src/js/casc/casc-source-remote.js` line 55
- **Status**: Pending
- **Details**: JS logs the full builds data structure via `log.write('%o', this.builds)`. C++ (line 74) only logs `"Remote builds loaded: " + std::to_string(builds.size())`.

### 324. [cdn-resolver.cpp] Missing HTTP response status check in `_resolveRegionProduct()`
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 152–153
- **Status**: Pending
- **Details**: JS checks `if (!res.ok)` and throws with the HTTP status code and URL. C++ (line 172) calls `generics::get(url)` with no HTTP status check at all. If the server returns an error status with a body, C++ would try to parse it as a version config, producing garbage results.

### 325. [cdn-resolver.cpp] `getBestHost()` cache update race condition
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 66–73
- **Status**: Pending
- **Details**: In JS, the resolution result is always stored back into the cache after awaiting the promise. In C++ (lines 239–244), the cache is only updated `if (isNewResolution)`. If a second caller waits on an existing future, it skips the cache update. A third concurrent caller might trigger a redundant resolution. Subtle race condition absent from the JS single-threaded model.

### 326. [cdn-resolver.cpp] `getRankedHosts()` same cache update race condition
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 108–114
- **Status**: Pending
- **Details**: Same issue as #325 in `getRankedHosts()`. Only updates cache `if (isNewResolution)`.

### 327. [cdn-resolver.cpp] `backgroundThreads` vector never cleared
- **JS Source**: `src/js/casc/cdn-resolver.js` line 34
- **Status**: Pending
- **Details**: JS uses fire-and-forget async calls for pre-resolution. C++ (lines 48, 196–198) pushes `std::jthread` instances into a `backgroundThreads` vector that is never cleared. The vector grows without bound if `startPreResolution` is called repeatedly. `std::jthread` joins on destruction, so cleanup is deferred to program exit.

### 328. [db2.cpp] Missing auto-parse proxy behavior (lazy parsing on method access)
- **JS Source**: `src/js/casc/db2.js` lines 37–73
- **Status**: Pending
- **Details**: In JS, `create_wrapper` returns a `Proxy` around the `WDCReader`. When any method is called, the proxy intercepts and automatically calls `parse()` if the reader hasn't been loaded yet. In C++ (lines 24–36), `getTable()` returns a raw `WDCReader&` with no parsing done and no automatic parsing on method calls. Callers must manually call `parse()` before using the reader.

### 329. [db2.cpp] Missing `parse_promise` deduplication for concurrent callers
- **JS Source**: `src/js/casc/db2.js` lines 38, 60–63
- **Status**: Pending
- **Details**: In JS, `create_wrapper` captures a `parse_promise` variable. If multiple async callers invoke a method on the same table simultaneously, only the first triggers `parse()` and subsequent callers await the same promise. In C++, there is no deduplication — multiple threads calling `parse()` on the same reader could parse concurrently, causing data races or redundant work.

### 330. [db2.cpp] Missing `getRelationRows` validation guards
- **JS Source**: `src/js/casc/db2.js` lines 45–56
- **Status**: Pending
- **Details**: In JS, the wrapper proxy has special handling for `getRelationRows`. If the table is not loaded, it throws `'Table must be loaded before calling getRelationRows'`. If loaded but rows are `null`, it throws `'Table must be preloaded before calling getRelationRows'`. None of this validation exists in C++. A caller could invoke `getRelationRows()` on an unparsed or un-preloaded reader with no guard.

### 331. [db2.cpp] Extra `clearCache()` function not in original JS
- **JS Source**: N/A
- **Status**: Pending
- **Details**: C++ (lines 61–63, db2.h line 57) adds a `clearCache()` function that clears the entire table cache. This function does not exist in the original JS module. While potentially useful, it is an addition not present in the original source.

### 332. [db2.cpp] Cache stores raw readers instead of wrappers
- **JS Source**: `src/js/casc/db2.js` lines 8, 30–31, 88–89
- **Status**: Pending
- **Details**: In JS, the `cache` Map stores wrapped Proxy objects (`create_wrapper(reader)`) with auto-parse and validation guard behavior. In C++ (line 17), the cache stores raw `std::unique_ptr<db::WDCReader>` instances with none of these behaviors. Cached readers have no protections.

### 333. [dbd-manifest.cpp] `prepareManifest()` returns void instead of bool
- **JS Source**: `src/js/casc/dbd-manifest.js` lines 50–56
- **Status**: Pending
- **Details**: The JS function returns `Promise<boolean>` and always returns `true`. The C++ version (line 22) returns `void`. While the boolean return is always `true` and likely never checked, this is a signature deviation. If any caller checked the return value, it would be a compilation error.

### 334. [dbd-manifest.cpp] Truthiness check for JSON fields is stricter than JS
- **JS Source**: `src/js/casc/dbd-manifest.js` line 31
- **Status**: Pending
- **Details**: JS uses simple truthiness: `if (entry.tableName && entry.db2FileDataID)`. C++ (lines 55–58) uses explicit type checks (`is_string()`, `is_number()`, `.empty()` checks). Stricter — rejects a `tableName` that is a non-string truthy value or a `db2FileDataID` that is a non-number truthy value.

### 335. [dbd-manifest.cpp] Data race on `is_preloaded` flag — not atomic or mutex-protected
- **JS Source**: `src/js/casc/dbd-manifest.js` lines 10, 38, 41, 51
- **Status**: Pending
- **Details**: C++ adds `manifest_mutex` (line 30) to protect `table_to_id` and `id_to_table` access. However, `is_preloaded` is written on lines 68 and 71 (inside async lambda, potentially on worker thread) and read on line 80 (`prepareManifest`) on the calling thread — all without mutex protection or atomic access. This is a data race (undefined behavior in C++). Should be `std::atomic<bool>`.

### 336. [dbd-manifest.cpp] `preload()` has TOCTOU race on `preload_promise`
- **JS Source**: `src/js/casc/dbd-manifest.js` lines 18–43
- **Status**: Pending
- **Details**: The check `if (preload_promise.has_value())` (line 41) and the assignment `preload_promise = std::async(...)` (line 44) are not protected by any lock. If two threads call `preload()` concurrently, both could pass the check and launch two async tasks. In JS this cannot happen because JS is single-threaded. Should use a mutex or `std::call_once`.

### 337. [realmlist.cpp] Missing HTTP response status check
- **JS Source**: `src/js/casc/realmlist.js` lines 50–64
- **Status**: Pending
- **Details**: JS has two distinct error paths: `if (res.ok)` for success and `log.write('Failed to retrieve realmlist from ${url} (${res.status})')` for failure. C++ (lines 83–95) calls `generics::get(url)` with no HTTP status check. If the HTTP library returns error pages as data without throwing, C++ would attempt to JSON-parse the error page, producing a misleading parse error instead of a clean status-code error message.

### 338. [realmlist.cpp] `url` validation logic differs from JS
- **JS Source**: `src/js/casc/realmlist.js` lines 39–41
- **Status**: Pending
- **Details**: JS does `let url = String(core.view.config.realmListURL)` which converts any value (including `undefined` → `"undefined"`) to a string. C++ checks `config.contains("realmListURL") && config["realmListURL"].is_string()` then checks `url.empty()`. Stricter: rejects non-string config values and empty strings that JS would accept via coercion.

### 339. [listfile.cpp] `preload()` does not deduplicate concurrent calls properly
- **JS Source**: `src/js/casc/listfile.js` lines 478–487
- **Status**: Pending
- **Details**: JS stores a `preload_promise` that multiple callers can `await`. All concurrent callers receive the same promise and wait for the single preload to finish. C++ uses a `bool preload_in_progress` flag. When a second caller sees `preload_in_progress == true`, it returns immediately without waiting. This means the second caller proceeds as if preload is done when it is not, potentially causing downstream failures.

### 340. [listfile.cpp] `prepareListfile()` does not wait for in-progress preload
- **JS Source**: `src/js/casc/listfile.js` lines 489–500
- **Status**: Pending
- **Details**: JS does `return await preload_promise` to block until preload completes. C++ has a comment `"running concurrently — it would have completed. Just return."` and returns immediately. This comment is incorrect — if preload is in progress, it has NOT completed. The caller would proceed without the listfile being loaded.

### 341. [listfile.cpp] `emplace()` vs `set()` — duplicate handling differs in multiple functions
- **JS Source**: `src/js/casc/listfile.js` lines 428–430, 541–542, 626–627, 703–707
- **Status**: Pending
- **Details**: Throughout the codebase, JS uses `Map.set()` which overwrites existing entries with the same key. C++ uses `std::unordered_map::emplace()` which does NOT overwrite — it silently discards the new value if the key exists. Affects: listfile line parsing (preloadedIdLookup), `applyPreload` legacy mode, `loadIDTable`, and `ingestIdentifiedFiles`. Most impactful in `ingestIdentifiedFiles` where JS always overwrites but C++ never does.

### 342. [listfile.cpp] `loadUnknownModels` calls extra initialization not present in JS
- **JS Source**: `src/js/casc/listfile.js` lines 610–614
- **Status**: Pending
- **Details**: JS calls `DBModelFileData.getFileDataIDs()` directly with no explicit initialization. C++ (lines 687–693) calls `db::caches::DBModelFileData::initializeModelFileData()` before `getFileDataIDs()`. This extra call has no JS equivalent and may cause side effects or redundant initialization.

### 343. [listfile.cpp] `applyPreload` binary mode `filter_and_format` returns wrong type
- **JS Source**: `src/js/casc/listfile.js` lines 572–582
- **Status**: Pending
- **Details**: C++ lambda `filter_and_format` (lines 875–885) returns `std::vector<nlohmann::json>` instead of `std::vector<std::string>`. Each formatted string is implicitly wrapped in a `nlohmann::json` object. The JS returns a plain string array. Depending on how the view members are typed, this may cause type mismatches or unnecessary JSON overhead.

### 344. [listfile.cpp] `renderListfile` — empty `file_data_ids` vector treated as "no filter" instead of "match nothing"
- **JS Source**: `src/js/casc/listfile.js` lines 710–756
- **Status**: Pending
- **Details**: JS distinguishes between `file_data_ids === undefined` (no filter — include everything) and `file_data_ids = []` (empty filter — include nothing). C++ uses `bool has_id_filter = !file_data_ids.empty()` which treats an empty vector the same as "no filter" — it includes ALL entries. Calling `renderListfile({})` in C++ returns everything, while `renderListfile([])` in JS returns nothing from legacy lookups.

### 345. [listfile.cpp] `ExtFilter` struct does not store the actual exclusion regex
- **JS Source**: `src/js/casc/listfile.js` lines 446, 511–513, 647–649, 664–666
- **Status**: Pending
- **Details**: JS extension filter can be `['.wmo', constants.LISTFILE_MODEL_FILTER]` where any regex can be used per-extension. C++ `ExtFilter` struct only stores `bool has_exclusion` and always hardcodes `constants::LISTFILE_MODEL_FILTER()` when true. Impossible to use a different exclusion regex per extension — a structural deviation.

### 346. [listfile.cpp] `getByID` returns empty string instead of undefined/null sentinel
- **JS Source**: `src/js/casc/listfile.js` lines 778–794
- **Status**: Pending
- **Details**: JS `getByID` returns `undefined` when a file data ID is not found. C++ returns an empty string `""`. In `getByIDOrUnknown`, JS uses nullish coalescing (`result ?? formatUnknownFile(...)`) which only triggers on `undefined`/`null`. C++ uses `!result.empty()`. If a file legitimately has an empty filename, the behavior would differ.

### 347. [listfile.cpp] `getFilteredEntries` API signature change — regex detection
- **JS Source**: `src/js/casc/listfile.js` lines 832–857
- **Status**: Pending
- **Details**: JS function accepts either a `string` or `RegExp` and auto-detects via `search instanceof RegExp`. C++ takes `const std::string& search, bool is_regex = false`, requiring the caller to explicitly specify the regex flag. Additionally, C++ silently returns empty results on invalid regex (catch block), while JS would propagate the error.

### 348. [listfile.cpp] `parseFileEntry` compiles regex on every call
- **JS Source**: `src/js/casc/listfile.js` lines 871–876
- **Status**: Pending
- **Details**: C++ (lines 1005–1015) constructs `std::regex fid_regex(R"(\[(\d+)\]$)")` inside the function body, recompiling it on every invocation. `std::regex` construction is expensive. The regex should be `static const` to avoid repeated compilation.

### 349. [tact-keys.cpp] Remote key line-splitting is more lenient than JS
- **JS Source**: `src/js/casc/tact-keys.js` lines 99–100
- **Status**: Pending
- **Details**: JS uses `line.split(' ')` which requires exactly 2 parts (rejects lines with double spaces or trailing spaces producing 3+ parts). C++ (lines 205–216) finds the first space and trims the remainder, accepting lines with multiple spaces. Behavioral deviation for malformed input.

### 350. [tact-keys.cpp] Error message loses HTTP status code
- **JS Source**: `src/js/casc/tact-keys.js` lines 91–92
- **Status**: Pending
- **Details**: JS error message is `"Unable to update tactKeys, HTTP ${res.status}"` with the HTTP status code. C++ (lines 190–197) uses generic `"Unable to update tactKeys"` — catches all exceptions and rethrows without status info. Also adds an `empty response` error path that doesn't exist in JS (JS would just parse 0 keys from empty body).

### 351. [vp9-avi-demuxer.cpp] `find_chunk` loop bound off-by-one
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js` — `i < length - 4`
- **Status**: Pending
- **Details**: JS uses `i < length - 4`. C++ (line 71) uses `i + 3 < size` which is equivalent to `i < size - 3`, allowing one extra position compared to JS's `i < size - 4`. Could read out of bounds on the last byte.

### 352. [vp9-avi-demuxer.cpp] `parse_header` missing nullable return
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js` — returns `null` when `strf` chunk is missing
- **Status**: Pending
- **Details**: JS returns `null` when the `strf` chunk is not found. C++ (line 34) returns a default-constructed `VP9Config` instead. Should return `std::optional<VP9Config>` to match JS's nullable return path. Callers cannot distinguish "missing chunk" from "valid config with default values".

## src/js/3D/ Audit

### 353. [Skin.h] `SubMesh::triangleStart` is `uint16_t` but must hold a 32-bit value
- **JS Source**: `src/js/3D/Skin.js` lines 61, 72
- **Status**: Pending
- **Details**: JS reads `triangleStart` as `readUInt16LE()` then adds `level << 16` to it (`this.subMeshes[i].triangleStart += this.subMeshes[i].level << 16`). In JS, numbers are 64-bit doubles so this works fine, producing values up to `0xFFFF + (0xFFFF << 16)`. In C++, `SubMesh::triangleStart` is declared as `uint16_t` in `Skin.h` (line 18). The operation `sm.triangleStart += sm.level << 16` at `Skin.cpp` line 87 silently overflows — the high 16 bits from `level << 16` are truncated, so `triangleStart` always equals the original 16-bit value unchanged. This should be `uint32_t` to match JS behaviour.

### 354. [Shaders.cpp] `SHADER_MANIFEST` is `static` but JS exports it
- **JS Source**: `src/js/3D/Shaders.js` lines 13–19, 147
- **Status**: Pending
- **Details**: JS exports `SHADER_MANIFEST` via `module.exports` (line 147), making it accessible to other modules. C++ declares `SHADER_MANIFEST` as `static const` at `Shaders.cpp` line 22, giving it internal linkage (file-local). It is not declared in `Shaders.h`. Any other translation unit that needs to look up shader manifest entries (e.g., to enumerate available shader names) cannot access it. Either make it non-static or add a declaration in the header.

### 355. [Shaders.cpp] `create_program` allocates with `new` but no ownership/cleanup — memory leak
- **JS Source**: `src/js/3D/Shaders.js` lines 56–72
- **Status**: Pending
- **Details**: JS `create_program` creates a `new ShaderProgram(...)` and relies on garbage collection for cleanup. C++ `create_program` (line 86) uses `new gl::ShaderProgram(...)` and returns a raw pointer. The `unregister` function (line 103) removes the pointer from the `active_programs` tracking set but does not `delete` the program. No other code deletes the programs either. This is a memory leak — every created shader program leaks when unregistered. The raw pointer should either be wrapped in a `std::unique_ptr` or `unregister`/a cleanup function should `delete` the program.

### 356. [Texture.h] Extra `fileName` member not present in JS
- **JS Source**: `src/js/3D/Texture.js` lines 15–18
- **Status**: Pending
- **Details**: JS `Texture` class has only `flags`, `fileDataID`, and `data` as instance properties. The C++ `Texture` class in `Texture.h` (line 36) declares an additional `std::string fileName` public member that has no counterpart in JS. `setFileName` in JS sets `this.fileDataID` (not `this.fileName`) — the C++ version correctly sets `this->fileDataID` but the unused `fileName` member remains as dead state. This is a minor deviation; the extra member should be removed to match JS fidelity, or documented as intentional.

## src/js/3D/camera/ Audit

### 357. [CameraControlsGL.cpp] `init()` omits all event listener registration
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 198–216
- **Status**: Pending
- **Details**: JS `init()` registers six event listeners: `contextmenu` (prevent default), `mousedown`, `wheel` on `dom_element`, and `mousemove`/`mouseup` on `document`. It also stores `move_listener` and `up_listener` references for later removal in `dispose()`. C++ `init()` (lines 206–214) omits all of this with a comment about GLFW callbacks. The `move_listener`/`up_listener` members from JS are absent. While GLFW handles input differently, the contextmenu prevention (right-click menu suppression) has no C++ equivalent documented or implemented.

### 358. [CameraControlsGL.cpp] `dispose()` is empty — no cleanup
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 218–221
- **Status**: Pending
- **Details**: JS `dispose()` removes `mousemove` and `mouseup` event listeners from `document` using stored handler references. C++ `dispose()` (lines 216–217) is completely empty. If any GLFW callback cleanup is needed, it is not performed. This could leave dangling callbacks if a `CameraControlsGL` instance is destroyed while callbacks are still registered.

### 359. [CameraControlsGL.cpp] `on_mouse_down` missing `window.focus()` fallback
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` line 226
- **Status**: Pending
- **Details**: JS calls `this.dom_element.focus ? this.dom_element.focus() : window.focus()` — if `dom_element.focus` is falsy, it falls back to `window.focus()`. C++ (lines 221–222) calls `dom_element.focus()` only if `dom_element.focus` is set, but has no fallback equivalent for focusing the window when the dom element has no focus function.

### 360. [CameraControlsGL.cpp] `update()` unconditionally copies `camera.quaternion` — JS guards with null check
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 417–420
- **Status**: Pending
- **Details**: JS `update()` at line 417 uses `this.camera.quaternion || [0, 0, 0, 1]` for the distance check, and at lines 419–420 only copies `this.camera.quaternion` to `last_quaternion` if `this.camera.quaternion` is truthy (using spread `[...this.camera.quaternion]`). C++ (lines 407–410) unconditionally accesses `camera.quaternion` in both the distance check and the copy — no null/optional guard. Since `CameraGL::quaternion` is always initialized, this works but changes the conditional semantics: JS would use a default `[0,0,0,1]` if `quaternion` were ever null at runtime.

### 361. [CameraControlsGL.cpp] `update()` guards `camera.lookAt` with null check — JS calls unconditionally
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` line 408
- **Status**: Pending
- **Details**: JS calls `this.camera.lookAt(this.target[0], this.target[1], this.target[2])` unconditionally at line 408 — if `lookAt` is undefined, JS would throw a runtime error. C++ (lines 398–399) adds `if (camera.lookAt)` before calling, silently skipping if not set. This is a behavioral deviation: JS would crash on a missing `lookAt`, while C++ silently produces no camera orientation update.

### 362. [CameraControlsGL.cpp] `get_pan_scale` missing `fov || 50` fallback pattern
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 300–301
- **Status**: Pending
- **Details**: JS reads `this.camera.fov || 50` at line 300, falling back to `50` if `fov` is `0`, `undefined`, or `null`. C++ (line 292) reads `camera.fov` directly. While `CameraGL::fov` is initialized to `50.0f`, if it is ever set to `0.0f` at runtime, JS would use `50` but C++ would use `0.0f`, producing a zero `v_fov` and `NaN` from `tan(0/2)`.

### 363. [CameraControlsGL.h] `Spherical` and `CameraGL`/`DomElementGL` structs are local — JS has no equivalent types
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 155–196
- **Status**: Pending
- **Details**: JS uses plain objects and arrays for `spherical`, `camera`, and `dom_element` (duck typing). C++ defines three structs (`CameraGL`, `DomElementGL`, `Spherical`) in `CameraControlsGL.h` (lines 16–40). These are reasonable C++ equivalents but are unique to this header — if other files need the same camera or dom element interface, they define their own duplicate structs (see `CharacterCameraControlsGL.h` `CharacterCameraGL`/`CharacterDomElementGL`). This duplication could cause type mismatches across the codebase.

### 364. [CharacterCameraControlsGL.cpp] Constructor omits event listener registration
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 27–35
- **Status**: Pending
- **Details**: JS constructor registers three event listeners on `dom_element`: `mousedown`, `wheel`, and `contextmenu` (with `e.preventDefault()`). It also stores `mouse_down_handler`, `mouse_move_handler`, `mouse_up_handler`, `mouse_wheel_handler` as instance properties for later removal. C++ constructor (lines 17–27) does none of this — it only initializes member variables. While GLFW handles input differently, the stored handler references and contextmenu prevention are absent.

### 365. [CharacterCameraControlsGL.cpp] `dispose()` is empty — JS removes four event listeners
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 170–175
- **Status**: Pending
- **Details**: JS `dispose()` removes four event listeners: `mousedown` and `wheel` from `dom_element`, and `mousemove` and `mouseup` from `document`. C++ `dispose()` (lines 151–152) is completely empty. Any GLFW callback cleanup that may be needed is not performed.

### 366. [CharacterCameraControlsGL.cpp] `on_mouse_move` guards `camera.lookAt` with null check — JS calls unconditionally
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` line 112
- **Status**: Pending
- **Details**: JS calls `this.camera.lookAt(this.target[0], this.target[1], this.target[2])` unconditionally at line 112 during panning. C++ (line 99) guards with `if (camera.lookAt)`. This is a behavioral deviation: JS would crash if `lookAt` is missing, while C++ silently skips the camera orientation update, potentially leaving the camera in an inconsistent state after panning.

### 367. [CharacterCameraControlsGL.cpp] `on_mouse_wheel` guards `camera.update_view` with null check — JS calls unconditionally
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` line 162
- **Status**: Pending
- **Details**: JS calls `this.camera.update_view()` unconditionally at line 162 after zooming. C++ (line 143) guards with `if (camera.update_view)`. This is a behavioral deviation: JS would crash if `update_view` is missing, while C++ silently skips the view matrix update after zoom, potentially leaving the rendering state stale.

### 368. [CharacterCameraControlsGL.h] Duplicate camera/dom structs — `CharacterCameraGL` vs `CameraGL`
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 14–16
- **Status**: Pending
- **Details**: JS uses duck typing — both `CameraControlsGL` and `CharacterCameraControlsGL` accept the same camera object with `position`, `lookAt`, etc. C++ defines separate struct types: `CameraGL` (in `CameraControlsGL.h`) and `CharacterCameraGL` (in `CharacterCameraControlsGL.h`). These are structurally similar but distinct types: `CameraGL` has `up`, `quaternion`, `fov` members while `CharacterCameraGL` has `update_view` instead. This prevents passing the same camera object to both control types, which JS allows freely. A shared base struct or a single unified type would better match JS's duck-typed flexibility.

## src/js/3D/exporters/ Audit

### 369. [ADTExporter.cpp] `loadTexture` uses `blp.width`/`blp.height` instead of `blp.scaledWidth`/`blp.scaledHeight`
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 63
- **Status**: Pending
- **Details**: JS `loadTexture` calls `gl.texImage2D(..., blp.scaledWidth, blp.scaledHeight, ...)`. C++ (line 112) uses `blp.width`/`blp.height`. If a BLP's scaled dimensions differ from raw dimensions, the GL texture will have wrong dimensions, causing rendering mismatches.

### 370. [ADTExporter.cpp] `useADTSets` flag check — `model & 0x80` vs `model.flags & 0x80`
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1301
- **Status**: Pending
- **Details**: JS does `const useADTSets = model & 0x80` — bitwise-AND on the *object* itself, which always evaluates to `0` in JS since `ToInt32(object)` is `0`, making `useADTSets` always falsy. C++ (line 1569) correctly reads `model.flags & 0x80`, which can produce `true`. This means C++ can produce different doodad-set behaviour for WMO models.

### 371. [ADTExporter.cpp] Cancellation returns populated `ADTExportResult` instead of `undefined`
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 623, 1025, 1252, 1537
- **Status**: Pending
- **Details**: JS `return;` returns `undefined` on cancellation. C++ (lines 811, 1228, 1494, 1904) does `return out;` returning a populated `ADTExportResult`. Callers checking for cancellation may behave differently.

### 372. [ADTExporter.cpp] Liquid JSON export uses explicit fields — JS uses spread operator
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1428–1438
- **Status**: Pending
- **Details**: JS uses `{ ...instance, worldPosition, terrainChunkPosition }` and `{ ...chunk, instances }` to copy *all* fields. C++ (lines 1737–1772) only serializes explicitly listed fields (`chunkIndex`, `instanceIndex`, `liquidType`, etc. for instance; `instances` and `attributes` for chunk). Any additional instance or chunk fields not explicitly listed are lost in C++ output.

### 373. [ADTExporter.cpp] Minimap export uses `blp.width` for scaling — JS uses `blp.scaledWidth`
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 873, 877
- **Status**: Pending
- **Details**: JS calls `blp.toCanvas(0b0111)` and then uses `blp.scaledWidth` for the scale factor. C++ (lines 1050–1051) calls `blp.toUInt8Array(0, 0b0111)` and uses `blp.width`. If `scaledWidth != width`, the scale factor and resulting minimap resolution will differ.

### 374. [ADTExporter.cpp] GL index buffer uses `uint32` — JS uses `Uint16`
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1117–1118
- **Status**: Pending
- **Details**: JS creates `new Uint16Array(indices)` and uses `gl.UNSIGNED_SHORT`. C++ (lines 1322–1323) uses `uint32_t` data with `GL_UNSIGNED_INT`. Functionally works but the GL data type and memory layout differ; doubles index buffer memory usage.

### 375. [ADTExporter.cpp] Doodad/model `ScaleFactor` missing fallback for undefined scale
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1270
- **Status**: Pending
- **Details**: JS uses `model.scale !== undefined ? model.scale / 1024 : 1` — defaults to `1` if `scale` is absent. C++ (lines 1516/1525) always computes `model.scale / 1024.0f` with no fallback. If `scale` is `0`, C++ uses `0` while JS would use `1`.

### 376. [ADTExporter.cpp] `texParams` guard only checks bounds, not element truthiness
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 639
- **Status**: Pending
- **Details**: JS checks `if (texParams && texParams[i])` — also verifies the element is truthy (not `0`/`null`/`undefined`). C++ (line 785) only checks `if (i < texParams.size())` — a default-constructed element at index `i` passes the check. This could cause different behaviour if texture parameters contain zero/null entries.

### 377. [ADTExporter.cpp] `STB_IMAGE_RESIZE_IMPLEMENTATION` defined in .cpp — potential ODR violation
- **JS Source**: N/A (C++-specific)
- **Status**: Pending
- **Details**: C++ line 10 defines `#define STB_IMAGE_RESIZE_IMPLEMENTATION` before `#include <stb_image_resize2.h>`. If another translation unit also defines this macro, it will cause duplicate-symbol linker errors. This implementation define should be in exactly one `.cpp` file in the project.

### 378. [ADTExporter.cpp] Unique texture ID ordering — `Set` insertion-order vs `std::set` sorted
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 921
- **Status**: Pending
- **Details**: JS uses `[...new Set(materialIDs.filter(...))]` which preserves insertion order. C++ (lines 1101–1105) uses `std::set<uint32_t>` which sorts numerically. Texture array indices in the output will differ. Since the mapping is self-consistent, rendering should be identical but debug output or derived data will vary.

### 379. [M2Exporter.cpp] `addURITexture()` parameter semantics changed
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59–61
- **Status**: Pending
- **Details**: JS `addURITexture(out, dataURI)` takes a string name and a base64 data URI, storing string→string in a Map. C++ takes `uint32_t textureType` and a pre-decoded `BufferWrapper pngData` (header line 123). The key type changes from arbitrary string to uint32_t. JS decodes the base64 inside `exportTextures`; C++ requires callers to decode before calling.

### 380. [M2Exporter.cpp] `dataTextures` map key type changed from string to uint32_t
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 35
- **Status**: Pending
- **Details**: JS `this.dataTextures = new Map()` stores string keys (the `out` parameter from `addURITexture`). C++ (header line 207) uses `std::map<uint32_t, BufferWrapper>`. The JS API accepts any string key; C++ restricts to uint32_t. This constrains the API and changes lookup semantics.

### 381. [M2Exporter.cpp] Data textures silently dropped from GLB/GLTF output
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 361, 366
- **Status**: Pending
- **Details**: JS passes `result.texture_buffers` (with `'data-5'`-style string keys) to `gltf.setTextureBuffers()` and `gltf.setTextureMap()`. C++ (lines 610–636) attempts to convert string keys to `uint32_t` via `std::stoul()`; entries with "data-" prefix keys are silently dropped in the catch block. Data textures will NOT be embedded in GLB output in C++.

### 382. [M2Exporter.cpp] `posedVertices` empty-vs-null check differs
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 738
- **Status**: Pending
- **Details**: JS uses `this.posedVertices ?? this.m2.vertices` — nullish coalescing; an empty array `[]` is truthy and WOULD be used. C++ (line 1038) uses `!posedVertices.empty() ? posedVertices : m2->vertices` — an empty vector falls through to m2 data. If `setPosedGeometry()` is called with empty arrays, JS uses the empty arrays while C++ falls back to bind pose.

### 383. [M2Exporter.cpp] `getSkin()` return not null-checked in equipment helper methods
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 420–421, 559–561, 683–685
- **Status**: Pending
- **Details**: JS null-checks the skin result: `const skin = await m2.getSkin(0); if (!skin) return;`. C++ `_addEquipmentToGLTF` (line 700), `_exportEquipmentToOBJ` (line 847), and `_exportEquipmentToSTL` (line 980) all take a reference directly from `getSkin(0)` with no null/validity check. If `getSkin` fails to return valid data, C++ has undefined behaviour.

### 384. [M2Exporter.cpp] Meta JSON `subMeshes` serialization — explicit fields vs JS spread
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 794
- **Status**: Pending
- **Details**: JS uses `Object.assign({ enabled: subMeshEnabled }, skin.subMeshes[i])` which copies ALL submesh properties. C++ (lines 1102–1117) manually serializes specific SubMesh fields. If the SubMesh struct has additional fields not explicitly listed, they will be missing from the JSON output.

### 385. [M2Exporter.cpp] Meta JSON `textures` entry serialization — missing fields vs JS spread
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 804–808
- **Status**: Pending
- **Details**: JS uses `Object.assign({ fileNameInternal, fileNameExternal, mtlName }, texture)` which includes ALL texture properties. C++ (lines 1127–1138) only serializes `fileDataID`, `flags`, `fileNameInternal`, `fileNameExternal`, `mtlName`. Any extra JS texture properties (e.g., `type`) not listed are missing from C++ JSON.

### 386. [M2Exporter.cpp] `fileNameInternal` returns empty string instead of null for unknown files
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 805
- **Status**: Pending
- **Details**: JS `listfile.getByID(texture.fileDataID)` returns `undefined` when not found. C++ `casc::listfile::getByID(texture.fileDataID)` returns empty string. JSON output will have `"fileNameInternal": ""` instead of `null`/absent.

### 387. [M2Exporter.cpp] Data texture fileDataIDs zeroed in manifests
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 747–748, 997–998
- **Status**: Pending
- **Details**: JS pushes `fileDataID: texFileDataID` to file manifests where `texFileDataID` can be a string like `'data-5'` for data textures. C++ (lines 1050–1052, 1367–1368) tries `std::stoul(texKey)` which fails for "data-" prefix keys and falls through to `texID=0`. Data texture manifest entries lose their identifiers.

### 388. [M2Exporter.cpp] Constructor takes extra `casc` parameter not in JS
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 31
- **Status**: Pending
- **Details**: JS constructor is `constructor(data, variantTextures, fileDataID)` — 3 params; uses `core.view.casc` inline. C++ (header line 103) is `M2Exporter(BufferWrapper data, ..., casc::CASC* casc)` — 4 params; stores `casc` as member. Callers must pass CASC source explicitly.

### 389. [CharacterExporter.cpp] `applyExternalBoneMatrices` called unconditionally — JS guards with `if (renderer.applyExternalBoneMatrices)`
- **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 272–273
- **Status**: Pending
- **Details**: JS checks `if (renderer.applyExternalBoneMatrices)` before calling the method (duck-type guard). C++ (line 311) calls `renderer->applyExternalBoneMatrices(...)` unconditionally. If the renderer does not implement this method (or it's not defined), C++ would fail at compile time; at runtime, if the method is a no-op, behaviour matches but the guard pattern differs.

### 390. [CharacterExporter.cpp] `remap_bone_indices` truncates remap values above 255
- **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 126–138
- **Status**: Pending
- **Details**: JS creates `new Uint8Array(bone_indices.length)` and sets `remapped[i] = remap_table[original_idx]`. JS Uint8Array automatically truncates to 0–255 but the remap_table values could be larger. C++ (line 148) casts `static_cast<uint8_t>(remap_table[original_idx])` which also truncates; the remap_table is `std::vector<int16_t>` which can hold negative values that would wrap differently than JS's unsigned truncation.

### 391. [CharacterExporter.h] `EquipmentGeometry::uv`/`uv2` are raw pointers — JS returns direct references
- **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 314–321
- **Status**: Pending
- **Details**: JS `_process_equipment_renderer` returns `{ uv: m2.uv, uv2: m2.uv2 }` which are direct references to the model's UV arrays. C++ (header lines 52–53) stores `const std::vector<float>* uv` and `const std::vector<float>* uv2` — raw pointers to the model data. If the underlying model (`M2Loader`) is destroyed before these pointers are used, they become dangling. JS garbage collection prevents this.

### 392. [M2LegacyExporter.cpp] Meta JSON serialization uses explicit field lists — JS uses direct property assignment
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 206–258
- **Status**: Pending
- **Details**: JS meta export uses `json.addProperty('materials', this.m2.materials)` which serializes the entire materials array as-is. C++ (lines 306–312) manually serializes each material field (`flags`, `blendingMode`). Similarly, bounding boxes, submeshes, and texture units are manually decomposed into individual JSON properties. If the M2 loader structs have additional fields, they will be missing from C++ JSON output.

### 393. [M3Exporter.cpp] `addURITexture` parameter type differs — JS takes string, C++ takes string
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49–51
- **Status**: Pending
- **Details**: JS `addURITexture(out, dataURI)` stores `this.dataTextures.set(out, dataURI)` — both params are strings (out=path, dataURI=base64). C++ (header line 59) takes `const std::string& out` and `BufferWrapper pngData`, storing `string→BufferWrapper` in `std::map`. The value type changes from base64 data URI string to decoded buffer — callers must provide pre-decoded PNG data.

### 394. [M3Exporter.cpp] `exportTextures()` is a stub returning empty map — matches JS
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 62–65
- **Status**: Pending
- **Details**: Both JS and C++ `exportTextures()` return an empty map. The C++ comment at line 60 documents this intentional match. While technically not a deviation, `dataTextures` populated via `addURITexture` are never consumed in either version, meaning data textures for M3 models are silently lost.

### 395. [M3Exporter.cpp] `geosetName` extraction uses BufferWrapper seek/read — JS uses string slice
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 101, 160, 220
- **Status**: Pending
- **Details**: JS reads geoset names via `this.m3.stringBlock.slice(geoset.nameCharStart, geoset.nameCharStart + geoset.nameCharCount)` — directly slicing the string block. C++ (lines 113–116, 185–188, 261–264) uses `m3->stringBlock->seek(geoset.nameCharStart)` then `m3->stringBlock->readString(geoset.nameCharCount)`. This mutates the BufferWrapper's read position, which could cause issues if stringBlock is read multiple times or concurrently.

### 396. [M3Exporter.cpp] `exportAsGLTF()` missing `gltf.setTextureMap()` call
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` line 92
- **Status**: Pending
- **Details**: JS calls `gltf.setTextureMap(textureMap)` at line 92 after `exportTextures`. C++ (lines 97–100) calls `exportTextures` but has a comment "Currently exportTextures is a stub returning empty map, so this is a no-op" — the `setTextureMap` call is present in JS but the C++ omits it (or passes a different type). When texture export is eventually implemented, C++ must add this call.

### 397. [M3Exporter.cpp] `exportAsOBJ()` commented-out collision export block
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 176–184
- **Status**: Pending
- **Details**: Both JS and C++ have the collision export code commented out (C++ lines 211–219). This is a pre-existing TODO in both versions — collision export for M3 models is not yet implemented. The commented code references `this.m2.collisionPositions` (wrong model type — should be m3), suggesting it was copied from M2Exporter.

### 398. [WMOExporter.cpp] `exportTextures()` — `runtimeData` bounds checks added defensively
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 100–102
- **Status**: Pending
- **Details**: JS unconditionally accesses `material.runtimeData[0]` through `[3]` for shader type 23. C++ (lines 241–244) guards each access with `if (material.runtimeData.size() > N)`. If runtimeData has fewer than 4 elements, fewer textures are added. This is a defensive improvement but changes behaviour — JS would crash, C++ silently skips.

### 399. [WMOExporter.cpp] `formatUnknownFile` call signature differs
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` line 160
- **Status**: Pending
- **Details**: JS calls `listfile.formatUnknownFile(texFile)` with one argument (the full filename string like `"12345.png"`). C++ (line 310) calls `casc::listfile::formatUnknownFile(texFileDataID, raw ? ".blp" : ".png")` with two arguments (integer ID + extension). Both must produce identical path strings for the output to match.

### 400. [WMOExporter.cpp] `exportRaw()` — `groupFileDataID == 0` fallback vs JS `??` (nullish coalescing)
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` line 1232
- **Status**: Pending
- **Details**: JS uses `this.wmo.groupIDs?.[groupOffset] ?? listfile.getByFilename(groupName)` — `??` only falls back on `undefined`/`null`, NOT on `0`. A groupID of `0` stays `0`. C++ (lines 1578–1584) falls back to filename lookup when `groupFileDataID == 0`. A groupID of `0` triggers the filename lookup in C++ but not in JS, potentially exporting WMO groups that JS would skip.

### 401. [WMOExporter.cpp] `exportAsGLTF()` — log format missing `toUpperCase()` for format name
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 225, 232
- **Status**: Pending
- **Details**: JS uses `format.toUpperCase()` producing `"GLTF"` / `"GLB"` in log output. C++ (lines 376, 383) uses `format` as-is, logging lowercase `"gltf"` / `"glb"`. Minor cosmetic deviation in log messages.

### 402. [WMOExporter.cpp] `exportRaw()` data check — `data === undefined` vs `data.byteLength() == 0`
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` line 1189
- **Status**: Pending
- **Details**: JS checks `if (this.wmo.data === undefined)` — only true when no buffer was provided at all. C++ (line 1531) checks `if (data.byteLength() == 0)` — true for any empty buffer. An explicitly-provided empty buffer would take the CASC-fetch branch in C++ but the write-buffer branch in JS.

### 403. [WMOExporter.h] Extra `loadWMO()` and `getDoodadSetNames()` methods not in JS
- **JS Source**: N/A
- **Status**: Pending
- **Details**: C++ header (lines 157–165) declares `loadWMO()` and `getDoodadSetNames()` utility methods not present in the JS WMOExporter class. These are additive API extensions for callers that need doodad set names before export. While not a functional deviation, they extend the API surface beyond the JS original.

### 404. [WMOLegacyExporter.cpp] `doodadSetMask` check differs — C++ uses `std::optional` with `has_value()`
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` line 265
- **Status**: Pending
- **Details**: JS checks `if (!doodadSetMask?.[i]?.checked)` — optional chaining handles both missing mask and missing entry gracefully. C++ (line 320) checks `!doodadSetMask.has_value() || i >= doodadSetMask->size() || !(*doodadSetMask)[i].checked`. While functionally equivalent, C++ uses `std::optional<std::vector<>>` which was not used for `groupMask` in the JS (JS uses simple truthiness). The `doodadSetMask` member is also `std::optional` (header line 103) while JS has no explicit nullability — it just checks via optional chaining.

### 405. [WMOLegacyExporter.cpp] `groupMask` stored as `std::optional<std::vector<>>` — JS uses simple truthiness
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 169–175
- **Status**: Pending
- **Details**: JS checks `if (groupMask)` (truthy check on the array). C++ stores `groupMask` as `std::optional<std::vector<WMOGroupMaskEntry>>` (header line 102), checking `groupMask.has_value()`. While functionally equivalent for the "set or not set" case, an empty array in JS is truthy (creates empty mask set), while an empty optional vector in C++ means "no mask". `setGroupMask` should set `std::optional` even if the vector is empty to match JS truthy semantics.

### 406. [WMOLegacyExporter.cpp] Meta JSON serializes materials with explicit field list
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` line 383
- **Status**: Pending
- **Details**: JS does `json.addProperty('materials', wmo.materials)` which serializes the entire materials array as-is with all properties. C++ (lines 476–493) manually serializes each material field (`flags`, `shader`, `blendMode`, `texture1`, etc.). If the WMOLegacyLoader material struct has additional fields, they will be missing from C++ JSON output. Same applies to `doodadSets` (C++ lines 497–505) and `doodads` (C++ lines 509–519) and `groupInfo` (C++ lines 464–472).

### 407. [WMOLegacyExporter.cpp] `doodadCache` is `std::unordered_set<std::string>` — JS uses module-scope `Set`
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` line 22
- **Status**: Pending
- **Details**: JS `doodadCache` is `new Set()` at module scope. C++ (line 38) uses `std::unordered_set<std::string>` in an anonymous namespace. Functionally equivalent, but the JS `Set` can store any type while C++ is typed to `string`. Both use case-insensitive lookups via `toLower()`. The `clearCache()` static method (JS line 585, C++ implied via header line 94) correctly clears both.

## src/js/3D/gl/ Audit

### 408. [GLContext.cpp] `dispose()` does not null out `canvas`/`gl` — JS sets both to `null`
- **JS Source**: `src/js/3D/gl/GLContext.js` lines 403–407
- **Status**: Pending
- **Details**: JS `dispose()` sets `this.gl = null` and `this.canvas = null` to release the WebGL2 context and canvas reference. C++ `dispose()` (line 289–291) is empty — the comment says the context is cleaned up when GLFW window is destroyed. This is an intentional adaptation, but means C++ callers cannot check for a disposed context (JS callers could check `this.gl === null`). If any code relies on that null check to detect a disposed context, the C++ version will not match.

### 409. [GLContext.cpp] Constructor takes no arguments — JS takes `canvas` and `options`
- **JS Source**: `src/js/3D/gl/GLContext.js` lines 29–45
- **Status**: Pending
- **Details**: JS constructor accepts a canvas element and an options object (`antialias`, `alpha`, `preserveDrawingBuffer`, `powerPreference`) and creates a WebGL2 context. C++ constructor (line 15) takes no arguments — the OpenGL context is created by GLFW externally. This is a necessary adaptation, but means any code that passes options to the GLContext constructor would need to be reworked.

### 410. [GLContext.h] `_bound_textures` uses `GLuint` (0 == no texture) — JS uses `null`
- **JS Source**: `src/js/3D/gl/GLContext.js` line 79
- **Status**: Pending
- **Details**: JS `_bound_textures` is `new Array(16).fill(null)` — `null` is used to indicate no texture bound. C++ (header line 110) uses `std::array<GLuint, 16>{}` initialized to `0`. Since OpenGL name `0` means "no texture", this is functionally correct. However, `bind_texture` (C++ line 272) compares `_bound_textures[unit] == texture` — if a valid texture with ID 0 existed (impossible in practice), it would mismatch. Minor concern.

### 411. [GLTexture.cpp] `set_rgba` flip_y option defaults to `true` — JS always flips unconditionally
- **JS Source**: `src/js/3D/gl/GLTexture.js` lines 34–56
- **Status**: Pending
- **Details**: JS `set_rgba()` always sets `gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true)` before upload and resets to `false` after. C++ (line 36) checks `if (options.flip_y && pixels)` before flipping. The C++ `TextureOptions::flip_y` defaults to `true` (header line 30), so the default matches JS. However, if any caller explicitly sets `flip_y = false` in C++, the texture will NOT be flipped, which JS never allows. Also, C++ does a CPU-side row flip (memcpy loop) while JS uses the GPU-side UNPACK_FLIP_Y — functionally equivalent but with different performance characteristics.

### 412. [GLTexture.cpp] `set_canvas` delegates to `set_rgba` — JS uses different `texImage2D` overload
- **JS Source**: `src/js/3D/gl/GLTexture.js` lines 63–85
- **Status**: Pending
- **Details**: JS `set_canvas(canvas, options)` calls `gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, gl.RGBA, gl.UNSIGNED_BYTE, canvas)` with a 6-argument overload that takes the canvas directly (no width/height args — inferred from canvas). C++ `set_canvas` (line 60–65) simply delegates to `set_rgba()` with explicit pixel data and dimensions. This is a necessary adaptation since desktop GL has no canvas object. The JS version sets `has_alpha = true` unconditionally; C++ also does this (line 63).

### 413. [GLTexture.cpp] `_apply_filter` anisotropy constant differs — JS uses extension property, C++ uses core GL define
- **JS Source**: `src/js/3D/gl/GLTexture.js` lines 131–143
- **Status**: Pending
- **Details**: JS accesses `this.ctx.ext_aniso.TEXTURE_MAX_ANISOTROPY_EXT` — the extension object property. C++ (line 102) uses `GL_TEXTURE_MAX_ANISOTROPY` — the core GL 4.6 define. Functionally identical (same enum value `0x84FE`), but the JS also checks `this.ctx.ext_aniso` as a truthy extension object, while C++ checks `ctx_.ext_aniso` as a bool. Both work correctly.

### 414. [GLTexture.h] `BLPTextureFlags` uses `std::optional<bool>` — JS uses `flags.wrap_s ?? (flags.flags & 0x1)`
- **JS Source**: `src/js/3D/gl/GLTexture.js` lines 167–168
- **Status**: Pending
- **Details**: JS `set_blp` determines wrap modes using nullish coalescing: `flags.wrap_s ?? (flags.flags & 0x1)`. If `wrap_s` is `undefined`/`null`, it falls back to the bitflag. C++ uses `std::optional<bool>` with `value_or()` (line 114): `flags.wrap_s.value_or((flags.flags & 0x1) != 0)`. Functionally equivalent. However, in JS the expression `flags.flags & 0x1` evaluates to a number (0 or 1), which is then used as a boolean — `0` is falsy, `1` is truthy. C++ adds explicit `!= 0` comparison, which is correct.

### 415. [ShaderProgram.cpp] `_compile` leak-guards added for first-time compilation — JS only guards in `recompile()`
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 29–55
- **Status**: Pending
- **Details**: JS `_compile()` checks `if (!vert_shader || !frag_shader) return;` but does NOT clean up the successfully compiled shader if the other fails (JS line 35–36). C++ `_compile()` (lines 26–31) properly deletes both shaders if either fails. This is a C++ improvement that prevents GPU resource leaks, but diverges from the JS behaviour (JS leaks the successful shader in this edge case).

### 416. [ShaderProgram.cpp] Uniform null-check uses `-1` — JS uses `!== null`
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 131–135
- **Status**: Pending
- **Details**: JS `set_uniform_1i` checks `if (loc !== null)` — WebGL returns `null` for unknown uniforms. C++ (line 117) checks `if (loc != -1)` — desktop GL returns `-1` for unknown uniforms. Both are correct for their respective APIs. The location cache also differs: JS stores `null` for unfound uniforms, C++ stores `-1`. This is a correct adaptation.

### 417. [ShaderProgram.cpp] `set_uniform_3fv` / `set_uniform_4fv` take extra `count` parameter
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 187–201
- **Status**: Pending
- **Details**: JS `set_uniform_3fv(name, value)` and `set_uniform_4fv(name, value)` pass the array directly — WebGL infers the count from the array length. C++ versions (lines 147–158) take an extra `GLsizei count` parameter (defaulting to 1 in the header). This extends the API beyond the JS original — callers can pass multiple vec3/vec4 values at once in C++ but not in JS. Not a bug, but an API deviation.

### 418. [ShaderProgram.cpp] `set_uniform_mat4_array` takes extra `count` parameter — JS infers from array
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 230–234
- **Status**: Pending
- **Details**: JS `set_uniform_mat4_array(name, transpose, value)` passes the entire flat array to `gl.uniformMatrix4fv()` — WebGL infers the matrix count from array length / 16. C++ (line 175–182) requires an explicit `count` parameter. This is a necessary adaptation since C++ glUniformMatrix4fv requires count. The default is `count = 1` (header line 51), which would only upload one matrix — callers must explicitly pass count for multi-matrix uploads.

### 419. [ShaderProgram.h] `_unregister_fn` is a global static callback — JS uses lazy `require('../Shaders')`
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 288–292
- **Status**: Pending
- **Details**: JS `dispose()` uses a lazy `require('../Shaders')` to call `Shaders.unregister(this)`. C++ (header line 68) uses a static `std::function<void(ShaderProgram*)> _unregister_fn` callback that must be set externally. The Shaders module must assign this callback during initialization. If the callback is never set (e.g., Shaders module not yet converted), dispose() silently skips unregistration. The comment on header line 67 acknowledges this.

### 420. [UniformBuffer.cpp] `upload` uses total buffer size — JS uploads `this.data` (ArrayBuffer)
- **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 179–186
- **Status**: Pending
- **Details**: JS `upload()` calls `gl.bufferSubData(gl.UNIFORM_BUFFER, 0, this.data)` — uploads the entire ArrayBuffer. C++ (line 97–99) calls `glBufferSubData(GL_UNIFORM_BUFFER, 0, static_cast<GLsizeiptr>(size), data_.data())` — uses `size` member. Both upload the full buffer, but if `size` and `data_.size()` ever diverge (e.g., after a resize), the C++ version would use the wrong size. Currently they are always equal.

### 421. [UniformBuffer.cpp] `upload_range` does not set `dirty = false` — JS also does not
- **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 193–196
- **Status**: Pending
- **Details**: Neither JS nor C++ `upload_range()` modifies the `dirty` flag. This means after a partial upload, `dirty` remains `true`, and a subsequent `upload()` call will re-upload the entire buffer. Both versions have this same behaviour — this is a match but may be a subtle shared bug if callers expect `upload_range` to clear dirtiness.

### 422. [UniformBuffer.cpp] `dispose` clears and shrinks vector — JS nulls out views
- **JS Source**: `src/js/3D/gl/UniformBuffer.js` lines 198–208
- **Status**: Pending
- **Details**: JS `dispose()` sets `this.data = null`, `this.view = null`, `this.float_view = null`, `this.int_view = null` — releasing all typed array views. C++ (lines 109–117) calls `data_.clear()` and `data_.shrink_to_fit()` — releasing vector memory. Both correctly free CPU resources. The buffer deletion uses `glDeleteBuffers` in C++ and `gl.deleteBuffer` in JS — both correct.

### 423. [VertexArray.cpp] `set_vertex_buffer` takes raw `void*` + size — JS takes typed array directly
- **JS Source**: `src/js/3D/gl/VertexArray.js` lines 45–54
- **Status**: Pending
- **Details**: JS `set_vertex_buffer(data, usage)` passes the typed array (Float32Array/ArrayBuffer) directly to `gl.bufferData()` — WebGL infers size from the array. C++ (line 20–28) takes `const void* data` + `size_t size_bytes` + `GLenum usage`. Callers must compute and pass the byte size. This is a necessary adaptation but changes the API surface.

### 424. [VertexArray.cpp] `set_index_buffer` split into two overloads — JS auto-detects type
- **JS Source**: `src/js/3D/gl/VertexArray.js` lines 61–77
- **Status**: Pending
- **Details**: JS `set_index_buffer(data, usage)` accepts either `Uint16Array` or `Uint32Array` and uses `instanceof` to determine `index_type`. C++ provides two overloads: `set_index_buffer(const uint16_t*, ...)` and `set_index_buffer(const uint32_t*, ...)` (lines 30–56). Both correctly set `index_type` to `GL_UNSIGNED_SHORT` or `GL_UNSIGNED_INT` respectively. Functionally equivalent.

### 425. [VertexArray.cpp] `draw()` uses `count < 0` sentinel — JS uses `count ?? this.index_count`
- **JS Source**: `src/js/3D/gl/VertexArray.js` lines 283–286
- **Status**: Pending
- **Details**: JS `draw(mode, count, offset = 0)` uses nullish coalescing `count = count ?? this.index_count` — if count is `undefined`, uses full index count. C++ (line 251–258) uses `GLsizei count = -1` default and checks `if (count < 0) count = index_count`. Functionally equivalent, but `-1` is a less natural sentinel than `undefined`. Both compute byte offset identically: `offset * (index_type == UNSIGNED_INT ? 4 : 2)`.

## src/js/3D/loaders Audit

### 426. [M2Loader.cpp] `loadAnims()` missing `animIsChunked` parameter — always loads with default `isChunked=true`
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 118–124
- **Status**: Pending
- **Details**: JS computes `animIsChunked` based on `(this.flags & 0x200000) === 0x200000 || this.skeletonFileID > 0` and passes it to `loader.load(animIsChunked)`. The C++ `loadAnims()` (line 121) calls `loader->load()` without any argument, relying on the default `isChunked=true`. This means non-chunked anim files (where `animIsChunked` would be false) are incorrectly parsed as chunked. The JS explicitly checks flags and skeletonFileID to determine the format.

### 427. [M2Loader.cpp] `loadAnims()` missing `skeletonBoneData` vs `animData` selection after ANIMLoader
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 126–129
- **Status**: Pending
- **Details**: After loading an anim file, JS checks `if (loader.skeletonBoneData !== undefined)` and stores either `BufferWrapper.from(loader.skeletonBoneData)` or `BufferWrapper.from(loader.animData)`. The C++ (line 124) simply stores the original `bufPtr` pointer to the raw data buffer, bypassing the ANIMLoader's parsed chunk data entirely. This means the anim file data stored in `animFiles` is the raw file buffer, not the correct skeleton bone or anim data chunk.

### 428. [M2Loader.cpp] `loadAnims()` missing `_patch_bone_animation()` call after anim file load
- **JS Source**: `src/js/3D/loaders/M2Loader.js` line 132
- **Status**: Pending
- **Details**: JS calls `this._patch_bone_animation(i)` after successfully storing an anim file buffer, which patches the animation data into the bone transformation tracks. The C++ `loadAnims()` never calls `_patch_bone_animation()` after storing the anim file, so loaded animation data is never integrated into bones. The `_patch_bone_animation` method exists in C++ (line 197) but is never called from `loadAnims()`.

### 429. [M2Loader.cpp] `loadAnimsForIndex()` missing `animIsChunked` parameter — always loads with default
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 181–187
- **Status**: Pending
- **Details**: Same issue as entry 426 but in `loadAnimsForIndex()`. JS computes `animIsChunked` based on `(this.flags & 0x200000) === 0x200000 || this.skeletonFileID > 0` and passes it to `loader.load(animIsChunked)`. The C++ (line 181) calls `loader->load()` without argument, always using `isChunked=true`.

### 430. [M2Loader.cpp] `loadAnimsForIndex()` missing `skeletonBoneData` vs `animData` selection
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 190–193
- **Status**: Pending
- **Details**: Same issue as entry 427 but in `loadAnimsForIndex()`. JS selects between `loader.skeletonBoneData` and `loader.animData` after loading. The C++ (line 182) simply stores the raw buffer pointer instead of selecting the appropriate parsed data.

### 431. [M2Loader.cpp] `loadAnimsForIndex()` missing `_patch_bone_animation()` call
- **JS Source**: `src/js/3D/loaders/M2Loader.js` line 196
- **Status**: Pending
- **Details**: Same issue as entry 428 but in `loadAnimsForIndex()`. JS calls `this._patch_bone_animation(animationIndex)` after storing the buffer. The C++ returns `true` on line 183 immediately after storing the buffer, without ever calling `_patch_bone_animation()`.

### 432. [M2Loader.cpp] `parseChunk_MD21_textures()` seeks to `nameOfs` without adding `ofs` — differs from legacy loader
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 787–798
- **Status**: Pending
- **Details**: In the JS M2Loader `parseChunk_MD21_textures`, `this.data.seek(nameOfs)` is used (line 790), NOT `nameOfs + ofs`. This differs from M2LegacyLoader which uses `data.seek(nameOfs + ofs)` (line 534). The C++ M2Loader (line 770) also uses `this->data.seek(nameOfs)` without ofs, matching the JS. However, the JS M2Loader also checks only `nameOfs > 0` (line 787), while M2LegacyLoader checks `nameOfs > 0 && nameLength > 0`. The C++ M2Loader (line 767) matches JS by checking only `nameOfs > 0`.

### 433. [SKELLoader.cpp] `loadAnims()` missing `skeletonBoneData` vs `animData` selection after ANIMLoader
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 441–444
- **Status**: Pending
- **Details**: After loading an anim file, JS SKELLoader checks `if (loader.skeletonBoneData !== undefined)` and stores either `BufferWrapper.from(loader.skeletonBoneData)` or `BufferWrapper.from(loader.animData)`. The C++ (line 446) simply stores the raw buffer pointer `bufPtr`, bypassing the ANIMLoader's parsed chunk data. This means the animation data buffer stored in `animFiles` is the raw file, not the correct parsed skeleton bone or anim data chunk.

### 435. [SKELLoader.cpp] `loadAnims()` missing `_patch_bone_animation()` call after storing anim buffer
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` line 447
- **Status**: Pending
- **Details**: JS calls `this._patch_bone_animation(i)` after storing an anim file buffer in `loadAnims()`, which patches the loaded animation data into bone transformation tracks. The C++ `loadAnims()` (lines 438–449) never calls `_patch_bone_animation()`. The method exists (line 351) but is never invoked from `loadAnims()`, so loaded animation data is never integrated into bones.

### 436. [SKELLoader.cpp] `loadAnimsForIndex()` missing `skeletonBoneData` vs `animData` selection
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 335–338
- **Status**: Pending
- **Details**: Same issue as entry 433 but in `loadAnimsForIndex()`. JS selects between `loader.skeletonBoneData` and `loader.animData`. The C++ (line 337) stores the raw buffer pointer.

### 437. [SKELLoader.cpp] `loadAnimsForIndex()` missing `_patch_bone_animation()` call
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` line 341
- **Status**: Pending
- **Details**: Same issue as entry 435 but in `loadAnimsForIndex()`. JS calls `this._patch_bone_animation(animation_index)` after storing the buffer. The C++ returns `true` on line 338 without calling `_patch_bone_animation()`.

### 438. [M2Loader.cpp] `parseChunk_MD21_modelName()` seeks twice to the same offset unnecessarily
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 607–618
- **Status**: Pending
- **Details**: Both JS (lines 612, 615) and C++ (lines 583, 586) call `seek(modelNameOfs + ofs)` twice in succession. The first seek serves no purpose. This is a bug in the original JS that was faithfully ported, but it wastes a seek call. The JS has: `this.data.seek(modelNameOfs + ofs);` on line 612 (after saving base), then immediately `this.data.seek(modelNameOfs + ofs);` again on line 615. The C++ mirrors this exactly.

### 439. [M2Loader.h] `globalLoops` declared as `std::vector<int16_t>` — JS reads `readInt16LE` but legacy loader reads `readUInt32LE`
- **JS Source**: `src/js/3D/loaders/M2Loader.js` line 883
- **Status**: Pending
- **Details**: JS M2Loader reads globalLoops as `this.data.readInt16LE(globalLoopCount)` (line 883), and the C++ matches this with `readInt16LE()` and `std::vector<int16_t>`. However, the legacy M2 loader (M2LegacyLoader.js line 125) reads globalLoops as `data.readUInt32LE(count)` (unsigned 32-bit). This appears to be a semantic difference in the original JS between the modern and legacy M2 formats. The C++ M2Loader faithfully matches the JS M2Loader.

### 440. [M2LegacyLoader.h] `LegacyM2SubMesh::triangleStart` declared as `uint32_t` but initially read as `uint16_t`
- **JS Source**: `src/js/3D/loaders/M2LegacyLoader.js` lines 445–460
- **Status**: Pending
- **Details**: In the JS, `triangleStart` is read as `data.readUInt16LE()` (line 445), then modified via `triangleStart += level << 16` (line 460) which can produce values > 16 bits. The C++ header (M2LegacyLoader.h line 138) correctly declares `uint32_t triangleStart` to hold the combined value. The C++ read (line 436) reads `data.readUInt16LE()` and then adds `sm.level << 16` (line 450). This is functionally correct and matches the JS behavior — the uint32_t type is the right choice.

### 441. [WMOLegacyLoader.cpp] `parse_MOGP()` alpha portal fields use `uint16_t` cast — JS uses `readUInt32LE` for alpha
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 457–464
- **Status**: Pending
- **Details**: JS WMOLegacyLoader MOGP handler uses `data.readUInt32LE()` for both `ofsPortals` and `numPortals` when `this.version === WMO_VER_ALPHA` (lines 459–460), and `data.readUInt16LE()` for non-alpha (lines 462–463). The C++ header (WMOLegacyLoader.h lines 126–127) declares both fields as `uint16_t`, which would truncate values from `readUInt32LE()` in alpha format. Need to verify that the C++ parse_MOGP reads them correctly for alpha format.

### 442. [WMOLegacyLoader.cpp] `parse_MOMT()` alpha material entry size and version field — verified correct
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 240–289
- **Status**: Verified — C++ matches JS
- **Details**: JS MOMT handler uses `entrySize = 0x40` (64 bytes) for alpha and standard format, then alpha parsing reads 52 bytes of fields followed by `data.move(entrySize - 52)` to skip the rest (line 269). C++ (lines 327–366 in WMOLegacyLoader.cpp) correctly reads the per-material version field `data.readUInt32LE()` and discards it, reads all 12 material fields (52 bytes total), and skips remaining padding with `data.move(entrySize - 52)`. This matches JS exactly.

### 443. [WMOLegacyLoader.cpp] `parse_MODD()` uses `readUInt24LE` — verified working
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` line 382
- **Status**: Verified — C++ has readUInt24LE
- **Details**: JS reads doodad offset with `data.readUInt24LE()` (line 382). C++ (line 463) also calls `data.readUInt24LE()`, confirming BufferWrapper has this method. No issue.

### 444. [WMOLegacyLoader.cpp] `parse_MODN()` converts `.mdx` to `.m2` — verified correct
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 371–373
- **Status**: Verified — C++ matches JS
- **Details**: After reading doodad names, JS iterates all entries and converts: lowercases and replaces `.mdx` with `.m2`. C++ (lines 447–453 in WMOLegacyLoader.cpp) performs the same conversion using `std::transform` for lowercasing and `file.find(".mdx")` / `file.replace()` for extension replacement. This matches JS behavior.

### 445. [WMOLegacyLoader.h] `ofsPortals` and `numPortals` should be `uint32_t` to handle alpha format
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 459–460
- **Status**: Pending
- **Details**: As noted in entry 441, the JS reads these as `readUInt32LE()` for alpha format. The C++ header declares them as `uint16_t` (lines 126–127). For alpha format WMOs (version 14), these fields should be 32-bit to avoid data truncation. The field types should be widened to `uint32_t`.

### 446. [M2Loader.cpp] `parseChunk_MD21_textures()` does not check `nameLength > 0` — JS also omits this check
- **JS Source**: `src/js/3D/loaders/M2Loader.js` line 787
- **Status**: Pending
- **Details**: JS M2Loader checks `if (textureType === 0 && nameOfs > 0)` without checking `nameLength > 0` (line 787). This differs from M2LegacyLoader which checks both `nameOfs > 0 && nameLength > 0` (line 532). The C++ M2Loader (line 767) matches JS M2Loader by only checking `nameOfs > 0`. This means zero-length texture names could trigger a seek and read in M2Loader but would be filtered in M2LegacyLoader. Both C++ and JS are consistent within their respective loaders.

### 447. [M2Loader.cpp] `parseChunk_MD21_textures()` does not use `replace(/\0/g, '')` — uses `std::remove` instead
- **JS Source**: `src/js/3D/loaders/M2Loader.js` line 792
- **Status**: Pending
- **Details**: JS M2Loader reads `fileName = this.data.readString(nameLength)` then does `fileName.replace('\0', '')` (line 792). Note: this JS code has a bug — it uses `replace('\0', '')` (string) instead of `replace(/\0/g, '')` (regex), so it only removes the FIRST null character. The C++ (line 773) uses `fileName.erase(std::remove(...), fileName.end())` which correctly removes ALL null characters. The C++ is actually more correct than the JS.

### 448. [MDXLoader.cpp] Node registration deferred to post-parse — acceptable design difference from JS
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` lines 208–209
- **Status**: Verified — Acceptable design difference
- **Details**: JS `_read_node` immediately registers nodes: `if (node.objectId !== null) this.nodes[node.objectId] = node;` (lines 208–209). The C++ defers all node registration to after parsing is complete (lines 76–106), building a `nodes` lookup from final container addresses. This is documented with a comment explaining the C++ difference (pointer stability). The behavior is functionally equivalent — nodes are still accessible by objectId after loading — but the timing differs. This is the correct approach in C++ since objects are moved into vectors after `_read_node` returns, which would invalidate any pointers.

### 449. [M2Loader.cpp] `globalLoops` uses `readInt16LE` — possible shared bug with JS
- **JS Source**: `src/js/3D/loaders/M2Loader.js` line 883
- **Status**: Pending
- **Details**: The M2 format specification defines global loops (global sequences) as 32-bit unsigned timestamps, but both JS M2Loader (line 883) and C++ M2Loader (line 859) read them as `readInt16LE`. This matches the original JS behavior but may be incorrect per the M2 format specification. The legacy loader (M2LegacyLoader.js line 125) correctly uses `readUInt32LE`. Both C++ and JS modern loaders are consistent with each other but may have the same bug.

### 451. [M2LegacyLoader.cpp] `_parse_header()` parse sequence — verified correct
- **JS Source**: `src/js/3D/loaders/M2LegacyLoader.js` lines 75–105
- **Status**: Verified — Correct
- **Details**: C++ `_parse_header()` (lines 63–97 in M2LegacyLoader.cpp) calls the same methods in the same order as the JS: model_name, global_loops, animations, animation_lookup, playable_animation_lookup, bones, (skip 8 bytes), vertices, views_inline (pre-WotLK only), colors, textures, texture_weights, texture_transforms, replaceable_texture_lookup, materials, (skip 8 bytes), texture_combos, (skip 8 bytes), transparency_lookup, texture_transform_lookup, bounding_box, collision, attachments. The sequence matches JS exactly.

### 452. [WMOLegacyLoader.h] `ofsPortals` and `numPortals` `uint16_t` truncation confirmed — duplicate of entry 441/445
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 457–464
- **Status**: Pending
- **Details**: C++ `parse_MOGP()` (lines 531–534) reads `data.readUInt32LE()` for alpha format but casts to `uint16_t` via `static_cast<uint16_t>()`, which truncates values above 65535. The header declares both fields as `uint16_t`. For full fidelity with JS (which stores the full 32-bit values), these should be widened to `uint32_t`. See entries 441 and 445.

## src/js/3D/renderers Audit

### 453. [CharMaterialRenderer.cpp] `setTextureTarget()` flattened parameter signature loses structured data
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 114–143
- **Status**: Pending
- **Details**: The JS `setTextureTarget()` accepts four structured objects (`chrCustomizationMaterial`, `charComponentTextureSection`, `chrModelMaterial`, `chrModelTextureLayer`) and one optional `blpOverride`. The C++ version (lines 128–135) flattens these into 12 scalar parameters. While all required fields are passed, the flattening means the caller must know the internal field layout of each structure, rather than passing opaque objects. The JS stores all four original objects in the `textureTargets` entry for later reference (e.g. `layer.textureLayer.BlendMode`), whereas C++ only stores the specific fields it extracts. Fields like `textureLayer.TextureType`, `textureLayer.Layer`, `textureLayer.Flags`, `textureLayer.TextureSectionTypeBitMask`, `textureLayer.TextureSectionTypeBitMask2`, `section.SectionType`, `section.OverlapSectionMask`, `material.Flags`, and `material.Unk` are declared in the C++ `CharTextureTarget` struct but never populated by `setTextureTarget()`, leaving them at default values (0).

### 454. [CharMaterialRenderer.cpp] `update()` draw call placed inside blend mode 4/6/7 branch instead of after it
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 382–418
- **Status**: Pending
- **Details**: In the JS `update()` method, `gl.drawArrays(gl.TRIANGLES, 0, 6)` is called once at line 417 — outside and after the blend mode 4/6/7 `if` block. In the C++ `update()` (lines 488–543), `glDrawArrays(GL_TRIANGLES, 0, 6)` is called in two places: line 533 inside the `if` branch (for blend modes 4/6/7) and line 542 inside the `else` branch. While functionally equivalent (drawArrays is always called once per layer), the JS has a single call after the if-block whereas C++ duplicates the call in both branches. Not a bug, but a structural divergence.

### 455. [CharMaterialRenderer.cpp] `compileShaders()` returns on error instead of throwing
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 244–257
- **Status**: Pending
- **Details**: The JS `compileShaders()` throws `new Error('Failed to compile vertex shader')` and `new Error('Failed to compile fragment shader')` and `new Error('Failed to link shader program')` on failure. The C++ version (lines 297–365) logs the error and returns early without throwing or reporting the failure to the caller. Callers have no way to know that shader compilation failed — they'll proceed with a partially initialized renderer (e.g., `glShaderProg` may be 0 or partially linked).

### 456. [CharMaterialRenderer.cpp] `getCanvas()` returns FBO texture ID instead of canvas reference
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 57–59
- **Status**: Pending
- **Details**: The JS `getCanvas()` returns `this.glCanvas` (the HTML canvas DOM element). The C++ `getCanvas()` (header line 96) returns `fbo_texture_` (a GLuint texture ID). While this is the appropriate C++ analogue, any caller expecting a canvas-like object (with `.width`, `.height`, `.toDataURL()`, etc.) would need to use different APIs. The header documents this difference.

### 457. [CharMaterialRenderer.cpp] `update()` has extra `glClearColor(0.5)` after `clearCanvas()` already sets black
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 286–289
- **Status**: Pending
- **Details**: The JS `update()` calls `this.clearCanvas()` which sets `glClearColor(0, 0, 0, 1)` and clears, then immediately sets `glClearColor(0.5, 0.5, 0.5, 1)` (line 288) without another clear call. This effectively sets the clear color for any future clears to gray, but the canvas has already been cleared to black. The C++ (lines 382–386) replicates this exactly, including the redundant `glClearColor(0.5, 0.5, 0.5, 1)` after clearing to black. This is not a bug — it matches JS exactly — but it is a quirk: the gray clear color is set but never used (no subsequent clear call). Both versions behave identically.

### 458. [M2LegacyRendererGL.cpp] Reactive Vue watchers for geosets and wireframe not implemented
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 196–198
- **Status**: Pending
- **Details**: When `reactive` is true, the JS registers two Vue watchers: `this.geosetWatcher = core.view.$watch(this.geosetKey, () => this.updateGeosets(), { deep: true })` and `this.wireframeWatcher = core.view.$watch('config.modelViewerWireframe', () => {}, { deep: true })`. The C++ has an empty `if (reactive) { }` block (lines 216–217), meaning geoset checkbox changes from the UI will never trigger `updateGeosets()` at runtime. The corresponding dispose code (`this.geosetWatcher?.()`, `this.wireframeWatcher?.()`) is also missing.

### 459. [M2LegacyRendererGL.cpp] `_load_textures()` calls `setSlotFileLegacy` instead of `setSlotFile`
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` line 226
- **Status**: Pending
- **Details**: The JS calls `textureRibbon.setSlotFile(ribbonSlot, fileName, this.syncID)`. The C++ (line 255) calls `texture_ribbon::setSlotFileLegacy(ribbonSlot, fileName, syncID)`. These are two different functions — `setSlotFile` vs `setSlotFileLegacy`. The C++ is calling the wrong function name.

### 460. [M2LegacyRendererGL.cpp] `_load_textures()` has extra fallback to `texture.getTextureFile()` not in JS
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 228–236
- **Status**: Pending
- **Details**: The JS only has one code path for getting file data: `const data = mpq.getFile(fileName)`. The C++ (lines 264–266) adds an `else` branch: `file_data = texture.getTextureFile();` when `mpq` is null. This fallback does not exist in the original JS and introduces a code path not present in the source.

### 461. [M2LegacyRendererGL.cpp] `_create_skeleton()` initializes boneless bone_matrices to identity instead of zeros
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 443–444
- **Status**: Pending
- **Details**: When there are no bones, the JS creates `this.bone_matrices = new Float32Array(16)`, which produces 16 zeros. The C++ (lines 534–536) does `bone_matrices.assign(16, 0.0f)` followed by `std::copy(IDENTITY_MAT4.begin(), IDENTITY_MAT4.end(), bone_matrices.begin())`, producing an identity matrix. This changes the bone matrix data for boneless models from all-zeros to identity, which will affect rendering.

### 462. [M2LegacyRendererGL.cpp] `render()` u_time uniform uses epoch-based time instead of app-relative time
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` line 926
- **Status**: Pending
- **Details**: The JS uses `performance.now() * 0.001` for the `u_time` uniform, which returns milliseconds since page load (a relatively small, stable value). The C++ (lines 1139–1141) uses `std::chrono::steady_clock::now().time_since_epoch().count()`, which returns a very large number (seconds since epoch). This causes floating-point precision loss in the `u_time` uniform and the time value has completely different semantics (epoch-based vs app-start-based), which will affect any time-dependent shader effects.

### 463. [M2LegacyRendererGL.cpp] `_dispose_skin()` does not delete GPU buffer objects — resource leak
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 1025–1035
- **Status**: Pending
- **Details**: In `_dispose_skin()`, the JS does `this.buffers = []` which drops references (WebGL garbage-collects them). The C++ does `buffers.clear()` (line 1266) but never calls `glDeleteBuffers()` on the buffer handles stored in the `buffers` vector before clearing. This is a GPU resource leak — the OpenGL buffer objects are orphaned since OpenGL requires explicit deletion.

### 464. [M2LegacyRendererGL.cpp] `loadSkin()` reactive geoset mapping uses two-step copy instead of in-place mutation
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 430–433
- **Status**: Pending
- **Details**: The JS sets `core.view[this.geosetKey] = this.geosetArray` and calls `GeosetMapper.map(this.geosetArray)` which mutates the array entries' labels in-place. The C++ (lines 491–519) creates a separate `mapper_geosets` vector, passes that to `geoset_mapper::map()`, then copies labels back in a separate loop. This two-step copy-back approach could miss label updates if the mapper modifies fields beyond `label`.

### 465. [M2LegacyRendererGL.h] `M2LegacyDrawCall::count` is `uint16_t` — may truncate large submesh triangle counts
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` (draw call count assignment)
- **Status**: Pending
- **Details**: The `M2LegacyDrawCall::count` field is declared as `uint16_t` (header line 45). In JS, `dc.count` is a regular JS number (64-bit float, effectively unlimited for integers). If `submesh.triangleCount` exceeds 65535, the C++ value will overflow/truncate.

### 466. [M2RendererGL.cpp] Reactive Vue watchers for geosets, wireframe, and bones not implemented
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 381–384
- **Status**: Pending
- **Details**: JS registers three Vue watchers when `reactive` is true: `geosetWatcher`, `wireframeWatcher`, and `bonesWatcher`. The C++ `load()` method (lines 495–496) has an empty `if (reactive) {}` block — none of the three watchers are implemented. The corresponding `dispose()` cleanup (`this.geosetWatcher?.()`, `this.wireframeWatcher?.()`, `this.bonesWatcher?.()`) is also missing.

### 467. [M2RendererGL.cpp] `stopAnimation()` restores stale animation source flags instead of clearing them
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 712–714
- **Status**: Pending
- **Details**: In JS `stopAnimation()`, after calling `_update_bone_matrices()`, `current_anim_source` is set to `null`. In C++ (lines 943–944), `current_anim_from_skel` and `current_anim_from_child` are restored to previous values (`prev_from_skel`, `prev_from_child`) instead of being reset to `false`. This means C++ retains stale animation source flags after stopping.

### 468. [M2RendererGL.cpp] `_create_skeleton()` initializes boneless bone_matrices to identity instead of zeros
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 632–635
- **Status**: Pending
- **Details**: When no bone data is available, JS creates `this.bone_matrices = new Float32Array(16)` which initializes to all-zeros (16 zero floats). C++ (lines 817–818) initializes with `assign(16, 0.0f)` then copies `M2_IDENTITY_MAT4` into it, producing an actual identity matrix.

### 469. [M2RendererGL.cpp] `_load_textures()` bypasses `texture.getTextureFile()` — loads CASC file directly
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 416–417
- **Status**: Pending
- **Details**: JS loads textures via `const data = await texture.getTextureFile()` which uses the M2Texture object's own method (potentially involving internal logic or caching). C++ (line 535) bypasses this by calling `casc_source_->getVirtualFileByID(texture.fileDataID)` directly. Any logic inside `getTextureFile()` is not executed in C++.

### 470. [M2RendererGL.cpp] `buildBoneRemapTable()` only uses m2 bones, not skel bones
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1003–1054
- **Status**: Pending
- **Details**: JS `buildBoneRemapTable()` uses `this.bones` which can be either m2 bones or skel bones (whatever `_create_skeleton()` assigned). C++ (line 1236) exclusively uses `bones_m2`, returning early with `bone_remap_table.clear()` if `bones_m2` is null. Models that only have skel bones (via SKELLoader) cannot build a bone remap table in C++.

### 471. [M2RendererGL.cpp] `render()` u_time uniform uses static start-time offset instead of page-load time
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` line 1224
- **Status**: Pending
- **Details**: JS sets the `u_time` uniform to `performance.now() * 0.001` (time since page load). C++ uses a `static` `steady_clock` start time captured at first render call. The time bases differ: JS measures from page load, C++ from first render call.

### 472. [M2RendererGL.cpp] `getAttachmentTransform()` uses `uint16_t` for bone_idx — cannot detect negative sentinel values
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1563–1564
- **Status**: Pending
- **Details**: JS `getAttachmentTransform()` checks `if (bone_idx < 0 || !this.bone_matrices)`. C++ declares `bone_idx` as `uint16_t`, making the `< 0` check impossible. If `attachment->bone` represents a sentinel value like -1 (0xFFFF as uint16_t), C++ would interpret it as 65535 and attempt to index into bone_matrices rather than returning nullopt.

### 473. [M2RendererGL.cpp] Reactive geoset key assignment hardcoded to `modelViewerGeosets`
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 580–582
- **Status**: Pending
- **Details**: JS assigns `core.view[this.geosetKey] = this.geosetArray` using dynamic property key from `this.geosetKey` (default `'modelViewerGeosets'`). C++ (line 727) hardcodes `core::view->modelViewerGeosets`. If `geosetKey` is changed via `setGeosetKey()`, C++ still writes to the wrong view property.

### 474. [M2RendererGL.cpp] `updateAnimation()` returns early for zero-duration animations — JS does not
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 722–741
- **Status**: Pending
- **Details**: In JS, if an animation exists but has `duration === 0`, the code still updates `animation_time` and calls `_update_bone_matrices()`. In C++, `_get_anim_duration_ms()` returns 0 for both non-existent and zero-duration animations, and the function returns early without updating bone matrices. Zero-duration animations are silently skipped.

### 475. [M3RendererGL.cpp] `render()` u_time uniform uses epoch-based time — massive float precision loss
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` line 214
- **Status**: Pending
- **Details**: The `u_time` uniform uses completely different time semantics. JS uses `performance.now() * 0.001` (seconds since page load, a small number). C++ uses `std::chrono::high_resolution_clock::now().time_since_epoch()` (seconds since Unix epoch, a huge number ~1.7 billion). This causes float precision loss — a 32-bit float can only represent ~7 significant digits, so sub-second precision is lost.

### 476. [M3RendererGL.h] `model_matrix` is private with no accessor — JS exposes it publicly
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` line 46
- **Status**: Pending
- **Details**: `model_matrix` is a public property in JS that external code can read and write. In the C++ header, `model_matrix` is declared `private` with no getter or setter.

### 477. [M3RendererGL.h] `draw_calls` const accessor prevents mutation — JS allows direct mutation
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 42, 40–43
- **Status**: Pending
- **Details**: `draw_calls`, `vaos`, `buffers`, and `default_texture` are all public in JS. In C++ they are `private`. `draw_calls` has a `const` accessor (`get_draw_calls()`), but JS code that mutates individual entries (e.g., `dc.visible = false`) has no C++ equivalent. The other members have no accessors at all.

### 478. [M3RendererGL.cpp] `getBoundingBox()` returns nullopt for empty vertices — JS returns Infinity/−Infinity bounds
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 173–191
- **Status**: Pending
- **Details**: In JS, an empty vertices array `[]` is truthy, so it passes the guard and returns `{ min: [Infinity, Infinity, Infinity], max: [-Infinity, -Infinity, -Infinity] }`. In C++, `m3->vertices.empty()` returns true, causing the function to return `std::nullopt`. Different return values for the empty-vertices case.

### 479. [M3RendererGL.cpp] Spurious include of `texture-ribbon.h` — not used anywhere in the file
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` (no corresponding import)
- **Status**: Pending
- **Details**: The C++ file includes `#include "../../ui/texture-ribbon.h"` (line 13) but nothing from that header is used anywhere in the file. The JS source has no equivalent import for a texture-ribbon module.

### 480. [MDXRendererGL.cpp] Reactive Vue watchers for geosets and wireframe not implemented
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 185–189
- **Status**: Pending
- **Details**: The reactive watcher setup in `load()` is completely missing. JS sets `core.view[this.geosetKey] = this.geosetArray`, creates a `geosetWatcher` and `wireframeWatcher`. C++ has an empty block `if (reactive && !geosetArray.empty()) { }` with no implementation. Additionally, the C++ guard uses `!geosetArray.empty()` which would NOT enter the block when geosetArray is empty (zero geosets), while JS's `this.geosetArray` truthiness check would enter (empty array is truthy). The corresponding `dispose()` cleanup is also missing.

### 481. [MDXRendererGL.cpp] `_load_textures()` calls `setSlotFileLegacy` instead of `setSlotFile`
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 216
- **Status**: Pending
- **Details**: JS calls `textureRibbon.setSlotFile(ribbonSlot, fileName, this.syncID)`. C++ calls `texture_ribbon::setSlotFileLegacy(ribbonSlot, fileName, syncID)`. Different function name.

### 482. [MDXRendererGL.cpp] `render()` u_time uniform uses epoch-based time instead of app-relative time
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 681
- **Status**: Pending
- **Details**: JS uses `performance.now() * 0.001` for the `u_time` uniform (seconds since page load). C++ uses `std::chrono::steady_clock::now().time_since_epoch().count()` (seconds since epoch). This causes floating-point precision loss and different shader animation behavior.

### 483. [MDXRendererGL.cpp] `_build_geometry()` calls `setup_m2_separate_buffers` with 5 args instead of 6
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 368
- **Status**: Pending
- **Details**: JS calls `vao.setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo, null)` with 6 arguments (the 6th being null). C++ calls `vao->setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo)` with only 5 arguments. If the 6th parameter has a default that differs from null/0, the behavior would diverge.

### 484. [MDXRendererGL.cpp] `render()` texture bind calls wrapped in conditional guards — JS unconditionally binds
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 743–747
- **Status**: Pending
- **Details**: JS unconditionally calls `texture.bind(0)` (guaranteed non-null via `|| this.default_texture` fallback) and `this.default_texture.bind(1)`, `.bind(2)`, `.bind(3)`. C++ wraps all bind calls in `if (texture)` and `if (default_texture)` guards, making them conditional. This changes the rendering contract — if `default_texture` were null, C++ would skip binding slots 1–3 while JS would crash.

### 485. [MDXRendererGL.cpp] `_create_skeleton()` nodes use `std::optional<int>` for objectId — JS uses plain number
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 256–258
- **Status**: Pending
- **Details**: C++ checks `src_nodes[i]->objectId.has_value()` (std::optional), while JS accesses `nodes[i].objectId` directly as a plain number. C++ could skip nodes with no objectId that JS would process. Similarly, in `_update_node_matrices()`, C++ adds an extra guard `if (!node->objectId.has_value())` that JS does not have.

### 486. [MDXRendererGL.cpp] `_build_geometry()` UV fallback references `vertices` vector (wrong size) instead of creating zero-filled UV array
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 298–303
- **Status**: Pending
- **Details**: JS creates a fallback UV array with `new Float32Array(vertCount * 2)` (zero-filled, correct size for UVs). C++ creates a dangling reference `const std::vector<float>& uvs = ... : vertices;` where `vertices` has `vertCount * 3` elements (wrong size for UVs). Although the C++ subsequently creates a correctly-sized `flippedUvs.resize(vertCount * 2, 0.0f)` in the fallback path (so the reference is never dereferenced for the fallback case), the intermediate reference is semantically wrong.

### 487. [WMORendererGL.h] `WMODrawCall::count` is `uint16_t` — may truncate large batch face counts
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` line 309 (`count: batch.numFaces`)
- **Status**: Pending
- **Details**: `count` is declared as `uint16_t`. JS `numFaces` is an unrestricted JS Number. If `numFaces` exceeds 65535, C++ silently truncates, causing rendering corruption.

### 488. [WMORendererGL.cpp] Bug in doodad rotation — guards check `position.size()` but access `rotation[]`
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 405–409
- **Status**: Pending
- **Details**: C++ line 499: the rotation component guard checks `doodad.position.size() > 0` but accesses `doodad.rotation[0]`. It should check `doodad.rotation.size() > 0`. If `position` is non-empty but `rotation` is empty, this is undefined behavior (out-of-bounds access).

### 489. [WMORendererGL.cpp] Reactive watchers replaced with polling mechanism — architectural difference
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 105–107
- **Status**: Pending
- **Details**: JS registers three Vue watchers: `groupWatcher`, `setWatcher`, and `wireframeWatcher` that fire immediately on property mutation. C++ replaces this with a per-frame polling mechanism comparing `prev_group_checked`/`prev_set_checked` vectors in `render()`. This adds per-frame overhead and delays state change detection until the next render call. The wireframe watcher has no C++ analog at all.

### 490. [WMORendererGL.cpp] `dispose()` does not clear group/set data in the view — leaves stale state
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 668–669
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.groupArray.splice(0)` and `this.setArray.splice(0)` which remove all elements in-place from arrays shared by reference with the Vue view. C++ `dispose()` only calls `groupArray.clear()` / `setArray.clear()` on local member copies. The view's copies (`core::view->modelViewerWMOGroups` etc.) are NOT cleared, leaving stale data.

### 491. [WMORendererGL.cpp] Silently skips textures when `getTextureFile()` returns nullopt — JS would log error
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 178–200
- **Status**: Pending
- **Details**: If JS `texture.getTextureFile()` throws, it's caught and logged at line 200. In C++, if `getTextureFile()` returns `nullopt`, the code executes `continue` silently — no error is logged.

### 492. [WMORendererGL.cpp] `isClassic` check differs — JS truthiness vs C++ `empty()` for texture names
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` line 127
- **Status**: Pending
- **Details**: JS `!!wmo.textureNames` treats any non-null/undefined value (including empty array) as true. C++ `!wmo->textureNames.empty()` returns false for an empty container. If `textureNames` exists but is empty, JS enters classic mode while C++ does not.

### 493. [WMORendererGL.cpp] `updateAnimation()` calls `updateAnimation()` unconditionally — JS uses optional chaining
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 605–614
- **Status**: Pending
- **Details**: JS uses optional chaining `doodad.renderer.updateAnimation?.(delta_time)` which safely no-ops if the method doesn't exist. C++ calls `doodad.renderer->updateAnimation(delta_time)` unconditionally.

### 494. [WMORendererGL.cpp] View group/set arrays are assigned by value, not by reference — changes not reflected
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 97–98
- **Status**: Pending
- **Details**: JS assigns `view[this.wmoGroupKey] = this.groupArray` by reference — changes to the renderer's array are visible to the view and vice versa. C++ `get_wmo_groups_view() = groupArray` copies by value. The view and renderer have independent copies after this point. Subsequent modifications to either are not reflected in the other.

### 495. [WMOLegacyRendererGL.cpp] `_load_textures()` uses `set_blp()` instead of explicit RGBA/wrap/mipmap parameters
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 146–155
- **Status**: Pending
- **Details**: JS explicitly extracts RGBA pixels from BLP and calls `set_rgba()` with individually computed wrap modes (`material.flags & 0x40` for wrap_s, `material.flags & 0x80` for wrap_t), `has_alpha: blp.alphaDepth > 0`, and `generate_mipmaps: true`. C++ delegates to `gl_tex->set_blp(blp, blp_flags)` which may not produce identical wrap/alpha/mipmap behavior.

### 496. [WMOLegacyRendererGL.cpp] `_load_textures()` calls `setSlotFileLegacy` instead of `setSlotFile`
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` line 136
- **Status**: Pending
- **Details**: JS calls `textureRibbon.setSlotFile(ribbonSlot, textureName, this.syncID)`. C++ calls `texture_ribbon::setSlotFileLegacy(ribbonSlot, textureName, syncID)`. Different function name.

### 497. [WMOLegacyRendererGL.cpp] Reactive watchers replaced with polling — adds per-frame overhead
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 88–93
- **Status**: Pending
- **Details**: JS registers three Vue watchers that fire immediately on mutation. C++ replaces this with a polling mechanism in `render()` that compares `prev_group_checked`/`prev_set_checked` every frame. The wireframe watcher has no C++ analog at all (no `prev_wireframe` tracking).

### 498. [WMOLegacyRendererGL.h] `WMOLegacyDrawCall::count` is `uint16_t` — may truncate large batch face counts
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` line 228
- **Status**: Pending
- **Details**: `count` field is declared as `uint16_t` (header line 35). JS `batch.numFaces` is an unrestricted JS number. Values above 65535 silently truncate.

### 499. [WMOLegacyRendererGL.cpp] `updateGroups()` and `updateSets()` read from global view state instead of local arrays
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 345–351
- **Status**: Pending
- **Details**: JS `updateGroups()` reads from `this.groupArray` (the local member array). C++ `updateGroups()` (lines 418–420) reads from `core::view->modelViewerWMOGroups` (global view state). Same for `updateSets()`. If the local arrays and view copies diverge, C++ behavior differs from JS.

### 500. [WMOLegacyRendererGL.cpp] `dispose()` watcher cleanup replaced with clearing polling vectors
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 518–521
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.groupWatcher?.()`, `this.setWatcher?.()`, and `this.wireframeWatcher?.()` to unregister reactive watchers. C++ `dispose()` clears `prev_group_checked` and `prev_set_checked` instead. There is no wireframe tracking/cleanup at all.

### 501. [WMOLegacyRendererGL.cpp] `updateAnimation()` calls `updateAnimation()` unconditionally — JS uses optional chaining
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 492–499
- **Status**: Pending
- **Details**: JS uses `doodad.renderer.updateAnimation?.(delta_time)` which no-ops if `updateAnimation` is not defined. C++ calls `doodad.renderer->updateAnimation(delta_time)` unconditionally.

### 502. [WMOLegacyRendererGL.cpp] `_load_textures()` silently skips null mpq instead of throwing
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` line 139
- **Status**: Pending
- **Details**: JS calls `mpq.getFile(textureName)` without null checking — if `mpq` is null, it throws and is caught by the surrounding catch. C++ (lines 136, 167–168) adds `if (!mpq) continue;`, silently skipping the texture instead of logging an error.

### 503. [WMOLegacyRendererGL.cpp] `loadDoodadSet()` adds out-of-bounds guard not present in JS
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 287–291
- **Status**: Pending
- **Details**: C++ `loadDoodadSet()` (line 347) adds `if (firstIndex + i >= wmo->doodads.size()) continue;` which doesn't exist in JS. In JS, out-of-bounds access returns `undefined` and the subsequent property access throws, caught by catch. C++ silently skips.

## src/js/3D/writers Audit

### 504. [CSVWriter.cpp] `escapeCSVField()` treats empty string as null/undefined — JS does not
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 42–51
- **Status**: Pending
- **Details**: The JS `escapeCSVField()` checks `if (value === null || value === undefined)` and returns empty string. Then it calls `value.toString()`. The C++ version checks `if (value.empty())` and returns empty string. This means a value that is an empty string `""` in JS would be passed through `toString()` (returning `""`) and then returned as-is. In C++, an empty string returns `""` immediately. While functionally equivalent for strings, the JS version also handles non-string types (numbers, booleans) via `toString()`, whereas the C++ only accepts `const std::string&` — callers must convert to string before calling.

### 505. [CSVWriter.cpp] Row values are `std::string` only — JS supports arbitrary types per field
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 33–35, 74–79
- **Status**: Pending
- **Details**: The JS `addRow()` accepts an `object` whose field values can be any type (number, boolean, null, etc.) — `escapeCSVField` handles all types via `value.toString()`. The C++ `addRow()` only accepts `std::unordered_map<std::string, std::string>`, requiring all values to be pre-converted to strings by the caller. This shifts the type conversion responsibility to the caller and means null/undefined handling (JS returns `''` for these) must be handled externally.

### 506. [GLBWriter.h] Constants declared as `inline constexpr` in header — JS uses module-level `const`
- **JS Source**: `src/js/3D/writers/GLBWriter.js` lines 8–14
- **Status**: Pending
- **Details**: JS declares `GLB_MAGIC`, `GLB_VERSION`, `CHUNK_TYPE_JSON`, `CHUNK_TYPE_BIN` as module-scoped constants (not exported). The C++ declares these as `inline constexpr` in the header file, making them visible to all translation units that include `GLBWriter.h`. This leaks implementation-detail constants into the global namespace. They should be either `static constexpr` in the `.cpp` file or in an anonymous namespace. While not a functional bug, it deviates from the JS's module-local scoping.

### 507. [GLTFWriter.cpp] Generator string sources differ — C++ reads runtime config instead of build manifest
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 208–213
- **Status**: Pending
- **Details**: JS constructs the generator string as `util.format('wow.export v%s %s [%s]', manifest.version, manifest.flavour, manifest.guid)` reading from `nw.App.manifest` (build-time constants). C++ reads `constants::VERSION` for version but reads `selectedFlavour` and `selectedGuid` from `core::view->config` (runtime config). The C++ also conditionally omits flavour/guid when empty, while JS always formats all three values unconditionally — JS would produce `"undefined"` for absent values.

### 508. [GLTFWriter.cpp] Animation channel target `node` index differs when bone prefix mode is disabled
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 620–628, 757–765, 887–895
- **Status**: Pending
- **Details**: JS always uses `nodeIndex + 1` for translation, rotation, and scale animation channel target nodes. C++ uses `actual_node_idx` which equals `nodeIndex + 1` only when `modelsExportWithBonePrefix` is true, and equals `nodeIndex` when false. This changes animation behavior when bone prefix mode is disabled. The JS appears to have a bug here (the `+1` is correct only with prefix nodes), and the C++ fixes it without documenting the intentional deviation.

### 509. [GLTFWriter.cpp] `addTextureBuffer()` method added — does not exist in JS source
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` (entire class)
- **Status**: Pending
- **Details**: The C++ adds `addTextureBuffer(uint32_t fileDataID, BufferWrapper buffer)` (header line 98–99, cpp lines 108–110). The JS class only has `setTextureBuffers(texture_buffers)` to set the entire texture buffer map at once. The extra method is a C++ API addition not present in the original.

### 510. [GLTFWriter.cpp] `calculate_min_max` returns early on empty values — JS does not guard
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 34–51
- **Status**: Pending
- **Details**: C++ `calculate_min_max` has `if (values.empty()) return;` which skips setting min/max on the accessor. JS would create `target.min` and `target.max` as arrays filled with `undefined` (serializing as `null`). This means empty arrays produce no min/max keys in C++ output vs. `null`-filled arrays in JS output.

### 511. [GLTFWriter.cpp] JSON output key ordering differs — nlohmann uses alphabetical, JS preserves insertion order
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1484–1494
- **Status**: Pending
- **Details**: JS uses `JSON.stringify(root, null, '\t')` which preserves property insertion order. C++ uses `nlohmann::json` which by default sorts keys alphabetically (using `std::map` internally). This produces semantically equivalent but textually different GLTF/GLB JSON output. To match JS insertion order, `nlohmann::ordered_json` would be needed.

### 512. [GLTFWriter.cpp] GLTF text file written without error checking and may have different line endings
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` line 1493
- **Status**: Pending
- **Details**: JS uses `await fsp.writeFile(outGLTF, ..., 'utf8')` which rejects on error. C++ uses `std::ofstream` without checking open/write success. Additionally, on Windows `std::ofstream` defaults to text mode (`\n` → `\r\n` translation), producing different line endings than the JS version.

### 513. [JSONWriter.cpp] `dump()` indentation uses 1-space width with tab character — may differ from JS format
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 40–43
- **Status**: Pending
- **Details**: JS uses `JSON.stringify(this.data, ..., '\t')` which uses tab-based indentation. C++ uses `data.dump(1, '\t')` where the first argument `1` is the indent width (number of tab characters per level). Both produce tab indentation, but `nlohmann::json` may differ in key ordering (alphabetical vs. insertion-order). The JS also has a custom replacer function for BigInt values that converts them to strings; the C++ comment says nlohmann handles large integers natively, but C++ `int64_t` has a fixed range while JS BigInt is arbitrary-precision. If any property value exceeds int64 range, the C++ serialization would differ.

### 514. [JSONWriter.cpp] BigInt serialization handled differently — C++ lacks arbitrary-precision support
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 40–43
- **Status**: Pending
- **Details**: The JS `write()` uses a custom JSON replacer `(key, value) => typeof value === 'bigint' ? value.toString() : value` to serialize BigInt values as strings. The C++ comment says "nlohmann::json handles all types natively including large integers; no special BigInt serialization needed as in JS." However, nlohmann::json stores integers as `int64_t` or `uint64_t` (max ~18.4 quintillion), while JS BigInt can represent arbitrarily large integers. Values exceeding the 64-bit range would overflow in C++ but be correctly serialized as strings in JS.

### 515. [MTLWriter.cpp] `path.resolve()` vs `std::filesystem::weakly_canonical()` may differ for relative paths
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 60–63
- **Status**: Pending
- **Details**: JS uses `path.resolve(mtlDir, materialFile)` which resolves against `mtlDir` to produce an absolute path. C++ uses `std::filesystem::weakly_canonical(mtlDir / materialFile)` which additionally canonicalizes the path (resolves `.` and `..` segments, normalizes separators). These may produce different results for paths containing `..` segments, symlinks, or non-existent intermediate directories. `weakly_canonical` can also throw on some platforms.

### 516. [OBJWriter.cpp] Version header says "wow.export" — should say "wow.export.cpp"
- **JS Source**: `src/js/3D/writers/OBJWriter.js` line 138
- **Status**: Pending
- **Details**: C++ line 75 writes `"# Exported using wow.export v" + std::string(constants::VERSION)`. Per project conventions, user-facing text should say "wow.export.cpp" not "wow.export". The JS original says "wow.export" but the C++ port should use the project's own name.

### 517. [OBJWriter.cpp] `std::to_string()` float formatting differs from JS `toString()` — trailing zeros, precision
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 161, 170, 194
- **Status**: Pending
- **Details**: JS converts floats with default `toString()` which produces minimal representation (e.g., `1.5`, `0.123456789`). C++ uses `std::to_string()` which formats with 6 decimal places and trailing zeros (e.g., `1.500000`, `0.123457`). This produces larger output files and may introduce precision differences due to rounding. For example, JS `(0.1).toString()` → `"0.1"` but C++ `std::to_string(0.1)` → `"0.100000"`. This affects vertex, normal, and UV coordinate output.

### 518. [SQLWriter.cpp] `escapeSQLValue()` treats empty string as NULL — JS treats it differently
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 65–77
- **Status**: Pending
- **Details**: JS `escapeSQLValue()` checks `if (value === null || value === undefined)` and returns `'NULL'`. An empty string `""` would pass through to `toString()` (returning `""`), fail the `isNaN` check (since `isNaN("") === false` in JS, empty string coerces to 0), and be returned as-is (bare `""`). The C++ checks `if (value.empty())` and returns `"NULL"`. This means an empty string is treated as NULL in C++ but as a numeric value `0` (or empty string) in JS — a significant behavioral difference for SQL output.

### 519. [SQLWriter.cpp] `escapeSQLValue()` numeric detection logic differs from JS `isNaN()` behavior
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 72–73
- **Status**: Pending
- **Details**: JS uses `!isNaN(value) && str.trim() !== ''` for numeric detection. JS `isNaN()` coerces its argument: `isNaN("123") === false` (numeric), `isNaN("12.3e5") === false` (numeric), `isNaN("0x1F") === false` (hex is numeric). The C++ implementation manually checks for digits, decimal point, and sign characters but does not handle scientific notation (`1e5`), hexadecimal (`0xFF`), or other JS-coercible numeric formats. Values like `"1e5"` or `"0xFF"` would be treated as numeric in JS but as strings in C++.

### 520. [SQLWriter.cpp] `generateDDL()` skips fields not found in schema — JS uses undefined
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 154–166
- **Status**: Pending
- **Details**: JS does `const field_type = this.schema.get(field)` — if the field is not in the schema, `field_type` is `undefined`. Then `fieldTypeToSQL(undefined, field)` is called, which falls through all switch cases to the `default: return 'TEXT'` branch. So missing-schema fields get type `TEXT`. In C++, `if (it == schema->end()) continue;` skips the field entirely — it won't appear in the CREATE TABLE statement. This means C++ DDL output may have fewer columns than JS DDL output for the same data.

### 521. [SQLWriter.cpp] `generateDDL()` column_defs joined with different formatting than JS
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 171–173
- **Status**: Pending
- **Details**: JS uses `column_defs.join(',\n')` which puts commas at the end of each line except the last. C++ (lines 175–179) manually appends commas: each line gets a trailing comma unless it's the last entry (`if (i + 1 < column_defs.size()) result += ","`). In JS, the comma is on the same line as the column definition. In C++, the comma is also on the same line. However, the newline placement may differ slightly: JS produces `col1 INT,\ncol2 TEXT` while C++ produces `col1 INT,\ncol2 TEXT` — these should be equivalent. No functional difference but worth verifying during testing.

### 522. [SQLWriter.cpp] `toSQL()` value row formatting — comma placement and newlines differ slightly
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 191–203
- **Status**: Pending
- **Details**: JS batch format: `INSERT INTO ... VALUES\n(row1),\n(row2);` — commas appear after `)` on each row except the last in the batch, joined by `,\n`. The C++ (lines 206–222) produces `(row1),\n(row2)\n;\n\n` — the semicolon is on its own line followed by two newlines. The JS produces `(row2);\n\n` — the semicolon immediately follows the last row with a blank line after. This produces slightly different SQL formatting.

### 523. [STLWriter.cpp] Version header says "wow.export" — should say "wow.export.cpp"
- **JS Source**: `src/js/3D/writers/STLWriter.js` line 147
- **Status**: Pending
- **Details**: C++ line 93 writes `"Exported using wow.export v" + std::string(constants::VERSION)` into the STL header. Per project conventions, user-facing text should say "wow.export.cpp" not "wow.export". The JS original says "wow.export" but the C++ port should use its own project name.

### 524. [GLTFWriter.cpp] Generator string says "wow.export" — should say "wow.export.cpp"
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 208–213
- **Status**: Pending
- **Details**: The GLTF generator metadata string in C++ starts with `"wow.export v"`. Per project conventions, user-facing text should say "wow.export.cpp" not "wow.export".

## UI Visual Fidelity Audit

### 525. [app.h] Listbox ROW_SELECTED_U32 color is green (#22b549) instead of blue (#57afe2)
- **JS Source**: `src/app.css` `.ui-listbox .item.selected` / `.item:hover` (line ~1409)
- **Status**: Pending
- **Details**: The CSS specifies that selected listbox items use `background: var(--font-alt)` which is `#57afe2` (blue). The C++ `ROW_SELECTED_U32` at `app.h:109` is `IM_COL32(34, 181, 73, 40)` — a semi-transparent green derived from `#22b549` (the button/nav color). This affects every listbox throughout the application. The selected row should be blue `IM_COL32(87, 175, 226, 255)` to match the CSS, not green. Similarly, the hover state should also be `#57afe2`.

### 526. [app.h] Listbox ROW_HOVER_U32 color does not match CSS hover specification
- **JS Source**: `src/app.css` `.ui-listbox .item:hover` (line ~1411)
- **Status**: Pending
- **Details**: The CSS specifies listbox hover as `background: var(--font-alt) !important` which is `#57afe2`. The C++ `ROW_HOVER_U32` at `app.h:107` is `IM_COL32(255, 255, 255, 8)` — a nearly invisible white tint. Should be a visible blue hover highlight matching `#57afe2` (possibly with reduced opacity for hover vs. selected).

### 527. [app.h] Slider track color does not match CSS specification
- **JS Source**: `src/app.css` `.ui-slider` (line ~1307)
- **Status**: Pending
- **Details**: The CSS specifies the slider track background as `var(--background-dark)` = `#2c3136` with `border: 1px solid var(--border)`. The C++ `SLIDER_TRACK_U32` at `app.h:117` is `IM_COL32(80, 80, 80, 255)` = `#505050`, which is significantly lighter than the CSS spec `#2c3136` (44, 49, 54). Should be `IM_COL32(44, 49, 54, 255)`.

### 528. [app.h] Slider fill color uses green instead of blue
- **JS Source**: `src/app.css` `.ui-slider .fill` (line ~1315)
- **Status**: Pending
- **Details**: The CSS specifies the slider fill as `background: var(--font-alt)` = `#57afe2` (blue). The C++ slider at `slider.cpp:122` uses `app::theme::BUTTON_BASE_U32` = `#22b549` (green). The fill color should use `FONT_ALT_U32` = `#57afe2` instead of `BUTTON_BASE_U32`.

### 529. [app.h] Slider handle idle color does not match CSS
- **JS Source**: `src/app.css` `.ui-slider .handle` (line ~1319)
- **Status**: Pending
- **Details**: The CSS specifies the slider handle as `background: var(--border)` = `#6c757d`. The C++ `SLIDER_THUMB_U32` at `app.h:119` is `IM_COL32(200, 200, 200, 200)` = light gray with reduced opacity. Should be `IM_COL32(108, 117, 125, 255)` to match `#6c757d`.

### 530. [app.h] Slider handle hover color does not match CSS
- **JS Source**: `src/app.css` `.ui-slider .handle:hover` (line ~1329)
- **Status**: Pending
- **Details**: The CSS specifies the slider handle hover as `background: var(--font-alt)` = `#57afe2` (blue). The C++ `SLIDER_THUMB_ACTIVE_U32` at `app.h:120` is `IM_COL32(255, 255, 255, 220)` = white. Should be `IM_COL32(87, 175, 226, 255)` to match `#57afe2`.

### 531. [app.h] Data table selected row color is gray instead of blue
- **JS Source**: `src/app.css` `.ui-datatable tr.selected` (line ~1387)
- **Status**: Pending
- **Details**: The CSS specifies `background: var(--font-alt) !important` = `#57afe2` for selected data table rows. The C++ `TABLE_ROW_SELECTED_U32` at `app.h:127` is `IM_COL32(100, 100, 100, 100)` — a semi-transparent gray. Should be `IM_COL32(87, 175, 226, 255)` to match `#57afe2`.

### 532. [app.h] Data table hover row color is gray instead of blue
- **JS Source**: `src/app.css` `.ui-datatable tbody tr:hover` (line ~1389)
- **Status**: Pending
- **Details**: The CSS specifies `background: var(--font-alt)` = `#57afe2` for hovered data table rows. The C++ `TABLE_ROW_HOVER_U32` at `app.h:126` is `IM_COL32(100, 100, 100, 255)` — a solid gray. Should be `IM_COL32(87, 175, 226, 255)` to match `#57afe2`.

### 533. [listbox.cpp] Status bar missing background color and border-radius styling
- **JS Source**: `src/app.css` `.list-container .list-status` (line ~2459–2470)
- **Status**: Pending
- **Details**: The CSS specifies the listbox status bar has `background: #1f1f20`, `height: 27px`, `border-radius` bottom 10px, `font-weight: bold`, `padding-left: 10px`, `padding-top: 3px`. The C++ status bar rendering at `listbox.cpp:775–812` uses plain `ImGui::Text()` calls with no background rectangle, no border-radius, no specific padding, and no bold weight. The status bar should be rendered as a styled bar below the listbox with the specified background and rounded bottom corners.

### 534. [slider.cpp] Slider handle dimensions do not match CSS specification
- **JS Source**: `src/app.css` `.ui-slider .handle` (line ~1319–1328)
- **Status**: Pending
- **Details**: The CSS specifies the slider handle as `width: 10px`, `height: 28px`, centered vertically with `transform: translateY(-50%)`, and `box-shadow: black 0 0 8px`. The C++ slider implementation should be verified to match these exact dimensions. The CSS also specifies `z-index: 1` for the handle above the fill.

### 535. [context-menu.cpp] Missing background color, border, and shadow styling
- **JS Source**: `src/app.css` `.context-menu` (line ~1259–1287)
- **Status**: Pending
- **Details**: The CSS specifies the context menu has `background: #232323`, `border: 1px solid var(--border)` (#6c757d), `box-shadow: black 0 0 3px 0`. The C++ context-menu component does not apply these styles explicitly — it relies on ImGui's default popup styling. Items should have `padding: 8px`, `border-bottom: 1px solid var(--border)`, and hover background `#353535`. These CSS-specific styles are not replicated in the ImGui popup.

### 536. [combobox.cpp] Dropdown popup missing specific CSS styling
- **JS Source**: `src/app.css` `.ui-combobox ul` (line ~1294–1306)
- **Status**: Pending
- **Details**: The CSS specifies the combobox dropdown has `background: #232323`, `border: 1px solid var(--border)`, `box-shadow: black 0 0 3px 0`. Options have `border-bottom: 1px solid var(--border)`, `padding: 10px 15px`, and hover `background: #353535`. The C++ implementation relies on ImGui's default Selectable/ListBox styling, not these specific CSS colors and spacing.

### 537. [tab_fonts.cpp] Glyph cell size is 24×24px instead of CSS-specified 32×32px
- **JS Source**: `src/app.css` `.font-glyph-cell` (line ~2303–2314)
- **Status**: Pending
- **Details**: The CSS specifies `.font-glyph-cell` as `width: 32px; height: 32px; font-size: 20px; background: var(--background-alt); border-radius: 3px`. The C++ implementation at `tab_fonts.cpp:231` uses `ImVec2(24, 24)` for glyph selectables — 8px smaller than the CSS spec. Should be `ImVec2(32, 32)`.

### 538. [tab_fonts.cpp] Glyph cells missing background color and hover color
- **JS Source**: `src/app.css` `.font-glyph-cell` and `.font-glyph-cell:hover` (lines ~2303–2317)
- **Status**: Pending
- **Details**: The CSS specifies glyph cells have `background: var(--background-alt)` = `#3c4147` and hover `background: var(--font-alt)` = `#57afe2`. The C++ uses ImGui's default Selectable styling, which does not apply these specific background colors. Each cell should have the `#3c4147` background and `#57afe2` hover.

### 539. [tab_fonts.cpp] Font preview input area missing CSS styling
- **JS Source**: `src/app.css` `.font-preview-input` (lines ~2329–2352)
- **Status**: Pending
- **Details**: The CSS specifies the font preview input as `height: 120px`, `font-size: 32px`, `background: var(--background-dark)` = `#2c3136`, `border: 1px solid var(--border)`, and placeholder color `#888`. The C++ `InputTextMultiline` at `tab_fonts.cpp:256–258` uses the remaining available height (not fixed 120px), default ImGui font size (not 32px), and default styling. The preview input needs explicit height constraint and font size override.

### 540. [tab_fonts.cpp] Character grid does not fill correct vertical proportion
- **JS Source**: `src/app.css` `.font-character-grid` (lines ~2286–2301)
- **Status**: Pending
- **Details**: The CSS specifies the character grid as `position: absolute; top: 0; bottom: 140px` — meaning it fills all space except the bottom 140px reserved for the preview input container. The C++ uses `ImGui::GetContentRegionAvail().y * 0.5f` (50% of height) at `tab_fonts.cpp:218`, which does not match the CSS layout. The grid should fill the available space minus 140px (120px input + 20px container spacing).

### 541. [tab_text.cpp] Text preview uses TextWrapped instead of monospace pre with scroll
- **JS Source**: `src/app.css` `#tab-text .preview-background pre` (lines ~2252–2259)
- **Status**: Pending
- **Details**: The CSS specifies the text preview uses a `<pre>` element with `overflow: scroll`, `padding: 15px`, `position: absolute` (full container), and `user-select: text`. The C++ at `tab_text.cpp:174` uses `ImGui::TextWrapped()` which wraps text (pre does not wrap by default), has no explicit 15px padding, and no scrollable region. Should use a scrollable child window with monospace/non-wrapping text and proper padding.

### 542. [screen_settings.cpp] Settings content bounded at 800px instead of full-screen
- **JS Source**: `src/app.css` `#config-wrapper` / `#config` (lines ~1221–1245)
- **Status**: Pending
- **Details**: The CSS specifies `#config-wrapper` as `position: absolute` (full screen) and `#config` as `flex: 1` filling the available width. The C++ at `screen_settings.cpp:160` caps the content width to `800.0f` pixels, which means the settings page doesn't fill the full window width as the original JS app does. The settings content should span the full available width.

### 543. [screen_settings.cpp] Button bar layout is left-to-right instead of CSS row-reverse
- **JS Source**: `src/app.css` `#config-buttons` (lines ~1237–1245)
- **Status**: Pending
- **Details**: The CSS specifies `#config-buttons` as `display: flex; flex-direction: row-reverse; padding: 15px 0; border-top: 1px solid var(--border); background: var(--background)`. The C++ at `screen_settings.cpp:483–493` renders buttons left-to-right using `ImGui::Button()` + `ImGui::SameLine()`, but the CSS uses `row-reverse` so "Apply" appears on the right and "Reset to Defaults" on the far left. Also missing: explicit 15px vertical padding, border-top, background color, and the Reset button's `margin-right: auto; margin-left: 20px` alignment.

### 544. [screen_settings.cpp] Section headings use SeparatorText instead of styled h1 at 18px
- **JS Source**: `src/app.css` `#config > div h1` (line ~1232)
- **Status**: Pending
- **Details**: The CSS specifies section headings as `<h1>` with `font-size: 18px`. The C++ uses `ImGui::SeparatorText()` calls which render text with separator lines on either side — visually different from the original's plain bold heading. The headings should be rendered as 18px bold text without separator lines to match the original.

### 545. [screen_settings.cpp] Missing 20px section padding between setting groups
- **JS Source**: `src/app.css` `#config > div` (line ~1230)
- **Status**: Pending
- **Details**: The CSS specifies each settings section (`#config > div`) has `padding: 20px` (with bottom 0). The C++ settings implementation does not add explicit 20px padding between sections — ImGui's default spacing is used instead, which is typically smaller. Each group of settings should have 20px padding around it.

### 546. [app.cpp] Crash screen missing exclamation-triangle icon before heading
- **JS Source**: `src/app.css` `#crash-screen h1` (lines ~1212–1215), `src/index.html` noscript block
- **Status**: Pending
- **Details**: The CSS specifies the crash screen h1 has `background: url(./fa-icons/triangle-exclamation-white.svg) no-repeat left center; padding-left: 50px` — showing a warning triangle icon to the left of the heading text. The C++ `renderCrashScreen()` at `app.cpp:258–288` renders the heading as plain text without any icon. A Font Awesome warning triangle icon should precede the heading.

### 547. [tab_maps.cpp] Map viewer missing box-shadow styling
- **JS Source**: `src/app.css` `.ui-map-viewer` (lines ~1333–1345)
- **Status**: Pending
- **Details**: The CSS specifies the map viewer has `box-shadow: black 0 0 3px 0` and `border: 1px solid var(--border)`. The C++ map viewer component does not render box-shadow effects. While ImGui doesn't natively support box-shadow, a dark border or shadow rectangle could simulate this effect.

### 548. [tab_maps.cpp] Map viewer checkerboard pattern size not verified
- **JS Source**: `src/app.css` `.ui-map-viewer` background (lines ~1334–1338)
- **Status**: Pending
- **Details**: The CSS specifies the map viewer checkerboard pattern as `background-size: 30px 30px` using colors `--trans-check-a: #303030` and `--trans-check-b: #272727`. While the C++ uses the correct colors (`TRANS_CHECK_A_U32` and `TRANS_CHECK_B_U32`), the checkerboard tile size should be verified to be 30×30 pixels to match the CSS.

### 549. [tab_zones.cpp] Zone viewer missing border and box-shadow styling
- **JS Source**: `src/app.css` `.ui-map-viewer` (lines ~1333–1345)
- **Status**: Pending
- **Details**: Same as the map viewer — the zone viewer canvas area should have `border: 1px solid var(--border)` and `box-shadow: black 0 0 3px 0`. The C++ implementation is missing these border/shadow effects.

### 550. [tab_help.cpp] Not ported to C++ — still JavaScript
- **JS Source**: `src/js/modules/tab_help.js` (entire file)
- **Status**: Pending
- **Details**: The help tab (`tab_help.cpp`) is still a Node.js/JavaScript file, not converted to C++. The CSS specifies a `#help-screen` grid layout with `grid-template-columns: 1fr 1fr`, `gap: 20px`, article items with `padding: 15px 20px`, `background: var(--background-dark)`, `border-radius: 8px`, `border: 1px solid white`, title at `18px`, and tags at `13px` with `opacity: 0.7`. The entire tab needs conversion to C++ with ImGui.

### 551. [tab_blender.cpp] Not ported to C++ — still JavaScript
- **JS Source**: `src/js/modules/tab_blender.js` (entire file)
- **Status**: Pending
- **Details**: The Blender addon tab (`tab_blender.cpp`) is still a Node.js/JavaScript file. It provides UI for checking/installing/updating the Blender addon (3 buttons + status messages). Needs full C++ conversion.

### 552. [tab_changelog.cpp] Not ported to C++ — still JavaScript
- **JS Source**: `src/js/modules/tab_changelog.js` (entire file)
- **Status**: Pending
- **Details**: The changelog tab (`tab_changelog.cpp`) is still a Node.js/JavaScript file. It reads and displays a Markdown changelog file using the `markdown-content` component. Needs full C++ conversion.

### 553. [checkboxlist.cpp] Selected item color is green instead of CSS blue
- **JS Source**: `src/app.css` `.ui-checkboxlist .item.selected` (same styling as `.ui-listbox .item.selected`)
- **Status**: Pending
- **Details**: The checkboxlist at `checkboxlist.cpp:224` uses `app::theme::ROW_SELECTED_U32` which is `IM_COL32(34, 181, 73, 40)` — green. The CSS `.ui-checkboxlist` shares the same styling as `.ui-listbox` where selected items use `background: var(--font-alt)` = `#57afe2` (blue). Fix is the same as #525 — once `ROW_SELECTED_U32` is corrected, checkboxlist will also be fixed.

### 554. [markdown-content.cpp] Missing CSS background, border-radius, and heading font sizes
- **JS Source**: `src/app.css` `.markdown-content` (lines ~458–530)
- **Status**: Pending
- **Details**: The CSS specifies `.markdown-content` has `background: rgb(0 0 0 / 22%)`, `border-radius: 10px`, `padding: 20px`, `font-size: 20px`. Headings: h1 `1.8em`, h2 `1.5em`, h3 `1.2em` (all bold). Code blocks: `background: rgba(0,0,0,0.3)`, `padding: 2px 6px`, `border-radius: 3px`. The C++ markdown-content component should apply these specific styles when rendering markdown elements.

### 555. [tab_videos.cpp] Video player area rendering not implemented
- **JS Source**: `src/js/modules/tab_videos.js` (entire render function)
- **Status**: Pending
- **Details**: The videos tab should display a video list and a streaming video player area. The C++ implementation has HTTP streaming infrastructure but the actual video player display area (canvas/viewport for video frames) may not be fully rendered in the UI. The video playback area should match the reference screenshot layout.

### 556. [app.h] Missing FONT_DISABLED color constant referenced by character tab
- **JS Source**: `src/app.css` `.slot-empty` `color: var(--font-disabled)` (line ~2938)
- **Status**: Pending
- **Details**: The CSS references a `--font-disabled` variable for empty equipment slot text (italic, grayed out). No corresponding `FONT_DISABLED` constant exists in `app.h`. This color should be defined (likely a muted gray similar to `--font-faded` or darker) and used for disabled/empty state text.

### 557. [itemlistbox.cpp] Item icon border color should be #8a8a8a
- **JS Source**: `src/app.css` `.item-icon` `border: 1px solid #8a8a8a` (line ~1537)
- **Status**: Pending
- **Details**: The CSS specifies item icons have `border: 1px solid #8a8a8a` (138, 138, 138). The C++ itemlistbox renders item icons but should verify the border color matches this specific gray value rather than using a different theme color.

### 558. [itemlistbox.cpp] Item row height should be 46px with 1.2em font size
- **JS Source**: `src/app.css` `#tab-items #listbox-items .item` (lines ~1520–1525)
- **Status**: Pending
- **Details**: The CSS specifies items tab listbox items have `height: 46px`, `font-size: 1.2em`, and `display: flex; align-items: center`. This is taller than the standard 26px listbox item height. The C++ itemlistbox should use 46px row height with the larger font size for the items tab, matching the reference screenshot which shows larger item entries with icons.

### 559. [data-table.cpp] Table header padding should be 10px
- **JS Source**: `src/app.css` `.ui-datatable th` (lines ~1374–1377)
- **Status**: Pending
- **Details**: The CSS specifies data table headers have `border: 1px solid var(--border)` and `padding: 10px`. The C++ data-table should verify that header cells have proper 10px padding and visible border styling matching the CSS specification.

### 560. [data-table.cpp] Table row height should be 32px
- **JS Source**: `src/app.css` `.ui-datatable tr` (lines ~1366–1370)
- **Status**: Pending
- **Details**: The CSS specifies data table rows have `min-height: 32px; max-height: 32px; height: 32px` — a fixed 32px row height. The C++ implementation should ensure rows are exactly 32px tall to match the original layout.

### 561. [tab_characters.cpp] Character tab missing tab-control styling
- **JS Source**: `src/app.css` `.tab-control` and `.tab-control span` (lines ~2645–2654)
- **Status**: Pending
- **Details**: The CSS specifies the character tab's panel selector (Export/Textures/Settings) uses `.tab-control` with `display: flex`, spans at `font-size: 20px`, `padding: 5px`, background `var(--form-button-disabled)` (#696969 gray) for unselected and `var(--form-button-hover)` (#2665d2 blue) for selected, with `border-radius: 10px` on first/last children. The C++ implementation should verify these tab control styles match the reference screenshot appearance.

### 562. [tab_characters.cpp] Character equipment slot styling needs CSS match
- **JS Source**: `src/app.css` `.equipment-slot` (lines ~2913–2927)
- **Status**: Pending
- **Details**: The CSS specifies equipment slots as `display: flex; justify-content: space-between; padding: 6px 12px; background: var(--background-dark); border: 1px solid var(--border); border-radius: 8px; font-size: 13px`. Hover: `background: var(--background-alt)`. Labels use `color: var(--font-alt)` (#57afe2). The C++ implementation should match these exact sizes, colors, and border-radius values.

### 563. [tab_characters.cpp] Character import buttons at bottom missing specific CSS styling
- **JS Source**: `src/app.css` `.character-import-buttons` (lines ~2985–3065)
- **Status**: Pending
- **Details**: The CSS specifies the bottom character buttons (Battle.net, WMV, Wowhead, Save, Quick Save, Import JSON, Export JSON) with specific colors — BNet: `#148eff`, WMV: `#d22c1e`, Wowhead: `#e02020`, Save: `#5865f2`, Quick Save: `#22b549`, Import JSON: `#e67e22`, Export JSON: `#22b549`. Each has icon images as backgrounds. The C++ should verify these specific brand colors and icon backgrounds are applied.
