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
    src/main.cpp

    src/glad.c

    src/Shader.cpp
    src/VAO.cpp
    src/VBO.cpp
    src/EBO.cpp
    src/DIB.cpp

    src/auFontRenderer.cpp
    src/auGlyph.cpp
)

set_source_files_properties(
    src/glad.c
    PROPERTIES
        COMPILE_OPTIONS "-w"
)

add_executable(font_renderer
    ${PROJECT_SOURCES}
)

target_include_directories(font_renderer
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/lib
)

target_link_libraries(font_renderer
    PRIVATE
        glfw
        OpenGL::GL
        Freetype::Freetype
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

# Copy shader files beside the executable.
add_custom_command(TARGET font_renderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:font_renderer>/ressources"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/src/ressources/default.vert"
        "$<TARGET_FILE_DIR:font_renderer>/ressources/default.vert"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/src/ressources/default.frag"
        "$<TARGET_FILE_DIR:font_renderer>/ressources/default.frag"
)

add_custom_target(run
    COMMAND font_renderer
    DEPENDS font_renderer
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:font_renderer>"
)
