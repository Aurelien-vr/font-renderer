# FontRenderer

A small C++ library for rendering text with OpenGL.

FontRenderer loads TrueType fonts with [FreeType](https://freetype.org/), converts glyph outlines into geometry, triangulates them, and renders the result through OpenGL. Unlike bitmap-based text rendering, it uses glyph outlines so the text remains geometry-based.

## Features

- TrueType font loading through FreeType
- OpenGL text rendering
- Glyph-outline rendering instead of bitmap glyphs
- Glyph triangulation for rendering filled characters
- CMake-based build system
- Optional AddressSanitizer support for Debug builds

## Requirements

- C++17 compiler
- CMake 3.15 or newer
- GLFW 3.4 or newer
- OpenGL
- FreeType

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

Or use the included build script:

```bash
./build.sh
```

The executable is generated in the `build` directory.

## Project status

This project is still under development. The current implementation uses FreeType for TrueType parsing. A custom parser, `auTTFparser`, may replace it in the future.

## Screenshots

![FontRenderer screenshot](https://github.com/user-attachments/assets/d641951a-59f9-436b-aebc-5b6701a31f83)

![FontRenderer detail](https://github.com/user-attachments/assets/f886a9c7-2259-46f7-af00-df952681ad3a)
