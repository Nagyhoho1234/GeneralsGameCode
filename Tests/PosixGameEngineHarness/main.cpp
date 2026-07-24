// Phase 5(a) Milestone 12 (native port plan, Draft 36, "rung 2b") - the
// milestone's own payoff harness: the REAL, unmodified
// GameEngine::init()/update()/execute() on POSIX, driven through a
// harness-local PosixGameEngine subclass (harness_stub_classes.h) overriding
// only the 11 pure factory methods GameEngine.h itself declares - not a
// bypass, not hand-constructed singletons; the real init() chain constructs
// everything else itself. See docs/native-port-plan.md's Draft 36 section
// for the full design rationale (three original blockers retired/recast,
// the INI-scaffold cost, the "let the real init() chain tell you what's
// missing" step-0 spike discipline this harness's own implementation
// followed).
//
// Construction order (this file's own main()):
//   Prologue: five CriticalSection globals + initMemoryManager() (unchanged
//     from every prior harness in this port).
//   TheVersion = NEW Version - real class, matches WinMain.cpp's own
//     construction (GameEngine.cpp:196's own unguarded deref in release
//     builds, updateWindowTitle()).
//   TheGameText = CreateGameTextInterface() (un-init()'d) - matches
//     Milestone 10's own established shortcut (GameTextManager::fetch() is
//     safe without init(), only DEBUG_ASSERTCRASH's on m_initialized, a
//     no-op in this harness's release-style build); GameEngine::init()
//     itself calls initSubsystem(TheGameText, ..., CreateGameTextInterface(),
//     ...) again, real-initializing it for real at that point (this
//     construction is a pre-init() placeholder only insofar as
//     updateWindowTitle()'s own DEBUG_ASSERTCRASH wants a non-null pointer
//     before init() proper reaches that same subsystem).
//   TheWritableGlobalData = NEW GlobalData; ->m_headless = TRUE - real
//     class, no INI (Milestone 7/8's own established shortcut) - passed
//     directly to GameEngine::init()'s own
//     initSubsystem(TheWritableGlobalData, "TheWritableGlobalData",
//     TheWritableGlobalData, ...) call, which expects it already
//     constructed (unlike every other subsystem, which init() itself
//     constructs via a factory call). m_headless == TRUE selects every
//     real, already-portable headless-mode class this milestone's own
//     factories rely on (GhostObjectManagerDummy via GameLogic::init(),
//     MouseDummy/GameWindowManagerDummy via GameClient::init()'s own real
//     ternaries) - the SAME real --headless flag
//     (CommandLine.cpp:417) Milestone 10 already found and used for the
//     identical reason.
//   TheFramePacer = NEW FramePacer - real class, already portable
//     (Milestone 9's own established construction) - deliberately NOT
//     configured via enableFramesPerSecondLimit(TRUE)/
//     enableLogicTimeScale(TRUE) the way GameMain.cpp's own real entry
//     point does: leaving both flags at their real constructor defaults
//     (FALSE) makes FramePacer::getActualLogicTimeScaleFps()/
//     getActualFramesPerSecondLimit() both resolve to
//     RenderFpsPreset::UncappedFpsValue (confirmed by reading
//     FramePacer.cpp, not assumed), so
//     GameEngine::canUpdateRegularGameLogic() returns TRUE unconditionally -
//     deterministic, real, per-call ticking with no wall-clock-dependent
//     throttling, matching this harness's own explicit update()-loop
//     assertions (Check 3 below).
//   PosixGameEngine engine; TheGameEngine = &engine; engine.init() - THE
//     milestone's actual payoff call: the real, unmodified
//     GameEngine::init(), which internally real-constructs everything else
//     (TheFileSystem/TheLocalFileSystem/TheArchiveFileSystem/
//     TheNameKeyGenerator/TheCommandList/TheGameLODManager/TheGameText/
//     TheScienceStore/TheMultiplayerSettings/TheTerrainTypes/
//     TheTerrainRoads/TheGlobalLanguageData/TheAudio/TheFunctionLexicon/
//     TheModuleFactory/TheMessageStream/TheSidesList/TheCaveSystem/
//     TheRankInfoStore/ThePlayerTemplateStore/TheParticleSystemManager/
//     TheFXListStore/TheWeaponStore/TheObjectCreationListStore/
//     TheLocomotorStore/TheSpecialPowerStore/TheDamageFXStore/
//     TheArmorStore/TheBuildAssistant/TheThingFactory/TheGameClient/TheAI/
//     TheGameLogic/TheTeamFactory/TheCrateSystem/ThePlayerList/TheRecorder/
//     TheRadar/TheVictoryConditions/TheMetaMap/TheActionManager/
//     TheGameStateMap/TheGameState/TheGameResultsQueue/TheMapCache), through
//     this harness's own PosixGameEngine factory overrides for the 11 it
//     owns directly.
//   Check 3: N=5 real calls into engine.update() (Draft 36's own "matching
//     Milestone 10's precedent"), asserting the REAL, unmodified
//     GameLogic::update()'s own frame counter and hasUpdated() flag after
//     each call - the SAME exit criterion Milestone 10 established, now
//     reached via the real GameEngine::update() -> canUpdateGameLogic() ->
//     TheGameLogic->UPDATE() chain instead of a direct call.
//   Check 4: a harness-local QuitAfterFramesTranslator, attached to
//     TheMessageStream AFTER TheGameClient's own real translators (see
//     GameClient::init(), whose own translators are all attached at
//     priorities 10-999999999 - this harness's own translator uses a
//     lower-than-all-of-those priority, matching MessageStream's own
//     documented "lower runs first" priority order, deliberately BEFORE the
//     real GameClientMessageDispatcher's own priority 999999999 so it still
//     observes every real MSG_FRAME_TICK message GameClient::update()
//     appends each frame before that dispatcher consumes it) - counts real
//     MSG_FRAME_TICK messages and calls the real, public
//     TheGameEngine->setQuitting(TRUE) once N further real frames have
//     elapsed, then engine.execute() - the REAL "main loop of the game
//     engine" (GameEngine.cpp's own comment), looping engine.update() calls
//     internally until m_quitting flips true - a real, clean exit through
//     the actual public API, not a harness-forced break.
//   No teardown: matches every prior milestone's own established "no
//     meticulous teardown for objects that don't need it" precedent (M9/M10/
//     M11) - GameEngine::~GameEngine()'s own real body unconditionally calls
//     TheGameResultsQueue->endThreads()/TheSubsystemList->shutdownAll()/
//     reset(), none of which this harness's own construction needs to
//     re-exercise for its own exit criterion. Process exit reclaims
//     everything.
#include "PreRTS.h"

#include "Common/AsciiString.h"
#include "Common/CriticalSection.h"
#include "Common/GameMemory.h"
#include "Common/NameKeyGenerator.h"
#include "Common/GlobalData.h"
#include "Common/GameEngine.h"
#include "Common/MessageStream.h"
#include "Common/FramePacer.h"
#include "Common/version.h"
#include "Common/PlayerList.h"
#include "GameClient/GameText.h"
#include "GameClient/Keyboard.h"
#include "GameClient/MetaEvent.h"
#include "GameClient/MapUtil.h"
#include "GameLogic/GameLogic.h"
#include "harness_stub_classes.h"

#include <cstdio>
#include <cstdlib>

namespace
{
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

	// ---- QuitAfterFramesTranslator: this harness's own message translator
	// (Draft 36's own recommended execute()-exit design) - counts real
	// MSG_FRAME_TICK messages GameClient::update() appends every frame
	// (GameClient.cpp:524-525) and calls the real, public
	// TheGameEngine->setQuitting(TRUE) once kFramesToRun further real frames
	// have elapsed. Returns KEEP_MESSAGE unconditionally - this translator
	// only OBSERVES the message stream, it never consumes/alters it, so
	// every real translator GameClient::init() itself attached still sees
	// every message exactly as it would without this harness present. ----
	class QuitAfterFramesTranslator : public GameMessageTranslator
	{
	public:
		explicit QuitAfterFramesTranslator(int framesToRun) : m_framesSeen(0), m_framesToRun(framesToRun) {}

		virtual GameMessageDisposition translateGameMessage(const GameMessage *msg) override
		{
			if (msg->getType() == GameMessage::MSG_FRAME_TICK)
			{
				++m_framesSeen;
				if (m_framesSeen >= m_framesToRun)
				{
					TheGameEngine->setQuitting(TRUE);
				}
			}
			return KEEP_MESSAGE;
		}

		int getFramesSeen() const { return m_framesSeen; }

	private:
		int m_framesSeen;
		int m_framesToRun;
	};
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
		printf("=== Setup: pre-init() prologue (Draft 36's own enumerated list) ===\n");

		// ---- TheVersion: real class, matches WinMain.cpp's own
		// construction - GameEngine.cpp's own updateWindowTitle()
		// (called unconditionally from inside init()) unguarded-dereferences
		// it in release builds (Draft 36's own finding). ----
		TheVersion = NEW Version;
		Check(TheVersion != nullptr, "0a. TheVersion constructed (non-null) - real class, matches WinMain.cpp's own construction");

		// ---- TheGameText: real class, un-init()'d - matches Milestone 10's
		// own established shortcut (GameTextManager::fetch() is safe without
		// init()). GameEngine::init() itself real-initializes this same
		// subsystem again via its own initSubsystem(TheGameText, ...) call. ----
		TheGameText = CreateGameTextInterface();
		Check(TheGameText != nullptr, "0b. TheGameText constructed (non-null, un-init()'d)");

		// ---- TheWritableGlobalData: real class, no INI (Milestone 7/8's
		// own established shortcut). m_headless == TRUE selects every real
		// headless-mode class this milestone's own factories and the real
		// engine's own GameClient.cpp/GameLogic.cpp ternaries rely on. ----
		TheWritableGlobalData = NEW GlobalData;
		Check(TheWritableGlobalData != nullptr, "0c. TheWritableGlobalData constructed (non-null)");
		TheWritableGlobalData->m_headless = TRUE;
		Check(TheWritableGlobalData->m_headless == TRUE, "0d. m_headless set TRUE (real --headless flag, CommandLine.cpp:417)");

		// ---- TheFramePacer: real class, already portable (Milestone 9's
		// own established construction) - deliberately left at its real
		// constructor defaults (see this file's own header comment for why:
		// deterministic, uncapped per-call ticking for Check 3 below). ----
		TheFramePacer = NEW FramePacer;
		Check(TheFramePacer != nullptr, "0e. TheFramePacer constructed (non-null)");

		// ---- Check 1: the milestone's actual payoff call - the real,
		// unmodified GameEngine::init(), through this harness's own
		// PosixGameEngine factory overrides. ----
		printf("=== Check 1: real GameEngine::init() (the milestone's payoff call) ===\n");
		PosixGameEngine engine;
		TheGameEngine = &engine;
		Check(TheGameEngine != nullptr, "1a. TheGameEngine constructed (non-null, harness-local PosixGameEngine)");

		engine.init();

		Check(TheFileSystem != nullptr, "1b. TheFileSystem real-constructed by GameEngine::init() (non-null)");
		Check(TheLocalFileSystem != nullptr, "1c. TheLocalFileSystem real-constructed via createLocalFileSystem() (non-null)");
		Check(TheArchiveFileSystem != nullptr, "1d. TheArchiveFileSystem real-constructed via createArchiveFileSystem() (non-null)");
		Check(TheAudio != nullptr, "1e. TheAudio real-constructed via createAudioManager() (non-null)");
		Check(TheThingFactory != nullptr, "1f. TheThingFactory real-constructed via createThingFactory() (non-null)");
		Check(TheModuleFactory != nullptr, "1g. TheModuleFactory real-constructed via createModuleFactory() (non-null)");
		Check(TheFunctionLexicon != nullptr, "1h. TheFunctionLexicon real-constructed via createFunctionLexicon() (non-null)");
		Check(TheMessageStream != nullptr, "1i. TheMessageStream real-constructed by GameEngine::init() (non-null)");
		Check(TheGameClient != nullptr, "1j. TheGameClient real-constructed via createGameClient() (non-null)");
		Check(TheAI != nullptr, "1k. TheAI real-constructed by GameEngine::init() (non-null)");
		Check(TheGameLogic != nullptr, "1l. TheGameLogic real-constructed via createGameLogic() (non-null)");
		Check(ThePlayerList != nullptr, "1m. ThePlayerList real-constructed by GameEngine::init() (non-null)");
		Check(TheRadar != nullptr, "1n. TheRadar real-constructed via createRadar() (non-null)");
		Check(TheParticleSystemManager != nullptr, "1o. TheParticleSystemManager real-constructed via createParticleSystemManager() (non-null)");
		Check(TheMetaMap != nullptr, "1p. TheMetaMap real-constructed by GameEngine::init() (non-null)");
		Check(TheMapCache != nullptr, "1q. TheMapCache real-constructed by GameEngine::init() (non-null)");
		Check(TheGameLogic->getFrame() == 0, "1r. TheGameLogic->getFrame() == 0 after init()");
		Check(engine.getQuitting() == FALSE, "1s. engine.getQuitting() == FALSE after init() (no early -buildMapCache/-file quit request)");

		// ---- Check 2: TheGameClient's own real sub-construction, reached
		// through this harness's own GameClientStub factories. ----
		printf("=== Check 2: real GameClient::init() sub-construction ===\n");
		Check(TheDisplay != nullptr, "2a. TheDisplay real-constructed via GameClientStub::createGameDisplay() (non-null)");
		Check(TheFontLibrary != nullptr, "2b. TheFontLibrary real-constructed via GameClientStub::createFontLibrary() (non-null)");
		Check(TheDisplayStringManager != nullptr, "2c. TheDisplayStringManager real-constructed via GameClientStub::createDisplayStringManager() (non-null)");
		Check(TheInGameUI != nullptr, "2d. TheInGameUI real-constructed via GameClientStub::createInGameUI() (non-null)");
		Check(TheTacticalView != nullptr, "2e. TheTacticalView real-constructed by InGameUI::init() (ViewDummy, non-null)");
		Check(TheTerrainVisual != nullptr, "2f. TheTerrainVisual real-constructed via GameClientStub::createTerrainVisual() (non-null)");
		Check(TheVideoPlayer != nullptr, "2g. TheVideoPlayer real-constructed via GameClientStub::createVideoPlayer() (non-null)");
		Check(TheMouse != nullptr, "2h. TheMouse real-constructed (MouseDummy, headless ternary, non-null)");
		Check(TheWindowManager != nullptr, "2i. TheWindowManager real-constructed (GameWindowManagerDummy, headless ternary, non-null)");
		Check(TheKeyboard == nullptr, "2j. TheKeyboard stays null (headless - GameClient::init()'s own \"if (!m_headless)\" guard)");

		// ---- Check 3: the real tick loop - N=5 real calls into
		// engine.update(), asserting the REAL, unmodified GameLogic::update()'s
		// own frame counter and hasUpdated() flag after each call (Draft 36's
		// own "matching Milestone 10's precedent" exit criterion). ----
		printf("=== Check 3: N=5 real ticks through the unmodified GameEngine::update() ===\n");
		const int kNumTicks = 5;
		// TheSuperHackers @info Milestone 15 (native port plan, Draft 39,
		// "Phase 8 rung 0") Task 2: recorded across the tick loop below and
		// printed once more after the loop - the first real values this port
		// has ever read from GameLogic::getCRC(CRC_RECALC).
		UnsignedInt tickCrcValues[kNumTicks];
		for (int tick = 0; tick < kNumTicks; ++tick)
		{
			engine.update();

			char label[160];
			snprintf(label, sizeof(label), "3.%d. TheGameLogic->getFrame() == %d after tick %d (real frame counter, real GameEngine::update() -> GameLogic::update() chain)", tick, tick + 1, tick);
			Check(TheGameLogic->getFrame() == static_cast<UnsignedInt>(tick + 1), label);

			snprintf(label, sizeof(label), "3.%d. TheGameLogic->hasUpdated() == TRUE after tick %d", tick, tick);
			Check(TheGameLogic->hasUpdated() == TRUE, label);

			// ---- Milestone 15 Task 2: the first real call anywhere in this
			// fork of GameLogic::getCRC(CRC_RECALC) - a full sim-state CRC
			// over every Object/the logic random seed/ThePartitionManager/
			// ThePlayerList/TheAI (GameLogic.cpp:4195-4321), exercising
			// XferCRC::xferSnapshot() for real over real, live objects on
			// POSIX for the first time. Called TWICE back-to-back, with no
			// intervening real tick (no engine.update() between the two
			// calls) - getCRC(CRC_RECALC)'s own real body (read in full
			// during this milestone's implementation) constructs a scratch
			// XferCRC, walks state, deletes it - it does not mutate
			// TheGameLogic/ThePartitionManager/ThePlayerList/TheAI's own
			// state, so two calls over the SAME real, unchanged simulation
			// state must produce the identical CRC - a genuine "same real
			// inputs produce the same real CRC" proof, repeated independently
			// at 5 different real simulation states (frames 1 through 5), not
			// just once. ----
			const UnsignedInt crcFirstCall = TheGameLogic->getCRC(CRC_RECALC);
			const UnsignedInt crcSecondCall = TheGameLogic->getCRC(CRC_RECALC);
			tickCrcValues[tick] = crcFirstCall;

			snprintf(label, sizeof(label), "3.%d. getCRC(CRC_RECALC) repeat call (same real state, frame %d) == first call (0x%8.8X == 0x%8.8X, run-to-run stability proof)", tick, tick + 1, crcSecondCall, crcFirstCall);
			Check(crcSecondCall == crcFirstCall, label);
		}

		printf("=== Check 3 (continued): real getCRC(CRC_RECALC) values across 5 real ticks (Milestone 15 Task 2 finding) ===\n");
		for (int tick = 0; tick < kNumTicks; ++tick)
		{
			printf("  frame %d CRC = 0x%8.8X\n", tick + 1, tickCrcValues[tick]);
		}

		// ---- Check 4: the real execute() loop, exited through a real,
		// public setQuitting(TRUE) call from a harness-local message
		// translator - Draft 36's own recommended design (item 6). ----
		printf("=== Check 4: real GameEngine::execute() with a real, clean quit-message exit ===\n");
		const int kFramesBeforeQuit = 5;
		QuitAfterFramesTranslator *quitTranslator = NEW QuitAfterFramesTranslator(kFramesBeforeQuit);
		// Priority 5: lower than every real translator GameClient::init()
		// itself attached (10 through 999999999, MessageStream.h's own
		// documented "lower priority runs first" order), so this harness's
		// own translator observes every real MSG_FRAME_TICK message before
		// the real GameClientMessageDispatcher (priority 999999999) does.
		TheMessageStream->attachTranslator(quitTranslator, 5);
		Check(engine.getQuitting() == FALSE, "4a. engine.getQuitting() == FALSE before execute()");

		const UnsignedInt frameBeforeExecute = TheGameLogic->getFrame();

		engine.execute();

		Check(engine.getQuitting() == TRUE, "4b. engine.getQuitting() == TRUE after execute() returns (real, clean quit-message exit)");
		Check(quitTranslator->getFramesSeen() >= kFramesBeforeQuit, "4c. QuitAfterFramesTranslator observed >= 5 real MSG_FRAME_TICK messages");
		Check(TheGameLogic->getFrame() > frameBeforeExecute, "4d. TheGameLogic->getFrame() advanced further during execute()'s real internal update() loop");

		// ---- Milestone 15 Task 2, continued: a second, independent
		// repeat-call stability proof at a different real vantage point -
		// AFTER execute()'s own further real internal update() loop (more
		// frames, more real TheMessageStream/TheGameClient traffic than
		// Check 3's tick loop saw), not just once at the very end of the run.
		// Two consecutive getCRC(CRC_RECALC) calls with no real tick between
		// them, over this run's final real simulation state. ----
		const UnsignedInt crcAfterExecuteFirstCall = TheGameLogic->getCRC(CRC_RECALC);
		const UnsignedInt crcAfterExecuteSecondCall = TheGameLogic->getCRC(CRC_RECALC);
		char crcLabel[192];
		snprintf(crcLabel, sizeof(crcLabel), "4e. getCRC(CRC_RECALC) repeat call after execute() (0x%8.8X == 0x%8.8X, second independent stability proof) - final frame %u", crcAfterExecuteSecondCall, crcAfterExecuteFirstCall, TheGameLogic->getFrame());
		Check(crcAfterExecuteSecondCall == crcAfterExecuteFirstCall, crcLabel);

		// ---- No teardown: matches every prior milestone's own established
		// "no meticulous teardown for objects that don't need it" precedent
		// (see this file's own header comment). Process exit reclaims
		// everything. ----
		printf("=== Teardown: deliberately skipped (matches Milestone 9/10/11's own established precedent) ===\n");
	}

	shutdownMemoryManager();

	TheAsciiStringCriticalSection = nullptr;
	TheUnicodeStringCriticalSection = nullptr;
	TheDmaCriticalSection = nullptr;
	TheMemoryPoolCriticalSection = nullptr;
	TheDebugLogCriticalSection = nullptr;

	if (g_AnyFailure)
	{
		fprintf(stderr, "POSIXGAMEENGINEHARNESS_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("POSIXGAMEENGINEHARNESS_OK: all checks passed\n");
	return 0;
}
