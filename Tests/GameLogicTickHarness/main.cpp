// Phase 5(a) Milestone 10 (native port plan, Draft 34 "rung 2b-lite") - a
// headless TheAI->TheGameLogic->TheScriptEngine->TheTerrainLogic->
// ThePartitionManager tick harness on POSIX. Deliberately bypasses
// GameEngine::init()/execute()'s real display-layer dependencies -
// independent of W3DView.cpp/W3DDisplay.cpp entirely (Milestone 9/11's
// territory, not touched here). See docs/native-port-plan-rung2b-lite-draft.md
// (Draft 34) for the full design rationale, and
// .superpowers/sdd/m10-task1-spike-report.md for why the real, unmodified
// GameEngine.cpp is never compiled into this harness (its unconditional
// #include of GameNetwork/WOLBrowser/WebBrowser.h is genuinely,
// structurally ATL/COM-based - not a small shim).
//
// Construction order (this file's own main()), matching Draft 34 section 2's
// verified real order exactly:
//   NEW GlobalData()                                  - TheWritableGlobalData
//   TheWaterTransparency = newInstance(WaterTransparencySetting)
//   TheWeatherSetting     = newInstance(WeatherSetting)
//   TheCommandList  = NEW CommandList; TheCommandList->init()
//   TheSidesList    = NEW SidesList()
//   ThePlayerList   = NEW PlayerList()
//   TheRecorder     = createRecorder()
//   TheWeaponStore    = NEW WeaponStore()
//   TheLocomotorStore = NEW LocomotorStore()
//   TheVictoryConditions = createVictoryConditions()
//   TheBuildAssistant    = NEW BuildAssistant()
//   TheMessageStream = new MessageStream (GameEngine::createMessageStream()'s
//     own body, inlined here rather than calling through a GameEngine
//     instance we deliberately never construct - see link_stubs.cpp's
//     GameEngine::isTimeFrozen() comment for the parallel reasoning)
//   TheAI = NEW AI(); TheAI->init();
//   TheGameLogic = NEW GameLogic(); TheGameLogic->init();  <- runs the REAL,
//     unmodified GameLogic::init(), which itself real-constructs
//     ThePartitionManager/TheGhostObjectManager/TheTerrainLogic/TheScriptEngine
//     and calls GameLogic::reset() internally (Draft 34 s2).
//
//   TheSuperHackers @info Milestone 10 implementation finding, a disclosed
//   deviation from the draft's own §2 code block (which wrote
//   `NEW W3DGameLogic()`): the base GameLogic class itself is NOT abstract -
//   it has zero pure virtuals, and its own createTerrainLogic()/
//   createGhostObjectManager() (GameLogic.cpp:4735-4747) already return the
//   plain TerrainLogic base class and GhostObjectManagerDummy/GhostObjectManager,
//   exactly the lightweight objects this harness needs. W3DGameLogic overrides
//   createTerrainLogic() to hardcode `NEW W3DTerrainLogic`
//   (W3DGameLogic.h:61), and W3DTerrainLogic.cpp - like GameEngine.cpp and
//   W3DGhostObject.cpp - has never been compiled on this port's Linux
//   toolchain before (WIN32-gated in Core/GameEngineDevice/CMakeLists.txt) and
//   turns out, on inspection, to unconditionally pull in the full WW3D2/DX8
//   rendering closure via HeightMap.h (rendobj.h, dx8vertexbuffer.h,
//   dx8wrapper.h, shader.h, vertmaterial.h) - the SAME closure that produces
//   this port's 34 known pre-existing device-layer compile errors. Using the
//   plain GameLogic class instead sidesteps that closure entirely while still
//   running the exact same real, unmodified GameLogic::init()/reset()/update()
//   code this milestone's payoff actually is - GameLogic (not W3DGameLogic) IS
//   the class whose methods are under test here; the *Logic tree never needed
//   the W3D-prefixed subclass to prove the point. Matches Milestone 9's own
//   precedent of using the base TerrainLogic class for the same reason (Draft
//   32 finding 6).
//
// TheGameEngine is NOT constructed as a real (sub-)object at all this
// milestone - see link_stubs.cpp for the full rationale: GameLogic::update()'s
// only real call site (TheGameEngine->isTimeFrozen(), GameLogic.cpp:3798) is a
// STATIC member function, so the object expression is evaluated but never
// dereferenced - TheGameEngine can stay a real, correctly-typed, permanently
// null GameEngine* and the call is still genuinely safe. This sidesteps
// GameEngine.cpp's WebBrowser.h/ATL blocker entirely (the real .cpp is never
// compiled into this harness), and is a smaller, more disclosed deviation
// than the pre-spike draft's own "harness-local GameEngine subclass" plan
// (task breakdown item 2), which assumed constructing a live GameEngine-typed
// object would be needed. It is not: isTimeFrozen() is static.
//
// Exit criterion (Draft 34 section 4): call TheGameLogic->UPDATE() N=5 times;
// after each call, assert TheGameLogic->getFrame() == tickIndex (the real,
// unmodified GameLogic::update()'s own frame counter, not a harness-maintained
// one) and TheGameLogic->hasUpdated() == TRUE.
#include "PreRTS.h"

#include "Common/AsciiString.h"
#include "Common/CriticalSection.h"
#include "Common/GameMemory.h"
#include "Common/NameKeyGenerator.h"
#include "GameLogic/RankInfo.h"
#include "GameClient/GameText.h"
#include "Common/GlobalData.h"
#include "Common/GameEngine.h"
#include "Common/MessageStream.h"
#include "Common/PlayerList.h"
#include "Common/Recorder.h"
#include "Common/BuildAssistant.h"
#include "Common/Override.h"
#include "GameClient/Water.h"
#include "GameClient/Snow.h"
#include "GameLogic/SidesList.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/Locomotor.h"
#include "GameLogic/VictoryConditions.h"
#include "GameLogic/AI.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/GhostObject.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/TerrainLogic.h"
#include "GameClient/GameClient.h"
#include "Common/FramePacer.h"
#include "Common/Diagnostic/SimulationMathCrc.h"

#include <cstdio>
#include <cstdlib>

namespace
{
	// TheSuperHackers @info Milestone 10 implementation finding: a real gdb
	// backtrace showed GameLogic::update()'s own body (GameLogic.cpp:3788,
	// `TheGameClient->setFrame(now);`) unconditionally dereferencing
	// TheGameClient - exactly the harness-local minimal GameClient subclass
	// Draft 34's task breakdown item 3 already anticipated (this part of the
	// draft's own plan needed no correction, unlike the GameEngine stand-in).
	// GameClient itself is abstract (13 pure virtuals: createRayEffectByTemplate/
	// addScorch/friend_createDrawable/setTeamColor/setTextureLOD/
	// notifyTerrainObjectMoved/createGameDisplay/createInGameUI/
	// createWindowManager/createFontLibrary/createDisplayStringManager/
	// createVideoPlayer/createTerrainVisual/createKeyboard/createMouse/
	// createSnowManager/setFrameRate - GameClient.h:122-199), so a concrete
	// subclass is required (unlike GameLogic, which turned out to have zero
	// pure virtuals). Every factory returns nullptr (never invoked - this
	// harness never calls TheGameClient->init()); every setter is a trivial
	// no-op. Matches the draft's own "trivial stub body... never overrides or
	// calls init()/update()/step()/draw()" plan exactly.
	class HarnessGameClient : public GameClient
	{
	public:
		virtual void createRayEffectByTemplate(const Coord3D*, const Coord3D*, const ThingTemplate*) override {}
		virtual void addScorch(const Coord3D*, Real, Scorches) override {}
		virtual Drawable *friend_createDrawable(const ThingTemplate*, DrawableStatusBits) override { return nullptr; }
		virtual void setTeamColor(Int, Int, Int) override {}
		virtual void setTextureLOD(Int) override {}
		virtual void notifyTerrainObjectMoved(Object*) override {}

	private:
		virtual Display *createGameDisplay() override { return nullptr; }
		virtual InGameUI *createInGameUI() override { return nullptr; }
		virtual GameWindowManager *createWindowManager() override { return nullptr; }
		virtual FontLibrary *createFontLibrary() override { return nullptr; }
		virtual DisplayStringManager *createDisplayStringManager() override { return nullptr; }
		virtual VideoPlayerInterface *createVideoPlayer() override { return nullptr; }
		virtual TerrainVisual *createTerrainVisual() override { return nullptr; }
		virtual Keyboard *createKeyboard() override { return nullptr; }
		virtual Mouse *createMouse() override { return nullptr; }
		virtual SnowManager *createSnowManager() override { return nullptr; }
		virtual void setFrameRate(Real) override {}
	};

	// TheSuperHackers @info Milestone 15 (native port plan, Draft 39, "Phase
	// 8 rung 0") Task 1: the checked-in expected value for
	// SimulationMathCrc::calculate()'s first-ever real call on this port -
	// WSL2 GCC, build/linux-x64 (Release, -O3), the SAME toolchain/optimization
	// level this repo's own scoped-build verification step already builds
	// with (see docs/native-port-plan.md's Global Constraints). Draft 39's
	// own real 5-toolchain spike (WSL2 GCC -O0/-O2/-O3, real MSVC x64/x86)
	// already found this exact computation bit-identical across all five -
	// this constant is this harness's own regression tripwire against that
	// same real value, not a fresh, unverified guess.
	const UnsignedInt kExpectedSimulationMathCrc = 0x97B538BFu;

	bool g_AnyFailure = false;

	void Check(bool ok, const char* what)
	{
		if (ok) {
			printf("  %s: OK\n", what);
		} else {
			fprintf(stderr, "  %s: FAILED\n", what);
			g_AnyFailure = true;
		}
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IOLBF, 0);
	setvbuf(stderr, nullptr, _IOLBF, 0);

	// ---- Prologue: mirrors WinMain.cpp:875-882 exactly (unchanged from
	// every prior harness in this port). ----
	static CriticalSection critSec1, critSec2, critSec3, critSec4, critSec5;
	TheAsciiStringCriticalSection = &critSec1;
	TheUnicodeStringCriticalSection = &critSec2;
	TheDmaCriticalSection = &critSec3;
	TheMemoryPoolCriticalSection = &critSec4;
	TheDebugLogCriticalSection = &critSec5;

	initMemoryManager();

	// Literal nested block BEFORE shutdownMemoryManager() - Tests/GameFileSystem's
	// own found-by-gdb SIGSEGV lesson, carried over verbatim by every harness
	// since.
	{
		printf("=== Setup: real, INI-free construction of GlobalData + the\n");
		printf("    incidental real subsystems (Draft 34 section 1/2) ===\n");

		// TheSuperHackers @info Milestone 10 implementation finding, NOT in the
		// draft's own construction order (Draft 34 section 2): a real, running
		// gdb backtrace showed `PlayerList::PlayerList()` -> `Player::Player(0)`
		// -> `Player::init()` -> the NAMEKEY() macro -> `TheNameKeyGenerator->
		// nameToKey()` dereferencing a null `this` - TheNameKeyGenerator is a
		// real, load-bearing dependency of the default Player object PlayerList's
		// own constructor creates, missed by the draft's "few incidental
		// dependencies" framing since it never appears in GameLogic.cpp/AI.cpp/
		// ScriptEngine.cpp/PartitionManager.cpp/TerrainLogic.cpp's own bodies -
		// only in Player.cpp's, several calls deep from ThePlayerList's own
		// constructor. GameEngine.cpp's own real order confirms it is one of the
		// very FIRST subsystems constructed (GameEngine.cpp:406-407, right after
		// TheFileSystem, before TheGameLODManager/TheAudio/everything else) -
		// constructed here first for the same reason, real class, no INI (its
		// own init() is a trivial freeSockets()+m_nextID=1 reset).
		TheNameKeyGenerator = NEW NameKeyGenerator;
		TheNameKeyGenerator->init();
		Check(TheNameKeyGenerator != nullptr, "0z. TheNameKeyGenerator constructed (non-null) - real, load-bearing dependency of Player::init()'s NAMEKEY() calls, found via gdb");

		// TheSuperHackers @info Milestone 10 implementation finding, continued -
		// a second real gdb backtrace, same root cause class: Player::init()
		// also unconditionally calls Player::resetRank(), which dereferences
		// TheRankInfoStore->getRankInfo() on a null this. Real class, no INI
		// needed for construction (RankInfoStore::init() is just
		// `m_rankInfos.clear()` on an already-empty, default-constructed
		// std::vector - same "INI-free, zero rank data" shortcut as every
		// other store this harness constructs directly).
		TheRankInfoStore = NEW RankInfoStore;
		TheRankInfoStore->init();
		Check(TheRankInfoStore != nullptr, "0y. TheRankInfoStore constructed (non-null) - real, load-bearing dependency of Player::resetRank(), found via gdb");

		// TheSuperHackers @info Milestone 10 implementation finding, continued -
		// a third real gdb backtrace: createRecorder()'s real RecorderClass
		// constructor unconditionally calls init(), which calls
		// m_gameInfo.clearSlotList(), which calls GameSlot::setState(), which
		// dereferences TheGameText->fetch() on a null this. Real class, no
		// init()/CSF-file load: GameTextManager::fetch() (Core/GameEngine/
		// Source/GameClient/GameText.cpp:1245-1254) checks `m_stringInfo ==
		// nullptr` (true on an un-init()'d manager) and returns a safe default
		// (m_failed) before ever touching real string-table data - m_initialized
		// is only DEBUG_ASSERTCRASH'd, a no-op in this harness's release-style
		// build. Constructing without calling init() is genuinely safe here,
		// not merely convenient.
		TheGameText = CreateGameTextInterface();
		Check(TheGameText != nullptr, "0x. TheGameText constructed (non-null, un-init()'d - real, load-bearing dependency of RecorderClass::init(), found via gdb)");

		// ---- TheWritableGlobalData: real class, no INI (Milestone 7/8's own
		// established shortcut). ----
		TheWritableGlobalData = NEW GlobalData;
		Check(TheWritableGlobalData != nullptr, "0a. TheWritableGlobalData constructed (non-null)");
		Check(TheWritableGlobalData->m_headless == FALSE, "0b. m_headless == FALSE (real default, GlobalData.cpp:649)");

		// TheSuperHackers @info Milestone 10 (Draft 34) implementation finding,
		// NOT anticipated by the draft: GameLogic::init()'s real
		// createGhostObjectManager(TheGlobalData->m_headless) call, with
		// m_headless left at its default FALSE, constructs the real
		// W3DGhostObjectManager (Core/GameEngineDevice/.../W3DGhostObject.cpp) -
		// a file that, read in full during implementation, unconditionally
		// #includes WW3D2/rendobj.h, WW3D2/hlod.h, WW3D2/scene.h, WW3D2/matinfo.h,
		// W3DDevice/GameClient/{W3DModelDraw,W3DAssetManager,W3DDisplay,W3DScene}.h -
		// the full WW3D2 rendering closure, never previously attempted on this
		// port's Linux toolchain (like GameEngine.cpp, W3DGhostObject.cpp is
		// gated entirely inside Core/GameEngineDevice/CMakeLists.txt's `if(WIN32)`
		// block and has never been compiled on POSIX before). Pulling that whole
		// closure into a headless logic-only harness would be badly
		// disproportionate and would risk re-entangling this milestone with
		// Milestone 9/11's own rendering-layer work.
		//
		// Fix: set TheGlobalData->m_headless = TRUE here - this is not a
		// harness-only workaround but the SAME real, precedented flag the
		// engine's own command line already uses for exactly this purpose
		// (GeneralsMD/Code/GameEngine/Source/Common/CommandLine.cpp:417,
		// `TheWritableGlobalData->m_headless = TRUE;` for a `--headless` launch
		// option). With m_headless == TRUE, createGhostObjectManager(TRUE)
		// returns NEW GhostObjectManagerDummy (GhostObject.h:114-122) - a fully
		// inline, zero-dependency subclass of the real, already-portable,
		// already-compiling GhostObjectManager base class
		// (GeneralsMD/Code/GameEngine/Source/GameLogic/Object/GhostObject.cpp,
		// NOT WIN32-gated). Every other m_headless read reachable from this
		// harness's tick loop (GameLogic.cpp:1125,2308; both inside
		// startNewGame()/other methods this harness's plain UPDATE() loop never
		// reaches) is a safe, inert boolean feature flag either way - verified
		// by reading, not assumed.
		TheWritableGlobalData->m_headless = TRUE;
		Check(TheWritableGlobalData->m_headless == TRUE, "0b-2. m_headless set TRUE (real --headless flag, CommandLine.cpp:417) - selects GhostObjectManagerDummy, avoiding W3DGhostObject.cpp's WW3D2 rendering closure");

		// ---- TheWaterTransparency / TheWeatherSetting: real classes, no INI.
		// Draft 34 section 1's own finding: GameLogic::reset() (called
		// unconditionally from inside GameLogic::init()) unconditionally
		// dereferences these via deleteOverrides() on a null this if they are
		// left at their default-constructed nullptr - a real, previously-
		// unflagged crash risk this harness must route around, not INI content. ----
		TheWaterTransparency = newInstance(WaterTransparencySetting);
		TheWeatherSetting = newInstance(WeatherSetting);
		Check(TheWaterTransparency.getNonOverloadedPointer() != nullptr, "0c. TheWaterTransparency constructed (non-null)");
		Check(TheWeatherSetting.getNonOverloadedPointer() != nullptr, "0d. TheWeatherSetting constructed (non-null)");

		// ---- TheCommandList: real class, matches GameEngine::init()'s own
		// initSubsystem(TheCommandList, ...) call shape (construct + init()). ----
		TheCommandList = NEW CommandList;
		TheCommandList->init();
		Check(TheCommandList != nullptr, "0e. TheCommandList constructed (non-null)");

		// ---- TheSidesList: real class, needed because ScriptEngine::update()
		// unconditionally walks TheSidesList's side count (Draft 34 section 4
		// point 2) - without a real, non-null TheSidesList that line
		// dereferences a null pointer even with zero sides registered.
		// ThePlayerList is deliberately NOT constructed here - see the
		// construction-order note further down, right before TheGameLogic->init(). ----
		TheSidesList = NEW SidesList;
		Check(TheSidesList != nullptr, "0f. TheSidesList constructed (non-null)");

		// ---- TheRecorder: real class via the real createRecorder() factory -
		// GameLogic::update() calls TheRecorder->UPDATE() unconditionally. ----
		TheRecorder = createRecorder();
		Check(TheRecorder != nullptr, "0h. TheRecorder constructed (non-null)");

		// ---- TheWeaponStore / TheLocomotorStore: real classes, no INI -
		// GameLogic::update() calls both ->UPDATE() unconditionally
		// (GameLogic.cpp:3968-3969); both are real no-ops with empty stores. ----
		TheWeaponStore = NEW WeaponStore;
		TheLocomotorStore = NEW LocomotorStore;
		Check(TheWeaponStore != nullptr, "0i. TheWeaponStore constructed (non-null)");
		Check(TheLocomotorStore != nullptr, "0j. TheLocomotorStore constructed (non-null)");

		// ---- TheVictoryConditions / TheBuildAssistant: real classes via the
		// real createVictoryConditions() factory - GameLogic::update() calls
		// both ->UPDATE() unconditionally. ----
		TheVictoryConditions = createVictoryConditions();
		TheBuildAssistant = NEW BuildAssistant;
		Check(TheVictoryConditions != nullptr, "0k. TheVictoryConditions constructed (non-null)");
		Check(TheBuildAssistant != nullptr, "0l. TheBuildAssistant constructed (non-null)");

		// ---- TheMessageStream: real class - GameEngine::createMessageStream()'s
		// own body (`return MSGNEW(...) MessageStream;`) inlined here directly,
		// since this harness deliberately never constructs a live GameEngine
		// object to call that factory through (see link_stubs.cpp). ----
		TheMessageStream = NEW MessageStream;
		Check(TheMessageStream != nullptr, "0m. TheMessageStream constructed (non-null)");

		// ---- TheGameClient: the harness-local minimal subclass (see this
		// file's own header comment, near HarnessGameClient's definition) -
		// found via a real gdb backtrace: GameLogic::update()'s own body
		// unconditionally dereferences TheGameClient->setFrame() (GameLogic.cpp:3788).
		// Never call TheGameClient->init()/update()/step()/draw() - matches
		// Draft 34 task breakdown item 3 exactly. ----
		TheGameClient = NEW HarnessGameClient;
		Check(TheGameClient != nullptr, "0n. TheGameClient constructed (non-null, harness-local minimal subclass)");

		// ---- TheFramePacer: real class, already portable, already proven
		// cheap and safe by Milestone 9's Tests/RenderCameraTransform harness
		// (Draft 32's own finding) and confirmed again by Draft 34 section 3 -
		// GameLogic::update() unconditionally dereferences it
		// (GameLogic.cpp:3798,3800, `TheFramePacer->setTimeFrozen(...)` /
		// `TheFramePacer->isTimeFrozen()`), found via a real gdb backtrace. ----
		TheFramePacer = NEW FramePacer;
		Check(TheFramePacer != nullptr, "0o. TheFramePacer constructed (non-null)");

		// ---- TheAI: real class, real order (GameEngine.cpp's own real order:
		// TheAI before TheGameLogic, confirmed load-bearing by GameLogic::reset()
		// calling TheAI->reset() - Draft 34 section 2). ----
		printf("=== Check 1: real AI (TheAI before TheGameLogic, real order) ===\n");
		TheAI = NEW AI;
		TheAI->init();
		Check(TheAI != nullptr, "1a. TheAI constructed (non-null)");

		// ---- TheGameLogic: the real, base GameLogic class (see this file's
		// header comment for why not W3DGameLogic), running the REAL, unmodified
		// GameLogic::init() - this is the milestone's actual payoff call. It
		// internally real-constructs ThePartitionManager, TheGhostObjectManager
		// (GhostObjectManagerDummy, since m_headless == TRUE - see the comment
		// above), TheTerrainLogic (real, base TerrainLogic via
		// GameLogic::createTerrainLogic()'s own default), and TheScriptEngine,
		// then calls GameLogic::reset() internally. ----
		printf("=== Check 2: real GameLogic::init() (the milestone's payoff call) ===\n");
		TheGameLogic = NEW GameLogic;
		Check(TheGameLogic != nullptr, "2a. TheGameLogic constructed (non-null)");

		TheGameLogic->init();

		Check(ThePartitionManager != nullptr, "2b. ThePartitionManager real-constructed by GameLogic::init() (non-null)");
		Check(TheGhostObjectManager != nullptr, "2c. TheGhostObjectManager real-constructed by GameLogic::init() (non-null)");
		Check(TheTerrainLogic != nullptr, "2d. TheTerrainLogic real-constructed by GameLogic::init() (non-null)");
		Check(TheScriptEngine != nullptr, "2e. TheScriptEngine real-constructed by GameLogic::init() (non-null)");
		Check(TheGameLogic->getFrame() == 0, "2f. getFrame() == 0 after init() (GameLogic::reset()'s own m_frame = 0)");
		Check(TheGameLogic->hasUpdated() == FALSE, "2g. hasUpdated() == FALSE after init() (GameLogic::reset()'s own m_hasUpdated = FALSE)");

		// TheSuperHackers @info Milestone 10 implementation finding: a real
		// gdb backtrace showed AI::update() (called from
		// GameLogic::update()'s own TheAI->UPDATE() line, GameLogic.cpp:3945)
		// unconditionally calling ThePlayerList->UPDATE() (AI.cpp:363) - a
		// real dependency the draft's own section 4 analysis did not trace
		// into (it stopped at "TheAI->UPDATE()... all real, all cheap", not
		// reading AI::update()'s own body). More surprising: grepping the
		// ENTIRE real, shipped codebase shows `ThePlayerList = new PlayerList`
		// is constructed in exactly ONE place in the whole engine+tools tree -
		// Core/Tools/GUIEdit/Source/GUIEdit.cpp:555 - NEVER by
		// GameEngine.cpp/GameLogic.cpp itself. This explains a second real
		// finding: AcademyStats::init() (Core/GameEngine/Source/Common/RTS/
		// AcademyStats.cpp:100-103) has a real, pre-existing guard,
		// `if (!TheGameLogic) { return; // GUIEdit crashes on this, so bail }`
		// - written for EXACTLY this GUIEdit scenario (a tool constructing
		// players before a real GameLogic exists) - but that guard leaves
		// m_unknownSide/m_player at AcademyStats's own default-constructed
		// (uninitialized memory, not a safe default - AcademyStats::AcademyStats()
		// has an empty body) state, which AcademyStats::update() does not
		// itself re-check. GUIEdit never calls GameLogic::UPDATE() so it never
		// exercises this path; this harness's whole point is to call it. Fix:
		// construct ThePlayerList here, AFTER TheGameLogic already exists, so
		// AcademyStats::init()'s guard does not trigger and the default
		// player's AcademyStats initializes for real (m_unknownSide correctly
		// becomes TRUE, since our default player has no PlayerTemplate -
		// AcademyStats.cpp:113-121 - so AcademyStats::update() safely returns
		// early every tick thereafter). A disclosed reordering relative to the
		// draft's own §2 pseudocode, justified by evidence neither the draft
		// nor the shipped engine's own construction order actually constrains
		// this placement (ThePlayerList has no real "GameEngine::init() order"
		// to match, since the shipped engine never constructs it there at all). ----
		ThePlayerList = NEW PlayerList;
		Check(ThePlayerList != nullptr, "2h. ThePlayerList constructed (non-null, AFTER TheGameLogic - see this file's own comment) - real, load-bearing dependency of AI::update(), found via gdb");

		// ---- Check 3: the real tick loop - N=5 real calls into
		// TheGameLogic->UPDATE(), asserting the REAL, unmodified
		// GameLogic::update()'s own frame counter and hasUpdated() flag after
		// each call (Draft 34 section 4's recommended exit criterion). ----
		printf("=== Check 3: N=5 real ticks through the unmodified GameLogic::update() ===\n");
		const int kNumTicks = 5;
		for (int tick = 0; tick < kNumTicks; ++tick)
		{
			TheGameLogic->UPDATE();

			char label[128];
			snprintf(label, sizeof(label), "3.%d. getFrame() == %d after tick %d (real frame counter, real update() body)", tick, tick + 1, tick);
			Check(TheGameLogic->getFrame() == static_cast<UnsignedInt>(tick + 1), label);

			snprintf(label, sizeof(label), "3.%d. hasUpdated() == TRUE after tick %d", tick, tick);
			Check(TheGameLogic->hasUpdated() == TRUE, label);
		}

		// ---- Check 4: secondary, redundant confirmation that ThePartitionManager
		// and TheAI's own UPDATE() were genuinely invoked (not short-circuited
		// away), per Draft 34 section 4's suggested secondary check. Both real
		// subsystems stay internally consistent (no crash, no assert) across 5
		// real ticks with zero objects - that IS the confirmation; there is no
		// separate counter to read without adding harness-only instrumentation
		// to engine code, which this port avoids by policy. ----
		printf("=== Check 4: ThePartitionManager/TheAI survived 5 real ticks with no crash ===\n");
		Check(ThePartitionManager != nullptr, "4a. ThePartitionManager still alive after 5 ticks");
		Check(TheAI != nullptr, "4b. TheAI still alive after 5 ticks");

		// ---- Check 5 (Milestone 15, native port plan Draft 39, "Phase 8
		// rung 0"): the first-ever call anywhere in this fork of
		// SimulationMathCrc::calculate() - a purpose-built, already-upstream,
		// cross-platform floating-point-determinism probe
		// (Core/GameEngine/Source/Common/Diagnostic/SimulationMathCrc.cpp),
		// already compiled into this harness's own link closure (it lives in
		// Core/GameEngine/CMakeLists.txt's GAMEENGINE_SRC, consumed here via
		// corei_gameengine_private's INTERFACE_SOURCES - see this file's own
		// header comment) but never once invoked before this milestone.
		// Two checks: (a) repeat-call stability - calling it twice in the
		// same run, with no intervening state to diverge on (it is a pure,
		// self-contained computation over compile-time-constant inputs, not a
		// function of TheGameLogic's own simulation state), must produce the
		// identical CRC; (b) the value itself matches a checked-in expected
		// constant (kExpectedSimulationMathCrc, above) - a real regression
		// tripwire a future toolchain/optimization-flag/libm change would trip,
		// not merely a "did it crash" smoke test. ----
		printf("=== Check 5: SimulationMathCrc::calculate() - first-ever real call (Draft 39 Task 1) ===\n");
		const UnsignedInt simMathCrcFirstCall = SimulationMathCrc::calculate();
		const UnsignedInt simMathCrcSecondCall = SimulationMathCrc::calculate();

		char simMathLabel[192];
		snprintf(simMathLabel, sizeof(simMathLabel), "5a. SimulationMathCrc::calculate() == 0x%8.8X (checked-in expected value, WSL2 GCC build/linux-x64)", simMathCrcFirstCall);
		Check(simMathCrcFirstCall == kExpectedSimulationMathCrc, simMathLabel);

		snprintf(simMathLabel, sizeof(simMathLabel), "5b. repeat call in same run produces an identical CRC (0x%8.8X == 0x%8.8X, stability proof)", simMathCrcSecondCall, simMathCrcFirstCall);
		Check(simMathCrcSecondCall == simMathCrcFirstCall, simMathLabel);

		// ---- No teardown of TheGameEngine/TheGameLogic/TheAI/TheScriptEngine/
		// ThePartitionManager/TheTerrainLogic or any of the other singletons
		// constructed above: Draft 34's own "open questions for the
		// implementer" section recommends never destructing these at harness
		// exit (GameEngine::reset()'s real body unconditionally calls
		// TheWindowManager->winCreateLayout(...), which this harness never
		// constructs) - matching this port's established "no meticulous
		// teardown for objects that don't need it" precedent (same call already
		// made for several singletons in Tests/RenderRTS3DScene and
		// Tests/RenderCameraTransform). Process exit reclaims everything. ----
		printf("=== Teardown: deliberately skipped (Draft 34 open question 2) ===\n");
	}

	shutdownMemoryManager();

	TheAsciiStringCriticalSection = nullptr;
	TheUnicodeStringCriticalSection = nullptr;
	TheDmaCriticalSection = nullptr;
	TheMemoryPoolCriticalSection = nullptr;
	TheDebugLogCriticalSection = nullptr;

	if (g_AnyFailure)
	{
		fprintf(stderr, "GAMELOGICTICKHARNESS_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("GAMELOGICTICKHARNESS_OK: all checks passed\n");
	return 0;
}
