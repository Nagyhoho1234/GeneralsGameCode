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

## Phase 0 results (completed - file-by-file, not sampled)

Five parallel research passes covered W3DDevice (131 files, split into
Drawable/Draw, GameClient/GUI+root, and GameLogic), GameNetwork's
non-GameSpy transport layer, and GameSpy+registry+timers/threads. Full
per-file tables live in the session history; this section is the
distilled result, including several corrections to the estimates above.

**W3DDevice breakdown (131 files total across the three trees):**

- `GameClient/Drawable/Draw/` - 18 files, 9,444 lines. **Zero direct
  D3D8 dependency across the board** - every file calls the WW3D2
  abstraction layer (`RenderObjClass`/`HLodClass`/etc.), never
  `DX8Wrapper`/raw D3D8 types directly. This entire slice needs no
  rewrite for the graphics-API transition; the rewrite burden is one
  layer down, in WW3D2 itself. 5 of 18 (`W3DModelDraw`, `W3DSupplyDraw`,
  `W3DDebrisDraw`, `W3DLaserDraw`, `W3DRopeDraw`) are sim-reachable, but
  only through narrow, confirmed one-way interfaces (GameLogic writes
  visual-feedback parameters in; nothing reads render state back out) -
  lower risk than the terrain-height/bone-transform paths below.
- `GameClient/` (excluding Drawable) + `GUI/Gadget/` + `GUI/GUICallbacks/`
  + `Water/` - 35 files, 36,680 lines. 16 of 35 are D3D8-dependent
  (`BaseHeightMap`, `HeightMap`, `FlatHeightMap`, `TerrainTex`,
  `W3DShaderManager` (706 call sites - the single densest file found),
  `W3DWater` (179), `W3DTreeBuffer`, `W3DSnow`, `W3DSmudge`,
  `W3DTerrainBackground`, `W3DWaterTracks`, `W3DProfilerFrameCapture`,
  plus 4 lighter ones). **19 of 35 have zero direct D3D8 touch**,
  including all 11 GUI gadget files and `W3DControlBar.cpp` (HUD/
  command-bar) - cheap porting candidates. **Correction**: the GUI
  "gadgets" (`W3DPushButton` etc.) aren't classes, they're pairs of free
  draw-callback functions assigned as `GameWindow` function pointers -
  if anything easier to port than a class hierarchy would be. **0 of 35
  files in this subtree are sim-reachable** - clean presentation-only
  layer.
- `GameLogic/` (sibling to `GameClient/`, both under `W3DDevice/`) -
  **path correction: this directory does not exist under `Core/` at
  all.** It's duplicated per-tree at `Generals/Code/GameEngineDevice/
  Source/W3DDevice/GameLogic/` and `GeneralsMD/Code/GameEngineDevice/
  Source/W3DDevice/GameLogic/`, and the two copies have diverged (not
  byte-identical). 3 files each, 6 total, ~1,670 lines/tree:
  - `W3DTerrainLogic.cpp` (385 lines) - **confirmed genuinely
    lockstep-determinism-critical**, not just plausible risk.
    `getGroundHeight`/`getLayerHeight`/`isCliffCell`/
    `getMaximumPathfindExtent` are called from 15+ sites in
    `AIPathfind.cpp` via the `TheTerrainLogic` global; this class is the
    *only* `TerrainLogic` subclass in the repo, so every pathfinding
    height query in the game resolves here. Zero direct D3D8 calls
    itself, but reads through the render-layer heightmap object to
    answer queries.
  - `W3DGhostObject.cpp` (~1,250 lines) - sim-*adjacent*, but a
    **different risk category than terrain logic**: its `xfer()`
    persists fogged-object render-state into save files (save/load
    correctness risk), and both its own and its base class's `crc()`
    are no-ops - **this data is explicitly excluded from the
    multiplayer lockstep CRC check**. Deeply coupled to the mid-level
    WW3D2 render-object API (`Clone()`, `Set_Transform()`,
    `Set_Animation()`), so a rendering rewrite will need real, careful
    work here, but validate it against save/load compatibility, not
    network desync.
  - `W3DGameLogic.cpp` (33 lines) - negligible; the .cpp is just a
    license header, all content is two one-line factory methods in the
    header. Not a real risk surface.
  - Practical consequence: any determinism fix here has to be applied
    and re-verified **twice** (once per tree) unless this directory is
    unified into `Core/` first - a natural, small #555 side-quest.
- `Common/` subdirectory of `W3DDevice/` was not covered by this pass -
  the 131-file total doesn't fully reconcile from the pieces above
  (53 + 6 = 59), so there's a remaining ~70 files (`Common/`, plus
  likely `Drawable/`'s own per-tree pieces if any exist) still
  uninventoried. Flagging honestly rather than presenting the picture
  as complete.

**GameNetwork transport layer (non-GameSpy) - smaller and more
contained than the original estimate:**

31 logical files (~20,750 LOC) outside GameSpy. **The true socket
boundary is `udp.cpp`/`udp.h` (534 lines) alone**, and it is **already
partially cross-platform** - it has an existing `#ifdef _WIN32 ...
#else // UNIX` split for includes, `SetBlocking()`, and error-code
mapping, with the core socket calls (`socket`/`bind`/`sendto`/
`recvfrom`/`setsockopt`) already written in POSIX-compatible form. Only
7 of 31 files make direct Winsock calls at all
(`DownloadManager.cpp`, `FirewallHelper.cpp`, `IPEnumeration.cpp`,
`NAT.cpp`, `NetworkUtil.cpp`, `Transport.cpp`, `udp.cpp`), and most of
those are either `WSAStartup`/`WSACleanup`/`WSAGetLastError` init
boilerplate (duplicated across 3 files independently - a minor
cleanup, not a portability blocker) or `htons`/`htonl`/`gethostbyname`-
family calls that have **identical signatures on POSIX**, just a
different header. The other 24 files are pure session/logic/
serialization code with zero direct socket calls. **Verdict: this is
closer to "finish an existing partial port and consolidate duplicate
init calls" than "port Winsock to BSD sockets from scratch."** Two
files were mis-filed under "networking" by directory location only and
should move categories: `WOLBrowser/WebBrowser.cpp` (confirmed pure
COM/ATL, no networking - matches the separately-flagged ATL dependency)
and `GUIUtil.cpp` (pure lobby-GUI population code, no networking).

**GameSpy - 20 unique files (not 19), and itself a #555 duplication
gap:** 12 files in `Core/`, but 5 more Generals-only and 3 more
GeneralsMD-only .cpp files exist - GameSpy hasn't been unified into
`Core/` yet either, so (like `W3DDevice/GameLogic`) this work may need
doing twice unless unified first. Threading is more extensive than the
plan's "GameResultsThread, PingThread" implied: **5 distinct classes**
(`BuddyThreadClass`, `GameResultsThreadClass`, `PeerThreadClass`,
`PSThreadClass`, `PingThreadClass`) all subclass WWLib's `ThreadClass`/
`MutexClass`, so GameSpy's own threading work is entirely gated on
WWLib's thread abstraction actually being ported (see timers/threads
below) rather than being separate work. Most files are pure logic/UI
with zero Windows dependency. **Two real, non-mechanical exceptions**
worth flagging specifically since they don't fit the "swap the API"
pattern: `MainMenuUtils.cpp` spawns a raw `CreateThread` (bypasses
WWLib entirely, needs hand conversion) for async DNS resolution; and
`StagingRoomGameInfo.cpp` does local-IP enumeration via a legacy
SNMP DLL walk (`LoadLibrary("SNMPAPI.DLL")`) while `PingThread.cpp`
uses dynamically-loaded ICMP (`ICMP.DLL`) - both are genuinely
Windows-specific techniques with no POSIX equivalent, needing real
reimplementation (`getifaddrs()`, raw ICMP sockets), not a 1:1 swap.

**Registry - two separate wrapper layers, different complexity:**

1. `GameEngine/Source/Common/System/registry.cpp` (202 lines,
   duplicated per-tree, near-identical) - genuinely simple: 4
   primitives (get/set string/int), no file associations, no DLL
   registration, no COM. The three files the original draft named
   (`UserPreferences.cpp`, `GameText.cpp`, `GlobalLanguage.cpp`) each
   make **exactly one call**, all to the same read-only
   `GetRegistryLanguage()` convenience wrapper, for locale/language
   selection - trivial once `registry.cpp` itself is rewritten to a
   config file (and that file should be unified into `Core/` first,
   same rationale as GameSpy/`W3DDevice/GameLogic` above).
2. WWLib's `RegistryClass` (`Core/Libraries/Source/WWVegas/WWLib/
   registry.cpp`, 734 lines) - a separate, richer wrapper (binary
   blobs, value enumeration, bulk tree import/export), used by
   `dx8wrapper.cpp` (render-device settings) and `WWAudio.cpp` (3 call
   sites). Good news: its bulk tree-import/export operations are
   **dead code** (defined, never called) - the real porting surface is
   just scalar/binary get/set, mechanical but with more API surface
   than the simpler wrapper above.

**Timers/threads (WWLib) - existing portability scaffolding is a mix
of genuinely-done and silently-stubbed, which matters a lot for
effort estimation:**

- **Already done, no work needed**: `FastCriticalSectionClass`
  (portable `std::atomic_flag` implementation already in place), the
  `TIMEGETTIME` macro's definition (correctly branches to
  `gettimeofday()` on non-Windows), and CPUID/RDTSC in `cpudetect.cpp`
  (already routed through a portable `intrin_compat.h` shim with GCC/
  clang builtins).
- **Scaffolded but non-functional - looks portable, isn't**:
  `thread.cpp`'s `ThreadClass::Execute()`/`Stop()` have `#ifdef _UNIX`
  branches that are literally `return;` - threads never actually start
  on non-Windows today. `mutex.cpp`'s `MutexClass` and
  `CriticalSectionClass` (not `FastCriticalSectionClass`, which is
  fine) have `_UNIX` branches that are `//assert(0)` stubs. All of
  GameSpy's 5 threaded classes depend on these being real.
- **A genuinely live, currently-broken bug, not just missing
  scaffolding**: `SysTimeClass::Get()` and `systimer.cpp`'s `Reset()`
  call the literal `timeGetTime()` symbol directly instead of the
  portable `TIMEGETTIME` macro. There's a preprocessor guard intended
  to redefine `timeGetTime` to the portable path, but it only fires if
  `timeGetTime` is already a macro - on both MSVC and MinGW it's a real
  winmm function declaration, so the guard **never triggers**. No
  non-Windows definition of `timeGetTime` exists anywhere in the tree.
  Corrected file count: **86 files** (not 51) call `timeGetTime`/
  `GetTickCount` directly across all three trees - every one needs
  sweeping onto `TIMEGETTIME()`, or a real `timeGetTime` shim needs to
  be added for non-Windows.
- **Unimplemented, honestly labeled as such already**:
  `cpudetect.cpp`'s `Init_Memory()`/`Init_OS()` are `#ifdef WIN32`-only
  with `#warning FIX Init_Memory()`/`#warning FIX Init_OS()` on the
  other branch - genuinely missing, not yet scaffolded, small but real
  work (`sysconf`/`/proc/meminfo`/`sysctl` equivalents).

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

## Unify-before-porting: an explicit strategy, not just a side-note

Phase 0 kept surfacing the same pattern: files that need native-port
rewriting AND are still duplicated per-tree rather than unified into
`Core/`. This is now treated as a formal sequencing rule for this plan,
not just an incidental observation:

**Rule: for any file a native-port phase is about to touch, check
whether it's still duplicated per-tree first, and unify it (via #555's
established process - diff, classify cosmetic-vs-genuine divergence,
move the reconciled version into `Core/`) *before* doing the portable
rewrite, not after.** Reconciling the OLD (D3D8/Win32) code is usually
cheap - Phase 0 found most per-tree pairs are cosmetic-only once
actually diffed. Doing the portable rewrite twice, on two copies that
would then independently diverge based on whichever choices each
rewrite pass happened to make, is strictly worse and harder to
reconcile later. This mirrors what was already true for WW3D2 (finishing
its remaining #555 residue pays off the Phase 5 rewrite twice-over) -
it just turned out to apply much more broadly than that one module.

**Confirmed overlapping candidates found so far** (each should be
unified as part of, or immediately before, the native-port phase that
touches it):

- `W3DDevice/GameLogic/` (3 files/tree, `Phase 5(e)`/`Phase 8`) -
  small, diverged; `W3DTerrainLogic.cpp` is also the confirmed
  lockstep-determinism-critical file, so this one is worth doing
  *especially* carefully and early.
- GameSpy (8 of 20 files per-tree-only, `Phase 6`/networking work) -
  small-to-moderate.
- `registry.cpp` (`Phase 7`) - trivial, 202 lines, near-identical
  already.
- The W3DDevice Shadow subsystem (`Phase 5(e)`) - the clearest case of
  this rule actually mattering: `W3DProjectedShadow.cpp` (moderate
  divergence, ~35 lines - a buffer-safety rewrite, not just cosmetic)
  and `W3DVolumetricShadow.cpp` (large divergence, 358 lines - an added
  CNC3/WWShade shader-mesh shadow-caster path GeneralsMD has that
  Generals doesn't, plus API signature changes) are BOTH genuinely
  divergent AND among the most D3D8-heavy files found in all of
  `W3DDevice` (98 and 105 D3D8-vocabulary hits respectively). Porting
  these to a new graphics API without unifying first would mean making
  the same D3D8→new-API judgment calls twice, on two already-different
  starting points, one of which has a feature the other lacks.

Not every duplicated file found in Phase 0 needs this treatment - the 8
of 11 Shadow-subsystem-adjacent classes that turned out to be cosmetic-
only, for instance, can just be unified in passing with near-zero
effort whenever convenient, same as `Drawable/Draw`'s and GUI/gadget's
already-confirmed-clean files. The rule matters most where divergence
is genuine AND the file is D3D8/Win32-heavy - that's where doing it
twice is expensive, not everywhere duplication exists.

## Biggest risks (revised again after Phase 0)

1. Total scope is still genuinely large, but less uniformly risky than
   first thought: WW3D2 (100+ D3D8 call sites, still the single biggest
   item) + the D3D8-dependent half of W3DDevice's GameClient tree (16
   of 35 files, plus the separately-scoped `Drawable/` slice which
   turned out to need **no** rewrite at all) + shader-asset
   re-authoring + a COM/ATL stub decision + a hard 64-bit prerequisite
   for macOS. Networking turned out meaningfully smaller than
   estimated (see below). Still a multi-person, multi-month effort at
   minimum - just not uniformly so across every subsystem.
2. The Phase 3 API decision is still un-prototyped; recommending option
   (a) as a bridging step reduces but does not eliminate this risk.
3. Determinism risk is now precisely scoped rather than broadly
   flagged: `W3DTerrainLogic.cpp`'s 4 pathfinding-height methods are
   *confirmed* lockstep-critical (real `AIPathfind.cpp` call sites,
   not just plausible risk), while `W3DGhostObject.cpp` is a save/load
   risk specifically excluded from the multiplayer CRC check - these
   are two different failure modes needing two different kinds of
   testing (network-desync matching vs. save-file compatibility), not
   one blanket "test determinism" bucket.
4. 64-bit is a hard macOS blocker, not a nice-to-have - if this phase
   slips, macOS is not reachable at all regardless of progress
   elsewhere.
5. **Downgraded risk, upgraded confidence**: networking (previously "a
   genuine missing phase, mostly mechanical") is now known to be a
   small, clean-boundary port centered on one 534-line file that's
   already half-ported, plus 3 specific non-mechanical items (raw
   `CreateThread` in `MainMenuUtils.cpp`, SNMP-based IP detection and
   dynamically-loaded ICMP in the GameSpy ping/staging-room code) that
   need real reimplementation rather than a mechanical Winsock swap.
6. **New finding, not previously visible**: WWLib's thread/mutex
   portability code looks done (has `#ifdef _UNIX` branches) but isn't
   - `ThreadClass`/`MutexClass`/`CriticalSectionClass`'s non-Windows
   paths are no-op stubs, and a broken preprocessor guard means literal
   `timeGetTime()` calls (86 files, not 51) silently have no
   non-Windows definition at all today. This is exactly the kind of
   thing that would cause confusing runtime failures (not compile
   failures) if not caught before Phase 4/6 work starts, since the code
   compiles and looks portable at a glance.

## Provenance and licensing of external reference material

Two external forks are referenced above as architectural evidence that
native ports are achievable (upstream discussion #2886 for macOS,
Fighter19's fork for Linux/DXVK per upstream issue #2088). Checked
directly before relying on either further:

- This project's own license is genuinely GPLv3 (EA released the
  original C&C Generals/Zero Hour source under it, with additional
  terms) - GitHub's "Other" classification is just because the
  additional terms don't cleanly match a standard SPDX tag, not because
  the license is actually unclear.
- `dvcdsys/GeneralsGameCode-macOS` is a fork of *this exact repository*
  - as a derivative work it is bound by the same GPLv3 terms regardless
  of what license file (if any) it carries itself. Legal risk in
  referencing it is therefore low. But the author's own discussion post
  states it plainly: "built with heavy use of AI coding tools ... not a
  hand-crafted monument" - self-described as informal, casually-tested
  work (verified in normal play, not held to a contribution-review bar).
- Fighter19's fork shows no detected license file (`NOASSERTION`) -
  almost certainly still GPLv3-bound as a derivative work, but this
  hasn't been independently confirmed the way the macOS fork's lineage
  was.

**Rule for implementation**: treat both forks as architectural/
approach references only - "this shape of change is achievable, here's
roughly what area of the engine it touches" - not as a source to copy
code from. Whoever implements each phase should independently derive
and verify the actual code against this repo's own structure and
conventions (the same discipline already used throughout this session:
verify claims against ground truth, don't trust a source - internal or
external - blindly), rather than porting logic across from either fork
wholesale.

## Readiness assessment

**Phase 0 is now substantially complete** for W3DDevice (except the
`Common/` subdirectory, ~70 files still uninventoried - see the note
above), GameNetwork's transport layer, GameSpy, registry, and
timers/threads. **Still not implementation-ready as a whole**, though -
one gate remains before Phase 5 (rendering) can start with real
confidence, and one smaller gate before Phase 0 can be called fully
closed:

1. The Phase 3 graphics-API decision is still a recommendation on
   paper, not a validated spike. Nothing past Phase 4 should start
   until that spike (render2d + one ShaderClass-driven textured mesh,
   not a bare triangle) has actually run. This is now the single
   biggest remaining unknown in the whole plan.
2. `W3DDevice/Common/` (~70 files, the piece of the 131-file total this
   pass didn't reach) should get the same file-by-file treatment before
   Phase 5(e) is scoped in detail - everything else in W3DDevice turned
   out to have real surprises (zero-D3D8 Drawable slice, diverged
   per-tree GameLogic, precisely-scoped-not-broadly-flagged determinism
   risk) worth not assuming away for the remaining fifth of the
   directory.

Phase 1 (build-system slice) remains concrete and ready to start now -
CMake-gating work with no open design questions blocking it. The
networking, GameSpy, registry, and timer/thread findings above are
detailed enough to start Phase 7's implementation work directly,
ahead of the rendering phases, since none of it blocks or is blocked
by the Phase 3 API decision.

## Review history

- Draft 1: initial scope based on a targeted but incomplete grep-level
  inventory.
- Draft 2: revised after an independent verification pass that checked
  file counts, call graphs (`DX8Wrapper::` callers, bone-position
  callers), and build-system gating directly against the repo.
  Corrected: inventory undercounted ~2-3x, W3DDevice was nearly absent,
  build-system and 64-bit work were mis-sequenced, shader assets and
  several whole subsystems (networking, registry, timers, COM/ATL)
  were missing outright. Added explicit provenance/licensing review of
  the two external forks referenced as evidence.
- Draft 3 (this version): folded in completed Phase 0 file-by-file
  inventories (5 parallel research passes) for W3DDevice, GameNetwork,
  GameSpy, registry, and timers/threads. Notable corrections: the
  `W3DDevice/GameLogic` path doesn't exist under `Core/` (duplicated
  and diverged per-tree instead); `Drawable/Draw`'s 18 files need zero
  rewrite (all D3D8 work is one layer down in WW3D2); networking is a
  small, mostly-already-half-ported job, not the genuine missing phase
  it looked like; WWLib's thread/mutex "portability" scaffolding is
  largely non-functional stubs despite compiling cleanly; and three
  areas (`W3DDevice/GameLogic`, GameSpy, `registry.cpp`) turned out to
  be their own un-unified #555 duplication gaps, found as a side effect
  of this inventory rather than gone looking for.
