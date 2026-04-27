## [xml.cpp]

- FINDING: `build_object` uses `std::unordered_map` where JS uses `Object.entries`, producing different child property ordering in the result object.
  - JS Source: `src/js/xml.js` lines 148–153
  - Details: JS `Object.entries(groups)` iterates child-tag groups in insertion order (i.e., the order the distinct tag names first appeared among the children). C++ `std::unordered_map` iterates in an unspecified hash order. For the same XML input, the resulting JSON object can have its child-tag keys in a different order. JSON objects are unordered by spec, so callers that iterate with `items()` / range-for may see a different sequence than the JS caller would. In practice this does not break correctness for WoW data files (which are consumed by key name, not position), but it is a behavioural divergence from the JS source.

- FINDING: C++ `parse_attributes` and `parse_node` contain additional out-of-bounds guards not present in the JS source.
  - JS Source: `src/js/xml.js` lines 39–54, 60–98
  - Details: The JS closures do not check `pos >= xml.length` after every `pos++` or after every `skip_whitespace()` call. The C++ Parser adds several extra `if (pos >= xml.size()) break/return` guards (e.g., after consuming `<`, after `skip_whitespace()` inside `parse_attributes`, and on the closing `pos++` after consuming `>`). These guards make the C++ implementation more robust to malformed/truncated XML, but they represent defensive additions absent from the JS source. They do not alter behaviour for well-formed input.

- FINDING: `build_object` null guard is present in JS but has no C++ equivalent; however, it is dead code in both versions.
  - JS Source: `src/js/xml.js` lines 129–131
  - Details: JS `build_object` begins with `if (!node) return {};`. This guard could theoretically be reached if `build_object` were called directly with `null`, but in practice all call sites either check for a non-null node first (`if (node) root[node.tag] = build_object(node)`) or only call it on elements already pushed into a children array (which only accepts non-null nodes). The C++ version accepts `const Node&` and the `Node::valid` flag ensures only valid nodes are ever processed, so the guard is correctly omitted. No functional difference.

- FINDING: JS text-content nodes cause an infinite loop in the children parse loop; C++ has the same behaviour.
  - JS Source: `src/js/xml.js` lines 102–115
  - Details: If the XML contains bare text between element tags (e.g., `<root>hello</root>`), `parse_node` returns null/invalid without advancing `pos`, and the enclosing `while` loop spins indefinitely. This is a shared limitation in both JS and C++ — neither advances past non-`<` content that is not whitespace. WoW data XML files do not use text content, so this is an acceptable shared assumption, not a C++-specific deviation.

## [AnimMapper.cpp]

> Verified: Both the JS and C++ `ANIM_NAMES` arrays contain exactly 1776 entries in identical order, from `"Stand"` (index 0) to `"FlyEmoteTalkFrustrated"` (index 1775).

- FINDING: `get_anim_name` bounds check differs syntactically but is semantically equivalent for all integer inputs.
  - JS Source: `src/js/3D/AnimMapper.js` lines 1792–1797
  - Details: JS uses `if (anim_id in ANIM_NAMES)`, which tests for the existence of a property key on the array object; for a dense integer-indexed array this is true exactly when `anim_id >= 0 && anim_id < ANIM_NAMES.length`. C++ uses `if (anim_id >= 0 && static_cast<size_t>(anim_id) < ANIM_NAMES.size())`, which is the direct arithmetic equivalent. The only theoretical divergence would be a non-integer `anim_id` (e.g., `1.5`): JS `in` would return false (no property `"1.5"`), while C++ `static_cast<size_t>(1.5)` truncates to `1` and returns the entry at index 1. `anim_id` is always an integer in practice (read from binary WoW data), so this is not a real concern.

- FINDING: JS uses `module.exports` (CommonJS) while the pre-analysis note stated ES module `export`; the C++ free-function export is correct regardless.
  - JS Source: `src/js/3D/AnimMapper.js` line 1799
  - Details: `AnimMapper.js` ends with `module.exports = { get_anim_name };` (CommonJS), not an ES module `export`. The C++ equivalent is a free function declared in `AnimMapper.h`, which correctly replaces either module system. No action needed; noted only for accuracy.
