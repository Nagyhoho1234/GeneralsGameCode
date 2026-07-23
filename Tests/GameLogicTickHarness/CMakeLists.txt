# Phase 5(a) Milestone 10 (native port plan, Draft 34 "rung 2b-lite") - a
# headless TheAI->TheGameLogic->TheScriptEngine->TheTerrainLogic->
# ThePartitionManager tick harness. NOT WIN32 for the same reasons as its nine
# rendering/logic siblings and Tests/GameFileSystem. See main.cpp for the full
# construction order and checks, and docs/native-port-plan-rung2b-lite-draft.md
# (Draft 34) plus .superpowers/sdd/m10-task1-spike-report.md for the design
# rationale.
#
# A REAL, IMPLEMENTATION-TIME FINDING that changed this harness's own
# structure from every prior Tests/ harness's "hand-pick the exact .cpp
# files" pattern: GameLogic.cpp's real link closure (traced by the real
# linker, not predicted) is not a small, hand-pickable set the way
# TerrainLogic.cpp's was for Tests/RenderCameraTransform - it genuinely needs
# Object.cpp, Player.cpp, Team.cpp, Drawable.cpp, ControlBar.cpp, the whole
# Pathfinder (AIPathfind.cpp), ThingTemplate/ThingFactory, ScriptList,
# GameInfo, INI.cpp, and roughly 300+ further real .cpp files, split across
# TWO real, authoritative source lists this port's own build already
# maintains:
#   - GeneralsMD/Code/GameEngine/CMakeLists.txt's own GAMEENGINE_SRC (read via
#     the already-configured z_gameengine target's SOURCES property), and
#   - Core/GameEngine/CMakeLists.txt's own, DIFFERENT GAMEENGINE_SRC variable
#     of the same name (a real, easy-to-miss shadowing - GeneralsMD's own
#     list deliberately comments out files like INI.cpp, SubsystemInterface.cpp,
#     Water.cpp, Snow.cpp, AIPathfind.cpp precisely because they are supplied
#     by THIS other, Core-tree list instead, consumed via
#     corei_gameengine_private's `target_sources(... INTERFACE ${GAMEENGINE_SRC})`
#     - read here via corei_gameengine_private's INTERFACE_SOURCES property).
# Both lists together are the real, complete, already-portable (34-error
# baseline unaffected) closure z_gameengine's own real build already proves
# compiles - minus a small, explicitly-named set of genuinely Windows-only
# files (below), none reachable from this harness's zero-object, zero-map,
# zero-network tick loop. This is a disclosed, larger-than-planned deviation
# from Draft 34's task breakdown (which predicted a short, explicit file list
# matching Tests/RenderCameraTransform's own pattern) - the real code's own
# link closure turned out to be far less severable than the draft's static
# reading suggested, without weakening the "real, unmodified engine code"
# property this milestone still delivers on: every file below is the exact
# same real source z_gameengine/corei_gameengine_private themselves compile,
# none of it rewritten or stubbed to fit.
#
# This harness has NO WW3D2/GL/rendering, no doctest, no asset loading, no
# .big archive - it deliberately does NOT link z_wwvegas/core_ww3d2 (the
# WW3D2 rendering library carrying this port's 34 known pre-existing device-
# layer errors), nor corei_gameengine_public directly (see the
# target_link_libraries block below for why - a real, separate
# implementation-time finding, `-ld3d8lib` not found on Linux). GameEngine.cpp
# is deliberately excluded (see link_stubs.cpp for the full TheGameEngine
# rationale); W3DTerrainLogic.cpp/W3DGhostObject.cpp were never in either
# GAMEENGINE_SRC list to begin with (they are WIN32-gated device-layer files
# in a completely separate CMakeLists.txt) - this harness uses the real,
# base TerrainLogic/GhostObjectManager classes instead (see main.cpp's
# header comment).
#
# Ordering note: Tests/ (add_subdirectory(Tests)) runs BEFORE GeneralsMD/
# (add_subdirectory(GeneralsMD)) in the top-level CMakeLists.txt, so the real
# z_gameengine/corei_gameengine_private targets do not exist yet at the point
# this file is normally processed. Rather than reorder the shared, top-level
# CMakeLists.txt (wider blast radius, affects every platform/target), this
# file defers its own target-creation logic via
# cmake_language(DEFER DIRECTORY ...), which runs once at the end of the
# top-level directory's processing - after add_subdirectory(GeneralsMD) has
# returned and both targets genuinely exist. Fully self-contained to this
# one new file; zero risk to any other target's build order.
function(_m10_add_game_logic_tick_harness)
set(_m10_this_dir "${CMAKE_SOURCE_DIR}/Tests/GameLogicTickHarness")

get_target_property(_m10_src_gmd z_gameengine SOURCES)
get_target_property(_m10_src_core corei_gameengine_private INTERFACE_SOURCES)

# Real, named exclusions - each backed by a real compile error found during
# this milestone's own implementation (-k0 "keep going", enumerating the
# true full set across both lists in a few passes rather than one file at a
# time), not predicted. Every excluded file transitively reaches one of five
# genuinely Windows-only headers, none reachable from this harness's
# zero-object, zero-UI, zero-network tick loop:
#   atlbase.h (via GameNetwork/WOLBrowser/WebBrowser.h - the real WebBrowser
#     class is genuinely, structurally ATL/COM-based, see link_stubs.cpp and
#     .superpowers/sdd/m10-task1-spike-report.md):
#       Source/Common/GameEngine.cpp (this harness never constructs a real
#         GameEngine-derived object at all), GameNetwork/WOLBrowser/WebBrowser.cpp,
#       GameNetwork/GameSpy/MainMenuUtils.cpp, GUICallbacks/Menus/MainMenu.cpp,
#         WOLLadderScreen.cpp, WOLLoginMenu.cpp, WOLWelcomeMenu.cpp
#   winsock.h (via WWDownload/ftp.h, reached through GameNetwork/DownloadManager.h):
#       GameNetwork/DownloadManager.cpp, GUICallbacks/Menus/DownloadMenu.cpp
#   imagehlp.h (via WWLib/DbgHelpLoader.h, reached through Common/MiniDumper.h):
#       Common/System/Debug.cpp, Common/System/MiniDumper.cpp, Common/System/StackDump.cpp
#   mbstring.h (via GameClient/GUI/IMEManager.cpp's own direct #include):
#       GameClient/GUI/IMEManager.cpp
#   d3dx8math.h (via Common/BezierSegment.h, a real D3DX-based Bezier-curve
#     math header used by a handful of camera/projectile-arc files):
#       Common/Bezier/BezierSegment.cpp, Common/Bezier/BezFwdIterator.cpp,
#       Common/INI/INIWebpageURL.cpp, GameLogic/Object/Behavior/DumbProjectileBehavior.cpp
set(_m10_exclude_regex
    "Source/Common/GameEngine\\.cpp$|GUI/IMEManager\\.cpp$|Source/Common/System/Debug\\.cpp$|WOLBrowser/WebBrowser\\.cpp$|GameSpy/MainMenuUtils\\.cpp$|Menus/MainMenu\\.cpp$|Menus/WOLLadderScreen\\.cpp$|Menus/WOLLoginMenu\\.cpp$|Menus/WOLWelcomeMenu\\.cpp$|GameNetwork/DownloadManager\\.cpp$|Menus/DownloadMenu\\.cpp$|System/MiniDumper\\.cpp$|System/StackDump\\.cpp$|Bezier/BezierSegment\\.cpp$|Bezier/BezFwdIterator\\.cpp$|INI/INIWebpageURL\\.cpp$|Behavior/DumbProjectileBehavior\\.cpp$")

list(FILTER _m10_src_gmd EXCLUDE REGEX "${_m10_exclude_regex}")
list(FILTER _m10_src_core EXCLUDE REGEX "${_m10_exclude_regex}")

# Both lists declare relative paths, rooted at their OWN CMakeLists.txt's
# source dir - re-root each independently.
set(_m10_gameengine_sources)
foreach(_m10_src ${_m10_src_gmd})
    if(_m10_src MATCHES "\\.cpp$")
        if(IS_ABSOLUTE "${_m10_src}")
            list(APPEND _m10_gameengine_sources "${_m10_src}")
        else()
            list(APPEND _m10_gameengine_sources "${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/${_m10_src}")
        endif()
    endif()
endforeach()
foreach(_m10_src ${_m10_src_core})
    if(_m10_src MATCHES "\\.cpp$")
        if(IS_ABSOLUTE "${_m10_src}")
            list(APPEND _m10_gameengine_sources "${_m10_src}")
        else()
            list(APPEND _m10_gameengine_sources "${CMAKE_SOURCE_DIR}/Core/GameEngine/${_m10_src}")
        endif()
    endif()
endforeach()
list(REMOVE_DUPLICATES _m10_gameengine_sources)

add_executable(GameLogicTickHarnessTest
    ${_m10_this_dir}/main.cpp
    ${_m10_this_dir}/link_stubs.cpp

    ${_m10_gameengine_sources}
)

target_link_libraries(GameLogicTickHarnessTest PRIVATE
    zi_always
    core_compression
    # corei_always: supplies core_config/core_profile_tracy/core_utility/
    # corei_libraries_include/resources - the same bundle
    # corei_gameengine_private would otherwise have provided via its own
    # target_link_libraries, WITHOUT that target's INTERFACE-source
    # side effect (see this file's own header comment - already consumed
    # directly above via corei_gameengine_private's INTERFACE_SOURCES
    # property instead of by linking the target, precisely to sidestep
    # its Windows-only files without also losing its legitimate ones).
    corei_always
    # corei_gameengine_include: JUST the "GameEngine/Include" path -
    # corei_gameengine_public is deliberately NOT linked directly - a real,
    # implementation-time finding: it unconditionally links `d3d8lib`
    # (Core/GameEngine/CMakeLists.txt:1209), which is only ever defined as a
    # real CMake target on WIN32 (cmake/dx8.cmake) - on Linux it silently
    # falls through to a raw `-ld3d8lib` linker flag that fails (`cannot
    # find -ld3d8lib`). Previously unexercised because every existing
    # harness in this port links core_wwvegas/portabled3d8_gl instead of
    # corei_gameengine_public directly - this harness is the first to try.
    corei_gameengine_include
    gamespy::gamespy
    # core_wwvegas (NOT z_wwvegas): z_gameengine itself links z_wwvegas
    # PUBLIC for its WWLib/WWMath/WWDebug/WWSaveLoad include paths (needed:
    # Common/GameCommon.h #includes WWCommon.h) - but z_wwvegas also links
    # z_ww3d2, the big WW3D2 rendering library carrying this port's 34 known
    # pre-existing device-layer errors. core_wwvegas is the Core-tree
    # sibling INTERFACE library providing the SAME include paths + small
    # already-portable static libs (core_wwdebug/core_wwlib/core_wwmath/
    # core_wwsaveload) WITHOUT core_ww3d2 (Core/Libraries/Source/WWVegas/
    # CMakeLists.txt:50-72) - already proven safe by every prior rendering
    # harness in this port that links it instead of z_wwvegas.
    core_wwvegas
)

target_compile_features(GameLogicTickHarnessTest PRIVATE cxx_std_17)

# Matches the real IG_DEBUG_STACKTRACE define every z_gameengine/g_gameengine
# TU already compiles with (normally supplied via corei_gameengine_private's
# target_compile_definitions, not linked here - see above) - kept for
# behavioral parity with the real build configuration, not a new choice.
target_compile_definitions(GameLogicTickHarnessTest PRIVATE IG_DEBUG_STACKTRACE)

target_compile_options(GameLogicTickHarnessTest PRIVATE -ffunction-sections -fdata-sections)
target_link_options(GameLogicTickHarnessTest PRIVATE -Wl,--gc-sections)

# Real, hard-evidence-confirmed integration finding, carried over verbatim
# from every prior harness in this port (same Mesa/LLVM interposition +
# std::filesystem inline/library-split problem, same fix) - kept even though
# this harness has no Mesa/GL dependency, since the static-libstdc++ half of
# this fix is independently useful for a standalone executable like this one.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(GameLogicTickHarnessTest PRIVATE
        -static-libstdc++ -static-libgcc
        -Wl,--version-script=${_m10_this_dir}/hide_global_allocator_symbols.map)
endif()

target_include_directories(GameLogicTickHarnessTest PRIVATE
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include/Precompiled
)

add_test(NAME GameLogicTickHarnessTest COMMAND GameLogicTickHarnessTest)
endfunction()

cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _m10_add_game_logic_tick_harness)
