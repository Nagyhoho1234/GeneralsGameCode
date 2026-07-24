# Phase 5(a) Milestone 12 (native port plan, Draft 36, "rung 2b") - the real,
# unmodified GameEngine::init()/update()/execute() on POSIX. Extends
# Milestone 10's DEFER-closure link technique (Tests/GameLogicTickHarness/
# CMakeLists.txt) and Milestone 11's stub-subclass-plus-closure combination
# (Tests/RenderViewUpdateDraw/CMakeLists.txt) one step further: THIS harness
# is the first to include Source/Common/GameEngine.cpp ITSELF in the closure,
# rather than excluding it - the one file every prior milestone in this port
# deliberately left out (its unconditional #include of
# GameNetwork/WOLBrowser/WebBrowser.h was genuinely, structurally ATL/COM-
# based). Draft 36's own real compile spike found that blocker retired by a
# tiny, disclosed fix directly in the real, shared GameEngine.cpp (guarding
# that one #include behind #ifdef _WIN32 - zero behavior change on Windows,
# where the branch is untouched) plus two small compat-header shims
# (Dependencies/Utility/Utility/compat.h: HINSTANCE/CComModule,
# SetWindowText/SetWindowTextW) - see that file's own header comment for the
# full rationale. No other tracked file needed to change.
#
# This harness has NO WW3D2/GL/rendering, no doctest, no asset loading beyond
# the test-authored Data/INI scaffold - it deliberately does NOT link
# z_wwvegas/core_ww3d2 (the WW3D2 rendering library carrying this port's 34
# known pre-existing device-layer errors), nor any GameEngineDevice-tree
# (W3D-device-layer) sources at all: unlike Milestone 11, this harness's own
# construction path never reaches W3DView/W3DScene/W3DDisplay (TheDisplay
# stays a harness-local DisplayStub, matching Milestone 11's own pattern, and
# TheGameClient's real createGameDisplay() factory is never called - only
# createTerrainVisual()/createDisplayStringManager()/createInGameUI() are,
# per this harness's own PosixGameEngine/GameClientStub - see
# harness_stub_classes.h).
function(_m12_add_posix_game_engine_harness)
set(_m12_this_dir "${CMAKE_SOURCE_DIR}/Tests/PosixGameEngineHarness")

get_target_property(_m12_src_gmd z_gameengine SOURCES)
get_target_property(_m12_src_core corei_gameengine_private INTERFACE_SOURCES)

# Verbatim copy of Tests/GameLogicTickHarness/CMakeLists.txt's own exclude
# regex MINUS "Source/Common/GameEngine\\.cpp$" - THIS milestone's whole
# point is to include that file for real (see this file's own header
# comment). Every other entry is unchanged: a real, genuinely Windows-only
# header reachable from the respective .cpp, none reachable from this
# harness's own construction path either (confirmed by this file's own real
# link attempt during implementation).
set(_m12_exclude_regex
    "GUI/IMEManager\\.cpp$|Source/Common/System/Debug\\.cpp$|WOLBrowser/WebBrowser\\.cpp$|GameSpy/MainMenuUtils\\.cpp$|Menus/MainMenu\\.cpp$|Menus/WOLLadderScreen\\.cpp$|Menus/WOLLoginMenu\\.cpp$|Menus/WOLWelcomeMenu\\.cpp$|GameNetwork/DownloadManager\\.cpp$|Menus/DownloadMenu\\.cpp$|System/MiniDumper\\.cpp$|System/StackDump\\.cpp$|Bezier/BezierSegment\\.cpp$|Bezier/BezFwdIterator\\.cpp$|INI/INIWebpageURL\\.cpp$|Behavior/DumbProjectileBehavior\\.cpp$")

list(FILTER _m12_src_gmd EXCLUDE REGEX "${_m12_exclude_regex}")
list(FILTER _m12_src_core EXCLUDE REGEX "${_m12_exclude_regex}")

# Both lists declare relative paths, rooted at their OWN CMakeLists.txt's
# source dir - re-root each independently (identical technique to Tests/
# GameLogicTickHarness/CMakeLists.txt and Tests/RenderViewUpdateDraw/CMakeLists.txt).
set(_m12_gameengine_sources)
foreach(_m12_src ${_m12_src_gmd})
    if(_m12_src MATCHES "\\.cpp$")
        if(IS_ABSOLUTE "${_m12_src}")
            list(APPEND _m12_gameengine_sources "${_m12_src}")
        else()
            list(APPEND _m12_gameengine_sources "${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/${_m12_src}")
        endif()
    endif()
endforeach()
foreach(_m12_src ${_m12_src_core})
    if(_m12_src MATCHES "\\.cpp$")
        if(IS_ABSOLUTE "${_m12_src}")
            list(APPEND _m12_gameengine_sources "${_m12_src}")
        else()
            list(APPEND _m12_gameengine_sources "${CMAKE_SOURCE_DIR}/Core/GameEngine/${_m12_src}")
        endif()
    endif()
endforeach()
list(REMOVE_DUPLICATES _m12_gameengine_sources)

add_executable(PosixGameEngineHarnessTest
    ${_m12_this_dir}/main.cpp
    ${_m12_this_dir}/link_stubs.cpp
    ${_m12_this_dir}/harness_stub_classes.cpp

    ${_m12_gameengine_sources}

    # ---- StdLocalFileSystem/StdBIGFileSystem: the same proven-running
    # POSIX file-system classes Milestone 7 shipped, needed here for real -
    # this milestone's PosixGameEngine::createLocalFileSystem()/
    # createArchiveFileSystem() factories return them for real, and the real
    # GameEngine::init() genuinely loads this harness's test-authored
    # Data/INI scaffold through them. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFile.cpp
)

target_link_libraries(PosixGameEngineHarnessTest PRIVATE
    zi_always
    core_compression
    # corei_always / corei_gameengine_include / gamespy::gamespy: Milestone
    # 10/11's own link-library set (see Tests/GameLogicTickHarness/CMakeLists.txt's
    # header comment for the full rationale of each). corei_gameengine_public
    # is deliberately NOT linked directly (real, implementation-time finding
    # carried over from Milestone 10: it unconditionally links `d3d8lib`,
    # only ever a real CMake target on WIN32) - corei_gameengine_include
    # supplies just the "GameEngine/Include" path instead.
    corei_always
    corei_gameengine_include
    gamespy::gamespy
    # core_wwvegas (NOT z_wwvegas): z_gameengine itself links z_wwvegas
    # PUBLIC for its WWLib/WWMath/WWDebug/WWSaveLoad include paths - but
    # z_wwvegas also links z_ww3d2, the big WW3D2 rendering library carrying
    # this port's 34 known pre-existing device-layer errors. core_wwvegas is
    # the Core-tree sibling INTERFACE library providing the SAME include
    # paths + small already-portable static libs WITHOUT core_ww3d2 -
    # already proven safe by Milestone 10's own harness (this one has no
    # more rendering dependency than that one did).
    core_wwvegas
)

target_compile_features(PosixGameEngineHarnessTest PRIVATE cxx_std_17)

# Matches the real IG_DEBUG_STACKTRACE define every z_gameengine/g_gameengine
# TU already compiles with (Milestone 10's own finding, carried over here
# since this target compiles the same real closure).
target_compile_definitions(PosixGameEngineHarnessTest PRIVATE IG_DEBUG_STACKTRACE)

target_compile_options(PosixGameEngineHarnessTest PRIVATE -ffunction-sections -fdata-sections)
target_link_options(PosixGameEngineHarnessTest PRIVATE -Wl,--gc-sections)

# Real, hard-evidence-confirmed integration finding, carried over verbatim
# from every prior harness in this port (same Mesa/LLVM interposition +
# std::filesystem inline/library-split problem, same fix) - kept even though
# this harness has no Mesa/GL dependency, since the static-libstdc++ half of
# this fix is independently useful for a standalone executable performing
# real std::filesystem-based file I/O (this harness's real INI scaffold
# loading), matching Tests/GameLogicTickHarness's own established reasoning.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(PosixGameEngineHarnessTest PRIVATE
        -static-libstdc++ -static-libgcc
        -Wl,--version-script=${_m12_this_dir}/hide_global_allocator_symbols.map)
endif()

target_include_directories(PosixGameEngineHarnessTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include/Precompiled
)

# StdBIGFileSystem::init() scans its cwd recursively for "*.big" - dedicated
# scratch dir so this harness's run never collides with any other target's
# output. This harness's test-authored Data/INI scaffold (tracked in the
# source tree, see this directory's own Data/ subtree) is copied alongside
# it at configure time so the real GameEngine::init()'s real
# INI::loadFileDirectory() calls resolve against real, on-disk files -
# TheLocalFileSystem/TheArchiveFileSystem are both real, live classes this
# milestone's PosixGameEngine factories return (unlike every prior
# milestone), so "no real INI content" is no longer an option the way it was
# for Milestone 7-11's own shortcuts.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/posixgameengine_scratch)
file(COPY ${_m12_this_dir}/Data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/posixgameengine_scratch)

add_test(NAME PosixGameEngineHarnessTest COMMAND PosixGameEngineHarnessTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/posixgameengine_scratch)
endfunction()

# Ordering note (identical to Tests/GameLogicTickHarness/CMakeLists.txt and
# Tests/RenderViewUpdateDraw/CMakeLists.txt): Tests/ is add_subdirectory()'d
# BEFORE GeneralsMD/ in the top-level CMakeLists.txt, so z_gameengine/
# corei_gameengine_private do not exist yet at the point this file is
# normally processed. Defer this target's own creation to the end of the
# top-level directory's processing instead of reordering the shared
# top-level CMakeLists.txt.
cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _m12_add_posix_game_engine_harness)
