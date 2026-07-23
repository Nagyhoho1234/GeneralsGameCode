// Harness-local link stubs (native port plan Phase 5(a) Milestone 9 Task 3,
// Draft 32) - this harness constructs the game's real RTS3DScene (unchanged
// from Tests/RenderRTS3DScene, see that file's own comments for the
// unchanged rationale of every stub carried over verbatim below) PLUS, NEW
// for this harness, a real base TerrainLogic and a real, WHOLE W3DView.cpp
// TU (only updateCameraTransform() is ever CALLED; every other virtual
// method - draw()/drawView()/update()/pickDrawable()/
// iterateDrawablesInRegion() - must still compile and link because it is
// the SAME translation unit / vtable, but is never invoked at runtime). Its
// link closure needs both TerrainLogic-specific stubs (Task 2's own
// scratch-harness finding, task-2-report.md: PartitionManager,
// TheGhostObjectManager, TheRadar, TheTacticalView, TheGameLogic, plus
// Drawable.cpp's TintEnvelope vtable and CachedFileInputStream/
// DataChunkInput pulled in via TerrainLogic::loadMap) AND a NEW set for
// W3DView.o's own unreachable-but-must-link closure, discovered by the
// REAL LINKER during this task's own development (not predicted by static
// reading alone - task brief item 5). Follows the established
// anim_sound_link_stub.cpp / Tests/RenderGameAssets/Tests/RenderRTS3DScene
// link_stubs.cpp pattern: loud comments, no hidden behavior.

// ============================================================================
// ---- Carried over UNCHANGED from Tests/RenderRTS3DScene/link_stubs.cpp
// (same construction: real GlobalData + real RTS3DScene + real W3DScene.cpp/
// W3DShroud.cpp/GameUtility.cpp). See that file's own comments for the full,
// unchanged rationale of every stub in this block. ----
// ============================================================================

#include "Common/SubsystemInterface.h"
SubsystemInterfaceList *TheSubsystemList = nullptr;

#include "Common/GameAudio.h"
AudioManager *TheAudio = nullptr;

// TheSuperHackers @note Milestone 9 Task 3: TheGameLogic is now ALSO
// link-live (not just runtime-dead) from a SECOND, NEW source beyond Tests/
// RenderRTS3DScene's original rationale (below, unchanged): W3DView::update()
// (unreachable-but-must-link, see this file's own header comment)
// unconditionally calls "!TheGameLogic->isGamePaused()" with NO null guard
// (W3DView.cpp:1577,1717) - link-live because update() is a real vtable
// entry, runtime-dead because this harness never calls update() (only the
// private updateCameraTransform(), via a test-only friend grant - see
// W3DView.h and main.cpp). isGamePaused() itself needs its own stub below
// (it is not inline).
//
// Original Tests/RenderRTS3DScene rationale, unchanged: RTS3DScene::
// Visibility_Check reads "TheGameLogic ? TheGameLogic->getFrame() : 0" and
// "TheGlobalData->m_enableBehindBuildingMarkers && TheGameLogic &&
// TheGameLogic->getShowBehindBuildingMarkers()" - both explicitly
// null-guarded. renderOneObject's ghost-object branch is only reached with a
// real DrawableInfo, never true for this harness's plain quad. getFrame()/
// getShowBehindBuildingMarkers()/findObjectByID() are all fully inline in
// GameLogic.h.
#include "GameLogic/GameLogic.h"
GameLogic *TheGameLogic = nullptr;

#include "W3DDevice/GameClient/W3DShadow.h"
W3DShadowManager *TheW3DShadowManager = nullptr;

// TheSuperHackers @note Milestone 9 Task 3: TheTacticalView's DEFINITION
// (unlike every other Tests/RenderRTS3DScene-carried-over stub in this
// block) is NOT supplied here - it is now the real one, "View
// *TheTacticalView = nullptr;" (View.cpp:41), since this harness links the
// real, already-portable View.cpp for real (W3DView's own base class -
// needed for its vtable). Still stays null for this harness's entire run,
// same value/rationale as every other deliberately-absent singleton in
// this file - just defined in the real TU instead of a stub now.
#include "GameClient/View.h"

#include "GameClient/ParticleSys.h"
ParticleSystemManager *TheParticleSystemManager = nullptr;

#include "W3DDevice/GameClient/BaseHeightMap.h"
BaseHeightMapRenderObjClass *TheTerrainRenderObject = nullptr;

#include "Common/Override.h"
#include "GameClient/ControlBar.h"
ControlBar *TheControlBar = nullptr;
#include "Common/PlayerList.h"
PlayerList *ThePlayerList = nullptr;

#include "Common/version.h"
Version *TheVersion = nullptr;

UnsignedInt Version::getVersionNumber() const
{
	return 0;
}

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

#include "Common/Debug.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Object.h"

const Vector3 *Drawable::getTintColor() const
{
	return nullptr;
}

const Vector3 *Drawable::getSelectionColor() const
{
	return nullptr;
}

Bool Thing::isKindOf(KindOfType /*t*/) const
{
	return false;
}

Player *Object::getControllingPlayer() const
{
	DEBUG_CRASH(("RenderCameraTransformTest: Object::getControllingPlayer() stub called - this should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

ObjectShroudStatus Object::getShroudedStatus(Int /*playerIndex*/) const
{
	return OBJECTSHROUD_CLEAR;
}

void Drawable::friend_lockDirtyStuffForIteration()
{
}

void Drawable::friend_unlockDirtyStuffForIteration()
{
}

// TheSuperHackers @note Milestone 9 Task 3: TintEnvelope's vtable (crc/
// xfer/loadPostProcess, its own Snapshot overrides) is pulled in transitively
// by this file's own "#include "GameClient/Drawable.h"" above (Drawable.h
// declares/uses TintEnvelope inline) - exactly the gap Task 2's own
// scratch-harness finding predicted ("Drawable.cpp's TintEnvelope vtable"),
// now confirmed by the real linker for this harness. No TintEnvelope
// instance is ever constructed by anything this harness calls (this
// harness's one render object is never given a Drawable at all - see the
// accessor-stub comments above), so these three bodies are never actually
// invoked; empty/zero bodies are safe, matching this file's established
// "harness-local member-function stub" discipline.
#include "Common/Xfer.h"

void TintEnvelope::crc(Xfer * /*xfer*/)
{
}

void TintEnvelope::xfer(Xfer * /*xfer*/)
{
}

void TintEnvelope::loadPostProcess()
{
}

// ============================================================================
// ---- NEW for this harness: TerrainLogic.cpp's own link closure (Task 2's
// scratch-harness finding, task-2-report.md, quoted verbatim in this task's
// brief) - PartitionManager, TheGhostObjectManager, plus Drawable.cpp's
// TintEnvelope vtable and CachedFileInputStream/DataChunkInput (pulled in
// via TerrainLogic::loadMap, never called at runtime by this harness - its
// TerrainLogic is never given a map to load - but still needing to link
// because loadMap() is a real, non-virtual, out-of-line member function
// compiled into the TerrainLogic.o translation unit regardless of whether
// this harness's main() ever calls it). TheRadar/TheTacticalView/TheGameLogic
// are already covered above. ----
// ============================================================================

#include "GameLogic/PartitionManager.h"
PartitionManager *ThePartitionManager = nullptr;

#include "GameLogic/GhostObject.h"
GhostObjectManager *TheGhostObjectManager = nullptr;

#include "Common/Radar.h"
Radar *TheRadar = nullptr;

// TheTerrainVisual (GameClient/TerrainVisual.h) - only reached from
// TerrainLogic::loadMap() (link-live via the vtable, runtime-dead - this
// harness never loads a map).
#include "GameClient/TerrainVisual.h"
TerrainVisual *TheTerrainVisual = nullptr;

// MapObject::TheMapObjectListPtr - static member storage only (real
// definition lives in the heavy, per-tree MapObject.cpp, not linked here).
// MapObject::getFirstMapObject() is inline in MapObject.h and reads this
// directly; only reached from TerrainLogic::loadMap() (link-live,
// runtime-dead).
#include "Common/MapObject.h"
MapObject *MapObject::TheMapObjectListPtr = nullptr;

// PartitionManager::processEntirePendingUndoShroudRevealQueue()/
// storeFoggedCells()/restoreFoggedCells() - non-virtual, out-of-line real
// bodies in the heavy, per-tree PartitionManager.cpp (not linked here).
// Only reached from TerrainLogic::setActiveBoundary() (non-virtual, never
// called by this harness - map-boundary/shroud bookkeeping this harness's
// flat, mapless TerrainLogic never exercises).
#include "GameLogic/PartitionManager.h"
void PartitionManager::processEntirePendingUndoShroudRevealQueue()
{
	DEBUG_CRASH(("RenderCameraTransformTest: PartitionManager::processEntirePendingUndoShroudRevealQueue() stub called - should be unreachable; see link_stubs.cpp"));
}

void PartitionManager::storeFoggedCells(ShroudStatusStoreRestore & /*outPartitionStore*/, Bool /*storeToFog*/) const
{
	DEBUG_CRASH(("RenderCameraTransformTest: PartitionManager::storeFoggedCells() stub called - should be unreachable; see link_stubs.cpp"));
}

void PartitionManager::restoreFoggedCells(const ShroudStatusStoreRestore & /*inPartitionStore*/, Bool /*restoreToFog*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: PartitionManager::restoreFoggedCells() stub called - should be unreachable; see link_stubs.cpp"));
}

// GameLogic::getFirstObject() - same rationale (TerrainLogic::
// setActiveBoundary(), never called).
Object *GameLogic::getFirstObject()
{
	DEBUG_CRASH(("RenderCameraTransformTest: GameLogic::getFirstObject() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

// Object::friend_prepareForMapBoundaryAdjust()/friend_notifyOfNewMapBoundary()
// - non-virtual, out-of-line real bodies in the heavy, per-tree Object.cpp
// (not linked here). Same rationale (TerrainLogic::setActiveBoundary(),
// never called).
void Object::friend_prepareForMapBoundaryAdjust()
{
	DEBUG_CRASH(("RenderCameraTransformTest: Object::friend_prepareForMapBoundaryAdjust() stub called - should be unreachable; see link_stubs.cpp"));
}

void Object::friend_notifyOfNewMapBoundary()
{
	DEBUG_CRASH(("RenderCameraTransformTest: Object::friend_notifyOfNewMapBoundary() stub called - should be unreachable; see link_stubs.cpp"));
}

// PolygonTrigger::getPolygonTriggerByID()/getWaterHandle() - only reached
// from TerrainLogic::xfer() (link-live via the vtable - xfer is virtual -
// runtime-dead, this harness never xfers/saves its TerrainLogic).
#include "GameLogic/PolygonTrigger.h"
PolygonTrigger *PolygonTrigger::getPolygonTriggerByID(Int /*triggerID*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: PolygonTrigger::getPolygonTriggerByID() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

const WaterHandle *PolygonTrigger::getWaterHandle() const
{
	DEBUG_CRASH(("RenderCameraTransformTest: PolygonTrigger::getWaterHandle() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

Bool PolygonTrigger::pointInTrigger(ICoord3D & /*point*/) const
{
	DEBUG_CRASH(("RenderCameraTransformTest: PolygonTrigger::pointInTrigger() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

void PolygonTrigger::setPoint(const ICoord3D & /*point*/, Int /*ndx*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: PolygonTrigger::setPoint() stub called - should be unreachable; see link_stubs.cpp"));
}

// ---- TerrainLogic.cpp's OWN nested Bridge class + its non-virtual bridge/
// waypoint/pathfind-adjacent methods (addBridgeToLogic/
// addLandmarkBridgeToLogic/deleteBridge/updateBridgeDamageStates/
// addWaypoint/setActiveBoundary/getWaterHandle/setWaterHeight/
// getLayerForDestination/objectInteractsWithBridgeLayer, none ever called
// by this harness) pull in a real, substantial closure of their own -
// GameLogic::destroyObject, Object::attemptDamage/
// updateObjValuesFromMapProperties, PartitionManager::
// iterateObjectsInRange, Pathfinder::addBridge/changeBridgeState/
// forceMapRecalculation/isPointOnWall, SimpleObjectIterator::
// nextWithNumeric/reset, TerrainRoadCollection::findBridge, Thing::
// getTemplate/setOrientation/setPosition, ThingFactory::
// findTemplateInternal/newObject, TheTerrainRoads/TheThingFactory, and the
// DamageInfo/DamageInfoInput/DamageInfoOutput Snapshot vtables (their
// crc()/loadPostProcess() are already inline no-ops in Damage.h; only
// xfer() is out-of-line). All discovered by the real linker (task brief
// item 5), not predicted by static reading. All genuinely link-live
// (loadPostProcess()/xfer()/crc() are virtual TerrainLogic overrides;
// everything else is reached transitively from THOSE, or from other
// already-established link-live-but-never-called non-virtual TerrainLogic
// methods) and genuinely runtime-dead (this harness never loads a map, so
// TerrainLogic's bridge/waypoint lists stay empty for its entire run, and
// none of these methods are ever actually invoked).
#include "GameLogic/GameLogic.h"
void GameLogic::destroyObject(Object * /*obj*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: GameLogic::destroyObject() stub called - should be unreachable; see link_stubs.cpp"));
}

void Object::attemptDamage(DamageInfo * /*damageInfo*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: Object::attemptDamage() stub called - should be unreachable; see link_stubs.cpp"));
}

void Object::updateObjValuesFromMapProperties(Dict * /*properties*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: Object::updateObjValuesFromMapProperties() stub called - should be unreachable; see link_stubs.cpp"));
}

SimpleObjectIterator *PartitionManager::iterateObjectsInRange(
	const Coord3D * /*pos*/, Real /*maxDist*/, DistanceCalculationType /*dc*/,
	PartitionFilter ** /*filters*/, IterOrderType /*order*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: PartitionManager::iterateObjectsInRange() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

#include "GameLogic/AIPathfind.h"
PathfindLayerEnum Pathfinder::addBridge(Bridge * /*theBridge*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: Pathfinder::addBridge() stub called - should be unreachable; see link_stubs.cpp"));
	return LAYER_GROUND;
}

void Pathfinder::changeBridgeState(PathfindLayerEnum /*layer*/, Bool /*repaired*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: Pathfinder::changeBridgeState() stub called - should be unreachable; see link_stubs.cpp"));
}

void Pathfinder::forceMapRecalculation()
{
	DEBUG_CRASH(("RenderCameraTransformTest: Pathfinder::forceMapRecalculation() stub called - should be unreachable; see link_stubs.cpp"));
}

Bool Pathfinder::isPointOnWall(const Coord3D * /*pos*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: Pathfinder::isPointOnWall() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

#include "GameLogic/ObjectIter.h"
Object *SimpleObjectIterator::nextWithNumeric(Real * /*num*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: SimpleObjectIterator::nextWithNumeric() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

void SimpleObjectIterator::reset()
{
	DEBUG_CRASH(("RenderCameraTransformTest: SimpleObjectIterator::reset() stub called - should be unreachable; see link_stubs.cpp"));
}

#include "GameClient/TerrainRoads.h"
TerrainRoadCollection *TheTerrainRoads = nullptr;

TerrainRoadType *TerrainRoadCollection::findBridge(AsciiString /*name*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: TerrainRoadCollection::findBridge() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

// Thing::getTemplate()/setOrientation()/setPosition() - non-virtual,
// out-of-line real bodies in the heavy, per-tree Object.cpp/Thing.cpp (not
// linked here).
const ThingTemplate *Thing::getTemplate() const
{
	DEBUG_CRASH(("RenderCameraTransformTest: Thing::getTemplate() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

void Thing::setOrientation(Real /*angle*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: Thing::setOrientation() stub called - should be unreachable; see link_stubs.cpp"));
}

void Thing::setPosition(const Coord3D * /*pos*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: Thing::setPosition() stub called - should be unreachable; see link_stubs.cpp"));
}

#include "Common/ThingFactory.h"
ThingFactory *TheThingFactory = nullptr;

ThingTemplate *ThingFactory::findTemplateInternal(const AsciiString & /*name*/, Bool /*check*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: ThingFactory::findTemplateInternal() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

Object *ThingFactory::newObject(const ThingTemplate * /*tmplate*/, Team * /*team*/, ObjectStatusMaskType /*statusMask*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: ThingFactory::newObject() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

// DamageInfoInput::xfer()/DamageInfoOutput::xfer()/DamageInfo::xfer() - the
// only out-of-line Snapshot overrides in Damage.h (crc()/loadPostProcess()
// are already inline no-ops there) - needed for these three classes'
// vtables to resolve (Object::attemptDamage()'s DamageInfo parameter type,
// itself link-live-but-never-called, above).
#include "GameLogic/Damage.h"
void DamageInfoInput::xfer(Xfer * /*xfer*/)
{
}

void DamageInfoOutput::xfer(Xfer * /*xfer*/)
{
}

void DamageInfo::xfer(Xfer * /*xfer*/)
{
}

// TheNameKeyGenerator (Common/NameKeyGenerator.h) - StaticNameKey::key()'s
// real body (linked for real, NameKeyGenerator.cpp) already null-guards it
// internally. Its DEFINITION is the real one too (NameKeyGenerator.cpp
// itself defines "NameKeyGenerator *TheNameKeyGenerator = nullptr;") -
// same rationale as TheTacticalView above, no stub needed here.

// TheKey_waypointPathLabel1/2/3/TheKey_waypointPathBiDirectional
// (Common/WellKnownKeys.h's DEFINE_KEY macro) - genuinely, only ever
// DEFINED (not merely declared) in Core/GameEngineDevice/.../
// WorldHeightMap.cpp ("#define INSTANTIATE_WELL_KNOWN_KEYS" before
// including WellKnownKeys.h) - a real, pre-existing codebase quirk this
// task found (not an omission introduced here), confirmed by grepping the
// WHOLE repository: no other definition exists anywhere. Linking the real,
// heavy WorldHeightMap.cpp (terrain mesh rendering, a standing non-goal)
// just for these four small key objects would be wildly disproportionate,
// so they are re-defined here instead - using the EXACT SAME macro
// expansion DEFINE_KEY(NAME) would produce ("extern const StaticNameKey
// TheKey_##NAME; const StaticNameKey TheKey_##NAME(#NAME);"), same class,
// same constructor, same string literal per name (WellKnownKeys.h:511,
// 518, 525, 533) - a real, behaviorally-identical definition relocated to
// where it's actually needed, not an approximation. Only reached from
// TerrainLogic::addWaypoint() (non-virtual, only called from loadMap(),
// never invoked by this harness).
// Matches DEFINE_KEY(NAME)'s exact two-statement expansion - the leading
// "extern const StaticNameKey ...;" declaration is REQUIRED, not
// decorative: a bare "const StaticNameKey X(...)" definition would have
// internal (file-local) linkage by default in C++ (unlike C), which is
// exactly why a first standalone "extern" declaration must precede it here
// to match the real macro's own two-part linkage-fixing idiom.
extern const StaticNameKey TheKey_waypointPathLabel1;
const StaticNameKey TheKey_waypointPathLabel1("waypointPathLabel1");
extern const StaticNameKey TheKey_waypointPathLabel2;
const StaticNameKey TheKey_waypointPathLabel2("waypointPathLabel2");
extern const StaticNameKey TheKey_waypointPathLabel3;
const StaticNameKey TheKey_waypointPathLabel3("waypointPathLabel3");
extern const StaticNameKey TheKey_waypointPathBiDirectional;
const StaticNameKey TheKey_waypointPathBiDirectional("waypointPathBiDirectional");

// BridgeBehavior::getBridgeBehaviorInterfaceFromObject() /
// BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject() - static,
// non-virtual, out-of-line real bodies in the heavy, per-tree
// BridgeBehavior.cpp/BridgeTowerBehavior.cpp module TUs (not linked here).
// Only reached from Bridge::createTower()/Bridge::Bridge(Object*) (never
// called - see this block's own header comment).
#include "GameLogic/Module/BridgeBehavior.h"
BridgeBehaviorInterface *BridgeBehavior::getBridgeBehaviorInterfaceFromObject(Object * /*obj*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: BridgeBehavior::getBridgeBehaviorInterfaceFromObject() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

#include "GameLogic/Module/BridgeTowerBehavior.h"
BridgeTowerBehaviorInterface *BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject(Object * /*obj*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

// MapObject::getWaypointID()/getWaypointName() - non-virtual, out-of-line
// real bodies genuinely live in the heavy, per-tree WorldHeightMap.cpp (a
// real, pre-existing codebase quirk - grep-confirmed no other definition
// exists anywhere in the repository), not linked here (terrain mesh
// rendering, a standing non-goal). Only reached from TerrainLogic::
// addWaypoint() (never called - see this block's own header comment).
WaypointID MapObject::getWaypointID()
{
	DEBUG_CRASH(("RenderCameraTransformTest: MapObject::getWaypointID() stub called - should be unreachable; see link_stubs.cpp"));
	return (WaypointID)0;
}

AsciiString MapObject::getWaypointName()
{
	DEBUG_CRASH(("RenderCameraTransformTest: MapObject::getWaypointName() stub called - should be unreachable; see link_stubs.cpp"));
	return AsciiString::TheEmptyString;
}

// PolygonTrigger::ThePolygonTriggerListPtr - static member storage only
// (real definition lives in the heavy, per-tree PolygonTrigger.cpp, not
// linked here - see this file's earlier PolygonTrigger::deleteTriggers()
// stub comment for the same rationale). Read directly by
// PolygonTrigger::getFirstPolygonTrigger() (inline, PolygonTrigger.h:105),
// itself only reached from TerrainLogic::getTriggerAreaByName()/
// getWaterHandleByName()/getWaterHandle() (never called - see this block's
// own header comment). This harness never loads a map, so this stays at
// its own real default (nullptr) for the harness's entire run.
PolygonTrigger *PolygonTrigger::ThePolygonTriggerListPtr = nullptr;

// TerrainLogic::~TerrainLogic() unconditionally calls reset() (TerrainLogic.cpp:988-993),
// which unconditionally calls PolygonTrigger::deleteTriggers() (TerrainLogic.cpp:1006-1014) -
// genuinely RUNTIME-live (not just link-live): this harness's own teardown really calls
// "delete TheTerrainLogic" (main.cpp). PolygonTrigger.cpp is not linked here (a heavier,
// Snapshot-derived per-tree TU this harness has no other need for). Real body
// (PolygonTrigger.cpp:334-340) deletes PolygonTrigger::ThePolygonTriggerListPtr's list
// contents via deleteInstance(); this harness never loads a map, so that static member
// stays at its own real default (nullptr, PolygonTrigger.cpp:39) for the harness's entire
// run - deleteInstance(nullptr) is this codebase's standard safe no-op pattern, so the real
// body's own effect here is already a true no-op. An empty body is exactly behaviorally
// equivalent, not an approximation - and does not need ThePolygonTriggerListPtr's own
// storage defined, since it never touches it.
#include "GameLogic/PolygonTrigger.h"
void PolygonTrigger::deleteTriggers()
{
}

// ============================================================================
// ---- NEW for this harness: CameraShakeSystem harness-local stub (Draft 32
// design decisions / this task's own finding - see CMakeLists.txt's header
// comment for the full "why a stub, not the real TU" rationale: the real
// Core/GameEngineDevice/.../CameraShakeSystem.cpp is WIN32-gated AND
// unconditionally #includes d3dx8core.h, a genuinely non-portable header
// with no PortableD3D8 equivalent today). CameraShakeSystemClass's own
// class DEFINITION lives in CameraShakeSystem.h (included transitively by
// W3DView.cpp), so this harness supplies real, out-of-line, TRUE-NO-OP
// bodies for its four methods (true no-ops because Add_Camera_Shake() is
// never called by this harness, so the real CameraShakerList would stay
// empty regardless - these stubs are behaviorally IDENTICAL to what the
// real bodies would do with an empty list, not an approximation) plus the
// one real global instance every W3DView.cpp method expects by name. ----
// ============================================================================
#include "W3DDevice/GameClient/CameraShakeSystem.h"

CameraShakeSystemClass::CameraShakeSystemClass()
{
}

CameraShakeSystemClass::~CameraShakeSystemClass()
{
	// Real body (CameraShakeSystem.cpp:188-198) deletes every remaining
	// CameraShakerClass out of CameraShakerList. This harness's
	// CameraShakerList is always empty (Add_Camera_Shake() is never
	// called), so the real body's own loop would run zero iterations -
	// an empty body here is exactly behaviorally equivalent, not a
	// shortcut.
}

void CameraShakeSystemClass::Timestep(float /*dt*/)
{
	// Real body (CameraShakeSystem.cpp:237-261) iterates CameraShakerList,
	// timestepping and expiring each shaker. Always-empty list here (see
	// above) makes the real body's own loop a no-op - this stub is that
	// same no-op, not an approximation.
}

bool CameraShakeSystemClass::IsCameraShaking()
{
	// Real body (CameraShakeSystem.cpp:221-234) returns true iff
	// CameraShakerList has any entry. Always empty here, so false is the
	// exact real answer, not a guess. (Link-live only via W3DView::update()'s
	// unreachable-but-must-link body, W3DView.cpp:1603 - never actually
	// called since this harness never calls update().)
	return false;
}

void CameraShakeSystemClass::Update_Camera_Shaker(Vector3 /*camera_position*/, Vector3 *shaker_angle)
{
	// Real body (CameraShakeSystem.cpp:263-299) accumulates each active
	// shaker's contribution into *shaker_angle, starting from (0,0,0).
	// Always-empty list here means the real accumulation loop contributes
	// nothing - (0,0,0) is the exact real answer for this harness's
	// configuration, matching buildCameraTransform()'s own use of the
	// result (transform->Rotate_X/Y/Z(m_shakerAngles.X/Y/Z), a true no-op
	// rotation when all three are zero).
	*shaker_angle = Vector3(0.0f, 0.0f, 0.0f);
}

// The one real global instance - matches CameraShakeSystem.cpp's own
// "CameraShakeSystemClass CameraShakerSystem;" declaration exactly (same
// name, same type, same translation-unit-scope global), just constructed
// via this harness's stub ctor above instead of the real (non-portable) TU.
CameraShakeSystemClass CameraShakerSystem;

// ============================================================================
// ---- NEW for this harness: TheNetwork (GameNetwork/NetworkInterface.h) -
// FramePacer::getActualLogicTimeScaleFps() (real, linked TU, called for
// real by getLogicTimeStepMilliseconds(), which THIS harness's
// buildCameraTransform() call chain genuinely invokes) reads
// "if (TheNetwork != nullptr) { return TheNetwork->getFrameRate(); }" -
// null-guarded, so this harness (no networking, standing deferral) is safe
// with TheNetwork == nullptr. ----
// ============================================================================
#include "GameNetwork/NetworkInterface.h"
NetworkInterface *TheNetwork = nullptr;

// ============================================================================
// ---- NEW for this harness: W3DView.o's own unreachable-but-must-link
// closure (draw()/drawView()/update()/pickDrawable()/
// iterateDrawablesInRegion(), all link-live via the vtable since this
// harness constructs a real, concrete W3DView, all runtime-dead since this
// harness only ever CALLS updateCameraTransform() via the friend grant -
// see W3DView.h and main.cpp). Discovered by the REAL LINKER during this
// task's own development (task brief item 5), not predicted by static
// reading alone. ----
// ============================================================================

// TheDisplay (GameClient/Display.h) - genuinely RUNTIME-live (not merely
// link-live): View::scaleCameraHeightForAspectRatio() (View.cpp:130-155)
// checks "if (TheDisplay == nullptr) return height;" FIRST, a real,
// null-guarded early-out this harness's own real call chain reaches
// (getCameraOffsetZ() -> scaleCameraHeightForAspectRatio(), called from
// buildCameraPosition(), called from the real updateCameraTransform() this
// harness calls via friend). TheDisplay stays null deliberately - matching
// TheTerrainRenderObject/TheRadar/TheWindowManager (task brief item 3),
// even though the plan's finding 5 did not name TheDisplay in that list
// explicitly (a real gap this task found: TheDisplay is genuinely
// null-guarded in the camera-transform-core path too, just via a different,
// General-purpose helper function rather than inside updateCameraTransform()
// itself).
#include "GameClient/Display.h"
Display *TheDisplay = nullptr;

// TheScriptEngine / TheGameClient (GameLogic/ScriptEngine.h,
// GameClient/GameClient.h) - link-live only, via W3DView::update()'s
// unreachable-but-must-link body (":1539,1577,1691,1717" - all inside
// update(), never called by this harness).
#include "GameLogic/ScriptEngine.h"
ScriptEngine *TheScriptEngine = nullptr;

#include "GameClient/GameClient.h"
GameClient *TheGameClient = nullptr;

// ScriptEngine::isTimeFrozenDebug()/isTimeFrozenScript()/isTimeFast() -
// non-virtual, out-of-line real bodies live in the heavy, per-tree
// ScriptEngine.cpp (not linked here, rung-3b-ii-or-later scope). Every call
// site is inside W3DView::update() (link-live, runtime-dead - see above).
Bool ScriptEngine::isTimeFrozenDebug()
{
	DEBUG_CRASH(("RenderCameraTransformTest: ScriptEngine::isTimeFrozenDebug() stub called - should be unreachable (only called from W3DView::update(), never invoked by this harness); see link_stubs.cpp"));
	return false;
}

Bool ScriptEngine::isTimeFrozenScript()
{
	DEBUG_CRASH(("RenderCameraTransformTest: ScriptEngine::isTimeFrozenScript() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

Bool ScriptEngine::isTimeFast()
{
	DEBUG_CRASH(("RenderCameraTransformTest: ScriptEngine::isTimeFast() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

// GameLogic::isGamePaused() - same rationale (W3DView.cpp:1577, inside
// update()).
Bool GameLogic::isGamePaused()
{
	DEBUG_CRASH(("RenderCameraTransformTest: GameLogic::isGamePaused() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

// Drawable::clientOnly_getFirstRenderObjInfo() - only reached from
// W3DView::update()'s cameraLock-follow branch (W3DView.cpp:1468), which is
// only entered when a camera lock is set (setCameraLock(), never called by
// this harness - m_cameraLock stays INVALID_ID, its real default) AND
// update() itself is called (never, by this harness's own design).
Bool Drawable::clientOnly_getFirstRenderObjInfo(Coord3D * /*pos*/, Real * /*boundingSphereRadius*/, Matrix3D * /*transform*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: Drawable::clientOnly_getFirstRenderObjInfo() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

// Object::isUsingAirborneLocomotor() / Thing::getHeightAboveTerrainOrWater()
// - same cameraLock-follow branch (W3DView.cpp:1547, via
// Thing::isAboveTerrainOrWater(), inline in Thing.h, calling this).
Bool Object::isUsingAirborneLocomotor() const
{
	DEBUG_CRASH(("RenderCameraTransformTest: Object::isUsingAirborneLocomotor() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

Real Thing::getHeightAboveTerrainOrWater() const
{
	DEBUG_CRASH(("RenderCameraTransformTest: Thing::getHeightAboveTerrainOrWater() stub called - should be unreachable; see link_stubs.cpp"));
	return 0.0f;
}

// W3DShaderManager::filterSetup() - only reached from W3DView::reset()'s
// setViewFilter()/setViewFilterMode() calls (W3DView.cpp:1795-1827); reset()
// is link-live (vtable) but never called by this harness.
Bool W3DShaderManager::filterSetup(FilterTypes /*filter*/, FilterModes /*mode*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: W3DShaderManager::filterSetup() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

// ---- The rest of this batch: W3DView::draw()'s own unreachable-but-must-
// link closure (link-live via the vtable, runtime-dead - draw() is never
// called by this harness, matching update()'s own treatment above). ----

// W3DDisplay::m_3DScene/m_2DScene - STATIC member variables of W3DDisplay
// (W3DDisplay.h:152-153), read directly by W3DView::draw()/updateTerrain().
// W3DDisplay.cpp itself is deliberately NOT linked here (WIN32-gated per
// Task 1's unification, unchanged this milestone) - these two static
// members' storage is supplied here directly, matching the same pattern
// already established for W3DShaderManager::m_Textures[8] above.
#include "W3DDevice/GameClient/W3DDisplay.h"
RTS3DScene *W3DDisplay::m_3DScene = nullptr;
RTS2DScene *W3DDisplay::m_2DScene = nullptr;

// TheAI (GameLogic/AI.h) - AI::pathfinder() is inline in AI.h and reads
// "TheAI->getPathfinder()"; only reached from W3DView::draw()'s debug AI
// visualization code (":2xxx" range).
#include "GameLogic/AI.h"
AI *TheAI = nullptr;

// GameClient::flushTextBearingDrawables() - non-virtual, out-of-line real
// body in the heavy, per-tree GameClient.cpp (not linked here); only
// reached from W3DView::draw() (":2106").
void GameClient::flushTextBearingDrawables()
{
	DEBUG_CRASH(("RenderCameraTransformTest: GameClient::flushTextBearingDrawables() stub called - should be unreachable; see link_stubs.cpp"));
}

// TheWaterTransparency (GameClient/Water.h) - NOT a pointer, a value-typed
// OVERRIDE<WaterTransparencySetting> (Common/Override.h's template, already
// included above for TheControlBar). OVERRIDE<T>'s default constructor
// (Override.h:53, "OVERRIDE(const T *overridable = nullptr)") is exactly
// this - a real, default-constructed, empty override, matching every other
// deliberately-absent singleton in this file. Its own operator->() already
// null-guards internally (Override.h:107-112, "if (!m_overridable) return
// nullptr;"), so this is safe even if the link-live-but-runtime-dead call
// site in W3DView::draw() (":2001", feeding WW3D::Begin_Render's
// water-opacity argument) were ever reached.
#include "GameClient/Water.h"
OVERRIDE<WaterTransparencySetting> TheWaterTransparency;

// W3DShaderManager::filterPreRender()/filterPostRender() - only reached
// from W3DView::draw()'s screen-filter setup/teardown (":1876,1907,1944").
Bool W3DShaderManager::filterPreRender(FilterTypes /*filter*/, Bool & /*skipRender*/, CustomScenePassModes & /*scenePassMode*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: W3DShaderManager::filterPreRender() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

Bool W3DShaderManager::filterPostRender(FilterTypes /*filter*/, FilterModes /*mode*/, Coord2D & /*scrollDelta*/, Bool & /*doExtraRender*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: W3DShaderManager::filterPostRender() stub called - should be unreachable; see link_stubs.cpp"));
	return false;
}

// Drawable::setDrawableHidden() - non-virtual, out-of-line real body in the
// heavy, per-tree Drawable.cpp (not linked here); only reached from
// W3DView::draw()'s wireframe-mode drawable-hiding code (":1883,1914").
void Drawable::setDrawableHidden(Bool /*hidden*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: Drawable::setDrawableHidden() stub called - should be unreachable; see link_stubs.cpp"));
}

// SequentialScript's vtable (crc/xfer/loadPostProcess, its own Snapshot
// overrides) - pulled in transitively by this file's own
// "#include "GameLogic/ScriptEngine.h"" above (needed for the
// TheScriptEngine/isTimeFrozenDebug/isTimeFrozenScript/isTimeFast stubs) -
// the same shape of gap as TintEnvelope's, above. No SequentialScript
// instance is ever constructed by anything this harness calls, so these
// three bodies are never actually invoked.
void SequentialScript::crc(Xfer * /*xfer*/)
{
}

void SequentialScript::xfer(Xfer * /*xfer*/)
{
}

void SequentialScript::loadPostProcess()
{
}

// ============================================================================
// ---- NEW for this harness, batch 2: more of W3DView.o's unreachable-but-
// must-link closure, from draw()/shake()/Add_Camera_Shake()/
// worldToScreenTriReturn()/getPickRay()/pickDrawable()/
// iterateDrawablesInRegion()/setFadeParameters()/setViewFilterPos() - all
// link-live via the vtable or via other link-live-but-runtime-dead callers
// above, all genuinely unreachable at runtime for this harness (it never
// calls draw()/pickDrawable()/iterateDrawablesInRegion()/shake()/
// Add_Camera_Shake()/setFadeParameters()/setViewFilterPos(), and
// m_isCameraSlaved/m_useRealZoomCam stay at their real false defaults so
// buildCameraTransform()'s own slaved-camera branch is dead too). ----
// ============================================================================

// ScreenBWFilter::m_fadeFrames/m_fadeDirection/m_curFadeFrame + the same
// three for ScreenCrossFadeFilter (W3DShaderManager.h:198-210,236-254) -
// static storage only (their real Init/reset/set/preRender/postRender
// bodies live in the heavy, per-tree W3DShaderManager.cpp, WIN32-gated via
// d3dx8tex.h - Draft 30 finding 6, unchanged this milestone - and are never
// referenced here since no concrete filter object is ever constructed).
// Both classes' setFadeParameters() are inline statics W3DView::
// setFadeParameters() (link-live vtable entry, never called) invokes
// unconditionally.
Int ScreenBWFilter::m_fadeFrames = 0;
Int ScreenBWFilter::m_fadeDirection = 0;
Int ScreenBWFilter::m_curFadeFrame = 0;
Real ScreenBWFilter::m_curFadeValue = 0.0f;

Int ScreenCrossFadeFilter::m_fadeFrames = 0;
Int ScreenCrossFadeFilter::m_fadeDirection = 0;
Int ScreenCrossFadeFilter::m_curFadeFrame = 0;
Real ScreenCrossFadeFilter::m_curFadeValue = 0.0f;

// ScreenMotionBlurFilter::m_zoomToPos/m_zoomToValid - same rationale;
// W3DView::setViewFilterPos() (link-live override, never called) invokes
// the inline static setZoomToPos() unconditionally.
Coord3D ScreenMotionBlurFilter::m_zoomToPos;
Bool ScreenMotionBlurFilter::m_zoomToValid = false;

// Drawable::draw()/drawIconUI()/getDrawModules() - non-virtual, out-of-line
// real bodies in the heavy, per-tree Drawable.cpp (not linked here). draw()
// is only reached from the local drawDrawable() callback W3DView::update()
// passes to TheGameClient->iterateDrawablesInRegion() (dead, update() is
// never called); drawIconUI() from the local drawablePostDraw() callback
// (same story, inside draw()); getDrawModules() from buildCameraTransform()'s
// m_isCameraSlaved branch (always false, see file header comment).
void Drawable::draw()
{
	DEBUG_CRASH(("RenderCameraTransformTest: Drawable::draw() stub called - should be unreachable; see link_stubs.cpp"));
}

void Drawable::drawIconUI()
{
	DEBUG_CRASH(("RenderCameraTransformTest: Drawable::drawIconUI() stub called - should be unreachable; see link_stubs.cpp"));
}

DrawModule **Drawable::getDrawModules()
{
	DEBUG_CRASH(("RenderCameraTransformTest: Drawable::getDrawModules() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

// GetGameClientRandomValueReal() (GameClient/ClientRandomValue.h) - only
// reached from W3DView::shake() (link-live override, never called).
#include "GameClient/ClientRandomValue.h"
Real GetGameClientRandomValueReal(Real /*lo*/, Real /*hi*/, const char * /*file*/, int /*line*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: GetGameClientRandomValueReal() stub called - should be unreachable; see link_stubs.cpp"));
	return 0.0f;
}

// CameraShakeSystemClass::Add_Camera_Shake() - only reached from
// W3DView::Add_Camera_Shake() (link-live override, never called by this
// harness - see main.cpp's own comment on why CameraShakerSystem's
// contribution is provably zero for this harness's configuration).
void CameraShakeSystemClass::Add_Camera_Shake(const Vector3 & /*position*/, float /*radius*/, float /*duration*/, float /*power*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: CameraShakeSystemClass::Add_Camera_Shake() stub called - should be unreachable; see link_stubs.cpp"));
}

// TheWindowManager (GameClient/GameWindowManager.h) - task brief item 3:
// deliberately left null, matching TheTerrainRenderObject/TheRadar. Only
// reached from W3DView::pickDrawable() (link-live override, never called).
#include "GameClient/GameWindowManager.h"
GameWindowManager *TheWindowManager = nullptr;

// GameWindow::winGetParent()/winGetStatus() - non-virtual, out-of-line real
// bodies in the heavy, per-tree GameWindow.cpp (not linked here); only
// reached from pickDrawable()'s TheWindowManager-guarded branch (dead
// twice over: TheWindowManager is null above, AND pickDrawable() itself is
// never called).
#include "GameClient/GameWindow.h"
GameWindow *GameWindow::winGetParent()
{
	DEBUG_CRASH(("RenderCameraTransformTest: GameWindow::winGetParent() stub called - should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

UnsignedInt GameWindow::winGetStatus()
{
	DEBUG_CRASH(("RenderCameraTransformTest: GameWindow::winGetStatus() stub called - should be unreachable; see link_stubs.cpp"));
	return 0;
}

// TheInGameUI (GameClient/InGameUI.h) - only reached from
// iterateDrawablesInRegion() (link-live override, never called).
#include "GameClient/InGameUI.h"
InGameUI *TheInGameUI = nullptr;

// getPickTypesForContext() (GameClient/SelectionInfo.h) - real body
// (SelectionInfo.cpp:248) is a small, clean, already-portable-looking
// function, but only reached from iterateDrawablesInRegion() (link-live,
// never called) - a one-line stub is cheaper than pulling in
// SelectionInfo.cpp's own broader closure for a function never actually
// invoked here.
#include "GameClient/SelectionInfo.h"
UnsignedInt getPickTypesForContext(Bool /*forceAttackMode*/)
{
	DEBUG_CRASH(("RenderCameraTransformTest: getPickTypesForContext() stub called - should be unreachable; see link_stubs.cpp"));
	return 0;
}

// WorldHeightMap::getDrawRegion2D() - non-virtual, out-of-line real body in
// the heavy WorldHeightMap.cpp (not linked here - a non-goal, terrain mesh
// data). Only reached from updateCameraClipPlanes()'s/
// getAxisAlignedViewRegion()'s "if (TheTerrainRenderObject &&
// TheTerrainRenderObject->getMap())" branches - TheTerrainRenderObject is
// null (task brief item 3), so both branches are dead regardless of
// whether updateCameraClipPlanes() itself is called (it IS called for
// real, via setCameraTransform() <- updateCameraTransform(), but this
// specific branch never executes with TheTerrainRenderObject == nullptr).
#include "W3DDevice/GameClient/WorldHeightMap.h"
Region2D WorldHeightMap::getDrawRegion2D()
{
	DEBUG_CRASH(("RenderCameraTransformTest: WorldHeightMap::getDrawRegion2D() stub called - should be unreachable; see link_stubs.cpp"));
	Region2D r;
	r.zero();
	return r;
}
