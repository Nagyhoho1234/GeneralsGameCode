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
  - `GeneralsMD/Code/Main/WinMain.cpp` (1033 lines; Generals' copy is
    1006 lines, near-identical):
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
  pulled into this tree yet. **Phase 6 should start by checking whether
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

- `GameClient/Drawable/Draw/` - 19 files, 9,444 lines. **Zero direct
  D3D8 dependency across the board** - every file calls the WW3D2
  abstraction layer (`RenderObjClass`/`HLodClass`/etc.), never
  `DX8Wrapper`/raw D3D8 types directly. This entire slice needs no
  rewrite for the graphics-API transition; the rewrite burden is one
  layer down, in WW3D2 itself. 5 of 19 (`W3DModelDraw`, `W3DSupplyDraw`,
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
- `Common/` + `GameClient/` root remainder (the last 71 of 131 files,
  now fully inventoried in two follow-up passes) - **entirely
  duplicated per-tree, not unified into `Core/`**, except one file
  (`W3DRadar.cpp`). 13 "infrastructure" classes (factories, GUI window
  plumbing, the Shadow subsystem) plus 22 `GameClient/` root classes
  (asset manager, display, scene, road/bridge buffers, shroud, etc.) -
  35 unique classes/tree, ×2 trees + 1 unified `W3DRadar.cpp` = 71.
  Findings that change the plan's earlier assumptions:
  - **At least 14 of these 35 classes are genuinely divergent between
    trees** (3 of the infrastructure classes, 11 of the GameClient-root
    ones), not just cosmetic - and several carry real Zero-Hour-only
    gameplay features, not just engine drift: stealth-detection material
    passes
    and infantry-light scaling (`W3DScene.cpp`), a listening-outpost
    "reveals enemy paths" feature (`W3dWaypointBuffer.cpp`), heat-
    distortion/snow particle hooks (`W3DParticleSys.cpp`), localized-
    language asset paths (`W3DFileSystem.cpp`). **Unifying these is not
    a blind dedup** - it needs careful feature-preserving merges, unlike
    most of the cosmetic-only pairs found elsewhere in this inventory.
  - **Correction to an earlier assumption**: `W3DDisplay.cpp` does
    *not* do device creation/reset (`IDirect3D8::CreateDevice` and
    `D3DPRESENT_PARAMETERS` handling both live one layer down, in
    `dx8wrapper.cpp:551,574`, confirmed by direct read). `W3DDisplay.cpp`
    owns resolution/bit-depth fallback policy and debug-stat plumbing
    instead - real work, but a different kind than "the device-init
    file" implied.
  - **The COM/ATL dependency is deeper than one file**: `W3DWebBrowser.cpp`
    itself uses ATL types, but its base class `WebBrowser` (in `Core/`)
    is where `<atlbase.h>`, the `FEBDispatch` ATL template, and the
    global `CComObject<WebBrowser>` instance actually live - the
    stub-or-remove decision has to handle that whole hierarchy, not
    just the 78-line subclass.
  - **Sim-reachability corrections, both directions**: `W3DShroud.cpp`
    is confirmed sim-reachable (fog-of-war - `PartitionManager.cpp`
    calls `TheDisplay->setShroudLevel()` on every per-player cell
    shroud change, one-way). `W3DScene.cpp` turned out *not*
    sim-reachable despite being a plausible-looking candidate (zero
    hits anywhere in GameLogic). New finding: `W3DInGameUI.cpp` is
    called 59+ times from GameLogic (mostly one-way UI feedback; one
    read-back for veterancy-promotion UI refresh, confirmed not
    outcome-affecting, so not a determinism risk, but worth knowing
    about before a rewrite touches it).
  - This closes the `W3DDevice` inventory completely - all 131 files
    across all three trees are now accounted for.

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

**GameSpy - 19 unique logical files (22 total file paths), and itself a
#555 duplication gap:** 14 .cpp in `Core/`, plus 5 Generals-only and 3
GeneralsMD-only .cpp files (the 3 GeneralsMD-only names are a subset of
the 5 Generals-only names, so 8 per-tree-only file *paths* but 5 unique
per-tree-only logical files) - GameSpy hasn't been unified into `Core/`
yet either, so (like `W3DDevice/GameLogic`) this work may need doing
twice unless unified first. Threading is more extensive than the
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

**Registry - three separate wrapper layers, different complexity** (a
holistic review pass found a third one this section originally missed):

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
3. `Core/Libraries/Source/WWVegas/WWDownload/registry.cpp` (172 lines) -
   a third, separate implementation, compiled into both game targets
   and serving the `DownloadManager`/patch-download path this plan's
   networking section already covers. Not yet characterized in detail;
   flag for the same config-file treatment as the other two rather than
   assuming it's identical in scope to either.

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

## Open decision: target graphics API (desk-checked, no longer purely
speculative)

D3D8 is a fixed-function-plus-early-shader era API. The decision
between two options still needs a real prototype to fully close, but a
desk-check (reading `shader.cpp`/`mapper.cpp`/`W3DShaderManager.cpp`
in full, characterizing the actual shipped shader assets, and checking
what upstream itself is already doing) resolved several things that
were previously just plausibility arguments:

- **(a) Keep DX8Wrapper's D3D8 vocabulary as the portable interface,
  reimplement only its backend.** The ~104 `DX8Wrapper::` callers stay
  as-is. `dx8wrapper.h` already internally state-caches everything into
  tables (`RenderStates[]`, `TextureStageStates[8][32]`) - it is
  already a state-table abstraction that happens to use D3D8 enum
  names, not a thin pass-through in practice, which makes swapping the
  backend more mechanical than "in-tree D3D8-subset reimplementation"
  made it sound.
- **(b) Replace the vocabulary itself** (OpenGL 3.3+, or bgfx/SDL_gpu).

**The "100+ call sites" framing overstated how different these options
actually are.** The desk-check found the whole texture-stage-combiner
system - the thing that made (b) sound like a much bigger job - is
small and centralized: it lives in one function,
`ShaderClass::Apply()` (`shader.cpp:409-1044`), drives only 2 real
texture stages, and the entire game only defines ~40 named
configurations total (22 built-in presets + ~17 game-defined
`SHADE_CNST` constants). Mapper classes (~20 of them) are even
simpler - one virtual function, 4 texture-coordinate-generation modes,
a matrix uniform. A single small GLSL "ubershader" (or a permutation
cache keyed on the existing 32-bit `ShaderBits` word, mirroring code
that already exists at `shader.cpp:421`) covers the whole system under
either option. Most of the "100+ call sites" set the *same* bounded
vocabulary repeatedly, not genuinely distinct configurations - so (b)
is not the much-larger effort it looked like, and (a) is not
meaningfully "safer" on this specific axis.

**The shader-binary-asset item from the previous draft was wrong and
is downgraded from blocker to minor task.** All 13 shipped `.vso`/`.pso`
binaries (plus 3 tiny inline-assembled ps.1.1 shaders in `W3DWater.cpp`)
have their assembly **source already shipping in-tree, GPL-licensed**,
at `GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/GameClient/
Shaders/*.nvp/.nvv` (14 files, 571 lines total, terrain/road-noise/
B&W-filter/tree-card effects) plus the water shader source separately
at `Core/GameEngineDevice/Source/W3DDevice/GameClient/Water/wave.nvp`/
`wave.nvv`. Re-authoring
the complete set in GLSL is a days-scale task, not a project-blocking
unknown, under either option (a) or (b). Separately, `W3DShaderManager.cpp`'s
706 D3D8 call sites (previously flagged as the densest file found) are
mostly per-chipset fallback lists (Voodoo3 support, 8-stage/2-stage
NVidia-only paths, hardware-capability benchmarking) that a modern-GPU
port deletes outright - the real surface is ~6 effect families plus 4
screen filters, not 706 distinct things to port.

**Upstream is already building option (a), which changes this from a
theoretical recommendation to "follow the maintainers' actual
direction."** Live upstream discussion #1575 ("Migrate to DX9") has
maintainer bobtista actively implementing exactly this architecture on
branch `bobtista/feat/render-backend-interface` - a render-backend
interface with DX8 as the default passthrough, and bgfx as the first
alternative backend, in progress. Maintainer xezon's stated constraint
is not to break DX8 while adding backends; maintainer Mauller's stated
position is to **unify Generals/ZH DX8 code first** - independently
arriving at the same "unify before porting" rule this plan formalized
above, before knowing that discussion existed. `cmake/dx8.cmake`
already fetching TheSuperHackers' own header-only `min-dx8-sdk` is
consistent with this being the intended direction, not a coincidence.
A second architectural reference was also found there: fbraz3's
"GeneralsX" fork (SDL3+DXVK+OpenAL, reported working) - subject to the
same reference-only, don't-copy-code rule as the macOS fork already
covered in the provenance section.

**Recommendation, now evidence-backed rather than a plausibility
argument**: option (a) first - not primarily because option (b)'s
shader work is too large (the desk-check shows it isn't), but because
(a) is mechanically verifiable file-by-file under an unchanged, already
state-cached vocabulary, and because it is the path upstream is
independently already moving toward, which maximizes the chance this
work is mergeable rather than a permanently-divergent fork. `shader.cpp`/
`mapper.cpp` being per-tree duplicated (confirmed, not yet in `Core/`)
means unifying WW3D2's remaining #555 residue is a genuine prerequisite
for doing this work once instead of twice, not just a nice-to-have.

**What the desk-check could not resolve - still needs the real
prototype**: pixel-exact combiner-math and alpha-test-reference
semantics matching, vertex/index buffer lock/unlock and render-target
lifecycle behavior, D3D-vs-GL clip-space/texture-matrix/half-texel
conventions, and whether an ubershader vs. permutation-cache approach
performs acceptably (near-certain non-issue on modern hardware, but
unverified). **The spike design from Draft 2 is still correct and
should not change**: `render2d`/UI quad rendering plus one
ShaderClass-driven textured mesh, not a bare triangle - that combination
is exactly where the desk-check says the real remaining risk (resource/
semantics matching, not combiner emulation) actually lives.

## Phased plan (revised order and scope)

**Phase 0 - Full inventory, extended.** In addition to the original
scope (GDI/font check, MilesAudioDevice check - both now done above),
also produce a real file-by-file list for W3DDevice's 131 files,
networking's 48+ files, and the registry/timer/thread call sites -
these are not small enough to estimate from a grep count the way the
smaller subsystems are.

**Phase 1 (moved earlier from "Phase 6") - Build system. In progress;
scope was corrected empirically, not just re-estimated.** The original
framing here - "`Core/GameEngineDevice/CMakeLists.txt:229-234` links
`d3d8lib`/`milesstub`/`binkstub` unconditionally... Configure fails on
Linux/macOS today before a single line of porting code runs" - was
wrong on the specific mechanism, discovered by actually running
`cmake -S . -B ... -G Ninja` on Linux (WSL2 Ubuntu 26.04, no vcpkg/
preset) rather than reasoning from the source alone: **CONFIGURE
already succeeds today**, unchanged. CMake's `target_link_libraries`
does not error at configure time for a plain library name like
`d3d8lib` when no CMake target by that name exists - it only fails at
link time. The real blocker is at BUILD time, and starts far earlier
than GameEngineDevice: even `WWLib`/`WWMath` (linked by essentially
everything) didn't compile on Linux.

Fixed so far (commit `1ad028e5e`): `cmake/config-build.cmake` already
unconditionally defines `_UNIX` on UNIX (prior upstream work, `e53e4d266`
"Fix Linux compilation of WWLib (#698)"), which activates
`#ifdef _UNIX / #include "osdep.h"` blocks in `WWMath`'s
`vector3.h`/`matrix3d.h` and `WWSaveLoad`'s `pointerremap.h` - but
`osdep.h` was never added. Confirmed via full git history (present
since the initial 2003 EA source commit) this header has never existed
in any public release of this codebase - it almost certainly referred
to an internal Westwood build-system header shared across their
W3D-engine titles, never part of the Generals/Zero Hour release. Added
as an empty, documented stub (confirmed empirically neither file
actually uses a symbol from it). Also fixed, once this stopped masking
them: an unconditional `<windows.h>` in `WWLib`'s precompiled-header
list; an unconditional, unused `<windows.h>` in `WWSaveLoad/saveload.cpp`;
and `WWLib/stringex.h`'s `strlcpy`/`strlcat`/`wcslcpy`/`wcslcat`
fallbacks conflicting with glibc 2.38+'s own versions (the `#ifndef
HAVE_STRLCPY`-style guards were already there, clearly designed for
exactly this, but nothing defined the macros; wired up
`check_symbol_exists` detection; also found a second unguarded
declaration block the original guard didn't cover).

**Second pass fixed** (commit `9e1b76094`), gating the actual
"device-layer sources" this phase originally named: `Core/GameEngineDevice`
and the Generals/GeneralsMD per-tree residue (entire W3DDevice/Win32Device/
MilesAudioDevice/VideoDevice source lists, `binkstub`/`d3d8lib`/`milesstub`
linking - the original specific blocker), `Core/Libraries/WW3D2` (100+
D3D8 call sites, gated wholesale), two D3D8-conversion functions found
mixed into otherwise-portable `WWMath` (`matrix3d`/`matrix4`'s
`To_D3DMATRIX`/`To_D3DXMATRIX`, verified their only callers are
themselves Windows-only before gating), the whole `WWDownload` subsystem
(Winsock/`process.h`/`HKEY` - `Download.h` and `urlBuilder.cpp` both
depend on `ftp.h`'s `FTPClass` directly, no clean portable subset without
untangling it, which is Phase 7 work), `WWLib`'s `DbgHelpLoader`
(Windows crash-symbol resolution, already dead weight when
`RTS_ENABLE_CRASHDUMP` is off by default but still compiled
unconditionally), and `PreRTS.h`'s huge Windows-only include block
(`atlbase.h`, `mmsystem.h`, `objbase.h`, `shellapi.h`, ...) split from
the portable standard-C-library includes mixed into the same file. Also
fixed two incomplete portability shims this surfaced (a missing
`_stricmp`->`strcasecmp` alias, a missing `<wchar.h>` include) and
switched `g_wwdownload`/`z_wwdownload`/`g_ww3d2`/`z_ww3d2` to `INTERFACE`
libraries on non-Windows, since a `STATIC` library needs at least one
source file even when everything it links contributes zero.

**Third through eighth passes** (commits `ffd7fd552` through `2f63f1e53`)
worked through most of that GameEngine-core tail file by file, verifying
via WSL2 rebuilds after each fix and re-verifying both `g_gameenginedevice`/
`z_gameenginedevice` on Windows (0 errors) after each batch:
`registry.cpp` (portable stub - real callers keep working, callers fall
back to hardcoded defaults), `FrameRateLimit.cpp` (real
`std::chrono`-based reimplementation, not a stub, since this affects
actual gameplay frame pacing), `WorkerProcess`/`ReplaySimulation`
(Windows Job-Object process spawning gated out, falls back to
single-process replay-sim mode), `GlobalData.cpp`'s user-data-path and
exe-CRC lookups (real `$HOME/Documents` + `/proc/self/exe`/
`_NSGetExecutablePath` reimplementations, both trees separately since
they'd genuinely diverged), `IPEnumeration.cpp`/`Transport.cpp`/`udp.cpp`
(Winsock-to-BSD-sockets, mostly mechanical as this plan always expected -
`udp.cpp` turned out to already be mostly prepared for this by prior
authors), `CriticalSection.h` (`std::recursive_mutex`, both trees),
`GameStateMap.cpp`'s scratch-pad-map cleanup (`opendir`/`readdir`
in place of `FindFirstFile`), `ClientInstance.cpp` (`flock()`-based
instance-lock, real reimplementation), `CommandLine.cpp` (`stat()`;
`GetCommandLineA()` left as an explicit gap pending Phase 4's portable
entry point, not guessed at), `AIPathfind.cpp`'s stray `__fastcall`,
and a batch of CRT-name mismatches (`_isnan` fixed **centrally** in
`compat.h` once - 12+ call sites across the codebase used it directly -
rather than per-file; `GlobalAlloc`/`GetModuleFileName`/`__int64`+
`_rdtsc`/`_fpreset`).

**Net movement across this whole tail: 258 -> 48 unique remaining error
lines** (an ~81% reduction), all independently re-verified not to have
regressed the Windows build at each step. **Still remaining**: a
scattered set of smaller files (`Image.cpp`, `PartitionManager.cpp`,
`OpenContain.cpp`, `GameSpy`'s `PeerThread`/`PingThread`/
`GameResultsThread`/`BuddyThread.cpp`, `ThingTemplate.cpp`, `Player.cpp`,
`GameState.h`, `FirewallHelper.cpp`, `GlobalLanguage.cpp`, and about a
dozen single-error files), plus a few qualitatively different items not
attempted in this pass: a struct-size `static_assert` failure in
`LANAPI.h` that looks like a genuine 64-bit alignment issue (Phase 2
territory, not a simple gating fix - **correction below, Draft 11: this
diagnosis was wrong**), `atlbase.h` in `WebBrowser.h` (real COM/ATL
work, Phase 7), and a build error inside the vendored `gamespy-src`
third-party dependency itself (not this codebase's code). Real (not
32-bit-only) Linux/macOS CMake presets also still haven't been added.

**Ninth through fourteenth passes** (commits `f6acfaea0`, `9200ced5a`,
`580251222`, `0fe95e6b4`, `050e63670`, plus a fable-caught fix folded
into the second of those) picked the 48-line tail back up, working
through the `GameSpy` thread files properly this time - and one real
mistake happened mid-batch, corrected same-session: `PingThread.cpp`/
`GameResultsThread.cpp` were initially fixed by just re-gating their
`#include <winsock.h>` behind `_WIN32` with nothing on the other side,
which **regressed the Linux build from 44 to 117 errors** because both
files use Winsock APIs (`WSADATA`/`WSAStartup`/`HOSTENT`/
`gethostbyname`/`WSACleanup`, and in `GameResultsThread.cpp` also
`connect()`/`send()`/`closesocket()`/`WSAGetLastError()`) directly in
their bodies, not just via that one include. Properly fixed with the
same Winsock-to-BSD-sockets treatment as `IPEnumeration.cpp`/
`Transport.cpp` (POSIX socket headers, `errno`-based error handling
mirroring the `WSAGetLastError` paths); `PingThread.cpp`'s `doPing()` -
a ~230-line Windows-only ICMP.DLL ping implementation - got an honest
stub (`return -1`, "ping unknown") since a real port needs raw-socket
ICMP echo requiring elevated privileges, real non-mechanical work. A
fable review of that fix caught one more real gap: `getWSAErrorString()`
was gated on `DEBUG_LOGGING` only, not `_WIN32`, so a Linux **debug**
build (not the release-style config used for all Phase 1 verification
so far) would have failed on its ~50 WSA-named case labels; fixed with
a `strerror()`-backed equivalent for that build config. Also centralized
several more MSVC-name compat shims in `compat.h`/`time_compat.h`
following the `_isnan` precedent (`itoa` - base-10 only, matching every
call site; `__max`/`__min`; `_access`/`CreateDirectory`), fixed a real
typo bug unrelated to portability (`StdLocalFileSystem.cpp`'s
`normalizePath()` referenced an undeclared `unNormalized` where the
variable is named `nonNormalized` - dead code until this port made the
branch it lives in compile for the first time), and ported
`GameState.cpp`'s `iterateSaveFiles()` (both trees) from
`GetCurrentDirectory`+`FindFirstFile`+chdir to `opendir()` directly,
matching the `GameStateMap.cpp` precedent from the eighth pass.

**The biggest single fix of this batch: `time_compat.h` gained a
portable `SYSTEMTIME` struct (Windows-layout-compatible, since
`Recorder.cpp` binary-serializes it directly into replay files) and
`GetLocalTime()`.** `GameState.h` declares
`getUnicodeDateBuffer`/`getUnicodeTimeBuffer` taking a `SYSTEMTIME`, and
is `#include`d by nearly every GameEngine translation unit - so the
undeclared-`SYSTEMTIME` error was being reported once per translation
unit. This one header-level fix (plus a `strftime()`-based portable
fallback replacing `GameState.cpp`'s `GetVersionEx`/`GetDateFormat`/
`GetTimeFormatW` body, an honest simplification that can't reach
per-user Windows regional format overrides, only the process locale)
resolved roughly 90 of the error total by itself - the same
one-header-many-errors shape `osdep.h` had in the first pass.

**`KeyDefs.h`'s `#include <dinput.h>` was the single largest remaining
error contributor (121 of 153 lines at that point) and got the same
treatment**: the header only uses DirectInput's `DIK_*` scan-code
*values*, baked directly into the `KeyDefType` enum (`KEY_A = DIK_A`,
etc.) - no DirectInput COM API is touched anywhere in this file. The
~85 `DIK_*` values it actually references were reproduced directly
(standard PC keyboard scan set 1, matching the public DirectX SDK's
published values) rather than gated out or stubbed, since real
keyboard-input work later needs real scan codes, not placeholders.
Fixed in both `Generals/` and `GeneralsMD/`'s copies.

**Net movement, ninth-through-fourteenth passes: 48 -> 446 unique error
lines - not a regression.** Both `KeyDefs.h` and `GameState.h` are
extremely widely-included headers that were themselves the reason the
build died before reaching huge swaths of GUI (`GameClient/GUI/Gadget`,
`GameClient/GUI/GUICallbacks/Menus`) and `GameNetwork` code. Fixing them
let the build proceed **much** further than before, and the ~300 newly
visible errors are code that was previously unreachable, not new
breakage - confirmed by an unchanged 0-error Windows build after every
commit in this batch. See "Draft 11" below for the root-cause analysis
and fix plan for this newly-exposed layer.

**Phase 2 - 64-bit, promoted from "non-goal" to prerequisite (macOS
only, strongly recommended for Linux too). Closed for the currently-
portable subset (Core/GameEngine + Core/GameEngineDevice's non-D3D8/
non-Win32 sources, both Generals and GeneralsMD) - see "Draft 15"
below for full detail.** Originally: every game-capable preset was
32-bit (`win32`, `mingw-w64-i686`, "Unix 32bit"), and the root
CMakeLists hard-gates dx8/miles/bink on `CMAKE_SIZEOF_VOID_P EQUAL 4`
(though this specific gate turned out to be Windows-only and never
the actual Linux/macOS blocker - see Draft 15). macOS has had no
32-bit userland since 10.15 (2019) - a native macOS build is
impossible without this, full stop. 32-bit Linux userland (SDL2:i386,
32-bit vcpkg deps) is a practical dead end too. This isn't "VC6
removal" (modern MSVC/MinGW already work); it's specifically the
32-bit-only assumption baked into the build. **Resolution turned out
to be smaller than expected**: every session-long WSL2 Linux build
this whole port has already implicitly been a native 64-bit build
(WSL2 Ubuntu is x86_64), so nearly all of Phase 2's real substance
(pointer-width bugs) was already being caught and fixed as part of
Phase 1's batches without being separately labeled "Phase 2" - what
remained once Phase 1's tail closed was two genuine bugs (not pointer-
width shims), real CMake presets, and a wave of macOS-specific gaps
only real cross-compiler (Clang/libc++ vs GCC/libstdc++) CI could
catch.

**Phase 3 - Decide and prototype the graphics API approach** (per the
revised "Open decision" above), using the render2d+textured-mesh spike,
not a bare triangle. **Closed.** This spike (`native-port-spike/`, GL 3.3
core profile, standalone/not wired into the main build) has now run on
three independent GL implementations - Windows/NVIDIA, Linux/Mesa-
llvmpipe, and real macOS (Apple Software Renderer, GL 4.1 core,
`GL_VERSION: 4.1 APPLE-23.1.1`) - producing pixel-identical output on all
three (same ~2e-7 clip-space delta, same 114113/262144 non-background
pixel count), and the texture-origin V-flip is validated on all three
too. See "Readiness assessment" below for the full history, including a
GitHub CI limitation that was initially misdiagnosed as a platform dead
end and then correctly root-caused and fixed.

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
browser→stub-or-remove decision), unlike rendering. Phase 0's file-level
detail for all of these is now complete (see "Phase 0 results" above),
so this phase can start directly, in parallel with Phase 5, since none
of it blocks or is blocked by the graphics work.

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
- GameSpy - **re-investigated, this estimate was wrong.** Not a
  cosmetic-divergence merge like registry.cpp/shader.cpp: `Generals/`
  still actively builds an older per-file design (`GameSpy.cpp`,
  `GameSpyChat.cpp`, `GameSpyGameInfo.cpp`, `GameSpyGP.cpp`,
  `GameSpyPersistentStorage.cpp`), while `GeneralsMD/` has copies of
  some of these files sitting unused in its source tree - genuinely
  commented out of `GeneralsMD/Code/GameEngine/CMakeLists.txt` (verified:
  lines 521-523, 1089-1091) - because it already migrated to Core's
  newer, more comprehensive 19-file `GameNetwork/GameSpy/` subsystem
  instead. Real unification here means migrating Generals off its old
  implementation onto Core's, not diffing two per-tree copies - a
  materially bigger and riskier change (touching Generals' active
  networking code path) than "5 unique logical files, small-to-moderate"
  implied. Deferred pending a dedicated look, not attempted opportunistically.
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
- `shader.cpp`/`mapper.cpp` themselves (`Phase 3`/`Phase 5`) - confirmed
  during the graphics-API desk-check to still be per-tree duplicated,
  not in `Core/`. These are the files the whole Phase 3 spike and Phase
  5(c) shader/mapper-pipeline rewrite are centered on - unifying them is
  as close to a hard prerequisite as this rule produces anywhere in the
  plan, not just a nice-to-have.
- 11 of the 33 `W3DDevice/Common`+`GameClient`-root files (`Phase 5(e)`)
  - unlike most of this plan's other unify-before-porting candidates,
  several of these carry **real Zero-Hour-only gameplay features**
  (stealth-detection rendering, a listening-outpost path-reveal
  mechanic, smudge/snow particle hooks), not just engine-version drift.
  Unifying these needs careful feature-preserving merges - closer in
  spirit to the judgment-call adoption work already done for GUIEdit/
  WorldBuilder than to a mechanical dedup.

Not every duplicated file found in Phase 0 needs this treatment - the
majority of files across every W3DDevice subtree inventoried turned out
to be cosmetic-only once actually diffed (8 of 11 Shadow-subsystem-
adjacent classes, 11 of 22 `GameClient`-root classes, most of GameSpy)
and can just be unified in passing with near-zero effort whenever
convenient. The rule matters most where divergence is genuine AND the
file is either D3D8/Win32-heavy or carries real per-game feature work -
that's where doing it twice is expensive, not everywhere duplication
exists.

## Biggest risks (revised again after Phase 0)

1. Total scope is still genuinely large, but less uniformly risky than
   first thought: WW3D2 (100+ D3D8 call sites, still the single biggest
   item) + the D3D8-dependent half of W3DDevice's GameClient tree (16
   of 35 files, plus the separately-scoped `Drawable/` slice which
   turned out to need **no** rewrite at all) + the Common/GameClient-root
   remainder's D3D8-heaviest files (`W3DVolumetricShadow.cpp` 105 hits,
   `W3DProjectedShadow.cpp` 98, `W3DDisplay.cpp` 60 - this last group
   was under-weighted in an earlier draft of this risk item and is now
   included) + shader-asset re-authoring (downgraded to days-scale, see
   above) + a COM/ATL stub decision (now known to span a shared `Core/`
   base class, not one file) + a hard 64-bit prerequisite for macOS.
   Networking turned out meaningfully smaller than estimated (see
   below). Still a multi-person, multi-month effort at
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

**Phase 0 is now fully complete** - every file across all 131
`W3DDevice` files (three trees), GameNetwork's transport layer,
GameSpy, registry, and timers/threads has been individually inventoried,
not sampled or estimated.

**The Phase 3 spike has now run** (`native-port-spike/`, standalone,
not wired into the main game build - see that directory's CMakeLists.txt
for how to configure/build it independently). Result: **the resource/
semantics-matching risks the desk-check flagged as unresolved held up
under a real GL 3.3 core-profile renderer**, not just on paper:

- The D3D8-vs-GL clip-space convention (D3D8 NDC z in [0,w], GL in
  [-w,w]) was validated numerically, not just visually: a D3D8-style
  (RH, z-range-only) projection matrix converted via the standard row
  remap (`z_gl = 2*z_d3d - w_d3d`) agrees with a native GL projection
  matrix to within float precision (~2e-7) across five sample points.
  Note the spike isolates the z-range question from D3D8's actual
  left-handedness deliberately - handedness is a separate, independently
  well-understood porting step (winding-order/cull-mode adjustment), and
  an early version of this check that conflated the two produced a
  large, meaningless delta before being caught and fixed.
- Alpha-test-reference behavior without `glAlphaFunc` (removed in GL 3.3
  core) works as `discard` in the fragment shader, driven directly by
  the same `ShaderBits` vocabulary `shader.cpp` uses (`ALPHATEST_ENABLE`
  bit decoded to a shader uniform, not a separate ad hoc flag).
- A render2d-equivalent orthographic textured quad and a perspective-
  projected, alpha-tested, additively-blended `ShaderClass`-driven quad
  both rendered correctly to an offscreen FBO, confirmed by pixel
  inspection (not just a non-crash check) - including correct
  `GL_DEPTH_TEST` occlusion against a third nearer opaque quad.
- **Now also run headless under Mesa/llvmpipe** (WSL2 Ubuntu 26.04, Mesa
  26.0.3, `LIBGL_ALWAYS_SOFTWARE=1` forcing the software rasterizer, no
  GPU passthrough): after two portability fixes (`gl_core33.h`'s
  Windows-only `windows.h`/`APIENTRY` handling made conditional on
  `_WIN32`; a missing `<cstddef>` include for `ptrdiff_t`, which MSVC
  pulls in transitively via `windows.h` but GCC does not) and disabling
  GLFW's Wayland backend (`-DGLFW_BUILD_WAYLAND=OFF` - not installed in
  this environment, X11 already available and sufficient), the spike
  built clean and produced **pixel-identical output** to the Windows/
  NVIDIA run: same ~2e-7 clip-space delta, same 114113/262144 non-
  background pixel count, visually identical render. This is real
  independent-implementation evidence, not just a second data point on
  the same driver - Mesa's llvmpipe core-profile GL 3.3/4.5 implementation
  is written independently of NVIDIA's, so agreement between the two is
  the actual signal the desk-check's core-profile-strictness concern was
  asking for.
- **Texture-origin (D3D8 top-left vs GL bottom-left) V-flip: now
  validated**, without needing a real D3D8 reference render. The spike
  renders a four-quadrant orientation marker (red/green/blue/yellow, no
  rotation or flip maps it onto itself) authored in top-left-origin row
  order, twice: once uploaded as-is (row 0 first) - reproducing the exact
  bug a naive D3D8-to-GL texture-loader port would have, since GL treats
  row 0 as v=0/screen-bottom - and once with rows reversed at upload time
  (the fix; flipping V at UV-generation time instead is equivalent).
  Checked both visually (`vflip_check.png`: left quad shows blue/yellow
  at top - wrong; right quad shows red/green at top - correct, matching
  the intended authoring) and numerically (sampling known pixel positions
  and checking which quadrant color landed where), confirmed on both
  Windows/NVIDIA and Linux/Mesa-llvmpipe.
- **macOS GL 4.1 core validation: done. An initial "platform dead end"
  conclusion was wrong - root cause was GLFW, not the platform, and the
  fix worked.** `.github/workflows/macos-spike.yml` was built and run 5
  times on `macos-latest`. Runs 1-4 all built clean (after fixing a real
  portability bug: no `<GL/gl.h>` on macOS, needs `<OpenGL/gl3.h>`) but
  `glfwCreateWindow` always failed with `NSGL: Failed to find a suitable
  pixel format` (GLFW error 0x10009), even after minimizing depth/
  stencil/sample window hints. First-pass web research (go-gl/glfw#335,
  Razakhel/RaZ#21, go-flutter-desktop/go-flutter#504) concluded this was
  a fundamental GitHub-hosted-runner limitation with no fix. A follow-up
  fable investigation found that conclusion didn't hold up: GitHub's
  macOS VMs do lack a working *accelerated* GPU device (tracked upstream
  at actions/runner-images#7085), but the actual proximate cause was that
  GLFW's `src/nsgl_context.m` unconditionally requests
  `NSOpenGLPFAAccelerated` with no way to opt out, which is what turns
  "no accelerated GPU" into "no context at all" - Apple's software GL
  renderer is present on these VMs and does serve real GL 3.2+/4.1 core
  contexts (confirmed independently: libsdl-org/SDL#1180), it was just
  being excluded by GLFW's hardcoded request. Vendored the one-line fix
  proposed upstream but never merged (glfw/glfw#2080: swap
  `NSOpenGLPFAAccelerated` for `NSOpenGLPFARendererID`/
  `kCGLRendererGenericFloatID`) as a CMake `PATCH_COMMAND`
  (`native-port-spike/patch_glfw_nsgl.cmake`, portable `file(READ)`/
  `string(REPLACE)`/`file(WRITE)` rather than `sed`, so it runs
  identically cross-platform - a no-op everywhere except the GLFW source
  tree's own macOS-only file). **Run 5, with the patch applied, passed
  completely**: `GL_VERSION: 4.1 APPLE-23.1.1`, `GL_RENDERER: Apple
  Software Renderer` - real Apple GL 4.1 core-profile semantics, not a
  mock - producing output pixel-identical to both the Windows/NVIDIA and
  Linux/Mesa-llvmpipe runs (same ~2e-7 clip-space delta, same
  114113/262144 non-background pixel count), with the texture-origin
  V-flip check also passing. This is now genuine three-independent-
  implementation agreement on the exact question Phase 3 existed to
  answer.

This resolves the plan's single biggest previously-open unknown: option
(a) is no longer just evidence-backed by desk-check, it has a working,
numerically-checked prototype behind it.

Everything else is now either ready to implement directly (Phase 1's
build-system slice; Phase 7's networking/registry/timer work, which has
enough file-level detail to start without further research) or ready to
scope precisely (Phase 5's rendering sub-phases, now informed by exactly
which W3DDevice files are D3D8-heavy vs. clean, sim-reachable vs. not,
and genuinely divergent vs. cosmetic per-tree, plus a validated spike
behind the graphics-API choice). The unify-before-porting candidates
list is also now complete enough to sequence real #555 work alongside
the relevant native-port phases rather than needing further discovery;
`registry.cpp` and `shader.cpp`/`mapper.cpp`/`vertmaterial.cpp` (the
latter flagged above as close to a hard prerequisite for Phase 5(c)'s
shader/mapper rewrite, though not for the Phase 3 spike itself - see
Draft 6 history) are both merged (see git history for #555).

## Draft 11: Phase 1's newly-exposed GUI/GameNetwork error layer -
root-cause verification and fix plan

A fable review pass (planning only, no code written) verified the
446-error batch the `KeyDefs.h`/`GameState.h` fixes exposed and
produced the fix plan below. Every claim here was checked against the
actual code (file:line), not inferred from symbol names.

**Headline finding: `WindowMsgData` is a single-point-of-fix on the
same order as `KeyDefs.h`/`GameState.h`, and it's safe to widen.**
`Core/GameEngine/Include/GameClient/GameWindow.h:76` -
`typedef UnsignedInt WindowMsgData;` - is the argument type of every
GUI callback signature (`Gadget.h:482-525`'s 20+ free-function
declarations), and the codebase's universal convention is to pass raw
pointers through it via cast: `GadgetTextEntry.h:69` -
`winSendSystemMsg( g, GEM_SET_TEXT, (WindowMsgData)&text, 0 )` (a
`UnicodeString*`) - and because this is an inline function in a widely
`#include`d header, this one line accounts for all 42 `GadgetTextEntry.h`
duplicate errors, the same shape as the `KeyDefs.h` fix. Same pattern at
10+ sites each in `GadgetPushButton.cpp`, `GadgetTextEntry.cpp`, and
throughout the Gadget/menu files generally.

Checked for safety on all three axes that would make widening this
typedef risky, and none apply: **(1) never stored** - dispatch is
synchronous (`GameWindowManager.cpp:705` -
`return window->m_system( window, msg, mData1, mData2 );`, no message
queue), and a repo-wide grep for `WindowMsgData` struct/array members
found zero; **(2) never serialized** - no `xfer`/`Snapshot`/CRC code
touches it, GUI messaging is pure transient client-side state, outside
both the save-file and lockstep-CRC surfaces; **(3) no Windows-build
impact** - on the only shipping targets (32-bit Win32),
`uintptr_t` *is* `unsigned int`, so widening is a zero-codegen-change
no-op there. One implementer note: this codebase still nominally
supports VC6 (`CPP_11` macros elsewhere), which has no `<cstdint>` -
get `uintptr_t` via `Dependencies/Utility/Utility/stdint_adapter.h` or
gate on `_MSC_VER < 1300`.

**Verdict: change `GameWindow.h:76` to a pointer-sized unsigned
integer. This is exclusively a Phase 1 compile issue with no
behavioral, save-format, or network contract attached**, and should
eliminate the `(WindowMsgData)` cast-error family wholesale - `GadgetTextEntry.h`'s
42, most of `GadgetListBox.cpp`'s 32, the Gadget{PushButton,
VerticalSlider,ComboBox,CheckBox,TextEntry,RadioButton,
HorizontalSlider} files (~70 more), most menu files' handful each,
`ControlBar.cpp`/`GameWindowManager.cpp`/`GameLogicDispatch.cpp`/
`HotKey.cpp` - a conservative estimate of **~180-250 of the 446 error
lines from one line changed**.

**A sibling pattern needs per-site edits, not a typedef change**:
integers stored in `void*` slots via `GadgetListBoxGetItemData()`/
`winGetUserData()`, cast back with a literal 32-bit target type -
`(Int)GadgetListBoxGetItemData(...)`, `(GPProfile)...`, etc. (e.g.
`WOLBuddyOverlay.cpp:218-219,270,396`, `WOLLobbyMenu.cpp:1486,1564,1800`,
`LanLobbyMenu.cpp:347`, `PopupLadderSelect.cpp:133,376,442,634`,
`LobbyUtils.cpp:212,699,921`, `LANAPICallbacks.cpp:636`,
`SkirmishMapSelectMenu.cpp:102`, `W3DProgressBar.cpp:79,189`). These are
value-truncation-*safe* (the stored value was always an `Int`) but GCC
correctly rejects the pointer-to-smaller-int cast. Fix:
`(Int)(uintptr_t)ptr` on retrieval, `(void*)(uintptr_t)value` on store.
Accounts for `WOLBuddyOverlay.cpp` (7), `LobbyUtils.cpp` (4),
`GUIUtil.cpp` (3), `LANAPICallbacks.cpp` (1), and part of several other
files' counts.

**Four corrections to prior claims, found while verifying the above:**

1. **`LANAPI.h`'s `static_assert` is not a 64-bit alignment issue** (this
   plan's own prior claim, now corrected). `LANMessage` is
   `#pragma pack(push, 1)` (`LANAPI.h:143`, confirmed) - alignment/padding
   cannot be the cause. The real cause: the struct is full of `WideChar`
   arrays, and `WideChar` is `typedef wchar_t` (`Lib/BaseType.h:36`,
   confirmed) - **2 bytes on Windows, 4 bytes on Linux/macOS** - so the
   packed struct roughly doubles in size and trips
   `static_assert(sizeof(LANMessage) <= MAX_LANAPI_PACKET_SIZE)`
   (`LANAPI.h:270`). This is a wire-format width issue (identical on
   32-bit Linux too, nothing to do with 64-bit), belongs in Phase 7
   (networking) alongside the rest of the LAN/GameSpy wire format work,
   not Phase 2. Interim Phase 1 unblock: relax the assert/ceiling under
   `!_WIN32` with an explicit "LAN wire format not Windows-compatible
   yet" comment - real cross-platform LAN play needs an explicit
   16-bit-wide wire format, not just a bigger buffer.
2. **`getifaddrs()` (this plan's prior suggestion) is the wrong
   primitive for `StagingRoomGameInfo.cpp`'s SNMP code.** Reading
   `GetLocalChatConnectionAddress` (`StagingRoomGameInfo.cpp:101`): the
   SNMP MIB-II TCP-table walk answers "which *local* IP is my
   established connection to `peerchat.gamespy.com:6667` using?" - a
   route-selected source address on a multi-NIC/NAT machine, not an
   interface list. Correct POSIX equivalent: the standard
   connected-UDP trick - `socket(AF_INET, SOCK_DGRAM)` -> `connect()` to
   the server address (sends no packets) -> `getsockname()` -> local IP
   -> `close()`. ~25 lines replacing ~350 lines of SNMP DLL-loading,
   strictly more accurate than an interface walk. Unify-before-porting
   flag: **Generals has a second, un-unified copy of this exact SNMP
   function** at `Generals/Code/GameEngine/Source/GameNetwork/
   GameSpyGameInfo.cpp:97` (the old per-tree GameSpy path this plan
   already flagged elsewhere as still actively built) - it isn't in the
   current error list only because it lives in a different build
   target, and will need the same fix when that target is built.
3. **`GetDoubleClickTime` already has a portable precedent in-tree -
   no fresh reimplementation needed.** `GlobalData.cpp:1046-1048`
   (GeneralsMD `:1053-1055`) already branches Windows
   `GetDoubleClickTime()` vs. a hardcoded `500` default on other
   platforms. `GadgetListBox.cpp:70` is a second, independent call site
   predating that fix (and a namespace-scope static initializer, so it
   can't read `TheGlobalData` even if it wanted the same value at
   runtime) - fix is a `compat.h` inline returning 500, matching the
   existing precedent, not a new design decision.
4. **Draft 9's centralized `__int64` fix does not cover
   `Network.cpp`.** `stdint_adapter.h`'s `__int64` typedefs and
   `intrin_compat.h`'s are both MSVC-direction-only / behind
   `_MSC_VER < 1300`; no non-Windows `__int64` exists anywhere in
   `Dependencies/Utility`. `Network.cpp` needs
   `#define __int64 long long` (a macro, so `unsigned __int64` still
   composes) added there.

Also confirmed fine as-is: `timeGetTime()`/`GetTickCount()` already
have portable definitions in `time_compat.h:29-41` (from earlier
passes), so `GadgetListBox.cpp`'s calls to them aren't part of this
error batch at all.

**Category-by-category classification** (mechanical shim / real
reimplementation / honest deferral, per this plan's established
three-way split):

- **`StagingRoomGameInfo.cpp` SNMP** (51 errors) - real reimplementation
  (correction 2 above); affects actual online NAT/multi-NIC behavior
  (called from `PeerThread.cpp:2281`), and the portable version is
  smaller than the original, so worth doing for real rather than
  stubbing.
- **`Network.cpp` `__int64`/`LARGE_INTEGER`** (24) - mechanical shim,
  but must stay behaviorally real: this is live multiplayer frame-pacing
  code (`Network.cpp:205-207,340,730-810`), same category as
  `FrameRateLimit.cpp` from an earlier pass. `compat.h`:
  `#define __int64 long long`. `time_compat.h`: a `LARGE_INTEGER` union
  with a `QuadPart` member (the code casts `(LARGE_INTEGER*)&`some
  `__int64`, so layout must match) plus `QueryPerformanceCounter` via
  `clock_gettime(CLOCK_MONOTONIC)` in nanoseconds and
  `QueryPerformanceFrequency` = 1e9.
- **`GameLogic.cpp`'s `setFPMode()`** (7, both trees) - real
  reimplementation with a documented gap.
  `_fpreset(); _controlfp(newVal, _MCW_PC|_MCW_RC)` sets rounding to
  `_RC_NEAR` and precision to `_PC_24`. Non-Windows:
  `fesetenv(FE_DFL_ENV); fesetround(FE_TONEAREST);` (matches `_RC_NEAR`,
  same precedent as `SimulationMathCrc.cpp`'s earlier `_fpreset` fix -
  rounding control needed a second call the earlier fix didn't need).
  `_PC_24` (x87 80-bit-to-24-bit precision truncation) has no portable
  equivalent under SSE2 codegen and needs none - x87 precision control
  doesn't exist there, and MSVC's own `_controlfp` ignores `_MCW_PC` on
  x86-64 too. Document as a Phase 8 determinism note (an x87 32-bit
  Windows build vs. an SSE 64-bit Linux build can diverge in FP
  regardless of this port - a pre-existing cross-platform-lockstep
  question, not one this shim creates).
- **`VK_RETURN`** (part of `KeyboardOptionsMenu.cpp`'s 18, both trees) -
  mechanical. `KeyboardOptionsMenu.cpp:725` compares a `WideChar` from
  `GWM_IME_CHAR` against `VK_RETURN` - it's really testing for the
  character `'\r'` (0x0D), not a virtual-key event. This is the *only*
  `VK_*` use anywhere in Core/Generals/GeneralsMD GameEngine sources
  (exhaustive grep, 2 hits, both this same line in each tree) - no
  VK_* table needed, unlike `DIK_*`. `#define VK_RETURN 0x0D` in
  `compat.h`. The rest of this file's 18 errors are `WindowMsgData`
  casts.
- **`ReplayMenu.cpp`** (14) - mixed. `DeleteFile`/`FormatMessage`+
  `FORMAT_MESSAGE_FROM_SYSTEM`+`GetLastError` (delete-replay button,
  error toast) are mechanical shims - `compat.h`: `DeleteFile(p)` ->
  `remove(p)==0` (**note the inverted return convention**: Win32
  returns nonzero on success, `remove()` returns 0), `GetLastError()` ->
  `errno`, `FormatMessage`/`FormatMessageW` -> `strerror(errno)` into the
  caller's buffer. The "copy replay to Desktop" button
  (`SHGetSpecialFolderLocation(CSIDL_DESKTOPDIRECTORY)`+`LPITEMIDLIST`+
  `SHGetPathFromIDList`+`CopyFile`) is a real reimplementation:
  `$HOME/Desktop` plus a stdio-based `CopyFile` shim (read/write loop,
  Win32 return semantics, not `std::filesystem` given VC6 nominally
  still applies elsewhere) - also fixes the same shim's use in
  `Recorder.cpp:738`'s `archiveReplay()`.
- **`MEMORYSTATUS`** (`GameClient.cpp`, 4/tree) - mechanical shim,
  diagnostics-only (feeds `DEBUG_LOG` before/after asset preload, zero
  gameplay risk). `compat.h`: a `MEMORYSTATUS` struct plus
  `GlobalMemoryStatus()` via `sysinfo()` on Linux (zeroed fallback
  elsewhere - macOS is 64-bit-Phase-2-blocked anyway so this doesn't
  need a mac path yet). Correction: this is `GameClient.cpp`-only, not
  spread across menu files as the raw grep grouping suggested -
  `SkirmishGameOptionsMenu.cpp`'s 5 errors are unconfirmed and likely
  `WindowMsgData` casts instead, per the flag below.
- **`Keyboard.cpp`'s `HKL`** (2) - small real reimplementation.
  `Keyboard.cpp:342-351` - `GetKeyboardLayout(0)` checked against 5
  French LANGIDs to switch key-*name* display tables to AZERTY, falling
  back to the existing `OurLanguage` default otherwise. Non-Windows:
  detect French from `LC_ALL`/`LC_CTYPE`/`LANG` env prefix `fr` instead.
  Affects only French users' hotkey display names; real XKB/SDL layout
  querying belongs to Phase 4 (windowing/input).
- **Already-deferred items, unchanged**: `WebBrowser.h`/`ftp.h` (COM/ATL,
  Phase 7), `DbgHelpLoader.h` (imagehlp.h, dead weight when crash-dumps
  are off), `BezierSegment.h` (d3dx8math.h), `IMEManager.cpp`
  (mbstring.h, Phase 4 IME), `GlobalLanguage.cpp`'s
  `Add`/`RemoveFontResource` (no-op stub now, real fonts come from
  Fontconfig in Phase 7), the pointer-truncation files
  (`PartitionManager.cpp` etc., Phase 2 - may reuse the same
  `(uintptr_t)` double-cast idiom the `WindowMsgData` fix establishes).
  `gamespy-src`'s vendored `gsplatform.h` - worth a 15-minute check
  first (the GameSpy SDK historically had its own `_LINUX` platform
  macro; defining it might fix the vendored header without touching
  vendored code) before falling back to deferring it as before.

**Ordered fix plan** (batch = fix -> WSL2 `ninja -k0` recount -> Windows
rebuild re-verify -> commit -> push, same workflow as every prior pass):

1. **Batch 1 (do first and alone, so its error-count delta is
   measurable)**: the `WindowMsgData` typedef change, plus the
   `(Int)(uintptr_t)`/`(void*)(uintptr_t)` double-cast fixes for the
   `GadgetListBoxGetItemData`/`winGetUserData` sites the typedef change
   can't reach. Both trees, since most affected files are per-tree
   duplicated - fix pairs together.
2. **Batch 2 (one commit, all in `Dependencies/Utility/Utility/`)**:
   `__int64` macro, `LARGE_INTEGER`+`QueryPerformanceCounter`/
   `Frequency`, `VK_RETURN`, `DeleteFile`, `CopyFile`, `GetLastError`+
   `FormatMessage`/`FormatMessageW`, `GetDoubleClickTime`,
   `MEMORYSTATUS`/`GlobalMemoryStatus`.
3. **Batch 3 (real reimplementations, grouped by risk)**: `setFPMode()`
   in both trees' `GameLogic.cpp`; `GetLocalChatConnectionAddress`
   connect+`getsockname` rewrite in `StagingRoomGameInfo.cpp` (note
   Generals' un-unified duplicate in the commit message, don't fix it
   opportunistically); `Keyboard.cpp`'s locale-based French detection;
   `ReplayMenu.cpp`'s `$HOME/Desktop` path (both trees).
4. **Batch 4 (documented deferrals, one commit)**: `LANAPI.h` assert
   relaxation with the corrected wire-compat comment; the
   `gsplatform.h` `_LINUX`-define experiment (or defer); confirm
   everything else stays deferred as already catalogued.

**Flags for the implementer**: `SkirmishGameOptionsMenu.cpp`'s 5 errors
were grouped with `MEMORYSTATUS` by the raw per-file error count but do
NOT actually contain it (verified: zero matches) - don't assume, check
the post-Batch-1 rebuild's residue. The ~20 other unconfirmed
"probably `WindowMsgData`" menu files (single-digit error counts each)
will be confirmed or refuted for free by the Batch 1/2 rebuilds - don't
pre-investigate each one, just diff the error inventory after each
batch, the same workflow already used successfully for 14 passes
running.

**Batch 1 result (commit `5106888e2`): confirmed, 446 -> 194 unique
error lines.** `GameWindow.h:76`'s `typedef uintptr_t WindowMsgData`
alone dropped 446 to 256 (190 errors, right in the estimated
~180-250 range). The sibling `(Type)(uintptr_t)expr` double-cast fix
across ~45 call sites (`GadgetListBoxGetItemData`/
`GadgetComboBoxGetItemData`/`GadgetButtonGetData`/`winGetUserData`
call sites, plus `userData`/`param` callback-data arguments in
`INI.cpp`/`ControlBar.cpp`/`ThingTemplate.cpp`/`AIStates.cpp`/
`BuddyThread.cpp`/`PartitionManager.cpp`, both trees where duplicated)
dropped 256 to 194. One site turned out not to be the safe
value-in-slot pattern: `GadgetListBox.cpp`'s `GLM_GET_SELECTION`
multi-select branch stores a real `Int*` array pointer through the
`WindowMsgData` slot (Chat.cpp's caller passes `(Int*)&somePointerVar`
and expects the full address back) - a plain `(Int)` truncating cast
there would have been a genuine new 64-bit bug (writing 4 of 8 bytes
into the caller's pointer variable), not just a compile error, so this
one site was fixed by writing through `Int**` instead of the
mechanical idiom. Batches 2-4 (below) are not yet implemented.
Everything not covered by Batch 1 remains exactly as catalogued above,
plus a handful of newly-surfaced small items the 194-line residue
exposed (`_wtoi`/`iswascii` MSVC CRT names, a `pause()` name collision
with POSIX `unistd.h`, `OSVERSIONINFO`/`GetVersionEx` in
`PopupPlayerInfo.cpp` - the same Win9x-detection pattern already fixed
once in `GameState.cpp`) - small, Batch-2/3-shaped work, not a new
category.

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
- Draft 3: folded in completed Phase 0 file-by-file inventories (5
  parallel research passes) for W3DDevice's `Drawable`/`GameClient`/
  `GameLogic` subtrees, GameNetwork, GameSpy, registry, and
  timers/threads. Notable corrections: the `W3DDevice/GameLogic` path
  doesn't exist under `Core/` (duplicated and diverged per-tree
  instead); `Drawable/Draw`'s 18 files need zero rewrite (all D3D8 work
  is one layer down in WW3D2); networking is a small, mostly-already-
  half-ported job, not the genuine missing phase it looked like;
  WWLib's thread/mutex "portability" scaffolding is largely
  non-functional stubs despite compiling cleanly; and three areas
  (`W3DDevice/GameLogic`, GameSpy, `registry.cpp`) turned out to be
  their own un-unified #555 duplication gaps, found as a side effect of
  this inventory rather than gone looking for. Formalized
  "unify-before-porting" as an explicit sequencing rule rather than a
  passive observation.
- Draft 4: closed the last Phase 0 gap (`W3DDevice/Common`+
  `GameClient`-root, 71 files, 2 more parallel passes - confirming this
  entire slice is per-tree-duplicated too, and that at least 14 of its
  35 unique classes carry real Zero-Hour-only gameplay features, not
  just engine drift) and desk-checked the graphics-API decision with a
  focused pass reading `shader.cpp`/`mapper.cpp`/`W3DShaderManager.cpp`
  in full. Major findings: the texture-combiner system is small and
  centralized (~40 named configs, one function), not the open-ended
  risk it looked like; the shader-binary-asset "blocker" was wrong -
  the GPL-licensed assembly source ships in-tree, re-authoring is
  days-scale; and upstream is already building exactly the recommended
  option (a) architecture in live discussion #1575, with a maintainer
  independently stating the same "unify Generals/ZH first" rule this
  plan had already formalized. Phase 0 declared fully complete.
- Draft 5 (this version): a holistic fable review of the full document
  (not a narrow section, the first since Draft 2) independently
  re-verified a sample of compiled claims directly against the repo and
  re-fetched discussion #1575 to check it wasn't paraphrased into
  something stronger than what was said - both held up. It also found
  the Phase 0 arithmetic didn't actually reconcile on paper (18+35+6+67
  = 126, not 131; the true breakdown is 19+35+6+71=131), the GameSpy
  file count was wrong (14 in Core, 19 unique logical files, not
  "12"/"20"), two stale phase-number cross-references had crept in from
  re-numbering (audio mislabeled Phase 4 instead of 6, GameSpy
  mislabeled Phase 6 instead of 7), a third registry wrapper
  (`WWDownload/registry.cpp`) had been missed entirely, and the
  "Biggest risks" section hadn't been updated to include the D3D8-heavy
  files Draft 4's own research had just found. All fixed here. No
  load-bearing conclusion (phase ordering, the option (a) recommendation,
  the unify-before-porting rule, the thread/mutex-stub or CRC-exclusion
  findings) needed to change - this pass was arithmetic and
  cross-reference hygiene, not a substantive correction.
- Draft 6 (this version): moved from planning to first execution. A
  fable review of the "proceed with unify + spike" plan (independent of
  the holistic Draft 5 review) caught two real issues before work
  started: `shader.cpp`/`mapper.cpp` unification is not a `#555`-style
  mechanical dedup (328/1580+ genuine diff lines - a real feature-
  preserving merge), and the plan's own claim that this unification is a
  hard prerequisite for the Phase 3 spike was wrong (`shader.h`'s
  `ShaderBits` layout is byte-identical between trees, so the spike only
  needed the vocabulary, not the .cpp merge - the two were run in
  parallel instead of sequentially as a result). `registry.cpp` unified
  into `Core/` (both `g_gameengine`/`z_gameengine` build-verified);
  `shader.cpp`/`mapper.cpp`/`vertmaterial.cpp` unification in progress.
  The Phase 3 spike ran for the first time - see "Readiness assessment"
  for what it did and didn't validate.
- Draft 7: Phase 3 fully closed (spike passed on a third independent GL
  implementation, real macOS GL 4.1 core via a vendored GLFW fix after
  an initial "platform dead end" CI conclusion turned out to be wrong;
  texture-origin V-flip validated too). `W3DDevice/GameLogic` unified.
  Phase 1 work started and its scope corrected empirically rather than
  re-estimated from reading source: CONFIGURE was already found to
  succeed on Linux (contrary to this doc's prior claim), and the real
  blocker turned out to start at BUILD time, in WWLib/WWMath - much
  earlier and more foundational than the GameEngineDevice-focused
  description this doc had. Fixed the foundational blockers found there
  (a decades-old missing `osdep.h` traced through full git history to
  Westwood's original 2003 source release; three related portability
  bugs it had been masking); catalogued, but did not yet fix, a longer
  tail of D3D8/Winsock/ATL/registry issues found by building
  `g_gameenginedevice` on Linux with `ninja -k0` to collect every
  independent error at once.
- Draft 8: fixed the catalogued Phase 1 tail from Draft 7 (D3D8/Winsock/
  ATL/registry gating across GameEngineDevice, WW3D2, WWDownload, WWLib,
  PreRTS.h - commit `9e1b76094`), then found pushing further reaches
  much deeper than expected: raw Win32 API usage throughout GameEngine's
  own core code (not just its device/rendering layer), plus at least one
  apparent 64-bit alignment bug (a `static_assert` on `LANMessage`'s
  size). 258 unique build errors remain on Linux. This is the second
  time actually attempting the build (rather than reading source and
  estimating) revealed the true scope to be substantially larger than
  this phase's stated framing - Phase 1 is not close to done, and what's
  left overlaps Phase 2/7 rather than staying contained to "build
  system" work.
- Draft 9: worked through most of the Draft 8 tail across six more
  commits (`ffd7fd552`..`2f63f1e53`), file by file, verifying via WSL2
  rebuild after each fix and re-verifying the Windows build after each
  batch. 258 -> 48 unique remaining error lines (~81% reduction). Notable
  pattern: several fixes were real portable reimplementations (
  `std::chrono`-based `FrameRateLimit`, `flock()`-based `ClientInstance`,
  `opendir`-based scratch-pad cleanup, `$HOME`-based user-data paths),
  not stubs, since these affect actual gameplay/UX, not just build
  success - stubbing was reserved for genuinely out-of-scope things
  (`registry.cpp`'s Windows-Registry backend, `GetCommandLineA()` pending
  a portable entry point). Also found that fixing one widely-duplicated
  MSVC-name mismatch (`_isnan`, used directly at 12+ call sites) in the
  existing central compat-macro header was far more leveraged than
  per-file fixes - worth checking for before assuming every remaining
  error needs its own bespoke fix. Remaining tail is now a scattered set
  of smaller files plus a few qualitatively harder items intentionally
  not attempted: a likely-genuine 64-bit alignment `static_assert`
  failure (Phase 2 territory), real COM/ATL work (`WebBrowser.h`, Phase
  7), and an error inside a vendored third-party dependency's own header
  (not this codebase's code to fix).
- Draft 10: a fable review of the Draft 9 batch (`ffd7fd552`..`2f63f1e53`)
  confirmed the "both trees fixed" check held everywhere, and found
  three real issues (commit `22a13a291`): `PerfTimer.cpp` assumed x86
  unconditionally on non-Windows (`__rdtsc()` via `<x86intrin.h>`),
  which would have broken on Apple Silicon macOS - a real target of this
  port, not a hypothetical one; `ClientInstance.cpp`'s `flock()` lock
  conflated a real `open()` failure with "another instance is running,"
  risking an infinite retry loop, and used a predictable, shared,
  world-writable `/tmp` path; and three `_NSGetExecutablePath`/`readlink`
  call sites didn't initialize their buffer before the call, risking
  operating on uninitialized stack memory on failure. All three fixed.
  One flagged finding (a `compat.h` macro allegedly leaking onto
  Windows) was independently re-verified as a false positive - the
  macro was already correctly scoped, and the already-passing Windows
  rebuild from the prior commit corroborated this. Not every fable
  finding is automatically correct; each one still needs to be checked
  against the actual code before acting on it.
- Draft 11 (this version): five more commits worked through the
  48-line tail (one mid-batch mistake - re-gating
  `PingThread.cpp`/`GameResultsThread.cpp`'s `#include <winsock.h>`
  without replacing what depended on it, regressing 44->117 errors -
  caught and properly fixed same-session), then fixing `KeyDefs.h`
  (the single largest remaining contributor, 121 of 153 error lines)
  and `GameState.h`/`GameState.cpp` (another ~90 lines from one header)
  let the build proceed **much** further than before, surfacing 446
  total error lines across 66 files - not a regression, but a
  previously-unreachable layer of GUI (`GameClient/GUI/Gadget`,
  `GUICallbacks/Menus`) and `GameNetwork` code becoming visible for the
  first time. A fable planning pass (verification only, no code
  written) analyzed this new batch and found a second single-point-of-fix
  on the same order: `GameWindow.h`'s `WindowMsgData` typedef
  (`UnsignedInt`, used to carry raw pointers through the GUI
  message-callback system) is safe to widen to a pointer-sized integer -
  verified never stored, never serialized, zero Windows-build impact -
  and should resolve an estimated ~180-250 of the 446 lines by itself.
  Also corrected a prior claim: `LANAPI.h`'s `static_assert` failure was
  previously diagnosed as a 64-bit alignment issue (Phase 2); it's
  actually a `wchar_t`-width wire-format issue (2 bytes on Windows, 4 on
  Linux/macOS, inside a `#pragma pack(1)` struct) that exists identically
  on 32-bit Linux too, and belongs in Phase 7 (networking) instead. Full
  root-cause analysis, corrections, and an ordered fix plan for the
  remaining categories (`Network.cpp`'s `__int64`/`LARGE_INTEGER` timer
  code, `GameLogic.cpp`'s floating-point control word, a Windows SNMP
  local-IP-detection function that needs a `connect()`+`getsockname()`
  rewrite instead of the previously-suggested `getifaddrs()`, and
  several smaller mechanical shims) are in the new section above. Not
  yet implemented - this draft is the plan, not the fix.
- Draft 12 (this version): implemented Draft 11's Batch 1 (commit
  `5106888e2`). `GameWindow.h`'s `WindowMsgData` typedef widened to
  `uintptr_t`, confirmed safe by the Draft 11 verification -
  eliminated 190 of 446 WSL2 build errors by itself, landing right in
  the estimated ~180-250 range. The sibling `(Type)(uintptr_t)expr`
  double-cast fix followed across ~45 call sites in both trees where
  duplicated, eliminating 62 more (446 -> 194 total). One site
  (`GadgetListBox.cpp`'s multi-select `GLM_GET_SELECTION` handler)
  turned out to store a real pointer through the message-data slot
  rather than an encoded int - the mechanical idiom would have quietly
  introduced a real 64-bit correctness bug (a plain `(Int)` cast
  writing only 4 of the caller's 8 pointer bytes) instead of just
  fixing a compile error, so it got a `Int**`-typed fix instead,
  actually more correct than the Windows original's implicit
  32-bit-pointer assumption. Batches 2-4 not yet started; see the
  updated note at the end of the Draft 11 section above for the exact
  residue and a few newly-surfaced small items (`_wtoi`/`iswascii`, a
  `pause()` name collision with POSIX `unistd.h`, one more
  `OSVERSIONINFO`/`GetVersionEx` Win9x-detection site) the 194-line
  rebuild exposed.
- Draft 13 (this version): implemented Draft 11's Batches 2-4
  (commits `38e7ae34a`, `223539908`, `d28d8284b`), taking
  `g_gameenginedevice` (Generals) from 194 down to **21 unique WSL2
  error lines - every single one an already-catalogued, deliberately
  deferred item** (real COM/ATL work in `WebBrowser.h`/`ftp.h`, Phase
  7; `DbgHelpLoader.h`'s `imagehlp.h`, dead weight when crash-dumps are
  off; `BezierSegment.h`'s `d3dx8math.h`; pointer-truncation in
  `LocalFile.cpp`/`TransportContain.cpp`/`FirewallHelper.cpp`, Phase 2;
  `IMEManager.cpp`'s `mbstring.h`, Phase 4 IME). Windows build stayed
  at 0 errors throughout every commit.

  Batch 2's compat shims caught one real self-inflicted regression
  mid-batch, same pattern as the earlier `winsock.h` mistake: defining
  `__int64` as a preprocessor macro (`#define __int64 long long`)
  seemed reasonable since `unsigned __int64` still composes through
  substitution, but `WWDebug/wwprofile.h` already has its own
  pre-existing `#ifdef _UNIX typedef signed long long __int64;` - the
  macro rewrote that line's own name, producing `typedef ... long long
  long long;` and cascading into 1022 bogus errors across every file
  that transitively includes `compat.h`. Fixed by using a plain
  typedef instead (legal to redeclare identically in C++, unlike a
  macro) - a reminder that this codebase's existing portability
  scaffolding should be checked for before assuming a symbol needs a
  fresh compat shim.

  Batch 3's real reimplementations: `StagingRoomGameInfo.cpp`'s SNMP
  MIB-II walk replaced with the standard connected-UDP
  `connect()`+`getsockname()` trick (contract verified against every
  caller - the function returns network-byte-order, callers `ntohl()`
  it themselves); French-keyboard detection in `Keyboard.cpp` ported
  to locale-env sniffing; the "copy replay to Desktop" button in
  `ReplayMenu.cpp` (both trees) ported to `$HOME/Desktop`, matching the
  `$HOME/Documents` precedent.

  Batch 4 also resolved two items that turned out not to be what the
  plan assumed: `gsplatform.h`'s 30 errors were not a missing
  `_LINUX` platform macro (that macro turned out to be pure
  documentation in this vendored header, unused in its actual `#if`
  logic, which already correctly branches on the already-defined
  `_UNIX`) - the real cause was our own `string_compat.h`'s `_strlwr`
  having C++ linkage where the vendored SDK's own declaration expects
  `extern "C"`. And two genuine pre-existing bugs surfaced that GCC
  caught and MSVC's permissive mode didn't: `GameWindowManagerScript.cpp`
  (both trees) returning `FALSE` (`false`, not a null-pointer constant)
  from a function returning `GameWindow*`, and `QuitMenu.cpp` (both
  trees) comparing a pointer against `FALSE` with `!=` instead of
  `nullptr`.

  **New scope discovered while checking the other build target**:
  `z_gameenginedevice` (GeneralsMD/Zero Hour) had not been WSL-built at
  all this session - only `g_gameenginedevice` (Generals) had. A first
  build shows 71 unique error lines, and the majority are NOT new
  categories - they're the identical `WindowMsgData`-sibling
  `(Type)(uintptr_t)expr` pattern from Batch 1, in GeneralsMD's own
  separate copies of ~13 menu files (`WOLQuickMatchMenu.cpp`,
  `SkirmishGameOptionsMenu.cpp`, `WOLGameSetupMenu.cpp`,
  `WOLBuddyOverlay.cpp`, `LanGameOptionsMenu.cpp`, `WOLLobbyMenu.cpp`,
  `PopupLadderSelect.cpp`, `PopupPlayerInfo.cpp`, `PopupHostGame.cpp`,
  `OptionsMenu.cpp`, `LanLobbyMenu.cpp`, `SkirmishMapSelectMenu.cpp`,
  `ScoreScreen.cpp`) that Batch 1 only fixed in the Generals tree.
  Genuinely new: `SabotageInternetCenterCrateCollide.cpp` (2 errors,
  same sibling-cast pattern) - a Zero-Hour-only file with no Generals
  counterpart. The remaining errors (`WebBrowser.h`/`ftp.h`/
  `DbgHelpLoader.h`/`BezierSegment.h`/`LocalFile.cpp`/
  `TransportContain.cpp`/`FirewallHelper.cpp`/`IMEManager.cpp`) are the
  same already-deferred items as the Generals tree. Not yet fixed -
  next session's starting point is applying the established Batch-1
  sibling-cast idiom to this newly-checked target.
- Draft 14 (this version): applied the Batch-1 sibling-cast idiom to
  GeneralsMD's ~13 not-yet-fixed menu files plus one Zero-Hour-only
  file (`SabotageInternetCenterCrateCollide.cpp`) (commit `2aa67f8bb`).
  Also caught a second instance of the `FALSE`-compared-to-pointer bug
  class (`TransportContain.cpp`, both trees) that this session's own
  earlier error triage had mischaracterized as "pointer-truncation,
  Phase 2" without actually reading the line - a reminder to verify a
  bucket assignment against the actual code, not just the compiler's
  one-line error summary. **`z_gameenginedevice` (GeneralsMD) now
  matches `g_gameenginedevice`: both are down to error lines that are
  entirely shared `Core/` files already deferred for the other tree
  too** - no tree-specific errors remain in either target.

  A fable review of all 7 commits since Draft 12 (`5106888e2` through
  `2aa67f8bb`) - the `WindowMsgData` widening, the ~45+13 double-cast
  sites across both trees, the compat.h/time_compat.h shims, the
  `setFPMode`/SNMP/keyboard/Desktop-path real reimplementations, the
  `LANAPI.h` relaxation, and the three `FALSE`-vs-pointer bug fixes -
  independently re-verified every claim against the actual code (not
  the commit messages) and found **no defects**: `WindowMsgData` is
  confirmed never stored/serialized anywhere in the repo; the
  `GLM_GET_SELECTION` `Int**` fix matches its callers' actual
  `(Int*)&selections`-with-`Int* selections` contract exactly; no
  double-cast site wraps the wrong sub-expression; `LARGE_INTEGER`'s
  `.QuadPart` member is never accessed by any of this typedef's
  non-Windows-reachable call sites; `DeleteFile`/`CopyFile`'s Win32
  return-value semantics match every caller; the SNMP rewrite's
  network-byte-order contract is preserved exactly; all three
  `FALSE`-vs-pointer fixes are behaviorally equivalent replacements in
  both trees where duplicated; and `#ifdef`/`#endif` nesting balances
  across all 62 touched files. Two non-defect residue items worth
  remembering for later, not part of this pass's scope: `W3DDevice`'s
  `W3DProgressBar.cpp:79,189,232` has the same unguarded
  `(Int)window->winGetUserData()` pattern, currently invisible only
  because all of `W3DDevice` sits behind `if(WIN32)` in
  `Core/GameEngineDevice/CMakeLists.txt` and will need the same
  treatment once Phase 5 brings that target online; and Generals' own
  un-unified `GameSpyGameInfo.cpp:97` SNMP duplicate (already flagged
  in the Batch 3 commit message) still needs the same rewrite whenever
  that older GameSpy code path is built/touched.
- Draft 15 (this version): finalized Phase 1 and closed Phase 2 for
  the currently-portable subset (commits `ffb6772e0` through
  `cedf03b09`), working unattended per explicit user request.

  **Phase 1 finalization**: fixed the two remaining "pointer-
  truncation, Phase 2" items from Draft 14's triage after actually
  reading the code (not just the compiler's one-line summary) -
  both turned out to be genuine, platform-independent bugs, not
  pointer-width issues at all. `LocalFile.cpp`'s `writeChar()` (both
  overloads) returned the character pointer's own address instead of
  "a copy of the character written" (its own doc comment's words) -
  zero callers exist anywhere in this codebase, so fixed for real.
  `FirewallHelper.cpp` had a `ntohl()` call whose return value was
  entirely discarded (a complete no-op - the real byte-order handling
  already happens correctly a few lines later) - removed the dead
  statement. This is the second time this session a Draft's own
  triage bucket assignment turned out to be wrong on closer reading
  (the first was `TransportContain.cpp` in Draft 14) - a reminder that
  "pointer-truncation, Phase 2, deferred" is a conclusion to verify
  per-site, not a label to trust from the compiler's diagnostic class
  alone.

  Added real `linux-x64`/`linux-x64-debug` and `macos-arm64`/
  `macos-arm64-debug`/`macos-x64` CMake presets (native compiler
  toolchain, no vcpkg) - closing Phase 1's explicitly-flagged "real
  (not 32-bit-only) CMake presets" gap. `linux-x64` was verified
  locally to reproduce this session's ad-hoc WSL2 build exactly (same
  17-unique-error-line result for both `g_gameenginedevice` and
  `z_gameenginedevice`). Rewrote `macos-native.yml` (previously
  "advisory, expected to fail until Phase 1-2 land," using the old
  32-bit `unix` preset as a stand-in) to use `macos-arm64` and build
  the actual currently-portable target set instead of a doomed
  full-project attempt; added `linux-native.yml` as its Linux
  counterpart with a regression-count guard.

  **Real CI immediately paid for itself**: the first `macos-native.yml`
  run (real `macos-latest` hardware) failed in 53 seconds on a bug
  invisible from WSL2 alone - `MEMORYSTATUS`'s `GlobalMemoryStatus()`
  shim unconditionally included `<sys/sysinfo.h>`, which is Linux
  (glibc)-only and doesn't exist on macOS/BSD at all. Fixed with a
  `__linux__`/`__APPLE__` split (macOS branch uses `sysctlbyname` for
  physical RAM/swap totals - standard, verifiable sysctls -
  and honestly stubs available/free RAM to 0 rather than guessing at
  unverifiable Mach `host_statistics64()` API syntax; this whole
  struct is diagnostics-only, zero behavioral risk either way).

  The second CI run got to 1m7s and surfaced four more real macOS-only
  gaps, all invisible from Linux testing alone: `wchar_compat.h`'s
  `#define iswascii(...)` corrupted macOS/BSD libc's own real
  `iswascii()` declaration by substituting into its own name - the
  *identical* failure mode `__int64`-as-a-macro had with
  `wwprofile.h` earlier this session (glibc lacks this BSD-ism, so the
  macro is now `__APPLE__`-excluded); `FastAllocator.h` used
  `<malloc.h>`, a Linux/Windows-only convenience header, for plain
  malloc/free (swapped to `<cstdlib>`); `thread_compat.h`'s
  `GetCurrentThreadId()` returned `pthread_self()` as `int` - fine on
  Linux where `pthread_t` is an integer, but macOS's `pthread_t` is an
  opaque pointer that cannot convert to `int` at all (fixed with
  `pthread_mach_thread_np()`, macOS's actual small-integer thread ID);
  `time_compat.h`'s `timeGetTime()` used `CLOCK_BOOTTIME`, which is
  Linux-specific and undeclared on macOS (added a `CLOCK_MONOTONIC`
  fallback). All four repeated across ~200 log lines but were
  confirmed to be exactly these 4 root-cause locations via file:line
  dedup - foundational compat headers included almost everywhere.

  The third CI run got to 3m37s-4m36s (real compilation progress each
  time) and found a final wave: `endian_compat.h`'s `__APPLE__` branch
  used `UInt16`/`UInt32`/`UInt64` (old Mac Carbon types, never
  included, so never declared) instead of the `uint16_t`/`uint32_t`/
  `uint64_t` every other branch correctly uses - a genuine pre-existing
  bug; `StackDump.h`/`.cpp` (both trees) had `EXCEPTION_POINTERS`
  (Windows SEH) and `DWORD` in `DumpExceptionInfo()`/
  `StackDumpFromContext()` even in the header's "disabled" stub
  branch - confirmed zero non-Windows callers exist (the only callers,
  `WinMain.cpp`/`WorldBuilder.cpp`, are themselves Windows-only/MFC-
  tool code), gated both behind `_WIN32`. This one never surfaced on
  Linux because `StackDump.cpp`'s own compile already failed earlier
  in the same translation unit via the already-known `DbgHelpLoader.h`
  gap, masking the second issue - macOS's Clang apparently produces
  this specific diagnostic before that one, or continues further per
  translation unit; either way, the lesson is that "0 new errors on
  Linux" doesn't mean "0 new errors," just "0 new errors *visible*
  before the first fatal error in each translation unit." Also fixed:
  `ini.cpp`/`TARGA.cpp`/`GameMemoryNull.cpp`'s `<malloc.h>` (same
  pattern as `FastAllocator.h`, swept proactively across the rest of
  the reachable tree - `GameMemory.h`'s own copy and two others were
  already correctly gated behind build options that default off, no
  fix needed there); `StdLocalFileSystem.cpp`'s `for (auto& p : path)`
  over a `std::filesystem::path` - libc++ (macOS/Clang) returns path
  components by value from its iterator, which a non-const reference
  cannot bind to, while libstdc++ (Linux/GCC) is more permissive
  (changed to `const auto&`); `PeerThread.cpp`'s `socklen_t`, whose
  declaration was reaching the file only transitively via some other
  header on Linux and not at all on macOS (added an explicit
  `<sys/socket.h>` include).

  **Result: both `linux-native.yml` and `macos-native.yml` now pass
  on real CI with exactly 17 errors per target (34 total) on each
  platform - identical, and every single one is an already-catalogued,
  deliberately deferred item** (`WebBrowser.h`/`ftp.h` real COM/ATL
  work - Phase 7, `DbgHelpLoader.h`'s `imagehlp.h` - dead weight when
  crash-dumps are off, `BezierSegment.h`'s `d3dx8math.h`,
  `IMEManager.cpp`'s `mbstring.h` - Phase 4 IME). No pointer-width,
  build-system, or platform-specific gap remains anywhere in the
  currently-reachable subset of the codebase, verified on Windows (0
  errors, every commit), Linux (real CI, not just WSL2), and macOS
  (real CI) simultaneously - the first time this port has had all
  three platforms green in the same session. WSL2 local testing alone
  would not have caught 10 of these ~12 fixes in this draft; this is
  the strongest evidence yet for why the macOS-CI-validation habit
  (established back in the Phase 3 spike's GLFW investigation) matters
  as a standing practice, not a one-off.
