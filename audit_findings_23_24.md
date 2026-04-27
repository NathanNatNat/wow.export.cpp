# Audit Findings: Files 23–24 (wmv, wowhead)

## [wmv.cpp]

- FINDING: `get_legacy_value` uses `-1` as a NaN sentinel with a misleading comment
  - JS Source: `src/js/wmv.js` lines 87–91
  - Details: JS `parseInt(char_details?.skinColor?.['@_value'] ?? '0')` produces `NaN` when the attribute is present but non-numeric (e.g. `"abc"`). C++ `get_legacy_value` returns `parsed.value_or(-1)`, i.e. `-1`, in that case, and returns `0` when the value is null/missing (matching JS `parseInt('0')`). The downstream consumer in `tab_characters.cpp` (line 1732) guards with `static_cast<size_t>(value) < choices->size()`, so `-1` (wraps to `SIZE_MAX`) is always out-of-range and no customization is pushed — matching JS behaviour where `choices[NaN]` is `undefined` (falsy). The functional outcome is correct, but the two code comments in `get_legacy_value` conflate both cases rather than documenting them separately: the null path comment is accurate; the non-numeric path comment says "JS NaN sentinel" without explaining that `-1` triggers unsigned wrap-around downstream. This is a documentation accuracy issue, not a functional bug.

- FINDING: `ParseResultV1`/`ParseResultV2` variant — verify all callers dispatch correctly
  - JS Source: `src/js/wmv.js` lines 69–75, 113–120
  - Details: JS `wmv_parse` returns plain objects with different shapes (V1 has `legacy_values`, V2 has `customizations`). C++ models this as `std::variant<ParseResultV1, ParseResultV2>`. Callers must use `std::visit` or `std::get<>` to retrieve the active member; direct member access without dispatch would be a compile error. No specific error was found in the currently visible C++ code, but any new caller site should be verified for correct variant dispatch.

**Verified clean** (no deviations from JS source):
- `extract_race_gender_from_path` — race map entries, lowercase/backslash normalisation, gender detection logic (`male`/`female` substring logic), and error message all match JS exactly (lines 123–175).
- `wmv_parse_v1` and `wmv_parse_v2` equipment parsing — `normalize_array`, null guards, `wmv_slot`/`item_id` validity checks, and `get_slot_id_for_wmv_slot` call all match JS exactly (lines 96–112, 51–66).
- `wmv_parse_v2` customization parsing — array normalisation, `option_id`/`choice_id` extraction, and `isNaN` guard all match JS exactly (lines 35–47).
- All five `throw` error messages match JS character-for-character (lines 13, 22, 28, 82, 172).
- All JS exports present: JS exports only `wmv_parse`; C++ exposes `wmv::wmv_parse` via `wmv.h`.

## [wowhead.cpp]

- FINDING: `wowhead_parse_hash` accesses `hash[0]` without an internal empty-string guard
  - JS Source: `src/js/wowhead.js` line 64
  - Details: `wowhead_parse_hash` is a static internal function called only from `wowhead_parse`, which throws before calling it if the hash is empty. The guard is therefore sufficient, but the function has no internal assertion or check of its own. The JS function has the same implicit reliance on the caller. This is a minor defensive-programming concern, not a deviation from JS behaviour.

**Verified clean** (no deviations from JS source):
- `charset` — 59-character string `"0zMcmVokRsaqbdrfwihuGINALpTjnyxtgevElBCDFHJKOPQSUWXYZ123456"` matches JS exactly (line 7).
- `WOWHEAD_SLOT_TO_SLOT_ID` — all 13 entries (slots 1–13) match JS exactly (lines 11–25). C++ uses `std::unordered_map`; iteration order is irrelevant because all slot_id values are unique.
- `decode` — reverse-order iteration via `str[str.size() - 1 - i]` is equivalent to JS split-then-reverse; base multiplier `58` matches; single-character path returns raw `charset_index` result (can be -1) matching JS `charset.indexOf` (lines 27–49).
- `decompress_zeros` — `count == 0` appends nothing (matches JS `'08'.repeat(0)` = `""`); `count < 0` appends `'9'` + next char (matches JS returning the full regex match `_`) (lines 51–56).
- `parse_v15` — equip_start search from index 6, segment-prefix stripping, bonus-skip and enchant-skip conditions, and `wh_slot` advancement all match JS exactly. Empty-segment handling in bonus-skip matches: JS `"".startsWith('7')` is false so `!startsWith` is true; C++ `segments[seg_idx].empty()` is true — both skip (lines 74–181).
- `parse_v15` `seg.length >= 2` — JS and C++ both refuse to strip a single-char `"7"` prefix; the prior `seg.empty()` guard in C++ prevents the 7-prefix branch from being reached on empty input. No deviation (line 123).
- `parse_legacy` `slot_map_legacy` — all 8 entries match JS exactly; `std::unordered_map` iteration order is safe because segment indices and slot_id values are all unique (lines 204–213).
- `parse_legacy` item extraction — `seg.size() > 4 ? seg.size() - 4 : 0` is equivalent to `Math.max(0, seg.length - 4)` for all string lengths including 0, 4, and >4 (line 217).
- `extract_hash_from_url` — `std::regex_search` with pattern `"dressing-room#(.+)"` is equivalent to JS `url.match(/dressing-room#(.+)/)` for all valid wowhead URLs (line 59).
- Error message in `wowhead_parse` matches JS exactly: `"invalid wowhead url: missing dressing-room hash"` (line 240).
- All JS exports present: JS exports only `wowhead_parse`; C++ exposes `wowhead::wowhead_parse` via `wowhead.h`.
