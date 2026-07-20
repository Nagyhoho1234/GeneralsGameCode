cmake_minimum_required(VERSION 3.25)
project(native_port_spike CXX)

# Standalone Phase 3 graphics-API spike (docs/native-port-plan.md).
# NOT wired into the main game build - configure/build this directory on its own:
#   cmake -S native-port-spike -B native-port-spike/build
#   cmake --build native-port-spike/build
#
# Purpose: validate the D3D8-vs-GL semantics questions the plan's desk-check
# could not resolve on paper (clip-space/half-texel conventions, alpha-test-
# reference behavior without glAlphaFunc, buffer lifecycle) by driving a real
# GL 3.3 core-profile renderer off the same ShaderBits vocabulary shader.cpp
# uses, rather than by inspection alone.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    # Vendors glfw/glfw#2080 (never merged upstream) - see
    # patch_glfw_nsgl.cmake for why: GitHub's hosted macOS CI runners lack a
    # working accelerated GPU, and GLFW's NSGL backend unconditionally
    # requires one with no opt-out, which is what turns "no accelerated GPU"
    # into "no GL context at all" there. No-op on non-Apple platforms.
    PATCH_COMMAND ${CMAKE_COMMAND} -DGLFW_SRC_DIR=<SOURCE_DIR> -P ${CMAKE_CURRENT_SOURCE_DIR}/patch_glfw_nsgl.cmake
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# stb_image_write - single-header, public domain, used here purely to dump the
# offscreen-rendered frame to PNG so the result can be inspected without a
# visible window (matters for later headless Mesa/llvmpipe runs on Linux CI).
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/TheSuperHackers/stb.git
    GIT_TAG        5c205738c191bcb0abc65c4febfa9bd25ff35234
)
FetchContent_MakeAvailable(stb)

add_executable(native_port_spike
    main.cpp
    gl_core33.cpp
)
target_include_directories(native_port_spike PRIVATE ${stb_SOURCE_DIR})

if(WIN32)
    target_link_libraries(native_port_spike PRIVATE glfw opengl32)
else()
    # Linux/macOS: link the system GL library (Mesa's libGL, or macOS's
    # OpenGL framework via find_package) instead of Windows' opengl32 import
    # lib. GL 3.3+ entry points are still resolved dynamically via
    # glfwGetProcAddress (gl_core33.*) either way - this only supplies the
    # GL 1.1 symbols gl_core33.h calls directly (glClear, glGenTextures, ...).
    find_package(OpenGL REQUIRED)
    target_link_libraries(native_port_spike PRIVATE glfw OpenGL::GL)
endif()
