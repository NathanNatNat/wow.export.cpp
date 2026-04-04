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
