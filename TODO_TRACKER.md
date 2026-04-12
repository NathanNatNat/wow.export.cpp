# TODO Tracker

### 1. [buffer.cpp] Missing fromCanvas() method
- **JS Source**: `src/js/buffer.js` lines 89–106
- **Status**: Pending
- **Details**: JS `BufferWrapper.fromCanvas(canvas, mimeType, quality)` converts an HTML canvas to a buffer via `canvas.toBlob()`. This relies on browser Canvas/Blob APIs that have no direct C++ equivalent. Needs a replacement using the project's image I/O libraries (stb_image_write, libwebp) if canvas-like rendering is ever needed.

### 2. [buffer.cpp] Missing decodeAudio() method
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Status**: Pending
- **Details**: JS `decodeAudio(context)` uses the browser `AudioContext.decodeAudioData()` API to decode audio from a buffer. C++ should use miniaudio for equivalent functionality when audio decoding from buffers is required.

### 3. [blte-reader.cpp] Missing decodeAudio() override
- **JS Source**: `src/js/casc/blte-reader.js` lines 337–340
- **Status**: Pending
- **Details**: JS `BLTEReader.decodeAudio(context)` calls `processAllBlocks()` before delegating to `super.decodeAudio(context)`. This override ensures all BLTE blocks are decoded before audio data is consumed. Blocked until `BufferWrapper.decodeAudio()` is implemented (TODO #2).

### 4. [blte-stream-reader.cpp] Missing createReadableStream() method
- **JS Source**: `src/js/casc/blte-stream-reader.js` lines 168–193
- **Status**: Pending
- **Details**: JS `createReadableStream()` creates a browser `ReadableStream` for progressive block consumption with pull-based semantics. No direct C++ equivalent exists. May be implemented as a C++ iterator/generator pattern or callback-based streaming API if needed by consumers.

### 5. [blp.cpp] Missing toCanvas() and drawToCanvas() methods
- **JS Source**: `src/js/casc/blp.js` lines 103–117, 221–234
- **Status**: Pending
- **Details**: JS `toCanvas()` creates an HTML canvas and `drawToCanvas()` renders BLP data onto it using the browser Canvas 2D API. These are browser-specific methods with no direct C++ equivalent. Rendering to Dear ImGui textures is handled separately. Not needed unless direct canvas-style output is required.
