# Milestone 7 plan (Draft 28, standalone review copy — not yet merged into native-port-plan.md)

## Draft 28: Milestone 7 plan - rung 2a, the portable file-system stack +
real game-asset access (the first GameEngine code ever to RUN on POSIX,
and the first pixels ever rendered from a real `.big` archive on GL) -
planned against Milestone 6's actual delivered code (branch
`native-port-plan`, clean, all of Draft 26/27's work landed).

**Why rung 2 as Draft 27 stated it is NOT one milestone, stated up
front with the code as evidence.** Draft 27's forward reference named
rung 2 as "the portable engine skeleton (`main()`/`GameEngine::execute`/
`Win32GameEngine`-equivalent factories + `Win32BIGFileSystem`/
`Win32LocalFileSystem` for real assets)". The code says that bundle is
two very different-sized things. The entry chain is real and small:
`WinMain` (`GeneralsMD/Code/Main/WinMain.cpp:855-1017`) does critical
sections + `initMemoryManager` (`:875-882`), window creation (`:939`,
skipped when `TheGlobalData->m_headless`), then `GameMain()` (`:986`);
`GameMain` (`GeneralsMD/Code/GameEngine/Source/Common/GameMain.cpp:
39-71`) is just `CreateGameEngine()` (`WinMain.cpp:1022-1032`, `NEW
Win32GameEngine`) → `init()` → `execute()`. But `GameEngine::init()`
(`GeneralsMD/Code/GameEngine/Source/Common/GameEngine.cpp:353-836`) is
monolithic: it constructs, in order, the file systems (`:403-445`),
`TheWritableGlobalData` INI load (`:456`), `TheGameText` (`:532`),
`TheAudio` (`:560`), `TheParticleSystemManager` (`:581`), a dozen INI
stores, `TheGameClient` (`:634` — `createGameClient()` = `NEW
W3DGameClient`, `Win32GameEngine.h:91`, which is rung 3's entire
display/client layer), `TheAI`/`TheGameLogic` (`:648-649`), `TheRadar`
(`:654`), `TheGameResultsQueue` (`:689`, GameSpy threads). There is no
partial-init path; `m_headless` swaps in dummies only for audio/radar/
particles (`:560`, `:581`, `:654`) and still constructs the real
`W3DGameClient`. So a real, non-faked `GameEngine::execute()` on POSIX
requires the rung-3 client layer (plus text and the INI closure) to
exist first. Rung 2 therefore splits:

- **rung 2a (THIS milestone)**: the file-system stack — the one part of
  `GameEngine::init()`'s spine (`:403-445`: `TheFileSystem`,
  `TheLocalFileSystem`, `TheArchiveFileSystem`) that is genuinely
  severable, already ~portable, and independently verifiable against
  real `.big` bytes and real pixels.
- **rung 2b (LATER, after or alongside rung 3)**: `main()` +
  `CreateGameEngine`-equivalent + `execute()`. Note for that future
  milestone: `GameEngine.cpp`/`GameEngine.h`/`GameMain.cpp`/`WinMain.cpp`
  are all still per-tree and genuinely diverged (GameEngine.cpp: 1174
  lines GeneralsMD vs 1120 Generals, 354-line diff) — rung 2b needs its
  own unify-before-porting pass, same as rung 3's.

This ordering correction (2a → 3 → 2b, not 2 → 3) is this draft's main
push-back on Draft 27's sketch, and it is exactly the "smaller, real,
verifiable" preference this port has applied at every prior fork.

**Milestone 7 scope statement.** Four jobs: (1) make the already-in-tree
portable file-system layer real — the `StdDevice` twins
(`StdLocalFileSystem`/`StdLocalFile`/`StdBIGFileSystem`/`StdBIGFile`)
plus the Core-unified Common layer (`FileSystem`/`ArchiveFileSystem`/
`ArchiveFile`/`File`/`LocalFile`/`RAMFile`/`StreamingArchiveFile`)
executed for the first time ever on POSIX, behind a new headless
`ctest` harness that authors a real `.big` on disk and proves
open/list/info/precedence/streaming semantics byte-for-byte — the first
GameEngine code ever to run (not merely compile) on POSIX; (2) fix the
latent bugs already found in that never-executed layer (finding 6); (3)
unify `W3DFileSystem`/`GameFileClass` — the real seam that points
WW3D's `_TheFileFactory` at `TheFileSystem` — into `Core/` (it is
per-tree and diverged today) and make it portable; (4) the exit
harness: `Tests/RenderGameAssets/`, Milestone 6's engine-driven frame
loop where the textured quad's `.w3d` and `.tga` bytes exist ONLY
inside a test-authored `.big` archive, loaded through the engine's own
`W3DFileSystem` → `TheFileSystem` → `StdBIGFile` → `RAMFile` chain,
pixel-verified. Goal-state: every byte on screen came out of a real
BIG archive through the engine's own file stack — the missing "image
from shipped-game-asset-format" capability Draft 26/27 explicitly
listed, with only the archive's *contents* being test-authored.

**Key findings, verified against current code (file:line):**

1. **The portable file-system equivalents already exist, already build
   on POSIX, and have never once executed.**
   `Core/GameEngineDevice/CMakeLists.txt:208-220` appends the four
   StdDevice TUs *outside* the `if(WIN32)` gate (the gate's own header
   comment, `:1-6`, calls the block "deliberately NOT inside this
   guard - it already works cross-platform"); they are C++17
   `<filesystem>`-based with explicit backslash→slash conversion and
   case-insensitive path fixup (`StdLocalFileSystem.cpp:45-131`). The
   34-error POSIX baseline's catalogued per-file breakdown (Draft 21:
   atlbase 6, winsock 4, imagehlp 3, d3dx8math 3, mbstring 1, per
   tree) contains no StdDevice or file-system TU — so all of it
   compiles clean on Linux and macOS today. But no game code ever
   constructs them: the only factory sites in the tree construct the
   Win32 twins (`Win32GameEngine.h:95-96`), and grep finds zero `NEW
   StdLocalFileSystem`/`NEW StdBIGFileSystem` anywhere outside
   `StdBIGFileSystem.cpp:89`'s own `NEW StdBIGFile`. The plan's own
   Phase-0 inventory already concluded (native-port-plan.md:77-82)
   that "swapping the factory really is all that's needed" for file
   I/O. This milestone is that swap, made real and proven.

2. **The Common layer beneath them is Core-unified and in the
   POSIX-compiling set.** `Core/GameEngine/CMakeLists.txt:643-676`
   lists `ArchiveFile.cpp`/`ArchiveFileSystem.cpp`/`AsciiString.cpp`/
   `Debug.cpp`/`File.cpp`/`FileSystem.cpp`/`LocalFile.cpp`/
   `LocalFileSystem.cpp`/`RAMFile.cpp`/`registry.cpp`/
   `StreamingArchiveFile.cpp`/`SubsystemInterface.cpp` — all compiled
   into both per-tree GameEngine libs whose POSIX builds carry only
   the 17 catalogued errors each, none in these files. Semantics that
   the harness must prove (not assume): `FileSystem::openFile` is
   local-first, archive-fallback (`FileSystem.cpp:175-220`);
   `StdBIGFile::openFile` serves `RAMFile` for plain reads and
   `StreamingArchiveFile` for `File::STREAMING` (`StdBIGFile.cpp:
   61-101`); `StdBIGFileSystem::init` requires `TheLocalFileSystem`
   first (`StdBIGFileSystem.cpp:53-59` asserts it) and scans
   `loadBigFilesFromDirectory("", "*.big")` — i.e. the working
   directory. Registry lookups on POSIX already fail-soft to defaults
   (`registry.cpp:34-46`'s own comment documents this contract for
   exactly this caller, `StdBIGFileSystem.cpp:61-69`'s
   RTS_ZEROHOUR-only Generals-install lookup).

3. **`W3DFileSystem` is the real WW3D↔GameEngine seam, it is
   separable from `W3DDisplay`, and it is per-tree/diverged — a
   unify-then-port candidate sized for this milestone.** Constructing
   it is the installation: `W3DFileSystem::W3DFileSystem()` sets
   `_TheFileFactory = this` (`GeneralsMD/.../W3DDevice/GameClient/
   W3DFileSystem.cpp:438-445`), replacing WWLib's default factory
   (`Core/Libraries/Source/WWVegas/WWLib/ffactory.cpp:49`) that all
   six existing harnesses implicitly use. The game does this from
   `W3DDisplay::init` (`W3DDisplay.cpp:754`) but nothing in the class
   requires `W3DDisplay` — WorldBuilder proves separability by
   installing its own subclass standalone (`GeneralsMD/Code/Tools/
   WorldBuilder/src/WorldBuilder.cpp:408`). Size/divergence: 547
   lines (ZH) vs 520 (Generals); the diff is ZH's localized-directory
   lookup (`W3DFileSystem.cpp:171-183`, `GetRegistryLanguage()`
   paths) and `reprioritizeTexturesBySize` (`:442-443`, `:474-546`) —
   RTS_ZEROHOUR-guard material, exactly the shape of prior Core
   unifications (`W3DView.cpp` precedent:
   `Core/GameEngineDevice/CMakeLists.txt:185`). Portability: its
   link/runtime deps are `TheFileSystem`, `TheArchiveFileSystem`
   (`friend_getArchivedDirectoryInfo`, `:476`), `TheGlobalData`
   (null-guarded at `:281`, `:303` — safe to leave null in the
   harness), `GetRegistryLanguage` (portable), plus one `#include
   <io.h>` (`:52`) with zero `_access`/io.h-API callers in the file
   (verified by grep) — dead-include residue to gate or drop.
   `GameFileClass` maps bare asset names into `Art\W3D\` /
   `Art\Textures\` (`:196-209`), which is what lets the exit harness
   ask for `quad.w3d`/`quad.tga` by name and have the bytes come out
   of the archive.

4. **The allocator coupling is a real, verified mutual-exclusion trap —
   checked now, at planning time, precisely to avoid Draft 26's class
   of late correction.** GameEngine file-system TUs allocate via
   `newInstance`/`MSGNEW`, which require the real memory manager
   (`GameMemory.cpp:3420` `initMemoryManager`; a lazy
   `initMemoryManagerLeakVersion` fallback exists at `:3490-3505`).
   `GameMemory.cpp` also defines the global `operator new`/`new[]`
   family and the W3D pool glue `createW3DMemPool` (`:3274-3360`,
   `:3560`). `core_wwstub`'s `wwallocstub.cpp:25-86` defines those
   SAME symbols malloc-backed — and every existing GL harness links
   `core_wwstub` (`Tests/RenderWW3DFrame/CMakeLists.txt`). One link
   cannot contain both. Consequence: the exit harness swaps
   `core_wwstub`'s allocator half OUT and the real
   `GameMemory.cpp`(+`GameMemoryInit.cpp`) IN — which also means the
   entire WW3D render stack runs on the game's real memory manager on
   POSIX for the first time (a real deliverable in itself, and a real
   risk surface: budget for it in verification, don't discover it in
   a stack trace). `wwdebugstub.cpp` (the other `core_wwstub` member)
   is still needed — plan for splitting the stub lib or direct-listing
   `wwdebugstub.cpp` in the new harness. Relatedly, thread-safety
   globals are per-tree and null by default
   (`GeneralsMD/.../System/CriticalSection.cpp:30`); `WinMain.cpp:
   875-882` wires five real `CriticalSection` objects before
   `initMemoryManager`, and the texture loader is a real second
   thread in these harnesses since M4 — the harness prologue must
   mirror `WinMain`'s sequence. That prologue is, deliberately, the
   seed of rung 2b's portable `main()`.

5. **A `TheSubsystemList`/`TheAudio` link-closure wrinkle is known in
   advance because these TUs will be direct link inputs.** Draft 26's
   late lesson (directly-linked `.o` files must resolve every
   external, dead code or not) applies to every TU the new harnesses
   compile directly. Already identified: `SubsystemInterface.cpp`
   references `TheSubsystemList` (`SubsystemInterface.cpp:51-52,
   59-60`, null-guarded at runtime) whose definition lives inside the
   monolithic per-tree `GameEngine.cpp:156` — unacceptable to drag;
   and `StdBIGFileSystem::closeArchiveFile` references `TheAudio`
   (defined in `GameAudio.cpp:142`, another heavy TU). Both need
   harness-local link-stub definitions (`SubsystemInterfaceList
   *TheSubsystemList = nullptr;` / `AudioManager *TheAudio =
   nullptr;`) with loud comments — the established
   `anim_sound_link_stub` pattern (Draft 26 finding 4), and both are
   null-guarded (or unexercised: the harness never closes `Music.big`)
   at runtime. The full closure (does `AsciiString.cpp` pull
   `UnicodeString.cpp`? does anything pull `Xfer`?) is for the linker
   to enumerate — open question 1, expected 0-5 additions.

6. **Two latent defects already found in the never-executed layer;
   fixing them is part of making it real.** (a)
   `StdLocalFileSystem.cpp:112` calls `filename.string().c_str()`
   where `filename` is `const Char *` — invalid C++ that only
   compiles because `DEBUG_LOG` compiles away in the release-style
   configs every verification so far has used; any `DEBUG_LOGGING`
   build of this TU fails. Same class as the Phase-1
   `getWSAErrorString` find (native-port-plan.md:568-572); audit all
   `DEBUG_LOG` bodies in the four Std TUs while there. (b)
   `W3DFileSystem.cpp:52`'s `<io.h>` (finding 3) breaks any POSIX
   compile of the unified file despite having no callers. Neither is
   speculative — both are in code this milestone makes reachable.

7. **The exit harness's `.big` authoring is fully specified by the
   engine's own reader, and every asset-authoring ingredient already
   exists.** `StdBIGFileSystem::openArchiveFile`
   (`StdBIGFileSystem.cpp:81-180`) documents the format by
   consumption: `"BIGF"` magic, byte-swapped (`betoh`) little/big
   endian fields (archive size read raw and unused; entry count,
   per-file offset and size big-endian), directory at seek `0x10`,
   nul-terminated full paths with `\` or `/` separators accepted
   (`:155` handles both). `Tests/RenderWW3DFrame/main.cpp:196-221`
   authors real TGA bytes and `:230+` real `.w3d` chunk bytes (M5's
   template) — the new harness reuses both, writing them into an
   archive instead of loose temp files. ctest's `WORKING_DIRECTORY`
   property covers `init()`'s cwd-relative `*.big` scan so the
   engine's real `init()` path runs unmodified.

**Design decisions:**

- **The Std twins ARE the port; `Win32LocalFileSystem`/
  `Win32BIGFileSystem` stay Windows-only and untouched** (findings 1,
  2). No new file-I/O implementation is written. This is the
  plan-document's founding pattern (native-port-plan.md:8-12 cites
  `StdLocalFileSystem` as the model) finally cashed in. The Win32
  twins remain the Windows factories' choice — zero Windows behavior
  change, the M1 seam discipline.
- **Two harnesses, semantics before pixels.** A headless
  `Tests/GameFileSystem/` proves file semantics byte-for-byte with no
  GL dependency (and becomes the first GameEngine code ever *run* on
  POSIX); `Tests/RenderGameAssets/` then proves the full chain to
  pixels. Failures localize to the right layer instead of surfacing
  as "wrong pixels".
- **`W3DFileSystem` is unified and ported NOW, not left for rung 3**
  (finding 3): it is small, separable (WorldBuilder precedent), and
  it is the difference between the exit harness using the engine's
  own bridge versus inventing harness-only file-factory scaffolding —
  the same "converge onto the game's real path" principle that
  justified ending Trap 1 in M6. This also shrinks rung 3's
  unify-first pile by one file.
- **The exit harness runs on the real `GameMemory`, not the malloc
  stub** (finding 4): the mutual-exclusion is forced anyway, and
  running the proven render loop over the real allocator is exactly
  the kind of integration this port wants surfaced by a harness, not
  by the eventual game binary. The six existing harnesses keep
  `core_wwstub` untouched.
- **Test-authored archives only; no retail assets anywhere.** The
  repo/CI ships no game data; the harness authors its `.big` at
  runtime the same way M5/M6 authored `.w3d`/`.tga` bytes. Honest
  consequence: retail-`.big` quirks (multi-thousand-entry
  directories, `Textures.big`/`TexturesZH.big` shadowing,
  `INIZH.big` skip logic) stay unverified — open question 6.
- **Texture loads stay on M6's synchronous path**
  (`WW3D::Set_Thumbnail_Enabled(false)` + `Set_Texture_Bitdepth(32)`,
  the Draft 27 precedent): the first archive-backed texture read
  happens on the main thread. Background-thread archive reads (the
  loader pthread seeking the shared archive `File` handle) are a
  real, separate concurrency question deferred with a loud comment —
  open question 5.
- **Move-then-port discipline continues** for the `W3DFileSystem`
  unification (Draft 21's sorted-multiset no-loss/no-duplicate
  method, MSVC-verified on both trees before any POSIX compile
  depends on it).

**Explicit non-goals (deliberately NOT this milestone):**
rung 2b — portable `main()`, `GameEngine`/`GameMain`/`WinMain`
unification, `GameEngine::execute`, `CreateGameEngine`-equivalents,
`serviceWindowsOS` replacement (deferred to after rung 3, per the
ordering correction above); rung 3 — `W3DDisplay`/`W3DScene`/`W3DView`
and their unify-first pass (Draft 26's non-goal list stands);
`TheWritableGlobalData`/INI loading/`CommandLine`/`TheGameText`/
`MapCache`/`loadMods` (`GameEngine.cpp:456-527`, `:501` — all need the
INI closure, later); registry-to-config-file replacement (Phase 7 —
POSIX keeps registry.cpp's fail-soft defaults, including
`StdBIGFileSystem.cpp:61-69`'s empty Generals install path, whose
`DEBUG_ASSERTCRASH` "Be 1337!" only exists in debug configs); audio
(TheAudio stays a null link-stub; `closeArchiveFile`'s `Music.big`
special case unexercised); networking, input, fonts, IME, fullscreen
(standing Phase 6/7/4 deferrals); porting the Win32 file-system twins
(permanently unnecessary per finding 1); retail-asset validation in CI
(no assets to ship — see open question 6).

**Implementation ordering** (after every step: WSL2 scoped baseline
`--target g_gameenginedevice z_gameenginedevice -- -k 0` holds 34/34,
real MSVC win32 rebuild at 0 new errors, all existing `ctest` entries
re-run when anything they link is touched — Drafts 25/26 standing
rules):

1. **Std-layer latent-bug fixes** (finding 6a): correct
   `StdLocalFileSystem.cpp:112-113`'s invalid `filename.string()`
   (log the `const Char*` or the built path), audit every `DEBUG_LOG`
   in the four Std TUs by compiling once with `DEBUG_LOGGING` forced
   on locally. Small, independently buildable, zero behavior change
   in default configs.
2. **`Tests/GameFileSystem/` — the headless harness.** Prologue
   mirrors `WinMain.cpp:875-882` (five real `CriticalSection`s +
   `initMemoryManager()`), then constructs `StdLocalFileSystem` →
   `StdBIGFileSystem` → `FileSystem` in `GameEngine::init`'s own
   order (`GameEngine.cpp:403-445`, minus the subsystem list), with
   harness-local loud link-stubs for `TheSubsystemList`/`TheAudio`
   (finding 5). Checks: (1) Windows-style path fixup — open
   `Data\INI\Foo.ini` by backslashed, wrong-case name against a
   mixed-case on-disk tree, byte-compare; (2) directory listing with
   masks + subdirectories + `getFileInfo` sizes; (3) author a real
   multi-file `.big` (nested paths, `\` separators, big-endian
   fields per finding 7) and verify the loaded directory tree; (4)
   archive reads byte-exact via `RAMFile` AND via `File::STREAMING`
   (`StreamingArchiveFile`); (5) local-shadows-archive precedence
   (`FileSystem.cpp:175-220`) proven both ways; (6) clean teardown,
   `ctest` exit 0, stable across 5+ runs. **Sequencing note, stated
   honestly**: this step's true TU closure is enumerated by the
   linker, not this plan (finding 5, open question 1) — budget for
   0-5 additional Core TUs or stubs, and record whatever the linker
   proves in the close-out draft.
3. **`W3DFileSystem` unification** (finding 3): per-tree copies →
   `Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DFileSystem.cpp`
   with `RTS_ZEROHOUR` guards for the localization lookup and
   `reprioritizeTexturesBySize`; multiset no-loss verification; both
   MSVC trees rebuild 0-error, byte-identical behavior; `<io.h>`
   dropped or `_WIN32`-gated; the TU joins the un-gated portable
   block (finding 1's precedent) so POSIX game targets compile it
   too — verify the 34/34 baseline is genuinely unchanged after the
   reachable-set growth. Independently buildable; no harness depends
   on it yet.
4. **`Tests/RenderGameAssets/` — the exit harness.** Step 2's
   prologue + file systems; construct `TheW3DFileSystem` (now
   Core-unified) — `_TheFileFactory` swap happens in its constructor,
   the engine's own mechanism (`W3DFileSystem.cpp:438-445`); then
   M6's engine-driven loop (`WW3D::Init` → `Set_Render_Device` →
   `Begin_Render`/`Render`/`End_Render`, windowed=1, thumbnail
   disabled per Draft 27 precedent) with `quad.w3d` +`quad.tga`
   authored ONLY into the `.big` under `Art\W3D\`/`Art\Textures\`
   (finding 3's `GameFileClass` mapping, finding 7's authoring).
   Checks: init round-trip; asset-manager load by bare name succeeds
   (proving factory→`TheFileSystem`→archive resolution); a
   pixel-verified frame with the authored texel color at the
   predicted position (byte-provenance: those bytes exist nowhere on
   disk outside the archive); negative control — the same run with
   the archive absent must fail to load (guards against silent
   fallback to loose files); full teardown including
   `~W3DFileSystem`'s `_TheFileFactory` restore and `WW3D::Shutdown`.
   This target links real `GameMemory.cpp`+`GameMemoryInit.cpp` and
   NOT `core_wwstub`'s allocator (finding 4) — `wwdebugstub.cpp`
   direct-listed or the stub lib split. **Depends on steps 2 AND 3.**
5. **CI wiring + the real run.** Both harnesses into
   `linux-native.yml` (`GameFileSystem` needs no display;
   `RenderGameAssets` behind `xvfb-run` like its six siblings),
   `workflow_dispatch` advisory posture; all 9 `ctest` entries green;
   fold in Draft 27's still-open honesty item — an actual
   `workflow_dispatch` CI execution (M6's own pending exit-criterion
   run plus this milestone's) is this milestone's exit criterion too.

**What this milestone does NOT yet make possible, honestly:** still no
game executable on POSIX — no `main()`, no `GameEngine::execute`, no
menus, no INI-driven subsystems, no text, no input, no audio; a person
sees two test harnesses, one of them rendering one quad whose texture
came out of an archive. What it does make possible: the complete asset
path a real frame needs — archive discovery → BIG directory tree →
`RAMFile`/streaming reads → `_TheFileFactory` → asset manager →
pixels — is the engine's own code on both platforms, the real memory
manager runs under the whole stack, and rung 3 (`W3DDisplay`, which
begins by constructing exactly this milestone's `W3DFileSystem`,
`W3DDisplay.cpp:754`) has its file-and-asset floor already proven.

**Open questions for the implementer:**
1. **Step 2's true link closure** — candidates beyond the listed TUs:
   `UnicodeString.cpp` (AsciiString↔UnicodeString conversions),
   `Xfer`, `GameCommon.cpp`; the linker enumerates the truth; add
   portable TUs rather than stubbing wherever the TU is small and
   clean (Draft 24 precedent), stub only heavyweight globals
   (`TheAudio`, `TheSubsystemList`) with loud comments.
2. **Per-tree flavor for the harnesses**: GeneralsMD headers +
   `RTS_ZEROHOUR=1` (matching every existing harness's GeneralsMD
   usage) is the recommendation; confirm the `zi_always`-equivalent
   defines the GameEngine TUs need when compiled outside their
   libraries, and that `PreRTS.h` (`Include/Precompiled`) resolves.
3. **`core_wwstub` handling in step 4**: split the lib
   (`wwdebugstub` vs `wwallocstub`) vs direct-listing
   `wwdebugstub.cpp` — decide by which keeps the other six harnesses'
   links bit-identical.
4. **Windows coverage for `Tests/GameFileSystem`**: the Std layer
   compiles on Windows too; running this harness there would be its
   first Windows test coverage but breaks the `NOT WIN32` harness
   convention (whose rationale — "Windows already has the real D3D8
   backend" — does not apply to a backend-free file test). Default to
   `NOT WIN32` for consistency; revisit deliberately.
5. **Background-thread archive reads**: once thumbnails are re-enabled
   (some future milestone), the TextureLoader pthread will
   `openFromArchive` against the shared per-archive `File` handle
   concurrently with the main thread. Whether the engine's own
   Windows behavior relies on external serialization was NOT
   determined by this planning pass — flagged as an honest unknown to
   resolve before that milestone, not now.
6. **Retail-asset spot check**: a local-only, manual (never CI) run
   of `Tests/GameFileSystem` against a real installed `Textures.big`
   would retire the "test-authored archives only" caveat cheaply —
   worth doing once during implementation if an install is at hand;
   record the result either way.
7. **`W3DFileSystem` POSIX game-target inclusion** (step 3): adding
   it to the un-gated Core block grows the POSIX-reachable set of the
   *game* targets, not just the harness — if any transitively-included
   header surprises appear (GlobalData.h/MapObject.h are believed
   clean, both already compile in other POSIX TUs, but "believed" is
   not "verified for this include chain"), fall back to
   harness-only compilation for this milestone and record it.

---

### Critical Files for Implementation
- `Core/GameEngineDevice/Source/StdDevice/Common/StdLocalFileSystem.cpp` (and siblings `StdBIGFileSystem.cpp` / `StdBIGFile.cpp` / `StdLocalFile.cpp`)
- `Core/GameEngine/Source/Common/System/FileSystem.cpp` (plus `ArchiveFileSystem.cpp` / `RAMFile.cpp` / `StreamingArchiveFile.cpp` in the same directory)
- `GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/GameClient/W3DFileSystem.cpp` (unify with the Generals copy into `Core/GameEngineDevice`)
- `Core/GameEngine/Source/Common/System/GameMemory.cpp` (allocator swap vs `Core/Libraries/Source/WWVegas/WWStub/wwallocstub.cpp`)
- `Tests/RenderWW3DFrame/main.cpp` (template for the two new harnesses, `Tests/GameFileSystem` and `Tests/RenderGameAssets`)
