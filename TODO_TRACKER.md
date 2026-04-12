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
