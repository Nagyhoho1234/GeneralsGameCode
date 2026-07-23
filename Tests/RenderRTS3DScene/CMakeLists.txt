# Phase 5(a) Milestone 8 Task 4 (native port plan, Draft 30) - the
# milestone's payoff task and exit harness: a pixel rendered through the
# GAME'S OWN real scene manager (RTS3DScene, Core/GameEngineDevice/.../
# W3DScene.cpp, unified in Task 2) with a real, defaults-constructed
# GlobalData object underneath it - the first GameClient scene object and
# the first GlobalData ever constructed and executed on POSIX. See main.cpp
# for the full check list and docs/native-port-plan.md's Draft 30 section
# (findings 3, 5, 6, 7, 8 and the full Step 4 implementation-ordering text)
# for the full design rationale.
#
# NOT WIN32: same reasoning as all seven of its rendering siblings and
# Tests/GameFileSystem - Windows already has the real D3D8 backend and its
# own CI; this only exercises the GL backend + the unified GameClient scene
# layer + a real GlobalData together, for the first time.
#
# Structure: Tests/RenderGameAssets's ENTIRE prologue/link structure, cloned
# wholesale per the task brief (CriticalSections + initMemoryManager, the
# file-system trio, W3DFileSystem construction + .big-archive-backed quad
# loading, the real GameMemory.cpp allocator swap, -ffunction-sections/
# --gc-sections, the GNU-only linker version script + static-libstdc++/
# static-libgcc combination for the Mesa/LLVM interposition issue) PLUS,
# newly for this harness:
#   - GeneralsMD's real GlobalData.cpp (Draft 30 finding 5: its constructor
#     is set-defaults-only, no INI closure needed - this REPLACES Tests/
#     RenderGameAssets's link_stubs.cpp's "GlobalData *TheWritableGlobalData
#     = nullptr" stub with a real, defaults-constructed object).
#   - The unified Core/GameEngineDevice/.../W3DScene.cpp (RTS3DScene itself)
#     plus exactly the one sibling its real, runtime-live code path needs:
#     W3DShroud.cpp (RTS3DScene's ctor unconditionally constructs a
#     W3DShroudMaterialPassClass/W3DMaskMaterialPassClass whenever
#     TheGlobalData->m_shroudOn is true - the real default - so their
#     vtables, hence W3DShroud.cpp's Install_Materials/UnInstall_Materials
#     bodies, must resolve; W3DShroud.cpp's OWN heavier methods
#     (init/render/reset, the real W3DShroud class, not the two
#     MaterialPassClass subclasses) are never referenced by anything this
#     harness calls and are dropped by --gc-sections, the same mechanism
#     Tests/RenderGameAssets's own CMakeLists.txt comment already documents
#     for SubsystemInterfaceList::initSubsystem).
#   - Core/GameEngine/Source/Common/GameUtility.cpp (rts::
#     getObservedOrLocalPlayerIndex_Safe, called unconditionally from
#     RTS3DScene::renderOneObject/Customized_Render/
#     flushOccludedObjectsIntoStencil) - a small, clean TU per the
#     established Drafts 24/28 policy ("compile small clean TUs, stub
#     heavyweight ones"), needing only two more harness-local singleton
#     stubs (TheControlBar/ThePlayerList, both null-guarded at every real
#     call site it makes - see link_stubs.cpp).
#   - link_stubs.cpp additionally supplies 10 singleton-pointer stubs
#     (TheGameLogic/TheW3DShadowManager/TheTacticalView/
#     TheParticleSystemManager/TheTerrainRenderObject/TheControlBar/
#     ThePlayerList/TheVersion, alongside RenderGameAssets's carried-over
#     TheSubsystemList/TheAudio), the four Flush() engine-extern
#     trampolines (PrepareShadows/DoShadows/DoTrees/DoParticles, per Draft
#     30 design decisions), W3DShaderManager::setShader/resetShader/the
#     m_Textures[] storage (W3DShaderManager.cpp is d3dx8tex.h-bound, not
#     POSIX-compilable), 7 out-of-line member-function stubs
#     (Drawable::getTintColor/getSelectionColor/
#     friend_(un)lockDirtyStuffForIteration, Thing::isKindOf,
#     Object::getShroudedStatus/getControllingPlayer,
#     Version::getVersionNumber) - all documented in-place with the actual
#     runtime-(un)reachability evidence, per Draft 30 open question 1's
#     "harness-local member-function stubs" policy - close to the plan's
#     ~10-15-entry prediction (finding 6) once every individual small stub
#     is counted, though several candidates the plan flagged as open
#     questions turned out to be fully inline in their headers needing only
#     their pointer (GameLogic::getFrame/getShowBehindBuildingMarkers,
#     View::getOrigin/getWidth/getHeight, W3DShadowManager::queueShadows/
#     setStencilShadowMask/getStencilShadowMask, ParticleSystemManager::
#     queueParticleRender being pure virtual) once actually read, needing
#     only their singleton pointer's storage, not a function-body stub.
#
# Two genuine, previously-undiscovered gaps this task found and fixed
# properly at the source (NOT papered over with harness-local stubs),
# because W3DScene.cpp is WIN32-gated in Core/GameEngineDevice/CMakeLists.txt
# and had therefore NEVER been compiled on any non-Windows target before
# this harness direct-listed it - the "34/34 baseline" g_gameenginedevice/
# z_gameenginedevice targets never actually build it on Linux:
#   1. Core/Libraries/Source/WWVegas/WW3D2/PortableD3D8/d3d8.h was missing
#      the standard "typedef IDirect3DDevice8 *LPDIRECT3DDEVICE8,
#      *PDIRECT3DDEVICE8;" every real D3D8 header defines - W3DScene.cpp's
#      renderStenciledPlayerColor() uses it. Fixed there (one line, matching
#      the real header's own form/position exactly) - see that file's own
#      comment for the full story.
#   2. WW3D2/light.cpp/.h (LightClass) had simply never been promoted from
#      WW3D2_SRC's WIN32-gated block to WW3D2_SRC_PORTABLE, despite having
#      zero D3D/Windows dependencies (grep-verified) - an outright omission,
#      not a deliberate not-yet-ported call (unlike nearly every other
#      WW3D2_SRC entry, which already carries a "portable, see
#      WW3D2_SRC_PORTABLE" marker). RTS3DScene's real,
#      always-executed light-copying code
#      (updateFixedLightEnvironments's "*m_infantryLight[i] =
#      *m_globalLight[i];") needs LightClass's out-of-line ctor/dtor/
#      operator=/Clone/etc. Fixed by moving light.cpp/.h to WW3D2_SRC_PORTABLE
#      in Core/Libraries/Source/WWVegas/WW3D2/CMakeLists.txt (both lists feed
#      the same corei_ww3d2 target on every platform, so this is additive on
#      Windows too, re-verified by a real MSVC rebuild - see this task's
#      close-out report).
add_executable(RenderRTS3DSceneTest
    main.cpp
    link_stubs.cpp

    # ---- Core/GameEngine Common/System layer: identical file list to
    # Tests/RenderGameAssets/CMakeLists.txt (that harness's finding 2's
    # confirmed-clean set, Debug.cpp deliberately excluded - see that
    # target's own long-form comment for the unchanged rationale). ----
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

    # ---- The real memory manager: NEITHER core_wwstub's allocator half NOR
    # any other allocator is linked here - GameMemory.cpp/GameMemoryInit.cpp
    # is the only allocator in this link, unconditionally (identical
    # rationale to Tests/RenderGameAssets). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/GameMemory.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/GameMemoryInit.cpp

    # ---- The four StdDevice twins. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFile.cpp

    # ---- W3DFileSystem.cpp (Milestone 7 Task 3, Core-unified): the real
    # WW3D<->GameEngine seam, unchanged from Tests/RenderGameAssets. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DFileSystem.cpp

    # ---- CriticalSection.h/.cpp: genuinely per-tree only. GeneralsMD's
    # copy, matching every other harness's "GeneralsMD + RTS_ZEROHOUR=1"
    # flavor. ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/System/CriticalSection.cpp

    # ---- wwdebugstub.cpp, direct-listed (see Tests/RenderGameAssets's own
    # comment - unchanged rationale): core_wwstub's OTHER member
    # (wwallocstub.cpp) must NOT be linked here. ----
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWStub/wwdebugstub.cpp

    # ---- matrixmapper.cpp: per-tree, needed by decalsys.cpp's
    # MatrixMapperClass::Set_Texture_Transform reference (Tests/
    # RenderWW3DFrame/Tests/RenderGameAssets's own matching comment). ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp

    # ---- NEW for this harness (Draft 30 finding 5): the real,
    # GeneralsMD-flavor GlobalData.cpp, compiled directly (per-tree, NOT
    # unified this milestone - the established per-tree-flavor pattern).
    # Its constructor is set-defaults-only (":562-1092", verified by
    # reading) - no INI closure needed, no other GameEngine TU pulled in by
    # it beyond what's already listed above. ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/GlobalData.cpp

    # ---- NEW for this harness (Draft 30 finding 2, Task 2's deliverable):
    # the unified RTS3DScene/RTS2DScene/RTS3DInterfaceScene TU itself. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DScene.cpp

    # ---- NEW for this harness: W3DShroud.cpp, needed ONLY for
    # W3DShroudMaterialPassClass/W3DMaskMaterialPassClass's
    # Install_Materials/UnInstall_Materials vtable entries (RTS3DScene's ctor
    # really constructs both whenever TheGlobalData->m_shroudOn is true, the
    # real default) - see link_stubs.cpp's matching comment for the full
    # runtime-(un)reachability story. W3DShroud.cpp's OWN much heavier
    # W3DShroud class methods (init/render/reset/etc., a different class in
    # the same file) are never referenced by anything this harness calls and
    # are dropped by --gc-sections below. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DShroud.cpp

    # ---- NEW for this harness: Common/Snapshot.cpp + GeneralsMD's
    # Common/RTS/Money.cpp. Found by the linker, not predicted by the plan:
    # GlobalData derives from SubsystemInterface and provides real
    # (non-pure) overrides for init/update/reset/draw - constructing a real
    # GlobalData therefore requires its COMPLETE vtable to resolve, which
    # pulls in GlobalData::init() (-> generateExeCRC(), guarded by
    # "if (TheVersion)", stubbed in link_stubs.cpp) even though this
    # harness never calls init() itself. Separately, GlobalData has a
    # genuinely, unconditionally-constructed Money m_defaultStartingCash
    # member (its ctor runs for every GlobalData instance) - Money derives
    # from Snapshot and overrides ITS pure virtuals (crc/xfer/loadPostProcess), so Money's own
    # vtable must resolve too. Both TUs are small (56/214 lines) and clean
    # (Snapshot.cpp: no non-portable deps at all; Money.cpp: Xfer/GameAudio/
    # PlayerList headers for TYPES only - Money::crc/xfer/loadPostProcess
    # DO call real Xfer methods, all inline in Xfer.h, verified by reading -
    # no further link cost). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/Snapshot.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/RTS/Money.cpp

    # ---- NEW for this harness: rts::getObservedOrLocalPlayerIndex_Safe,
    # called unconditionally from RTS3DScene::renderOneObject/
    # Customized_Render/flushOccludedObjectsIntoStencil. A small, clean,
    # already-portable TU (Draft 30 finding 3) - needs only two more
    # singleton stubs (TheControlBar/ThePlayerList, both null-guarded at
    # every call site it makes; see link_stubs.cpp). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/GameUtility.cpp
)

target_link_libraries(RenderRTS3DSceneTest PRIVATE
    # zi_always (RTS_ZEROHOUR=1 + corei_always, GeneralsMD flavor) - see
    # Tests/RenderGameAssets's matching comment for why this, not the bare
    # corei_always Tests/RenderWW3DFrame uses.
    zi_always
    core_wwvegas
    corei_ww3d2
    portabled3d8_gl
    # Deliberately NOT core_wwstub - see the allocator-swap comment in
    # Tests/RenderGameAssets/CMakeLists.txt (unchanged rationale).
)

target_compile_features(RenderRTS3DSceneTest PRIVATE cxx_std_17)

# Real link-closure finding, carried over unchanged from Tests/
# RenderGameAssets/Tests/GameFileSystem (same reasons, PLUS this harness's
# own new dead paths: GlobalData.cpp's INI-driven static field-parse tables
# and W3DShroud.cpp's own heavier W3DShroud-class methods, both genuinely
# unreachable from anything this harness's main() calls). Standard
# dead-code elimination, scoped to this target only, lets the linker itself
# prove these paths unreachable rather than hand-stubbing every incidental
# symbol.
target_compile_options(RenderRTS3DSceneTest PRIVATE -ffunction-sections -fdata-sections)
target_link_options(RenderRTS3DSceneTest PRIVATE -Wl,--gc-sections)

# Real, hard-evidence-confirmed integration finding, carried over verbatim
# from Tests/RenderGameAssets/CMakeLists.txt (same GameMemory.cpp allocator
# swap, same Mesa/LLVM interposition + std::filesystem inline/library-split
# problem, same fix - see that file's own long-form comment for the full
# story with hard evidence; unchanged and re-verified for this new target
# during this task's own development).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(RenderRTS3DSceneTest PRIVATE
        -static-libstdc++ -static-libgcc
        -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/hide_global_allocator_symbols.map)
endif()

# Header resolution: Tests/RenderGameAssets/CMakeLists.txt's set, PLUS
# GeneralsMD/Code/GameEngineDevice/Include (NEW for this harness):
# W3DShroud.cpp #includes "W3DDevice/GameClient/W3DPoly.h", which - unlike
# its own file and every other Task-1/2 sibling - has NOT been Core-unified
# yet (grep-verified: only Generals/GeneralsMD per-tree copies exist). Only
# its header is needed here (for a type used in a dead, --gc-sections'd
# code path inside W3DShroud.cpp - W3DPoly.h's own .cpp is NOT linked into
# this target), so resolving it via GeneralsMD's per-tree Include dir -
# exactly how the real z_gameenginedevice target itself resolves it - is
# sufficient; no new sibling-unification work is needed for this harness.
target_include_directories(RenderRTS3DSceneTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include/Precompiled
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

# StdBIGFileSystem::init() scans its cwd recursively for "*.big" - ctest's
# WORKING_DIRECTORY covers this the same way Tests/GameFileSystem's/Tests/
# RenderGameAssets's does. Dedicated scratch dir so this harness's authored
# archive never collides with any other target's output.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/renderrts3dscene_scratch)

add_test(NAME RenderRTS3DSceneTest COMMAND RenderRTS3DSceneTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/renderrts3dscene_scratch)
