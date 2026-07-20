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
target_link_libraries(native_port_spike PRIVATE glfw opengl32)
