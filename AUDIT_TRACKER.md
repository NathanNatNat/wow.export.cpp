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
