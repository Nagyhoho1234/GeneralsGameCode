// Phase 5(a) Milestone 13 (native port plan, Draft 37, "rung 3b-ii-b") -
// this harness's own stub-subclass surface. Starts from Tests/
// PosixGameEngineHarness/harness_stub_classes.h's own eleven classes
// (DisplayStringStub/DisplayStringManagerStub/TerrainVisualStub/
// AudioManagerStub/InGameUIStub/DisplayStub/FontLibraryStub/MouseStub/
// GameWindowManagerStub/GameClientStub/PosixGameEngine - see that file's own
// header comment for the full, real, per-class rationale, unchanged here),
// with exactly two Draft 37-required changes:
//
//   Cost B (the stub upgrade): GameClientStub::friend_createDrawable() now
//   returns a REAL Drawable (newInstance(Drawable)(tmplate, statusBits)) -
//   copied verbatim from the real W3DGameClient::friend_createDrawable()'s
//   own one-line body (GeneralsMD/Code/GameEngineDevice/.../
//   W3DDevice/GameClient/W3DGameClient.cpp) - instead of Milestone 12's
//   nullptr (which would crash GameLogic::bindObjectAndDrawable()'s
//   unconditional "draw->friend_bindToObject(obj)" dereference, confirmed
//   by reading that real call site).
//
//   ModuleFactoryStub (new): a harness-local ModuleFactory subclass whose
//   own init() does exactly ModuleFactory::init() (the real, non-pure base
//   body, already part of the linked closure) followed by ONE
//   addModule(W3DModelDraw) call - the SAME real addModule macro the game's
//   own W3DModuleFactory::init() uses (GeneralsMD/Code/GameEngineDevice/.../
//   W3DDevice/Common/Thing/W3DModuleFactory.cpp:64), confining the link cost
//   to W3DModelDraw.cpp alone rather than dragging in W3DModuleFactory.cpp's
//   other ~20 addModule() calls (each pulling in its own device-tier .cpp,
//   none of which this milestone's non-goals need).
//
//   PosixGameLogicStub/HarnessTerrainLogic (new, a genuine finding this
//   milestone's own reading turned up, not explicitly named by Draft 37):
//   the real Object::reactToTransformChange() (called unconditionally from
//   Object::setPosition(), GameLogic.cpp/Object.cpp real bodies) does
//   "TheTerrainLogic->getExtent(&mapExtent)" with NO null guard - and the
//   base TerrainLogic::getExtent()/getExtentIncludingBorder() (Include/
//   GameLogic/TerrainLogic.h) are each a bare "{ DEBUG_CRASH((...)); }" with
//   NO body otherwise, leaving the output Region3D UNINITIALIZED (a real,
//   latent UB read in the engine's own base class - the SAME hazard
//   Milestone 11's retry already found and fixed for its own
//   view->update() call, Tests/RenderViewUpdateDraw/harness_stub_classes.h's
//   own HarnessTerrainLogic). PosixGameEngine::createGameLogic() returning
//   the plain base GameLogic class (Milestone 12's own established choice)
//   means GameLogic::init()'s own "TheTerrainLogic = createTerrainLogic();"
//   resolves to the base GameLogic::createTerrainLogic()'s "NEW
//   TerrainLogic" (GameLogic.cpp:4744-4747) - the hazardous base class -
//   UNLESS createGameLogic() itself returns a subclass overriding
//   createTerrainLogic(). PosixGameLogicStub does exactly that, reusing
//   Milestone 11's own HarnessTerrainLogic verbatim.
//
// TheSuperHackers @test Milestone 13: real, minimal concrete stub subclasses
// driving the real, unmodified GameEngine::init()/update()/execute() PLUS a
// real, named Object+Drawable through TheThingFactory->newObject().

#pragma once

#include "GameClient/GameClient.h"
#include "GameClient/Keyboard.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Display.h"
#include "GameClient/GameFont.h"
#include "GameClient/Mouse.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/View.h"
#include "GameClient/TerrainVisual.h"
#include "GameClient/DisplayStringManager.h"
#include "GameClient/DisplayString.h"
#include "GameClient/VideoPlayer.h"
#include "GameClient/Drawable.h"
#include "Common/GameAudio.h"
#include "Common/GameEngine.h"
#include "Common/LocalFileSystem.h"
#include "Common/ArchiveFileSystem.h"
#include "Common/Radar.h"
#include "GameClient/ParticleSys.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/ModuleFactory.h"
#include "GameLogic/TerrainLogic.h"
#include "Common/FunctionLexicon.h"
#include "GameLogic/GameLogic.h"
#include "StdDevice/Common/StdLocalFileSystem.h"
#include "StdDevice/Common/StdBIGFileSystem.h"
#include "W3DDevice/GameClient/Module/W3DModelDraw.h"

// ---- HarnessTerrainLogic: adapted from Tests/RenderViewUpdateDraw/
// harness_stub_classes.h's own class (same real UB-hazard rationale - the
// base TerrainLogic::getExtent()/getExtentIncludingBorder() are
// uninitialized-output DEBUG_CRASH-only stubs), but with a DELIBERATELY
// SMALLER extent than Milestone 11's own -100000..100000 choice - a genuine,
// real finding THIS milestone's own step-0 spike turned up (not
// foreseeable from Milestone 11's own scope, which never called
// PartitionManager::init()): PartitionManager::init() ALSO consumes
// TheTerrainLogic->getExtent() directly (PartitionManager.cpp:2675), sizing
// a real m_cellCountX * m_cellCountY PartitionCell array from it
// (m_cellSize clamped to a 1.0 minimum when TheGlobalData->
// m_partitionCellSize's own real default is 0.0f) - Milestone 11's
// -100000..100000 choice, fed through this same real formula, requests a
// 200000 x 200000 (= 4*10^10) PartitionCell array, a real, reproducible
// SIGSEGV (gdb-confirmed: PartitionManager.cpp:2687's own "m_cells =
// MSGNEW(...) PartitionCell[m_totalCellCount];"), not a prediction. A much
// smaller, still-finite box - large enough to hold this milestone's own
// single named unit (world position (500,500,0), see main.cpp) with ample
// margin for the real camera math's own position/pivot clamping - keeps
// PartitionManager's own real cell grid to a modest, fast-to-allocate
// 1000 x 1000 (one million) cells instead. ----
class HarnessTerrainLogic : public TerrainLogic
{
public:
	virtual void getExtent( Region3D *extent ) const override
	{
		extent->lo.x = extent->lo.y = 0.0f;
		extent->lo.z = -1000.0f;
		extent->hi.x = extent->hi.y = 1000.0f;
		extent->hi.z = 1000.0f;
	}
	virtual void getExtentIncludingBorder( Region3D *extent ) const override
	{
		getExtent(extent);
	}
};

// ---- PosixGameLogicStub: NEW for Milestone 13 - the plain base GameLogic
// class (Milestone 12's own established choice, unchanged - zero pure
// virtuals, real createGhostObjectManager() already returns the portable
// headless-mode Dummy variant), with exactly ONE override:
// createTerrainLogic() now returns HarnessTerrainLogic instead of the
// hazardous base TerrainLogic (see this file's own header comment). ----
class PosixGameLogicStub : public GameLogic
{
public:
	virtual TerrainLogic *createTerrainLogic() override { return NEW HarnessTerrainLogic; }
};

// ---- DisplayStringStub/DisplayStringManagerStub/TerrainVisualStub/
// AudioManagerStub/InGameUIStub/DisplayStub/FontLibraryStub/MouseStub/
// GameWindowManagerStub: verbatim copies of Tests/PosixGameEngineHarness/
// harness_stub_classes.h's own classes - see that file for the full, real,
// per-method rationale of each (unchanged by this milestone). ----

class DisplayStringStub : public DisplayString
{
public:
	DisplayStringStub() {}

	static DisplayStringStub *create() { return new(DisplayString::DisplayString_GLUE_NOT_IMPLEMENTED) DisplayStringStub; }

	virtual void setWordWrap( Int /*wordWrap*/ ) override {}
	virtual void setWordWrapCentered( Bool /*isCentered*/ ) override {}
	virtual void draw( Int /*x*/, Int /*y*/, Color /*color*/, Color /*dropColor*/ ) override {}
	virtual void draw( Int /*x*/, Int /*y*/, Color /*color*/, Color /*dropColor*/, Int /*xDrop*/, Int /*yDrop*/ ) override {}
	virtual void getSize( Int *width, Int *height ) override { *width = 0; *height = 0; }
	virtual Int getWidth( Int /*charPos*/ = -1 ) override { return 0; }
	virtual void setUseHotkey( Bool /*useHotkey*/, Color /*hotKeyColor*/ ) override {}
};

class DisplayStringManagerStub : public DisplayStringManager
{
public:
	DisplayStringManagerStub() {}
	virtual ~DisplayStringManagerStub() {}

	virtual DisplayString *newDisplayString() override
	{
		DisplayString *string = DisplayStringStub::create();
		link( string );
		return string;
	}

	virtual void freeDisplayString( DisplayString *string ) override
	{
		if (string == nullptr)
			return;
		unLink( string );
		deleteInstance( string );
	}

	virtual DisplayString *getGroupNumeralString( Int /*numeral*/ ) override { return newDisplayString(); }
	virtual DisplayString *getFormationLetterString() override { return newDisplayString(); }
};

class TerrainVisualStub : public TerrainVisual
{
public:
	TerrainVisualStub() {}
	virtual ~TerrainVisualStub() {}

	virtual void getTerrainColorAt( Real /*x*/, Real /*y*/, RGBColor * /*pColor*/ ) override {}
	virtual TerrainType *getTerrainTile( Real /*x*/, Real /*y*/ ) override { return nullptr; }

	virtual void enableWaterGrid( Bool /*enable*/ ) override {}
	virtual void setWaterGridHeightClamps( const WaterHandle * /*waterTable*/, Real /*minZ*/, Real /*maxZ*/ ) override {}
	virtual void setWaterAttenuationFactors( const WaterHandle * /*waterTable*/, Real /*a*/, Real /*b*/, Real /*c*/, Real /*range*/ ) override {}
	virtual void setWaterTransform( const WaterHandle * /*waterTable*/, Real /*angle*/, Real /*x*/, Real /*y*/, Real /*z*/ ) override {}
	virtual void setWaterTransform( const Matrix3D * /*transform*/ ) override {}
	virtual void getWaterTransform( const WaterHandle * /*waterTable*/, Matrix3D * /*transform*/ ) override {}
	virtual void setWaterGridResolution( const WaterHandle * /*waterTable*/, Real /*gridCellsX*/, Real /*gridCellsY*/, Real /*cellSize*/ ) override {}
	virtual void getWaterGridResolution( const WaterHandle * /*waterTable*/, Real *gridCellsX, Real *gridCellsY, Real *cellSize ) override { *gridCellsX = 0.0f; *gridCellsY = 0.0f; *cellSize = 0.0f; }
	virtual void changeWaterHeight( Real /*x*/, Real /*y*/, Real /*delta*/ ) override {}
	virtual void addWaterVelocity( Real /*worldX*/, Real /*worldY*/, Real /*velocity*/, Real /*preferredHeight*/ ) override {}
	virtual Bool getWaterGridHeight( Real /*worldX*/, Real /*worldY*/, Real *height) override { *height = 0.0f; return FALSE; }

	virtual void setTerrainTracksDetail() override {}
	virtual void setShoreLineDetail() override {}

	virtual void addFactionBib(Object * /*factionBuilding*/, Bool /*highlight*/, Real /*extra*/ = 0) override {}
	virtual void removeFactionBib(Object * /*factionBuilding*/) override {}

	virtual void addFactionBibDrawable(Drawable * /*factionBuilding*/, Bool /*highlight*/, Real /*extra*/ = 0) override {}
	virtual void removeFactionBibDrawable(Drawable * /*factionBuilding*/) override {}

	virtual void removeAllBibs() override {}
	virtual void removeBibHighlighting() override {}

	virtual void removeTreesAndPropsForConstruction(const Coord3D* /*pos*/, const GeometryInfo& /*geom*/, Real /*angle*/) override {}

	virtual void addProp(const ThingTemplate * /*tt*/, const Coord3D * /*pos*/, Real /*angle*/) override {}

	virtual void setRawMapHeight(const ICoord2D * /*gridPos*/, Int /*height*/) override {}
	virtual Int getRawMapHeight(const ICoord2D * /*gridPos*/) override { return 0; }

	virtual void replaceSkyboxTextures(const AsciiString * /*oldTexName*/[NumSkyboxTextures], const AsciiString * /*newTexName*/[NumSkyboxTextures]) override {}
};

class AudioManagerStub : public AudioManager
{
public:
	AudioManagerStub() {}
	virtual ~AudioManagerStub() {}

	virtual void stopAudio( AudioAffect /*which*/ ) override {}
	virtual void pauseAudio( AudioAffect /*which*/ ) override {}
	virtual void resumeAudio( AudioAffect /*which*/ ) override {}
	virtual void pauseAmbient( Bool /*shouldPause*/ ) override {}

	virtual void killAudioEventImmediately( AudioHandle /*audioEvent*/ ) override {}

	virtual AsciiString nextMusicTrack() override { return AsciiString::TheEmptyString; }
	virtual AsciiString prevMusicTrack() override { return AsciiString::TheEmptyString; }
	virtual Bool isMusicPlaying() const override { return FALSE; }
	virtual Bool hasMusicTrackCompleted( const AsciiString& /*trackName*/, Int /*numberOfTimes*/ ) const override { return FALSE; }

	virtual void openDevice() override {}
	virtual void closeDevice() override {}
	virtual void *getDevice() override { return nullptr; }

	virtual void notifyOfAudioCompletion( UnsignedInt /*audioCompleted*/, UnsignedInt /*flags*/ ) override {}

	virtual UnsignedInt getProviderCount() const override { return 0; }
	virtual AsciiString getProviderName( UnsignedInt /*providerNum*/ ) const override { return AsciiString::TheEmptyString; }
	virtual UnsignedInt getProviderIndex( AsciiString /*providerName*/ ) const override { return 0; }
	virtual void selectProvider( UnsignedInt /*providerNdx*/ ) override {}
	virtual void unselectProvider() override {}
	virtual UnsignedInt getSelectedProvider() const override { return 0; }
	virtual void setSpeakerType( UnsignedInt /*speakerType*/ ) override {}
	virtual UnsignedInt getSpeakerType() override { return 0; }

	virtual UnsignedInt getNum2DSamples() const override { return 0; }
	virtual UnsignedInt getNum3DSamples() const override { return 0; }
	virtual UnsignedInt getNumStreams() const override { return 0; }
	virtual UnsignedInt getNumAvailable2DSamples() const override { return 0; }
	virtual UnsignedInt getNumAvailable3DSamples() const override { return 0; }

	virtual Bool doesViolateLimit( AudioEventRTS * /*event*/ ) const override { return FALSE; }
	virtual Bool isPlayingLowerPriority( AudioEventRTS * /*event*/ ) const override { return FALSE; }
	virtual Bool isPlayingAlready( AudioEventRTS * /*event*/ ) const override { return FALSE; }
	virtual Bool isObjectPlayingVoice( UnsignedInt /*objID*/ ) const override { return FALSE; }

	virtual void adjustVolumeOfPlayingAudio(AsciiString /*eventName*/, Real /*newVolume*/) override {}
	virtual void removePlayingAudio( AsciiString /*eventName*/ ) override {}
	virtual void removeAllDisabledAudio() override {}

	virtual Bool has3DSensitiveStreamsPlaying() const override { return FALSE; }

	virtual void *getHandleForBink() override { return nullptr; }
	virtual void releaseHandleForBink() override {}

	virtual void friend_forcePlayAudioEventRTS(const AudioEventRTS* /*eventToPlay*/) override {}

	virtual void setPreferredProvider(AsciiString /*providerNdx*/) override {}
	virtual void setPreferredSpeaker(AsciiString /*speakerType*/) override {}

	virtual Real getFileLengthMS( AsciiString /*strToLoad*/ ) const override { return 0.0f; }

	virtual void closeAnySamplesUsingFile( const void * /*fileToClose*/ ) override {}

protected:
	virtual void setDeviceListenerPosition() override {}
};

class InGameUIStub : public InGameUI
{
public:
	InGameUIStub() {}
	virtual ~InGameUIStub() {}

	virtual void draw() override {}
	virtual View *createView(bool /*dummy*/ = false) override { return NEW ViewDummy; }
};

class DisplayStub : public Display
{
public:
	DisplayStub() {}
	virtual ~DisplayStub() {}

	virtual void doSmartAssetPurgeAndPreload(const char * /*usageFileName*/) override {}
#if defined(RTS_DEBUG)
	virtual void dumpAssetUsage(const char * /*mapname*/) override {}
#endif
	virtual VideoBuffer *createVideoBuffer() override { return nullptr; }
	virtual void setClipRegion( IRegion2D * /*region*/ ) override {}
	virtual Bool isClippingEnabled() override { return FALSE; }
	virtual void enableClipping( Bool /*onoff*/ ) override {}
	virtual void setTimeOfDay( TimeOfDay /*tod*/ ) override {}
	virtual void createLightPulse( const Coord3D * /*pos*/, const RGBColor * /*color*/, Real /*innerRadius*/, Real /*attenuationWidth*/,
		UnsignedInt /*increaseFrameTime*/, UnsignedInt /*decayFrameTime*/ ) override {}
	virtual void drawLine( Int /*startX*/, Int /*startY*/, Int /*endX*/, Int /*endY*/,
		Real /*lineWidth*/, UnsignedInt /*lineColor*/ ) override {}
	virtual void drawLine( Int /*startX*/, Int /*startY*/, Int /*endX*/, Int /*endY*/,
		Real /*lineWidth*/, UnsignedInt /*lineColor1*/, UnsignedInt /*lineColor2*/ ) override {}
	virtual void drawOpenRect( Int /*startX*/, Int /*startY*/, Int /*width*/, Int /*height*/,
		Real /*lineWidth*/, UnsignedInt /*lineColor*/ ) override {}
	virtual void drawFillRect( Int /*startX*/, Int /*startY*/, Int /*width*/, Int /*height*/,
		UnsignedInt /*color*/ ) override {}
	virtual void drawRectClock(Int /*startX*/, Int /*startY*/, Int /*width*/, Int /*height*/, Int /*percent*/, UnsignedInt /*color*/) override {}
	virtual void drawRemainingRectClock(Int /*startX*/, Int /*startY*/, Int /*width*/, Int /*height*/, Int /*percent*/, UnsignedInt /*color*/) override {}
	virtual void drawImage( const Image * /*image*/, Int /*startX*/, Int /*startY*/,
		Int /*endX*/, Int /*endY*/, Color /*color*/ = 0xFFFFFFFF, DrawImageMode /*mode*/ = DRAW_IMAGE_ALPHA) override {}
	virtual void drawScaledVideoBuffer( VideoBuffer * /*buffer*/, VideoStreamInterface * /*stream*/ ) override {}
	virtual void drawVideoBuffer( VideoBuffer * /*buffer*/, Int /*startX*/, Int /*startY*/,
		Int /*endX*/, Int /*endY*/ ) override {}
	virtual void setShroudLevel(Int /*x*/, Int /*y*/, CellShroudStatus /*setting*/ ) override {}
	virtual void clearShroud() override {}
	virtual void setBorderShroudLevel(UnsignedByte /*level*/) override {}
#if defined(RTS_DEBUG)
	virtual void dumpModelAssets(const char * /*path*/) override {}
#endif
	virtual void preloadModelAssets( AsciiString /*model*/ ) override {}
	virtual void preloadTextureAssets( AsciiString /*texture*/ ) override {}
	virtual void takeScreenShot(ScreenshotFormat /*format*/, Int /*jpegQuality*/ = DEFAULT_JPEG_QUALITY) override {}
	virtual void toggleMovieCapture() override {}
	virtual void toggleLetterBox() override {}
	virtual void enableLetterBox(Bool /*enable*/) override {}
	virtual Real getAverageFPS() override { return 0.0f; }
	virtual Real getCurrentFPS() override { return 0.0f; }
	virtual Int getLastFrameDrawCalls() override { return 0; }
};

class FontLibraryStub : public FontLibrary
{
public:
	FontLibraryStub() {}
	virtual ~FontLibraryStub() {}

protected:
	virtual Bool loadFontData( GameFont * /*font*/ ) override { return FALSE; }
};

class MouseStub : public Mouse
{
public:
	MouseStub() {}
	virtual ~MouseStub() {}

	virtual void setCursor( MouseCursor /*cursor*/ ) override {}
	virtual void initCursorResources() override {}
	virtual void capture() override {}
	virtual void releaseCapture() override {}
	virtual UnsignedByte getMouseEvent( MouseIO * /*result*/, Bool /*flush*/ ) override { return 0; }
};

class GameWindowManagerStub : public GameWindowManager
{
public:
	GameWindowManagerStub() {}
	virtual ~GameWindowManagerStub() {}

	virtual GameWindow *allocateNewWindow() override { return nullptr; }
	virtual GameWinDrawFunc getPushButtonImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getPushButtonDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getCheckBoxImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getCheckBoxDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getRadioButtonImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getRadioButtonDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getTabControlImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getTabControlDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getListBoxImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getListBoxDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getComboBoxImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getComboBoxDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getHorizontalSliderImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getHorizontalSliderDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getVerticalSliderImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getVerticalSliderDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getProgressBarImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getProgressBarDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getStaticTextImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getStaticTextDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getTextEntryImageDrawFunc() override { return nullptr; }
	virtual GameWinDrawFunc getTextEntryDrawFunc() override { return nullptr; }
};

// ---- GameClientStub: Milestone 12's own class, with exactly ONE change
// (Draft 37 Cost B) - friend_createDrawable() now returns a real Drawable
// instead of nullptr, copied verbatim from the real
// W3DGameClient::friend_createDrawable()'s own one-line body. ----
class GameClientStub : public GameClient
{
public:
	GameClientStub() {}
	virtual ~GameClientStub() {}

	virtual void createRayEffectByTemplate( const Coord3D * /*start*/, const Coord3D * /*end*/, const ThingTemplate * /*tmpl*/ ) override {}
	virtual void addScorch(const Coord3D * /*pos*/, Real /*radius*/, Scorches /*type*/) override {}

	// TheSuperHackers @port Milestone 13 (native port plan, Draft 37, Cost
	// B): real body, copied verbatim from the real
	// W3DGameClient::friend_createDrawable() (GeneralsMD/Code/
	// GameEngineDevice/.../W3DDevice/GameClient/W3DGameClient.cpp) - Drawable
	// is a concrete GameEngine-tier class already in this harness's own
	// linked closure. Milestone 12's own nullptr body would crash the real
	// GameLogic::bindObjectAndDrawable()'s unconditional
	// "draw->friend_bindToObject(obj)" dereference (confirmed by reading
	// that real call site) - only reached once this milestone starts
	// calling TheThingFactory->newObject() for real, which Milestone 12
	// never did.
	virtual Drawable *friend_createDrawable( const ThingTemplate *thing, DrawableStatusBits statusBits = DRAWABLE_STATUS_DEFAULT ) override
	{
		return newInstance(Drawable)(thing, statusBits);
	}

	virtual void setTeamColor( Int /*red*/, Int /*green*/, Int /*blue*/ ) override {}
	virtual void setTextureLOD( Int /*level*/ ) override {}
	virtual void notifyTerrainObjectMoved(Object * /*obj*/) override {}

	virtual Display *createGameDisplay() override { return NEW DisplayStub; }
	virtual InGameUI *createInGameUI() override { return NEW InGameUIStub; }
	virtual GameWindowManager *createWindowManager() override { return NEW GameWindowManagerStub; }
	virtual Mouse *createMouse() override { return NEW MouseStub; }
	virtual FontLibrary *createFontLibrary() override { return NEW FontLibraryStub; }
	virtual DisplayStringManager *createDisplayStringManager() override { return NEW DisplayStringManagerStub; }
	virtual VideoPlayerInterface *createVideoPlayer() override { return NEW VideoPlayer; }
	virtual TerrainVisual *createTerrainVisual() override { return NEW TerrainVisualStub; }
	// createKeyboard(): dead code in this harness's own m_headless == TRUE
	// configuration DURING engine.init() (see PosixGameEngine's own header
	// comment / main.cpp's own header comment for why m_headless is later
	// flipped to FALSE only AFTER construction, purely so
	// W3DView::updateCameraTransform() stops early-returning - by then,
	// GameClient::init()'s own "if (!TheGlobalData->m_headless)" keyboard
	// block has already run, with m_headless still TRUE, so this factory is
	// never actually invoked) - matches Milestone 11/12's own established
	// choice.
	virtual Keyboard *createKeyboard() override { return nullptr; }
	virtual SnowManager *createSnowManager() override { return nullptr; }

	virtual void setFrameRate(Real /*msecsPerFrame*/) override {}
};

// ---- ModuleFactoryStub: NEW for Milestone 13 (Draft 37's own design) - the
// real, non-pure base ModuleFactory::init() (already part of the linked
// closure) plus ONE addModule(W3DModelDraw) call, the SAME real macro the
// game's own W3DModuleFactory::init() uses. See this file's own header
// comment for the full rationale. ----
class ModuleFactoryStub : public ModuleFactory
{
public:
	ModuleFactoryStub() {}
	virtual ~ModuleFactoryStub() {}

	virtual void init() override
	{
		ModuleFactory::init();
		addModule( W3DModelDraw );
	}
};

// ---- PosixGameEngine: Milestone 12's own class, with exactly ONE change -
// createModuleFactory() now returns this milestone's own ModuleFactoryStub
// (registering W3DModelDraw) instead of the plain base ModuleFactory. ----
class PosixGameEngine : public GameEngine
{
public:
	PosixGameEngine() {}
	virtual ~PosixGameEngine() {}

	virtual LocalFileSystem *createLocalFileSystem() override { return NEW StdLocalFileSystem; }
	virtual ArchiveFileSystem *createArchiveFileSystem() override { return NEW StdBIGFileSystem; }

	// TheSuperHackers @port Milestone 13 (native port plan, Draft 37): this
	// milestone's own PosixGameLogicStub (see above) - the one change from
	// Milestone 12's plain "NEW GameLogic".
	virtual GameLogic *createGameLogic() override { return NEW PosixGameLogicStub; }

	virtual GameClient *createGameClient() override { return NEW GameClientStub; }

	virtual Radar *createRadar(Bool /*dummy*/) override { return NEW RadarDummy; }
	virtual ParticleSystemManager *createParticleSystemManager(Bool /*dummy*/) override { return NEW ParticleSystemManagerDummy; }

	virtual ThingFactory *createThingFactory() override { return NEW ThingFactory; }

	// TheSuperHackers @port Milestone 13 (native port plan, Draft 37): this
	// milestone's own ModuleFactoryStub (see above) - the one change from
	// Milestone 12's plain "NEW ModuleFactory".
	virtual ModuleFactory *createModuleFactory() override { return NEW ModuleFactoryStub; }

	virtual FunctionLexicon *createFunctionLexicon() override { return NEW FunctionLexicon; }

	virtual WebBrowser *createWebBrowser() override { return nullptr; }

	virtual AudioManager *createAudioManager(Bool /*dummy*/) override { return NEW AudioManagerStub; }
};
