## [tiled-png-writer.cpp]

- FINDING: `write()` uses a detached thread + `std::shared_future<void>` instead of JS `async`/`Promise`
  - JS Source: `src/js/tiled-png-writer.js` lines 123–125
  - Details: JS `write()` is `async` and returns a `Promise<void>` via `await this.getBuffer().writeToFile(file)`. The C++ version calls `getBuffer()` synchronously on the caller's thread, then spawns a detached `std::thread` for the file I/O, returning a `std::shared_future<void>`. Because the thread is detached, if the caller discards the returned future, any exception thrown by `buffer.writeToFile` is silently lost — JS always propagates write errors to the awaiting caller, C++ does not. Whether callers in this codebase always call `.get()` on the returned future needs to be verified.

- FINDING: `std::map` iteration order differs from JS `Map` insertion order, breaking alpha blending for overlapping tiles
  - JS Source: `src/js/tiled-png-writer.js` lines 25, 58–59
  - Details: JS `this.tiles` is a `Map()` (insertion-ordered). C++ uses `std::map<std::string, Tile>` (lexicographic key order). Keys are `"tileX,tileY"` strings, so `"10,0"` sorts before `"2,0"`, which does not match numeric insertion order. The existing header comment argues that iteration order is irrelevant because rendering is position-based — this is only true for fully-opaque pixels (srcA == 1, direct overwrite). For semi-transparent overlapping tiles the Porter-Duff "over" composite is order-dependent: blending tile A over B yields a different result than B over A. The header comment should be updated to note this alpha-blending caveat.

- FINDING: `getStats()` `expectedTiles` field uses `uint32_t` for the product of two `uint32_t` values without overflow protection
  - JS Source: `src/js/tiled-png-writer.js` lines 131–140
  - Details: JS computes `this.tileCols * this.tileRows` in Number (64-bit float), which handles large values without overflow. C++ computes `tileCols * tileRows` as `uint32_t * uint32_t` in `tiled-png-writer.cpp` line 138. For very large images (e.g. 65536 × 65536 pixels with 1-pixel tiles, giving 65536 × 65536 = 4 294 967 296 tiles) this silently overflows `uint32_t`. The `Stats::expectedTiles` field is declared `uint32_t` in the header — it should be `size_t` or `uint64_t` to match JS Number semantics.


## [updater.cpp]

- FINDING: `BUILD_GUID()` generates a fresh random UUID each run instead of using a fixed build-specific GUID
  - JS Source: `src/js/updater.js` lines 24, 33–36
  - Details: JS reads `nw.App.manifest.guid` — a fixed GUID baked into the NW.js manifest at release time, identical across all runs of the same build. C++ `constants::BUILD_GUID()` (`constants.cpp` lines 234–255) generates a UUID v4 at startup via `std::random_device` and caches it for the process lifetime. Because the local GUID changes on every launch, the comparison `remoteGuid != localGuid` at `updater.cpp` line 88 will almost always be true, so `checkForUpdates()` will almost always report an update available even when none exists. This makes the update check non-functional.

- FINDING: `applyUpdate()` omits `permissions` from the `downloadFile` call, and `FileNode` never captures it
  - JS Source: `src/js/updater.js` lines 59–64, 120
  - Details: JS calls `generics.downloadFile(remoteEndpoint, localFile, node.meta.ofs, node.meta.compSize, true, node.meta.permissions)` — the sixth argument passes the Unix file mode from the update manifest (e.g. `0o755` for executables) to `fsp.chmod()` after writing. The C++ `FileNode` struct (`updater.cpp` lines 114–120) has no `permissions` field — the manifest value is never read. Consequently `generics::downloadFile` is called with only five arguments (`updater.cpp` line 204) and defaults to mode `0600`. On Linux, executable files downloaded by the updater (including the updater binary itself) will be written as non-executable, causing `launchUpdater()` to fail. On Windows the `chmod` call is compiled out so the bug is latent there.

- FINDING: `utilFormat()` replaces only the first `%s`, unlike Node.js `util.format()` which replaces all
  - JS Source: `src/js/updater.js` lines 25, 113
  - Details: `util.format(fmt, ...args)` replaces every `%s` in sequence. `utilFormat()` at `updater.cpp` lines 53–59 uses a single `find` + `replace` and returns, leaving any subsequent `%s` as a literal in the output string. The current `updateURL` config value (`"https://www.kruithne.net/wow.export/update/%s/"`) contains only one `%s`, so there is no bug today. If the config value is ever changed to contain multiple placeholders, the function will silently produce a malformed URL.

- FINDING: `applyUpdate()` is synchronous, blocking the calling thread for the full duration of update verification and download
  - JS Source: `src/js/updater.js` lines 50–125
  - Details: JS `applyUpdate()` is `async` and yields to the event loop at every `await`, keeping the UI responsive. The C++ version runs all file-stat, hash, directory-create, and HTTP-download operations synchronously on the calling thread. This is the documented JS async → C++ synchronous mapping (noted in `updater.cpp` lines 102–111) and the step ordering is identical, but it will freeze the UI for the full duration of the update process if called on the render thread.

- FINDING: `launchUpdater()` calls `std::exit(0)` without flushing or destroying application state
  - JS Source: `src/js/updater.js` lines 161–162
  - Details: JS calls `process.exit()` from within the NW.js renderer process. C++ calls `std::exit(0)`, which invokes `atexit` handlers and destroys static-duration objects but skips destructors for all objects on the call stack above `launchUpdater()`. Logging handles, open file streams, and OpenGL contexts held by stack objects will not be properly closed. This is acceptable for a deliberate "restart for update" exit path but should be noted as a deviation from the JS behavior where the NW.js runtime performs its own orderly shutdown.
