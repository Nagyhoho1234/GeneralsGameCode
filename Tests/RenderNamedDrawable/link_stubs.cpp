// Phase 5(a) Milestone 13 (native port plan, Draft 37) - harness-local link
// closure this harness needs beyond what its real, linked
// GameEngine.cpp/z_gameengine/corei_gameengine_private closure (Milestone
// 12's own DEFER-closure technique) PLUS Milestone 11's hand-picked WW3D2/GL
// render-stack sources PLUS this milestone's own two new device files
// (W3DModelDraw.cpp/W3DAssetManager.cpp) already provide.
//
// Starting point: a MERGE of Tests/PosixGameEngineHarness/link_stubs.cpp
// (this milestone's own real GameEngine.cpp closure - unchanged, since this
// harness links the exact same closure) and Tests/RenderViewUpdateDraw/
// link_stubs.cpp's own render-stack-only entries (the ones NOT already
// superseded by a real, linked GameEngine.cpp - M11 itself excluded
// GameEngine.cpp, so its own TheGameEngine/isTimeFrozen()/isGameHalted()/
// TheSubsystemList stubs are OMITTED here, matching Milestone 12's own
// established "the real GameEngine.cpp body wins" finding). A real link
// attempt is the authority on the exact final set - this starting point is
// disclosed as a merge, not a from-scratch derivation; see this milestone's
// own close-out notes for whatever a real link attempt added or removed.
#include "PreRTS.h"

// ============================================================================
// ---- Genuinely still-needed singleton pointers (Milestone 12's own set,
// unchanged - none of these classes' real .cpp files are part of the linked
// closure). ----
// ============================================================================

#include "GameClient/Shadow.h"
ProjectedShadowManager *TheProjectedShadowManager = nullptr;

#include "GameClient/IMEManager.h"
IMEManagerInterface *TheIMEManager = nullptr;

IMEManagerInterface *CreateIMEManagerInterface()
{
	return nullptr;
}

void ReleaseCrash(const char* reason)
{
	fprintf(stderr, "RENDERNAMEDDRAWABLETEST: ReleaseCrash() stub called: %s\n", reason ? reason : "(null)");
	abort();
}

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

#include "Common/MapObject.h"
MapObject *MapObject::TheMapObjectListPtr = nullptr;

WaypointID MapObject::getWaypointID()
{
	DEBUG_CRASH(("RenderNamedDrawableTest: MapObject::getWaypointID() stub called - should be unreachable; see link_stubs.cpp"));
	return (WaypointID)0;
}

AsciiString MapObject::getWaypointName()
{
	DEBUG_CRASH(("RenderNamedDrawableTest: MapObject::getWaypointName() stub called - should be unreachable; see link_stubs.cpp"));
	return AsciiString::TheEmptyString;
}

void MapObject::setName(AsciiString /*name*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: MapObject::setName() stub called - should be unreachable; see link_stubs.cpp"));
}

void MapObject::setThingTemplate(const ThingTemplate* /*thing*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: MapObject::setThingTemplate() stub called - should be unreachable; see link_stubs.cpp"));
}

const ThingTemplate *MapObject::getThingTemplate() const
{
	DEBUG_CRASH(("RenderNamedDrawableTest: MapObject::getThingTemplate() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

#include "Common/INI.h"
void INI::parseWebpageURLDefinition(INI* /*ini*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: INI::parseWebpageURLDefinition() stub called - should be unreachable; see link_stubs.cpp"));
}

// ============================================================================
// ---- Menu Init/Update/Shutdown/Input/System entry points (Milestone 12's
// own set, unchanged). ----
// ============================================================================
#include "GameClient/WindowLayout.h"

void MainMenuInit( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void MainMenuUpdate( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void MainMenuShutdown( WindowLayout * /*layout*/, void * /*userData*/ ) {}
WindowMsgHandledType MainMenuSystem( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ ) { return MSG_IGNORED; }
WindowMsgHandledType MainMenuInput( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ ) { return MSG_IGNORED; }

void DownloadMenuInit( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void DownloadMenuUpdate( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void DownloadMenuShutdown( WindowLayout * /*layout*/, void * /*userData*/ ) {}
WindowMsgHandledType DownloadMenuInput( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ ) { return MSG_IGNORED; }

void WOLLadderScreenInit( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void WOLLadderScreenUpdate( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void WOLLadderScreenShutdown( WindowLayout * /*layout*/, void * /*userData*/ ) {}
WindowMsgHandledType WOLLadderScreenSystem( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ ) { return MSG_IGNORED; }
WindowMsgHandledType WOLLadderScreenInput( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ ) { return MSG_IGNORED; }

void WOLLoginMenuInit( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void WOLLoginMenuUpdate( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void WOLLoginMenuShutdown( WindowLayout * /*layout*/, void * /*userData*/ ) {}
WindowMsgHandledType WOLLoginMenuSystem( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ ) { return MSG_IGNORED; }
WindowMsgHandledType WOLLoginMenuInput( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ ) { return MSG_IGNORED; }

void WOLWelcomeMenuInit( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void WOLWelcomeMenuUpdate( WindowLayout * /*layout*/, void * /*userData*/ ) {}
void WOLWelcomeMenuShutdown( WindowLayout * /*layout*/, void * /*userData*/ ) {}
WindowMsgHandledType WOLWelcomeMenuInput( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ ) { return MSG_IGNORED; }

WindowMsgHandledType MOTDSystem( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ ) { return MSG_IGNORED; }

#include "Common/GameCommon.h"
void setupGameStart(AsciiString /*mapName*/, GameDifficulty /*diff*/) {}
void DoResolutionDialog() {}

#include "Common/GameLOD.h"
Bool testMinimumRequirements(ChipsetType *videoChipType, CpuType *cpuType, Int *cpuFreq, MemValueType *numRAM, Real *intBenchIndex, Real *floatBenchIndex, Real *memBenchIndex)
{
	if (videoChipType) *videoChipType = DC_UNKNOWN;
	if (cpuType) *cpuType = XX;
	if (cpuFreq) *cpuFreq = 0;
	if (numRAM) *numRAM = 0;
	if (intBenchIndex) *intBenchIndex = 0.0f;
	if (floatBenchIndex) *floatBenchIndex = 0.0f;
	if (memBenchIndex) *memBenchIndex = 0.0f;
	return FALSE;
}

// WW3D::Get_Texture_Reduction() / TextureFilterClass::TextureFilterModeString /
// TextureFilterClass::getTextureFilterMode(): UNLIKE Milestone 12 (which
// never linked core_ww3d2), this harness genuinely links the real WW3D2
// rendering library (corei_ww3d2, matching Tests/RenderViewUpdateDraw's own
// link set) - REAL definitions for all of these are available (texturefilter.cpp
// is part of the real, linked closure), so these stubs are deliberately
// OMITTED (a real "multiple definition" link error resulted from including
// them - confirmed by this milestone's own real link attempt, not assumed).

#include "GameClient/Smudge.h"
SmudgeManager *TheSmudgeManager = nullptr;

#include "GameNetwork/GameSpy/PersistentStorageDefs.h"
void UpdateLocalPlayerStats()
{
}

#include "Registry.h"
bool GetStringFromRegistry(std::string /*path*/, std::string /*key*/, std::string& /*val*/)
{
	return false;
}

bool GetUnsignedIntFromRegistry(std::string /*path*/, std::string /*key*/, unsigned int& /*val*/)
{
	return false;
}

bool SetStringInRegistry(std::string /*path*/, std::string /*key*/, std::string /*val*/)
{
	return false;
}

bool SetUnsignedIntInRegistry(std::string /*path*/, std::string /*key*/, unsigned int /*val*/)
{
	return false;
}

#include "GameLogic/Module/DumbProjectileBehavior.h"
DumbProjectileBehaviorModuleData::DumbProjectileBehaviorModuleData()
{
	DEBUG_CRASH(("RenderNamedDrawableTest: DumbProjectileBehaviorModuleData::DumbProjectileBehaviorModuleData() stub called - should be unreachable; see link_stubs.cpp"));
}

void DumbProjectileBehaviorModuleData::buildFieldParse(MultiIniFieldParse& /*p*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: DumbProjectileBehaviorModuleData::buildFieldParse() stub called - should be unreachable; see link_stubs.cpp"));
}

DumbProjectileBehavior::DumbProjectileBehavior( Thing *thing, const ModuleData* moduleData )
	: UpdateModule( thing, moduleData )
{
	DEBUG_CRASH(("RenderNamedDrawableTest: DumbProjectileBehavior::DumbProjectileBehavior() stub constructor called - should be unreachable; see link_stubs.cpp"));
}

UpdateSleepTime DumbProjectileBehavior::update()
{
	DEBUG_CRASH(("RenderNamedDrawableTest: DumbProjectileBehavior::update() stub called - should be unreachable; see link_stubs.cpp"));
	return UPDATE_SLEEP_NONE;
}

void DumbProjectileBehavior::projectileLaunchAtObjectOrPosition(const Object* /*victim*/, const Coord3D* /*victimPos*/, const Object* /*launcher*/, WeaponSlotType /*wslot*/, Int /*specificBarrelToUse*/, const WeaponTemplate* /*detWeap*/, const ParticleSystemTemplate* /*exhaustSysOverride*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: DumbProjectileBehavior::projectileLaunchAtObjectOrPosition() stub called - should be unreachable; see link_stubs.cpp"));
}

void DumbProjectileBehavior::projectileFireAtObjectOrPosition(const Object* /*victim*/, const Coord3D* /*victimPos*/, const WeaponTemplate* /*detWeap*/, const ParticleSystemTemplate* /*exhaustSysOverride*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: DumbProjectileBehavior::projectileFireAtObjectOrPosition() stub called - should be unreachable; see link_stubs.cpp"));
}

Bool DumbProjectileBehavior::projectileHandleCollision(Object* /*other*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: DumbProjectileBehavior::projectileHandleCollision() stub called - should be unreachable; see link_stubs.cpp"));
	return FALSE;
}

void DumbProjectileBehavior::crc(Xfer* /*xfer*/)
{
}

void DumbProjectileBehavior::xfer(Xfer* /*xfer*/)
{
}

void DumbProjectileBehavior::loadPostProcess()
{
}

DumbProjectileBehavior::~DumbProjectileBehavior()
{
}

MapObject::MapObject(Coord3D /*loc*/, AsciiString /*name*/, Real /*angle*/, Int /*flags*/, const Dict* /*props*/, const ThingTemplate* /*thingTemplate*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: MapObject::MapObject() stub constructor called - should be unreachable; see link_stubs.cpp"));
}

MapObject::~MapObject()
{
}

Dict MapObject::TheWorldDict;

void ReleaseCrashLocalized(const AsciiString& p, const AsciiString& m)
{
	fprintf(stderr, "RENDERNAMEDDRAWABLETEST: ReleaseCrashLocalized() stub called: %s / %s\n", p.str(), m.str());
	abort();
}

#include "GameClient/GUICallbacks.h"
WindowMsgHandledType WOLWelcomeMenuSystem( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ )
{
	DEBUG_CRASH(("RenderNamedDrawableTest: WOLWelcomeMenuSystem() stub called - should be unreachable; see link_stubs.cpp"));
	return MSG_IGNORED;
}

WindowMsgHandledType DownloadMenuSystem( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ )
{
	DEBUG_CRASH(("RenderNamedDrawableTest: DownloadMenuSystem() stub called - should be unreachable; see link_stubs.cpp"));
	return MSG_IGNORED;
}

const Char *g_strFile = "data\\Generals.str";
const Char *g_csfFile = "data\\%s\\Generals.csf";

#define INSTANTIATE_WELL_KNOWN_KEYS
#include "Common/WellKnownKeys.h"

// ============================================================================
// ---- Milestone 11's own render-stack-only entries, carried forward since
// this harness ALSO links W3DView.cpp/W3DScene.cpp/W3DShroud.cpp/
// W3DStatusCircle.cpp (Tests/RenderViewUpdateDraw/link_stubs.cpp's own
// header comment documents the full, real, per-symbol rationale of each -
// unchanged here; TheGameEngine/isTimeFrozen()/isGameHalted()/
// TheSubsystemList are DELIBERATELY OMITTED, since the real, linked
// GameEngine.cpp already defines all of them for real). ----
// ============================================================================

#include "W3DDevice/GameClient/BaseHeightMap.h"
BaseHeightMapRenderObjClass *TheTerrainRenderObject = nullptr;

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

#include "W3DDevice/GameClient/W3DShaderManager.h"
TextureClass *W3DShaderManager::m_Textures[8];

Int W3DShaderManager::setShader(ShaderTypes /*shader*/, Int pass)
{
	return pass;
}

void W3DShaderManager::resetShader(ShaderTypes /*shader*/)
{
}

Bool W3DShaderManager::filterSetup(FilterTypes /*filter*/, FilterModes /*mode*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: W3DShaderManager::filterSetup() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

// W3DShaderManager::filterPreRender(): see Tests/RenderViewUpdateDraw/
// link_stubs.cpp's own header comment on this exact symbol (Draft 35 finding
// 7) for the full, real rationale - this harness's own real
// view->drawView() call reaches the exact same default-filter branch.
Bool W3DShaderManager::filterPreRender(FilterTypes /*filter*/, Bool & /*skipRender*/, CustomScenePassModes & /*scenePassMode*/)
{
	return false;
}

Bool W3DShaderManager::filterPostRender(FilterTypes /*filter*/, FilterModes /*mode*/, Coord2D & /*scrollDelta*/, Bool & /*doExtraRender*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: W3DShaderManager::filterPostRender() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

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
	DEBUG_CRASH(("RenderNamedDrawableTest: CameraShakeSystemClass::Add_Camera_Shake() stub called - should be unreachable; see link_stubs.cpp"));
}

CameraShakeSystemClass CameraShakerSystem;

// W3DDisplay::m_3DScene/m_2DScene/m_assetManager - STATIC member variables
// of W3DDisplay (W3DDisplay.h), read directly by W3DView::draw()/
// updateTerrain() (m_3DScene/m_2DScene, Milestone 11's own established
// technique) and by W3DModelDraw's own real body (m_assetManager,
// ":632", "W3DDisplay::m_assetManager->Create_Render_Obj(...)" - THIS
// milestone's own new requirement, Draft 37's own "one hard link
// requirement"). W3DDisplay.cpp itself is deliberately NOT linked here
// (WIN32-gated, a standing non-goal per this milestone's own explicit
// non-goals list) - all three static members' storage is supplied here
// directly instead.
#include "W3DDevice/GameClient/W3DDisplay.h"
RTS3DScene *W3DDisplay::m_3DScene = nullptr;
RTS2DScene *W3DDisplay::m_2DScene = nullptr;
W3DAssetManager *W3DDisplay::m_assetManager = nullptr;

#include "W3DDevice/GameClient/WorldHeightMap.h"
Region2D WorldHeightMap::getDrawRegion2D()
{
	DEBUG_CRASH(("RenderNamedDrawableTest: WorldHeightMap::getDrawRegion2D() stub called - should be unreachable; see link_stubs.cpp"));
	Region2D r;
	r.zero();
	return r;
}

// ============================================================================
// ---- NEW for Milestone 13's own two device files (W3DModelDraw.cpp/
// W3DAssetManager.cpp) - genuine, real link errors this milestone's own
// step-0 spike measured (not predicted by static reading). ----
// ============================================================================

// TheW3DShadowManager: real singleton, normally constructed by the device
// layer's own real shadow-manager bring-up (out of scope - this harness has
// no shadow rendering concept). W3DModelDraw.cpp's own setModelState()/
// allocateShadows() both null-guard every real use ("if (m_renderObject &&
// TheW3DShadowManager && ...)") - a real, permanently-null pointer is
// genuinely safe, not merely unexercised (this harness's own INI-authored
// unit never sets a non-NONE ShadowType either, so this stays doubly dead).
#include "W3DDevice/GameClient/W3DShadow.h"
W3DShadowManager *TheW3DShadowManager = nullptr;

// W3DShadowManager::addShadow(): real, non-virtual, out-of-line body lives
// in the excluded device-layer W3DShadow.cpp - only ever reached through
// "TheW3DShadowManager->addShadow(...)" call sites, all real, null-guarded
// on TheW3DShadowManager first (same reasoning as TheW3DShadowManager
// itself, above) - link-live only.
Shadow *W3DShadowManager::addShadow(RenderObjClass* /*robj*/, Shadow::ShadowTypeInfo* /*shadowInfo*/, Drawable* /*draw*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: W3DShadowManager::addShadow() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

// TheTerrainTracksRenderObjClassSystem / TerrainTracksRenderObjClass::
// addCapEdgeToTrack()/addEdgeToTrack() / TerrainTracksRenderObjClassSystem::
// bindTrack()/unbindTrack(): real, already-portable definitions live in
// Core/GameEngineDevice/.../W3DDevice/GameClient/W3DTerrainTracks.cpp - NOT
// part of this harness's own link closure (a genuinely separate device file
// this milestone's own scope doesn't need - vehicle tread-mark rendering,
// no different in kind from every other out-of-scope W3D-device-tier
// feature this port's harnesses already stub out). Every real call site in
// W3DModelDraw.cpp null-guards on "TheTerrainTracksRenderObjClassSystem !=
// nullptr" first (setModelState():3072, ":3075) or is only reached via a
// real, live m_trackRenderObject this harness's own zero-shadow/zero-track
// unit never creates (setHidden()/reactToTransformChange()/~W3DModelDraw()) -
// link-live (vtable/address-of-style references only), never actually
// invoked at runtime.
#include "W3DDevice/GameClient/W3DTerrainTracks.h"
TerrainTracksRenderObjClassSystem *TheTerrainTracksRenderObjClassSystem = nullptr;

void TerrainTracksRenderObjClass::addCapEdgeToTrack(Real /*x*/, Real /*y*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: TerrainTracksRenderObjClass::addCapEdgeToTrack() stub called - should be unreachable; see link_stubs.cpp"));
}

void TerrainTracksRenderObjClass::addEdgeToTrack(Real /*x*/, Real /*y*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: TerrainTracksRenderObjClass::addEdgeToTrack() stub called - should be unreachable; see link_stubs.cpp"));
}

TerrainTracksRenderObjClass *TerrainTracksRenderObjClassSystem::bindTrack(RenderObjClass* /*renderObject*/, Real /*length*/, const Char* /*texturename*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: TerrainTracksRenderObjClassSystem::bindTrack() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

void TerrainTracksRenderObjClassSystem::unbindTrack(TerrainTracksRenderObjClass* /*mod*/)
{
	DEBUG_CRASH(("RenderNamedDrawableTest: TerrainTracksRenderObjClassSystem::unbindTrack() stub called - should be unreachable; see link_stubs.cpp"));
}
