// Harness-local link stubs, originally cloned verbatim from Tests/
// RenderCameraTransform/link_stubs.cpp for this milestone's first attempt
// (commit 67d7b83f0), now COMPREHENSIVELY REWRITTEN for the retry (see
// main.cpp's own header comment and CMakeLists.txt's own header comment for
// the full "why" of the DEFER-closure link strategy this retry adopts).
//
// A REAL, IMPLEMENTATION-TIME FINDING, larger than anticipated even after
// the CMakeLists.txt-level duplicate-source pruning: linking z_gameengine's
// ENTIRE real source closure (300+ .cpp files) means the overwhelming
// majority of this file's ORIGINAL stub surface duplicates a real,
// now-linked definition - not just the half-dozen singletons the retry's
// own design anticipated (TheGameLogic/TheScriptEngine/TheGameClient/
// TheDisplay/TheInGameUI), but nearly every OTHER singleton pointer and
// member-function stub this file used to carry (TheAI, TheAudio,
// TheControlBar, TheGhostObjectManager, TheNetwork, TheParticleSystemManager,
// ThePartitionManager, ThePlayerList, TheRadar, TheTerrainRoads,
// TheTerrainVisual, TheThingFactory, TheVersion, TheWaterTransparency,
// TheWindowManager, and roughly two dozen member-function bodies -
// PartitionManager/Pathfinder/PolygonTrigger/SimpleObjectIterator/
// TerrainRoadCollection/ThingFactory/Drawable/GameWindow/TintEnvelope/
// SequentialScript/DamageInfo* methods, BridgeBehavior/BridgeTowerBehavior
// statics, getPickTypesForContext(), GetGameClientRandomValueReal(),
// Version::getVersionNumber()) - ALL measured as real "multiple definition"
// link errors by a real link attempt, not predicted by static reading. Every
// one of those was REMOVED here; what remains below is exactly what a real
// link attempt against the FULL closure still asked for - genuinely
// Windows-only or still-unlinked-heavier-subsystem symbols only (WorldHeightMap.cpp's
// exclusive definitions, the WIN32-gated W3DShaderManager.cpp/
// CameraShakeSystem.cpp/W3DDisplay.cpp device files, IME/OSDisplay/crash-
// handler platform seams, and the WellKnownKeys.h Dict-key globals). Follows
// the established anim_sound_link_stub.cpp / Tests/GameLogicTickHarness/
// link_stubs.cpp pattern: loud comments, no hidden behavior.
#include "PreRTS.h"

// ============================================================================
// ---- Genuinely still-needed singleton pointers: none of these classes'
// real .cpp files are part of the linked closure (all are either device-tier
// W3D/Win32 files or heavier not-yet-linked subsystems). ----
// ============================================================================

#include "Common/SubsystemInterface.h"
SubsystemInterfaceList *TheSubsystemList = nullptr;

#include "W3DDevice/GameClient/W3DShadow.h"
W3DShadowManager *TheW3DShadowManager = nullptr;

#include "W3DDevice/GameClient/BaseHeightMap.h"
BaseHeightMapRenderObjClass *TheTerrainRenderObject = nullptr;

// TheProjectedShadowManager: real symbol declared in GameClient/Shadow.h,
// normally defined in the device-layer W3DProjectedShadow.cpp (not linked
// here). RadiusDecal.cpp (part of the real, linked closure) unconditionally
// dereferences it inside RadiusDecalTemplate::createRadiusDecal(), but that
// method is only reached from real object/veterancy-ring code paths this
// harness's zero-object world never calls - link-live, runtime-dead.
#include "GameClient/Shadow.h"
ProjectedShadowManager *TheProjectedShadowManager = nullptr;

// TheGameEngine: GameEngine.cpp itself is excluded (unconditionally
// #includes GameNetwork/WOLBrowser/WebBrowser.h, genuinely ATL/COM-based -
// see Tests/GameLogicTickHarness/link_stubs.cpp's own header comment for the
// full story, identical reasoning here). isTimeFrozen()/isGameHalted() are
// both `static` (GameEngine.h:67-68), so `TheGameEngine->isTimeFrozen()`
// call sites (link-live via GameLogic.cpp/GameClient.cpp's own real,
// now-linked bodies, never actually reached at runtime since this harness
// never calls GameLogic::update()/GameClient::update()) only need
// TheGameEngine's VALUE, never a dereference - a real, correctly-typed,
// permanently-null pointer is genuinely safe, not merely unexercised.
#include "Common/GameEngine.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/ScriptEngine.h"
GameEngine *TheGameEngine = nullptr;

Bool GameEngine::isTimeFrozen()
{
	// TheNetwork and TheTacticalView both stay permanently null in this
	// harness - the real body's null-guarded branches for both are
	// unreachable here, so they are omitted rather than reproduced against
	// globals with no other need in this harness (same simplification Tests/
	// GameLogicTickHarness/link_stubs.cpp already established).
	if (TheScriptEngine != nullptr)
	{
		if (TheScriptEngine->isTimeFrozenDebug() || TheScriptEngine->isTimeFrozenScript())
			return true;
	}
	return false;
}

Bool GameEngine::isGameHalted()
{
	if (TheGameLogic != nullptr && TheGameLogic->isGamePaused())
		return true;
	return false;
}

// TheIMEManager / CreateIMEManagerInterface(): the real singleton is only
// ever constructed by the device/GUI layer (out of scope). The real
// CreateIMEManagerInterface() body lives in GameClient/GUI/IMEManager.cpp,
// excluded (mbstring.h, matching Tests/GameLogicTickHarness's own exclusion
// list rationale) - link-live via GameClient::init() (never called by this
// harness).
#include "GameClient/IMEManager.h"
IMEManagerInterface *TheIMEManager = nullptr;

IMEManagerInterface *CreateIMEManagerInterface()
{
	return nullptr;
}

// ReleaseCrash: declared unconditionally (Debug.h, "EVEN IN FINAL RELEASE
// BUILDS") but genuinely only ever DEFINED in Debug.cpp, deliberately not
// linked here (real Win32 MessageBox/ShowWindow calls, no portable
// equivalent - Tests/GameFileSystem's own established finding). A minimal,
// honest stub matching RELEASE_CRASH's own real purpose (stop, loudly)
// rather than a silent no-op.
void ReleaseCrash(const char* reason)
{
	fprintf(stderr, "RENDERVIEWUPDATEDRAWTEST: ReleaseCrash() stub called (should be unreachable): %s\n", reason ? reason : "(null)");
	abort();
}

// OSDisplaySetBusyState()/oversizeTheTerrain()/doSkyBoxSet(): real
// definitions live in device-layer Win32OSDisplay.cpp/BaseHeightMap.cpp/
// W3DWater.cpp (all WIN32/terrain-mesh, not linked here) - link-live via
// GameLogic.cpp/ScriptActions.cpp's own real, now-linked bodies, runtime-dead
// (never actually called by anything this harness's main() invokes).
#include "Common/OSDisplay.h"
void OSDisplaySetBusyState(Bool /*busyDisplay*/, Bool /*busySystem*/)
{
}

void oversizeTheTerrain(Int /*amount*/)
{
}

void doSkyBoxSet(Bool /*startDraw*/)
{
}

// MapObject::TheMapObjectListPtr / getWaypointID() / getWaypointName() /
// setName() / setThingTemplate() / getThingTemplate(): real, non-virtual,
// out-of-line bodies genuinely live in the heavy, device-layer
// WorldHeightMap.cpp (terrain mesh rendering + map loading, a standing
// non-goal, not linked here) - a real, pre-existing codebase quirk
// (grep-confirmed: no MapObject.cpp exists anywhere in this repository).
// This harness never loads a map, so TheMapObjectListPtr stays at its own
// real default (nullptr); the five instance methods are only reached from
// real map-loading code paths (TerrainLogic::addWaypoint(), never called).
#include "Common/MapObject.h"
MapObject *MapObject::TheMapObjectListPtr = nullptr;

WaypointID MapObject::getWaypointID()
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: MapObject::getWaypointID() stub called - should be unreachable; see link_stubs.cpp"));
	return (WaypointID)0;
}

AsciiString MapObject::getWaypointName()
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: MapObject::getWaypointName() stub called - should be unreachable; see link_stubs.cpp"));
	return AsciiString::TheEmptyString;
}

void MapObject::setName(AsciiString /*name*/)
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: MapObject::setName() stub called - should be unreachable; see link_stubs.cpp"));
}

void MapObject::setThingTemplate(const ThingTemplate* /*thing*/)
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: MapObject::setThingTemplate() stub called - should be unreachable; see link_stubs.cpp"));
}

const ThingTemplate *MapObject::getThingTemplate() const
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: MapObject::getThingTemplate() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

// INI::parseWebpageURLDefinition(): real definition lives in
// Common/INI/INIWebpageURL.cpp, excluded from this harness's closure
// (d3dx8math.h via Common/BezierSegment.h - CMakeLists.txt's own exclusion
// list). Referenced by address only, inside INI.cpp's own static field-
// parser dispatch table - never called without real "WebpageURL" INI
// content, which this harness never authors.
#include "Common/INI.h"
void INI::parseWebpageURLDefinition(INI* /*ini*/)
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: INI::parseWebpageURLDefinition() stub called - should be unreachable; see link_stubs.cpp"));
}

// All TheKey_* well-known Dict-key globals (Common/WellKnownKeys.h's
// DEFINE_KEY macro) are genuinely, only ever DEFINED (not merely declared)
// in Core/GameEngineDevice/.../WorldHeightMap.cpp ("#define
// INSTANTIATE_WELL_KNOWN_KEYS" before including WellKnownKeys.h) - a real,
// pre-existing codebase quirk (grep-confirmed: no other definition exists
// anywhere). The full, real GameEngine closure this retry links reaches
// dozens of these (SidesList::validateSides(), Object::
// updateObjValuesFromMapProperties(), GameLogic::tryStartNewGame(), etc.,
// all link-live via address-of even though never populated by any actual
// Dict at runtime with this harness's empty object/player state). Rather
// than hand-duplicate each DEFINE_KEY(...) expansion one at a time (this
// milestone's first attempt only needed four and hand-copied them - see git
// history), this defines the SAME INSTANTIATE_WELL_KNOWN_KEYS macro the real
// WorldHeightMap.cpp does and includes the SAME real header - the exact real
// definitions, just triggered from this harness's own TU instead of the
// heavy (terrain-mesh, out of scope) WorldHeightMap.cpp. Matches Tests/
// GameLogicTickHarness/link_stubs.cpp's own established technique.
#define INSTANTIATE_WELL_KNOWN_KEYS
#include "Common/WellKnownKeys.h"

// PrepareShadows()/DoShadows()/DoTrees()/DoParticles(): real definitions
// live in the heavy, device-layer per-tree shadow/tree/particle rendering
// TUs (not linked here - standing non-goals, matching Tests/
// RenderRTS3DScene's own established rationale, unchanged). Link-live via
// RTS3DScene::Flush() (part of the real, linked W3DScene.cpp), runtime-dead
// for this harness's Drawable-free scene.
#include "WW3D2/rinfo.h"

void PrepareShadows()
{
}

void DoShadows(RenderInfoClass & /*rinfo*/, Bool /*stencilPass*/)
{
}

void DoTrees(RenderInfoClass & /*rinfo*/)
{
}

void DoParticles(RenderInfoClass & /*rinfo*/)
{
}

// ============================================================================
// ---- W3DShaderManager.cpp / CameraShakeSystem.cpp / W3DDisplay.cpp: all
// three remain genuinely WIN32/D3D8-gated device files, none part of the
// linked closure (unchanged from the milestone's first attempt - see each
// block's own comment for the per-symbol rationale, carried over verbatim). ----
// ============================================================================

#include "W3DDevice/GameClient/W3DShaderManager.h"
TextureClass *W3DShaderManager::m_Textures[8];

Int W3DShaderManager::setShader(ShaderTypes /*shader*/, Int pass)
{
	return pass;
}

void W3DShaderManager::resetShader(ShaderTypes /*shader*/)
{
}

// W3DShaderManager::filterSetup() - only reached from W3DView::reset()'s
// setViewFilter()/setViewFilterMode() calls; reset() is link-live (vtable)
// but never called by this harness.
Bool W3DShaderManager::filterSetup(FilterTypes /*filter*/, FilterModes /*mode*/)
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: W3DShaderManager::filterSetup() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

// W3DShaderManager::filterPreRender() IS genuinely reachable (not merely
// link-live) - W3DView::draw()'s filter branch runs unconditionally by
// default (m_viewFilterMode/m_viewFilter default to FM_VIEW_DEFAULT/
// FT_VIEW_DEFAULT, both non-zero, Draft 35 finding 7), and this harness
// genuinely calls draw() (via view->drawView()) for real. Draft 35 finding
// 7's own evidence: the real registered filter for FT_VIEW_DEFAULT is
// ScreenDefaultFilter, whose real preRender() is a hard-coded "return
// FALSE;" (W3DShaderManager.cpp:175-182, a dated, already-landed bugfix -
// "Disable Render To Texture redirection for the default filter...
// corrupts depth testing producing black screen"). This stub reproduces
// that exact, documented real behavior.
Bool W3DShaderManager::filterPreRender(FilterTypes /*filter*/, Bool & /*skipRender*/, CustomScenePassModes & /*scenePassMode*/)
{
	return false;
}

// filterPostRender() is only called if filterPreRender() returned true
// (draw()'s own "if (preRenderResult)" guard) - filterPreRender() above
// always returns false, so this stays genuinely unreachable.
Bool W3DShaderManager::filterPostRender(FilterTypes /*filter*/, FilterModes /*mode*/, Coord2D & /*scrollDelta*/, Bool & /*doExtraRender*/)
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: W3DShaderManager::filterPostRender() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

// ScreenBWFilter/ScreenCrossFadeFilter/ScreenMotionBlurFilter static storage
// only - their real Init/reset/set/preRender/postRender bodies live in the
// heavy W3DShaderManager.cpp (not linked); never referenced here since no
// concrete filter object is ever constructed. W3DView::setFadeParameters()/
// setViewFilterPos() (link-live vtable entries, never called) invoke their
// inline statics unconditionally.
Int ScreenBWFilter::m_fadeFrames = 0;
Int ScreenBWFilter::m_fadeDirection = 0;
Int ScreenBWFilter::m_curFadeFrame = 0;
Real ScreenBWFilter::m_curFadeValue = 0.0f;

Int ScreenCrossFadeFilter::m_fadeFrames = 0;
Int ScreenCrossFadeFilter::m_fadeDirection = 0;
Int ScreenCrossFadeFilter::m_curFadeFrame = 0;
Real ScreenCrossFadeFilter::m_curFadeValue = 0.0f;

Coord3D ScreenMotionBlurFilter::m_zoomToPos;
Bool ScreenMotionBlurFilter::m_zoomToValid = false;

// CameraShakeSystemClass: the real Core/GameEngineDevice/.../
// CameraShakeSystem.cpp is WIN32-gated AND unconditionally #includes
// d3dx8core.h, a genuinely non-portable header with no PortableD3D8
// equivalent today. Real, out-of-line, TRUE-NO-OP bodies for its four
// methods (true no-ops because Add_Camera_Shake() is never called by this
// harness, so the real CameraShakerList would stay empty regardless) plus
// the one real global instance every W3DView.cpp method expects by name.
#include "W3DDevice/GameClient/CameraShakeSystem.h"

CameraShakeSystemClass::CameraShakeSystemClass()
{
}

CameraShakeSystemClass::~CameraShakeSystemClass()
{
}

void CameraShakeSystemClass::Timestep(float /*dt*/)
{
}

bool CameraShakeSystemClass::IsCameraShaking()
{
	return false;
}

void CameraShakeSystemClass::Update_Camera_Shaker(Vector3 /*camera_position*/, Vector3 *shaker_angle)
{
	*shaker_angle = Vector3(0.0f, 0.0f, 0.0f);
}

void CameraShakeSystemClass::Add_Camera_Shake(const Vector3 & /*position*/, float /*radius*/, float /*duration*/, float /*power*/)
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: CameraShakeSystemClass::Add_Camera_Shake() stub called - should be unreachable; see link_stubs.cpp"));
}

CameraShakeSystemClass CameraShakerSystem;

// W3DDisplay::m_3DScene/m_2DScene - STATIC member variables of W3DDisplay
// (W3DDisplay.h), read directly by W3DView::draw()/updateTerrain().
// W3DDisplay.cpp itself is deliberately NOT linked here (WIN32-gated) -
// these two static members' storage is supplied here directly.
#include "W3DDevice/GameClient/W3DDisplay.h"
RTS3DScene *W3DDisplay::m_3DScene = nullptr;
RTS2DScene *W3DDisplay::m_2DScene = nullptr;

// WorldHeightMap::getDrawRegion2D() - non-virtual, out-of-line real body in
// the heavy WorldHeightMap.cpp (not linked - terrain mesh data, a standing
// non-goal). Only reached from getAxisAlignedViewRegion()'s "if
// (TheTerrainRenderObject && TheTerrainRenderObject->getMap())" branch -
// TheTerrainRenderObject is null, so this branch is dead regardless.
#include "W3DDevice/GameClient/WorldHeightMap.h"
Region2D WorldHeightMap::getDrawRegion2D()
{
	DEBUG_CRASH(("RenderViewUpdateDrawTest: WorldHeightMap::getDrawRegion2D() stub called - should be unreachable; see link_stubs.cpp"));
	Region2D r;
	r.zero();
	return r;
}
