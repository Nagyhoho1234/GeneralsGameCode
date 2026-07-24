// Phase 5(a) Milestone 12 (native port plan, Draft 36, "rung 2b") - harness-
// local link closure this harness needs beyond what its real, linked
// GameEngine.cpp/z_gameengine/corei_gameengine_private closure already
// provides. Starts from Tests/RenderViewUpdateDraw/link_stubs.cpp's own
// pruned set (that milestone's real link attempt already found the
// authoritative "what's genuinely still missing after linking the whole
// real closure" list for the SAME z_gameengine/corei_gameengine_private
// closure this harness also links) MINUS GameEngine::isTimeFrozen()/
// isGameHalted() (Milestone 10's own hand-written stubs would collide with
// the REAL GameEngine.cpp bodies THIS milestone is the first to actually
// link - see GameEngine.cpp's own header comment) MINUS the W3D-device-tier
// rendering statics (W3DShaderManager/CameraShakeSystem/W3DDisplay
// statics, PrepareShadows/DoShadows/DoTrees/DoParticles) Milestone 11 needed
// only because IT hand-linked W3DScene.cpp/W3DView.cpp - this milestone
// links NO GameEngineDevice-tree rendering sources at all (see this
// directory's own CMakeLists.txt header comment), so none of those are
// link-live here.
#include "PreRTS.h"

// ============================================================================
// ---- Genuinely still-needed singleton pointers: none of these classes'
// real .cpp files are part of the linked closure (all are either device-tier
// W3D/Win32 files or heavier not-yet-linked subsystems), same as Milestone
// 10/11's own established set. ----
// ============================================================================

#include "GameClient/Shadow.h"
// TheProjectedShadowManager: real symbol declared in GameClient/Shadow.h,
// normally defined in the device-layer W3DProjectedShadow.cpp (not linked
// here). RadiusDecal.cpp (part of the real, linked closure) unconditionally
// dereferences it inside RadiusDecalTemplate::createRadiusDecal(), but that
// method is only reached from real object/veterancy-ring code paths this
// harness's zero-object world never calls - link-live, runtime-dead.
ProjectedShadowManager *TheProjectedShadowManager = nullptr;

// TheIMEManager / CreateIMEManagerInterface(): the real singleton is only
// ever constructed by the device/GUI layer (out of scope). The real
// CreateIMEManagerInterface() body lives in GameClient/GUI/IMEManager.cpp,
// excluded (mbstring.h, matching every prior milestone's own established
// exclusion) - link-live via GameClient::init() (GameClient.cpp:353,
// "TheIMEManager = CreateIMEManagerInterface();" - genuinely CALLED for
// real this milestone, unlike Milestone 10/11, but the real body still
// returns nullptr safely: every real call site null-guards it).
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
// rather than a silent no-op - genuinely load-bearing THIS milestone: the
// real GameEngine::init()'s own catch(INIException) block calls
// RELEASE_CRASH on any INI load failure, so this is also this harness's
// real, loud signal if its own Data/INI scaffold is incomplete.
void ReleaseCrash(const char* reason)
{
	fprintf(stderr, "POSIXGAMEENGINEHARNESS: ReleaseCrash() stub called: %s\n", reason ? reason : "(null)");
	abort();
}

// OSDisplaySetBusyState()/oversizeTheTerrain()/doSkyBoxSet(): real
// definitions live in device-layer Win32OSDisplay.cpp/BaseHeightMap.cpp/
// W3DWater.cpp (all WIN32/terrain-mesh, not linked here) - link-live via
// GameLogic.cpp/ScriptActions.cpp's own real, now-linked bodies, runtime-dead
// (never actually called by anything this harness's main() invokes - no map
// is ever loaded).
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
	DEBUG_CRASH(("PosixGameEngineHarnessTest: MapObject::getWaypointID() stub called - should be unreachable; see link_stubs.cpp"));
	return (WaypointID)0;
}

AsciiString MapObject::getWaypointName()
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: MapObject::getWaypointName() stub called - should be unreachable; see link_stubs.cpp"));
	return AsciiString::TheEmptyString;
}

void MapObject::setName(AsciiString /*name*/)
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: MapObject::setName() stub called - should be unreachable; see link_stubs.cpp"));
}

void MapObject::setThingTemplate(const ThingTemplate* /*thing*/)
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: MapObject::setThingTemplate() stub called - should be unreachable; see link_stubs.cpp"));
}

const ThingTemplate *MapObject::getThingTemplate() const
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: MapObject::getThingTemplate() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

// INI::parseWebpageURLDefinition(): real definition lives in
// Common/INI/INIWebpageURL.cpp, excluded from this harness's closure
// (d3dx8math.h via Common/BezierSegment.h - this directory's own
// CMakeLists.txt exclusion list). Referenced by address only, inside
// INI.cpp's own static field-parser dispatch table - never called without
// real "WebpageURL" INI content, which this harness never authors.
#include "Common/INI.h"
void INI::parseWebpageURLDefinition(INI* /*ini*/)
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: INI::parseWebpageURLDefinition() stub called - should be unreachable; see link_stubs.cpp"));
}

// ============================================================================
// ---- Menu Init/Update/Shutdown/Input/System entry points: real definitions
// live in the excluded MainMenu.cpp/WOLLadderScreen.cpp/WOLLoginMenu.cpp/
// WOLWelcomeMenu.cpp/DownloadMenu.cpp (winsock.h/atlbase.h - this directory's
// own CMakeLists.txt exclusion list). ALL are referenced only by address,
// inside FunctionLexicon.cpp's own static winLayoutInitTable/
// winLayoutUpdateTable/winLayoutShutdownTable/gameWinInputTable/
// gameWinSystemTable dispatch tables (real, linked, vtable-completeness-
// style references - this harness never loads a .wnd layout or creates a
// real GameWindow, so none of these are ever actually invoked at runtime). ----
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

// setupGameStart()/DoResolutionDialog(): real definitions live in the
// excluded MainMenu.cpp/OptionsMenu.cpp is NOT excluded, but declares
// DoResolutionDialog() as `extern` and defines it in the excluded
// MainMenu.cpp - link-live via OptionsMenu.cpp's own real, linked body,
// runtime-dead (this harness never opens the options menu).
// setupGameStart() is likewise declared `extern` locally inside the
// excluded MainMenu.cpp and DifficultySelect.cpp (a real, linked file) -
// same reasoning.
#include "Common/GameCommon.h"
void setupGameStart(AsciiString /*mapName*/, GameDifficulty /*diff*/) {}
void DoResolutionDialog() {}

// testMinimumRequirements(): real definition lives in the device-layer
// W3DShaderManager.cpp (WIN32-gated, not linked here) - link-live via
// GameLOD.cpp's own real, linked GameLODManager::init()/
// GameLODManager::determineIntelligence() bodies, genuinely reached this
// milestone (GameEngine::init() real-constructs and real-init()s
// TheGameLODManager). A safe, "minimum requirements not met" default: only
// ever consulted for graphics-quality auto-detection, which this headless
// harness has no downstream observer for.
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

// WW3D::Get_Texture_Reduction(): real definition lives in the WW3D2
// rendering library (core_ww3d2, deliberately not linked here - this
// harness's own standing non-goal, same as every prior milestone). Link-live
// via GameLOD.cpp's own real, linked body; runtime-dead (this harness never
// loads a texture).
#include "ww3d.h"
int WW3D::Get_Texture_Reduction()
{
	return 0;
}

// TextureFilterClass::TextureFilterModeString / getTextureFilterMode():
// real definitions live in Core/Libraries/Source/WWVegas/WW3D2/
// texturefilter.cpp - part of the WW3D2 rendering closure this harness
// deliberately does not link. Reached only via OptionPreferences::
// getTextureFilterMode() (never actually invoked - no INI-driven options
// loading path this harness's plain "NEW GlobalData" construction reaches).
#include "texturefilter.h"
const char* const TextureFilterClass::TextureFilterModeString[TextureFilterClass::TEXTURE_FILTER_COUNT] =
{
	"NONE", "POINT", "BILINEAR", "TRILINEAR", "ANISOTROPIC"
};

TextureFilterClass::TextureFilterMode TextureFilterClass::getTextureFilterMode(const char* /*str*/)
{
	return TEXTURE_FILTER_BILINEAR;
}

// TheSmudgeManager: real singleton, only ever constructed by the device
// layer (W3DDevice's own smudge/decal rendering, out of scope - no rendering
// concept in this headless harness). Referenced by address from real, linked
// GameClient/GUI code that defensively null-guards every real use.
#include "GameClient/Smudge.h"
SmudgeManager *TheSmudgeManager = nullptr;

// UpdateLocalPlayerStats(): real definition lives in the excluded
// GameNetwork/GameSpy/PersistentStorageThread.cpp (GameSpy persistent-
// storage networking, a standing non-goal - this harness never constructs
// TheNetwork). Link-live via real, linked GameSpy-menu .cpp bodies this
// harness's own z_gameengine closure still includes; runtime-dead (no
// network game is ever played).
#include "GameNetwork/GameSpy/PersistentStorageDefs.h"
void UpdateLocalPlayerStats()
{
}

// GetStringFromRegistry()/GetUnsignedIntFromRegistry()/SetStringInRegistry()/
// SetUnsignedIntInRegistry() (std::string-based overload set,
// WWVegas/WWDownload/Registry.h - a SEPARATE, differently-typed quad from
// Common/System/Registry.h's own AsciiString-based GetRegistryLanguage()/
// GetRegistryGameName() helpers, which already have real, working
// non-Windows fallback bodies compiled into this harness for real):
// WWDownload/registry.cpp is genuinely Windows-registry-backed with no
// non-Windows fallback body anywhere in this tree (a real, pre-existing
// portability gap, not introduced by this harness) - referenced from
// OptionsMenu.cpp/PopupPlayerInfo.cpp/ScoreScreen.cpp (real, linked menu
// files), runtime-dead (this harness never opens the options menu or
// displays a score screen).
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

// DumbProjectileBehavior(Modulebata)::DumbProjectileBehavior()/
// buildFieldParse(): real definitions live in the excluded
// DumbProjectileBehavior.cpp (d3dx8math.h via Common/BezierSegment.h - this
// directory's own CMakeLists.txt exclusion list, same reasoning as every
// prior milestone's own established exclusion). Referenced by address only,
// from ModuleFactory.cpp's own real, linked module-registration table -
// this harness never constructs a real Object/module instance (zero-object
// world), so neither is ever actually invoked.
#include "GameLogic/Module/DumbProjectileBehavior.h"
DumbProjectileBehaviorModuleData::DumbProjectileBehaviorModuleData()
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: DumbProjectileBehaviorModuleData::DumbProjectileBehaviorModuleData() stub called - should be unreachable; see link_stubs.cpp"));
}

void DumbProjectileBehaviorModuleData::buildFieldParse(MultiIniFieldParse& /*p*/)
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: DumbProjectileBehaviorModuleData::buildFieldParse() stub called - should be unreachable; see link_stubs.cpp"));
}

DumbProjectileBehavior::DumbProjectileBehavior( Thing *thing, const ModuleData* moduleData )
	: UpdateModule( thing, moduleData )
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: DumbProjectileBehavior::DumbProjectileBehavior() stub constructor called - should be unreachable; see link_stubs.cpp"));
}

// DumbProjectileBehavior's remaining 6 out-of-line virtuals
// (update()/projectileLaunchAtObjectOrPosition()/
// projectileFireAtObjectOrPosition()/projectileHandleCollision()/crc()/
// xfer()/loadPostProcess()) - same real-definition-lives-in-the-excluded-
// .cpp, same vtable-completeness reasoning as the constructor/destructor
// above (a real, running gdb-free link attempt is the authority on the
// full set - every one of these appeared as a real "undefined reference to
// vtable" error, not predicted).
UpdateSleepTime DumbProjectileBehavior::update()
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: DumbProjectileBehavior::update() stub called - should be unreachable; see link_stubs.cpp"));
	return UPDATE_SLEEP_NONE;
}

void DumbProjectileBehavior::projectileLaunchAtObjectOrPosition(const Object* /*victim*/, const Coord3D* /*victimPos*/, const Object* /*launcher*/, WeaponSlotType /*wslot*/, Int /*specificBarrelToUse*/, const WeaponTemplate* /*detWeap*/, const ParticleSystemTemplate* /*exhaustSysOverride*/)
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: DumbProjectileBehavior::projectileLaunchAtObjectOrPosition() stub called - should be unreachable; see link_stubs.cpp"));
}

void DumbProjectileBehavior::projectileFireAtObjectOrPosition(const Object* /*victim*/, const Coord3D* /*victimPos*/, const WeaponTemplate* /*detWeap*/, const ParticleSystemTemplate* /*exhaustSysOverride*/)
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: DumbProjectileBehavior::projectileFireAtObjectOrPosition() stub called - should be unreachable; see link_stubs.cpp"));
}

Bool DumbProjectileBehavior::projectileHandleCollision(Object* /*other*/)
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: DumbProjectileBehavior::projectileHandleCollision() stub called - should be unreachable; see link_stubs.cpp"));
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

// ~DumbProjectileBehavior(): MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE's own
// macro expansion declares "virtual ~ARGCLASS() override;" (GameMemory.h) -
// a real, out-of-line virtual destructor this class's own vtable needs
// resolved (Itanium C++ ABI: any constructor requires the COMPLETE vtable,
// not just the members actually called) - real definition lives in the same
// excluded DumbProjectileBehavior.cpp as the constructor above.
DumbProjectileBehavior::~DumbProjectileBehavior()
{
}

// MapObject::MapObject()/MapObject::TheWorldDict: real, non-virtual bodies
// genuinely live in the heavy, device-layer WorldHeightMap.cpp (terrain mesh
// rendering + map loading, a standing non-goal, not linked here - same
// real, pre-existing codebase quirk this harness's other MapObject stubs
// already document). Referenced by address only from real, linked map-
// loading-adjacent code (e.g. ScriptEngine.cpp's own script-action
// dispatch); this harness never loads a map, so the real constructor body
// is never actually invoked.
MapObject::MapObject(Coord3D /*loc*/, AsciiString /*name*/, Real /*angle*/, Int /*flags*/, const Dict* /*props*/, const ThingTemplate* /*thingTemplate*/)
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: MapObject::MapObject() stub constructor called - should be unreachable; see link_stubs.cpp"));
}

// ~MapObject(): same MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE vtable-
// completeness reason as DumbProjectileBehavior's own destructor above.
MapObject::~MapObject()
{
}

Dict MapObject::TheWorldDict;

// ReleaseCrashLocalized(): same real reasoning as ReleaseCrash() above (real
// definition lives in the excluded Debug.cpp) - a second, localized-message
// overload GameEngine::init()'s own catch(ErrorCode) block calls on
// ERROR_INVALID_D3D (never thrown by this harness - no real Direct3D device
// is ever created).
void ReleaseCrashLocalized(const AsciiString& p, const AsciiString& m)
{
	fprintf(stderr, "POSIXGAMEENGINEHARNESS: ReleaseCrashLocalized() stub called: %s / %s\n", p.str(), m.str());
	abort();
}

// WOLWelcomeMenuSystem()/DownloadMenuSystem(): real definitions live in the
// excluded WOLWelcomeMenu.cpp/DownloadMenu.cpp (winsock.h/atlbase.h -
// this directory's own CMakeLists.txt exclusion list, same reasoning as
// every prior milestone's own established exclusion). Referenced by address
// only, inside FunctionLexicon.cpp's own static gameWinSystemTable dispatch
// table (real, linked, vtable-completeness-style reference, not something
// this harness's own zero-window construction ever actually invokes -
// TheGameClient never creates a real GameWindow in this harness).
#include "GameClient/GUICallbacks.h"
WindowMsgHandledType WOLWelcomeMenuSystem( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ )
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: WOLWelcomeMenuSystem() stub called - should be unreachable; see link_stubs.cpp"));
	return MSG_IGNORED;
}

WindowMsgHandledType DownloadMenuSystem( GameWindow * /*window*/, UnsignedInt /*msg*/, WindowMsgData /*mData1*/, WindowMsgData /*mData2*/ )
{
	DEBUG_CRASH(("PosixGameEngineHarnessTest: DownloadMenuSystem() stub called - should be unreachable; see link_stubs.cpp"));
	return MSG_IGNORED;
}

// g_strFile / g_csfFile: real, genuine "owned by main()" globals - declared
// `extern` locally inside GameText.cpp itself (not a shared header) and
// defined only in each real executable's own WinMain.cpp entry point (every
// one of WinMain.cpp/wdump.cpp/autorun.cpp/W3DView.cpp/the WorldBuilder/
// GUIEdit/ImagePacker/MapCacheBuilder tool mains - grep-confirmed, no shared
// definition exists). GENUINELY reached this milestone (unlike Milestone
// 10/11): GameEngine::init()'s own "initSubsystem(TheGameText, ...,
// CreateGameTextInterface(), nullptr)" call really calls
// GameTextManager::init() for real, which references g_strFile
// unconditionally (GameText.cpp:309,339) - reading the real body confirms
// this is a real filename it tries to open (via TheFileSystem), gracefully
// handling the "file does not exist" case rather than crashing (this
// harness authors no such file, matching every prior milestone's own
// "no real INI/string-table content" non-goal). Same real string literal
// GeneralsMD's own WinMain.cpp uses, for parity.
const Char *g_strFile = "data\\Generals.str";
const Char *g_csfFile = "data\\%s\\Generals.csf";

// All TheKey_* well-known Dict-key globals (Common/WellKnownKeys.h's
// DEFINE_KEY macro) are genuinely, only ever DEFINED (not merely declared)
// in Core/GameEngineDevice/.../WorldHeightMap.cpp ("#define
// INSTANTIATE_WELL_KNOWN_KEYS" before including WellKnownKeys.h) - a real,
// pre-existing codebase quirk (grep-confirmed: no other definition exists
// anywhere). Matches Milestone 10/11's own established technique: define the
// SAME macro the real WorldHeightMap.cpp does and include the SAME real
// header from this harness's own TU instead of the heavy (WW3D2-dependent,
// out of scope) WorldHeightMap.cpp.
#define INSTANTIATE_WELL_KNOWN_KEYS
#include "Common/WellKnownKeys.h"
