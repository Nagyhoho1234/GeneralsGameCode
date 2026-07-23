# Phase 5(a) Milestone 9 Task 3 (native port plan, Draft 32) - the
# milestone's payoff task: a pixel rendered through the game's own real
# camera-transform math (W3DView::updateCameraTransform(), Core/
# GameEngineDevice/.../W3DView.cpp, portability-fixed in Task 2) on top of a
# real, base TerrainLogic (Core-... no, per-tree GeneralsMD, see below) - the
# first TerrainLogic AND the first W3DView ever constructed and executed on
# POSIX. See main.cpp for the full check list and docs/native-port-plan.md's
# Draft 32 section (findings 5, 6, 7, the Design Decisions section, and open
# questions 1-3) for the full design rationale.
#
# NOT WIN32: same reasoning as all eight of its rendering siblings and
# Tests/GameFileSystem - Windows already has the real D3D8 backend and its
# own CI.
#
# Structure: Tests/RenderRTS3DScene's ENTIRE prologue/link structure, cloned
# wholesale per the task brief, PLUS newly for this harness:
#   - GeneralsMD's real TerrainLogic.cpp (Draft 32 finding 6: the BASE class,
#     NOT W3DTerrainLogic - constructor is set-defaults-only, no map load).
#     Task 2's own scratch-harness finding (task-2-report.md) already
#     confirmed this drags in a materially bigger link closure than the
#     class's own small size suggests: PartitionManager, TheGhostObjectManager,
#     TheRadar, TheTacticalView, TheGameLogic, plus Drawable.cpp's
#     TintEnvelope vtable and CachedFileInputStream/DataChunkInput (pulled in
#     via TerrainLogic::loadMap, never called at runtime but still needing to
#     link). See link_stubs.cpp for the actual resolution of each.
#   - Core/GameEngine's real FramePacer.cpp + FrameRateLimit.cpp - a genuine
#     gap this task found that Draft 32's finding 5 did NOT enumerate:
#     W3DView.cpp's buildCameraTransform()/zoomCameraToDesiredHeight()/
#     movePivotToGround() (all three inside the plan's own "camera-transform-
#     core" method list) unconditionally dereference TheFramePacer with NO
#     null guard (":392,:450,:467"). Both TUs are already proven to compile
#     cleanly on POSIX today (they are unconditionally listed in Core/
#     GameEngine/CMakeLists.txt, not WIN32-gated, and already produce .o
#     files in this repo's own build/linux-x64 tree as part of the standing
#     g_gameengine/z_gameengine targets) - real, small, already-portable TUs,
#     so constructing a REAL TheFramePacer (not a stub) is both cheap and the
#     more honest choice, matching this milestone's own "first X ever
#     constructed on POSIX" ethos applied to a second singleton.
#   - The unified Core/GameEngineDevice/.../W3DView.cpp itself (Task 2's
#     portability fix already applied) - the harness's own payoff object.
#   - link_stubs.cpp additionally supplies the CameraShakeSystem harness-
#     local stub (real Core/GameEngineDevice/.../CameraShakeSystem.cpp is
#     WIN32-gated AND unconditionally #includes d3dx8core.h, a genuinely
#     non-portable header with no PortableD3D8 equivalent - stubbing the
#     four small methods W3DView.cpp's camera-transform-core calls is far
#     cheaper than fixing that whole TU's closure for methods that are true
#     no-ops here anyway, since Add_Camera_Shake() is never called by this
#     harness so CameraShakerList stays empty) plus whatever the linker asks
#     for from W3DView.o's OTHER, unreachable-but-must-link methods (draw()/
#     drawView()/update()/pickDrawable()/iterateDrawablesInRegion() - see
#     that file's own long-form comments for the full, linker-discovered
#     list, not predicted by static reading alone).
add_executable(RenderCameraTransformTest
    main.cpp
    link_stubs.cpp

    # ---- Core/GameEngine Common/System layer: identical file list to
    # Tests/RenderRTS3DScene/CMakeLists.txt. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/ArchiveFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/ArchiveFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/AsciiString.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/UnicodeString.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/File.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/FileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/LocalFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/LocalFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/RAMFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/registry.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/StreamingArchiveFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/SubsystemInterface.cpp

    # ---- The real memory manager (unchanged rationale). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/GameMemory.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/GameMemoryInit.cpp

    # ---- The four StdDevice twins. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFile.cpp

    # ---- W3DFileSystem.cpp: the real WW3D<->GameEngine seam. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DFileSystem.cpp

    # ---- CriticalSection.h/.cpp: GeneralsMD's copy, matching every other
    # harness's "GeneralsMD + RTS_ZEROHOUR=1" flavor. ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/System/CriticalSection.cpp

    # ---- wwdebugstub.cpp, direct-listed. ----
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWStub/wwdebugstub.cpp

    # ---- matrixmapper.cpp: per-tree, needed by decalsys.cpp's
    # MatrixMapperClass::Set_Texture_Transform reference. ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp

    # ---- GeneralsMD's real GlobalData.cpp (Draft 30 finding 5, carried over
    # unchanged from Tests/RenderRTS3DScene). ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/GlobalData.cpp

    # ---- The unified RTS3DScene/RTS2DScene/RTS3DInterfaceScene TU. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DScene.cpp

    # ---- W3DShroud.cpp (carried over unchanged from Tests/RenderRTS3DScene -
    # same Install_Materials/UnInstall_Materials vtable-completeness reason). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DShroud.cpp

    # ---- Common/Snapshot.cpp + GeneralsMD's Common/RTS/Money.cpp (carried
    # over unchanged - GlobalData's vtable-completeness requirement). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/Snapshot.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/RTS/Money.cpp

    # ---- rts::getObservedOrLocalPlayerIndex_Safe (carried over unchanged). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/GameUtility.cpp

    # ---- NEW for this harness (Draft 32 finding 6, Task 2 Part B): the
    # real, GeneralsMD-flavor, BASE TerrainLogic.cpp (NOT W3DTerrainLogic).
    # Constructor is set-defaults-only. See link_stubs.cpp for the full
    # link-closure resolution Task 2's own scratch harness already found. ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/GameLogic/Map/TerrainLogic.cpp

    # ---- NEW for this harness: the real FramePacer.cpp/FrameRateLimit.cpp -
    # both already portable, already compiled on Linux today as part of the
    # standing g_gameengine/z_gameengine targets (see this file's own header
    # comment for the full story). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/FramePacer.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/FrameRateLimit.cpp

    # ---- NEW for this harness (Task 2 Part A's portability fix already
    # applied): the real, unified, WHOLE W3DView.cpp TU - this harness's own
    # payoff object. Only updateCameraTransform() (private, exposed to this
    # harness via a single, minimal, test-only friend grant in W3DView.h) is
    # ever CALLED; every other virtual method (draw()/drawView()/update()/
    # pickDrawable()/iterateDrawablesInRegion()) must still compile and
    # link (one TU, one vtable) but is never invoked at runtime - see
    # link_stubs.cpp for the actual, linker-discovered closure this pulled in. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DView.cpp

    # ---- NEW for this harness: the real, already Core-unified,
    # already-proven-portable View.cpp (base class of W3DView, needed for
    # W3DView's vtable to resolve: View::init()/xfer()/zoom()/getLocation()/
    # setLocation()/prependViewToList()/getScreenCornerWorldPointsAtZ() are
    # all real, out-of-line, non-pure base methods W3DView does not
    # override - the real linker's own first finding for this target). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/GameClient/View.cpp

    # ---- NEW for this harness: ParabolicEase.cpp - a tiny, fully
    # self-contained pure-math TU (no singleton dependencies at all, verified
    # by reading) needed by W3DView.o's unreachable-but-must-link
    # rotateCameraOneFrame()/moveAlongWaypointPath()/moveCameraTo() bodies
    # (each embeds a ParabolicEase member by value in its own Info struct,
    # W3DView.h:71,84,112,126 - operator()/setEaseTimes() are out-of-line).
    # Linked for real rather than stubbed, per the established "compile
    # small clean TUs" policy - cheaper and more honest than three
    # hand-written stub bodies for a file this small and clean. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/GameClient/ParabolicEase.cpp

    # ---- NEW for this harness: Line2D.cpp - another tiny, fully
    # self-contained pure-geometry TU (no singleton dependencies, verified
    # by reading) needed by W3DView.o's unreachable-but-must-link draw()
    # body's ClipLine2D() call (W3DView.cpp:1996). Linked for real, same
    # "compile small clean TUs" rationale as ParabolicEase.cpp above. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/GameClient/Line2D.cpp

    # ---- NEW for this harness: W3DConvert.cpp - a third tiny,
    # self-contained pure-math TU (W3DLogicalScreenToPixelScreen()/
    # PixelScreenToW3DLogicalScreen(), no singleton dependencies at all,
    # verified by reading) needed by W3DView.o's unreachable-but-must-link
    # getPickRay()/worldToScreenTriReturn()/calcCameraAreaOffset() bodies. ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/Common/W3DConvert.cpp

    # ---- NEW for this harness: TerrainLogic.cpp's own remaining link
    # closure (Task 2's scratch-harness finding, continued) - both real,
    # already-portable, already-proven-on-Linux-today TUs (both already have
    # .o files in this repo's own build/linux-x64 tree, as part of the
    # standing g_gameengine/z_gameengine targets - the SAME "already
    # compiles cleanly on POSIX today" argument this file's own header
    # comment makes for FramePacer.cpp/FrameRateLimit.cpp):
    #   - DataChunk.cpp: home of CachedFileInputStream/DataChunkInput, pulled
    #     in via TerrainLogic::loadMap() (link-live via the vtable - loadMap
    #     is virtual - runtime-dead, this harness never loads a map). Linked
    #     for real rather than stubbing 8+ individual methods, matching the
    #     "compile small clean TUs" policy.
    #   - Trig.cpp: Sin()/Cos(), pulled in via TerrainLogic::alignOnTerrain()
    #     (link-live, runtime-dead - never called by this harness). ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/System/DataChunk.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/System/Trig.cpp

    # ---- NEW for this harness: TerrainLogic.cpp's own remaining link
    # closure, continued (this task's real-linker-driven discovery, not
    # predicted by static reading) - three more real, already-portable,
    # already-proven-on-Linux-today TUs (all three already have .o files in
    # this repo's own build/linux-x64 tree, same argument as above):
    #   - Dict.cpp: Dict::getAsciiString()/getBool(), pulled in via
    #     TerrainLogic::addWaypoint() (non-virtual, only reached from
    #     loadMap(), link-live/runtime-dead).
    #   - ObjectStatusTypes.cpp: OBJECT_STATUS_MASK_NONE (a real global
    #     object, default-constructed to all-zero bits), pulled in via
    #     Bridge::createTower()/Bridge::Bridge() default arguments (nested
    #     class inside TerrainLogic.cpp, only reached from
    #     addBridgeToLogic()/addLandmarkBridgeToLogic(), never called).
    #   - NameKeyGenerator.cpp: StaticNameKey::key() (real body, null-guards
    #     TheNameKeyGenerator internally - NameKeyGenerator.cpp:272-274),
    #     pulled in via the TheKey_waypointPath* well-known-key globals
    #     (link_stubs.cpp's own definitions, below - see that file's own
    #     comment for why they must be defined there rather than by linking
    #     the real, heavy WorldHeightMap.cpp). ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/Dict.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/ObjectStatusTypes.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/NameKeyGenerator.cpp
)

target_link_libraries(RenderCameraTransformTest PRIVATE
    zi_always
    core_wwvegas
    corei_ww3d2
    portabled3d8_gl
    # NEW for this harness: DataChunk.cpp (#includes "Compression.h") needs
    # core_compression's own INTERFACE include directory - a real, already-
    # portable, already-linked-elsewhere (Tests/CMakeLists.txt's core_tests)
    # static library, not a heavy new dependency.
    core_compression
)

target_compile_features(RenderCameraTransformTest PRIVATE cxx_std_17)

# Real link-closure finding, carried over unchanged from Tests/
# RenderRTS3DScene (same reasons, PLUS this harness's own new dead paths:
# TerrainLogic.cpp's loadMap()/water/bridge machinery and W3DView.cpp's
# draw()/drawView()/pickDrawable()/iterateDrawablesInRegion() bodies, all
# genuinely unreachable from anything this harness's main() calls).
target_compile_options(RenderCameraTransformTest PRIVATE -ffunction-sections -fdata-sections)
target_link_options(RenderCameraTransformTest PRIVATE -Wl,--gc-sections)

# Real, hard-evidence-confirmed integration finding, carried over verbatim
# from Tests/RenderRTS3DScene/CMakeLists.txt (same Mesa/LLVM interposition +
# std::filesystem inline/library-split problem, same fix).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(RenderCameraTransformTest PRIVATE
        -static-libstdc++ -static-libgcc
        -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/hide_global_allocator_symbols.map)
endif()

# Header resolution: Tests/RenderRTS3DScene/CMakeLists.txt's set, unchanged -
# W3DView.cpp/TerrainLogic.cpp/FramePacer.cpp/FrameRateLimit.cpp need nothing
# beyond what that set already resolves (all Core/GeneralsMD Include paths).
target_include_directories(RenderCameraTransformTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include/Precompiled
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

# StdBIGFileSystem::init() scans its cwd recursively for "*.big" - dedicated
# scratch dir so this harness's authored archive never collides with any
# other target's output.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rendercameratransform_scratch)

add_test(NAME RenderCameraTransformTest COMMAND RenderCameraTransformTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rendercameratransform_scratch)
