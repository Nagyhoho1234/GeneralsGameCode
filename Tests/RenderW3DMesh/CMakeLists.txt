# Phase 5(a) Milestone 5 (native port plan, Draft 24) - the exit criterion:
# real .w3d bytes, authored at runtime via the real ChunkSaveClass/
# ChunkLoadClass against Core's now-unified w3d_file.h structs, loaded
# through a real WW3DAssetManager, rendered by the real dx8renderer.cpp
# mesh pipeline. See main.cpp for the full check list.
#
# NOT WIN32: same reasoning as the other four harnesses - Windows already
# has the real D3D8 backend and its own CI; this only exercises the GL
# backend.
#
# Unlike RenderEngineDrawPath/RenderTexturePipeline (Milestones 3-4's
# harnesses, which hand-pick files to dodge ww3d.cpp/mesh.cpp/assetmgr.cpp's
# then-still-Windows-only closure), this harness links corei_ww3d2 directly.
# Milestone 5 Steps 2/3/5/6 made WW3D2_SRC_PORTABLE the genuinely complete
# portable closure (mesh render path, asset-manager closure, static sort
# lists, the whole texture pipeline) - this harness needs nearly all of it,
# so hand-listing ~100 files here would just be a sixth copy of the same
# list Core/Libraries/Source/WWVegas/WW3D2/CMakeLists.txt already
# maintains. The older, narrower harnesses stay on their explicit lists
# unchanged (regression tests, not touched by this milestone).
add_executable(RenderW3DMeshTest
    main.cpp
    # matrixmapper.cpp/.h are per-tree (Generals/GeneralsMD diverge by
    # ~240 lines - real particle-system-era divergence, not yet unified
    # into Core/) and genuinely out of this milestone's unification scope
    # (Draft 24 finding 1's "stays per-tree, out of closure" call), but
    # decalsys.cpp (portable since this step) references
    # MatrixMapperClass::Set_Texture_Transform - decal generation is an
    # explicit Milestone 5 non-goal (Draft 24: "Render_Decal_Meshes runs
    # empty"), so the symbol just needs to link, not to be exercised.
    # "GeneralsMD wins" convention: added directly here rather than
    # re-opening the unification question this step doesn't need to
    # answer.
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp
    # anim_sound_link_stub.cpp is gone (native port plan Phase 5(a)
    # Milestone 6, Draft 26 Step 6, finding 4) - AnimatedSoundMgrClass's
    # stub graduated to engine level (Core/Libraries/Source/WWVegas/WW3D2/
    # animatedsoundmgr_null.cpp, in corei_ww3d2 via WW3D2_SRC_PORTABLE) now
    # that ww3d.cpp joined the portable list this same milestone and calls
    # AnimatedSoundMgrClass::Initialize/Shutdown itself; the harness-local
    # copy would now collide with it.
)

target_link_libraries(RenderW3DMeshTest PRIVATE
    core_wwvegas
    corei_ww3d2
    core_wwstub
    portabled3d8_gl
    corei_always
)

target_compile_features(RenderW3DMeshTest PRIVATE cxx_std_17)

# See RenderDeviceInit/CMakeLists.txt's matching comment - GeneralsMD's
# WW3D2 dir is added for header resolution only, no GeneralsMD .cpp is
# compiled into this target.
target_include_directories(RenderW3DMeshTest PRIVATE
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

add_test(NAME RenderW3DMeshTest COMMAND RenderW3DMeshTest)
