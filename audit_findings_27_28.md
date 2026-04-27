## [BoneMapper.cpp]

No findings.

All entries in `BONE_NAMES` (keys 0–89, 190–297 with documented gaps at 46–47 and 291) match the JS source exactly — both integer keys and string values. All entries in `CRC_BONE_NAMES` (uint32 keys with string values) match the JS source exactly. The `get_bone_name` function preserves the same priority order (BONE_NAMES → CRC_BONE_NAMES → `'bone_' + index` fallback) and produces identical output for all inputs. The `"bone_"` prefix is applied in all three branches, matching JS behaviour.

## [GeosetMapper.cpp]

No findings.

All 47 entries in `GEOSET_GROUPS` match the JS source exactly — keys and string values are identical. `getGeosetName` reproduces JS behaviour correctly: the `id == 0` early-return, the integer-division base computation (`(id / 100) * 100` is identical to `Math.floor(id / 100) * 100` for non-negative integers), the group-found branch (`group + (id - base)`), and the fallback branch (`"Geoset" + index + "_" + base` in JS / `"Geoset" + std::to_string(index) + "_" + std::to_string(base)` in C++) all match. The `map` function is synchronous `void` in C++ versus `async` in JS; because the JS body contains no `await`, the two are functionally equivalent. The `Geoset` struct in `GeosetMapper.h` has a `label` field of type `std::string`, correctly matching the JS `geoset.label` assignment. Both `map` and `getGeosetName` are accessible from the `geoset_mapper` namespace, satisfying the JS `module.exports = { map, getGeosetName }` export.
