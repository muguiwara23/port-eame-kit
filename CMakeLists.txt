# Port/eame kit: the C library every Port/eame game links (porteame::kit).
#
#     add_subdirectory(port-eame-kit)          # the kit as a git submodule
#     target_link_libraries(game PRIVATE porteame::kit)
#
# It needs SDL3 (the SDL3::SDL3 target, found here if the game has not
# already); the OpenGL ES drawing (touch_gles2.c) is built when
# PORTEAME_GLES2 is ON, and the game links OpenGL ES itself (on iOS
# "-framework OpenGLES", which tools/build_sdl3_ios.sh adds to the link flags).
cmake_minimum_required(VERSION 3.20)
project(porteame_kit VERSION 0.1.0 LANGUAGES C)

option(PORTEAME_GLES2 "Build the OpenGL ES 2 drawing of the touch controls" ON)
option(PORTEAME_SDL_RENDERER "Build the SDL_Renderer drawing of the touch controls" ON)
option(PORTEAME_TESTS "Build the kit's own tests (Linux)" OFF)
set(PORTEAME_GLES_INCLUDE_DIR "" CACHE PATH "Khronos OpenGL ES headers where the system has none (a Linux test build)")

if(NOT TARGET SDL3::SDL3)
    find_package(SDL3 REQUIRED CONFIG)
endif()

add_library(porteame_kit STATIC
    src/ios.c
    src/touch.c
    src/touch_art.c)
add_library(porteame::kit ALIAS porteame_kit)
target_include_directories(porteame_kit PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(porteame_kit PUBLIC SDL3::SDL3)
set_target_properties(porteame_kit PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
if(NOT MSVC)
    target_link_libraries(porteame_kit PUBLIC m)
    target_compile_options(porteame_kit PRIVATE -Wall -Wextra)
endif()

if(PORTEAME_GLES2)
    target_sources(porteame_kit PRIVATE src/touch_gles2.c)
    if(PORTEAME_GLES_INCLUDE_DIR)
        target_include_directories(porteame_kit PRIVATE "${PORTEAME_GLES_INCLUDE_DIR}")
        target_compile_definitions(porteame_kit PRIVATE SDL_USE_BUILTIN_OPENGL_DEFINITIONS)
    endif()
endif()
if(PORTEAME_SDL_RENDERER)
    target_sources(porteame_kit PRIVATE src/touch_sdl.c)
endif()

if(PORTEAME_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
