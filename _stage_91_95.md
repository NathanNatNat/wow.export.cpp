## File: src/js/casc/export-helper.cpp (audit entry #91)

FINDING: `getIncrementalFilename` is synchronous in C++ but async in JS
  JS Source: src/js/casc/export-helper.js lines 97–114
  Details: The JS version is `async` and uses `await generics.fileExists(filePath)` in both the initial check and the do-while loop. The C++ version calls `generics::fileExists` synchronously. This is an intentional adaptation to C++ (no async needed for filesystem I/O at this level), but callers that previously awaited the result must now call it differently. The behavioral difference is minimal in practice, but worth noting as a deviation.

FINDING: `mark()` logs stack trace via `logging::write()` instead of `console.log()`
  JS Source: src/js/casc/export-helper.js lines 286–287
  Details: In the JS source, when `stackTrace != null`, the stack trace is printed with `console.log(stackTrace)` — which goes to stdout/the NW.js developer console, NOT the log file. In the C++ version (line 301–302 of export-helper.cpp), `logging::write(stackTrace.value())` is used instead, sending the stack trace to the log file. This is a behavioral deviation: in JS, the stack trace bypasses the log system; in C++ it is written to the log.

FINDING: `TOAST_OPT_DIR` constant is commented out but the comment style differs
  JS Source: src/js/casc/export-helper.js line 13
  Details: JS has `//const TOAST_OPT_DIR = { 'Open Export Directory': () => core.openExportDirectory() };`. The C++ preserves this as a commented-out line (export-helper.cpp line 28) but changes the comment to a different type. Minor but worth noting for fidelity.

AUDIT_TRACKER_CHECK: 91

---

## File: src/js/casc/install-manifest.cpp (audit entry #92)

FINDING: Tag pre-computation `(i % n * 8) + j` differs in intent from JS optional chaining
  JS Source: src/js/casc/install-manifest.js lines 57–64
  Details: In JS, `this.files[(i % n * 8) + j]?.tags.push(tag.name)` uses optional chaining `?.` which silently skips if the index is out of bounds (i.e. file index exceeds `files.length`). The C++ does an explicit bounds check `if (fileIdx < files.size())`. Functionally equivalent, but both the JS and C++ formula `(i % n * 8) + j` have the same operator-precedence result: `((i % n) * 8) + j`. The formula is correct for both.

FINDING: Constructor does not initialize `version`, `hashSize`, `numTags`, `numFiles`, `maskSize` via initializer list
  JS Source: src/js/casc/install-manifest.js lines 8–18
  Details: In the JS constructor, all fields are explicitly set to 0 before `parse()` is called. In the C++ header (install-manifest.h lines 47–51), all fields have inline default-member-initializers (`= 0`), which achieves the same effect. This is functionally identical and idiomatic C++. No issue.

NO_ISSUES (beyond the one finding above)

AUDIT_TRACKER_CHECK: 92

---

## File: src/js/casc/jenkins96.cpp (audit entry #93)

FINDING: JS uses signed 32-bit integer arithmetic (`| 0`) while C++ uses unsigned `uint32_t`
  JS Source: src/js/casc/jenkins96.js lines 9–12
  Details: JS initializes `a`, `b`, `c` using `| 0` at the end of each expression (e.g., `a = 0xDEADBEEF + len + init | 0`). The `| 0` converts to a signed 32-bit integer. All arithmetic in JS on these variables implicitly operates in 32-bit signed arithmetic (wrapping via two's complement). The C++ uses `uint32_t`, which wraps the same way but is unsigned. The bit patterns are identical after each operation since both use 32-bit wrapping modular arithmetic. The final JS `return [b >>> 0, c >>> 0]` forces unsigned 32-bit interpretation — precisely matching C++ `uint32_t`. This is a correct and intentional adaptation.

FINDING: Switch statement cases each break individually — no fall-through, same as JS
  JS Source: src/js/casc/jenkins96.js lines 30–43
  Details: Each `case` in the JS switch has a `break` statement, so there is no fall-through. The C++ mirrors this exactly. Both incorporate only a single byte per case (the specific byte at a fixed offset), which is correct for this Jenkins hash variant. No deviation.

NO_ISSUES (the JS-to-C++ mapping is correct and complete)

AUDIT_TRACKER_CHECK: 93

---

## File: src/js/casc/listfile.cpp (audit entry #94)

FINDING: `listfile_check_cache_expiry` uses `Number(...) || 0` in JS but plain `get<int64_t>()` in C++
  JS Source: src/js/casc/listfile.js lines 65–66
  Details: JS uses `Number(core.view.config.listfileCacheRefresh) || 0`, which coerces non-numeric config values (e.g., strings, null) to 0. The C++ uses `core::view->config["listfileCacheRefresh"].get<int64_t>()` which would throw a `nlohmann::json::type_error` if the config value is not a valid integer. The JS is more tolerant of misconfigured values; the C++ may crash on malformed configs. Should guard with a try/catch or use `.value()` with a fallback.

FINDING: `listfile_preload_legacy` uses `getFileDataIDsByExtension` which returns `vector<uint32_t>`, not `Map` as in JS binary mode
  JS Source: src/js/casc/listfile.js lines 459–468
  Details: In JS legacy mode, `preload_textures` etc. are plain arrays of fileDataIDs (returned by `getFileDataIDsByExtension`). The C++ correctly stores them as `std::vector<uint32_t>` (`preload_textures_ids`, etc.) in legacy mode. In binary mode, JS uses `new Map()` for these, and the C++ uses separate `*_map` and `*_order` structures. This is a correct structural adaptation.

FINDING: JS `applyPreload` binary mode `filter_and_format` iterates the preload Map directly in insertion order; C++ uses a separate `preload_order` vector
  JS Source: src/js/casc/listfile.js lines 583–601
  Details: In the JS binary mode `filter_and_format`, it iterates `preload_map.keys()` which yields keys in insertion order (ES2015+ Map guarantee). The C++ tracks insertion order separately via `preload_order` vectors to achieve the same ordering. This is functionally correct but adds implementation complexity. No behavioral deviation, just a structural difference.

FINDING: `applyPreload` return type mismatch — JS implicitly returns `undefined` on success; C++ returns `std::nullopt`
  JS Source: src/js/casc/listfile.js lines 548–621
  Details: The JS `applyPreload` function has no `return` at the end of the successful code path (after the try block), so it implicitly returns `undefined`. When `!is_preloaded`, it returns `0`. When `valid_entries === 0`, it returns `0` (inside try). The C++ returns `std::optional<int>`: `0` for the failure cases, `std::nullopt` for success. This is semantically equivalent — callers that need to distinguish should already handle this. Minor type-system difference, not a bug.

FINDING: `getFilenamesByExtension` in binary mode does not filter against `binary_id_to_pf_index` safely
  JS Source: src/js/casc/listfile.js lines 661–680
  Details: In the C++ binary mode path of `getFilenamesByExtension` (listfile.cpp lines 859–875), `binary_id_to_pf_index.find(fileDataID)` is called without checking if the iterator is `end()`. If the maps were somehow out of sync (which should not happen in normal operation), this would be undefined behavior. The JS version uses `binary_id_to_pf_index.get(id)` which returns `undefined` on miss, but the `binary_read_string_at_offset` call would then receive `undefined` as `pf_index`, causing different behavior. Both versions assume map consistency here, but the C++ version could dereference an invalid iterator.

FINDING: `preload()` JS checks `is_preloaded` before creating the promise; C++ has a subtle race condition in the async path
  JS Source: src/js/casc/listfile.js lines 498–507
  Details: The JS `preload()` function is single-threaded (Node.js event loop) so the `if (preload_promise) return preload_promise; if (is_preloaded) return true;` check is safe. The C++ uses a `std::mutex` guard in `preloadAsync()` for thread-safety. However, in `prepareListfileAsync()` (listfile.cpp lines 713–727), the `is_preloaded` check at line 714 happens OUTSIDE the mutex, and then the `preload_future` check at line 719 is inside the mutex. This creates a potential TOCTOU issue if `is_preloaded` transitions from false to true between the two checks on a different thread. This is a low-probability race but is a deviation from the JS single-threaded model.

FINDING: `getByFilename` MDL/MDX fallback check uses `.ends_with("mdx")` without dot, matching JS but allowing false positives
  JS Source: src/js/casc/listfile.js lines 856–857
  Details: JS checks `filename.endsWith('mdx')` (no leading dot). C++ mirrors this exactly: `lower.ends_with("mdx")`. This intentionally matches any filename ending in "mdx" (not just ".mdx"), which is how the JS works. Correctly ported.

AUDIT_TRACKER_CHECK: 94

---

## File: src/js/casc/locale-flags.cpp (audit entry #95)

FINDING: `flags_exports` and `names_exports` static arrays in the `.cpp` are `[[maybe_unused]]` dead code
  JS Source: src/js/casc/locale-flags.js lines 4–40
  Details: The `.cpp` file defines two `[[maybe_unused]]` static constexpr arrays (`flags_exports` and `names_exports`) that mirror the JS `module.exports.flags` and `module.exports.names` objects. These are never used — all actual usage goes through the `entries` array and helper functions in the `.h` file. These dead arrays should either be removed or documented as intentional. They also duplicate data already in `locale-flags.h`.

NO_ISSUES (beyond the one finding above — all 13 locales present, all flag values correct, commented-out locales enCN/enTW correctly preserved as comments)

AUDIT_TRACKER_CHECK: 95

---
