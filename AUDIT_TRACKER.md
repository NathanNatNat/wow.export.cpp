# JS → C++ Audit Tracker

> **This file tracks all deviations from the original JavaScript source.**
> Systemic translations (inherent to JS→C++ paradigm shift) are documented once as blanket entries.
> Individual deviations specific to one file/function get their own entry.

**Severity legend:**
- `ACCEPTABLE` — Intentional deviation, documented with rationale.
- `NEEDS REVIEW` — Deviation that may need further discussion.

---

## Systemic Translations (Blanket Entries)

These are NOT deviations — they are inherent structural translations from JS to C++.

### Module System
- **JS**: `require('./foo')` / `module.exports`
- **C++**: `#include "foo.h"` with declarations in the corresponding header file.
- Each `.cpp` file's `module.exports` becomes the public API in its paired `.h` file.
- Namespace structure mirrors the directory structure (e.g., `src/js/casc/` → `namespace casc`).

### Data Structure Mappings
- JS `Map` → `std::unordered_map` or `std::map`
- JS `Set` → `std::unordered_set` or `std::set`
- JS `Array` → `std::vector`
- JS `const` / `let` → C++ `const` / local variables
- JS `class` → C++ `class`

### Binary Data
- Node.js `Buffer` → `std::vector<uint8_t>` for owned buffers, `std::span<const uint8_t>` for non-owning views.

---

## Individual Deviations

### `src/js/MultiMap.cpp` — ACCEPTABLE
- **JS**: `MultiMap` extends `Map` with a custom `set()` that stores single values directly and converts to arrays on collision. Values can be any JS type.
- **C++**: `MultiMap<Key, Value>` is a template class using `std::variant<Value, std::vector<Value>>` internally. Additional `has()`, `erase()`, `clear()`, `size()`, and iterator methods are provided as they are standard `Map` methods available in JS via inheritance.
- **Rationale**: C++ cannot extend `std::unordered_map` with the same dynamic typing semantics. The template + variant approach preserves identical behavior: single values stored directly, converted to vector on collision. Extra methods correspond to inherited JS `Map` methods (`has`, `delete`, `clear`, `size`, `forEach`/iteration).

### `src/js/casc/locale-flags.cpp` — ACCEPTABLE
- **JS**: Exports an object with two sub-objects: `flags` (locale→number mapping) and `names` (locale→string mapping), keyed by locale ID strings.
- **C++**: Provides individual `constexpr uint32_t` constants for flags, a `constexpr std::array<LocaleEntry, 13>` combining both mappings, and `getFlag()`/`getName()` lookup helpers.
- **Rationale**: C++ cannot export a runtime object literal with string keys like JS. The combined array + lookup functions provide equivalent access patterns. The individual flag constants allow `casc::locale_flags::enUS` syntax matching JS `localeFlags.flags.enUS`.

### `src/js/casc/version-config.cpp` — ACCEPTABLE
- **JS**: Exports an anonymous function (`module.exports = data => {...}`) that takes a string and returns an array of plain objects with dynamic string keys.
- **C++**: Named function `casc::parseVersionConfig(std::string_view)` returning `std::vector<std::unordered_map<std::string, std::string>>`. Space removal matches JS `String.replace(' ', '')` exactly — only the first space is removed from each field name.
- **Rationale**: Anonymous exports need a name in C++. `parseVersionConfig` is derived from the filename. `unordered_map<string,string>` is the natural C++ equivalent of a plain JS object with string values.

### `src/js/casc/jenkins96.cpp` — ACCEPTABLE
- **JS**: Exports an anonymous function returning `[b >>> 0, c >>> 0]` (a two-element array). Uses `| 0` for int32 truncation and `>>>` for unsigned right shift.
- **C++**: Named function `casc::jenkins96(std::span<const uint8_t>, uint32_t, uint32_t)` returning `std::pair<uint32_t, uint32_t>`. All arithmetic uses `uint32_t` throughout, which naturally wraps at 32 bits (equivalent to JS `| 0` truncation). C++ `>>` on `uint32_t` is equivalent to JS `>>>`.
- **Rationale**: `std::pair` replaces the JS two-element array. Using `uint32_t` produces identical bit patterns to JS signed int32 arithmetic with `| 0`/`>>> 0` coercion.

### `src/js/xml.cpp` — ACCEPTABLE
- **JS**: Exports `parse_xml` function returning a plain JS object hierarchy. Uses closures over mutable `pos` variable for parser state.
- **C++**: Free function `parse_xml(std::string_view)` returning `nlohmann::json`. Uses a local `Parser` struct in an anonymous namespace to hold mutable state (equivalent to JS closure variables). Internal `Node` struct replaces JS object literals with `{tag, attrs, children, self_closing}`.
- **Rationale**: `nlohmann::json` is the natural C++ equivalent of JS's dynamic plain objects. The parser struct replaces JS closures while maintaining identical control flow and parse logic.

### `src/js/subtitles.cpp` — ACCEPTABLE
- **JS**: Exports `SUBTITLE_FORMAT` enum-like object and `get_subtitles_vtt(casc, file_data_id, format)` async function. Internal helper functions (`parse_sbt_timestamp`, `format_srt_timestamp`, `format_vtt_timestamp`, `parse_srt_timestamp`, `sbt_to_srt`, `srt_to_vtt`) are module-private in JS but only consumed by the exported function.
- **C++**: `subtitles::SubtitleFormat` enum class replaces the JS object. All functions are in the `subtitles` namespace. `get_subtitles_vtt` takes `(std::string_view text, SubtitleFormat format)` instead of `(casc, file_data_id, format)` — the CASC file loading is separated to the call site since this is a Tier 0 zero-dependency file. Helper functions (`parse_sbt_timestamp`, `format_srt_timestamp`, etc.) are public in the header since they are useful standalone and match the JS function names exactly. The BOM check (`charCodeAt(0) === 0xFEFF`) maps to UTF-8 BOM byte sequence (EF BB BF) since C++ operates on raw bytes.
- **Rationale**: Separating file I/O from format conversion is the natural C++ approach. The JS function is async only because of `casc.getFile()` — the conversion logic itself is synchronous. Making helpers public allows callers to use them directly (e.g., `sbt_to_srt`, `srt_to_vtt`) without going through the full pipeline.

### `src/js/hashing/xxhash64.cpp` — ACCEPTABLE
- **JS**: Exports `XXH64`, a dual-purpose function that acts as both a constructor (`new XXH64(seed)`) and a direct hash function (`XXH64(data, seed)`). Uses JS BigInt for 64-bit arithmetic with explicit `& MASK_64` masking. Internal `toUTF8Array(str)` converts UTF-16 JS strings to UTF-8 bytes. `XXH64_state` is the internal state class with `init()`, `update()`, `digest()` prototype methods.
- **C++**: `hashing::XXH64` class with constructor `XXH64(seed)`, methods `init()`, `update()`, `digest()` (matching JS names exactly), and static `hash()` for one-shot hashing (replaces the JS function-call pattern `XXH64(data, seed)`). `uint64_t` arithmetic naturally wraps at 64 bits, eliminating all `& MASK_64` operations. `toUTF8Array` is not ported — C++ `std::string_view` is already UTF-8 bytes, so the `update(std::string_view)` overload reinterprets directly. `read_u64_le`, `read_u32_le`, and `rotl64` are file-scope `static inline` functions matching their JS equivalents exactly.
- **Rationale**: JS BigInt `& MASK_64` is equivalent to C++ `uint64_t` modular arithmetic. `toUTF8Array` is a JS-specific UTF-16→UTF-8 conversion that has no C++ equivalent (strings are already byte sequences). The `XXH64::hash()` static method replaces JS's dual constructor/function pattern which doesn't exist in C++. `XXH64_state` is merged into `XXH64` since `XXH64.prototype = XXH64_state.prototype` in the JS source makes them share the same interface.

### `src/js/blob.cpp` — ACCEPTABLE
- **JS**: Exports `{ BlobPolyfill, URLPolyfill }`. Internal helpers include: `array2base64` (base64 encoder), `stringEncode` (UTF-16→UTF-8 encoder with surrogate pair handling), `stringDecode` (UTF-8→UTF-16 decoder with multi-byte sequence parsing), `textEncode`/`textDecode` (wrappers selecting native TextEncoder/TextDecoder or polyfill), `bufferClone` (ArrayBuffer clone), and JS type introspection functions (`getObjectTypeName`, `isPrototypeOf`, `isDataView`, `isArrayBuffer`, `arrayBufferClassNames`). `BlobPolyfill.stream()` returns a `ReadableStream` yielding 512KB chunks. `URLPolyfill.createObjectURL` has a fallback to native `URL.createObjectURL` for non-BlobPolyfill objects.
- **C++**: Classes `BlobPolyfill` and `URLPolyfill` at global scope. `array2base64` ported faithfully as a file-scope `static` function. `stringEncode`/`stringDecode` simplified to byte reinterpretation (C++ strings are already UTF-8, so UTF-16↔UTF-8 conversion is unnecessary). `textEncode`/`textDecode` are not separate — they map directly to the simplified `stringEncode`/`stringDecode`. `bufferClone` replaced by `std::vector` copy construction. JS type introspection functions (`getObjectTypeName`, `isPrototypeOf`, `isDataView`, `isArrayBuffer`, `arrayBufferClassNames`) replaced by C++ compile-time type dispatch via `BlobPart` overloaded constructors. `BlobPart` struct provides implicit construction from `std::span<const uint8_t>`, `std::vector<uint8_t>`, `std::string_view`, and `const BlobPolyfill&`. `stream()` uses a `std::function` callback instead of `ReadableStream` (no C++ equivalent). `arrayBuffer()` and `text()` return values directly instead of Promises (synchronous in C++). `URLPolyfill.createObjectURL` has no native fallback (all blobs are `BlobPolyfill` in C++). `size` and `type` are accessor methods instead of public properties.
- **Rationale**: JS `stringEncode`/`stringDecode` exist because JS strings are UTF-16 internally; C++ `std::string` is already UTF-8, making the encoding logic unnecessary. JS type introspection (`typeof`, `instanceof`, `Object.prototype.toString`) has no C++ equivalent — the type system handles this at compile time through overloaded constructors. `ReadableStream` has no C++ equivalent; a callback-based `stream()` provides identical chunking behavior (512KB chunks). Promises are unnecessary since all operations are synchronous in C++. The native `URL.createObjectURL` fallback path is unreachable in C++ (no other blob type exists).

### `src/js/constants.cpp` — ACCEPTABLE
- **JS**: Exports a flat object with 25+ top-level keys including runtime-computed filesystem paths (`INSTALL_PATH`, `DATA_DIR`, etc. via `path.join()`), nested objects (`GAME`, `CACHE`, `CONFIG`, `UPDATE`, `PATCH`, `BUILD`, `TIME`, `KINO`, `MAGIC`), arrays of objects (`PRODUCTS`, `FILE_IDENTIFIERS`, `EXPANSIONS`), string arrays (`NAV_BUTTON_ORDER`, `CONTEXT_MENU_ORDER`, `FONT_PREVIEW_QUOTES`), a regex (`LISTFILE_MODEL_FILTER`), and `VERSION` from `nw.App.manifest.version`. Module top-level code performs legacy directory migration and ensures data/log directories exist at `require()`-time.
- **C++**: All constants live in the `constants` namespace. Nested JS objects (`GAME`, `CACHE`, etc.) become nested namespaces. Runtime-computed paths are exposed as accessor functions (`const std::filesystem::path&`) populated by `init()`. Compile-time constants use `inline constexpr`. JS `path.join()` → `std::filesystem::path` `/` operator. `nw.App.manifest.version` → `inline constexpr std::string_view VERSION = "0.1.0"`. `LISTFILE_MODEL_FILTER` regex → `const std::regex&` via function-local static. `FILE_IDENTIFIERS.match` (string or array-of-strings) → `std::array<std::string_view, 4>` with `match_count`. `UPDATER_EXT` platform dispatch → `#ifdef _WIN32`. Module-level migration/directory-creation code → `init()` function. Executable path detection via `GetModuleFileNameW` (Windows) / `readlink("/proc/self/exe")` (Linux). Structs `Product`, `Region`, `Expansion`, `FileIdentifier` defined for typed array elements.
- **Rationale**: Runtime paths cannot be `constexpr` since they depend on the executable's location. An explicit `init()` function replaces JS module-load-time side effects (covered by systemic Module System translation). Nested namespaces naturally map JS nested objects (`constants.GAME.MAP_SIZE` → `constants::GAME::MAP_SIZE`). `std::array<string_view, 4>` for `FileIdentifier.matches` accommodates the MP3 entry which has 4 match patterns while keeping the data constexpr-compatible in the `.cpp` file. macOS (`darwin`) support removed per project scope (Windows/Linux only).

### `src/js/log.cpp` — ACCEPTABLE
- **JS**: Exports `{ write, timeLog, timeEnd, openRuntimeLog }`. Uses Node.js `fs.createWriteStream()` with stream backpressure (pool/drain pattern): when `stream.write()` returns `false`, messages are pooled (up to `MAX_LOG_POOL = 10000`) and drained via the `drain` event handler (`process.nextTick` for continued draining). `write(...parameters)` uses `util.format()` for printf-style variadic formatting. `timeLog()`/`timeEnd(label, ...params)` use `Date.now()` for millisecond timing. `openRuntimeLog()` uses `nw.Shell.openItem()`. `getErrorDump` is a global (non-exported) async function using `fs.promises.readFile()`. Debug mirror via `console.log(line)` when `!BUILD_RELEASE`.
- **C++**: All functions in `namespace logging` (renamed from `log` because `log` conflicts with the `log()` math function from `<cmath>` in MSVC's global namespace, causing error C2757). `std::ofstream` replaces `fs.createWriteStream()`. Pool/drain backpressure mechanism preserved: pool uses `std::deque<std::string>`, drain attempts on each `write()` call when `isClogged` (replaces JS `stream.on('drain', drainPool)` event). `write()` takes `std::string_view` (pre-formatted message) instead of variadic printf-style args — callers format before calling. `timeLog()`/`timeEnd()` use `std::chrono::steady_clock` instead of `Date.now()`. `timeEnd()` takes `std::string_view label` (simple label, no variadic params — callers format label before passing). `openRuntimeLog()` uses `ShellExecuteW` (Windows) / `xdg-open` via `std::system()` (Linux). `getErrorDump()` is a free function at global scope (synchronous, returns `std::string`). Debug mirror uses `std::fputs(line, stdout)` gated by `#ifndef NDEBUG` (replaces `!BUILD_RELEASE`). Thread safety via `std::mutex` (JS is single-threaded). `init()` function replaces module-level stream creation.
- **Rationale**: `namespace log` is impossible in C++ because `log` is a reserved identifier in `<cmath>` (the logarithm function) and MSVC brings it into the global namespace, causing compilation error C2757. `namespace logging` is the closest non-conflicting name. Node.js stream backpressure is fundamentally different from C++ synchronous file I/O — `std::ofstream` blocks on write, so the pool rarely fills. The mechanism is preserved for error recovery (disk full, etc.). `std::chrono::steady_clock` is more appropriate than `Date.now()` for elapsed time measurement (monotonic clock, not affected by system time changes). `write(string_view)` instead of variadic format matches C++ idiom where callers use `fmt::format()` or string concatenation. `std::system("xdg-open ...")` is safe because the path comes from internal `constants::RUNTIME_LOG()`, not user input. `NDEBUG` is the standard C++ equivalent of JS build-mode checks.

### `src/js/file-writer.cpp` — ACCEPTABLE
- **JS**: Exports `FileWriter` class. Constructor takes `(file, encoding = 'utf8')` and creates a Node.js `fs.createWriteStream`. `writeLine(line)` is async — it awaits a Promise if the stream is blocked (backpressure), then writes `line + '\n'`. If `stream.write()` returns `false`, sets `blocked = true` and registers a one-time `drain` listener. `_drain()` clears `blocked` and resolves the waiting Promise via `this.resolver?.()`. `close()` calls `stream.end()`.
- **C++**: `FileWriter` class with same API. Constructor takes `(const std::filesystem::path&, std::string_view encoding = "utf8")` and opens an `std::ofstream`. `encoding` parameter is accepted but unused (C++ streams write raw bytes; callers provide UTF-8 text directly). `writeLine(std::string_view)` is synchronous — `std::ofstream` writes block naturally, eliminating the need for async/await and Promise-based backpressure. If the stream was previously in a failed state (`blocked == true`), `_drain()` is called to reset. `_drain()` simply clears `blocked`. `close()` calls `stream.close()`. The `resolver` member (JS Promise resolve function) has no C++ equivalent and is omitted.
- **Rationale**: C++ file I/O is synchronous — writes block until complete, so the JS async/await + Promise backpressure pattern is unnecessary. The `blocked`/`_drain()` mechanism is preserved for error-state recovery (e.g., disk full). `encoding` is kept as a parameter for API fidelity but has no effect since C++ `std::ofstream` does not perform character encoding — callers are expected to provide properly encoded text. `resolver` is omitted because there is no Promise to resolve in synchronous C++.