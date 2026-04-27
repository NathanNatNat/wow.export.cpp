# Audit Stage: Files 121–125

<!-- ─── src/js/db/DBDParser.cpp ─────────────────────────────── -->

- [ ] 1. [DBDParser.cpp] Column name `?` replacement removes only the last `?` instead of all `?` characters
  - **JS Source**: `src/js/db/DBDParser.js` line 339
  - **Status**: Pending
  - **Details**: JS uses `match[3].replace('?', '')` which replaces **all** occurrences of `?` in the column name string. The C++ only removes the trailing `?` with `if (!columnName.empty() && columnName.back() == '?') columnName.pop_back();`. If a column name contains a `?` anywhere other than the end (e.g. `"Field?Test?"`), the JS produces `"FieldTest"` while C++ produces `"Field?Test"`. In practice DBD column names only have a trailing `?`, but the logic is not a faithful port.

- [ ] 2. [DBDParser.cpp] Annotation parsing uses substring search instead of exact token match
  - **JS Source**: `src/js/db/DBDParser.js` lines 286–294
  - **Status**: Pending
  - **Details**: JS splits annotations string by comma and uses `Array.includes()` for exact token matching: `fieldMatch[2].split(',').includes('id')`. The C++ uses `annotations.find("id") != std::string::npos`, which is a substring search. If a future annotation contained `"id"` as a substring (e.g. `"validid"`), the C++ would incorrectly set `field.isID = true`. The same applies to `"noninline"` and `"relation"`. The current known annotation values (`id`, `noninline`, `relation`) do not overlap as substrings, so there is no current breakage, but this is a deviation from the JS semantics.

- [ ] 3. [DBDParser.cpp] Empty chunks passed to `parseChunk()` are silently skipped in C++ but generate empty `DBDEntry` objects in JS
  - **JS Source**: `src/js/db/DBDParser.js` lines 216–221, 238–242
  - **Status**: Pending
  - **Details**: In the JS `parse()` loop, `parseChunk(chunk)` is called every time an empty line is encountered, even when `chunk` is still an empty array (e.g. consecutive blank lines at the start of the file). When `chunk` is `[]`, `chunk[0]` is `undefined`, so `parseChunk` falls into the `else` branch and pushes an empty `DBDEntry` with no fields into `this.entries`. The C++ guards with `if (!chunk.empty() && chunk[0] == "COLUMNS")` and only processes non-empty chunks. In the else branch, `entries.push_back(std::move(entry))` is only reached when `chunk` is non-empty. The empty `DBDEntry` objects in JS are harmless (they never match anything in `isValidFor()`), so there is no functional impact, but the structural behavior differs.

<!-- ─── src/js/db/FieldType.cpp ─────────────────────────────── -->
<!-- No issues found -->

<!-- ─── src/js/db/WDCReader.cpp ─────────────────────────────── -->

- [ ] 4. [WDCReader.cpp] `getRelationRows()` incorrectly requires preload; JS does not
  - **JS Source**: `src/js/db/WDCReader.js` lines 216–234
  - **Status**: Pending
  - **Details**: The JS `getRelationRows()` only requires the table to be loaded (`isLoaded`). It reads each record directly via `_readRecord()` without requiring `preload()`. The C++ version at lines 305–309 throws a `std::runtime_error` if `rows` is not preloaded: `"Table must be preloaded before calling getRelationRows. Use db2.preload.<TableName>() first."` This is a behavioral deviation — callers that call `getRelationRows()` without first calling `preload()` will work in JS but throw in C++. The JS comment `Required for getRelationRows() to work properly` appears to be advisory; it does not enforce it. The C++ should remove the preload requirement and instead call `_readRecord()` directly for each record ID in the lookup, matching JS behavior.

- [ ] 5. [WDCReader.cpp] `getAllRowsAsync()` returns `std::map` by value instead of by reference, inconsistent with `getAllRows()` which returns a const reference
  - **JS Source**: `src/js/db/WDCReader.js` lines 141–193
  - **Status**: Pending
  - **Details**: The synchronous `getAllRows()` returns `const std::map<uint32_t, DataRecord>&` — a reference to either `rows` or `transientRows`. The async version `getAllRowsAsync()` at line 1297–1299 returns `std::future<std::map<uint32_t, DataRecord>>` (by value, making a copy). While making a copy is safe, there is a subtle threading issue: if `preload()` has not been called, `getAllRows()` fills `transientRows` (a member variable) and returns a reference to it. A concurrent call to `getAllRowsAsync()` could race with `getAllRows()` filling `transientRows` from another thread. The JS original is single-threaded so this risk does not exist in JS. The async methods are C++-only additions, but the `transientRows` approach is not thread-safe.

- [ ] 6. [WDCReader.cpp] `idFieldIndex` is initialized to `0` instead of `null`; `idField` is `optional<string>` instead of `null` — minor behavioral difference in unloaded state
  - **JS Source**: `src/js/db/WDCReader.js` lines 79–80
  - **Status**: Pending
  - **Details**: The JS constructor sets `this.idField = null` and `this.idFieldIndex = null`. The C++ has `idFieldIndex = 0` (uint16_t) and `idField` as `std::optional<std::string>` (empty). The value `0` for `idFieldIndex` before loading is indistinguishable from a valid index of `0`, whereas JS `null` is clearly uninitialized. The `getIDIndex()` method in both JS and C++ guards with `isLoaded`, so callers should not access these before loading. However, the C++ `_readRecordFromSection` references `idFieldIndex` at line 1269 without checking `isLoaded` (it is called internally after loading starts), so the uninitialized `0` is never used incorrectly in practice. This is a low-severity deviation.

- [ ] 7. [WDCReader.cpp] BitpackedIndexedArray index arithmetic uses integer multiplication instead of BigInt, potential overflow for large `bitpackedValue`
  - **JS Source**: `src/js/db/WDCReader.js` lines 820–822
  - **Status**: Pending
  - **Details**: The JS computes the pallet data index as `bitpackedValue * BigInt(recordFieldInfo.fieldCompressionPacking[2]) + BigInt(i)` using BigInt arithmetic which is arbitrary-precision and cannot overflow. The C++ computes `static_cast<size_t>(bitpackedValue * arrSize + i)` at line 1149 where `bitpackedValue` is `uint64_t` and `arrSize` is `uint32_t`. The multiplication `bitpackedValue * arrSize` is done in uint64_t, which could theoretically overflow for pathologically large values, though in practice DB2 pallet data indices are small. This is a theoretical deviation.

<!-- ─── src/js/db/caches/DBCharacterCustomization.cpp ─────────────────────────────── -->

- [ ] 8. [DBCharacterCustomization.cpp] `chr_cust_mat_map` is keyed by `materialID` (the lookup key) instead of `mat_row.ID` (the row's own ID field)
  - **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` line 102
  - **Status**: Pending
  - **Details**: JS keys `chr_cust_mat_map` by `mat_row.ID`, the row's own `ID` field from the `ChrCustomizationMaterial` DB2 table: `chr_cust_mat_map.set(mat_row.ID, {...})`. The C++ uses `materialID` (the value from `ChrCustomizationMaterialID` in the element row): `chr_cust_mat_map[materialID] = mat_info`. Since the row is retrieved via `getRow(materialID)`, the row's own `ID` should equal `materialID` for direct (non-copy-table) rows. However, if the row was accessed via the copy table (JS `getRow` sets `tempCopy.ID = recordID` which would be the destination copy ID), the row's `ID` field would differ from the original source `materialID`. This is an edge case but represents a semantic deviation from the JS source.

- [ ] 9. [DBCharacterCustomization.cpp] `ensureInitialized()` uses a synchronous blocking pattern with mutex instead of the JS async promise-caching pattern
  - **JS Source**: `src/js/db/caches/DBCharacterCustomization.js` lines 39–48
  - **Status**: Pending
  - **Details**: The JS uses a promise-caching pattern: if `init_promise` is set, it `await`s the same promise, ensuring all concurrent async callers share a single initialization pass. The C++ uses `is_initializing` with a `std::condition_variable` wait. Both achieve the same goal of single-initialization with concurrent waiter support. The C++ also runs `_initialize()` synchronously on the calling thread (blocking), whereas JS runs it asynchronously. This is an intentional C++ adaptation since `std::async`/`std::future` equivalents are not used here. The functional behavior is preserved. No fix required, but worth noting as a structural deviation.

<!-- ─── src/js/db/caches/DBComponentModelFileData.cpp ─────────────────────────────── -->

- [ ] 10. [DBComponentModelFileData.cpp] `initialize()` does not implement the promise-deduplication pattern from JS — async concurrent callers would each block instead of sharing one init
  - **JS Source**: `src/js/db/caches/DBComponentModelFileData.js` lines 18–43
  - **Status**: Pending
  - **Details**: The JS `initialize()` caches an `init_promise` and returns it to all concurrent callers, ensuring the initialization IIFE runs only once even under concurrent async calls. The C++ `initialize()` uses a mutex and `is_initializing` flag with a `std::condition_variable` to prevent double-initialization and wait for completion. This is functionally equivalent for the concurrency case (second caller blocks until initialization completes). The structure is different but the observable behavior is preserved. Low-severity deviation.

- [ ] 11. [DBComponentModelFileData.cpp] `initializeAsync()` is a C++-only addition with no JS counterpart
  - **JS Source**: `src/js/db/caches/DBComponentModelFileData.js` (no equivalent)
  - **Status**: Pending
  - **Details**: The C++ exposes `initializeAsync()` which wraps `initialize()` in `std::async`. This has no equivalent in the JS source. The JS `initialize()` itself is already async (returns a Promise). This is an intentional addition for the C++ async model and is not a bug, but it is an addition beyond the JS API surface.
