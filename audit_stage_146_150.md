<!-- ─── src/js/modules/legacy_tab_data.cpp ──────────────────────────────── -->

- [ ] 1. [legacy_tab_data.cpp] DBC listbox passes non-empty unittype despite JS using `:includefilecount="false"`
  - **JS Source**: `src/js/modules/legacy_tab_data.js` line 131
  - **Status**: Pending
  - **Details**: The JS Listbox prop `:includefilecount="false"` disables the file counter. The C++ `listbox::render` call passes `"dbc file"` as `unittype`, which enables the file count display in the C++ implementation. To match the JS behaviour the unittype should be `""` (empty string) so that `listbox::renderStatusBar` shows nothing. The existing `renderStatusBar("table", {}, listbox_dbc_state)` call on the DBC list also renders an unwanted status bar not present in the JS template.

- [ ] 2. [legacy_tab_data.cpp] DBC listbox passes `persistscrollkey="dbc"` but JS template has no `persistscrollkey`
  - **JS Source**: `src/js/modules/legacy_tab_data.js` line 131
  - **Status**: Pending
  - **Details**: The JS `<Listbox>` for the DBC list does not set the `persistscrollkey` prop. The C++ call passes `"dbc"` as the persist scroll key. This causes scroll position to be saved/restored across tab switches when it should not be. The argument should be `""` (empty string) to disable scroll persistence.

- [ ] 3. [legacy_tab_data.cpp] Options checkboxes are missing hover tooltips present in the JS template
  - **JS Source**: `src/js/modules/legacy_tab_data.js` lines 146–157
  - **Status**: Pending
  - **Details**: Three checkbox labels in the `tab-data-options` section carry `title` attributes in the JS template, which render as browser hover tooltips:
    - "Copy Header" → `title="Include header row when copying"`
    - "Create Table" → `title="Include DROP/CREATE TABLE statements"`
    - "Export all rows" → `title="Export all rows"`
    The C++ renders these checkboxes (`ImGui::Checkbox`) without any `ImGui::IsItemHovered()` + `ImGui::SetTooltip()` call after each one. Each checkbox should have a corresponding tooltip.

<!-- ─── src/js/modules/legacy_tab_files.cpp ─────────────────────────────── -->

- [ ] 4. [legacy_tab_files.cpp] "Regex Enabled" label is missing hover tooltip and `SameLine` before filter input
  - **JS Source**: `src/js/modules/legacy_tab_files.js` line 84
  - **Status**: Pending
  - **Details**: The JS template places `<div class="regex-info" :title="$core.view.regexTooltip">Regex Enabled</div>` and the filter `<input>` together inline. The C++ renders `ImGui::TextUnformatted("Regex Enabled")` without calling `ImGui::IsItemHovered()` + `ImGui::SetTooltip(view.regexTooltip.c_str())` and without `ImGui::SameLine()` afterwards, so the filter input falls onto a new line. Both the tooltip and the `SameLine()` call are required to match the JS layout and behaviour.

- [ ] 5. [legacy_tab_files.cpp] Missing `renderStatusBar` call — file count not displayed despite JS using `:includefilecount="true"`
  - **JS Source**: `src/js/modules/legacy_tab_files.js` line 75
  - **Status**: Pending
  - **Details**: The JS Listbox prop `:includefilecount="true"` enables a visible file counter below the list. The C++ call correctly passes `unittype = "file"` to `listbox::render`, but no `listbox::renderStatusBar(...)` call is made and no `BeginStatusBar`/`EndStatusBar` region is opened. As a result the file count is never shown. A `BeginStatusBar` / `renderStatusBar("file", {}, listbox_state)` / `EndStatusBar` block must be added between `EndListContainer` and `BeginFilterBar` to match JS behaviour.

<!-- ─── src/js/modules/legacy_tab_fonts.cpp ─────────────────────────────── -->

- [ ] 6. [legacy_tab_fonts.cpp] Filter input uses `ImGui::InputText` instead of `ImGui::InputTextWithHint` — placeholder text missing
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 72
  - **Status**: Pending
  - **Details**: The JS template has `<input type="text" placeholder="Filter fonts..."/>`. The C++ filter bar uses `ImGui::InputText("##FilterFonts", ...)` which has no placeholder. It should be `ImGui::InputTextWithHint("##FilterFonts", "Filter fonts...", ...)` to display the placeholder hint when the field is empty, matching the JS behaviour.

- [ ] 7. [legacy_tab_fonts.cpp] "Regex Enabled" label is missing hover tooltip and `SameLine` before filter input
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 71
  - **Status**: Pending
  - **Details**: The JS template has `<div class="regex-info" :title="$core.view.regexTooltip">Regex Enabled</div>` inline with the filter input. The C++ (lines 248–255) renders `ImGui::TextUnformatted("Regex Enabled")` without `ImGui::IsItemHovered()` + `ImGui::SetTooltip(view.regexTooltip.c_str())` and without `ImGui::SameLine()`, so the filter input is placed on a new line. Both the tooltip and `SameLine()` are required.

- [ ] 8. [legacy_tab_fonts.cpp] Context menu contains extra items not present in the JS template
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` lines 64–68
  - **Status**: Pending
  - **Details**: The JS context menu for the fonts listbox has exactly three items: "Copy file path(s)", "Copy export path(s)", "Open export directory". The C++ context menu lambda (lines 217–235) adds two additional items guarded by `hasFileDataIDs`: "Copy file path(s) (listfile format)" and "Copy file data ID(s)". These items do not exist in the JS template and should be removed. Because MPQ-sourced font files never have file data IDs, `hasFileDataIDs` will always be false, so they never appear in practice — but the code is still a deviation from the JS source.

- [ ] 9. [legacy_tab_fonts.cpp] Export button uses deprecated `app::theme::BeginDisabledButton`/`EndDisabledButton` instead of `ImGui::BeginDisabled`/`EndDisabled`
  - **JS Source**: `src/js/modules/legacy_tab_fonts.js` line 83
  - **Status**: Pending
  - **Details**: Lines 345–348 in legacy_tab_fonts.cpp use `app::theme::BeginDisabledButton()` and `app::theme::EndDisabledButton()` to disable the Export button when busy. Per CLAUDE.md, `app::theme` APIs should be progressively removed and not used in new code. The correct approach is `if (busy) ImGui::BeginDisabled(); ... if (busy) ImGui::EndDisabled();` as used in the files and textures tabs.

<!-- ─── src/js/modules/legacy_tab_home.cpp ──────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/modules/legacy_tab_textures.cpp ──────────────────────────── -->

- [ ] 10. [legacy_tab_textures.cpp] "Regex Enabled" label is missing `SameLine` before filter input
  - **JS Source**: `src/js/modules/legacy_tab_textures.js` line 125
  - **Status**: Pending
  - **Details**: The JS template renders the regex info div and the filter input together inline inside `<div class="filter">`. The C++ filter bar (lines 347–358) renders `ImGui::TextUnformatted("Regex Enabled")` with an `IsItemHovered` + `SetTooltip` (correct) but does NOT call `ImGui::SameLine()` afterwards. This means the filter input is placed on a new line below "Regex Enabled" instead of on the same line, deviating from the JS layout. An `ImGui::SameLine()` call must be added after the closing brace of the `if (view.config.value("regexFilters", false))` block, before `ImGui::SetNextItemWidth`.
