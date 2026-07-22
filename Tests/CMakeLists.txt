# Opt-in unit test harness for the openSAGE (GeneralsGameCode) engine.
# Enabled via -DRTS_BUILD_TESTS=ON. Not wired into any preset or CI by default.

include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.12
)
FetchContent_MakeAvailable(doctest)

add_executable(core_tests
    test_main.cpp
    test_compression.cpp
)

target_link_libraries(core_tests PRIVATE
    doctest::doctest
    core_compression
    corei_libraries_include
    core_utility
)

target_compile_features(core_tests PRIVATE cxx_std_17)

add_test(NAME core_tests COMMAND core_tests)

# Phase 5(a) Milestone 1 (native port plan) - GL device-init verification,
# non-Windows only. See RenderDeviceInit/CMakeLists.txt.
if(NOT WIN32)
    add_subdirectory(RenderDeviceInit)
endif()

# Phase 5(a) Milestone 2 (native port plan) - "draw one textured triangle"
# through the real device-object-level buffer/texture/draw-call API,
# non-Windows only. See RenderTexturedTriangle/CMakeLists.txt.
if(NOT WIN32)
    add_subdirectory(RenderTexturedTriangle)
endif()

# Phase 5(a) Milestone 3 (native port plan) - the real engine draw path
# (DX8VertexBufferClass/DX8IndexBufferClass, DX8Wrapper's high-level
# Set_Vertex_Buffer/Set_Shader/Set_Material/Draw_Triangles) end-to-end,
# non-Windows only. See RenderEngineDrawPath/CMakeLists.txt.
if(NOT WIN32)
    add_subdirectory(RenderEngineDrawPath)
endif()

# Phase 5(a) Milestone 4 (native port plan, Draft 22) - the real texture
# pipeline end-to-end (TextureClass/TextureLoader/TextureFilterClass/
# MissingTexture/DDSFileClass), including the first real background-thread
# texture load on POSIX, non-Windows only. See
# RenderTexturePipeline/CMakeLists.txt.
if(NOT WIN32)
    add_subdirectory(RenderTexturePipeline)
endif()

# Phase 5(a) Milestone 5 (native port plan, Draft 24) - real W3D meshes on
# GL: mesh/HLod/assetmgr Core unification and portability, the dx8renderer
# mesh pipeline, non-Windows only. See RenderW3DMesh/CMakeLists.txt.
if(NOT WIN32)
    add_subdirectory(RenderW3DMesh)
endif()
