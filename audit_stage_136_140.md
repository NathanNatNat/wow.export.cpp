<!-- ─── src/js/db/caches/DBItemDisplays.cpp ─────────────────────────────── -->

- [ ] 1. [DBItemDisplays.cpp] Extra function `getTexturesByDisplayId` has no JS counterpart in DBItemDisplays.js
  - **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 66–68 (module.exports)
  - **Status**: Pending
  - **Details**: The JS module only exports `initializeItemDisplays` and `getItemDisplaysByFileDataID`. The C++ adds a third function `getTexturesByDisplayId` (defined at line 117–119 of DBItemDisplays.cpp, declared in DBItemDisplays.h line 35) that delegates to `DBItemDisplayInfoModelMatRes::getItemDisplayIdTextureFileIds`. This function does not exist in the JS source and represents an undocumented addition with no JS counterpart.

- [ ] 2. [DBItemDisplays.cpp] `getItemDisplayIdTextureFileIds` is called outside the per-modelFileDataID loop, changing the skip behaviour
  - **JS Source**: `src/js/db/caches/DBItemDisplays.js` lines 39–49
  - **Status**: Pending
  - **Details**: In the JS, `DBItemDisplayInfoModelMatRes.getItemDisplayIdTextureFileIds(itemDisplayInfoID)` is called inside the `for (const modelFileDataID of modelFileDataIDs)` loop, and a `continue` there skips only that single iteration. In the C++ (lines 83–93), the call is hoisted outside the loop and an early `continue` exits the entire display row if the result is null. In practice the result is the same for all iterations of the inner loop (same displayInfoID → same texture IDs), so the final map content is identical. However, the control-flow structure deviates from the JS original.

<!-- ─── src/js/db/caches/DBItemGeosets.cpp ─────────────────────────────── -->

- [ ] 3. [DBItemGeosets.cpp] `init_promise` concurrent-initialization guard is not ported
  - **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 20, 162–163, 225–226
  - **Status**: Pending
  - **Details**: The JS `initialize()` function stores an in-flight promise in `init_promise` and returns it immediately on re-entry to prevent double-initialization when called concurrently from multiple async contexts. The C++ `initialize()` (line 145) only checks `is_initialized` before proceeding. If C++ ever calls `initialize()` from multiple threads simultaneously before the flag is set, both could run the full initialization body. The JS pattern also resets `init_promise = null` after completion. While this may not cause issues in the current single-threaded call sites, it is a structural deviation from the JS original.

- [ ] 4. [DBItemGeosets.cpp] `ensure_initialized` / `ensureInitialized` missing async semantics — structural deviation
  - **JS Source**: `src/js/db/caches/DBItemGeosets.js` lines 231–234
  - **Status**: Pending
  - **Details**: The JS `ensure_initialized` is `async` and awaits `initialize()`, which itself returns the shared `init_promise`. The C++ `ensureInitialized()` (line 224) is synchronous. This is a known and acceptable adaptation for C++, but it should be noted as a deviation from the JS async model.

<!-- ─── src/js/db/caches/DBItemModels.cpp ─────────────────────────────── -->

- [ ] 5. [DBItemModels.cpp] `init_promise` concurrent-initialization guard is not ported
  - **JS Source**: `src/js/db/caches/DBItemModels.js` lines 19, 27–28, 102–103
  - **Status**: Pending
  - **Details**: Same pattern as DBItemGeosets: the JS `initialize()` uses `init_promise` to guard against concurrent calls and sets it to `null` after completion. The C++ `initialize()` (line 132) only checks `is_initialized`. The structural deviation is the same as finding 3 above.

- [ ] 6. [DBItemModels.cpp] `ensure_initialized` / `ensureInitialized` missing async semantics — structural deviation
  - **JS Source**: `src/js/db/caches/DBItemModels.js` lines 108–111
  - **Status**: Pending
  - **Details**: The JS `ensure_initialized` is `async` and awaits `initialize()`. The C++ `ensureInitialized()` (line 219) is synchronous. Acceptable C++ adaptation but a noted structural deviation.

<!-- ─── src/js/db/caches/DBItems.cpp ─────────────────────────────── -->

- [ ] 7. [DBItems.cpp] `init_promise` concurrent-initialization guard is not ported
  - **JS Source**: `src/js/db/caches/DBItems.js` lines 16, 18–19, 50–51
  - **Status**: Pending
  - **Details**: Same pattern as findings 3 and 5: the JS `initialize_items()` uses an `init_promise` to coalesce concurrent async callers and resets it to `null` after completion. The C++ `initialize()` (line 42) only checks `is_initialized_flag`. Structural deviation from the JS original.

- [ ] 8. [DBItems.cpp] `Display_lang` empty-string case not handled as a fallback for the item name
  - **JS Source**: `src/js/db/caches/DBItems.js` lines 40–41
  - **Status**: Pending
  - **Details**: The JS uses `item_row.Display_lang ?? 'Unknown item #' + item_id`. The nullish-coalescing operator `??` triggers on `null` or `undefined` but not on an empty string `""`. The C++ (lines 65–69) uses `item_row.find("Display_lang")` — if the field is present but the variant holds a non-string type (e.g., `int64_t`), `fieldToString` returns `""` and the empty string is silently stored as the name without applying the fallback. If the DB2 field is present but empty or zero-typed, the C++ will silently store an empty name instead of the `"Unknown item #N"` fallback, whereas the JS would use the fallback for `null`/`undefined` values. The check should also apply the fallback when `fieldToString` returns an empty string.

<!-- ─── src/js/db/caches/DBModelFileData.cpp ─────────────────────────────── -->
<!-- No issues found -->
