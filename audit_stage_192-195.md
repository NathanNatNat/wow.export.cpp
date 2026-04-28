## File 192. [cache-collector.cpp]

- cache-collector.cpp: `parse_url` drops the query string from URLs
  - **JS Source**: `src/js/workers/cache-collector.js` lines 19–44
  - **Status**: Pending
  - **Details**: JS `https_request` passes `parsed.pathname + parsed.search` as the request path (line 25), which includes any query-string parameters. The C++ `parse_url` (cache-collector.cpp lines 157–188) only captures the path up to the first `/`, losing everything after `?`. URLs passed to the worker that include query parameters would produce incorrect HTTP requests.

- cache-collector.cpp: No top-level entry point equivalent to `collect().catch(...)`
  - **JS Source**: `src/js/workers/cache-collector.js` line 431
  - **Status**: Pending
  - **Details**: The JS file auto-invokes `collect().catch(err => log(\`fatal: ${err.message}\`))` at module load time. The C++ exposes `collect()` as a library function, so the caller is responsible for invoking it; the top-level invocation and the fatal-error log path are not replicated inside the translation unit itself. This is an architectural difference (worker thread vs library call) but worth documenting for completeness.

- cache-collector.cpp: `upload_flavor` payload missing `binary_hashes || {}` fallback
  - **JS Source**: `src/js/workers/cache-collector.js` line 312
  - **Status**: Pending
  - **Details**: JS builds the submit payload with `binary_hashes: result.binary_hashes || {}` (line 312), ensuring an empty object is sent when `binary_hashes` is absent/null. C++ uses `result.binary_hashes` directly (cache-collector.cpp line 571), which relies on `FlavorResult::binary_hashes` always being initialised — currently it is, but the explicit fallback present in JS is not replicated.

## File 193. [equip-item.cpp]

- equip-item.cpp: Vue reactivity spread assignments omitted — architectural deviation from JS
  - **JS Source**: `src/js/wow/equip-item.js` lines 26–27
  - **Status**: Pending
  - **Details**: JS reassigns `core.view.chrEquippedItems = { ...core.view.chrEquippedItems }` and `core.view.chrEquippedItemSkins = { ...core.view.chrEquippedItemSkins }` after mutation to trigger Vue.js reactive updates (lines 26–27). C++ has no reactive data-binding framework, so these reassignments are omitted. If the C++ view system gains a notification/dirty-flag mechanism for equipped-item changes, this code path will need to be revisited.

- equip-item.cpp: "other slot empty" check uses stricter null-only test vs JS falsy check
  - **JS Source**: `src/js/wow/equip-item.js` line 15
  - **Status**: Pending
  - **Details**: JS checks `!core.view.chrEquippedItems[other]` (line 15), which is falsy for `undefined`, `null`, `0`, `false`, and `""`. The C++ equivalent (equip-item.cpp lines 42–45) checks `.is_null()` and `.contains()` only, treating JSON `0`, `false`, or `""` as truthy (slot occupied). In practice this is unlikely to cause a difference since item IDs are always positive integers, but it is a semantic deviation from the JS behaviour.

- equip-item.cpp: Function signature differs from JS — takes separate `item_id`/`item_name` instead of an item object
  - **JS Source**: `src/js/wow/equip-item.js` lines 4–5
  - **Status**: Pending
  - **Details**: JS `equip_item(core, item, pending_slot)` receives `item.id` and `item.name` from the item object (lines 5–6, 30). C++ `equip_item(uint32_t item_id, const std::string& item_name, int pending_slot)` accepts them as separate arguments (equip-item.cpp line 21). While functionally equivalent if callers pass the correct values, the signature change must be verified against all call sites to ensure no data is lost or mismatched.

## File 194. [EquipmentSlots.cpp]

- EquipmentSlots.cpp: `filter_name` field added to all `EQUIPMENT_SLOTS` entries — JS only defines it on shoulder entries
  - **JS Source**: `src/js/wow/EquipmentSlots.js` lines 11–27
  - **Status**: Pending
  - **Details**: In the JS `EQUIPMENT_SLOTS` array, only the two shoulder entries carry a `filter_name` property (lines 14–15); all other entries have `filter_name` as `undefined`. The C++ `EquipmentSlotEntry` struct (EquipmentSlots.h line 26) defines `filter_name` for every slot, with non-shoulder slots echoing their display name. The existing C++ usage in `tab_characters.cpp` (lines 3994, 4002) checks `filter_name.empty() ? slot.name : slot.filter_name`, which replicates the JS `filter_name ?? slot.name` pattern correctly. However, any future code that checks whether `filter_name` is *different from* the display name (to identify shoulder-class slots) would behave differently between JS and C++, since non-shoulder slots in JS have no `filter_name` at all while C++ always has it equal to `name`.

- EquipmentSlots.cpp: `get_slot_id_for_inventory_type` / `get_slot_id_for_wmv_slot` return `std::nullopt` on miss vs JS `null`
  - **JS Source**: `src/js/wow/EquipmentSlots.js` lines 157–159, 161–163
  - **Status**: Pending
  - **Details**: JS functions return `null` (via `?? null`) when the key is not found. C++ returns `std::optional` with `std::nullopt`. Callers in C++ must use `.has_value()` / `.value()` checks; any caller that was ported expecting a raw int without optional handling would silently fail. This is a standard C++ idiom translation but is flagged because callers must be verified.

## File 195. [ItemSlot.cpp]

No findings.
