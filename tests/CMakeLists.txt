# Port/eame kit: its own tests, on Linux (SDL3 with the offscreen driver,
# Mesa for OpenGL ES). See tests/README.md.
add_executable(test_touch test_touch.c)
target_link_libraries(test_touch PRIVATE porteame::kit)
add_test(NAME touch COMMAND test_touch)

# The iOS paths of ios.c, built here.
add_executable(test_ios test_ios.c ../src/ios.c)
target_include_directories(test_ios PRIVATE ../include)
target_compile_definitions(test_ios PRIVATE PE_TEST_IOS)
target_link_libraries(test_ios PRIVATE SDL3::SDL3)
add_test(NAME ios COMMAND test_ios "${CMAKE_CURRENT_BINARY_DIR}/ios-home")

if(PORTEAME_GLES2 AND PORTEAME_SDL_RENDERER)
    find_library(PORTEAME_GLESV2 NAMES GLESv2 libGLESv2.so.2 REQUIRED)
    add_executable(render_touch render_touch.c)
    target_link_libraries(render_touch PRIVATE porteame::kit "${PORTEAME_GLESV2}")
    if(PORTEAME_GLES_INCLUDE_DIR)
        target_include_directories(render_touch PRIVATE "${PORTEAME_GLES_INCLUDE_DIR}")
        target_compile_definitions(render_touch PRIVATE SDL_USE_BUILTIN_OPENGL_DEFINITIONS)
    endif()
    add_test(NAME render COMMAND render_touch "${CMAKE_CURRENT_BINARY_DIR}/touch")
    set_tests_properties(render PROPERTIES ENVIRONMENT "SDL_VIDEO_DRIVER=offscreen;ASAN_OPTIONS=detect_leaks=0")
endif()

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME package COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_package.py")
endif()
