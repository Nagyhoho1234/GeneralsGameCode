// Phase 5(a) Milestone 10 (native port plan, Draft 34 "rung 2b-lite") - link
// closure this harness needs beyond what its hand-picked real .cpp files
// already provide, following this port's established "loud comment per stub"
// discipline (Tests/RenderRTS3DScene/link_stubs.cpp, Tests/RenderCameraTransform/
// link_stubs.cpp).
//
// The single, load-bearing design decision recorded here: TheGameEngine is
// NEVER a real, constructed GameEngine-derived object this milestone. See
// docs/native-port-plan-rung2b-lite-draft.md (Draft 34) section 5 and
// .superpowers/sdd/m10-task1-spike-report.md for the full story - in short,
// GameEngine.cpp unconditionally #includes GameNetwork/WOLBrowser/WebBrowser.h,
// whose real WebBrowser class is genuinely, structurally ATL/COM-based
// (FEBDispatch<...>, IBrowserDispatch, IID_IBrowserDispatch - not merely an
// unnecessary #include), so the real, unmodified GameEngine.cpp is never
// compiled into this harness at all.
//
// GameLogic::update()'s one real call site that needs a live TheGameEngine
// (GameLogic.cpp:3798, `TheFramePacer->setTimeFrozen(TheGameEngine->isTimeFrozen())`)
// calls a function declared `static Bool isTimeFrozen();` on the real
// GameEngine class (GameEngine.h:67). Because it is STATIC, the C++ standard
// requires the object expression (`TheGameEngine`) to be evaluated for its
// value only - it is never dereferenced. TheGameEngine can therefore stay a
// real, correctly-typed (`GameEngine*`), permanently-null pointer, and this
// call site is genuinely, provably safe - not merely unexercised. This is
// smaller and more disclosed than the pre-spike draft's own "harness-local
// GameEngine subclass" plan (task breakdown item 2), which assumed a live
// object was required; it is not.
//
// GameEngine::isTimeFrozen()'s real body (GameEngine.cpp:312-331) additionally
// checks TheNetwork and TheTacticalView, both real singletons this harness
// deliberately never constructs (networking and the display/view layer are
// both explicit non-goals - Draft 34's "Explicit non-goals" section). Rather
// than pull in GameNetwork/NetworkDefs.h and GameClient/View.h purely to
// declare two more permanently-null globals, this harness provides its own,
// smaller-but-behaviorally-identical body: in this harness's exact
// configuration TheNetwork and TheTacticalView would stay null forever
// regardless, so the real function's null-guarded branches for both are
// unreachable dead code here - this harness's body reduces to exactly the
// one live branch (TheScriptEngine's frozen flags), which IS exercised
// (TheScriptEngine is real and non-null). Disclosed deviation, not a scope
// gap: matches this port's established "provably safe for this specific
// harness's configuration" reasoning (see docs/native-port-plan.md's Draft 32
// section, the FramePacer::isActualFramesPerSecondLimitEnabled() discussion).
#include "PreRTS.h"

#include "Common/GameEngine.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/GameLogic.h"
#include "GameClient/Shadow.h"
#include "GameNetwork/NetworkDefs.h"

// All TheKey_* well-known Dict-key globals (Common/WellKnownKeys.h's
// DEFINE_KEY macro) are genuinely, only ever DEFINED (not merely declared)
// in Core/GameEngineDevice/.../WorldHeightMap.cpp ("#define
// INSTANTIATE_WELL_KNOWN_KEYS" before including WellKnownKeys.h) - a real,
// pre-existing codebase quirk (confirmed by grepping the whole repository:
// no other definition exists anywhere), already found and worked around the
// same way by Tests/RenderCameraTransform/link_stubs.cpp for four of these
// symbols. This harness needs ~90 of them (GameLogic.cpp/Team.cpp/Player.cpp/
// SidesList.cpp/TerrainLogic.cpp's own real Dict field-table entries, link-
// live via address-of even though never populated by any actual Dict at
// runtime with zero objects/players). Rather than hand-duplicate ~90 macro
// expansions (error-prone, drifts from the real header), this defines the
// SAME `INSTANTIATE_WELL_KNOWN_KEYS` macro the real WorldHeightMap.cpp does
// and includes the SAME real header - the exact real definitions, just
// triggered from this harness's own TU instead of the heavy (WW3D2-
// dependent, out of scope) WorldHeightMap.cpp.
#define INSTANTIATE_WELL_KNOWN_KEYS
#include "Common/WellKnownKeys.h"

GameEngine *TheGameEngine = nullptr;

// TheProjectedShadowManager: real symbol declared in GameClient/Shadow.h
// (":224"), normally defined in the device-layer W3DProjectedShadow.cpp
// (never compiled into this harness - Core/GameEngineDevice's real shadow
// implementation, out of scope, same reasoning as W3DTerrainLogic.cpp/
// W3DGhostObject.cpp). RadiusDecal.cpp (a real, already-portable, non-device
// file this harness links - part of the closure GameLogic.cpp's own vtable-
// completeness pulls in) unconditionally dereferences it inside
// RadiusDecalTemplate::createRadiusDecal(), but that method is only ever
// reached from real object/veterancy-ring code paths this harness's zero-
// object tick loop never calls - link-live (the symbol must exist), runtime-
// dead. Left permanently null, matching TheDisplay/TheTerrainRenderObject/
// TheWindowManager's established "never constructed, safely, because
// nothing in the target call path dereferences it" pattern.
ProjectedShadowManager *TheProjectedShadowManager = nullptr;

Bool GameEngine::isTimeFrozen()
{
	// TheNetwork and TheTacticalView both stay permanently null in this
	// harness (see this file's header comment) - the real function's
	// null-guarded branches for both are unreachable here, so they are
	// omitted rather than reproduced against globals this harness never
	// defines.
	if (TheScriptEngine != nullptr)
	{
		if (TheScriptEngine->isTimeFrozenDebug() || TheScriptEngine->isTimeFrozenScript())
			return true;
	}

	return false;
}

// GameEngine::isGameHalted(): the real body (GameEngine.cpp:334-348),
// simplified the same way isTimeFrozen() above was - TheNetwork stays
// permanently null in this harness (never constructed - networking is an
// explicit non-goal), so the real function's `if (TheNetwork != nullptr)`
// branch (which calls TheNetwork->isStalling(), requiring the COMPLETE
// NetworkInterface class - a real networking-layer type this harness
// otherwise needs nothing from) is always false and omitted here, exactly
// as isTimeFrozen()'s TheNetwork/TheTacticalView branches were - the real
// else-branch (TheGameLogic->isGamePaused()) is genuinely reachable
// (TheGameLogic is real and non-null) and IS reproduced faithfully. Link-
// live via GameClient::update() (GameClient.cpp:627, never called by this
// harness - TheGameClient's own init()/update() are deliberately never
// invoked, see main.cpp's own HarnessGameClient comment), so this body is
// compiled but never actually executed at runtime.
Bool GameEngine::isGameHalted()
{
	if (TheGameLogic != nullptr && TheGameLogic->isGamePaused())
		return true;

	return false;
}

#include "GameClient/IMEManager.h"
// CreateIMEManagerInterface(): real definition lives in the excluded
// GameClient/GUI/IMEManager.cpp (mbstring.h - see this harness's
// CMakeLists.txt's own exclusion list). Link-live via GameClient::init()
// (GameClient.cpp:353, `TheIMEManager = CreateIMEManagerInterface();`) -
// never called by this harness (TheGameClient->init() is deliberately never
// invoked). Matches TheIMEManager's own permanently-null default above.
IMEManagerInterface *CreateIMEManagerInterface()
{
	return nullptr;
}

// ReleaseCrash: declared unconditionally (no DEBUG_LOGGING/_WIN32 guard -
// Debug.h's own comment says "EVEN IN FINAL RELEASE BUILDS") but genuinely,
// only ever DEFINED in Debug.cpp (":748-891"), which is deliberately NOT
// linked into this harness - a real, pre-existing portability gap Tests/
// GameFileSystem's own CMakeLists.txt already found and documented in full
// (ReleaseCrash/ReleaseCrashLocalized call real Win32 MessageBox/ShowWindow
// APIs with no portable equivalent anywhere in this tree). Reached only from
// GameLogic::friend_awakenUpdateModule()'s RELEASE_CRASH assertion guard
// (GameLogic.cpp:3187), itself only reachable if an UpdateModule's internal
// bookkeeping is already corrupt - never true for this harness's zero-object
// world. A minimal, honest stub (print + abort, matching what a release
// crash handler's OWN purpose is - stop, loudly - rather than a silent
// no-op) since, unlike DEBUG_CRASH, RELEASE_CRASH is a real invariant
// backstop meant to fire in shipped builds too.
void ReleaseCrash(const char* reason)
{
	fprintf(stderr, "GAMELOGICTICKHARNESS: ReleaseCrash() stub called (should be unreachable with zero objects): %s\n", reason ? reason : "(null)");
	abort();
}

#include "Common/OSDisplay.h"
// OSDisplaySetBusyState: real definition lives in the device-layer
// Win32OSDisplay.cpp (GeneralsMD/Code/GameEngineDevice/Source/Win32Device/...,
// WIN32-gated, out of scope). Reached from GameLogic::setGamePaused()/
// exitGame() (GameLogic.cpp:1127) - neither called by this harness's plain
// UPDATE() tick loop. Safe no-op stub; this harness has no OS busy-cursor
// concept at all.
void OSDisplaySetBusyState(Bool /*busyDisplay*/, Bool /*busySystem*/)
{
}

// oversizeTheTerrain()/doSkyBoxSet(): real definitions live in the heavy,
// device-layer BaseHeightMap.cpp/W3DWater.cpp (terrain mesh/skybox
// rendering, standing non-goals) - both declared `extern` locally inside
// ScriptActions.cpp itself (not a shared header), link-live because
// ScriptActions.cpp is real and compiled, runtime-dead because this
// harness's tick loop never runs a script action that calls either.
void oversizeTheTerrain(Int /*amount*/)
{
}

void doSkyBoxSet(Bool /*startDraw*/)
{
}

#include "texturefilter.h"
// TextureFilterClass::getTextureFilterMode(const char*): real definition
// lives in Core/Libraries/Source/WWVegas/WW3D2/texturefilter.cpp - part of
// the WW3D2 rendering closure this harness deliberately does not link (same
// z_ww3d2/34-known-error boundary as everywhere else in this file). Reached
// only via OptionPreferences::getTextureFilterMode() (Core/GameEngine/
// Source/Common/OptionPreferences.cpp:83-90), itself only actually invoked
// by GlobalData's real INI-driven options-loading path (GlobalData.cpp:1255)
// - never reached by this harness's plain `NEW GlobalData` construction
// (Milestone 7/8's own established shortcut, no INI). A minimal, real-
// enum-returning stub; never observed by any of this harness's checks.
TextureFilterClass::TextureFilterMode TextureFilterClass::getTextureFilterMode(const char* /*str*/)
{
	return TEXTURE_FILTER_BILINEAR;
}

#include "GameClient/IMEManager.h"
// TheIMEManager: real singleton, only ever constructed by the device/GUI
// layer (out of scope - no IME/input-method concept in a headless logic
// harness). Referenced by GameClient/GUI/IMEManager.cpp (excluded - see
// this harness's CMakeLists.txt's own exclusion list, mbstring.h), but the
// GLOBAL POINTER itself is declared in a header several OTHER, real, linked
// files reference defensively (always null-guarded). Left permanently null.
IMEManagerInterface *TheIMEManager = nullptr;

#include "Common/MapObject.h"
// MapObject::TheMapObjectListPtr / getWaypointID() / getWaypointName() /
// setName() / setThingTemplate() / getThingTemplate(): real, non-virtual,
// out-of-line bodies genuinely live in the heavy, device-layer
// WorldHeightMap.cpp (terrain mesh rendering + map loading, a standing
// non-goal) - a real, pre-existing codebase quirk (grep-confirmed: no
// MapObject.cpp exists anywhere in this repository), already found and
// worked around the same way by Tests/RenderCameraTransform/link_stubs.cpp
// for two of these six members. This harness never loads a map, so
// TheMapObjectListPtr stays at its own real default (nullptr) for the
// harness's entire run, and the five instance methods are only reached (via
// TerrainLogic::addWaypoint()/GameLogic::tryStartNewGame(), both never
// called by this harness's plain construct-then-UPDATE() flow) from real
// map-loading code paths.
MapObject *MapObject::TheMapObjectListPtr = nullptr;

WaypointID MapObject::getWaypointID()
{
	DEBUG_CRASH(("GameLogicTickHarnessTest: MapObject::getWaypointID() stub called - should be unreachable; see link_stubs.cpp"));
	return (WaypointID)0;
}

AsciiString MapObject::getWaypointName()
{
	DEBUG_CRASH(("GameLogicTickHarnessTest: MapObject::getWaypointName() stub called - should be unreachable; see link_stubs.cpp"));
	return AsciiString::TheEmptyString;
}

void MapObject::setName(AsciiString /*name*/)
{
	DEBUG_CRASH(("GameLogicTickHarnessTest: MapObject::setName() stub called - should be unreachable; see link_stubs.cpp"));
}

void MapObject::setThingTemplate(const ThingTemplate* /*thing*/)
{
	DEBUG_CRASH(("GameLogicTickHarnessTest: MapObject::setThingTemplate() stub called - should be unreachable; see link_stubs.cpp"));
}

const ThingTemplate *MapObject::getThingTemplate() const
{
	DEBUG_CRASH(("GameLogicTickHarnessTest: MapObject::getThingTemplate() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

#include "Common/INI.h"
// INI::parseWebpageURLDefinition(): real definition lives in
// Common/INI/INIWebpageURL.cpp, excluded from this harness (d3dx8math.h via
// Common/BezierSegment.h - see this harness's CMakeLists.txt's own exclusion
// list). Referenced by address only, inside INI.cpp's own static field-
// parser dispatch table (theTypeTable) - never called without real
// "WebpageURL" INI content, which this harness never authors (Draft 34's own
// explicit non-goal: "any real INI content authorship of any kind").
void INI::parseWebpageURLDefinition(INI* /*ini*/)
{
	DEBUG_CRASH(("GameLogicTickHarnessTest: INI::parseWebpageURLDefinition() stub called - should be unreachable; see link_stubs.cpp"));
}

// g_strFile: a real, genuine "owned by main()" global - declared `extern`
// locally inside GameText.cpp itself (not a shared header) and defined only
// in each real executable's own WinMain.cpp entry point (every one of
// WinMain.cpp/wdump.cpp/autorun.cpp/W3DView.cpp/the WorldBuilder/GUIEdit/
// ImagePacker/MapCacheBuilder tool mains - grep-confirmed, no shared
// definition exists). Link-live because GameTextManager::init()
// (GameTextManager overrides GameTextInterface's virtual init(), so its
// address is vtable-referenced regardless of whether this harness calls it)
// references it; runtime-dead since this harness never calls
// TheGameText->init() at all (see main.cpp's own construction-order
// comment - GameTextManager::fetch() is safe without it). Same real string
// literal GeneralsMD's own WinMain.cpp uses ("data\\Generals.str"), for
// parity - never actually read by this harness.
const Char *g_strFile = "data\\Generals.str";

// g_csfFile: same "owned by main()" pattern as g_strFile immediately above
// (GameText.cpp:290 inside the same link-live-but-runtime-dead
// GameTextManager::init()) - same real string literal GeneralsMD's own
// WinMain.cpp uses.
const Char *g_csfFile = "data\\%s\\Generals.csf";
