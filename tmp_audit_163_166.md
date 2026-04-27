<!-- FILE: src/js/modules/tab_help.cpp -->
<!-- Intentionally removed per CLAUDE.md — no findings needed -->

<!-- FILE: src/js/modules/tab_home.cpp -->
<!-- No findings for this file -->

<!-- FILE: src/js/modules/tab_install.cpp -->
### [tab_install.cpp] Missing listbox::renderStatusBar calls — includefilecount status bar absent from both views
- **JS Source**: `src/js/modules/tab_install.js` lines 165, 184
- **Status**: Pending
- **Details**: Both Listbox components in the JS template use `:includefilecount="true"` — one with `unittype="install file"` (main manifest view) and one with `unittype="string"` (string viewer). In the C++ port, `listbox::render()` does not internally render the status bar; it must be called separately via `listbox::renderStatusBar()`. The `tab_install.cpp` render function never calls `listbox::renderStatusBar()` for either listbox. This means the file count status bar ("N install files found.") is entirely absent from the Install Manifest tab in both views. Compare with other tabs that correctly call `listbox::renderStatusBar()` after each listbox (e.g., `tab_audio.cpp` line 553, `tab_fonts.cpp` line 289). The status bar should be added after `ImGui::EndChild()` for each list container, before the tray section.

<!-- FILE: src/js/modules/tab_item_sets.cpp -->
### [tab_item_sets.cpp] equip_set does not clear chrEquippedItemSkins for each equipped slot
- **JS Source**: `src/js/modules/tab_item_sets.js` lines 99–115
- **Status**: Pending
- **Details**: The JS `equip_set` method explicitly deletes the skin override for each equipped slot: `delete this.$core.view.chrEquippedItemSkins[slot_id]` (line 103) for each item equipped. The C++ `equip_set` (tab_item_sets.cpp lines 209–218) only sets `view.chrEquippedItems[std::to_string(slot_id_opt.value())] = item_id` but never clears `chrEquippedItemSkins` for those slots. This means when a set is equipped, any existing skin customization for those slots persists, causing incorrect rendering in the character preview (old skin override takes precedence over the newly equipped set item). The fix is to also erase the slot from `chrEquippedItemSkins` after setting the equipped item, as done in `equip-item.cpp` lines 54–55.
