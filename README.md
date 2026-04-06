# wow.export (C++ Conversion)

This project is a **personal project** and a **C++ conversion** of [**wow.export**](https://github.com/Kruithne/wow.export), the number one export toolkit for World of Warcraft. The original project was written in JavaScript/NW.js. This fork rewrites it in modern C++ (C++23) using native libraries instead of a browser-based runtime.

This is an independent effort and is not affiliated with or endorsed by the original wow.export project.

## Credits

All credit for the design, functionality, and original implementation of wow.export goes to its creators and contributors:

- **[Kruithne](https://github.com/Kruithne)** — Original project owner and primary developer
- **[Marlamin](https://github.com/Marlamin)** — Major contributor

This project would not exist without their work. Please see the full list of contributors on the [original repository](https://github.com/Kruithne/wow.export/graphs/contributors).

## License

This project is licensed under the **MIT License**, the same license as the original wow.export. See [LICENSE](LICENSE) for the full text.

## Dependencies

The C++ conversion is built upon the following libraries (all included as git submodules):

| Library | Purpose |
|---------|---------|
| [GLFW](https://github.com/glfw/glfw) | Windowing and input |
| [Dear ImGui](https://github.com/ocornut/imgui) (docking branch) | User interface (replaces Vue.js) |
| [GLAD2](https://github.com/Dav1dde/glad) | OpenGL 4.6 core profile loader |
| [GLM](https://github.com/g-truc/glm) | Mathematics (vectors, matrices, etc.) |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing and serialization |
| [spdlog](https://github.com/gabime/spdlog) | Logging (bundles fmt) |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | HTTP client for CDN access |
| [zlib](https://github.com/madler/zlib) | Compression/decompression |
| [stb](https://github.com/nothings/stb) | Image loading and writing (stb_image, stb_image_write) |
| [libwebp](https://github.com/webmproject/libwebp) | WebP image encoding/decoding |
| [nanosvg](https://github.com/memononen/nanosvg) | SVG parsing and rasterization |
| [miniaudio](https://github.com/mackron/miniaudio) | Audio playback |
| [pugixml](https://github.com/zeux/pugixml) | XML parsing |

Additional standard library features used: `std::jthread`, `std::async`, `std::filesystem`, `std::format`.

### Platforms

- **Windows x64** — MSVC (Visual Studio)
- **Linux x64** — GCC

### Language Standard

- C++23

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

