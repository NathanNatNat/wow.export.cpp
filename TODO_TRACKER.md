# TODO Tracker

> **Progress: 67/906 verified (7%)** — ✅ = Verified, ⬜ = Pending

- [x] 1. [app.cpp] Auto-updater flow from app.js is not ported
- **JS Source**: `src/app.js` lines 691–704
- **Details**: Updater flow ported (disabled with comment). The full JS if/else structure is present in commented-out form using `std::thread` + `core::postToMainThread()`. The else branch (debug/disabled auto-update) runs unconditionally. `#include "js/updater.h"` added.

- [x] 2. [app.cpp] Drag enter/leave overlay behavior differs from JS
- **JS Source**: `src/app.js` lines 590–660
- **Details**: Documented as a GLFW platform limitation. GLFW only provides `glfwSetDropCallback` (fires on actual drop) — there are no drag-enter/drag-leave callbacks. Implementing this requires platform-specific APIs (Win32 COM IDropTarget, X11 XDnD protocol). The drop handler itself (`glfw_drop_callback`) is fully ported and functionally correct.

- [x] 3. [app.cpp] Crash screen heading text differs from original JS
- **JS Source**: `src/app.js` line 70 / `src/index.html` line 70
- **Status**: Verified
- **Details**: Changed heading from `"wow.export.cpp has crashed!"` to `"Oh no! The kākāpō has exploded..."` matching the original JS `<h1>` text.

- [x] 4. [app.cpp] Crash screen missing logo background element
- **JS Source**: `src/app.js` lines 37–39
- **Status**: Verified
- **Details**: Added `s_logoTexture` watermark rendering at 5% opacity (IM_COL32(255,255,255,13)) centered behind the crash screen content, matching CSS `#logo-background { opacity: 0.05; }`.

- [x] 5. [app.cpp] Crash screen version/flavour/build color differs from JS CSS
- **JS Source**: `src/app.js` lines 44–47 / `src/app.css` `#crash-screen-versions span`
- **Status**: Verified
- **Details**: Changed version/flavour/build text from default FONT_PRIMARY to `app::theme::BORDER` color (#6c757d), matching CSS `color: var(--border)`. Also fixed error code text: removed incorrect red color (ImVec4(1,0.3,0.3,1)) — JS CSS uses default inherited color with bold weight, not red.

- [x] 6. [app.cpp] Crash screen error text styling does not match JS CSS
- **JS Source**: `src/app.js` lines 49–51 / `src/app.css` `#crash-screen-text`
- **Status**: Verified
- **Details**: Fixed: error code now uses bold font (`getBoldFont()`), 5px `SameLine` spacing matches `margin-right: 5px`, and `Dummy(0, 20)` provides `margin: 20px 0`. Red color removed (was not in JS CSS).

- [x] 7. [app.cpp] ScaleAllSizes(1.5f) has no JS equivalent and alters all UI metrics
- **JS Source**: N/A (no equivalent in `src/app.js`)
- **Status**: Verified
- **Details**: Removed `ScaleAllSizes(1.5f)` call. JS has no global scale multiplier; sizes come from CSS. Dynamic scaling for small displays is handled separately (update_container_scale equivalent).

- [x] 8. [app.cpp] handleContextMenuClick accesses opt.handler instead of opt.action?.handler
- **JS Source**: `src/app.js` lines 318–322
- **Status**: Verified
- **Details**: C++ ContextMenuOption struct is flat (`opt.handler`, `opt.dev_only`) which is equivalent to JS nested `opt.action.handler` / `opt.action.dev_only`. JS `register_static_context_menu_option` wraps handler+dev_only into `{ handler, dev_only }` action object; C++ stores them directly. Behavior is identical.

- [x] 9. [app.cpp] click() disabled check uses bool parameter instead of DOM class check
- **JS Source**: `src/app.js` lines 369–371
- **Status**: Verified
- **Details**: ImGui has no DOM classes; disabled state is managed by `BeginDisabled()`/`EndDisabled()` which prevents interaction. The `bool disabled` parameter is the correct ImGui-idiomatic equivalent of checking `classList.contains('disabled')`. Behavior is identical.

- [x] 10. [app.cpp] JS source_select.setActive() called twice; C++ only calls once
- **JS Source**: `src/app.js` lines 696, 719
- **Status**: Verified
- **Details**: Both call sites are present in C++: line ~2804 inside the commented-out updater block (matching JS line 696), and line ~2839 unconditionally (matching JS line 719). Since the updater is disabled, only the unconditional call runs — matching JS behavior when `BUILD_RELEASE` is false.

- [x] 11. [app.cpp] whats-new.html path resolution differs from JS
- **JS Source**: `src/app.js` lines 710–711
- **Status**: Verified
- **Details**: C++ `DATA_DIR() / "whats-new.html"` follows the established convention where JS `src/` maps to C++ `data/` (same pattern as shaders: JS `src/shaders` → C++ `data/shaders`, and config: JS `src/default_config.jsonc` → C++ `data/default_config.jsonc`). Path is correct.

- [x] 12. [app.cpp] Vue error handler uses 'ERR_VUE' error code; C++ render catch uses 'ERR_RENDER'
- **JS Source**: `src/app.js` line 514
- **Status**: Verified
- **Details**: Changed C++ error code from `ERR_RENDER` to `ERR_VUE` to match JS. The C++ render-loop catch is the equivalent of the Vue error handler (both catch errors during UI rendering).

- [x] 13. [app.cpp] JS `data-kb-link` click handler for help articles not ported
- **JS Source**: `src/app.js` lines 116–131
- **Status**: Verified
- **Details**: N/A for ImGui — there are no DOM elements or global click delegation. The equivalent functionality is handled inline at each rendering site: components like `home-showcase.cpp` and `markdown-content.cpp` call `modules::openHelpArticle(kb_id)` directly when rendering kb-link elements. External link clicks call `ExternalLinks::open()` directly. This is the correct ImGui pattern.

- [x] 14. [app.cpp] JS `showDevTools()` for debug builds has no C++ equivalent
- **JS Source**: `src/app.js` lines 76–78
- **Status**: Verified
- **Details**: Expected platform difference. JS `showDevTools()` opens Chrome DevTools (NW.js-specific). No C++ equivalent exists. The C++ app has other debug aids (Dear ImGui Demo window, F5 reload, runtime log).

- [x] 15. [core.cpp] Loading screen updates are deferred to a main-thread queue instead of immediate state writes
- **JS Source**: `src/js/core.js` lines 413–420, 439–443
- **Details**: Verified. JS writes `core.view` loading fields synchronously because JS is single-threaded. C++ posts these mutations via `postToMainThread()` because `showLoadingScreen`/`hideLoadingScreen` are called from background threads (CASC loading). The one-frame delay is invisible to users and all state changes are identical to JS. This is a necessary platform adaptation for thread safety in multi-threaded C++.

- [x] 16. [core.cpp] progressLoadingScreen no longer awaits a forced redraw
- **JS Source**: `src/js/core.js` lines 426–434
- **Details**: Verified. JS calls `await generics.redraw()` to force a synchronous DOM repaint so progress is visible in single-threaded NW.js. In C++/ImGui, the main loop repaints every frame automatically, so a forced redraw is unnecessary. Loading happens on background threads, and state updates via `postToMainThread()` are picked up on the next frame render. This is a necessary platform adaptation for ImGui's immediate-mode rendering model.

- [x] 17. [core.cpp] Toast payload shape differs from JS `options` object contract
- **JS Source**: `src/js/core.js` lines 470–472
- **Details**: Verified. JS stores `{ type, message, options, closable }` where `options` is an object with string keys (labels) and function values (callbacks), rendered via `v-for="(func, label) in toast.options"`. C++ uses `std::vector<ToastAction>` where each ToastAction has `label` and `callback` fields — a type-safe equivalent. All JS callers pass `{ 'Label': () => fn() }` patterns that map directly to `ToastAction{label, callback}`. Empty vector equals JS `null` options. Functionally identical.

- [x] 18. [core.cpp] isDev uses NDEBUG instead of JS BUILD_RELEASE flag
- **JS Source**: `src/js/core.js` line 33
- **Details**: Verified. JS sets `isDev: !BUILD_RELEASE` where `BUILD_RELEASE = process.env.BUILD_RELEASE === 'true'`. C++ uses `#ifdef NDEBUG` in core.h (lines 260–264) which gives identical results: Debug → isDev=true, Release → isDev=false. app.cpp (lines 80–84) also maps NDEBUG to a `BUILD_RELEASE` constexpr bool for consistency. NDEBUG is the standard C++ mechanism for the same concept. RelWithDebInfo has NDEBUG defined (isDev=false), which is correct since it's an optimized build.

- [x] 19. [core.cpp] openInExplorer() is a C++ addition with no direct JS equivalent
- **JS Source**: `src/js/core.js` line 485 (`nw.Shell.openItem()`)
- **Details**: Verified. JS uses `nw.Shell.openItem(path)` which is a NW.js built-in that opens files/directories with the OS default handler. C++ extracts the platform-specific logic (ShellExecuteW on Windows, xdg-open on Linux) into `openInExplorer()` as a reusable function. This is a necessary platform adaptation — NW.js provides this built-in; C++ requires explicit platform code. The function correctly handles UTF-8→UTF-16 conversion on Windows via MultiByteToWideChar. `openExportDirectory()` calls `openInExplorer()` matching the JS `openExportDirectory` → `nw.Shell.openItem()` call chain.

- [x] 20. [config.cpp] Deep config watcher auto-save path from Vue is not equivalent
- **JS Source**: `src/js/config.js` lines 60–61
- **Status**: Verified
- **Details**: JS attaches `core.view.$watch('config', () => save(), { deep: true })` — Vue's deep watcher calls `save()` automatically on any config mutation. C++ has no reactive state system; ImGui is immediate-mode. Config changes trigger `config::save()` directly from UI code. This is a necessary platform adaptation — the explicit save() call pattern is the correct ImGui equivalent. Comment at config.cpp line 110–111 documents this.

- [x] 21. [config.cpp] save scheduling differs from setImmediate event-loop behavior
- **JS Source**: `src/js/config.js` lines 83–90
- **Status**: Verified
- **Details**: Fixed. JS defers with `setImmediate(doSave)` to the next event-loop tick. C++ now uses `core::postToMainThread(doSave)` to defer to the next frame tick, matching the JS deferred behavior. Previously used `std::async` which had a blocking-destructor issue (see item 25). The `postToMainThread` approach runs doSave on the main thread on the next frame, identical to JS running it on the main thread on the next event-loop tick.

- [x] 22. [config.cpp] doSave write failure behavior differs
- **JS Source**: `src/js/config.js` lines 106–108
- **Status**: Verified
- **Details**: Fixed. JS `await fsp.writeFile(...)` rejects on failure, which would propagate as an uncaught error. C++ now logs an error message via `logging::write()` when the file cannot be opened, rather than silently skipping. The error is logged rather than thrown because doSave runs deferred (via postToMainThread) where an uncaught exception would crash the app — matching JS behavior where setImmediate errors go to the global error handler.

- [x] 23. [config.cpp] EPERM detection logic differs from JS exception code check
- **JS Source**: `src/js/config.js` lines 43–49
- **Status**: Verified
- **Details**: JS checks `e.code === 'EPERM'` on the error object. C++ catches `std::exception` and checks the message text for "permission"/"EPERM"/"Permission denied"/"Access is denied". This works because `generics::readJSON()` throws `std::runtime_error("Permission denied: ...")` when a file exists but cannot be opened (generics.cpp line 500). The string-based detection correctly catches the permission error. Both paths log the same message and show the same toast. Functionally identical.

- [x] 24. [config.cpp] doSave() array comparison uses value equality instead of JS reference equality
- **JS Source**: `src/js/config.js` lines 98–104
- **Status**: Verified
- **Details**: Fixed. JS `defaultConfig[key] === value` uses strict reference equality — since `copyConfig` clones arrays with `value.slice(0)`, array config values are always different instances from defaults, so arrays are always persisted. C++ now skips the default-comparison for array values (`!it.value().is_array()` guard), ensuring arrays are always written to the user config file, matching JS behavior exactly.

- [x] 25. [config.cpp] save() std::async discarded future blocks in destructor making save synchronous
- **JS Source**: `src/js/config.js` lines 83–91
- **Status**: Verified
- **Details**: Fixed. Replaced `std::async(std::launch::async, doSave)` with `core::postToMainThread(doSave)`. The discarded `std::future` from `std::async` blocked in its destructor, making save synchronous. `postToMainThread` enqueues the task for the next frame — no future, no blocking, and thread-safe since doSave accesses `core::view->config` which lives on the main thread.

- [x] 26. [constants.h/constants.cpp] Version/flavour/build constants are compile-time values
- **JS Source**: `src/js/constants.js` line 46; `src/app.js` lines 44–47
- **Status**: Verified
- **Details**: JS reads `nw.App.manifest.version`, `.flavour`, and `.guid` at runtime from the NW.js package manifest. C++ defines `VERSION`, `FLAVOUR`, and `BUILD_GUID` as `inline constexpr std::string_view` in constants.h (lines 42, 47, 51). This is a necessary platform adaptation — there is no NW.js manifest in C++. The constants serve the identical purpose: identifying the build for update checks, user-agent strings, and crash reports. Values must be updated manually or via build-system substitution for releases, as documented in the header comment.

- [x] 27. [constants.cpp] DATA path root differs from JS nw.App.dataPath behavior
- **JS Source**: `src/js/constants.js` lines 16, 35–39
- **Status**: Verified
- **Details**: JS uses `nw.App.dataPath` which resolves to the OS-specific user-data directory (%LOCALAPPDATA% on Windows, ~/.config on Linux). C++ uses `<install>/data` — a subdirectory alongside the executable. This is a deliberate platform adaptation documented in constants.cpp lines 105–111. The C++ port bundles resources (shaders, config, fonts) in `data/` alongside the executable, while NW.js stores user data separately. All derived paths (cache, config, etc.) are computed from this base. Functionality is identical — only the root location differs.

- [x] 28. [constants.cpp] Cache directory constant changed from casc/ to cache/
- **JS Source**: `src/js/constants.js` lines 73–81
- **Status**: Verified
- **Details**: JS uses `path.join(DATA_PATH, 'casc')` as the cache directory name. C++ renames it to `cache/` with automatic migration from legacy `casc/` on first run (constants.cpp lines 149–155). The directory structure and content are identical — only the top-level name differs. The migration logic ensures backward compatibility. Deviation documented in constants.h line 67–68 and constants.cpp lines 144–148.

- [x] 29. [constants.cpp] Shader/default-config paths differ from JS layout
- **JS Source**: `src/js/constants.js` lines 43, 95–96
- **Status**: Verified
- **Details**: JS uses `path.join(INSTALL_PATH, 'src', 'shaders')` and `path.join(INSTALL_PATH, 'src', 'default_config.jsonc')`. C++ uses `<install>/data/shaders` and `<install>/data/default_config.jsonc`. This is a necessary adaptation — the C++ build system copies resources to `data/` via CMake POST_BUILD steps, while JS reads directly from `src/`. Deviations documented in constants.cpp lines 163–166 and 180–182. Functionality is identical.

- [x] 30. [constants.cpp] Updater helper extension mapping differs from JS
- **JS Source**: `src/js/constants.js` lines 18, 101
- **Status**: Verified
- **Details**: JS defines `UPDATER_EXT = { win32: '.exe', darwin: '.app' }` and uses `'updater' + (UPDATER_EXT[process.platform] || '')`. C++ uses `#ifdef _WIN32` to select `updater.exe` vs `updater` (constants.cpp lines 186–190). The darwin/macOS path is intentionally omitted — the C++ port targets Windows and Linux only (per project scope). On Linux, the JS fallback `|| ''` yields `'updater'` with no extension, matching the C++ `"updater"` exactly.

- [x] 31. [constants.cpp] Blender base-dir platform mapping omits JS darwin path
- **JS Source**: `src/js/constants.js` lines 24–32
- **Status**: Verified
- **Details**: JS handles `win32`, `darwin`, and `linux` in `getBlenderBaseDir()`. C++ `getBlenderBaseDir()` (constants.cpp lines 49–63) handles Windows (`%APPDATA%/Blender Foundation/Blender`) and Linux (`~/.config/blender`) with a comment noting the darwin case is intentionally omitted (line 56–57). This matches the project scope (Windows and Linux only). All three paths produce identical results on their respective platforms.

- [x] 32. [constants.cpp] RUNTIME_LOG placed in separate Logs/ subdirectory instead of DATA_PATH root
- **JS Source**: `src/js/constants.js` line 38
- **Status**: Verified
- **Details**: JS sets `RUNTIME_LOG: path.join(DATA_PATH, 'runtime.log')` — log file in the data directory root. C++ uses `<install>/Logs/runtime.log` — a separate `Logs/` subdirectory. This is a deliberate organizational choice documented in constants.cpp lines 113–116. The extra nesting level keeps log files separated from user data files. Functionality is identical — `logging::init()` and `getErrorDump()` both use `constants::RUNTIME_LOG()` consistently.

- [x] 33. [constants.cpp] Legacy directory migration code in init() has no JS equivalent
- **JS Source**: `src/js/constants.js` (no equivalent)
- **Status**: Verified
- **Details**: C++ `constants::init()` contains migration logic to rename legacy `config/`→`persistence/`→`data/` directories (lines 123–137) and `casc/`→`cache/` (lines 149–155). This has no JS counterpart and is a C++-specific addition to handle evolving directory naming across C++ port versions. The code is defensive (wrapped in try/catch, only runs when conditions are met), harmless if directories don't exist, and ensures seamless upgrades. Documented in constants.cpp lines 118–122 and 144–148.

- [x] 34. [constants.cpp] Explicit create_directories() for data and log dirs not present in JS
- **JS Source**: `src/js/constants.js` (no equivalent)
- **Status**: Verified
- **Details**: C++ `constants::init()` calls `fs::create_directories(s_data_dir)` and `fs::create_directories(s_log_dir)` (lines 141–142) to ensure directories exist before any module writes to them. JS relies on NW.js to automatically create `nw.App.dataPath` before the app starts. This is a necessary C++ adaptation — without it, first-run writes to config/log files would fail. The calls are idempotent (no-op if directories already exist).

- [x] 35. [log.cpp] `write()` API contract differs from JS variadic util.format behavior
- **JS Source**: `src/js/log.js` lines 78–80, 114
- **Details**: Added variadic template overload `write(std::format_string<Arg, Args...>, Arg&&, Args&&...)` in log.h that calls the base `write(std::string_view)`, matching JS `write(...parameters)` with `util.format()`. Callers can now use `logging::write("pattern {} {}", arg1, arg2)` directly.

- [x] 36. [log.cpp] Pool drain scheduling differs from JS `drain` event + `process.nextTick`
- **JS Source**: `src/js/log.js` lines 32–49, 111–112
- **Details**: Added `logging::flush()` function that drains all remaining pooled entries. The `drainPending` flag serves as C++ equivalent of `process.nextTick(drainPool)`. Comments updated to document the JS→C++ drain mechanism mapping. `flush()` should be called at shutdown to ensure no entries remain.

- [x] 37. [log.cpp] Log stream initialization timing differs from JS module-load behavior
- **JS Source**: `src/js/log.js` lines 111–112
- **Details**: Added `ensureStreamOpen()` helper called from `write()` and `flush()` that lazily opens the stream if `init()` wasn't called yet. This matches JS behavior where the stream is created at module-load time, ensuring no log entries are lost if writes occur before explicit initialization.

- [x] 38. [log.cpp] timeEnd() signature loses JS variadic parameter support
- **JS Source**: `src/js/log.js` lines 64–66
- **Status**: Verified
- **Details**: Added variadic template overload `timeEnd(std::format_string<Arg, Args...>, Arg&&, Args&&...)` in log.h that formats the label from the arguments and passes it to the base `timeEnd(std::string_view)`. This matches JS `timeEnd(label, ...params)` where extra params fill format specifiers in the label, and elapsed time is appended automatically.

- [x] 39. [log.cpp] getErrorDump() is synchronous in C++ vs async in JS
- **JS Source**: `src/js/log.js` lines 102–108
- **Status**: Verified
- **Details**: Documented as intentional deviation in both log.h and log.cpp. JS declares `getErrorDump` as async returning a Promise; C++ reads synchronously. This is deliberate — during a crash the event loop may be unavailable, so blocking I/O is more reliable for diagnostics.

- [x] 40. [updater.cpp] Update manifest flavour/guid source differs from JS runtime manifest
- **JS Source**: `src/js/updater.js` lines 24–26, 33–35, 113
- **Status**: Verified
- **Details**: Documented in checkForUpdates() comment. JS reads `nw.App.manifest.flavour/guid` at runtime; C++ uses `constants::FLAVOUR` and `constants::BUILD_GUID`. Both are set at build time and identify the current installation's update channel and version. Functional behavior is identical.

- [x] 41. [updater.cpp] Async update flow is flattened into synchronous calls
- **JS Source**: `src/js/updater.js` lines 50, 61, 79, 103–104, 119–124
- **Status**: Verified
- **Details**: Documented in applyUpdate() comment. JS uses async/await; C++ executes the same logical sequence synchronously. This is the standard JS async → C++ synchronous mapping — step ordering and error handling are identical, only the execution model differs (no event loop yielding).

- [x] 42. [updater.cpp] Launch failure logging omits JS error-object log line
- **JS Source**: `src/js/updater.js` lines 163–166
- **Status**: Verified
- **Details**: Added second `logging::write(std::string("Error: ") + e.what())` in the catch block of `launchUpdater()`, matching JS `log.write(e)` (updater.js line 165) which logs the raw error object via `util.format()`. The Error.toString() produces "Error: message", matched in C++.

- [x] 43. [updater.cpp] Linux fork+exec failure path has no error logging unlike JS child.on('error') handler
- **JS Source**: `src/js/updater.js` lines 150–155
- **Status**: Verified
- **Details**: Added pipe-based exec failure detection. The write end is set `FD_CLOEXEC` so a successful `execl()` closes it (parent reads EOF). On failure, the child writes `errno` to the pipe and the parent reads it, logging `"ERROR: Failed to spawn updater: <strerror>"` matching JS `child.on('error')` handler (updater.js line 153). Added `<fcntl.h>`, `<cerrno>`, `<cstring>` includes for Linux.

- [ ] 44. [screen_source_select.cpp] Source selection load flow is no longer Promise-based like JS
- **JS Source**: `src/js/modules/screen_source_select.js` lines 85–140, 142–167, 169–204, 267–287
- **Status**: Pending
- **Details**: JS source-open and build-load paths are async/await methods; C++ replaces these with synchronous calls and background-thread posting, changing timing/error propagation behavior versus the original module flow.

- [ ] 45. [screen_source_select.cpp] Hidden directory input reset/click flow is replaced with direct native dialog calls
- **JS Source**: `src/js/modules/screen_source_select.js` lines 252–258, 289–295, 326–337
- **Status**: Pending
- **Details**: JS creates persistent `<input nwdirectory>` selectors and resets `.value` before click to preserve reselection behavior; C++ calls `file_field::openDirectoryDialog()` directly and does not preserve the original selector-reset path.

- [ ] 46. [screen_source_select.cpp] CASC initialization failure toast omits JS support action
- **JS Source**: `src/js/modules/screen_source_select.js` lines 134–137
- **Status**: Pending
- **Details**: JS error toast includes both `View Log` and `Visit Support Discord` actions; C++ keeps only `View Log`, removing one original recovery handler.

- [ ] 47. [screen_source_select.cpp] Missing "Visit Support Discord" toast action button
- **JS Source**: `src/js/modules/screen_source_select.js` lines 134–137
- **Status**: Pending
- **Details**: JS setToast error for CASC load failures includes two action buttons: 'View Log' and 'Visit Support Discord' which calls ExternalLinks.open('::DISCORD'). The C++ only includes 'View Log' in both the local and remote CASC load error paths.

- [ ] 48. [screen_source_select.cpp] Hardcoded CDN URL format instead of using constants::PATCH::HOST
- **JS Source**: `src/js/modules/screen_source_select.js` line 215
- **Status**: Pending
- **Details**: JS uses util.format(constants.PATCH.HOST, region.tag) to build the CDN URL. C++ hardcodes the URL format string instead of using the constant.

- [ ] 49. [screen_source_select.cpp] CDN ping intermediate update batched instead of per-ping progressive
- **JS Source**: `src/js/modules/screen_source_select.js` lines 227–233
- **Status**: Pending
- **Details**: JS triggers Vue reactivity per-ping inside each pings finally(). C++ pushes results to a mutex-guarded vector drained in bulk by render(). UI updates are batched rather than per-ping.

- [ ] 50. [screen_settings.cpp] Settings descriptions/help text from JS template are largely omitted
- **JS Source**: `src/js/modules/screen_settings.js` lines 24–353
- **Status**: Pending
- **Details**: JS includes extensive per-setting explanatory `<p>` text and warning copy, but C++ mostly renders condensed headings/controls; this is a substantial visual/content mismatch versus the original settings screen.

- [ ] 51. [screen_settings.cpp] Cache/listfile interval labels changed from days to hours
- **JS Source**: `src/js/modules/screen_settings.js` lines 271–274, 329–332
- **Status**: Pending
- **Details**: JS labels `cacheExpiry` and `listfileCacheRefresh` as day-based values, while C++ headings explicitly state hours (`Cache Expiry (hours)`, `Listfile Update Frequency (hours)`), changing user-facing semantics.

- [ ] 52. [screen_settings.cpp] Multi-button style groups are replaced with radio/checkbox controls
- **JS Source**: `src/js/modules/screen_settings.js` lines 111–115, 178–183, 232–236
- **Status**: Pending
- **Details**: JS uses `.ui-multi-button` grouped toggles for path format, export metadata, and copy mode; C++ replaces these with ImGui radio buttons/checkboxes, causing visible layout and styling deviations.

- [ ] 53. [screen_settings.cpp] "Manually Clear Cache" heading missing "(Requires Restart)" from JS
- **JS Source**: `src/js/modules/screen_settings.js` lines 282–285
- **Status**: Pending
- **Details**: JS heading is "Manually Clear Cache (Requires Restart)". C++ doesn't have a separate heading for this section — it just renders a button with the cache size (line 419–421). The "(Requires Restart)" information is not conveyed to the user.

- [ ] 54. [screen_settings.cpp] WebP Quality uses `SliderInt` vs JS `<input type="number">`
- **JS Source**: `src/js/modules/screen_settings.js` lines 156–157
- **Status**: Pending
- **Details**: JS uses `<input type="number" min="1" max="100">` for WebP quality — a numeric input field with up/down arrows. C++ uses `ImGui::SliderInt` (line 306) — a draggable slider. The interaction model differs: clicking/typing a specific value vs dragging to a value. Visual appearance also differs.

- [ ] 55. [screen_settings.cpp] Config buttons not visually disabled when busy
- **JS Source**: `src/js/modules/screen_settings.js` lines 355–357
- **Status**: Pending
- **Details**: JS applies `:class="{ disabled: $core.view.isBusy }"` to all 3 buttons (Discard, Apply, Reset to Defaults), visually greying them out. C++ doesn't apply disabled styling to the buttons (lines 509–519) — the functions internally check `isBusy` but the buttons appear clickable. Users may click buttons that silently do nothing when busy.

- [ ] 56. [screen_settings.cpp] Encryption key inputs don't enforce `maxlength` from JS
- **JS Source**: `src/js/modules/screen_settings.js` lines 295–296
- **Status**: Pending
- **Details**: JS key name input has `maxlength="16"` and key value input has `maxlength="32"`. C++ uses `char key_name_buf[32]` and `char key_val_buf[64]` (lines 433–434) but `ImGui::InputText` doesn't enforce the JS maxlength limits — the C++ buffers are larger than the JS maximums, allowing longer input.

- [ ] 57. [screen_settings.cpp] Listfile Source heading missing "(Legacy)" suffix
- **JS Source**: `src/js/modules/screen_settings.js` lines 322–323
- **Status**: Pending
- **Details**: JS heading is "Listfile Source (Legacy)". C++ heading is just "Listfile Source" (line 460). The "(Legacy)" qualifier that distinguishes this from the binary listfile source is missing.

- [ ] 58. [screen_settings.cpp] Locale dropdown shows full locale names vs JS short locale keys
- **JS Source**: `src/js/modules/screen_settings.js` lines 381–383
- **Status**: Pending
- **Details**: JS `available_locale_keys` computed property returns `{ value: e }` objects where `e` is the short key like "enUS". The MenuButton displays these short keys. C++ creates `MenuOption` with `label = getName(key)` (e.g., "English (US)") and `value = key` (line 291). The dropdown items show full locale names in C++ but short codes in JS.

- [ ] 59. [modules.cpp] Dynamic component registry and hot-reload proxy behavior is not ported
- **JS Source**: `src/js/modules.js` lines 6–24, 62–109, 270–275
- **Status**: Pending
- **Details**: JS exposes `COMPONENTS`, `COMPONENT_PATH_MAP`, `component_cache`, and `component_registry` proxy with dynamic require-cache invalidation; C++ replaces this with a static/no-op `register_components()` path.

- [ ] 60. [modules.cpp] `wrap_module` computed helper injection differs from JS
- **JS Source**: `src/js/modules.js` lines 200–207
- **Status**: Pending
- **Details**: JS injects `$modules`, `$core`, and `$components` via `module_def.computed`; C++ does not provide equivalent computed helper bindings.

- [ ] 61. [modules.cpp] `activated` lifecycle wrapping logic is missing
- **JS Source**: `src/js/modules.js` lines 244–251
- **Status**: Pending
- **Details**: JS wraps `activated` so `initialize()` is retried before calling original `activated`; C++ only wraps `initialize` and does not implement equivalent `activated` wrapper behavior.

- [ ] 62. [modules.cpp] `initialize(core_instance)` bootstrap flow differs from JS module wiring
- **JS Source**: `src/js/modules.js` lines 277–287
- **Status**: Pending
- **Details**: JS stores `core_instance`, assigns `manager = module.exports`, and wraps every entry in `MODULES` with `Vue.markRaw`; C++ `initialize()` takes no core parameter and builds a static function-pointer registry instead.

- [ ] 63. [modules.cpp] `set_active` assigns different active-module payload to view state
- **JS Source**: `src/js/modules.js` lines 254–267, 293–303
- **Status**: Pending
- **Details**: JS sets `core.view.activeModule` to the wrapped module proxy (including proxy getters like `__name`/`setActive`/`reload`); C++ writes a minimal JSON object with `__name` only.

- [ ] 64. [modules.cpp] Dev hot-reload no longer refreshes module/component code from disk
- **JS Source**: `src/js/modules.js` lines 337–355, 395–401
- **Status**: Pending
- **Details**: JS clears `require.cache` and re-requires modules/components during reload; C++ only resets internal flags and re-wraps existing in-memory module definitions.

- [ ] 65. [modules.cpp] `set_active` eagerly initializes modules unlike JS activation flow
- **JS Source**: `src/js/modules.js` lines 244–251, 293–303
- **Status**: Pending
- **Details**: JS `set_active` only switches `core.view.activeModule`; first-time initialization is triggered by wrapped `activated()`. C++ calls `initialize()` directly inside `set_active`, changing lifecycle timing/side effects.

- [ ] 66. [modules.cpp] `modContextMenuOptions` payload omits JS `action` object data
- **JS Source**: `src/js/modules.js` lines 162–167, 176–198, 289–291
- **Status**: Pending
- **Details**: JS writes option objects containing `action` (or `{ handler, dev_only }` for static options) into `core.view.modContextMenuOptions`; C++ serializes only `id/label/icon/dev_only` to view state, dropping JS action payload shape.

- [ ] 67. [modules.cpp] Unknown nav/context entries lose JS insertion-order behavior
- **JS Source**: `src/js/modules.js` lines 112–114, 139–157, 176–195
- **Status**: Pending
- **Details**: JS builds arrays from `Map` insertion order before stable sort (entries not in order arrays keep insertion order). C++ stores entries in `std::map`, so unordered items are pre-sorted by key, changing final order semantics.

- [ ] 68. [modules.cpp] `display_label` in `wrap_module` error messages shows module key instead of nav button label
- **JS Source**: `src/js/modules.js` lines 208–213
- **Status**: Pending
- **Details**: In JS `wrap_module`, `display_label` starts as `module_name` but is updated to `label` inside the `registerNavButton` callback (line 213: `display_label = label`). The captured `display_label` is then used in the wrapped `initialize` error messages (line 236–237), producing user-friendly text like "Failed to initialize Maps tab". In C++ `wrap_module` (modules.cpp line 194), `display_label` is set to `mod.name` and never updated from the nav button registration, so error messages show the module key (e.g., "Failed to initialize tab_maps tab") instead of the display label (e.g., "Failed to initialize Maps tab").

- [ ] 69. [module_test_b.cpp] Busy-state text formatting differs from JS boolean rendering
- **JS Source**: `src/js/modules/module_test_b.js` line 8
- **Status**: Pending
- **Details**: JS displays `Busy State` using Vue boolean/string rendering, while C++ prints `core::view->isBusy` with `%d`, emitting numeric values rather than matching the original presentation contract.

- [ ] 70. [module_test_b.cpp] Message char buffer limited to 255 chars vs JS unlimited string
- **JS Source**: `src/js/modules/module_test_b.js` lines 15–16
- **Status**: Pending
- **Details**: JS `data()` returns `{ message: 'Hello Thrall' }` with no length limit. C++ uses `static char message[256]` (line 19). If a user types a message longer than 255 characters, C++ will silently truncate while JS would accept it fully.

- [ ] 71. [module_test_b.cpp] Uses `logging::write` vs JS `console.log`
- **JS Source**: `src/js/modules/module_test_b.js` lines 38, 42
- **Status**: Pending
- **Details**: JS `mounted`/`unmounted` hooks use `console.log`. C++ uses `logging::write` (lines 43, 47). Same difference as module_test_a — different logging target.

- [ ] 72. [module_test_a.cpp] Module UI template structure differs from JS component markup
- **JS Source**: `src/js/modules/module_test_a.js` lines 2–8
- **Status**: Pending
- **Details**: JS renders a structured HTML block (`.module-test-a`, `<h2>`, `<p>`, buttons) styled via CSS, while C++ uses plain ImGui text/button primitives without equivalent DOM/CSS layout fidelity.

- [ ] 73. [module_test_a.cpp] Counter is static (persists across mount/unmount) vs JS instance data (resets)
- **JS Source**: `src/js/modules/module_test_a.js` lines 11–13
- **Status**: Pending
- **Details**: JS `data()` returns `{ counter: 0 }` — each mount creates fresh component state, resetting counter to 0. C++ `static int counter = 0` (line 16) persists across module activations/deactivations. If the user switches away and back, JS counter resets to 0 but C++ counter keeps its accumulated value.

- [ ] 74. [module_test_a.cpp] Uses `logging::write` vs JS `console.log`
- **JS Source**: `src/js/modules/module_test_a.js` lines 28, 32
- **Status**: Pending
- **Details**: JS `mounted`/`unmounted` lifecycle hooks use `console.log('module_test_a mounted/unmounted')`. C++ uses `logging::write(...)` (lines 32–33). The output destination differs: JS goes to browser devtools console; C++ goes to the application log file. For a test module this is minor but changes observability.

- [ ] 75. [blob.cpp] Blob stream semantics differ (async pull stream vs eager callback loop)
- **JS Source**: `src/js/blob.js` lines 271–288
- **Details**: JS returns a lazy `ReadableStream` with async `pull()`, but C++ `BlobPolyfill::stream` is synchronous and eager.

- [ ] 76. [blob.cpp] URLPolyfill.createObjectURL native fallback path is missing
- **JS Source**: `src/js/blob.js` lines 294–300
- **Details**: JS falls back to native `URL.createObjectURL(blob)` for non-polyfill blobs; C++ only accepts `BlobPolyfill` and has no fallback path.

- [ ] 77. [blob.cpp] URLPolyfill.revokeObjectURL native revoke path is missing
- **JS Source**: `src/js/blob.js` lines 302–306
- **Details**: JS calls native `URL.revokeObjectURL(url)` for non-`data:` URLs; C++ is effectively a no-op for that case.

- [ ] 78. [blob.cpp] BlobPolyfill::slice() treats end=0 differently from JS
- **JS Source**: `src/js/blob.js` lines 262–265
- **Status**: Pending
- **Details**: JS uses `end || this._buffer.length`, where `||` treats 0 as falsy and defaults to the full buffer length. C++ uses `std::optional<std::size_t>` with `value_or(_buffer.size())`, so passing `end=0` gives a 0-length slice in C++ but the full buffer in JS. This is a behavioral difference for the edge case where `end` is explicitly 0.

- [ ] 79. [buffer.cpp] alloc(false) behavior differs from JS allocUnsafe
- **JS Source**: `src/js/buffer.js` lines 54–56
- **Details**: JS uses `Buffer.allocUnsafe()` when `secure=false`; C++ always zero-initializes via `std::vector<uint8_t>(length)`.

- [ ] 80. [buffer.cpp] fromCanvas API/behavior is not directly ported
- **JS Source**: `src/js/buffer.js` lines 89–107
- **Details**: JS accepts canvas/OffscreenCanvas and browser Blob APIs, while C++ replaces this with `fromPixelData(...)` and different call semantics.

- [ ] 81. [buffer.cpp] readString encoding parameter does not affect behavior
- **JS Source**: `src/js/buffer.js` lines 551–553
- **Details**: JS passes `encoding` into `Buffer.toString(encoding, ...)`; C++ accepts the parameter but ignores it.

- [ ] 82. [buffer.cpp] decodeAudio(context) method is missing
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Details**: JS exposes `decodeAudio(context)` on `BufferWrapper`; C++ intentionally omits this method and relies on other audio paths.

- [ ] 83. [buffer.cpp] getDataURL creates data URLs instead of blob URLs
- **JS Source**: `src/js/buffer.js` lines 989–995
- **Details**: JS uses `URL.createObjectURL(new Blob(...))` (`blob:` URLs), while C++ emits `data:application/octet-stream;base64,...`.

- [ ] 84. [buffer.cpp] readBuffer wrap parameter is split into separate APIs
- **JS Source**: `src/js/buffer.js` lines 531–542
- **Details**: JS has `readBuffer(length, wrap=true, inflate=false)`; C++ replaces this with `readBuffer(...)` and `readBufferRaw(...)`.

- [ ] 85. [buffer.cpp] setCapacity(secure=false) always zero-initializes unlike JS Buffer.allocUnsafe
- **JS Source**: `src/js/buffer.js` lines 1021–1029
- **Status**: Pending
- **Details**: JS `setCapacity` uses `Buffer.allocUnsafe(capacity)` when `secure=false`, leaving expanded memory uninitialized. C++ uses `std::vector<uint8_t>(capacity, 0)` which always zero-initializes. This mirrors the existing alloc() difference (TODO 6) but applies to setCapacity specifically. Performance-only difference with no functional impact.

- [ ] 86. [buffer.cpp] startsWith(array) reads entries from sequential positions instead of all from offset 0
- **JS Source**: `src/js/buffer.js` lines 592–604
- **Status**: Pending
- **Details**: Both JS and C++ `startsWith(array)` call `seek(0)` once before the loop, then read each alternative entry sequentially. After reading the first entry (e.g., 3 bytes), the offset advances, so the second entry is checked at offset 3, not 0. This is a faithful port of the JS logic, but the likely intent is to check if the buffer starts with ANY of the alternatives (all from offset 0). In practice, FILE_IDENTIFIERS matching likely calls the single-string overload per match, so the array overload may not be triggered for multi-match identifiers.

- [ ] 87. [generics.cpp] Exported get() API shape differs from JS fetch-style response contract
- **JS Source**: `src/js/generics.js` lines 22–54
- **Details**: JS `get()` returns a fetch `Response` object (`ok`, `status`, `statusText`, body/json methods), while C++ `get()` returns raw bytes and hides response metadata from callers.

- [ ] 88. [generics.cpp] get() fallback completion behavior differs when all URLs return non-ok responses
- **JS Source**: `src/js/generics.js` lines 38–54
- **Details**: JS returns the last non-ok `Response` after exhausting fallback URLs; C++ throws `HTTP <status> <statusText>` instead of returning a non-ok response object.

- [ ] 89. [generics.cpp] requestData status/redirect handling differs from JS manual 3xx flow
- **JS Source**: `src/js/generics.js` lines 159–167
- **Details**: JS manually follows 301/302 and accepts status codes up to 302 in that flow; C++ relies on auto-follow and then enforces a strict 2xx success check in `doHttpGetRaw()`.

- [ ] 90. [generics.cpp] redraw() is a no-op instead of double requestAnimationFrame scheduling
- **JS Source**: `src/js/generics.js` lines 263–268
- **Details**: JS resolves `redraw()` only after two animation frames to force UI repaint ordering; C++ redraw is intentionally empty, changing timing guarantees for callers that rely on redraw completion.

- [ ] 91. [generics.cpp] batchWork scheduling model differs from MessageChannel event-loop batching
- **JS Source**: `src/js/generics.js` lines 420–469
- **Details**: JS slices work via MessageChannel posts between batches; C++ runs batches in a tight loop with `std::this_thread::yield()`, which is not equivalent to browser event-loop task scheduling.

- [ ] 92. [generics.cpp] get() timeout semantics differ from JS 30-second AbortSignal
- **JS Source**: `src/js/generics.js` lines 23–31
- **Status**: Pending
- **Details**: JS uses `AbortSignal.timeout(30000)` which enforces a hard 30-second total timeout for the entire fetch operation. C++ `doHttpGet()` (generics.cpp lines 123–124) sets `set_connection_timeout(30)` and `set_read_timeout(60)` separately, allowing up to 90 seconds total (30s to connect + 60s to read). This means C++ requests can take up to 3x longer than the JS equivalent before timing out.

- [ ] 93. [generics.cpp] queue() initial batch size off-by-one compared to JS
- **JS Source**: `src/js/generics.js` lines 63–83
- **Status**: Pending
- **Details**: JS initializes `free = limit` and `complete = -1`, then calls `check()` which increments both (`complete++; free++`), resulting in `free = limit + 1` items launched in the first batch. C++ (generics.cpp lines 399–433) launches exactly `limit` items in the first batch. This off-by-one means the JS version processes `limit + 1` items concurrently on the initial dispatch, while C++ processes exactly `limit`.

- [ ] 94. [generics.cpp] fileExists() checks existence only, not accessibility like JS fsp.access
- **JS Source**: `src/js/generics.js` lines 343–350
- **Status**: Pending
- **Details**: JS uses `await fsp.access(file)` which checks that the file both exists AND is accessible to the current process (read permission). C++ uses `std::filesystem::exists(file)` (generics.cpp line 710) which only checks existence, not accessibility. A file that exists but has restrictive permissions (e.g., mode 000) would return `true` in C++ but `false` in JS, potentially causing downstream errors when attempting to read/open such files.

- [ ] 95. [generics.cpp] computeFileHash() has malformed duplicate doc comment block
- **JS Source**: `src/js/generics.js` lines 328–336
- **Status**: Pending
- **Details**: generics.cpp lines 251–258 contain a broken doc comment where a new `/**` block starts (line 254) before the previous `/**` block (line 252) is properly closed. The first comment says "Compute hash of a file using streaming I/O." and the second says "Compute hash of a file using streaming mbedTLS MD API." This is a documentation-only issue that does not affect compilation but makes the comment block malformed.

- [ ] 96. [generics.cpp] downloadFile() error logging loses full error object details
- **JS Source**: `src/js/generics.js` lines 243–244
- **Status**: Pending
- **Details**: JS logs the full error object with `log.write(error)` (line 244) which includes the error message, stack trace, and any additional properties. C++ (generics.cpp line 595) only logs `error.what()` message string via `logging::write(std::format("Failed to download from {}: {}", currentUrl, error.what()))`. Stack trace and other diagnostic information available in the original JS error are lost.

- [ ] 97. [generics.cpp] requestData() is publicly declared but is a private/unexported function in JS
- **JS Source**: `src/js/generics.js` lines 145–205, 484–502
- **Status**: Pending
- **Details**: JS `requestData()` is a file-scoped function that is NOT included in `module.exports` (lines 484–502). C++ declares `requestData()` in `generics.h` line 86 as a public function in the `generics` namespace, making it part of the public API. While this doesn't break functionality, it exposes an internal implementation detail that JS keeps private, and external callers could depend on this function when they shouldn't.

- [ ] 98. [mmap.cpp] Module architecture differs from JS wrapper around `mmap.node`
- **JS Source**: `src/js/mmap.js` lines 8–23, 49–52
- **Details**: JS delegates mapping behavior to native addon object construction (`new mmap_native.MmapObject()`); C++ reimplements mapping logic directly in this module, changing parity with the original JS/native boundary.

- [ ] 99. [mmap.cpp] Virtual-file ownership semantics differ from JS object-lifetime model
- **JS Source**: `src/js/mmap.js` lines 14–24, 30–43
- **Details**: JS tracks objects in a `Set` and only calls `unmap()`/`clear()`; C++ tracks raw pointers in a global set and deletes them in `release_virtual_files()`, introducing different lifetime/aliasing behavior versus JS-managed object references.

- [ ] 100. [mmap.cpp] C++ map() explicitly rejects empty files which JS wrapper does not handle
- **JS Source**: `src/js/mmap.js` lines 20–23
- **Status**: Pending
- **Details**: C++ `MmapObject::map()` explicitly checks for `size == 0` and returns false with `lastError = "File is empty"` (mmap.cpp lines 113–118 on Windows, lines 162–167 on Linux). The JS wrapper in `mmap.js` has no such guard — it delegates entirely to `new mmap_native.MmapObject()` and the native addon's `map()` method. Whether the native `.node` addon rejects empty files is unknown from the JS source alone, making this a potential behavioral divergence where C++ would fail on empty files while JS might succeed (mapping zero bytes).

- [ ] 101. [xml.cpp] End-of-input handling can dereference past bounds unlike JS parser semantics
- **JS Source**: `src/js/xml.js` lines 25–29, 39, 89, 97
- **Status**: Pending
- **Details**: JS safely reads `xml[pos]` as `undefined` at end-of-input, but C++ reads `xml[pos]` in `parse_attributes()`/`parse_node()` without guarding `pos < xml.size()` at several checks, which can trigger out-of-bounds access on malformed/truncated XML.

- [ ] 102. [subtitles.cpp] `get_subtitles_vtt` API and data-loading path differ from JS
- **JS Source**: `src/js/subtitles.js` lines 172–175
- **Status**: Pending
- **Details**: JS `get_subtitles_vtt(casc, file_data_id, format)` loads file data internally via CASC; C++ takes preloaded subtitle text and format only.

- [ ] 103. [subtitles.cpp] BOM stripping behavior differs from original JS
- **JS Source**: `src/js/subtitles.js` lines 176–178
- **Status**: Pending
- **Details**: JS removes leading UTF-16 BOM codepoint (`0xFEFF`) via `charCodeAt`; C++ strips UTF-8 byte-order mark bytes (`EF BB BF`) instead.

- [ ] 104. [subtitles.cpp] Invalid SBT timestamp parsing semantics differ from JS `parseInt` behavior
- **JS Source**: `src/js/subtitles.js` lines 13–20
- **Status**: Pending
- **Details**: JS uses `parseInt(...)` and can propagate `NaN` for malformed timestamp segments, while C++ digit-filter parsing can still produce numeric output from mixed/invalid strings.

- [ ] 105. [wmv.cpp] safe_parse_int returns 0 for fully non-numeric strings while JS parseInt returns NaN
- **JS Source**: `src/js/wmv.js` lines 44, 57–58, 87–91
- **Status**: Pending
- **Details**: JS `parseInt('abc')` returns `NaN`, which propagates through the parsed result (e.g., in v1 `legacy_values` or v2 `customizations`/`equipment`). C++ `safe_parse_int()` (wmv.cpp lines 26–43) catches `std::stoi` exceptions for non-numeric strings and returns `std::nullopt`, which callers convert to 0 via `value_or(0)`. For .chr files with non-numeric `@_value` attributes, JS would store `NaN` while C++ stores `0`, potentially causing different downstream behavior in character customization or equipment application.

- [ ] 106. [external-links.h] Windows open() uses naive wstring conversion instead of proper MultiByteToWideChar
- **JS Source**: `src/js/external-links.js` lines 31–35
- **Status**: Pending
- **Details**: `ExternalLinks::open()` in external-links.h line 75 converts URL to wstring via `std::wstring(url.begin(), url.end())`, a naive char-by-char copy that only works for ASCII characters. This is inconsistent with `core::openInExplorer()` (core.cpp lines 418–420) which properly uses `MultiByteToWideChar(CP_UTF8, ...)` for correct UTF-8 to UTF-16 conversion. While URLs are typically ASCII, this is a correctness bug for any URL containing non-ASCII bytes.

- [ ] 107. [external-links.h] wowHead_viewItem() hardcodes URL string instead of using WOWHEAD_ITEM constant
- **JS Source**: `src/js/external-links.js` lines 24, 42–43
- **Status**: Pending
- **Details**: JS defines `const WOWHEAD_ITEM = 'https://www.wowhead.com/item=%d'` and uses it via `util.format(WOWHEAD_ITEM, itemID)`. C++ defines `WOWHEAD_ITEM` as `"https://www.wowhead.com/item={}"` (external-links.h line 48) but `wowHead_viewItem()` (line 89) hardcodes `std::format("https://www.wowhead.com/item={}", itemID)` instead of using the constant. The constant is effectively dead code.

- [ ] 108. [external-links.h] renderLink() missing CSS a:hover visual effects (color change and underline)
- **JS Source**: `src/app.css` lines 93–101 (a tag styling, a:hover)
- **Status**: Pending
- **Details**: `ExternalLinks::renderLink()` (external-links.h lines 107–117) only changes the cursor to a hand on hover. The original CSS defines `a:hover { color: var(--font-highlight); text-decoration: underline; }` which means links should change to pure white (#ffffff) and show an underline on hover. The C++ renderLink() is missing both the hover color change and the underline decoration, causing a visual fidelity difference from the original JS app.

- [ ] 109. [external-links.cpp] JS logic is not implemented in the .cpp translation unit
- **JS Source**: `src/js/external-links.js` lines 12–44
- **Details**: `external-links.cpp` only includes `external-links.h`; the sibling `.cpp` file does not contain line-by-line equivalents of JS constants/methods (`STATIC_LINKS`, `WOWHEAD_ITEM`, `open`, `wowHead_viewItem`).

- [ ] 110. [gpu-info.cpp] macOS GPU info path from JS is missing in C++
- **JS Source**: `src/js/gpu-info.js` lines 199–243
- **Details**: JS implements `get_macos_gpu_info()` and a `darwin` branch in `get_platform_gpu_info()`, but C++ only handles Windows/Linux and returns `nullopt` for all other platforms.

- [ ] 111. [gpu-info.cpp] WebGL debug renderer detection logic differs from JS extension-gated behavior
- **JS Source**: `src/js/gpu-info.js` lines 30–34, 340–347
- **Details**: JS only populates vendor/renderer when `WEBGL_debug_renderer_info` is available and otherwise logs `WebGL debug info unavailable`; C++ reads `GL_VENDOR/GL_RENDERER` directly, changing the fallback path and emitted diagnostics.

- [ ] 112. [gpu-info.cpp] `exec_cmd` timeout behavior is not equivalent on Windows
- **JS Source**: `src/js/gpu-info.js` lines 65–73
- **Details**: JS enforces `{ timeout: 5000 }` through `child_process.exec`; C++ only wraps Linux commands with `timeout 5` and does not enforce the same timeout semantics on Windows `_popen`.

- [ ] 113. [gpu-info.cpp] Extension category normalization diverges from JS WebGL formatting
- **JS Source**: `src/js/gpu-info.js` lines 250–303
- **Details**: JS normalizes `WEBGL_/EXT_/OES_` extension names for compact logging, while C++ uses different `GL_ARB_/GL_EXT_/GL_OES_` stripping rules and produces different category labels/content.

- [ ] 114. [gpu-info.cpp] Caps logging condition differs from JS — C++ uses `max_tex_size > 0` while JS always logs caps
- **JS Source**: `src/js/gpu-info.js` lines 348–349
- **Status**: Pending
- **Details**: JS checks `if (webgl.caps)` (line 348) before logging capabilities. Since `caps` is always assigned as an object `{}` (line 25), this condition is always truthy when the WebGL context exists. C++ (gpu-info.cpp line 539) checks `if (gl->caps.max_tex_size > 0)` which would skip caps logging if `max_tex_size` happened to be 0. While unlikely in practice with real GPUs, this is a behavioral deviation from JS which unconditionally logs caps when the GL context is available.

- [ ] 115. [font_helpers.cpp] `detect_glyphs_async` no longer implements JS DOM/callback contract
- **JS Source**: `src/js/modules/font_helpers.js` lines 56–106
- **Status**: Pending
- **Details**: JS clears and repopulates `grid_element`, wires per-glyph click handlers, and invokes `on_complete`; C++ only accumulates codepoints in `GlyphDetectionState` and drops the DOM/callback path from the original module API.

- [ ] 116. [font_helpers.cpp] Active detection cancellation semantics differ from JS global `active_detection`
- **JS Source**: `src/js/modules/font_helpers.js` lines 17, 57–63
- **Status**: Pending
- **Details**: JS tracks a module-level `active_detection` and cancels prior runs automatically, while C++ relies on caller-owned state and does not preserve the same global cancellation behavior across concurrent detections.

- [ ] 117. [font_helpers.cpp] `inject_font_face` return type/behavior differs from JS blob-URL + `document.fonts` flow
- **JS Source**: `src/js/modules/font_helpers.js` lines 113–133
- **Status**: Pending
- **Details**: JS injects `@font-face`, waits for `document.fonts.load/check`, and returns a URL string; C++ adds the font directly to ImGui atlas and returns `ImFont*`, dropping JS URL lifecycle and decode verification behavior.

- [ ] 118. [font_helpers.cpp] `check_glyph_support` uses fundamentally different detection algorithm
- **JS Source**: `src/js/modules/font_helpers.js` lines 30–53
- **Status**: Pending
- **Details**: JS detects glyph support by rendering the character with a fallback font and the target font on a canvas, then comparing alpha sums. C++ uses `ImFont::IsGlyphInFont()` which only checks if a glyph index exists in the loaded font atlas. The JS canvas-based approach can detect visual differences between fallback and target glyphs; the C++ approach may report false positives for glyphs mapped to the fallback/notdef glyph.

- [ ] 119. [font_helpers.cpp] `inject_font_face` is synchronous and returns `void*` vs JS async returning blob URL string
- **JS Source**: `src/js/modules/font_helpers.js` lines 113–133
- **Status**: Pending
- **Details**: JS `inject_font_face` is async: creates a `@font-face` CSS rule with a blob URL, calls `document.fonts.load()`, verifies with `document.fonts.check()`, and returns the blob URL string (or throws). C++ version is synchronous: calls `ImGui::MemAlloc` + `AddFontFromMemoryTTF`, returns `void*` pointer to `ImFont`. Callers that depend on the return type being a string or the async flow will behave differently.

- [ ] 120. [icon-render.cpp] Icon load pipeline is still stubbed and never replaces placeholders
- **JS Source**: `src/js/icon-render.js` lines 57–64, 93–106
- **Details**: JS asynchronously loads CASC icon data, decodes BLP, and updates the rendered icon; C++ `processQueue()` contains placeholder comments and does not fetch/decode icon data, so queued icons remain on the default image.

- [ ] 121. [icon-render.cpp] Queue execution model differs from JS async recursive processing
- **JS Source**: `src/js/icon-render.js` lines 48–65, 77–91
- **Details**: JS processes one queue entry per async chain and re-enters via `.finally(() => processQueue())`; C++ drains the queue in a synchronous `while` loop, changing scheduling and starvation behavior.

- [ ] 122. [icon-render.cpp] Dynamic stylesheet/CSS-rule icon path is replaced with non-equivalent texture cache flow
- **JS Source**: `src/js/icon-render.js` lines 14–46, 67–75, 93–109
- **Details**: JS creates `.icon-<id>` CSS rules in a dynamic stylesheet and renders via `background-image`; C++ uses `_registeredIcons/_textureCache` and does not implement stylesheet rule insertion/removal semantics from the JS module.

- [ ] 123. [icon-render.h] Truncated doc comment — sentence fragment on line 17
- **JS Source**: `src/js/icon-render.js` lines 1–109
- **Status**: Pending
- **Details**: The namespace doc comment in `icon-render.h` lines 14–18 contains a sentence fragment: `" * in a cache. The queue mechanism with priority ordering is preserved."` on line 17. The preceding text (lines 14–15) describes the JS CSS-based approach but the transition to the C++ approach is missing — "in a cache." is an orphaned fragment, likely left over from an incomplete edit. The full comment should describe that in C++, icons are loaded as BLP files, decoded to RGBA pixel data, and stored as GL textures in a cache.

- [ ] 124. [stb-impl.cpp] Required sibling JS source file is missing, blocking parity verification
- **JS Source**: `src/js/stb-impl.js` lines N/A (file missing)
- **Status**: Blocked
- **Details**: `src/js/stb-impl.cpp` exists, but `src/js/stb-impl.js` is absent, so line-by-line comparison against an original JS sibling cannot be completed.

- [ ] 125. [png-writer.cpp] `write()` call contract differs from JS async behavior
- **JS Source**: `src/js/png-writer.js` lines 243–249
- **Status**: Pending
- **Details**: JS `write(file)` is `async` and returns/awaits `this.getBuffer().writeToFile(file)`; C++ `PNGWriter::write(...)` is synchronous `void`.

- [ ] 126. [tiled-png-writer.cpp] `write()` contract is synchronous instead of JS Promise-based async
- **JS Source**: `src/js/tiled-png-writer.js` lines 123–125
- **Status**: Pending
- **Details**: JS exposes `async write(file)` and returns `await this.getBuffer().writeToFile(file)`, while C++ `TiledPNGWriter::write(...)` is `void` and synchronous.

- [ ] 127. [file-writer.cpp] writeLine backpressure/await behavior differs from JS stream semantics
- **JS Source**: `src/js/file-writer.js` lines 24–33, 35–38
- **Details**: JS `writeLine()` is async and waits on resolver/drain when `stream.write()` backpressures; C++ writes synchronously and keeps blocked/drain as structural no-ops.

- [ ] 128. [file-writer.cpp] Closed-stream writes are silently ignored unlike JS stream-end behavior
- **JS Source**: `src/js/file-writer.js` lines 24–33, 40–42
- **Details**: C++ guards `writeLine()`/`close()` with `is_open()` and returns early; JS writes to a stream that has been `end()`ed follow Node stream error semantics rather than a silent no-op guard.

- [ ] 129. [audio-helper.cpp] AudioPlayer::load does not return decoded buffer like JS
- **JS Source**: `src/js/ui/audio-helper.js` lines 31–35
- **Status**: Pending
- **Details**: JS `load()` returns the decoded `AudioBuffer`; C++ `AudioPlayer::load(...)` returns `void`, changing function contract.

- [ ] 130. [audio-helper.cpp] End-of-playback callback is polling-driven instead of event-driven
- **JS Source**: `src/js/ui/audio-helper.js` lines 57–67, 115–127
- **Status**: Pending
- **Details**: JS triggers completion via `source.onended`; C++ checks `ma_sound_at_end()` inside `get_position()` and invokes `on_ended` there, requiring polling and adding side effects to position queries.

- [ ] 131. [audio-helper.cpp] get_position() has side effects not present in JS
- **JS Source**: `src/js/ui/audio-helper.js` lines 115–130
- **Status**: Pending
- **Details**: In JS, `get_position()` is a pure getter — the browser's `onended` callback independently fires when playback finishes. In C++, `get_position()` polls `ma_sound_at_end()` and fires `on_ended`, resets `is_playing`, calls `stop_source()`, and resets `start_offset`. Consumers MUST poll `get_position()` periodically for end-of-playback detection. Documented in code but architecturally different.

- [ ] 132. [audio-helper.cpp] detectFileType accepts raw bytes instead of BufferWrapper
- **JS Source**: `src/js/ui/audio-helper.js` lines 163–170
- **Status**: Pending
- **Details**: JS `detectFileType(data)` takes a BufferWrapper and calls `data.startsWith(...)` which accepts arrays of prefixes. C++ takes `(const uint8_t* data, size_t size)` — raw pointer and length. Functionally equivalent but different calling convention.

- [ ] 133. [audio-helper.cpp] play() checks !engine in addition to empty data
- **JS Source**: `src/js/ui/audio-helper.js` lines 43–44
- **Status**: Pending
- **Details**: JS `play()` only checks `if (!this.buffer)`. C++ checks `if (audio_data.empty() || !engine)`. If JS `init()` was never called, `this.context.createBufferSource()` would throw at runtime. C++ handles this gracefully with the extra guard — more defensive but different error behavior.

- [ ] 134. [wowhead.cpp] Parse result field name differs from JS API (`class` vs `player_class`)
- **JS Source**: `src/js/wowhead.js` lines 172, 226
- **Status**: Pending
- **Details**: JS returns `class` in parsed output objects; C++ stores this value in `ParseResult::player_class`, changing the exported result shape.

- [ ] 135. [xxhash64.cpp] Public API contract differs from JS callable-export behavior
- **JS Source**: `src/js/hashing/xxhash64.js` lines 64–75, 286–288
- **Status**: Pending
- **Details**: JS exports a callable function that doubles as constructor/state prototype (`module.exports = XXH64`), while C++ exposes a class/static-method API only, changing the original module’s call surface semantics.

- [ ] 136. [casc-source.cpp] `getFileByName` no longer forwards to subclass file-reader path like JS
- **JS Source**: `src/js/casc/casc-source.js` lines 169–191
- **Status**: Pending
- **Details**: JS `getFileByName(...)` forwards all read flags to polymorphic `this.getFile(...)` on subclass implementations; C++ `CASC::getFileByName(...)` returns only the encoding key from base `getFile`, changing behavior and API contract.

- [ ] 137. [casc-source.cpp] Base CASC APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/casc/casc-source.js` lines 70–275, 312–444
- **Status**: Pending
- **Details**: JS methods (`getInstallManifest/getFile/getFileEncodingInfo/getFileByName/getVirtualFileByID/getVirtualFileByName/prepareListfile/prepareDBDManifest/loadListfile/parseRootFile/parseEncodingFile`) are Promise-based; C++ equivalents are synchronous.

- [ ] 138. [casc-source-remote.cpp] Remote CASC lifecycle and data-access methods are synchronous instead of JS async methods
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 37–556
- **Status**: Pending
- **Details**: JS uses async Promise-based control flow for initialization/config/archive loading and file access (`init/getVersionConfig/getConfig/getCDNConfig/getFile/getFileStream/preload/load/loadEncoding/loadRoot/loadArchives/loadServerConfig/parseArchiveIndex/getDataFile/getDataFilePartial/loadConfigs/resolveCDNHost/_ensureFileInCache/getFileEncodingInfo`); C++ is synchronous.

- [ ] 139. [casc-source-remote.cpp] HTTP error detail from remote config requests differs from JS
- **JS Source**: `src/js/casc/casc-source-remote.js` lines 75–79, 98–102, 121
- **Status**: Pending
- **Details**: JS includes HTTP status codes in thrown messages for `getConfig/getCDNConfig`; C++ checks only for empty `generics::get()` payload and throws generic HTTP error strings without the JS status payload.

- [ ] 140. [casc-source-local.cpp] Local CASC public/file-loading methods are synchronous instead of JS async methods
- **JS Source**: `src/js/casc/casc-source-local.js` lines 42–517
- **Status**: Pending
- **Details**: JS methods (`init/getFile/getFileStream/load/loadConfigs/loadIndexes/parseIndex/loadEncoding/loadRoot/initializeRemoteCASC/getDataFileWithRemoteFallback/getDataFile/_ensureFileInCache/getFileEncodingInfo`) are Promise-based; C++ equivalents are synchronous.

- [ ] 141. [casc-source-local.cpp] Remote CASC initialization region fallback differs from JS behavior
- **JS Source**: `src/js/casc/casc-source-local.js` lines 324–332
- **Status**: Pending
- **Details**: JS directly constructs `new CASCRemote(core.view.selectedCDNRegion.tag)`; C++ silently falls back to `constants::PATCH::DEFAULT_REGION` when `selectedCDNRegion.tag` is missing, altering failure/selection behavior.

- [ ] 142. [casc-source-local.cpp] `getProductList()` handles missing Branch field gracefully instead of throwing like JS
- **JS Source**: `src/js/casc/casc-source-local.js` line 152
- **Status**: Pending
- **Details**: JS `entry.Branch.toUpperCase()` accesses `entry.Branch` directly. If the build info entry has no `Branch` field, JS would access `undefined` and `.toUpperCase()` would throw a TypeError, causing that product to fail to be listed. C++ (lines 223–229) checks `branchIt != entry.end()` first and uses an empty string as the fallback. This means C++ would include the product with empty parentheses in the label (e.g., `"Retail () 10.2.7"`), while JS would crash and omit it (or propagate the error). In practice, the Branch field is always present in `.build.info` files, so this is a theoretical difference.

- [ ] 143. [cdn-resolver.cpp] Resolver API and internal host-resolution flow are synchronous instead of JS async Promise flow
- **JS Source**: `src/js/casc/cdn-resolver.js` lines 43–116, 143–217
- **Status**: Pending
- **Details**: JS `getBestHost/getRankedHosts/_resolveRegionProduct/_resolveHosts` are async and await Promise pipelines; C++ resolves through blocking waits/futures and synchronous return APIs.

- [ ] 144. [version-config.cpp] Extra data fields beyond header count silently discarded instead of creating JS `undefined` key
- **JS Source**: `src/js/casc/version-config.js` lines 26–29
- **Status**: Pending
- **Details**: JS iterates `entryFields.length` (the pipe-split data values) which may exceed `fields.length` (the header field count). When there are more data values than header fields, JS creates node entries with key `"undefined"` (since `fields[i]` is `undefined` for excess indices). C++ (line 88) loops `while (pos <= entry.size() && fi < fields.size())`, stopping at the header field count and silently discarding extra data values. In practice, WoW CASC version configs always have matching field counts, but the edge-case behavior differs.

- [ ] 145. [realmlist.cpp] `load` API is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/casc/realmlist.js` lines 36–65
- **Status**: Pending
- **Details**: JS exposes `async load()` with awaited cache/network I/O; C++ ports `load()` as synchronous blocking flow, changing timing/error propagation semantics.

- [ ] 146. [realmlist.cpp] `realmListURL` coercion semantics differ from JS `String(...)` behavior
- **JS Source**: `src/js/casc/realmlist.js` lines 39–42
- **Status**: Pending
- **Details**: JS converts any value with `String(core.view.config.realmListURL)` (including `undefined`), while C++ treats missing/null as empty and throws `Missing/malformed realmListURL`, changing edge-case behavior.

- [ ] 147. [realmlist.cpp] Remote non-OK handling/logging path differs from JS response contract
- **JS Source**: `src/js/casc/realmlist.js` lines 51–63
- **Status**: Pending
- **Details**: JS branches on `res.ok` and logs `Failed to retrieve ... (${res.status})` for non-OK responses; C++ uses byte-returning `generics::get()` and exception-based failure handling, removing explicit JS `res.ok/res.status` behavior.

- [ ] 148. [blte-reader.cpp] `decodeAudio(context)` API from JS is missing
- **JS Source**: `src/js/casc/blte-reader.js` lines 337–340
- **Status**: Pending
- **Details**: JS exposes `async decodeAudio(context)` after block processing. C++ removes this method entirely, so the sibling port is missing a public API/code path present in the original module.

- [ ] 149. [blte-reader.cpp] `getDataURL()` no longer honors pre-populated `dataURL` short-circuit
- **JS Source**: `src/js/casc/blte-reader.js` lines 346–353
- **Status**: Pending
- **Details**: JS returns existing `this.dataURL` without forcing `processAllBlocks()`. C++ always processes blocks before delegating to `BufferWrapper::getDataURL()`, changing caching/override behavior.

- [ ] 150. [blte-reader.cpp] `_handleBlock` encrypted block catch-all swallows all non-EncryptionError exceptions silently
- **JS Source**: `src/js/casc/blte-reader.js` lines 203–216
- **Status**: Pending
- **Details**: JS encrypted block handler catches `EncryptionError` specifically and re-throws for other exceptions (the catch block only handles `e instanceof EncryptionError`). C++ has `catch (const EncryptionError&)` for encryption errors, but also has a bare `catch (...)` on line 197–198 that silently swallows all other exceptions. This means C++ silently ignores errors like decompression failures inside encrypted blocks, while JS would propagate them.

- [ ] 151. [blte-reader.cpp] `decodeAudio()` not ported — browser-specific Web Audio API
- **JS Source**: `src/js/casc/blte-reader.js` lines 337–340
- **Status**: Pending
- **Details**: JS `decodeAudio(context)` calls `this.processAllBlocks()` then `super.decodeAudio(context)` using the Web Audio API's `AudioContext.decodeAudioData()`. C++ has a comment (lines 279–281) noting this is browser-specific and uses miniaudio instead. The method is not implemented in C++ BLTEReader.

- [ ] 152. [blte-reader.cpp] `getDataURL()` caching differs — JS checks `this.dataURL` first, C++ always processes blocks
- **JS Source**: `src/js/casc/blte-reader.js` lines 346–353
- **Status**: Pending
- **Details**: JS `getDataURL()` checks `if (!this.dataURL)` first, and only processes blocks if no cached value exists. The `dataURL` property could be set externally. C++ always calls `processAllBlocks()` first, relying on `BufferWrapper::getDataURL()` for internal caching. If an external caller sets `dataURL` before calling `getDataURL()`, JS would return the externally-set value without processing blocks, while C++ always processes blocks first.

- [ ] 153. [blte-reader.cpp] `_decompressBlock` passes two bools to `readBuffer()` in JS but only one in C++
- **JS Source**: `src/js/casc/blte-reader.js` line 242
- **Status**: Pending
- **Details**: JS: `data.readBuffer(blockEnd - data.offset, true, true)` — passes two `true` flags (decompress=true, copy=true). C++ line 220: `data.readBuffer(blockEnd - data.offset(), true)` — passes only one `true` flag (decompress=true). The second flag in JS may control whether the data is copied. If C++'s `readBuffer` implementation doesn't need a copy flag (e.g., always copies), this is functionally equivalent. Otherwise, there could be a difference in memory ownership.

- [ ] 154. [blte-stream-reader.cpp] Block retrieval/decode flow is synchronous instead of JS async
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 54–118
- **Status**: Pending
- **Details**: JS defines `async getBlock` and `async _decodeBlock` and awaits async `blockFetcher`. C++ changes these paths to synchronous calls, altering control flow and error timing.

- [ ] 155. [blte-stream-reader.cpp] `createReadableStream()` Web Streams API path is missing
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 168–193
- **Status**: Pending
- **Details**: JS provides `createReadableStream()` for progressive consumption and cancellation behavior. C++ has no equivalent method, leaving the stream-based event handler/code path unported.

- [ ] 156. [blte-stream-reader.cpp] `streamBlocks` and `createBlobURL` behavior differs from JS
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 199–218
- **Status**: Pending
- **Details**: JS uses an async generator for `streamBlocks()` and returns an object URL from `createBlobURL()` via `BlobPolyfill/URLPolyfill`. C++ uses eager callback iteration and returns concatenated raw bytes (`BufferWrapper`) instead of a blob URL string.

- [ ] 157. [blte-stream-reader.cpp] `createReadableStream()` not ported — Web Streams API specific
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 168–193
- **Status**: Pending
- **Details**: JS `createReadableStream()` returns a `ReadableStream` (Web Streams API) that progressively pulls blocks on demand and supports cancellation. This is browser-specific and has no direct C++ equivalent. The C++ header documents this deviation. The `streamBlocks()` callback-based approach provides similar functionality.

- [ ] 158. [blte-stream-reader.cpp] `streamBlocks()` changed from async generator to synchronous callback
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 199–202
- **Status**: Pending
- **Details**: JS `async *streamBlocks()` is an async generator that yields decoded blocks lazily. Consumers use `for await (const block of reader.streamBlocks())`. C++ `streamBlocks()` takes a callback `std::function<void(BufferWrapper&)>` and iterates eagerly through all blocks, invoking the callback for each. This changes the consumption pattern from lazy to eager.

- [ ] 159. [blte-stream-reader.cpp] `createBlobURL()` returns BufferWrapper instead of string URL
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 208–218
- **Status**: Pending
- **Details**: JS `createBlobURL()` creates a `Blob` with MIME type `'video/x-msvideo'` from all decoded blocks, then returns a URL string via `URLPolyfill.createObjectURL(blob)`. C++ `createBlobURL()` concatenates all decoded blocks into a single `BufferWrapper` and returns it (raw data, no URL, no MIME type). This is a significant API difference — callers expecting a URL string will not work with the C++ version.

- [ ] 160. [blte-stream-reader.cpp] Cache eviction uses `std::deque` for FIFO ordering vs JS `Map` insertion order
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 71–77
- **Status**: Pending
- **Details**: JS uses `Map.keys().next().value` to get the oldest entry (Maps maintain insertion order in JS). C++ uses a separate `std::deque<size_t> cacheOrder` alongside `std::unordered_map` because `std::unordered_map` doesn't maintain insertion order. Functionally equivalent LRU eviction.

- [ ] 161. [blte-stream-reader.cpp] `_decodeBlock` for compressed blocks passes one bool in C++ vs two in JS
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 109–110
- **Status**: Pending
- **Details**: JS: `blockData.readBuffer(blockData.remainingBytes, true, true)` passes two `true` args to `readBuffer`. C++ line 75: `blockData.readBuffer(blockData.remainingBytes(), true)` passes only one. Same issue as entry 449 — the second bool flag for copy behavior may or may not be needed depending on BufferWrapper implementation.

- [ ] 162. [build-cache.cpp] Build cache APIs are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/casc/build-cache.js` lines 49–152, 174–257
- **Status**: Pending
- **Details**: JS uses async methods (`init/getFile/storeFile/saveCacheIntegrity/saveManifest`) and async event handlers with awaited I/O; C++ runs equivalent flows synchronously, changing timing/error propagation behavior.

- [ ] 163. [build-cache.cpp] Cache cleanup size subtraction behavior differs from JS
- **JS Source**: `src/js/casc/build-cache.js` lines 247–254
- **Status**: Pending
- **Details**: JS always performs `deleteSize -= manifestSize` (can go negative with Number); C++ adds an unsigned underflow guard before subtraction, changing edge-case cache-size accounting semantics.

- [ ] 164. [build-cache.cpp] `saveCacheIntegrity()` silently ignores file write failures
- **JS Source**: `src/js/casc/build-cache.js` line 144
- **Status**: Pending
- **Details**: JS `await fsp.writeFile(constants.CACHE.INTEGRITY_FILE, JSON.stringify(cacheIntegrity), 'utf8')` throws if the file cannot be written (e.g., disk full, permission denied). C++ `saveCacheIntegrity()` (line 149–153) checks `ofs.is_open()` and silently does nothing if the file cannot be opened. This means integrity data could be lost without any error or log message in C++, whereas JS would propagate the error to the caller.

- [ ] 165. [cache-collector.cpp] Hand-rolled MD5 and SHA256 instead of using mbedTLS
- **JS Source**: `src/js/workers/cache-collector.js` lines 5–10
- **Status**: Pending
- **Details**: JS uses Node.js `crypto.createHash('md5')` and `crypto.createHash('sha256')`. C++ implements MD5 and SHA256 from scratch in `md5_impl` and `sha256_impl` namespaces (~200 lines each). Project convention specifies mbedTLS (`mbedtls/md.h`) for crypto hashing. Hand-rolled implementations increase maintenance burden and risk of subtle correctness bugs.

- [ ] 166. [cache-collector.cpp] upload_chunks converts binary multipart body through std::string
- **JS Source**: `src/js/workers/cache-collector.js` lines 95–115
- **Status**: Pending
- **Details**: JS handles multipart body as Buffer objects preserving binary integrity. C++ converts `std::vector<uint8_t>` to `std::string(body_bytes.begin(), body_bytes.end())` before passing to `https_request`. While `std::string` can hold embedded null bytes, this conversion path is fragile — httplib's content_type/body API must handle binary strings correctly or data may be corrupted.

- [ ] 167. [cache-collector.cpp] random_hex uses std::mt19937 instead of cryptographically secure random
- **JS Source**: `src/js/workers/cache-collector.js` lines 85–90
- **Status**: Pending
- **Details**: JS uses `crypto.randomBytes(16).toString('hex')` which is cryptographically secure. C++ uses `std::random_device` + `std::mt19937` which is NOT guaranteed to be cryptographic on all platforms (MSVC's `std::random_device` uses CryptGenRandom but GCC/Linux may use `/dev/urandom` or a PRNG). Used only for multipart boundary generation so security impact is minimal, but it deviates from JS behavior.

- [ ] 168. [listfile.cpp] Public listfile APIs are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/casc/listfile.js` lines 478–500, 603–620, 710–756
- **Status**: Pending
- **Details**: JS exposes async `preload`, `prepareListfile`, `loadUnknownTextures`, `loadUnknownModels`, `loadUnknowns`, and `renderListfile`; C++ ports these as synchronous/blocking methods.

- [ ] 169. [listfile.cpp] Shared preload promise semantics differ from JS
- **JS Source**: `src/js/casc/listfile.js` lines 478–500
- **Status**: Pending
- **Details**: JS stores and returns `preload_promise` so all callers can await the same Promise result; C++ uses `void` entrypoints with internal future state and does not expose equivalent awaitable API behavior.

- [ ] 170. [listfile.cpp] `applyPreload` return contract differs from JS
- **JS Source**: `src/js/casc/listfile.js` lines 528–532, 591–601
- **Status**: Pending
- **Details**: JS returns `0` in fallback/no-match paths and otherwise returns `undefined`; C++ changes this API to `void`, removing JS return-value semantics.

- [ ] 171. [listfile.cpp] `getByID` not-found sentinel differs from JS
- **JS Source**: `src/js/casc/listfile.js` lines 778–794
- **Status**: Pending
- **Details**: JS returns `undefined` when ID lookup fails; C++ returns empty string, changing not-found representation and call-site semantics.

- [ ] 172. [listfile.cpp] `getFilteredEntries` search contract differs from JS
- **JS Source**: `src/js/casc/listfile.js` lines 832–857
- **Status**: Pending
- **Details**: JS auto-detects regex via `search instanceof RegExp` and propagates regex errors; C++ requires explicit `is_regex` flag and swallows invalid regex by returning empty results.

- [ ] 173. [listfile.cpp] Binary preload list ordering can differ from JS Map insertion order
- **JS Source**: `src/js/casc/listfile.js` lines 563–588
- **Status**: Pending
- **Details**: JS iterates `Map` keys in insertion order when building filtered preloaded lists; C++ iterates `std::unordered_map` in non-deterministic hash order, which can reorder displayed list entries.

- [ ] 174. [tact-keys.cpp] Tact key lifecycle APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/casc/tact-keys.js` lines 65–137
- **Status**: Pending
- **Details**: JS `load`, `save`, and `doSave` are Promise-based/async; C++ ports to synchronous methods and immediate file I/O.

- [ ] 175. [tact-keys.cpp] Save scheduling differs from JS `setImmediate` batching behavior
- **JS Source**: `src/js/casc/tact-keys.js` lines 122–135
- **Status**: Pending
- **Details**: JS coalesces multiple save requests into a next-tick `setImmediate(doSave)` write; C++ runs `doSave()` immediately, changing batching/timing semantics.

- [ ] 176. [tact-keys.cpp] Remote update error contract differs from JS HTTP status error path
- **JS Source**: `src/js/casc/tact-keys.js` lines 89–93
- **Status**: Pending
- **Details**: JS throws `Unable to update tactKeys, HTTP ${res.status}` when response is non-OK; C++ throws a generic `Unable to update tactKeys: ...` message from caught exceptions without preserving JS status-based error contract.

- [ ] 177. [tact-keys.cpp] `getKey` returns empty string instead of JS `undefined` when key not found
- **JS Source**: `src/js/casc/tact-keys.js` lines 19–21
- **Status**: Pending
- **Details**: JS `getKey` returns `KEY_RING[keyName.toLowerCase()]` which evaluates to `undefined` when the key is not in the object. C++ `getKey` (line 130) returns `{}` (empty string) when not found. Callers that compare the result against `undefined` (JS) vs checking `.empty()` (C++) should be functionally equivalent, but the sentinel value difference could cause issues if any code path checks for empty string vs non-existent key, or if a key legitimately has an empty value (not possible for TACT keys but differs in contract).

- [ ] 178. [content-flags.cpp] Sibling `.cpp` translation unit does not contain line-by-line ported JS constant exports
- **JS Source**: `src/js/casc/content-flags.js` lines 4–15
- **Status**: Pending
- **Details**: JS sibling exports all content-flag constants in the module body, while `content-flags.cpp` only includes the header and comments; parity exists only via header constants, not in the sibling `.cpp` implementation.

- [ ] 179. [locale-flags.cpp] Sibling `.cpp` translation unit does not contain line-by-line JS exports
- **JS Source**: `src/js/casc/locale-flags.js` lines 4–40
- **Status**: Pending
- **Details**: JS sibling exports `flags` and `names` objects in module body, while `locale-flags.cpp` only includes the header and comments; parity lives in header constants rather than `.cpp` implementation.

- [ ] 180. [mpq.cpp] console.error replaced by logging::write for decompression errors
- **JS Source**: `src/js/mpq/mpq.js` line 422
- **Status**: Pending
- **Details**: JS uses `console.error('decompression error:', e)` in inflateData catch. C++ uses `logging::write` which goes to the log file rather than stderr. Different log destination.

- [ ] 181. [mpq-install.cpp] _scan_mpq_files sorts entire accumulated vector at every recursion depth
- **JS Source**: `src/js/mpq/mpq-install.js` lines 25–41
- **Status**: Pending
- **Details**: In JS, each recursive call returns a separate local `results` array that is sorted and returned. In C++, all recursion levels share the same `results` vector (passed by reference), and `std::sort` is called at every depth, redundantly re-sorting all previously added entries. Final result is identical but differs in performance characteristics.

- [ ] 182. [mpq-install.cpp] Archive push ordering differs from JS
- **JS Source**: `src/js/mpq/mpq-install.js` lines 56–75
- **Status**: Pending
- **Details**: JS pushes the archive into `this.archives` first, then iterates files and stores `archive_index: this.archives.length - 1`. C++ iterates files first using `archives.size()` as the index, then pushes the archive. Both produce the correct index, but code structure differs — if future code is added between listfile population and archive push, the C++ ordering could break.

- [ ] 183. [bzip2.cpp] Dead code branch in StrangeCRC::update for negative index
- **JS Source**: `src/js/mpq/bzip2.js` line 114
- **Status**: Pending
- **Details**: In JS, `this.globalCrc` is stored as signed 32-bit via `| 0`, so `(this.globalCrc >> 24)` uses signed right shift and CAN produce negative values, making the `if (index < 0)` branch reachable. In C++, `globalCrc` is `uint32_t`, so `globalCrc >> 24` always yields 0–255 and the `if (index < 0)` branch is dead code. Not a correctness bug but dead code diverging from JS control flow.

- [ ] 184. [bzip2.cpp] Missing default parameters in updateBuffer
- **JS Source**: `src/js/mpq/bzip2.js` line 121
- **Status**: Pending
- **Details**: JS declares `updateBuffer(buf, off = 0, len = buf.length)` with default values for `off` and `len`. C++ declares `void updateBuffer(const uint8_t* buf, int off, int len)` without defaults. Not currently a runtime bug since `updateBuffer` is only called internally with all three args, but the API surface doesn't match the JS.

- [ ] 185. [CompressionType.cpp] Sibling `.cpp` translation unit does not contain line-by-line JS exports
- **JS Source**: `src/js/db/CompressionType.js` lines 1–8
- **Status**: Pending
- **Details**: JS sibling exports compression constants in module body, while `CompressionType.cpp` only includes the header and comments; implementation parity is header-only, not in the `.cpp` sibling.

- [ ] 186. [CompressionType.cpp] Fully ported — no issues found
- **JS Source**: `src/js/db/CompressionType.js` lines 1–8
- **Status**: Pending
- **Details**: JS defines 6 compression type constants (None=0 through BitpackedSigned=5) as a plain object export. C++ defines them as `enum CompressionType : uint32_t` in the header with matching values. The .cpp file is a placeholder that includes the header. All values match exactly. No deviations found.

- [ ] 187. [db2.cpp] `getRelationRows` preload-guard error semantics differ from JS proxy behavior
- **JS Source**: `src/js/casc/db2.js` lines 45–53
- **Status**: Pending
- **Details**: JS proxy throws explicit errors if `getRelationRows` is called before parse/preload; C++ delegates to `WDCReader::getRelationRows()` behavior (commented as returning empty), so the JS error path is not preserved.

- [ ] 188. [db2.cpp] JS async proxy call model is replaced with synchronous parse-once wrappers
- **JS Source**: `src/js/casc/db2.js` lines 58–67, 75–92
- **Status**: Pending
- **Details**: JS wraps reader methods in async proxy handlers and memoized parse promises; C++ uses synchronous `std::call_once` parse guards with direct `WDCReader&` access, changing call timing and API behavior.

- [ ] 189. [db2.cpp] Extra `clearCache()` function added that does not exist in original JS module
- **JS Source**: `src/js/casc/db2.js` (entire file)
- **Status**: Pending
- **Details**: C++ `db2::clearCache()` (line 85–87 in db2.cpp, declared in db2.h line 58) clears the entire table cache, releasing all WDCReader instances. The original JS module exports only `db2_proxy` (the Proxy object) with its `.preload` property — there is no `clearCache` or equivalent cleanup function. This is additional API surface not present in the JS source.

- [ ] 190. [dbd-manifest.cpp] JS truthiness filter for manifest entries is not preserved for object/array values
- **JS Source**: `src/js/casc/dbd-manifest.js` lines 30–34
- **Status**: Pending
- **Details**: JS `if (entry.tableName && entry.db2FileDataID)` treats objects/arrays as truthy; C++ truthiness logic only handles string/number/bool and treats other JSON value types as false, diverging from JS semantics.

- [ ] 191. [DBDParser.cpp] Empty-chunk parsing behavior differs from original JS control flow
- **JS Source**: `src/js/db/DBDParser.js` lines 215–223, 237–320
- **Status**: Pending
- **Details**: JS `parseChunk` path can process empty chunks produced by blank-line splits, while C++ returns early on `chunk.empty()`, changing edge-case entry creation behavior.

- [ ] 192. [DBDParser.cpp] `PATTERN_BUILD_ID` regex uses `.` instead of `\.` for literal dots
- **JS Source**: `src/js/db/DBDParser.js` line 48
- **Status**: Pending
- **Details**: JS `PATTERN_BUILD_ID = /(\d+).(\d+).(\d+).(\d+)/` uses unescaped `.` which matches any character (not just literal dots). C++ `PATTERN_BUILD_ID(R"((\d+).(\d+).(\d+).(\d+))")` (line 51) also uses unescaped `.`. Both have the same bug — `1a2b3c4d` would match when it shouldn't. However since both JS and C++ have the same regex, behavior is identical.

- [ ] 193. [DBDParser.cpp] `isBuildInRange` comparison logic has known bug matching JS — compares each component independently
- **JS Source**: `src/js/db/DBDParser.js` lines 76–94
- **Status**: Pending
- **Details**: Both JS and C++ `isBuildInRange` compare major/minor/patch/rev independently (e.g., `build.minor < min.minor || build.minor > max.minor`). This is incorrect for ranges like `1.5.0.0 - 2.3.0.0` — build `1.8.0.0` would fail because `minor 8 > max.minor 3`. A correct implementation would compare tuples lexicographically. Both versions have the same bug, so behavior is identical.

- [ ] 194. [DBDParser.cpp] `isValidFor` checks empty layoutHash against `layoutHashes` set — could match unintended entries
- **JS Source**: `src/js/db/DBDParser.js` lines 161–177
- **Status**: Pending
- **Details**: JS `isValidFor` checks `this.layoutHashes.has(layoutHash)` — if `layoutHash` is `null` (which it is when called from DBCReader line 182), `Set.has(null)` returns false unless null was explicitly added. C++ `isValidFor` checks `layoutHashes.count(layoutHash)` — if `layoutHash` is an empty string `""`, and if any DBD entry has an empty string in its layoutHashes set, it would incorrectly match. The C++ call from DBCReader line 194 passes `""` (empty string) which should be safe since no valid layout hash is empty, but it's a subtle behavioral difference from JS null.

- [ ] 195. [DBCReader.cpp] Public load path is synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/db/DBCReader.js` lines 162–209, 244–279
- **Status**: Pending
- **Details**: JS `loadSchema()`/`parse()` are async and awaited. C++ ports these flows synchronously, changing timing and error propagation semantics.

- [ ] 196. [DBCReader.cpp] DBD cache backend behavior differs from JS CASC cache API
- **JS Source**: `src/js/db/DBCReader.js` lines 177–195
- **Status**: Pending
- **Details**: JS reads/writes DBD cache via `core.view.casc?.cache.getFile/storeFile`. C++ uses direct filesystem cache paths and bypasses the JS cache interface behavior.

- [ ] 197. [DBCReader.cpp] `loadSchema` is synchronous in C++ but `async` in JS — caching uses filesystem instead of CASC cache
- **JS Source**: `src/js/db/DBCReader.js` lines 162–209
- **Status**: Pending
- **Details**: JS `loadSchema` is `async` and uses `core.view.casc?.cache.getFile()` / `cache.storeFile()` for DBD caching via the CASC cache system. C++ `loadSchema` (lines 156–236) is synchronous, uses `std::filesystem::exists` / `BufferWrapper::readFile` / `writeToFile` for filesystem-based caching. The C++ approach bypasses the CASC cache entirely — if the CASC cache has different behavior (e.g., automatic cleanup, shared across sessions), the C++ version won't benefit from it.

- [ ] 198. [DBCReader.cpp] `parse` is synchronous in C++ but `async` in JS
- **JS Source**: `src/js/db/DBCReader.js` line 244
- **Status**: Pending
- **Details**: JS `parse` is declared `async` (line 244) because it calls `await this.loadSchema()`. C++ `parse` (line 275) is synchronous. This means that in JS, DBC parsing yields to the event loop during schema loading (allowing UI updates), while C++ blocks the calling thread until parsing completes. For large tables or slow network downloads, this could freeze the UI.

- [ ] 199. [DBCReader.cpp] `getRow` returns `std::optional<DataRecord>` but JS returns the row directly or `undefined`
- **JS Source**: `src/js/db/DBCReader.js` lines 114–121
- **Status**: Pending
- **Details**: JS `getRow` returns `this.rows.get(index)` which returns `undefined` if the key doesn't exist in the Map. C++ (line 105) returns `std::nullopt` if the key doesn't exist. This is semantically equivalent, but callers must handle `std::optional` correctly. Also, JS uses `Map.get(index)` which does key lookup, while C++ uses `rows.value().at(static_cast<uint32_t>(index))` which uses `std::map::at` — both are O(log n) lookups.

- [ ] 200. [DBCReader.cpp] `getAllRows` returns `std::map` (sorted by key) but JS returns `Map` (insertion order)
- **JS Source**: `src/js/db/DBCReader.js` lines 127–143
- **Status**: Pending
- **Details**: JS `getAllRows` returns a `new Map()` which preserves insertion order (rows are inserted in index order 0..N). C++ returns `std::map<uint32_t, DataRecord>` which sorts by key. Since the ID is `record.ID ?? i`, the order could differ if record IDs are not sequential. If iteration order matters to callers, this could cause behavioral differences.

- [ ] 201. [DBCReader.cpp] Int64/UInt64 field types read as 32-bit values — JS also reads 32-bit for DBC
- **JS Source**: `src/js/db/DBCReader.js` lines 389–408
- **Status**: Pending
- **Details**: JS `_read_field` handles `FieldType.Int64` and `FieldType.UInt64` but the switch cases only exist for 8/16/32-bit types and Float — there are no Int64/UInt64 cases. The default reads `readUInt32LE()`. C++ `_read_field` (lines 458–479) also lacks Int64/UInt64 cases and falls through to the default `readUInt32LE()`. Both are equivalent, but 64-bit DBC fields would be read incorrectly (only lower 32 bits). This matches the JS behavior but is worth documenting.

- [ ] 202. [WDCReader.cpp] Public schema/data loading APIs are synchronous instead of JS async Promise flow
- **JS Source**: `src/js/db/WDCReader.js` lines 240–277, 303–564
- **Status**: Pending
- **Details**: JS `loadSchema(...)` and `parse()` are async and await cache/network/CASC operations; C++ ports both as blocking synchronous methods, changing timing and error propagation behavior.

- [ ] 203. [WDCReader.cpp] DBD cache access path bypasses JS CASC cache API contract
- **JS Source**: `src/js/db/WDCReader.js` lines 251–266
- **Status**: Pending
- **Details**: JS uses `casc.cache.getFile/storeFile` for DBD cache reads/writes; C++ directly reads/writes filesystem paths under `constants::CACHE::DIR_DBD()`, changing cache backend behavior and integration points.

- [ ] 204. [WDCReader.cpp] Row collection ordering/identity semantics differ from JS Map behavior
- **JS Source**: `src/js/db/WDCReader.js` lines 149–193, 200–208
- **Status**: Pending
- **Details**: JS stores rows in `Map` preserving insertion order and returns the same cached map object after `preload()`. C++ uses `std::map` (key-sorted order) and returns map copies, changing iteration order and object identity/mutability semantics.

- [ ] 205. [WDCReader.cpp] Numeric input coercion from JS `parseInt(...)` is not preserved
- **JS Source**: `src/js/db/WDCReader.js` lines 119–120, 220
- **Status**: Pending
- **Details**: JS coerces `recordID` and `foreignKeyValue` with `parseInt(...)` in `getRow`/`getRelationRows`; C++ requires already-typed integers, so string/loose inputs no longer follow JS coercion behavior.

- [ ] 206. [WDCReader.cpp] `idField` initialized to empty string instead of JS `null` — divergent sentinel before first record read
- **JS Source**: `src/js/db/WDCReader.js` lines 79, 176
- **Status**: Pending
- **Details**: JS initializes `this.idField = null` (line 79). C++ declares `std::string idField;` which default-initializes to `""`. In `getAllRows()` (JS line 176), the fallback `record[this.idField]` with `null` key returns `undefined` in JS, while C++ `record.value().at(idField)` with empty string key throws `std::out_of_range` if no field has an empty name. This only matters for tables with no ID map and no inline ID field, but represents a behavioral divergence in the edge case.

- [ ] 207. [WDCReader.cpp] BigInt arbitrary-precision bit-shift operations vs C++ fixed-width uint64_t — potential UB for large fieldSizeBits
- **JS Source**: `src/js/db/WDCReader.js` lines 816–818
- **Status**: Pending
- **Details**: JS uses BigInt for bitpacked field computation (`1n << BigInt(fieldSizeBits)`) which supports arbitrary precision. C++ uses `1ULL << recordFieldInfo.fieldSizeBits` (line 1048) which produces undefined behavior if `fieldSizeBits >= 64`. While field sizes this large are unlikely in practice, JS would handle them correctly while C++ would produce incorrect results or UB.

- [ ] 208. [FieldType.cpp] Sibling `.cpp` translation unit does not contain line-by-line JS exports
- **JS Source**: `src/js/db/FieldType.js` lines 1–14
- **Status**: Pending
- **Details**: JS sibling exports field-type symbols in module body, while `FieldType.cpp` only includes the header and comments; parity is provided in header enums, not in `.cpp` implementation.

- [ ] 209. [FieldType.cpp] Fully ported — no issues found
- **JS Source**: `src/js/db/FieldType.js` lines 1–13
- **Status**: Pending
- **Details**: JS defines 12 field types using `Symbol()` for unique identity. C++ defines them as `enum class FieldType : uint32_t` in the header with matching names (String, Int8, UInt8, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Relation, NonInlineID). All types are present and correctly mapped. The .cpp file is a placeholder. No deviations found.

- [ ] 210. [DBCharacterCustomization.cpp] Initialization flow is synchronous and drops JS shared-promise waiting behavior
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 37–46
- **Status**: Pending
- **Details**: JS `ensureInitialized` awaits a shared `init_promise` so concurrent callers wait for completion; C++ uses `is_initializing` early-return logic and can return before initialization completes.

- [ ] 211. [DBCharacterCustomization.cpp] Getter not-found contracts differ from JS `Map.get(...)` undefined behavior
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 212–253
- **Status**: Pending
- **Details**: JS getters return `undefined` when keys are missing; C++ ports several getters to `0`, `nullptr`, or `std::nullopt`, changing not-found sentinel behavior expected by JS-style callers.

- [ ] 212. [DBCharacterCustomization.cpp] `chr_cust_mat_map` stores FileDataID=0 for absent tfd_map entries; JS stores `undefined`
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` line 88
- **Status**: Pending
- **Details**: JS `tfd_map.get(mat_row.MaterialResourcesID)` returns `undefined` when key is absent, and this `undefined` is stored as the `FileDataID` property. C++ (line 144–145) uses a fallback of 0 when the key is not found. Later code checking `FileDataID` may behave differently: JS `undefined` is falsy but `!== 0` and `!== undefined` checks distinguish it, while C++ `0` conflates absent entries with genuinely zero-valued entries.

- [ ] 213. [DBCharacterCustomization.cpp] Race/model/option maps use `unordered_map` with no ordering guarantee; JS `Map` preserves insertion order
- **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 95–162, 172–185
- **Status**: Pending
- **Details**: JS `options_by_chr_model`, `chr_race_map`, `chr_race_x_chr_model_map` are `Map` objects that preserve DB2 iteration/insertion order. C++ uses `std::unordered_map` with hash-based ordering that provides no ordering guarantee. When UI code iterates these maps (e.g., to build race selection lists or option panels), iteration order could differ from JS, potentially affecting display ordering unless callers explicitly sort.

- [ ] 214. [DBComponentModelFileData.cpp] Initialization API is synchronous and does not preserve JS promise-sharing semantics
- **JS Source**: `src/js/db/caches/DBComponentModelFileData.js` lines 18–43
- **Status**: Pending
- **Details**: JS `initialize()` is async and returns shared `init_promise` while loading; C++ initialization is synchronous with `is_initializing` early return, so concurrent calls can return before data is ready.

- [ ] 215. [DBComponentTextureFileData.cpp] Initialization API is synchronous and does not preserve JS promise-sharing semantics
- **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js` lines 17–41
- **Status**: Pending
- **Details**: JS `initialize()` is async and returns shared `init_promise`; C++ uses synchronous loading with boolean reentry guards, changing completion/wait behavior for concurrent callers.

- [ ] 216. [DBCreatureDisplayExtra.cpp] Initialization flow is synchronous and does not mirror JS awaited init promise
- **JS Source**: `src/js/db/caches/DBCreatureDisplayExtra.js` lines 15–24, 26–53
- **Status**: Pending
- **Details**: JS `ensureInitialized()` awaits `_initialize()` via `init_promise`; C++ makes `_initialize()` synchronous with `is_initializing` early return, so callers may proceed without JS-equivalent awaited completion semantics.

- [ ] 217. [DBCreatureList.cpp] Public load API is synchronous instead of JS Promise-based async loading
- **JS Source**: `src/js/db/caches/DBCreatureList.js` lines 12–43
- **Status**: Pending
- **Details**: JS `initialize_creature_list` is async and awaits DB2 row retrieval; C++ ports this path as synchronous, changing timing and error-propagation behavior.

- [ ] 218. [DBCreatureList.cpp] `get_all_creatures()` returns `unordered_map` (no order) vs JS `Map` (insertion order)
- **JS Source**: `src/js/db/caches/DBCreatureList.js` lines 9, 45–47
- **Status**: Pending
- **Details**: JS `creatures` is a `Map` preserving insertion order from `Creature.db2` iteration. C++ `creatures` is `std::unordered_map<uint32_t, CreatureEntry>` with hash-based ordering. Code that iterates all creatures (e.g., for UI list display or filtering) may see different ordering, which could affect creature list presentation order in the UI.

- [ ] 219. [DBCreatures.cpp] `initializeCreatureData` is synchronous instead of JS async data-loading flow
- **JS Source**: `src/js/db/caches/DBCreatures.js` lines 17–73
- **Status**: Pending
- **Details**: JS implementation awaits multiple `getAllRows()` calls and exposes Promise timing; C++ performs blocking synchronous table loads, diverging from JS async behavior contract.

- [ ] 220. [DBCreatures.cpp] `creatureDisplays` entries are value copies, not shared references with `creatureDisplayInfoMap`
- **JS Source**: `src/js/db/caches/DBCreatures.js` lines 55–66
- **Status**: Pending
- **Details**: JS line 55 gets a reference to a display object from `creatureDisplayInfoMap`, mutates `extraGeosets` on it (line 58–60), then pushes the same object reference into `creatureDisplays` (line 64). Both maps share the same object identity. C++ line 106 gets a reference to modify `extraGeosets`, but line 115 `push_back(display)` copies the struct into `creatureDisplays`, creating an independent instance. After initialization, JS maps share object identity (mutations visible through either map); C++ maps hold independent copies.

- [ ] 221. [DBCreaturesLegacy.cpp] Model path normalization misses JS `.mdl` to `.m2` conversion
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 45–47
- **Status**: Pending
- **Details**: JS normalizes both `.mdl` and `.mdx` model extensions to `.m2` during model map build; C++ `normalizePath` converts only `.mdx`, so `.mdl` model rows resolve differently.

- [ ] 222. [DBCreaturesLegacy.cpp] Legacy load API is synchronous instead of JS Promise-based async parse flow
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 19–112
- **Status**: Pending
- **Details**: JS uses async initialization and awaits DBC parsing operations; C++ performs synchronous parsing and loading, altering timing/error behavior relative to original Promise flow.

- [ ] 223. [DBCreaturesLegacy.cpp] Exception logging omits JS stack trace output behavior
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 108–111
- **Status**: Pending
- **Details**: JS logs both error message and stack (`log.write('%o', e.stack)`); C++ logs only `e.what()` and drops stack trace output.

- [ ] 224. [DBCreaturesLegacy.cpp] `creatureDisplays` uses `unordered_map` (no order) vs JS `Map` (insertion order)
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 11, 100–103
- **Status**: Pending
- **Details**: JS `creatureDisplays` is a `Map` keyed by lowercase model path that preserves insertion order from DBC iteration. C++ uses `std::unordered_map<std::string, std::vector<LegacyCreatureDisplay>>` with string-hash ordering. Iteration over all creature displays may produce different ordering, though this is less impactful since lookups are typically by specific model path.

- [ ] 225. [DBDecor.cpp] Decor cache initialization is synchronous instead of JS async table load
- **JS Source**: `src/js/db/caches/DBDecor.js` lines 15–40
- **Status**: Pending
- **Details**: JS `initializeDecorData` is async and awaits DB2 reads; C++ uses a synchronous blocking initializer, changing API timing behavior.

- [ ] 226. [DBDecor.cpp] `decorItems` unordered_map iteration order differs from JS `Map` insertion order
- **JS Source**: `src/js/db/caches/DBDecor.js` lines 9, 46–48
- **Status**: Pending
- **Details**: JS `decorItems` is a `Map` preserving insertion order from `HouseDecor.db2` iteration. C++ uses `std::unordered_map<uint32_t, DecorItem>`. Code iterating all decor items (e.g., `getAllDecorItems()` return value or `getDecorItemByModelFileDataID()` linear scan) may encounter items in different order, potentially affecting UI list order for decor items.

- [ ] 227. [DBDecorCategories.cpp] Category cache loading is synchronous and unordered-container iteration differs from JS Map/Set ordering
- **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 10–56
- **Status**: Pending
- **Details**: JS uses async initialization plus `Map`/`Set` insertion order iteration; C++ ports to synchronous initialization with `std::unordered_map`/`std::unordered_set`, which can change iteration ordering and timing semantics.

- [ ] 228. [DBGuildTabard.cpp] Sibling `.cpp` file is still unconverted JavaScript and appears swapped with `.js`
- **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 1–133
- **Status**: Pending
- **Details**: `DBGuildTabard.cpp` contains JS module code (`require`, `module.exports`, async functions), while the sibling `DBGuildTabard.js` contains C++ code, so the `.cpp` translation unit is not actually a C++ line-by-line port of the JS source.

- [ ] 229. [DBGuildTabard.cpp] Color maps use `unordered_map` — iteration order differs from JS `Map` for UI color pickers
- **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 22–25, 75–82
- **Status**: Pending
- **Details**: JS `background_colors`, `border_colors`, and `emblem_colors` are `Map` instances (preserving insertion order from `GuildColorBackground`/`GuildColorBorder`/`GuildColorEmblem` DB2 iteration). C++ `getBackgroundColors()`, `getBorderColors()`, `getEmblemColors()` return `const std::unordered_map<uint32_t, ColorRGB>&` with hash-based ordering. If any caller iterates these maps to build a tabard color picker UI, the color presentation order will differ from the JS original. Note: prior entry 215 stated the .cpp file was still unconverted JS — this is no longer the case; the file is now properly ported to C++.

- [ ] 230. [DBItemCharTextures.cpp] Initialization flow is synchronous and drops JS shared-promise semantics
- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 34–88
- **Status**: Pending
- **Details**: JS uses `init_promise` and async `initialize/ensureInitialized` so concurrent callers await the same in-flight work; C++ uses synchronous initialization with no promise-sharing behavior.

- [ ] 231. [DBItemCharTextures.cpp] Race/gender texture selection fallback differs from JS behavior
- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 129–135
- **Status**: Pending
- **Details**: JS pushes `bestFileDataID` as returned (can be `undefined` when no match), but C++ falls back to the first entry (`value_or((*file_data_ids)[0])`), changing file-data selection behavior.

- [ ] 232. [DBItemCharTextures.cpp] `value_or` fallback in `getTexturesByDisplayId` is redundant (prior entry 217 is inaccurate)
- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 129–135; `src/js/db/caches/DBComponentTextureFileData.js` lines 51–97
- **Status**: Pending
- **Details**: Entry 217 states that C++ `value_or((*file_data_ids)[0])` at line 127 changes file-data selection behavior compared to JS's `fileDataID: bestFileDataID`. However, the JS `getTextureForRaceGender` function (DBComponentTextureFileData.js line 97) already falls back to `file_data_ids[0]` as its final return when no race/gender match is found, and it only returns `null` when the input array is empty — which is already guarded by the `if (file_data_ids && !file_data_ids->empty())` check at line 115 of the C++ code. The `value_or` in C++ therefore never triggers in practice, and the actual behavior is identical to JS. Entry 217 should be reconsidered.

- [ ] 233. [DBItemDisplays.cpp] Item display cache initialization is synchronous instead of JS async flow
- **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 18–53
- **Status**: Pending
- **Details**: JS `initializeItemDisplays` is Promise-based and awaits DB2/cache calls; C++ ports this path as synchronous blocking logic.

- [ ] 234. [DBItemDisplays.cpp] `ItemDisplay::textures` is a deep copy per entry, not a shared reference as in JS
- **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 40–41
- **Status**: Pending
- **Details**: JS line 41 stores `textures: textureFileDataIDs` where `textureFileDataIDs` is the array reference returned by `DBTextureFileData.getTextureFDIDsByMatID(matResIDs[0])`. Multiple `ItemDisplay` objects that share the same `matResIDs[0]` will reference the same textures array in memory. C++ line 85 copies via `display.textures = *textureFileDataIDs`, so each `ItemDisplay` holds an independent vector. This is functionally equivalent since textures are never mutated after initialization, but the memory semantics differ (JS shares, C++ copies).

- [ ] 235. [DBItemGeosets.cpp] Initialization lifecycle is synchronous and omits JS `init_promise` contract
- **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 154–220
- **Status**: Pending
- **Details**: JS uses async initialization with `init_promise` deduplication; C++ uses a synchronous one-shot initializer and cannot preserve awaitable initialization semantics.

- [ ] 236. [DBItemGeosets.cpp] Equipped-items input coercion differs from JS `Object.entries` + `parseInt` behavior
- **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 251–259, 339–345
- **Status**: Pending
- **Details**: JS accepts plain objects keyed by strings and parses slot IDs with `parseInt`; C++ requires `std::unordered_map<int, uint32_t>` inputs, removing JS key-coercion behavior.

- [ ] 237. [DBItemModels.cpp] Item model cache initialization is synchronous instead of JS Promise-based flow
- **JS Source**: `src/js/db/caches/DBItemModels.js` lines 22–103
- **Status**: Pending
- **Details**: JS uses async `initialize` with shared `init_promise` and awaited dependent caches; C++ performs the entire load synchronously with no async/promise contract.

- [ ] 238. [DBItems.cpp] Item cache initialization is synchronous and does not preserve JS shared `init_promise`
- **JS Source**: `src/js/db/caches/DBItems.js` lines 14–59
- **Status**: Pending
- **Details**: JS deduplicates concurrent initialization via `init_promise` and async functions; C++ uses synchronous initialization and lacks equivalent awaitable behavior.

- [ ] 239. [DBItems.cpp] `items_by_id` uses `unordered_map` (hash order) vs JS `Map` (insertion order)
- **JS Source**: `src/js/db/caches/DBItems.js` lines 10, 36–46
- **Status**: Pending
- **Details**: JS `items_by_id` is a `Map` preserving insertion order from `ItemSparse` DB2 iteration. C++ uses `std::unordered_map<uint32_t, ItemInfo>` with hash-based ordering. While current accessors (`getItemById`, `getItemSlotId`, `isItemBow`) only perform key lookups, any future code that iterates all items (e.g., for item list display or filtering) would produce a different ordering than the JS original.

- [ ] 240. [DBModelFileData.cpp] Model mapping loader is synchronous instead of JS async API
- **JS Source**: `src/js/db/caches/DBModelFileData.js` lines 17–35
- **Status**: Pending
- **Details**: JS exposes `initializeModelFileData` as an async Promise-based loader; C++ implementation is synchronous blocking code.

- [ ] 241. [DBNpcEquipment.cpp] NPC equipment cache initialization is synchronous and drops JS `init_promise`
- **JS Source**: `src/js/db/caches/DBNpcEquipment.js` lines 30–66
- **Status**: Pending
- **Details**: JS uses async initialization with in-flight promise reuse; C++ initialization is synchronous and does not retain the JS async concurrency contract.

- [ ] 242. [DBNpcEquipment.cpp] Inner equipment slot map uses `unordered_map` vs JS `Map` (insertion order)
- **JS Source**: `src/js/db/caches/DBNpcEquipment.js` lines 25, 49–52
- **Status**: Pending
- **Details**: JS `equipment_map` maps `CreatureDisplayInfoExtraID -> Map<slot_id, item_display_info_id>` where the inner `Map` preserves insertion order from `NPCModelItemSlotDisplayInfo` DB2 iteration. C++ uses `std::unordered_map<uint32_t, std::unordered_map<int, uint32_t>>` — both levels have hash-based ordering. If a caller iterates equipment slots for a creature (e.g., to process items in slot order for equipping), the iteration order differs from JS.

- [ ] 243. [DBTextureFileData.cpp] Texture mapping loader/ensure APIs are synchronous instead of JS async APIs
- **JS Source**: `src/js/db/caches/DBTextureFileData.js` lines 16–52
- **Status**: Pending
- **Details**: JS defines async `initializeTextureFileData` and `ensureInitialized`; C++ ports both as synchronous methods.

- [ ] 244. [DBTextureFileData.cpp] UsageType remap path remains a TODO placeholder in C++ port
- **JS Source**: `src/js/db/caches/DBTextureFileData.js` line 24
- **Status**: Pending
- **Details**: C++ retains the same `TODO` comment (`Need to remap this to support other UsageTypes`) and still skips non-zero `UsageType`, leaving this path explicitly unfinished.

- [x] 245. [context-menu.cpp] Invisible hover buffer zone (`.context-menu-zone`) is not ported
- **JS Source**: `src/js/components/context-menu.js` lines 54–56
- **Status**: Verified
- **Details**: JS includes a dedicated `.context-menu-zone` element to extend hover bounds around the menu. C++ explicitly omits it, which can change close-on-mouseleave behavior near menu edges.

- [ ] 246. [context-menu.cpp] JS uses `window.innerHeight/innerWidth` but C++ uses `io.DisplaySize` which may differ with multi-viewport
- **JS Source**: `src/js/components/context-menu.js` lines 35–36
- **Status**: Pending
- **Details**: JS `reposition()` compares `positionY > window.innerHeight / 2` and `positionX > window.innerWidth / 2` using the browser window dimensions. C++ (lines 30–31) uses `io.DisplaySize.y / 2.0f` and `io.DisplaySize.x / 2.0f`. With ImGui multi-viewport enabled, `io.DisplaySize` represents the main viewport size, not the actual monitor or display size. If the context menu is triggered from a secondary viewport, the comparison may be incorrect because `io.DisplaySize` doesn't account for multi-viewport positioning.

- [ ] 247. [context-menu.cpp] JS uses `clientMouseX`/`clientMouseY` from global mousemove listener but C++ uses `io.MousePos` at time of activation
- **JS Source**: `src/js/components/context-menu.js` lines 7–14, 33–34
- **Status**: Pending
- **Details**: JS tracks mouse position via a global `window.addEventListener('mousemove', ...)` that updates `clientMouseX`/`clientMouseY` on every mouse move. When `reposition()` is called (on `node` watch, via `$nextTick`), it reads the latest tracked mouse position. C++ (line 28) reads `io.MousePos` which is the current frame's mouse position. In JS, the `$nextTick` delay means the position is read one tick after the node change. In C++, the position is read the same frame the node becomes active. This could cause a subtle positioning difference if the mouse moves between frames.

- [ ] 248. [context-menu.cpp] `mounted()` initial reposition is not ported
- **JS Source**: `src/js/components/context-menu.js` lines 48–52
- **Status**: Pending
- **Details**: JS `mounted()` calls `this.reposition()` to set initial position. C++ does not have a mounted equivalent — the first frame when `nodeActive` transitions to true triggers `reposition()` via the watch logic (line 55), which covers the primary use case. However, if the context menu renders with `node` already truthy on the first frame, JS would have called `reposition()` in both `mounted()` AND the watch handler, while C++ only calls it in the watch handler. The mounted call is a safety net in JS.

- [ ] 249. [context-menu.cpp] CSS `span` items use `padding: 8px`, `border-bottom: 1px solid var(--border)`, `text-overflow: ellipsis` — not enforced by C++ rendering
- **JS Source**: `src/app.css` lines 900–913
- **Status**: Pending
- **Details**: CSS `.context-menu > span` has `padding: 8px`, `border-bottom: 1px solid var(--border)`, `text-overflow: ellipsis`, `white-space: nowrap`, `overflow: hidden`. The last child has `border-bottom: 0`. C++ `contentCallback` renders the menu items but does not enforce these CSS rules — the callback is responsible for rendering, and the context-menu component does not apply per-item padding, separators, or text truncation. This means the visual appearance depends entirely on how callers render items, potentially deviating from the JS styling.

- [ ] 250. [context-menu.cpp] CSS `span:hover` background `#353535` with `cursor: pointer` not enforced on menu items
- **JS Source**: `src/app.css` lines 907–910
- **Status**: Pending
- **Details**: CSS `.context-menu > span:hover` sets `background: #353535; cursor: pointer`. C++ delegates item rendering to the `contentCallback`, which may or may not apply hover highlighting. The context-menu component itself does not apply per-item hover effects, unlike the JS CSS which applies them universally to all `span` children.

- [ ] 251. [menu-button.cpp] Click emit payload drops the original event object
- **JS Source**: `src/js/components/menu-button.js` lines 45–50
- **Status**: Pending
- **Details**: JS emits `click` with the DOM event argument (`this.$emit('click', e)`). C++ exposes `onClick()` with no event payload, changing callback contract.

- [ ] 252. [menu-button.cpp] Context-menu close behavior differs from original component flow
- **JS Source**: `src/js/components/menu-button.js` lines 75–80; `src/js/components/context-menu.js` line 54
- **Status**: Pending
- **Details**: JS menu closes via context-menu `@close` events (mouseleave/click behavior). C++ popup primarily closes on click-outside checks and does not mirror the same close trigger semantics.

- [ ] 253. [menu-button.cpp] Arrow width 20px instead of CSS 29px
- **JS Source**: `src/app.css` lines 1005–1022
- **Status**: Pending
- **Details**: CSS `.ui-menu-button .arrow { width: 29px; }` defines the arrow/caret button as 29px wide. C++ uses `const float arrowWidth = 20.0f` (line 125). The arrow area is 9px narrower than the original. The main button also uses `padding-right: 40px` in CSS (line 958) to reserve space for the arrow overlay, but in C++ the layout is side-by-side so the padding approach differs.

- [ ] 254. [menu-button.cpp] Arrow uses `ICON_FA_CARET_DOWN` text instead of CSS `caret-down.svg` background image
- **JS Source**: `src/app.css` lines 1017–1020
- **Status**: Pending
- **Details**: CSS `.arrow` uses `background-image: url(./fa-icons/caret-down.svg)` with `background-size: 10px` centered. C++ uses `ImGui::Button(ICON_FA_CARET_DOWN, ...)` (line 136). If `ICON_FA_CARET_DOWN` is not defined or not a valid FontAwesome codepoint, the button will show garbled text or nothing. Even if defined, the icon rendering may differ from the SVG.

- [ ] 255. [menu-button.cpp] Arrow missing left border `border-left: 1px solid rgba(255, 255, 255, 0.32)`
- **JS Source**: `src/app.css` line 1021
- **Status**: Pending
- **Details**: CSS `.arrow` has `border-left: 1px solid rgba(255, 255, 255, 0.3215686275)` separating the arrow from the main button. C++ places the arrow button with `ImGui::SameLine(0.0f, 0.0f)` (line 135) with no visual separator. A thin line should be drawn between the main button and the arrow.

- [ ] 256. [menu-button.cpp] Dropdown menu uses ImGui popup window instead of CSS-styled `<ul>` with `--form-button-menu` background
- **JS Source**: `src/app.css` lines 964–994
- **Status**: Pending
- **Details**: CSS `.ui-menu-button .menu` uses `background: var(--form-button-menu)`, `padding: 8px 10px` per item, rounded bottom corners (`border-radius: 5px`), and positions at `top: 85%` with `padding-top: 5%`. C++ uses `ImGui::Begin` with `ImGuiWindowFlags_AlwaysAutoResize` (lines 153–180) which uses ImGui's default popup styling. The background color, item padding, corner rounding, and position offset may all differ from the CSS.

- [ ] 257. [menu-button.cpp] Dropdown menu items missing hover `background: var(--form-button-menu-hover)`
- **JS Source**: `src/app.css` lines 976–978
- **Status**: Pending
- **Details**: CSS `.ui-menu-button .menu li:hover { background: var(--form-button-menu-hover); }` provides a specific hover color. C++ uses `ImGui::Selectable` (line 169) which uses ImGui's default `ImGuiCol_HeaderHovered` color, which may not match `--form-button-menu-hover`.

- [ ] 258. [menu-button.cpp] Menu close behavior uses `IsMouseClicked(0)` outside check instead of JS `@close` mouse-leave
- **JS Source**: `src/js/components/menu-button.js` line 78
- **Status**: Pending
- **Details**: JS `<context-menu>` component closes on mouse-leave (`@close="open = false"`) and also on click-outside. C++ (lines 175–178) only closes on click-outside via `!IsWindowHovered && IsMouseClicked(0)`. If the user moves the mouse away from the menu without clicking, the JS version closes the menu but the C++ version keeps it open.

- [ ] 259. [combobox.cpp] Blur-close timing is frame-based instead of JS 200ms timeout
- **JS Source**: `src/js/components/combobox.js` lines 67–72
- **Status**: Pending
- **Details**: JS uses `setTimeout(..., 200)` for blur-close timing, but C++ uses a fixed 12-frame countdown. The effective delay changes with frame rate, so dropdown close behavior differs from JS.

- [ ] 260. [combobox.cpp] Dropdown menu is rendered in normal layout flow instead of absolute popup overlay
- **JS Source**: `src/js/components/combobox.js` lines 87–93
- **Status**: Pending
- **Details**: JS renders `<ul>` as an absolutely positioned popup under the input. C++ renders the dropdown as an ImGui child region in normal flow, which can alter layout/overlap behavior and visual parity.

- [ ] 261. [combobox.cpp] Dropdown `z-index: 5` and `position: absolute; top: 100%` not replicated
- **JS Source**: `src/js/components/combobox.js` template line 90, `src/app.css` lines 1355–1366
- **Status**: Pending
- **Details**: CSS `.ui-combobox` is `position: relative` and `.ui-combobox ul` has `position: absolute; top: 100%; z-index: 5`. C++ uses `ImGui::BeginChild("##dropdown", ...)` which is an inline child window, not a floating overlay. This means the dropdown may be clipped by the parent window boundaries, doesn't layer over other UI elements, and doesn't position at `top: 100%` of the input field. A proper ImGui equivalent would use a popup or a separate window.

- [ ] 262. [combobox.cpp] Dropdown list does not have `list-style: none` equivalent — no bullet points but uses Selectable
- **JS Source**: `src/app.css` line 1359
- **Status**: Pending
- **Details**: CSS `list-style: none` removes bullet points from the `<ul>`. C++ uses `ImGui::Selectable()` which doesn't have bullets, but the Selectable widget has its own hover highlighting behavior that differs from the CSS `li:hover { background: #353535; cursor: pointer; }`. The C++ pushes `ImGuiCol_HeaderHovered` to match, which is correct for the background color, but the cursor change is not possible in ImGui (ImGui does not support custom cursor-on-hover per widget).

- [ ] 263. [combobox.cpp] Missing `box-shadow: black 0 0 3px 0` on dropdown
- **JS Source**: `src/app.css` line 1363
- **Status**: Pending
- **Details**: CSS `.ui-combobox ul` has `box-shadow: black 0 0 3px 0`. ImGui does not support box-shadow on child windows. The C++ dropdown (line 215–216) uses `ImGui::BeginChild` with borders but no shadow, resulting in a flatter visual appearance compared to the JS version.

- [ ] 264. [combobox.cpp] `filteredSource` uses `startsWith` (JS) but C++ uses `find(...) == 0` which is functionally equivalent but `std::string::starts_with` is available in C++20/23
- **JS Source**: `src/js/components/combobox.js` line 38
- **Status**: Pending
- **Details**: JS uses `item.label.toLowerCase().startsWith(currentTextLower)`. C++ (line 34) uses `labelLower.find(currentTextLower) == 0`. While functionally identical, the C++23 `std::string::starts_with()` method would be cleaner and more idiomatic for the project's C++23 standard. Minor style issue, not a behavioral difference.

- [ ] 265. [slider.cpp] Document-level mouse listener lifecycle from JS is not ported directly
- **JS Source**: `src/js/components/slider.js` lines 23–29, 35–38
- **Status**: Pending
- **Details**: JS installs/removes global `mousemove`/`mouseup` listeners in `mounted`/`beforeUnmount`. C++ handles drag state via ImGui per-frame input polling and has no equivalent listener registration lifecycle.

- [ ] 266. [slider.cpp] Slider fill color uses `SLIDER_FILL_U32` but CSS uses `var(--font-alt)` (#57afe2)
- **JS Source**: `src/app.css` lines 1267–1274
- **Status**: Pending
- **Details**: CSS `.ui-slider .fill { background: var(--font-alt); }` uses `--font-alt` (#57afe2, blue). C++ uses `app::theme::SLIDER_FILL_U32` (line 142). If `SLIDER_FILL_U32` does not map to #57afe2 / `FONT_ALT_U32`, the fill color will differ. Verify that `SLIDER_FILL_U32` matches `FONT_ALT_U32`.

- [ ] 267. [slider.cpp] Slider track background uses `SLIDER_TRACK_U32` but CSS uses `var(--background-dark)` (#2c3136)
- **JS Source**: `src/app.css` lines 1259–1266
- **Status**: Pending
- **Details**: CSS `.ui-slider { background: var(--background-dark); }` uses `--background-dark` (#2c3136). C++ uses `app::theme::SLIDER_TRACK_U32` (line 131). If this constant doesn't map to #2c3136, the track color will differ.

- [ ] 268. [slider.cpp] Handle position uses `left: (modelValue * 100)%` without `translateX(-50%)` centering
- **JS Source**: `src/app.css` lines 1275–1286
- **Status**: Pending
- **Details**: CSS `.handle { left: 50%; top: 50%; transform: translateY(-50%); }` — wait, the template uses `:style="{ left: (modelValue * 100) + '%' }"` which overrides the CSS `left: 50%`. The CSS `transform: translateY(-50%)` only vertically centers. C++ positions handle at `handleX = winPos.x + fillWidth` (line 148) without centering the handle horizontally on the value position. In JS, the handle's left edge is at the value position, so in C++ this is correct. No issue here — CSS comment on line 146 is accurate.

- [ ] 269. [checkboxlist.cpp] Component lifecycle/event model differs from JS mounted/unmount listener flow
- **JS Source**: `src/js/components/checkboxlist.js` lines 28–51, 122–134
- **Status**: Pending
- **Details**: JS registers/removes document-level mouse listeners and a `ResizeObserver`; C++ emulates behavior via per-frame ImGui polling and internal state, not equivalent listener lifecycle semantics.

- [ ] 270. [checkboxlist.cpp] Scroll bound edge-case behavior differs for zero scrollbar range
- **JS Source**: `src/js/components/checkboxlist.js` lines 102–106
- **Status**: Pending
- **Details**: JS sets `scrollRel = this.scroll / max` (allowing `Infinity/NaN` when `max === 0`); C++ clamps to `0.0f` when range is zero, changing parity in that edge case.

- [ ] 271. [checkboxlist.cpp] Scrollbar height behavior differs from original CSS
- **JS Source**: `src/js/components/checkboxlist.js` lines 93–94; `src/app.css` lines 1097–1103
- **Status**: Pending
- **Details**: JS/CSS uses `.scroller` with fixed `height: 45px` and resize math based on that DOM height; C++ computes a dynamic proportional thumb height with `std::max(20.0f, ...)`, producing different visual size/scroll behavior.

- [ ] 272. [checkboxlist.cpp] Scrollbar default styling differs from CSS reference
- **JS Source**: `src/app.css` lines 1106–1114, 1116–1117
- **Status**: Pending
- **Details**: CSS default scrollbar inner color/border uses `var(--border)` and hover uses `var(--font-highlight)`; C++ uses `FONT_PRIMARY` for default thumb color, causing visual mismatch against reference styling.


- [ ] 273. [checkboxlist.cpp] Missing container border and box-shadow from CSS reference
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-checkboxlist` has `border: 1px solid var(--border)` and `box-shadow: black 0 0 3px 0px` providing a bordered, shadowed container. C++ (line 171) uses `ImGui::BeginChild("##checkboxlist_container", ...)` with `ImGuiChildFlags_None` which does not render a border or shadow, producing a visually different container appearance.

- [ ] 274. [checkboxlist.cpp] Missing default item background and CSS padding values
- **JS Source**: `src/app.css` lines 1081–1086
- **Status**: Pending
- **Details**: CSS sets ALL items to `background: var(--background-dark)` with `padding: 2px 8px`. Even-indexed items override with `background: var(--background-alt)`. C++ (lines 241–244) only draws background for even items (`BG_ALT_U32`) and selected items (`FONT_ALT_U32`); odd non-selected items receive no explicit background, inheriting the ImGui child window background instead of the CSS `--background-dark` color. The `2px 8px` padding is not replicated — ImGui uses its default item spacing.

- [ ] 275. [checkboxlist.cpp] Missing item text left margin from CSS `.item span` rule
- **JS Source**: `src/app.css` lines 1065–1068
- **Status**: Pending
- **Details**: CSS `.ui-checkboxlist .item span` has `margin: 0 0 1px 5px` giving the label text 5px left margin and 1px bottom margin relative to the checkbox. C++ (line 260) uses `ImGui::SameLine()` between the checkbox and selectable label with default ImGui spacing (typically 4px item spacing), which may not exactly match the 5px CSS margin. The 1px bottom margin is also not replicated.

- [ ] 276. [listbox.cpp] Keep-alive lifecycle listener behavior (`activated`/`deactivated`) is missing
- **JS Source**: `src/js/components/listbox.js` lines 97–113
- **Status**: Pending
- **Details**: JS conditionally registers/unregisters paste and keydown listeners on keep-alive activation state. C++ has no equivalent lifecycle gating, so keyboard/paste handling differs when component activation changes.

- [ ] 277. [listbox.cpp] Context menu emit payload omits original JS mouse event object
- **JS Source**: `src/js/components/listbox.js` lines 493–497
- **Status**: Pending
- **Details**: JS emits `{ item, selection, event }` including the full event object. C++ emits only simplified coordinates/fields, which drops event data expected by the original contract.

- [ ] 278. [listbox.cpp] Multi-subfield span structure from `item.split('\31')` is flattened
- **JS Source**: `src/js/components/listbox.js` lines 506–508
- **Status**: Pending
- **Details**: JS renders each subfield in separate `<span class="sub sub-N">` elements. C++ concatenates subfields into one display string, removing per-subfield structure and styling parity.

- [ ] 279. [listbox.cpp] `wheelMouse` uses `core.view.config.scrollSpeed` from JS but C++ reads from `core::view->config`
- **JS Source**: `src/js/components/listbox.js` lines 330–336
- **Status**: Pending
- **Details**: JS: `const scrollCount = core.view.config.scrollSpeed === 0 ? Math.floor(this.$refs.root.clientHeight / child.clientHeight) : core.view.config.scrollSpeed`. C++ (lines 216–222) reads `core::view->config.value("scrollSpeed", 0)` using `nlohmann::json::value()`. If `core::view` is null (line 217 checks), it defaults to `scrollSpeed = 0`. The JSON access pattern (`config.value("scrollSpeed", 0)`) differs from the JS direct property access (`config.scrollSpeed`). If the config object uses a different key name or nested structure, the value might not be found.

- [ ] 280. [listbox.cpp] `handlePaste` creates new selection instead of clearing existing and pushing entries
- **JS Source**: `src/js/components/listbox.js` lines 305–318
- **Status**: Pending
- **Details**: JS: `const newSelection = this.selection.slice(); newSelection.splice(0); newSelection.push(...entries);` — creates a copy of current selection, clears it, then pushes clipboard entries. This effectively replaces the selection with clipboard entries. C++ (lines 196–203) creates `entries` vector from clipboard text and calls `onSelectionChanged(entries)` directly. The JS code creates the new array from `selection.slice()` then `splice(0)` to clear — the intermediate copy is unnecessary but the end result is the same: the selection is replaced with the clipboard entries. Functionally equivalent.

- [ ] 281. [listbox.cpp] `activeQuickFilter` toggle logic matches JS but CSS pattern regex differs
- **JS Source**: `src/js/components/listbox.js` lines 213–216
- **Status**: Pending
- **Details**: JS quick filter: `const pattern = new RegExp('\\.${this.activeQuickFilter.toLowerCase()}(\\s\\[\\d+\\])?$', 'i')`. C++ `computeFilteredItems()` should apply the same regex pattern for quick filtering. Need to verify the C++ implements the quick filter regex pattern identically — specifically the `(\s\[\d+\])?$` suffix which handles optional file data ID suffixes like ` [12345]` at end of filenames.

- [ ] 282. [listbox.cpp] Missing container `border`, `box-shadow`, and `background` from CSS `.ui-listbox`
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-listbox` has `background: var(--background); border: 1px solid var(--border); box-shadow: black 0 0 3px 0px; overflow: hidden`. C++ uses `ImGui::BeginChild()` for the container but may not apply explicit border color matching `var(--border)` or the box-shadow. The container should have a visible border and shadow.

- [ ] 283. [listbox.cpp] `.item:hover` uses `var(--font-alt) !important` in CSS but C++ hover effect may differ
- **JS Source**: `src/app.css` lines 1070–1072
- **Status**: Pending
- **Details**: CSS `.ui-listbox .item:hover { background: var(--font-alt) !important; }` applies `--font-alt` (#57afe2) as hover background with `!important` overriding even selected items. C++ hover rendering (if implemented) should use the same color. Need to verify the C++ listbox render function applies hover highlighting with the correct color.

- [ ] 284. [listbox.cpp] `contextmenu` event not emitted in base listbox JS but C++ has `onContextMenu` support
- **JS Source**: `src/js/components/listbox.js` line 41
- **Status**: Pending
- **Details**: JS declares `emits: ['update:selection', 'update:filter', 'contextmenu']` but the JS template does not have any `@contextmenu` event handler on the items. The `contextmenu` event is declared but never emitted in the base listbox template code. C++ (lines 449–476) has a full `handleContextMenu` implementation that fires on right-click. This C++ feature goes beyond what the JS base listbox actually does (though it's declared in emits). The JS may handle context menu at a parent level instead.

- [ ] 285. [listbox.cpp] Scroller thumb color uses `FONT_PRIMARY_U32` / `FONT_HIGHLIGHT_U32` but CSS uses `var(--border)` / `var(--font-highlight)`
- **JS Source**: `src/app.css` lines 1106–1118
- **Status**: Pending
- **Details**: CSS `.scroller > div` default background is `var(--border)` with `border: 1px solid var(--border)`. On hover/active (`.scroller:hover > div, .scroller.using > div`), it changes to `var(--font-highlight)`. C++ should use `BORDER_U32` for default state and `FONT_HIGHLIGHT_U32` for hover/active, not `FONT_PRIMARY_U32` for the default state. `FONT_PRIMARY_U32` is white with alpha, while `--border` is a different color.

- [ ] 286. [listbox.cpp] Quick filter links use `ImGui::SmallButton` but CSS uses `<a>` tags with specific styling
- **JS Source**: `src/app.css` (quick-filters styling)
- **Status**: Pending
- **Details**: The C++ quick filter rendering (lines 801–827) uses `ImGui::SmallButton()` for filter links and `ImGui::Text("/")` for separators. The JS version uses `<a>` anchor tags with CSS styling including `color: #888` for inactive and `color: #ffffff; font-weight: bold` for active filters. `ImGui::SmallButton` has button styling (background, border) that doesn't match the CSS anchor link appearance. Should use styled text or selectable to match the link-like appearance.

- [ ] 287. [listbox.cpp] `includefilecount` prop exists in JS but C++ doesn't use it — counter is always shown when `unittype` is non-empty
- **JS Source**: `src/js/components/listbox.js` line 40
- **Status**: Pending
- **Details**: JS has `includefilecount` prop that, when true, includes a file counter. The counter display is conditional on `unittype` in the template (`v-if="unittype"`). C++ header declares the render function without an `includefilecount` parameter — it only checks `unittype` for showing the counter. The `includefilecount` prop may control additional behavior in the JS version that isn't captured by just checking `unittype`.

- [ ] 288. [listbox.cpp] `activated()` / `deactivated()` Vue lifecycle hooks for keep-alive not ported
- **JS Source**: `src/js/components/listbox.js` lines 96–113
- **Status**: Pending
- **Details**: JS has `activated()` and `deactivated()` lifecycle hooks that register/unregister paste and keyboard listeners when the component enters/leaves a `<keep-alive>` cache. C++ renders each frame via `render()` calls — there's no keep-alive equivalent. The keyboard and paste handling happens inline each frame. However, if multiple listbox instances exist across different tabs, the C++ version may process keyboard input for all of them simultaneously (since all `render()` calls run every frame), while JS only processes input for the active (non-deactivated) instance. This could cause input to be consumed by the wrong listbox.

- [ ] 289. [listboxb.cpp] Selection payload changed from item values to row indices
- **JS Source**: `src/js/components/listboxb.js` lines 226–273
- **Status**: Pending
- **Details**: JS selection stores selected item values directly. C++ stores selected indices (`std::vector<int>`), which changes emitted selection data and behavior when item ordering changes.

- [ ] 290. [listboxb.cpp] Selection highlighting logic uses index identity instead of value identity
- **JS Source**: `src/js/components/listboxb.js` lines 281–283
- **Status**: Pending
- **Details**: JS checks `selection.includes(item)` by item value/object. C++ checks index membership, so highlight/selection parity diverges when values repeat or list contents are reordered.

- [ ] 291. [listboxb.cpp] JS `selection` stores item objects but C++ stores item indices
- **JS Source**: `src/js/components/listboxb.js` lines 14, 230–231, 208–214, 281
- **Status**: Pending
- **Details**: JS `selection` prop stores item objects (e.g., `this.selection.indexOf(item)` where `item` is an object from `this.items`). The JS template uses `selection.includes(item)` for object identity comparison. C++ uses `std::vector<int>` for selection (indices into items array). This means C++ selection is index-based while JS is identity-based. If items are reordered or filtered, the indices in C++ could point to wrong items, while JS object references remain valid. The C++ approach is documented in the header.

- [ ] 292. [listboxb.cpp] JS `handleKey` Ctrl+C copies `this.selection.join('\n')` (object labels) but C++ copies `items[idx].label`
- **JS Source**: `src/js/components/listboxb.js` lines 179–181
- **Status**: Pending
- **Details**: JS: `nw.Clipboard.get().set(this.selection.join('\n'), 'text')` — joins selection items (which are objects with `.label`) using `join('\n')`. When JS objects are joined, they call `toString()` which returns `[object Object]` unless overridden. This is likely a JS bug — the code should probably join labels. C++ (lines 166–175) correctly copies `items[idx].label` for each selected index. C++ behavior is more correct than the JS source.

- [ ] 293. [listboxb.cpp] Alternating row color parity may not match CSS `:nth-child(even)` due to 0-indexed `startIdx`
- **JS Source**: `src/app.css` lines 1091–1092
- **Status**: Pending
- **Details**: CSS `.ui-listbox .item:nth-child(even)` uses 1-indexed DOM position to determine even rows. C++ (line 373) uses `(i - startIdx) % 2 == 0` to determine the visual position parity. When `startIdx` changes due to scrolling, the parity flips — row 0 is always "even" in C++ regardless of its actual index in the data. In JS, `:nth-child` is based on the rendered DOM elements (displayItems), so scrolling changes which items are at even positions. Since both C++ and JS re-render from `displayItems`, the visual parity should match for the visible items. However, the C++ uses `(i - startIdx) % 2 == 0` as alt (even visual position), while CSS even is the 2nd, 4th, etc. DOM child (also 0-indexed). This mapping should be verified.

- [ ] 294. [listboxb.cpp] Missing container `border`, `box-shadow` from CSS `.ui-listbox`
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-listbox` has `border: 1px solid var(--border); box-shadow: black 0 0 3px 0px`. C++ (line 305) uses `ImGui::BeginChild("##listboxb_container", availSize, ImGuiChildFlags_None, ...)` with `ImGuiChildFlags_None` which does NOT draw a border. The container should have `ImGuiChildFlags_Borders` and matching border color to replicate the CSS appearance.

- [ ] 295. [listboxb.cpp] Scroller thumb uses `TEXT_ACTIVE_U32` / `TEXT_IDLE_U32` but CSS uses `var(--border)` / `var(--font-highlight)`
- **JS Source**: `src/app.css` lines 1106–1118
- **Status**: Pending
- **Details**: CSS `.scroller > div` uses `background: var(--border)` default and `var(--font-highlight)` on hover. C++ (lines 347–349) uses `app::theme::TEXT_ACTIVE_U32` and `app::theme::TEXT_IDLE_U32` which may not match the CSS variables. Should use `BORDER_U32` for default and `FONT_HIGHLIGHT_U32` for hover to match CSS.

- [ ] 296. [listboxb.cpp] Row width uses `availSize.x - 10.0f` but CSS doesn't subtract 10px
- **JS Source**: `src/js/components/listboxb.js` template line 281
- **Status**: Pending
- **Details**: C++ (lines 372, 387) uses `availSize.x - 10.0f` for row max width and selectable width, presumably to leave room for the scrollbar (8px wide). However, the CSS `.ui-listbox .item` has no explicit width reduction — items fill the full container width and the scroller overlaps via `position: absolute`. The 10px subtraction in C++ could cause items to be narrower than expected.

- [ ] 297. [listboxb.cpp] `displayItems` computed as `items.slice(scrollIndex, scrollIndex + slotCount)` — C++ iterates directly with indices
- **JS Source**: `src/js/components/listboxb.js` lines 89–91
- **Status**: Pending
- **Details**: JS creates `displayItems` as a computed property returning a slice of the items array. The template iterates `displayItems` and uses `selectItem(item, $event)` passing the item object. C++ iterates from `startIdx` to `endIdx` directly and passes the index to `selectItem`. This is equivalent but the selection model difference (objects vs indices) means the selection semantics differ as noted in entry 510.

- [ ] 298. [listbox-context.cpp] handle_context_menu signature changed from data object to direct selection array
- **JS Source**: `src/js/ui/listbox-context.js` lines 1–10
- **Status**: Pending
- **Details**: JS `handle_context_menu(data, isLegacy)` takes a data object with a `.selection` property. C++ takes `(const std::vector<std::string>& selection, bool isLegacy)` directly. Callers must adapt to pass the selection array instead of wrapping it in a data object.

- [ ] 299. [listbox-context.cpp] get_export_directory returns empty string instead of null
- **JS Source**: `src/js/ui/listbox-context.js` lines 25–40
- **Status**: Pending
- **Details**: JS returns `null` when selection is empty or no valid directory found. C++ returns `""` (empty string). Callers must check `.empty()` instead of a null/nullptr check.

- [ ] 300. [listbox-maps.cpp] Missing `recalculateBounds()` call after resetting scroll on expansion filter change
- **JS Source**: `src/js/components/listbox-maps.js` lines 27–31
- **Status**: Pending
- **Details**: JS `watch: { expansionFilter: function() { this.scroll = 0; this.scrollRel = 0; this.recalculateBounds(); } }` calls `recalculateBounds()` after resetting scroll values. C++ (lines 91–95) sets `scroll = 0.0f` and `scrollRel = 0.0f` but does NOT call `recalculateBounds()`. The JS `recalculateBounds()` ensures scroll values are clamped and relative values are consistent. While setting both to 0 should be inherently valid, it differs from the JS behavior and could miss persist-scroll-key saving that happens in `recalculateBounds()`.

- [ ] 301. [listbox-maps.cpp] JS `filteredItems` has inline text filtering + selection pruning, C++ delegates to base listbox
- **JS Source**: `src/js/components/listbox-maps.js` lines 43–88
- **Status**: Pending
- **Details**: JS `filteredItems` computed property applies expansion filtering, THEN text filtering (debounced filter + regex), THEN selection pruning — all inline in the computed property. C++ pre-filters by expansion (line 103) then delegates to `listbox::render()` which handles text filtering internally. The order of operations should be equivalent (expansion first, then text), but the text filtering and selection pruning happen inside the base listbox's `computeFilteredItems()` which is called within `render()`. The JS version computes the full filtered list as a reactive computed property, while C++ recomputes each frame. Functionally equivalent.

- [ ] 302. [listbox-zones.cpp] Missing `recalculateBounds()` call after resetting scroll on expansion filter change
- **JS Source**: `src/js/components/listbox-zones.js` lines 27–31
- **Status**: Pending
- **Details**: Same issue as entry 499 but for listbox-zones. JS calls `this.recalculateBounds()` after resetting scroll to 0 on expansion filter change. C++ (lines 91–95) only sets values to 0 without calling `recalculateBounds()`. Missing persist-scroll-key saving opportunity and bounds validation.

- [ ] 303. [listbox-zones.cpp] Identical implementation to listbox-maps.cpp — shared code could be refactored
- **JS Source**: `src/js/components/listbox-zones.js` (entire file — identical structure to listbox-maps.js)
- **Status**: Pending
- **Details**: `listbox-zones.cpp` is a near-identical copy of `listbox-maps.cpp` with only the namespace name and comment text differing (maps→zones). The JS sources are also structurally identical. This is not a bug but a code duplication opportunity — both could share a common `listbox-expansion-filter` utility. This matches the JS source's structure (both extend `listboxComponent` identically).

- [ ] 304. [itemlistbox.cpp] Selection model changed from item-object references to item ID integers
- **JS Source**: `src/js/components/itemlistbox.js` lines 117–129, 271–315
- **Status**: Pending
- **Details**: JS selection stores full item objects and compares by object identity (`includes/indexOf(item)`). C++ stores numeric IDs, changing selection semantics and update payload shape.

- [ ] 305. [itemlistbox.cpp] Item action controls are rendered as ImGui buttons instead of list item links
- **JS Source**: `src/js/components/itemlistbox.js` lines 336–339
- **Status**: Pending
- **Details**: JS renders actions as `<ul class="item-buttons"><li>...</li></ul>` with CSS styling. C++ uses `SmallButton` widgets, producing different visual and interaction behavior.

- [ ] 306. [itemlistbox.cpp] JS `emits: ['update:selection', 'equip']` but C++ header declares `onOptions` callback
- **JS Source**: `src/js/components/itemlistbox.js` line 20
- **Status**: Pending
- **Details**: JS declares `emits: ['update:selection', 'equip']` — only two events. The JS template has `<li @click.self="$emit('options', item)">Options</li>` but 'options' is NOT in the emits array (it should be). C++ header (itemlistbox.h lines 85–86) declares both `onEquip` and `onOptions` callbacks. The C++ adds the `onOptions` callback that the JS template uses but doesn't formally declare in emits. This is technically a JS bug that C++ correctly accounts for, but the JS source discrepancy should be noted.

- [ ] 307. [itemlistbox.cpp] `handleKey` copies `displayName` but JS copies `displayName` via `e.displayName` from selection objects
- **JS Source**: `src/js/components/itemlistbox.js` lines 226–228
- **Status**: Pending
- **Details**: JS `handleKey` copies: `nw.Clipboard.get().set(this.selection.map(e => e.displayName).join('\n'), 'text')` — maps selection items (which are full item objects) to their `displayName`. C++ (lines 272–284) looks up each selected ID in `filteredItems` via `indexOfItemById()` and copies `displayName`. The JS maps directly from `this.selection` (which contains item object references), while C++ must look up by ID since selection stores IDs not objects. Functionally equivalent if all selected IDs exist in filteredItems, but could produce different results if a selected ID is no longer in the filtered list (C++ skips it, JS would still have the object reference).

- [ ] 308. [itemlistbox.cpp] Item row height uses `46px` but JS CSS uses `height: 26px` in `.ui-listbox .item`
- **JS Source**: `src/app.css` line 1089, `src/js/components/itemlistbox.js` line 155
- **Status**: Pending
- **Details**: CSS `.ui-listbox .item` has `height: 26px`. However, C++ (line 81) uses `46.0f` for item height and (line 439) uses `itemHeightVal = 46.0f`. The comment on line 80 references `#tab-items #listbox-items .item { height: 46px }` — this is a tab-specific CSS override. Need to verify that `#tab-items #listbox-items .item` actually overrides to 46px in app.css. If so, 46px is correct for item listboxes specifically.

- [ ] 309. [itemlistbox.cpp] Missing container `border`, `box-shadow`, and `background` from CSS `.ui-listbox`
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-listbox` has `background: var(--background); border: 1px solid var(--border); box-shadow: black 0 0 3px 0px; overflow: hidden`. C++ (line 453, based on BeginChild call) likely uses default ImGui child window styling without explicit border color, box-shadow, or background matching the CSS. The listbox container should have a visible border and shadow for visual separation.

- [ ] 310. [itemlistbox.cpp] Quality 7 and 8 both map to Heirloom color but CSS may define separate classes
- **JS Source**: `src/js/components/itemlistbox.js` template line 335
- **Status**: Pending
- **Details**: C++ `getQualityColor()` (lines 400–401) maps both quality 7 and 8 to the same color `#00ccff`. The JS template uses `'item-quality-' + item.quality` as a CSS class, so quality 7 and 8 would use `.item-quality-7` and `.item-quality-8` respectively. If the CSS defines different colors for these two classes, the C++ would be incorrect. Need to check app.css for `.item-quality-7` and `.item-quality-8` definitions.

- [ ] 311. [itemlistbox.cpp] `.item-icon` rendering not implemented — icon placeholder only
- **JS Source**: `src/js/components/itemlistbox.js` template line 334
- **Status**: Pending
- **Details**: JS template: `<div :class="['item-icon', 'icon-' + item.icon ]"></div>`. This renders the item icon using CSS background-image from the `icon-{fileDataID}` class. C++ `render()` does load icons via `IconRender::loadIcon()` but the actual icon texture rendering in the list rows needs verification — the code should render the icon texture at the correct size and position matching the CSS `.item-icon` dimensions.

- [ ] 312. [data-table.cpp] Filter icon click no longer focuses the data-table filter input
- **JS Source**: `src/js/components/data-table.js` lines 742–760
- **Status**: Pending
- **Details**: JS appends `column:` filter text and then focuses `#data-table-filter-input` with cursor placement. C++ only emits the new filter string and leaves focus behavior unimplemented.

- [ ] 313. [data-table.cpp] Empty-string numeric sorting semantics differ from JS `Number(...)`
- **JS Source**: `src/js/components/data-table.js` lines 179–193
- **Status**: Pending
- **Details**: JS treats `''` as numeric (`Number('') === 0`) during sort. C++ `tryParseNumber` rejects empty strings, so those values sort as text instead of numeric values.

- [ ] 314. [data-table.cpp] Header sort/filter icons are custom-drawn instead of CSS/SVG assets
- **JS Source**: `src/js/components/data-table.js` lines 988–1003
- **Status**: Pending
- **Details**: JS uses CSS icon classes backed by `fa-icons` images for exact visuals. C++ draws procedural triangles/lines, producing non-identical icon appearance versus the JS UI.

- [ ] 315. [data-table.cpp] Native scroll-to-custom-scroll sync path from JS is missing
- **JS Source**: `src/js/components/data-table.js` lines 51–57, 513–527
- **Status**: Pending
- **Details**: JS listens for native root scroll events and synchronizes custom scrollbar state. C++ omits this path entirely, so behavior differs from the original scroll synchronization model.

- [ ] 316. [data-table.cpp] JS sort uses `Array.prototype.sort()` (unstable in some engines) but C++ uses `std::stable_sort`
- **JS Source**: `src/js/components/data-table.js` line 170
- **Status**: Pending
- **Details**: JS uses `sorted.sort(...)` which in modern engines (V8, SpiderMonkey) uses TimSort (stable). C++ (line 296) uses `std::stable_sort` which is guaranteed stable. The comment in C++ (line 295) notes this intention. While functionally equivalent in modern engines, the JS spec doesn't guarantee stability, so the C++ version is technically more deterministic. This is a minor difference — practically identical behavior.

- [ ] 317. [data-table.cpp] JS `localeCompare()` for string sorting vs C++ `std::string::compare()` after toLower
- **JS Source**: `src/js/components/data-table.js` line 192
- **Status**: Pending
- **Details**: JS uses `aStr.localeCompare(bStr)` which is locale-aware and handles Unicode collation. C++ (lines 319–323) uses `aStr.compare(bStr)` after `toLower()` which is a byte-wise comparison after ASCII lowercasing. This means non-ASCII characters (accented letters, etc.) sort differently: JS respects locale-specific ordering (e.g., 'é' sorts near 'e'), while C++ uses raw byte values. For WoW data that may contain localized strings, this could produce different sort orders.

- [ ] 318. [data-table.cpp] `preventMiddleMousePan` from JS is not ported
- **JS Source**: `src/js/components/data-table.js` lines 52–53, mounted
- **Status**: Pending
- **Details**: JS registers an `onMiddleMouseDown` handler to `this.preventMiddleMousePan(e)` on the root element, preventing the browser's default middle-mouse-button scroll/pan behavior. C++ does not need this since ImGui doesn't have browser-like middle-click pan, but the handler is mentioned in the JS `mounted()` and `beforeUnmount()` hooks. No behavioral difference expected, but the JS source code is not represented in C++ comments.

- [ ] 319. [data-table.cpp] `syncScrollPosition` is intentionally omitted but JS uses it to sync native+custom scroll
- **JS Source**: `src/js/components/data-table.js` lines 51, 56
- **Status**: Pending
- **Details**: JS registers `this.onScroll = e => this.syncScrollPosition(e)` on the root element's `scroll` event, syncing the custom scrollbar position with the browser's native scroll. C++ (lines 422–430) correctly documents this as unnecessary since ImGui has no native scroll. The JS `syncScrollPosition` method reads `this.$refs.root.scrollLeft` and updates `horizontalScrollRel` accordingly. This is covered in C++ by direct scroll management. Correctly omitted.

- [ ] 320. [data-table.cpp] `list-status` text uses `ImGui::TextDisabled` but CSS `.list-status` may have different styling
- **JS Source**: `src/js/components/data-table.js` template line 1017–1020, `src/app.css` lines 2506+
- **Status**: Pending
- **Details**: C++ (line 1380) uses `ImGui::TextDisabled("%s", ...)` for the status line, which applies a dimmed text color. The JS template uses a plain `<div class="list-status">` with `<span>` text. CSS `.list-status` styling should be checked — it may have specific font size, color, or padding that differs from ImGui's disabled text appearance. The `toLocaleString()` formatting is correctly replicated with `formatWithThousandsSep()`.

- [ ] 321. [data-table.cpp] Missing CSS container `border: 1px solid var(--border)` and `box-shadow: black 0 0 3px 0px`
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-datatable` inherits from `.ui-listbox, .ui-checkboxlist, .ui-datatable` which sets `border: 1px solid var(--border); box-shadow: black 0 0 3px 0px`. C++ (line 994) uses `ImGui::BeginChild("##datatable_root", availSize, ImGuiChildFlags_Borders, ...)` which draws a border but ImGui borders use default border color, not `var(--border)`. Box-shadow is not supported in ImGui. The border color and shadow provide visual separation in the JS version.

- [ ] 322. [data-table.cpp] Row alternating colors use `BG_ALT_U32` / `BG_DARK_U32` but CSS uses `--background-dark` (default) and `--background-alt` (even)
- **JS Source**: `src/app.css` lines 1081–1093
- **Status**: Pending
- **Details**: CSS sets all rows to `background: var(--background-dark)` with even rows overriding to `background: var(--background-alt)`. C++ (lines 1240–1246) sets odd rows (displayRow % 2 == 0) to `BG_DARK_U32` and even rows (displayRow % 2 == 1) to `BG_ALT_U32`. The parity is based on `displayRow` (visual position) not the absolute row index. JS CSS `:nth-child(even)` is based on DOM position which corresponds to visual position, so this should match. However, the C++ maps `displayRow % 2 == 0` to dark and `displayRow % 2 == 1` to alt, while CSS uses `nth-child(even)` (1-indexed, so positions 2,4,6...) for alt. Since `displayRow` is 0-indexed, `displayRow == 0` is the first row (CSS odd/1st child = dark), `displayRow == 1` is second row (CSS even = alt). This mapping is correct.

- [ ] 323. [data-table.cpp] Cell padding is `5px` in C++ but CSS uses `padding: 5px 10px` for `td`
- **JS Source**: `src/app.css` lines 1225–1231
- **Status**: Pending
- **Details**: CSS `.ui-datatable table tr td` has `padding: 5px 10px` (5px top/bottom, 10px left/right). C++ (line 1293) uses `const float cellPadding = 5.0f` which is applied as left padding only. The right padding and top/bottom padding are not explicitly applied — the text is vertically centered via `(rowHeight - ImGui::GetTextLineHeight()) / 2.0f` but horizontal padding should be 10px left, not 5px.

- [ ] 324. [data-table.cpp] Missing `Number(val)` equivalence check in `escape_value` — JS `isNaN(val)` checks the original value type
- **JS Source**: `src/js/components/data-table.js` lines 950–958
- **Status**: Pending
- **Details**: JS `escape_value` checks `if (val === null || val === undefined) return 'NULL'` then `if (!isNaN(val) && str.trim() !== '') return str`. The `!isNaN(val)` check tests if the ORIGINAL value (not string) is numeric. C++ (lines 874–879) uses `tryParseNumber(val, num)` which parses the string representation. In JS, `!isNaN(Number(""))` is `false` (Number("") is 0 but isNaN(0) is false, so !isNaN is true) — wait, `Number("")` is `0` and `isNaN(0)` is `false`, so `!isNaN(Number(""))` is `true`. But the JS also checks `str.trim() !== ''` to exclude empty strings. C++ `tryParseNumber` checks `pos == s.size()` which would fail for empty string since `stod("")` throws. So both handle empty strings correctly (treated as non-numeric). Functionally equivalent.

- [ ] 325. [data-table.cpp] `formatWithThousandsSep` needs to match JS `toLocaleString()` thousands separator
- **JS Source**: `src/js/components/data-table.js` template lines 1018–1019
- **Status**: Pending
- **Details**: JS uses `filteredItems.length.toLocaleString()` and `rows.length.toLocaleString()` which formats numbers with locale-appropriate thousands separators. C++ uses `formatWithThousandsSep()` (line 1374–1377). The C++ function should produce comma-separated thousands (e.g., "1,234") to match the default English locale used in most WoW installations. If the function uses a different separator or format, the status text would differ visually.

- [ ] 326. [data-table.cpp] Header height hardcoded to `40px` but CSS uses `padding: 10px` top/bottom on `th`
- **JS Source**: `src/app.css` lines 1163–1168
- **Status**: Pending
- **Details**: C++ (line 971) hardcodes `const float headerHeight = 40.0f` with comment "padding 10px top/bottom + ~20px text". CSS `.ui-datatable table tr th` has `padding: 10px` (all sides). The actual header height depends on the font size — CSS text is typically ~14px default, so 10px + 14px + 10px = 34px, not 40px. If the CSS font size is different or the browser computes differently, the hardcoded 40px may not match.

- [ ] 327. [data-table.cpp] `handleFilterIconClick` not fully visible but CSS filter icon uses specific SVG styling
- **JS Source**: `src/app.css` lines 1176–1191
- **Status**: Pending
- **Details**: CSS `.filter-icon` uses a background SVG image (`background-image: url(./fa-icons/funnel.svg)`) with `background-size: contain; width: 18px; height: 14px`. C++ (lines 1101–1113) draws a custom triangle + rectangle shape as a funnel icon approximation. The custom drawing may not match the SVG icon's exact shape and proportions. The CSS also specifies `opacity: 0.5` default and `opacity: 1.0` on hover, while C++ uses `ICON_DEFAULT_U32` and `FONT_HIGHLIGHT_U32` colors.

- [ ] 328. [data-table.cpp] Sort icon CSS uses SVG background images but C++ draws triangles
- **JS Source**: `src/app.css` lines 1198–1224
- **Status**: Pending
- **Details**: CSS `.sort-icon` uses `background-image: url(./fa-icons/sort.svg)` for the default state, with `.sort-icon-up` and `.sort-icon-down` using different SVG files. The icons have `width: 12px; height: 18px; opacity: 0.5`. C++ (lines 1119–1157) draws triangle shapes to approximate sort icons. The triangle approximation may not match the SVG icon's exact appearance — SVGs typically have more refined shapes with anti-aliasing.

- [ ] 329. [file-field.cpp] Directory dialog trigger moved from input focus to separate browse button
- **JS Source**: `src/js/components/file-field.js` lines 34–40, 46
- **Status**: Pending
- **Details**: JS opens the directory picker when the text field receives focus. C++ opens the dialog only from a dedicated `...` button, changing interaction flow and UI behavior.

- [ ] 330. [file-field.cpp] Same-directory reselection behavior differs from JS file input reset logic
- **JS Source**: `src/js/components/file-field.js` lines 35–38
- **Status**: Pending
- **Details**: JS clears the hidden file input value before click so selecting the same directory re-triggers change emission. C++ dialog path does not mirror this reset contract.

- [ ] 331. [file-field.cpp] JS opens dialog on `@focus` but C++ opens on button click
- **JS Source**: `src/js/components/file-field.js` lines 33–40, template line 46
- **Status**: Pending
- **Details**: JS template uses `@focus="openDialog"` on the text input — when the input receives focus, the directory picker opens immediately. C++ (lines 128–132) uses a separate "..." button next to the input to trigger the dialog. The JS behavior is: clicking the text field opens the dialog, and the field never actually receives text focus for editing. C++ allows direct text editing in the field AND has a browse button. This is a significant UX difference — in JS, the field is effectively read-only (clicking always opens picker), while in C++ it's editable with an optional browse button.

- [ ] 332. [file-field.cpp] JS `mounted()` creates hidden `<input type="file" nwdirectory>` element — C++ uses portable-file-dialogs
- **JS Source**: `src/js/components/file-field.js` lines 14–23
- **Status**: Pending
- **Details**: JS creates a hidden file input with `nwdirectory` attribute (NW.js specific for directory selection), listens for `change` event, and emits the selected value. C++ uses `pfd::select_folder()` which opens a native folder dialog. The underlying mechanism differs (NW.js DOM file input vs. native OS dialog) but the user-facing behavior should be equivalent — both present a directory picker. C++ implementation correctly replaces the NW.js-specific API.

- [ ] 333. [file-field.cpp] JS `openDialog()` clears file input value before opening — C++ does not clear state
- **JS Source**: `src/js/components/file-field.js` lines 34–39
- **Status**: Pending
- **Details**: JS `openDialog()` sets `this.fileSelector.value = ''` before calling `click()` and then calls `this.$el.blur()`. This ensures the `change` event fires even if the user selects the same directory again. C++ `openDialog()` (line 74–81) calls `openDirectoryDialog()` directly without any pre-clear. Since `pfd::select_folder()` returns the result directly (not via an event), re-selecting the same directory works fine — the result is always returned. The `blur()` call is unnecessary in C++ since the dialog is modal.

- [ ] 334. [file-field.cpp] Missing placeholder rendering position uses hardcoded offsets
- **JS Source**: `src/js/components/file-field.js` template line 46
- **Status**: Pending
- **Details**: C++ (lines 120–126) renders placeholder text with `ImVec2(textPos.x + 4.0f, textPos.y + 2.0f)` using hardcoded offsets. The JS template uses the browser's native placeholder rendering via `:placeholder="placeholder"` which automatically positions and styles the placeholder text. The C++ offsets may not match the actual input text baseline, causing misalignment.

- [ ] 335. [file-field.cpp] Extra `openFileDialog()` and `saveFileDialog()` functions not in JS source
- **JS Source**: `src/js/components/file-field.js` (entire file)
- **Status**: Pending
- **Details**: C++ adds `openFileDialog()` (lines 44–53) and `saveFileDialog()` (lines 60–73) as public utility functions. The JS file-field component only provides directory selection via `nwdirectory`. These extra functions may be useful for other parts of the C++ app but are not present in the original JS component source. They are additional API surface.

- [ ] 336. [resize-layer.cpp] ResizeObserver lifecycle is replaced by per-frame width polling
- **JS Source**: `src/js/components/resize-layer.js` lines 12–15, 21–23
- **Status**: Pending
- **Details**: JS emits resize through `ResizeObserver` mount/unmount lifecycle. C++ emits when measured width changes during render, so behavior is tied to render frames instead of observer callbacks.

- [ ] 337. [resize-layer.cpp] Fully ported — no issues found
- **JS Source**: `src/js/components/resize-layer.js` lines 1–26
- **Status**: Pending
- **Details**: The resize-layer component is a simple wrapper that emits a 'resize' event when the element width changes. JS uses `ResizeObserver` and `beforeUnmount` cleanup. C++ uses `ImGui::GetContentRegionAvail().x` polling each frame and compares against previous width. The conversion is functionally complete and correct. No deviations found.

- [ ] 338. [markdown-content.cpp] Inline image markdown is not rendered as images
- **JS Source**: `src/js/components/markdown-content.js` lines 208–216, 251–253
- **Status**: Pending
- **Details**: JS converts `![alt](src)` to `<img ...>` in `v-html` output. C++ renders image segments as placeholder text (`[Image: ...]`), causing functional and visual mismatch.

- [ ] 339. [markdown-content.cpp] Inline bold/italic formatting behavior differs from JS HTML rendering
- **JS Source**: `src/js/components/markdown-content.js` lines 219–224
- **Status**: Pending
- **Details**: JS emits `<strong>`/`<em>` markup; C++ substitutes color-only text rendering because ImGui has no inline bold/italic in this path, so typography does not match original styling.

- [ ] 340. [markdown-content.cpp] CSS base font-size 20px not applied — ImGui uses default ~14px font
- **JS Source**: `src/app.css` lines 236–243
- **Status**: Pending
- **Details**: CSS `.markdown-content { font-size: 20px; }` sets the base font size for all markdown content to 20px. C++ `render()` does not call `ImGui::SetWindowFontScale()` to scale up to 20px equivalent. The default ImGui font is ~13-14px, so all markdown text renders significantly smaller than the JS version. Header scales (1.8em, 1.5em, 1.2em) are applied correctly relative to the current font scale, but since the base is wrong, all absolute sizes are too small.

- [ ] 341. [markdown-content.cpp] Link color uses `FONT_ALT` (#57afe2 blue) instead of CSS `--font-highlight` (#ffffff white)
- **JS Source**: `src/app.css` lines 311–314
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner a { color: var(--font-highlight); text-decoration: underline; }` uses pure white (#ffffff) for links with an underline. C++ `renderInlineSegments` line 254 uses `app::theme::FONT_ALT` (blue #57afe2) for links. The link color should be `FONT_HIGHLIGHT` (white) to match the CSS.

- [ ] 342. [markdown-content.cpp] Links rendered without underline decoration
- **JS Source**: `src/app.css` lines 311–314
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner a { text-decoration: underline; }` renders links with an underline. C++ `renderInlineSegments` (lines 253–267) renders link text without any underline decoration. ImGui does not natively support underlined text, but a manual underline can be drawn using `ImGui::GetWindowDrawList()->AddLine()` beneath the text.

- [ ] 343. [markdown-content.cpp] Inline code missing background `rgba(0,0,0,0.3)` with `padding: 2px 6px` and `border-radius: 3px`
- **JS Source**: `src/app.css` lines 283–288
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner code` has a dark background, padding, and rounded corners. C++ `renderInlineSegments` (lines 248–251) only changes the text color to `(0.9, 0.7, 0.5)` for inline code, without any background rectangle. The background color should be `IM_COL32(0, 0, 0, 77)` with a rounded rect behind the text.

- [ ] 344. [markdown-content.cpp] Inline code color `(0.9, 0.7, 0.5)` has no CSS basis — CSS uses monospace font, not a special color
- **JS Source**: `src/app.css` lines 283–289
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner code` uses `font-family: monospace` but inherits the parent text color (white). C++ applies `ImVec4(0.9f, 0.7f, 0.5f, 1.0f)` (orange-ish) which has no equivalent in the CSS. The text color should remain the same as surrounding text; only the font family should change (which ImGui can't easily do for inline segments).

- [ ] 345. [markdown-content.cpp] Bold text rendered with white color instead of bold font weight
- **JS Source**: `src/app.css` lines 303–305
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner strong { font-weight: bold; }` makes bold text heavier. C++ `renderInlineSegments` (lines 237–240) pushes white color `(1,1,1,1)` for bold, which is the same as normal text on a dark background. The visual difference from normal text is imperceptible. ImGui doesn't support inline bold without loading a separate bold font face and switching to it.

- [ ] 346. [markdown-content.cpp] Italic text rendered with dim blue `(0.8, 0.8, 0.9)` instead of italic font style
- **JS Source**: `src/app.css` lines 307–309
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner em { font-style: italic; }` renders text in italic. C++ `renderInlineSegments` (lines 243–246) uses a dimmed blue-white color `(0.8, 0.8, 0.9)` which has no CSS basis. ImGui doesn't support italic text natively, but the color chosen doesn't match any CSS variable.

- [ ] 347. [markdown-content.cpp] h1 header missing bottom separator matching CSS `border-bottom`
- **JS Source**: `src/app.css` lines 249–253
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner h1` has `margin: 0` but standard HTML h1 elements typically render with a bottom margin. C++ adds `ImGui::Separator()` after h1 (line 347) which draws a full-width horizontal line. The JS HTML rendering does not add a separator after h1 — the `<h1>` tag simply has larger text. The `ImGui::Separator()` is not present in the CSS and creates a visual difference.

- [ ] 348. [markdown-content.cpp] Scrollbar thumb uses hardcoded gray colors instead of CSS `var(--border)` and `var(--font-highlight)`
- **JS Source**: `src/app.css` lines 322–346
- **Status**: Pending
- **Details**: CSS `.vscroller > div` uses `background: var(--border)` (#6c757d) default and `background: var(--font-highlight)` (#ffffff) on hover/active. C++ uses hardcoded `IM_COL32(120, 120, 120, 150)` default and `IM_COL32(180, 180, 180, 200)` for active (lines 457–459). Neither color matches the CSS variables. Default should be `BORDER_U32` and active should be `FONT_HIGHLIGHT_U32`.

- [ ] 349. [markdown-content.cpp] Scrollbar thumb missing `border: 1px solid var(--border)` and `border-radius: 5px` from CSS
- **JS Source**: `src/app.css` lines 332–341
- **Status**: Pending
- **Details**: CSS `.vscroller > div` has `border: 1px solid var(--border)` and `border-radius: 5px`. C++ draws the thumb with `AddRectFilled` with 4px rounding (line 461) but no border outline. Should add `AddRect` with `BORDER_U32` and use 5px rounding.

- [ ] 350. [markdown-content.cpp] Scrollbar track `right: 3px` positioning and `opacity: 0.7` not matched
- **JS Source**: `src/app.css` lines 322–330
- **Status**: Pending
- **Details**: CSS `.vscroller { right: 3px; opacity: 0.7; }` positions the scrollbar 3px from the right edge with 70% opacity. C++ positions the scrollbar at `containerSize.x - scrollbar_width - 2.0f` (line 451, using 2px instead of 3px) and uses full opacity for the thumb colors. The alpha values in the thumb colors partially compensate but don't match the 70% opacity overlay.

- [ ] 351. [markdown-content.cpp] `parseInline` processes text before HTML escaping, while JS escapes first then applies regex
- **JS Source**: `src/js/components/markdown-content.js` lines 204–237
- **Status**: Pending
- **Details**: JS `parseInline` calls `this.escapeHtml(text)` first (line 205) before applying regex replacements for bold, italic, code, links, and images. This means the regex patterns match against HTML-escaped text (e.g., `&amp;` instead of `&`). C++ `parseInline` processes raw text directly without escaping. While ImGui doesn't need HTML escaping, if markdown content contains `&`, `<`, or `>` characters, the parsing behavior could differ because the JS regex operates on escaped text while C++ operates on raw text.

- [ ] 352. [markdown-content.cpp] List items use `•` bullet with 16px indent instead of CSS `padding-left: 2em` with disc marker
- **JS Source**: `src/app.css` lines 272–276
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner ul { padding-left: 2em; list-style-type: disc; }` uses standard disc bullets with 2em left padding. At 20px base font, 2em = 40px. C++ uses `ImGui::Indent(16.0f)` with a manual `•` character (lines 369–373). 16px vs 40px indent is a significant visual difference, and the bullet character may render differently than the CSS disc marker.

- [ ] 353. [texture-ribbon.cpp] Additional GL texture management functions not present in JS
- **JS Source**: `src/js/ui/texture-ribbon.js` (entire file)
- **Status**: Pending
- **Details**: C++ adds `clearSlotTextures()`, `getSlotTexture()`, `s_slotTextures` map, and `s_slotSrcCache` map for OpenGL texture lifecycle management. JS uses DOM img elements with `.src` assignment which handles image loading/display natively. C++ `reset()` additionally calls `clearSlotTextures()` to delete GL resources. These are expected platform additions but expand the API surface beyond the JS module exports.
- [ ] 354. [home-showcase.cpp] Showcase card/video/background layer rendering is not ported
- **JS Source**: `src/js/components/home-showcase.js` lines 15–29, 32–42, 55–57
- **Status**: Pending
- **Details**: JS builds a layered CSS showcase card with optional autoplay video and title overlay. C++ replaces this with plain text/buttons and no equivalent visual composition.

- [ ] 355. [home-showcase.cpp] `background_style` computed output is missing from C++ render path
- **JS Source**: `src/js/components/home-showcase.js` lines 15–29, 55–57
- **Status**: Pending
- **Details**: JS computes `backgroundImage/backgroundSize/backgroundPosition/backgroundRepeat` from base + showcase layers. C++ does not apply these style layers, so showcase visuals diverge.

- [ ] 356. [home-showcase.cpp] Feedback action wiring differs from JS `data-kb-link` behavior
- **JS Source**: `src/js/components/home-showcase.js` lines 39–41
- **Status**: Pending
- **Details**: JS emits a KB-link anchor (`data-kb-link="KB011"`) handled by the app link system. C++ directly calls `ExternalLinks::open("KB011")`, which is not the same contract/path as the original markup-driven behavior.

- [ ] 357. [home-showcase.cpp] `build_background_style()` function not ported — no background image layers
- **JS Source**: `src/js/components/home-showcase.js` lines 15–29
- **Status**: Pending
- **Details**: JS `build_background_style(showcase)` constructs a CSS background-image style from `BASE_LAYERS` + `showcase.layers`, building comma-separated `backgroundImage`, `backgroundSize`, `backgroundPosition`, and `backgroundRepeat` values. C++ (line 127–129) has a comment noting that background image layers have no ImGui equivalent, but the function is entirely missing. The showcase visual identity relies heavily on these layered background images. The C++ version only shows text, losing the visual showcase entirely.

- [ ] 358. [home-showcase.cpp] `BASE_LAYERS` stored as single `ShowcaseLayer` instead of array
- **JS Source**: `src/js/components/home-showcase.js` lines 3–9
- **Status**: Pending
- **Details**: JS `BASE_LAYERS` is an array: `[{ image: './images/logo.png', size: '50px', position: 'bottom 10px right 10px' }]`. C++ (line 22) stores `BASE_LAYER` as a single `ShowcaseLayer` object, not a `std::vector`. While there's only one base layer in the JS source, the JS code uses array spread `[...BASE_LAYERS, ...showcase.layers]` which would support multiple base layers if added. The C++ naming (`BASE_LAYER` singular) and type differ from the JS plural array pattern.

- [ ] 359. [home-showcase.cpp] Video playback (`<video>` element) not implemented
- **JS Source**: `src/js/components/home-showcase.js` template line 34
- **Status**: Pending
- **Details**: JS template: `<video v-if="current.video" :src="current.video" autoplay loop muted playsinline></video>`. This renders an auto-playing, looping, muted video overlay on the showcase. C++ (line 126–129) notes this in a comment but does not implement any video rendering. ImGui does not natively support video playback; a custom texture upload with a video decoder (e.g., via miniaudio for audio, FFmpeg for video) would be needed.

- [ ] 360. [home-showcase.cpp] `computed: background_style()` not ported
- **JS Source**: `src/js/components/home-showcase.js` lines 55–57
- **Status**: Pending
- **Details**: JS computed property `background_style()` calls `build_background_style(this.current)` and the result is bound to the `<a>` element's `:style`. This creates the visual showcase appearance with layered background images. C++ does not compute or apply any background styling. The showcase area appears as plain text instead of an image showcase.

- [ ] 361. [home-showcase.cpp] Title text says "Made with wow.export.cpp" but JS says "Made with wow.export"
- **JS Source**: `src/js/components/home-showcase.js` template line 33
- **Status**: Pending
- **Details**: JS template: `<h1 id="home-showcase-header">Made with wow.export</h1>`. C++ (line 107): `ImGui::Text("Made with wow.export.cpp")`. Per the project conventions in copilot-instructions.md, user-facing text should say `wow.export.cpp`, so this is intentionally different. However, the JS source file says `wow.export`, so this is a documented deviation from the JS source.

- [ ] 362. [home-showcase.cpp] Showcase rendering is plain text instead of styled grid layout
- **JS Source**: `src/app.css` lines 3580–3633
- **Status**: Pending
- **Details**: CSS `#home-showcase-header` is in a grid layout (`grid-column: 1; grid-row: 1`), `#home-showcase` has `border: 1px solid var(--border); border-radius: 10px; cursor: pointer; overflow: hidden`, and `#home-showcase-links > a` has `font-size: 12px; color: #888`. C++ uses `ImGui::Text()`, `ImGui::SmallButton()`, and `ImGui::Separator()` without any grid layout, border-radius, or matching font sizes. The visual appearance is completely different from the styled JS version.

- [ ] 363. [home-showcase.cpp] Feedback link opens "KB011" directly instead of resolving via `data-kb-link` attribute
- **JS Source**: `src/js/components/home-showcase.js` template line 41
- **Status**: Pending
- **Details**: JS template: `<a data-kb-link="KB011">Feedback</a>`. The `data-kb-link` attribute is processed by the app's global link handler which resolves KB article IDs to URLs. C++ (line 142) calls `ExternalLinks::open("KB011")` directly with the raw string. If `ExternalLinks::open()` doesn't resolve KB article IDs (and only handles URLs), this would fail to open the correct link. The JS version relies on a global click handler that intercepts `data-kb-link` attributes.

- [ ] 364. [home-showcase.cpp] `.showcase-title` CSS uses Gambler font at 40px — not replicated
- **JS Source**: `src/app.css` lines 3607–3614
- **Status**: Pending
- **Details**: CSS `.showcase-title` uses `font-family: "Gambler", sans-serif; font-size: 40px; color: white` positioned absolutely at `top: 15px; left: 20px`. C++ (line 123–124) uses `ImGui::Text("%s", current->title.c_str())` with default ImGui font — no custom font, no large size, no absolute positioning. The showcase title should be a large overlay text in a specific font.

- [ ] 365. [map-viewer.cpp] Tile image drawing path is still unimplemented
- **JS Source**: `src/js/components/map-viewer.js` lines 380–402, 1111–1113
- **Status**: Pending
- **Details**: JS draws loaded tiles to canvas via `putImageData(...)` on main/double-buffer contexts. C++ caches tile pixels but does not upload/draw them, so only overlays render and map tiles are not visually equivalent.

- [ ] 366. [map-viewer.cpp] Tile loading flow is synchronous instead of JS Promise-based async queueing
- **JS Source**: `src/js/components/map-viewer.js` lines 192–197, 380–414
- **Status**: Pending
- **Details**: JS tile loader is async (`loader(...).then(...)`) with Promise completion timing. C++ calls loader synchronously in `loadTile(...)`, changing queue timing and behavior during panning/zoom updates.

- [ ] 367. [map-viewer.cpp] Box-select-mode active color uses NAV_SELECTED (#22B549) instead of CSS #5fdb65
- **JS Source**: `src/app.css` line 1348–1349
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .info span.active` uses `color: #5fdb65` (RGB 95, 219, 101). C++ line 1178 uses `app::theme::NAV_SELECTED` which maps to `BUTTON_BASE` = `(0.133, 0.710, 0.286)` = RGB(34, 181, 73). The active highlight color is noticeably different — should be a dedicated color constant matching #5fdb65.

- [ ] 368. [map-viewer.cpp] Map-viewer info bar text lacks CSS `text-shadow: black 0 0 6px`
- **JS Source**: `src/app.css` lines 1326–1328
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer div { text-shadow: black 0 0 6px; }` applies a text shadow to all divs inside the map viewer, including the info bar and hover-info. C++ renders these with plain `ImGui::TextUnformatted` (lines 1166–1189) with no text shadow effect. ImGui does not natively support text shadows, so a manual shadow would need to be rendered (draw text offset in black, then draw normal text on top).

- [ ] 369. [map-viewer.cpp] Map-viewer info bar spans missing CSS `margin: 0 10px` horizontal spacing
- **JS Source**: `src/app.css` lines 1345–1346
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .info span { margin: 0 10px; }` gives each info label 10px left/right margin. C++ uses `ImGui::SameLine()` between items (lines 1167–1175) with default spacing. The default ImGui item spacing is typically ~8px which may not exactly match the 20px total gap (10px + 10px) between spans.

- [ ] 370. [map-viewer.cpp] Map-viewer checkerboard background pattern not implemented
- **JS Source**: `src/app.css` lines 1300–1306
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer` has a complex checkerboard background using `background-image: linear-gradient(45deg, ...)` with `--trans-check-a` and `--trans-check-b` colors, `background-size: 30px 30px`, and `background-position: 0 0, 15px 15px`. C++ `renderWidget` (line 1148) uses `ImGui::BeginChild` with no custom background drawing — the checkerboard transparency pattern is missing entirely. The JS version shows a checkerboard behind transparent map tiles.

- [ ] 371. [map-viewer.cpp] Map-viewer hover-info positioned at top via ImGui layout instead of CSS `bottom: 3px; left: 3px`
- **JS Source**: `src/app.css` lines 1330–1333
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .hover-info` is positioned `bottom: 3px; left: 3px` (bottom-left corner of the map viewer). C++ renders hover-info at line 1187–1189 via `ImGui::SameLine()` after the info bar spans, placing it inline at the top. It should be rendered at the bottom-left of the map viewer container using `ImGui::SetCursorPos` or overlay drawing.

- [ ] 372. [map-viewer.cpp] Box-select-mode cursor not changed to crosshair
- **JS Source**: `src/app.css` lines 1351–1353
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer.box-select-mode { cursor: crosshair; }` changes the cursor to a crosshair when box-select mode is active. C++ does not call `ImGui::SetMouseCursor(ImGuiMouseCursor_Hand)` or any cursor change when `state.isBoxSelectMode` is true. ImGui does not have a native crosshair cursor, but this should at least document the visual difference.

- [ ] 373. [map-viewer.cpp] Tile rendering to canvas via GL textures not implemented — tiles cached in memory but not displayed
- **JS Source**: `src/js/components/map-viewer.js` lines 350–500 (loadTile, renderWithDoubleBuffer)
- **Status**: Pending
- **Details**: The JS version draws tiles to a `<canvas>` via `context.putImageData()` and `context.drawImage()`. C++ caches tile pixel data in `s_state.tilePixelCache` (map-viewer.cpp line 81) but never uploads it as OpenGL textures or renders it to the screen. The TODO comment at line 1194–1198 acknowledges this. The map overlay (selection highlights, hover) draws over empty space. This is a critical functional gap — the map tiles are invisible.

- [ ] 374. [map-viewer.cpp] `handleTileInteraction` emits selection changes via mutable reference instead of `$emit('update:selection')`
- **JS Source**: `src/js/components/map-viewer.js` lines 846–874
- **Status**: Pending
- **Details**: JS `handleTileInteraction` modifies `this.selection` array directly (splice/push) and Vue reactivity propagates changes via `v-model`. C++ modifies the `selection` vector reference directly (lines 845–848). However, JS Select All (line 812) uses `this.$emit('update:selection', newSelection)` with a new array — the C++ equivalent calls `onSelectionChanged(newSelection)` callback in `handleKeyPress` (line 780) and `finalizeBoxSelection` (line 941). This means `handleTileInteraction` mutates in-place but Select All and box-select create new arrays — potential inconsistency in selection update patterns.

- [ ] 375. [map-viewer.cpp] Map margin `20px 20px 0 10px` from CSS not applied
- **JS Source**: `src/app.css` line 1307
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer { margin: 20px 20px 0 10px; }` adds specific margins around the map viewer. C++ `renderWidget` uses `ImGui::GetContentRegionAvail()` for sizing (line 1144) without adding any padding/margin equivalent. The parent layout is responsible for this in ImGui, but if not handled there, the map viewer will be flush against adjacent elements.

- [ ] 376. [tab_home.cpp] Home showcase content is replaced with custom nav-card UI instead of the JS `HomeShowcase` component
- **JS Source**: `src/js/modules/tab_home.js` lines 4–5
- **Status**: Pending
- **Details**: JS renders `<HomeShowcase />`; C++ replaces that section with a custom navigation-card grid (`renderNavCard`/`renderHomeLayout`), changing the original home-tab content and visuals.

- [ ] 377. [tab_home.cpp] `whatsNewHTML` is rendered as plain text instead of HTML content
- **JS Source**: `src/js/modules/tab_home.js` line 6
- **Status**: Pending
- **Details**: JS uses `v-html="$core.view.whatsNewHTML"` to render formatted markup; C++ calls `ImGui::TextWrapped` on the raw string, so HTML formatting/links are not rendered.

- [ ] 378. [tab_home.cpp] "wow.export vX.X.X" title text is an invention not in original JS
- **JS Source**: `src/js/modules/tab_home.js` lines 2–23
- **Status**: Pending
- **Details**: C++ renders a large "wow.export vX.X.X" title in Row 1, Column 1. The original JS template has no such title. Row 1 of the original grid is the HomeShowcase header reading "Made with wow.export" (from home-showcase.js).

- [ ] 379. [tab_home.cpp] HomeShowcase component entirely replaced with navigation cards
- **JS Source**: `src/js/modules/tab_home.js` line 4
- **Status**: Pending
- **Details**: The JS HomeShowcase component loads showcase.json, displays multi-layered background images, optional video with autoplay, clickable links, a Refresh link, and a Feedback link. The C++ replaces ALL of this with a grid of navigation card shortcuts. This is a massive visual and functional deviation.

- [ ] 380. [tab_home.cpp] URLs hardcoded instead of using external-links system
- **JS Source**: `src/js/modules/tab_home.js` lines 9–19
- **Status**: Pending
- **Details**: JS uses data-external="::DISCORD", "::GITHUB", "::PATREON" which resolves through ExternalLinks. C++ hardcodes URLs directly as static constexpr strings instead of using external-links.h lookup table.

- [ ] 381. [tab_home.cpp] whatsNewHTML rendered as plain text instead of HTML
- **JS Source**: `src/js/modules/tab_home.js` line 6
- **Status**: Pending
- **Details**: JS uses v-html="$core.view.whatsNewHTML" which renders actual HTML markup (headings, paragraphs, bold, etc.). C++ uses ImGui::TextWrapped() which renders plain text only. HTML tags appear as raw text, not formatted content. CSS container-query responsive sizing is also absent.

- [ ] 382. [tab_home.cpp] #home-changes vertical padding differs from CSS
- **JS Source**: `src/js/modules/tab_home.js` lines 5–7
- **Status**: Pending
- **Details**: CSS specifies padding: 0 50px (zero vertical, 50px horizontal). C++ uses contentPadY=20.0f, adding 20px vertical padding not present in the original. CSS justify-content: center vertical centering is also missing.

- [ ] 383. [tab_home.cpp] Background image "cover" mode not implemented correctly
- **JS Source**: `src/js/modules/tab_home.js` line 5
- **Status**: Pending
- **Details**: CSS uses background center/cover which maintains aspect ratio while filling container, cropping overflow. C++ uses AddImageRounded with UV (0,0)-(1,1) which stretches the image to fill, distorting aspect ratio instead of cropping.

- [ ] 384. [tab_home.cpp] Help button icon size, position, and rotation differ from CSS
- **JS Source**: `src/js/modules/tab_home.js` lines 9–20
- **Status**: Pending
- **Details**: CSS ::before uses width/height: 120px, right: -20px, transform: rotate(20deg), opacity: 0.2. C++ uses iconSize=80px (should be 120), positions 10px from right edge (should extend 20px beyond), and has no rotation.

- [ ] 385. [tab_home.cpp] Missing hover transition animations on help buttons
- **JS Source**: `src/js/modules/tab_home.js` lines 8–21
- **Status**: Pending
- **Details**: CSS specifies transition: all 0.3s ease on help button divs and transition: transform 0.3s ease on ::before icon with hover scale(1.1). C++ has no transition/animation effects — changes are instantaneous.

- [ ] 386. [tab_home.cpp] Help button subtitle uses FONT_FADED instead of inheriting parent color
- **JS Source**: `src/js/modules/tab_home.js` lines 10–19
- **Status**: Pending
- **Details**: In JS, both <b> and <span> children inherit the same parent text color. On hover, both change to --nav-option-selected. C++ uses FONT_FADED for the subtitle, making it dimmer than the title.

- [ ] 387. [tab_home.cpp] #home-help-buttons grid-column full-width not fully replicated
- **JS Source**: `src/js/modules/tab_home.js` lines 8–21
- **Status**: Pending
- **Details**: CSS places help buttons spanning full width with grid-column: 1 / -1 and centers them with justify-content/align-items. C++ manually calculates centering with fixed card width of 940px. If the window is narrower, buttons push off-screen rather than wrapping.

- [ ] 388. [tab_home.cpp] #home-changes container query responsive font sizes not implemented
- **JS Source**: `src/js/modules/tab_home.js` line 6
- **Status**: Pending
- **Details**: CSS defines container-type: size with container query units: h1 font-size: clamp(16px, 4cqi, 28px) and p font-size: clamp(12px, 3cqi, 20px). C++ does not implement any responsive font sizing.

- [ ] 389. [tab_home.cpp] openInExplorer used for URL opening instead of external-links open
- **JS Source**: `src/js/modules/tab_home.js` lines 9–17
- **Status**: Pending
- **Details**: JS uses ExternalLinks.open() via data-external which calls nw.Shell.openExternal(). C++ calls core::openInExplorer(url). openInExplorer is typically for file paths/folders, while JS uses openExternal for URLs.

- [ ] 390. [tab_home.cpp] No cleanup/destruction of OpenGL textures
- **JS Source**: `src/js/modules/tab_home.js` lines N/A
- **Status**: Pending
- **Details**: C++ allocates OpenGL textures with glGenTextures and loadSvgTexture but has no cleanup/shutdown function to call glDeleteTextures. Textures leak if the tab is re-initialized or the application shuts down.

- [ ] 391. [legacy_tab_home.cpp] Legacy home tab template is replaced by shared `tab_home` layout
- **JS Source**: `src/js/modules/legacy_tab_home.js` lines 2–23
- **Status**: Pending
- **Details**: JS defines a dedicated legacy-home template structure (`#legacy-tab-home`, changelog HTML block, and external-link button rows), while C++ delegates to `tab_home::renderHomeLayout()`, so the legacy tab is not a line-by-line equivalent render path.

- [ ] 392. [legacy_tab_home.cpp] External link help buttons (Discord, GitHub, Patreon) not rendered
- **JS Source**: `src/js/modules/legacy_tab_home.js` lines 8–21
- **Status**: Pending
- **Details**: JS template includes 3 help buttons: Discord ("Stuck? Need Help?"), GitHub ("Gnomish Heritage?"), and Patreon ("Support Us!"), each with `data-external` links and description text. C++ `render()` delegates entirely to `tab_home::renderHomeLayout()` (line 28) and does not render these help buttons. Unless `renderHomeLayout()` includes them, these interactive elements are missing from the legacy home tab.

- [ ] 393. [tab_changelog.cpp] Changelog path resolution logic differs from JS two-path contract
- **JS Source**: `src/js/modules/tab_changelog.js` lines 14–16
- **Status**: Pending
- **Details**: JS uses `BUILD_RELEASE ? './src/CHANGELOG.md' : '../../CHANGELOG.md'`; C++ adds a third fallback (`CHANGELOG.md`) and different path probing order, changing source resolution behavior.

- [ ] 394. [tab_changelog.cpp] Changelog screen typography/layout diverges from JS `#changelog` template styling
- **JS Source**: `src/js/modules/tab_changelog.js` lines 31–35
- **Status**: Pending
- **Details**: JS uses dedicated `#changelog`/`#changelog-text` template structure and CSS styling; C++ renders plain ImGui title/separator/button layout, causing visible UI differences.

- [ ] 395. [tab_changelog.cpp] Heading rendered as plain ImGui::Text instead of styled h1
- **JS Source**: `src/js/modules/tab_changelog.js` line 32
- **Status**: Pending
- **Details**: JS uses h1 tag which renders as a large bold heading per CSS. C++ uses ImGui::Text with default font size and weight.

- [ ] 396. [tab_help.cpp] Search filtering no longer uses JS 300ms debounce behavior
- **JS Source**: `src/js/modules/tab_help.js` lines 145–149, 153–157
- **Status**: Pending
- **Details**: JS applies article filtering via `setTimeout(..., 300)` debounce on `search_query`; C++ filters immediately on each input change, changing responsiveness and update timing.

- [ ] 397. [tab_help.cpp] Help article list presentation differs from JS title/tag/KB layout
- **JS Source**: `src/js/modules/tab_help.js` lines 115–121
- **Status**: Pending
- **Details**: JS renders per-item title and a separate tags row with KB badge styling; C++ combines content into selectable labels and tooltip tags, so article list visuals/structure are not identical.

- [ ] 398. [tab_help.cpp] Missing 300ms debounce on search filter
- **JS Source**: `src/js/modules/tab_help.js` lines 145–149
- **Status**: Pending
- **Details**: JS implements a 300ms setTimeout debounce before filtering. C++ calls update_filter() immediately on every keystroke.

- [ ] 399. [tab_help.cpp] Article list layout differs tags shown in tooltip instead of inline
- **JS Source**: `src/js/modules/tab_help.js` lines 115–121
- **Status**: Pending
- **Details**: JS renders each article with visible title and tags divs inline. C++ renders KB_ID and title as a single Selectable with tags only in hover tooltip. Tags and KB ID badge are not visually inline.

- [ ] 400. [tab_help.cpp] load_help_docs is synchronous instead of async
- **JS Source**: `src/js/modules/tab_help.js` line 8
- **Status**: Pending
- **Details**: JS load_help_docs is async. C++ is synchronous blocking the main thread during file reads.

- [ ] 401. [tab_blender.cpp] Blender version gating semantics differ from JS string comparison behavior
- **JS Source**: `src/js/modules/tab_blender.js` lines 91, 139
- **Status**: Pending
- **Details**: JS compares versions as strings (`version >= MIN_VER`, `blender_version < MIN_VER`), while C++ parses with `std::stod` and compares numerically, changing edge-case ordering behavior.

- [ ] 402. [tab_blender.cpp] Blender install screen layout is not a pixel-equivalent port of JS markup
- **JS Source**: `src/js/modules/tab_blender.js` lines 59–69
- **Status**: Pending
- **Details**: JS uses structured `#blender-info`/`#blender-info-buttons` markup with CSS-defined spacing/styling; C++ replaces it with simple ImGui text/separator/buttons, producing visual/layout mismatch.

- [ ] 403. [tab_blender.cpp] get_blender_installations uses regex_match instead of regex_search
- **JS Source**: `src/js/modules/tab_blender.js` line 39
- **Status**: Pending
- **Details**: JS match() performs a search anywhere in string. C++ uses std::regex_match which requires the ENTIRE string to match. Directory names like blender-2.83 would match in JS but fail in C++.

- [ ] 404. [tab_blender.cpp] Blender version comparison uses numeric instead of string comparison
- **JS Source**: `src/js/modules/tab_blender.js` lines 91, 139
- **Status**: Pending
- **Details**: JS compares version strings lexicographically. C++ converts to double via std::stod() and compares numerically. Versions like 2.80 vs 2.8 would compare differently.

- [ ] 405. [tab_blender.cpp] start_automatic_install and checkLocalVersion are synchronous
- **JS Source**: `src/js/modules/tab_blender.js` lines 81, 127
- **Status**: Pending
- **Details**: Both JS functions are async with await. C++ implementations are synchronous, blocking the render thread.

- [ ] 406. [tab_install.cpp] Install listbox copy/paste options are hardcoded instead of using JS config-driven behavior
- **JS Source**: `src/js/modules/tab_install.js` lines 165, 184
- **Status**: Pending
- **Details**: JS listbox wiring uses `$core.view.config.copyMode`, `pasteSelection`, and `removePathSpacesCopy`; C++ passes `CopyMode::Default` with `pasteselection=false` and `copytrimwhitespace=false`, changing list interaction behavior.

- [ ] 407. [tab_install.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS renders `Regex Enabled` with `:title="$core.view.regexTooltip"`; C++ renders plain text without the tooltip contract, changing UI affordance.

- [ ] 408. [tab_install.cpp] Async operations converted to synchronous calls
- **JS Source**: `src/js/modules/tab_install.js` lines 52–146
- **Status**: Pending
- **Details**: JS export_install_files(), view_strings(), and export_strings() are all async functions that await CASC file I/O. C++ versions are fully synchronous, blocking the UI thread.

- [ ] 409. [tab_install.cpp] CASC getFile replaced with low-level two-step call, losing BLTE decoding
- **JS Source**: `src/js/modules/tab_install.js` lines 73–74
- **Status**: Pending
- **Details**: JS calls core.view.casc.getFile() which returns a BLTEReader for BLTE block decompression. C++ calls getEncodingKeyForContentKey then _ensureFileInCache then BufferWrapper::readFile, skipping BLTE decompression entirely. Exported files may be corrupt.

- [ ] 410. [tab_install.cpp] processAllBlocks() call missing in view_strings_impl
- **JS Source**: `src/js/modules/tab_install.js` lines 103–105
- **Status**: Pending
- **Details**: JS calls data.processAllBlocks() after getFile() to force all BLTE blocks decompressed before extract_strings. C++ skips this step because it uses plain BufferWrapper instead of BLTEReader.

- [ ] 411. [tab_install.cpp] export_install_files missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_install.js` line 78
- **Status**: Pending
- **Details**: JS calls helper.mark(file_name, false, e.message, e.stack) passing both message and stack trace. C++ passes only e.what(), omitting the stack trace parameter.

- [ ] 412. [tab_install.cpp] First listbox missing copyMode from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode" to the main install Listbox. C++ hardcodes listbox::CopyMode::Default instead of reading from view.config.

- [ ] 413. [tab_install.cpp] First listbox missing pasteSelection from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :pasteselection="$core.view.config.pasteSelection". C++ hardcodes false instead of reading from view.config.

- [ ] 414. [tab_install.cpp] First listbox missing copytrimwhitespace from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :copytrimwhitespace="$core.view.config.removePathSpacesCopy". C++ hardcodes false, disabling the remove-path-spaces-on-copy feature.

- [ ] 415. [tab_install.cpp] Second listbox (strings) missing copyMode from config
- **JS Source**: `src/js/modules/tab_install.js` line 184
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode" to the strings Listbox. C++ hardcodes listbox::CopyMode::Default.

- [ ] 416. [tab_install.cpp] First listbox unittype should be "install file" not "file"
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS template has unittype="install file". C++ passes "file". The status bar will show "X files found" instead of "X install files found".

- [ ] 417. [tab_install.cpp] Strings listbox nocopy incorrectly set to true
- **JS Source**: `src/js/modules/tab_install.js` line 184
- **Status**: Pending
- **Details**: The JS strings Listbox does NOT pass :nocopy (defaults to false). C++ passes true for nocopy, incorrectly disabling copy functionality in the strings list view.

- [ ] 418. [tab_install.cpp] Strings sidebar missing CSS styling equivalents
- **JS Source**: `src/js/modules/tab_install.js` lines 194–197
- **Status**: Pending
- **Details**: CSS defines .strings-header font-size 14px opacity 0.7, .strings-filename font-size 12px word-break: break-all, 5px gap. C++ uses default font sizes and ImGui::Spacing() which may not match.

- [ ] 419. [tab_install.cpp] Input placeholder text not rendered
- **JS Source**: `src/js/modules/tab_install.js` lines 170, 189
- **Status**: Pending
- **Details**: JS filter inputs have placeholder="Filter install files..." and "Filter strings...". C++ uses plain ImGui::InputText without hint/placeholder.

- [ ] 420. [tab_install.cpp] Regex tooltip not rendered
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS "Regex Enabled" div has :title="$core.view.regexTooltip" showing tooltip on hover. C++ has no tooltip implementation.

- [ ] 421. [tab_install.cpp] extract_strings and update_install_listfile exposed in header but should be file-local
- **JS Source**: `src/js/modules/tab_install.js` lines 16, 41
- **Status**: Pending
- **Details**: In JS, extract_strings and update_install_listfile are file-local (not exported). C++ header exposes them publicly. They should be static in the .cpp and removed from the header.

- [ ] 422. [tab_models.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_models.js` line 296
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 423. [tab_models.cpp] preview_model and export_files are synchronous instead of async
- **JS Source**: `src/js/modules/tab_models.js` lines 61, 180
- **Status**: Pending
- **Details**: JS preview_model and export_files are async using await for CASC reads and renderer operations. C++ versions are fully synchronous, blocking UI thread.

- [ ] 424. [tab_models.cpp] create_renderer last parameter is file_data_id instead of file_name
- **JS Source**: `src/js/modules/tab_models.js` line 99
- **Status**: Pending
- **Details**: JS passes file_name as last argument. C++ passes file_data_id (uint32_t). If renderer uses this for name-based logic (e.g. fallback texture paths), it would fail.

- [ ] 425. [tab_models.cpp] M3 has_content hardcoded to true instead of checking draw_calls/groups
- **JS Source**: `src/js/modules/tab_models.js` line 148
- **Status**: Pending
- **Details**: JS checks active_renderer.draw_calls?.length > 0 || active_renderer.groups?.length > 0 for all types. C++ hardcodes M3 as always having content. M3 models with no 3D data won't show the "no 3D data" toast.

- [ ] 426. [tab_models.cpp] Missing "View Log" button in generic error toast
- **JS Source**: `src/js/modules/tab_models.js` line 163
- **Status**: Pending
- **Details**: JS passes { 'View Log': () => log.openRuntimeLog() } as toast buttons on preview failure. C++ passes empty {}, so user has no way to open the runtime log from error toast.

- [ ] 427. [tab_models.cpp] path.basename strips differently than C++ extension removal
- **JS Source**: `src/js/modules/tab_models.js` line 107
- **Status**: Pending
- **Details**: JS path.basename(model_name, 'm2') strips literal 'm2' leaving trailing dot (e.g. 'creature.'). C++ strips at last dot giving 'creature' (no trailing dot). Affects skin name replacement results.

- [ ] 428. [tab_models.cpp] Drop handler prompt lambda missing count parameter
- **JS Source**: `src/js/modules/tab_models.js` line 580
- **Status**: Pending
- **Details**: JS prompt receives count parameter: count => util.format('Export %d models as %s', count, ...). C++ lambda returns string without using/accepting a count, so prompt won't show number of files.

- [ ] 429. [tab_models.cpp] Model quick filters not passed to listbox
- **JS Source**: `src/js/modules/tab_models.js` line 286
- **Status**: Pending
- **Details**: JS passes :quickfilters="$core.view.modelQuickFilters" to Listbox. C++ passes empty {}. Quick filter buttons won't appear.

- [ ] 430. [tab_models.cpp] Missing "Regex Enabled" indicator in filter bar
- **JS Source**: `src/js/modules/tab_models.js` line 296
- **Status**: Pending
- **Details**: JS renders regex-info div with tooltip when config.regexFilters is true. C++ filter bar only renders the text input, completely omitting the regex indicator and tooltip.

- [ ] 431. [tab_models.cpp] Sidebar checkboxes missing tooltip text
- **JS Source**: `src/js/modules/tab_models.js` lines 358–425
- **Status**: Pending
- **Details**: Every JS checkbox has a title attribute for contextual help. C++ checkboxes have no ImGui tooltip equivalents, losing all tooltip help text.

- [ ] 432. [tab_models.cpp] WMO Groups uses raw checkboxes instead of CheckboxList component
- **JS Source**: `src/js/modules/tab_models.js` line 440
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for WMO groups. C++ uses manual loop of ImGui::Checkbox instead of checkboxlist::render(), causing visual/behavioral inconsistency.

- [ ] 433. [tab_models.cpp] WMO Doodad Sets uses raw checkboxes instead of CheckboxList component
- **JS Source**: `src/js/modules/tab_models.js` line 445
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for doodad sets. C++ uses individual ImGui::Checkbox in a loop instead of checkboxlist::render().

- [ ] 434. [tab_models.cpp] getActiveRenderer() only returns M2, not polymorphic like JS
- **JS Source**: `src/js/modules/tab_models.js` line 652
- **Status**: Pending
- **Details**: JS returns active_renderer regardless of type (M2/M3/WMO). C++ always returns active_renderer_result.m2.get() which is nullptr for M3/WMO. External callers get nullptr for non-M2 models.

- [ ] 435. [tab_models.cpp] enableM2Skins config default may differ
- **JS Source**: `src/js/modules/tab_models.js` line 548
- **Status**: Pending
- **Details**: JS checks config.enableM2Skins with truthiness (undefined = false). C++ uses view.config.value("enableM2Skins", true) defaulting to true. Different behavior on first run when key not set.

- [ ] 436. [tab_models.cpp] helper.mark on failure missing stack trace parameter
- **JS Source**: `src/js/modules/tab_models.js` line 269
- **Status**: Pending
- **Details**: JS calls helper.mark(file_name, false, e.message, e.stack) with 4 args. C++ calls helper.mark(file_name, false, e.what()) with only 3, losing stack trace.

- [ ] 437. [tab_models.cpp] MenuButton missing "upward" class styling
- **JS Source**: `src/js/modules/tab_models.js` line 354
- **Status**: Pending
- **Details**: JS MenuButton has class="upward" making dropdown open upward. C++ menu_button::render doesn't pass any upward/direction flag, so dropdown may render downward and overlap content.

- [ ] 438. [tab_models.cpp] Texture ribbon slot click behavior differs from JS
- **JS Source**: `src/js/modules/tab_models.js` line 302
- **Status**: Pending
- **Details**: JS uses @click (left-click) to open context menu. C++ uses both right-click and left-click to open the same popup. JS only uses left-click for texture ribbon — right-click is not handled.

- [ ] 439. [tab_models_legacy.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 340
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 440. [tab_models_legacy.cpp] WMOLegacyRendererGL constructor passes 0 instead of file_name
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 86
- **Status**: Pending
- **Details**: JS passes (data, file_name, gl_context, showTextures). C++ passes (data, 0, *gl_ctx, showTextures). WMO renderer needs file_name for group file path resolution. Passing 0 is incorrect.

- [ ] 441. [tab_models_legacy.cpp] Missing "View Log" button in preview_model error toast
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 185
- **Status**: Pending
- **Details**: JS provides { 'View Log': () => log.openRuntimeLog() } as toast action. C++ passes empty {}, losing the user-facing button.

- [ ] 442. [tab_models_legacy.cpp] Missing requestAnimationFrame deferral for fitCamera
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 182
- **Status**: Pending
- **Details**: JS wraps fitCamera in requestAnimationFrame() for next-frame deferral. C++ calls fitCamera() synchronously, which may execute before render state is fully set up.

- [ ] 443. [tab_models_legacy.cpp] PNG/CLIPBOARD export_paths stream not written for PNG exports
- **JS Source**: `src/js/modules/tab_models_legacy.js` lines 197–224
- **Status**: Pending
- **Details**: JS writes 'PNG:' + out_file to export_paths stream. C++ delegates to model_viewer_utils::export_preview which opens its own FileWriter. The export_files-level stream gets no entries for PNG exports.

- [ ] 444. [tab_models_legacy.cpp] helper.mark on failure missing stack trace argument
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 311
- **Status**: Pending
- **Details**: JS calls helper.mark(file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 445. [tab_models_legacy.cpp] Listbox missing quickfilters from view
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 332
- **Status**: Pending
- **Details**: JS passes :quickfilters="$core.view.legacyModelQuickFilters" (which is ['m2','mdx','wmo']). C++ passes empty {}. Quick filter buttons won't appear.

- [ ] 446. [tab_models_legacy.cpp] Listbox missing copyMode config binding
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 332
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode". C++ hardcodes listbox::CopyMode::Default.

- [ ] 447. [tab_models_legacy.cpp] Listbox missing pasteSelection config binding
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 332
- **Status**: Pending
- **Details**: JS passes :pasteselection="$core.view.config.pasteSelection". C++ hardcodes false.

- [ ] 448. [tab_models_legacy.cpp] Listbox missing copytrimwhitespace config binding
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 332
- **Status**: Pending
- **Details**: JS passes :copytrimwhitespace="$core.view.config.removePathSpacesCopy". C++ hardcodes false.

- [ ] 449. [tab_models_legacy.cpp] Missing "Regex Enabled" indicator in filter bar
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 340
- **Status**: Pending
- **Details**: JS shows regex-info div with tooltip when config.regexFilters is true. C++ filter bar only renders text input, no regex indicator.

- [ ] 450. [tab_models_legacy.cpp] Filter input missing placeholder text
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 341
- **Status**: Pending
- **Details**: JS uses placeholder="Filter models...". C++ ImGui::InputText has no hint text. Should use InputTextWithHint.

- [ ] 451. [tab_models_legacy.cpp] All sidebar checkboxes missing tooltip text
- **JS Source**: `src/js/modules/tab_models_legacy.js` lines 377–399
- **Status**: Pending
- **Details**: JS has title="..." on every sidebar checkbox (6 items). C++ renders none of these tooltips.

- [ ] 452. [tab_models_legacy.cpp] step/seek/start_scrub/end_scrub only handle M2, not MDX
- **JS Source**: `src/js/modules/tab_models_legacy.js` lines 462–496
- **Status**: Pending
- **Details**: JS uses optional chaining on single active_renderer for animation methods. C++ only checks active_renderer_m2 — MDX renderer is ignored for all animation control operations.

- [ ] 453. [tab_models_legacy.cpp] WMO Groups rendered with raw Checkbox instead of Checkboxlist component
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 414
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for WMO groups. C++ uses manual loop of ImGui::Checkbox. Checkboxlist may have additional styling/behavior.

- [ ] 454. [tab_models_legacy.cpp] Doodad Sets rendered with raw Checkbox instead of Checkboxlist component
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 419
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for doodad sets. C++ uses raw ImGui::Checkbox loop.

- [ ] 455. [tab_models_legacy.cpp] getActiveRenderer() only returns M2, not active renderer
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 592
- **Status**: Pending
- **Details**: JS returns active_renderer which could be M2, WMO, or MDX. C++ always returns active_renderer_m2.get(), returning nullptr when active model is WMO or MDX.

- [ ] 456. [tab_models_legacy.cpp] preview_model and export_files are synchronous instead of async
- **JS Source**: `src/js/modules/tab_models_legacy.js` lines 42, 191
- **Status**: Pending
- **Details**: JS preview_model and export_files are async with await. C++ versions are fully synchronous, blocking UI thread for expensive operations.

- [ ] 457. [tab_models_legacy.cpp] MenuButton missing "upward" class/direction
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 373
- **Status**: Pending
- **Details**: JS uses class="upward" on MenuButton so dropdown opens upward. C++ menu_button::render doesn't pass upward/direction flag.

- [ ] 458. [tab_textures.cpp] Baked NPC texture apply path stores a file data ID instead of the JS BLP object
- **JS Source**: `src/js/modules/tab_textures.js` lines 423–427
- **Status**: Pending
- **Details**: JS loads the selected texture file and stores a `BLPFile` instance in `chrCustBakedNPCTexture`; C++ stores only the resolved file data ID, changing downstream data shape/behavior.

- [ ] 459. [tab_textures.cpp] Baked NPC texture failure toast omits JS `view log` action callback
- **JS Source**: `src/js/modules/tab_textures.js` lines 430–431
- **Status**: Pending
- **Details**: JS error toast includes `{ 'view log': () => log.openRuntimeLog() }`; C++ error toast has no action handlers, removing the original troubleshooting entry point.

- [ ] 460. [tab_textures.cpp] Texture channel controls are rendered as checkboxes instead of JS channel chips
- **JS Source**: `src/js/modules/tab_textures.js` lines 306–311
- **Status**: Pending
- **Details**: JS uses styled `li` channel chips (`R/G/B/A`) with selected-state classes; C++ renders standard ImGui checkboxes, causing visible control-style differences.

- [ ] 461. [tab_textures.cpp] Listbox override texture list not forwarded
- **JS Source**: `src/js/modules/tab_textures.js` line 291
- **Status**: Pending
- **Details**: JS passes :override="$core.view.overrideTextureList" to Listbox. C++ passes nullptr for overrideItems. When another tab sets an override texture list, the listbox will ignore it entirely.

- [ ] 462. [tab_textures.cpp] MenuButton replaced with plain Button — no format dropdown
- **JS Source**: `src/js/modules/tab_textures.js` line 328
- **Status**: Pending
- **Details**: JS uses MenuButton component with :options providing dropdown to change export format (PNG/WEBP/BLP). C++ uses plain ImGui::Button showing only current format — no way to change export texture format from this tab's UI.

- [ ] 463. [tab_textures.cpp] apply_baked_npc_texture skips CASC file load and BLP creation
- **JS Source**: `src/js/modules/tab_textures.js` lines 421–426
- **Status**: Pending
- **Details**: JS loads file from CASC, creates BLPFile, stores BLP object in chrCustBakedNPCTexture. C++ just stores the raw file data ID integer without loading or decoding. Downstream consumers expecting decoded BLP will receive only an integer.

- [ ] 464. [tab_textures.cpp] Missing "View Log" action button on baked NPC texture error toast
- **JS Source**: `src/js/modules/tab_textures.js` line 430
- **Status**: Pending
- **Details**: JS error toast passes { 'view log': () => log.openRuntimeLog() }. C++ passes empty {}.

- [ ] 465. [tab_textures.cpp] Atlas overlay regions not cleared when atlas_id found but entry missing
- **JS Source**: `src/js/modules/tab_textures.js` lines 184–213
- **Status**: Pending
- **Details**: JS always assigns textureAtlasOverlayRegions = render_regions unconditionally after the if(entry) block. C++ doesn't clear regions when texture_atlas_map has file_data_id but texture_atlas_entries lacks atlas_id, leaving stale data.

- [ ] 466. [tab_textures.cpp] Drop handler prompt omits file count
- **JS Source**: `src/js/modules/tab_textures.js` line 468
- **Status**: Pending
- **Details**: JS prompt includes count parameter: count => util.format('Export %d textures as %s', count, ...). C++ lambda takes no count and produces "Export textures as PNG" without count.

- [ ] 467. [tab_textures.cpp] Atlas region tooltip positioning not implemented
- **JS Source**: `src/js/modules/tab_textures.js` lines 148–182
- **Status**: Pending
- **Details**: JS attach_overlay_listener adds mousemove handler for dynamic tooltip repositioning based on mouse position (4 CSS tooltip classes). C++ draws region name text at fixed position with no hover interaction.

- [ ] 468. [tab_textures.cpp] Filter input missing placeholder text
- **JS Source**: `src/js/modules/tab_textures.js` line 302
- **Status**: Pending
- **Details**: JS uses placeholder="Filter textures...". C++ uses ImGui::InputText with no hint text.

- [ ] 469. [tab_textures.cpp] Regex tooltip text missing
- **JS Source**: `src/js/modules/tab_textures.js` line 301
- **Status**: Pending
- **Details**: JS shows :title="$core.view.regexTooltip" on "Regex Enabled" div. C++ has no tooltip.

- [ ] 470. [tab_textures.cpp] export_texture_atlas_regions missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_textures.js` line 261
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 471. [tab_textures.cpp] All async operations are synchronous — blocks UI thread
- **JS Source**: `src/js/modules/tab_textures.js` lines 23–413
- **Status**: Pending
- **Details**: JS preview_texture_by_id, load_texture_atlas_data, reload_texture_atlas_data, export_texture_atlas_regions, export_textures, initialize, and apply_baked_npc_texture are all async. C++ equivalents all run synchronously on UI thread.

- [ ] 472. [tab_textures.cpp] Channel mask toggles rendered as checkboxes instead of styled inline buttons
- **JS Source**: `src/js/modules/tab_textures.js` lines 306–311
- **Status**: Pending
- **Details**: JS renders channel toggles as <li> items with colored backgrounds/borders using .selected CSS class. C++ uses ImGui::Checkbox with colored check marks — visually different from original compact colored square buttons.

- [ ] 473. [tab_textures.cpp] Preview image max dimensions not clamped to texture dimensions
- **JS Source**: `src/js/modules/tab_textures.js` line 312
- **Status**: Pending
- **Details**: JS applies max-width/max-height from texture dimensions so preview never upscales beyond native resolution. C++ computes scale = min(avail/tex) which upscales small textures to fill the panel.

- [ ] 474. [legacy_tab_textures.cpp] Listbox context menu render path from JS template is missing
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 118–122, 147–161
- **Status**: Pending
- **Details**: JS mounts a `ContextMenu` component for texture list selections, but C++ never calls `context_menu::render(...)` in `render()`, leaving the expected right-click action menu unrendered.

- [ ] 475. [legacy_tab_textures.cpp] Channel toggle visuals/interaction differ from JS channel list UI
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 130–135
- **Status**: Pending
- **Details**: JS uses styled `<li>` channel pills with selected classes (`R/G/B/A`), while C++ uses standard ImGui checkboxes, producing non-identical visuals and interaction behavior.

- [ ] 476. [legacy_tab_textures.cpp] PNG/JPG preview info shows lowercase extension vs JS uppercase
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 58
- **Status**: Pending
- **Details**: JS formats PNG/JPG info as `${ext.slice(1).toUpperCase()}` (e.g., "256x256 (PNG)"). C++ uses `ext.substr(1)` without uppercasing (line 132), producing "256x256 (png)". Minor visual inconsistency in the preview info text.

- [ ] 477. [legacy_tab_textures.cpp] Listbox hardcodes `pasteselection` and `copytrimwhitespace` to false vs JS config values
- **JS Source**: `src/js/modules/legacy_tab_textures.js` line 117
- **Status**: Pending
- **Details**: JS Listbox passes `:pasteselection="$core.view.config.pasteSelection"` and `:copytrimwhitespace="$core.view.config.removePathSpacesCopy"` from user config. C++ hardcodes both to `false` (lines 284–285). User settings for paste selection and whitespace trimming are ignored in the legacy textures tab.

- [ ] 478. [legacy_tab_textures.cpp] Listbox hardcodes `CopyMode::Default` vs JS `$core.view.config.copyMode`
- **JS Source**: `src/js/modules/legacy_tab_textures.js` line 117
- **Status**: Pending
- **Details**: JS Listbox passes `:copymode="$core.view.config.copyMode"` so the user's copy mode preference (Full/DIR/FID) is respected. C++ hardcodes `listbox::CopyMode::Default` (line 283). The user's configured copy mode is ignored.

- [ ] 479. [legacy_tab_textures.cpp] Channel checkboxes always visible vs JS conditional on `texturePreviewURL`
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 130–135
- **Status**: Pending
- **Details**: JS only shows channel toggle buttons (`<ul class="preview-channels">`) when `$core.view.texturePreviewURL.length > 0`. C++ always renders the R/G/B/A checkboxes regardless of whether a texture is loaded (lines 329–350). Before any texture is selected, the channel controls are visible but non-functional.

- [ ] 480. [tab_audio.cpp] Audio quick-filter list path is missing from listbox wiring
- **JS Source**: `src/js/modules/tab_audio.js` line 190
- **Status**: Pending
- **Details**: JS passes `$core.view.audioQuickFilters` into the sound listbox. C++ passes an empty quickfilter list (`{}`), so the original quick-filter behavior is not available.

- [ ] 481. [tab_audio.cpp] `unload_track` no longer revokes preview URL data like JS
- **JS Source**: `src/js/modules/tab_audio.js` lines 95–97
- **Status**: Pending
- **Details**: JS explicitly calls `file_data?.revokeDataURL()` and clears `file_data`; C++ has no equivalent revoke path, changing cleanup behavior for previewed track resources.

- [ ] 482. [tab_audio.cpp] Sound player visuals differ from the JS template/CSS implementation
- **JS Source**: `src/js/modules/tab_audio.js` lines 203–228
- **Status**: Pending
- **Details**: JS renders `#sound-player-anim`, custom component sliders, and CSS-styled play-state button classes; C++ uses plain ImGui button/slider widgets and a different animated icon presentation, so visuals are not identical.

- [ ] 483. [tab_audio.cpp] play_track uses get_duration() <= 0 instead of checking buffer existence
- **JS Source**: `src/js/modules/tab_audio.js` lines 100–101
- **Status**: Pending
- **Details**: JS checks !player.buffer to determine if a track is loaded. C++ checks player.get_duration() <= 0. A loaded but zero-duration track would behave differently.

- [ ] 484. [tab_audio.cpp] Missing audioQuickFilters prop on Listbox
- **JS Source**: `src/js/modules/tab_audio.js` line 190
- **Status**: Pending
- **Details**: JS passes quickfilters to the Listbox component. C++ passes empty {}. Quick-filter buttons for audio file types will not appear.

- [ ] 485. [tab_audio.cpp] unittype is "sound" instead of "sound file"
- **JS Source**: `src/js/modules/tab_audio.js` line 190
- **Status**: Pending
- **Details**: JS sets unittype to "sound file". C++ passes "sound". The file count display text differs.

- [ ] 486. [tab_audio.cpp] export_sounds missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_audio.js` line 175
- **Status**: Pending
- **Details**: JS passes e.message and e.stack to helper.mark(). C++ only passes e.what().

- [ ] 487. [tab_audio.cpp] load_track play_track and export_sounds are synchronous blocking main thread
- **JS Source**: `src/js/modules/tab_audio.js` lines 47, 99, 122
- **Status**: Pending
- **Details**: All three JS functions are async with await. C++ implementations are synchronous, blocking the render thread.

- [ ] 488. [legacy_tab_audio.cpp] Playback UI visuals diverge from JS template/CSS
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 201–241
- **Status**: Pending
- **Details**: JS renders `#sound-player-anim`, CSS-styled play button state classes, and component sliders, while C++ replaces this with ImGui text/buttons/checkboxes and a custom icon pulse, so layout/styling is not pixel-identical.

- [ ] 489. [legacy_tab_audio.cpp] Seek-loop scheduling differs from JS `requestAnimationFrame` lifecycle
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 19–42
- **Status**: Pending
- **Details**: JS drives seek updates with `requestAnimationFrame` and explicit cancellation IDs; C++ updates via render-loop polling with `seek_loop_active`, changing timing and loop lifecycle semantics.

- [ ] 490. [legacy_tab_audio.cpp] Context menu adds FileDataID-related items not present in JS legacy audio template
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 205–209
- **Status**: Pending
- **Details**: JS context menu has 3 items: "Copy file path(s)", "Copy export path(s)", "Open export directory". C++ adds conditional "Copy file path(s) (listfile format)" and "Copy file data ID(s)" when `hasFileDataIDs` is true (lines 399–402). Legacy MPQ files don't have FileDataIDs, so these extra menu items are incorrect for the legacy audio tab.

- [ ] 491. [legacy_tab_audio.cpp] Sound player info combines seek/title/duration into single Text call vs JS 3 separate spans
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 218–222
- **Status**: Pending
- **Details**: JS renders sound player info as 3 separate `<span>` elements in a flex container: seek formatted time, title (with CSS class "title"), and duration formatted time. C++ combines them into a single `ImGui::Text("%s  %s  %s", ...)` call (line 453). The title won't have distinct styling, and the layout/alignment will differ from the JS flex row.

- [ ] 492. [legacy_tab_audio.cpp] Play/Pause uses text toggle ("Play"/"Pause") vs JS CSS class-based visual toggle
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 225
- **Status**: Pending
- **Details**: JS uses `<input type="button" :class="{ isPlaying: !soundPlayerState }">` — the button appearance changes via CSS class (likely showing a play/pause icon). C++ uses `ImGui::Button(view.soundPlayerState ? "Pause" : "Play")` with text labels. The visual appearance differs significantly from the original icon-based toggle.

- [ ] 493. [legacy_tab_audio.cpp] Volume slider is ImGui::SliderFloat with format string vs JS custom Slider component
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 226
- **Status**: Pending
- **Details**: JS uses a custom `<Slider>` component with id "slider-volume" for volume control. C++ uses `ImGui::SliderFloat` with format "Vol: %.0f%%" (line 474). The custom JS Slider has its own visual styling defined in CSS; the ImGui slider will look different (default ImGui styling vs themed slider).

- [ ] 494. [legacy_tab_audio.cpp] Loop/Autoplay checkboxes placed in preview container instead of preview-controls div
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 231–239
- **Status**: Pending
- **Details**: JS places Loop/Autoplay checkboxes and Export button together in the `preview-controls` div. C++ places Loop/Autoplay in the `PreviewContainer` section (lines 479–487) and Export in `PreviewControls` (lines 492–498). This changes the visual layout — checkboxes are above the export button area instead of beside it.

- [ ] 495. [legacy_tab_audio.cpp] `load_track` checks `player.get_duration() <= 0` vs JS `!player.buffer`
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 100–101
- **Status**: Pending
- **Details**: JS `play_track` checks `!player.buffer` to determine if a track needs loading. C++ checks `player.get_duration() <= 0` (line 136). If a loaded track has zero duration (e.g., corrupt file that loads but has 0-length), C++ would re-load while JS would not. The check semantics are subtly different.

- [ ] 496. [legacy_tab_audio.cpp] `export_sounds` `helper.mark` doesn't pass error stack trace
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 189
- **Status**: Pending
- **Details**: JS calls `helper.mark(export_file_name, false, e.message, e.stack)` with 4 arguments including the stack trace. C++ calls `helper.mark(export_file_name, false, e.what())` with only 3 arguments (line 239). Error stack information is lost in C++ export failure reports.

- [ ] 497. [legacy_tab_audio.cpp] Filter input missing placeholder text "Filter sound files..."
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 213
- **Status**: Pending
- **Details**: JS filter input has `placeholder="Filter sound files..."`. C++ `ImGui::InputText("##FilterLegacySounds", ...)` (line 420) has no placeholder/hint text. The empty filter field won't show the helpful hint.

- [ ] 498. [tab_videos.cpp] Video preview playback is opened externally instead of using an embedded player
- **JS Source**: `src/js/modules/tab_videos.js` lines 219–276, 493
- **Status**: Pending
- **Details**: JS renders and controls an in-tab `<video>` element with `onended`/`onerror` and subtitle track attachment, while C++ opens the stream URL in an external handler and shows status text in the preview area.

- [ ] 499. [tab_videos.cpp] Video export format selector from MenuButton is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 505, 559–571
- **Status**: Pending
- **Details**: JS uses a `MenuButton` bound to `config.exportVideoFormat` and dispatches format-specific export via selection; C++ renders a single `Export Selected` button with no in-UI format picker.

- [ ] 500. [tab_videos.cpp] Kino processing toast omits the explicit Cancel action payload
- **JS Source**: `src/js/modules/tab_videos.js` lines 394–400
- **Status**: Pending
- **Details**: JS updates progress toast with `{ 'Cancel': cancel_processing }`; C++ calls `setToast(..., {}, ...)`, removing the explicit cancel action binding from the toast configuration.

- [ ] 501. [tab_videos.cpp] Dev-mode kino processing trigger export is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 467–469
- **Status**: Pending
- **Details**: JS exposes `window.trigger_kino_processing = trigger_kino_processing` in non-release mode; C++ has no equivalent debug export hook.

- [ ] 502. [tab_videos.cpp] Corrupted AVI fallback does not force CASC fallback fetch path
- **JS Source**: `src/js/modules/tab_videos.js` line 697
- **Status**: Pending
- **Details**: JS retries corrupted cinematic reads with `getFileByName(file_name, false, false, true, true)` to force fallback behavior; C++ retries `getVirtualFileByName(file_name)` with normal arguments.

- [ ] 503. [tab_videos.cpp] MenuButton export format dropdown completely missing
- **JS Source**: `src/js/modules/tab_videos.js` line 505
- **Status**: Pending
- **Details**: JS uses `<MenuButton :options="menuButtonVideos" :default="config.exportVideoFormat" @change="..." @click="export_selected">` which renders a dropdown to pick MP4/AVI/MP3/SUBTITLES and triggers export. C++ renders a plain `ImGui::Button("Export Selected")` with no format selector. Users cannot change the export format from this tab.

- [ ] 504. [tab_videos.cpp] AVI export corruption fallback is a no-op
- **JS Source**: `src/js/modules/tab_videos.js` line 697
- **Status**: Pending
- **Details**: JS calls `getFileByName(file_name, false, false, true, true)` with extra params (forceFallback). C++ calls `getVirtualFileByName(file_name)` identically to the first attempt, with a comment admitting `// Note: C++ getVirtualFileByName doesn't support forceFallback; retry normally.` The corruption recovery path will always fail the same way twice.

- [ ] 505. [tab_videos.cpp] Video preview is text-only, not an embedded player
- **JS Source**: `src/js/modules/tab_videos.js` line 493
- **Status**: Pending
- **Details**: JS renders a `<video>` element with full controls, autoplay, subtitles overlay via `<track>`. C++ opens the URL in the system's external media player (`core::openInExplorer(url)`) and shows plain text. No inline playback, no controls, no subtitle overlay in the app window.

- [ ] 506. [tab_videos.cpp] No onended/onerror callbacks for video playback
- **JS Source**: `src/js/modules/tab_videos.js` lines 263–275
- **Status**: Pending
- **Details**: JS attaches `video.onended` (resets `is_streaming`/`videoPlayerState`) and `video.onerror` (shows error toast). C++ delegates to external player and has neither callback — `is_streaming` and `videoPlayerState` are never automatically reset when playback finishes; user must manually click "Stop Video."

- [ ] 507. [tab_videos.cpp] build_payload runs on main thread, blocking UI
- **JS Source**: `src/js/modules/tab_videos.js` line 125
- **Status**: Pending
- **Details**: JS `build_payload` is `async`/`await` (non-blocking). In C++, it's called synchronously on the main thread before launching the background thread. DB2 queries + CASC lookups could freeze the UI.

- [ ] 508. [tab_videos.cpp] stop_video does not join/stop background thread
- **JS Source**: `src/js/modules/tab_videos.js` lines 27–57
- **Status**: Pending
- **Details**: JS clears `setTimeout` handle which fully cancels pending work. C++ sets `poll_cancelled = true` but does not `reset()` or join `stream_worker_thread`. The thread may still be running and post results after stop. Only `stream_video` joins it before a new stream.

- [ ] 509. [tab_videos.cpp] MP4 download HTTP error check missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 631–633
- **Status**: Pending
- **Details**: JS explicitly checks `if (!response.ok)` and marks the file with 'Failed to download MP4: ' + response.status. C++ uses `generics::get(*mp4_url)` with no status check — if the server returns a non-200 status, behavior depends on `generics::get()` implementation.

- [ ] 510. [tab_videos.cpp] All helper.mark error calls missing stack trace argument
- **JS Source**: `src/js/modules/tab_videos.js` lines 642, 690, 702, 763, 822
- **Status**: Pending
- **Details**: JS passes `(file, false, e.message, e.stack)` (4 args) for error marking. C++ passes only `(file, false, e.what())` (3 args). Stack traces are lost in every export error path (MP4, AVI×2, MP3, Subtitles).

- [ ] 511. [tab_videos.cpp] stream_video outer catch missing stack trace log
- **JS Source**: `src/js/modules/tab_videos.js` line 214
- **Status**: Pending
- **Details**: JS logs `log.write(e.stack)` separately from the error message. C++ only logs `e.what()`.

- [ ] 512. [tab_videos.cpp] Cancel button missing from kino_processing toast
- **JS Source**: `src/js/modules/tab_videos.js` line 399
- **Status**: Pending
- **Details**: JS passes `{ 'Cancel': cancel_processing }` as toast buttons. C++ passes `{}` — no Cancel button is shown during batch processing, leaving users with no way to cancel.

- [ ] 513. [tab_videos.cpp] Regex tooltip missing on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_videos.js` line 489
- **Status**: Pending
- **Details**: JS shows `:title="$core.view.regexTooltip"` tooltip on the "Regex Enabled" text. C++ renders `ImGui::TextUnformatted("Regex Enabled")` with no tooltip.

- [ ] 514. [tab_videos.cpp] Spurious "Connecting to video server..." toast not in JS
- **JS Source**: `src/js/modules/tab_videos.js` (none)
- **Status**: Pending
- **Details**: JS shows no toast before the initial HTTP request — it only shows "Video is being processed..." on 202 status. C++ always shows a "Connecting to video server..." progress toast before the request, which is not in the original.

- [ ] 515. [tab_videos.cpp] "View Log" button text capitalization differs from JS
- **JS Source**: `src/js/modules/tab_videos.js` lines 195, 215
- **Status**: Pending
- **Details**: JS uses lowercase `'view log'`. C++ uses title case `"View Log"`.

- [ ] 516. [tab_videos.cpp] Filter input buffer capped at 255 chars
- **JS Source**: `src/js/modules/tab_videos.js` line 490
- **Status**: Pending
- **Details**: JS `v-model` has no character limit. C++ uses `char filter_buf[256]` which truncates filter input at 255 characters.

- [ ] 517. [tab_videos.cpp] kino_post hardcodes hostname and path instead of using constant
- **JS Source**: `src/js/modules/tab_videos.js` lines 137, 349, 431
- **Status**: Pending
- **Details**: JS uses `constants.KINO.API_URL` dynamically via `fetch()`. C++ hardcodes `httplib::SSLClient cli("www.kruithne.net")` and `.Post("/wow.export/v2/get_video", ...)` instead of parsing the constant. If the constant changes, C++ won't reflect it.

- [ ] 518. [tab_videos.cpp] Subtitle loading uses different API path than JS
- **JS Source**: `src/js/modules/tab_videos.js` lines 226–230
- **Status**: Pending
- **Details**: JS calls `subtitles.get_subtitles_vtt(core_ref.view.casc, subtitle_info.file_data_id, subtitle_info.format)` which fetches+converts internally. C++ manually fetches via `casc->getVirtualFileByID()`, reads as string, then calls `subtitles::get_subtitles_vtt(raw_subtitle_text, fmt)`. Different function signature — caller now responsible for fetching.

- [ ] 519. [tab_videos.cpp] MP4 download may lack User-Agent header
- **JS Source**: `src/js/modules/tab_videos.js` line 628
- **Status**: Pending
- **Details**: JS explicitly sets `'User-Agent': constants.USER_AGENT` for the MP4 download fetch. C++ uses `generics::get(*mp4_url)` which may or may not set User-Agent, depending on that function's implementation.

- [ ] 520. [tab_videos.cpp] Dead variable prev_selection_first never read
- **JS Source**: `src/js/modules/tab_videos.js` (none)
- **Status**: Pending
- **Details**: `prev_selection_first` is set on line 953 but never read for any comparison. The selection comparison uses `selected_file` instead. This is dead code.

- [ ] 521. [tab_videos.cpp] Dev-mode trigger_kino_processing not exposed in C++
- **JS Source**: `src/js/modules/tab_videos.js` lines 468–469
- **Status**: Pending
- **Details**: JS exposes `window.trigger_kino_processing = trigger_kino_processing` when `!BUILD_RELEASE`. C++ has only a comment. No equivalent debug hook exists.

- [ ] 522. [tab_text.cpp] Text preview failure toast omits JS `View Log` action callback
- **JS Source**: `src/js/modules/tab_text.js` lines 138–139
- **Status**: Pending
- **Details**: JS preview failure toast provides `{ 'View Log': () => log.openRuntimeLog() }`; C++ passes empty toast actions, removing the original recovery handler.

- [ ] 523. [tab_text.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_text.js` line 31
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 524. [tab_text.cpp] getFileByName vs getVirtualFileByName in preview and export
- **JS Source**: `src/js/modules/tab_text.js` lines 129, 108
- **Status**: Pending
- **Details**: JS calls casc.getFileByName(first) for preview and export. C++ calls getVirtualFileByName which is a different method with different behavior (extra DBD manifest logic, unknown/ path handling).

- [ ] 525. [tab_text.cpp] readString() encoding parameter missing
- **JS Source**: `src/js/modules/tab_text.js` line 130
- **Status**: Pending
- **Details**: JS calls file.readString(undefined, 'utf8') passing explicit utf8 encoding. C++ calls file.readString() with no encoding argument. May produce different output for non-ASCII content.

- [ ] 526. [tab_text.cpp] Missing 'View Log' button in error toast
- **JS Source**: `src/js/modules/tab_text.js` lines 138–139
- **Status**: Pending
- **Details**: JS passes { 'View Log': () => log.openRuntimeLog() } as toast action. C++ passes empty {}. User has no way to open runtime log from error toast.

- [ ] 527. [tab_text.cpp] Regex tooltip missing on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_text.js` line 31
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ renders text with no tooltip.

- [ ] 528. [tab_text.cpp] Filter input missing placeholder text
- **JS Source**: `src/js/modules/tab_text.js` line 32
- **Status**: Pending
- **Details**: JS has placeholder="Filter text files...". C++ uses ImGui::InputText with no hint text.

- [ ] 529. [tab_text.cpp] export_text is synchronous instead of async
- **JS Source**: `src/js/modules/tab_text.js` lines 77–121
- **Status**: Pending
- **Details**: JS export_text is async with await for generics.fileExists(), casc.getFileByName(), and data.writeToFile(). C++ runs entirely synchronously, freezing UI during multi-file export.

- [ ] 530. [tab_text.cpp] export_text error handler missing stack trace parameter
- **JS Source**: `src/js/modules/tab_text.js` line 116
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 531. [tab_text.cpp] Text preview child window padding differs from CSS
- **JS Source**: `src/js/modules/tab_text.js` line 36
- **Status**: Pending
- **Details**: CSS padding: 15px adds padding on all four sides. C++ sets ImGui::SetCursorPos(15, 15) for top-left only — no bottom/right padding. Content scrolls to exact end of text with no margin.

- [ ] 532. [tab_fonts.cpp] Font preview textarea is not rendered with the selected loaded font family
- **JS Source**: `src/js/modules/tab_fonts.js` lines 67, 159–163
- **Status**: Pending
- **Details**: JS binds preview textarea style `fontFamily` to the loaded font id; C++ updates `fontPreviewFontFamily` state but renders `InputTextMultiline` without switching ImGui font, so preview text does not reflect selected font family.

- [ ] 533. [tab_fonts.cpp] Loaded font cache contract differs from JS URL-based font-face lifecycle
- **JS Source**: `src/js/modules/tab_fonts.js` lines 10, 30–32
- **Status**: Pending
- **Details**: JS caches `font_id -> blob URL` from `inject_font_face`, preserving URL/font-face lifecycle; C++ caches `font_id -> void*` ImGui font pointer, changing resource model and API behavior.

- [ ] 534. [tab_fonts.cpp] Font preview textarea does not render in the loaded font
- **JS Source**: `src/js/modules/tab_fonts.js` line 67
- **Status**: Pending
- **Details**: JS applies fontFamily style to the preview textarea. C++ uses InputTextMultiline without pushing the loaded ImGui font. Preview renders in default font.

- [ ] 535. [tab_fonts.cpp] Missing data.processAllBlocks() call in load_font
- **JS Source**: `src/js/modules/tab_fonts.js` lines 28–29
- **Status**: Pending
- **Details**: JS explicitly calls data.processAllBlocks() to ensure all BLTE blocks are decompressed. C++ calls getVirtualFileByName() without explicit processAllBlocks(). Font data may be incomplete.

- [ ] 536. [tab_fonts.cpp] export_fonts missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_fonts.js` line 141
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

- [ ] 537. [tab_fonts.cpp] load_font and export_fonts are synchronous blocking main thread
- **JS Source**: `src/js/modules/tab_fonts.js` lines 16, 102
- **Status**: Pending
- **Details**: Both JS functions are async. C++ implementations are synchronous blocking the render thread.

- [ ] 538. [legacy_tab_fonts.cpp] Preview text is not rendered with the selected font family
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 78, 165–169
- **Status**: Pending
- **Details**: JS binds textarea `fontFamily` to `fontPreviewFontFamily`, but C++ renders `InputTextMultiline` without switching to the loaded `ImFont`, so font preview output does not use the selected legacy font.

- [ ] 539. [legacy_tab_fonts.cpp] Font loading contract differs from JS URL-based `loaded_fonts` cache
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 18–41
- **Status**: Pending
- **Details**: JS caches `font_id -> blob URL` and reuses CSS font-family identifiers; C++ caches `font_id -> void*` ImGui font pointers, changing the original module’s data model and font-resource lifecycle behavior.

- [ ] 540. [legacy_tab_fonts.cpp] Glyph cells rendered in default ImGui font, not the selected font family
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 88–91
- **Status**: Pending
- **Details**: JS glyph cells have `cell.style.fontFamily = '"${font_family}", monospace'` so each cell displays in the loaded font. C++ uses `ImGui::Selectable(utf8_buf, ...)` (line 263) which renders in the default ImGui font. Glyphs from the inspected font (e.g., decorative characters) will appear as the default UI font instead, making the glyph grid useless for visual font inspection.

- [ ] 541. [legacy_tab_fonts.cpp] Font preview placeholder shown as tooltip vs JS textarea placeholder
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 78
- **Status**: Pending
- **Details**: JS uses `<textarea :placeholder="$core.view.fontPreviewPlaceholder">` which shows ghost placeholder text inside the text area when empty. C++ uses `ImGui::SetItemTooltip(...)` (line 293) which only shows the text on mouse hover as a tooltip popup. The visual behavior differs — JS shows persistent in-field hint text; C++ shows nothing until hover.

- [ ] 542. [legacy_tab_fonts.cpp] Glyph cell size hardcoded 24x24 may not match JS CSS
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 76
- **Status**: Pending
- **Details**: C++ glyph cells use `ImVec2(24, 24)` (line 263) for the Selectable size. JS uses CSS class `font-glyph-cell` whose dimensions are defined in `app.css`. The CSS may define different sizing, padding, or font-size for glyph cells, causing a visual mismatch.

- [ ] 543. [tab_data.cpp] Data-table cell copy stringification differs from JS `String(value)` behavior
- **JS Source**: `src/js/modules/tab_data.js` lines 172–177
- **Status**: Pending
- **Details**: JS copies with `String(value)`, while C++ uses `value.dump()`; for string JSON values this includes JSON quoting/escaping, changing clipboard output.

- [ ] 544. [tab_data.cpp] DB2 load error toast omits JS `View Log` action callback
- **JS Source**: `src/js/modules/tab_data.js` lines 80–82
- **Status**: Pending
- **Details**: JS error toast includes `{'View Log': () => log.openRuntimeLog()}`; C++ error toast uses empty actions, removing the original recovery handler.

- [ ] 545. [tab_data.cpp] Listbox single parameter is true should be false (multi-select broken)
- **JS Source**: `src/js/modules/tab_data.js` lines 97–99
- **Status**: Pending
- **Details**: JS Listbox does not pass single, enabling multi-selection. C++ passes true restricting to single-entry mode. This breaks all multi-table export logic.

- [ ] 546. [tab_data.cpp] Listbox nocopy is false should be true
- **JS Source**: `src/js/modules/tab_data.js` line 99
- **Status**: Pending
- **Details**: JS passes nocopy true to disable Ctrl+C. C++ passes false leaving Ctrl+C enabled.

- [ ] 547. [tab_data.cpp] Listbox unittype is "table" should be "db2 file"
- **JS Source**: `src/js/modules/tab_data.js` line 99
- **Status**: Pending
- **Details**: JS uses unittype "db2 file". C++ passes "table". The file count display text differs.

- [ ] 548. [tab_data.cpp] Listbox pasteselection and copytrimwhitespace hardcoded false
- **JS Source**: `src/js/modules/tab_data.js` lines 98–99
- **Status**: Pending
- **Details**: JS binds pasteselection to config.pasteSelection and copytrimwhitespace to config.removePathSpacesCopy. C++ hardcodes both to false.

- [ ] 549. [tab_data.cpp] load_table error toast missing View Log action button
- **JS Source**: `src/js/modules/tab_data.js` line 80
- **Status**: Pending
- **Details**: JS includes View Log action. C++ passes empty map.

- [ ] 550. [tab_data.cpp] Context menu labels are static instead of dynamic row count
- **JS Source**: `src/js/modules/tab_data.js` lines 108–110
- **Status**: Pending
- **Details**: JS renders "Copy N rows as CSV" with actual selectedCount and pluralization. C++ uses static labels losing the count.

- [ ] 551. [tab_data.cpp] Context menu node not cleared on close
- **JS Source**: `src/js/modules/tab_data.js` line 107
- **Status**: Pending
- **Details**: JS resets nodeDataTable to null on close. C++ never resets it so the condition stays true permanently.

- [ ] 552. [tab_data.cpp] copy_cell uses value.dump() producing JSON-quoted strings
- **JS Source**: `src/js/modules/tab_data.js` lines 172–177
- **Status**: Pending
- **Details**: JS uses String(value) producing unquoted output. C++ uses value.dump() adding extra quotes for strings.

- [ ] 553. [tab_data.cpp] Selection watcher prevents retry after failed load
- **JS Source**: `src/js/modules/tab_data.js` lines 371–377
- **Status**: Pending
- **Details**: JS compares selected_file which is not updated on failure allowing retry. C++ compares prev_selection_last updated unconditionally preventing retry.

- [ ] 554. [tab_data.cpp] Missing Regex Enabled indicators in both filter bars
- **JS Source**: `src/js/modules/tab_data.js` lines 102–103, 129–130
- **Status**: Pending
- **Details**: JS shows Regex Enabled div in both the DB2 filter bar and data table tray filter. C++ has no regex indicators.

- [ ] 555. [tab_data.cpp] helper.mark() calls missing stack trace argument
- **JS Source**: `src/js/modules/tab_data.js` lines 250, 314, 358
- **Status**: Pending
- **Details**: JS passes both e.message and e.stack. C++ only passes e.what().

- [ ] 556. [legacy_tab_data.cpp] Export format menu omits JS SQL/DBC options
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 172–176, 222–231
- **Status**: Pending
- **Details**: JS menu exposes `CSV`, `SQL`, and `DBC` export actions, but C++ `legacy_data_opts` only includes `Export as CSV`, making SQL/DBC exports unavailable through the settings menu path.

- [ ] 557. [legacy_tab_data.cpp] `copy_cell` empty-string handling differs from JS
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 215–220
- **Status**: Pending
- **Details**: JS copies any non-null/undefined value (including empty string), while C++ returns early on `value.empty()`, so empty-cell clipboard behavior is not equivalent.

- [ ] 558. [legacy_tab_data.cpp] DBC filename extraction uses `std::filesystem::path` which won't split backslash-delimited MPQ paths on Linux
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 33–36
- **Status**: Pending
- **Details**: JS uses `full_path.split('\\')` to extract the DBC filename from backslash-delimited MPQ paths. C++ uses `std::filesystem::path(full_path).filename()` (line 81). On Linux, `std::filesystem::path` treats `\` as a regular character, not a separator, so `filename()` would return the entire path string instead of just the filename. This would cause the table name extraction to fail for MPQ paths like `DBFilesClient\Achievement.dbc`.

- [ ] 559. [legacy_tab_data.cpp] Listbox `unittype` is "table" vs JS "dbc file"
- **JS Source**: `src/js/modules/legacy_tab_data.js` line 131
- **Status**: Pending
- **Details**: JS Listbox uses `unittype="dbc file"` for the file count display. C++ uses `"table"` (line 293). The status bar will show "X tables" instead of "X dbc files", which is a user-facing text difference.

- [ ] 560. [legacy_tab_data.cpp] Listbox `nocopy` is `false` vs JS `:nocopy="true"`
- **JS Source**: `src/js/modules/legacy_tab_data.js` line 131
- **Status**: Pending
- **Details**: JS DBC listbox has `:nocopy="true"` which disables CTRL+C copy functionality. C++ passes `false` for nocopy (line 297), allowing copy. This is a behavioral difference — users can copy DBC table names in C++ but not in JS.

- [ ] 561. [legacy_tab_data.cpp] Missing regex info display in DBC filter bar
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 134–135
- **Status**: Pending
- **Details**: JS has `<div class="regex-info" v-if="$core.view.config.regexFilters">Regex Enabled</div>` in the DBC listbox filter section. C++ DBC filter bar (lines 309–316) does not show the regex enabled indicator.

- [ ] 562. [legacy_tab_data.cpp] Context menu uses `ImGui::BeginPopupContextItem` vs JS ContextMenu component
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 139–143
- **Status**: Pending
- **Details**: JS uses the custom `ContextMenu` component with slot-based content rendering and a close event. C++ uses native `ImGui::BeginPopupContextItem` (line 368) which has different popup behavior, positioning, and styling compared to the custom ContextMenu component used elsewhere in the app.

- [ ] 563. [tab_raw.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_raw.js` line 158
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 564. [tab_raw.cpp] export_raw_files uses getVirtualFileByName and drops partialDecrypt=true
- **JS Source**: `src/js/modules/tab_raw.js` line 123
- **Status**: Pending
- **Details**: JS calls core.view.casc.getFileByName(file_name, true) passing partialDecrypt=true. C++ calls getVirtualFileByName(file_name) without partialDecrypt parameter, silently dropping partial decryption capability for encrypted files.

- [ ] 565. [tab_raw.cpp] export_raw_files error mark missing stack trace argument
- **JS Source**: `src/js/modules/tab_raw.js` line 128
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 566. [tab_raw.cpp] parent_path() returns "" not "." for bare filenames
- **JS Source**: `src/js/modules/tab_raw.js` lines 113–115
- **Status**: Pending
- **Details**: JS path.dirname returns "." for bare filenames, then checks dir === ".". C++ parent_path() returns "" for bare filenames, then checks dir == "." which never matches. Functionally similar result but fragile — should check dir.empty() || dir == ".".

- [ ] 567. [tab_raw.cpp] Missing placeholder text on filter input
- **JS Source**: `src/js/modules/tab_raw.js` line 159
- **Status**: Pending
- **Details**: JS has placeholder="Filter raw files...". C++ uses ImGui::InputText with no hint text.

- [ ] 568. [tab_raw.cpp] Missing tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_raw.js` line 158
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ has no tooltip.

- [ ] 569. [tab_raw.cpp] All async functions converted to synchronous — blocks UI thread
- **JS Source**: `src/js/modules/tab_raw.js` lines 12, 31, 91
- **Status**: Pending
- **Details**: JS compute_raw_files, detect_raw_files, and export_raw_files are all async. C++ versions are synchronous, blocking render thread during CASC I/O and disk operations.

- [ ] 570. [tab_raw.cpp] detect_raw_files manually sets is_dirty=true — deviates from JS
- **JS Source**: `src/js/modules/tab_raw.js` lines 75–76
- **Status**: Pending
- **Details**: JS calls listfile.ingestIdentifiedFiles then compute_raw_files without setting is_dirty. Since is_dirty was false, JS would return early (apparent JS bug). C++ adds is_dirty=true to fix this, which is arguably correct but deviates from original JS behavior.

- [ ] 571. [legacy_tab_files.cpp] Listbox context menu includes extra FileDataID actions absent in JS
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 76–80
- **Status**: Pending
- **Details**: JS legacy-files menu only provides copy file path, copy export path, and open export directory; C++ conditionally adds listfile-format and fileDataID entries, changing context-menu behavior.

- [ ] 572. [legacy_tab_files.cpp] Layout doesn't use `app::layout` helpers — uses raw `ImGui::BeginChild`
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 72–89
- **Status**: Pending
- **Details**: Other legacy tabs (audio, fonts, textures, data) use `app::layout::BeginTab/EndTab`, `CalcListTabRegions`, `BeginListContainer`, etc. for consistent layout. `legacy_tab_files.cpp` uses raw `ImGui::BeginChild("legacy-files-list-container", ...)` (line 124) without the layout system. This will produce inconsistent sizing and positioning compared to sibling legacy tabs.

- [ ] 573. [legacy_tab_files.cpp] Filter input missing placeholder text "Filter files..."
- **JS Source**: `src/js/modules/legacy_tab_files.js` line 85
- **Status**: Pending
- **Details**: JS filter input has `placeholder="Filter files..."`. C++ `ImGui::InputText("##FilterFiles", ...)` (line 207) has no placeholder/hint text.

- [ ] 574. [legacy_tab_files.cpp] Tray layout structure differs from JS
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 82–88
- **Status**: Pending
- **Details**: JS wraps the filter and export button in a `#tab-legacy-files-tray` div with its own layout (likely flex row). C++ renders filter input, then `ImGui::SameLine()`, then the export button (lines 206–216). The proportions and alignment of filter vs button may not match the JS CSS-defined tray layout.

- [ ] 575. [tab_maps.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_maps.js` line 302
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 576. [tab_maps.cpp] Hand-rolled MD5 instead of mbedTLS
- **JS Source**: `src/js/modules/tab_maps.js` line 914
- **Status**: Pending
- **Details**: C++ implements a full RFC 1321 MD5 from scratch instead of using mbedTLS MD API (mbedtls/md.h) as specified in project conventions.

- [ ] 577. [tab_maps.cpp] load_map_tile uses nearest-neighbor scaling instead of bilinear interpolation
- **JS Source**: `src/js/modules/tab_maps.js` lines 62–71
- **Status**: Pending
- **Details**: JS Canvas 2D drawImage performs bilinear interpolation when scaling. C++ uses nearest-neighbor sampling (integer coordinate snapping), making scaled minimap tiles look blockier/pixelated.

- [ ] 578. [tab_maps.cpp] load_map_tile uses blp.width instead of blp.scaledWidth
- **JS Source**: `src/js/modules/tab_maps.js` line 62
- **Status**: Pending
- **Details**: JS computes scale as size / blp.scaledWidth. C++ uses blp.width. If the BLP has a scaledWidth differing from raw width (e.g. mipmaps), the scaling factor will be wrong.

- [ ] 579. [tab_maps.cpp] load_wmo_minimap_tile ignores drawX/drawY and scaleX/scaleY positioning
- **JS Source**: `src/js/modules/tab_maps.js` lines 107–112
- **Status**: Pending
- **Details**: JS draws each tile at its specific offset (tile.drawX * output_scale, tile.drawY * output_scale) with scaled dimensions. C++ ignores drawX, drawY, scaleX, scaleY entirely — stretching all tiles to fill the full cell. Multi-tile compositing within a grid cell is completely broken.

- [ ] 580. [tab_maps.cpp] export_map_wmo_minimap uses max-alpha instead of source-over compositing
- **JS Source**: `src/js/modules/tab_maps.js` lines 721–733
- **Status**: Pending
- **Details**: JS Canvas 2D drawImage uses Porter-Duff source-over compositing. C++ export computes alpha as max(dst_alpha, src_alpha) instead of correct source-over formula.

- [ ] 581. [tab_maps.cpp] WDT file load is outside try-catch block
- **JS Source**: `src/js/modules/tab_maps.js` lines 433–434
- **Status**: Pending
- **Details**: In JS, getFileByName(wdt_path) is inside the try block. In C++, getVirtualFileByName(wdt_path) is BEFORE the try block. If WDT file doesn't exist, exception propagates uncaught.

- [ ] 582. [tab_maps.cpp] mapViewerHasWorldModel check differs from JS
- **JS Source**: `src/js/modules/tab_maps.js` lines 438–439
- **Status**: Pending
- **Details**: JS checks if (wdt.worldModelPlacement) — any non-null object is truthy. C++ checks if (worldModelPlacement.id != 0), which misses placement with id=0. Also affects has_global_wmo and export_map_wmo checks.

- [ ] 583. [tab_maps.cpp] Missing e.stack in all helper.mark error calls
- **JS Source**: `src/js/modules/tab_maps.js` lines 680, 759, 815, 848, 931, 1122
- **Status**: Pending
- **Details**: JS passes both e.message and e.stack to helper.mark. C++ only passes e.what(), omitting stack trace. Affects 6 export functions.

- [ ] 584. [tab_maps.cpp] All async functions converted to synchronous — UI will block
- **JS Source**: `src/js/modules/tab_maps.js` lines 49–980
- **Status**: Pending
- **Details**: Every async function (load_map_tile, load_wmo_minimap_tile, collect_game_objects, extract_height_data_from_tile, load_map, setup_wmo_minimap, all export functions, initialize) is synchronous C++. Long exports freeze the UI.

- [ ] 585. [tab_maps.cpp] Missing optional chaining for export_paths
- **JS Source**: `src/js/modules/tab_maps.js` lines 752–853
- **Status**: Pending
- **Details**: JS uses optional chaining export_paths?.writeLine() and export_paths?.close(). C++ calls directly without null checks. If openLastExportStream returns invalid object, C++ will crash.

- [ ] 586. [tab_maps.cpp] Missing "Filter maps..." placeholder text
- **JS Source**: `src/js/modules/tab_maps.js` line 303
- **Status**: Pending
- **Details**: JS filter input has placeholder="Filter maps...". C++ uses ImGui::InputText without placeholder text.

- [ ] 587. [tab_maps.cpp] Missing regex tooltip
- **JS Source**: `src/js/modules/tab_maps.js` line 302
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" on regex-info div. C++ renders ImGui::TextDisabled("Regex Enabled") without any tooltip.

- [ ] 588. [tab_maps.cpp] Sidebar headers use SeparatorText instead of styled span
- **JS Source**: `src/js/modules/tab_maps.js` lines 313, 342, 352, 354
- **Status**: Pending
- **Details**: JS uses <span class="header"> which renders as bold text. C++ uses ImGui::SeparatorText() which draws a horizontal separator line with text — visually different.

- [ ] 589. [tab_maps.cpp] collect_game_objects returns vector instead of Set
- **JS Source**: `src/js/modules/tab_maps.js` lines 146–157
- **Status**: Pending
- **Details**: JS returns a Set guaranteeing uniqueness. C++ returns std::vector<ADTGameObject> which can contain duplicates.

- [ ] 590. [tab_maps.cpp] Selection watch may miss intermediate changes
- **JS Source**: `src/js/modules/tab_maps.js` lines 1135–1143
- **Status**: Pending
- **Details**: JS Vue $watch triggers on any reactive change. C++ only compares the first element string between frames. If selection changes and reverts within same frame, or changes to different item with same first entry, C++ misses it.

- [ ] 591. [tab_zones.cpp] Default phase filtering excludes non-zero phases unlike JS
- **JS Source**: `src/js/modules/tab_zones.js` lines 78–79
- **Status**: Pending
- **Details**: JS includes all `UiMapXMapArt` links when `phase_id === null`; C++ filters to `PhaseID == 0` when no phase is selected.

- [ ] 592. [tab_zones.cpp] UiMapArtStyleLayer lookup key differs from JS relation logic
- **JS Source**: `src/js/modules/tab_zones.js` lines 88–90
- **Status**: Pending
- **Details**: JS resolves style layers by matching `UiMapArtStyleID` to `art_entry.UiMapArtStyleID`; C++ matches `UiMapArtID` to the linked art ID, changing style-layer association behavior.

- [ ] 593. [tab_zones.cpp] Base tile relation lookup uses layer-row ID instead of UiMapArt ID
- **JS Source**: `src/js/modules/tab_zones.js` lines 120–121
- **Status**: Pending
- **Details**: JS fetches `UiMapArtTile` relation rows with `art_style.ID` from the UiMapArt entry; C++ stores `CombinedArtStyle::id` as the UiMapArtStyleLayer row ID and uses that in `getRelationRows`, altering tile resolution.

- [ ] 594. [tab_zones.cpp] Base map tile OffsetX/OffsetY offsets are ignored
- **JS Source**: `src/js/modules/tab_zones.js` lines 181–182
- **Status**: Pending
- **Details**: JS applies `tile.OffsetX`/`tile.OffsetY` when placing map tiles; C++ calculates tile position from row/column and tile dimensions only.

- [ ] 595. [tab_zones.cpp] Zone listbox copy/paste trim options are hardcoded instead of config-bound
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: JS binds `copymode`, `pasteselection`, and `copytrimwhitespace` to config values; C++ hardcodes `CopyMode::Default`, `pasteselection=false`, and `copytrimwhitespace=false`.

- [ ] 596. [tab_zones.cpp] Phase selector placement differs from JS preview overlay layout
- **JS Source**: `src/js/modules/tab_zones.js` lines 341–349
- **Status**: Pending
- **Details**: JS renders the phase dropdown in a `preview-dropdown-overlay` inside the preview background; C++ renders phase selection in the bottom control bar.

- [ ] 597. [tab_zones.cpp] UiMapArtStyleLayer join uses wrong field name
- **JS Source**: `src/js/modules/tab_zones.js` lines 88–91
- **Status**: Pending
- **Details**: JS joins `art_style_layer.UiMapArtStyleID === art_entry.UiMapArtStyleID`. C++ joins on `layer_row["UiMapArtID"]` — a completely different field name. The C++ looks for "UiMapArtID" in UiMapArtStyleLayer table, but JS matches on "UiMapArtStyleID" from both tables. This produces wrong rows or no rows.

- [ ] 598. [tab_zones.cpp] CombinedArtStyle.id stores wrong ID (layer ID vs art ID)
- **JS Source**: `src/js/modules/tab_zones.js` lines 94–101
- **Status**: Pending
- **Details**: JS `combined_style` includes `...art_entry` (spread), so `combined_style.ID` = the UiMapArt row ID (`art_id`). C++ sets `style.id = static_cast<int>(layer_id)` which is the UiMapArtStyleLayer table key. This wrong ID propagates to `getRelationRows()` calls for UiMapArtTile and WorldMapOverlay.

- [ ] 599. [tab_zones.cpp] C++ adds ALL matching style layers; JS keeps only LAST
- **JS Source**: `src/js/modules/tab_zones.js` lines 86–91
- **Status**: Pending
- **Details**: JS declares `let style_layer;` then overwrites in a loop, keeping only the last match. C++ `push_back`s every matching row into `art_styles`. This creates duplicate/extra entries causing redundant or incorrect rendering.

- [ ] 600. [tab_zones.cpp] Phase filter logic differs when phase_id is null
- **JS Source**: `src/js/modules/tab_zones.js` line 78
- **Status**: Pending
- **Details**: JS: `if (phase_id === null || link_entry.PhaseID === phase_id)` — when phase_id is null, ALL entries are included. C++: when `phase_id` is nullopt, only entries with `row_phase == 0` are included. C++ omits non-default phases when no phase is specified, while JS shows all.

- [ ] 601. [tab_zones.cpp] Missing tile OffsetX/OffsetY in render_map_tiles
- **JS Source**: `src/js/modules/tab_zones.js` lines 181–182
- **Status**: Pending
- **Details**: JS: `final_x = pixel_x + (tile.OffsetX || 0); final_y = pixel_y + (tile.OffsetY || 0)`. C++ only uses `pixel_x = col * tile_width; pixel_y = row * tile_height` with no offset. Tiles with non-zero offsets will be mispositioned.

- [ ] 602. [tab_zones.cpp] Tile layer rendering architecture differs from JS
- **JS Source**: `src/js/modules/tab_zones.js` lines 126–152
- **Status**: Pending
- **Details**: JS groups ALL tiles for an art_style by their LayerIndex, then renders each group in sorted order. C++ calls `render_map_tiles(art_style, art_style.layer_index, ...)` which filters tiles to only those matching the single layer_index. Combined with the duplicate style layers issue, rendering pipeline differs significantly.

- [ ] 603. [tab_zones.cpp] parse_zone_entry doesn't throw on bad input
- **JS Source**: `src/js/modules/tab_zones.js` lines 17–18
- **Status**: Pending
- **Details**: JS throws `new Error('unexpected zone entry')` on regex mismatch. C++ returns an empty `ZoneDisplayInfo{}` with `id=0`. Callers add `zone.id > 0` guards, but error propagation differs.

- [ ] 604. [tab_zones.cpp] UiMap row existence not validated
- **JS Source**: `src/js/modules/tab_zones.js` lines 67–71
- **Status**: Pending
- **Details**: JS checks `if (!map_data)` and throws `'UiMap entry not found'`. C++ fetches the row but casts to void: `(void)ui_map_row_opt;` — never checks the result or throws.

- [ ] 605. [tab_zones.cpp] Pixel buffer not cleared at start of render when first layer is non-zero
- **JS Source**: `src/js/modules/tab_zones.js` line 59
- **Status**: Pending
- **Details**: JS calls `ctx.clearRect(0, 0, canvas.width, canvas.height)` at the start. C++ only allocates/clears the pixel buffer inside the `if (art_style.layer_index == 0)` block. If the first art_style has layer_index != 0, stale pixel data remains.

- [ ] 606. [tab_zones.cpp] Listbox copyMode hardcoded instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ passes `listbox::CopyMode::Default` instead of reading from `view.config["copyMode"]`.

- [ ] 607. [tab_zones.cpp] Listbox pasteSelection hardcoded false instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ hardcodes `false` instead of reading `view.config["pasteSelection"]`.

- [ ] 608. [tab_zones.cpp] Listbox copytrimwhitespace hardcoded false instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ hardcodes `false` instead of reading `view.config["removePathSpacesCopy"]`.

- [ ] 609. [tab_zones.cpp] export_zone_map helper.mark missing stack trace
- **JS Source**: `src/js/modules/tab_zones.js` line 491
- **Status**: Pending
- **Details**: JS: `helper.mark(zone_entry, false, e.message, e.stack)` — passes both message and stack. C++: `helper.mark(..., false, e.what())` — only passes message, no stack trace.

- [ ] 610. [tab_zones.cpp] Phase dropdown placed in control bar instead of preview overlay
- **JS Source**: `src/js/modules/tab_zones.js` lines 341–347
- **Status**: Pending
- **Details**: JS puts the phase `<select>` inside `preview-dropdown-overlay` div overlaid on the zone canvas. C++ places the `ImGui::BeginCombo` in the bottom controls bar alongside checkboxes/button. This is a layout difference.

- [ ] 611. [tab_zones.cpp] Missing regex tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_zones.js` line 325
- **Status**: Pending
- **Details**: JS shows a tooltip on "Regex Enabled" via `:title="$core.view.regexTooltip"`. C++ renders `ImGui::TextUnformatted("Regex Enabled")` with no tooltip.

- [ ] 612. [tab_zones.cpp] EXPANSION_NAMES static vector is dead code
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: `EXPANSION_NAMES` vector is defined but never referenced. The actual expansion rendering uses `constants::EXPANSIONS`. Should be removed.

- [ ] 613. [tab_zones.cpp] ZoneDisplayInfo vs ZoneEntry naming mismatch with header
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: The header declares `ZoneEntry` struct but the cpp defines a separate `ZoneDisplayInfo` struct for `parse_zone_entry`. The header's `ZoneEntry` appears unused.

- [ ] 614. [tab_zones.cpp] Missing per-tile position logging in render_map_tiles
- **JS Source**: `src/js/modules/tab_zones.js` lines 184–185
- **Status**: Pending
- **Details**: JS logs `'rendering tile FileDataID %d at position (%d,%d) -> (%d,%d) [Layer %d]'` for each tile. C++ has no per-tile log.

- [ ] 615. [tab_zones.cpp] Missing "no tiles found" log for art style
- **JS Source**: `src/js/modules/tab_zones.js` lines 121–123
- **Status**: Pending
- **Details**: JS logs `'no tiles found for UiMapArt ID %d'` and `continue`s. C++ has no equivalent check/log.

- [ ] 616. [tab_zones.cpp] Missing "no overlays found" log
- **JS Source**: `src/js/modules/tab_zones.js` lines 212–214
- **Status**: Pending
- **Details**: JS logs `'no WorldMapOverlay entries found for UiMapArt ID %d'` when overlays array is empty. C++ has no such log.

- [ ] 617. [tab_zones.cpp] Missing "no overlay tiles" log per overlay
- **JS Source**: `src/js/modules/tab_zones.js` lines 219–222
- **Status**: Pending
- **Details**: JS logs `'no tiles found for WorldMapOverlay ID %d'` and `continue`s for empty tile sets. C++ calls `render_overlay_tiles` regardless.

- [ ] 618. [tab_zones.cpp] Unsafe Windows wstring conversion corrupts multi-byte UTF-8 paths
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: `std::wstring wpath(dir.begin(), dir.end())` does byte-by-byte copy which corrupts multi-byte UTF-8 paths. Should use `MultiByteToWideChar` or equivalent.

- [ ] 619. [tab_zones.cpp] Linux shell command injection risk in openInExplorer
- **JS Source**: `src/js/modules/tab_zones.js` line 393
- **Status**: Pending
- **Details**: `"xdg-open \"" + dir + "\" &"` passed to `std::system()`. If `dir` contains shell metacharacters, this is exploitable. JS uses `nw.Shell.openItem` which is safe.

- [ ] 620. [tab_items.cpp] Wowhead item handler is stubbed out
- **JS Source**: `src/js/modules/tab_items.js` lines 322–324
- **Status**: Pending
- **Details**: JS calls `ExternalLinks.wowHead_viewItem(item_id)` from the context action; C++ `view_on_wowhead(...)` immediately returns and does nothing.

- [ ] 621. [tab_items.cpp] Item sidebar checklist interaction/layout diverges from JS clickable row design
- **JS Source**: `src/js/modules/tab_items.js` lines 254–266
- **Status**: Pending
- **Details**: JS uses `.sidebar-checklist-item` rows with selected-state styling and row-level click toggling; C++ renders plain ImGui checkboxes, changing sidebar visuals and interaction feel.

- [ ] 622. [tab_items.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_items.js` line 248
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 623. [tab_items.cpp] view_on_wowhead is stubbed — does nothing
- **JS Source**: `src/js/modules/tab_items.js` lines 322–324
- **Status**: Pending
- **Details**: JS calls ExternalLinks.wowHead_viewItem(item_id) which opens a Wowhead URL. C++ function is { return; } — a no-op. external_links.h already provides wowHead_viewItem().

- [ ] 624. [tab_items.cpp] copy_to_clipboard bypasses core.view.copyToClipboard
- **JS Source**: `src/js/modules/tab_items.js` lines 318–320
- **Status**: Pending
- **Details**: JS calls this.$core.view.copyToClipboard(value) which may have additional behavior (e.g. toast notification). C++ calls ImGui::SetClipboardText() directly, skipping view layer.

- [ ] 625. [tab_items.cpp] std::set ordering differs from JS Set insertion order
- **JS Source**: `src/js/modules/tab_items.js` lines 85–127
- **Status**: Pending
- **Details**: view_item_models and view_item_textures use JS Set which preserves insertion order. C++ uses std::set<std::string> which sorts lexicographically. Should use std::vector with uniqueness check.

- [ ] 626. [tab_items.cpp] Second loop (itemViewerShowAll) retrieves item name from wrong source
- **JS Source**: `src/js/modules/tab_items.js` lines 181–211
- **Status**: Pending
- **Details**: JS constructs new Item(item_id, item_row, null, null, null) where Item constructor reads item_row.Display_lang. C++ looks up name via DBItems::getItemById(item_id), a different data source.

- [ ] 627. [tab_items.cpp] Regex tooltip not rendered
- **JS Source**: `src/js/modules/tab_items.js` line 248
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" on "Regex Enabled" div. C++ renders ImGui::TextUnformatted("Regex Enabled") without any tooltip.

- [ ] 628. [tab_items.cpp] Sidebar headers use SeparatorText instead of styled header span
- **JS Source**: `src/js/modules/tab_items.js` lines 252, 262
- **Status**: Pending
- **Details**: JS uses <span class="header">Item Types</span> which renders as styled header text. C++ uses ImGui::SeparatorText() which draws a horizontal separator line with text — visually different.

- [ ] 629. [tab_items.cpp] Sidebar checklist items lack .selected class visual feedback
- **JS Source**: `src/js/modules/tab_items.js` lines 254–257
- **Status**: Pending
- **Details**: JS adds :class="{ selected: item.checked }" to checklist items. CSS gives .sidebar-checklist-item.selected a background of rgba(255,255,255,0.05). C++ uses plain ImGui::Checkbox with no highlight.

- [ ] 630. [tab_items.cpp] All async operations converted to synchronous — UI may block
- **JS Source**: `src/js/modules/tab_items.js` lines 104, 129, 277, 345–346
- **Status**: Pending
- **Details**: JS initialize_items, view_item_textures, and the mounted initialize flow are all async with await. C++ converts all to synchronous blocking calls, freezing the ImGui render loop.

- [ ] 631. [tab_items.cpp] Quality color applied only to CheckMark, not to label text
- **JS Source**: `src/js/modules/tab_items.js` lines 264–265
- **Status**: Pending
- **Details**: CSS accent-color applies quality color to the checkbox. C++ pushes ImGuiCol_CheckMark only coloring the checkmark glyph. The checkbox background/frame and label text are unaffected.

- [ ] 632. [tab_items.cpp] Filter input buffer limited to 256 bytes
- **JS Source**: `src/js/modules/tab_items.js` line 249
- **Status**: Pending
- **Details**: JS input has no character limit. C++ uses char filter_buf[256] with std::strncpy, silently truncating beyond 255 characters.

- [ ] 633. [tab_item_sets.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_item_sets.js` line 82
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ shows plain `Regex Enabled` text without tooltip behavior.

- [ ] 634. [tab_item_sets.cpp] Missing filter input placeholder text
- **JS Source**: `src/js/modules/tab_item_sets.js` line 83
- **Status**: Pending
- **Details**: JS uses placeholder="Filter item sets..." on the text input. C++ uses ImGui::InputText without hint/placeholder text.

- [ ] 635. [tab_item_sets.cpp] Missing regex tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_item_sets.js` line 82
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ just renders ImGui::TextUnformatted("Regex Enabled") with no tooltip.

- [ ] 636. [tab_item_sets.cpp] Async initialization converted to synchronous blocking calls
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 23–65
- **Status**: Pending
- **Details**: JS initialize_item_sets is async with await for progressLoadingScreen(), DBItems.ensureInitialized(), db2 getAllRows(). C++ calls all synchronously, blocking the UI thread and preventing loading screen updates.

- [ ] 637. [tab_item_sets.cpp] apply_filter converts ItemSet structs to JSON objects unnecessarily
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 67–69
- **Status**: Pending
- **Details**: JS simply assigns the array of ItemSet objects directly. C++ iterates every ItemSet, constructs nlohmann::json objects, and pushes them. render() then converts JSON back into ItemEntry structs every frame — double-conversion overhead.

- [ ] 638. [tab_item_sets.cpp] render() re-creates item_entries vector from JSON every frame
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 76–86
- **Status**: Pending
- **Details**: C++ render allocates a vector, loops over all JSON items, copies fields into ItemEntry structs, and pushes — every frame. JS template binds directly to existing objects with no per-frame allocation.

- [ ] 639. [tab_item_sets.cpp] Regex-enabled text and filter input lack proper layout container
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 81–84
- **Status**: Pending
- **Details**: JS wraps regex info and filter input inside div class="filter" providing inline layout. C++ renders them sequentially without SameLine() or horizontal group, causing "Regex Enabled" to appear above the filter input instead of beside it.

- [ ] 640. [tab_item_sets.cpp] fieldToUint32Vec does not handle single-value fields
- **JS Source**: `src/js/modules/tab_item_sets.js` line 38
- **Status**: Pending
- **Details**: JS set_row.ItemID is expected to be an array with .filter(id => id !== 0). C++ fieldToUint32Vec only handles vector variants. If a DB2 reader returns a single scalar, the function returns an empty vector, silently dropping data.

- [ ] 641. [tab_characters.cpp] Saved-character thumbnail card rendering is replaced by a placeholder button path
- **JS Source**: `src/js/modules/tab_characters.js` lines 1928–1934
- **Status**: Pending
- **Details**: JS renders thumbnail backgrounds and overlay action buttons in `.saved-character-card`; C++ renders a generic button with a `// thumbnail placeholder` path, so card visuals and thumbnail behavior diverge.

- [ ] 642. [tab_characters.cpp] Main-screen quick-save flow skips JS thumbnail capture step
- **JS Source**: `src/js/modules/tab_characters.js` lines 1973, 2328–2333
- **Status**: Pending
- **Details**: JS quick-save button routes through `open_save_prompt()` which captures `chrPendingThumbnail` before prompting; C++ main `Save` button only toggles prompt state and does not capture a fresh thumbnail first.

- [ ] 643. [tab_characters.cpp] Outside-click handlers for import/color popups from JS mounted lifecycle are missing
- **JS Source**: `src/js/modules/tab_characters.js` lines 2668–2685
- **Status**: Pending
- **Details**: JS registers a document click listener to close color pickers and floating import panels when clicking elsewhere; C++ has no equivalent mounted/unmounted document-listener path, changing panel-dismiss behavior.

- [ ] 644. [tab_characters.cpp] import_wmv_character() is completely stubbed
- **JS Source**: `src/js/modules/tab_characters.js` lines 988–1021
- **Status**: Pending
- **Details**: JS creates a file input, reads a .chr file, passes it to wmv_parse(), and calls apply_import_data(). C++ just does return. Also missing wmv_parse import and lastWMVImportPath config persistence.

- [ ] 645. [tab_characters.cpp] import_wowhead_character() is completely stubbed
- **JS Source**: `src/js/modules/tab_characters.js` lines 1023–1044
- **Status**: Pending
- **Details**: JS validates URL contains dressing-room, calls wowhead_parse(), then apply_import_data(). C++ just does return. Also missing wowhead_parse import.

- [ ] 646. [tab_characters.cpp] Missing texture application on attachment equipment models
- **JS Source**: `src/js/modules/tab_characters.js` lines 620–622
- **Status**: Pending
- **Details**: JS applies replaceable textures to each attachment model via applyReplaceableTextures(). C++ loads the attachment model but never calls applyReplaceableTextures().

- [ ] 647. [tab_characters.cpp] Missing texture application on collection equipment models
- **JS Source**: `src/js/modules/tab_characters.js` lines 664–668
- **Status**: Pending
- **Details**: JS selects correct texture index and applies it to collection models. C++ loads the collection model but never applies textures.

- [ ] 648. [tab_characters.cpp] Missing geoset visibility for collection models
- **JS Source**: `src/js/modules/tab_characters.js` lines 652–661
- **Status**: Pending
- **Details**: JS calls renderer.hideAllGeosets() then renderer.setGeosetGroupDisplay() using display.attachmentGeosetGroup and SLOT_TO_GEOSET_GROUPS mapping. C++ has the mapping defined but never uses it.

- [ ] 649. [tab_characters.cpp] OBJ/STL export missing chr_materials URI textures geoset mask and pose application
- **JS Source**: `src/js/modules/tab_characters.js` lines 1712–1722
- **Status**: Pending
- **Details**: JS iterates chr_materials for URI textures, sets geoset mask, and applies posed geometry. C++ does none of these for OBJ/STL export.

- [ ] 650. [tab_characters.cpp] OBJ/STL/GLTF export missing CharacterExporter equipment models
- **JS Source**: `src/js/modules/tab_characters.js` lines 1725–1811
- **Status**: Pending
- **Details**: JS creates CharacterExporter, collects equipment geometry with textures/bones, and passes to exporter. C++ skips equipment model export entirely for all formats.

- [ ] 651. [tab_characters.cpp] load_character_model always sets animation to none instead of auto-selecting stand
- **JS Source**: `src/js/modules/tab_characters.js` lines 744–745
- **Status**: Pending
- **Details**: JS finds stand animation (id 0.0) and auto-selects it. C++ always sets none so the character has no animation after load.

- [ ] 652. [tab_characters.cpp] load_character_model missing on_model_rotate callback
- **JS Source**: `src/js/modules/tab_characters.js` lines 719–722
- **Status**: Pending
- **Details**: JS calls controls.on_model_rotate after loading. C++ does not invoke any rotation callback after model load.

- [ ] 653. [tab_characters.cpp] import_character does not lowercase character name in URL
- **JS Source**: `src/js/modules/tab_characters.js` line 965
- **Status**: Pending
- **Details**: JS uses encodeURIComponent(character_name.toLowerCase()). C++ uses url_encode(character_name) without lowercasing. Battle.net API may be case-sensitive.

- [ ] 654. [tab_characters.cpp] import_character error handling uses string search instead of HTTP status
- **JS Source**: `src/js/modules/tab_characters.js` lines 969–983
- **Status**: Pending
- **Details**: JS separately handles HTTP 404 vs other errors. C++ catches all exceptions and does err_msg.find(404) string search which may not match actual HTTP 404 responses.

- [ ] 655. [tab_characters.cpp] import_json_character save-to-my-characters preserves guild_tabard (JS does not)
- **JS Source**: `src/js/modules/tab_characters.js` lines 1545–1553
- **Status**: Pending
- **Details**: JS save_data only includes race_id, model_id, choices, equipment. C++ also copies guild_tabard if present. Behavioral deviation from JS.

- [ ] 656. [tab_characters.cpp] Missing getEquipmentRenderers and getCollectionRenderers callbacks on viewer context
- **JS Source**: `src/js/modules/tab_characters.js` lines 2608–2609
- **Status**: Pending
- **Details**: JS context includes getEquipmentRenderers and getCollectionRenderers callbacks. C++ only sets getActiveRenderer. Equipment models may not render in the viewport.

- [ ] 657. [tab_characters.cpp] Equipment slot items missing quality color styling
- **JS Source**: `src/js/modules/tab_characters.js` line 2192
- **Status**: Pending
- **Details**: JS applies item-quality-X CSS class based on item quality. C++ renders equipment item names as plain text without quality-based coloring.

- [ ] 658. [tab_characters.cpp] navigate_to_items_for_slot missing type mask filtering
- **JS Source**: `src/js/modules/tab_characters.js` lines 2400–2414
- **Status**: Pending
- **Details**: JS sets itemViewerTypeMask to filter items tab by slot name. C++ just calls tab_items::setActive() with no filtering.

- [ ] 659. [tab_characters.cpp] Saved characters grid missing thumbnail rendering
- **JS Source**: `src/js/modules/tab_characters.js` lines 1928–1935
- **Status**: Pending
- **Details**: JS renders actual thumbnail images via backgroundImage CSS. C++ uses ImGui::Button with no image rendering.

- [ ] 660. [tab_characters.cpp] Texture preview panel is placeholder text
- **JS Source**: `src/js/modules/tab_characters.js` lines 2121–2123
- **Status**: Pending
- **Details**: JS has a DOM node where charTextureOverlay attaches canvas layers. C++ has ImGui::Text placeholder.

- [ ] 661. [tab_characters.cpp] Color picker uses ImGui Tooltip instead of positioned popup
- **JS Source**: `src/js/modules/tab_characters.js` lines 2051–2068
- **Status**: Pending
- **Details**: JS positions the color picker popup at event.clientX/Y with absolute CSS positioning. C++ uses ImGui::BeginTooltip() which follows the mouse cursor.

- [ ] 662. [tab_characters.cpp] Missing document click handler for dismissing panels
- **JS Source**: `src/js/modules/tab_characters.js` lines 2668–2685
- **Status**: Pending
- **Details**: JS adds a click listener that closes color pickers and import panels when clicking outside. C++ does not explicitly handle clicking outside these panels.

- [ ] 663. [tab_characters.cpp] Missing unmounted() cleanup
- **JS Source**: `src/js/modules/tab_characters.js` lines 2699–2701
- **Status**: Pending
- **Details**: JS has unmounted() calling reset_module_state() for cleanup when tab is destroyed. C++ does not export an unmounted function.

- [ ] 664. [tab_creatures.cpp] Creature list context actions are not equivalent to JS copy-name/copy-ID menu
- **JS Source**: `src/js/modules/tab_creatures.js` lines 983–986, 1179–1203
- **Status**: Pending
- **Details**: JS creature list context menu exposes only `Copy name(s)` and `Copy ID(s)` handlers; C++ delegates to generic `listbox_context::handle_context_menu(...)`, changing the context action contract from the original creature-specific menu.

- [ ] 665. [tab_creatures.cpp] has_content check and toast/camera logic scoped incorrectly
- **JS Source**: `src/js/modules/tab_creatures.js` lines 713–722
- **Status**: Pending
- **Details**: In JS, the has_content check, hideToast, and fitCamera are outside the if/else running for both character and standard models. In C++ this block is inside the else (standard-model only). For character-model creatures, the loading toast is never dismissed.

- [ ] 666. [tab_creatures.cpp] Collection model geoset logic has three bugs
- **JS Source**: `src/js/modules/tab_creatures.js` lines 421–429
- **Status**: Pending
- **Details**: (1) JS calls hideAllGeosets() before applying - C++ never does. (2) JS uses mapping.group_index for lookup - C++ uses coll_idx. (3) JS uses mapping.char_geoset for setGeosetGroupDisplay - C++ uses mapping.group_index.

- [ ] 667. [tab_creatures.cpp] Scrubber IsItemActivated() called before SliderInt checks wrong widget
- **JS Source**: `src/js/modules/tab_creatures.js` lines 1035–1038
- **Status**: Pending
- **Details**: C++ calls IsItemActivated() before SliderInt() renders. IsItemActivated checks the last widget (Step-Right button) not the slider. start_scrub() will never fire correctly.

- [ ] 668. [tab_creatures.cpp] Missing export_paths.writeLine calls in multiple export paths
- **JS Source**: `src/js/modules/tab_creatures.js` lines 747, 792, 923
- **Status**: Pending
- **Details**: JS writes to export_paths for RAW and non-RAW character-model export and standard export. C++ omits all writeLine calls and does not pass export_paths to export_model.

- [ ] 669. [tab_creatures.cpp] GLTF format.toLowerCase() not applied
- **JS Source**: `src/js/modules/tab_creatures.js` line 921
- **Status**: Pending
- **Details**: JS passes format.toLowerCase() to exportAsGLTF. C++ passes format as-is (uppercase). If the exporter is case-sensitive output may differ.

- [ ] 670. [tab_creatures.cpp] Error toast for model load missing View Log action button
- **JS Source**: `src/js/modules/tab_creatures.js` line 728
- **Status**: Pending
- **Details**: JS passes View Log action. C++ passes empty map. User cannot open runtime log from this error.

- [ ] 671. [tab_creatures.cpp] path.basename behavior not replicated in skin name
- **JS Source**: `src/js/modules/tab_creatures.js` line 668
- **Status**: Pending
- **Details**: Node.js path.basename produces trailing dot. C++ strips full .m2 extension producing no dot. Skin name stripping matches different substrings producing different display labels.

- [ ] 672. [tab_creatures.cpp] Missing Regex Enabled indicator in filter bar
- **JS Source**: `src/js/modules/tab_creatures.js` line 989
- **Status**: Pending
- **Details**: JS shows Regex Enabled div with tooltip when regex filters are active. C++ filter bar has no indicator.

- [ ] 673. [tab_creatures.cpp] Listbox context menu Copy names and Copy IDs not rendered in UI
- **JS Source**: `src/js/modules/tab_creatures.js` lines 983–986
- **Status**: Pending
- **Details**: JS renders ContextMenu with Copy names and Copy IDs options. C++ does not render an ImGui context menu popup. The functions exist but are never invoked from the UI.

- [ ] 674. [tab_creatures.cpp] Sorting uses byte comparison instead of locale-aware localeCompare
- **JS Source**: `src/js/modules/tab_creatures.js` line 1161
- **Status**: Pending
- **Details**: JS uses localeCompare. C++ uses name_a < name_b. Creatures with diacritics may sort differently.

- [ ] 675. [tab_decor.cpp] PNG/CLIPBOARD export branch does not short-circuit like JS
- **JS Source**: `src/js/modules/tab_decor.js` lines 129–140
- **Status**: Pending
- **Details**: JS returns immediately after preview export for PNG/CLIPBOARD; C++ closes the export stream but continues into full model export logic, changing export behavior for these formats.

- [ ] 676. [tab_decor.cpp] Decor list context menu open/interaction path differs from JS ContextMenu component flow
- **JS Source**: `src/js/modules/tab_decor.js` lines 234–237
- **Status**: Pending
- **Details**: JS renders a dedicated ContextMenu node for listbox selections (`Copy name(s)` / `Copy file data ID(s)`); C++ uses a manual popup path without equivalent Vue component lifecycle/wiring, deviating from original interaction flow.

- [ ] 677. [tab_decor.cpp] Missing return after PNG/CLIPBOARD export branch falls through
- **JS Source**: `src/js/modules/tab_decor.js` lines 138–140
- **Status**: Pending
- **Details**: JS does return after PNG/CLIPBOARD block. C++ has no return so execution falls through to the ExportHelper loop redundantly exporting all selected entries as models.

- [ ] 678. [tab_decor.cpp] create_renderer receives file_data_id instead of file_name
- **JS Source**: `src/js/modules/tab_decor.js` line 85
- **Status**: Pending
- **Details**: JS passes file_name (string) as 5th argument to create_renderer. C++ passes file_data_id (uint32_t). Parameter type mismatch.

- [ ] 679. [tab_decor.cpp] getActiveRenderer() only returns M2 renderer not any active renderer
- **JS Source**: `src/js/modules/tab_decor.js` line 611
- **Status**: Pending
- **Details**: JS getActiveRenderer returns the single active_renderer which could be M2, WMO, or M3. C++ returns active_renderer_result.m2.get() returning nullptr when active model is WMO or M3.

- [ ] 680. [tab_decor.cpp] Error toast for preview_decor missing View Log action button
- **JS Source**: `src/js/modules/tab_decor.js` line 119
- **Status**: Pending
- **Details**: JS includes View Log action. C++ passes empty map.

- [ ] 681. [tab_decor.cpp] helper.mark on failure missing stack trace parameter
- **JS Source**: `src/js/modules/tab_decor.js` line 184
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

- [ ] 682. [tab_decor.cpp] Sorting uses byte comparison instead of locale-aware localeCompare
- **JS Source**: `src/js/modules/tab_decor.js` lines 401–405
- **Status**: Pending
- **Details**: JS uses localeCompare. C++ uses name_a < name_b after tolower. Different sort for non-ASCII names.

- [ ] 683. [tab_decor.cpp] Missing scrub pause/resume behavior on animation slider
- **JS Source**: `src/js/modules/tab_decor.js` lines 546–561
- **Status**: Pending
- **Details**: JS start_scrub saves paused state and pauses animation while dragging. C++ SliderInt has no mouse-down/up event handling.

- [ ] 684. [tab_decor.cpp] Missing Regex Enabled indicator in filter bar
- **JS Source**: `src/js/modules/tab_decor.js` line 240
- **Status**: Pending
- **Details**: JS shows Regex Enabled div above filter input. C++ filter bar has no such indicator.

- [ ] 685. [tab_decor.cpp] Missing tooltips on all sidebar Preview and Export checkboxes
- **JS Source**: `src/js/modules/tab_decor.js` lines 314–354
- **Status**: Pending
- **Details**: JS has title attributes for every checkbox. C++ has no tooltip calls for any of them.

- [ ] 686. [tab_decor.cpp] Category group header click-to-toggle-all not implemented
- **JS Source**: `src/js/modules/tab_decor.js` line 301
- **Status**: Pending
- **Details**: JS clicking category name toggles all subcategories on/off. C++ uses TreeNodeEx which only expands/collapses. The toggle function exists but is never called from render.

- [ ] 687. [tab_decor.cpp] WMO Groups and Doodad Sets use manual checkbox loop instead of CheckboxList
- **JS Source**: `src/js/modules/tab_decor.js` lines 363–369
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for both. C++ uses manual ImGui::Checkbox loops instead of checkboxlist::render(). Inconsistent and may cause visual differences.

- [ ] 688. [tab_decor.cpp] Context menu popup may never open
- **JS Source**: `src/js/modules/tab_decor.js` lines 233–237
- **Status**: Pending
- **Details**: The DecorListboxContextMenu popup requires ImGui::OpenPopup to be called. The handle_listbox_context callback does not open this popup. The popup rendering code will never trigger.

- [ ] 689. [blp.cpp] Canvas rendering APIs (`toCanvas`/`drawToCanvas`) are not ported
- **JS Source**: `src/js/casc/blp.js` lines 95, 103–117, 221–234
- **Status**: Pending
- **Details**: JS exposes canvas-based rendering and uses `toCanvas(...).toDataURL()` in `getDataURL()`. C++ removes canvas APIs and routes `getDataURL()` through PNG encoding, which changes available surface API and rendering path behavior.

- [ ] 690. [blp.cpp] WebP/PNG save methods are synchronous instead of JS async Promise APIs
- **JS Source**: `src/js/casc/blp.js` lines 146–194
- **Status**: Pending
- **Details**: JS implements `async saveToPNG`, `async toWebP`, and `async saveToWebP`. C++ equivalents are synchronous, changing completion/error semantics for consumers expecting Promise-based behavior.

- [ ] 691. [blp.cpp] 4-bit alpha nibble indexing behavior differs from original JS
- **JS Source**: `src/js/casc/blp.js` lines 286–299
- **Status**: Pending
- **Details**: JS uses `this.rawData[this.scaledLength + (index / 2)]` (floating index for odd values), while C++ uses integer division. This intentionally fixes a JS bug but still deviates from original runtime behavior.

- [ ] 692. [blp.cpp] DXT block overrun guard differs from JS equality check
- **JS Source**: `src/js/casc/blp.js` lines 323–324
- **Status**: Pending
- **Details**: JS skips only when `this.rawData.length === pos`. C++ skips when `pos >= rawData_.size()`, adding defensive handling for overrun states and altering edge-case decode behavior.

- [ ] 693. [blp.cpp] `toBuffer()` fallback differs for unknown encodings
- **JS Source**: `src/js/casc/blp.js` lines 242–250
- **Status**: Pending
- **Details**: JS has no default branch and therefore returns `undefined` for unsupported encodings. C++ returns an empty `BufferWrapper`, changing caller-observed fallback behavior.

- [ ] 694. [blp.cpp] `getDataURL()` implementation differs — JS uses `toCanvas().toDataURL()`, C++ uses `toPNG()` then `BufferWrapper::getDataURL()`
- **JS Source**: `src/js/casc/blp.js` lines 94–96
- **Status**: Pending
- **Details**: JS `getDataURL()` creates an HTML canvas, draws the BLP to it, and calls `canvas.toDataURL()` which produces a `data:image/png;base64,...` string. C++ `getDataURL()` calls `toPNG()` to get PNG data, then `BufferWrapper::getDataURL()` to encode it as a data URL. The C++ approach produces the same data URL format but bypasses the canvas entirely. This is a documented deviation (comment at lines 100–103).

- [ ] 695. [blp.cpp] `toCanvas()` and `drawToCanvas()` methods not ported — browser-specific
- **JS Source**: `src/js/casc/blp.js` lines 103–117, 221–234
- **Status**: Pending
- **Details**: JS `toCanvas()` creates an HTML `<canvas>` element and draws the BLP onto it. `drawToCanvas()` takes an existing canvas and draws the BLP pixels using 2D context methods (`createImageData`, `putImageData`). These are browser-specific APIs with no C++ equivalent. The C++ port replaces these with `toPNG()`, `toBuffer()`, and `toUInt8Array()` which provide the same pixel data without canvas.

- [ ] 696. [blp.cpp] `dataURL` property initialized to `null` in JS constructor, C++ uses `std::optional<std::string>`
- **JS Source**: `src/js/casc/blp.js` line 86
- **Status**: Pending
- **Details**: JS sets `this.dataURL = null` in the BLPImage constructor. C++ declares `std::optional<std::string> dataURL` in the header which defaults to `std::nullopt`. The C++ `getDataURL()` method doesn't cache to this field (it relies on `BufferWrapper::getDataURL()` caching instead). The JS `getDataURL()` also doesn't set this field — it returns from `toCanvas().toDataURL()`. The `dataURL` field appears to be unused caching infrastructure in both versions.

- [ ] 697. [blp.cpp] `toWebP()` uses libwebp C API directly instead of JS `webp-wasm` module
- **JS Source**: `src/js/casc/blp.js` lines 157–182
- **Status**: Pending
- **Details**: JS uses `webp-wasm` npm module with `webp.encode(imgData, options)` for WebP encoding. C++ uses libwebp's C API directly (`WebPEncodeLosslessRGBA` / `WebPEncodeRGBA`). The JS `options` object `{ lossless: true }` or `{ quality: N }` maps to C++ separate code paths for quality == 100 (lossless) vs lossy. Functionally equivalent.

- [ ] 698. [blp.cpp] `_getCompressed()` DXT color interpolation uses integer division in C++ which may produce different rounding vs JS floating-point division
- **JS Source**: `src/js/casc/blp.js` lines 341–346
- **Status**: Pending
- **Details**: JS `colours[i + 8] = (c + d) / 2` and `colours[i + 8] = (2 * c + d) / 3` use JS number (float64) division. C++ uses integer division since `colours` is `int[]`. For even sums, `/2` is exact; for odd sums, C++ truncates toward zero while JS keeps the decimal. For `/3`, C++ truncates while JS produces exact fraction. When these float values are eventually stored as uint8 pixel values, JS truncates via typed array assignment while C++ truncates at division. The difference is at most 1 LSB for some pixel values.

- [ ] 699. [blp.cpp] DXT5 alpha interpolation uses `| 0` (bitwise OR zero) in JS to floor, C++ uses integer division which truncates toward zero
- **JS Source**: `src/js/casc/blp.js` lines 389, 395
- **Status**: Pending
- **Details**: JS `colours[i + 1] = (((5 - i) * a0 + i * a1) / 5) | 0` uses `| 0` to convert float to int32 (truncates toward zero). C++ uses plain integer division `alphaColours[i + 1] = ((5 - i) * a0 + i * a1) / 5` which also truncates toward zero (all values are int). These should produce identical results since all operands are non-negative integers.

- [ ] 700. [char-texture-overlay.cpp] Overlay button visibility updater is a no-op instead of internally toggling visibility
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 23–31
- **Status**: Pending
- **Details**: JS toggles `#chr-overlay-btn` display between `flex` and `none` inside `update_button_visibility()`, while C++ leaves `update_button_visibility()` empty and relies on external rendering checks.

- [ ] 701. [char-texture-overlay.cpp] Active-layer reattach flow is stubbed out
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 63–70
- **Status**: Pending
- **Details**: JS schedules `process.nextTick()` to re-append the active canvas after tab changes; C++ `ensureActiveLayerAttached()` is intentionally a no-op, so no equivalent reattach path runs.

- [ ] 702. [char-texture-overlay.cpp] Missing event registrations for tab switch and overlay navigation
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 71–84
- **Status**: Pending
- **Details**: JS registers `core.events.on('screen-tab-characters', ensure_active_layer_attached)`, `core.events.on('click-chr-next-overlay', ...)`, `core.events.on('click-chr-prev-overlay', ...)` at module load time. C++ exposes nextOverlay/prevOverlay/ensureActiveLayerAttached as public functions but has no event hook registration. Callers must invoke these manually; the implicit event-driven wiring from JS is absent.

- [ ] 703. [char-texture-overlay.cpp] getLayerCount/nextOverlay/prevOverlay are C++ additions not in JS module exports
- **JS Source**: `src/js/ui/char-texture-overlay.js` lines 86–89
- **Status**: Pending
- **Details**: JS module.exports only exports: add, remove, ensureActiveLayerAttached, getActiveLayer. C++ header additionally exposes getLayerCount(), nextOverlay(), prevOverlay(). The overlay navigation is handled by event listeners in JS that are internal to the module; C++ makes them public API. Not a bug, but an API surface deviation.

- [ ] 704. [character-appearance.cpp] apply_customization_textures init() ordering differs from JS
- **JS Source**: `src/js/ui/character-appearance.js` lines 130–145
- **Status**: Pending
- **Details**: JS constructs CharMaterialRenderer, inserts it into the chr_materials map, then calls `await chr_material.init()`. C++ calls `mat->init()` BEFORE inserting into `chr_materials` map. If init() throws in C++, the material won't be in the map (clean); in JS it would be present in a partially initialized state. Subtle ordering difference in error scenarios.

- [ ] 705. [character-appearance.cpp] setTextureTarget API decomposed from composite objects to scalar parameters
- **JS Source**: `src/js/ui/character-appearance.js` lines 155–170
- **Status**: Pending
- **Details**: JS calls `setTextureTarget(chr_material, { FileDataID, ChrModelTextureTargetID }, { X, Y, Width, Height })` with composite objects. C++ decomposes these into individual scalar parameters. Correct adaptation for C++ but callers must match the decomposed C++ signature exactly.

- [ ] 706. [GLContext.cpp] Context creation and capability detection behavior differs from JS canvas/WebGL2 path
- **JS Source**: `src/js/3D/gl/GLContext.js` lines 29–41, 55–63
- **Status**: Pending
- **Details**: JS creates the context with per-call options and throws `WebGL2 not supported` if context acquisition fails; it also conditionally enables anisotropy/float-texture extension flags. C++ assumes an already-created GL context and sets some capability flags via desktop-core assumptions instead of matching JS extension-availability behavior.

- [ ] 707. [ShaderProgram.cpp] `_compile` method handles partial shader failure differently from JS
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 29–36
- **Status**: Pending
- **Details**: When one shader compiles successfully but the other fails, JS `_compile` (line 35-36) simply returns without deleting the successfully compiled shader, leaking a WebGL resource until garbage collection. C++ `_compile` (lines 30-35) correctly deletes both shaders on partial failure (`if (vert_shader) glDeleteShader(vert_shader); if (frag_shader) glDeleteShader(frag_shader);`). This is a behavioral improvement over JS but is technically a deviation from the original logic. Note: JS `recompile()` (line 248-256) does handle this correctly — only `_compile` has the issue.

- [ ] 708. [ShaderProgram.cpp] `set_uniform_3fv`/`set_uniform_4fv`/`set_uniform_mat4_array` have extra count parameter
- **JS Source**: `src/js/3D/gl/ShaderProgram.js` lines 187–234
- **Status**: Pending
- **Details**: JS `set_uniform_3fv(name, value)` calls `gl.uniform3fv(loc, value)` where the WebGL2 API infers the count from the typed array length. C++ `set_uniform_3fv(name, value, count=1)` takes an explicit `count` parameter (defaulting to 1) because OpenGL's `glUniform3fv(loc, count, value)` requires it. Same applies to `set_uniform_4fv` and `set_uniform_mat4_array`. While single-value calls work identically (count defaults to 1), callers passing arrays of multiple vec3/vec4/mat4 values must specify the correct count in C++. This is a necessary C++ adaptation but changes the API contract.

- [ ] 709. [Shaders.cpp] C++ adds automatic _unregister_fn callback on ShaderProgram not present in JS
- **JS Source**: `src/js/3D/Shaders.js` lines 56–72
- **Status**: Pending
- **Details**: C++ `create_program()` (Shaders.cpp lines 79–83) installs a static `_unregister_fn` callback on `gl::ShaderProgram` that automatically calls `shaders::unregister()` when a ShaderProgram is destroyed. JS has no equivalent auto-cleanup mechanism — callers must explicitly call `unregister(program)` (Shaders.js line 78–86). This means in C++, a program is automatically removed from `active_programs` on destruction, while in JS a disposed program remains in the set until manually unregistered. This changes `reload_all()` behavior: JS could attempt to recompile stale programs that were not explicitly unregistered, while C++ never encounters this scenario.

- [ ] 710. [Texture.cpp] `getTextureFile()` return contract differs from JS async/null behavior
- **JS Source**: `src/js/3D/Texture.js` lines 32–41
- **Status**: Pending
- **Details**: JS returns a Promise from `async getTextureFile()` and yields `null` when unset; C++ returns `std::optional<BufferWrapper>` synchronously, changing both async behavior and API shape.

- [ ] 711. [Skin.cpp] `load()` API timing differs from JS Promise-based async flow
- **JS Source**: `src/js/3D/Skin.js` lines 20–23, 96–100
- **Status**: Pending
- **Details**: JS exposes `async load()` and awaits CASC file retrieval (`await core.view.casc.getFile(...)`), while C++ `Skin::load()` is synchronous and throws directly, changing caller timing/error-propagation semantics.

- [ ] 712. [GridRenderer.cpp] GLSL shader version differs — C++ uses `#version 460 core`, JS uses `#version 300 es`
- **JS Source**: `src/js/3D/renderers/GridRenderer.js` lines 9–35
- **Status**: Pending
- **Details**: C++ vertex/fragment shaders use `#version 460 core` (desktop OpenGL 4.6). JS uses `#version 300 es` with `precision highp float;` (WebGL 2.0/OpenGL ES 3.0). The shader logic is identical but the version/precision declarations differ. This is an expected platform adaptation but should be documented.

- [ ] 713. [ShadowPlaneRenderer.cpp] GLSL shader version differs — C++ uses `#version 460 core`, JS uses `#version 300 es`
- **JS Source**: `src/js/3D/renderers/ShadowPlaneRenderer.js` lines 9–40
- **Status**: Pending
- **Details**: Same issue as GridRenderer — C++ uses desktop GL 4.6 shaders, JS uses WebGL 2.0 shaders. Logic is identical but version/precision declarations differ. Expected platform adaptation.

- [ ] 714. [CameraControlsGL.cpp] Event listener lifecycle from JS `init()/dispose()` is not ported equivalently
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 198–221
- **Status**: Pending
- **Details**: JS `init()` registers DOM/document listeners and `dispose()` removes document listeners, but C++ relies on externally forwarded events and only resets state, changing ownership/lifecycle semantics.

- [ ] 715. [CameraControlsGL.cpp] Input default-handling behavior differs from JS browser event flow
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` lines 223–248, 257–262, 309–329
- **Status**: Pending
- **Details**: JS calls `preventDefault()` (and wheel `stopPropagation()`), while C++ handler methods have no equivalent suppression path, so browser-default behavior parity is not represented.

- [ ] 716. [CameraControlsGL.cpp] Mouse-down focus fallback differs from JS
- **JS Source**: `src/js/3D/camera/CameraControlsGL.js` line 226
- **Status**: Pending
- **Details**: JS falls back to `window.focus()` when `dom_element.focus` is unavailable; C++ only invokes `dom_element.focus` if present and has no fallback focus path.

- [ ] 717. [CharacterCameraControlsGL.cpp] DOM/document listener registration-removal flow differs from JS
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 27–35, 42–43, 51–52, 122–129, 170–175
- **Status**: Pending
- **Details**: JS stores handler refs, registers mousedown/wheel/contextmenu listeners in constructor, dynamically attaches/removes document mousemove/mouseup listeners, and removes listeners in `dispose()`; C++ omits this lifecycle and depends on caller-forwarded events.

- [ ] 718. [CharacterCameraControlsGL.cpp] Event suppression parity is missing in mouse handlers
- **JS Source**: `src/js/3D/camera/CharacterCameraControlsGL.js` lines 45, 54, 68, 114, 133–135
- **Status**: Pending
- **Details**: JS calls `preventDefault()` during rotate/pan interactions and both `preventDefault()`/`stopPropagation()` on wheel; C++ has no equivalent event suppression behavior.

- [ ] 719. [uv-drawer.cpp] Output format changed from data URL string to raw RGBA pixels
- **JS Source**: `src/js/ui/uv-drawer.js` lines 1–5, 45–50
- **Status**: Pending
- **Details**: JS `generateUVLayerDataURL` returns `canvas.toDataURL('image/png')` (a data URL string). C++ `generateUVLayerPixels` returns `std::vector<uint8_t>` raw RGBA pixel data. The caller (model-viewer-utils.cpp) handles PNG encoding and GL texture upload separately. Function name and return type both differ.

- [ ] 720. [uv-drawer.cpp] Line rendering algorithm differs from Canvas 2D API
- **JS Source**: `src/js/ui/uv-drawer.js` lines 10–45
- **Status**: Pending
- **Details**: JS uses Canvas 2D `ctx.strokeStyle`, `ctx.lineWidth = 0.5`, `ctx.beginPath/moveTo/lineTo/stroke` with all triangles in a single path (overlapping edges drawn at same alpha). C++ uses Xiaolin Wu's anti-aliased line algorithm drawing each segment individually with alpha compositing. Visual results will differ at edge overlaps and anti-aliasing quality — not pixel-identical to the JS version.

- [ ] 721. [model-viewer-gl.cpp] Character-mode reactive watchers are replaced with render-time polling
- **JS Source**: `src/js/components/model-viewer-gl.js` lines 469–473
- **Status**: Pending
- **Details**: JS registers Vue `$watch` handlers for `chrUse3DCamera`, `chrRenderShadow`, and `chrModelLoading`. C++ polls these values each frame in `renderWidget`, changing update/lifecycle behavior.

- [ ] 722. [model-viewer-gl.cpp] Active renderer contract is narrowed from JS duck-typed renderer to `M2RendererGL`
- **JS Source**: `src/js/components/model-viewer-gl.js` lines 223–226, 304–307, 409–416
- **Status**: Pending
- **Details**: JS uses optional/duck-typed renderer checks (`getActiveRenderer?.()` + method checks). C++ hard-types active renderer as `M2RendererGL*`, reducing parity with the original renderer-agnostic method contract.

- [ ] 723. [model-viewer-gl.cpp] JS `controls.update()` called unconditionally but C++ splits into orbit/char controls with null checks
- **JS Source**: `src/js/components/model-viewer-gl.js` line 250
- **Status**: Pending
- **Details**: JS line 250 calls `this.controls.update()` unconditionally — `this.controls` is always set to either `CameraControlsGL` or `CharacterCameraControlsGL`. C++ (lines 434–441) splits this into `if (use_character_controls && char_controls)` and `else if (orbit_controls)`, which is functionally equivalent. However, the JS also calls `sync_camera_*` before update implicitly via the camera adapter pattern, while C++ manually calls `sync_camera_to_gl`/`sync_camera_to_char_gl`. This is correct but diverges structurally.

- [ ] 724. [model-viewer-gl.cpp] JS `context.controls = this.controls` stores single reference; C++ splits into `controls_orbit` and `controls_character`
- **JS Source**: `src/js/components/model-viewer-gl.js` line 395
- **Status**: Pending
- **Details**: JS `recreate_controls` sets `this.context.controls = this.controls` (line 395) — a single duck-typed reference. C++ sets `context.controls_orbit` and `context.controls_character` separately (lines 625–636). Any code that accesses `context.controls` in JS would need to check both in C++. This structural difference could cause issues if external code expects a single controls reference.

- [ ] 725. [model-viewer-gl.cpp] JS `activeRenderer.animation_paused` is a property but C++ uses `is_animation_paused()` method
- **JS Source**: `src/js/components/model-viewer-gl.js` line 228
- **Status**: Pending
- **Details**: JS checks `!activeRenderer.animation_paused` (line 228) as a property access. C++ calls `!activeRenderer->is_animation_paused()` (line 408) as a method. This is functionally equivalent if `is_animation_paused()` returns the same value, but if the M2RendererGL interface ever changes the method name or semantics, this could diverge.

- [ ] 726. [model-viewer-gl.cpp] JS `beforeUnmount` cleans up watcher array but C++ has no equivalent watcher disposal
- **JS Source**: `src/js/components/model-viewer-gl.js` lines 509–512
- **Status**: Pending
- **Details**: JS `beforeUnmount` iterates `this.watchers` and calls each watcher function to unsubscribe (lines 509–512). C++ `dispose()` (lines 725–757) does not have an equivalent watcher cleanup — instead it uses polling in `renderWidget` (lines 798–820) which stops automatically when rendering stops. This is functionally equivalent since polling ceases, but the patterns differ.

- [ ] 727. [model-viewer-gl.cpp] JS `window.addEventListener('resize', this.onResize)` but C++ FBO resize is implicit via ImGui layout
- **JS Source**: `src/js/components/model-viewer-gl.js` lines 477–494
- **Status**: Pending
- **Details**: JS registers a `window.resize` event handler that reads `container.getBoundingClientRect()`, sets canvas size to `width * devicePixelRatio`, and updates viewport/camera aspect. C++ checks `fbo_width != width || fbo_height != height` each frame (line 783) and recreates the FBO. Both achieve the same result but C++ never calls `window.removeEventListener` cleanup (handled implicitly by stopping renders).

- [ ] 728. [model-viewer-gl.cpp] `handle_input` only processes events when `IsItemHovered` — JS events are document-level
- **JS Source**: `src/js/components/model-viewer-gl.js` lines 9 (CameraControlsGL constructor adds document listeners)
- **Status**: Pending
- **Details**: JS `CameraControlsGL` and `CharacterCameraControlsGL` register mousemove/mouseup on `document`, meaning mouse drag continues even when the cursor leaves the canvas. C++ `handle_input` (line 318) returns early if `!ImGui::IsItemHovered()`, which means dragging the camera and moving the mouse outside the widget area will stop the camera update. The comment on line 359 says "always forward, regardless of hover, since panning may extend outside" but the early return on line 318 contradicts this.

- [ ] 729. [model-viewer-utils.cpp] Clipboard preview export copies base64 text instead of PNG image data
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 299–303
- **Status**: Pending
- **Details**: JS writes PNG binary payload to the clipboard (`clipboard.set(..., 'png', true)`), while C++ uses `ImGui::SetClipboardText(buf.toBase64().c_str())`, resulting in text clipboard content rather than image clipboard content.

- [ ] 730. [model-viewer-utils.cpp] Animation selection guard treats empty string as null/undefined
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 251–253
- **Status**: Pending
- **Details**: JS exits only for `null`/`undefined`; C++ exits for `selected_animation_id.empty()`, which changes behavior for explicit empty-string IDs.

- [ ] 731. [model-viewer-utils.cpp] WMO renderer/export constructor inputs differ from JS filename-based path
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 208, 405
- **Status**: Pending
- **Details**: JS constructs `WMORendererGL`/`WMOExporter` with `file_name`; C++ uses `file_data_id`-based constructors and ignores the filename parameter in these paths.

- [ ] 732. [model-viewer-utils.cpp] View-state proxy is hardcoded to three prefixes instead of dynamic property resolution
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 503–528
- **Status**: Pending
- **Details**: JS proxy resolves fields dynamically via `core.view[prefix + ...]`; C++ only maps `"model"`, `"decor"`, and `"creature"` with explicit branches, removing generic-prefix behavior.

- [ ] 733. [model-viewer-utils.cpp] export_preview CLIPBOARD copies base64 text instead of PNG image data
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 115–120
- **Status**: Pending
- **Details**: JS `clipboard.set(buf.toBase64(), 'png', true)` copies actual PNG image data to the system clipboard so pasting in an image editor shows the screenshot. C++ `ImGui::SetClipboardText(buf.toBase64().c_str())` copies base64-encoded text. Pasting in an image editor produces text, not an image. A platform-specific clipboard API (Win32 CF_DIB, X11 image/png) is needed for parity.

- [ ] 734. [model-viewer-utils.cpp] WMO renderer and exporter pass file_data_id instead of file_name
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 85–95, 190–210
- **Status**: Pending
- **Details**: JS `WMORendererGL(data, file_name, gl_context, show_textures)` and `WMOExporter(data, file_name)` use the file name string. C++ passes `file_data_id` (uint32_t) to both `WMORendererGL(data, file_data_id, ctx, show_textures)` and `WMOExporter(data, file_data_id, casc)`. The `file_name` parameter is explicitly discarded via `(void)file_name`. This is a documented API adaptation but changes how WMO resources are resolved internally.

- [ ] 735. [model-viewer-utils.cpp] create_view_state only supports 3 hardcoded prefixes
- **JS Source**: `src/js/ui/model-viewer-utils.js` lines 235–270
- **Status**: Pending
- **Details**: JS uses dynamic property access `core.view[prefix + 'TexturePreviewURL']` supporting any prefix string. C++ hardcodes only "model", "decor", "creature" — any other prefix silently returns a ViewStateProxy with all nullptr fields. This matches current runtime usage but would break if new tab types are added without updating the if/else chain.

- [ ] 736. [MultiMap.cpp] MultiMap logic is not ported in the `.cpp` sibling translation unit
- **JS Source**: `src/js/MultiMap.js` lines 6–32
- **Status**: Pending
- **Details**: The JS sibling contains the full `MultiMap extends Map` implementation, but `src/js/MultiMap.cpp` only includes `MultiMap.h` and comments; line-by-line implementation parity is not present in the `.cpp` file itself.

- [ ] 737. [MultiMap.cpp] Public API model differs from JS `Map` subclass contract
- **JS Source**: `src/js/MultiMap.js` lines 6, 20–28, 32
- **Status**: Pending
- **Details**: JS exports an actual `Map` subclass with standard `Map` behavior/interop, while C++ exposes a template wrapper (header implementation) returning `std::variant` pointers and not `Map`-equivalent runtime semantics.

- [ ] 738. [M2Loader.cpp] Primary loader methods are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 37, 67, 87, 146, 332
- **Status**: Pending
- **Details**: JS exposes async `load`, `getSkin`, `loadAnims`, `loadAnimsForIndex`, and `parseChunk_MD21`; C++ ports these as synchronous methods, altering call/await semantics.

- [ ] 739. [M2Loader.cpp] `loadAnims` error propagation differs from JS
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 87–138
- **Status**: Pending
- **Details**: JS `loadAnims` does not catch loader/CASC failures (Promise rejects). C++ wraps per-entry loads in `try/catch` and continues, swallowing errors and changing failure behavior.

- [ ] 740. [M2Loader.cpp] Model-name null stripping differs from original JS behavior
- **JS Source**: `src/js/3D/loaders/M2Loader.js` line 792
- **Status**: Pending
- **Details**: JS calls `fileName.replace('\0', '')` (single replacement call result not reassigned), while C++ removes all null bytes in-place; resulting `name` values differ.

- [ ] 741. [M2Loader.cpp] `loadAnimsForIndex()` catch block logs fileDataID instead of animation.id
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 199–201
- **Status**: Pending
- **Details**: JS catch block logs `"Failed to load .anim file for animation " + animation.id + ": " + e.message`, identifying the animation by its `id` field. C++ catch block logs `"Failed to load .anim file (fileDataID={}): {}"` using the CASC `fileDataID` and `e.what()`. The logged identifier differs — JS reports the animation's logical `id`, C++ reports the file data ID. Message format also differs.

- [ ] 742. [M2Loader.cpp] `parseChunk_SFID` guard check uses `viewCount == 0` instead of undefined-check
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 272–273
- **Status**: Pending
- **Details**: JS checks `if (this.viewCount === undefined)` — true only when MD21 hasn't been parsed yet (property never assigned). C++ checks `if (this->viewCount == 0)`. Since the C++ member is default-initialized to 0, this would throw even AFTER MD21 sets viewCount to a legitimate 0 (a model with no views). The JS would NOT throw in that case because viewCount would be defined as 0 (`0 !== undefined`). The guard should track whether MD21 has been parsed (e.g., using a bool flag), not check for `viewCount == 0`.

- [ ] 743. [M2Loader.cpp] `parseChunk_TXID` guard check uses `textures.empty()` instead of undefined-check
- **JS Source**: `src/js/3D/loaders/M2Loader.js` lines 290–291
- **Status**: Pending
- **Details**: JS checks `if (this.textures === undefined)` — true only when MD21 hasn't been parsed (textures array never created). C++ checks `if (this->textures.empty())`. If MD21 parses 0 textures, JS would NOT throw (textures is a defined empty array), but C++ WOULD throw. Same semantic issue as the SFID check — should track MD21 parse state rather than vector emptiness.

- [ ] 744. [M2LegacyLoader.cpp] `load`/`getSkin` APIs are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/M2LegacyLoader.js` lines 34, 819
- **Status**: Pending
- **Details**: JS exposes `async load()` and `async getSkin(index)` while C++ exposes synchronous `void load()` and `LegacyM2Skin& getSkin(int)`, changing await/timing behavior.

- [ ] 745. [M2Generics.cpp] Error message text differs in useAnims branch ("Unhandled" vs "Unknown")
- **JS Source**: `src/js/3D/loaders/M2Generics.js` lines 78, 101
- **Status**: Pending
- **Details**: JS `read_m2_array_array` has two separate switch blocks — the useAnims branch (line 78) throws `"Unhandled data type: ${dataType}"` while the non-useAnims branch (line 101) throws `"Unknown data type: ${dataType}"`. C++ collapses both branches into a single `read_value()` helper that always throws `"Unknown data type: "` for both paths. The error message for the useAnims branch differs from the original JS.

- [ ] 746. [M3Loader.cpp] Loader methods are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/M3Loader.js` lines 67, 104, 269, 277, 299, 315
- **Status**: Pending
- **Details**: JS exposes async `load`, `parseChunk_M3DT`, and async sub-chunk parsers; C++ ports these paths as synchronous calls, changing API timing/await semantics.

- [ ] 747. [MDXLoader.cpp] `load` API is synchronous instead of JS async Promise-based method
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` line 28
- **Status**: Pending
- **Details**: JS exposes `async load()` while C++ exposes synchronous `void load()`, changing await/timing behavior.

- [ ] 748. [MDXLoader.cpp] ATCH handler fixes JS `readUInt32LE(-4)` bug without TODO_TRACKER documentation
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` line 404
- **Status**: Pending
- **Details**: JS ATCH handler has `this.data.readUInt32LE(-4)` which is a bug — `BufferWrapper._readInt` passes `_checkBounds(-16)` (always passes since remainingBytes >= 0 > -16), but `new Array(-4)` throws a `RangeError`. C++ correctly fixes this by using a saved `attachmentSize` variable. The fix has a code comment but per project conventions, deviations from the original JS should also be tracked in TODO_TRACKER.md.

- [ ] 749. [MDXLoader.cpp] Node registration deferred to post-parsing (structural deviation)
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` lines 208–209
- **Status**: Pending
- **Details**: In JS, `_read_node()` immediately assigns `this.nodes[node.objectId] = node` (line 209). In C++, this is deferred to `load()` because objects are moved into their final vectors after `_read_node` returns, invalidating any earlier pointers. This is correctly documented with a code comment and is functionally equivalent — all 9 node-bearing types (bones, helpers, attachments, eventObjects, hitTestShapes, particleEmitters, particleEmitters2, lights, ribbonEmitters) are properly registered. This is a structural deviation that should be tracked.

- [ ] 750. [SKELLoader.cpp] Loader animation APIs are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 36, 308, 407
- **Status**: Pending
- **Details**: JS exposes async `load`, `loadAnimsForIndex`, and `loadAnims`; C++ ports all three as synchronous methods, altering call/await behavior.

- [ ] 751. [SKELLoader.cpp] Animation-load failure handling differs from JS
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 332–344, 438–448
- **Status**: Pending
- **Details**: JS does not catch ANIM/CASC load failures in `loadAnimsForIndex`/`loadAnims` (Promise rejects). C++ catches exceptions, logs, and returns/continues, changing failure propagation.

- [ ] 752. [SKELLoader.cpp] Extra bounds check in `loadAnimsForIndex()` not present in JS
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 308–312
- **Status**: Pending
- **Details**: C++ adds `if (animation_index >= this->animations.size()) return false;` that does not exist in JS. In JS, accessing an out-of-bounds index on `this.animations` returns `undefined`, and `animation.flags` would throw a TypeError. C++ silently returns false instead of throwing, changing error behavior.

- [ ] 753. [SKELLoader.cpp] `skeletonBoneData` existence check uses `.empty()` instead of `!== undefined`
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 335–338, 441–444
- **Status**: Pending
- **Details**: JS checks `loader.skeletonBoneData !== undefined` — the property only exists if a SKID chunk was parsed. C++ checks `!loader->skeletonBoneData.empty()`. If ANIMLoader ever sets `skeletonBoneData` to a valid but empty buffer, JS would use it (property exists), but C++ would skip it (empty). This is a potential semantic difference depending on ANIMLoader behavior.

- [ ] 754. [SKELLoader.cpp] `loadAnims()` doesn't guard against missing `animFileIDs` like `loadAnimsForIndex()` does
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 319, 425
- **Status**: Pending
- **Details**: JS `loadAnimsForIndex()` has `if (!this.animFileIDs) return false;` (line 319) to guard against undefined `animFileIDs`. However, JS `loadAnims()` does NOT have this guard — it directly iterates `this.animFileIDs` (line 425), which would throw a TypeError if undefined. In C++, `animFileIDs` is always a default-constructed empty vector, so the for-loop is a no-op. The C++ is more robust but produces different behavior (graceful no-op vs JS crash).

- [ ] 755. [BONELoader.cpp] `load` API is synchronous instead of JS async Promise-based method
- **JS Source**: `src/js/3D/loaders/BONELoader.js` line 24
- **Status**: Pending
- **Details**: JS exposes `async load()` while C++ exposes synchronous `void load()`, changing API timing/await semantics.

- [ ] 756. [ANIMLoader.cpp] `load` API is synchronous instead of JS async Promise-based method
- **JS Source**: `src/js/3D/loaders/ANIMLoader.js` line 25
- **Status**: Pending
- **Details**: JS exposes `async load(isChunked = true)` while C++ exposes synchronous `void load(bool isChunked)`, changing API timing/await semantics.

- [ ] 757. [WMOLoader.cpp] `load`/`getGroup` APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 37, 64
- **Status**: Pending
- **Details**: JS exposes async `load()` and `getGroup(index)` while C++ ports both as synchronous methods, changing await/timing behavior.

- [ ] 758. [WMOLoader.cpp] `getGroup` omits JS filename-based fallback when `groupIDs` are missing
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 75–79
- **Status**: Pending
- **Details**: JS loads by `groupIDs[index]` when present, otherwise falls back to `getFileByName(this.fileName.replace(...))`; C++ hard-requires `groupIDs` and throws out-of-range instead of performing the filename fallback.

- [ ] 759. [WMOLoader.cpp] `getGroup()` passes `groupFileID` to child constructor instead of no fileID
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` line 80
- **Status**: Pending
- **Details**: JS creates group `WMOLoader` with `undefined` as fileID: `new WMOLoader(data, undefined, this.renderingOnly)`. The group's `fileDataID` and `fileName` are intentionally unset. C++ passes the actual `groupFileID`, triggering an unnecessary `casc::listfile::getByID()` lookup in the constructor and setting `fileDataID`/`fileName` on the group. The constructor should use fileID=0 (C++ sentinel for "undefined") to match JS.

- [ ] 760. [WMOLoader.cpp] MOGP `flags` field renamed to `groupFlags`, diverging from JS property name
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` line 361
- **Status**: Pending
- **Details**: JS MOGP handler stores `this.flags = data.readUInt32LE()`. C++ stores this as `this->groupFlags` (header line 218, MOGP parser line 426) because the C++ class already has `uint16_t flags` from MOHD. Any downstream code porting JS that accesses `wmoGroup.flags` for MOGP flags must use `groupFlags` in C++. This naming deviation matches the same issue found in WMOLegacyLoader.cpp (entry 376).

- [ ] 761. [WMOLoader.cpp] `hasLiquid` boolean is a C++ addition not present in JS
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 328–338
- **Status**: Pending
- **Details**: JS simply assigns `this.liquid = { ... }` in the MLIQ handler. Consumer code checks `if (this.liquid)` for existence. In C++, the `WMOLiquid liquid` member is always default-constructed, so a `bool hasLiquid = false` flag (header line 209) was added and set to `true` in `parse_MLIQ`. This is a reasonable C++ adaptation, but all downstream JS code that checks `if (this.liquid)` must be ported to check `if (this.hasLiquid)` instead — all consumers need verification.

- [ ] 762. [WMOLoader.cpp] MOPR filler skip uses `data.move(4)` but per wowdev.wiki entry is 8 bytes total
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 208–216
- **Status**: Pending
- **Details**: MOPR entry count is calculated as `chunkSize / 8` (8 bytes per entry). Fields read: `portalIndex(2) + groupIndex(2) + side(2)` = 6 bytes, then `data.move(4)` skips 4 more = 10 bytes per entry. Per wowdev.wiki, `SMOPortalRef` has a 2-byte `filler` (uint16_t), making entries 8 bytes. `data.move(2)` would be correct, not `data.move(4)`. Both JS and C++ match (C++ faithfully ports the JS), but both overread by 2 bytes per entry. The outer `data.seek(nextChunkPos)` corrects the position so parsing doesn't break, but this is a latent bug in both codebases.

- [ ] 763. [WMOLegacyLoader.cpp] `load`/internal load helpers/`getGroup` are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 33, 54, 86, 116
- **Status**: Pending
- **Details**: JS uses async `load`, `_load_alpha_format`, `_load_standard_format`, and `getGroup`; C++ ports these paths synchronously, changing await/timing behavior.

- [ ] 764. [WMOLegacyLoader.cpp] Group-loader initialization differs from JS in `getGroup`
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 146–149
- **Status**: Pending
- **Details**: JS creates group loaders with `fileID` undefined and explicitly seeds `group.version = this.version` before `await group.load()`. C++ does not pre-seed `version`, changing legacy group parse assumptions.

- [ ] 765. [WMOLegacyLoader.cpp] MOGP `flags` field renamed to `groupFlags`, diverging from JS property name
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` line 453
- **Status**: Pending
- **Details**: JS MOGP handler stores `this.flags = data.readUInt32LE()`. C++ stores this as `this->groupFlags` (header line 124, MOGP parser line 527) because the C++ class already has `uint16_t flags` from MOHD. Any downstream JS-ported code accessing `group.flags` for MOGP flags must use `group.groupFlags` in C++, which is a naming deviation that could cause porting bugs.

- [ ] 766. [WMOLegacyLoader.cpp] `getGroup` empty-check differs for `groupCount == 0` edge case
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 117–118
- **Status**: Pending
- **Details**: JS checks `if (!this.groups)` — tests whether the `groups` property was ever set (by MOHD handler). An empty JS array `new Array(0)` is truthy, so `!this.groups` is false when `groupCount == 0` — `getGroup` proceeds to the index check. C++ uses `if (this->groups.empty())` which returns true for `groupCount == 0`, incorrectly throwing the exception. A separate bool flag (e.g., `groupsInitialized`) would replicate JS semantics more faithfully.

- [ ] 767. [WDTLoader.cpp] `MWMO` string null handling differs from JS
- **JS Source**: `src/js/3D/loaders/WDTLoader.js` line 86
- **Status**: Pending
- **Details**: JS uses `.replace('\0', '')` (first match only), while C++ removes all `'\0'` bytes from the string, producing different `worldModel` values in edge cases.

- [ ] 768. [WDTLoader.cpp] `worldModelPlacement`/`worldModel`/MPHD fields not optional — cannot distinguish "chunk absent" from "chunk with zeros"
- **JS Source**: `src/js/3D/loaders/WDTLoader.js` lines 52–103
- **Status**: Pending
- **Details**: In JS, `this.worldModelPlacement` is only assigned when MODF is encountered. If MODF is absent, the property is `undefined` and `if (wdt.worldModelPlacement)` is false. In C++, `WDTWorldModelPlacement worldModelPlacement` is always default-constructed with zeroed fields, making it impossible to distinguish "MODF absent" from "MODF with zeros." Same for `worldModel` (always empty string vs. JS `undefined`) and MPHD fields (always 0 vs. JS `undefined`). Consider `std::optional<T>` for these fields.

- [ ] 769. [ADTExporter.cpp] `calculateUVBounds` skips chunks when `vertices` is empty, unlike JS truthiness check
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 267–268
- **Status**: Pending
- **Details**: JS only skips when `chunk`/`chunk.vertices` is missing; an empty typed array is still truthy and processing continues. C++ adds `chunk.vertices.empty()` as an additional skip condition, changing edge-case behavior.

- [ ] 770. [ADTExporter.cpp] Export API flow is synchronous instead of JS Promise-based `async export()`
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 309–367
- **Status**: Pending
- **Details**: JS `export()` is asynchronous and yields between CASC/file operations; C++ `exportTile()` performs the flow synchronously, changing timing/cancellation behavior relative to the original async path.

- [ ] 771. [ADTExporter.cpp] Scale factor check `!= 0` instead of `!== undefined` changes behavior for scale=0
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1270
- **Status**: Pending
- **Details**: JS checks `model.scale !== undefined ? model.scale / 1024 : 1`. C++ (ADTExporter.cpp ~line 1521) checks `model.scale != 0 ? model.scale / 1024.0f : 1.0f`. In JS, a `scale` of `0` would produce `0 / 1024 = 0` (a valid zero-scale value). In C++, a `scale` of `0` triggers the else branch and returns `1.0f`. This is a behavioral difference — a model with scale=0 would be invisible in JS but normal-sized in C++, affecting M2 doodad CSV export and placement transforms.

- [ ] 772. [ADTExporter.cpp] GL index buffer uses GL_UNSIGNED_INT (uint32) instead of JS GL_UNSIGNED_SHORT (uint16)
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1117–1118
- **Status**: Pending
- **Details**: JS creates `new Uint16Array(indices)` and uses `gl.UNSIGNED_SHORT` for the index element buffer when rendering alpha map tiles. C++ (ADTExporter.cpp ~lines 1327–1328) uses `sizeof(uint32_t)` and `GL_UNSIGNED_INT`. For the 16×16×145 = 37120 vertex terrain grid the indices fit in uint16, so both work, but the GPU draws with different index types. This is a minor fidelity deviation in the GL pipeline even though the visual output is identical.

- [ ] 773. [ADTExporter.cpp] Liquid JSON serialization uses explicit fields instead of JS spread operator
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1428–1438
- **Status**: Pending
- **Details**: JS uses `{ ...chunk, instances: enhancedInstances }` and `{ ...instance, worldPosition, terrainChunkPosition }` which copies *all* fields from the chunk/instance objects via the spread operator. C++ (ADTExporter.cpp ~lines 1744–1780) manually enumerates specific fields for JSON serialization. If the ADTLoader's liquid chunk or instance structs have any additional fields not listed in the C++ serialization, those fields would appear in the JS JSON output but be missing in the C++ output. This is a fragile pattern that could silently omit data if new fields are added to the loader structs.

- [ ] 774. [ADTExporter.cpp] STB_IMAGE_RESIZE_IMPLEMENTATION defined at file scope risks ODR violation
- **JS Source**: N/A (C++ build concern)
- **Status**: Pending
- **Details**: ADTExporter.cpp (line 10) defines `#define STB_IMAGE_RESIZE_IMPLEMENTATION` before including `<stb_image_resize2.h>`. If any other translation unit in the project also defines this macro, the linker will encounter duplicate symbol definitions (ODR violation). STB implementation macros should typically be isolated in a single dedicated .cpp file (like stb-impl.cpp already exists for stb_image/stb_image_write) to avoid this risk.

- [ ] 775. [WMOShaderMapper.cpp] Pixel shader enum naming deviates from JS export contract
- **JS Source**: `src/js/3D/WMOShaderMapper.js` lines 35, 90, 94
- **Status**: Pending
- **Details**: JS exports `WMOPixelShader.MapObjParallax`, while C++ renames this constant to `MapObjParallax_PS`; numeric mapping is preserved but exported identifier parity differs from the original module.

- [ ] 776. [CharMaterialRenderer.cpp] Core renderer methods are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 49, 105, 114, 170, 189, 231, 282
- **Status**: Pending
- **Details**: JS defines `init`, `reset`, `setTextureTarget`, `loadTexture`, `loadTextureFromBLP`, `compileShaders`, and `update` as async/await flows. C++ ports these methods synchronously, changing timing/error-propagation behavior expected by async call sites.

- [ ] 777. [CharMaterialRenderer.cpp] `getCanvas()` method missing — JS returns `this.glCanvas` for external use
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 55–59
- **Status**: Pending
- **Details**: JS `getCanvas()` returns the canvas element so external code can access the rendered character material texture. C++ has no equivalent method. Any code that calls `getCanvas()` will fail.

- [ ] 778. [CharMaterialRenderer.cpp] `update()` draw call placement differs — C++ draws inside blend-mode conditional instead of after it
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 382–417
- **Status**: Pending
- **Details**: JS draws ONCE per layer at line 417 (`this.gl.drawArrays(this.gl.TRIANGLES, 0, 6)`) OUTSIDE the blend-mode 4/6/7 if block. C++ has the draw call INSIDE the if block (for blend modes 4/6/7) at line ~534 AND inside the else block at line ~543. This means the draw happens in both branches but the pre-draw setup is different, which could lead to incorrect rendering for certain blend modes.

- [ ] 779. [CharMaterialRenderer.cpp] `setTextureTarget()` signature completely changed — JS takes full objects, C++ takes individual scalar parameters
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 114–144
- **Status**: Pending
- **Details**: JS signature is `setTextureTarget(chrCustomizationMaterial, charComponentTextureSection, chrModelMaterial, chrModelTextureLayer, useAlpha, blpOverride)` receiving full objects. C++ takes individual fields: `setTextureTarget(chrModelTextureTargetID, fileDataID, sectionX, sectionY, sectionWidth, sectionHeight, materialTextureType, materialWidth, materialHeight, textureLayerBlendMode, useAlpha, blpOverride)`. If JS objects contain additional fields used downstream, C++ will lose them.

- [ ] 780. [CharMaterialRenderer.cpp] `clearCanvas()` binds/unbinds FBO in C++ but JS does not
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 218–225
- **Status**: Pending
- **Details**: JS `clearCanvas()` operates on the current WebGL framebuffer (the canvas) without explicit bind/unbind. C++ explicitly binds `fbo_` before clearing and unbinds after. This is architecturally correct for desktop GL but represents a behavioral difference if called while another FBO is bound.

- [ ] 781. [CharMaterialRenderer.cpp] `dispose()` missing WebGL context loss equivalent
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` line 160
- **Status**: Pending
- **Details**: JS calls `gl.getExtension('WEBGL_lose_context').loseContext()` to invalidate all WebGL resources at once. C++ manually deletes each GL resource individually (FBO, textures, depth buffer, VAO). The C++ approach is correct for desktop GL but the order and completeness of cleanup should be verified.

- [ ] 782. [CharacterExporter.cpp] `get_item_id_for_slot` does not preserve JS falsy fallback semantics
- **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 342–345
- **Status**: Pending
- **Details**: JS uses `a || b || null`, so a slot `item_id` of `0` falls through to collection/null. C++ returns the first found `item_id` directly (including `0`), which differs for falsy-ID edge cases.

- [ ] 783. [CharacterExporter.cpp] remap_bone_indices truncates remap_table.size() to uint8_t causing incorrect comparison
- **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 126–138
- **Status**: Pending
- **Details**: C++ `remap_bone_indices()` (CharacterExporter.cpp line 147) compares `original_idx < static_cast<uint8_t>(remap_table.size())`. If `remap_table` has 256 or more entries, `static_cast<uint8_t>(256)` wraps to `0`, making the comparison `original_idx < 0` always false for unsigned types — no indices would be remapped at all. For tables with 257–511 entries, the truncated size wraps to small values, skipping valid remap entries for higher indices. JS has no such issue since `original_idx < remap_table.length` uses normal number comparison. The fix should be `static_cast<size_t>(original_idx) < remap_table.size()` or simply removing the cast.

- [ ] 784. [M2RendererGL.cpp] Multiple texture/skeleton/animation methods are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 362, 401, 431, 587, 663, 1336, 1371, 1399, 1424
- **Status**: Pending
- **Details**: JS keeps `load`, `_load_textures`, `loadSkin`, `_create_skeleton`, `playAnimation`, `overrideTextureType*`, and `applyReplaceableTextures` asynchronous; C++ ports them synchronously, changing promise timing and exception propagation behavior.

- [ ] 785. [M2RendererGL.cpp] Shader time uniform start-point differs from JS `performance.now()` baseline
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` line 1224
- **Status**: Pending
- **Details**: JS feeds `u_time` from `performance.now() * 0.001` (seconds since page load). C++ computes time from a static timestamp initialized on first render call, shifting animation phase baseline relative to JS.

- [ ] 786. [M2RendererGL.cpp] Reactive watchers not set up — `geosetWatcher`, `wireframeWatcher`, `bonesWatcher` completely missing
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 381–383
- **Status**: Pending
- **Details**: JS `load()` sets up three Vue watchers for geoset, wireframe, and bone visibility. C++ has empty braces at lines 495–496 with no watcher registration. UI changes to geosets won't trigger `updateGeosets()` automatically.

- [ ] 787. [M2RendererGL.cpp] `dispose()` missing watcher cleanup — no `geosetWatcher`, `wireframeWatcher`, `bonesWatcher` unregister
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1629–1633
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.geosetWatcher?.()`, `this.wireframeWatcher?.()`, `this.bonesWatcher?.()` to unregister watchers. C++ has no equivalent cleanup because watchers were never created.

- [ ] 788. [M2RendererGL.cpp] Bone matrix upload uses SSBO instead of uniform array
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1228–1231
- **Status**: Pending
- **Details**: JS uploads bone matrices via `gl.uniformMatrix4fv(loc, false, this.bone_matrices)` (uniform array). C++ uses Shader Storage Buffer Objects (SSBOs) at lines 1482–1495 (`glBindBuffer(GL_SHADER_STORAGE_BUFFER, ...)`, `glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ...)`). This is a valid modern OpenGL optimization but requires the shader to declare an SSBO binding instead of a uniform array. If the shader still expects a uniform array, bone animation will not work.

- [ ] 789. [M2RendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` line 1224
- **Status**: Pending
- **Details**: Same issue as M2LegacyRendererGL — JS uses `performance.now() * 0.001`, C++ uses elapsed time from first render call (lines 1477–1480).

- [ ] 790. [M2RendererGL.cpp] `overrideTextureTypeWithCanvas()` takes raw pixel data instead of canvas element
- **JS Source**: `src/js/3D/renderers/M2RendererGL.js` lines 1371–1390
- **Status**: Pending
- **Details**: JS takes `HTMLCanvasElement canvas` and calls `gl_tex.set_canvas(canvas, {...})`. C++ takes `const uint8_t* pixels, int width, int height` and calls `gl_tex->set_canvas(pixels, width, height, opts)`. This is an expected platform adaptation but the interface change means all callers must provide raw pixel data instead of a canvas reference.

- [ ] 791. [M2LegacyRendererGL.cpp] Loader/skin/animation entrypoints are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 183, 210, 252, 294, 455
- **Status**: Pending
- **Details**: JS exposes async `load`, `_load_textures`, `applyCreatureSkin`, `loadSkin`, and `playAnimation`; C++ ports these execution paths as synchronous methods, altering await behavior and scheduling.

- [ ] 792. [M2LegacyRendererGL.cpp] Reactive watchers not set up — `geosetWatcher` and `wireframeWatcher` completely missing
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 196–197
- **Status**: Pending
- **Details**: JS `load()` sets up Vue reactive watchers: `this.geosetWatcher = core.view.$watch(this.geosetKey, () => this.updateGeosets(), { deep: true })` and `this.wireframeWatcher = core.view.$watch('config.modelViewerWireframe', () => {}, { deep: true })`. C++ has an empty `if (reactive) {}` block at lines 216–217. Geoset changes from the UI will not trigger `updateGeosets()`. No polling replacement exists.

- [ ] 793. [M2LegacyRendererGL.cpp] `dispose()` missing watcher cleanup calls
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 1038–1039
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.geosetWatcher?.()` and `this.wireframeWatcher?.()` to unregister Vue watchers. C++ `dispose()` has no equivalent cleanup because watchers were never set up. If a polling mechanism is later added, cleanup must be added here too.

- [ ] 794. [M2LegacyRendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` line 926
- **Status**: Pending
- **Details**: JS sets `u_time` to `performance.now() * 0.001` (absolute time from page load in seconds). C++ uses a static `std::chrono::steady_clock` start time and computes elapsed seconds from first render call. Both produce monotonically increasing values suitable for shader animations, but absolute values will differ, potentially affecting time-dependent shader effects.

- [ ] 795. [M2LegacyRendererGL.cpp] Track data property names differ from JS — uses `flatValues`/`nestedTimestamps` instead of `values`/`timestamps`
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 581–609
- **Status**: Pending
- **Details**: JS accesses bone animation data as `bone.translation.values`, `bone.translation.timestamps`, `bone.translation.timestamps[anim_idx]`, `bone.translation.values[anim_idx]`. C++ accesses `bone.translation.flatValues`, `bone.translation.flatTimestamps`, `bone.translation.nestedTimestamps[anim_idx]`, `bone.translation.nestedValues[anim_idx]`. This implies the M2LegacyLoader stores data in a different structure, which must match the renderer's expectations.

- [ ] 796. [M2LegacyRendererGL.cpp] `loadSkin()` geoset assignment to `core.view` uses JSON serialization instead of direct assignment
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` lines 431–432
- **Status**: Pending
- **Details**: JS directly assigns `core.view[this.geosetKey] = this.geosetArray` and passes `this.geosetArray` to `GeosetMapper.map()`. C++ builds nlohmann::json objects manually from `geosetArray` entries (lines 490–498) and constructs a separate `mapper_geosets` vector for GeosetMapper (lines 500–507), then updates labels back. This indirect approach may not synchronize correctly if core.view expects the raw array reference.

- [ ] 797. [M2LegacyRendererGL.cpp] `setSlotFile` called as `setSlotFileLegacy` — function name differs from JS
- **JS Source**: `src/js/3D/renderers/M2LegacyRendererGL.js` line 226
- **Status**: Pending
- **Details**: JS calls `textureRibbon.setSlotFile(ribbonSlot, fileName, this.syncID)`. C++ calls `texture_ribbon::setSlotFileLegacy(ribbonSlot, fileName, syncID)` at line 255. The C++ function has a different name, which may indicate it has different behavior or was renamed for disambiguation.

- [ ] 798. [M3RendererGL.cpp] Load APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 56, 76
- **Status**: Pending
- **Details**: JS defines async `load` and `loadLOD`; C++ ports both as synchronous calls, changing await/timing semantics.

- [ ] 799. [M3RendererGL.cpp] `getBoundingBox()` missing vertex array empty check
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 174–175
- **Status**: Pending
- **Details**: JS checks `if (!this.m3 || !this.m3.vertices) return null`. C++ only checks `if (!m3) return std::nullopt` at line 198–199 without checking if vertices array is empty. If m3 is loaded but vertices array is empty, C++ will attempt bounding box calculation on empty data.

- [ ] 800. [M3RendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` line 214
- **Status**: Pending
- **Details**: Same issue as M2RendererGL/M2LegacyRendererGL — C++ uses `std::chrono::steady_clock` elapsed time (lines 242–246) instead of `performance.now() * 0.001`.

- [ ] 801. [MDXRendererGL.cpp] Load and texture/animation paths are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 174, 200, 407
- **Status**: Pending
- **Details**: JS uses async `load`, `_load_textures`, and `playAnimation`; C++ ports these paths synchronously, changing asynchronous control flow and failure timing.

- [ ] 802. [MDXRendererGL.cpp] Skeleton node flattening changes JS undefined/NaN behavior for `objectId`
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 256–264
- **Status**: Pending
- **Details**: JS compares raw `nodes[i].objectId` and can propagate undefined/NaN semantics. C++ uses `std::optional<int>` checks and skips undefined IDs, which changes edge-case matrix-index behavior from JS.

- [ ] 803. [MDXRendererGL.cpp] Reactive watchers not set up — `geosetWatcher` and `wireframeWatcher` completely missing
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 187–188
- **Status**: Pending
- **Details**: JS `load()` sets up Vue watchers: `this.geosetWatcher = core.view.$watch(this.geosetKey, () => this.updateGeosets(), { deep: true })` and `this.wireframeWatcher = core.view.$watch('config.modelViewerWireframe', () => {}, { deep: true })`. C++ completely omits these watchers. Comment at lines 228–229 states "polling is handled in render()." but no polling code exists.

- [ ] 804. [MDXRendererGL.cpp] `dispose()` missing watcher cleanup calls
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 780–781
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.geosetWatcher?.()` and `this.wireframeWatcher?.()`. C++ has no equivalent cleanup because watchers were never created.

- [ ] 805. [MDXRendererGL.cpp] `_create_skeleton()` doesn't initialize `node_matrices` to identity when nodes are empty
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 252
- **Status**: Pending
- **Details**: JS sets `this.node_matrices = new Float32Array(16)` which creates a zero-filled 16-element array (single identity-sized buffer). C++ does `node_matrices.resize(16)` at line 313 which leaves elements uninitialized. Should zero-initialize or set to identity to match JS behavior.

- [ ] 806. [MDXRendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 681
- **Status**: Pending
- **Details**: Same issue as other renderers — C++ uses elapsed time from first render call instead of `performance.now() * 0.001`.

- [ ] 807. [MDXRendererGL.cpp] Interpolation constants `INTERP_NONE/LINEAR/HERMITE/BEZIER` defined but never used in either JS or C++
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 27–30
- **Status**: Pending
- **Details**: Both files define `INTERP_NONE=0`, `INTERP_LINEAR=1`, `INTERP_HERMITE=2`, `INTERP_BEZIER=3` but neither uses them. The `_sample_vec3()` and `_sample_quat()` methods only implement linear interpolation (lerp/slerp), never checking interpolation type. Hermite and Bezier interpolation are not implemented in either codebase.

- [ ] 808. [MDXRendererGL.cpp] `_build_geometry()` VAO setup passes 5 params instead of 6 — JS passes `null` as 6th parameter
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 368
- **Status**: Pending
- **Details**: JS calls `vao.setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo, null)` with 6 parameters (last is null for index buffer). C++ calls `vao->setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo)` with only 5 parameters. The 6th parameter (index/element buffer) is missing in C++.

- [ ] 809. [WMORendererGL.cpp] Load/update-set paths are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 81, 119, 206, 353, 434
- **Status**: Pending
- **Details**: JS defines async `load`, `_load_textures`, `_load_groups`, `loadDoodadSet`, and `updateSets`; C++ ports these methods synchronously, changing await/timing behavior.

- [ ] 810. [WMORendererGL.cpp] Reactive view binding/watcher lifecycle differs from JS
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 101–107, 637–639
- **Status**: Pending
- **Details**: JS stores `groupArray`/`setArray` by reference in `core.view` and updates via Vue `$watch` callbacks with explicit unregister in `dispose`. C++ copies arrays into view state and replaces watcher callbacks with polling logic, changing reactivity/update timing semantics.

- [ ] 811. [WMORendererGL.cpp] Reactive watchers replaced with polling — `groupWatcher`, `setWatcher`, `wireframeWatcher` not created
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 105–107
- **Status**: Pending
- **Details**: Same approach as WMOLegacyRendererGL — JS uses Vue watchers, C++ uses per-frame polling in `render()` (lines 643–676). Architecturally different but functionally equivalent with potential one-frame delay.

- [ ] 812. [WMORendererGL.cpp] `_load_textures()` `isClassic` check differs — JS tests `!!wmo.textureNames` (truthiness), C++ tests `!wmo->textureNames.empty()`
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` line 126
- **Status**: Pending
- **Details**: JS `!!wmo.textureNames` is true if the property exists and is truthy (even an empty array `[]` is truthy). C++ `!wmo->textureNames.empty()` is only true if the map has entries. If a WMO has the texture names chunk but it's empty, JS enters classic mode but C++ does not. Comment at C++ line 140–143 acknowledges this.

- [ ] 813. [WMORendererGL.cpp] `get_wmo_groups_view()`/`get_wmo_sets_view()` accessor methods don't exist in JS — C++ addition for multi-viewer support
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 64–65, 103–104
- **Status**: Pending
- **Details**: JS uses `view[this.wmoGroupKey]` and `view[this.wmoSetKey]` for dynamic property access. C++ implements `get_wmo_groups_view()` and `get_wmo_sets_view()` methods (lines 60–69) that return references to the appropriate core::view member based on the key string, supporting `modelViewerWMOGroups`, `creatureViewerWMOGroups`, and `decorViewerWMOGroups`. This is a valid C++ adaptation of JS's dynamic property access.

- [ ] 814. [WMOLegacyRendererGL.cpp] Load/update-set paths are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 77, 104, 168, 270, 353
- **Status**: Pending
- **Details**: JS exposes async `load`, `_load_textures`, `_load_groups`, `loadDoodadSet`, and `updateSets`; C++ ports these paths as synchronous methods, altering Promise scheduling and error propagation behavior.

- [ ] 815. [WMOLegacyRendererGL.cpp] Doodad-set iteration adds bounds guard not present in JS
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 287–289
- **Status**: Pending
- **Details**: JS directly accesses `wmo.doodads[firstIndex + i]` without a pre-check. C++ introduces explicit range guarding/continue behavior, changing edge-case handling when doodad counts/indices are inconsistent.

- [ ] 816. [WMOLegacyRendererGL.cpp] Vue watcher-based reactive updates are replaced with render-time polling
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 88–93, 519–521
- **Status**: Pending
- **Details**: JS wires `$watch` callbacks and unregisters them in `dispose`. C++ removes watcher registration and uses per-frame state polling, which changes update trigger timing and reactivity semantics.

- [ ] 817. [WMOLegacyRendererGL.cpp] Reactive watchers replaced with polling — `groupWatcher`, `setWatcher`, `wireframeWatcher` not created
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 91–93
- **Status**: Pending
- **Details**: JS sets up three Vue watchers in `load()`. C++ replaces these with manual per-frame polling in `render()` (lines 517–551), comparing current state against `prev_group_checked`/`prev_set_checked` arrays. This is functionally equivalent but architecturally different — watchers are event-driven, polling is frame-driven with potential one-frame delay.

- [ ] 818. [WMOLegacyRendererGL.cpp] Texture wrap flag logic potentially inverted
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 146–147
- **Status**: Pending
- **Details**: JS sets `wrap_s = (material.flags & 0x40) ? gl.CLAMP_TO_EDGE : gl.REPEAT` and `wrap_t = (material.flags & 0x80) ? gl.CLAMP_TO_EDGE : gl.REPEAT`. C++ creates `BLPTextureFlags` with `wrap_s = !(material.flags & 0x40)` at line 184–185. The boolean negation may invert the wrap behavior — if `true` maps to CLAMP in the BLPTextureFlags API, then `!(flags & 0x40)` produces the opposite of what JS does. Need to verify the BLPTextureFlags API to confirm.

- [ ] 819. [data-exporter.cpp] SQL null handling differs for empty-string values
- **JS Source**: `src/js/ui/data-exporter.js` lines 181–187
- **Status**: Pending
- **Details**: JS forwards actual empty strings as values and only maps `null`/`undefined` to SQL null; C++ passes only `std::string` data and routes empty strings through SQLWriter’s null sentinel path, changing empty-string semantics.

- [ ] 820. [data-exporter.cpp] Export failure records omit stack traces from helper marks
- **JS Source**: `src/js/ui/data-exporter.js` lines 68–71, 124–127, 196–199, 246–249
- **Status**: Pending
- **Details**: JS passes both `e.message` and `e.stack` to `helper.mark(...)`; C++ passes `e.what()` and `std::nullopt`, dropping stack details from failure metadata.

- [ ] 821. [data-exporter.cpp] exportDataTable defaults exportDBFormat to "CSV" while JS has no default
- **JS Source**: `src/js/ui/data-exporter.js` line 30
- **Status**: Pending
- **Details**: JS reads `core.view.config.exportDBFormat` directly; if the key is unset, it's `undefined` which won't match 'CSV' or 'SQL', so the function returns without exporting. C++ uses `.value("exportDBFormat", "CSV")` which defaults to "CSV" — always exports as CSV if the config key is absent. Different behavior when config is incomplete.

- [ ] 822. [export-helper.cpp] `getIncrementalFilename` is synchronous instead of JS async Promise API
- **JS Source**: `src/js/casc/export-helper.js` lines 97–114
- **Status**: Pending
- **Details**: JS exposes `static async getIncrementalFilename(...)` and awaits `generics.fileExists`; C++ implementation is synchronous, changing timing/error behavior expected by Promise-style callers.

- [ ] 823. [export-helper.cpp] Export failure stack-trace output target differs from JS
- **JS Source**: `src/js/casc/export-helper.js` lines 284–288
- **Status**: Pending
- **Details**: JS writes stack traces with `console.log(stackTrace)` in `mark(...)`; C++ routes stack trace strings through `logging::write(...)`, changing where detailed error output appears.

- [ ] 824. [texture-exporter.cpp] overwriteFiles config default is true in C++ vs undefined (falsy) in JS
- **JS Source**: `src/js/ui/texture-exporter.js` lines 100–105
- **Status**: Pending
- **Details**: JS reads `core.view.config.overwriteFiles` which is undefined if not set (falsy → no overwrite). C++ uses `.value("overwriteFiles", true)` defaulting to `true`. If the config key is absent, C++ overwrites existing files by default while JS would not. Behavioral difference in default export behavior.

- [ ] 825. [texture-exporter.cpp] Added .jpeg extension handling not present in JS
- **JS Source**: `src/js/ui/texture-exporter.js` lines 145–150
- **Status**: Pending
- **Details**: JS only checks `file_ext === '.jpg'` for JPG detection. C++ additionally checks `file_ext == ".jpeg"`. Minor deviation: C++ handles an extra extension that JS does not recognize, so a file named "foo.jpeg" would be processed as JPG in C++ but treated as unknown in JS.

- [ ] 826. [texture-exporter.cpp] markFileName declared outside try-catch, fixing JS let-scoping bug
- **JS Source**: `src/js/ui/texture-exporter.js` lines 124–180
- **Status**: Pending
- **Details**: JS declares `let markFileName` inside the try block, then references it in the catch block. Because `let` is block-scoped, `markFileName` is NOT accessible in the catch, which would cause a ReferenceError if an error occurs. C++ declares `markFileName` before the try block, avoiding the issue. This is a deviation from JS that actually fixes a latent JS scoping bug.

- [ ] 827. [M2Exporter.cpp] `addURITexture` input contract differs from JS (data URI string vs decoded PNG buffer)
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59–61, 111–112
- **Status**: Pending
- **Details**: JS stores a data-URI string and decodes it inside `exportTextures()`. C++ `addURITexture` accepts `BufferWrapper` PNG bytes directly, changing caller-facing behavior and where decoding occurs.

- [ ] 828. [M2Exporter.cpp] Equipment UV2 export guard differs from JS truthy check
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 568
- **Status**: Pending
- **Details**: JS exports UV2 when `config.modelsExportUV2 && uv2` (empty arrays are truthy). C++ requires `!uv2.empty()`, so empty-but-present UV2 buffers are not exported.

- [ ] 829. [M2Exporter.cpp] Data textures silently dropped from GLTF/GLB texture maps and buffers
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 357–366
- **Status**: Pending
- **Details**: In JS, `textureMap` is a `Map` with mixed key types — numeric fileDataIDs and string keys like `"data-5"` for data textures (canvas-composited textures). These are passed directly to `gltf.setTextureMap()`. In C++ (M2Exporter.cpp ~lines 610–636), the string-keyed `textureMap` is converted to a `uint32_t`-keyed `gltfTexMap` via `std::stoul()`. Keys like `"data-5"` fail parsing and are silently dropped in the `catch (...)` block. The same happens for `texture_buffers` in GLB mode. This means data textures (canvas-composited textures for character models) are lost in GLTF/GLB exports — meshes will reference material names that have no corresponding texture entry.

- [ ] 830. [M2Exporter.cpp] uint16_t loop variable for triangle iteration risks overflow/infinite loop
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 375, 496, 638, 701, 850, 936
- **Status**: Pending
- **Details**: All triangle iteration loops in M2Exporter.cpp use `for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)`. If `mesh.triangleCount` is 65535, incrementing `vI` from 65534 to 65535 works, but then `vI++` wraps to 0, causing an infinite loop. If `triangleCount` exceeds 65535 (stored as uint32_t in the struct), the loop would also be incorrect since `uint16_t` can never reach the termination condition. JS uses `let vI` which is a double-precision float with no overflow. Should use `uint32_t` for the loop variable.

- [ ] 831. [M2Exporter.cpp] addURITexture accepts BufferWrapper instead of data URI string
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59–61
- **Status**: Pending
- **Details**: JS `addURITexture(out, dataURI)` stores a raw base64 data URI string, which is decoded later in `exportTextures()` via `BufferWrapper.fromBase64(dataTexture.replace(...))`. C++ `addURITexture(uint32_t textureType, BufferWrapper pngData)` accepts already-decoded PNG data, shifting the decoding responsibility to the caller. This is a contract change that alters the interface boundary — callers must now pre-decode the data URI before passing it. While not a bug if all callers are adapted, it changes the API surface compared to the original JS.

- [ ] 832. [M2Exporter.cpp] JSON submesh serialization uses fixed field enumeration instead of JS Object.assign
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 794
- **Status**: Pending
- **Details**: JS uses `Object.assign({ enabled: subMeshEnabled }, skin.subMeshes[i])` which dynamically copies *all* properties from the submesh object. C++ (M2Exporter.cpp ~lines 1111–1126) manually enumerates a fixed set of properties (submeshID, level, vertexStart, vertexCount, triangleStart, triangleCount, boneCount, boneStart, boneInfluences, centerBoneIndex, centerPosition, sortCenterPosition, sortRadius). If the Skin's SubMesh struct gains new fields, they would automatically appear in JS JSON output but would be missing in C++ JSON output. This is a fragile pattern that could silently omit metadata.

- [ ] 833. [M2Exporter.cpp] Data texture file manifest entries get fileDataID=0 instead of string key
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 748
- **Status**: Pending
- **Details**: In JS, `texFileDataID` for data textures is the string key `"data-X"`, which gets stored as-is in the file manifest. In C++ (~line 1059), `std::stoul(texKey)` fails for `"data-X"` keys and `texID` defaults to 0 in the `catch (...)` block. This means data textures in the file manifest will have `fileDataID = 0` instead of a meaningful identifier, losing the ability to correlate manifest entries with specific data texture types.

- [ ] 834. [M2Exporter.cpp] formatUnknownFile call signature differs from JS
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 194
- **Status**: Pending
- **Details**: JS calls `listfile.formatUnknownFile(texFile)` where `texFile` is a string like `"12345.png"`. C++ (~line 410) calls `casc::listfile::formatUnknownFile(texFileDataID, raw ? ".blp" : ".png")` passing the numeric ID and extension separately. The C++ call passes `raw ? ".blp" : ".png"` but this code appears in the `!raw` branch (line 406 checks `!raw`), so the `raw` ternary would always evaluate to `.png`. While not necessarily a bug (depends on `formatUnknownFile` implementation), the call signature divergence means the output filename format may differ.

- [ ] 835. [M2LegacyExporter.cpp] Skin texture override condition differs when `skinTextures` is an empty array
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 65–70, 176–181, 220–225
- **Status**: Pending
- **Details**: JS checks `this.skinTextures` truthiness (empty array is truthy) and may overwrite to `undefined`, then skip texture. C++ requires `!skinTextures.empty()`, so it keeps original texture paths in that edge case.

- [ ] 836. [M2LegacyExporter.cpp] Export API flow is synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 39, 123, 262, 299
- **Status**: Pending
- **Details**: JS export methods (`exportTextures`, `exportAsOBJ`, `exportAsSTL`, `exportRaw`) are async and yield during I/O. C++ runs these paths synchronously, altering timing/cancellation behavior versus JS.

- [ ] 837. [M2LegacyExporter.cpp] uint16_t loop variable for triangle iteration risks overflow
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 164, 289
- **Status**: Pending
- **Details**: Same issue as M2Exporter: triangle iteration loops in M2LegacyExporter.cpp (exportAsOBJ ~line 212, exportAsSTL ~line 401) use `for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)`. If `mesh.triangleCount` reaches or exceeds 65535, `uint16_t` overflow causes an infinite loop or incorrect iteration. JS uses `let vI` with no overflow limit. Should use `uint32_t` for the loop variable.

- [ ] 838. [M3Exporter.cpp] `addURITexture` input contract differs from JS (data URI string vs decoded PNG buffer)
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49–50
- **Status**: Pending
- **Details**: JS stores raw data-URI strings in `dataTextures`; C++ stores `BufferWrapper` PNG bytes, changing caller contract and data normalization stage.

- [ ] 839. [M3Exporter.cpp] UV2 export condition checks non-empty instead of JS defined-ness
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 88–89, 141–142
- **Status**: Pending
- **Details**: JS exports UV1 whenever it is defined (`!== undefined`), including empty arrays. C++ requires `!m3->uv1.empty()`, which changes behavior for defined-but-empty UV sets.

- [ ] 840. [M3Exporter.cpp] addURITexture accepts BufferWrapper instead of data URI string
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49–51
- **Status**: Pending
- **Details**: Same issue as M2Exporter: JS `addURITexture(out, dataURI)` stores a raw base64 data URI string keyed by output path. C++ `addURITexture(const std::string& out, BufferWrapper pngData)` accepts already-decoded PNG data. This is an API contract change that shifts decoding responsibility to the caller.

- [ ] 841. [M3Exporter.cpp] exportTextures returns map<uint32_t, string> instead of JS Map with mixed key types
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 62–65
- **Status**: Pending
- **Details**: While the JS `exportTextures()` currently returns an empty Map (texture export not yet implemented), the C++ return type `std::map<uint32_t, std::string>` constrains future implementation to numeric-only keys. If M3 texture export is later implemented following M2Exporter's pattern (which uses string keys like `"data-X"` for data textures), the uint32_t key type would need to change. The JS Map supports mixed key types natively. This is a forward-compatibility concern rather than a current bug.

- [ ] 842. [WMOExporter.cpp] Export methods are synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 62, 219, 360, 739, 841, 1179
- **Status**: Pending
- **Details**: JS uses async export methods (`exportTextures`, `exportAsGLTF`, `exportAsOBJ`, `exportAsSTL`, `exportGroupsAsSeparateOBJ`, `exportRaw`) with awaited CASC/file operations, while C++ executes these paths synchronously.

- [ ] 843. [WMOExporter.cpp] uint16_t loop variable for face iteration risks overflow/infinite loop (4 locations)
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 385, 551, 862, 1004 (batch.numFaces iteration)
- **Status**: Pending
- **Details**: Four face iteration loops in WMOExporter.cpp (lines 484, 643, 1077, 1202) use `for (uint16_t fi = 0; fi < batch.numFaces; fi++)`. If `batch.numFaces` reaches or exceeds 65535, the `uint16_t` loop variable wraps to 0, causing an infinite loop or incorrect iteration. JS uses `let i` which is a double-precision float with no overflow at these magnitudes. Should use `uint32_t` for the loop variable. Same issue as entry 352 (M2Exporter) and entry 357 (M2LegacyExporter).

- [ ] 844. [WMOExporter.cpp] Constructor takes explicit casc::CASC* parameter not present in JS
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 34–36
- **Status**: Pending
- **Details**: JS constructor is `constructor(data, fileID)` and obtains CASC source internally via `core.view.casc`. C++ constructor is `WMOExporter(BufferWrapper data, uint32_t fileDataID, casc::CASC* casc)` with explicit casc pointer. Additionally, `fileDataID` is constrained to `uint32_t` while JS accepts `string|number` for `fileID`. This is an API deviation — callers must pass the correct CASC instance and cannot pass string file paths.

- [ ] 845. [WMOExporter.cpp] Extra loadWMO() and getDoodadSetNames() accessor methods not in JS
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 34–36
- **Status**: Pending
- **Details**: C++ adds `loadWMO()` (line 1698) and `getDoodadSetNames()` (line 1702) methods that do not exist in the JS WMOExporter class. In JS, `this.wmo` is a public property accessed directly by callers. In C++, `wmo` is a private `std::unique_ptr<WMOLoader>`, so these accessor methods were added to expose the loader. This is a necessary C++ adaptation but changes the public API surface.

- [ ] 846. [WMOLegacyExporter.cpp] Export methods are synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 47, 130, 392, 478
- **Status**: Pending
- **Details**: JS legacy WMO export methods are async and await texture/model I/O; C++ methods (`exportTextures`, `exportAsOBJ`, `exportAsSTL`, `exportRaw`) are synchronous, changing timing/cancellation semantics.

- [ ] 847. [WMOLegacyExporter.cpp] uint16_t loop variable for face iteration risks overflow/infinite loop (2 locations)
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 202, 425 (batch.numFaces iteration)
- **Status**: Pending
- **Details**: Two face iteration loops in WMOLegacyExporter.cpp (lines 288, 603) use `for (uint16_t i = 0; i < batch.numFaces; i++)`. If `batch.numFaces` reaches or exceeds 65535, the `uint16_t` loop variable wraps to 0, causing an infinite loop or incorrect iteration. JS uses `let i` with no overflow risk. Should use `uint32_t` for the loop variable. Same issue as entries 352, 357, 360.

- [ ] 848. [vp9-avi-demuxer.cpp] Parsing/extraction flow is synchronous callback-based instead of JS async APIs
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js` lines 22–23, 83–126
- **Status**: Pending
- **Details**: JS exposes `async parse_header()` and `async* extract_frames()` generator semantics; C++ ports these to synchronous methods with callback iteration, changing consumption and scheduling behavior.

- [ ] 849. [OBJWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 129–225
- **Status**: Pending
- **Details**: JS implements asynchronous writes (`await writer.writeLine(...)` and async filesystem calls). C++ `write()` is synchronous, which changes ordering and error propagation relative to the original Promise API.

- [ ] 850. [OBJWriter.cpp] `appendGeometry` UV handling differs — JS uses `Array.isArray`/spread, C++ uses `insert`
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 84–99
- **Status**: Pending
- **Details**: JS `appendGeometry` handles multiple UV arrays and uses `Array.isArray` + spread operator for concatenation. C++ uses `std::vector::insert` for appending. Functionally equivalent.

- [ ] 851. [OBJWriter.cpp] Face output format uses 1-based indexing with `v[i+1]//vn[i+1]` or `v[i+1]/vt[i+1]/vn[i+1]` — matches JS correctly
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 119–142
- **Status**: Pending
- **Details**: Both JS and C++ output 1-based vertex indices in OBJ face format (e.g., `f v//vn v//vn v//vn` when no UVs, `f v/vt/vn v/vt/vn v/vt/vn` when UVs present). Vertex offset is added correctly in both implementations. Verified as correct.

- [ ] 852. [OBJWriter.cpp] Only first UV set is written in OBJ faces; JS `this.uvs[0]` matches C++ `uvs[0]`
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 119, 130–131
- **Status**: Pending
- **Details**: Both JS and C++ check `this.uvs[0]` / `uvs[0]` for the first UV set when determining whether to include UV indices in face output. Only the first UV set is used in OBJ face references. Verified as matching.

- [ ] 853. [MTLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 41–68
- **Status**: Pending
- **Details**: JS awaits file existence checks, directory creation, and line writes in `async write()`. C++ performs the same work synchronously, so behavior differs for call sites that rely on async completion semantics.

- [ ] 854. [MTLWriter.cpp] `material.name` extraction uses `std::filesystem::path(name).stem().string()` but JS uses `path.basename(name, path.extname(name))`
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 35–37
- **Status**: Pending
- **Details**: C++ line 30 uses `std::filesystem::path(name).stem().string()` to extract the filename without extension. JS uses `path.basename(name, path.extname(name))`. These should produce identical results for typical filenames. However, if `name` contains multiple dots (e.g., `texture.v2.png`), `stem()` returns `texture.v2` while `basename('texture.v2.png', '.png')` also returns `texture.v2`. Functionally equivalent.

- [ ] 855. [MTLWriter.cpp] MTL file uses `map_Kd` texture directive correctly matching JS
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 38–39
- **Status**: Pending
- **Details**: Both JS and C++ write `map_Kd <file>` for diffuse texture mapping in material definitions. Verified as correct.

- [ ] 856. [GLTFWriter.cpp] Export entrypoint is synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 194–1504
- **Status**: Pending
- **Details**: JS defines `async write(overwrite, format)` and awaits filesystem/export operations throughout. C++ exposes `void write(...)` and executes all I/O synchronously, changing call timing/error propagation semantics for callers expecting Promise behavior.

- [ ] 857. [GLTFWriter.cpp] `add_scene_node` returns size_t index in C++ but the JS function returns the node object itself
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 276–280
- **Status**: Pending
- **Details**: JS `add_scene_node` returns the pushed node object reference (used for `skeleton.children.push()` later). C++ returns the index (size_t) instead, and uses index-based access to modify nodes later. This is functionally equivalent but bone parent lookup uses `bone_lookup_map[bone.parentBone]` to store the node index in C++ vs. storing the node object reference in JS. This difference means C++ accesses `nodes[parent_node_idx]` while JS mutates the object directly.

- [ ] 858. [GLTFWriter.cpp] `add_buffered_accessor` lambda omits `target` from bufferView when `buffer_target < 0` in C++, JS passes `undefined` which is serialized differently
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 282–296
- **Status**: Pending
- **Details**: JS `add_buffered_accessor` always includes `target: buffer_target` in the bufferView. When `buffer_target` is `undefined`, JSON.stringify omits the key entirely. C++ explicitly checks `if (buffer_target >= 0)` before adding the target key. This produces identical JSON output since JS `undefined` values are omitted by JSON.stringify, matching C++ not adding the key at all. Functionally equivalent.

- [ ] 859. [GLTFWriter.cpp] Animation channel target node uses `actual_node_idx` (variable per prefix setting) but JS always uses `nodeIndex + 1`
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 620–628, 757–765, 887–895
- **Status**: Pending
- **Details**: In JS, animation channel target node is always `nodeIndex + 1` regardless of prefix setting. In C++, `actual_node_idx` is used, which varies based on `usePrefix`. When `usePrefix` is true, C++ sets `actual_node_idx = nodes.size()` after pushing prefix_node (so it points to the real bone node, matching JS `nodeIndex + 1`). When `usePrefix` is false, `actual_node_idx = nodes.size()` before pushing the node, so it points to the same node. The JS code always does `nodeIndex + 1` which is only correct when prefix nodes exist. C++ correctly handles both cases. This is a JS bug that C++ fixes intentionally.

- [ ] 860. [GLTFWriter.cpp] `bone_lookup_map` stores index-to-index mapping using `std::map<int, size_t>` instead of JS Map storing index-to-object
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` line 464
- **Status**: Pending
- **Details**: JS `bone_lookup_map.set(bi, node)` stores the node object, which is then mutated later when children are added. C++ `bone_lookup_map[bi] = actual_node_idx` stores the index into the `nodes` array, and children are added via `nodes[parent_node_idx]["children"]`. This is functionally equivalent — JS mutates the object reference in the map and C++ indexes into the JSON array.

- [ ] 861. [GLTFWriter.cpp] Mesh primitive always includes `material` property in JS even when `materialMap.get()` returns `undefined`, C++ conditionally omits it
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1110–1119
- **Status**: Pending
- **Details**: JS always sets `material: materialMap.get(mesh.matName)` in the primitive, even if the material isn't found (result is `undefined`, which gets stripped by JSON.stringify). C++ uses `auto mat_it = materialMap.find(mesh.matName)` and only sets `primitive["material"]` if found. The final JSON output is identical since JS undefined is omitted, but the approach differs.

- [ ] 862. [GLTFWriter.cpp] Equipment mesh primitive always includes `material` in JS; C++ conditionally includes it
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1404–1411
- **Status**: Pending
- **Details**: Same pattern as entry 422 but for equipment meshes. JS sets `material: materialMap.get(mesh.matName)` which may be `undefined`. C++ checks `eq_mat_it != materialMap.end()` before setting material. Functionally equivalent in JSON output.

- [ ] 863. [GLTFWriter.cpp] `addTextureBuffer` method does not exist in JS — C++ addition
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` (no equivalent)
- **Status**: Pending
- **Details**: C++ adds `addTextureBuffer(uint32_t fileDataID, BufferWrapper buffer)` method (lines 113–115) which has no JS counterpart. JS only has `setTextureBuffers()` to set the entire map at once. The C++ addition allows incrementally adding individual texture buffers, which changes the API surface.

- [ ] 864. [GLTFWriter.cpp] Animation buffer name extraction in glb mode uses `rfind('_')` to extract `anim_idx`, but JS uses `split('_')` to get index at position 3
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1468–1470
- **Status**: Pending
- **Details**: JS extracts animation index from bufferView name via `name_parts = bufferView.name.split('_'); anim_idx = name_parts[3]`. C++ uses `bv_name.rfind('_')` and then `std::stoi(bv_name.substr(last_underscore + 1))` to get the animation index. For names like `TRANS_TIMESTAMPS_0_1`, JS gets `name_parts[3] = "1"`, C++ gets substring after last underscore = `"1"`. These produce the same result. However, for `SCALE_TIMESTAMPS_0_1`, both work the same. Functionally equivalent.

- [ ] 865. [GLTFWriter.cpp] `skeleton` variable in JS is a node object reference, C++ is a node index
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 344–347, 449
- **Status**: Pending
- **Details**: JS `const skeleton = add_scene_node({name: ..., children: []})` returns the actual node object. Later, `skeleton.children.push(nodeIndex)` mutates it directly. C++ `size_t skeleton_idx = add_scene_node(...)` gets an index, and later accesses `nodes[skeleton_idx]["children"].push_back(...)`. Functionally equivalent.

- [ ] 866. [GLTFWriter.cpp] `usePrefix` is read inside the bone loop instead of outside like JS
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 460, 466
- **Status**: Pending
- **Details**: JS checks `core.view.config.modelsExportWithBonePrefix` outside the bone loop at line 460 (const is evaluated once). C++ reads `core::view->config.value("modelsExportWithBonePrefix", false)` inside the loop at line 470, which re-reads the config for every bone. Since config shouldn't change during export, this is functionally equivalent but slightly less efficient.

- [ ] 867. [GLBWriter.cpp] GLB JSON chunk padding fills with NUL (0x00) instead of spaces (0x20) as required by the glTF 2.0 spec
- **JS Source**: `src/js/3D/writers/GLBWriter.js` lines 20–28
- **Status**: Pending
- **Details**: The glTF 2.0 spec requires that the JSON chunk be padded with trailing space characters (0x20) to maintain 4-byte alignment. C++ `BufferWrapper::alloc(size, true)` zero-fills the buffer, so JSON padding bytes are 0x00. JS `Buffer.alloc(size)` also zero-fills, so JS has the same issue. However, this should be documented as a potential spec compliance issue for both versions.

- [ ] 868. [GLBWriter.cpp] Binary chunk padding uses zero bytes, matching JS behavior correctly
- **JS Source**: `src/js/3D/writers/GLBWriter.js` lines 29–36
- **Status**: Pending
- **Details**: Both JS and C++ use zero bytes (0x00) for BIN chunk padding. The glTF 2.0 spec requires BIN chunks to be padded with NUL (0x00), so this is correct. No issue here, verified as correct.

- [ ] 869. [JSONWriter.cpp] `write()` is synchronous and BigInt-stringify behavior differs from JS
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 33–43
- **Status**: Pending
- **Details**: JS uses `async write()` and a `JSON.stringify` replacer that converts `bigint` values to strings. C++ `write()` is synchronous and writes `nlohmann::json::dump()` directly, which changes both async semantics and JS BigInt serialization parity.

- [ ] 870. [JSONWriter.cpp] `write()` uses `dump(1, '\t')` for pretty-printing; JS uses `JSON.stringify(data, null, '\t')`
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 37–42
- **Status**: Pending
- **Details**: Both produce tab-indented JSON, but nlohmann `dump(1, '\t')` uses indent width of 1 with tab character, while JS `JSON.stringify` with `'\t'` uses tab for each indent level. The output should be identical for well-formed JSON.

- [ ] 871. [JSONWriter.cpp] `write()` default parameter correctly matches JS `overwrite = true`
- **JS Source**: `src/js/3D/writers/JSONWriter.js` line 30
- **Status**: Pending
- **Details**: Both JS and C++ default `overwrite` to `true`. Verified as correct.

- [ ] 872. [CSVWriter.cpp] `.cpp`/`.js` sibling contents are swapped, leaving `.cpp` as unconverted JavaScript
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 1–86
- **Status**: Pending
- **Details**: `CSVWriter.cpp` currently contains JavaScript (`require`, `class`, `module.exports`) while `CSVWriter.js` contains C++ (`#include`, `CSVWriter::...`). This violates expected source pairing and leaves the `.cpp` translation unit unconverted.

- [ ] 873. [CSVWriter.cpp] `addField()` overloaded — JS uses variadic `...fields`, C++ has two overloads (single string and vector)
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 25–27
- **Status**: Pending
- **Details**: JS `addField(...fields)` accepts any number of arguments via rest parameters and pushes all. C++ provides `addField(const std::string&)` for single fields and `addField(const std::vector<std::string>&)` for multiple. Callers must adapt to one of these two signatures instead of passing multiple individual arguments.

- [ ] 874. [CSVWriter.cpp] `escapeCSVField()` handles `null`/`undefined` differently — JS converts via `.toString()`, C++ returns empty for empty string
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 42–51
- **Status**: Pending
- **Details**: JS `escapeCSVField()` handles `null`/`undefined` by returning empty string (line 43–44), then calls `value.toString()` for other types. C++ only accepts `const std::string&` and returns empty for empty strings (line 28–29). JS could receive numbers/booleans and stringify them; C++ requires pre-conversion to string by the caller.

- [ ] 875. [CSVWriter.cpp] `write()` default parameter differs — JS defaults `overwrite = true`, C++ has no default
- **JS Source**: `src/js/3D/writers/CSVWriter.js` line 57
- **Status**: Pending
- **Details**: JS `async write(overwrite = true)` defaults to overwriting. C++ `void write(bool overwrite)` has no default value. Callers must always explicitly pass the overwrite flag in C++.

- [ ] 876. [SQLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 210–229
- **Status**: Pending
- **Details**: JS `async write()` awaits file checks, directory creation, and output writes. C++ performs the same operations synchronously, diverging from JS caller-visible async behavior.

- [ ] 877. [SQLWriter.cpp] Empty-string SQL value handling differs from JS null/undefined checks
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 66–76
- **Status**: Pending
- **Details**: JS returns `NULL` only for `null`/`undefined`; an empty string serializes to `''`. C++ maps `value.empty()` to `NULL`, so genuine empty-string field values are emitted as SQL `NULL`, changing exported data.

- [ ] 878. [SQLWriter.cpp] `addField()` overloaded — JS uses variadic `...fields`, C++ has two overloads (single string and vector)
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 48–49
- **Status**: Pending
- **Details**: JS `addField(...fields)` accepts any number of arguments via rest parameters and pushes all. C++ provides `addField(const std::string&)` for single fields and `addField(const std::vector<std::string>&)` for multiple. Same pattern as CSVWriter entry 413.

- [ ] 879. [SQLWriter.cpp] `generateDDL()` output format differs slightly — C++ builds strings directly, JS uses `lines.join('\n')`
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 141–177
- **Status**: Pending
- **Details**: JS builds an array of `lines` and joins with `\n` at the end. The output includes `DROP TABLE IF EXISTS ...\n\nCREATE TABLE ... (\n<columns>\n);\n\n`. C++ builds the result string directly with `+= "\n"`. The C++ version outputs `DROP TABLE IF EXISTS ...;\n\nCREATE TABLE ... (\n<columns joined with ,\n>\n);\n` which should match. However, JS `lines.push('')` creates an empty element that adds an extra `\n` when joined, and the column_defs are joined separately with `,\n`. The overall output may have subtle whitespace differences in the final string.

- [ ] 880. [SQLWriter.cpp] `toSQL()` format differs — JS uses `lines.join('\n')` with `value_rows.join(',\n') + ';'`, C++ concatenates directly
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 183–204
- **Status**: Pending
- **Details**: JS's `toSQL()` builds lines array and joins with `\n`. Each batch creates `INSERT INTO ... VALUES\n` then `(vals),(vals),...(vals);\n\n`. C++ directly concatenates: `INSERT INTO ... VALUES\n(vals),\n(vals);\n\n`. The difference is that JS joins value_rows with `,\n` (so no leading newline on first row), while C++ adds `,\n` as a separator between rows within the loop. The output format may differ — JS produces `(vals),(vals)\n(vals);` while C++ produces `(vals),\n(vals),\n(vals);\n`. Minor formatting difference in output.

- [ ] 881. [STLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/STLWriter.js` lines 131–249
- **Status**: Pending
- **Details**: JS writer path is asynchronous and awaited by callers. C++ `write()` runs synchronously, changing API timing semantics compared to the original implementation.

- [ ] 882. [STLWriter.cpp] Header string says `wow.export.cpp` while JS says `wow.export` — intentional branding difference
- **JS Source**: `src/js/3D/writers/STLWriter.js` line 147
- **Status**: Pending
- **Details**: JS: `'Exported using wow.export v' + constants.VERSION`. C++: `"Exported using wow.export.cpp v" + std::string(constants::VERSION)`. This is an intentional branding change per project conventions (user-facing text should say wow.export.cpp). Verified as correct.

- [ ] 883. [STLWriter.cpp] `appendGeometry` simplified — C++ doesn't handle `Float32Array` vs `Array` distinction
- **JS Source**: `src/js/3D/writers/STLWriter.js` lines 66–86
- **Status**: Pending
- **Details**: JS `appendGeometry` checks `Array.isArray(this.verts)` to decide between spread and `Float32Array.from()` for concatenation. C++ always uses `std::vector::insert`, which works correctly regardless. The JS type distinction is a JS-specific concern that doesn't apply to C++. Functionally equivalent.

- [x] 884. [tab_models.cpp] Quick filters not passed to listbox — JS passes `modelQuickFilters` (M2/M3/WMO) but C++ passes empty `{}`
- **JS Source**: `src/js/modules/tab_models.js` line 286 (`:quickfilters="$core.view.modelQuickFilters"`)
- **Status**: Pending
- **Details**: The reference screenshot shows "Quick filter: M2 / M3 / WMO" links at the bottom-right of the list status bar. The C++ code at line 809 passes `{}` (empty) for quickfilters. `core.h` line 303 defines `modelQuickFilters = {"m2", "m3", "wmo"}` but it is not wired to the listbox render call.

- [x] 885. [tab_models.cpp] Filter input missing placeholder text "Filter models..."
- **JS Source**: `src/js/modules/tab_models.js` line 297 (`placeholder="Filter models..."`)
- **Status**: Pending
- **Details**: The JS template has `<input ... placeholder="Filter models..."/>`. The C++ code at line 876 uses `ImGui::InputText("##FilterModels", ...)` without a hint/placeholder. Should use `ImGui::InputTextWithHint("##FilterModels", "Filter models...", ...)` to match the original.

- [x] 886. [tab_models.cpp] Regex Enabled indicator missing from filter bar
- **JS Source**: `src/js/modules/tab_models.js` lines 296–297 (`<div class="regex-info" v-if="config.regexFilters">Regex Enabled</div>`)
- **Status**: Pending
- **Details**: When `regexFilters` is enabled in config, the JS shows a "Regex Enabled" badge in the filter bar (styled via `.filter > .regex-info` in app.css line 2441: positioned absolute right, background `--border`, rounded, 0.8em font). The C++ filter bar at lines 872–879 has no equivalent indicator.

- [x] 887. [tab_models.cpp] Preview container missing checkerboard background pattern
- **JS Source**: `src/js/modules/tab_models.js` line 332 (`<div class="preview-background" id="model-preview">`)
- **Status**: Pending
- **Details**: The CSS `.preview-container .preview-background` (app.css lines 1951–1964) defines a checkerboard pattern using `background-image: linear-gradient(45deg, var(--trans-check-a) 25%, ...)`, `background-size: 30px 30px`, border `1px solid var(--border)`, and `box-shadow: black 0 0 3px 0`. The C++ `BeginPreviewContainer` creates a plain ImGui child window with no border, no shadow, and no checkerboard background. The 3D viewport background is rendered by the model-viewer-gl FBO, but the surrounding container should have the styled border and box-shadow.

- [x] 888. [tab_models.cpp] Background color picker position/style differs from CSS
- **JS Source**: `src/js/modules/tab_models.js` line 333 (`<input type="color" id="background-color-input">`)
- **Status**: Pending
- **Details**: The CSS `#background-color-input` (app.css lines 1975–1990) positions the color picker absolute top-right (top: 10px, right: 10px), 24×24px, with border `2px solid var(--border)`, border-radius 4px, z-index 100, box-shadow. The C++ at line 1028–1035 uses `ImGui::ColorEdit3` with `ImGuiColorEditFlags_NoInputs`, which renders an inline color swatch in the flow rather than overlaid at the top-right corner of the preview.

- [x] 889. [tab_models.cpp] Texture preview "Close Preview" button style differs from CSS toast overlay
- **JS Source**: `src/js/modules/tab_models.js` lines 314–315 (`<div id="model-texture-preview">`, `<div id="model-texture-preview-toast">Close Preview</div>`)
- **Status**: Pending
- **Details**: The CSS `#model-texture-preview-toast` (app.css lines 2049–2062) positions the "Close Preview" button absolute top-right with semi-transparent black background, small border, 12px font, hover transition. The C++ at line 986 uses a standard `ImGui::Button("Close Preview")` which has default ImGui button styling (solid color, no transparency, no position override). The entire `#model-texture-preview` should be an absolute overlay (z-index 1) covering the preview area, but in C++ it's rendered inline.

- [x] 890. [tab_models.cpp] UV layer buttons style differs from CSS `.uv-layer-button`
- **JS Source**: `src/js/modules/tab_models.js` lines 321–330 (`class="uv-layer-button"`)
- **Status**: Pending
- **Details**: The CSS `.uv-layer-button` (app.css lines 2141–2161) uses semi-transparent black background `rgba(0,0,0,0.7)`, border `1px solid var(--border)`, padding `5px 10px`, font-size 12px. Active state uses green border/text (`#00ff00`). The C++ at lines 1008–1024 uses standard ImGui buttons with `ImGui::PushStyleColor(ImGuiCol_Button, ButtonActive)` for active state, which doesn't match the green color scheme or the semi-transparent styling.

- [x] 891. [tab_models.cpp] UV overlay positioned inline instead of absolute over texture preview
- **JS Source**: `src/js/modules/tab_models.js` lines 316–319 (`.image` and `.uv-overlay` divs)
- **Status**: Pending
- **Details**: The CSS `.uv-overlay` (app.css lines 2121–2131) is positioned absolute over the texture image (top/left/right/bottom 0, pointer-events none). The C++ at lines 999–1001 uses `ImGui::SetCursorScreenPos` to overlay the UV texture, which is correct in approach, but the `#uv-layer-buttons` (app.css lines 2133–2139) should be positioned absolute top-left (top: 10px, left: 10px) — the C++ renders them inline below the image.

- [x] 892. [tab_models.cpp] Texture ribbon positioned inline instead of absolute bottom overlay
- **JS Source**: `src/js/modules/tab_models.js` lines 300–313 (`id="texture-ribbon"`)
- **Status**: Pending
- **Details**: The CSS `#texture-ribbon` (app.css lines 1898–1906) positions the ribbon absolute at `bottom: 10px`, centered horizontally (`justify-content: center`), with `z-index: 2` overlaying the 3D preview. The C++ at lines 884–983 renders the texture ribbon inline at the top of the preview container, pushing the model viewer down, instead of floating it over the 3D viewport.

- [x] 893. [tab_models.cpp] Texture ribbon slot styling differs — no border/shadow/background-color
- **JS Source**: `src/js/modules/tab_models.js` line 302 (`class="slot"`)
- **Status**: Pending
- **Details**: The CSS `#texture-ribbon .slot` (app.css lines 1926–1940) has width/height 64px, margin `0 5px`, border `1px solid var(--border)`, box-shadow `black 0 0 3px 0`, background-size contain, background-color `#232323`, and hover border-color white. The C++ uses `ImGui::ImageButton` at lines 914–918 with default ImGui styling, missing the custom border color, dark background, and shadow.

- [x] 894. [tab_models.cpp] Texture ribbon prev/next buttons style differs — should use `‹`/`›` glyphs
- **JS Source**: `src/js/modules/tab_models.js` lines 301/303 (`#texture-ribbon-prev`, `#texture-ribbon-next`)
- **Status**: Pending
- **Details**: The CSS `#texture-ribbon-next::before`/`#texture-ribbon-prev::before` (app.css lines 1913–1924) use `content: "›"` and `content: "‹"` glyphs at font-size 4em, positioned absolutely within a 30px-wide element. The C++ at lines 895/932 uses `ImGui::SmallButton("<##ribbon_prev")` and `ImGui::SmallButton(">##ribbon_next")`, which are small default-styled buttons with ASCII characters.

- [x] 895. [tab_models.cpp] Animation controls use text buttons instead of icon buttons with SVG backgrounds
- **JS Source**: `src/js/modules/tab_models.js` lines 342–344 (`class="anim-btn anim-play"`, etc.)
- **Status**: Pending
- **Details**: The CSS `.anim-btn` (app.css lines 3405–3441) defines 24×24px buttons with SVG background images (play.svg, pause.svg, arrow-left.svg, arrow-right.svg), border `1px solid var(--border)`, background-color `var(--background)`. The C++ at lines 1074–1086 uses `ImGui::Button("<<")`, `ImGui::Button("Play"/"Pause")`, `ImGui::Button(">>")` — text labels instead of icon images. The disabled state (`.anim-btn.disabled` opacity 0.5) is handled correctly via `ImGui::BeginDisabled()`/`EndDisabled()`.

- [x] 896. [tab_models.cpp] Animation scrubber uses ImGui::SliderInt instead of styled range input
- **JS Source**: `src/js/modules/tab_models.js` lines 345–348 (`class="anim-scrubber"`, `<input type="range">`)
- **Status**: Pending
- **Details**: The CSS `.anim-scrubber input[type="range"]` (app.css lines 3450–3473) defines a custom-styled range slider with 6px height, dark background, rounded border, custom 14×14 thumb. The frame display (`.anim-frame-display`, app.css lines 3475–3484) has min-width 32px, padding, dark background, border, border-radius, 11px font, centered text. The C++ at lines 1091–1104 uses `ImGui::SliderInt` and `ImGui::Text` which use default ImGui styling.

- [x] 897. [tab_models.cpp] Animation dropdown positioned inline instead of absolute top-left overlay
- **JS Source**: `src/js/modules/tab_models.js` line 335 (`class="preview-dropdown-overlay"`)
- **Status**: Pending
- **Details**: The CSS `.preview-dropdown-overlay` (app.css lines 3375–3390) positions the animation dropdown absolute at `top: 10px`, `left: 10px`, `z-index: 1` — floating over the 3D viewport. The select element uses `background-color: var(--background)`, `color: var(--font-primary)`, `border: 1px solid var(--border)`, `border-radius: 3px`, `padding: 5px 8px`, `font-size: 12px`, `min-width: 150px`. The C++ at lines 1041–1067 renders the combo and controls inline below the model viewer, not overlaid.

- [x] 898. [tab_models.cpp] Sidebar section headers use `ImGui::SeparatorText` instead of styled `<span class="header">`
- **JS Source**: `src/js/modules/tab_models.js` lines 357/386/428/434/438/439/444 (`<span class="header">Preview</span>`, etc.)
- **Status**: Pending
- **Details**: The CSS `.sidebar span.header` (app.css lines 1457–1460) styles section headers as `display: block; margin: 5px 0` — simple bold text labels with vertical spacing, no horizontal rule. The C++ at lines 1127/1169/1226/1245/1280/1301 uses `ImGui::SeparatorText("Preview")`, etc., which renders text centered within a horizontal separator line. This visually differs from the original — there should be no separator line, just a text label.

- [x] 899. [tab_models.cpp] Sidebar checkbox labels font-size not set to 16px
- **JS Source**: `src/js/modules/tab_models.js` lines 358–385 (`<label class="ui-checkbox">`, `<span>`)
- **Status**: Pending
- **Details**: The CSS `.sidebar label span` (app.css line 1461–1463) sets `font-size: 16px` for checkbox label text. The CSS `.ui-checkbox` (app.css lines 941–951) sets `font-size: 18px`, `display: flex`, `margin: 0 15px`, checkbox 18×18px with 5px right margin. The C++ uses default `ImGui::Checkbox` which inherits the global font size and spacing, without matching these specific dimensions.

- [x] 900. [tab_models.cpp] Sidebar "Enable All / Disable All" links styled as ImGui::SmallButton instead of `<a>` links
- **JS Source**: `src/js/modules/tab_models.js` lines 431–432, 441–442
- **Status**: Pending
- **Details**: The CSS `#tab-models #model-sidebar .list-toggles` (app.css lines 1649–1653) sets `font-size: 14px`, `text-align: center`, `margin-top: 5px`. The links are `<a>` tags styled with `color: var(--font-primary)` (white 80%), hover underline + `var(--font-highlight)`. The C++ at lines 1232–1242 uses `ImGui::SmallButton("Enable All")` and `ImGui::SmallButton("Disable All")` which appear as framed buttons, not clickable text links.

- [x] 901. [tab_models.cpp] WMO Groups section uses raw `ImGui::Checkbox` loop instead of `Checkboxlist` component
- **JS Source**: `src/js/modules/tab_models.js` lines 439–440 (`<component :is="$components.Checkboxlist" :items="$core.view.modelViewerWMOGroups">`)
- **Status**: Pending
- **Details**: The JS uses the `Checkboxlist` component for WMO Groups, which provides virtual scrolling, a custom scrollbar, and a fixed 156px height (CSS `#tab-models #model-sidebar .ui-checkboxlist { height: 156px }`). The C++ at lines 1282–1287 uses a raw `for` loop with `ImGui::Checkbox` per group — no virtual scrolling, no custom scrollbar, no fixed height constraint. Should use `checkboxlist::render()` like the M2 geosets section does at line 1228.

- [x] 902. [tab_models.cpp] WMO Doodad Sets section uses raw `ImGui::Checkbox` loop instead of `Checkboxlist` component
- **JS Source**: `src/js/modules/tab_models.js` line 445 (`<component :is="$components.Checkboxlist" :items="$core.view.modelViewerWMOSets">`)
- **Status**: Pending
- **Details**: Same as WMO Groups — the JS uses the `Checkboxlist` component for Doodad Sets but the C++ at lines 1303–1308 uses raw `ImGui::Checkbox` calls without virtual scrolling or fixed height.

- [x] 903. [tab_models.cpp] WMO Groups section missing "Enable All / Disable All" links with correct styling
- **JS Source**: `src/js/modules/tab_models.js` lines 441–442 (`<a @click="setAllWMOGroups(true)">Enable All</a>`)
- **Status**: Pending
- **Details**: The WMO Groups "Enable All / Disable All" links at C++ lines 1289–1299 use `ImGui::SmallButton` (same issue as entry 900), but the JS also wraps them in a `<div class="list-toggles">` with centered 14px text. Additionally, the C++ buttons are functionally correct but visually wrong.

- [x] 904. [tab_models.cpp] Sidebar geoset/skin/WMO checkboxlist height not constrained to 156px
- **JS Source**: CSS `#tab-models #model-sidebar .ui-checkboxlist, #tab-models #model-sidebar .ui-listbox { height: 156px }` (app.css lines 1646–1648)
- **Status**: Pending
- **Details**: The CSS constrains all `.ui-checkboxlist` and `.ui-listbox` elements within `#model-sidebar` to a fixed 156px height. The C++ `checkboxlist::render` and `listboxb::render` calls at lines 1228 and 1266 do not pass any explicit height constraint, relying on ImGui's default sizing. This may cause the sidebar to overflow or have incorrect proportions.

- [x] 905. [tab_models.cpp] Preview controls export button not right-aligned
- **JS Source**: `src/js/modules/tab_models.js` line 354 (`<component :is="$components.MenuButton" ... class="upward">`)
- **Status**: Pending
- **Details**: The CSS `.preview-controls` (app.css lines 1885–1892) uses `display: flex; justify-content: flex-end; align-items: center; margin-right: 20px`. This right-aligns the export button. The C++ `BeginPreviewControls` only vertically centers content but the caller at lines 1113–1122 does not explicitly right-align the `menu_button::render` call. The button should be pushed to the right edge of the controls area.

- [x] 906. [tab_models.cpp] MenuButton export button missing "upward" dropdown direction class
- **JS Source**: `src/js/modules/tab_models.js` line 354 (`class="upward"`)
- **Status**: Pending
- **Details**: The JS template specifies `class="upward"` on the MenuButton, which causes the dropdown to open upward (CSS `.ui-menu-button.upward .menu { bottom: 85% }`, app.css lines 987–994). The C++ `menu_button::render` call at line 1117 does not specify upward direction — verify the menu_button component honors this for proper visual parity.
