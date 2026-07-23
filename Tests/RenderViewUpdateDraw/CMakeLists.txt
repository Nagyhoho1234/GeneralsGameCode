# Phase 5(a) Milestone 11 RETRY (native port plan, rung 3b-ii-a, Draft 35,
# Draft 33's "Follow-up plan / Workstream B") - the milestone's original
# steps 1-4 (real TheGameLogic/TheScriptEngine construction, the five-class
# GameClient/InGameUI/Display/FontLibrary/Mouse stub-subclass surface, the
# full update()/draw()/drawView() real-execution proof), attempted again
# with a DIFFERENT, now-mandated link strategy after the first attempt's
# minimal-stub-subclass approach was PROVEN NOT TO WORK (see git history on
# this file / main.cpp / harness_stubs.* for that abandoned attempt, and
# docs/native-port-plan.md's Draft 33 section for the measured 372/237
# undefined-symbol counts that disproved it).
#
# THE REAL, GENERALIZABLE FINDING that forced this rewrite (unchanged from
# the abandoned attempt's own disclosure, repeated here since it is this
# file's own reason for existing in its current shape): a derived class's
# vtable needs a resolved function address for EVERY virtual method in its
# base class hierarchy, not just the ones actually overridden - so a stub
# subclass overriding only GameClient's/InGameUI's pure virtuals still
# forces the linker to demand GameClient.cpp's/InGameUI.cpp's real bodies
# for every non-pure virtual left un-overridden (draw()/update() themselves
# call plenty of those). Isolating just TheGameLogic/TheScriptEngine
# construction alone still measured 237 undefined symbols.
#
# THE FIX, copied from Tests/GameLogicTickHarness/CMakeLists.txt (Milestone
# 10's harness, already landed, already proven on a first real attempt) per
# the reconciled follow-up plan: instead of hand-picking which real .cpp
# files to compile, inherit z_gameengine's ENTIRE real source closure (via a
# cmake_language(DEFER) read of that target's SOURCES property and
# corei_gameengine_private's INTERFACE_SOURCES property - Tests/ configures
# before GeneralsMD/ does, so this must be deferred to the end of the
# top-level directory's processing), filtered by a small, named,
# Windows-only exclude list - THE SAME list Milestone 10 already needed and
# proved sufficient, reused verbatim here (this harness's own construction
# path - GameLogic/ScriptEngine/GameClient/InGameUI/Display/FontLibrary/
# Mouse plus the existing render stack - reaches nothing beyond what
# Milestone 10 already excluded; confirmed by a real link attempt, not
# assumed). Once EVERYTHING real is linked in wholesale, there is no
# missing-base-symbol problem left to hit, and the stub subclasses in
# harness_stub_classes.h can go back to overriding ONLY their abstract
# base's pure virtuals, exactly as originally planned - the real base
# bodies now come from the closure instead of causing link errors.
#
# A GENUINELY NEW RISK this combination creates that Milestone 10 never had
# to handle, confirmed for real during this file's own implementation (not
# assumed): z_gameengine's closure and this harness's own hand-picked
# WW3D2/GL render-stack sources (inherited unchanged from the milestone's
# first attempt) overlap heavily - GlobalData.cpp, TerrainLogic.cpp,
# FramePacer.cpp/FrameRateLimit.cpp, View.cpp, ParabolicEase.cpp, Line2D.cpp,
# DataChunk.cpp, Trig.cpp, Dict.cpp, NameKeyGenerator.cpp, ObjectStatusTypes.cpp,
# Money.cpp, CriticalSection.cpp, Snapshot.cpp, GameUtility.cpp, and the
# entire Common/System/{ArchiveFile,ArchiveFileSystem,AsciiString,
# UnicodeString,File,FileSystem,LocalFile,LocalFileSystem,RAMFile,registry,
# StreamingArchiveFile,SubsystemInterface,GameMemory,GameMemoryInit}.cpp
# block are ALL already part of z_gameengine's/corei_gameengine_private's own
# real source lists (confirmed by direct reading of both CMakeLists.txt
# files, not assumed) - keeping them ALSO hand-picked below would compile
# each one twice into two different object files feeding the SAME
# executable, a real duplicate-symbol link error. All of the above are
# REMOVED from this file's own hand-picked list (resolved by excluding the
# hand-picked duplicate, per the milestone brief's own guidance, since the
# closure is the strictly larger and more complete of the two sources for
# every one of them) - what remains hand-picked below is exactly the
# GameEngineDevice-tree (W3D-device-layer) subset that z_gameengine's own
# closure never contained in the first place (it is a different CMake
# target/tree entirely): the four StdDevice twins, W3DFileSystem.cpp,
# wwdebugstub.cpp, matrixmapper.cpp, W3DScene.cpp, W3DShroud.cpp,
# W3DConvert.cpp, and this harness's own payoff object, W3DView.cpp.
#
# link_stubs.cpp received the SAME kind of pruning for the same reason: every
# stub in it that merely re-supplied a singleton pointer or member-function
# body the closure NOW provides for real (TheGameLogic, TheScriptEngine,
# TheGameClient, TheDisplay, TheInGameUI, TheFontLibrary, TheMouse,
# GameLogic::isGamePaused()/getFirstObject()/destroyObject(),
# ScriptEngine::isTimeFrozenDebug()/isTimeFrozenScript()/isTimeFast(),
# GameClient::flushTextBearingDrawables(), Object::attemptDamage()/
# updateObjValuesFromMapProperties(), and others enumerated in that file's
# own header comment) was REMOVED - see that file for the full, real,
# linker-driven accounting of which stubs survived and why.
function(_m11_add_render_view_update_draw_test)
set(_m11_this_dir "${CMAKE_SOURCE_DIR}/Tests/RenderViewUpdateDraw")

get_target_property(_m11_src_gmd z_gameengine SOURCES)
get_target_property(_m11_src_core corei_gameengine_private INTERFACE_SOURCES)

# Verbatim copy of Tests/GameLogicTickHarness/CMakeLists.txt's own exclude
# regex (see that file's header comment for the full, real, per-file
# rationale of each entry - every one a genuinely Windows-only header
# reachable from the respective .cpp, none reachable from this harness's own
# construction path either, confirmed by this file's own real link attempt).
set(_m11_exclude_regex
    "Source/Common/GameEngine\\.cpp$|GUI/IMEManager\\.cpp$|Source/Common/System/Debug\\.cpp$|WOLBrowser/WebBrowser\\.cpp$|GameSpy/MainMenuUtils\\.cpp$|Menus/MainMenu\\.cpp$|Menus/WOLLadderScreen\\.cpp$|Menus/WOLLoginMenu\\.cpp$|Menus/WOLWelcomeMenu\\.cpp$|GameNetwork/DownloadManager\\.cpp$|Menus/DownloadMenu\\.cpp$|System/MiniDumper\\.cpp$|System/StackDump\\.cpp$|Bezier/BezierSegment\\.cpp$|Bezier/BezFwdIterator\\.cpp$|INI/INIWebpageURL\\.cpp$|Behavior/DumbProjectileBehavior\\.cpp$")

list(FILTER _m11_src_gmd EXCLUDE REGEX "${_m11_exclude_regex}")
list(FILTER _m11_src_core EXCLUDE REGEX "${_m11_exclude_regex}")

# Both lists declare relative paths, rooted at their OWN CMakeLists.txt's
# source dir - re-root each independently (identical technique to Tests/
# GameLogicTickHarness/CMakeLists.txt).
set(_m11_gameengine_sources)
foreach(_m11_src ${_m11_src_gmd})
    if(_m11_src MATCHES "\\.cpp$")
        if(IS_ABSOLUTE "${_m11_src}")
            list(APPEND _m11_gameengine_sources "${_m11_src}")
        else()
            list(APPEND _m11_gameengine_sources "${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/${_m11_src}")
        endif()
    endif()
endforeach()
foreach(_m11_src ${_m11_src_core})
    if(_m11_src MATCHES "\\.cpp$")
        if(IS_ABSOLUTE "${_m11_src}")
            list(APPEND _m11_gameengine_sources "${_m11_src}")
        else()
            list(APPEND _m11_gameengine_sources "${CMAKE_SOURCE_DIR}/Core/GameEngine/${_m11_src}")
        endif()
    endif()
endforeach()
list(REMOVE_DUPLICATES _m11_gameengine_sources)

add_executable(RenderViewUpdateDrawTest
    ${_m11_this_dir}/main.cpp
    ${_m11_this_dir}/link_stubs.cpp
    ${_m11_this_dir}/harness_stub_classes.cpp

    # ---- The full real z_gameengine/corei_gameengine_private source
    # closure (Milestone 10's own technique) - supplies, among ~300+ other
    # real .cpp files, GameLogic.cpp, ScriptEngine.cpp, GameClient.cpp,
    # InGameUI.cpp, Display.cpp, GameFont.cpp (FontLibrary), Mouse.cpp,
    # TerrainLogic.cpp, GlobalData.cpp, FramePacer.cpp/FrameRateLimit.cpp,
    # View.cpp, ParabolicEase.cpp, Line2D.cpp, and the whole Common/System
    # file-system/memory-manager layer - see this file's own header comment
    # for the full accounting of what this replaces from the milestone's
    # first attempt. ----
    ${_m11_gameengine_sources}

    # ---- GameEngineDevice-tree (W3D-device-layer) sources - NOT part of
    # z_gameengine's closure (a different CMake target/tree entirely), so
    # still hand-picked exactly as the milestone's first attempt already
    # established. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFile.cpp

    # ---- W3DFileSystem.cpp: the real WW3D<->GameEngine seam. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DFileSystem.cpp

    # ---- wwdebugstub.cpp, direct-listed (unchanged from the first attempt). ----
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWStub/wwdebugstub.cpp

    # ---- matrixmapper.cpp: per-tree, needed by decalsys.cpp's
    # MatrixMapperClass::Set_Texture_Transform reference (unchanged). ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp

    # ---- The unified RTS3DScene/RTS2DScene/RTS3DInterfaceScene TU. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DScene.cpp

    # ---- W3DShroud.cpp (carried over unchanged - Install_Materials/
    # UnInstall_Materials vtable-completeness reason). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DShroud.cpp

    # ---- The real, unified, WHOLE W3DView.cpp TU - this harness's own
    # payoff object, unchanged from the first attempt. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DView.cpp

    # ---- W3DConvert.cpp: W3DLogicalScreenToPixelScreen()/
    # PixelScreenToW3DLogicalScreen(), needed by W3DView.cpp's
    # getPickRay()/worldToScreenTriReturn()/calcCameraAreaOffset(). ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/Common/W3DConvert.cpp

    # ---- NEW for this retry: the real W3DStatusCircle.cpp (Draft 35 finding
    # 10's own real early-out proof needs the REAL class, not a stub - its
    # constructor is what RTS2DScene::RTS2DScene() unconditionally calls,
    # W3DScene.cpp:2171). WIN32-gated in Core/GameEngineDevice/CMakeLists.txt
    # (grouped with four other small W3DScene.cpp closure siblings, none of
    # which this harness needs), but a real link attempt is the authority on
    # whether its own #include "W3DDevice/GameClient/WorldHeightMap.h" drags
    # in anything genuinely non-portable - see link_stubs.cpp for whatever
    # that attempt actually found. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DStatusCircle.cpp
)

target_link_libraries(RenderViewUpdateDrawTest PRIVATE
    zi_always
    core_wwvegas
    corei_ww3d2
    portabled3d8_gl
    # core_compression: DataChunk.cpp (#includes "Compression.h") needs its
    # INTERFACE include directory (unchanged from the first attempt).
    core_compression
    # corei_always / corei_gameengine_include / gamespy::gamespy: Milestone
    # 10's own link-library set (see Tests/GameLogicTickHarness/CMakeLists.txt's
    # header comment for the full rationale of each) - now needed here too
    # since this target compiles the same real GameEngine closure.
    # corei_gameengine_public is deliberately NOT linked directly (same
    # `-ld3d8lib` finding M10 documented); corei_gameengine_include supplies
    # just the "GameEngine/Include" path instead.
    corei_always
    corei_gameengine_include
    gamespy::gamespy
)

target_compile_features(RenderViewUpdateDrawTest PRIVATE cxx_std_17)

# Matches the real IG_DEBUG_STACKTRACE define every z_gameengine/g_gameengine
# TU already compiles with (M10's own finding, carried over here since this
# target now compiles the same real closure).
target_compile_definitions(RenderViewUpdateDrawTest PRIVATE IG_DEBUG_STACKTRACE)

# Real link-closure finding, carried over unchanged from the milestone's
# first attempt (same reasons, PLUS the closure's own large volume of dead
# code paths this harness's main() never reaches).
target_compile_options(RenderViewUpdateDrawTest PRIVATE -ffunction-sections -fdata-sections)
target_link_options(RenderViewUpdateDrawTest PRIVATE -Wl,--gc-sections)

# Real, hard-evidence-confirmed integration finding, carried over verbatim
# from Tests/RenderRTS3DScene/CMakeLists.txt (same Mesa/LLVM interposition +
# std::filesystem inline/library-split problem, same fix).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(RenderViewUpdateDrawTest PRIVATE
        -static-libstdc++ -static-libgcc
        -Wl,--version-script=${_m11_this_dir}/hide_global_allocator_symbols.map)
endif()

# Header resolution: the milestone's first attempt's set, unchanged - the
# real closure's own files need nothing beyond what these paths already
# resolve (same Core/GeneralsMD Include paths z_gameengine/
# corei_gameengine_private themselves use to compile the exact same files).
target_include_directories(RenderViewUpdateDrawTest PRIVATE
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
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/renderviewupdatedraw_scratch)

add_test(NAME RenderViewUpdateDrawTest COMMAND RenderViewUpdateDrawTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/renderviewupdatedraw_scratch)
endfunction()

# Ordering note (identical to Tests/GameLogicTickHarness/CMakeLists.txt):
# Tests/ is add_subdirectory()'d BEFORE GeneralsMD/ in the top-level
# CMakeLists.txt, so z_gameengine/corei_gameengine_private do not exist yet
# at the point this file is normally processed. Defer this target's own
# creation to the end of the top-level directory's processing instead of
# reordering the shared top-level CMakeLists.txt.
cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _m11_add_render_view_update_draw_test)
