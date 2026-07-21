# Phase 5(a) Milestone 2 verification harness (native port plan). Standalone
# executable (following Tests/RenderDeviceInit/'s pattern, not doctest) -
# drives the real device-object-level buffer/texture/draw-call API
# (CreateVertexBuffer/CreateIndexBuffer/CreateTexture/Lock/Unlock/
# SetStreamSource/SetIndices/SetTexture/SetRenderState/
# DrawIndexedPrimitive) and asserts 6 real pixel-level checks against the
# offscreen FBO - see main.cpp for the full list.
#
# NOT WIN32: same reasoning as RenderDeviceInit - Windows already has the
# real D3D8 backend and its own CI; this only exercises the GL backend.
#
# Sibling to Tests/RenderDeviceInit, not an extension of it - keeps that
# harness an untouched Milestone 1 regression test. Compiles
# dx8wrapper_gl.cpp/dx8wrapper_common.cpp/gl_core33.cpp directly for the
# same reason RenderDeviceInit does (see that CMakeLists.txt's comment):
# ww3d.cpp/camera.cpp/formconv.cpp reference dozens of still-per-tree/
# Windows-only WW3D2 subsystems this milestone deliberately doesn't touch.
add_executable(RenderTexturedTriangleTest
    main.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper_common.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper_gl.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/PortableD3D8/gl_core33.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/ww3d_common.cpp
)

target_link_libraries(RenderTexturedTriangleTest PRIVATE
    core_wwvegas
    portabled3d8_gl
    corei_always
)

target_compile_features(RenderTexturedTriangleTest PRIVATE cxx_std_17)

# See RenderDeviceInit/CMakeLists.txt's matching comment - GeneralsMD's
# WW3D2 dir is added for header resolution only, no GeneralsMD .cpp is
# compiled into this target.
target_include_directories(RenderTexturedTriangleTest PRIVATE
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

add_test(NAME RenderTexturedTriangleTest COMMAND RenderTexturedTriangleTest)
