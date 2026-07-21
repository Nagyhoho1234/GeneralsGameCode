# Phase 5(a) Milestone 1 verification harness (native port plan). Standalone
# executable (not a doctest case, following native-port-spike/'s pattern) -
# drives the real DX8Wrapper::Init/Set_Render_Device/Create_Device/
# Begin_Scene/Clear/End_Scene chain against the PortableD3D8 GL backend and
# asserts the offscreen FBO was actually cleared to the requested color.
#
# NOT WIN32: Windows already has the real D3D8 backend and its own CI
# (win32/win-x64 presets, macos-native.yml's sibling). This only exercises
# the GL backend introduced by this milestone.
#
# Compiles dx8wrapper_gl.cpp/dx8wrapper_common.cpp/gl_core33.cpp directly
# (not via corei_ww3d2) rather than linking the whole WW3D2_SRC_PORTABLE
# set - ww3d.cpp/camera.cpp/formconv.cpp reference dozens of still-per-tree/
# Windows-only WW3D2 subsystems (mesh rendering, W3D memory pools, texture
# filtering, ...) that this milestone deliberately doesn't touch, and every
# symbol referenced anywhere in those translation units must resolve at
# link time even though main.cpp never calls into WW3D:: at all.
add_executable(RenderDeviceInitTest
    main.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper_common.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper_gl.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/PortableD3D8/gl_core33.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/ww3d_common.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8caps.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/formconv.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/ww3dformat.cpp
)

target_link_libraries(RenderDeviceInitTest PRIVATE
    core_wwvegas
    portabled3d8_gl
    corei_always
)

target_compile_features(RenderDeviceInitTest PRIVATE cxx_std_17)

# dx8wrapper.h transitively includes a handful of headers
# (lightenvironment.h, ...) that aren't unified into Core/ yet - still
# per-tree only (native port plan, mid-level rendering layer, out of scope
# for this milestone). GeneralsMD's own WW3D2 dir is added here for header
# resolution ONLY (no GeneralsMD .cpp is compiled into this target) - the
# same incidental include-path overlap that already lets
# z_gameenginedevice/g_gameenginedevice build Core/'s dx8wrapper.h today.
target_include_directories(RenderDeviceInitTest PRIVATE
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

add_test(NAME RenderDeviceInitTest COMMAND RenderDeviceInitTest)
