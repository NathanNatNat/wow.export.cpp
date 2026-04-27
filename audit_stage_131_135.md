<!-- ─── src/js/db/caches/DBDecor.cpp ─────────────────────────────── -->

- [ ] 1. [DBDecor.cpp] `initializeDecorData` is synchronous; JS original is async with `await`
  - **JS Source**: `src/js/db/caches/DBDecor.js` lines 15–40
  - **Status**: Pending
  - **Details**: The JS `initializeDecorData` is declared `async` and uses `await db2.HouseDecor.getAllRows()`. The C++ version is a plain synchronous function calling `casc::db2::preloadTable("HouseDecor").getAllRows()`. This is a deliberate architectural change (all DB2 loading is synchronous in C++), but it should be verified that callers have been updated to not `await` the result.

- [ ] 2. [DBDecor.cpp] Log format uses `std::format` `{}` instead of JS `%d` printf-style
  - **JS Source**: `src/js/db/caches/DBDecor.js` line 38
  - **Status**: Pending
  - **Details**: JS logs `'Loaded %d house decor items', decorItems.size` using a variadic log signature. C++ logs `std::format("Loaded {} house decor items", decorItems.size())`. While the output text is equivalent, the calling convention differs — `logging::write` receives a pre-formatted string in C++ rather than a format+args pair. This is consistent with the rest of the C++ codebase but is a deviation from the JS API.

<!-- ─── src/js/db/caches/DBDecorCategories.cpp ───────────────────── -->

- [ ] 3. [DBDecorCategories.cpp] `get_subcategories_for_decor` returns `nullptr` instead of JS `null`; return type should convey "null or set"
  - **JS Source**: `src/js/db/caches/DBDecorCategories.js` line 55
  - **Status**: Pending
  - **Details**: JS returns `decor_subcategory_map.get(decor_id) ?? null`. In C++ the function returns `const std::unordered_set<uint32_t>*` and returns `nullptr` when not found. This is an acceptable C++ idiom, but callers must be checked to handle `nullptr` correctly since JS callers would receive `null` (not a JS Set), which they test with `if (!result)`.

- [ ] 4. [DBDecorCategories.cpp] `initialize_categories` is synchronous; JS original is async
  - **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 10–51
  - **Status**: Pending
  - **Details**: The JS `initialize_categories` is `async` and uses `await` on all three `getAllRows()` calls. The C++ version is synchronous. Same architectural note as finding 1 — callers must be confirmed to not `await`.

- [ ] 5. [DBDecorCategories.cpp] `Name_lang` field accessed via `row.at()` without guard, may throw if field is absent
  - **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 17–19, 25–29
  - **Status**: Pending
  - **Details**: In `initialize_categories`, both the categories and subcategories loops call `fieldToString(row.at("Name_lang"))` using `at()` which throws `std::out_of_range` if the key is missing. The JS code accesses `row.Name_lang` with optional chaining fallback (`row.Name_lang || ...`), meaning a missing field is handled gracefully. The C++ should use `row.find("Name_lang")` with a fallback (as `DBDecor.cpp` does for the same field) to avoid potential crashes.

- [ ] 6. [DBDecorCategories.cpp] `DecorXDecorSubcategory` loop: `decor_id_found` check does not replicate JS `undefined` check correctly when `HouseDecorID` is present but zero
  - **JS Source**: `src/js/db/caches/DBDecorCategories.js` lines 33–47
  - **Status**: Pending
  - **Details**: JS checks `if (decor_id === undefined || sub_id === undefined) continue;`. A value of `0` would NOT skip the record. In C++, the code tracks whether the field was found at all (`decor_id_found`), meaning `decor_id = 0` from a present field is kept — this matches JS. However, there is no equivalent guard for `sub_id == 0`: JS skips only if `sub_id === undefined`, not if it is 0. The C++ code also does not skip when `sub_id == 0`, so for the sub_id side this is correct. The logic is functionally equivalent, but the comment on line 98–99 is slightly misleading.

<!-- ─── src/js/db/caches/DBGuildTabard.cpp ────────────────────────── -->

- [ ] 7. [DBGuildTabard.cpp] Missing `init_promise` deduplication: concurrent calls to `initialize()` may run the body multiple times
  - **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 36–89
  - **Status**: Pending
  - **Details**: The JS `initialize` uses `init_promise` to ensure that if the async function is called concurrently before `is_initialized` is set, all callers await the same in-flight promise rather than starting duplicate work. The C++ `initialize()` and `ensureInitialized()` are synchronous and single-threaded in this context, but if they are ever called from multiple threads the `is_initialized` check is not protected by a mutex. This is a potential thread-safety gap. The JS deduplication pattern should either be replicated or documented as not needed.

- [ ] 8. [DBGuildTabard.cpp] `GuildColorBackground/Border/Emblem` getAllRows use `.entries()` in JS vs structured binding in C++
  - **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 75–82
  - **Status**: Pending
  - **Details**: The JS loads background/border/emblem colors using `(await db2.GuildColorBackground.getAllRows()).entries()` which yields `[id, row]` pairs using the Map's id as key. The C++ uses structured bindings `for (const auto& [id, row] : ...)` which should also yield the table's row ID as `id`. This is functionally equivalent, but should be confirmed that `casc::db2::preloadTable(...).getAllRows()` returns pairs with the DB2 record ID as the first element (matching the JS Map key from `getAllRows()`).

- [ ] 9. [DBGuildTabard.cpp] `ColorRGB` struct stores `uint32_t` for r/g/b but JS stores plain numbers (likely 0–255 byte values)
  - **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 75–82
  - **Status**: Pending
  - **Details**: The JS stores `{ r: row.Red, g: row.Green, b: row.Blue }` where these are raw DB2 field values. C++ uses `uint32_t` for each channel. The DB2 color fields are typically 8-bit byte values (0–255). Using `uint32_t` is wider than necessary but does not cause data loss for values in 0–255 range. However, callers that expect byte-range RGB should be aware. This is a minor type mismatch.

- [ ] 10. [DBGuildTabard.cpp] `initialize` is synchronous; JS original is async with init_promise deduplication
  - **JS Source**: `src/js/db/caches/DBGuildTabard.js` lines 36–90
  - **Status**: Pending
  - **Details**: The JS `initialize` is `async` and wraps its body in an inner IIFE assigned to `init_promise`, then returns `init_promise`. The C++ version is a plain synchronous function. Callers must be verified to not `await` the result.

<!-- ─── src/js/db/caches/DBItemCharTextures.cpp ──────────────────── -->

- [ ] 11. [DBItemCharTextures.cpp] Missing `init_promise` deduplication for concurrent initialization calls
  - **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 34–93
  - **Status**: Pending
  - **Details**: The JS `initialize` uses an `init_promise` variable so that concurrent async callers await the same in-flight initialization. The C++ `initialize()` is synchronous and has no equivalent deduplication or mutex guard. Same concern as finding 7.

- [ ] 12. [DBItemCharTextures.cpp] `resolve_display_id` uses `std::map<uint32_t, uint32_t>` sorted iteration but JS sorts an array of keys; behavior is equivalent only if key type and sort order match
  - **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 106–119
  - **Status**: Pending
  - **Details**: JS does `[...modifiers.keys()].sort((a, b) => a - b)` to get the lowest modifier key. C++ stores modifiers in `std::map<uint32_t, uint32_t>` which is sorted in ascending order, so `modifiers.begin()` yields the lowest key. Since `uint32_t` sorts the same as numeric ascending this is functionally equivalent. However the C++ uses `uint32_t` keys while JS Map keys are numbers (signed). If any modifier IDs are negative in the DB2 data, `uint32_t` would wrap them, changing sort order. Recommend verifying modifier IDs are always non-negative.

- [ ] 13. [DBItemCharTextures.cpp] `DBComponentTextureFileData::initialize()` called synchronously; JS awaits it
  - **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 44–45
  - **Status**: Pending
  - **Details**: JS calls `await DBTextureFileData.ensureInitialized()` and `await DBComponentTextureFileData.initialize()`. The C++ calls them synchronously. This is consistent with the general architectural shift but should be verified that these dependencies are always initialized before `DBItemCharTextures::initialize()` is called.

- [ ] 14. [DBItemCharTextures.cpp] `getTexturesByDisplayId` default parameter for `race_id` and `gender_index` is `null` in JS but `-1` in C++
  - **JS Source**: `src/js/db/caches/DBItemCharTextures.js` lines 155, 130
  - **Status**: Pending
  - **Details**: JS declares `get_item_textures = (item_id, race_id = null, gender_index = null, modifier_id)` and `get_textures_by_display_id = (display_id, race_id = null, gender_index = null)`. C++ uses `-1` as the sentinel for "no preference". The C++ code then maps `race_id >= 0` to `std::optional<uint32_t>` and otherwise passes `std::nullopt`. This correctly maps `-1` (C++ "null") to `null` (JS). Functionally equivalent provided callers always pass `-1` or a valid non-negative race/gender ID, never `0` intending "no preference" (since `0` would be passed as a real value).

<!-- ─── src/js/db/caches/DBItemDisplayInfoModelMatRes.cpp ──────────── -->

- [ ] 15. [DBItemDisplayInfoModelMatRes.cpp] `fieldToUint32` does not handle `float` variant unlike other files in this directory
  - **JS Source**: `src/js/db/caches/DBItemDisplayInfoModelMatRes.js` lines 26–28
  - **Status**: Pending
  - **Details**: The `fieldToUint32` helper in `DBItemDisplayInfoModelMatRes.cpp` only handles `int64_t` and `uint64_t` variants of `db::FieldValue`, returning 0 for all others including `float`. Other files in the same directory (e.g. `DBDecor.cpp`, `DBGuildTabard.cpp`, `DBDecorCategories.cpp`) include a `float` branch: `if (auto* p = std::get_if<float>(&val)) return static_cast<uint32_t>(*p);`. If `ItemDisplayInfoID` or `MaterialResourcesID` are stored as floats in the DB2 variant, this helper would silently return 0, causing data to be dropped without any diagnostic.

- [ ] 16. [DBItemDisplayInfoModelMatRes.cpp] `initialize` is synchronous; JS original (`initializeIDIMMR`) is async
  - **JS Source**: `src/js/db/caches/DBItemDisplayInfoModelMatRes.js` lines 15–40
  - **Status**: Pending
  - **Details**: The JS `initializeIDIMMR` is `async` and uses `await DBTextureFileData.ensureInitialized()` and `await db2.ItemDisplayInfoModelMatRes.getAllRows()`. The C++ version is a plain synchronous function. Same architectural note as findings 1, 4, and 10.

- [ ] 17. [DBItemDisplayInfoModelMatRes.cpp] `ensureInitialized` does not guard against concurrent calls (no `init_promise` equivalent)
  - **JS Source**: `src/js/db/caches/DBItemDisplayInfoModelMatRes.js` lines 42–45
  - **Status**: Pending
  - **Details**: The JS `ensure_initialized` is `async` and simply calls `await initializeIDIMMR()` which itself has no concurrent deduplication. The C++ equivalent is synchronous with a simple `is_initialized` guard — same level of protection. No additional concern beyond finding 16.
