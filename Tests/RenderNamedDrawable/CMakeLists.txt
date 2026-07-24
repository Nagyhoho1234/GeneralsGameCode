# Phase 5(a) Milestone 13 (native port plan, Draft 37, "rung 3b-ii-b") - a
# real, named Object+Drawable, constructed through the real
# ThingFactory::newObject()/GameLogic::sendObjectCreated()/
# bindObjectAndDrawable() chain, rendered through the real W3DModelDraw
# module, and picked back through the real W3DView::pickDrawable()/
# TheGameClient->iterateDrawablesInRegion() - the payoff Milestone 12 (rung
# 2b, real GameEngine::init()) unlocked per Draft 37's own chain-tracing (see
# docs/native-port-plan.md's Draft 37 section for the full rationale).
#
# New harness directory (not an extension of Tests/PosixGameEngineHarness):
# this milestone combines THREE previously-separate source closures - M12's
# real GameEngine::init()/GameClient::init() DEFER-closure technique (this
# file's own _m13_gameengine_sources block, copied verbatim from Tests/
# PosixGameEngineHarness/CMakeLists.txt), M11's hand-picked WW3D2/GL
# render-stack sources (Tests/RenderViewUpdateDraw/CMakeLists.txt's own
# GameEngineDevice-tree block, copied verbatim), PLUS two genuinely NEW
# per-tree device files (W3DModelDraw.cpp/W3DAssetManager.cpp) - a
# combination substantial and different enough from either parent milestone
# to warrant its own directory rather than bolting a third, unrelated
# concern onto Tests/PosixGameEngineHarness's already-established "real
# GameEngine::init()/update()/execute() smoke test" identity.
function(_m13_add_render_named_drawable_test)
set(_m13_this_dir "${CMAKE_SOURCE_DIR}/Tests/RenderNamedDrawable")

get_target_property(_m13_src_gmd z_gameengine SOURCES)
get_target_property(_m13_src_core corei_gameengine_private INTERFACE_SOURCES)

# Verbatim copy of Tests/PosixGameEngineHarness/CMakeLists.txt's own exclude
# regex (that milestone's real link attempt is the authority on this list -
# see that file's header comment for the full, real, per-file rationale of
# each entry).
set(_m13_exclude_regex
    "GUI/IMEManager\\.cpp$|Source/Common/System/Debug\\.cpp$|WOLBrowser/WebBrowser\\.cpp$|GameSpy/MainMenuUtils\\.cpp$|Menus/MainMenu\\.cpp$|Menus/WOLLadderScreen\\.cpp$|Menus/WOLLoginMenu\\.cpp$|Menus/WOLWelcomeMenu\\.cpp$|GameNetwork/DownloadManager\\.cpp$|Menus/DownloadMenu\\.cpp$|System/MiniDumper\\.cpp$|System/StackDump\\.cpp$|Bezier/BezierSegment\\.cpp$|Bezier/BezFwdIterator\\.cpp$|INI/INIWebpageURL\\.cpp$|Behavior/DumbProjectileBehavior\\.cpp$")

list(FILTER _m13_src_gmd EXCLUDE REGEX "${_m13_exclude_regex}")
list(FILTER _m13_src_core EXCLUDE REGEX "${_m13_exclude_regex}")

# Both lists declare relative paths, rooted at their OWN CMakeLists.txt's
# source dir - re-root each independently (identical technique to every
# prior DEFER-closure harness in this port).
set(_m13_gameengine_sources)
foreach(_m13_src ${_m13_src_gmd})
    if(_m13_src MATCHES "\\.cpp$")
        if(IS_ABSOLUTE "${_m13_src}")
            list(APPEND _m13_gameengine_sources "${_m13_src}")
        else()
            list(APPEND _m13_gameengine_sources "${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/${_m13_src}")
        endif()
    endif()
endforeach()
foreach(_m13_src ${_m13_src_core})
    if(_m13_src MATCHES "\\.cpp$")
        if(IS_ABSOLUTE "${_m13_src}")
            list(APPEND _m13_gameengine_sources "${_m13_src}")
        else()
            list(APPEND _m13_gameengine_sources "${CMAKE_SOURCE_DIR}/Core/GameEngine/${_m13_src}")
        endif()
    endif()
endforeach()
list(REMOVE_DUPLICATES _m13_gameengine_sources)

add_executable(RenderNamedDrawableTest
    ${_m13_this_dir}/main.cpp
    ${_m13_this_dir}/link_stubs.cpp
    ${_m13_this_dir}/harness_stub_classes.cpp

    # ---- The full real z_gameengine/corei_gameengine_private source
    # closure (Milestone 10's DEFER-closure technique, carried forward from
    # Milestone 12's own harness). ----
    ${_m13_gameengine_sources}

    # ---- StdLocalFileSystem/StdBIGFileSystem (Milestone 7/12's own real
    # POSIX file-system classes) - GameEngine::init() genuinely loads this
    # harness's own test-authored Data/INI scaffold through them. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFile.cpp

    # ---- Milestone 11's render-stack sources (Tests/RenderViewUpdateDraw/
    # CMakeLists.txt's own GameEngineDevice-tree hand-picked block, copied
    # verbatim - NOT part of z_gameengine's closure, a different CMake
    # target/tree entirely). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWStub/wwdebugstub.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DScene.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DShroud.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DView.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/Common/W3DConvert.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DStatusCircle.cpp

    # ---- NEW for Milestone 13: the two files Draft 37's own real compile
    # spike measured (see this milestone's own header comment on each file's
    # portability fix - the #ifdef _WIN32-guarded <windows.h> include plus
    # two uintptr_t casts in W3DModelDraw.cpp, the _strnicmp shim
    # (Dependencies/Utility/Utility/string_compat.h) plus two pointer-
    # difference-cast fixes in W3DAssetManager.cpp). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DModelDraw.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/GameClient/W3DAssetManager.cpp
)

target_link_libraries(RenderNamedDrawableTest PRIVATE
    zi_always
    core_compression
    corei_always
    corei_gameengine_include
    gamespy::gamespy
    core_wwvegas
    corei_ww3d2
    portabled3d8_gl
)

target_compile_features(RenderNamedDrawableTest PRIVATE cxx_std_17)

target_compile_definitions(RenderNamedDrawableTest PRIVATE IG_DEBUG_STACKTRACE)

target_compile_options(RenderNamedDrawableTest PRIVATE -ffunction-sections -fdata-sections)
target_link_options(RenderNamedDrawableTest PRIVATE -Wl,--gc-sections)

# Real, hard-evidence-confirmed integration finding, carried over verbatim
# from every prior rendering harness in this port (same Mesa/LLVM
# interposition + std::filesystem inline/library-split problem, same fix).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(RenderNamedDrawableTest PRIVATE
        -static-libstdc++ -static-libgcc
        -Wl,--version-script=${_m13_this_dir}/hide_global_allocator_symbols.map)
endif()

target_include_directories(RenderNamedDrawableTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include/Precompiled
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

# StdBIGFileSystem::init() scans its cwd recursively for "*.big" - dedicated
# scratch dir so this harness's run never collides with any other target's
# output. This harness's own test-authored Data/INI scaffold is copied
# alongside it at configure time (Milestone 12's own established
# technique).
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rendernameddrawable_scratch)
file(COPY ${_m13_this_dir}/Data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rendernameddrawable_scratch)

add_test(NAME RenderNamedDrawableTest COMMAND RenderNamedDrawableTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rendernameddrawable_scratch)
endfunction()

# Ordering note (identical to every prior DEFER-closure harness): Tests/ is
# add_subdirectory()'d BEFORE GeneralsMD/ in the top-level CMakeLists.txt, so
# z_gameengine/corei_gameengine_private do not exist yet at the point this
# file is normally processed. Defer this target's own creation to the end of
# the top-level directory's processing instead of reordering the shared
# top-level CMakeLists.txt.
cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _m13_add_render_named_drawable_test)
