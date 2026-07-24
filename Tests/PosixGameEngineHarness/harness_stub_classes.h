// Phase 5(a) Milestone 12 (native port plan, Draft 36, "rung 2b") - the
// harness-local stub-subclass surface driving the REAL, unmodified
// GameEngine::init()/update()/execute(). Extends Milestone 11's five-class
// pattern (Tests/RenderViewUpdateDraw/harness_stub_classes.h - GameClientStub/
// InGameUIStub/DisplayStub/FontLibraryStub/MouseStub, each overriding ONLY
// its abstract base's pure virtuals, safe under the DEFER-closure link
// strategy since the real, non-pure base bodies come from the linked
// z_gameengine/corei_gameengine_private closure) with three genuinely NEW
// stub classes Draft 36 identified: AudioManagerStub (43 pure virtuals),
// TerrainVisualStub (28), DisplayStringManagerStub (4, plus a small
// DisplayString stub) - none of which Milestone 11 needed, since that
// milestone never called GameClient::init()/GameEngine::init() for real.
//
// A key difference from Milestone 11's own GameClientStub: THIS milestone
// DOES call the real GameClient::init() (via the real GameEngine::init()'s
// own initSubsystem(TheGameClient, ...) call) - so, unlike Milestone 11
// (which never called init() and could leave every factory returning
// nullptr), several of GameClientStub's factories here must return REAL
// instances, because GameClient::init()'s own body genuinely dereferences
// their results unconditionally (see each factory's own comment below for
// the exact real call site). Two of GameClient::init()'s own factories
// (createKeyboard()/createMouse()/createWindowManager()) are short-circuited
// by the real engine's own headless-mode ternaries
// ("TheGlobalData->m_headless ? NEW MouseDummy : createMouse()", etc.,
// GameClient.cpp:324/342, and createKeyboard() is skipped entirely under
// "if (!TheGlobalData->m_headless)") - this harness always runs with
// m_headless == TRUE (see main.cpp), so those three GameClientStub factories
// are genuinely never invoked and stay trivial nullptr-returning stubs,
// exactly like Milestone 11's originals.
//
// TheSuperHackers @test Milestone 12: real, minimal concrete stub subclasses
// driving the real, unmodified GameEngine::init()/update()/execute().

#pragma once

#include "GameClient/GameClient.h"
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
#include "Common/GameAudio.h"
#include "Common/GameEngine.h"
#include "Common/LocalFileSystem.h"
#include "Common/ArchiveFileSystem.h"
#include "Common/Radar.h"
#include "GameClient/ParticleSys.h"
#include "Common/ThingFactory.h"
#include "Common/ModuleFactory.h"
#include "Common/FunctionLexicon.h"
#include "GameLogic/GameLogic.h"
#include "StdDevice/Common/StdLocalFileSystem.h"
#include "StdDevice/Common/StdBIGFileSystem.h"

// ---- DisplayStringStub: overrides DisplayString's 7 pure virtuals only
// (the real subclass would be a device-tier text-rendering implementation -
// not compiled on POSIX). Only ever constructed by
// DisplayStringManagerStub::newDisplayString() below; this harness never
// calls draw()/getSize()/getWidth() on any instance (zero on-screen text),
// so every body is a loud, one-line no-op/zero-return, matching this port's
// established stub-body convention. ----
class DisplayStringStub : public DisplayString
{
public:
	DisplayStringStub() {}

	// create(): MemoryPoolObject's operator new/delete (inherited from
	// DisplayString via MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE) are
	// protected, and DisplayString only grants friend access to
	// DisplayStringManager itself - NOT to derived classes (C++ friendship
	// is not inherited), so DisplayStringManagerStub::newDisplayString()
	// cannot construct one directly. A static factory method defined IN
	// DisplayStringStub's own class scope can, since protected members are
	// accessible to a class's own member functions.
	//
	// TheSuperHackers @info Milestone 12 implementation finding: plain "NEW
	// DisplayStringStub" compiles but throws ERROR_BUG at runtime -
	// MEMORY_POOL_GLUE_WITHOUT_GCMP's own plain, single-argument
	// "operator new(size_t s)" (GameMemory.h:655-661) is a deliberate
	// misuse trap ("This operator new should normally never be called...")
	// that always throws; the REAL, working construction path for any
	// MemoryPoolObject-derived class is the "newInstance(ARGCLASS)" macro
	// (GameMemory.h:200/214, expanding to the special MagicEnum-tagged
	// overload that actually calls getClassMemoryPool()->
	// allocateBlockImplementation(...)) - the SAME macro this port's own
	// GameLogicTickHarness/RenderCameraTransform harnesses already use for
	// "TheWaterTransparency = newInstance(WaterTransparencySetting);".
	// "NEW"/"MSGNEW" are for ordinary, non-pooled classes only.
	//
	// TheSuperHackers @info Milestone 12 implementation finding, continued:
	// the "newInstance(ARGCLASS)" macro itself expands to
	// "new(ARGCLASS::ARGCLASS##_GLUE_NOT_IMPLEMENTED) ARGCLASS" - both the
	// MagicEnum tag AND the constructed type come from the SAME ARGCLASS,
	// which only works when the class actually being constructed is the
	// one the memory-pool-glue macro was invoked on. DisplayStringStub
	// itself has no memory pool glue (only its base DisplayString does,
	// inherited here), so "newInstance(DisplayStringStub)" doesn't compile
	// ("DisplayStringStub_GLUE_NOT_IMPLEMENTED is not a member of
	// DisplayStringStub") - the placement-tag must be spelled out
	// explicitly against the base class that actually owns it, while the
	// constructed type is still the derived DisplayStringStub.
	static DisplayStringStub *create() { return new(DisplayString::DisplayString_GLUE_NOT_IMPLEMENTED) DisplayStringStub; }

	virtual void setWordWrap( Int /*wordWrap*/ ) override {}
	virtual void setWordWrapCentered( Bool /*isCentered*/ ) override {}
	virtual void draw( Int /*x*/, Int /*y*/, Color /*color*/, Color /*dropColor*/ ) override {}
	virtual void draw( Int /*x*/, Int /*y*/, Color /*color*/, Color /*dropColor*/, Int /*xDrop*/, Int /*yDrop*/ ) override {}
	virtual void getSize( Int *width, Int *height ) override { *width = 0; *height = 0; }
	virtual Int getWidth( Int /*charPos*/ = -1 ) override { return 0; }
	virtual void setUseHotkey( Bool /*useHotkey*/, Color /*hotKeyColor*/ ) override {}
};

// ---- DisplayStringManagerStub: overrides DisplayStringManager's 4 pure
// virtuals only (the real subclass would be a device-tier text-rendering
// manager - not compiled on POSIX). Genuinely load-bearing, not merely
// pure-virtual completeness: GameClient::init()'s own body unconditionally
// dereferences "TheDisplayStringManager->postProcessLoad()" (GameClient.cpp:433)
// with NO null guard (confirmed by reading, not assumed - Draft 36's own
// finding), so createDisplayStringManager() below returns a real, non-null
// instance rather than nullptr. newDisplayString()/getGroupNumeralString()/
// getFormationLetterString() ARE real, callable bodies (not stubs) - they
// use the base DisplayStringManager::link()/unLink() bookkeeping (already
// real, non-pure, part of the linked closure), so any real caller reachable
// from this harness's own construct/init/update/execute path gets a real,
// working (if visually inert) DisplayString back, not a null-deref hazard. ----
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

// ---- TerrainVisualStub: overrides TerrainVisual's 28 pure virtuals only
// (the real subclass would be W3DTerrainVisual, GeneralsMD/Code/GameEngineDevice/.../
// W3DDevice/GameClient/W3DTerrainVisual.cpp - not compiled on POSIX, same
// WW3D2-rendering-closure reasoning as every other W3D-prefixed device class
// this port's harnesses stub out). The real, non-pure base TerrainVisual::
// init()/reset()/update()/load() (TerrainVisual.cpp, part of the linked
// corei_gameengine_private closure) are all real, trivial, no-INI bodies -
// see that file's own header comment - so this stub needs no constructor-
// time setup beyond the base class's own. Every override below is a loud,
// one-line no-op/zero-return/false-return: this harness never loads a map,
// so none of these water/bib/prop/height-modification entry points are ever
// actually invoked at runtime - link-live only (GameClient::init()'s own
// "TheTerrainVisual->init()" call reaches only the real, concrete base
// methods, none of these). ----
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

// ---- AudioManagerStub: overrides AudioManager's 42 pure virtuals (43 in a
// DEBUG build - audioDebugDisplay() is guarded #if defined(RTS_DEBUG), this
// port's harnesses always compile RTS_RELEASE, matching every other
// harness's own build configuration) - the real subclass would be
// MilesAudioManager (Core/GameEngineDevice/.../MilesAudioDevice/
// MilesAudioManager.h, a genuine Miles SDK dependency, out of scope per
// Draft 36's own retirement of that blocker: the fix is a harness-local
// factory override, not solving anything Miles-SDK-specific). The real,
// non-pure base AudioManager::init() (GameAudio.cpp:215) is fully portable
// (INI::loadFileDirectory calls against this harness's own test-authored
// scaffold + two NEW allocations for m_music/m_sound - MusicManager/
// SoundManager, both already-portable base classes with no device
// dependency); m_audioSettings/m_miscAudio/m_silentAudioEvent are all
// constructor-allocated by the real AudioManager() base constructor
// (GameAudio.cpp:177-179), so no null-deref risk even against this
// harness's near-empty INI content. The real, non-pure base
// AudioManager::update() unconditionally dereferences TheTacticalView
// (GameAudio.cpp:285, "TheTacticalView->getPosition()") - genuinely safe
// here because this harness's own InGameUIStub::createView() (below)
// returns a real ViewDummy, not nullptr, matching Draft 36's own finding. ----
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

// ---- InGameUIStub: overrides InGameUI's 2 pure virtuals only (the real
// subclass would be W3DInGameUI - not compiled on POSIX). createView()
// DIFFERS from Milestone 11's own InGameUIStub (which always returned
// nullptr, safe there since InGameUI::init() was never called): THIS
// milestone's real GameClient::init() DOES call the real
// "TheInGameUI->init()", whose own real body unconditionally does
// "TheTacticalView = createView(TheGlobalData->m_headless);" followed by
// "if (TheTacticalView && TheDisplay) { TheTacticalView->init(); ... }"
// (InGameUI.cpp:1370-1382) - with m_headless always TRUE in this harness,
// returning a real ViewDummy (Core/GameEngine/Include/GameClient/View.h, a
// real, already-portable, headless-mode-designed class - NOT a harness
// invention) here is both the semantically-correct choice (the "dummy"
// argument IS TheGlobalData->m_headless) and the fix for AudioManagerStub's
// own unconditional "TheTacticalView->getPosition()" dereference. ----
class InGameUIStub : public InGameUI
{
public:
	InGameUIStub() {}
	virtual ~InGameUIStub() {}

	virtual void draw() override {}
	virtual View *createView(bool /*dummy*/ = false) override { return NEW ViewDummy; }
};

// ---- DisplayStub: overrides Display's pure virtuals only (the real
// subclass would be W3DDisplay - deliberately NOT compiled on POSIX this
// port cycle). Verbatim copy of Milestone 11's own DisplayStub (Tests/
// RenderViewUpdateDraw/harness_stub_classes.h) - genuinely load-bearing here
// too: GameClient::init() calls "TheDisplay = createGameDisplay();"
// UNCONDITIONALLY (not headless-gated, GameClient.cpp:330), and
// GameClient::update()'s own intro-branch unconditionally dereferences
// "TheDisplay->UPDATE(); TheDisplay->DRAW();" with no null guard whenever
// m_intro is still non-null (GameClient.cpp:612-614) - real, non-null
// required. ----
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

// ---- FontLibraryStub: overrides FontLibrary's one pure virtual only (the
// real subclass would be a Win32 GDI-backed font loader - not compiled on
// POSIX). Genuinely load-bearing: GameClient::init() calls
// "TheFontLibrary = createFontLibrary();" unconditionally (GameClient.cpp:319)
// and ~GameClient() unconditionally does "TheFontLibrary->reset(); delete
// TheFontLibrary;". ----
class FontLibraryStub : public FontLibrary
{
public:
	FontLibraryStub() {}
	virtual ~FontLibraryStub() {}

protected:
	virtual Bool loadFontData( GameFont * /*font*/ ) override { return FALSE; }
};

// ---- MouseStub: overrides Mouse's 4 pure virtuals only. Kept for interface
// completeness (GameClientStub::createMouse() must return SOME concrete
// type), but genuinely never invoked in this harness's own configuration:
// GameClient::init()'s own real body short-circuits with
// "TheMouse = TheGlobalData->m_headless ? NEW MouseDummy : createMouse();"
// (GameClient.cpp:324) - a real, already-portable, engine-provided headless
// class (Core/GameEngine/Include/GameClient/Mouse.h) - and this harness
// always runs with m_headless == TRUE, so createMouse() is dead code here. ----
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

// ---- GameWindowManagerStub: overrides GameWindowManager's 23 pure virtuals.
// Kept for interface completeness (GameClientStub::createWindowManager()
// must return SOME concrete type), but genuinely never invoked in this
// harness's own configuration: GameClient::init()'s own real body
// short-circuits with "TheWindowManager = TheGlobalData->m_headless ? NEW
// GameWindowManagerDummy : createWindowManager();" (GameClient.cpp:342) - a
// real, already-portable, engine-provided headless class
// (GeneralsMD/Code/GameEngine/Include/GameClient/GameWindowManager.h) - and
// this harness always runs with m_headless == TRUE, so createWindowManager()
// is dead code here. ----
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

// ---- GameClientStub: overrides GameClient's pure virtuals only (the real
// subclass would be W3DGameClient - not compiled on POSIX). UNLIKE
// Milestone 11's own GameClientStub (whose factories were ALL nullptr,
// since that milestone never called GameClient::init()), several factories
// here return real instances - see each one's own comment for the exact
// real call site that requires it. ----
class GameClientStub : public GameClient
{
public:
	GameClientStub() {}
	virtual ~GameClientStub() {}

	virtual void createRayEffectByTemplate( const Coord3D * /*start*/, const Coord3D * /*end*/, const ThingTemplate * /*tmpl*/ ) override {}
	virtual void addScorch(const Coord3D * /*pos*/, Real /*radius*/, Scorches /*type*/) override {}
	virtual Drawable *friend_createDrawable( const ThingTemplate * /*thing*/, DrawableStatusBits /*statusBits*/ = DRAWABLE_STATUS_DEFAULT ) override { return nullptr; }
	virtual void setTeamColor( Int /*red*/, Int /*green*/, Int /*blue*/ ) override {}
	virtual void setTextureLOD( Int /*level*/ ) override {}
	virtual void notifyTerrainObjectMoved(Object * /*obj*/) override {}

	// createGameDisplay(): GameClient::init() calls this UNCONDITIONALLY
	// (GameClient.cpp:330, "TheDisplay = createGameDisplay();"), and
	// GameClient::update()'s intro-branch unconditionally dereferences the
	// result (see DisplayStub's own comment) - real instance required.
	virtual Display *createGameDisplay() override { return NEW DisplayStub; }

	// createInGameUI(): GameClient::init() calls this UNCONDITIONALLY
	// (GameClient.cpp:368, "TheInGameUI = createInGameUI();"), and its
	// result's own init() is called right after - real instance required
	// (see InGameUIStub's own comment for the TheTacticalView/ViewDummy
	// consequence).
	virtual InGameUI *createInGameUI() override { return NEW InGameUIStub; }

	// createWindowManager()/createMouse(): dead code in this harness's own
	// m_headless == TRUE configuration - see GameWindowManagerStub's/
	// MouseStub's own comments.
	virtual GameWindowManager *createWindowManager() override { return NEW GameWindowManagerStub; }
	virtual Mouse *createMouse() override { return NEW MouseStub; }

	// createFontLibrary(): GameClient::init() calls this UNCONDITIONALLY
	// (GameClient.cpp:319), and ~GameClient() unconditionally deletes the
	// result - real instance required (see FontLibraryStub's own comment).
	virtual FontLibrary *createFontLibrary() override { return NEW FontLibraryStub; }

	// createDisplayStringManager(): GameClient::init()'s own
	// "TheDisplayStringManager->postProcessLoad()" call (GameClient.cpp:433)
	// is unconditionally dereferenced with NO null guard - real instance
	// required (see DisplayStringManagerStub's own comment, Draft 36's own
	// finding).
	virtual DisplayStringManager *createDisplayStringManager() override { return NEW DisplayStringManagerStub; }

	// createVideoPlayer(): GameClient::update()'s own body unconditionally
	// dereferences "TheVideoPlayer->UPDATE();" (GameClient.cpp:624, no null
	// guard) - the REAL, concrete VideoPlayer base class (GameClient/
	// VideoPlayer.h) has zero pure virtuals left (confirmed by reading, not
	// assumed - every VideoPlayerInterface pure virtual is overridden with a
	// real, non-pure body), so a plain "NEW VideoPlayer" is the honest
	// choice here, not a harness-local stub class.
	virtual VideoPlayerInterface *createVideoPlayer() override { return NEW VideoPlayer; }

	// createTerrainVisual(): GameClient::init() calls this UNCONDITIONALLY
	// (GameClient.cpp:386, "TheTerrainVisual = createTerrainVisual();"), and
	// its result's own init() is called right after - real instance
	// required (see TerrainVisualStub's own comment).
	virtual TerrainVisual *createTerrainVisual() override { return NEW TerrainVisualStub; }

	// createKeyboard(): dead code in this harness's own m_headless == TRUE
	// configuration - GameClient::init() only calls this
	// "if (!TheGlobalData->m_headless)" (GameClient.cpp:271-275), never true
	// here.
	virtual Keyboard *createKeyboard() override { return nullptr; }

	// createSnowManager(): every real call site null-guards its result
	// ("if (TheSnowManager) ..." - GameClient.cpp:436, :477, :572) - nullptr
	// stays genuinely safe, matching Milestone 11's own established choice.
	virtual SnowManager *createSnowManager() override { return nullptr; }

	virtual void setFrameRate(Real /*msecsPerFrame*/) override {}
};

// ---- PosixGameEngine: the milestone's own payoff class - a harness-local
// GameEngine subclass overriding ONLY the 11 pure factory methods
// (GameEngine.h:87-98, confirmed by reading, not assumed - exactly these 11,
// no more, no less). Everything else (init()/update()/execute()/reset()/
// isTimeFrozen()/isGameHalted()/the constructor/destructor) is the REAL,
// UNMODIFIED GameEngine class - this is not a bypass, and no singleton this
// harness's own factories don't directly own is hand-constructed; the real
// init() chain constructs everything else itself, exactly as Draft 36's
// scope statement requires. ----
class PosixGameEngine : public GameEngine
{
public:
	PosixGameEngine() {}
	virtual ~PosixGameEngine() {}

	// createLocalFileSystem()/createArchiveFileSystem(): the real, already-
	// portable POSIX classes Milestone 7 shipped (Core/GameEngineDevice/.../
	// StdDevice/Common/) - genuinely load-bearing here (unlike every prior
	// milestone's own INI-free shortcut): this milestone's real
	// GameEngine::init() genuinely loads this harness's own test-authored
	// Data/INI scaffold (see this directory's own Data/ subtree) through
	// them.
	virtual LocalFileSystem *createLocalFileSystem() override { return NEW StdLocalFileSystem; }
	virtual ArchiveFileSystem *createArchiveFileSystem() override { return NEW StdBIGFileSystem; }

	// createGameLogic(): the plain, base GameLogic class - matches
	// Milestone 10's own choice (Tests/GameLogicTickHarness/main.cpp's own
	// header comment): the base class has zero pure virtuals, and its own
	// createTerrainLogic()/createGhostObjectManager() already return the
	// portable base TerrainLogic/GhostObjectManagerDummy classes this
	// harness needs (m_headless == TRUE selects the Dummy variant) -
	// W3DGameLogic would pull in the full WW3D2 rendering closure via
	// W3DTerrainLogic.cpp, out of scope here exactly as it was for
	// Milestone 10.
	virtual GameLogic *createGameLogic() override { return NEW GameLogic; }

	// createGameClient(): this milestone's own extended GameClientStub (see
	// above) - Milestone 11's five-class stub-subclass pattern, upgraded
	// with real instances for the three genuinely new stub classes
	// (AudioManagerStub is a GameEngine-level factory below, not
	// GameClient's - TerrainVisualStub/DisplayStringManagerStub are).
	virtual GameClient *createGameClient() override { return NEW GameClientStub; }

	// createRadar()/createParticleSystemManager(): both real, already-
	// portable, engine-provided headless-mode classes (Core/GameEngine/
	// Include/Common/Radar.h / GameClient/ParticleSys.h) - NOT harness
	// inventions, matching Draft 36's own "already proven portable"
	// finding. The `dummy` argument passed in is TheGlobalData->m_headless
	// (always TRUE in this harness), but both real classes are chosen here
	// unconditionally since this harness never runs in a non-headless
	// configuration anyway.
	virtual Radar *createRadar(Bool /*dummy*/) override { return NEW RadarDummy; }
	virtual ParticleSystemManager *createParticleSystemManager(Bool /*dummy*/) override { return NEW ParticleSystemManagerDummy; }

	// createThingFactory()/createModuleFactory()/createFunctionLexicon():
	// concrete base classes with zero pure virtuals (confirmed by reading
	// each header directly, not assumed - Draft 36's own claim, verified
	// during this milestone's own implementation).
	virtual ThingFactory *createThingFactory() override { return NEW ThingFactory; }
	virtual ModuleFactory *createModuleFactory() override { return NEW ModuleFactory; }
	virtual FunctionLexicon *createFunctionLexicon() override { return NEW FunctionLexicon; }

	// createWebBrowser(): nullptr, now legitimately unneeded (not merely
	// unimplemented) once GameEngine.cpp's own #include of
	// GameNetwork/WOLBrowser/WebBrowser.h is guarded behind #ifdef _WIN32
	// (see GameEngine.cpp's own header comment on that guard) - the real
	// commented-out call site (GameEngine.cpp:684-685-ish,
	// "//initSubsystem((CComObject<WebBrowser> *)TheWebBrowser...") never
	// actually calls this factory at all.
	virtual WebBrowser *createWebBrowser() override { return nullptr; }

	// createAudioManager(): this milestone's own new AudioManagerStub (see
	// above) - NOT MilesAudioManager (Draft 36's own retirement of the
	// Miles SDK blocker: the fix is this harness-local factory override,
	// not solving anything Miles-SDK-specific).
	virtual AudioManager *createAudioManager(Bool /*dummy*/) override { return NEW AudioManagerStub; }
};
