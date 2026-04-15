# TODO Tracker

> **Progress: 0/124 verified (0%)** — ✅ = Verified, ⬜ = Pending

### 1. ⬜ [app.cpp] Crash screen heading text differs from original JS
- **JS Source**: `src/index.html` line 70, `src/app.js` line 24
- **Status**: Pending
- **Details**: The JS crash screen heading says `"Oh no! The kākāpō has exploded..."` (from the `<noscript>` block in index.html). The C++ version at line 353 says `"wow.export.cpp has crashed!"`. While user-facing text should say "wow.export.cpp", the original heading text style/phrasing should be preserved as closely as possible (e.g., `"Oh no! wow.export.cpp has exploded..."`). The current text deviates from the original JS heading style.

### 2. ⬜ [app.cpp] Crash screen version/flavour/build layout differs from CSS
- **JS Source**: `src/app.js` lines 44–47, `src/app.css` lines 832–835
- **Status**: Pending
- **Details**: In JS, the version, flavour, and build are displayed as separate `<span>` elements inside `#crash-screen-versions` with CSS `margin: 0 5px; color: var(--border)` (font-faded/border color #6c757d). In C++ (lines 360–364), they are rendered as separate `ImGui::Text` calls with `ImGui::SameLine()` using the default text color (white 80%). They should use `COLOR_FONT_FADED` / `FONT_FADED` color and have 5px horizontal margins between them to match the CSS styling.

### 3. ⬜ [app.cpp] Crash screen error text styling doesn't match CSS
- **JS Source**: `src/app.css` lines 823–831
- **Status**: Pending
- **Details**: CSS specifies `#crash-screen-text { font-weight: normal; font-size: 20px; margin: 20px 0; }` and `#crash-screen-text-code { font-weight: bold; margin-right: 5px; }`. In C++ (lines 368–369), the error code uses `TextColored` with red (1.0, 0.3, 0.3) but the JS has no red color — the error code is just bold text. The error message uses `TextWrapped` without the 20px font size. Both should match the CSS exactly: error code in bold (no red), error message in normal weight at 20px font size, with 20px vertical margins.

### 4. ⬜ [app.cpp] Crash screen log textarea has no top margin
- **JS Source**: `src/app.css` lines 815–817
- **Status**: Pending
- **Details**: CSS specifies `#crash-screen-log { margin-top: 20px; height: 100%; }`. The C++ version (line 396) uses `ImGui::Spacing()` before the log textarea, which is only about 6px. Should use a 20px spacing/dummy to match the 20px top margin in CSS.

### 5. ⬜ [app.cpp] Crash screen form-tray button styling doesn't match CSS
- **JS Source**: `src/app.css` lines 843–848
- **Status**: Pending
- **Details**: CSS specifies `.form-tray { display: flex; margin: 10px 0; }` and `.form-tray input { margin: 0 5px; }`. The C++ version (lines 372–391) uses `ImGui::Spacing()` for top margin (6px instead of 10px) and `ImGui::SameLine()` between buttons (with default 8px spacing, not 5px per side = 10px total gap). Should use proper 10px vertical margin and 5px horizontal spacing between buttons.

### 6. ⬜ [app.cpp] goToTexture() method is missing
- **JS Source**: `src/app.js` lines 418–434
- **Status**: Pending
- **Details**: The JS `goToTexture(fileDataID)` method switches to the textures tab, directly previews a texture by fileDataID, resets the selection, and sets the filter text. This function is referenced by other modules (e.g., from the model viewer to navigate to a texture). There is no equivalent function in app.cpp. It needs to be ported to maintain identical cross-tab navigation behavior.

### 7. ⬜ [app.cpp] Updater module not ported — update check flow is skipped
- **JS Source**: `src/app.js` lines 688–705, `src/js/updater.js`
- **Status**: Pending
- **Details**: The JS version checks for updates on startup in release builds: `core.showLoadingScreen(1, 'Checking for updates...')` followed by `updater.checkForUpdates()`. If an update is available, it applies it; otherwise it hides the loading screen and activates source_select. The C++ version (lines 2741–2753) has this entirely commented out with a TODO reference. The updater module (`src/js/updater.js`) has not been ported to C++.

### 8. ⬜ [app.cpp] Drag-enter/drag-leave overlay prompt not implemented
- **JS Source**: `src/app.js` lines 589–657
- **Status**: Pending
- **Details**: The JS uses `window.ondragenter` with a `dropStack` counter to show a file drop prompt overlay while the user is dragging files over the window (before dropping). The prompt shows the handler's description or "That file cannot be converted." The C++ version (lines 2717–2726) only implements `glfwSetDropCallback` (the drop event), not the drag-enter/drag-leave events. The file drop prompt overlay is only cleared on drop, never shown proactively during drag-over. This is documented as a GLFW limitation but the drop prompt overlay behavior is incomplete.

### 9. ⬜ [app.cpp] Missing ERR_UNHANDLED_REJECTION crash handler
- **JS Source**: `src/app.js` line 72
- **Status**: Pending
- **Details**: JS registers `process.on('unhandledRejection', e => crash('ERR_UNHANDLED_REJECTION', e.message))` for unhandled promise rejections. C++ has no direct equivalent for unhandled futures/promises. The C++ version has `std::set_terminate` for unhandled exceptions and signal handlers for fatal signals, but there is no mechanism to catch unhandled `std::future` exceptions that might be the equivalent of unhandled rejections. This should be documented as a known C++ limitation or handled with a future-exception wrapper pattern.

### 10. ⬜ [app.cpp] Vue error handler interlink not fully ported
- **JS Source**: `src/app.js` line 514
- **Status**: Pending
- **Details**: JS sets `app.config.errorHandler = err => crash('ERR_VUE', err.message)` to catch Vue rendering errors. The C++ equivalent at line 2662 has just a comment `// Interlink error handling for Vue.` but no code. While the C++ render loop at line 1126–1131 wraps module rendering in a try/catch that calls `crash("ERR_RENDER", e.what())`, there's no global catch for errors in other UI rendering paths (like renderAppShell itself outside the module render call). Should ensure all rendering is wrapped in error handling equivalent to Vue's errorHandler.

### 11. ⬜ [app.cpp] Nav tooltip position and styling differs from CSS
- **JS Source**: `src/app.css` lines 507–525
- **Status**: Pending
- **Details**: In CSS, `.nav-label` is positioned at `left: 100%; top: 50%; transform: translateY(-50%); margin-left: 8px;` with `height: 52px; display: flex; align-items: center; background: var(--background-dark);`. The C++ tooltip (lines 580–595) uses `ImGui::BeginTooltip()` positioned at `ImGui::GetItemRectMax().x + 8` which is correct for the X, but the tooltip appears as a standard ImGui tooltip popup with `PopupBg` styling rather than matching the exact `.nav-label` styling which has no border/shadow and exactly 52px height with `padding: 0 10px 0 5px`. The tooltip should have zero border, no shadow, and be exactly 52px tall.

### 12. ⬜ [app.cpp] Nav icon active filter doesn't match CSS filter chain
- **JS Source**: `src/app.css` lines 504–506
- **Status**: Pending
- **Details**: The CSS active nav icon uses a complex SVG filter chain: `filter: brightness(0) saturate(100%) invert(58%) sepia(68%) saturate(481%) hue-rotate(83deg) brightness(93%) contrast(93%)`. This produces a specific green tint applied to the SVG icon. The C++ version (line 544) uses a flat green color `COLOR_NAV_ACTIVE` (which is `BUTTON_BASE` = #22b549). However, for Font Awesome icon glyphs this is fine since they're rendered as text with a color tint. For SVG texture fallback icons (line 570), the tint is applied as a multiplicative color which produces different results than the CSS filter chain. SVG texture icons when active may not look identical to the JS.

### 13. ⬜ [app.cpp] Nav icon hover uses full white instead of CSS brightness(2)
- **JS Source**: `src/app.css` line 501
- **Status**: Pending
- **Details**: The CSS hover effect is `filter: brightness(2)` which doubles the brightness of the existing icon appearance (it doesn't make it pure white — it makes it brighter than the default). The C++ version (line 547) uses `IM_COL32(255, 255, 255, 255)` (full opaque white) on hover, whereas the default is `IM_COL32(255, 255, 255, 204)` (80% alpha). Since the icon images are already white-on-transparent SVGs, `brightness(2)` on an 80% opacity icon would make it 160% white which clamps to full white — so the current behavior is actually correct for white icons. However, for any non-white icon content, this would be incorrect.

### 14. ⬜ [app.cpp] Footer links font weight not set to bold
- **JS Source**: `src/app.css` lines 93–95
- **Status**: Pending
- **Details**: CSS `a { font-weight: bold; }` — the footer links ("Website", "Discord", etc.) use `<a>` tags which are rendered in bold. The C++ version (lines 758–808) does use `app::theme::getBoldFont()` via `ImGui::PushFont(bold)` which is correct. However, the bold font is pushed for the entire links section including the " - " separators. In the JS, the " - " separators are plain text nodes between `<a>` tags and would inherit the `#footer` font weight (normal), not the bold from `<a>`. The separators should be rendered in normal weight, not bold. ⬜

### 15. ⬜ [app.cpp] Footer copyright text vertical positioning
- **JS Source**: `src/app.css` lines 202–209
- **Status**: Pending
- **Details**: The CSS footer uses `display: flex; flex-direction: column; align-items: center; justify-content: center;` to vertically center two lines of content. The C++ version (lines 768–816) manually calculates `links_y` and positions the copyright text at `links_y + line_h + 4.0f`. The 4px gap is a hardcoded approximation. The JS flexbox centering would naturally distribute vertical space equally above and below the two-line block. If the font metrics differ slightly between CSS/Selawik and ImGui/Selawik TTF, the vertical centering could be slightly off.

### 16. ⬜ [app.cpp] Header logo click doesn't check isLoading state
- **JS Source**: `src/app.js` line 142, general Vue template behavior
- **Status**: Pending
- **Details**: In the JS version, clicking the logo/title in the header navigates home. The C++ version (lines 490–496, 506–513) allows clicking the logo and title text to navigate home even when `isLoading` is true, because the click handlers don't check `isLoading`. While the JS nav buttons are hidden during loading (`v-if="!isLoading"`), the logo itself doesn't explicitly block clicks during loading either. However, the nav buttons section at line 518 correctly checks `!core::view->isLoading`. This should be verified for consistency — if JS allows logo clicks during loading, then C++ is correct; if not, it needs a guard.

### 17. ⬜ [app.cpp] Loading bar background color uses a named constant not matching CSS exactly
- **JS Source**: `src/app.css` lines 786–791
- **Status**: Pending
- **Details**: CSS specifies `#loading-bar { background: rgba(0, 0, 0, 0.2196078431); }`. The C++ version uses `app::theme::LOADING_BAR_BG_U32` (line 1241). Need to verify this constant is `IM_COL32(0, 0, 0, 56)` which is `0.2196 * 255 ≈ 56`. The CSS value `0.2196078431` is an unusual precision — it equals exactly `56/255`. If the constant is correct, this is fine; if not, it's a visual discrepancy.

### 18. ⬜ [app.cpp] Loading screen background image uses cover sizing in CSS but may not match in C++
- **JS Source**: `src/app.css` lines 766–775
- **Status**: Pending
- **Details**: CSS specifies `background-size: cover;` for the loading background image, which scales the image to cover the entire viewport while maintaining aspect ratio (potentially cropping edges). The C++ version (lines 1159–1164) uses `AddImage` that stretches the image to fill the viewport without preserving aspect ratio. For the loading.gif / loading-xmas.gif images, this may cause visible stretching/distortion if the window aspect ratio differs from the image aspect ratio. Should implement cover-style scaling (scale to fill, crop excess) to match CSS `background-size: cover`.

### 19. ⬜ [app.cpp] Loading screen only loads first frame of GIF, not animated
- **JS Source**: `src/app.css` lines 766–775
- **Status**: Pending
- **Details**: The JS version uses `loading.gif` and `loading-xmas.gif` as actual animated GIFs displayed via CSS `background-image`. The C++ version (lines 105–111, 198–207) uses `stbi_load` which only loads the first frame of the GIF. This means the loading screen background is static in C++ but animated in JS. This is a visual difference. To match the original, either implement GIF animation (cycling through frames) or document this as a known visual deviation.

### 20. ⬜ [app.cpp] Context menu hamburger icon click behavior differs from JS
- **JS Source**: `src/app.js` line 613 comment
- **Status**: Pending
- **Details**: The JS uses `@click="contextMenus.stateNavExtra = true"` which always opens the context menu (sets to true, never toggles). The C++ version (line 615) uses `ImGui::OpenPopup("##MenuExtraPopup")` which follows ImGui's popup semantics — if the popup is already open and you click the button, it closes because ImGui closes popups on click-outside first. This is functionally close but the exact click-to-reopen behavior may differ. In JS, clicking the hamburger while the menu is already open keeps it open; in C++ it may close and reopen or just close depending on ImGui's event ordering.

### 21. ⬜ [app.cpp] Context menu option icons are not rendered
- **JS Source**: `src/app.js` lines 548–553
- **Status**: Pending
- **Details**: The JS context menu options are registered with icon filenames (e.g., `'timeline.svg'`, `'arrow-rotate-left.svg'`, `'palette.svg'`). In the JS template, these icons appear as `<div class="icon" :style="{'--nav-icon': 'url(./fa-icons/' + opt.icon + ')'}">` next to the label. The C++ version (lines 648–659) renders context menu items with `ImGui::MenuItem(opt.label.c_str())` which shows only text, no icons. The icons should be rendered (using Font Awesome glyphs or SVG textures) to match the JS visual appearance.

### 22. ⬜ [app.cpp] isXmas check references core::view->isXmas but initialization not shown
- **JS Source**: `src/app.js` (Vue data) — `isXmas` is set in `core.makeNewView()`
- **Status**: Pending
- **Details**: The loading screen (line 1157) checks `core::view->isXmas` to select the Christmas background. The `isXmas` field must be properly initialized in `core::makeNewView()` based on the current date (December). Need to verify that `AppState.isXmas` is correctly computed in core.cpp's `makeNewView()` — if it's always false, the Christmas loading screen will never appear, which differs from JS behavior.

### 23. ⬜ [app.cpp] Drop overlay uses ImGui window instead of CSS full-screen overlay
- **JS Source**: `src/app.css` lines 143–164
- **Status**: Pending
- **Details**: The CSS `#drop-overlay` has `background: var(--background-trans); z-index: 9999` (semi-transparent background covering the entire screen). The `#drop-overlay-icon` uses `background-image: url(./fa-icons/copy.svg)` at `width: 100px; height: 100px`. The C++ version (lines 1269–1315) creates an ImGui window with `BG_TRANS` background and renders the copy icon via Font Awesome at 100px. The visual result should be similar but the z-ordering depends on ImGui window creation order — since it's created after the content window, it should render on top, but it lacks the `z-index: 9999` equivalent. If a loading screen or other overlay is also active, the drop overlay might render behind it.

### 24. ⬜ [app.cpp] Header title font-size should be 25px with 3px bottom padding
- **JS Source**: `src/app.css` lines 192–201
- **Status**: Pending
- **Details**: CSS specifies `#container #header #logo { font-size: 25px; font-weight: 700; padding: 0 0 3px 40px; background-size: 32px; }`. The logo area has a 32px background image and 40px left padding. The text has a 3px bottom padding for vertical alignment. The C++ version (line 504) uses `ImGui::PushFont(bold, 25.0f)` for the title which sets the font size correctly, but there's no 3px bottom offset. The logo image is rendered at 32px (correct) with cursor positioning that accounts for vertical centering `(HEADER_HEIGHT - 32) * 0.5f = 10.5px` from top. The text uses `(HEADER_HEIGHT - 25) * 0.5f = 14px` from top. In CSS, the 3px bottom padding effectively shifts the text up by 1.5px. This minor offset may cause slight vertical misalignment.

### 25. ⬜ [app.cpp] Header right-side icon spacing doesn't exactly match CSS
- **JS Source**: `src/app.css` (header layout)
- **Status**: Pending
- **Details**: The header right-side icons (help, hamburger) are positioned using hardcoded pixel offsets from the right edge (lines 608, 669). The hamburger has `right_x -= 15 + 20` (15px right margin + 20px icon width) and help has `right_x -= 10 + 20` (10px gap + 20px icon width). In the JS, these are laid out using CSS flexbox with `margin-left: auto` or absolute positioning from the right. The exact spacing depends on the original CSS rules for these icons. Need to verify the icon positions match the original layout pixel-for-pixel.

### 26. ⬜ [blob.cpp] `slice()` falsy-zero semantics for `end` parameter differ from JS
- **JS Source**: `src/js/blob.js` lines 262–265
- **Status**: Pending
- **Details**: JS uses `end || this._buffer.length`, treating explicit `0` as "use full buffer length." C++ uses `std::optional<std::size_t>` with `value_or(_buffer.size())`, only defaulting when `std::nullopt`. Passing `end = 0` produces different results.

### 27. ⬜ [blob.cpp] `slice()` does not support negative indices
- **JS Source**: `src/js/blob.js` lines 262–265
- **Status**: Pending
- **Details**: JS `Uint8Array.prototype.slice` supports negative indices (counting from end). C++ takes `std::size_t` (unsigned), so negative indices cannot be expressed. Additionally, C++ adds explicit bounds-clamping not present in JS.

### 28. ⬜ [blob.cpp] `stream()` is synchronous/eager instead of async/lazy ReadableStream
- **JS Source**: `src/js/blob.js` lines 271–288
- **Status**: Pending
- **Details**: JS returns a `ReadableStream` with async `pull()` callback; C++ takes a `std::function` callback and iterates all chunks eagerly in a blocking `while` loop. Execution model differs.

### 29. ⬜ [blob.cpp] `arrayBuffer()` and `text()` are synchronous instead of returning Promises
- **JS Source**: `src/js/blob.js` lines 254–260
- **Status**: Pending
- **Details**: JS returns `Promise.resolve(...)`. C++ returns values directly (synchronous). Standard C++ adaptation but changes API contract.

### 30. ⬜ [buffer.cpp] fromPixelData missing JPEG (image/jpeg) MIME type support
- **JS Source**: `src/js/buffer.js` lines 89–107
- **Status**: Pending
- **Details**: JS supports any browser-supported MIME type via canvas `toBlob()`. C++ only handles `image/webp` and `image/png`, throwing for any other MIME type including `image/jpeg`.

### 31. ⬜ [buffer.cpp] calculateHash output encoding limited to hex and base64
- **JS Source**: `src/js/buffer.js` lines 1036–1038
- **Status**: Pending
- **Details**: JS `crypto.createHash().digest(encoding)` supports all Node.js Buffer encodings. C++ only supports `hex` and `base64`, throwing for anything else. Not documented in existing TODOs.

### 32. ⬜ [buffer.cpp] Destructor does not call unmapSource — potential mmap leak
- **JS Source**: `src/js/buffer.js` lines 1063–1068
- **Status**: Pending
- **Details**: C++ destructor is `= default`, which does not release memory-mapped resources. If a BufferWrapper created via `fromMmap()` is destroyed without explicit `unmapSource()`, the mmap memory is leaked.

### 33. ⬜ [buffer.cpp] readFile is synchronous while JS version is async
- **JS Source**: `src/js/buffer.js` lines 113–115
- **Status**: Pending
- **Details**: JS `readFile()` is `async` and returns a Promise. C++ blocks the calling thread with `std::ifstream`. Deviation not documented.

### 34. ⬜ [buffer.cpp] writeToFile is synchronous while JS version is async
- **JS Source**: `src/js/buffer.js` lines 935–938
- **Status**: Pending
- **Details**: JS `writeToFile()` is `async` and uses `await`. C++ uses synchronous `std::filesystem::create_directories()` and `std::ofstream`. Deviation not documented.

### 35. ⬜ [buffer.h] Additional constructor with offset parameter has no JS equivalent
- **JS Source**: `src/js/buffer.js` lines 133–136
- **Status**: Pending
- **Details**: C++ declares `BufferWrapper(std::vector<uint8_t> buf, size_t offset)` which has no JS equivalent. JS constructor always initializes `_ofs = 0`.

### 36. ⬜ [config.cpp] `save()` discards `std::future`, causing blocking behavior
- **JS Source**: `src/js/config.js` lines 83–91
- **Status**: Pending
- **Details**: JS uses `setImmediate(doSave)` (non-blocking). C++ `std::async` returns a `std::future` that is immediately discarded; the temporary future's destructor blocks until `doSave` completes, making the call synchronous.

### 37. ⬜ [config.cpp] `isSaving` and `isQueued` are not thread-safe
- **JS Source**: `src/js/config.js` lines 12–13
- **Status**: Pending
- **Details**: JS is single-threaded so no synchronization needed. C++ `save()` runs on the main thread while `doSave()` runs on a separate thread via `std::async`, creating a data race on `isSaving` and `isQueued` without `std::atomic` or mutex.

### 38. ⬜ [config.cpp] `doSave()` deep equality for arrays differs from JS reference equality
- **JS Source**: `src/js/config.js` lines 99–101
- **Status**: Pending
- **Details**: JS `===` checks reference equality for arrays (always `false` after clone). C++ `nlohmann::json::operator==` performs deep structural comparison. Arrays matching defaults are not persisted in C++ but always persisted in JS.

### 39. ⬜ [config.cpp] `doSave()` silently ignores file write failures
- **JS Source**: `src/js/config.js` line 107
- **Status**: Pending
- **Details**: JS `await fsp.writeFile()` throws on I/O errors. C++ silently does nothing if the file cannot be opened and doesn't check for write errors.

### 40. ⬜ [config.cpp] EPERM detection uses heuristic substring matching instead of error code
- **JS Source**: `src/js/config.js` lines 44–49
- **Status**: Pending
- **Details**: JS checks `e.code === 'EPERM'`. C++ checks for substrings "permission", "EPERM", etc. in exception message — fragile and may false-positive/negative.

### 41. ⬜ [config.cpp] No equivalent of Vue `$watch` for automatic config persistence
- **JS Source**: `src/js/config.js` line 60
- **Status**: Pending
- **Details**: JS uses `core.view.$watch('config', () => save(), { deep: true })` for auto-save. C++ requires explicit `config::save()` calls; missing call sites silently lose data.

### 42. ⬜ [constants.cpp] DATA_DIR points to install-relative directory instead of OS user-data directory
- **JS Source**: `src/js/constants.js` line 16
- **Status**: Pending
- **Details**: JS `nw.App.dataPath` resolves to OS user-data directory. C++ uses `s_install_path / "data"`, placing user data alongside the executable. Changes file storage location.

### 43. ⬜ [constants.cpp] RUNTIME_LOG path differs from JS
- **JS Source**: `src/js/constants.js` line 38
- **Status**: Pending
- **Details**: JS: `path.join(DATA_PATH, 'runtime.log')`. C++: `<install>/Logs/runtime.log`. Both base directory and parent folder name differ.

### 44. ⬜ [constants.h] VERSION is hardcoded instead of read from manifest
- **JS Source**: `src/js/constants.js` line 46
- **Status**: Pending
- **Details**: JS reads version dynamically via `nw.App.manifest.version`. C++ hardcodes `"0.1.0"`. Must be manually updated for each release.

### 45. ⬜ [constants.h] FLAVOUR constant is hardcoded as "win-x64" — not platform-adaptive
- **JS Source**: `src/js/constants.js` (no equivalent in constants.js)
- **Status**: Pending
- **Details**: C++ hardcodes `FLAVOUR = "win-x64"`. Should be `"linux-x64"` on Linux builds. JS reads from `nw.App.manifest.flavour` at runtime.

### 46. ⬜ [constants.cpp] getBlenderBaseDir() returns empty path on missing env var instead of throwing
- **JS Source**: `src/js/constants.js` lines 20–33
- **Status**: Pending
- **Details**: JS would throw a `TypeError` if env var is unavailable. C++ returns empty `fs::path{}` silently.

### 47. ⬜ [core.cpp] Toast `options` field renamed to `actions` with different type
- **JS Source**: `src/js/core.js` lines 470–471
- **Status**: Pending
- **Details**: JS stores `toast.options` (any object or null). C++ renames to `toast->actions` (`std::vector<ToastAction>`). Name and type differ.

### 48. ⬜ [core.cpp] `showLoadingScreen()` defers state updates via postToMainThread instead of synchronous
- **JS Source**: `src/js/core.js` lines 413–420
- **Status**: Pending
- **Details**: JS synchronously sets `loadPct`, `loadingTitle`, `isLoading`, `isBusy`. C++ posts via `postToMainThread()`, introducing a one-frame delay. Callers checking state immediately after would see stale values.

### 49. ⬜ [core.cpp] `progressLoadingScreen()` missing `await generics.redraw()`
- **JS Source**: `src/js/core.js` lines 426–434
- **Status**: Pending
- **Details**: JS forces an immediate UI repaint via `await generics.redraw()`. C++ has no equivalent yield mechanism, so fast sequential calls may batch updates into a single frame.

### 50. ⬜ [core.h] `EventEmitter` default `maxListeners` is 666 for all instances
- **JS Source**: `src/js/core.js` lines 18–19
- **Status**: Pending
- **Details**: JS sets maxListeners=666 only on the global instance. C++ sets 666 as class-wide default for all EventEmitter instances. Also, maxListeners is never enforced (dead code).

### 51. ⬜ [core.h] `isDev` determined by `NDEBUG` macro instead of `BUILD_RELEASE`
- **JS Source**: `src/js/core.js` line 34
- **Status**: Pending
- **Details**: JS uses `!BUILD_RELEASE`. C++ uses `#ifdef NDEBUG`. `RelWithDebInfo` would set NDEBUG (isDev=false) which may not match JS intent.

### 52. ⬜ [external-links.h] `wowHead_viewItem` hardcodes URL instead of using WOWHEAD_ITEM constant
- **JS Source**: `src/js/external-links.js` lines 42–44
- **Status**: Pending
- **Details**: JS uses `util.format(WOWHEAD_ITEM, itemID)`. C++ duplicates the URL string inline instead of using the already-defined constant.

### 53. ⬜ [external-links.h] Extra `resolve()` and `renderLink()` functions not present in JS
- **JS Source**: `src/js/external-links.js` lines 31–34
- **Status**: Pending
- **Details**: JS `open()` performs static-link resolution inline — no separate `resolve` function. C++ adds `resolve()` and `renderLink()` as separate public functions, changing module API surface and responsibilities.

### 54. ⬜ [file-writer.cpp] Backpressure mechanism not ported — `blocked` is dead state
- **JS Source**: `src/js/file-writer.js` lines 24–38
- **Status**: Pending
- **Details**: JS `writeLine` checks `stream.write()` return, sets `blocked=true`, registers drain listener, and awaits a Promise. C++ `writeLine` is synchronous, never sets `blocked=true`, `_drain()` doesn't call resolver. The `blocked` member is dead state.

### 55. ⬜ [file-writer.cpp] Extra guards for `!stream.is_open()` not present in JS class
- **JS Source**: `src/js/file-writer.js` lines 24–42
- **Status**: Pending
- **Details**: C++ `writeLine` and `close` check `if (!stream.is_open()) return;`. JS methods have no such guards — the original class assumes the stream is valid.

### 56. ⬜ [file-writer.h] Extra `isOpen()` method not present in JS
- **JS Source**: `src/js/file-writer.js` (entire file)
- **Status**: Pending
- **Details**: C++ adds `isOpen()` method. JS `FileWriter` class has no equivalent. API surface addition.

### 57. ⬜ [generics.cpp] `get()` returns raw bytes instead of Response-like object
- **JS Source**: `src/js/generics.js` lines 22–54
- **Status**: Pending
- **Details**: JS returns a `Response` with `.ok`, `.status`, `.statusText`, `.json()`. C++ returns `std::vector<uint8_t>` only, throwing on non-ok. Changes API contract — JS callers can inspect non-ok responses.

### 58. ⬜ [generics.cpp] `get()` missing `Cache-Control: no-cache` header
- **JS Source**: `src/js/generics.js` lines 23–24
- **Status**: Pending
- **Details**: JS `fetch_options` includes `cache: 'no-cache'`. C++ only sets User-Agent, so proxies/CDNs may return stale content.

### 59. ⬜ [generics.cpp] `get()` timeout semantics differ — 30s overall vs 30s connect + 60s read
- **JS Source**: `src/js/generics.js` line 30
- **Status**: Pending
- **Details**: JS `AbortSignal.timeout(30000)` aborts entire fetch after 30s. C++ uses `set_connection_timeout(30)` + `set_read_timeout(60)` for up to 90s total.

### 60. ⬜ [generics.cpp] `getJSON()` bypasses `get()` logging
- **JS Source**: `src/js/generics.js` lines 116–122
- **Status**: Pending
- **Details**: JS `getJSON()` calls `get(url)` which logs requests. C++ calls private `doHttpGet()` directly, skipping log lines.

### 61. ⬜ [generics.cpp] `queue()` launches `limit` concurrent tasks instead of `limit + 1`
- **JS Source**: `src/js/generics.js` lines 63–83
- **Status**: Pending
- **Details**: JS initializes `free = limit; complete = -1;` then `check()` increments both, launching up to `limit + 1` tasks initially. C++ caps at exactly `limit`.

### 62. ⬜ [generics.cpp] `getFileHash` reads entire file into memory before hashing
- **JS Source**: `src/js/generics.js` lines 329–337
- **Status**: Pending
- **Details**: JS uses `fs.createReadStream` piped to `crypto.createHash` (streaming). C++ reads all chunks into a vector then hashes. Defeats streaming purpose for large files.

### 63. ⬜ [gpu-info.cpp] `get_gl_info` returns nullopt when vendor/renderer are null — JS continues
- **JS Source**: `src/js/gpu-info.js` lines 30–34
- **Status**: Pending
- **Details**: JS continues to collect caps and extensions even when debug renderer info is unavailable. C++ returns `std::nullopt`, discarding capability/extension data.

### 64. ⬜ [gpu-info.cpp] No timeout for `exec_cmd` on Windows
- **JS Source**: `src/js/gpu-info.js` line 67
- **Status**: Pending
- **Details**: JS uses `exec(cmd, { timeout: 5000 })`. C++ wraps with `timeout 5` on Linux but uses `_popen` with no timeout on Windows.

### 65. ⬜ [icon-render.cpp] processQueue() body is stubbed — no CASC/BLP loading
- **JS Source**: `src/js/icon-render.js` lines 57–59
- **Status**: Pending
- **Details**: JS calls `core.view.casc.getFile()`, creates `BLPFile`, sets `backgroundImage`. C++ has empty try block. Icons never load real textures.

### 66. ⬜ [icon-render.cpp] processQueue() uses synchronous while-loop instead of async one-at-a-time
- **JS Source**: `src/js/icon-render.js` lines 48–65
- **Status**: Pending
- **Details**: JS pops one entry, does async CASC getFile, then recurses via `.finally()`. C++ processes all items in tight synchronous loop with no yielding.

### 67. ⬜ [log.cpp] `write()` is not variadic — takes single pre-formatted string
- **JS Source**: `src/js/log.js` lines 78–95
- **Status**: Pending
- **Details**: JS `write(...parameters)` accepts variadic args with `util.format`. C++ requires callers to pre-format with `std::format()`.

### 68. ⬜ [log.cpp] `write()` control flow allows a line to be both written and pooled
- **JS Source**: `src/js/log.js` lines 81–89
- **Status**: Pending
- **Details**: JS uses `if/else` (mutually exclusive). C++ uses two separate `if` blocks — if write fails and sets `isClogged=true`, the same line is also pushed to pool.

### 69. ⬜ [log.cpp] `drainPool()` triggered only by `write()` calls, not by stream drain events
- **JS Source**: `src/js/log.js` lines 32–49, 111–112
- **Status**: Pending
- **Details**: JS registers `stream.on('drain', drainPool)`. C++ checks `drainPending` flag in `write()`. Pooled entries stay stuck if no new writes occur.

### 70. ⬜ [log.cpp] `timeLog()`/`timeEnd()` do not hold log mutex — race condition on `markTimer`
- **JS Source**: `src/js/log.js` lines 55–66
- **Status**: Pending
- **Details**: JS is single-threaded. C++ `write()` uses mutex but `timeLog()` and `timeEnd()` read/write `markTimer` without mutex. Data race.

### 71. ⬜ [mmap.cpp] Thread-safety mutex added without required deviation comment
- **JS Source**: `src/js/mmap.js` lines 14, 20–24, 30–47
- **Status**: Pending
- **Details**: C++ adds `std::mutex` and `std::lock_guard` in `create_virtual_file` and `release_virtual_files`. JS is single-threaded. Per fidelity rules, C++-only deviations require a documenting comment.

### 72. ⬜ [modules.cpp] Missing `activated()` lifecycle wrapping in `wrap_module`
- **JS Source**: `src/js/modules.js` lines 244–251
- **Status**: Pending
- **Details**: JS wraps `module_def.activated` to call `initialize()` on first activation. C++ `ModuleDef` has no `activated` field; `set_active()` calls `initialize()` directly, changing execution order.

### 73. ⬜ [modules.cpp] `wrap_module` missing `register_context` with callbacks
- **JS Source**: `src/js/modules.js` lines 210–218
- **Status**: Pending
- **Details**: JS creates a `register_context` with `registerNavButton` and `registerContextMenuOption` methods. C++ calls `mod.registerModule()` directly with no context object.

### 74. ⬜ [modules.cpp] `display_label` never updated from nav button label
- **JS Source**: `src/js/modules.js` lines 208, 213, 236–237
- **Status**: Pending
- **Details**: JS updates `display_label` to the nav button label for error messages. C++ always uses module name (e.g., "tab_maps" instead of "Maps").

### 75. ⬜ [modules.cpp] `set_active()` calls `initialize()` directly — JS does not
- **JS Source**: `src/js/modules.js` lines 293–304
- **Status**: Pending
- **Details**: JS `set_active()` only sets `active_module`. Initialization is triggered by Vue's `activated()` hook. C++ calls `initialize()` synchronously during `set_active()`.

### 76. ⬜ [modules.cpp] `openHelpArticle()` and `consumePendingKbId()` belong in tab_help, not modules
- **JS Source**: `src/js/modules.js` (not present); `src/js/modules/tab_help.js` lines 95, 102–105
- **Status**: Pending
- **Details**: C++ adds these functions to `modules.cpp`. In JS, they are local to `tab_help.js`. Structural deviation — functionality belongs in the tab_help module.

### 77. ⬜ [png-writer.h] `data` member is private — public in JS
- **JS Source**: `src/js/png-writer.js` line 189
- **Status**: Pending
- **Details**: JS `this.data` is public. C++ declares it `private`, accessible only through `getPixelData()`. Any JS code accessing `.data` directly has no C++ equivalent.

### 78. ⬜ [subtitles.cpp] `get_subtitles_vtt` missing CASC file-loading logic
- **JS Source**: `src/js/subtitles.js` lines 172–187
- **Status**: Pending
- **Details**: JS takes `(casc, file_data_id, format)` and internally loads the file. C++ takes `(std::string_view text, SubtitleFormat format)`, pushing CASC loading to the caller. API contract change.

### 79. ⬜ [subtitles.cpp] `parse_int` lambda differs from JS `parseInt` for malformed input
- **JS Source**: `src/js/subtitles.js` lines 13–16
- **Status**: Pending
- **Details**: JS `parseInt("1a2", 10)` returns `1` (stops at first non-digit). C++ skips non-digits and continues, returning `12`.

### 80. ⬜ [subtitles.cpp] `parse_srt_timestamp` constructs `std::regex` on every call
- **JS Source**: `src/js/subtitles.js` lines 56–67
- **Status**: Pending
- **Details**: C++ constructs expensive `std::regex` objects on every invocation. Should be `static const` for performance parity with JS regex caching.

### 81. ⬜ [updater.cpp] `checkForUpdates` and `applyUpdate` are synchronous, not async
- **JS Source**: `src/js/updater.js` lines 22, 50
- **Status**: Pending
- **Details**: Both are `async` in JS. C++ versions are synchronous blocking calls. Would freeze UI if called from the UI thread.

### 82. ⬜ [updater.cpp] `launchUpdater` does not log full error object on failure
- **JS Source**: `src/js/updater.js` lines 164–166
- **Status**: Pending
- **Details**: JS has two log calls: formatted message and full error object. C++ only logs `e.what()` once, missing the second `log.write(e)`.

### 83. ⬜ [xml.cpp] `build_object` uses `std::unordered_map` — does not preserve child insertion order
- **JS Source**: `src/js/xml.js` lines 138–155
- **Status**: Pending
- **Details**: JS plain objects preserve insertion order for string keys. C++ `std::unordered_map` does not, potentially changing key ordering in output JSON.

### 84. ⬜ [wmv.cpp] `wmv_parse` return type uses `std::variant` instead of plain object
- **JS Source**: `src/js/wmv.js` lines 9–23, 69–75, 114–120
- **Status**: Pending
- **Details**: JS `wmv_parse` returns a plain object with `{ race, gender, customizations/legacy_values, equipment, model_path }`. Both v1 and v2 return the same shape (with different fields). C++ returns `std::variant<ParseResultV1, ParseResultV2>`, requiring callers to use `std::visit` or `std::get` to access the result. This changes the API contract — JS callers can check for the presence of `customizations` vs `legacy_values` to distinguish versions, while C++ callers must pattern-match on the variant type.

### 85. ⬜ [wmv.cpp] `equipment` field uses `std::unordered_map` — does not preserve insertion order
- **JS Source**: `src/js/wmv.js` lines 50–67, 95–112
- **Status**: Pending
- **Details**: JS `equipment` is a plain object (`{}`), which preserves integer key insertion order in modern JS engines. C++ uses `std::unordered_map<int, int>`, which does not preserve insertion order. If any consumer iterates the equipment map and depends on slot ordering, results will differ.

### 86. ⬜ [wmv.cpp] `parse_equipment` extracted as shared helper — JS duplicates the code
- **JS Source**: `src/js/wmv.js` lines 50–67, 94–112
- **Status**: Pending
- **Details**: JS duplicates the equipment parsing loop identically in both `wmv_parse_v2` (lines 50–67) and `wmv_parse_v1` (lines 94–112). C++ extracts this into a shared `parse_equipment()` helper function (line 143). While functionally equivalent, this is a structural refactoring not present in the original JS. Per fidelity rules, structural deviations should be documented.

### 87. ⬜ [app.cpp] Crash screen missing `#logo-background` watermark
- **JS Source**: `src/app.js` lines 36–39, `src/app.css` lines 132–141
- **Status**: Pending
- **Details**: The JS crash function (lines 36–39) explicitly appends a `<div id="logo-background">` to `document.body` to keep the centered logo watermark (5% opacity) visible in the crash screen background. CSS specifies `#logo-background { background: url(./images/logo.png) no-repeat center center; opacity: 0.05; }`. The C++ `renderCrashScreen()` function (lines 326–404) does not render this watermark — it renders only the crash window content without the logo background. The logo watermark should be drawn behind the crash screen window to match the JS visual appearance.

### 88. ⬜ [app.cpp] `restartApplication()` uses process restart instead of JS in-place reload
- **JS Source**: `src/app.js` lines 63–69, 389–391
- **Status**: Pending
- **Details**: JS uses `chrome.runtime.reload()` (NW.js API) which reloads the application runtime in-place — fast, no new process, preserves environment. C++ `app::restartApplication()` (lines 1775–1799) uses `CreateProcess`/`execl` to spawn a new process and then calls `std::exit(0)`. This is heavier: it starts a new OS process, may fail to release file handles or network connections before the new process starts, and is significantly slower than NW.js in-place reload. This affects both the "Restart Application" button in the crash screen (line 391) and the F5 debug reload in the main loop (line 2805).

### 89. ⬜ [app.cpp] `emit()` and `click()` functions are not variadic — JS versions spread multiple params
- **JS Source**: `src/app.js` lines 437–444, 447–449
- **Status**: Pending
- **Details**: JS `emit(tag, ...params)` is variadic and spreads all extra params to `core.events.emit(tag, ...params)`. JS `click(tag, event, ...params)` is also variadic and passes all extra params to the event emitter. C++ has two overloads: `emit(tag)` and `emit(tag, const std::any& arg)`, and similarly for `click()`. If any caller passes 2 or more extra params, the C++ version silently drops the extras. In addition, `click()` in JS checks `event.target.classList.contains('disabled')` to auto-detect the disabled state from the DOM element class — C++ requires callers to explicitly pass a `disabled` bool, relying on ImGui's `BeginDisabled()` for UI-level disabled handling rather than automatic class-based detection.

### 90. ⬜ [generics.cpp] `batchWork()` does not catch exceptions thrown by the processor
- **JS Source**: `src/js/generics.js` lines 438–465
- **Status**: Pending
- **Details**: JS wraps every `processBatch()` call in `try { ... } catch (error) { cleanup(); reject(error); }`, so exceptions from `processor` reject the Promise and close the `MessageChannel` ports. C++ `batchWork()` has no try/catch around `processor(work[i], i)` — an exception thrown by the processor propagates directly to the caller of `batchWork()`, skipping any cleanup or progress logging. The error-handling contract differs: JS rejects gracefully, C++ propagates as a thrown exception.

### 91. ⬜ [generics.cpp] `downloadFile` missing second `log.write(error)` call on failure
- **JS Source**: `src/js/generics.js` lines 243–244
- **Status**: Pending
- **Details**: JS logs two lines when a download attempt fails: `log.write(\`Failed to download from ${currentUrl}: ${error.message}\`)` then `log.write(error)` (logs the full error object). C++ only calls `logging::write` once with the exception message. The second log call (full error details/stack) is missing.

### 92. ⬜ [generics.cpp] `filesize()` returns string "NaN" for NaN input; JS returns the numeric NaN
- **JS Source**: `src/js/generics.js` line 279–280
- **Status**: Pending
- **Details**: JS `if (isNaN(input)) return input;` returns the original NaN value (a JS number). C++ returns the string `"NaN"`. This is a necessary deviation because the C++ return type is `std::string`, but the deviation is not documented with a comment or TODO entry. Callers that previously checked `typeof result === 'number'` or `isNaN(result)` would see different behaviour.

### 93. ⬜ [buffer.cpp] `setCapacity(capacity, false)` always zeroes new memory; JS `Buffer.allocUnsafe` does not
- **JS Source**: `src/js/buffer.js` lines 1021–1029
- **Status**: Pending
- **Details**: JS `setCapacity` with `secure = false` calls `Buffer.allocUnsafe(capacity)`, which does not zero memory (performance optimisation). C++ uses `std::vector<uint8_t>(capacity, 0)`, which always value-initialises to zero regardless of the `secure` parameter. The `secure` argument is completely ignored for the allocation step. The same deviation is noted for `alloc()` in TODO 65, but `setCapacity` is not covered there.

### 94. ⬜ [buffer.cpp] `writeBuffer(span, copyLength)` silently replicates a JS bug without a TODO entry
- **JS Source**: `src/js/buffer.js` lines 914–918
- **Status**: Pending
- **Details**: JS `writeBuffer` when given a raw Buffer (not a BufferWrapper) and `copyLength > buf.length` executes `new Error(...)` without `throw`, silently discarding the error. The C++ implementation at lines 950–954 intentionally replicates this as a no-op and documents it with an inline comment. However there is no corresponding TODO_TRACKER entry for this intentional JS-bug replication, which makes it invisible to future code reviewers looking for fidelity deviations.

### 95. ⬜ [file-writer.cpp] Constructor fails silently when file cannot be opened; JS `createWriteStream` throws
- **JS Source**: `src/js/file-writer.js` line 15
- **Status**: Pending
- **Details**: JS `fs.createWriteStream(file, ...)` throws synchronously (or emits an `error` event) when the target file cannot be created (bad path, permissions denied, parent directory missing). The C++ `std::ofstream` constructor silently transitions to a bad/closed state without throwing. Callers that do not explicitly check `isOpen()` after construction will silently lose all written data with no indication of failure. This diverges from the fail-fast JS behaviour.

### 96. ⬜ [modules.cpp] `add_module` incorrectly substitutes `mounted_fn` when `initialize_fn` is null
- **JS Source**: `src/js/modules.js` lines 200–252
- **Status**: Pending
- **Details**: In C++ `add_module`, when `initialize_fn` is nullptr, the code sets `mod.initialize = mounted_fn`. The `wrap_module` function then wraps `mounted_fn` with an idempotency guard, causing it to run at most once (on first activation). In Vue.js, `mounted()` is a lifecycle hook that runs every time a component mounts, not just the first time — it should not be guarded for idempotency. JS `wrap_module` only wraps `methods.initialize` if it is defined; modules without it are not affected by the guard. Affected modules include `module_test_a`, `module_test_b`, `source_select`, and others that provide `mounted` but no separate `initialize`.

### 97. ⬜ [modules.cpp] `wrap_module` lambda captures `ModuleDef` by reference — dangling reference risk
- **JS Source**: `src/js/modules.js` lines 222–242
- **Status**: Pending
- **Details**: The lambda assigned to `mod.initialize` captures `mod` by reference (`[&mod, original_initialize, display_label]()`). `mod` is a reference to an element in the `module_registry` `std::vector`. If any code ever calls `module_registry.push_back()` / `emplace_back()` after `wrap_module` has been called (e.g., a dynamically registered module), the vector may reallocate its storage, invalidating all `&mod` references and causing undefined behaviour on the next `initialize()` call. Currently no modules are added after `initialize()`, but there is no `const`-lock, `reserve`, or comment documenting this fragile assumption.

### 98. ⬜ [subtitles.cpp] `sbt_to_srt` and `srt_to_vtt` construct `std::regex` on every call
- **JS Source**: `src/js/subtitles.js` lines 87, 142
- **Status**: Pending
- **Details**: TODO #80 documents the same issue in `parse_srt_timestamp`. The same problem also exists in `sbt_to_srt` (C++ line 176, `timestamp_re`) and `srt_to_vtt` (C++ line 241, `timestamp_re`): both construct a `std::regex` object on every function invocation. In JS, regex literals are compiled once by the engine. All three functions should declare their regex objects as `static const` for performance parity with JS.

### 99. ⬜ [tiled-png-writer.cpp] `write()` is synchronous; JS version is `async`
- **JS Source**: `src/js/tiled-png-writer.js` lines 123–125
- **Status**: Pending
- **Details**: JS `write(file)` is declared `async` and returns `await this.getBuffer().writeToFile(file)`. C++ `write(const std::filesystem::path&)` is entirely synchronous. A code comment in `tiled-png-writer.cpp` documents this deviation but it is not recorded in TODO_TRACKER. Callers that previously awaited the result to detect write errors or chain further work receive no equivalent feedback in C++.

### 100. ⬜ [tiled-png-writer.cpp] `tiles` uses `std::map` (key-sorted) instead of JS `Map` (insertion-ordered)
- **JS Source**: `src/js/tiled-png-writer.js` line 25
- **Status**: Pending
- **Details**: JS constructor does `this.tiles = new Map()`, which preserves insertion order. C++ uses `std::map<std::string, Tile>`, which iterates tiles in lexicographic key order (`"0,0"`, `"0,1"`, ..., `"1,10"`, `"1,2"` — not numeric order). A comment in `tiled-png-writer.h` explains that rendering is position-based so iteration order does not affect pixel output, but per fidelity rules the structural deviation must be recorded here as well as in code comments.

### 101. ⬜ [updater.cpp] `constants::FLAVOUR` hardcoded as `"win-x64"` and `constants::BUILD_GUID` hardcoded as `"cpp-dev"` — not read from runtime manifest
- **JS Source**: `src/js/updater.js` lines 24–25, 33, 113
- **Status**: Pending
- **Details**: JS reads `nw.App.manifest.flavour` and `nw.App.manifest.guid` at runtime from `package.json`, enabling per-distribution and per-build values. C++ uses `constants::FLAVOUR = "win-x64"` (compile-time, always "win-x64" even on Linux) and `constants::BUILD_GUID = "cpp-dev"` (compile-time). Consequences: (a) on Linux, the update URL always requests win-x64 artefacts — wrong platform; (b) `BUILD_GUID = "cpp-dev"` will never equal a real server GUID, causing `checkForUpdates` to always return `true` (update available) in a production environment. No CMake option exists to set these per-platform or per-build.

### 102. ⬜ [wmv.cpp] `normalize_array` extracted as a C++ helper; JS inlines the `Array.isArray` check at every call site
- **JS Source**: `src/js/wmv.js` lines 36–38, 52–53, 97–98
- **Status**: Pending
- **Details**: JS inlines `Array.isArray(x) ? x : [x]` at three separate call sites (customization array, v2 equipment items, v1 equipment items). C++ extracts this into a shared `normalize_array()` helper function (line 135). While functionally equivalent, this is a structural refactoring not present in the original JS and deviates from the line-by-line fidelity requirement. Similar to the existing entry #86 for `parse_equipment`.

### 103. ⬜ [log.cpp] `getErrorDump()` is synchronous in C++; JS version is `async`
- **JS Source**: `src/js/log.js` lines 102–108
- **Status**: Pending
- **Details**: JS `getErrorDump` is an `async` function using `fs.promises.readFile`, returning a Promise. C++ reads the file synchronously with `std::ifstream`. If called from the render/UI thread during crash handling this could cause a momentary block. Additionally, in JS `getErrorDump` is a bare global assignment (`getErrorDump = async () => {...}`, no `const`) so it is attached to the `window` object — accessible from crash handler code without a module import. C++ declares it as a free function in `log.h` outside the `logging` namespace, preserving the module-free accessibility but through a different mechanism. The async deviation is not documented in either `log.cpp` or TODO_TRACKER.

### 104. ⬜ [log.cpp] Linux `openRuntimeLog()` builds shell command via string concatenation — path injection risk
- **JS Source**: `src/js/log.js` lines 72–73
- **Status**: Pending
- **Details**: C++ constructs `"xdg-open '" + constants::RUNTIME_LOG().string() + "' &"` and passes it to `std::system()`. If `RUNTIME_LOG` resolves to a path containing a single-quote character (e.g., a username with an apostrophe), the resulting shell command is malformed or exploitable. JS uses `nw.Shell.openItem()`, which is a safe native API that never invokes a shell. No sanitisation or quoting is applied to the path before embedding it in the command string.

### 105. ⬜ [mmap.cpp] `create_virtual_file` returns a raw `new`-allocated pointer; ownership model is undocumented in the header
- **JS Source**: `src/js/mmap.js` lines 20–24
- **Status**: Pending
- **Details**: C++ `create_virtual_file()` returns an `MmapObject*` allocated with `new` and tracked in the internal `virtual_files` set. Callers must not `delete` the pointer themselves, and must not dereference it after `release_virtual_files()` (which `delete`s all tracked objects). `mmap.h` documents neither the ownership rule nor the lifetime constraint. JS relies on garbage collection and has no equivalent concern. This missing documentation creates a risk of double-free or use-after-free if callers store the returned pointer beyond the cleanup call.

### 106. ⬜ [external-links.cpp] Translation unit is a placeholder and does not contain the JS module logic
- **JS Source**: `src/js/external-links.js` lines 26–44
- **Status**: Pending
- **Details**: `external-links.cpp` only contains comments plus `#include "external-links.h"` and no functions/classes matching the JS module body. The JS file defines the exported class and both `open()` and `wowHead_viewItem()` methods in this file. The C++ implementation being moved entirely into a header means this `.cpp` file itself is effectively an unconverted placeholder instead of a line-by-line port of the source file it is supposed to mirror.

### 107. ⬜ [gpu-info.cpp] Renderer/vendor path bypasses JS `WEBGL_debug_renderer_info` behavior
- **JS Source**: `src/js/gpu-info.js` lines 30–34, 343–347
- **Status**: Pending
- **Details**: JS only populates vendor/renderer when `gl.getExtension('WEBGL_debug_renderer_info')` is available; otherwise it logs `"GPU: WebGL debug info unavailable"`. C++ always queries `glGetString(GL_VENDOR/GL_RENDERER)` directly, which can produce renderer/vendor output even when the JS path would not. This changes logging behavior and diagnostic parity with the original implementation.

### 108. ⬜ [gpu-info.cpp] `log_gpu_info()` is synchronous while JS implementation is async
- **JS Source**: `src/js/gpu-info.js` lines 329–361
- **Status**: Pending
- **Details**: JS exposes `log_gpu_info` as `async` and awaits platform command queries through promise-based `exec_cmd`. C++ defines `void log_gpu_info()` and performs command execution synchronously on the calling thread. This can block startup/UI flow in scenarios where JS would yield back to the event loop while subprocess queries complete.

### 109. ⬜ [core.cpp] Linux path opening uses shell command string instead of JS native shell API
- **JS Source**: `src/js/core.js` lines 484–486
- **Status**: Pending
- **Details**: JS opens the export directory with `nw.Shell.openItem(...)` (native API, no shell string construction). C++ Linux path builds `xdg-open \"...\" &` and passes it to `std::system()`. This changes behavior for special characters/quoting and introduces shell parsing semantics that do not exist in the original JS code path.

### 110. ⬜ [config.cpp] `load()` is synchronous in C++ but asynchronous in JS
- **JS Source**: `src/js/config.js` lines 37–43
- **Status**: Pending
- **Details**: JS defines `load` as `async` and awaits disk reads, allowing callers to await completion. C++ `void load()` performs reads synchronously and returns no future/promise-like handle. This changes call semantics and can block the calling thread while reading/parsing config files.

### 111. ⬜ [constants.cpp] Cache root directory name differs (`casc` in JS vs `cache` in C++)
- **JS Source**: `src/js/constants.js` lines 72–81
- **Status**: Pending
- **Details**: JS defines `CACHE.DIR` and all related cache paths under `<DATA_PATH>/casc`. C++ renames this root to `<data>/cache` and adds migration logic from `casc` to `cache`. This is a path-contract deviation that changes on-disk layout relative to the original JS behavior.

### 112. ⬜ [constants.cpp] `SHADER_PATH` points to `<data>/shaders` instead of `<INSTALL_PATH>/src/shaders`
- **JS Source**: `src/js/constants.js` line 43
- **Status**: Pending
- **Details**: JS resolves shader files from `path.join(INSTALL_PATH, 'src', 'shaders')`. C++ sets `s_shader_path = s_data_dir / "shaders"`. This changes resource lookup location and diverges from the original runtime path contract.

### 113. ⬜ [constants.cpp] `CONFIG.DEFAULT_PATH` points to `<data>/default_config.jsonc` instead of `<INSTALL_PATH>/src/default_config.jsonc`
- **JS Source**: `src/js/constants.js` line 95
- **Status**: Pending
- **Details**: JS default config path is tied to the install tree (`INSTALL_PATH/src/default_config.jsonc`). C++ rewires it to `s_data_dir / "default_config.jsonc"`. This changes where defaults are loaded from and diverges from the original module’s constant export behavior.

### 114. ⬜ [blob.cpp] `URLPolyfill.createObjectURL` fallback path for non-polyfill blobs is missing
- **JS Source**: `src/js/blob.js` lines 294–300
- **Status**: Pending
- **Details**: JS checks `if (blob instanceof BlobPolyfill)` and otherwise calls native `URL.createObjectURL(blob)`. C++ hard-restricts `createObjectURL` to `const BlobPolyfill&` and removes the fallback code path entirely, so non-`BlobPolyfill` blob inputs cannot be handled.

### 115. ⬜ [blob.cpp] `URLPolyfill.revokeObjectURL` does not revoke non-data URLs
- **JS Source**: `src/js/blob.js` lines 302–306
- **Status**: Pending
- **Details**: JS calls native `URL.revokeObjectURL(url)` for non-`data:` URLs. C++ keeps the `data:` prefix check but leaves the non-data branch as a no-op, so equivalent revocation behavior is missing.

### 116. ⬜ [blob.cpp] `BlobPolyfill` constructor no longer supports JS fallback coercion `textEncode(String(chunk))`
- **JS Source**: `src/js/blob.js` lines 228–240
- **Status**: Pending
- **Details**: JS accepts arbitrary chunk values and coerces unsupported types through `String(chunk)` before UTF-8 encoding. C++ only accepts explicitly modeled `BlobPart` inputs (byte buffers/strings/blob), so the generic fallback conversion path is not present.

### 117. ⬜ [xml.cpp] Parser performs unchecked `xml[pos]` reads where JS safely sees `undefined`
- **JS Source**: `src/js/xml.js` lines 25–55, 66–99, 117–123
- **Status**: Pending
- **Details**: JS string indexing past bounds yields `undefined` and remains safe on malformed/truncated XML. C++ directly indexes `xml[pos]` in multiple branches without always guarding `pos < xml.size()`, which can cause out-of-bounds reads and undefined behavior for malformed input.

### 118. ⬜ [xml.cpp] Object key ordering differs because parsed objects are stored in ordered JSON maps
- **JS Source**: `src/js/xml.js` lines 132–165
- **Status**: Pending
- **Details**: JS object keys preserve insertion order as built during parsing. C++ builds parsed objects with `nlohmann::json` default object storage (ordered `std::map`), which reorders keys lexicographically and changes observable key iteration order beyond the already-tracked `unordered_map` child grouping issue.

### 119. ⬜ [install-type.cpp] Translation unit is a placeholder and does not contain JS module logic
- **JS Source**: `src/js/install-type.js` lines 1–6
- **Status**: Pending
- **Details**: JS file defines and exports `InstallType`. C++ `install-type.cpp` contains only include/comment placeholders and no code, so the module implementation is not actually in the matching `.cpp` file.

### 120. ⬜ [crc32.cpp] Return value semantics differ from JS signed 32-bit number behavior
- **JS Source**: `src/js/crc32.js` lines 28–34
- **Status**: Pending
- **Details**: JS bitwise operators produce a signed 32-bit `number` result (negative values possible). C++ returns `uint32_t`, which changes signedness/representation semantics for callers expecting the JS numeric behavior.

### 121. ⬜ [MultiMap.cpp] Translation unit is a placeholder and does not contain JS module logic
- **JS Source**: `src/js/MultiMap.js` lines 6–32
- **Status**: Pending
- **Details**: JS `MultiMap` class implementation lives in the file body. C++ `MultiMap.cpp` contains only include/comment placeholders, with implementation moved to the header, so the matching `.cpp` file remains an unconverted placeholder.

### 122. ⬜ [MultiMap.cpp] Iteration order semantics differ from JS `Map`
- **JS Source**: `src/js/MultiMap.js` lines 6–30
- **Status**: Pending
- **Details**: JS `MultiMap` extends `Map`, which guarantees insertion-order iteration. C++ implementation uses `std::unordered_map`, so iteration order is non-deterministic and can diverge from JS behavior anywhere map traversal order is observable.

### 123. ⬜ [cache-collector.cpp] `parse_url()` is not equivalent to JS WHATWG URL parsing
- **JS Source**: `src/js/workers/cache-collector.js` lines 21–27
- **Status**: Pending
- **Details**: JS uses `new URL(url)` and then `pathname + search`, which correctly handles full URL grammar (userinfo, IPv6 literals, host-only URLs with query strings, etc.). C++ `parse_url()` is a manual parser (`src/js/workers/cache-collector.cpp` lines 365–395) and only splits on `://`, `/`, and `:`, so edge-case URLs can be parsed differently and sent to the wrong endpoint.

### 124. ⬜ [cache-collector.cpp] Host-only URLs with query strings lose query parameters
- **JS Source**: `src/js/workers/cache-collector.js` lines 21–26
- **Status**: Pending
- **Details**: In JS, `new URL("https://example.com?x=1")` produces `pathname="/"` and `search="?x=1"`, so request path becomes `"/?x=1"`. In C++, `parse_url()` sets `path="/"` whenever no `/` exists after host (`src/js/workers/cache-collector.cpp` lines 375–383), dropping `?x=1`. This changes request behavior for valid URLs that rely on query parameters without an explicit path segment.
