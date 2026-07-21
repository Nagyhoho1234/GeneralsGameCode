# GLFW + system OpenGL for the PortableD3D8 GL backend (dx8wrapper_gl.cpp,
# native port plan Phase 5(a)). Non-Windows only - Windows keeps using the
# real D3D8 backend (dx8wrapper_d3d8.cpp, cmake/dx8.cmake). Mirrors
# native-port-spike/CMakeLists.txt, whose GLFW+GL setup was already proven on
# real Windows/Linux/macOS CI.

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    # Vendors glfw/glfw#2080 (never merged upstream): GitHub's hosted macOS CI
    # runners lack a working accelerated GPU, and GLFW's NSGL backend
    # unconditionally requires one with no opt-out. No-op on non-Apple
    # platforms. See cmake/patch_glfw_nsgl.cmake.
    PATCH_COMMAND ${CMAKE_COMMAND} -DGLFW_SRC_DIR=<SOURCE_DIR> -P ${CMAKE_CURRENT_LIST_DIR}/patch_glfw_nsgl.cmake
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
# X11 only - CI/WSL2 build images don't reliably have wayland-scanner
# installed, and this milestone's harness runs headless (offscreen FBO +
# glReadPixels) so it doesn't need Wayland either way.
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

add_library(portabled3d8_gl INTERFACE)
target_link_libraries(portabled3d8_gl INTERFACE glfw OpenGL::GL)
