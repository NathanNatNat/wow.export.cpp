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
