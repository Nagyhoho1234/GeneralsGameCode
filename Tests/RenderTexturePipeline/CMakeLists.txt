# Phase 5(a) Milestone 4 (native port plan, Draft 22) - the real texture
# pipeline end-to-end (TextureClass/TextureLoader/TextureFilterClass/
# MissingTexture/DDSFileClass/ThumbnailManagerClass, real GL texture
# creation, the real GL sampler-state layer, and the first real
# background-thread texture load on POSIX), non-Windows only. See
# main.cpp's file header comment for the full 6-check list. Sibling to
# RenderDeviceInit/RenderTexturedTriangle/RenderEngineDrawPath, not an
# extension of any of them - all three stay untouched regression tests.
#
# Milestone 6 (native port plan Phase 5(a), Draft 26 Steps 2+4 combined):
# switched from a hand-picked file list to linking corei_ww3d2 directly,
# like RenderW3DMeshTest. Reason: this harness's Set_Render_Device now runs
# DX8Wrapper::Do_Onetime_Device_Dependent_Inits (moved to the always-
# compiled dx8wrapper_draw.cpp as part of ending Trap 1), which references
# TheDX8MeshRenderer/BoxRenderObjClass/PointGroupClass/ShatterSystem -
# dx8wrapper_draw.cpp links directly into this target's object list (not
# archived into a static library first), so the linker must resolve every
# external reference in it unconditionally, the same class of failure the
# plan's Draft 26 correction note describes for Do_Onetime's first (isolated)
# move attempt. Hand-listing the whole mesh-render/asset-manager closure
# here would just re-derive corei_ww3d2's own list a sixth time; linking it
# directly is simpler and is what RenderW3DMeshTest already established.
# matrixmapper.cpp is added for the same link-closure reason documented in
# RenderW3DMeshTest/CMakeLists.txt (decalsys.cpp's MatrixMapperClass::
# Set_Texture_Transform reference) - corei_ww3d2 pulls in decalsys.cpp
# here exactly as it does there.
# anim_sound_link_stub.cpp is gone (native port plan Phase 5(a) Milestone
# 6, Draft 26 Step 6, finding 4) - AnimatedSoundMgrClass's stub graduated
# to engine level (animatedsoundmgr_null.cpp, in corei_ww3d2) when
# ww3d.cpp joined the portable list this same milestone.
add_executable(RenderTexturePipelineTest
    main.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp
)

target_link_libraries(RenderTexturePipelineTest PRIVATE
    core_wwvegas
    corei_ww3d2
    core_wwstub
    portabled3d8_gl
    corei_always
)

target_compile_features(RenderTexturePipelineTest PRIVATE cxx_std_17)

# See RenderDeviceInit/CMakeLists.txt's matching comment - GeneralsMD's
# WW3D2 dir is added for header resolution only, no GeneralsMD .cpp is
# compiled into this target.
target_include_directories(RenderTexturePipelineTest PRIVATE
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

add_test(NAME RenderTexturePipelineTest COMMAND RenderTexturePipelineTest)
