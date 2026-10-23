cmake_minimum_required(VERSION 3.15)

project(font_renderer
    VERSION 0.1
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_ASAN "Enable AddressSanitizer in Debug builds" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 3.4 REQUIRED)
find_package(Freetype REQUIRED)

set(PROJECT_SOURCES
    src/glad.c

    src/graphics/Shader.cpp
    src/graphics/VAO.cpp
    src/graphics/VBO.cpp
    src/graphics/EBO.cpp
    src/graphics/DIB.cpp

    src/font/auColor.cpp
    src/font/auFontRenderer.cpp
    src/font/auGlyph.cpp

    src/geometry/bezier.cpp
    src/geometry/earClipping.cpp
)

set_source_files_properties(
    src/glad.c
    PROPERTIES
        COMPILE_OPTIONS "-w"
)

add_library(font_renderer STATIC
    ${PROJECT_SOURCES}
)

target_include_directories(font_renderer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/font
        ${PROJECT_SOURCE_DIR}/src/graphics
        ${PROJECT_SOURCE_DIR}/src/geometry
        ${PROJECT_SOURCE_DIR}/lib
)

target_link_libraries(font_renderer
    PUBLIC
        glfw
        Freetype::Freetype
    PRIVATE
        OpenGL::GL
)

target_compile_options(font_renderer
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

if(ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(font_renderer
        PRIVATE
            $<$<CONFIG:Debug>:-fsanitize=address>
            $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
    )

    target_link_options(font_renderer
        PRIVATE
            $<$<CONFIG:Debug>:-fsanitize=address>
            $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
    )
endif()
