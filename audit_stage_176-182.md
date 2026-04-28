## File 176. [bitstream.cpp]

No findings.

## File 177. [build-version.cpp]

- build-version.cpp: `find_version_in_buffer` loop boundary differs from JS
  - **JS Source**: `src/js/mpq/build-version.js` lines 55–68
  - **Status**: Pending
  - **Details**: JS loops `while (pos < buf.length - 52)`, stopping before the final 52-byte window. C++ searches via `std::search` up to `buf.end()` and then calls `parse_vs_fixed_file_info` which validates the 52-byte window independently. In practice there is no behavioral difference, but the loop termination condition differs from JS.

- build-version.cpp: `DEFAULT_BUILD` constant from JS is not exposed in C++
  - **JS Source**: `src/js/mpq/build-version.js` lines 10, 159–162
  - **Status**: Pending
  - **Details**: JS exports `DEFAULT_BUILD = '1.12.1.5875'`. The C++ implementation does not declare this constant in `build-version.h`; the fallback value is hardcoded inline in `detect_build_version`. Any consumer that needs `DEFAULT_BUILD` by name has no access to it.

## File 178. [bzip2.cpp]

- bzip2.cpp: `N_ITERS` and `NUM_OVERSHOOT_BYTES` constants are commented out
  - **JS Source**: `src/js/mpq/bzip2.js` lines 51–53
  - **Status**: Pending
  - **Details**: JS defines `const N_ITERS = 4` and `const NUM_OVERSHOOT_BYTES = 20`. The C++ file comments both out. Neither is used in the decompression logic in JS or C++, but they should be present as uncommented constants to match the JS source faithfully.

## File 179. [huffman.cpp]

No findings.

## File 180. [mpq-install.cpp]

- mpq-install.cpp: `_scan_mpq_files` is not recursive; JS version recurses into subdirectories
  - **JS Source**: `src/js/mpq/mpq-install.js` lines 25–41
  - **Status**: Pending
  - **Details**: JS `_scan_mpq_files` recursively calls itself for subdirectories, finding MPQ files anywhere under the install root. C++ uses `fs::directory_iterator` which is non-recursive and only scans the top-level directory. MPQ files in subdirectories are silently missed. Should use `fs::recursive_directory_iterator` or implement recursive descent mirroring the JS.

- mpq-install.cpp: `getFilesByExtension` and `getAllFiles` return results in non-deterministic order
  - **JS Source**: `src/js/mpq/mpq-install.js` lines 87–120
  - **Status**: Pending
  - **Details**: JS iterates a `Map` in deterministic insertion order (archives loaded in sorted path order). C++ iterates an `std::unordered_map` in unspecified order. Callers that depend on a stable ordering will observe different results.

## File 181. [mpq.cpp]

No findings.

## File 182. [pkware.cpp]

No findings.
