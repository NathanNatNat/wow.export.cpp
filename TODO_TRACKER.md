# TODO Tracker

### 1. [buffer.cpp] Missing fromCanvas() method
- **JS Source**: `src/js/buffer.js` lines 89–106
- **Status**: Pending
- **Details**: JS `BufferWrapper.fromCanvas(canvas, mimeType, quality)` converts an HTML canvas to a buffer via `canvas.toBlob()`. This relies on browser Canvas/Blob APIs that have no direct C++ equivalent. Needs a replacement using the project's image I/O libraries (stb_image_write, libwebp) if canvas-like rendering is ever needed.

### 2. [buffer.cpp] Missing decodeAudio() method
- **JS Source**: `src/js/buffer.js` lines 981–983
- **Status**: Pending
- **Details**: JS `decodeAudio(context)` uses the browser `AudioContext.decodeAudioData()` API to decode audio from a buffer. C++ should use miniaudio for equivalent functionality when audio decoding from buffers is required.
