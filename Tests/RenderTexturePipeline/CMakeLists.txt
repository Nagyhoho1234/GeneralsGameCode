# Phase 5(a) Milestone 4 (native port plan, Draft 22) - the real texture
# pipeline end-to-end (TextureClass/TextureLoader/TextureFilterClass/
# MissingTexture/DDSFileClass/ThumbnailManagerClass, real GL texture
# creation, the real GL sampler-state layer, and the first real
# background-thread texture load on POSIX), non-Windows only. See
# main.cpp's file header comment for the full 6-check list. Sibling to
# RenderDeviceInit/RenderTexturedTriangle/RenderEngineDrawPath, not an
# extension of any of them - all three stay untouched regression tests.
#
# Compiles the portable WW3D2 TUs this milestone's steps 1-5 moved/added
# directly (not via corei_ww3d2), same link-closure reason those harnesses
# do: ww3d.cpp/camera.cpp/mesh.cpp/assetmgr.cpp/etc. still reference dozens
# of per-tree/Windows-only subsystems Milestone 5 owns. Builds on
# RenderEngineDrawPath's file list (the real draw path this harness also
# needs) plus the nine texture-pipeline TUs and assetmgr_common.cpp (for
# WW3DAssetManager::TheInstance's static definition - link finding 1).
add_executable(RenderTexturePipelineTest
    main.cpp
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
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/assetmgr_common.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/texture.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/texturefilter.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/textureloader.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/texturethumbnail.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/ddsfile.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/dx8texman.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/missingtexture.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/surfaceclass.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2/bitmaphandler.cpp
)

target_link_libraries(RenderTexturePipelineTest PRIVATE
    core_wwvegas
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
