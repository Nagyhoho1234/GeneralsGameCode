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
# harness an untouched Milestone 1 regression test.
#
# Milestone 6 (native port plan Phase 5(a), Draft 26 Steps 2+4 combined):
# switched from a hand-picked file list to linking corei_ww3d2 directly -
# see RenderDeviceInit/CMakeLists.txt's matching comment for why (GL
# Create_Device/Release_Device now unconditionally call
# Do_Onetime_Device_Dependent_Inits/_Shutdowns, ending Trap 1 completely).
# anim_sound_link_stub.cpp is gone (native port plan Phase 5(a) Milestone
# 6, Draft 26 Step 6, finding 4) - AnimatedSoundMgrClass's stub graduated
# to engine level (animatedsoundmgr_null.cpp, in corei_ww3d2) when
# ww3d.cpp joined the portable list this same milestone.
add_executable(RenderTexturedTriangleTest
    main.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp
)

target_link_libraries(RenderTexturedTriangleTest PRIVATE
    core_wwvegas
    corei_ww3d2
    core_wwstub
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
