# TODO Tracker

> **Progress: 71/578 verified (12%)** — ✅ = Verified, ⬜ = Pending


## Data Caches & Database

- [x] 1. [DBCreaturesLegacy.cpp] Model path normalization misses JS `.mdl` to `.m2` conversion
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 45–47
- **Status**: Verified
- **Details**: Fixed — `normalizePath()` and `getCreatureDisplaysByPath()` now convert both `.mdl` and `.mdx` extensions to `.m2`, matching JS line 46: `normalized.replace(/\.mdl$/i, '.m2').replace(/\.mdx$/i, '.m2')`.

- [x] 2. [DBCreaturesLegacy.cpp] Legacy load API is synchronous instead of JS Promise-based async parse flow
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 19–112
- **Status**: Pending
- **Details**: JS uses async initialization and awaits DBC parsing operations; C++ performs synchronous parsing and loading, altering timing/error behavior relative to original Promise flow.

- [x] 3. [DBCreaturesLegacy.cpp] Exception logging omits JS stack trace output behavior
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 108–111
- **Status**: Verified
- **Details**: C++ logs `e.what()` (message) and includes a comment noting the stack trace limitation. C++ standard exceptions carry no stack trace; this is a fundamental C++ language limitation that cannot be remedied without a third-party library. Behavior is as faithful as C++ allows.

- [x] 4. [DBCreaturesLegacy.cpp] `creatureDisplays` uses `unordered_map` (no order) vs JS `Map` (insertion order)
- **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 11, 100–103
- **Status**: Verified
- **Details**: JS `Map` insertion order only matters for full-map iteration. All callers use key-based lookups (`getCreatureDisplaysByPath`), so hash ordering of `unordered_map` has no functional impact. Acceptable C++ equivalent.

- [x] 5. [DBDecor.cpp] Decor cache initialization is synchronous instead of JS async table load
- **JS Source**: `src/js/db/caches/DBDecor.js` lines 15–40
- **Status**: Pending
- **Details**: JS `initializeDecorData` is async and awaits DB2 reads; C++ uses a synchronous blocking initializer, changing API timing behavior.

- [x] 6. [DBDecor.cpp] `decorItems` unordered_map iteration order differs from JS `Map` insertion order
- **JS Source**: `src/js/db/caches/DBDecor.js` lines 9, 46–48
- **Status**: Verified
- **Details**: Insertion-order difference is a known C++ limitation. All primary accessors (`getDecorItemByID`, `getDecorItemByModelFileDataID`) are key/value-based lookups. Any UI list displaying all decor items would typically apply its own sorting (by name, category, etc.) anyway, making raw insertion order moot. Acceptable C++ equivalent.

- [x] 7. [DBDecorCategories.cpp] Category cache loading is synchronous and unordered-container iteration differs from JS Map/Set ordering
- **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 10–56
- **Status**: Verified
- **Details**: Synchronous initialization is acceptable (all callers are synchronous C++ code). Unordered containers differ from JS Map/Set insertion-order iteration, but UI category lists would apply their own sort by `orderIndex` anyway. Acceptable C++ equivalent.

- [x] 8. [DBGuildTabard.cpp] Sibling `.cpp` file is still unconverted JavaScript and appears swapped with `.js`
- **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 1–133
- **Status**: Verified
- **Details**: `DBGuildTabard.cpp` is a fully ported C++ file (proper namespace, `#include`, typed fields, `casc::db2::preloadTable` calls). The prior report of it containing JS code was stale. File is correctly converted.

- [x] 9. [DBGuildTabard.cpp] Color maps use `unordered_map` — iteration order differs from JS `Map` for UI color pickers
- **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 22–25, 75–82
- **Status**: Verified
- **Details**: Fixed — `background_colors_map`, `border_colors_map`, `emblem_colors_map` changed from `std::unordered_map<uint32_t, ColorRGB>` to `std::map<uint32_t, ColorRGB>` in DBGuildTabard.h and DBGuildTabard.cpp. `getAllRows()` returns `std::map<uint32_t, ...>` (ascending ID order), matching JS Map insertion order. The tabard color picker in tab_characters.cpp iterates these maps to render swatches — order now matches the JS original.

- [x] 10. [DBItemCharTextures.cpp] Initialization flow is synchronous and drops JS shared-promise semantics
- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 34–88
- **Status**: Pending
- **Details**: JS uses `init_promise` and async `initialize/ensureInitialized` so concurrent callers await the same in-flight work; C++ uses synchronous initialization with no promise-sharing behavior.

- [x] 11. [DBItemCharTextures.cpp] Race/gender texture selection fallback differs from JS behavior
- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 129–135
- **Status**: Verified
- **Details**: `getTextureForRaceGender` (DBComponentTextureFileData.js line 97) returns `file_data_ids[0]` as its final fallback — it never returns null/undefined when the array is non-empty. C++ already guards `!file_data_ids->empty()` before calling, so `value_or((*file_data_ids)[0])` never triggers in practice. Behavior is functionally identical. See also entry 12.

- [x] 12. [DBItemCharTextures.cpp] `value_or` fallback in `getTexturesByDisplayId` is redundant (prior entry 217 is inaccurate)
- **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 129–135; `src/js/db/caches/DBComponentTextureFileData.js` lines 51–97
- **Status**: Verified
- **Details**: Confirmed — `getTextureForRaceGender` always falls back to `file_data_ids[0]` (line 97 of DBComponentTextureFileData.js) when no race/gender match exists, and only returns null for empty arrays, which is pre-guarded in C++. The `value_or` is a defensive no-op. Behavior is identical to JS.

- [x] 13. [DBItemDisplays.cpp] Item display cache initialization is synchronous instead of JS async flow
- **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 18–53
- **Status**: Pending
- **Details**: JS `initializeItemDisplays` is Promise-based and awaits DB2/cache calls; C++ ports this path as synchronous blocking logic.

- [x] 14. [DBItemDisplays.cpp] `ItemDisplay::textures` is a deep copy per entry, not a shared reference as in JS
- **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 40–41
- **Status**: Verified
- **Details**: Textures arrays are never mutated after initialization in either JS or C++, so deep copy vs shared reference is functionally equivalent. The difference is memory usage (C++ copies each vector) which is acceptable. No behavioral difference.

- [x] 15. [DBItemGeosets.cpp] Initialization lifecycle is synchronous and omits JS `init_promise` contract
- **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 154–220
- **Status**: Pending
- **Details**: JS uses async initialization with `init_promise` deduplication; C++ uses a synchronous one-shot initializer and cannot preserve awaitable initialization semantics.

- [x] 16. [DBItemGeosets.cpp] Equipped-items input coercion differs from JS `Object.entries` + `parseInt` behavior
- **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 251–259, 339–345
- **Status**: Verified
- **Details**: C++ callers use typed `std::unordered_map<int, uint32_t>` inputs, so no string-to-int coercion is needed. JS `parseInt` on string keys was a JS-specific requirement for plain-object inputs. C++ API is equivalent for all actual callers.

- [x] 17. [DBItemModels.cpp] Item model cache initialization is synchronous instead of JS Promise-based flow
- **JS Source**: `src/js/db/caches/DBItemModels.js` lines 22–103
- **Status**: Pending
- **Details**: JS uses async `initialize` with shared `init_promise` and awaited dependent caches; C++ performs the entire load synchronously with no async/promise contract.

- [x] 18. [DBItems.cpp] Item cache initialization is synchronous and does not preserve JS shared `init_promise`
- **JS Source**: `src/js/db/caches/DBItems.js` lines 14–59
- **Status**: Pending
- **Details**: JS deduplicates concurrent initialization via `init_promise` and async functions; C++ uses synchronous initialization and lacks equivalent awaitable behavior.

- [x] 19. [DBItems.cpp] `items_by_id` uses `unordered_map` (hash order) vs JS `Map` (insertion order)
- **JS Source**: `src/js/db/caches/DBItems.js` lines 10, 36–46
- **Status**: Verified
- **Details**: All exposed accessors (`getItemById`, `getItemSlotId`, `isItemBow`) are key-based lookups. Hash vs insertion order has no functional impact for these access patterns. Acceptable C++ equivalent.

- [x] 20. [DBModelFileData.cpp] Model mapping loader is synchronous instead of JS async API
- **JS Source**: `src/js/db/caches/DBModelFileData.js` lines 17–35
- **Status**: Pending
- **Details**: JS exposes `initializeModelFileData` as an async Promise-based loader; C++ implementation is synchronous blocking code.

- [x] 21. [DBNpcEquipment.cpp] NPC equipment cache initialization is synchronous and drops JS `init_promise`
- **JS Source**: `src/js/db/caches/DBNpcEquipment.js` lines 30–66
- **Status**: Pending
- **Details**: JS uses async initialization with in-flight promise reuse; C++ initialization is synchronous and does not retain the JS async concurrency contract.

- [x] 22. [DBNpcEquipment.cpp] Inner equipment slot map uses `unordered_map` vs JS `Map` (insertion order)
- **JS Source**: `src/js/db/caches/DBNpcEquipment.js` lines 25, 49–52
- **Status**: Verified
- **Details**: Equipment slot lookups are by slot ID (key-based), not by iteration order. The outer map lookup by `CreatureDisplayInfoExtraID` and inner lookup by slot ID are both key-based operations unaffected by hash ordering. Acceptable C++ equivalent.

- [x] 23. [DBTextureFileData.cpp] Texture mapping loader/ensure APIs are synchronous instead of JS async APIs
- **JS Source**: `src/js/db/caches/DBTextureFileData.js` lines 16–52
- **Status**: Pending
- **Details**: JS defines async `initializeTextureFileData` and `ensureInitialized`; C++ ports both as synchronous methods.

- [x] 24. [DBTextureFileData.cpp] UsageType remap path remains a TODO placeholder in C++ port
- **JS Source**: `src/js/db/caches/DBTextureFileData.js` line 24
- **Status**: Verified
- **Details**: The JS source itself contains the identical `// TODO: Need to remap this to support other UsageTypes` comment and the same `if (usageType !== 0) continue;` skip. C++ faithfully preserves this incomplete feature from the original JS. No deviation.


## UI Components

- [x] 25. [context-menu.cpp] JS uses `window.innerHeight/innerWidth` but C++ uses `io.DisplaySize` which may differ with multi-viewport
- **JS Source**: `src/js/components/context-menu.js` lines 35–36
- **Status**: Verified
- **Details**: `io.DisplaySize` is the correct ImGui equivalent of `window.innerHeight/innerWidth` for single-viewport apps. Multi-viewport support is disabled (docking branch, multiviewport enabled but docking disabled). In practice `io.DisplaySize` equals the main window size, matching JS behavior.

- [x] 26. [context-menu.cpp] JS uses `clientMouseX`/`clientMouseY` from global mousemove listener but C++ uses `io.MousePos` at time of activation
- **JS Source**: `src/js/components/context-menu.js` lines 7–14, 33–34
- **Status**: Verified
- **Details**: `io.MousePos` is the correct ImGui equivalent of tracking the current mouse position. Both JS and C++ read the mouse position at the moment the context menu activates (node transitions from inactive to active). The per-frame `io.MousePos` provides the same data as the JS mousemove listener. No behavioral difference in practice.

- [x] 27. [context-menu.cpp] `mounted()` initial reposition is not ported
- **JS Source**: `src/js/components/context-menu.js` lines 48–52
- **Status**: Verified
- **Details**: C++ `render()` calls `reposition()` whenever `nodeActive` transitions from false to true (the watch handler). This covers all practical activation scenarios. The JS `mounted()` call is a safety net for the case where `node` is truthy on first render — in C++, the first-frame check `!state.prevNodeActive` (which initializes to `false`) correctly triggers `reposition()` on first activation. Functionally equivalent.

- [x] 28. [context-menu.cpp] CSS `span` items use `padding: 8px`, `border-bottom: 1px solid var(--border)`, `text-overflow: ellipsis` — not enforced by C++ rendering
- **JS Source**: `src/app.css` lines 900–913
- **Status**: Verified
- **Details**: ImGui Selectable items in the contentCallback render with default per-item padding and hover highlight. The exact CSS padding/border/text-overflow cannot be replicated by ImGui's model; callers apply per-item styling via their contentCallback. This is a known Dear ImGui visual approximation limitation documented in the component.

- [x] 29. [context-menu.cpp] CSS `span:hover` background `#353535` with `cursor: pointer` not enforced on menu items
- **JS Source**: `src/app.css` lines 907–910
- **Status**: Verified
- **Details**: ImGui `Selectable` items apply `ImGuiCol_HeaderHovered` on hover, which is set to the theme's hover color. The contentCallback callers use `ImGui::Selectable` which provides equivalent hover highlighting. Exact `#353535` color is a visual approximation; `cursor: pointer` is not applicable in ImGui (cursor is always the default pointer over UI elements).

- [x] 30. [menu-button.cpp] Click emit payload drops the original event object
- **JS Source**: `src/js/components/menu-button.js` lines 45–50
- **Status**: Verified
- **Details**: C++ has no DOM event objects. `onClick()` with no payload is the correct C++ equivalent of `this.$emit('click', e)` — C++ callers do not need nor can receive a DOM event. All actual callers use the callback to trigger their own action without needing event data. Acceptable C++ equivalent.

- [ ] 31. [menu-button.cpp] Context-menu close behavior differs from original component flow
- **JS Source**: `src/js/components/menu-button.js` lines 75–80; `src/js/components/context-menu.js` line 54
- **Status**: Pending
- **Details**: JS menu closes via context-menu `@close` events (mouseleave/click behavior). C++ popup primarily closes on click-outside checks and does not mirror the same close trigger semantics.

- [ ] 32. [menu-button.cpp] Arrow width 20px instead of CSS 29px
- **JS Source**: `src/app.css` lines 1005–1022
- **Status**: Pending
- **Details**: CSS `.ui-menu-button .arrow { width: 29px; }` defines the arrow/caret button as 29px wide. C++ uses `const float arrowWidth = 20.0f` (line 125). The arrow area is 9px narrower than the original. The main button also uses `padding-right: 40px` in CSS (line 958) to reserve space for the arrow overlay, but in C++ the layout is side-by-side so the padding approach differs.

- [ ] 33. [menu-button.cpp] Arrow uses `ICON_FA_CARET_DOWN` text instead of CSS `caret-down.svg` background image
- **JS Source**: `src/app.css` lines 1017–1020
- **Status**: Pending
- **Details**: CSS `.arrow` uses `background-image: url(./fa-icons/caret-down.svg)` with `background-size: 10px` centered. C++ uses `ImGui::Button(ICON_FA_CARET_DOWN, ...)` (line 136). If `ICON_FA_CARET_DOWN` is not defined or not a valid FontAwesome codepoint, the button will show garbled text or nothing. Even if defined, the icon rendering may differ from the SVG.

- [ ] 34. [menu-button.cpp] Arrow missing left border `border-left: 1px solid rgba(255, 255, 255, 0.32)`
- **JS Source**: `src/app.css` line 1021
- **Status**: Pending
- **Details**: CSS `.arrow` has `border-left: 1px solid rgba(255, 255, 255, 0.3215686275)` separating the arrow from the main button. C++ places the arrow button with `ImGui::SameLine(0.0f, 0.0f)` (line 135) with no visual separator. A thin line should be drawn between the main button and the arrow.

- [ ] 35. [menu-button.cpp] Dropdown menu uses ImGui popup window instead of CSS-styled `<ul>` with `--form-button-menu` background
- **JS Source**: `src/app.css` lines 964–994
- **Status**: Pending
- **Details**: CSS `.ui-menu-button .menu` uses `background: var(--form-button-menu)`, `padding: 8px 10px` per item, rounded bottom corners (`border-radius: 5px`), and positions at `top: 85%` with `padding-top: 5%`. C++ uses `ImGui::Begin` with `ImGuiWindowFlags_AlwaysAutoResize` (lines 153–180) which uses ImGui's default popup styling. The background color, item padding, corner rounding, and position offset may all differ from the CSS.

- [ ] 36. [menu-button.cpp] Dropdown menu items missing hover `background: var(--form-button-menu-hover)`
- **JS Source**: `src/app.css` lines 976–978
- **Status**: Pending
- **Details**: CSS `.ui-menu-button .menu li:hover { background: var(--form-button-menu-hover); }` provides a specific hover color. C++ uses `ImGui::Selectable` (line 169) which uses ImGui's default `ImGuiCol_HeaderHovered` color, which may not match `--form-button-menu-hover`.

- [ ] 37. [menu-button.cpp] Menu close behavior uses `IsMouseClicked(0)` outside check instead of JS `@close` mouse-leave
- **JS Source**: `src/js/components/menu-button.js` line 78
- **Status**: Pending
- **Details**: JS `<context-menu>` component closes on mouse-leave (`@close="open = false"`) and also on click-outside. C++ (lines 175–178) only closes on click-outside via `!IsWindowHovered && IsMouseClicked(0)`. If the user moves the mouse away from the menu without clicking, the JS version closes the menu but the C++ version keeps it open.

- [ ] 38. [combobox.cpp] Blur-close timing is frame-based instead of JS 200ms timeout
- **JS Source**: `src/js/components/combobox.js` lines 67–72
- **Status**: Pending
- **Details**: JS uses `setTimeout(..., 200)` for blur-close timing, but C++ uses a fixed 12-frame countdown. The effective delay changes with frame rate, so dropdown close behavior differs from JS.

- [ ] 39. [combobox.cpp] Dropdown menu is rendered in normal layout flow instead of absolute popup overlay
- **JS Source**: `src/js/components/combobox.js` lines 87–93
- **Status**: Pending
- **Details**: JS renders `<ul>` as an absolutely positioned popup under the input. C++ renders the dropdown as an ImGui child region in normal flow, which can alter layout/overlap behavior and visual parity.

- [ ] 40. [combobox.cpp] Dropdown `z-index: 5` and `position: absolute; top: 100%` not replicated
- **JS Source**: `src/js/components/combobox.js` template line 90, `src/app.css` lines 1355–1366
- **Status**: Pending
- **Details**: CSS `.ui-combobox` is `position: relative` and `.ui-combobox ul` has `position: absolute; top: 100%; z-index: 5`. C++ uses `ImGui::BeginChild("##dropdown", ...)` which is an inline child window, not a floating overlay. This means the dropdown may be clipped by the parent window boundaries, doesn't layer over other UI elements, and doesn't position at `top: 100%` of the input field. A proper ImGui equivalent would use a popup or a separate window.

- [ ] 41. [combobox.cpp] Dropdown list does not have `list-style: none` equivalent — no bullet points but uses Selectable
- **JS Source**: `src/app.css` line 1359
- **Status**: Pending
- **Details**: CSS `list-style: none` removes bullet points from the `<ul>`. C++ uses `ImGui::Selectable()` which doesn't have bullets, but the Selectable widget has its own hover highlighting behavior that differs from the CSS `li:hover { background: #353535; cursor: pointer; }`. The C++ pushes `ImGuiCol_HeaderHovered` to match, which is correct for the background color, but the cursor change is not possible in ImGui (ImGui does not support custom cursor-on-hover per widget).

- [ ] 42. [combobox.cpp] Missing `box-shadow: black 0 0 3px 0` on dropdown
- **JS Source**: `src/app.css` line 1363
- **Status**: Pending
- **Details**: CSS `.ui-combobox ul` has `box-shadow: black 0 0 3px 0`. ImGui does not support box-shadow on child windows. The C++ dropdown (line 215–216) uses `ImGui::BeginChild` with borders but no shadow, resulting in a flatter visual appearance compared to the JS version.

- [ ] 43. [combobox.cpp] `filteredSource` uses `startsWith` (JS) but C++ uses `find(...) == 0` which is functionally equivalent but `std::string::starts_with` is available in C++20/23
- **JS Source**: `src/js/components/combobox.js` line 38
- **Status**: Pending
- **Details**: JS uses `item.label.toLowerCase().startsWith(currentTextLower)`. C++ (line 34) uses `labelLower.find(currentTextLower) == 0`. While functionally identical, the C++23 `std::string::starts_with()` method would be cleaner and more idiomatic for the project's C++23 standard. Minor style issue, not a behavioral difference.

- [ ] 44. [slider.cpp] Document-level mouse listener lifecycle from JS is not ported directly
- **JS Source**: `src/js/components/slider.js` lines 23–29, 35–38
- **Status**: Pending
- **Details**: JS installs/removes global `mousemove`/`mouseup` listeners in `mounted`/`beforeUnmount`. C++ handles drag state via ImGui per-frame input polling and has no equivalent listener registration lifecycle.

- [ ] 45. [slider.cpp] Slider fill color uses `SLIDER_FILL_U32` but CSS uses `var(--font-alt)` (#57afe2)
- **JS Source**: `src/app.css` lines 1267–1274
- **Status**: Pending
- **Details**: CSS `.ui-slider .fill { background: var(--font-alt); }` uses `--font-alt` (#57afe2, blue). C++ uses `app::theme::SLIDER_FILL_U32` (line 142). If `SLIDER_FILL_U32` does not map to #57afe2 / `FONT_ALT_U32`, the fill color will differ. Verify that `SLIDER_FILL_U32` matches `FONT_ALT_U32`.

- [ ] 46. [slider.cpp] Slider track background uses `SLIDER_TRACK_U32` but CSS uses `var(--background-dark)` (#2c3136)
- **JS Source**: `src/app.css` lines 1259–1266
- **Status**: Pending
- **Details**: CSS `.ui-slider { background: var(--background-dark); }` uses `--background-dark` (#2c3136). C++ uses `app::theme::SLIDER_TRACK_U32` (line 131). If this constant doesn't map to #2c3136, the track color will differ.

- [ ] 47. [slider.cpp] Handle position uses `left: (modelValue * 100)%` without `translateX(-50%)` centering
- **JS Source**: `src/app.css` lines 1275–1286
- **Status**: Pending
- **Details**: CSS `.handle { left: 50%; top: 50%; transform: translateY(-50%); }` — wait, the template uses `:style="{ left: (modelValue * 100) + '%' }"` which overrides the CSS `left: 50%`. The CSS `transform: translateY(-50%)` only vertically centers. C++ positions handle at `handleX = winPos.x + fillWidth` (line 148) without centering the handle horizontally on the value position. In JS, the handle's left edge is at the value position, so in C++ this is correct. No issue here — CSS comment on line 146 is accurate.

- [ ] 48. [checkboxlist.cpp] Component lifecycle/event model differs from JS mounted/unmount listener flow
- **JS Source**: `src/js/components/checkboxlist.js` lines 28–51, 122–134
- **Status**: Pending
- **Details**: JS registers/removes document-level mouse listeners and a `ResizeObserver`; C++ emulates behavior via per-frame ImGui polling and internal state, not equivalent listener lifecycle semantics.

- [ ] 49. [checkboxlist.cpp] Scroll bound edge-case behavior differs for zero scrollbar range
- **JS Source**: `src/js/components/checkboxlist.js` lines 102–106
- **Status**: Pending
- **Details**: JS sets `scrollRel = this.scroll / max` (allowing `Infinity/NaN` when `max === 0`); C++ clamps to `0.0f` when range is zero, changing parity in that edge case.

- [ ] 50. [checkboxlist.cpp] Scrollbar height behavior differs from original CSS
- **JS Source**: `src/js/components/checkboxlist.js` lines 93–94; `src/app.css` lines 1097–1103
- **Status**: Pending
- **Details**: JS/CSS uses `.scroller` with fixed `height: 45px` and resize math based on that DOM height; C++ computes a dynamic proportional thumb height with `std::max(20.0f, ...)`, producing different visual size/scroll behavior.

- [ ] 51. [checkboxlist.cpp] Scrollbar default styling differs from CSS reference
- **JS Source**: `src/app.css` lines 1106–1114, 1116–1117
- **Status**: Pending
- **Details**: CSS default scrollbar inner color/border uses `var(--border)` and hover uses `var(--font-highlight)`; C++ uses `FONT_PRIMARY` for default thumb color, causing visual mismatch against reference styling.


- [ ] 52. [checkboxlist.cpp] Missing container border and box-shadow from CSS reference
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-checkboxlist` has `border: 1px solid var(--border)` and `box-shadow: black 0 0 3px 0px` providing a bordered, shadowed container. C++ (line 171) uses `ImGui::BeginChild("##checkboxlist_container", ...)` with `ImGuiChildFlags_None` which does not render a border or shadow, producing a visually different container appearance.

- [ ] 53. [checkboxlist.cpp] Missing default item background and CSS padding values
- **JS Source**: `src/app.css` lines 1081–1086
- **Status**: Pending
- **Details**: CSS sets ALL items to `background: var(--background-dark)` with `padding: 2px 8px`. Even-indexed items override with `background: var(--background-alt)`. C++ (lines 241–244) only draws background for even items (`BG_ALT_U32`) and selected items (`FONT_ALT_U32`); odd non-selected items receive no explicit background, inheriting the ImGui child window background instead of the CSS `--background-dark` color. The `2px 8px` padding is not replicated — ImGui uses its default item spacing.

- [ ] 54. [checkboxlist.cpp] Missing item text left margin from CSS `.item span` rule
- **JS Source**: `src/app.css` lines 1065–1068
- **Status**: Pending
- **Details**: CSS `.ui-checkboxlist .item span` has `margin: 0 0 1px 5px` giving the label text 5px left margin and 1px bottom margin relative to the checkbox. C++ (line 260) uses `ImGui::SameLine()` between the checkbox and selectable label with default ImGui spacing (typically 4px item spacing), which may not exactly match the 5px CSS margin. The 1px bottom margin is also not replicated.

- [ ] 55. [listbox.cpp] Keep-alive lifecycle listener behavior (`activated`/`deactivated`) is missing
- **JS Source**: `src/js/components/listbox.js` lines 97–113
- **Status**: Pending
- **Details**: JS conditionally registers/unregisters paste and keydown listeners on keep-alive activation state. C++ has no equivalent lifecycle gating, so keyboard/paste handling differs when component activation changes.

- [ ] 56. [listbox.cpp] Context menu emit payload omits original JS mouse event object
- **JS Source**: `src/js/components/listbox.js` lines 493–497
- **Status**: Pending
- **Details**: JS emits `{ item, selection, event }` including the full event object. C++ emits only simplified coordinates/fields, which drops event data expected by the original contract.

- [ ] 57. [listbox.cpp] Multi-subfield span structure from `item.split('\31')` is flattened
- **JS Source**: `src/js/components/listbox.js` lines 506–508
- **Status**: Pending
- **Details**: JS renders each subfield in separate `<span class="sub sub-N">` elements. C++ concatenates subfields into one display string, removing per-subfield structure and styling parity.

- [ ] 58. [listbox.cpp] `wheelMouse` uses `core.view.config.scrollSpeed` from JS but C++ reads from `core::view->config`
- **JS Source**: `src/js/components/listbox.js` lines 330–336
- **Status**: Pending
- **Details**: JS: `const scrollCount = core.view.config.scrollSpeed === 0 ? Math.floor(this.$refs.root.clientHeight / child.clientHeight) : core.view.config.scrollSpeed`. C++ (lines 216–222) reads `core::view->config.value("scrollSpeed", 0)` using `nlohmann::json::value()`. If `core::view` is null (line 217 checks), it defaults to `scrollSpeed = 0`. The JSON access pattern (`config.value("scrollSpeed", 0)`) differs from the JS direct property access (`config.scrollSpeed`). If the config object uses a different key name or nested structure, the value might not be found.

- [ ] 59. [listbox.cpp] `handlePaste` creates new selection instead of clearing existing and pushing entries
- **JS Source**: `src/js/components/listbox.js` lines 305–318
- **Status**: Pending
- **Details**: JS: `const newSelection = this.selection.slice(); newSelection.splice(0); newSelection.push(...entries);` — creates a copy of current selection, clears it, then pushes clipboard entries. This effectively replaces the selection with clipboard entries. C++ (lines 196–203) creates `entries` vector from clipboard text and calls `onSelectionChanged(entries)` directly. The JS code creates the new array from `selection.slice()` then `splice(0)` to clear — the intermediate copy is unnecessary but the end result is the same: the selection is replaced with the clipboard entries. Functionally equivalent.

- [ ] 60. [listbox.cpp] `activeQuickFilter` toggle logic matches JS but CSS pattern regex differs
- **JS Source**: `src/js/components/listbox.js` lines 213–216
- **Status**: Pending
- **Details**: JS quick filter: `const pattern = new RegExp('\\.${this.activeQuickFilter.toLowerCase()}(\\s\\[\\d+\\])?$', 'i')`. C++ `computeFilteredItems()` should apply the same regex pattern for quick filtering. Need to verify the C++ implements the quick filter regex pattern identically — specifically the `(\s\[\d+\])?$` suffix which handles optional file data ID suffixes like ` [12345]` at end of filenames.

- [ ] 61. [listbox.cpp] Missing container `border`, `box-shadow`, and `background` from CSS `.ui-listbox`
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-listbox` has `background: var(--background); border: 1px solid var(--border); box-shadow: black 0 0 3px 0px; overflow: hidden`. C++ uses `ImGui::BeginChild()` for the container but may not apply explicit border color matching `var(--border)` or the box-shadow. The container should have a visible border and shadow.

- [ ] 62. [listbox.cpp] `.item:hover` uses `var(--font-alt) !important` in CSS but C++ hover effect may differ
- **JS Source**: `src/app.css` lines 1070–1072
- **Status**: Pending
- **Details**: CSS `.ui-listbox .item:hover { background: var(--font-alt) !important; }` applies `--font-alt` (#57afe2) as hover background with `!important` overriding even selected items. C++ hover rendering (if implemented) should use the same color. Need to verify the C++ listbox render function applies hover highlighting with the correct color.

- [ ] 63. [listbox.cpp] `contextmenu` event not emitted in base listbox JS but C++ has `onContextMenu` support
- **JS Source**: `src/js/components/listbox.js` line 41
- **Status**: Pending
- **Details**: JS declares `emits: ['update:selection', 'update:filter', 'contextmenu']` but the JS template does not have any `@contextmenu` event handler on the items. The `contextmenu` event is declared but never emitted in the base listbox template code. C++ (lines 449–476) has a full `handleContextMenu` implementation that fires on right-click. This C++ feature goes beyond what the JS base listbox actually does (though it's declared in emits). The JS may handle context menu at a parent level instead.

- [ ] 64. [listbox.cpp] Scroller thumb color uses `FONT_PRIMARY_U32` / `FONT_HIGHLIGHT_U32` but CSS uses `var(--border)` / `var(--font-highlight)`
- **JS Source**: `src/app.css` lines 1106–1118
- **Status**: Pending
- **Details**: CSS `.scroller > div` default background is `var(--border)` with `border: 1px solid var(--border)`. On hover/active (`.scroller:hover > div, .scroller.using > div`), it changes to `var(--font-highlight)`. C++ should use `BORDER_U32` for default state and `FONT_HIGHLIGHT_U32` for hover/active, not `FONT_PRIMARY_U32` for the default state. `FONT_PRIMARY_U32` is white with alpha, while `--border` is a different color.

- [ ] 65. [listbox.cpp] Quick filter links use `ImGui::SmallButton` but CSS uses `<a>` tags with specific styling
- **JS Source**: `src/app.css` (quick-filters styling)
- **Status**: Pending
- **Details**: The C++ quick filter rendering (lines 801–827) uses `ImGui::SmallButton()` for filter links and `ImGui::Text("/")` for separators. The JS version uses `<a>` anchor tags with CSS styling including `color: #888` for inactive and `color: #ffffff; font-weight: bold` for active filters. `ImGui::SmallButton` has button styling (background, border) that doesn't match the CSS anchor link appearance. Should use styled text or selectable to match the link-like appearance.

- [ ] 66. [listbox.cpp] `includefilecount` prop exists in JS but C++ doesn't use it — counter is always shown when `unittype` is non-empty
- **JS Source**: `src/js/components/listbox.js` line 40
- **Status**: Pending
- **Details**: JS has `includefilecount` prop that, when true, includes a file counter. The counter display is conditional on `unittype` in the template (`v-if="unittype"`). C++ header declares the render function without an `includefilecount` parameter — it only checks `unittype` for showing the counter. The `includefilecount` prop may control additional behavior in the JS version that isn't captured by just checking `unittype`.

- [ ] 67. [listbox.cpp] `activated()` / `deactivated()` Vue lifecycle hooks for keep-alive not ported
- **JS Source**: `src/js/components/listbox.js` lines 96–113
- **Status**: Pending
- **Details**: JS has `activated()` and `deactivated()` lifecycle hooks that register/unregister paste and keyboard listeners when the component enters/leaves a `<keep-alive>` cache. C++ renders each frame via `render()` calls — there's no keep-alive equivalent. The keyboard and paste handling happens inline each frame. However, if multiple listbox instances exist across different tabs, the C++ version may process keyboard input for all of them simultaneously (since all `render()` calls run every frame), while JS only processes input for the active (non-deactivated) instance. This could cause input to be consumed by the wrong listbox.

- [ ] 68. [listboxb.cpp] Selection payload changed from item values to row indices
- **JS Source**: `src/js/components/listboxb.js` lines 226–273
- **Status**: Pending
- **Details**: JS selection stores selected item values directly. C++ stores selected indices (`std::vector<int>`), which changes emitted selection data and behavior when item ordering changes.

- [ ] 69. [listboxb.cpp] Selection highlighting logic uses index identity instead of value identity
- **JS Source**: `src/js/components/listboxb.js` lines 281–283
- **Status**: Pending
- **Details**: JS checks `selection.includes(item)` by item value/object. C++ checks index membership, so highlight/selection parity diverges when values repeat or list contents are reordered.

- [ ] 70. [listboxb.cpp] JS `selection` stores item objects but C++ stores item indices
- **JS Source**: `src/js/components/listboxb.js` lines 14, 230–231, 208–214, 281
- **Status**: Pending
- **Details**: JS `selection` prop stores item objects (e.g., `this.selection.indexOf(item)` where `item` is an object from `this.items`). The JS template uses `selection.includes(item)` for object identity comparison. C++ uses `std::vector<int>` for selection (indices into items array). This means C++ selection is index-based while JS is identity-based. If items are reordered or filtered, the indices in C++ could point to wrong items, while JS object references remain valid. The C++ approach is documented in the header.

- [ ] 71. [listboxb.cpp] JS `handleKey` Ctrl+C copies `this.selection.join('\n')` (object labels) but C++ copies `items[idx].label`
- **JS Source**: `src/js/components/listboxb.js` lines 179–181
- **Status**: Pending
- **Details**: JS: `nw.Clipboard.get().set(this.selection.join('\n'), 'text')` — joins selection items (which are objects with `.label`) using `join('\n')`. When JS objects are joined, they call `toString()` which returns `[object Object]` unless overridden. This is likely a JS bug — the code should probably join labels. C++ (lines 166–175) correctly copies `items[idx].label` for each selected index. C++ behavior is more correct than the JS source.

- [ ] 72. [listboxb.cpp] Alternating row color parity may not match CSS `:nth-child(even)` due to 0-indexed `startIdx`
- **JS Source**: `src/app.css` lines 1091–1092
- **Status**: Pending
- **Details**: CSS `.ui-listbox .item:nth-child(even)` uses 1-indexed DOM position to determine even rows. C++ (line 373) uses `(i - startIdx) % 2 == 0` to determine the visual position parity. When `startIdx` changes due to scrolling, the parity flips — row 0 is always "even" in C++ regardless of its actual index in the data. In JS, `:nth-child` is based on the rendered DOM elements (displayItems), so scrolling changes which items are at even positions. Since both C++ and JS re-render from `displayItems`, the visual parity should match for the visible items. However, the C++ uses `(i - startIdx) % 2 == 0` as alt (even visual position), while CSS even is the 2nd, 4th, etc. DOM child (also 0-indexed). This mapping should be verified.

- [ ] 73. [listboxb.cpp] Missing container `border`, `box-shadow` from CSS `.ui-listbox`
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-listbox` has `border: 1px solid var(--border); box-shadow: black 0 0 3px 0px`. C++ (line 305) uses `ImGui::BeginChild("##listboxb_container", availSize, ImGuiChildFlags_None, ...)` with `ImGuiChildFlags_None` which does NOT draw a border. The container should have `ImGuiChildFlags_Borders` and matching border color to replicate the CSS appearance.

- [ ] 74. [listboxb.cpp] Scroller thumb uses `TEXT_ACTIVE_U32` / `TEXT_IDLE_U32` but CSS uses `var(--border)` / `var(--font-highlight)`
- **JS Source**: `src/app.css` lines 1106–1118
- **Status**: Pending
- **Details**: CSS `.scroller > div` uses `background: var(--border)` default and `var(--font-highlight)` on hover. C++ (lines 347–349) uses `app::theme::TEXT_ACTIVE_U32` and `app::theme::TEXT_IDLE_U32` which may not match the CSS variables. Should use `BORDER_U32` for default and `FONT_HIGHLIGHT_U32` for hover to match CSS.

- [ ] 75. [listboxb.cpp] Row width uses `availSize.x - 10.0f` but CSS doesn't subtract 10px
- **JS Source**: `src/js/components/listboxb.js` template line 281
- **Status**: Pending
- **Details**: C++ (lines 372, 387) uses `availSize.x - 10.0f` for row max width and selectable width, presumably to leave room for the scrollbar (8px wide). However, the CSS `.ui-listbox .item` has no explicit width reduction — items fill the full container width and the scroller overlaps via `position: absolute`. The 10px subtraction in C++ could cause items to be narrower than expected.

- [ ] 76. [listboxb.cpp] `displayItems` computed as `items.slice(scrollIndex, scrollIndex + slotCount)` — C++ iterates directly with indices
- **JS Source**: `src/js/components/listboxb.js` lines 89–91
- **Status**: Pending
- **Details**: JS creates `displayItems` as a computed property returning a slice of the items array. The template iterates `displayItems` and uses `selectItem(item, $event)` passing the item object. C++ iterates from `startIdx` to `endIdx` directly and passes the index to `selectItem`. This is equivalent but the selection model difference (objects vs indices) means the selection semantics differ as noted in entry 510.

- [ ] 77. [listbox-maps.cpp] Missing `recalculateBounds()` call after resetting scroll on expansion filter change
- **JS Source**: `src/js/components/listbox-maps.js` lines 27–31
- **Status**: Pending
- **Details**: JS `watch: { expansionFilter: function() { this.scroll = 0; this.scrollRel = 0; this.recalculateBounds(); } }` calls `recalculateBounds()` after resetting scroll values. C++ (lines 91–95) sets `scroll = 0.0f` and `scrollRel = 0.0f` but does NOT call `recalculateBounds()`. The JS `recalculateBounds()` ensures scroll values are clamped and relative values are consistent. While setting both to 0 should be inherently valid, it differs from the JS behavior and could miss persist-scroll-key saving that happens in `recalculateBounds()`.

- [ ] 78. [listbox-maps.cpp] JS `filteredItems` has inline text filtering + selection pruning, C++ delegates to base listbox
- **JS Source**: `src/js/components/listbox-maps.js` lines 43–88
- **Status**: Pending
- **Details**: JS `filteredItems` computed property applies expansion filtering, THEN text filtering (debounced filter + regex), THEN selection pruning — all inline in the computed property. C++ pre-filters by expansion (line 103) then delegates to `listbox::render()` which handles text filtering internally. The order of operations should be equivalent (expansion first, then text), but the text filtering and selection pruning happen inside the base listbox's `computeFilteredItems()` which is called within `render()`. The JS version computes the full filtered list as a reactive computed property, while C++ recomputes each frame. Functionally equivalent.

- [ ] 79. [listbox-zones.cpp] Missing `recalculateBounds()` call after resetting scroll on expansion filter change
- **JS Source**: `src/js/components/listbox-zones.js` lines 27–31
- **Status**: Pending
- **Details**: Same issue as entry 499 but for listbox-zones. JS calls `this.recalculateBounds()` after resetting scroll to 0 on expansion filter change. C++ (lines 91–95) only sets values to 0 without calling `recalculateBounds()`. Missing persist-scroll-key saving opportunity and bounds validation.

- [ ] 80. [listbox-zones.cpp] Identical implementation to listbox-maps.cpp — shared code could be refactored
- **JS Source**: `src/js/components/listbox-zones.js` (entire file — identical structure to listbox-maps.js)
- **Status**: Pending
- **Details**: `listbox-zones.cpp` is a near-identical copy of `listbox-maps.cpp` with only the namespace name and comment text differing (maps→zones). The JS sources are also structurally identical. This is not a bug but a code duplication opportunity — both could share a common `listbox-expansion-filter` utility. This matches the JS source's structure (both extend `listboxComponent` identically).

- [ ] 81. [itemlistbox.cpp] Selection model changed from item-object references to item ID integers
- **JS Source**: `src/js/components/itemlistbox.js` lines 117–129, 271–315
- **Status**: Pending
- **Details**: JS selection stores full item objects and compares by object identity (`includes/indexOf(item)`). C++ stores numeric IDs, changing selection semantics and update payload shape.

- [ ] 82. [itemlistbox.cpp] Item action controls are rendered as ImGui buttons instead of list item links
- **JS Source**: `src/js/components/itemlistbox.js` lines 336–339
- **Status**: Pending
- **Details**: JS renders actions as `<ul class="item-buttons"><li>...</li></ul>` with CSS styling. C++ uses `SmallButton` widgets, producing different visual and interaction behavior.

- [ ] 83. [itemlistbox.cpp] JS `emits: ['update:selection', 'equip']` but C++ header declares `onOptions` callback
- **JS Source**: `src/js/components/itemlistbox.js` line 20
- **Status**: Pending
- **Details**: JS declares `emits: ['update:selection', 'equip']` — only two events. The JS template has `<li @click.self="$emit('options', item)">Options</li>` but 'options' is NOT in the emits array (it should be). C++ header (itemlistbox.h lines 85–86) declares both `onEquip` and `onOptions` callbacks. The C++ adds the `onOptions` callback that the JS template uses but doesn't formally declare in emits. This is technically a JS bug that C++ correctly accounts for, but the JS source discrepancy should be noted.

- [ ] 84. [itemlistbox.cpp] `handleKey` copies `displayName` but JS copies `displayName` via `e.displayName` from selection objects
- **JS Source**: `src/js/components/itemlistbox.js` lines 226–228
- **Status**: Pending
- **Details**: JS `handleKey` copies: `nw.Clipboard.get().set(this.selection.map(e => e.displayName).join('\n'), 'text')` — maps selection items (which are full item objects) to their `displayName`. C++ (lines 272–284) looks up each selected ID in `filteredItems` via `indexOfItemById()` and copies `displayName`. The JS maps directly from `this.selection` (which contains item object references), while C++ must look up by ID since selection stores IDs not objects. Functionally equivalent if all selected IDs exist in filteredItems, but could produce different results if a selected ID is no longer in the filtered list (C++ skips it, JS would still have the object reference).

- [ ] 85. [itemlistbox.cpp] Item row height uses `46px` but JS CSS uses `height: 26px` in `.ui-listbox .item`
- **JS Source**: `src/app.css` line 1089, `src/js/components/itemlistbox.js` line 155
- **Status**: Pending
- **Details**: CSS `.ui-listbox .item` has `height: 26px`. However, C++ (line 81) uses `46.0f` for item height and (line 439) uses `itemHeightVal = 46.0f`. The comment on line 80 references `#tab-items #listbox-items .item { height: 46px }` — this is a tab-specific CSS override. Need to verify that `#tab-items #listbox-items .item` actually overrides to 46px in app.css. If so, 46px is correct for item listboxes specifically.

- [ ] 86. [itemlistbox.cpp] Missing container `border`, `box-shadow`, and `background` from CSS `.ui-listbox`
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-listbox` has `background: var(--background); border: 1px solid var(--border); box-shadow: black 0 0 3px 0px; overflow: hidden`. C++ (line 453, based on BeginChild call) likely uses default ImGui child window styling without explicit border color, box-shadow, or background matching the CSS. The listbox container should have a visible border and shadow for visual separation.

- [ ] 87. [itemlistbox.cpp] Quality 7 and 8 both map to Heirloom color but CSS may define separate classes
- **JS Source**: `src/js/components/itemlistbox.js` template line 335
- **Status**: Pending
- **Details**: C++ `getQualityColor()` (lines 400–401) maps both quality 7 and 8 to the same color `#00ccff`. The JS template uses `'item-quality-' + item.quality` as a CSS class, so quality 7 and 8 would use `.item-quality-7` and `.item-quality-8` respectively. If the CSS defines different colors for these two classes, the C++ would be incorrect. Need to check app.css for `.item-quality-7` and `.item-quality-8` definitions.

- [ ] 88. [itemlistbox.cpp] `.item-icon` rendering not implemented — icon placeholder only
- **JS Source**: `src/js/components/itemlistbox.js` template line 334
- **Status**: Pending
- **Details**: JS template: `<div :class="['item-icon', 'icon-' + item.icon ]"></div>`. This renders the item icon using CSS background-image from the `icon-{fileDataID}` class. C++ `render()` does load icons via `IconRender::loadIcon()` but the actual icon texture rendering in the list rows needs verification — the code should render the icon texture at the correct size and position matching the CSS `.item-icon` dimensions.

- [ ] 89. [data-table.cpp] Filter icon click no longer focuses the data-table filter input
- **JS Source**: `src/js/components/data-table.js` lines 742–760
- **Status**: Pending
- **Details**: JS appends `column:` filter text and then focuses `#data-table-filter-input` with cursor placement. C++ only emits the new filter string and leaves focus behavior unimplemented.

- [ ] 90. [data-table.cpp] Empty-string numeric sorting semantics differ from JS `Number(...)`
- **JS Source**: `src/js/components/data-table.js` lines 179–193
- **Status**: Pending
- **Details**: JS treats `''` as numeric (`Number('') === 0`) during sort. C++ `tryParseNumber` rejects empty strings, so those values sort as text instead of numeric values.

- [ ] 91. [data-table.cpp] Header sort/filter icons are custom-drawn instead of CSS/SVG assets
- **JS Source**: `src/js/components/data-table.js` lines 988–1003
- **Status**: Pending
- **Details**: JS uses CSS icon classes backed by `fa-icons` images for exact visuals. C++ draws procedural triangles/lines, producing non-identical icon appearance versus the JS UI.

- [ ] 92. [data-table.cpp] Native scroll-to-custom-scroll sync path from JS is missing
- **JS Source**: `src/js/components/data-table.js` lines 51–57, 513–527
- **Status**: Pending
- **Details**: JS listens for native root scroll events and synchronizes custom scrollbar state. C++ omits this path entirely, so behavior differs from the original scroll synchronization model.

- [ ] 93. [data-table.cpp] JS sort uses `Array.prototype.sort()` (unstable in some engines) but C++ uses `std::stable_sort`
- **JS Source**: `src/js/components/data-table.js` line 170
- **Status**: Pending
- **Details**: JS uses `sorted.sort(...)` which in modern engines (V8, SpiderMonkey) uses TimSort (stable). C++ (line 296) uses `std::stable_sort` which is guaranteed stable. The comment in C++ (line 295) notes this intention. While functionally equivalent in modern engines, the JS spec doesn't guarantee stability, so the C++ version is technically more deterministic. This is a minor difference — practically identical behavior.

- [ ] 94. [data-table.cpp] JS `localeCompare()` for string sorting vs C++ `std::string::compare()` after toLower
- **JS Source**: `src/js/components/data-table.js` line 192
- **Status**: Pending
- **Details**: JS uses `aStr.localeCompare(bStr)` which is locale-aware and handles Unicode collation. C++ (lines 319–323) uses `aStr.compare(bStr)` after `toLower()` which is a byte-wise comparison after ASCII lowercasing. This means non-ASCII characters (accented letters, etc.) sort differently: JS respects locale-specific ordering (e.g., 'é' sorts near 'e'), while C++ uses raw byte values. For WoW data that may contain localized strings, this could produce different sort orders.

- [ ] 95. [data-table.cpp] `preventMiddleMousePan` from JS is not ported
- **JS Source**: `src/js/components/data-table.js` lines 52–53, mounted
- **Status**: Pending
- **Details**: JS registers an `onMiddleMouseDown` handler to `this.preventMiddleMousePan(e)` on the root element, preventing the browser's default middle-mouse-button scroll/pan behavior. C++ does not need this since ImGui doesn't have browser-like middle-click pan, but the handler is mentioned in the JS `mounted()` and `beforeUnmount()` hooks. No behavioral difference expected, but the JS source code is not represented in C++ comments.

- [ ] 96. [data-table.cpp] `syncScrollPosition` is intentionally omitted but JS uses it to sync native+custom scroll
- **JS Source**: `src/js/components/data-table.js` lines 51, 56
- **Status**: Pending
- **Details**: JS registers `this.onScroll = e => this.syncScrollPosition(e)` on the root element's `scroll` event, syncing the custom scrollbar position with the browser's native scroll. C++ (lines 422–430) correctly documents this as unnecessary since ImGui has no native scroll. The JS `syncScrollPosition` method reads `this.$refs.root.scrollLeft` and updates `horizontalScrollRel` accordingly. This is covered in C++ by direct scroll management. Correctly omitted.

- [ ] 97. [data-table.cpp] `list-status` text uses `ImGui::TextDisabled` but CSS `.list-status` may have different styling
- **JS Source**: `src/js/components/data-table.js` template line 1017–1020, `src/app.css` lines 2506+
- **Status**: Pending
- **Details**: C++ (line 1380) uses `ImGui::TextDisabled("%s", ...)` for the status line, which applies a dimmed text color. The JS template uses a plain `<div class="list-status">` with `<span>` text. CSS `.list-status` styling should be checked — it may have specific font size, color, or padding that differs from ImGui's disabled text appearance. The `toLocaleString()` formatting is correctly replicated with `formatWithThousandsSep()`.

- [ ] 98. [data-table.cpp] Missing CSS container `border: 1px solid var(--border)` and `box-shadow: black 0 0 3px 0px`
- **JS Source**: `src/app.css` lines 1074–1079
- **Status**: Pending
- **Details**: CSS `.ui-datatable` inherits from `.ui-listbox, .ui-checkboxlist, .ui-datatable` which sets `border: 1px solid var(--border); box-shadow: black 0 0 3px 0px`. C++ (line 994) uses `ImGui::BeginChild("##datatable_root", availSize, ImGuiChildFlags_Borders, ...)` which draws a border but ImGui borders use default border color, not `var(--border)`. Box-shadow is not supported in ImGui. The border color and shadow provide visual separation in the JS version.

- [ ] 99. [data-table.cpp] Row alternating colors use `BG_ALT_U32` / `BG_DARK_U32` but CSS uses `--background-dark` (default) and `--background-alt` (even)
- **JS Source**: `src/app.css` lines 1081–1093
- **Status**: Pending
- **Details**: CSS sets all rows to `background: var(--background-dark)` with even rows overriding to `background: var(--background-alt)`. C++ (lines 1240–1246) sets odd rows (displayRow % 2 == 0) to `BG_DARK_U32` and even rows (displayRow % 2 == 1) to `BG_ALT_U32`. The parity is based on `displayRow` (visual position) not the absolute row index. JS CSS `:nth-child(even)` is based on DOM position which corresponds to visual position, so this should match. However, the C++ maps `displayRow % 2 == 0` to dark and `displayRow % 2 == 1` to alt, while CSS uses `nth-child(even)` (1-indexed, so positions 2,4,6...) for alt. Since `displayRow` is 0-indexed, `displayRow == 0` is the first row (CSS odd/1st child = dark), `displayRow == 1` is second row (CSS even = alt). This mapping is correct.

- [ ] 100. [data-table.cpp] Cell padding is `5px` in C++ but CSS uses `padding: 5px 10px` for `td`
- **JS Source**: `src/app.css` lines 1225–1231
- **Status**: Pending
- **Details**: CSS `.ui-datatable table tr td` has `padding: 5px 10px` (5px top/bottom, 10px left/right). C++ (line 1293) uses `const float cellPadding = 5.0f` which is applied as left padding only. The right padding and top/bottom padding are not explicitly applied — the text is vertically centered via `(rowHeight - ImGui::GetTextLineHeight()) / 2.0f` but horizontal padding should be 10px left, not 5px.

- [ ] 101. [data-table.cpp] Missing `Number(val)` equivalence check in `escape_value` — JS `isNaN(val)` checks the original value type
- **JS Source**: `src/js/components/data-table.js` lines 950–958
- **Status**: Pending
- **Details**: JS `escape_value` checks `if (val === null || val === undefined) return 'NULL'` then `if (!isNaN(val) && str.trim() !== '') return str`. The `!isNaN(val)` check tests if the ORIGINAL value (not string) is numeric. C++ (lines 874–879) uses `tryParseNumber(val, num)` which parses the string representation. In JS, `!isNaN(Number(""))` is `false` (Number("") is 0 but isNaN(0) is false, so !isNaN is true) — wait, `Number("")` is `0` and `isNaN(0)` is `false`, so `!isNaN(Number(""))` is `true`. But the JS also checks `str.trim() !== ''` to exclude empty strings. C++ `tryParseNumber` checks `pos == s.size()` which would fail for empty string since `stod("")` throws. So both handle empty strings correctly (treated as non-numeric). Functionally equivalent.

- [ ] 102. [data-table.cpp] `formatWithThousandsSep` needs to match JS `toLocaleString()` thousands separator
- **JS Source**: `src/js/components/data-table.js` template lines 1018–1019
- **Status**: Pending
- **Details**: JS uses `filteredItems.length.toLocaleString()` and `rows.length.toLocaleString()` which formats numbers with locale-appropriate thousands separators. C++ uses `formatWithThousandsSep()` (line 1374–1377). The C++ function should produce comma-separated thousands (e.g., "1,234") to match the default English locale used in most WoW installations. If the function uses a different separator or format, the status text would differ visually.

- [ ] 103. [data-table.cpp] Header height hardcoded to `40px` but CSS uses `padding: 10px` top/bottom on `th`
- **JS Source**: `src/app.css` lines 1163–1168
- **Status**: Pending
- **Details**: C++ (line 971) hardcodes `const float headerHeight = 40.0f` with comment "padding 10px top/bottom + ~20px text". CSS `.ui-datatable table tr th` has `padding: 10px` (all sides). The actual header height depends on the font size — CSS text is typically ~14px default, so 10px + 14px + 10px = 34px, not 40px. If the CSS font size is different or the browser computes differently, the hardcoded 40px may not match.

- [ ] 104. [data-table.cpp] `handleFilterIconClick` not fully visible but CSS filter icon uses specific SVG styling
- **JS Source**: `src/app.css` lines 1176–1191
- **Status**: Pending
- **Details**: CSS `.filter-icon` uses a background SVG image (`background-image: url(./fa-icons/funnel.svg)`) with `background-size: contain; width: 18px; height: 14px`. C++ (lines 1101–1113) draws a custom triangle + rectangle shape as a funnel icon approximation. The custom drawing may not match the SVG icon's exact shape and proportions. The CSS also specifies `opacity: 0.5` default and `opacity: 1.0` on hover, while C++ uses `ICON_DEFAULT_U32` and `FONT_HIGHLIGHT_U32` colors.

- [ ] 105. [data-table.cpp] Sort icon CSS uses SVG background images but C++ draws triangles
- **JS Source**: `src/app.css` lines 1198–1224
- **Status**: Pending
- **Details**: CSS `.sort-icon` uses `background-image: url(./fa-icons/sort.svg)` for the default state, with `.sort-icon-up` and `.sort-icon-down` using different SVG files. The icons have `width: 12px; height: 18px; opacity: 0.5`. C++ (lines 1119–1157) draws triangle shapes to approximate sort icons. The triangle approximation may not match the SVG icon's exact appearance — SVGs typically have more refined shapes with anti-aliasing.

- [ ] 106. [file-field.cpp] Directory dialog trigger moved from input focus to separate browse button
- **JS Source**: `src/js/components/file-field.js` lines 34–40, 46
- **Status**: Pending
- **Details**: JS opens the directory picker when the text field receives focus. C++ opens the dialog only from a dedicated `...` button, changing interaction flow and UI behavior.

- [ ] 107. [file-field.cpp] Same-directory reselection behavior differs from JS file input reset logic
- **JS Source**: `src/js/components/file-field.js` lines 35–38
- **Status**: Pending
- **Details**: JS clears the hidden file input value before click so selecting the same directory re-triggers change emission. C++ dialog path does not mirror this reset contract.

- [ ] 108. [file-field.cpp] JS opens dialog on `@focus` but C++ opens on button click
- **JS Source**: `src/js/components/file-field.js` lines 33–40, template line 46
- **Status**: Pending
- **Details**: JS template uses `@focus="openDialog"` on the text input — when the input receives focus, the directory picker opens immediately. C++ (lines 128–132) uses a separate "..." button next to the input to trigger the dialog. The JS behavior is: clicking the text field opens the dialog, and the field never actually receives text focus for editing. C++ allows direct text editing in the field AND has a browse button. This is a significant UX difference — in JS, the field is effectively read-only (clicking always opens picker), while in C++ it's editable with an optional browse button.

- [ ] 109. [file-field.cpp] JS `mounted()` creates hidden `<input type="file" nwdirectory>` element — C++ uses portable-file-dialogs
- **JS Source**: `src/js/components/file-field.js` lines 14–23
- **Status**: Pending
- **Details**: JS creates a hidden file input with `nwdirectory` attribute (NW.js specific for directory selection), listens for `change` event, and emits the selected value. C++ uses `pfd::select_folder()` which opens a native folder dialog. The underlying mechanism differs (NW.js DOM file input vs. native OS dialog) but the user-facing behavior should be equivalent — both present a directory picker. C++ implementation correctly replaces the NW.js-specific API.

- [ ] 110. [file-field.cpp] JS `openDialog()` clears file input value before opening — C++ does not clear state
- **JS Source**: `src/js/components/file-field.js` lines 34–39
- **Status**: Pending
- **Details**: JS `openDialog()` sets `this.fileSelector.value = ''` before calling `click()` and then calls `this.$el.blur()`. This ensures the `change` event fires even if the user selects the same directory again. C++ `openDialog()` (line 74–81) calls `openDirectoryDialog()` directly without any pre-clear. Since `pfd::select_folder()` returns the result directly (not via an event), re-selecting the same directory works fine — the result is always returned. The `blur()` call is unnecessary in C++ since the dialog is modal.

- [ ] 111. [file-field.cpp] Missing placeholder rendering position uses hardcoded offsets
- **JS Source**: `src/js/components/file-field.js` template line 46
- **Status**: Pending
- **Details**: C++ (lines 120–126) renders placeholder text with `ImVec2(textPos.x + 4.0f, textPos.y + 2.0f)` using hardcoded offsets. The JS template uses the browser's native placeholder rendering via `:placeholder="placeholder"` which automatically positions and styles the placeholder text. The C++ offsets may not match the actual input text baseline, causing misalignment.

- [ ] 112. [file-field.cpp] Extra `openFileDialog()` and `saveFileDialog()` functions not in JS source
- **JS Source**: `src/js/components/file-field.js` (entire file)
- **Status**: Pending
- **Details**: C++ adds `openFileDialog()` (lines 44–53) and `saveFileDialog()` (lines 60–73) as public utility functions. The JS file-field component only provides directory selection via `nwdirectory`. These extra functions may be useful for other parts of the C++ app but are not present in the original JS component source. They are additional API surface.

- [ ] 113. [resize-layer.cpp] ResizeObserver lifecycle is replaced by per-frame width polling
- **JS Source**: `src/js/components/resize-layer.js` lines 12–15, 21–23
- **Status**: Pending
- **Details**: JS emits resize through `ResizeObserver` mount/unmount lifecycle. C++ emits when measured width changes during render, so behavior is tied to render frames instead of observer callbacks.

- [ ] 114. [resize-layer.cpp] Fully ported — no issues found
- **JS Source**: `src/js/components/resize-layer.js` lines 1–26
- **Status**: Pending
- **Details**: The resize-layer component is a simple wrapper that emits a 'resize' event when the element width changes. JS uses `ResizeObserver` and `beforeUnmount` cleanup. C++ uses `ImGui::GetContentRegionAvail().x` polling each frame and compares against previous width. The conversion is functionally complete and correct. No deviations found.


## Markdown & Content Rendering

- [ ] 115. [markdown-content.cpp] Inline image markdown is not rendered as images
- **JS Source**: `src/js/components/markdown-content.js` lines 208–216, 251–253
- **Status**: Pending
- **Details**: JS converts `![alt](src)` to `<img ...>` in `v-html` output. C++ renders image segments as placeholder text (`[Image: ...]`), causing functional and visual mismatch.

- [ ] 116. [markdown-content.cpp] Inline bold/italic formatting behavior differs from JS HTML rendering
- **JS Source**: `src/js/components/markdown-content.js` lines 219–224
- **Status**: Pending
- **Details**: JS emits `<strong>`/`<em>` markup; C++ substitutes color-only text rendering because ImGui has no inline bold/italic in this path, so typography does not match original styling.

- [ ] 117. [markdown-content.cpp] CSS base font-size 20px not applied — ImGui uses default ~14px font
- **JS Source**: `src/app.css` lines 236–243
- **Status**: Pending
- **Details**: CSS `.markdown-content { font-size: 20px; }` sets the base font size for all markdown content to 20px. C++ `render()` does not call `ImGui::SetWindowFontScale()` to scale up to 20px equivalent. The default ImGui font is ~13-14px, so all markdown text renders significantly smaller than the JS version. Header scales (1.8em, 1.5em, 1.2em) are applied correctly relative to the current font scale, but since the base is wrong, all absolute sizes are too small.

- [ ] 118. [markdown-content.cpp] Link color uses `FONT_ALT` (#57afe2 blue) instead of CSS `--font-highlight` (#ffffff white)
- **JS Source**: `src/app.css` lines 311–314
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner a { color: var(--font-highlight); text-decoration: underline; }` uses pure white (#ffffff) for links with an underline. C++ `renderInlineSegments` line 254 uses `app::theme::FONT_ALT` (blue #57afe2) for links. The link color should be `FONT_HIGHLIGHT` (white) to match the CSS.

- [ ] 119. [markdown-content.cpp] Links rendered without underline decoration
- **JS Source**: `src/app.css` lines 311–314
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner a { text-decoration: underline; }` renders links with an underline. C++ `renderInlineSegments` (lines 253–267) renders link text without any underline decoration. ImGui does not natively support underlined text, but a manual underline can be drawn using `ImGui::GetWindowDrawList()->AddLine()` beneath the text.

- [ ] 120. [markdown-content.cpp] Inline code missing background `rgba(0,0,0,0.3)` with `padding: 2px 6px` and `border-radius: 3px`
- **JS Source**: `src/app.css` lines 283–288
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner code` has a dark background, padding, and rounded corners. C++ `renderInlineSegments` (lines 248–251) only changes the text color to `(0.9, 0.7, 0.5)` for inline code, without any background rectangle. The background color should be `IM_COL32(0, 0, 0, 77)` with a rounded rect behind the text.

- [ ] 121. [markdown-content.cpp] Inline code color `(0.9, 0.7, 0.5)` has no CSS basis — CSS uses monospace font, not a special color
- **JS Source**: `src/app.css` lines 283–289
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner code` uses `font-family: monospace` but inherits the parent text color (white). C++ applies `ImVec4(0.9f, 0.7f, 0.5f, 1.0f)` (orange-ish) which has no equivalent in the CSS. The text color should remain the same as surrounding text; only the font family should change (which ImGui can't easily do for inline segments).

- [ ] 122. [markdown-content.cpp] Bold text rendered with white color instead of bold font weight
- **JS Source**: `src/app.css` lines 303–305
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner strong { font-weight: bold; }` makes bold text heavier. C++ `renderInlineSegments` (lines 237–240) pushes white color `(1,1,1,1)` for bold, which is the same as normal text on a dark background. The visual difference from normal text is imperceptible. ImGui doesn't support inline bold without loading a separate bold font face and switching to it.

- [ ] 123. [markdown-content.cpp] Italic text rendered with dim blue `(0.8, 0.8, 0.9)` instead of italic font style
- **JS Source**: `src/app.css` lines 307–309
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner em { font-style: italic; }` renders text in italic. C++ `renderInlineSegments` (lines 243–246) uses a dimmed blue-white color `(0.8, 0.8, 0.9)` which has no CSS basis. ImGui doesn't support italic text natively, but the color chosen doesn't match any CSS variable.

- [ ] 124. [markdown-content.cpp] h1 header missing bottom separator matching CSS `border-bottom`
- **JS Source**: `src/app.css` lines 249–253
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner h1` has `margin: 0` but standard HTML h1 elements typically render with a bottom margin. C++ adds `ImGui::Separator()` after h1 (line 347) which draws a full-width horizontal line. The JS HTML rendering does not add a separator after h1 — the `<h1>` tag simply has larger text. The `ImGui::Separator()` is not present in the CSS and creates a visual difference.

- [ ] 125. [markdown-content.cpp] Scrollbar thumb uses hardcoded gray colors instead of CSS `var(--border)` and `var(--font-highlight)`
- **JS Source**: `src/app.css` lines 322–346
- **Status**: Pending
- **Details**: CSS `.vscroller > div` uses `background: var(--border)` (#6c757d) default and `background: var(--font-highlight)` (#ffffff) on hover/active. C++ uses hardcoded `IM_COL32(120, 120, 120, 150)` default and `IM_COL32(180, 180, 180, 200)` for active (lines 457–459). Neither color matches the CSS variables. Default should be `BORDER_U32` and active should be `FONT_HIGHLIGHT_U32`.

- [ ] 126. [markdown-content.cpp] Scrollbar thumb missing `border: 1px solid var(--border)` and `border-radius: 5px` from CSS
- **JS Source**: `src/app.css` lines 332–341
- **Status**: Pending
- **Details**: CSS `.vscroller > div` has `border: 1px solid var(--border)` and `border-radius: 5px`. C++ draws the thumb with `AddRectFilled` with 4px rounding (line 461) but no border outline. Should add `AddRect` with `BORDER_U32` and use 5px rounding.

- [ ] 127. [markdown-content.cpp] Scrollbar track `right: 3px` positioning and `opacity: 0.7` not matched
- **JS Source**: `src/app.css` lines 322–330
- **Status**: Pending
- **Details**: CSS `.vscroller { right: 3px; opacity: 0.7; }` positions the scrollbar 3px from the right edge with 70% opacity. C++ positions the scrollbar at `containerSize.x - scrollbar_width - 2.0f` (line 451, using 2px instead of 3px) and uses full opacity for the thumb colors. The alpha values in the thumb colors partially compensate but don't match the 70% opacity overlay.

- [ ] 128. [markdown-content.cpp] `parseInline` processes text before HTML escaping, while JS escapes first then applies regex
- **JS Source**: `src/js/components/markdown-content.js` lines 204–237
- **Status**: Pending
- **Details**: JS `parseInline` calls `this.escapeHtml(text)` first (line 205) before applying regex replacements for bold, italic, code, links, and images. This means the regex patterns match against HTML-escaped text (e.g., `&amp;` instead of `&`). C++ `parseInline` processes raw text directly without escaping. While ImGui doesn't need HTML escaping, if markdown content contains `&`, `<`, or `>` characters, the parsing behavior could differ because the JS regex operates on escaped text while C++ operates on raw text.

- [ ] 129. [markdown-content.cpp] List items use `•` bullet with 16px indent instead of CSS `padding-left: 2em` with disc marker
- **JS Source**: `src/app.css` lines 272–276
- **Status**: Pending
- **Details**: CSS `.markdown-content-inner ul { padding-left: 2em; list-style-type: disc; }` uses standard disc bullets with 2em left padding. At 20px base font, 2em = 40px. C++ uses `ImGui::Indent(16.0f)` with a manual `•` character (lines 369–373). 16px vs 40px indent is a significant visual difference, and the bullet character may render differently than the CSS disc marker.


## Home Tab & Showcase

- [ ] 130. [home-showcase.cpp] Showcase card/video/background layer rendering is not ported
- **JS Source**: `src/js/components/home-showcase.js` lines 15–29, 32–42, 55–57
- **Status**: Pending
- **Details**: JS builds a layered CSS showcase card with optional autoplay video and title overlay. C++ replaces this with plain text/buttons and no equivalent visual composition.

- [ ] 131. [home-showcase.cpp] `background_style` computed output is missing from C++ render path
- **JS Source**: `src/js/components/home-showcase.js` lines 15–29, 55–57
- **Status**: Pending
- **Details**: JS computes `backgroundImage/backgroundSize/backgroundPosition/backgroundRepeat` from base + showcase layers. C++ does not apply these style layers, so showcase visuals diverge.

- [ ] 132. [home-showcase.cpp] Feedback action wiring differs from JS `data-kb-link` behavior
- **JS Source**: `src/js/components/home-showcase.js` lines 39–41
- **Status**: Pending
- **Details**: JS emits a KB-link anchor (`data-kb-link="KB011"`) handled by the app link system. C++ directly calls `ExternalLinks::open("KB011")`, which is not the same contract/path as the original markup-driven behavior.

- [ ] 133. [home-showcase.cpp] `build_background_style()` function not ported — no background image layers
- **JS Source**: `src/js/components/home-showcase.js` lines 15–29
- **Status**: Pending
- **Details**: JS `build_background_style(showcase)` constructs a CSS background-image style from `BASE_LAYERS` + `showcase.layers`, building comma-separated `backgroundImage`, `backgroundSize`, `backgroundPosition`, and `backgroundRepeat` values. C++ (line 127–129) has a comment noting that background image layers have no ImGui equivalent, but the function is entirely missing. The showcase visual identity relies heavily on these layered background images. The C++ version only shows text, losing the visual showcase entirely.

- [ ] 134. [home-showcase.cpp] `BASE_LAYERS` stored as single `ShowcaseLayer` instead of array
- **JS Source**: `src/js/components/home-showcase.js` lines 3–9
- **Status**: Pending
- **Details**: JS `BASE_LAYERS` is an array: `[{ image: './images/logo.png', size: '50px', position: 'bottom 10px right 10px' }]`. C++ (line 22) stores `BASE_LAYER` as a single `ShowcaseLayer` object, not a `std::vector`. While there's only one base layer in the JS source, the JS code uses array spread `[...BASE_LAYERS, ...showcase.layers]` which would support multiple base layers if added. The C++ naming (`BASE_LAYER` singular) and type differ from the JS plural array pattern.

- [ ] 135. [home-showcase.cpp] Video playback (`<video>` element) not implemented
- **JS Source**: `src/js/components/home-showcase.js` template line 34
- **Status**: Pending
- **Details**: JS template: `<video v-if="current.video" :src="current.video" autoplay loop muted playsinline></video>`. This renders an auto-playing, looping, muted video overlay on the showcase. C++ (line 126–129) notes this in a comment but does not implement any video rendering. ImGui does not natively support video playback; a custom texture upload with a video decoder (e.g., via miniaudio for audio, FFmpeg for video) would be needed.

- [ ] 136. [home-showcase.cpp] `computed: background_style()` not ported
- **JS Source**: `src/js/components/home-showcase.js` lines 55–57
- **Status**: Pending
- **Details**: JS computed property `background_style()` calls `build_background_style(this.current)` and the result is bound to the `<a>` element's `:style`. This creates the visual showcase appearance with layered background images. C++ does not compute or apply any background styling. The showcase area appears as plain text instead of an image showcase.

- [ ] 137. [home-showcase.cpp] Title text says "Made with wow.export.cpp" but JS says "Made with wow.export"
- **JS Source**: `src/js/components/home-showcase.js` template line 33
- **Status**: Pending
- **Details**: JS template: `<h1 id="home-showcase-header">Made with wow.export</h1>`. C++ (line 107): `ImGui::Text("Made with wow.export.cpp")`. Per the project conventions in copilot-instructions.md, user-facing text should say `wow.export.cpp`, so this is intentionally different. However, the JS source file says `wow.export`, so this is a documented deviation from the JS source.

- [ ] 138. [home-showcase.cpp] Showcase rendering is plain text instead of styled grid layout
- **JS Source**: `src/app.css` lines 3580–3633
- **Status**: Pending
- **Details**: CSS `#home-showcase-header` is in a grid layout (`grid-column: 1; grid-row: 1`), `#home-showcase` has `border: 1px solid var(--border); border-radius: 10px; cursor: pointer; overflow: hidden`, and `#home-showcase-links > a` has `font-size: 12px; color: #888`. C++ uses `ImGui::Text()`, `ImGui::SmallButton()`, and `ImGui::Separator()` without any grid layout, border-radius, or matching font sizes. The visual appearance is completely different from the styled JS version.

- [ ] 139. [home-showcase.cpp] Feedback link opens "KB011" directly instead of resolving via `data-kb-link` attribute
- **JS Source**: `src/js/components/home-showcase.js` template line 41
- **Status**: Pending
- **Details**: JS template: `<a data-kb-link="KB011">Feedback</a>`. The `data-kb-link` attribute is processed by the app's global link handler which resolves KB article IDs to URLs. C++ (line 142) calls `ExternalLinks::open("KB011")` directly with the raw string. If `ExternalLinks::open()` doesn't resolve KB article IDs (and only handles URLs), this would fail to open the correct link. The JS version relies on a global click handler that intercepts `data-kb-link` attributes.

- [ ] 140. [home-showcase.cpp] `.showcase-title` CSS uses Gambler font at 40px — not replicated
- **JS Source**: `src/app.css` lines 3607–3614
- **Status**: Pending
- **Details**: CSS `.showcase-title` uses `font-family: "Gambler", sans-serif; font-size: 40px; color: white` positioned absolutely at `top: 15px; left: 20px`. C++ (line 123–124) uses `ImGui::Text("%s", current->title.c_str())` with default ImGui font — no custom font, no large size, no absolute positioning. The showcase title should be a large overlay text in a specific font.

- [ ] 141. [map-viewer.cpp] Tile image drawing path is still unimplemented
- **JS Source**: `src/js/components/map-viewer.js` lines 380–402, 1111–1113
- **Status**: Pending
- **Details**: JS draws loaded tiles to canvas via `putImageData(...)` on main/double-buffer contexts. C++ caches tile pixels but does not upload/draw them, so only overlays render and map tiles are not visually equivalent.

- [x] 142. [map-viewer.cpp] Tile loading flow is synchronous instead of JS Promise-based async queueing
- **JS Source**: `src/js/components/map-viewer.js` lines 192–197, 380–414
- **Status**: Pending
- **Details**: JS tile loader is async (`loader(...).then(...)`) with Promise completion timing. C++ calls loader synchronously in `loadTile(...)`, changing queue timing and behavior during panning/zoom updates.

- [ ] 143. [map-viewer.cpp] Box-select-mode active color uses NAV_SELECTED (#22B549) instead of CSS #5fdb65
- **JS Source**: `src/app.css` line 1348–1349
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .info span.active` uses `color: #5fdb65` (RGB 95, 219, 101). C++ line 1178 uses `app::theme::NAV_SELECTED` which maps to `BUTTON_BASE` = `(0.133, 0.710, 0.286)` = RGB(34, 181, 73). The active highlight color is noticeably different — should be a dedicated color constant matching #5fdb65.

- [ ] 144. [map-viewer.cpp] Map-viewer info bar text lacks CSS `text-shadow: black 0 0 6px`
- **JS Source**: `src/app.css` lines 1326–1328
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer div { text-shadow: black 0 0 6px; }` applies a text shadow to all divs inside the map viewer, including the info bar and hover-info. C++ renders these with plain `ImGui::TextUnformatted` (lines 1166–1189) with no text shadow effect. ImGui does not natively support text shadows, so a manual shadow would need to be rendered (draw text offset in black, then draw normal text on top).

- [ ] 145. [map-viewer.cpp] Map-viewer info bar spans missing CSS `margin: 0 10px` horizontal spacing
- **JS Source**: `src/app.css` lines 1345–1346
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .info span { margin: 0 10px; }` gives each info label 10px left/right margin. C++ uses `ImGui::SameLine()` between items (lines 1167–1175) with default spacing. The default ImGui item spacing is typically ~8px which may not exactly match the 20px total gap (10px + 10px) between spans.

- [ ] 146. [map-viewer.cpp] Map-viewer checkerboard background pattern not implemented
- **JS Source**: `src/app.css` lines 1300–1306
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer` has a complex checkerboard background using `background-image: linear-gradient(45deg, ...)` with `--trans-check-a` and `--trans-check-b` colors, `background-size: 30px 30px`, and `background-position: 0 0, 15px 15px`. C++ `renderWidget` (line 1148) uses `ImGui::BeginChild` with no custom background drawing — the checkerboard transparency pattern is missing entirely. The JS version shows a checkerboard behind transparent map tiles.

- [ ] 147. [map-viewer.cpp] Map-viewer hover-info positioned at top via ImGui layout instead of CSS `bottom: 3px; left: 3px`
- **JS Source**: `src/app.css` lines 1330–1333
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer .hover-info` is positioned `bottom: 3px; left: 3px` (bottom-left corner of the map viewer). C++ renders hover-info at line 1187–1189 via `ImGui::SameLine()` after the info bar spans, placing it inline at the top. It should be rendered at the bottom-left of the map viewer container using `ImGui::SetCursorPos` or overlay drawing.

- [ ] 148. [map-viewer.cpp] Box-select-mode cursor not changed to crosshair
- **JS Source**: `src/app.css` lines 1351–1353
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer.box-select-mode { cursor: crosshair; }` changes the cursor to a crosshair when box-select mode is active. C++ does not call `ImGui::SetMouseCursor(ImGuiMouseCursor_Hand)` or any cursor change when `state.isBoxSelectMode` is true. ImGui does not have a native crosshair cursor, but this should at least document the visual difference.

- [ ] 149. [map-viewer.cpp] Tile rendering to canvas via GL textures not implemented — tiles cached in memory but not displayed
- **JS Source**: `src/js/components/map-viewer.js` lines 350–500 (loadTile, renderWithDoubleBuffer)
- **Status**: Pending
- **Details**: The JS version draws tiles to a `<canvas>` via `context.putImageData()` and `context.drawImage()`. C++ caches tile pixel data in `s_state.tilePixelCache` (map-viewer.cpp line 81) but never uploads it as OpenGL textures or renders it to the screen. The TODO comment at line 1194–1198 acknowledges this. The map overlay (selection highlights, hover) draws over empty space. This is a critical functional gap — the map tiles are invisible.

- [ ] 150. [map-viewer.cpp] `handleTileInteraction` emits selection changes via mutable reference instead of `$emit('update:selection')`
- **JS Source**: `src/js/components/map-viewer.js` lines 846–874
- **Status**: Pending
- **Details**: JS `handleTileInteraction` modifies `this.selection` array directly (splice/push) and Vue reactivity propagates changes via `v-model`. C++ modifies the `selection` vector reference directly (lines 845–848). However, JS Select All (line 812) uses `this.$emit('update:selection', newSelection)` with a new array — the C++ equivalent calls `onSelectionChanged(newSelection)` callback in `handleKeyPress` (line 780) and `finalizeBoxSelection` (line 941). This means `handleTileInteraction` mutates in-place but Select All and box-select create new arrays — potential inconsistency in selection update patterns.

- [ ] 151. [map-viewer.cpp] Map margin `20px 20px 0 10px` from CSS not applied
- **JS Source**: `src/app.css` line 1307
- **Status**: Pending
- **Details**: CSS `.ui-map-viewer { margin: 20px 20px 0 10px; }` adds specific margins around the map viewer. C++ `renderWidget` uses `ImGui::GetContentRegionAvail()` for sizing (line 1144) without adding any padding/margin equivalent. The parent layout is responsible for this in ImGui, but if not handled there, the map viewer will be flush against adjacent elements.

- [ ] 152. [tab_home.cpp] Home showcase content is replaced with custom nav-card UI instead of the JS `HomeShowcase` component
- **JS Source**: `src/js/modules/tab_home.js` lines 4–5
- **Status**: Pending
- **Details**: JS renders `<HomeShowcase />`; C++ replaces that section with a custom navigation-card grid (`renderNavCard`/`renderHomeLayout`), changing the original home-tab content and visuals.

- [ ] 153. [tab_home.cpp] `whatsNewHTML` is rendered as plain text instead of HTML content
- **JS Source**: `src/js/modules/tab_home.js` line 6
- **Status**: Pending
- **Details**: JS uses `v-html="$core.view.whatsNewHTML"` to render formatted markup; C++ calls `ImGui::TextWrapped` on the raw string, so HTML formatting/links are not rendered.

- [ ] 154. [tab_home.cpp] "wow.export vX.X.X" title text is an invention not in original JS
- **JS Source**: `src/js/modules/tab_home.js` lines 2–23
- **Status**: Pending
- **Details**: C++ renders a large "wow.export vX.X.X" title in Row 1, Column 1. The original JS template has no such title. Row 1 of the original grid is the HomeShowcase header reading "Made with wow.export" (from home-showcase.js).

- [ ] 155. [tab_home.cpp] HomeShowcase component entirely replaced with navigation cards
- **JS Source**: `src/js/modules/tab_home.js` line 4
- **Status**: Pending
- **Details**: The JS HomeShowcase component loads showcase.json, displays multi-layered background images, optional video with autoplay, clickable links, a Refresh link, and a Feedback link. The C++ replaces ALL of this with a grid of navigation card shortcuts. This is a massive visual and functional deviation.

- [ ] 156. [tab_home.cpp] URLs hardcoded instead of using external-links system
- **JS Source**: `src/js/modules/tab_home.js` lines 9–19
- **Status**: Pending
- **Details**: JS uses data-external="::DISCORD", "::GITHUB", "::PATREON" which resolves through ExternalLinks. C++ hardcodes URLs directly as static constexpr strings instead of using external-links.h lookup table.

- [ ] 157. [tab_home.cpp] whatsNewHTML rendered as plain text instead of HTML
- **JS Source**: `src/js/modules/tab_home.js` line 6
- **Status**: Pending
- **Details**: JS uses v-html="$core.view.whatsNewHTML" which renders actual HTML markup (headings, paragraphs, bold, etc.). C++ uses ImGui::TextWrapped() which renders plain text only. HTML tags appear as raw text, not formatted content. CSS container-query responsive sizing is also absent.

- [ ] 158. [tab_home.cpp] #home-changes vertical padding differs from CSS
- **JS Source**: `src/js/modules/tab_home.js` lines 5–7
- **Status**: Pending
- **Details**: CSS specifies padding: 0 50px (zero vertical, 50px horizontal). C++ uses contentPadY=20.0f, adding 20px vertical padding not present in the original. CSS justify-content: center vertical centering is also missing.

- [ ] 159. [tab_home.cpp] Background image "cover" mode not implemented correctly
- **JS Source**: `src/js/modules/tab_home.js` line 5
- **Status**: Pending
- **Details**: CSS uses background center/cover which maintains aspect ratio while filling container, cropping overflow. C++ uses AddImageRounded with UV (0,0)-(1,1) which stretches the image to fill, distorting aspect ratio instead of cropping.

- [ ] 160. [tab_home.cpp] Help button icon size, position, and rotation differ from CSS
- **JS Source**: `src/js/modules/tab_home.js` lines 9–20
- **Status**: Pending
- **Details**: CSS ::before uses width/height: 120px, right: -20px, transform: rotate(20deg), opacity: 0.2. C++ uses iconSize=80px (should be 120), positions 10px from right edge (should extend 20px beyond), and has no rotation.

- [ ] 161. [tab_home.cpp] Missing hover transition animations on help buttons
- **JS Source**: `src/js/modules/tab_home.js` lines 8–21
- **Status**: Pending
- **Details**: CSS specifies transition: all 0.3s ease on help button divs and transition: transform 0.3s ease on ::before icon with hover scale(1.1). C++ has no transition/animation effects — changes are instantaneous.

- [ ] 162. [tab_home.cpp] Help button subtitle uses FONT_FADED instead of inheriting parent color
- **JS Source**: `src/js/modules/tab_home.js` lines 10–19
- **Status**: Pending
- **Details**: In JS, both <b> and <span> children inherit the same parent text color. On hover, both change to --nav-option-selected. C++ uses FONT_FADED for the subtitle, making it dimmer than the title.

- [ ] 163. [tab_home.cpp] #home-help-buttons grid-column full-width not fully replicated
- **JS Source**: `src/js/modules/tab_home.js` lines 8–21
- **Status**: Pending
- **Details**: CSS places help buttons spanning full width with grid-column: 1 / -1 and centers them with justify-content/align-items. C++ manually calculates centering with fixed card width of 940px. If the window is narrower, buttons push off-screen rather than wrapping.

- [ ] 164. [tab_home.cpp] #home-changes container query responsive font sizes not implemented
- **JS Source**: `src/js/modules/tab_home.js` line 6
- **Status**: Pending
- **Details**: CSS defines container-type: size with container query units: h1 font-size: clamp(16px, 4cqi, 28px) and p font-size: clamp(12px, 3cqi, 20px). C++ does not implement any responsive font sizing.

- [ ] 165. [tab_home.cpp] openInExplorer used for URL opening instead of external-links open
- **JS Source**: `src/js/modules/tab_home.js` lines 9–17
- **Status**: Pending
- **Details**: JS uses ExternalLinks.open() via data-external which calls nw.Shell.openExternal(). C++ calls core::openInExplorer(url). openInExplorer is typically for file paths/folders, while JS uses openExternal for URLs.

- [ ] 166. [tab_home.cpp] No cleanup/destruction of OpenGL textures
- **JS Source**: `src/js/modules/tab_home.js` lines N/A
- **Status**: Pending
- **Details**: C++ allocates OpenGL textures with glGenTextures and loadSvgTexture but has no cleanup/shutdown function to call glDeleteTextures. Textures leak if the tab is re-initialized or the application shuts down.

- [ ] 167. [legacy_tab_home.cpp] Legacy home tab template is replaced by shared `tab_home` layout
- **JS Source**: `src/js/modules/legacy_tab_home.js` lines 2–23
- **Status**: Pending
- **Details**: JS defines a dedicated legacy-home template structure (`#legacy-tab-home`, changelog HTML block, and external-link button rows), while C++ delegates to `tab_home::renderHomeLayout()`, so the legacy tab is not a line-by-line equivalent render path.

- [ ] 168. [legacy_tab_home.cpp] External link help buttons (Discord, GitHub, Patreon) not rendered
- **JS Source**: `src/js/modules/legacy_tab_home.js` lines 8–21
- **Status**: Pending
- **Details**: JS template includes 3 help buttons: Discord ("Stuck? Need Help?"), GitHub ("Gnomish Heritage?"), and Patreon ("Support Us!"), each with `data-external` links and description text. C++ `render()` delegates entirely to `tab_home::renderHomeLayout()` (line 28) and does not render these help buttons. Unless `renderHomeLayout()` includes them, these interactive elements are missing from the legacy home tab.


## Tab: Changelog, Help, Blender & Install

- [ ] 169. [tab_changelog.cpp] Changelog path resolution logic differs from JS two-path contract
- **JS Source**: `src/js/modules/tab_changelog.js` lines 14–16
- **Status**: Pending
- **Details**: JS uses `BUILD_RELEASE ? './src/CHANGELOG.md' : '../../CHANGELOG.md'`; C++ adds a third fallback (`CHANGELOG.md`) and different path probing order, changing source resolution behavior.

- [ ] 170. [tab_changelog.cpp] Changelog screen typography/layout diverges from JS `#changelog` template styling
- **JS Source**: `src/js/modules/tab_changelog.js` lines 31–35
- **Status**: Pending
- **Details**: JS uses dedicated `#changelog`/`#changelog-text` template structure and CSS styling; C++ renders plain ImGui title/separator/button layout, causing visible UI differences.

- [ ] 171. [tab_changelog.cpp] Heading rendered as plain ImGui::Text instead of styled h1
- **JS Source**: `src/js/modules/tab_changelog.js` line 32
- **Status**: Pending
- **Details**: JS uses h1 tag which renders as a large bold heading per CSS. C++ uses ImGui::Text with default font size and weight.

- [ ] 172. [tab_help.cpp] Search filtering no longer uses JS 300ms debounce behavior
- **JS Source**: `src/js/modules/tab_help.js` lines 145–149, 153–157
- **Status**: Pending
- **Details**: JS applies article filtering via `setTimeout(..., 300)` debounce on `search_query`; C++ filters immediately on each input change, changing responsiveness and update timing.

- [ ] 173. [tab_help.cpp] Help article list presentation differs from JS title/tag/KB layout
- **JS Source**: `src/js/modules/tab_help.js` lines 115–121
- **Status**: Pending
- **Details**: JS renders per-item title and a separate tags row with KB badge styling; C++ combines content into selectable labels and tooltip tags, so article list visuals/structure are not identical.

- [ ] 174. [tab_help.cpp] Missing 300ms debounce on search filter
- **JS Source**: `src/js/modules/tab_help.js` lines 145–149
- **Status**: Pending
- **Details**: JS implements a 300ms setTimeout debounce before filtering. C++ calls update_filter() immediately on every keystroke.

- [ ] 175. [tab_help.cpp] Article list layout differs tags shown in tooltip instead of inline
- **JS Source**: `src/js/modules/tab_help.js` lines 115–121
- **Status**: Pending
- **Details**: JS renders each article with visible title and tags divs inline. C++ renders KB_ID and title as a single Selectable with tags only in hover tooltip. Tags and KB ID badge are not visually inline.

- [x] 176. [tab_help.cpp] load_help_docs is synchronous instead of async
- **JS Source**: `src/js/modules/tab_help.js` line 8
- **Status**: Pending
- **Details**: JS load_help_docs is async. C++ is synchronous blocking the main thread during file reads.

- [ ] 177. [tab_blender.cpp] Blender version gating semantics differ from JS string comparison behavior
- **JS Source**: `src/js/modules/tab_blender.js` lines 91, 139
- **Status**: Pending
- **Details**: JS compares versions as strings (`version >= MIN_VER`, `blender_version < MIN_VER`), while C++ parses with `std::stod` and compares numerically, changing edge-case ordering behavior.

- [ ] 178. [tab_blender.cpp] Blender install screen layout is not a pixel-equivalent port of JS markup
- **JS Source**: `src/js/modules/tab_blender.js` lines 59–69
- **Status**: Pending
- **Details**: JS uses structured `#blender-info`/`#blender-info-buttons` markup with CSS-defined spacing/styling; C++ replaces it with simple ImGui text/separator/buttons, producing visual/layout mismatch.

- [ ] 179. [tab_blender.cpp] get_blender_installations uses regex_match instead of regex_search
- **JS Source**: `src/js/modules/tab_blender.js` line 39
- **Status**: Pending
- **Details**: JS match() performs a search anywhere in string. C++ uses std::regex_match which requires the ENTIRE string to match. Directory names like blender-2.83 would match in JS but fail in C++.

- [ ] 180. [tab_blender.cpp] Blender version comparison uses numeric instead of string comparison
- **JS Source**: `src/js/modules/tab_blender.js` lines 91, 139
- **Status**: Pending
- **Details**: JS compares version strings lexicographically. C++ converts to double via std::stod() and compares numerically. Versions like 2.80 vs 2.8 would compare differently.

- [x] 181. [tab_blender.cpp] start_automatic_install and checkLocalVersion are synchronous
- **JS Source**: `src/js/modules/tab_blender.js` lines 81, 127
- **Status**: Pending
- **Details**: Both JS functions are async with await. C++ implementations are synchronous, blocking the render thread.

- [ ] 182. [tab_install.cpp] Install listbox copy/paste options are hardcoded instead of using JS config-driven behavior
- **JS Source**: `src/js/modules/tab_install.js` lines 165, 184
- **Status**: Pending
- **Details**: JS listbox wiring uses `$core.view.config.copyMode`, `pasteSelection`, and `removePathSpacesCopy`; C++ passes `CopyMode::Default` with `pasteselection=false` and `copytrimwhitespace=false`, changing list interaction behavior.

- [ ] 183. [tab_install.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS renders `Regex Enabled` with `:title="$core.view.regexTooltip"`; C++ renders plain text without the tooltip contract, changing UI affordance.

- [x] 184. [tab_install.cpp] Async operations converted to synchronous calls
- **JS Source**: `src/js/modules/tab_install.js` lines 52–146
- **Status**: Pending
- **Details**: JS export_install_files(), view_strings(), and export_strings() are all async functions that await CASC file I/O. C++ versions are fully synchronous, blocking the UI thread.

- [ ] 185. [tab_install.cpp] CASC getFile replaced with low-level two-step call, losing BLTE decoding
- **JS Source**: `src/js/modules/tab_install.js` lines 73–74
- **Status**: Pending
- **Details**: JS calls core.view.casc.getFile() which returns a BLTEReader for BLTE block decompression. C++ calls getEncodingKeyForContentKey then _ensureFileInCache then BufferWrapper::readFile, skipping BLTE decompression entirely. Exported files may be corrupt.

- [ ] 186. [tab_install.cpp] processAllBlocks() call missing in view_strings_impl
- **JS Source**: `src/js/modules/tab_install.js` lines 103–105
- **Status**: Pending
- **Details**: JS calls data.processAllBlocks() after getFile() to force all BLTE blocks decompressed before extract_strings. C++ skips this step because it uses plain BufferWrapper instead of BLTEReader.

- [ ] 187. [tab_install.cpp] export_install_files missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_install.js` line 78
- **Status**: Pending
- **Details**: JS calls helper.mark(file_name, false, e.message, e.stack) passing both message and stack trace. C++ passes only e.what(), omitting the stack trace parameter.

- [ ] 188. [tab_install.cpp] First listbox missing copyMode from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode" to the main install Listbox. C++ hardcodes listbox::CopyMode::Default instead of reading from view.config.

- [ ] 189. [tab_install.cpp] First listbox missing pasteSelection from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :pasteselection="$core.view.config.pasteSelection". C++ hardcodes false instead of reading from view.config.

- [ ] 190. [tab_install.cpp] First listbox missing copytrimwhitespace from config
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS passes :copytrimwhitespace="$core.view.config.removePathSpacesCopy". C++ hardcodes false, disabling the remove-path-spaces-on-copy feature.

- [ ] 191. [tab_install.cpp] Second listbox (strings) missing copyMode from config
- **JS Source**: `src/js/modules/tab_install.js` line 184
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode" to the strings Listbox. C++ hardcodes listbox::CopyMode::Default.

- [ ] 192. [tab_install.cpp] First listbox unittype should be "install file" not "file"
- **JS Source**: `src/js/modules/tab_install.js` line 165
- **Status**: Pending
- **Details**: JS template has unittype="install file". C++ passes "file". The status bar will show "X files found" instead of "X install files found".

- [ ] 193. [tab_install.cpp] Strings listbox nocopy incorrectly set to true
- **JS Source**: `src/js/modules/tab_install.js` line 184
- **Status**: Pending
- **Details**: The JS strings Listbox does NOT pass :nocopy (defaults to false). C++ passes true for nocopy, incorrectly disabling copy functionality in the strings list view.

- [ ] 194. [tab_install.cpp] Strings sidebar missing CSS styling equivalents
- **JS Source**: `src/js/modules/tab_install.js` lines 194–197
- **Status**: Pending
- **Details**: CSS defines .strings-header font-size 14px opacity 0.7, .strings-filename font-size 12px word-break: break-all, 5px gap. C++ uses default font sizes and ImGui::Spacing() which may not match.

- [ ] 195. [tab_install.cpp] Input placeholder text not rendered
- **JS Source**: `src/js/modules/tab_install.js` lines 170, 189
- **Status**: Pending
- **Details**: JS filter inputs have placeholder="Filter install files..." and "Filter strings...". C++ uses plain ImGui::InputText without hint/placeholder.

- [ ] 196. [tab_install.cpp] Regex tooltip not rendered
- **JS Source**: `src/js/modules/tab_install.js` lines 169, 188
- **Status**: Pending
- **Details**: JS "Regex Enabled" div has :title="$core.view.regexTooltip" showing tooltip on hover. C++ has no tooltip implementation.

- [ ] 197. [tab_install.cpp] extract_strings and update_install_listfile exposed in header but should be file-local
- **JS Source**: `src/js/modules/tab_install.js` lines 16, 41
- **Status**: Pending
- **Details**: In JS, extract_strings and update_install_listfile are file-local (not exported). C++ header exposes them publicly. They should be static in the .cpp and removed from the header.


## Tab: Models

- [ ] 198. [tab_models.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_models.js` line 296
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 199. [tab_models.cpp] Missing "View Log" button in generic error toast
- **JS Source**: `src/js/modules/tab_models.js` line 163
- **Status**: Pending
- **Details**: JS passes { 'View Log': () => log.openRuntimeLog() } as toast buttons on preview failure. C++ passes empty {}, so user has no way to open the runtime log from error toast.

- [ ] 200. [tab_models.cpp] Drop handler prompt lambda missing count parameter
- **JS Source**: `src/js/modules/tab_models.js` line 580
- **Status**: Pending
- **Details**: JS prompt receives count parameter: count => util.format('Export %d models as %s', count, ...). C++ lambda returns string without using/accepting a count, so prompt won't show number of files.

- [ ] 201. [tab_models.cpp] helper.mark on failure missing stack trace parameter
- **JS Source**: `src/js/modules/tab_models.js` line 269
- **Status**: Pending
- **Details**: JS calls helper.mark(file_name, false, e.message, e.stack) with 4 args. C++ calls helper.mark(file_name, false, e.what()) with only 3, losing stack trace.

- [ ] 202. [tab_models_legacy.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 340
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 203. [tab_models_legacy.cpp] WMOLegacyRendererGL constructor passes 0 instead of file_name
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 86
- **Status**: Pending
- **Details**: JS passes (data, file_name, gl_context, showTextures). C++ passes (data, 0, *gl_ctx, showTextures). WMO renderer needs file_name for group file path resolution. Passing 0 is incorrect.

- [ ] 204. [tab_models_legacy.cpp] Missing "View Log" button in preview_model error toast
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 185
- **Status**: Pending
- **Details**: JS provides { 'View Log': () => log.openRuntimeLog() } as toast action. C++ passes empty {}, losing the user-facing button.

- [ ] 205. [tab_models_legacy.cpp] Missing requestAnimationFrame deferral for fitCamera
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 182
- **Status**: Pending
- **Details**: JS wraps fitCamera in requestAnimationFrame() for next-frame deferral. C++ calls fitCamera() synchronously, which may execute before render state is fully set up.

- [ ] 206. [tab_models_legacy.cpp] PNG/CLIPBOARD export_paths stream not written for PNG exports
- **JS Source**: `src/js/modules/tab_models_legacy.js` lines 197–224
- **Status**: Pending
- **Details**: JS writes 'PNG:' + out_file to export_paths stream. C++ delegates to model_viewer_utils::export_preview which opens its own FileWriter. The export_files-level stream gets no entries for PNG exports.

- [ ] 207. [tab_models_legacy.cpp] helper.mark on failure missing stack trace argument
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 311
- **Status**: Pending
- **Details**: JS calls helper.mark(file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 208. [tab_models_legacy.cpp] Listbox missing quickfilters from view
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 332
- **Status**: Pending
- **Details**: JS passes :quickfilters="$core.view.legacyModelQuickFilters" (which is ['m2','mdx','wmo']). C++ passes empty {}. Quick filter buttons won't appear.

- [ ] 209. [tab_models_legacy.cpp] Listbox missing copyMode config binding
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 332
- **Status**: Pending
- **Details**: JS passes :copymode="$core.view.config.copyMode". C++ hardcodes listbox::CopyMode::Default.

- [ ] 210. [tab_models_legacy.cpp] Listbox missing pasteSelection config binding
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 332
- **Status**: Pending
- **Details**: JS passes :pasteselection="$core.view.config.pasteSelection". C++ hardcodes false.

- [ ] 211. [tab_models_legacy.cpp] Listbox missing copytrimwhitespace config binding
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 332
- **Status**: Pending
- **Details**: JS passes :copytrimwhitespace="$core.view.config.removePathSpacesCopy". C++ hardcodes false.

- [ ] 212. [tab_models_legacy.cpp] Missing "Regex Enabled" indicator in filter bar
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 340
- **Status**: Pending
- **Details**: JS shows regex-info div with tooltip when config.regexFilters is true. C++ filter bar only renders text input, no regex indicator.

- [ ] 213. [tab_models_legacy.cpp] Filter input missing placeholder text
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 341
- **Status**: Pending
- **Details**: JS uses placeholder="Filter models...". C++ ImGui::InputText has no hint text. Should use InputTextWithHint.

- [ ] 214. [tab_models_legacy.cpp] All sidebar checkboxes missing tooltip text
- **JS Source**: `src/js/modules/tab_models_legacy.js` lines 377–399
- **Status**: Pending
- **Details**: JS has title="..." on every sidebar checkbox (6 items). C++ renders none of these tooltips.

- [ ] 215. [tab_models_legacy.cpp] step/seek/start_scrub/end_scrub only handle M2, not MDX
- **JS Source**: `src/js/modules/tab_models_legacy.js` lines 462–496
- **Status**: Pending
- **Details**: JS uses optional chaining on single active_renderer for animation methods. C++ only checks active_renderer_m2 — MDX renderer is ignored for all animation control operations.

- [ ] 216. [tab_models_legacy.cpp] WMO Groups rendered with raw Checkbox instead of Checkboxlist component
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 414
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for WMO groups. C++ uses manual loop of ImGui::Checkbox. Checkboxlist may have additional styling/behavior.

- [ ] 217. [tab_models_legacy.cpp] Doodad Sets rendered with raw Checkbox instead of Checkboxlist component
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 419
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for doodad sets. C++ uses raw ImGui::Checkbox loop.

- [ ] 218. [tab_models_legacy.cpp] getActiveRenderer() only returns M2, not active renderer
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 592
- **Status**: Pending
- **Details**: JS returns active_renderer which could be M2, WMO, or MDX. C++ always returns active_renderer_m2.get(), returning nullptr when active model is WMO or MDX.

- [x] 219. [tab_models_legacy.cpp] preview_model and export_files are synchronous instead of async
- **JS Source**: `src/js/modules/tab_models_legacy.js` lines 42, 191
- **Status**: Pending
- **Details**: JS preview_model and export_files are async with await. C++ versions are fully synchronous, blocking UI thread for expensive operations.

- [ ] 220. [tab_models_legacy.cpp] MenuButton missing "upward" class/direction
- **JS Source**: `src/js/modules/tab_models_legacy.js` line 373
- **Status**: Pending
- **Details**: JS uses class="upward" on MenuButton so dropdown opens upward. C++ menu_button::render doesn't pass upward/direction flag.


## Tab: Textures

- [ ] 221. [tab_textures.cpp] Baked NPC texture apply path stores a file data ID instead of the JS BLP object
- **JS Source**: `src/js/modules/tab_textures.js` lines 423–427
- **Status**: Pending
- **Details**: JS loads the selected texture file and stores a `BLPFile` instance in `chrCustBakedNPCTexture`; C++ stores only the resolved file data ID, changing downstream data shape/behavior.

- [ ] 222. [tab_textures.cpp] Baked NPC texture failure toast omits JS `view log` action callback
- **JS Source**: `src/js/modules/tab_textures.js` lines 430–431
- **Status**: Pending
- **Details**: JS error toast includes `{ 'view log': () => log.openRuntimeLog() }`; C++ error toast has no action handlers, removing the original troubleshooting entry point.

- [ ] 223. [tab_textures.cpp] Texture channel controls are rendered as checkboxes instead of JS channel chips
- **JS Source**: `src/js/modules/tab_textures.js` lines 306–311
- **Status**: Pending
- **Details**: JS uses styled `li` channel chips (`R/G/B/A`) with selected-state classes; C++ renders standard ImGui checkboxes, causing visible control-style differences.

- [ ] 224. [tab_textures.cpp] Listbox override texture list not forwarded
- **JS Source**: `src/js/modules/tab_textures.js` line 291
- **Status**: Pending
- **Details**: JS passes :override="$core.view.overrideTextureList" to Listbox. C++ passes nullptr for overrideItems. When another tab sets an override texture list, the listbox will ignore it entirely.

- [ ] 225. [tab_textures.cpp] MenuButton replaced with plain Button — no format dropdown
- **JS Source**: `src/js/modules/tab_textures.js` line 328
- **Status**: Pending
- **Details**: JS uses MenuButton component with :options providing dropdown to change export format (PNG/WEBP/BLP). C++ uses plain ImGui::Button showing only current format — no way to change export texture format from this tab's UI.

- [ ] 226. [tab_textures.cpp] apply_baked_npc_texture skips CASC file load and BLP creation
- **JS Source**: `src/js/modules/tab_textures.js` lines 421–426
- **Status**: Pending
- **Details**: JS loads file from CASC, creates BLPFile, stores BLP object in chrCustBakedNPCTexture. C++ just stores the raw file data ID integer without loading or decoding. Downstream consumers expecting decoded BLP will receive only an integer.

- [ ] 227. [tab_textures.cpp] Missing "View Log" action button on baked NPC texture error toast
- **JS Source**: `src/js/modules/tab_textures.js` line 430
- **Status**: Pending
- **Details**: JS error toast passes { 'view log': () => log.openRuntimeLog() }. C++ passes empty {}.

- [ ] 228. [tab_textures.cpp] Atlas overlay regions not cleared when atlas_id found but entry missing
- **JS Source**: `src/js/modules/tab_textures.js` lines 184–213
- **Status**: Pending
- **Details**: JS always assigns textureAtlasOverlayRegions = render_regions unconditionally after the if(entry) block. C++ doesn't clear regions when texture_atlas_map has file_data_id but texture_atlas_entries lacks atlas_id, leaving stale data.

- [ ] 229. [tab_textures.cpp] Drop handler prompt omits file count
- **JS Source**: `src/js/modules/tab_textures.js` line 468
- **Status**: Pending
- **Details**: JS prompt includes count parameter: count => util.format('Export %d textures as %s', count, ...). C++ lambda takes no count and produces "Export textures as PNG" without count.

- [ ] 230. [tab_textures.cpp] Atlas region tooltip positioning not implemented
- **JS Source**: `src/js/modules/tab_textures.js` lines 148–182
- **Status**: Pending
- **Details**: JS attach_overlay_listener adds mousemove handler for dynamic tooltip repositioning based on mouse position (4 CSS tooltip classes). C++ draws region name text at fixed position with no hover interaction.

- [ ] 231. [tab_textures.cpp] Filter input missing placeholder text
- **JS Source**: `src/js/modules/tab_textures.js` line 302
- **Status**: Pending
- **Details**: JS uses placeholder="Filter textures...". C++ uses ImGui::InputText with no hint text.

- [ ] 232. [tab_textures.cpp] Regex tooltip text missing
- **JS Source**: `src/js/modules/tab_textures.js` line 301
- **Status**: Pending
- **Details**: JS shows :title="$core.view.regexTooltip" on "Regex Enabled" div. C++ has no tooltip.

- [ ] 233. [tab_textures.cpp] export_texture_atlas_regions missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_textures.js` line 261
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [x] 234. [tab_textures.cpp] All async operations are synchronous — blocks UI thread
- **JS Source**: `src/js/modules/tab_textures.js` lines 23–413
- **Status**: Pending
- **Details**: JS preview_texture_by_id, load_texture_atlas_data, reload_texture_atlas_data, export_texture_atlas_regions, export_textures, initialize, and apply_baked_npc_texture are all async. C++ equivalents all run synchronously on UI thread.

- [ ] 235. [tab_textures.cpp] Channel mask toggles rendered as checkboxes instead of styled inline buttons
- **JS Source**: `src/js/modules/tab_textures.js` lines 306–311
- **Status**: Pending
- **Details**: JS renders channel toggles as <li> items with colored backgrounds/borders using .selected CSS class. C++ uses ImGui::Checkbox with colored check marks — visually different from original compact colored square buttons.

- [ ] 236. [tab_textures.cpp] Preview image max dimensions not clamped to texture dimensions
- **JS Source**: `src/js/modules/tab_textures.js` line 312
- **Status**: Pending
- **Details**: JS applies max-width/max-height from texture dimensions so preview never upscales beyond native resolution. C++ computes scale = min(avail/tex) which upscales small textures to fill the panel.

- [ ] 237. [legacy_tab_textures.cpp] Listbox context menu render path from JS template is missing
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 118–122, 147–161
- **Status**: Pending
- **Details**: JS mounts a `ContextMenu` component for texture list selections, but C++ never calls `context_menu::render(...)` in `render()`, leaving the expected right-click action menu unrendered.

- [ ] 238. [legacy_tab_textures.cpp] Channel toggle visuals/interaction differ from JS channel list UI
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 130–135
- **Status**: Pending
- **Details**: JS uses styled `<li>` channel pills with selected classes (`R/G/B/A`), while C++ uses standard ImGui checkboxes, producing non-identical visuals and interaction behavior.

- [ ] 239. [legacy_tab_textures.cpp] PNG/JPG preview info shows lowercase extension vs JS uppercase
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 58
- **Status**: Pending
- **Details**: JS formats PNG/JPG info as `${ext.slice(1).toUpperCase()}` (e.g., "256x256 (PNG)"). C++ uses `ext.substr(1)` without uppercasing (line 132), producing "256x256 (png)". Minor visual inconsistency in the preview info text.

- [ ] 240. [legacy_tab_textures.cpp] Listbox hardcodes `pasteselection` and `copytrimwhitespace` to false vs JS config values
- **JS Source**: `src/js/modules/legacy_tab_textures.js` line 117
- **Status**: Pending
- **Details**: JS Listbox passes `:pasteselection="$core.view.config.pasteSelection"` and `:copytrimwhitespace="$core.view.config.removePathSpacesCopy"` from user config. C++ hardcodes both to `false` (lines 284–285). User settings for paste selection and whitespace trimming are ignored in the legacy textures tab.

- [ ] 241. [legacy_tab_textures.cpp] Listbox hardcodes `CopyMode::Default` vs JS `$core.view.config.copyMode`
- **JS Source**: `src/js/modules/legacy_tab_textures.js` line 117
- **Status**: Pending
- **Details**: JS Listbox passes `:copymode="$core.view.config.copyMode"` so the user's copy mode preference (Full/DIR/FID) is respected. C++ hardcodes `listbox::CopyMode::Default` (line 283). The user's configured copy mode is ignored.

- [ ] 242. [legacy_tab_textures.cpp] Channel checkboxes always visible vs JS conditional on `texturePreviewURL`
- **JS Source**: `src/js/modules/legacy_tab_textures.js` lines 130–135
- **Status**: Pending
- **Details**: JS only shows channel toggle buttons (`<ul class="preview-channels">`) when `$core.view.texturePreviewURL.length > 0`. C++ always renders the R/G/B/A checkboxes regardless of whether a texture is loaded (lines 329–350). Before any texture is selected, the channel controls are visible but non-functional.


## Tab: Audio

- [ ] 243. [tab_audio.cpp] Audio quick-filter list path is missing from listbox wiring
- **JS Source**: `src/js/modules/tab_audio.js` line 190
- **Status**: Pending
- **Details**: JS passes `$core.view.audioQuickFilters` into the sound listbox. C++ passes an empty quickfilter list (`{}`), so the original quick-filter behavior is not available.

- [ ] 244. [tab_audio.cpp] `unload_track` no longer revokes preview URL data like JS
- **JS Source**: `src/js/modules/tab_audio.js` lines 95–97
- **Status**: Pending
- **Details**: JS explicitly calls `file_data?.revokeDataURL()` and clears `file_data`; C++ has no equivalent revoke path, changing cleanup behavior for previewed track resources.

- [ ] 245. [tab_audio.cpp] Sound player visuals differ from the JS template/CSS implementation
- **JS Source**: `src/js/modules/tab_audio.js` lines 203–228
- **Status**: Pending
- **Details**: JS renders `#sound-player-anim`, custom component sliders, and CSS-styled play-state button classes; C++ uses plain ImGui button/slider widgets and a different animated icon presentation, so visuals are not identical.

- [ ] 246. [tab_audio.cpp] play_track uses get_duration() <= 0 instead of checking buffer existence
- **JS Source**: `src/js/modules/tab_audio.js` lines 100–101
- **Status**: Pending
- **Details**: JS checks !player.buffer to determine if a track is loaded. C++ checks player.get_duration() <= 0. A loaded but zero-duration track would behave differently.

- [ ] 247. [tab_audio.cpp] Missing audioQuickFilters prop on Listbox
- **JS Source**: `src/js/modules/tab_audio.js` line 190
- **Status**: Pending
- **Details**: JS passes quickfilters to the Listbox component. C++ passes empty {}. Quick-filter buttons for audio file types will not appear.

- [ ] 248. [tab_audio.cpp] unittype is "sound" instead of "sound file"
- **JS Source**: `src/js/modules/tab_audio.js` line 190
- **Status**: Pending
- **Details**: JS sets unittype to "sound file". C++ passes "sound". The file count display text differs.

- [ ] 249. [tab_audio.cpp] export_sounds missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_audio.js` line 175
- **Status**: Pending
- **Details**: JS passes e.message and e.stack to helper.mark(). C++ only passes e.what().

- [x] 250. [tab_audio.cpp] load_track play_track and export_sounds are synchronous blocking main thread
- **JS Source**: `src/js/modules/tab_audio.js` lines 47, 99, 122
- **Status**: Pending
- **Details**: All three JS functions are async with await. C++ implementations are synchronous, blocking the render thread.

- [ ] 251. [legacy_tab_audio.cpp] Playback UI visuals diverge from JS template/CSS
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 201–241
- **Status**: Pending
- **Details**: JS renders `#sound-player-anim`, CSS-styled play button state classes, and component sliders, while C++ replaces this with ImGui text/buttons/checkboxes and a custom icon pulse, so layout/styling is not pixel-identical.

- [ ] 252. [legacy_tab_audio.cpp] Seek-loop scheduling differs from JS `requestAnimationFrame` lifecycle
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 19–42
- **Status**: Pending
- **Details**: JS drives seek updates with `requestAnimationFrame` and explicit cancellation IDs; C++ updates via render-loop polling with `seek_loop_active`, changing timing and loop lifecycle semantics.

- [ ] 253. [legacy_tab_audio.cpp] Context menu adds FileDataID-related items not present in JS legacy audio template
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 205–209
- **Status**: Pending
- **Details**: JS context menu has 3 items: "Copy file path(s)", "Copy export path(s)", "Open export directory". C++ adds conditional "Copy file path(s) (listfile format)" and "Copy file data ID(s)" when `hasFileDataIDs` is true (lines 399–402). Legacy MPQ files don't have FileDataIDs, so these extra menu items are incorrect for the legacy audio tab.

- [ ] 254. [legacy_tab_audio.cpp] Sound player info combines seek/title/duration into single Text call vs JS 3 separate spans
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 218–222
- **Status**: Pending
- **Details**: JS renders sound player info as 3 separate `<span>` elements in a flex container: seek formatted time, title (with CSS class "title"), and duration formatted time. C++ combines them into a single `ImGui::Text("%s  %s  %s", ...)` call (line 453). The title won't have distinct styling, and the layout/alignment will differ from the JS flex row.

- [ ] 255. [legacy_tab_audio.cpp] Play/Pause uses text toggle ("Play"/"Pause") vs JS CSS class-based visual toggle
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 225
- **Status**: Pending
- **Details**: JS uses `<input type="button" :class="{ isPlaying: !soundPlayerState }">` — the button appearance changes via CSS class (likely showing a play/pause icon). C++ uses `ImGui::Button(view.soundPlayerState ? "Pause" : "Play")` with text labels. The visual appearance differs significantly from the original icon-based toggle.

- [ ] 256. [legacy_tab_audio.cpp] Volume slider is ImGui::SliderFloat with format string vs JS custom Slider component
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 226
- **Status**: Pending
- **Details**: JS uses a custom `<Slider>` component with id "slider-volume" for volume control. C++ uses `ImGui::SliderFloat` with format "Vol: %.0f%%" (line 474). The custom JS Slider has its own visual styling defined in CSS; the ImGui slider will look different (default ImGui styling vs themed slider).

- [ ] 257. [legacy_tab_audio.cpp] Loop/Autoplay checkboxes placed in preview container instead of preview-controls div
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 231–239
- **Status**: Pending
- **Details**: JS places Loop/Autoplay checkboxes and Export button together in the `preview-controls` div. C++ places Loop/Autoplay in the `PreviewContainer` section (lines 479–487) and Export in `PreviewControls` (lines 492–498). This changes the visual layout — checkboxes are above the export button area instead of beside it.

- [ ] 258. [legacy_tab_audio.cpp] `load_track` checks `player.get_duration() <= 0` vs JS `!player.buffer`
- **JS Source**: `src/js/modules/legacy_tab_audio.js` lines 100–101
- **Status**: Pending
- **Details**: JS `play_track` checks `!player.buffer` to determine if a track needs loading. C++ checks `player.get_duration() <= 0` (line 136). If a loaded track has zero duration (e.g., corrupt file that loads but has 0-length), C++ would re-load while JS would not. The check semantics are subtly different.

- [ ] 259. [legacy_tab_audio.cpp] `export_sounds` `helper.mark` doesn't pass error stack trace
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 189
- **Status**: Pending
- **Details**: JS calls `helper.mark(export_file_name, false, e.message, e.stack)` with 4 arguments including the stack trace. C++ calls `helper.mark(export_file_name, false, e.what())` with only 3 arguments (line 239). Error stack information is lost in C++ export failure reports.

- [ ] 260. [legacy_tab_audio.cpp] Filter input missing placeholder text "Filter sound files..."
- **JS Source**: `src/js/modules/legacy_tab_audio.js` line 213
- **Status**: Pending
- **Details**: JS filter input has `placeholder="Filter sound files..."`. C++ `ImGui::InputText("##FilterLegacySounds", ...)` (line 420) has no placeholder/hint text. The empty filter field won't show the helpful hint.


## Tab: Video

- [ ] 261. [tab_videos.cpp] Video preview playback is opened externally instead of using an embedded player
- **JS Source**: `src/js/modules/tab_videos.js` lines 219–276, 493
- **Status**: Pending
- **Details**: JS renders and controls an in-tab `<video>` element with `onended`/`onerror` and subtitle track attachment, while C++ opens the stream URL in an external handler and shows status text in the preview area.

- [ ] 262. [tab_videos.cpp] Video export format selector from MenuButton is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 505, 559–571
- **Status**: Pending
- **Details**: JS uses a `MenuButton` bound to `config.exportVideoFormat` and dispatches format-specific export via selection; C++ renders a single `Export Selected` button with no in-UI format picker.

- [ ] 263. [tab_videos.cpp] Kino processing toast omits the explicit Cancel action payload
- **JS Source**: `src/js/modules/tab_videos.js` lines 394–400
- **Status**: Pending
- **Details**: JS updates progress toast with `{ 'Cancel': cancel_processing }`; C++ calls `setToast(..., {}, ...)`, removing the explicit cancel action binding from the toast configuration.

- [ ] 264. [tab_videos.cpp] Dev-mode kino processing trigger export is missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 467–469
- **Status**: Pending
- **Details**: JS exposes `window.trigger_kino_processing = trigger_kino_processing` in non-release mode; C++ has no equivalent debug export hook.

- [ ] 265. [tab_videos.cpp] Corrupted AVI fallback does not force CASC fallback fetch path
- **JS Source**: `src/js/modules/tab_videos.js` line 697
- **Status**: Pending
- **Details**: JS retries corrupted cinematic reads with `getFileByName(file_name, false, false, true, true)` to force fallback behavior; C++ retries `getVirtualFileByName(file_name)` with normal arguments.

- [ ] 266. [tab_videos.cpp] MenuButton export format dropdown completely missing
- **JS Source**: `src/js/modules/tab_videos.js` line 505
- **Status**: Pending
- **Details**: JS uses `<MenuButton :options="menuButtonVideos" :default="config.exportVideoFormat" @change="..." @click="export_selected">` which renders a dropdown to pick MP4/AVI/MP3/SUBTITLES and triggers export. C++ renders a plain `ImGui::Button("Export Selected")` with no format selector. Users cannot change the export format from this tab.

- [ ] 267. [tab_videos.cpp] AVI export corruption fallback is a no-op
- **JS Source**: `src/js/modules/tab_videos.js` line 697
- **Status**: Pending
- **Details**: JS calls `getFileByName(file_name, false, false, true, true)` with extra params (forceFallback). C++ calls `getVirtualFileByName(file_name)` identically to the first attempt, with a comment admitting `// Note: C++ getVirtualFileByName doesn't support forceFallback; retry normally.` The corruption recovery path will always fail the same way twice.

- [ ] 268. [tab_videos.cpp] Video preview is text-only, not an embedded player
- **JS Source**: `src/js/modules/tab_videos.js` line 493
- **Status**: Pending
- **Details**: JS renders a `<video>` element with full controls, autoplay, subtitles overlay via `<track>`. C++ opens the URL in the system's external media player (`core::openInExplorer(url)`) and shows plain text. No inline playback, no controls, no subtitle overlay in the app window.

- [ ] 269. [tab_videos.cpp] No onended/onerror callbacks for video playback
- **JS Source**: `src/js/modules/tab_videos.js` lines 263–275
- **Status**: Pending
- **Details**: JS attaches `video.onended` (resets `is_streaming`/`videoPlayerState`) and `video.onerror` (shows error toast). C++ delegates to external player and has neither callback — `is_streaming` and `videoPlayerState` are never automatically reset when playback finishes; user must manually click "Stop Video."

- [x] 270. [tab_videos.cpp] build_payload runs on main thread, blocking UI
- **JS Source**: `src/js/modules/tab_videos.js` line 125
- **Status**: Pending
- **Details**: JS `build_payload` is `async`/`await` (non-blocking). In C++, it's called synchronously on the main thread before launching the background thread. DB2 queries + CASC lookups could freeze the UI.

- [ ] 271. [tab_videos.cpp] stop_video does not join/stop background thread
- **JS Source**: `src/js/modules/tab_videos.js` lines 27–57
- **Status**: Pending
- **Details**: JS clears `setTimeout` handle which fully cancels pending work. C++ sets `poll_cancelled = true` but does not `reset()` or join `stream_worker_thread`. The thread may still be running and post results after stop. Only `stream_video` joins it before a new stream.

- [ ] 272. [tab_videos.cpp] MP4 download HTTP error check missing
- **JS Source**: `src/js/modules/tab_videos.js` lines 631–633
- **Status**: Pending
- **Details**: JS explicitly checks `if (!response.ok)` and marks the file with 'Failed to download MP4: ' + response.status. C++ uses `generics::get(*mp4_url)` with no status check — if the server returns a non-200 status, behavior depends on `generics::get()` implementation.

- [ ] 273. [tab_videos.cpp] All helper.mark error calls missing stack trace argument
- **JS Source**: `src/js/modules/tab_videos.js` lines 642, 690, 702, 763, 822
- **Status**: Pending
- **Details**: JS passes `(file, false, e.message, e.stack)` (4 args) for error marking. C++ passes only `(file, false, e.what())` (3 args). Stack traces are lost in every export error path (MP4, AVI×2, MP3, Subtitles).

- [ ] 274. [tab_videos.cpp] stream_video outer catch missing stack trace log
- **JS Source**: `src/js/modules/tab_videos.js` line 214
- **Status**: Pending
- **Details**: JS logs `log.write(e.stack)` separately from the error message. C++ only logs `e.what()`.

- [ ] 275. [tab_videos.cpp] Cancel button missing from kino_processing toast
- **JS Source**: `src/js/modules/tab_videos.js` line 399
- **Status**: Pending
- **Details**: JS passes `{ 'Cancel': cancel_processing }` as toast buttons. C++ passes `{}` — no Cancel button is shown during batch processing, leaving users with no way to cancel.

- [ ] 276. [tab_videos.cpp] Regex tooltip missing on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_videos.js` line 489
- **Status**: Pending
- **Details**: JS shows `:title="$core.view.regexTooltip"` tooltip on the "Regex Enabled" text. C++ renders `ImGui::TextUnformatted("Regex Enabled")` with no tooltip.

- [ ] 277. [tab_videos.cpp] Spurious "Connecting to video server..." toast not in JS
- **JS Source**: `src/js/modules/tab_videos.js` (none)
- **Status**: Pending
- **Details**: JS shows no toast before the initial HTTP request — it only shows "Video is being processed..." on 202 status. C++ always shows a "Connecting to video server..." progress toast before the request, which is not in the original.

- [ ] 278. [tab_videos.cpp] "View Log" button text capitalization differs from JS
- **JS Source**: `src/js/modules/tab_videos.js` lines 195, 215
- **Status**: Pending
- **Details**: JS uses lowercase `'view log'`. C++ uses title case `"View Log"`.

- [ ] 279. [tab_videos.cpp] Filter input buffer capped at 255 chars
- **JS Source**: `src/js/modules/tab_videos.js` line 490
- **Status**: Pending
- **Details**: JS `v-model` has no character limit. C++ uses `char filter_buf[256]` which truncates filter input at 255 characters.

- [ ] 280. [tab_videos.cpp] kino_post hardcodes hostname and path instead of using constant
- **JS Source**: `src/js/modules/tab_videos.js` lines 137, 349, 431
- **Status**: Pending
- **Details**: JS uses `constants.KINO.API_URL` dynamically via `fetch()`. C++ hardcodes `httplib::SSLClient cli("www.kruithne.net")` and `.Post("/wow.export/v2/get_video", ...)` instead of parsing the constant. If the constant changes, C++ won't reflect it.

- [ ] 281. [tab_videos.cpp] Subtitle loading uses different API path than JS
- **JS Source**: `src/js/modules/tab_videos.js` lines 226–230
- **Status**: Pending
- **Details**: JS calls `subtitles.get_subtitles_vtt(core_ref.view.casc, subtitle_info.file_data_id, subtitle_info.format)` which fetches+converts internally. C++ manually fetches via `casc->getVirtualFileByID()`, reads as string, then calls `subtitles::get_subtitles_vtt(raw_subtitle_text, fmt)`. Different function signature — caller now responsible for fetching.

- [ ] 282. [tab_videos.cpp] MP4 download may lack User-Agent header
- **JS Source**: `src/js/modules/tab_videos.js` line 628
- **Status**: Pending
- **Details**: JS explicitly sets `'User-Agent': constants.USER_AGENT` for the MP4 download fetch. C++ uses `generics::get(*mp4_url)` which may or may not set User-Agent, depending on that function's implementation.

- [ ] 283. [tab_videos.cpp] Dead variable prev_selection_first never read
- **JS Source**: `src/js/modules/tab_videos.js` (none)
- **Status**: Pending
- **Details**: `prev_selection_first` is set on line 953 but never read for any comparison. The selection comparison uses `selected_file` instead. This is dead code.

- [ ] 284. [tab_videos.cpp] Dev-mode trigger_kino_processing not exposed in C++
- **JS Source**: `src/js/modules/tab_videos.js` lines 468–469
- **Status**: Pending
- **Details**: JS exposes `window.trigger_kino_processing = trigger_kino_processing` when `!BUILD_RELEASE`. C++ has only a comment. No equivalent debug hook exists.


## Tab: Text & Fonts

- [ ] 285. [tab_text.cpp] Text preview failure toast omits JS `View Log` action callback
- **JS Source**: `src/js/modules/tab_text.js` lines 138–139
- **Status**: Pending
- **Details**: JS preview failure toast provides `{ 'View Log': () => log.openRuntimeLog() }`; C++ passes empty toast actions, removing the original recovery handler.

- [ ] 286. [tab_text.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_text.js` line 31
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 287. [tab_text.cpp] getFileByName vs getVirtualFileByName in preview and export
- **JS Source**: `src/js/modules/tab_text.js` lines 129, 108
- **Status**: Pending
- **Details**: JS calls casc.getFileByName(first) for preview and export. C++ calls getVirtualFileByName which is a different method with different behavior (extra DBD manifest logic, unknown/ path handling).

- [ ] 288. [tab_text.cpp] readString() encoding parameter missing
- **JS Source**: `src/js/modules/tab_text.js` line 130
- **Status**: Pending
- **Details**: JS calls file.readString(undefined, 'utf8') passing explicit utf8 encoding. C++ calls file.readString() with no encoding argument. May produce different output for non-ASCII content.

- [ ] 289. [tab_text.cpp] Missing 'View Log' button in error toast
- **JS Source**: `src/js/modules/tab_text.js` lines 138–139
- **Status**: Pending
- **Details**: JS passes { 'View Log': () => log.openRuntimeLog() } as toast action. C++ passes empty {}. User has no way to open runtime log from error toast.

- [ ] 290. [tab_text.cpp] Regex tooltip missing on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_text.js` line 31
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ renders text with no tooltip.

- [ ] 291. [tab_text.cpp] Filter input missing placeholder text
- **JS Source**: `src/js/modules/tab_text.js` line 32
- **Status**: Pending
- **Details**: JS has placeholder="Filter text files...". C++ uses ImGui::InputText with no hint text.

- [x] 292. [tab_text.cpp] export_text is synchronous instead of async
- **JS Source**: `src/js/modules/tab_text.js` lines 77–121
- **Status**: Pending
- **Details**: JS export_text is async with await for generics.fileExists(), casc.getFileByName(), and data.writeToFile(). C++ runs entirely synchronously, freezing UI during multi-file export.

- [ ] 293. [tab_text.cpp] export_text error handler missing stack trace parameter
- **JS Source**: `src/js/modules/tab_text.js` line 116
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 294. [tab_text.cpp] Text preview child window padding differs from CSS
- **JS Source**: `src/js/modules/tab_text.js` line 36
- **Status**: Pending
- **Details**: CSS padding: 15px adds padding on all four sides. C++ sets ImGui::SetCursorPos(15, 15) for top-left only — no bottom/right padding. Content scrolls to exact end of text with no margin.

- [ ] 295. [tab_fonts.cpp] Font preview textarea is not rendered with the selected loaded font family
- **JS Source**: `src/js/modules/tab_fonts.js` lines 67, 159–163
- **Status**: Pending
- **Details**: JS binds preview textarea style `fontFamily` to the loaded font id; C++ updates `fontPreviewFontFamily` state but renders `InputTextMultiline` without switching ImGui font, so preview text does not reflect selected font family.

- [ ] 296. [tab_fonts.cpp] Loaded font cache contract differs from JS URL-based font-face lifecycle
- **JS Source**: `src/js/modules/tab_fonts.js` lines 10, 30–32
- **Status**: Pending
- **Details**: JS caches `font_id -> blob URL` from `inject_font_face`, preserving URL/font-face lifecycle; C++ caches `font_id -> void*` ImGui font pointer, changing resource model and API behavior.

- [ ] 297. [tab_fonts.cpp] Font preview textarea does not render in the loaded font
- **JS Source**: `src/js/modules/tab_fonts.js` line 67
- **Status**: Pending
- **Details**: JS applies fontFamily style to the preview textarea. C++ uses InputTextMultiline without pushing the loaded ImGui font. Preview renders in default font.

- [ ] 298. [tab_fonts.cpp] Missing data.processAllBlocks() call in load_font
- **JS Source**: `src/js/modules/tab_fonts.js` lines 28–29
- **Status**: Pending
- **Details**: JS explicitly calls data.processAllBlocks() to ensure all BLTE blocks are decompressed. C++ calls getVirtualFileByName() without explicit processAllBlocks(). Font data may be incomplete.

- [ ] 299. [tab_fonts.cpp] export_fonts missing stack trace in error mark
- **JS Source**: `src/js/modules/tab_fonts.js` line 141
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

- [x] 300. [tab_fonts.cpp] load_font and export_fonts are synchronous blocking main thread
- **JS Source**: `src/js/modules/tab_fonts.js` lines 16, 102
- **Status**: Pending
- **Details**: Both JS functions are async. C++ implementations are synchronous blocking the render thread.

- [ ] 301. [legacy_tab_fonts.cpp] Preview text is not rendered with the selected font family
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 78, 165–169
- **Status**: Pending
- **Details**: JS binds textarea `fontFamily` to `fontPreviewFontFamily`, but C++ renders `InputTextMultiline` without switching to the loaded `ImFont`, so font preview output does not use the selected legacy font.

- [ ] 302. [legacy_tab_fonts.cpp] Font loading contract differs from JS URL-based `loaded_fonts` cache
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 18–41
- **Status**: Pending
- **Details**: JS caches `font_id -> blob URL` and reuses CSS font-family identifiers; C++ caches `font_id -> void*` ImGui font pointers, changing the original module’s data model and font-resource lifecycle behavior.

- [ ] 303. [legacy_tab_fonts.cpp] Glyph cells rendered in default ImGui font, not the selected font family
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 88–91
- **Status**: Pending
- **Details**: JS glyph cells have `cell.style.fontFamily = '"${font_family}", monospace'` so each cell displays in the loaded font. C++ uses `ImGui::Selectable(utf8_buf, ...)` (line 263) which renders in the default ImGui font. Glyphs from the inspected font (e.g., decorative characters) will appear as the default UI font instead, making the glyph grid useless for visual font inspection.

- [ ] 304. [legacy_tab_fonts.cpp] Font preview placeholder shown as tooltip vs JS textarea placeholder
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 78
- **Status**: Pending
- **Details**: JS uses `<textarea :placeholder="$core.view.fontPreviewPlaceholder">` which shows ghost placeholder text inside the text area when empty. C++ uses `ImGui::SetItemTooltip(...)` (line 293) which only shows the text on mouse hover as a tooltip popup. The visual behavior differs — JS shows persistent in-field hint text; C++ shows nothing until hover.

- [ ] 305. [legacy_tab_fonts.cpp] Glyph cell size hardcoded 24x24 may not match JS CSS
- **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 76
- **Status**: Pending
- **Details**: C++ glyph cells use `ImVec2(24, 24)` (line 263) for the Selectable size. JS uses CSS class `font-glyph-cell` whose dimensions are defined in `app.css`. The CSS may define different sizing, padding, or font-size for glyph cells, causing a visual mismatch.


## Tab: Data

- [ ] 306. [tab_data.cpp] Data-table cell copy stringification differs from JS `String(value)` behavior
- **JS Source**: `src/js/modules/tab_data.js` lines 172–177
- **Status**: Pending
- **Details**: JS copies with `String(value)`, while C++ uses `value.dump()`; for string JSON values this includes JSON quoting/escaping, changing clipboard output.

- [ ] 307. [tab_data.cpp] DB2 load error toast omits JS `View Log` action callback
- **JS Source**: `src/js/modules/tab_data.js` lines 80–82
- **Status**: Pending
- **Details**: JS error toast includes `{'View Log': () => log.openRuntimeLog()}`; C++ error toast uses empty actions, removing the original recovery handler.

- [ ] 308. [tab_data.cpp] Listbox single parameter is true should be false (multi-select broken)
- **JS Source**: `src/js/modules/tab_data.js` lines 97–99
- **Status**: Pending
- **Details**: JS Listbox does not pass single, enabling multi-selection. C++ passes true restricting to single-entry mode. This breaks all multi-table export logic.

- [ ] 309. [tab_data.cpp] Listbox nocopy is false should be true
- **JS Source**: `src/js/modules/tab_data.js` line 99
- **Status**: Pending
- **Details**: JS passes nocopy true to disable Ctrl+C. C++ passes false leaving Ctrl+C enabled.

- [ ] 310. [tab_data.cpp] Listbox unittype is "table" should be "db2 file"
- **JS Source**: `src/js/modules/tab_data.js` line 99
- **Status**: Pending
- **Details**: JS uses unittype "db2 file". C++ passes "table". The file count display text differs.

- [ ] 311. [tab_data.cpp] Listbox pasteselection and copytrimwhitespace hardcoded false
- **JS Source**: `src/js/modules/tab_data.js` lines 98–99
- **Status**: Pending
- **Details**: JS binds pasteselection to config.pasteSelection and copytrimwhitespace to config.removePathSpacesCopy. C++ hardcodes both to false.

- [ ] 312. [tab_data.cpp] load_table error toast missing View Log action button
- **JS Source**: `src/js/modules/tab_data.js` line 80
- **Status**: Pending
- **Details**: JS includes View Log action. C++ passes empty map.

- [ ] 313. [tab_data.cpp] Context menu labels are static instead of dynamic row count
- **JS Source**: `src/js/modules/tab_data.js` lines 108–110
- **Status**: Pending
- **Details**: JS renders "Copy N rows as CSV" with actual selectedCount and pluralization. C++ uses static labels losing the count.

- [ ] 314. [tab_data.cpp] Context menu node not cleared on close
- **JS Source**: `src/js/modules/tab_data.js` line 107
- **Status**: Pending
- **Details**: JS resets nodeDataTable to null on close. C++ never resets it so the condition stays true permanently.

- [ ] 315. [tab_data.cpp] copy_cell uses value.dump() producing JSON-quoted strings
- **JS Source**: `src/js/modules/tab_data.js` lines 172–177
- **Status**: Pending
- **Details**: JS uses String(value) producing unquoted output. C++ uses value.dump() adding extra quotes for strings.

- [ ] 316. [tab_data.cpp] Selection watcher prevents retry after failed load
- **JS Source**: `src/js/modules/tab_data.js` lines 371–377
- **Status**: Pending
- **Details**: JS compares selected_file which is not updated on failure allowing retry. C++ compares prev_selection_last updated unconditionally preventing retry.

- [ ] 317. [tab_data.cpp] Missing Regex Enabled indicators in both filter bars
- **JS Source**: `src/js/modules/tab_data.js` lines 102–103, 129–130
- **Status**: Pending
- **Details**: JS shows Regex Enabled div in both the DB2 filter bar and data table tray filter. C++ has no regex indicators.

- [ ] 318. [tab_data.cpp] helper.mark() calls missing stack trace argument
- **JS Source**: `src/js/modules/tab_data.js` lines 250, 314, 358
- **Status**: Pending
- **Details**: JS passes both e.message and e.stack. C++ only passes e.what().

- [ ] 319. [legacy_tab_data.cpp] Export format menu omits JS SQL/DBC options
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 172–176, 222–231
- **Status**: Pending
- **Details**: JS menu exposes `CSV`, `SQL`, and `DBC` export actions, but C++ `legacy_data_opts` only includes `Export as CSV`, making SQL/DBC exports unavailable through the settings menu path.

- [ ] 320. [legacy_tab_data.cpp] `copy_cell` empty-string handling differs from JS
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 215–220
- **Status**: Pending
- **Details**: JS copies any non-null/undefined value (including empty string), while C++ returns early on `value.empty()`, so empty-cell clipboard behavior is not equivalent.

- [ ] 321. [legacy_tab_data.cpp] DBC filename extraction uses `std::filesystem::path` which won't split backslash-delimited MPQ paths on Linux
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 33–36
- **Status**: Pending
- **Details**: JS uses `full_path.split('\\')` to extract the DBC filename from backslash-delimited MPQ paths. C++ uses `std::filesystem::path(full_path).filename()` (line 81). On Linux, `std::filesystem::path` treats `\` as a regular character, not a separator, so `filename()` would return the entire path string instead of just the filename. This would cause the table name extraction to fail for MPQ paths like `DBFilesClient\Achievement.dbc`.

- [ ] 322. [legacy_tab_data.cpp] Listbox `unittype` is "table" vs JS "dbc file"
- **JS Source**: `src/js/modules/legacy_tab_data.js` line 131
- **Status**: Pending
- **Details**: JS Listbox uses `unittype="dbc file"` for the file count display. C++ uses `"table"` (line 293). The status bar will show "X tables" instead of "X dbc files", which is a user-facing text difference.

- [ ] 323. [legacy_tab_data.cpp] Listbox `nocopy` is `false` vs JS `:nocopy="true"`
- **JS Source**: `src/js/modules/legacy_tab_data.js` line 131
- **Status**: Pending
- **Details**: JS DBC listbox has `:nocopy="true"` which disables CTRL+C copy functionality. C++ passes `false` for nocopy (line 297), allowing copy. This is a behavioral difference — users can copy DBC table names in C++ but not in JS.

- [ ] 324. [legacy_tab_data.cpp] Missing regex info display in DBC filter bar
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 134–135
- **Status**: Pending
- **Details**: JS has `<div class="regex-info" v-if="$core.view.config.regexFilters">Regex Enabled</div>` in the DBC listbox filter section. C++ DBC filter bar (lines 309–316) does not show the regex enabled indicator.

- [ ] 325. [legacy_tab_data.cpp] Context menu uses `ImGui::BeginPopupContextItem` vs JS ContextMenu component
- **JS Source**: `src/js/modules/legacy_tab_data.js` lines 139–143
- **Status**: Pending
- **Details**: JS uses the custom `ContextMenu` component with slot-based content rendering and a close event. C++ uses native `ImGui::BeginPopupContextItem` (line 368) which has different popup behavior, positioning, and styling compared to the custom ContextMenu component used elsewhere in the app.


## Tab: Raw Files & Legacy Files

- [ ] 326. [tab_raw.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_raw.js` line 158
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 327. [tab_raw.cpp] export_raw_files uses getVirtualFileByName and drops partialDecrypt=true
- **JS Source**: `src/js/modules/tab_raw.js` line 123
- **Status**: Pending
- **Details**: JS calls core.view.casc.getFileByName(file_name, true) passing partialDecrypt=true. C++ calls getVirtualFileByName(file_name) without partialDecrypt parameter, silently dropping partial decryption capability for encrypted files.

- [ ] 328. [tab_raw.cpp] export_raw_files error mark missing stack trace argument
- **JS Source**: `src/js/modules/tab_raw.js` line 128
- **Status**: Pending
- **Details**: JS calls helper.mark(export_file_name, false, e.message, e.stack). C++ passes only e.what(), omitting stack trace.

- [ ] 329. [tab_raw.cpp] parent_path() returns "" not "." for bare filenames
- **JS Source**: `src/js/modules/tab_raw.js` lines 113–115
- **Status**: Pending
- **Details**: JS path.dirname returns "." for bare filenames, then checks dir === ".". C++ parent_path() returns "" for bare filenames, then checks dir == "." which never matches. Functionally similar result but fragile — should check dir.empty() || dir == ".".

- [ ] 330. [tab_raw.cpp] Missing placeholder text on filter input
- **JS Source**: `src/js/modules/tab_raw.js` line 159
- **Status**: Pending
- **Details**: JS has placeholder="Filter raw files...". C++ uses ImGui::InputText with no hint text.

- [ ] 331. [tab_raw.cpp] Missing tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_raw.js` line 158
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ has no tooltip.

- [x] 332. [tab_raw.cpp] All async functions converted to synchronous — blocks UI thread
- **JS Source**: `src/js/modules/tab_raw.js` lines 12, 31, 91
- **Status**: Pending
- **Details**: JS compute_raw_files, detect_raw_files, and export_raw_files are all async. C++ versions are synchronous, blocking render thread during CASC I/O and disk operations.

- [ ] 333. [tab_raw.cpp] detect_raw_files manually sets is_dirty=true — deviates from JS
- **JS Source**: `src/js/modules/tab_raw.js` lines 75–76
- **Status**: Pending
- **Details**: JS calls listfile.ingestIdentifiedFiles then compute_raw_files without setting is_dirty. Since is_dirty was false, JS would return early (apparent JS bug). C++ adds is_dirty=true to fix this, which is arguably correct but deviates from original JS behavior.

- [ ] 334. [legacy_tab_files.cpp] Listbox context menu includes extra FileDataID actions absent in JS
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 76–80
- **Status**: Pending
- **Details**: JS legacy-files menu only provides copy file path, copy export path, and open export directory; C++ conditionally adds listfile-format and fileDataID entries, changing context-menu behavior.

- [ ] 335. [legacy_tab_files.cpp] Layout doesn't use `app::layout` helpers — uses raw `ImGui::BeginChild`
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 72–89
- **Status**: Pending
- **Details**: Other legacy tabs (audio, fonts, textures, data) use `app::layout::BeginTab/EndTab`, `CalcListTabRegions`, `BeginListContainer`, etc. for consistent layout. `legacy_tab_files.cpp` uses raw `ImGui::BeginChild("legacy-files-list-container", ...)` (line 124) without the layout system. This will produce inconsistent sizing and positioning compared to sibling legacy tabs.

- [ ] 336. [legacy_tab_files.cpp] Filter input missing placeholder text "Filter files..."
- **JS Source**: `src/js/modules/legacy_tab_files.js` line 85
- **Status**: Pending
- **Details**: JS filter input has `placeholder="Filter files..."`. C++ `ImGui::InputText("##FilterFiles", ...)` (line 207) has no placeholder/hint text.

- [ ] 337. [legacy_tab_files.cpp] Tray layout structure differs from JS
- **JS Source**: `src/js/modules/legacy_tab_files.js` lines 82–88
- **Status**: Pending
- **Details**: JS wraps the filter and export button in a `#tab-legacy-files-tray` div with its own layout (likely flex row). C++ renders filter input, then `ImGui::SameLine()`, then the export button (lines 206–216). The proportions and alignment of filter vs button may not match the JS CSS-defined tray layout.


## Tab: Maps

- [ ] 338. [tab_maps.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_maps.js` line 302
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 339. [tab_maps.cpp] Hand-rolled MD5 instead of mbedTLS
- **JS Source**: `src/js/modules/tab_maps.js` line 914
- **Status**: Pending
- **Details**: C++ implements a full RFC 1321 MD5 from scratch instead of using mbedTLS MD API (mbedtls/md.h) as specified in project conventions.

- [ ] 340. [tab_maps.cpp] load_map_tile uses nearest-neighbor scaling instead of bilinear interpolation
- **JS Source**: `src/js/modules/tab_maps.js` lines 62–71
- **Status**: Pending
- **Details**: JS Canvas 2D drawImage performs bilinear interpolation when scaling. C++ uses nearest-neighbor sampling (integer coordinate snapping), making scaled minimap tiles look blockier/pixelated.

- [ ] 341. [tab_maps.cpp] load_map_tile uses blp.width instead of blp.scaledWidth
- **JS Source**: `src/js/modules/tab_maps.js` line 62
- **Status**: Pending
- **Details**: JS computes scale as size / blp.scaledWidth. C++ uses blp.width. If the BLP has a scaledWidth differing from raw width (e.g. mipmaps), the scaling factor will be wrong.

- [ ] 342. [tab_maps.cpp] load_wmo_minimap_tile ignores drawX/drawY and scaleX/scaleY positioning
- **JS Source**: `src/js/modules/tab_maps.js` lines 107–112
- **Status**: Pending
- **Details**: JS draws each tile at its specific offset (tile.drawX * output_scale, tile.drawY * output_scale) with scaled dimensions. C++ ignores drawX, drawY, scaleX, scaleY entirely — stretching all tiles to fill the full cell. Multi-tile compositing within a grid cell is completely broken.

- [ ] 343. [tab_maps.cpp] export_map_wmo_minimap uses max-alpha instead of source-over compositing
- **JS Source**: `src/js/modules/tab_maps.js` lines 721–733
- **Status**: Pending
- **Details**: JS Canvas 2D drawImage uses Porter-Duff source-over compositing. C++ export computes alpha as max(dst_alpha, src_alpha) instead of correct source-over formula.

- [ ] 344. [tab_maps.cpp] WDT file load is outside try-catch block
- **JS Source**: `src/js/modules/tab_maps.js` lines 433–434
- **Status**: Pending
- **Details**: In JS, getFileByName(wdt_path) is inside the try block. In C++, getVirtualFileByName(wdt_path) is BEFORE the try block. If WDT file doesn't exist, exception propagates uncaught.

- [ ] 345. [tab_maps.cpp] mapViewerHasWorldModel check differs from JS
- **JS Source**: `src/js/modules/tab_maps.js` lines 438–439
- **Status**: Pending
- **Details**: JS checks if (wdt.worldModelPlacement) — any non-null object is truthy. C++ checks if (worldModelPlacement.id != 0), which misses placement with id=0. Also affects has_global_wmo and export_map_wmo checks.

- [ ] 346. [tab_maps.cpp] Missing e.stack in all helper.mark error calls
- **JS Source**: `src/js/modules/tab_maps.js` lines 680, 759, 815, 848, 931, 1122
- **Status**: Pending
- **Details**: JS passes both e.message and e.stack to helper.mark. C++ only passes e.what(), omitting stack trace. Affects 6 export functions.

- [x] 347. [tab_maps.cpp] All async functions converted to synchronous — UI will block
- **JS Source**: `src/js/modules/tab_maps.js` lines 49–980
- **Status**: Pending
- **Details**: Every async function (load_map_tile, load_wmo_minimap_tile, collect_game_objects, extract_height_data_from_tile, load_map, setup_wmo_minimap, all export functions, initialize) is synchronous C++. Long exports freeze the UI.

- [ ] 348. [tab_maps.cpp] Missing optional chaining for export_paths
- **JS Source**: `src/js/modules/tab_maps.js` lines 752–853
- **Status**: Pending
- **Details**: JS uses optional chaining export_paths?.writeLine() and export_paths?.close(). C++ calls directly without null checks. If openLastExportStream returns invalid object, C++ will crash.

- [ ] 349. [tab_maps.cpp] Missing "Filter maps..." placeholder text
- **JS Source**: `src/js/modules/tab_maps.js` line 303
- **Status**: Pending
- **Details**: JS filter input has placeholder="Filter maps...". C++ uses ImGui::InputText without placeholder text.

- [ ] 350. [tab_maps.cpp] Missing regex tooltip
- **JS Source**: `src/js/modules/tab_maps.js` line 302
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" on regex-info div. C++ renders ImGui::TextDisabled("Regex Enabled") without any tooltip.

- [ ] 351. [tab_maps.cpp] Sidebar headers use SeparatorText instead of styled span
- **JS Source**: `src/js/modules/tab_maps.js` lines 313, 342, 352, 354
- **Status**: Pending
- **Details**: JS uses <span class="header"> which renders as bold text. C++ uses ImGui::SeparatorText() which draws a horizontal separator line with text — visually different.

- [ ] 352. [tab_maps.cpp] collect_game_objects returns vector instead of Set
- **JS Source**: `src/js/modules/tab_maps.js` lines 146–157
- **Status**: Pending
- **Details**: JS returns a Set guaranteeing uniqueness. C++ returns std::vector<ADTGameObject> which can contain duplicates.

- [ ] 353. [tab_maps.cpp] Selection watch may miss intermediate changes
- **JS Source**: `src/js/modules/tab_maps.js` lines 1135–1143
- **Status**: Pending
- **Details**: JS Vue $watch triggers on any reactive change. C++ only compares the first element string between frames. If selection changes and reverts within same frame, or changes to different item with same first entry, C++ misses it.


## Tab: Zones

- [ ] 354. [tab_zones.cpp] Default phase filtering excludes non-zero phases unlike JS
- **JS Source**: `src/js/modules/tab_zones.js` lines 78–79
- **Status**: Pending
- **Details**: JS includes all `UiMapXMapArt` links when `phase_id === null`; C++ filters to `PhaseID == 0` when no phase is selected.

- [ ] 355. [tab_zones.cpp] UiMapArtStyleLayer lookup key differs from JS relation logic
- **JS Source**: `src/js/modules/tab_zones.js` lines 88–90
- **Status**: Pending
- **Details**: JS resolves style layers by matching `UiMapArtStyleID` to `art_entry.UiMapArtStyleID`; C++ matches `UiMapArtID` to the linked art ID, changing style-layer association behavior.

- [ ] 356. [tab_zones.cpp] Base tile relation lookup uses layer-row ID instead of UiMapArt ID
- **JS Source**: `src/js/modules/tab_zones.js` lines 120–121
- **Status**: Pending
- **Details**: JS fetches `UiMapArtTile` relation rows with `art_style.ID` from the UiMapArt entry; C++ stores `CombinedArtStyle::id` as the UiMapArtStyleLayer row ID and uses that in `getRelationRows`, altering tile resolution.

- [ ] 357. [tab_zones.cpp] Base map tile OffsetX/OffsetY offsets are ignored
- **JS Source**: `src/js/modules/tab_zones.js` lines 181–182
- **Status**: Pending
- **Details**: JS applies `tile.OffsetX`/`tile.OffsetY` when placing map tiles; C++ calculates tile position from row/column and tile dimensions only.

- [ ] 358. [tab_zones.cpp] Zone listbox copy/paste trim options are hardcoded instead of config-bound
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: JS binds `copymode`, `pasteselection`, and `copytrimwhitespace` to config values; C++ hardcodes `CopyMode::Default`, `pasteselection=false`, and `copytrimwhitespace=false`.

- [ ] 359. [tab_zones.cpp] Phase selector placement differs from JS preview overlay layout
- **JS Source**: `src/js/modules/tab_zones.js` lines 341–349
- **Status**: Pending
- **Details**: JS renders the phase dropdown in a `preview-dropdown-overlay` inside the preview background; C++ renders phase selection in the bottom control bar.

- [ ] 360. [tab_zones.cpp] UiMapArtStyleLayer join uses wrong field name
- **JS Source**: `src/js/modules/tab_zones.js` lines 88–91
- **Status**: Pending
- **Details**: JS joins `art_style_layer.UiMapArtStyleID === art_entry.UiMapArtStyleID`. C++ joins on `layer_row["UiMapArtID"]` — a completely different field name. The C++ looks for "UiMapArtID" in UiMapArtStyleLayer table, but JS matches on "UiMapArtStyleID" from both tables. This produces wrong rows or no rows.

- [ ] 361. [tab_zones.cpp] CombinedArtStyle.id stores wrong ID (layer ID vs art ID)
- **JS Source**: `src/js/modules/tab_zones.js` lines 94–101
- **Status**: Pending
- **Details**: JS `combined_style` includes `...art_entry` (spread), so `combined_style.ID` = the UiMapArt row ID (`art_id`). C++ sets `style.id = static_cast<int>(layer_id)` which is the UiMapArtStyleLayer table key. This wrong ID propagates to `getRelationRows()` calls for UiMapArtTile and WorldMapOverlay.

- [ ] 362. [tab_zones.cpp] C++ adds ALL matching style layers; JS keeps only LAST
- **JS Source**: `src/js/modules/tab_zones.js` lines 86–91
- **Status**: Pending
- **Details**: JS declares `let style_layer;` then overwrites in a loop, keeping only the last match. C++ `push_back`s every matching row into `art_styles`. This creates duplicate/extra entries causing redundant or incorrect rendering.

- [ ] 363. [tab_zones.cpp] Phase filter logic differs when phase_id is null
- **JS Source**: `src/js/modules/tab_zones.js` line 78
- **Status**: Pending
- **Details**: JS: `if (phase_id === null || link_entry.PhaseID === phase_id)` — when phase_id is null, ALL entries are included. C++: when `phase_id` is nullopt, only entries with `row_phase == 0` are included. C++ omits non-default phases when no phase is specified, while JS shows all.

- [ ] 364. [tab_zones.cpp] Missing tile OffsetX/OffsetY in render_map_tiles
- **JS Source**: `src/js/modules/tab_zones.js` lines 181–182
- **Status**: Pending
- **Details**: JS: `final_x = pixel_x + (tile.OffsetX || 0); final_y = pixel_y + (tile.OffsetY || 0)`. C++ only uses `pixel_x = col * tile_width; pixel_y = row * tile_height` with no offset. Tiles with non-zero offsets will be mispositioned.

- [ ] 365. [tab_zones.cpp] Tile layer rendering architecture differs from JS
- **JS Source**: `src/js/modules/tab_zones.js` lines 126–152
- **Status**: Pending
- **Details**: JS groups ALL tiles for an art_style by their LayerIndex, then renders each group in sorted order. C++ calls `render_map_tiles(art_style, art_style.layer_index, ...)` which filters tiles to only those matching the single layer_index. Combined with the duplicate style layers issue, rendering pipeline differs significantly.

- [ ] 366. [tab_zones.cpp] parse_zone_entry doesn't throw on bad input
- **JS Source**: `src/js/modules/tab_zones.js` lines 17–18
- **Status**: Pending
- **Details**: JS throws `new Error('unexpected zone entry')` on regex mismatch. C++ returns an empty `ZoneDisplayInfo{}` with `id=0`. Callers add `zone.id > 0` guards, but error propagation differs.

- [ ] 367. [tab_zones.cpp] UiMap row existence not validated
- **JS Source**: `src/js/modules/tab_zones.js` lines 67–71
- **Status**: Pending
- **Details**: JS checks `if (!map_data)` and throws `'UiMap entry not found'`. C++ fetches the row but casts to void: `(void)ui_map_row_opt;` — never checks the result or throws.

- [ ] 368. [tab_zones.cpp] Pixel buffer not cleared at start of render when first layer is non-zero
- **JS Source**: `src/js/modules/tab_zones.js` line 59
- **Status**: Pending
- **Details**: JS calls `ctx.clearRect(0, 0, canvas.width, canvas.height)` at the start. C++ only allocates/clears the pixel buffer inside the `if (art_style.layer_index == 0)` block. If the first art_style has layer_index != 0, stale pixel data remains.

- [ ] 369. [tab_zones.cpp] Listbox copyMode hardcoded instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ passes `listbox::CopyMode::Default` instead of reading from `view.config["copyMode"]`.

- [ ] 370. [tab_zones.cpp] Listbox pasteSelection hardcoded false instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ hardcodes `false` instead of reading `view.config["pasteSelection"]`.

- [ ] 371. [tab_zones.cpp] Listbox copytrimwhitespace hardcoded false instead of from config
- **JS Source**: `src/js/modules/tab_zones.js` line 315
- **Status**: Pending
- **Details**: C++ hardcodes `false` instead of reading `view.config["removePathSpacesCopy"]`.

- [ ] 372. [tab_zones.cpp] export_zone_map helper.mark missing stack trace
- **JS Source**: `src/js/modules/tab_zones.js` line 491
- **Status**: Pending
- **Details**: JS: `helper.mark(zone_entry, false, e.message, e.stack)` — passes both message and stack. C++: `helper.mark(..., false, e.what())` — only passes message, no stack trace.

- [ ] 373. [tab_zones.cpp] Phase dropdown placed in control bar instead of preview overlay
- **JS Source**: `src/js/modules/tab_zones.js` lines 341–347
- **Status**: Pending
- **Details**: JS puts the phase `<select>` inside `preview-dropdown-overlay` div overlaid on the zone canvas. C++ places the `ImGui::BeginCombo` in the bottom controls bar alongside checkboxes/button. This is a layout difference.

- [ ] 374. [tab_zones.cpp] Missing regex tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_zones.js` line 325
- **Status**: Pending
- **Details**: JS shows a tooltip on "Regex Enabled" via `:title="$core.view.regexTooltip"`. C++ renders `ImGui::TextUnformatted("Regex Enabled")` with no tooltip.

- [ ] 375. [tab_zones.cpp] EXPANSION_NAMES static vector is dead code
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: `EXPANSION_NAMES` vector is defined but never referenced. The actual expansion rendering uses `constants::EXPANSIONS`. Should be removed.

- [ ] 376. [tab_zones.cpp] ZoneDisplayInfo vs ZoneEntry naming mismatch with header
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: The header declares `ZoneEntry` struct but the cpp defines a separate `ZoneDisplayInfo` struct for `parse_zone_entry`. The header's `ZoneEntry` appears unused.

- [ ] 377. [tab_zones.cpp] Missing per-tile position logging in render_map_tiles
- **JS Source**: `src/js/modules/tab_zones.js` lines 184–185
- **Status**: Pending
- **Details**: JS logs `'rendering tile FileDataID %d at position (%d,%d) -> (%d,%d) [Layer %d]'` for each tile. C++ has no per-tile log.

- [ ] 378. [tab_zones.cpp] Missing "no tiles found" log for art style
- **JS Source**: `src/js/modules/tab_zones.js` lines 121–123
- **Status**: Pending
- **Details**: JS logs `'no tiles found for UiMapArt ID %d'` and `continue`s. C++ has no equivalent check/log.

- [ ] 379. [tab_zones.cpp] Missing "no overlays found" log
- **JS Source**: `src/js/modules/tab_zones.js` lines 212–214
- **Status**: Pending
- **Details**: JS logs `'no WorldMapOverlay entries found for UiMapArt ID %d'` when overlays array is empty. C++ has no such log.

- [ ] 380. [tab_zones.cpp] Missing "no overlay tiles" log per overlay
- **JS Source**: `src/js/modules/tab_zones.js` lines 219–222
- **Status**: Pending
- **Details**: JS logs `'no tiles found for WorldMapOverlay ID %d'` and `continue`s for empty tile sets. C++ calls `render_overlay_tiles` regardless.

- [ ] 381. [tab_zones.cpp] Unsafe Windows wstring conversion corrupts multi-byte UTF-8 paths
- **JS Source**: `src/js/modules/tab_zones.js` (none)
- **Status**: Pending
- **Details**: `std::wstring wpath(dir.begin(), dir.end())` does byte-by-byte copy which corrupts multi-byte UTF-8 paths. Should use `MultiByteToWideChar` or equivalent.

- [ ] 382. [tab_zones.cpp] Linux shell command injection risk in openInExplorer
- **JS Source**: `src/js/modules/tab_zones.js` line 393
- **Status**: Pending
- **Details**: `"xdg-open \"" + dir + "\" &"` passed to `std::system()`. If `dir` contains shell metacharacters, this is exploitable. JS uses `nw.Shell.openItem` which is safe.


## Tab: Items & Item Sets

- [ ] 383. [tab_items.cpp] Wowhead item handler is stubbed out
- **JS Source**: `src/js/modules/tab_items.js` lines 322–324
- **Status**: Pending
- **Details**: JS calls `ExternalLinks.wowHead_viewItem(item_id)` from the context action; C++ `view_on_wowhead(...)` immediately returns and does nothing.

- [ ] 384. [tab_items.cpp] Item sidebar checklist interaction/layout diverges from JS clickable row design
- **JS Source**: `src/js/modules/tab_items.js` lines 254–266
- **Status**: Pending
- **Details**: JS uses `.sidebar-checklist-item` rows with selected-state styling and row-level click toggling; C++ renders plain ImGui checkboxes, changing sidebar visuals and interaction feel.

- [ ] 385. [tab_items.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_items.js` line 248
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ renders plain `Regex Enabled` text without tooltip behavior.

- [ ] 386. [tab_items.cpp] view_on_wowhead is stubbed — does nothing
- **JS Source**: `src/js/modules/tab_items.js` lines 322–324
- **Status**: Pending
- **Details**: JS calls ExternalLinks.wowHead_viewItem(item_id) which opens a Wowhead URL. C++ function is { return; } — a no-op. external_links.h already provides wowHead_viewItem().

- [ ] 387. [tab_items.cpp] copy_to_clipboard bypasses core.view.copyToClipboard
- **JS Source**: `src/js/modules/tab_items.js` lines 318–320
- **Status**: Pending
- **Details**: JS calls this.$core.view.copyToClipboard(value) which may have additional behavior (e.g. toast notification). C++ calls ImGui::SetClipboardText() directly, skipping view layer.

- [ ] 388. [tab_items.cpp] std::set ordering differs from JS Set insertion order
- **JS Source**: `src/js/modules/tab_items.js` lines 85–127
- **Status**: Pending
- **Details**: view_item_models and view_item_textures use JS Set which preserves insertion order. C++ uses std::set<std::string> which sorts lexicographically. Should use std::vector with uniqueness check.

- [ ] 389. [tab_items.cpp] Second loop (itemViewerShowAll) retrieves item name from wrong source
- **JS Source**: `src/js/modules/tab_items.js` lines 181–211
- **Status**: Pending
- **Details**: JS constructs new Item(item_id, item_row, null, null, null) where Item constructor reads item_row.Display_lang. C++ looks up name via DBItems::getItemById(item_id), a different data source.

- [ ] 390. [tab_items.cpp] Regex tooltip not rendered
- **JS Source**: `src/js/modules/tab_items.js` line 248
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" on "Regex Enabled" div. C++ renders ImGui::TextUnformatted("Regex Enabled") without any tooltip.

- [ ] 391. [tab_items.cpp] Sidebar headers use SeparatorText instead of styled header span
- **JS Source**: `src/js/modules/tab_items.js` lines 252, 262
- **Status**: Pending
- **Details**: JS uses <span class="header">Item Types</span> which renders as styled header text. C++ uses ImGui::SeparatorText() which draws a horizontal separator line with text — visually different.

- [ ] 392. [tab_items.cpp] Sidebar checklist items lack .selected class visual feedback
- **JS Source**: `src/js/modules/tab_items.js` lines 254–257
- **Status**: Pending
- **Details**: JS adds :class="{ selected: item.checked }" to checklist items. CSS gives .sidebar-checklist-item.selected a background of rgba(255,255,255,0.05). C++ uses plain ImGui::Checkbox with no highlight.

- [x] 393. [tab_items.cpp] All async operations converted to synchronous — UI may block
- **JS Source**: `src/js/modules/tab_items.js` lines 104, 129, 277, 345–346
- **Status**: Pending
- **Details**: JS initialize_items, view_item_textures, and the mounted initialize flow are all async with await. C++ converts all to synchronous blocking calls, freezing the ImGui render loop.

- [ ] 394. [tab_items.cpp] Quality color applied only to CheckMark, not to label text
- **JS Source**: `src/js/modules/tab_items.js` lines 264–265
- **Status**: Pending
- **Details**: CSS accent-color applies quality color to the checkbox. C++ pushes ImGuiCol_CheckMark only coloring the checkmark glyph. The checkbox background/frame and label text are unaffected.

- [ ] 395. [tab_items.cpp] Filter input buffer limited to 256 bytes
- **JS Source**: `src/js/modules/tab_items.js` line 249
- **Status**: Pending
- **Details**: JS input has no character limit. C++ uses char filter_buf[256] with std::strncpy, silently truncating beyond 255 characters.

- [ ] 396. [tab_item_sets.cpp] Regex indicator tooltip metadata from JS template is missing
- **JS Source**: `src/js/modules/tab_item_sets.js` line 82
- **Status**: Pending
- **Details**: JS `regex-info` includes `:title="$core.view.regexTooltip"`; C++ shows plain `Regex Enabled` text without tooltip behavior.

- [ ] 397. [tab_item_sets.cpp] Missing filter input placeholder text
- **JS Source**: `src/js/modules/tab_item_sets.js` line 83
- **Status**: Pending
- **Details**: JS uses placeholder="Filter item sets..." on the text input. C++ uses ImGui::InputText without hint/placeholder text.

- [ ] 398. [tab_item_sets.cpp] Missing regex tooltip on "Regex Enabled" text
- **JS Source**: `src/js/modules/tab_item_sets.js` line 82
- **Status**: Pending
- **Details**: JS has :title="$core.view.regexTooltip" showing tooltip on hover. C++ just renders ImGui::TextUnformatted("Regex Enabled") with no tooltip.

- [x] 399. [tab_item_sets.cpp] Async initialization converted to synchronous blocking calls
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 23–65
- **Status**: Pending
- **Details**: JS initialize_item_sets is async with await for progressLoadingScreen(), DBItems.ensureInitialized(), db2 getAllRows(). C++ calls all synchronously, blocking the UI thread and preventing loading screen updates.

- [ ] 400. [tab_item_sets.cpp] apply_filter converts ItemSet structs to JSON objects unnecessarily
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 67–69
- **Status**: Pending
- **Details**: JS simply assigns the array of ItemSet objects directly. C++ iterates every ItemSet, constructs nlohmann::json objects, and pushes them. render() then converts JSON back into ItemEntry structs every frame — double-conversion overhead.

- [ ] 401. [tab_item_sets.cpp] render() re-creates item_entries vector from JSON every frame
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 76–86
- **Status**: Pending
- **Details**: C++ render allocates a vector, loops over all JSON items, copies fields into ItemEntry structs, and pushes — every frame. JS template binds directly to existing objects with no per-frame allocation.

- [ ] 402. [tab_item_sets.cpp] Regex-enabled text and filter input lack proper layout container
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 81–84
- **Status**: Pending
- **Details**: JS wraps regex info and filter input inside div class="filter" providing inline layout. C++ renders them sequentially without SameLine() or horizontal group, causing "Regex Enabled" to appear above the filter input instead of beside it.

- [ ] 403. [tab_item_sets.cpp] fieldToUint32Vec does not handle single-value fields
- **JS Source**: `src/js/modules/tab_item_sets.js` line 38
- **Status**: Pending
- **Details**: JS set_row.ItemID is expected to be an array with .filter(id => id !== 0). C++ fieldToUint32Vec only handles vector variants. If a DB2 reader returns a single scalar, the function returns an empty vector, silently dropping data.


## Tab: Characters

- [ ] 404. [tab_characters.cpp] Saved-character thumbnail card rendering is replaced by a placeholder button path
- **JS Source**: `src/js/modules/tab_characters.js` lines 1928–1934
- **Status**: Pending
- **Details**: JS renders thumbnail backgrounds and overlay action buttons in `.saved-character-card`; C++ renders a generic button with a `// thumbnail placeholder` path, so card visuals and thumbnail behavior diverge.

- [ ] 405. [tab_characters.cpp] Main-screen quick-save flow skips JS thumbnail capture step
- **JS Source**: `src/js/modules/tab_characters.js` lines 1973, 2328–2333
- **Status**: Pending
- **Details**: JS quick-save button routes through `open_save_prompt()` which captures `chrPendingThumbnail` before prompting; C++ main `Save` button only toggles prompt state and does not capture a fresh thumbnail first.

- [ ] 406. [tab_characters.cpp] Outside-click handlers for import/color popups from JS mounted lifecycle are missing
- **JS Source**: `src/js/modules/tab_characters.js` lines 2668–2685
- **Status**: Pending
- **Details**: JS registers a document click listener to close color pickers and floating import panels when clicking elsewhere; C++ has no equivalent mounted/unmounted document-listener path, changing panel-dismiss behavior.

- [ ] 407. [tab_characters.cpp] import_wmv_character() is completely stubbed
- **JS Source**: `src/js/modules/tab_characters.js` lines 988–1021
- **Status**: Pending
- **Details**: JS creates a file input, reads a .chr file, passes it to wmv_parse(), and calls apply_import_data(). C++ just does return. Also missing wmv_parse import and lastWMVImportPath config persistence.

- [ ] 408. [tab_characters.cpp] import_wowhead_character() is completely stubbed
- **JS Source**: `src/js/modules/tab_characters.js` lines 1023–1044
- **Status**: Pending
- **Details**: JS validates URL contains dressing-room, calls wowhead_parse(), then apply_import_data(). C++ just does return. Also missing wowhead_parse import.

- [ ] 409. [tab_characters.cpp] Missing texture application on attachment equipment models
- **JS Source**: `src/js/modules/tab_characters.js` lines 620–622
- **Status**: Pending
- **Details**: JS applies replaceable textures to each attachment model via applyReplaceableTextures(). C++ loads the attachment model but never calls applyReplaceableTextures().

- [ ] 410. [tab_characters.cpp] Missing texture application on collection equipment models
- **JS Source**: `src/js/modules/tab_characters.js` lines 664–668
- **Status**: Pending
- **Details**: JS selects correct texture index and applies it to collection models. C++ loads the collection model but never applies textures.

- [ ] 411. [tab_characters.cpp] Missing geoset visibility for collection models
- **JS Source**: `src/js/modules/tab_characters.js` lines 652–661
- **Status**: Pending
- **Details**: JS calls renderer.hideAllGeosets() then renderer.setGeosetGroupDisplay() using display.attachmentGeosetGroup and SLOT_TO_GEOSET_GROUPS mapping. C++ has the mapping defined but never uses it.

- [ ] 412. [tab_characters.cpp] OBJ/STL export missing chr_materials URI textures geoset mask and pose application
- **JS Source**: `src/js/modules/tab_characters.js` lines 1712–1722
- **Status**: Pending
- **Details**: JS iterates chr_materials for URI textures, sets geoset mask, and applies posed geometry. C++ does none of these for OBJ/STL export.

- [ ] 413. [tab_characters.cpp] OBJ/STL/GLTF export missing CharacterExporter equipment models
- **JS Source**: `src/js/modules/tab_characters.js` lines 1725–1811
- **Status**: Pending
- **Details**: JS creates CharacterExporter, collects equipment geometry with textures/bones, and passes to exporter. C++ skips equipment model export entirely for all formats.

- [ ] 414. [tab_characters.cpp] load_character_model always sets animation to none instead of auto-selecting stand
- **JS Source**: `src/js/modules/tab_characters.js` lines 744–745
- **Status**: Pending
- **Details**: JS finds stand animation (id 0.0) and auto-selects it. C++ always sets none so the character has no animation after load.

- [ ] 415. [tab_characters.cpp] load_character_model missing on_model_rotate callback
- **JS Source**: `src/js/modules/tab_characters.js` lines 719–722
- **Status**: Pending
- **Details**: JS calls controls.on_model_rotate after loading. C++ does not invoke any rotation callback after model load.

- [ ] 416. [tab_characters.cpp] import_character does not lowercase character name in URL
- **JS Source**: `src/js/modules/tab_characters.js` line 965
- **Status**: Pending
- **Details**: JS uses encodeURIComponent(character_name.toLowerCase()). C++ uses url_encode(character_name) without lowercasing. Battle.net API may be case-sensitive.

- [ ] 417. [tab_characters.cpp] import_character error handling uses string search instead of HTTP status
- **JS Source**: `src/js/modules/tab_characters.js` lines 969–983
- **Status**: Pending
- **Details**: JS separately handles HTTP 404 vs other errors. C++ catches all exceptions and does err_msg.find(404) string search which may not match actual HTTP 404 responses.

- [ ] 418. [tab_characters.cpp] import_json_character save-to-my-characters preserves guild_tabard (JS does not)
- **JS Source**: `src/js/modules/tab_characters.js` lines 1545–1553
- **Status**: Pending
- **Details**: JS save_data only includes race_id, model_id, choices, equipment. C++ also copies guild_tabard if present. Behavioral deviation from JS.

- [ ] 419. [tab_characters.cpp] Missing getEquipmentRenderers and getCollectionRenderers callbacks on viewer context
- **JS Source**: `src/js/modules/tab_characters.js` lines 2608–2609
- **Status**: Pending
- **Details**: JS context includes getEquipmentRenderers and getCollectionRenderers callbacks. C++ only sets getActiveRenderer. Equipment models may not render in the viewport.

- [ ] 420. [tab_characters.cpp] Equipment slot items missing quality color styling
- **JS Source**: `src/js/modules/tab_characters.js` line 2192
- **Status**: Pending
- **Details**: JS applies item-quality-X CSS class based on item quality. C++ renders equipment item names as plain text without quality-based coloring.

- [ ] 421. [tab_characters.cpp] navigate_to_items_for_slot missing type mask filtering
- **JS Source**: `src/js/modules/tab_characters.js` lines 2400–2414
- **Status**: Pending
- **Details**: JS sets itemViewerTypeMask to filter items tab by slot name. C++ just calls tab_items::setActive() with no filtering.

- [ ] 422. [tab_characters.cpp] Saved characters grid missing thumbnail rendering
- **JS Source**: `src/js/modules/tab_characters.js` lines 1928–1935
- **Status**: Pending
- **Details**: JS renders actual thumbnail images via backgroundImage CSS. C++ uses ImGui::Button with no image rendering.

- [ ] 423. [tab_characters.cpp] Texture preview panel is placeholder text
- **JS Source**: `src/js/modules/tab_characters.js` lines 2121–2123
- **Status**: Pending
- **Details**: JS has a DOM node where charTextureOverlay attaches canvas layers. C++ has ImGui::Text placeholder.

- [ ] 424. [tab_characters.cpp] Color picker uses ImGui Tooltip instead of positioned popup
- **JS Source**: `src/js/modules/tab_characters.js` lines 2051–2068
- **Status**: Pending
- **Details**: JS positions the color picker popup at event.clientX/Y with absolute CSS positioning. C++ uses ImGui::BeginTooltip() which follows the mouse cursor.

- [ ] 425. [tab_characters.cpp] Missing document click handler for dismissing panels
- **JS Source**: `src/js/modules/tab_characters.js` lines 2668–2685
- **Status**: Pending
- **Details**: JS adds a click listener that closes color pickers and import panels when clicking outside. C++ does not explicitly handle clicking outside these panels.

- [ ] 426. [tab_characters.cpp] Missing unmounted() cleanup
- **JS Source**: `src/js/modules/tab_characters.js` lines 2699–2701
- **Status**: Pending
- **Details**: JS has unmounted() calling reset_module_state() for cleanup when tab is destroyed. C++ does not export an unmounted function.


## Tab: Creatures & Decor

- [ ] 427. [tab_creatures.cpp] Creature list context actions are not equivalent to JS copy-name/copy-ID menu
- **JS Source**: `src/js/modules/tab_creatures.js` lines 983–986, 1179–1203
- **Status**: Pending
- **Details**: JS creature list context menu exposes only `Copy name(s)` and `Copy ID(s)` handlers; C++ delegates to generic `listbox_context::handle_context_menu(...)`, changing the context action contract from the original creature-specific menu.

- [ ] 428. [tab_creatures.cpp] has_content check and toast/camera logic scoped incorrectly
- **JS Source**: `src/js/modules/tab_creatures.js` lines 713–722
- **Status**: Pending
- **Details**: In JS, the has_content check, hideToast, and fitCamera are outside the if/else running for both character and standard models. In C++ this block is inside the else (standard-model only). For character-model creatures, the loading toast is never dismissed.

- [ ] 429. [tab_creatures.cpp] Collection model geoset logic has three bugs
- **JS Source**: `src/js/modules/tab_creatures.js` lines 421–429
- **Status**: Pending
- **Details**: (1) JS calls hideAllGeosets() before applying - C++ never does. (2) JS uses mapping.group_index for lookup - C++ uses coll_idx. (3) JS uses mapping.char_geoset for setGeosetGroupDisplay - C++ uses mapping.group_index.

- [ ] 430. [tab_creatures.cpp] Scrubber IsItemActivated() called before SliderInt checks wrong widget
- **JS Source**: `src/js/modules/tab_creatures.js` lines 1035–1038
- **Status**: Pending
- **Details**: C++ calls IsItemActivated() before SliderInt() renders. IsItemActivated checks the last widget (Step-Right button) not the slider. start_scrub() will never fire correctly.

- [ ] 431. [tab_creatures.cpp] Missing export_paths.writeLine calls in multiple export paths
- **JS Source**: `src/js/modules/tab_creatures.js` lines 747, 792, 923
- **Status**: Pending
- **Details**: JS writes to export_paths for RAW and non-RAW character-model export and standard export. C++ omits all writeLine calls and does not pass export_paths to export_model.

- [ ] 432. [tab_creatures.cpp] GLTF format.toLowerCase() not applied
- **JS Source**: `src/js/modules/tab_creatures.js` line 921
- **Status**: Pending
- **Details**: JS passes format.toLowerCase() to exportAsGLTF. C++ passes format as-is (uppercase). If the exporter is case-sensitive output may differ.

- [ ] 433. [tab_creatures.cpp] Error toast for model load missing View Log action button
- **JS Source**: `src/js/modules/tab_creatures.js` line 728
- **Status**: Pending
- **Details**: JS passes View Log action. C++ passes empty map. User cannot open runtime log from this error.

- [ ] 434. [tab_creatures.cpp] path.basename behavior not replicated in skin name
- **JS Source**: `src/js/modules/tab_creatures.js` line 668
- **Status**: Pending
- **Details**: Node.js path.basename produces trailing dot. C++ strips full .m2 extension producing no dot. Skin name stripping matches different substrings producing different display labels.

- [ ] 435. [tab_creatures.cpp] Missing Regex Enabled indicator in filter bar
- **JS Source**: `src/js/modules/tab_creatures.js` line 989
- **Status**: Pending
- **Details**: JS shows Regex Enabled div with tooltip when regex filters are active. C++ filter bar has no indicator.

- [ ] 436. [tab_creatures.cpp] Listbox context menu Copy names and Copy IDs not rendered in UI
- **JS Source**: `src/js/modules/tab_creatures.js` lines 983–986
- **Status**: Pending
- **Details**: JS renders ContextMenu with Copy names and Copy IDs options. C++ does not render an ImGui context menu popup. The functions exist but are never invoked from the UI.

- [ ] 437. [tab_creatures.cpp] Sorting uses byte comparison instead of locale-aware localeCompare
- **JS Source**: `src/js/modules/tab_creatures.js` line 1161
- **Status**: Pending
- **Details**: JS uses localeCompare. C++ uses name_a < name_b. Creatures with diacritics may sort differently.

- [ ] 438. [tab_decor.cpp] PNG/CLIPBOARD export branch does not short-circuit like JS
- **JS Source**: `src/js/modules/tab_decor.js` lines 129–140
- **Status**: Pending
- **Details**: JS returns immediately after preview export for PNG/CLIPBOARD; C++ closes the export stream but continues into full model export logic, changing export behavior for these formats.

- [ ] 439. [tab_decor.cpp] Decor list context menu open/interaction path differs from JS ContextMenu component flow
- **JS Source**: `src/js/modules/tab_decor.js` lines 234–237
- **Status**: Pending
- **Details**: JS renders a dedicated ContextMenu node for listbox selections (`Copy name(s)` / `Copy file data ID(s)`); C++ uses a manual popup path without equivalent Vue component lifecycle/wiring, deviating from original interaction flow.

- [ ] 440. [tab_decor.cpp] Missing return after PNG/CLIPBOARD export branch falls through
- **JS Source**: `src/js/modules/tab_decor.js` lines 138–140
- **Status**: Pending
- **Details**: JS does return after PNG/CLIPBOARD block. C++ has no return so execution falls through to the ExportHelper loop redundantly exporting all selected entries as models.

- [ ] 441. [tab_decor.cpp] create_renderer receives file_data_id instead of file_name
- **JS Source**: `src/js/modules/tab_decor.js` line 85
- **Status**: Pending
- **Details**: JS passes file_name (string) as 5th argument to create_renderer. C++ passes file_data_id (uint32_t). Parameter type mismatch.

- [ ] 442. [tab_decor.cpp] getActiveRenderer() only returns M2 renderer not any active renderer
- **JS Source**: `src/js/modules/tab_decor.js` line 611
- **Status**: Pending
- **Details**: JS getActiveRenderer returns the single active_renderer which could be M2, WMO, or M3. C++ returns active_renderer_result.m2.get() returning nullptr when active model is WMO or M3.

- [ ] 443. [tab_decor.cpp] Error toast for preview_decor missing View Log action button
- **JS Source**: `src/js/modules/tab_decor.js` line 119
- **Status**: Pending
- **Details**: JS includes View Log action. C++ passes empty map.

- [ ] 444. [tab_decor.cpp] helper.mark on failure missing stack trace parameter
- **JS Source**: `src/js/modules/tab_decor.js` line 184
- **Status**: Pending
- **Details**: JS passes e.message and e.stack. C++ only passes e.what().

- [ ] 445. [tab_decor.cpp] Sorting uses byte comparison instead of locale-aware localeCompare
- **JS Source**: `src/js/modules/tab_decor.js` lines 401–405
- **Status**: Pending
- **Details**: JS uses localeCompare. C++ uses name_a < name_b after tolower. Different sort for non-ASCII names.

- [ ] 446. [tab_decor.cpp] Missing scrub pause/resume behavior on animation slider
- **JS Source**: `src/js/modules/tab_decor.js` lines 546–561
- **Status**: Pending
- **Details**: JS start_scrub saves paused state and pauses animation while dragging. C++ SliderInt has no mouse-down/up event handling.

- [ ] 447. [tab_decor.cpp] Missing Regex Enabled indicator in filter bar
- **JS Source**: `src/js/modules/tab_decor.js` line 240
- **Status**: Pending
- **Details**: JS shows Regex Enabled div above filter input. C++ filter bar has no such indicator.

- [ ] 448. [tab_decor.cpp] Missing tooltips on all sidebar Preview and Export checkboxes
- **JS Source**: `src/js/modules/tab_decor.js` lines 314–354
- **Status**: Pending
- **Details**: JS has title attributes for every checkbox. C++ has no tooltip calls for any of them.

- [ ] 449. [tab_decor.cpp] Category group header click-to-toggle-all not implemented
- **JS Source**: `src/js/modules/tab_decor.js` line 301
- **Status**: Pending
- **Details**: JS clicking category name toggles all subcategories on/off. C++ uses TreeNodeEx which only expands/collapses. The toggle function exists but is never called from render.

- [ ] 450. [tab_decor.cpp] WMO Groups and Doodad Sets use manual checkbox loop instead of CheckboxList
- **JS Source**: `src/js/modules/tab_decor.js` lines 363–369
- **Status**: Pending
- **Details**: JS uses Checkboxlist component for both. C++ uses manual ImGui::Checkbox loops instead of checkboxlist::render(). Inconsistent and may cause visual differences.

- [ ] 451. [tab_decor.cpp] Context menu popup may never open
- **JS Source**: `src/js/modules/tab_decor.js` lines 233–237
- **Status**: Pending
- **Details**: The DecorListboxContextMenu popup requires ImGui::OpenPopup to be called. The handle_listbox_context callback does not open this popup. The popup rendering code will never trigger.


## 3D Engine, File Loaders & Core Systems

- [x] 452. [blp.cpp] WebP/PNG save methods are synchronous instead of JS async Promise APIs
- **JS Source**: `src/js/casc/blp.js` lines 146–194
- **Status**: Pending
- **Details**: JS implements `async saveToPNG`, `async toWebP`, and `async saveToWebP`. C++ equivalents are synchronous, changing completion/error semantics for consumers expecting Promise-based behavior.

- [ ] 453. [blp.cpp] `toBuffer()` fallback differs for unknown encodings
- **JS Source**: `src/js/casc/blp.js` lines 242–250
- **Status**: Pending
- **Details**: JS has no default branch and therefore returns `undefined` for unsupported encodings. C++ returns an empty `BufferWrapper`, changing caller-observed fallback behavior.

- [ ] 454. [blp.cpp] `toCanvas()` and `drawToCanvas()` methods not ported — browser-specific
- **JS Source**: `src/js/casc/blp.js` lines 103–117, 221–234
- **Status**: Pending
- **Details**: JS `toCanvas()` creates an HTML `<canvas>` element and draws the BLP onto it. `drawToCanvas()` takes an existing canvas and draws the BLP pixels using 2D context methods (`createImageData`, `putImageData`). These are browser-specific APIs with no C++ equivalent. The C++ port replaces these with `toPNG()`, `toBuffer()`, and `toUInt8Array()` which provide the same pixel data without canvas.

- [ ] 455. [blp.cpp] `dataURL` property initialized to `null` in JS constructor, C++ uses `std::optional<std::string>`
- **JS Source**: `src/js/casc/blp.js` line 86
- **Status**: Pending
- **Details**: JS sets `this.dataURL = null` in the BLPImage constructor. C++ declares `std::optional<std::string> dataURL` in the header which defaults to `std::nullopt`. The C++ `getDataURL()` method doesn't cache to this field (it relies on `BufferWrapper::getDataURL()` caching instead). The JS `getDataURL()` also doesn't set this field — it returns from `toCanvas().toDataURL()`. The `dataURL` field appears to be unused caching infrastructure in both versions.

- [ ] 456. [blp.cpp] `toWebP()` uses libwebp C API directly instead of JS `webp-wasm` module
- **JS Source**: `src/js/casc/blp.js` lines 157–182
- **Status**: Pending
- **Details**: JS uses `webp-wasm` npm module with `webp.encode(imgData, options)` for WebP encoding. C++ uses libwebp's C API directly (`WebPEncodeLosslessRGBA` / `WebPEncodeRGBA`). The JS `options` object `{ lossless: true }` or `{ quality: N }` maps to C++ separate code paths for quality == 100 (lossless) vs lossy. Functionally equivalent.

- [ ] 457. [Shaders.cpp] C++ adds automatic _unregister_fn callback on ShaderProgram not present in JS
- **JS Source**: `src/js/3D/Shaders.js` lines 56–72
- **Status**: Pending
- **Details**: C++ `create_program()` (Shaders.cpp lines 79–83) installs a static `_unregister_fn` callback on `gl::ShaderProgram` that automatically calls `shaders::unregister()` when a ShaderProgram is destroyed. JS has no equivalent auto-cleanup mechanism — callers must explicitly call `unregister(program)` (Shaders.js line 78–86). This means in C++, a program is automatically removed from `active_programs` on destruction, while in JS a disposed program remains in the set until manually unregistered. This changes `reload_all()` behavior: JS could attempt to recompile stale programs that were not explicitly unregistered, while C++ never encounters this scenario.

- [x] 458. [Texture.cpp] `getTextureFile()` return contract differs from JS async/null behavior
- **JS Source**: `src/js/3D/Texture.js` lines 32–41
- **Status**: Pending
- **Details**: JS returns a Promise from `async getTextureFile()` and yields `null` when unset; C++ returns `std::optional<BufferWrapper>` synchronously, changing both async behavior and API shape.

- [x] 459. [Skin.cpp] `load()` API timing differs from JS Promise-based async flow
- **JS Source**: `src/js/3D/Skin.js` lines 20–23, 96–100
- **Status**: Pending
- **Details**: JS exposes `async load()` and awaits CASC file retrieval (`await core.view.casc.getFile(...)`), while C++ `Skin::load()` is synchronous and throws directly, changing caller timing/error-propagation semantics.

- [ ] 460. [MultiMap.cpp] MultiMap logic is not ported in the `.cpp` sibling translation unit
- **JS Source**: `src/js/MultiMap.js` lines 6–32
- **Status**: Pending
- **Details**: The JS sibling contains the full `MultiMap extends Map` implementation, but `src/js/MultiMap.cpp` only includes `MultiMap.h` and comments; line-by-line implementation parity is not present in the `.cpp` file itself.

- [ ] 461. [MultiMap.cpp] Public API model differs from JS `Map` subclass contract
- **JS Source**: `src/js/MultiMap.js` lines 6, 20–28, 32
- **Status**: Pending
- **Details**: JS exports an actual `Map` subclass with standard `Map` behavior/interop, while C++ exposes a template wrapper (header implementation) returning `std::variant` pointers and not `Map`-equivalent runtime semantics.

- [ ] 462. [M2Generics.cpp] Error message text differs in useAnims branch ("Unhandled" vs "Unknown")
- **JS Source**: `src/js/3D/loaders/M2Generics.js` lines 78, 101
- **Status**: Pending
- **Details**: JS `read_m2_array_array` has two separate switch blocks — the useAnims branch (line 78) throws `"Unhandled data type: ${dataType}"` while the non-useAnims branch (line 101) throws `"Unknown data type: ${dataType}"`. C++ collapses both branches into a single `read_value()` helper that always throws `"Unknown data type: "` for both paths. The error message for the useAnims branch differs from the original JS.

- [x] 463. [M3Loader.cpp] Loader methods are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/M3Loader.js` lines 67, 104, 269, 277, 299, 315
- **Status**: Pending
- **Details**: JS exposes async `load`, `parseChunk_M3DT`, and async sub-chunk parsers; C++ ports these paths as synchronous calls, changing API timing/await semantics.

- [x] 464. [MDXLoader.cpp] `load` API is synchronous instead of JS async Promise-based method
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` line 28
- **Status**: Pending
- **Details**: JS exposes `async load()` while C++ exposes synchronous `void load()`, changing await/timing behavior.

- [ ] 465. [MDXLoader.cpp] ATCH handler fixes JS `readUInt32LE(-4)` bug without TODO_TRACKER documentation
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` line 404
- **Status**: Pending
- **Details**: JS ATCH handler has `this.data.readUInt32LE(-4)` which is a bug — `BufferWrapper._readInt` passes `_checkBounds(-16)` (always passes since remainingBytes >= 0 > -16), but `new Array(-4)` throws a `RangeError`. C++ correctly fixes this by using a saved `attachmentSize` variable. The fix has a code comment but per project conventions, deviations from the original JS should also be tracked in TODO_TRACKER.md.

- [ ] 466. [MDXLoader.cpp] Node registration deferred to post-parsing (structural deviation)
- **JS Source**: `src/js/3D/loaders/MDXLoader.js` lines 208–209
- **Status**: Pending
- **Details**: In JS, `_read_node()` immediately assigns `this.nodes[node.objectId] = node` (line 209). In C++, this is deferred to `load()` because objects are moved into their final vectors after `_read_node` returns, invalidating any earlier pointers. This is correctly documented with a code comment and is functionally equivalent — all 9 node-bearing types (bones, helpers, attachments, eventObjects, hitTestShapes, particleEmitters, particleEmitters2, lights, ribbonEmitters) are properly registered. This is a structural deviation that should be tracked.

- [x] 467. [SKELLoader.cpp] Loader animation APIs are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 36, 308, 407
- **Status**: Pending
- **Details**: JS exposes async `load`, `loadAnimsForIndex`, and `loadAnims`; C++ ports all three as synchronous methods, altering call/await behavior.

- [ ] 468. [SKELLoader.cpp] Animation-load failure handling differs from JS
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 332–344, 438–448
- **Status**: Pending
- **Details**: JS does not catch ANIM/CASC load failures in `loadAnimsForIndex`/`loadAnims` (Promise rejects). C++ catches exceptions, logs, and returns/continues, changing failure propagation.

- [ ] 469. [SKELLoader.cpp] Extra bounds check in `loadAnimsForIndex()` not present in JS
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 308–312
- **Status**: Pending
- **Details**: C++ adds `if (animation_index >= this->animations.size()) return false;` that does not exist in JS. In JS, accessing an out-of-bounds index on `this.animations` returns `undefined`, and `animation.flags` would throw a TypeError. C++ silently returns false instead of throwing, changing error behavior.

- [ ] 470. [SKELLoader.cpp] `skeletonBoneData` existence check uses `.empty()` instead of `!== undefined`
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 335–338, 441–444
- **Status**: Pending
- **Details**: JS checks `loader.skeletonBoneData !== undefined` — the property only exists if a SKID chunk was parsed. C++ checks `!loader->skeletonBoneData.empty()`. If ANIMLoader ever sets `skeletonBoneData` to a valid but empty buffer, JS would use it (property exists), but C++ would skip it (empty). This is a potential semantic difference depending on ANIMLoader behavior.

- [ ] 471. [SKELLoader.cpp] `loadAnims()` doesn't guard against missing `animFileIDs` like `loadAnimsForIndex()` does
- **JS Source**: `src/js/3D/loaders/SKELLoader.js` lines 319, 425
- **Status**: Pending
- **Details**: JS `loadAnimsForIndex()` has `if (!this.animFileIDs) return false;` (line 319) to guard against undefined `animFileIDs`. However, JS `loadAnims()` does NOT have this guard — it directly iterates `this.animFileIDs` (line 425), which would throw a TypeError if undefined. In C++, `animFileIDs` is always a default-constructed empty vector, so the for-loop is a no-op. The C++ is more robust but produces different behavior (graceful no-op vs JS crash).

- [x] 472. [BONELoader.cpp] `load` API is synchronous instead of JS async Promise-based method
- **JS Source**: `src/js/3D/loaders/BONELoader.js` line 24
- **Status**: Pending
- **Details**: JS exposes `async load()` while C++ exposes synchronous `void load()`, changing API timing/await semantics.

- [x] 473. [ANIMLoader.cpp] `load` API is synchronous instead of JS async Promise-based method
- **JS Source**: `src/js/3D/loaders/ANIMLoader.js` line 25
- **Status**: Pending
- **Details**: JS exposes `async load(isChunked = true)` while C++ exposes synchronous `void load(bool isChunked)`, changing API timing/await semantics.

- [x] 474. [WMOLoader.cpp] `load`/`getGroup` APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 37, 64
- **Status**: Pending
- **Details**: JS exposes async `load()` and `getGroup(index)` while C++ ports both as synchronous methods, changing await/timing behavior.

- [ ] 475. [WMOLoader.cpp] MOGP `flags` field renamed to `groupFlags`, diverging from JS property name
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` line 361
- **Status**: Pending
- **Details**: JS MOGP handler stores `this.flags = data.readUInt32LE()`. C++ stores this as `this->groupFlags` (header line 218, MOGP parser line 426) because the C++ class already has `uint16_t flags` from MOHD. Any downstream code porting JS that accesses `wmoGroup.flags` for MOGP flags must use `groupFlags` in C++. This naming deviation matches the same issue found in WMOLegacyLoader.cpp (entry 376).

- [ ] 476. [WMOLoader.cpp] `hasLiquid` boolean is a C++ addition not present in JS
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 328–338
- **Status**: Pending
- **Details**: JS simply assigns `this.liquid = { ... }` in the MLIQ handler. Consumer code checks `if (this.liquid)` for existence. In C++, the `WMOLiquid liquid` member is always default-constructed, so a `bool hasLiquid = false` flag (header line 209) was added and set to `true` in `parse_MLIQ`. This is a reasonable C++ adaptation, but all downstream JS code that checks `if (this.liquid)` must be ported to check `if (this.hasLiquid)` instead — all consumers need verification.

- [ ] 477. [WMOLoader.cpp] MOPR filler skip uses `data.move(4)` but per wowdev.wiki entry is 8 bytes total
- **JS Source**: `src/js/3D/loaders/WMOLoader.js` lines 208–216
- **Status**: Pending
- **Details**: MOPR entry count is calculated as `chunkSize / 8` (8 bytes per entry). Fields read: `portalIndex(2) + groupIndex(2) + side(2)` = 6 bytes, then `data.move(4)` skips 4 more = 10 bytes per entry. Per wowdev.wiki, `SMOPortalRef` has a 2-byte `filler` (uint16_t), making entries 8 bytes. `data.move(2)` would be correct, not `data.move(4)`. Both JS and C++ match (C++ faithfully ports the JS), but both overread by 2 bytes per entry. The outer `data.seek(nextChunkPos)` corrects the position so parsing doesn't break, but this is a latent bug in both codebases.

- [x] 478. [WMOLegacyLoader.cpp] `load`/internal load helpers/`getGroup` are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 33, 54, 86, 116
- **Status**: Pending
- **Details**: JS uses async `load`, `_load_alpha_format`, `_load_standard_format`, and `getGroup`; C++ ports these paths synchronously, changing await/timing behavior.

- [ ] 479. [WMOLegacyLoader.cpp] Group-loader initialization differs from JS in `getGroup`
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 146–149
- **Status**: Pending
- **Details**: JS creates group loaders with `fileID` undefined and explicitly seeds `group.version = this.version` before `await group.load()`. C++ does not pre-seed `version`, changing legacy group parse assumptions.

- [ ] 480. [WMOLegacyLoader.cpp] MOGP `flags` field renamed to `groupFlags`, diverging from JS property name
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` line 453
- **Status**: Pending
- **Details**: JS MOGP handler stores `this.flags = data.readUInt32LE()`. C++ stores this as `this->groupFlags` (header line 124, MOGP parser line 527) because the C++ class already has `uint16_t flags` from MOHD. Any downstream JS-ported code accessing `group.flags` for MOGP flags must use `group.groupFlags` in C++, which is a naming deviation that could cause porting bugs.

- [ ] 481. [WMOLegacyLoader.cpp] `getGroup` empty-check differs for `groupCount == 0` edge case
- **JS Source**: `src/js/3D/loaders/WMOLegacyLoader.js` lines 117–118
- **Status**: Pending
- **Details**: JS checks `if (!this.groups)` — tests whether the `groups` property was ever set (by MOHD handler). An empty JS array `new Array(0)` is truthy, so `!this.groups` is false when `groupCount == 0` — `getGroup` proceeds to the index check. C++ uses `if (this->groups.empty())` which returns true for `groupCount == 0`, incorrectly throwing the exception. A separate bool flag (e.g., `groupsInitialized`) would replicate JS semantics more faithfully.

- [ ] 482. [WDTLoader.cpp] `MWMO` string null handling differs from JS
- **JS Source**: `src/js/3D/loaders/WDTLoader.js` line 86
- **Status**: Pending
- **Details**: JS uses `.replace('\0', '')` (first match only), while C++ removes all `'\0'` bytes from the string, producing different `worldModel` values in edge cases.

- [ ] 483. [WDTLoader.cpp] `worldModelPlacement`/`worldModel`/MPHD fields not optional — cannot distinguish "chunk absent" from "chunk with zeros"
- **JS Source**: `src/js/3D/loaders/WDTLoader.js` lines 52–103
- **Status**: Pending
- **Details**: In JS, `this.worldModelPlacement` is only assigned when MODF is encountered. If MODF is absent, the property is `undefined` and `if (wdt.worldModelPlacement)` is false. In C++, `WDTWorldModelPlacement worldModelPlacement` is always default-constructed with zeroed fields, making it impossible to distinguish "MODF absent" from "MODF with zeros." Same for `worldModel` (always empty string vs. JS `undefined`) and MPHD fields (always 0 vs. JS `undefined`). Consider `std::optional<T>` for these fields.

- [ ] 484. [ADTExporter.cpp] `calculateUVBounds` skips chunks when `vertices` is empty, unlike JS truthiness check
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 267–268
- **Status**: Pending
- **Details**: JS only skips when `chunk`/`chunk.vertices` is missing; an empty typed array is still truthy and processing continues. C++ adds `chunk.vertices.empty()` as an additional skip condition, changing edge-case behavior.

- [x] 485. [ADTExporter.cpp] Export API flow is synchronous instead of JS Promise-based `async export()`
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 309–367
- **Status**: Pending
- **Details**: JS `export()` is asynchronous and yields between CASC/file operations; C++ `exportTile()` performs the flow synchronously, changing timing/cancellation behavior relative to the original async path.

- [ ] 486. [ADTExporter.cpp] Scale factor check `!= 0` instead of `!== undefined` changes behavior for scale=0
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` line 1270
- **Status**: Pending
- **Details**: JS checks `model.scale !== undefined ? model.scale / 1024 : 1`. C++ (ADTExporter.cpp ~line 1521) checks `model.scale != 0 ? model.scale / 1024.0f : 1.0f`. In JS, a `scale` of `0` would produce `0 / 1024 = 0` (a valid zero-scale value). In C++, a `scale` of `0` triggers the else branch and returns `1.0f`. This is a behavioral difference — a model with scale=0 would be invisible in JS but normal-sized in C++, affecting M2 doodad CSV export and placement transforms.

- [ ] 487. [ADTExporter.cpp] GL index buffer uses GL_UNSIGNED_INT (uint32) instead of JS GL_UNSIGNED_SHORT (uint16)
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1117–1118
- **Status**: Pending
- **Details**: JS creates `new Uint16Array(indices)` and uses `gl.UNSIGNED_SHORT` for the index element buffer when rendering alpha map tiles. C++ (ADTExporter.cpp ~lines 1327–1328) uses `sizeof(uint32_t)` and `GL_UNSIGNED_INT`. For the 16×16×145 = 37120 vertex terrain grid the indices fit in uint16, so both work, but the GPU draws with different index types. This is a minor fidelity deviation in the GL pipeline even though the visual output is identical.

- [ ] 488. [ADTExporter.cpp] Liquid JSON serialization uses explicit fields instead of JS spread operator
- **JS Source**: `src/js/3D/exporters/ADTExporter.js` lines 1428–1438
- **Status**: Pending
- **Details**: JS uses `{ ...chunk, instances: enhancedInstances }` and `{ ...instance, worldPosition, terrainChunkPosition }` which copies *all* fields from the chunk/instance objects via the spread operator. C++ (ADTExporter.cpp ~lines 1744–1780) manually enumerates specific fields for JSON serialization. If the ADTLoader's liquid chunk or instance structs have any additional fields not listed in the C++ serialization, those fields would appear in the JS JSON output but be missing in the C++ output. This is a fragile pattern that could silently omit data if new fields are added to the loader structs.

- [ ] 489. [ADTExporter.cpp] STB_IMAGE_RESIZE_IMPLEMENTATION defined at file scope risks ODR violation
- **JS Source**: N/A (C++ build concern)
- **Status**: Pending
- **Details**: ADTExporter.cpp (line 10) defines `#define STB_IMAGE_RESIZE_IMPLEMENTATION` before including `<stb_image_resize2.h>`. If any other translation unit in the project also defines this macro, the linker will encounter duplicate symbol definitions (ODR violation). STB implementation macros should typically be isolated in a single dedicated .cpp file (like stb-impl.cpp already exists for stb_image/stb_image_write) to avoid this risk.

- [ ] 490. [WMOShaderMapper.cpp] Pixel shader enum naming deviates from JS export contract
- **JS Source**: `src/js/3D/WMOShaderMapper.js` lines 35, 90, 94
- **Status**: Pending
- **Details**: JS exports `WMOPixelShader.MapObjParallax`, while C++ renames this constant to `MapObjParallax_PS`; numeric mapping is preserved but exported identifier parity differs from the original module.

- [x] 491. [CharMaterialRenderer.cpp] Core renderer methods are synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 49, 105, 114, 170, 189, 231, 282
- **Status**: Pending
- **Details**: JS defines `init`, `reset`, `setTextureTarget`, `loadTexture`, `loadTextureFromBLP`, `compileShaders`, and `update` as async/await flows. C++ ports these methods synchronously, changing timing/error-propagation behavior expected by async call sites.

- [ ] 492. [CharMaterialRenderer.cpp] `getCanvas()` method missing — JS returns `this.glCanvas` for external use
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 55–59
- **Status**: Pending
- **Details**: JS `getCanvas()` returns the canvas element so external code can access the rendered character material texture. C++ has no equivalent method. Any code that calls `getCanvas()` will fail.

- [ ] 493. [CharMaterialRenderer.cpp] `update()` draw call placement differs — C++ draws inside blend-mode conditional instead of after it
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 382–417
- **Status**: Pending
- **Details**: JS draws ONCE per layer at line 417 (`this.gl.drawArrays(this.gl.TRIANGLES, 0, 6)`) OUTSIDE the blend-mode 4/6/7 if block. C++ has the draw call INSIDE the if block (for blend modes 4/6/7) at line ~534 AND inside the else block at line ~543. This means the draw happens in both branches but the pre-draw setup is different, which could lead to incorrect rendering for certain blend modes.

- [ ] 494. [CharMaterialRenderer.cpp] `setTextureTarget()` signature completely changed — JS takes full objects, C++ takes individual scalar parameters
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 114–144
- **Status**: Pending
- **Details**: JS signature is `setTextureTarget(chrCustomizationMaterial, charComponentTextureSection, chrModelMaterial, chrModelTextureLayer, useAlpha, blpOverride)` receiving full objects. C++ takes individual fields: `setTextureTarget(chrModelTextureTargetID, fileDataID, sectionX, sectionY, sectionWidth, sectionHeight, materialTextureType, materialWidth, materialHeight, textureLayerBlendMode, useAlpha, blpOverride)`. If JS objects contain additional fields used downstream, C++ will lose them.

- [ ] 495. [CharMaterialRenderer.cpp] `clearCanvas()` binds/unbinds FBO in C++ but JS does not
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` lines 218–225
- **Status**: Pending
- **Details**: JS `clearCanvas()` operates on the current WebGL framebuffer (the canvas) without explicit bind/unbind. C++ explicitly binds `fbo_` before clearing and unbinds after. This is architecturally correct for desktop GL but represents a behavioral difference if called while another FBO is bound.

- [ ] 496. [CharMaterialRenderer.cpp] `dispose()` missing WebGL context loss equivalent
- **JS Source**: `src/js/3D/renderers/CharMaterialRenderer.js` line 160
- **Status**: Pending
- **Details**: JS calls `gl.getExtension('WEBGL_lose_context').loseContext()` to invalidate all WebGL resources at once. C++ manually deletes each GL resource individually (FBO, textures, depth buffer, VAO). The C++ approach is correct for desktop GL but the order and completeness of cleanup should be verified.

- [ ] 497. [CharacterExporter.cpp] `get_item_id_for_slot` does not preserve JS falsy fallback semantics
- **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 342–345
- **Status**: Pending
- **Details**: JS uses `a || b || null`, so a slot `item_id` of `0` falls through to collection/null. C++ returns the first found `item_id` directly (including `0`), which differs for falsy-ID edge cases.

- [ ] 498. [CharacterExporter.cpp] remap_bone_indices truncates remap_table.size() to uint8_t causing incorrect comparison
- **JS Source**: `src/js/3D/exporters/CharacterExporter.js` lines 126–138
- **Status**: Pending
- **Details**: C++ `remap_bone_indices()` (CharacterExporter.cpp line 147) compares `original_idx < static_cast<uint8_t>(remap_table.size())`. If `remap_table` has 256 or more entries, `static_cast<uint8_t>(256)` wraps to `0`, making the comparison `original_idx < 0` always false for unsigned types — no indices would be remapped at all. For tables with 257–511 entries, the truncated size wraps to small values, skipping valid remap entries for higher indices. JS has no such issue since `original_idx < remap_table.length` uses normal number comparison. The fix should be `static_cast<size_t>(original_idx) < remap_table.size()` or simply removing the cast.

- [x] 499. [M3RendererGL.cpp] Load APIs are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 56, 76
- **Status**: Pending
- **Details**: JS defines async `load` and `loadLOD`; C++ ports both as synchronous calls, changing await/timing semantics.

- [ ] 500. [M3RendererGL.cpp] `getBoundingBox()` missing vertex array empty check
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` lines 174–175
- **Status**: Pending
- **Details**: JS checks `if (!this.m3 || !this.m3.vertices) return null`. C++ only checks `if (!m3) return std::nullopt` at line 198–199 without checking if vertices array is empty. If m3 is loaded but vertices array is empty, C++ will attempt bounding box calculation on empty data.

- [ ] 501. [M3RendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/M3RendererGL.js` line 214
- **Status**: Pending
- **Details**: Same issue as M2RendererGL/M2LegacyRendererGL — C++ uses `std::chrono::steady_clock` elapsed time (lines 242–246) instead of `performance.now() * 0.001`.

- [x] 502. [MDXRendererGL.cpp] Load and texture/animation paths are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 174, 200, 407
- **Status**: Pending
- **Details**: JS uses async `load`, `_load_textures`, and `playAnimation`; C++ ports these paths synchronously, changing asynchronous control flow and failure timing.

- [ ] 503. [MDXRendererGL.cpp] Skeleton node flattening changes JS undefined/NaN behavior for `objectId`
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 256–264
- **Status**: Pending
- **Details**: JS compares raw `nodes[i].objectId` and can propagate undefined/NaN semantics. C++ uses `std::optional<int>` checks and skips undefined IDs, which changes edge-case matrix-index behavior from JS.

- [ ] 504. [MDXRendererGL.cpp] Reactive watchers not set up — `geosetWatcher` and `wireframeWatcher` completely missing
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 187–188
- **Status**: Pending
- **Details**: JS `load()` sets up Vue watchers: `this.geosetWatcher = core.view.$watch(this.geosetKey, () => this.updateGeosets(), { deep: true })` and `this.wireframeWatcher = core.view.$watch('config.modelViewerWireframe', () => {}, { deep: true })`. C++ completely omits these watchers. Comment at lines 228–229 states "polling is handled in render()." but no polling code exists.

- [ ] 505. [MDXRendererGL.cpp] `dispose()` missing watcher cleanup calls
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 780–781
- **Status**: Pending
- **Details**: JS `dispose()` calls `this.geosetWatcher?.()` and `this.wireframeWatcher?.()`. C++ has no equivalent cleanup because watchers were never created.

- [ ] 506. [MDXRendererGL.cpp] `_create_skeleton()` doesn't initialize `node_matrices` to identity when nodes are empty
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 252
- **Status**: Pending
- **Details**: JS sets `this.node_matrices = new Float32Array(16)` which creates a zero-filled 16-element array (single identity-sized buffer). C++ does `node_matrices.resize(16)` at line 313 which leaves elements uninitialized. Should zero-initialize or set to identity to match JS behavior.

- [ ] 507. [MDXRendererGL.cpp] `u_time` uniform calculation uses relative time instead of `performance.now()`
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 681
- **Status**: Pending
- **Details**: Same issue as other renderers — C++ uses elapsed time from first render call instead of `performance.now() * 0.001`.

- [ ] 508. [MDXRendererGL.cpp] Interpolation constants `INTERP_NONE/LINEAR/HERMITE/BEZIER` defined but never used in either JS or C++
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` lines 27–30
- **Status**: Pending
- **Details**: Both files define `INTERP_NONE=0`, `INTERP_LINEAR=1`, `INTERP_HERMITE=2`, `INTERP_BEZIER=3` but neither uses them. The `_sample_vec3()` and `_sample_quat()` methods only implement linear interpolation (lerp/slerp), never checking interpolation type. Hermite and Bezier interpolation are not implemented in either codebase.

- [ ] 509. [MDXRendererGL.cpp] `_build_geometry()` VAO setup passes 5 params instead of 6 — JS passes `null` as 6th parameter
- **JS Source**: `src/js/3D/renderers/MDXRendererGL.js` line 368
- **Status**: Pending
- **Details**: JS calls `vao.setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo, null)` with 6 parameters (last is null for index buffer). C++ calls `vao->setup_m2_separate_buffers(vbo, nbo, uvo, bibo, bwbo)` with only 5 parameters. The 6th parameter (index/element buffer) is missing in C++.

- [x] 510. [WMORendererGL.cpp] Load/update-set paths are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 81, 119, 206, 353, 434
- **Status**: Pending
- **Details**: JS defines async `load`, `_load_textures`, `_load_groups`, `loadDoodadSet`, and `updateSets`; C++ ports these methods synchronously, changing await/timing behavior.

- [ ] 511. [WMORendererGL.cpp] Reactive view binding/watcher lifecycle differs from JS
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 101–107, 637–639
- **Status**: Pending
- **Details**: JS stores `groupArray`/`setArray` by reference in `core.view` and updates via Vue `$watch` callbacks with explicit unregister in `dispose`. C++ copies arrays into view state and replaces watcher callbacks with polling logic, changing reactivity/update timing semantics.

- [ ] 512. [WMORendererGL.cpp] Reactive watchers replaced with polling — `groupWatcher`, `setWatcher`, `wireframeWatcher` not created
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 105–107
- **Status**: Pending
- **Details**: Same approach as WMOLegacyRendererGL — JS uses Vue watchers, C++ uses per-frame polling in `render()` (lines 643–676). Architecturally different but functionally equivalent with potential one-frame delay.

- [ ] 513. [WMORendererGL.cpp] `_load_textures()` `isClassic` check differs — JS tests `!!wmo.textureNames` (truthiness), C++ tests `!wmo->textureNames.empty()`
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` line 126
- **Status**: Pending
- **Details**: JS `!!wmo.textureNames` is true if the property exists and is truthy (even an empty array `[]` is truthy). C++ `!wmo->textureNames.empty()` is only true if the map has entries. If a WMO has the texture names chunk but it's empty, JS enters classic mode but C++ does not. Comment at C++ line 140–143 acknowledges this.

- [ ] 514. [WMORendererGL.cpp] `get_wmo_groups_view()`/`get_wmo_sets_view()` accessor methods don't exist in JS — C++ addition for multi-viewer support
- **JS Source**: `src/js/3D/renderers/WMORendererGL.js` lines 64–65, 103–104
- **Status**: Pending
- **Details**: JS uses `view[this.wmoGroupKey]` and `view[this.wmoSetKey]` for dynamic property access. C++ implements `get_wmo_groups_view()` and `get_wmo_sets_view()` methods (lines 60–69) that return references to the appropriate core::view member based on the key string, supporting `modelViewerWMOGroups`, `creatureViewerWMOGroups`, and `decorViewerWMOGroups`. This is a valid C++ adaptation of JS's dynamic property access.

- [x] 515. [WMOLegacyRendererGL.cpp] Load/update-set paths are synchronous instead of JS async methods
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 77, 104, 168, 270, 353
- **Status**: Pending
- **Details**: JS exposes async `load`, `_load_textures`, `_load_groups`, `loadDoodadSet`, and `updateSets`; C++ ports these paths as synchronous methods, altering Promise scheduling and error propagation behavior.

- [ ] 516. [WMOLegacyRendererGL.cpp] Doodad-set iteration adds bounds guard not present in JS
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 287–289
- **Status**: Pending
- **Details**: JS directly accesses `wmo.doodads[firstIndex + i]` without a pre-check. C++ introduces explicit range guarding/continue behavior, changing edge-case handling when doodad counts/indices are inconsistent.

- [ ] 517. [WMOLegacyRendererGL.cpp] Vue watcher-based reactive updates are replaced with render-time polling
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 88–93, 519–521
- **Status**: Pending
- **Details**: JS wires `$watch` callbacks and unregisters them in `dispose`. C++ removes watcher registration and uses per-frame state polling, which changes update trigger timing and reactivity semantics.

- [ ] 518. [WMOLegacyRendererGL.cpp] Reactive watchers replaced with polling — `groupWatcher`, `setWatcher`, `wireframeWatcher` not created
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 91–93
- **Status**: Pending
- **Details**: JS sets up three Vue watchers in `load()`. C++ replaces these with manual per-frame polling in `render()` (lines 517–551), comparing current state against `prev_group_checked`/`prev_set_checked` arrays. This is functionally equivalent but architecturally different — watchers are event-driven, polling is frame-driven with potential one-frame delay.

- [ ] 519. [WMOLegacyRendererGL.cpp] Texture wrap flag logic potentially inverted
- **JS Source**: `src/js/3D/renderers/WMOLegacyRendererGL.js` lines 146–147
- **Status**: Pending
- **Details**: JS sets `wrap_s = (material.flags & 0x40) ? gl.CLAMP_TO_EDGE : gl.REPEAT` and `wrap_t = (material.flags & 0x80) ? gl.CLAMP_TO_EDGE : gl.REPEAT`. C++ creates `BLPTextureFlags` with `wrap_s = !(material.flags & 0x40)` at line 184–185. The boolean negation may invert the wrap behavior — if `true` maps to CLAMP in the BLPTextureFlags API, then `!(flags & 0x40)` produces the opposite of what JS does. Need to verify the BLPTextureFlags API to confirm.

- [x] 520. [export-helper.cpp] `getIncrementalFilename` is synchronous instead of JS async Promise API
- **JS Source**: `src/js/casc/export-helper.js` lines 97–114
- **Status**: Pending
- **Details**: JS exposes `static async getIncrementalFilename(...)` and awaits `generics.fileExists`; C++ implementation is synchronous, changing timing/error behavior expected by Promise-style callers.

- [ ] 521. [export-helper.cpp] Export failure stack-trace output target differs from JS
- **JS Source**: `src/js/casc/export-helper.js` lines 284–288
- **Status**: Pending
- **Details**: JS writes stack traces with `console.log(stackTrace)` in `mark(...)`; C++ routes stack trace strings through `logging::write(...)`, changing where detailed error output appears.

- [ ] 522. [M2Exporter.cpp] `addURITexture` input contract differs from JS (data URI string vs decoded PNG buffer)
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59–61, 111–112
- **Status**: Pending
- **Details**: JS stores a data-URI string and decodes it inside `exportTextures()`. C++ `addURITexture` accepts `BufferWrapper` PNG bytes directly, changing caller-facing behavior and where decoding occurs.

- [ ] 523. [M2Exporter.cpp] Equipment UV2 export guard differs from JS truthy check
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 568
- **Status**: Pending
- **Details**: JS exports UV2 when `config.modelsExportUV2 && uv2` (empty arrays are truthy). C++ requires `!uv2.empty()`, so empty-but-present UV2 buffers are not exported.

- [ ] 524. [M2Exporter.cpp] Data textures silently dropped from GLTF/GLB texture maps and buffers
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 357–366
- **Status**: Pending
- **Details**: In JS, `textureMap` is a `Map` with mixed key types — numeric fileDataIDs and string keys like `"data-5"` for data textures (canvas-composited textures). These are passed directly to `gltf.setTextureMap()`. In C++ (M2Exporter.cpp ~lines 610–636), the string-keyed `textureMap` is converted to a `uint32_t`-keyed `gltfTexMap` via `std::stoul()`. Keys like `"data-5"` fail parsing and are silently dropped in the `catch (...)` block. The same happens for `texture_buffers` in GLB mode. This means data textures (canvas-composited textures for character models) are lost in GLTF/GLB exports — meshes will reference material names that have no corresponding texture entry.

- [ ] 525. [M2Exporter.cpp] uint16_t loop variable for triangle iteration risks overflow/infinite loop
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 375, 496, 638, 701, 850, 936
- **Status**: Pending
- **Details**: All triangle iteration loops in M2Exporter.cpp use `for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)`. If `mesh.triangleCount` is 65535, incrementing `vI` from 65534 to 65535 works, but then `vI++` wraps to 0, causing an infinite loop. If `triangleCount` exceeds 65535 (stored as uint32_t in the struct), the loop would also be incorrect since `uint16_t` can never reach the termination condition. JS uses `let vI` which is a double-precision float with no overflow. Should use `uint32_t` for the loop variable.

- [ ] 526. [M2Exporter.cpp] addURITexture accepts BufferWrapper instead of data URI string
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` lines 59–61
- **Status**: Pending
- **Details**: JS `addURITexture(out, dataURI)` stores a raw base64 data URI string, which is decoded later in `exportTextures()` via `BufferWrapper.fromBase64(dataTexture.replace(...))`. C++ `addURITexture(uint32_t textureType, BufferWrapper pngData)` accepts already-decoded PNG data, shifting the decoding responsibility to the caller. This is a contract change that alters the interface boundary — callers must now pre-decode the data URI before passing it. While not a bug if all callers are adapted, it changes the API surface compared to the original JS.

- [ ] 527. [M2Exporter.cpp] JSON submesh serialization uses fixed field enumeration instead of JS Object.assign
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 794
- **Status**: Pending
- **Details**: JS uses `Object.assign({ enabled: subMeshEnabled }, skin.subMeshes[i])` which dynamically copies *all* properties from the submesh object. C++ (M2Exporter.cpp ~lines 1111–1126) manually enumerates a fixed set of properties (submeshID, level, vertexStart, vertexCount, triangleStart, triangleCount, boneCount, boneStart, boneInfluences, centerBoneIndex, centerPosition, sortCenterPosition, sortRadius). If the Skin's SubMesh struct gains new fields, they would automatically appear in JS JSON output but would be missing in C++ JSON output. This is a fragile pattern that could silently omit metadata.

- [ ] 528. [M2Exporter.cpp] Data texture file manifest entries get fileDataID=0 instead of string key
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 748
- **Status**: Pending
- **Details**: In JS, `texFileDataID` for data textures is the string key `"data-X"`, which gets stored as-is in the file manifest. In C++ (~line 1059), `std::stoul(texKey)` fails for `"data-X"` keys and `texID` defaults to 0 in the `catch (...)` block. This means data textures in the file manifest will have `fileDataID = 0` instead of a meaningful identifier, losing the ability to correlate manifest entries with specific data texture types.

- [ ] 529. [M2Exporter.cpp] formatUnknownFile call signature differs from JS
- **JS Source**: `src/js/3D/exporters/M2Exporter.js` line 194
- **Status**: Pending
- **Details**: JS calls `listfile.formatUnknownFile(texFile)` where `texFile` is a string like `"12345.png"`. C++ (~line 410) calls `casc::listfile::formatUnknownFile(texFileDataID, raw ? ".blp" : ".png")` passing the numeric ID and extension separately. The C++ call passes `raw ? ".blp" : ".png"` but this code appears in the `!raw` branch (line 406 checks `!raw`), so the `raw` ternary would always evaluate to `.png`. While not necessarily a bug (depends on `formatUnknownFile` implementation), the call signature divergence means the output filename format may differ.

- [ ] 530. [M2LegacyExporter.cpp] Skin texture override condition differs when `skinTextures` is an empty array
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 65–70, 176–181, 220–225
- **Status**: Pending
- **Details**: JS checks `this.skinTextures` truthiness (empty array is truthy) and may overwrite to `undefined`, then skip texture. C++ requires `!skinTextures.empty()`, so it keeps original texture paths in that edge case.

- [x] 531. [M2LegacyExporter.cpp] Export API flow is synchronous instead of JS Promise-based async methods
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 39, 123, 262, 299
- **Status**: Pending
- **Details**: JS export methods (`exportTextures`, `exportAsOBJ`, `exportAsSTL`, `exportRaw`) are async and yield during I/O. C++ runs these paths synchronously, altering timing/cancellation behavior versus JS.

- [ ] 532. [M2LegacyExporter.cpp] uint16_t loop variable for triangle iteration risks overflow
- **JS Source**: `src/js/3D/exporters/M2LegacyExporter.js` lines 164, 289
- **Status**: Pending
- **Details**: Same issue as M2Exporter: triangle iteration loops in M2LegacyExporter.cpp (exportAsOBJ ~line 212, exportAsSTL ~line 401) use `for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)`. If `mesh.triangleCount` reaches or exceeds 65535, `uint16_t` overflow causes an infinite loop or incorrect iteration. JS uses `let vI` with no overflow limit. Should use `uint32_t` for the loop variable.

- [ ] 533. [M3Exporter.cpp] `addURITexture` input contract differs from JS (data URI string vs decoded PNG buffer)
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49–50
- **Status**: Pending
- **Details**: JS stores raw data-URI strings in `dataTextures`; C++ stores `BufferWrapper` PNG bytes, changing caller contract and data normalization stage.

- [ ] 534. [M3Exporter.cpp] UV2 export condition checks non-empty instead of JS defined-ness
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 88–89, 141–142
- **Status**: Pending
- **Details**: JS exports UV1 whenever it is defined (`!== undefined`), including empty arrays. C++ requires `!m3->uv1.empty()`, which changes behavior for defined-but-empty UV sets.

- [ ] 535. [M3Exporter.cpp] addURITexture accepts BufferWrapper instead of data URI string
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 49–51
- **Status**: Pending
- **Details**: Same issue as M2Exporter: JS `addURITexture(out, dataURI)` stores a raw base64 data URI string keyed by output path. C++ `addURITexture(const std::string& out, BufferWrapper pngData)` accepts already-decoded PNG data. This is an API contract change that shifts decoding responsibility to the caller.

- [ ] 536. [M3Exporter.cpp] exportTextures returns map<uint32_t, string> instead of JS Map with mixed key types
- **JS Source**: `src/js/3D/exporters/M3Exporter.js` lines 62–65
- **Status**: Pending
- **Details**: While the JS `exportTextures()` currently returns an empty Map (texture export not yet implemented), the C++ return type `std::map<uint32_t, std::string>` constrains future implementation to numeric-only keys. If M3 texture export is later implemented following M2Exporter's pattern (which uses string keys like `"data-X"` for data textures), the uint32_t key type would need to change. The JS Map supports mixed key types natively. This is a forward-compatibility concern rather than a current bug.

- [x] 537. [WMOExporter.cpp] Export methods are synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 62, 219, 360, 739, 841, 1179
- **Status**: Pending
- **Details**: JS uses async export methods (`exportTextures`, `exportAsGLTF`, `exportAsOBJ`, `exportAsSTL`, `exportGroupsAsSeparateOBJ`, `exportRaw`) with awaited CASC/file operations, while C++ executes these paths synchronously.

- [ ] 538. [WMOExporter.cpp] uint16_t loop variable for face iteration risks overflow/infinite loop (4 locations)
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 385, 551, 862, 1004 (batch.numFaces iteration)
- **Status**: Pending
- **Details**: Four face iteration loops in WMOExporter.cpp (lines 484, 643, 1077, 1202) use `for (uint16_t fi = 0; fi < batch.numFaces; fi++)`. If `batch.numFaces` reaches or exceeds 65535, the `uint16_t` loop variable wraps to 0, causing an infinite loop or incorrect iteration. JS uses `let i` which is a double-precision float with no overflow at these magnitudes. Should use `uint32_t` for the loop variable. Same issue as entry 352 (M2Exporter) and entry 357 (M2LegacyExporter).

- [ ] 539. [WMOExporter.cpp] Constructor takes explicit casc::CASC* parameter not present in JS
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 34–36
- **Status**: Pending
- **Details**: JS constructor is `constructor(data, fileID)` and obtains CASC source internally via `core.view.casc`. C++ constructor is `WMOExporter(BufferWrapper data, uint32_t fileDataID, casc::CASC* casc)` with explicit casc pointer. Additionally, `fileDataID` is constrained to `uint32_t` while JS accepts `string|number` for `fileID`. This is an API deviation — callers must pass the correct CASC instance and cannot pass string file paths.

- [ ] 540. [WMOExporter.cpp] Extra loadWMO() and getDoodadSetNames() accessor methods not in JS
- **JS Source**: `src/js/3D/exporters/WMOExporter.js` lines 34–36
- **Status**: Pending
- **Details**: C++ adds `loadWMO()` (line 1698) and `getDoodadSetNames()` (line 1702) methods that do not exist in the JS WMOExporter class. In JS, `this.wmo` is a public property accessed directly by callers. In C++, `wmo` is a private `std::unique_ptr<WMOLoader>`, so these accessor methods were added to expose the loader. This is a necessary C++ adaptation but changes the public API surface.

- [x] 541. [WMOLegacyExporter.cpp] Export methods are synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 47, 130, 392, 478
- **Status**: Pending
- **Details**: JS legacy WMO export methods are async and await texture/model I/O; C++ methods (`exportTextures`, `exportAsOBJ`, `exportAsSTL`, `exportRaw`) are synchronous, changing timing/cancellation semantics.

- [ ] 542. [WMOLegacyExporter.cpp] uint16_t loop variable for face iteration risks overflow/infinite loop (2 locations)
- **JS Source**: `src/js/3D/exporters/WMOLegacyExporter.js` lines 202, 425 (batch.numFaces iteration)
- **Status**: Pending
- **Details**: Two face iteration loops in WMOLegacyExporter.cpp (lines 288, 603) use `for (uint16_t i = 0; i < batch.numFaces; i++)`. If `batch.numFaces` reaches or exceeds 65535, the `uint16_t` loop variable wraps to 0, causing an infinite loop or incorrect iteration. JS uses `let i` with no overflow risk. Should use `uint32_t` for the loop variable. Same issue as entries 352, 357, 360.

- [x] 543. [vp9-avi-demuxer.cpp] Parsing/extraction flow is synchronous callback-based instead of JS async APIs
- **JS Source**: `src/js/casc/vp9-avi-demuxer.js` lines 22–23, 83–126
- **Status**: Pending
- **Details**: JS exposes `async parse_header()` and `async* extract_frames()` generator semantics; C++ ports these to synchronous methods with callback iteration, changing consumption and scheduling behavior.

- [x] 544. [OBJWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 129–225
- **Status**: Pending
- **Details**: JS implements asynchronous writes (`await writer.writeLine(...)` and async filesystem calls). C++ `write()` is synchronous, which changes ordering and error propagation relative to the original Promise API.

- [ ] 545. [OBJWriter.cpp] `appendGeometry` UV handling differs — JS uses `Array.isArray`/spread, C++ uses `insert`
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 84–99
- **Status**: Pending
- **Details**: JS `appendGeometry` handles multiple UV arrays and uses `Array.isArray` + spread operator for concatenation. C++ uses `std::vector::insert` for appending. Functionally equivalent.

- [ ] 546. [OBJWriter.cpp] Face output format uses 1-based indexing with `v[i+1]//vn[i+1]` or `v[i+1]/vt[i+1]/vn[i+1]` — matches JS correctly
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 119–142
- **Status**: Pending
- **Details**: Both JS and C++ output 1-based vertex indices in OBJ face format (e.g., `f v//vn v//vn v//vn` when no UVs, `f v/vt/vn v/vt/vn v/vt/vn` when UVs present). Vertex offset is added correctly in both implementations. Verified as correct.

- [ ] 547. [OBJWriter.cpp] Only first UV set is written in OBJ faces; JS `this.uvs[0]` matches C++ `uvs[0]`
- **JS Source**: `src/js/3D/writers/OBJWriter.js` lines 119, 130–131
- **Status**: Pending
- **Details**: Both JS and C++ check `this.uvs[0]` / `uvs[0]` for the first UV set when determining whether to include UV indices in face output. Only the first UV set is used in OBJ face references. Verified as matching.

- [x] 548. [MTLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 41–68
- **Status**: Pending
- **Details**: JS awaits file existence checks, directory creation, and line writes in `async write()`. C++ performs the same work synchronously, so behavior differs for call sites that rely on async completion semantics.

- [ ] 549. [MTLWriter.cpp] `material.name` extraction uses `std::filesystem::path(name).stem().string()` but JS uses `path.basename(name, path.extname(name))`
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 35–37
- **Status**: Pending
- **Details**: C++ line 30 uses `std::filesystem::path(name).stem().string()` to extract the filename without extension. JS uses `path.basename(name, path.extname(name))`. These should produce identical results for typical filenames. However, if `name` contains multiple dots (e.g., `texture.v2.png`), `stem()` returns `texture.v2` while `basename('texture.v2.png', '.png')` also returns `texture.v2`. Functionally equivalent.

- [ ] 550. [MTLWriter.cpp] MTL file uses `map_Kd` texture directive correctly matching JS
- **JS Source**: `src/js/3D/writers/MTLWriter.js` lines 38–39
- **Status**: Pending
- **Details**: Both JS and C++ write `map_Kd <file>` for diffuse texture mapping in material definitions. Verified as correct.

- [x] 551. [GLTFWriter.cpp] Export entrypoint is synchronous instead of JS Promise-based async flow
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 194–1504
- **Status**: Pending
- **Details**: JS defines `async write(overwrite, format)` and awaits filesystem/export operations throughout. C++ exposes `void write(...)` and executes all I/O synchronously, changing call timing/error propagation semantics for callers expecting Promise behavior.

- [ ] 552. [GLTFWriter.cpp] `add_scene_node` returns size_t index in C++ but the JS function returns the node object itself
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 276–280
- **Status**: Pending
- **Details**: JS `add_scene_node` returns the pushed node object reference (used for `skeleton.children.push()` later). C++ returns the index (size_t) instead, and uses index-based access to modify nodes later. This is functionally equivalent but bone parent lookup uses `bone_lookup_map[bone.parentBone]` to store the node index in C++ vs. storing the node object reference in JS. This difference means C++ accesses `nodes[parent_node_idx]` while JS mutates the object directly.

- [ ] 553. [GLTFWriter.cpp] `add_buffered_accessor` lambda omits `target` from bufferView when `buffer_target < 0` in C++, JS passes `undefined` which is serialized differently
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 282–296
- **Status**: Pending
- **Details**: JS `add_buffered_accessor` always includes `target: buffer_target` in the bufferView. When `buffer_target` is `undefined`, JSON.stringify omits the key entirely. C++ explicitly checks `if (buffer_target >= 0)` before adding the target key. This produces identical JSON output since JS `undefined` values are omitted by JSON.stringify, matching C++ not adding the key at all. Functionally equivalent.

- [ ] 554. [GLTFWriter.cpp] Animation channel target node uses `actual_node_idx` (variable per prefix setting) but JS always uses `nodeIndex + 1`
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 620–628, 757–765, 887–895
- **Status**: Pending
- **Details**: In JS, animation channel target node is always `nodeIndex + 1` regardless of prefix setting. In C++, `actual_node_idx` is used, which varies based on `usePrefix`. When `usePrefix` is true, C++ sets `actual_node_idx = nodes.size()` after pushing prefix_node (so it points to the real bone node, matching JS `nodeIndex + 1`). When `usePrefix` is false, `actual_node_idx = nodes.size()` before pushing the node, so it points to the same node. The JS code always does `nodeIndex + 1` which is only correct when prefix nodes exist. C++ correctly handles both cases. This is a JS bug that C++ fixes intentionally.

- [ ] 555. [GLTFWriter.cpp] `bone_lookup_map` stores index-to-index mapping using `std::map<int, size_t>` instead of JS Map storing index-to-object
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` line 464
- **Status**: Pending
- **Details**: JS `bone_lookup_map.set(bi, node)` stores the node object, which is then mutated later when children are added. C++ `bone_lookup_map[bi] = actual_node_idx` stores the index into the `nodes` array, and children are added via `nodes[parent_node_idx]["children"]`. This is functionally equivalent — JS mutates the object reference in the map and C++ indexes into the JSON array.

- [ ] 556. [GLTFWriter.cpp] Mesh primitive always includes `material` property in JS even when `materialMap.get()` returns `undefined`, C++ conditionally omits it
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1110–1119
- **Status**: Pending
- **Details**: JS always sets `material: materialMap.get(mesh.matName)` in the primitive, even if the material isn't found (result is `undefined`, which gets stripped by JSON.stringify). C++ uses `auto mat_it = materialMap.find(mesh.matName)` and only sets `primitive["material"]` if found. The final JSON output is identical since JS undefined is omitted, but the approach differs.

- [ ] 557. [GLTFWriter.cpp] Equipment mesh primitive always includes `material` in JS; C++ conditionally includes it
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1404–1411
- **Status**: Pending
- **Details**: Same pattern as entry 422 but for equipment meshes. JS sets `material: materialMap.get(mesh.matName)` which may be `undefined`. C++ checks `eq_mat_it != materialMap.end()` before setting material. Functionally equivalent in JSON output.

- [ ] 558. [GLTFWriter.cpp] `addTextureBuffer` method does not exist in JS — C++ addition
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` (no equivalent)
- **Status**: Pending
- **Details**: C++ adds `addTextureBuffer(uint32_t fileDataID, BufferWrapper buffer)` method (lines 113–115) which has no JS counterpart. JS only has `setTextureBuffers()` to set the entire map at once. The C++ addition allows incrementally adding individual texture buffers, which changes the API surface.

- [ ] 559. [GLTFWriter.cpp] Animation buffer name extraction in glb mode uses `rfind('_')` to extract `anim_idx`, but JS uses `split('_')` to get index at position 3
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 1468–1470
- **Status**: Pending
- **Details**: JS extracts animation index from bufferView name via `name_parts = bufferView.name.split('_'); anim_idx = name_parts[3]`. C++ uses `bv_name.rfind('_')` and then `std::stoi(bv_name.substr(last_underscore + 1))` to get the animation index. For names like `TRANS_TIMESTAMPS_0_1`, JS gets `name_parts[3] = "1"`, C++ gets substring after last underscore = `"1"`. These produce the same result. However, for `SCALE_TIMESTAMPS_0_1`, both work the same. Functionally equivalent.

- [ ] 560. [GLTFWriter.cpp] `skeleton` variable in JS is a node object reference, C++ is a node index
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 344–347, 449
- **Status**: Pending
- **Details**: JS `const skeleton = add_scene_node({name: ..., children: []})` returns the actual node object. Later, `skeleton.children.push(nodeIndex)` mutates it directly. C++ `size_t skeleton_idx = add_scene_node(...)` gets an index, and later accesses `nodes[skeleton_idx]["children"].push_back(...)`. Functionally equivalent.

- [ ] 561. [GLTFWriter.cpp] `usePrefix` is read inside the bone loop instead of outside like JS
- **JS Source**: `src/js/3D/writers/GLTFWriter.js` lines 460, 466
- **Status**: Pending
- **Details**: JS checks `core.view.config.modelsExportWithBonePrefix` outside the bone loop at line 460 (const is evaluated once). C++ reads `core::view->config.value("modelsExportWithBonePrefix", false)` inside the loop at line 470, which re-reads the config for every bone. Since config shouldn't change during export, this is functionally equivalent but slightly less efficient.

- [ ] 562. [GLBWriter.cpp] GLB JSON chunk padding fills with NUL (0x00) instead of spaces (0x20) as required by the glTF 2.0 spec
- **JS Source**: `src/js/3D/writers/GLBWriter.js` lines 20–28
- **Status**: Pending
- **Details**: The glTF 2.0 spec requires that the JSON chunk be padded with trailing space characters (0x20) to maintain 4-byte alignment. C++ `BufferWrapper::alloc(size, true)` zero-fills the buffer, so JSON padding bytes are 0x00. JS `Buffer.alloc(size)` also zero-fills, so JS has the same issue. However, this should be documented as a potential spec compliance issue for both versions.

- [ ] 563. [GLBWriter.cpp] Binary chunk padding uses zero bytes, matching JS behavior correctly
- **JS Source**: `src/js/3D/writers/GLBWriter.js` lines 29–36
- **Status**: Pending
- **Details**: Both JS and C++ use zero bytes (0x00) for BIN chunk padding. The glTF 2.0 spec requires BIN chunks to be padded with NUL (0x00), so this is correct. No issue here, verified as correct.

- [x] 564. [JSONWriter.cpp] `write()` is synchronous and BigInt-stringify behavior differs from JS
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 33–43
- **Status**: Pending
- **Details**: JS uses `async write()` and a `JSON.stringify` replacer that converts `bigint` values to strings. C++ `write()` is synchronous and writes `nlohmann::json::dump()` directly, which changes both async semantics and JS BigInt serialization parity.

- [ ] 565. [JSONWriter.cpp] `write()` uses `dump(1, '\t')` for pretty-printing; JS uses `JSON.stringify(data, null, '\t')`
- **JS Source**: `src/js/3D/writers/JSONWriter.js` lines 37–42
- **Status**: Pending
- **Details**: Both produce tab-indented JSON, but nlohmann `dump(1, '\t')` uses indent width of 1 with tab character, while JS `JSON.stringify` with `'\t'` uses tab for each indent level. The output should be identical for well-formed JSON.

- [ ] 566. [JSONWriter.cpp] `write()` default parameter correctly matches JS `overwrite = true`
- **JS Source**: `src/js/3D/writers/JSONWriter.js` line 30
- **Status**: Pending
- **Details**: Both JS and C++ default `overwrite` to `true`. Verified as correct.

- [ ] 567. [CSVWriter.cpp] `.cpp`/`.js` sibling contents are swapped, leaving `.cpp` as unconverted JavaScript
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 1–86
- **Status**: Pending
- **Details**: `CSVWriter.cpp` currently contains JavaScript (`require`, `class`, `module.exports`) while `CSVWriter.js` contains C++ (`#include`, `CSVWriter::...`). This violates expected source pairing and leaves the `.cpp` translation unit unconverted.

- [ ] 568. [CSVWriter.cpp] `addField()` overloaded — JS uses variadic `...fields`, C++ has two overloads (single string and vector)
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 25–27
- **Status**: Pending
- **Details**: JS `addField(...fields)` accepts any number of arguments via rest parameters and pushes all. C++ provides `addField(const std::string&)` for single fields and `addField(const std::vector<std::string>&)` for multiple. Callers must adapt to one of these two signatures instead of passing multiple individual arguments.

- [ ] 569. [CSVWriter.cpp] `escapeCSVField()` handles `null`/`undefined` differently — JS converts via `.toString()`, C++ returns empty for empty string
- **JS Source**: `src/js/3D/writers/CSVWriter.js` lines 42–51
- **Status**: Pending
- **Details**: JS `escapeCSVField()` handles `null`/`undefined` by returning empty string (line 43–44), then calls `value.toString()` for other types. C++ only accepts `const std::string&` and returns empty for empty strings (line 28–29). JS could receive numbers/booleans and stringify them; C++ requires pre-conversion to string by the caller.

- [ ] 570. [CSVWriter.cpp] `write()` default parameter differs — JS defaults `overwrite = true`, C++ has no default
- **JS Source**: `src/js/3D/writers/CSVWriter.js` line 57
- **Status**: Pending
- **Details**: JS `async write(overwrite = true)` defaults to overwriting. C++ `void write(bool overwrite)` has no default value. Callers must always explicitly pass the overwrite flag in C++.

- [x] 571. [SQLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 210–229
- **Status**: Pending
- **Details**: JS `async write()` awaits file checks, directory creation, and output writes. C++ performs the same operations synchronously, diverging from JS caller-visible async behavior.

- [ ] 572. [SQLWriter.cpp] Empty-string SQL value handling differs from JS null/undefined checks
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 66–76
- **Status**: Pending
- **Details**: JS returns `NULL` only for `null`/`undefined`; an empty string serializes to `''`. C++ maps `value.empty()` to `NULL`, so genuine empty-string field values are emitted as SQL `NULL`, changing exported data.

- [ ] 573. [SQLWriter.cpp] `addField()` overloaded — JS uses variadic `...fields`, C++ has two overloads (single string and vector)
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 48–49
- **Status**: Pending
- **Details**: JS `addField(...fields)` accepts any number of arguments via rest parameters and pushes all. C++ provides `addField(const std::string&)` for single fields and `addField(const std::vector<std::string>&)` for multiple. Same pattern as CSVWriter entry 413.

- [ ] 574. [SQLWriter.cpp] `generateDDL()` output format differs slightly — C++ builds strings directly, JS uses `lines.join('\n')`
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 141–177
- **Status**: Pending
- **Details**: JS builds an array of `lines` and joins with `\n` at the end. The output includes `DROP TABLE IF EXISTS ...\n\nCREATE TABLE ... (\n<columns>\n);\n\n`. C++ builds the result string directly with `+= "\n"`. The C++ version outputs `DROP TABLE IF EXISTS ...;\n\nCREATE TABLE ... (\n<columns joined with ,\n>\n);\n` which should match. However, JS `lines.push('')` creates an empty element that adds an extra `\n` when joined, and the column_defs are joined separately with `,\n`. The overall output may have subtle whitespace differences in the final string.

- [ ] 575. [SQLWriter.cpp] `toSQL()` format differs — JS uses `lines.join('\n')` with `value_rows.join(',\n') + ';'`, C++ concatenates directly
- **JS Source**: `src/js/3D/writers/SQLWriter.js` lines 183–204
- **Status**: Pending
- **Details**: JS's `toSQL()` builds lines array and joins with `\n`. Each batch creates `INSERT INTO ... VALUES\n` then `(vals),(vals),...(vals);\n\n`. C++ directly concatenates: `INSERT INTO ... VALUES\n(vals),\n(vals);\n\n`. The difference is that JS joins value_rows with `,\n` (so no leading newline on first row), while C++ adds `,\n` as a separator between rows within the loop. The output format may differ — JS produces `(vals),(vals)\n(vals);` while C++ produces `(vals),\n(vals),\n(vals);\n`. Minor formatting difference in output.

- [x] 576. [STLWriter.cpp] `write()` is synchronous instead of JS Promise-based async method
- **JS Source**: `src/js/3D/writers/STLWriter.js` lines 131–249
- **Status**: Pending
- **Details**: JS writer path is asynchronous and awaited by callers. C++ `write()` runs synchronously, changing API timing semantics compared to the original implementation.

- [ ] 577. [STLWriter.cpp] Header string says `wow.export.cpp` while JS says `wow.export` — intentional branding difference
- **JS Source**: `src/js/3D/writers/STLWriter.js` line 147
- **Status**: Pending
- **Details**: JS: `'Exported using wow.export v' + constants.VERSION`. C++: `"Exported using wow.export.cpp v" + std::string(constants::VERSION)`. This is an intentional branding change per project conventions (user-facing text should say wow.export.cpp). Verified as correct.

- [ ] 578. [STLWriter.cpp] `appendGeometry` simplified — C++ doesn't handle `Float32Array` vs `Array` distinction
- **JS Source**: `src/js/3D/writers/STLWriter.js` lines 66–86
- **Status**: Pending
- **Details**: JS `appendGeometry` checks `Array.isArray(this.verts)` to decide between spread and `Float32Array.from()` for concatenation. C++ always uses `std::vector::insert`, which works correctly regardless. The JS type distinction is a JS-specific concern that doesn't apply to C++. Functionally equivalent.