// Harness-local link stubs (native port plan Phase 5(a) Milestone 8 Task 4,
// Draft 30) - the milestone's exit harness constructs the game's real
// RTS3DScene (Core/GameEngineDevice/.../W3DScene.cpp), the first GameClient
// scene object ever built and executed on POSIX. Its link closure - as the
// plan's finding 6 predicted - needs singleton-pointer stubs (same pattern
// as Tests/RenderGameAssets/link_stubs.cpp) PLUS, newly for this harness,
// a handful of out-of-line MEMBER function stubs for Drawable/Thing/Object
// accessors whose real bodies live in the heavy, per-tree Drawable.cpp/
// Object.cpp/Thing.cpp (not linked here). Every stub below documents why the
// real call site is either null-guarded or, for the member-function stubs,
// genuinely unreachable at runtime in this harness's configuration (its one
// render object is never given DrawableInfo user data via Set_User_Data, so
// every "if (drawInfo)"-gated branch in RTS3DScene::renderOneObject/
// Visibility_Check/flushOccludedObjectsIntoStencil that would call into
// these accessors is dead code for this harness's whole run - the LINKER
// still needs a symbol because these are ordinary, non-virtual member
// function calls compiled unconditionally into those functions' bodies).
// Follows the established anim_sound_link_stub.cpp / Tests/GameFileSystem/
// link_stubs.cpp / Tests/RenderGameAssets/link_stubs.cpp pattern: loud
// comments, no hidden behavior.

// ---- TheSubsystemList (SubsystemInterface.h/.cpp) --------------------------
// Carried over unchanged from Tests/RenderGameAssets/link_stubs.cpp:
// SubsystemInterface::SubsystemInterface()/~SubsystemInterface() (the base
// class RTS3DScene itself now also derives from, alongside every file-system
// object) null-guard every touch. This harness never runs GameEngine::
// initSubsystem's SubsystemInterfaceList::initSubsystem path, so
// TheSubsystemList stays nullptr for this harness's entire lifetime - safe
// by the same null guard every other tool already relies on.
#include "Common/SubsystemInterface.h"
SubsystemInterfaceList *TheSubsystemList = nullptr;

// ---- TheAudio (GameAudio.h, defined in GameAudio.cpp) ----------------------
// Carried over unchanged from Tests/RenderGameAssets/link_stubs.cpp: only
// reached when closing an archive literally named "Music.big" (never this
// harness's archive name).
#include "Common/GameAudio.h"
AudioManager *TheAudio = nullptr;

// ---- TheGameLogic (GameLogic/GameLogic.h, defined in the heavy per-tree
// GameLogic.cpp) --------------------------------------------------------
// RTS3DScene::Visibility_Check reads "TheGameLogic ? TheGameLogic->getFrame()
// : 0" and "TheGlobalData->m_enableBehindBuildingMarkers && TheGameLogic &&
// TheGameLogic->getShowBehindBuildingMarkers()" - both explicitly null-
// guarded. renderOneObject's ghost-object branch
// ("TheGameLogic->findObjectByID(...)") is only reached when a real
// DrawableInfo exists with no live Drawable, which never happens for this
// harness's plain quad render object (Get_User_Data() always returns
// nullptr for it). getFrame()/getShowBehindBuildingMarkers()/
// findObjectByID() are all fully inline in GameLogic.h (verified by
// reading), so this stub only needs to supply the pointer itself - no
// further out-of-line symbols are pulled in by it.
#include "GameLogic/GameLogic.h"
GameLogic *TheGameLogic = nullptr;

// ---- TheW3DShadowManager (W3DShadow.h, real definition in the now-Core-
// unified Shadow/W3DShadow.cpp) ----------------------------------------
// Customized_Render's "if (TheW3DShadowManager && terrainObject && ...)
// TheW3DShadowManager->queueShadows(TRUE)" is null-guarded.
// flushOccludedObjectsIntoStencil (compiled into Flush() as a live function,
// called only "if (DX8Wrapper::Has_Stencil())" - which the GL backend always
// returns false, Draft 30 finding 4/design decision - so this function's
// body is link-live but runtime-dead) calls
// TheW3DShadowManager->setStencilShadowMask/getStencilShadowMask
// unguarded, but both are fully inline in W3DShadow.h (queueShadows/
// setStencilShadowMask/getStencilShadowMask all defined in the class body),
// so - like TheGameLogic above - this stub only needs the pointer itself.
// Not linking the real Shadow/W3DShadow.cpp TU here at all: its
// PrepareShadows()/DoShadows() trampolines get their OWN harness-local
// stubs below (matching Tests/RenderGameAssets's established pattern for
// SubsystemInterface-style externs), and nothing else in this harness's
// live call graph needs the real W3DShadowManager class's out-of-line
// methods (addShadow/init/RenderShadows/etc., all only reachable through a
// real, constructed W3DShadowManager instance, which this harness never
// creates).
#include "W3DDevice/GameClient/W3DShadow.h"
W3DShadowManager *TheW3DShadowManager = nullptr;

// ---- TheTacticalView (GameClient/View.h, real definition in View.cpp) -----
// Only reached from flushOccludedObjectsIntoStencil's helper
// renderStenciledPlayerColor() (TheTacticalView->getOrigin/getWidth/
// getHeight) - link-live (see TheW3DShadowManager's comment above for why),
// runtime-dead (Has_Stencil() is always false on GL). getOrigin/getWidth/
// getHeight are all virtual and defined inline in the View class body, and
// this harness never constructs a real View (or any subclass), so no vtable
// for a concrete View-derived type is ever emitted - the stub only needs
// the pointer.
#include "GameClient/View.h"
View *TheTacticalView = nullptr;

// ---- TheParticleSystemManager (GameClient/ParticleSys.h, real definition
// in Core/GameEngine/Source/GameClient/System/ParticleSys.cpp) -------------
// Customized_Render's "if (terrainObject != nullptr && TheParticleSystemManager
// != nullptr && ...) TheParticleSystemManager->queueParticleRender()" is
// null-guarded, AND queueParticleRender is pure virtual on the base class -
// a virtual call needs no link-time symbol from the caller's TU at all
// (only a real, constructed concrete instance's vtable would need one, and
// this harness never constructs any ParticleSystemManager). Stub is the
// pointer only.
#include "GameClient/ParticleSys.h"
ParticleSystemManager *TheParticleSystemManager = nullptr;

// ---- TheTerrainRenderObject (W3DDevice/GameClient/BaseHeightMap.h, real
// definition in BaseHeightMap.cpp) ------------------------------------------
// Not reached via the DoTrees() trampoline (that gets its OWN harness-local
// stub below, matching design decisions) but via a SEPARATE, direct
// reference: W3DShroudMaterialPassClass::Install_Materials (W3DShroud.cpp,
// compiled into this harness for real - see the CMakeLists.txt comment for
// why) reads "if (TheTerrainRenderObject->getShroud())" - link-live because
// this harness's RTS3DScene ctor constructs a real W3DShroudMaterialPassClass
// whenever TheGlobalData->m_shroudOn is true (the real GlobalData default,
// ENABLE_CONFIGURABLE_SHROUD's default-on path), so its vtable (hence
// Install_Materials's body) must resolve - but runtime-dead: this harness's
// quad's DrawableInfo is always null, so renderOneObject's plain
// "robj->Render(rinfo)" path is taken and the shroud material pass's
// Install_Materials/UnInstall_Materials are never actually invoked.
#include "W3DDevice/GameClient/BaseHeightMap.h"
BaseHeightMapRenderObjClass *TheTerrainRenderObject = nullptr;

// ---- TheControlBar / ThePlayerList (real definitions in the heavy,
// per-tree ControlBar.cpp / PlayerList.cpp) ---------------------------------
// rts::getObservedOrLocalPlayerIndex_Safe (Core/GameEngine/Source/Common/
// GameUtility.cpp, compiled into this harness for real - a small, clean TU
// per the established Drafts 24/28 policy) explicitly null-guards both:
// "if (TheControlBar != nullptr) ...; if (ThePlayerList != nullptr) ...",
// falling back to PlayerIndex 0 exactly as Draft 30 finding 3 predicted
// ("rts::getObservedOrLocalPlayerIndex_Safe... returns 0 with null
// TheControlBar/ThePlayerList by design").
// Common/Override.h must precede ControlBar.h here: ControlBar.h uses the
// OVERRIDE<T> template (e.g. "OVERRIDE<ThingTemplate> getThingTemplate()")
// without including its own defining header - in the real per-tree build,
// some earlier header in that TU's chain always brings it in first; this
// harness's link_stubs.cpp has no such earlier chain, so it must be
// explicit here.
#include "Common/Override.h"
#include "GameClient/ControlBar.h"
ControlBar *TheControlBar = nullptr;
#include "Common/PlayerList.h"
PlayerList *ThePlayerList = nullptr;

// ---- TheVersion (Common/version.h, real definition per-tree) --------------
// Found by the linker, not predicted by the plan: GlobalData derives from
// SubsystemInterface and provides real overrides for init/update/reset/
// draw - constructing a real GlobalData (this harness's Check 1) therefore
// requires its COMPLETE vtable to resolve, which pulls in
// GlobalData::init() (never called by this harness, but link-live via the
// vtable) -> generateExeCRC() -> "if (TheVersion) { version =
// TheVersion->getVersionNumber(); ... }" - null-guarded, so this stub is
// sufficient; Version::getVersionNumber() is never actually invoked.
#include "Common/version.h"
Version *TheVersion = nullptr;

// Version::getVersionNumber() itself (Common/version.h:45, non-virtual,
// real body in the per-tree version.cpp) - genuinely link-live for the
// SAME vtable-completeness reason as above ("if (TheVersion) { version =
// TheVersion->getVersionNumber(); }" still compiles the call unconditionally
// even though TheVersion is always null here, so it's never actually
// executed), but real version.cpp is a 453-line TU pulling in
// GameClient/GameText.h (TheGameText, a heavy localization subsystem this
// milestone's non-goals list explicitly excludes) for functionality
// (getAsciiVersion/etc.) this stub doesn't need at all. A one-line stub is
// far cheaper than that closure for a function that is provably never
// called.
UnsignedInt Version::getVersionNumber() const
{
	return 0;
}

// ---- The four Flush() engine externs: harness-local stub trampolines,
// verbatim-equivalence commented (Draft 30 design decisions / open question
// 2) ------------------------------------------------------------------
// RTS3DScene::Flush unconditionally calls all four. Rather than linking
// their real homes (Shadow/W3DShadow.cpp for the first two,
// Core/GameEngineDevice's BaseHeightMap.cpp for DoTrees,
// Core/GameEngine's ParticleSys.cpp for DoParticles - all rung-3b-or-later,
// heavy, terrain/shadow/particle-manager-coupled TUs), each real body is
// reproduced here as a 1-3-line null-guarded trampoline, quoted verbatim so
// the behavioral identity is directly reviewable. Every singleton each one
// guards (TheW3DShadowManager, TheParticleSystemManager - both already
// nullptr stubs above) makes each trampoline's body a true no-op in this
// harness, exactly as it would be in the real engine before those managers
// exist.
#include "WW3D2/rinfo.h"

// Real body (Shadow/W3DShadow.cpp): PrepareShadows() delegates to
// TheW3DProjectedShadowManager->preRender(), itself null-guarded inside.
// TheW3DProjectedShadowManager is never constructed by this harness (it
// would require the heavy W3DProjectedShadow.cpp, a non-goal), so this is a
// true no-op here regardless.
void PrepareShadows()
{
}

// Real body (Shadow/W3DShadow.cpp): DoShadows(rinfo, stencilPass) delegates
// to TheW3DShadowManager->RenderShadows() (non-stencil pass) or the
// projected/volumetric managers (stencil pass), all guarded. With
// TheW3DShadowManager null (this harness's stub above), a true no-op.
void DoShadows(RenderInfoClass & /*rinfo*/, Bool /*stencilPass*/)
{
}

// Real body (Core/GameEngineDevice's BaseHeightMap.cpp:120-125): DoTrees
// guards "if (TheTerrainRenderObject) { TheTerrainRenderObject->
// renderTrees(&rinfo.Camera); }". TheTerrainRenderObject is this harness's
// null stub above, so a true no-op.
void DoTrees(RenderInfoClass & /*rinfo*/)
{
}

// Real body (Core/GameEngine's ParticleSys.cpp:99-103): DoParticles guards
// "if (TheParticleSystemManager) { TheParticleSystemManager->
// queueParticleRender(); }". TheParticleSystemManager is this harness's
// null stub above, so a true no-op.
void DoParticles(RenderInfoClass & /*rinfo*/)
{
}

// ---- W3DShaderManager::setShader/resetShader + the m_Textures[] storage
// its (inline) setTexture() writes into (real home: W3DShaderManager.cpp,
// which #includes d3dx8tex.h - not POSIX-compilable today, Draft 30 finding
// 6) ------------------------------------------------------------------
// W3DShroudMaterialPassClass::Install_Materials/UnInstall_Materials and
// W3DMaskMaterialPassClass::Install_Materials/UnInstall_Materials
// (W3DShroud.cpp, compiled into this harness for real) call
// W3DShaderManager::setTexture/setShader/resetShader. setTexture() is
// itself already fully inline in W3DShaderManager.h (writes directly into
// the static m_Textures[] array), so it needs only that array's storage
// defined somewhere - provided here instead of pulling in the whole
// (heavy, d3dx8tex.h-bound) W3DShaderManager.cpp. setShader/resetShader are
// declared but not inline, so they get real stub bodies. All of this is
// link-live (the shroud/mask material passes are really constructed by
// RTS3DScene's ctor, so their vtables need real Install_Materials/
// UnInstall_Materials bodies) but runtime-dead for the same reason as
// TheTerrainRenderObject above: this harness's quad never triggers a
// shroud/mask material pass to actually install.
#include "W3DDevice/GameClient/W3DShaderManager.h"
// Array size (8) matches the extern declaration in W3DShaderManager.h
// ("static TextureClass *m_Textures[8];") exactly - this IS that array's
// one defining declaration for the whole program, substituting for the
// definition that would otherwise come from W3DShaderManager.cpp.
TextureClass *W3DShaderManager::m_Textures[8];

Int W3DShaderManager::setShader(ShaderTypes /*shader*/, Int pass)
{
	// Real body enables a specific numbered shader pass and returns the
	// number of passes; never actually invoked here (see file header
	// comment), so an arbitrary but harmless return value is fine.
	return pass;
}

void W3DShaderManager::resetShader(ShaderTypes /*shader*/)
{
	// Real body restores W3D's normal shader state; never actually invoked
	// here (see file header comment), so an empty body is a true no-op.
}

// ---- Drawable/Thing/Object out-of-line accessor stubs (Draft 30 open
// question 1's "harness-local member-function stubs") ----------------------
// Real bodies live in the heavy, per-tree Drawable.cpp/Object.cpp (neither
// linked into this harness). Every call site below is reached ONLY through
// a real, non-null DrawableInfo* (RTS3DScene::renderOneObject/
// Visibility_Check/flushOccludedObjectsIntoStencil's "if (drawInfo)"-gated
// branches) - this harness's one render object is never given
// DrawableInfo user data (no Set_User_Data call anywhere in main.cpp), so
// Get_User_Data() always returns nullptr for it and every one of these
// branches is dead code for this harness's entire run. The functions
// containing these calls (renderOneObject, Visibility_Check,
// flushOccludedObjectsIntoStencil) are themselves link-live (called from
// Flush()/Customized_Render(), which doRender()'s real chain always
// reaches), so the calls compiled inside them still need real symbols -
// legal only while the real Drawable.cpp/Object.cpp/Thing.cpp are absent
// from this link, per Draft 30's open question 1 and the Milestone 6/7
// precedent for this exact pattern.
#include "Common/Debug.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Object.h"

const Vector3 *Drawable::getTintColor() const
{
	// Real body returns a per-drawable FX tint color (or nullptr); this
	// harness's quad drawable is never constructed with tint state, and this
	// accessor is never reached (see comment above), so returning nullptr -
	// the same "no tint" value the real accessor returns for an untinted
	// drawable - is both safe and behaviorally faithful if ever hit.
	return nullptr;
}

const Vector3 *Drawable::getSelectionColor() const
{
	// Same rationale as getTintColor() above.
	return nullptr;
}

Bool Thing::isKindOf(KindOfType /*t*/) const
{
	// Real body patches through to the Thing's ThingTemplate's KindOf mask.
	// Never reached in this harness (see file header comment); "false" is
	// the safe/conservative answer for every KINDOF_* query call site above
	// (KINDOF_STRUCTURE/KINDOF_SCORE*/KINDOF_INFANTRY all gate additional
	// special-case handling this harness never needs to exercise).
	return false;
}

Player *Object::getControllingPlayer() const
{
	// Real body returns this object's controlling Player*. Only reached from
	// flushOccludedObjectsIntoStencil's stencil-occlusion bookkeeping loop
	// (dead code here, see file header comment) - returning nullptr would
	// crash that dead code IF it were ever reached, which correctly signals
	// a real problem rather than silently faking a Player, should some
	// future change make this path live without updating this stub.
	DEBUG_CRASH(("RenderRTS3DSceneTest: Object::getControllingPlayer() stub called - this should be unreachable; see link_stubs.cpp"));
	return nullptr;
}

ObjectShroudStatus Object::getShroudedStatus(Int /*playerIndex*/) const
{
	// Real body computes this object's shroud/fog state for the given
	// player. Only reached from renderOneObject's "obj = draw->getObject();
	// if (obj) { ss = obj->getShroudedStatus(...); }" branch (dead code
	// here, see file header comment) - OBJECTSHROUD_CLEAR ("not shrouded")
	// is the safe/conservative answer, matching what the harness's own quad
	// would need if this were ever live (it is never shrouded).
	return OBJECTSHROUD_CLEAR;
}

// ---- Drawable::friend_lockDirtyStuffForIteration/
// friend_unlockDirtyStuffForIteration (Drawable.h:748-750, real bodies in
// Drawable.cpp, guarded by "#ifdef DIRTY_CONDITION_FLAGS" - which IS
// unconditionally defined, Drawable.h:61) --------------------------------
// UNLIKE every other stub in this file, these two are genuinely,
// unconditionally EXECUTED every single frame: Visibility_Check/
// Customized_Render/renderSpecificDrawables each construct a
// StDrawableDirtyStuffLocker RAII guard (also "#ifdef DIRTY_CONDITION_FLAGS"-
// gated, same unconditional definition) whose ctor/dtor call these two
// static methods directly - not behind any DrawableInfo/drawInfo null
// check. The real bodies (Drawable.cpp:5417-5442) guard everything
// meaningful behind "if (TheGameClient)" (a singleton this milestone's
// non-goals list explicitly excludes) and a static s_modelLockCount
// reference-counter local to Drawable.cpp; with TheGameClient forever null
// in this harness, the real body's own effect would already be a no-op
// bookkeeping increment/decrement of a counter nothing else in this
// harness's closure ever reads. True no-op stubs are exactly
// behaviorally equivalent here, not a shortcut.
void Drawable::friend_lockDirtyStuffForIteration()
{
}

void Drawable::friend_unlockDirtyStuffForIteration()
{
}
