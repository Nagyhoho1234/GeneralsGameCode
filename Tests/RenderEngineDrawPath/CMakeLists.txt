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
#
# Milestone 6 (native port plan Phase 5(a), Draft 26 Steps 2+4 combined):
# switched from a hand-picked file list to linking corei_ww3d2 directly -
# see RenderDeviceInit/CMakeLists.txt's matching comment for why (GL
# Create_Device/Release_Device now unconditionally call
# Do_Onetime_Device_Dependent_Inits/_Shutdowns, ending Trap 1 completely).
# mesh_link_stub.cpp is dropped - the real mesh.cpp (corei_ww3d2) now links
# here and would collide with it. matrixmapper.cpp/anim_sound_link_stub.cpp
# are added for the same two link-closure reasons documented in
# RenderW3DMeshTest/CMakeLists.txt.
add_executable(RenderEngineDrawPathTest
    main.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp
    ${CMAKE_SOURCE_DIR}/Tests/RenderW3DMesh/anim_sound_link_stub.cpp
)

target_link_libraries(RenderEngineDrawPathTest PRIVATE
    core_wwvegas
    corei_ww3d2
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
