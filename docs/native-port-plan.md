# Native Linux/macOS Port Plan (no Wine, no DXVK-style translation)

## Goal

Build genuinely native Linux and macOS binaries of the game with no
Windows-compatibility layer involved at runtime (no Wine/Proton, no
DirectX-to-Vulkan translation shim). Win32/DirectX-specific source is
replaced with portable equivalents in-tree, following the pattern this
codebase already uses successfully for file I/O
(`Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFileSystem.cpp`,
which branches on `#ifndef _WIN32` inside one shared file rather than
keeping a separate Windows-only copy).

This is explicitly NOT the approach in upstream issue #2088 (DXVK
translates D3D8 calls to Vulkan at runtime, keeping the D3D8 call sites
unchanged). DXVK gets Linux binaries running faster, but is itself a
translation layer in the same spirit as Wine, and doesn't help macOS at
all (DXVK has no macOS backend). This plan targets removing the
dependency at the source level instead.

**Revision note**: this plan was reviewed by an independent pass that
verified every claim below against the actual codebase (file counts,
call graphs, build-system gating), not just read for prose soundness.
The first draft understated total scope by roughly 2-3x. This version
folds in what was actually found rather than presenting the smaller,
wrong estimate as fact.

## Current-state inventory (verified against this repo, corrected)

- **~56 game-relevant files** (of ~146 total under `Core/`; the
  remaining ~95 are `Core/Tools/`, out of scope) directly `#include`
  windows.h/d3d8.h/afxwin.h/afxext.h - but this undercounts real
  dependency badly, because it only counts direct includes:
  - `dx8wrapper.h` (which includes `d3d8.h`) is transitively included by
    **92 files** across Core+GeneralsMD.
  - **104 files** outside `Core/Tools/` call `DX8Wrapper::` methods.
  - **63 files** outside `Core/Tools/` reference raw D3D8 constants
    (`D3DRS_*`/`D3DTSS_*`/`D3DFMT_*`/`IDirect3D*`) directly, not just
    through the wrapper.
  - "dx8wrapper.cpp and ~15-18 siblings" (the original estimate) is off
    by roughly 4x for direct D3D8 call-site count alone.
- **`DX8Wrapper` is a pass-through, not an abstraction boundary.** Its
  public API takes raw D3D8 vocabulary (`Set_DX8_Render_State
  (D3DRENDERSTATETYPE, ...)`, `D3DFORMAT`, FVF codes), and the
  `DX8CALL` macro (`dx8wrapper.h:148-150`) lets any of the 104 calling
  files invoke `IDirect3DDevice8` methods directly, bypassing the
  wrapper entirely. This forces an explicit decision (see "Open
  decision" below) between two very differently-sized efforts.
- **W3DDevice is 131 .cpp files** across the three trees - comparable
  in size to the WW3D2 rewrite itself, and was almost entirely missing
  from the first draft. It includes the in-game GUI gadget rendering
  (`W3DPushButton`, `W3DListBox`, ...), shadows, water, roads, every
  drawable draw-module, radar, AND the GameLogic-side terrain/ghost
  object code (see determinism section - this part is sim-relevant,
  not just visual).
- **Shader binary assets, previously unmentioned - a real blocker**:
  `W3DShaderManager.cpp`, `W3DWater.cpp`, `W3DTreeBuffer.cpp` load
  compiled `.vso`/`.pso` D3D8 shader binaries from disk
  (`CreateVertexShader`/`CreatePixelShader`). These are game assets,
  not just code - any rendering-API change means every one of these
  needs to be re-authored for the new API, not just the C++ call sites.
- **Windowing/input is not a ~3-file surface.** The narrow
  `Core/GameEngineDevice/Source/Win32Device` directory (7 files) is
  real, but it's not the whole picture:
  - `GeneralsMD/Code/Main/WinMain.cpp` (1033 lines, per game tree):
    window class registration, `CreateWindow`, the message pump, and
    the `WndProc` that feeds `Win32Mouse` its events in the first
    place.
  - `Win32GameEngine` (device-factory object: hard-wires Miles, Win32
    file I/O, W3D client/logic together via `serviceWindowsOS()` and
    friends).
  - `W3DMouse` **subclasses** `Win32Mouse`
    (`Core/GameEngineDevice/Include/W3DDevice/GameClient/W3DMouse.h:62`)
    and does D3D cursor rendering - input isn't cleanly severable from
    rendering the way the first draft assumed.
  - File-I/O redundancy claim is CONFIRMED though: the game selects
    file systems via a factory in `Win32GameEngine.h:95-96`
    (`Win32LocalFileSystem`/`Win32BIGFileSystem`), nothing else in the
    game depends on the Win32 file path, and `StdLocalFileSystem`
    already handles Windows-path fixup. Swapping the factory really is
    all that's needed here - this part of the original plan was right.
- **Audio scope is bigger than "1 file" implied.**
  `MilesAudioManager.cpp` is 3268 lines with 101 `AIL_*` calls plus a
  direct `#include <dsound.h>`. Useful discovery: `FFmpegVideoPlayer.cpp`
  has a dead `#ifdef RTS_HAS_OPENAL` include of
  `OpenALAudioDevice/OpenALAudioManager.h` - a header that doesn't exist
  in this repo. This is very likely a reference to upstream
  GeneralsGameCode's own (more complete) OpenAL device that hasn't been
  pulled into this tree yet. **Phase 4 should start by checking whether
  upstream already has a working OpenAL backend to port in, rather than
  writing one from scratch.**
- **Video**: confirmed already solved - `VideoDevice/FFmpeg` exists
  alongside the Windows-only `Bink` backend, gated by
  `RTS_BUILD_OPTION_FFMPEG`.
- **Fonts**: GDI path is `render2dsentence.cpp`
  (`CreateFont`/`GetGlyphOutline`) plus the `W3DGameFont`/DisplayString
  layers above it. Scope is contained; FreeType+Fontconfig replacement
  is still the right call.
- **Networking - entirely missing from the first draft.** `GameNetwork`
  is 48+ .cpp files; Winsock usage in `udp.h/cpp`, `Transport`,
  `IPEnumeration`, `DownloadManager`, plus 19 GameSpy files including
  worker threads. Compiles into the game unconditionally. Winsock to
  BSD sockets is mostly mechanical, but it's a genuine missing phase.
- **Registry usage - entirely missing from the first draft.** Not just
  tools: `GameEngine/Source/Common/System/registry.cpp`,
  `UserPreferences.cpp`, `GameText.cpp`, `GlobalLanguage.cpp`, and even
  `DX8Wrapper`'s render-device settings
  (`VALUE_NAME_RENDER_DEVICE_*`, `dx8wrapper.h:64-69`) all read/write
  the Windows registry. Needs a config-file replacement layer.
- **Win32 timers/threads - present in the file count, never called out
  as a work item.** 51 non-tools files use `timeGetTime`/
  `GetTickCount`; WWLib's `thread.cpp`/`mutex.cpp`/`systimer.h`/
  `cpudetect.cpp` are Win32-based.
- **COM/ATL dependency, previously unmentioned**: the game (not just
  tools) uses ATL for the embedded WOL browser -
  `Win32GameEngine.h:111`, `NEW CComObject<W3DWebBrowser>`. Small in
  file count, but a real dependency needing a stub-or-remove decision.
- **Tools** (WorldBuilder, GUIEdit, W3DView, ImagePacker,
  MapCacheBuilder): MFC-based, confirmed genuinely out of scope - the
  game executable doesn't link MFC, and tools are separate build
  targets tracked by upstream issue #642.

## Open decision: target graphics API

D3D8 is a fixed-function-plus-early-shader era API. This decision now
has three options, and picking between them changes the effort
estimate by an integer factor - it must be made explicitly, not left
implicit:

- **(a) Keep DX8Wrapper's D3D8 vocabulary as the portable interface,
  reimplement only its backend.** Minimal call-site churn (the ~104
  `DX8Wrapper::` callers stay as-is), but this is functionally an
  in-tree D3D8-subset reimplementation - philosophically close to the
  DXVK approach this plan otherwise rejects, just shipped as source
  instead of a runtime shim. Note `cmake/dx8.cmake` already fetches a
  header-only `min-dx8-sdk`, so the D3D8 *types* already compile
  without a real Windows SDK - a hint this path may be what upstream
  half-expects.
- **(b) Replace the vocabulary itself** (direct OpenGL 3.3+, or an
  abstraction layer like bgfx/SDL_gpu) - touches 100+ call sites across
  WW3D2 and W3DDevice, not 15-18. Direct OpenGL has a real long-term
  risk on macOS (deprecated, capped at 4.1). bgfx/SDL_gpu already
  target Vulkan+Metal+GL under one API, matching the working macOS
  Metal-shim reference in upstream discussion #2886, at the cost of a
  third-party dependency and an extra indirection layer.

**Recommendation for review, revised**: option (a) first as a bridging
step - it's the only option that doesn't also require re-authoring
every shipped `.vso`/`.pso` shader binary immediately - with option (b)
as the real end state once (a) proves the engine runs natively at all.
Doing (b) directly is not wrong, but should be sized as the 100+
call-site, shader-reauthoring effort it actually is, not the 15-18 file
effort the first draft assumed.

**The prototype step needs to change too.** A single triangle or splash
screen does not exercise the real risk. The real risks are (1) fixed-
function texture-stage-combiner emulation (`D3DTSS_*` cascades - no
direct equivalent in a modern shader-only API, needs a shader-
permutation system), and (2) the shader-binary-asset problem above. A
better spike: get `render2d`/UI quad rendering plus one
ShaderClass-driven textured mesh through the candidate approach, not a
bare triangle.

## Phased plan (revised order and scope)

**Phase 0 - Full inventory, extended.** In addition to the original
scope (GDI/font check, MilesAudioDevice check - both now done above),
also produce a real file-by-file list for W3DDevice's 131 files,
networking's 48+ files, and the registry/timer/thread call sites -
these are not small enough to estimate from a grep count the way the
smaller subsystems are.

**Phase 1 (moved earlier from "Phase 6") - Build system, minimum
viable slice.** This must land before any prototyping can happen at
all: `CMakeLists.txt` currently includes `dx8.cmake`/`miles.cmake`/
`bink.cmake` only under `(WIN32 ...) AND CMAKE_SIZEOF_VOID_P EQUAL 4`,
but `Core/GameEngineDevice/CMakeLists.txt:229-234` links
`d3d8lib`/`milesstub`/`binkstub` **unconditionally**, and no game
target CMakeLists gates on `WIN32` at all. Configure fails on Linux/
macOS today before a single line of porting code runs. This phase is:
gate the Windows-only link dependencies and device-layer sources behind
`if(WIN32)`, add a minimal `if(NOT WIN32)` stub path so the game
target at least configures, and add real (not 32-bit-only) Linux/macOS
CMake presets.

**Phase 2 - 64-bit, promoted from "non-goal" to prerequisite (macOS
only, strongly recommended for Linux too).** Every game-capable preset
today is 32-bit (`win32`, `mingw-w64-i686`, "Unix 32bit"), and the root
CMakeLists hard-gates dx8/miles/bink on `CMAKE_SIZEOF_VOID_P EQUAL 4`.
macOS has had no 32-bit userland since 10.15 (2019) - a native macOS
build is impossible without this, full stop. 32-bit Linux userland
(SDL2:i386, 32-bit vcpkg deps) is a practical dead end too. This isn't
"VC6 removal" (modern MSVC/MinGW already work); it's specifically the
32-bit-only assumption baked into the build. Do this before Phase 3,
not deferred indefinitely.

**Phase 3 - Decide and prototype the graphics API approach** (per the
revised "Open decision" above), using the render2d+textured-mesh spike,
not a bare triangle.

**Phase 4 - Windowing + input**, now correctly scoped to include
`WinMain.cpp`'s window/message-pump code and `Win32GameEngine`'s
device-factory wiring, not just the narrow `Win32Device` directory.
`W3DMouse`'s D3D-cursor coupling means this phase has a real, if small,
dependency on Phase 3 being at least partially working - it is not as
cleanly independent as the first draft assumed. The engine's existing
headless-mode pattern (`TheGlobalData->m_headless` gating dummy
managers, `GameEngine.cpp:560-654`, `Win32GameEngine.h:97-117`'s dummy-
factory pattern) is a real, reusable template for a stub display device
here.

**Phase 5 - Rendering (the big one)**: reimplement the D3D8 call sites
per the Phase 3 decision. Sub-milestones: (a) device init + basic mesh
rendering, (b) texture + material pipeline, (c) shader/mapper pipeline
- this is where finishing more of #555's remaining WW3D2 per-tree
residue pays off directly, since every file already unified into
`Core/` only needs this rewrite once instead of twice - (d) particle/
dazzle effects, (e) W3DDevice's 131-file surface, including its
GameLogic-side terrain/ghost-object code (see Phase 8).

**Phase 6 - Audio**: check for upstream's existing OpenAL device first
(see inventory note above) before writing one from scratch; port/adapt
rather than reimplement if it exists and is usable.

**Phase 7 - Fonts, Networking, Registry, Timers/Threads, COM/ATL
stub.** Grouped together because each is independently well-understood
and mostly mechanical (FreeType+Fontconfig; Winsock→BSD sockets;
registry→config file; Win32 timer/thread APIs→POSIX equivalents; ATL
browser→stub-or-remove decision), unlike rendering. Can run partly in
parallel with Phase 5 once Phase 0's extended networking inventory
exists, since none of these blocks or is blocked by the graphics work.

**Phase 8 - Determinism validation, broadened.** The original three
flagged files (`animobj.cpp`, `motchan.cpp`, `meshgeometry.cpp`) are
real but not the whole picture. Bone-hierarchy transforms
(`htree.cpp`) belong on this list too - GameLogic modules
(`Object.cpp`, `ChinookAIUpdate.cpp`, `DeliverPayloadAIUpdate.cpp`,
`BoneFXUpdate.cpp`, `ParachuteContain.cpp`, `TransportContain.cpp`,
`MobNexusContain.cpp`, `DockUpdate.cpp`,
`RailedTransportDockUpdate.cpp`, `TransitionDamageFX.cpp`) call
`getPristineBonePositions`/`convertBonePosToWorldPos` directly. More
significantly: **pathfinding height queries are implemented in the
rendering device layer** - `W3DTerrainLogic::getGroundHeight`/
`getLayerHeight` (`W3DTerrainLogic.cpp:258,285`) back the pathfinder's
height data, and `W3DGhostObject` (GameLogic-side state) also lives
under W3DDevice. A W3DDevice rewrite is not "just visuals" - treat
"bone-transform paths" and "terrain-height/ghost-object paths" as
categories requiring save/replay-matching validation against the
existing Windows build, not just the three originally-named files.

## Explicit non-goals (unchanged from first draft, still sound)

- Porting WorldBuilder/GUIEdit/other MFC tools (separate effort, #642)
  - confirmed sound: the game doesn't link MFC, tools are separate
    targets.
- Solving Wine/Proton compatibility for players who prefer that route -
  this plan is about removing the *need* for it, not breaking it.

## Biggest risks (revised)

1. Total scope is genuinely large - WW3D2 (100+ D3D8 call sites) +
   W3DDevice (131 files, partially sim-critical) + shader-asset
   re-authoring + networking + registry + timers/threads + a COM/ATL
   stub decision + a hard 64-bit prerequisite for macOS. This is a
   multi-person, multi-month effort at minimum; Phase-based structure
   makes it incremental and testable, but should not be sold as smaller
   than it is.
2. The Phase 3 API decision is still un-prototyped; recommending option
   (a) as a bridging step reduces but does not eliminate this risk.
3. Determinism risk is broader than three files - bone-transform and
   terrain-height/pathfinding paths specifically need explicit
   save/replay-matching tests, not just visual QA, because they're
   reachable from GameLogic today.
4. 64-bit is a hard macOS blocker, not a nice-to-have - if this phase
   slips, macOS is not reachable at all regardless of progress
   elsewhere.

## Review history

- Draft 1: initial scope based on a targeted but incomplete grep-level
  inventory.
- Draft 2 (this version): revised after an independent verification
  pass that checked file counts, call graphs (`DX8Wrapper::` callers,
  bone-position callers), and build-system gating directly against the
  repo. Corrected: inventory undercounted ~2-3x, W3DDevice was nearly
  absent, build-system and 64-bit work were mis-sequenced, shader
  assets and several whole subsystems (networking, registry, timers,
  COM/ATL) were missing outright.
