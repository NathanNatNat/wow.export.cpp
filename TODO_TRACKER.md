# TODO Tracker

## app.cpp / app.h Audit

### 1. [app.cpp] Crash screen missing original UI elements and buttons
- **JS Source**: `src/app.js` lines 24–61, `src/index.html` `<noscript>` block
- **Status**: Pending
- **Details**: The original JS crash screen displays "Oh no! The kākāpō has exploded..." as the heading, shows version/flavour/build-guid fields, and provides four action buttons: "Report Issue" (opens issue tracker), "Get Help on Discord" (opens Discord), "Copy Log to Clipboard" (copies log textarea to clipboard), and "Restart Application" (calls `chrome.runtime.reload()`). The C++ `renderCrashScreen()` (lines 258–288) instead shows "wow.export.cpp has crashed!" as a heading (acceptable per naming convention), shows only the version, and lacks all four action buttons. The runtime log is rendered as read-only ImGui text rather than a selectable/copyable textarea. The flavour and build/guid fields are also missing from the crash screen display.

### 2. [app.cpp] Missing global crash handlers (unhandledRejection / uncaughtException equivalents)
- **JS Source**: `src/app.js` lines 72–73
- **Status**: Pending
- **Details**: The JS registers `process.on('unhandledRejection', ...)` and `process.on('uncaughtException', ...)` to catch unhandled errors and invoke `crash()`. The C++ has no equivalent global exception handler such as `std::set_terminate()`, signal handlers, or structured exception handling (on Windows) to catch unhandled exceptions that escape the main loop's try/catch.

### 3. [app.cpp] Missing `goToTexture(fileDataID)` method
- **JS Source**: `src/app.js` lines 418–434
- **Status**: Pending
- **Details**: The JS defines a `goToTexture(fileDataID)` method that: (1) switches to the textures tab via `modules.tab_textures.setActive()`, (2) directly previews a texture by file data ID via `modules.tab_textures.previewTextureByID()`, (3) clears the current selection via `view.selectionTextures.splice(0)`, and (4) sets the user input filter to `[fileDataID]` (with regex escaping if `config.regexFilters` is enabled). This method has no equivalent in the C++ port and is completely missing from `app.cpp`.

### 4. [app.cpp] `click()` method does not check for disabled state
- **JS Source**: `src/app.js` lines 369–372
- **Status**: Pending
- **Details**: The JS `click(tag, event, ...params)` method checks `if (!event.target.classList.contains('disabled'))` before emitting the event. The C++ `click(tag)` (line 1425–1427) emits the event unconditionally without any disabled check. It also doesn't accept variadic parameters — the JS version passes `...params` through to the event emitter.

### 5. [app.cpp] `emit()` method does not pass variadic parameters
- **JS Source**: `src/app.js` lines 379–381
- **Status**: Pending
- **Details**: The JS `emit(tag, ...params)` passes all additional parameters through to `core.events.emit(tag, ...params)`. The C++ `emit(tag)` (line 1434–1436) only passes the tag string, dropping any additional parameters.

### 6. [app.cpp] `setAllItemTypes()` and `setAllItemQualities()` are stubbed out
- **JS Source**: `src/app.js` lines 291–303
- **Status**: Pending
- **Details**: The JS `setAllItemTypes(state)` iterates `this.itemViewerTypeMask` and sets `entry.checked = state` for each entry. Similarly, `setAllItemQualities(state)` iterates `this.itemViewerQualityMask`. The C++ versions (lines 1354–1364) have empty function bodies with the `state` parameter explicitly unused. These need to iterate over the corresponding arrays in `core::view` and toggle the `checked` property.

### 7. [app.cpp] `getExternalLink()` returns void instead of ExternalLinks module
- **JS Source**: `src/app.js` lines 457–459
- **Status**: Pending
- **Details**: The JS `getExternalLink()` returns a reference to the `ExternalLinks` module, which provides URL opening functionality used by the UI. The C++ `getExternalLink()` (lines 1493–1495) has an empty body that simply returns void. This function should return a reference to whatever C++ equivalent handles external link resolution (mapping `::WEBSITE`, `::DISCORD`, `::PATREON`, `::GITHUB`, `::ISSUE_TRACKER` constants to URLs).

### 8. [app.cpp] Drag-and-drop missing `ondragenter` / `ondragleave` handlers — prompt never shown before drop
- **JS Source**: `src/app.js` lines 589–660
- **Status**: Pending
- **Details**: The JS has three drag/drop handlers: `ondragenter` (shows the file drop prompt with file count before the drop occurs, using a `dropStack` counter), `ondragleave` (hides the prompt when the drag leaves the window, decrementing `dropStack`), and `ondrop` (processes the dropped files). The C++ `glfw_drop_callback` (lines 1111–1152) only handles the drop event itself. The `fileDropPrompt` overlay is never shown proactively because GLFW doesn't provide drag-enter/drag-leave callbacks. The drop prompt overlay code exists in `renderAppShell()` but can never be triggered. This is a significant UX deviation — users never see the file drop prompt before releasing files.

### 9. [app.cpp] Drop handler processes only the first file instead of all matching files
- **JS Source**: `src/app.js` lines 626–647
- **Status**: Pending
- **Details**: The JS `ondrop` handler collects ALL matching files into an `include` array and passes the entire array to `handler.process(include)`. The C++ `glfw_drop_callback` (line 1148) passes only `include[0]` (single file) to `handler->process()`. This breaks batch processing of multiple dropped files.

### 10. [app.cpp] Auto-updater logic is commented out
- **JS Source**: `src/app.js` lines 688–701
- **Status**: Pending
- **Details**: The JS checks for updates at startup: `if (BUILD_RELEASE && !DISABLE_AUTO_UPDATE)` it calls `updater.checkForUpdates()` and either applies the update or hides the loading screen and sets the active module. The C++ has this entire block commented out (lines 2352–2362). The `updater` module is not imported. This means the C++ port has no auto-update capability.

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
