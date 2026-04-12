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
