# TODO Tracker

> **Progress: 0/25 verified (0%)** — ✅ = Verified, ⬜ = Pending

### 1. [app.cpp] Crash screen heading text differs from original JS
- **JS Source**: `src/index.html` line 70, `src/app.js` line 24
- **Status**: Pending
- **Details**: The JS crash screen heading says `"Oh no! The kākāpō has exploded..."` (from the `<noscript>` block in index.html). The C++ version at line 353 says `"wow.export.cpp has crashed!"`. While user-facing text should say "wow.export.cpp", the original heading text style/phrasing should be preserved as closely as possible (e.g., `"Oh no! wow.export.cpp has exploded..."`). The current text deviates from the original JS heading style.

### 2. [app.cpp] Crash screen version/flavour/build layout differs from CSS
- **JS Source**: `src/app.js` lines 44–47, `src/app.css` lines 832–835
- **Status**: Pending
- **Details**: In JS, the version, flavour, and build are displayed as separate `<span>` elements inside `#crash-screen-versions` with CSS `margin: 0 5px; color: var(--border)` (font-faded/border color #6c757d). In C++ (lines 360–364), they are rendered as separate `ImGui::Text` calls with `ImGui::SameLine()` using the default text color (white 80%). They should use `COLOR_FONT_FADED` / `FONT_FADED` color and have 5px horizontal margins between them to match the CSS styling.

### 3. [app.cpp] Crash screen error text styling doesn't match CSS
- **JS Source**: `src/app.css` lines 823–831
- **Status**: Pending
- **Details**: CSS specifies `#crash-screen-text { font-weight: normal; font-size: 20px; margin: 20px 0; }` and `#crash-screen-text-code { font-weight: bold; margin-right: 5px; }`. In C++ (lines 368–369), the error code uses `TextColored` with red (1.0, 0.3, 0.3) but the JS has no red color — the error code is just bold text. The error message uses `TextWrapped` without the 20px font size. Both should match the CSS exactly: error code in bold (no red), error message in normal weight at 20px font size, with 20px vertical margins.

### 4. [app.cpp] Crash screen log textarea has no top margin
- **JS Source**: `src/app.css` lines 815–817
- **Status**: Pending
- **Details**: CSS specifies `#crash-screen-log { margin-top: 20px; height: 100%; }`. The C++ version (line 396) uses `ImGui::Spacing()` before the log textarea, which is only about 6px. Should use a 20px spacing/dummy to match the 20px top margin in CSS.

### 5. [app.cpp] Crash screen form-tray button styling doesn't match CSS
- **JS Source**: `src/app.css` lines 843–848
- **Status**: Pending
- **Details**: CSS specifies `.form-tray { display: flex; margin: 10px 0; }` and `.form-tray input { margin: 0 5px; }`. The C++ version (lines 372–391) uses `ImGui::Spacing()` for top margin (6px instead of 10px) and `ImGui::SameLine()` between buttons (with default 8px spacing, not 5px per side = 10px total gap). Should use proper 10px vertical margin and 5px horizontal spacing between buttons.

### 6. [app.cpp] goToTexture() method is missing
- **JS Source**: `src/app.js` lines 418–434
- **Status**: Pending
- **Details**: The JS `goToTexture(fileDataID)` method switches to the textures tab, directly previews a texture by fileDataID, resets the selection, and sets the filter text. This function is referenced by other modules (e.g., from the model viewer to navigate to a texture). There is no equivalent function in app.cpp. It needs to be ported to maintain identical cross-tab navigation behavior.

### 7. [app.cpp] Updater module not ported — update check flow is skipped
- **JS Source**: `src/app.js` lines 688–705, `src/js/updater.js`
- **Status**: Pending
- **Details**: The JS version checks for updates on startup in release builds: `core.showLoadingScreen(1, 'Checking for updates...')` followed by `updater.checkForUpdates()`. If an update is available, it applies it; otherwise it hides the loading screen and activates source_select. The C++ version (lines 2741–2753) has this entirely commented out with a TODO reference. The updater module (`src/js/updater.js`) has not been ported to C++.

### 8. [app.cpp] Drag-enter/drag-leave overlay prompt not implemented
- **JS Source**: `src/app.js` lines 589–657
- **Status**: Pending
- **Details**: The JS uses `window.ondragenter` with a `dropStack` counter to show a file drop prompt overlay while the user is dragging files over the window (before dropping). The prompt shows the handler's description or "That file cannot be converted." The C++ version (lines 2717–2726) only implements `glfwSetDropCallback` (the drop event), not the drag-enter/drag-leave events. The file drop prompt overlay is only cleared on drop, never shown proactively during drag-over. This is documented as a GLFW limitation but the drop prompt overlay behavior is incomplete.

### 9. [app.cpp] Missing ERR_UNHANDLED_REJECTION crash handler
- **JS Source**: `src/app.js` line 72
- **Status**: Pending
- **Details**: JS registers `process.on('unhandledRejection', e => crash('ERR_UNHANDLED_REJECTION', e.message))` for unhandled promise rejections. C++ has no direct equivalent for unhandled futures/promises. The C++ version has `std::set_terminate` for unhandled exceptions and signal handlers for fatal signals, but there is no mechanism to catch unhandled `std::future` exceptions that might be the equivalent of unhandled rejections. This should be documented as a known C++ limitation or handled with a future-exception wrapper pattern.

### 10. [app.cpp] Vue error handler interlink not fully ported
- **JS Source**: `src/app.js` line 514
- **Status**: Pending
- **Details**: JS sets `app.config.errorHandler = err => crash('ERR_VUE', err.message)` to catch Vue rendering errors. The C++ equivalent at line 2662 has just a comment `// Interlink error handling for Vue.` but no code. While the C++ render loop at line 1126–1131 wraps module rendering in a try/catch that calls `crash("ERR_RENDER", e.what())`, there's no global catch for errors in other UI rendering paths (like renderAppShell itself outside the module render call). Should ensure all rendering is wrapped in error handling equivalent to Vue's errorHandler.

### 11. [app.cpp] Nav tooltip position and styling differs from CSS
- **JS Source**: `src/app.css` lines 507–525
- **Status**: Pending
- **Details**: In CSS, `.nav-label` is positioned at `left: 100%; top: 50%; transform: translateY(-50%); margin-left: 8px;` with `height: 52px; display: flex; align-items: center; background: var(--background-dark);`. The C++ tooltip (lines 580–595) uses `ImGui::BeginTooltip()` positioned at `ImGui::GetItemRectMax().x + 8` which is correct for the X, but the tooltip appears as a standard ImGui tooltip popup with `PopupBg` styling rather than matching the exact `.nav-label` styling which has no border/shadow and exactly 52px height with `padding: 0 10px 0 5px`. The tooltip should have zero border, no shadow, and be exactly 52px tall.

### 12. [app.cpp] Nav icon active filter doesn't match CSS filter chain
- **JS Source**: `src/app.css` lines 504–506
- **Status**: Pending
- **Details**: The CSS active nav icon uses a complex SVG filter chain: `filter: brightness(0) saturate(100%) invert(58%) sepia(68%) saturate(481%) hue-rotate(83deg) brightness(93%) contrast(93%)`. This produces a specific green tint applied to the SVG icon. The C++ version (line 544) uses a flat green color `COLOR_NAV_ACTIVE` (which is `BUTTON_BASE` = #22b549). However, for Font Awesome icon glyphs this is fine since they're rendered as text with a color tint. For SVG texture fallback icons (line 570), the tint is applied as a multiplicative color which produces different results than the CSS filter chain. SVG texture icons when active may not look identical to the JS.

### 13. [app.cpp] Nav icon hover uses full white instead of CSS brightness(2)
- **JS Source**: `src/app.css` line 501
- **Status**: Pending
- **Details**: The CSS hover effect is `filter: brightness(2)` which doubles the brightness of the existing icon appearance (it doesn't make it pure white — it makes it brighter than the default). The C++ version (line 547) uses `IM_COL32(255, 255, 255, 255)` (full opaque white) on hover, whereas the default is `IM_COL32(255, 255, 255, 204)` (80% alpha). Since the icon images are already white-on-transparent SVGs, `brightness(2)` on an 80% opacity icon would make it 160% white which clamps to full white — so the current behavior is actually correct for white icons. However, for any non-white icon content, this would be incorrect.

### 14. [app.cpp] Footer links font weight not set to bold
- **JS Source**: `src/app.css` lines 93–95
- **Status**: Pending
- **Details**: CSS `a { font-weight: bold; }` — the footer links ("Website", "Discord", etc.) use `<a>` tags which are rendered in bold. The C++ version (lines 758–808) does use `app::theme::getBoldFont()` via `ImGui::PushFont(bold)` which is correct. However, the bold font is pushed for the entire links section including the " - " separators. In the JS, the " - " separators are plain text nodes between `<a>` tags and would inherit the `#footer` font weight (normal), not the bold from `<a>`. The separators should be rendered in normal weight, not bold. ⬜

### 15. [app.cpp] Footer copyright text vertical positioning
- **JS Source**: `src/app.css` lines 202–209
- **Status**: Pending
- **Details**: The CSS footer uses `display: flex; flex-direction: column; align-items: center; justify-content: center;` to vertically center two lines of content. The C++ version (lines 768–816) manually calculates `links_y` and positions the copyright text at `links_y + line_h + 4.0f`. The 4px gap is a hardcoded approximation. The JS flexbox centering would naturally distribute vertical space equally above and below the two-line block. If the font metrics differ slightly between CSS/Selawik and ImGui/Selawik TTF, the vertical centering could be slightly off.

### 16. [app.cpp] Header logo click doesn't check isLoading state
- **JS Source**: `src/app.js` line 142, general Vue template behavior
- **Status**: Pending
- **Details**: In the JS version, clicking the logo/title in the header navigates home. The C++ version (lines 490–496, 506–513) allows clicking the logo and title text to navigate home even when `isLoading` is true, because the click handlers don't check `isLoading`. While the JS nav buttons are hidden during loading (`v-if="!isLoading"`), the logo itself doesn't explicitly block clicks during loading either. However, the nav buttons section at line 518 correctly checks `!core::view->isLoading`. This should be verified for consistency — if JS allows logo clicks during loading, then C++ is correct; if not, it needs a guard.

### 17. [app.cpp] Loading bar background color uses a named constant not matching CSS exactly
- **JS Source**: `src/app.css` lines 786–791
- **Status**: Pending
- **Details**: CSS specifies `#loading-bar { background: rgba(0, 0, 0, 0.2196078431); }`. The C++ version uses `app::theme::LOADING_BAR_BG_U32` (line 1241). Need to verify this constant is `IM_COL32(0, 0, 0, 56)` which is `0.2196 * 255 ≈ 56`. The CSS value `0.2196078431` is an unusual precision — it equals exactly `56/255`. If the constant is correct, this is fine; if not, it's a visual discrepancy.

### 18. [app.cpp] Loading screen background image uses cover sizing in CSS but may not match in C++
- **JS Source**: `src/app.css` lines 766–775
- **Status**: Pending
- **Details**: CSS specifies `background-size: cover;` for the loading background image, which scales the image to cover the entire viewport while maintaining aspect ratio (potentially cropping edges). The C++ version (lines 1159–1164) uses `AddImage` that stretches the image to fill the viewport without preserving aspect ratio. For the loading.gif / loading-xmas.gif images, this may cause visible stretching/distortion if the window aspect ratio differs from the image aspect ratio. Should implement cover-style scaling (scale to fill, crop excess) to match CSS `background-size: cover`.

### 19. [app.cpp] Loading screen only loads first frame of GIF, not animated
- **JS Source**: `src/app.css` lines 766–775
- **Status**: Pending
- **Details**: The JS version uses `loading.gif` and `loading-xmas.gif` as actual animated GIFs displayed via CSS `background-image`. The C++ version (lines 105–111, 198–207) uses `stbi_load` which only loads the first frame of the GIF. This means the loading screen background is static in C++ but animated in JS. This is a visual difference. To match the original, either implement GIF animation (cycling through frames) or document this as a known visual deviation.

### 20. [app.cpp] Context menu hamburger icon click behavior differs from JS
- **JS Source**: `src/app.js` line 613 comment
- **Status**: Pending
- **Details**: The JS uses `@click="contextMenus.stateNavExtra = true"` which always opens the context menu (sets to true, never toggles). The C++ version (line 615) uses `ImGui::OpenPopup("##MenuExtraPopup")` which follows ImGui's popup semantics — if the popup is already open and you click the button, it closes because ImGui closes popups on click-outside first. This is functionally close but the exact click-to-reopen behavior may differ. In JS, clicking the hamburger while the menu is already open keeps it open; in C++ it may close and reopen or just close depending on ImGui's event ordering.

### 21. [app.cpp] Context menu option icons are not rendered
- **JS Source**: `src/app.js` lines 548–553
- **Status**: Pending
- **Details**: The JS context menu options are registered with icon filenames (e.g., `'timeline.svg'`, `'arrow-rotate-left.svg'`, `'palette.svg'`). In the JS template, these icons appear as `<div class="icon" :style="{'--nav-icon': 'url(./fa-icons/' + opt.icon + ')'}">` next to the label. The C++ version (lines 648–659) renders context menu items with `ImGui::MenuItem(opt.label.c_str())` which shows only text, no icons. The icons should be rendered (using Font Awesome glyphs or SVG textures) to match the JS visual appearance.

### 22. [app.cpp] isXmas check references core::view->isXmas but initialization not shown
- **JS Source**: `src/app.js` (Vue data) — `isXmas` is set in `core.makeNewView()`
- **Status**: Pending
- **Details**: The loading screen (line 1157) checks `core::view->isXmas` to select the Christmas background. The `isXmas` field must be properly initialized in `core::makeNewView()` based on the current date (December). Need to verify that `AppState.isXmas` is correctly computed in core.cpp's `makeNewView()` — if it's always false, the Christmas loading screen will never appear, which differs from JS behavior.

### 23. [app.cpp] Drop overlay uses ImGui window instead of CSS full-screen overlay
- **JS Source**: `src/app.css` lines 143–164
- **Status**: Pending
- **Details**: The CSS `#drop-overlay` has `background: var(--background-trans); z-index: 9999` (semi-transparent background covering the entire screen). The `#drop-overlay-icon` uses `background-image: url(./fa-icons/copy.svg)` at `width: 100px; height: 100px`. The C++ version (lines 1269–1315) creates an ImGui window with `BG_TRANS` background and renders the copy icon via Font Awesome at 100px. The visual result should be similar but the z-ordering depends on ImGui window creation order — since it's created after the content window, it should render on top, but it lacks the `z-index: 9999` equivalent. If a loading screen or other overlay is also active, the drop overlay might render behind it.

### 24. [app.cpp] Header title font-size should be 25px with 3px bottom padding
- **JS Source**: `src/app.css` lines 192–201
- **Status**: Pending
- **Details**: CSS specifies `#container #header #logo { font-size: 25px; font-weight: 700; padding: 0 0 3px 40px; background-size: 32px; }`. The logo area has a 32px background image and 40px left padding. The text has a 3px bottom padding for vertical alignment. The C++ version (line 504) uses `ImGui::PushFont(bold, 25.0f)` for the title which sets the font size correctly, but there's no 3px bottom offset. The logo image is rendered at 32px (correct) with cursor positioning that accounts for vertical centering `(HEADER_HEIGHT - 32) * 0.5f = 10.5px` from top. The text uses `(HEADER_HEIGHT - 25) * 0.5f = 14px` from top. In CSS, the 3px bottom padding effectively shifts the text up by 1.5px. This minor offset may cause slight vertical misalignment.

### 25. [app.cpp] Header right-side icon spacing doesn't exactly match CSS
- **JS Source**: `src/app.css` (header layout)
- **Status**: Pending
- **Details**: The header right-side icons (help, hamburger) are positioned using hardcoded pixel offsets from the right edge (lines 608, 669). The hamburger has `right_x -= 15 + 20` (15px right margin + 20px icon width) and help has `right_x -= 10 + 20` (10px gap + 20px icon width). In the JS, these are laid out using CSS flexbox with `margin-left: auto` or absolute positioning from the right. The exact spacing depends on the original CSS rules for these icons. Need to verify the icon positions match the original layout pixel-for-pixel.
