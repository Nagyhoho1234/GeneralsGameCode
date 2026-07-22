# Phase 5(a) Milestone 3 verification harness (native port plan) - this
# milestone's exit criterion: the REAL engine draw path end-to-end
# (DX8VertexBufferClass/DX8IndexBufferClass through real WriteLockClass
# objects, DX8Wrapper::Set_Vertex_Buffer/Set_Index_Buffer/Set_Shader/
# Set_Material/Set_Transform/Draw_Triangles, real ShaderClass presets, a
# real perspective projection, and a DynamicVBAccessClass/
# DynamicIBAccessClass second draw) - not the device-object-level API
# Tests/RenderTexturedTriangle drove. See main.cpp for the full 6-check list.
#
# NOT WIN32: same reasoning as RenderDeviceInit/RenderTexturedTriangle -
# Windows already has the real D3D8 backend and its own CI; this only
# exercises the GL backend.
#
# Sibling to RenderDeviceInit/RenderTexturedTriangle, not an extension of
# either - both stay untouched regression tests for their own milestones.
# Compiles the portable WW3D2 TUs this milestone's steps 1-7 moved/added
# directly (not via corei_ww3d2) for the same link-closure reason those
# harnesses do: ww3d.cpp/camera.cpp/mesh.cpp/texture.cpp/etc. still
# reference dozens of per-tree/Windows-only subsystems Milestones 4-5 own.
add_executable(RenderEngineDrawPathTest
    main.cpp
    ${CMAKE_SOURCE_DIR}/Tests/mesh_link_stub.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper_common.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper_gl.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper_draw.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/PortableD3D8/gl_core33.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/ww3d_common.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8caps.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8fvf.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8vertexbuffer.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8indexbuffer.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/shader.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/vertmaterial.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/mapper.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/sortingrenderer.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/texture_common.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/formconv.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/ww3dformat.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/w3d_util.cpp
)

target_link_libraries(RenderEngineDrawPathTest PRIVATE
    core_wwvegas
    core_wwstub
    portabled3d8_gl
    corei_always
)

target_compile_features(RenderEngineDrawPathTest PRIVATE cxx_std_17)

# See RenderDeviceInit/CMakeLists.txt's matching comment - GeneralsMD's
# WW3D2 dir is added for header resolution only, no GeneralsMD .cpp is
# compiled into this target.
target_include_directories(RenderEngineDrawPathTest PRIVATE
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

add_test(NAME RenderEngineDrawPathTest COMMAND RenderEngineDrawPathTest)
