# Phase 5(a) Milestone 7 Task 4 (native port plan, Draft 28) - the
# milestone's exit harness: every byte on screen comes out of a real BIG
# archive through the engine's own file stack (GameFileClass ->
# TheFileSystem -> StdBIGFile -> RAMFile), not a loose temp file. See
# main.cpp for the full four-check list and docs/native-port-plan.md's
# Draft 28 section for the full design rationale.
#
# NOT WIN32: same reasoning as its six rendering siblings and
# Tests/GameFileSystem - Windows already has the real D3D8 backend and its
# own CI; this only exercises the GL backend + the POSIX Std file-system
# layer together for the first time.
#
# Structure: Tests/GameFileSystem's file-system TU list (Core/GameEngine's
# Common/System layer + the four StdDevice twins + GeneralsMD's
# CriticalSection.cpp) PLUS the newly-unified
# Core/GameEngineDevice/.../W3DFileSystem.cpp (Milestone 7 Task 3) PLUS
# Tests/RenderWW3DFrame's WW3D2/GL link set (core_wwvegas/corei_ww3d2/
# portabled3d8_gl/matrixmapper.cpp) - the first harness in this port to
# combine both halves.
#
# Allocator swap (Draft 28 finding 4, open question 3): the GameEngine
# file-system TUs above need the REAL Core/GameEngine/Source/Common/System/
# GameMemory.cpp(+GameMemoryInit.cpp) allocator (newInstance/MSGNEW), which
# defines the SAME global operator new/delete/createW3DMemPool symbols as
# core_wwstub's malloc-backed wwallocstub.cpp - one link cannot contain both.
# core_wwstub's OTHER member, wwdebugstub.cpp (still needed - this harness's
# WW3D2/WWLib code calls WWDEBUG_SAY/etc.), is direct-listed below instead of
# linking the whole core_wwstub target, deliberately chosen over splitting
# core_wwstub into two CMake targets: it keeps core_wwstub itself, and every
# one of the other 8 pre-existing harnesses that link it
# (RenderDeviceInit/RenderEngineDrawPath/RenderTexturedTriangle/
# RenderTexturePipeline/RenderW3DMesh/RenderWW3DFrame, plus the two Tools/
# W3DView + Tools/wdump consumers), completely untouched - verified by a real
# rebuild of all of them after this file was added (see the close-out
# report), rather than a CMakeLists.txt change to a shared target that every
# one of those consumers would also need to be re-verified against.
add_executable(RenderGameAssetsTest
    main.cpp
    link_stubs.cpp

    # ---- Core/GameEngine Common/System layer: same exact file list as
    # Tests/GameFileSystem/CMakeLists.txt (finding 2's confirmed-clean set,
    # Debug.cpp deliberately excluded - see that file's own long-form
    # comment for the full rationale, unchanged here: DEBUG_LOGGING/
    # DEBUG_CRASHING are off by default, so nothing in this closure ever
    # calls a real Debug.cpp function). ----
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

    # ---- The real memory manager (finding 4): NEITHER core_wwstub's
    # allocator half NOR any other allocator is linked here - GameMemory.cpp/
    # GameMemoryInit.cpp is the only allocator in this link, unconditionally.
    # This means the entire WW3D render stack below now runs on the game's
    # real memory manager on POSIX for the first time. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/GameMemory.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Source/Common/System/GameMemoryInit.cpp

    # ---- The four StdDevice twins (finding 1). ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFile.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFile.cpp

    # ---- W3DFileSystem.cpp (Milestone 7 Task 3, now Core-unified): the real
    # WW3D<->GameEngine seam. Its constructor performs the
    # "_TheFileFactory = this" swap (W3DFileSystem.cpp:511-518), replacing
    # WWLib's default factory (ffactory.cpp:49) every other harness
    # implicitly uses - this is the installation this whole harness exists
    # to exercise. ----
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DFileSystem.cpp

    # ---- CriticalSection.h/.cpp: genuinely per-tree only. GeneralsMD's
    # copy, matching every other harness's "GeneralsMD + RTS_ZEROHOUR=1"
    # flavor. Defines the five real CriticalSection* globals the harness
    # prologue wires up, mirroring WinMain.cpp:875-882. ----
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Source/Common/System/CriticalSection.cpp

    # ---- wwdebugstub.cpp, direct-listed (open question 3): core_wwstub's
    # OTHER member (TheCurrentIgnoreCrashPtr + the DEBUG_LOGGING/
    # DEBUG_CRASHING-gated DebugLog/DebugCrash bodies, both compiled-out in
    # this repo's default release-style config) - still needed since this
    # harness's WW3D2/WWLib code calls WWDEBUG_SAY/DebugCrash-family macros,
    # but core_wwstub's OTHER member (wwallocstub.cpp) must NOT be linked
    # here (see the allocator-swap comment above). ----
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWStub/wwdebugstub.cpp

    # See Tests/RenderWW3DFrame/CMakeLists.txt's matching comment:
    # matrixmapper.cpp is per-tree (Generals/GeneralsMD diverge), decalsys.cpp
    # (portable) references MatrixMapperClass::Set_Texture_Transform - the
    # symbol just needs to link, decal generation is not exercised by this
    # harness either.
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp
)

target_link_libraries(RenderGameAssetsTest PRIVATE
    # zi_always (RTS_ZEROHOUR=1 + corei_always, GeneralsMD flavor - open
    # question 2) instead of Tests/RenderWW3DFrame's bare corei_always: the
    # GameEngine Common/System TUs and W3DFileSystem.cpp above need
    # RTS_ZEROHOUR defined (W3DFileSystem.cpp's localized-lookup branch,
    # StdBIGFileSystem.cpp's registry lookup). zi_always already links
    # corei_always itself, so nothing is lost from the WW3D2 half.
    zi_always
    core_wwvegas
    corei_ww3d2
    portabled3d8_gl
    # Deliberately NOT core_wwstub - see the allocator-swap comment above.
)

target_compile_features(RenderGameAssetsTest PRIVATE cxx_std_17)

# Real link-closure finding, carried over unchanged from Tests/GameFileSystem
# (same TU set, same reasons): SubsystemInterface.cpp's never-called
# SubsystemInterfaceList::initSubsystem still compiles a real
# INI::loadFileDirectory call, and GameAudio.h's inline
# DynamicAudioEventRTS destructor (pulled in transitively by link_stubs.cpp's
# TheAudio stub) still needs AudioEventRTS::~AudioEventRTS() - both are
# genuinely dead code paths this harness never reaches. Standard dead-code
# elimination, scoped to this target only, lets the linker itself prove these
# paths unreachable rather than hand-stubbing every incidental symbol.
target_compile_options(RenderGameAssetsTest PRIVATE -ffunction-sections -fdata-sections)
target_link_options(RenderGameAssetsTest PRIVATE -Wl,--gc-sections)

# Real, hard-evidence-confirmed integration finding (a genuine "discover it
# in a stack trace" risk finding 4 flagged in advance): GameMemory.cpp's
# global operator new/delete override, once dynamically exported by this
# PIE executable, gets interposed onto every OTHER shared object loaded into
# the same process too - including Mesa's llvmpipe software rasterizer's
# libLLVM.so, whose own JIT'd-shader codegen (llvm::MCJIT::
# generateCodeForModule) reliably SIGSEGVs once its internal allocations get
# silently rerouted through TheDynamicMemoryAllocator's coarser (4-byte,
# not glibc-malloc-compatible 16-byte) alignment guarantee. Confirmed with
# an isolated minimal repro (see hide_global_allocator_symbols.map's own
# comment for the full story) before writing this fix - not a preemptive
# guess. Fix: a linker version script demoting exactly GameMemory.cpp's
# global-allocator symbols to "local" (not dynamically exported), so other,
# separately-loaded shared objects fall back to their own libstdc++
# operator new/delete instead of interposing onto ours - this harness's OWN
# code is completely unaffected (intra-executable symbol resolution happens
# at static link time, before dynamic-export visibility is consulted).
#
# That version script alone, however, creates a SECOND real problem, also
# found by hard evidence (a real "free(): invalid pointer" SIGABRT on this
# harness's very first std::filesystem call, isolated to a minimal repro
# the same way): std::filesystem's own implementation is split between code
# inlined into THIS executable and code compiled into libstdc++.so itself.
# Once the version script hides our operator new/delete from dynamic
# export, libstdc++.so's OWN internal calls to those symbols (made from
# non-inline std::filesystem library code, not from our .o files) can no
# longer find our override either - so a buffer allocated on one side of
# that inline/library-boundary split via one allocator can end up freed on
# the other side via a different one, corrupting the heap. This is not
# hypothetical: it reproduces the SAME way this harness's real file-system
# code path is exercised (StdLocalFileSystem.cpp's own fixFilenameFromWindowsPath
# uses std::filesystem::path/exists/directory_iterator for its case-
# insensitive lookup - exactly the path this harness's real GameFileClass
# asset lookups take whenever a name misses locally, which they always do
# here since quad.w3d/quad.tga exist ONLY in the archive). Fix: statically
# link libstdc++/libgcc for this target only, so std::filesystem's library-
# side implementation is compiled into (not dynamically resolved against a
# separate) this executable, eliminating the inline/library split entirely,
# while Mesa/libLLVM.so remain genuinely separate, dynamically-loaded
# shared objects unaffected by this (so they still correctly fail to find -
# and don't try to use - our hidden allocator symbols). Verified: both the
# Mesa/LLVM crash AND the std::filesystem corruption are gone together with
# this combination (5+ stable runs each in isolation, and this harness's
# full run - see the close-out report for the actual ctest evidence).
# GNU-only: -static-libstdc++/-static-libgcc and -Wl,--version-script are
# both GCC/GNU-ld-specific (no Clang/ld64 equivalent flag with the same
# semantics). Guarded rather than applied unconditionally - this repo's
# scoped baseline also targets macos-native.yml (Clang, Mach-O two-level
# namespace symbol binding, which does not exhibit this Linux/ELF flat-
# namespace interposition problem the same way in the first place), but
# that workflow does not build this harness today, so this is a defensive
# guard against a real future landmine, not yet independently verified on
# Clang/macOS.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(RenderGameAssetsTest PRIVATE
        -static-libstdc++ -static-libgcc
        -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/hide_global_allocator_symbols.map)
endif()

# Header resolution: Core/GameEngine + Core/GameEngineDevice's public Include
# dirs, GeneralsMD's per-tree GameEngine/Include (CriticalSection.h,
# GlobalData.h, GameDefines.h) and its Include/Precompiled (bare
# #include "PreRTS.h"), plus GeneralsMD's WW3D2 dir for header resolution
# only (Tests/RenderWW3DFrame's matching comment - no GeneralsMD .cpp beyond
# matrixmapper.cpp above is compiled into this target).
target_include_directories(RenderGameAssetsTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include/Precompiled
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

# StdBIGFileSystem::init() scans its cwd recursively for "*.big" (finding 2:
# loadBigFilesFromDirectory("", "*.big")) - ctest's WORKING_DIRECTORY covers
# this the same way Tests/GameFileSystem's does, so the engine's real init()
# path runs unmodified. Dedicated scratch dir so this harness's authored
# archive never collides with any other target's output.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rendergameassets_scratch)

add_test(NAME RenderGameAssetsTest COMMAND RenderGameAssetsTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rendergameassets_scratch)
