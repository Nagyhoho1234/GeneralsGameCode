# Phase 5(a) Milestone 7 Task 2 (native port plan, Draft 28) - the headless
# file-system harness: the FIRST GameEngine code (not WW3D2 rendering code)
# ever to RUN, not merely compile, on POSIX. See main.cpp for the full
# six-check list and docs/native-port-plan.md's Draft 28 section for the
# full design rationale (findings 1, 2, 4, 5, 6; design decisions).
#
# NO GL/display dependency at all - this is a pure headless file-system
# test, unlike its five WW3D2 rendering siblings.
#
# Link-closure note (finding 5's "let the linker enumerate the truth"):
# the four StdDevice TUs (already compiling clean on POSIX for multiple
# milestones per finding 1, but never linked into anything that runs) plus
# a small, hand-picked, already-portable slice of Core/GameEngine's Common/
# System layer (finding 2's exact file list) are direct link inputs here,
# the same "small clean TUs as real link inputs, Draft 24 precedent" the
# plan calls for - NOT the whole per-tree GameEngineDevice library (which
# still carries the 34-error POSIX baseline elsewhere and doesn't itself
# link as a runnable target). See link_stubs.cpp for the harness-local
# stubs the resulting closure actually needed.
add_executable(GameFileSystemTest
    main.cpp
    link_stubs.cpp

    # ---- Core/GameEngine Common/System layer: finding 2's exact file list,
    # already confirmed clean-compiling on POSIX (no entry in the 34-error
    # baseline), direct-linked here rather than pulled through z_gameengine/
    # g_gameengine (those libraries are not fully linkable yet - most of
    # GAMEENGINE_SRC still hits the 34-error baseline elsewhere in the same
    # per-tree library). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/ArchiveFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/ArchiveFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/AsciiString.cpp
    # UnicodeString.cpp: real link-closure finding (open question 1) - not a
    # guess. AsciiString::translate(const UnicodeString&) is compiled into
    # AsciiString.cpp's object file regardless of whether this harness calls
    # it, and non-LTO linking (this repo sets no -ffunction-sections/
    # --gc-sections, cmake/compilers.cmake confirmed) must resolve every
    # external referenced anywhere in a directly-linked .o, dead code or
    # not (Draft 26's lesson, restated in finding 5). Small (467 lines) and
    # already in finding 2's confirmed-clean set - a real portable TU, not
    # a stub.
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/UnicodeString.cpp

    # Debug.cpp deliberately NOT linked - a real, hard-evidence finding, not a
    # guess: ReleaseCrash/ReleaseCrashLocalized (Debug.cpp:748-891) are
    # UNCONDITIONAL (not behind any DEBUG_LOGGING/DEBUG_CRASHING/_WIN32 guard -
    # the doc comment even says "EVEN IN FINAL RELEASE BUILDS") and call real
    # Win32 UI/crash-reporting APIs with no portable equivalent anywhere in
    # this tree (::MessageBox/::MessageBoxW/ShowWindow/SW_HIDE/MB_OK et al,
    # plus StackDump.cpp's FillStackAddresses/StackDumpFromAddresses). This is
    # a genuine, previously-undiscovered portability gap in Debug.cpp itself
    # (found because this milestone is the first to ever compile it for a
    # POSIX-linked runnable target - Debug.cpp is part of z_gameengine/
    # g_gameengine, not the z_gameenginedevice/g_gameenginedevice targets this
    # port's established 34-error baseline scopes), well beyond this task's
    # file-system scope to fix (crash-dialog UI portability, same class as the
    # standing Phase 6/7 font/input/fullscreen deferrals). This harness needs
    # none of it: with DEBUG_LOGGING/DEBUG_CRASHING off by default (this repo's
    # release-style config), every DEBUG_LOG/DEBUG_CRASH/DEBUG_ASSERTCRASH call
    # site anywhere in this closure already expands to a pure no-op
    # (Common/Debug.h:167-171,205-206) - nothing here ever calls an actual
    # Debug.cpp function, so simply not linking the TU is correct, not a
    # workaround. Recorded here for the close-out report; not fixed as part of
    # this task.
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/File.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/FileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/LocalFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/LocalFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/RAMFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/registry.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/StreamingArchiveFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/SubsystemInterface.cpp

    # ---- The real memory manager (finding 4): GameEngine file-system TUs
    # allocate via newInstance/MSGNEW, which need TheMemoryPoolFactory/
    # TheDynamicMemoryAllocator up before any of them run. This harness
    # links NEITHER core_wwstub (no WW3D2/GL dependency at all) NOR any
    # other allocator - GameMemory.cpp/GameMemoryInit.cpp are the only
    # allocator in this link, unconditionally. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/GameMemory.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/GameMemoryInit.cpp

    # ---- The four StdDevice twins (finding 1): already compiling clean on
    # POSIX for multiple milestones, never linked into anything that runs
    # until this harness. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFile.cpp

    # ---- CriticalSection.h/.cpp are genuinely per-tree only (never in
    # Core/GameEngine's file list at all, commented out of both trees'
    # header lists too) - GeneralsMD's copy, matching every other harness's
    # "GeneralsMD + RTS_ZEROHOUR=1" flavor (open question 2). Defines the
    # five real CriticalSection* globals the harness prologue wires up,
    # mirroring WinMain.cpp:875-882. ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/System/CriticalSection.cpp
)

target_link_libraries(GameFileSystemTest PRIVATE
    zi_always
    corei_libraries_source_wwvegas
    corei_libraries_source_wwvegas_wwlib
)

target_compile_features(GameFileSystemTest PRIVATE cxx_std_17)

# Real link-closure finding (open question 1), not a guess: SubsystemInterface.
# cpp's SubsystemInterfaceList::initSubsystem (never called by this harness -
# it builds its own file-system spine directly, GUIEdit.cpp's real precedent,
# main.cpp's own comment) still compiles a real `INI ini;` + loadFileDirectory
# call, and GameAudio.h's inline DynamicAudioEventRTS destructor still needs
# AudioEventRTS::~AudioEventRTS() - both are genuinely dead code paths this
# harness never reaches, but a non-LTO, non-gc'd link resolves every external
# referenced anywhere in a directly-linked .o regardless (Draft 26's lesson).
# Pulling in INI.cpp to satisfy this would drag the entire INI-driven data
# model (an explicit non-goal, GameEngine.cpp:456-527) for code this harness
# never executes - the correct, surgical fix is standard dead-code-elimination
# (-ffunction-sections/-fdata-sections + --gc-sections), scoped to this target
# only, so the linker itself proves these paths are unreachable rather than
# hand-stubbing every incidental unreferenced symbol individually.
target_compile_options(GameFileSystemTest PRIVATE -ffunction-sections -fdata-sections)
target_link_options(GameFileSystemTest PRIVATE -Wl,--gc-sections)

# Header resolution: Core/GameEngine + Core/GameEngineDevice's public
# Include dirs, GeneralsMD's per-tree GameEngine/Include (CriticalSection.h,
# GlobalData.h, GameDefines.h) and its Include/Precompiled (bare
# #include "PreRTS.h", required first in every GameEngine .cpp - PreRTS.h.cpp
# convention, same as every real GameEngine TU).
target_include_directories(GameFileSystemTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include/Precompiled
)

# StdBIGFileSystem::init() scans its cwd recursively for "*.big" (finding 2:
# loadBigFilesFromDirectory("", "*.big")) - ctest's WORKING_DIRECTORY covers
# this the same way finding 7 notes for the exit harness, so the engine's
# real init() path runs unmodified. Dedicated scratch dir (not the build
# dir root) so this harness's authored tree/archive never collides with
# any other target's output.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gamefilesystem_scratch)

add_test(NAME GameFileSystemTest COMMAND GameFileSystemTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gamefilesystem_scratch)
