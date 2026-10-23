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

The static library is generated in the `build` directory as
`libfont_renderer.a`. The `src/main.cpp` file is kept as a reference/demo
entry point but is not built by the project.

## Use from another CMake project

The simplest development setup is to fetch this repository with CMake
`FetchContent`:

```cmake
include(FetchContent)

FetchContent_Declare(
    font_renderer
    GIT_REPOSITORY https://github.com/Aurelien-vr/font-renderer.git
    GIT_TAG 269de06011e8cfceeffe9acce2dd7b739ecda277
)

FetchContent_MakeAvailable(font_renderer)

target_link_libraries(my_app PRIVATE font_renderer)
```

Use a release tag or commit instead of `main` when possible, so builds remain
reproducible. The selected commit must already be pushed to GitHub.

The consuming project must also provide GLFW 3.4 or newer, OpenGL, and
FreeType. Before constructing the renderer, create an OpenGL context and load
OpenGL functions:

```cpp
#include <GLFW/glfw3.h>
#include "auFontRenderer.h"

// After glfwInit(), creating a window, making its context current,
// and loading OpenGL functions:
auFontRenderer font(window, {100, 300}, 64, {1, 1, 1});
font.loadFont();
font.auDraw("Hello");
```

The renderer must be used while the window's OpenGL context is current. The
optional last constructor argument selects the directory containing
`default.vert` and `default.frag`; pass an absolute or application-relative
path when the process is not started from the build directory:

```cpp
auFontRenderer font(window, {100, 300}, 64, {1, 1, 1}, "/path/to/ressources");
```

## Project status

This project is still under development. The current implementation uses FreeType for TrueType parsing. A custom parser, `auTTFparser`, may replace it in the future.

## Screenshots

![FontRenderer screenshot](https://github.com/user-attachments/assets/d641951a-59f9-436b-aebc-5b6701a31f83)

![FontRenderer detail](https://github.com/user-attachments/assets/f886a9c7-2259-46f7-af00-df952681ad3a)
