# wow.export (C++ Conversion)

This project is a **C++ conversion** of [**wow.export**](https://github.com/Kruithne/wow.export), the number one export toolkit for World of Warcraft. The original project was written in JavaScript/NW.js. This fork rewrites it in modern C++ (C++23) using native libraries such as Dear ImGui, GLFW, and OpenGL.

## Credits

All credit for the design, functionality, and original implementation of wow.export goes to its creators and contributors:

- **[Kruithne](https://github.com/Kruithne)** — Original project owner and primary developer
- **[Marlamin](https://github.com/Marlamin)** — Major contributor

This project would not exist without their work. Please see the full list of contributors on the [original repository](https://github.com/Kruithne/wow.export/graphs/contributors).

## License

This project is licensed under the **MIT License**, the same license as the original wow.export. See [LICENSE](LICENSE) for the full text.

## Features

All features from the original wow.export are being ported, including:

- Support for both Retail and Classic game clients
- Complete online support allowing streaming of all files without a client
- Support for legacy MPQ-based installations for local browsing
- Full 3D preview of M2 and WMO game models (doodads included)
- Export models as OBJ, GLTF, and more
- Overhead map viewer with terrain, texture, and object exporting
- Sound file preview and export
- Video file (cinematic) preview and export
- Locale support for all 13 languages supported by the client
- DB2 database viewer
- In-game text, interface, and script viewing
- Font file preview and export

## Building

### Prerequisites

- **CMake** ≥ 3.20
- **MSVC** (Windows, via latest Visual Studio) or **GCC** (Linux)
- **Python 3** with `jinja2` (`pip install jinja2`) — required at build time for GLAD2 OpenGL loader generation
- All library dependencies are git submodules under `extern/`

### Windows (MSVC)

```powershell
git submodule update --init --recursive
pip install jinja2
cmake --preset windows-msvc-debug
cmake --build out/build/windows-msvc-debug
```

### Linux (GCC)

```bash
# Install system packages for GLFW/X11/OpenGL
sudo apt-get install -y libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

git submodule update --init --recursive
pip install jinja2
cmake --preset linux-gcc-debug
cmake --build out/build/linux-gcc-debug
```

Build presets are available for Debug, Release, and RelWithDebInfo on both platforms. See [CMakePresets.json](CMakePresets.json) for details.
