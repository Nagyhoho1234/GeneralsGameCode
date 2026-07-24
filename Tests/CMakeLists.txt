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

# Phase 5(a) Milestone 6 (native port plan, Draft 26) - the milestone's exit
# criterion: a real frame loop driven entirely through WW3D::'s own entry
# points (Init/Set_Render_Device/Begin_Render/Render/End_Render/
# Set_Device_Resolution/Shutdown), non-Windows only. See
# RenderWW3DFrame/CMakeLists.txt.
if(NOT WIN32)
    add_subdirectory(RenderWW3DFrame)
endif()

# Phase 5(a) Milestone 7 Task 2 (native port plan, Draft 28) - the headless
# file-system harness: the first GameEngine code (not WW3D2 rendering code)
# ever to RUN, not merely compile, on POSIX. No GL/display dependency at
# all. NOT WIN32 for consistency with its five rendering siblings (open
# question 4: the Std layer compiles on Windows too, but Windows already
# gets its own coverage via the real D3D8/Win32 backend and CI - default
# to NOT WIN32 here, revisit deliberately if that changes). See
# GameFileSystem/CMakeLists.txt and main.cpp for the full plan.
if(NOT WIN32)
    add_subdirectory(GameFileSystem)
endif()

# Phase 5(a) Milestone 7 Task 4 (native port plan, Draft 28) - the
# milestone's exit harness: combines Task 2's file-system stack + Task 3's
# unified W3DFileSystem with Milestone 6's engine-driven frame loop - every
# byte on screen comes out of a real BIG archive through the engine's own
# file stack. NOT WIN32 for the same reasons as its six rendering siblings
# and GameFileSystem. See RenderGameAssets/CMakeLists.txt and main.cpp for
# the full plan.
if(NOT WIN32)
    add_subdirectory(RenderGameAssets)
endif()

# Phase 5(a) Milestone 8 Task 4 (native port plan, Draft 30) - the
# milestone's payoff task: a pixel rendered through the game's own real
# scene manager (RTS3DScene) with a real, defaults-constructed GlobalData
# underneath it - the first GameClient scene object and the first
# GlobalData ever constructed and executed on POSIX. NOT WIN32 for the same
# reasons as its seven rendering siblings and GameFileSystem. See
# RenderRTS3DScene/CMakeLists.txt and main.cpp for the full plan.
if(NOT WIN32)
    add_subdirectory(RenderRTS3DScene)
endif()

# Phase 5(a) Milestone 9 Task 3 (native port plan, Draft 32) - the
# milestone's payoff task: the game's own real camera-transform math
# (W3DView::updateCameraTransform()) on top of a real, base TerrainLogic -
# the first TerrainLogic and the first W3DView ever constructed and executed
# on POSIX. NOT WIN32 for the same reasons as its eight rendering siblings
# and GameFileSystem. See RenderCameraTransform/CMakeLists.txt and main.cpp
# for the full plan.
if(NOT WIN32)
    add_subdirectory(RenderCameraTransform)
endif()

# Phase 5(a) Milestone 10 (native port plan, Draft 34, "rung 2b-lite") - a
# headless TheAI->TheGameLogic->TheScriptEngine->TheTerrainLogic->
# ThePartitionManager tick harness, deliberately bypassing
# GameEngine::init()/execute()'s real display-layer dependencies. Independent
# of W3DView.cpp/W3DDisplay.cpp entirely - a separate logic-path workstream,
# not a continuation of rung 3b. NOT WIN32 for the same reasons as its nine
# rendering/logic siblings and GameFileSystem, though this harness has no
# rendering dependency at all (pure logic). See
# GameLogicTickHarness/CMakeLists.txt and main.cpp for the full plan.
if(NOT WIN32)
    add_subdirectory(GameLogicTickHarness)
endif()

# Phase 5(a) Milestone 11 (native port plan, rung 3b-ii-a, Draft 35) - the
# real, unmodified W3DView::update()/drawView()(->draw())/pickDrawable()
# control flow, in an empty-world configuration, on top of real
# TheGameLogic/TheScriptEngine and minimal concrete stub subclasses for
# GameClient/InGameUI/Display/FontLibrary/Mouse. Landed in two passes: a
# first attempt shipped only pickDrawable()'s ray-cast proof after hitting
# a real C++ vtable-completeness link wall; a retry adopted Milestone 10's
# "link the real source closure wholesale" technique and delivered the full
# original scope. NOT WIN32 for the same reasons as its ten rendering
# siblings and GameFileSystem. See RenderViewUpdateDraw/CMakeLists.txt and
# main.cpp for the full plan.
if(NOT WIN32)
    add_subdirectory(RenderViewUpdateDraw)
endif()

# Phase 5(a) Milestone 12 (native port plan, Draft 36, "rung 2b") - the real,
# unmodified GameEngine::init()/update()/execute() on POSIX: a harness-local
# PosixGameEngine subclass overriding the 11 pure factory methods, driving
# the real engine entry point this whole port has been building toward since
# Phase 5 began (not a bypass, not hand-constructed singletons - unlike
# Milestone 10's own deliberately-scoped-down "rung 2b-lite"). NOT WIN32 for
# the same reasons as its eleven rendering/logic siblings and GameFileSystem.
# See PosixGameEngineHarness/CMakeLists.txt and main.cpp for the full plan.
if(NOT WIN32)
    add_subdirectory(PosixGameEngineHarness)
endif()
