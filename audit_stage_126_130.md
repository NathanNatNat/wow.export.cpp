<!-- ─── src/js/db/caches/DBComponentTextureFileData.cpp ─────────────────────────────── -->

- [ ] 1. [DBComponentTextureFileData.cpp] `getTextureForRaceGender` wraps all race/gender-specific loops inside `if (race_id.has_value() && gender_index.has_value())`, diverging from JS logic
  - **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js` lines 59–87
  - **Status**: Pending
  - **Details**: In JS, `getTextureForRaceGender` always executes the exact-match loop, the race+any-gender loop, the fallback-race loop, and the race=0+specific-gender loop regardless of whether `race_id`/`gender_index` are null. A null comparison like `info.raceID === null` simply never matches, so those loops silently produce no result. In C++, when either `race_id` or `gender_index` is `std::nullopt`, all four loops are skipped entirely and execution jumps directly to the race=0 (any race) loop. The functional outcome is the same (null never matches), but the structure deviates from the original JS — a future change to any of those inner loops could inadvertently rely on the JS always-execute behaviour. The C++ guard should either be removed (relying on the nullopt comparison never matching) or the code should be clearly documented as a deliberate structural deviation.

- [ ] 2. [DBComponentTextureFileData.cpp] `initialize` uses mutex/condvar concurrency guard, but JS uses a single `init_promise` singleton guard
  - **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js` lines 17–41
  - **Status**: Pending
  - **Details**: The JS `initialize` uses `init_promise` to coalesce concurrent callers onto a single in-flight Promise. The C++ implementation uses `is_initializing` + `std::condition_variable` to serialize concurrent calls: the first caller runs initialization while subsequent callers block waiting on the condvar. This is a structural deviation, but the behaviour is functionally equivalent — only one initialization runs and all concurrent callers receive the result. Documented here for completeness; it is an acceptable C++ adaptation.

- [ ] 3. [DBComponentTextureFileData.cpp] `db2.ComponentTextureFileData.getAllRows()` replaced by `casc::db2::preloadTable("ComponentTextureFileData").getAllRows()` — table name must match exactly
  - **JS Source**: `src/js/db/caches/DBComponentTextureFileData.js` line 27
  - **Status**: Pending
  - **Details**: Minor: the JS accesses `db2.ComponentTextureFileData` (a named property). The C++ calls `casc::db2::preloadTable("ComponentTextureFileData")`. The string key must match the actual DB2 table name. If `preloadTable` is case-sensitive this is fine; just flagging for completeness.

<!-- ─── src/js/db/caches/DBCreatureDisplayExtra.cpp ─────────────────────────────── -->

- [ ] 4. [DBCreatureDisplayExtra.cpp] JS `_initialize` iterates `CreatureDisplayInfoOption` rows via `.values()` (ignoring the row ID key), but C++ iterates via `[_optId, row]` key-value pairs and discards the key with `(void)_optId`
  - **JS Source**: `src/js/db/caches/DBCreatureDisplayExtra.js` lines 39–48
  - **Status**: Pending
  - **Details**: The JS iterates `(await db2.CreatureDisplayInfoOption.getAllRows()).values()` — it only uses the row value, not the key. The C++ iterates `const auto& [_optId, row]` and silences the unused key with `(void)_optId`. The result is functionally identical, but it is worth noting since `_optId` is the row's own DB ID; the JS code explicitly discards it. No functional bug here.

- [ ] 5. [DBCreatureDisplayExtra.cpp] JS `get_extra` returns `undefined` when not found; C++ returns `nullptr`
  - **JS Source**: `src/js/db/caches/DBCreatureDisplayExtra.js` line 55
  - **Status**: Pending
  - **Details**: JS: `const get_extra = (id) => extra_map.get(id)` — returns `undefined` when the key is absent. C++ returns `nullptr` (a pointer). All call sites must check for `nullptr` rather than truthiness. This is a necessary C++ adaptation; callers must be verified to handle `nullptr` correctly.

- [ ] 6. [DBCreatureDisplayExtra.cpp] JS `get_customization_choices` returns a new empty array `[]` (via nullish coalescing `?? []`) when not found; C++ returns a `const&` to a static empty vector
  - **JS Source**: `src/js/db/caches/DBCreatureDisplayExtra.js` line 57
  - **Status**: Pending
  - **Details**: JS: `option_map.get(extra_id) ?? []` — each call for a missing key yields a fresh empty array. C++ returns a const reference to a static `empty_options` vector. Callers that hold onto the returned reference after a `reset()` or re-initialization would observe a stale reference; however, since there is no `reset()` function in this module, this is practically safe. The interface diverges (reference vs. value) and callers must not store the returned reference.

<!-- ─── src/js/db/caches/DBCreatureList.cpp ─────────────────────────────── -->

- [ ] 7. [DBCreatureList.cpp] JS `get_all_creatures` returns a `Map` (keyed by creature ID); C++ returns a `const std::vector<CreatureEntry>&`
  - **JS Source**: `src/js/db/caches/DBCreatureList.js` lines 45–47
  - **Status**: Pending
  - **Details**: In JS, `creatures` is a `Map` and `get_all_creatures()` returns that Map directly. Callers iterate it with `for (const [id, entry] of creatures)`. In C++, `creatures` is a `std::vector<CreatureEntry>` and `get_all_creatures()` returns a const reference to it. Callers must iterate with range-for over the vector. This is a structural change that all call sites must account for — any caller that uses Map-style iteration (by key) will break if not updated accordingly.

- [ ] 8. [DBCreatureList.cpp] JS `get_creature_by_id` returns `undefined` when not found; C++ returns `nullptr`
  - **JS Source**: `src/js/db/caches/DBCreatureList.js` lines 49–51
  - **Status**: Pending
  - **Details**: JS: `creatures.get(id)` — returns `undefined` when absent. C++ returns `nullptr`. All callers must check for `nullptr`. Acceptable C++ adaptation, but callers must be verified.

- [ ] 9. [DBCreatureList.h] Header documents `get_all_creatures` as returning "reference to the creature map" but it actually returns a vector
  - **JS Source**: `src/js/db/caches/DBCreatureList.js` lines 45–47
  - **Status**: Pending
  - **Details**: The comment in `DBCreatureList.h` line 32 says "Reference to the creature map." but the function returns `const std::vector<CreatureEntry>&`. The comment is stale and misleading, though not a functional defect.

<!-- ─── src/js/db/caches/DBCreatures.cpp ─────────────────────────────── -->

- [ ] 10. [DBCreatures.cpp] Indentation inconsistency inside `initializeCreatureData`: the outer `try` block and the `creatureGeosetMap`/`modelIDToDisplayInfoMap` section use different indent levels
  - **JS Source**: `src/js/db/caches/DBCreatures.js` lines 18–76
  - **Status**: Pending
  - **Details**: Lines 71–82 inside `initializeCreatureData` are indented with one less tab level than the surrounding `try` block (lines 70–135). This is a cosmetic/style issue but it could mask structural logic errors when reviewing the code. Not a functional bug.

- [ ] 11. [DBCreatures.cpp] JS `initializeCreatureData` checks `isInitialized` at the start and returns early; C++ uses a mutex guard instead
  - **JS Source**: `src/js/db/caches/DBCreatures.js` lines 18–20
  - **Status**: Pending
  - **Details**: The JS function begins with `if (isInitialized) return;`. The C++ version uses a mutex+condvar pattern (shared with other DB cache files) that handles concurrent callers. This is a structural difference but functionally equivalent and is an acceptable C++ adaptation.

- [ ] 12. [DBCreatures.cpp] JS `extraGeosets` is only present on `display` objects that need it (property does not exist otherwise); C++ uses `std::optional<std::vector<uint32_t>>` which defaults to `std::nullopt` — minor structural deviation
  - **JS Source**: `src/js/db/caches/DBCreatures.js` lines 60–63
  - **Status**: Pending
  - **Details**: In JS, `display.extraGeosets` is only assigned if `modelIDHasExtraGeosets` is true — otherwise the property does not exist on the object at all. In C++, `extraGeosets` is a member of `CreatureDisplayInfo` and defaults to `std::nullopt`. The check `display.extraGeosets.has_value()` (or checking for `std::nullopt`) is therefore the correct C++ equivalent of `display.extraGeosets !== undefined`. All callers must use `.has_value()` rather than a truthiness check. This is an acceptable C++ adaptation.

- [ ] 13. [DBCreatures.cpp] JS stores all `creatureDisplays`, `creatureDisplayInfoMap`, `displayIDToFileDataID`, `modelDataIDToFileDataID` as `Map`; C++ uses `std::unordered_map` with `std::reference_wrapper` for `creatureDisplays`
  - **JS Source**: `src/js/db/caches/DBCreatures.js` lines 9–12
  - **Status**: Pending
  - **Details**: The C++ `creatureDisplays` map stores `std::vector<std::reference_wrapper<const CreatureDisplayInfo>>` referencing entries in `creatureDisplayInfoMap`. This means `creatureDisplayInfoMap` must not be rehashed or cleared after population, otherwise the references become dangling. Since `initializeCreatureData` fills both maps in one pass and they are static globals, this is safe at runtime — but it is a fragile design that does not exist in the JS version (which stores direct object references). Documented for future maintainability.

<!-- ─── src/js/db/caches/DBCreaturesLegacy.cpp ─────────────────────────────── -->

- [ ] 14. [DBCreaturesLegacy.cpp] JS `initializeCreatureData` takes an `mpq` object and calls `mpq.getFile(path)`; C++ takes a `std::function<std::vector<uint8_t>(const std::string&)>` callback
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 19–112
  - **Status**: Pending
  - **Details**: The JS interface is `initializeCreatureData(mpq, build_id)` where `mpq` is an MPQInstall object with a `getFile` method. The C++ interface is `initializeCreatureData(std::function<...> getFile, build_id)` — the MPQ object is abstracted away as a callback. This is a valid C++ adaptation (the MPQ type doesn't exist directly in C++), but all call sites must pass a lambda wrapping the MPQ getFile call.

- [ ] 15. [DBCreaturesLegacy.cpp] JS `getCreatureDisplaysByPath` only converts `.mdx` to `.m2` (not `.mdl`); C++ `getCreatureDisplaysByPath` converts both `.mdl` and `.mdx` to `.m2`
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 119–132
  - **Status**: Pending
  - **Details**: In JS, `getCreatureDisplaysByPath` normalizes the incoming path with `normalized.replace(/\.mdx$/i, '.m2')` — only `.mdx`. The JS `initializeCreatureData` function also normalizes to `.m2` for both `.mdl` and `.mdx` when building the `model_id_to_path` map. However at lookup time, if a caller passes a `.mdl` path, JS would NOT convert it to `.m2` and would find no match. In C++, both `normalizePath` (used in both init and lookup) convert `.mdl` and `.mdx`, so a `.mdl` path at lookup time would match. This is a behavioural divergence from the JS — the C++ is more permissive in `getCreatureDisplaysByPath`. Whether this is intentional or a bug needs review.

- [ ] 16. [DBCreaturesLegacy.cpp] JS `getCreatureDisplaysByPath` returns `undefined` when not found; C++ returns `nullptr`
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 119–132
  - **Status**: Pending
  - **Details**: JS: `creatureDisplays.get(normalized)` — returns `undefined` when absent. C++ returns `nullptr`. All callers must check for `nullptr`. Acceptable C++ adaptation.

- [ ] 17. [DBCreaturesLegacy.cpp] JS error handler logs `e.stack` in addition to `e.message`; C++ only logs `e.what()` and has a comment noting the omission
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 108–111
  - **Status**: Pending
  - **Details**: JS logs both `e.message` and `e.stack` on error. C++ only logs `e.what()` and acknowledges the omission with a comment: "Note: JS also logs e.stack here, but C++ exceptions do not carry stack traces by default". This is an acceptable limitation (C++ has no standard stack-trace mechanism), but it means less diagnostic information on error. The comment is present so this is documented.

- [ ] 18. [DBCreaturesLegacy.cpp] JS `model_path` field lookup is `row.ModelName || row.ModelPath || row.field_2` (short-circuit OR); C++ uses sequential `if`-chain with separate `empty()` checks
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` lines 42–48
  - **Status**: Pending
  - **Details**: In JS, `row.ModelName || row.ModelPath || row.field_2` returns the first truthy value, falling through only on empty string, `0`, `null`, or `undefined`. The C++ code checks `model_path.empty()` after each field read, which only falls through on empty string. If a field exists and has a non-string value (e.g. a number 0 returned as a string "0"), JS would skip it while C++ would not (since "0" is non-empty). This is a minor edge-case discrepancy for unusual DBC schemas, but is practically safe since these fields are always strings.

- [ ] 19. [DBCreaturesLegacy.cpp] JS `model_id` lookup falls back to `row.field_1` via nullish coalescing `?? row.field_1`; C++ uses sequential `model_id_found` flag with `row.find("field_1")` fallback
  - **JS Source**: `src/js/db/caches/DBCreaturesLegacy.js` line 69
  - **Status**: Pending
  - **Details**: In JS: `const model_id = row.ModelID ?? row.field_1` — this falls back to `field_1` only when `ModelID` is `null` or `undefined`. The C++ uses a `model_id_found` flag and falls back to `"field_1"` only when `"ModelID"` is not found in the row map. The semantic is slightly different: in JS, a `ModelID` that exists but equals `0` would NOT fall back to `field_1` (since `0 ?? x` = `0`). In C++, if `"ModelID"` is present in the row and evaluates to `0`, `model_id` becomes 0 (correct). Both paths then proceed to look up `model_id_to_path.get(model_id)` / `model_id_to_path.find(model_id)`. Functionally equivalent in the expected data range.
