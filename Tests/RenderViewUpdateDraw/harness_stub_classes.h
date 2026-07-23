// Phase 5(a) Milestone 11 RETRY (native port plan, rung 3b-ii-a, Draft 35) -
// the five harness-local, minimal concrete stub subclasses Draft 35's
// design decisions originally called for: GameClientStub / InGameUIStub /
// DisplayStub / FontLibraryStub / MouseStub, each overriding ONLY its
// abstract base's pure virtuals. This is now genuinely sufficient - the
// first attempt at this milestone found it was NOT (see CMakeLists.txt's
// own header comment for the full story: a derived class's vtable needs
// every base virtual resolved, not just the overridden ones) - because this
// harness's CMakeLists.txt now links the REAL GameClient.cpp/InGameUI.cpp/
// Display.cpp/GameFont.cpp/Mouse.cpp bodies (via Milestone 10's DEFER-closure
// technique) for every non-pure virtual these stubs leave untouched.
//
// None of these five classes' factory methods (createGameDisplay(),
// createInGameUI(), etc.) are EVER CALLED by this harness - main.cpp
// constructs each concrete stub directly with `NEW`, never through
// GameClient::init()'s own factory-method call chain (init() itself is
// never called at all) - so every stub body below is a loud, one-line
// no-op/nullptr-return, never actually invoked at runtime. Each is commented
// with which real subclass would normally implement it for real
// (W3DGameClient / W3DInGameUI / W3DDisplay / Win32BIGFontLibrary-equivalent
// / Win32Mouse-equivalent), none of which this port compiles on POSIX.
//
// TheSuperHackers @test Milestone 11 retry: minimal concrete stub subclasses,
// pure-virtuals-only, now safe under the DEFER-closure link strategy.

#pragma once

#include "GameClient/GameClient.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Display.h"
#include "GameClient/GameFont.h"
#include "GameClient/Mouse.h"
#include "GameClient/GameWindowManager.h"
#include "GameLogic/TerrainLogic.h"

// ---- HarnessTerrainLogic: a real, implementation-time finding this retry
// discovered (NOT flagged by Draft 35): the BASE TerrainLogic::getExtent()/
// getExtentIncludingBorder() (Include/GameLogic/TerrainLogic.h) are each a
// bare "{ DEBUG_CRASH(("not implemented")); }" with NO body otherwise - in a
// Release build DEBUG_CRASH is a true no-op, so the real, unmodified caller
// code (W3DView::calcCameraAreaConstraints()/getAxisAlignedViewRegion(),
// both genuinely reached by this harness's real view->update() call) passes
// in an UNINITIALIZED, stack-allocated Region3D and gets back GARBAGE - a
// real, latent uninitialized-read hazard in the engine's own base class, not
// something introduced by this harness. calcCameraAreaConstraints() then
// clips the camera's own pivot position into a garbage-derived box, moving
// it away from wherever lookAt()/setPosition() actually placed it - directly
// observed corrupting this harness's own pixel checks (Check 5/6/8) before
// this override was added. A minimal, loud, disclosed override supplying a
// real, finite, large-enough box (matching the "flat, unloaded, effectively
// infinite" terrain this harness's whole design already assumes for
// getGroundHeight()) fixes the uninitialized read without touching any real
// engine file. ----
class HarnessTerrainLogic : public TerrainLogic
{
public:
	virtual void getExtent( Region3D *extent ) const override
	{
		extent->lo.x = extent->lo.y = extent->lo.z = -100000.0f;
		extent->hi.x = extent->hi.y = extent->hi.z = 100000.0f;
	}
	virtual void getExtentIncludingBorder( Region3D *extent ) const override
	{
		getExtent(extent);
	}
};

// ---- GameClientStub: overrides GameClient's pure virtuals only (the real
// subclass would be W3DGameClient, GeneralsMD/Code/GameEngineDevice/.../
// W3DDevice/GameClient/W3DGameClient.cpp - not compiled on POSIX). ----
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

	// Factory methods - never called (init() is never called on this stub).
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
	virtual void setFrameRate(Real /*msecsPerFrame*/) override {}
};

// ---- InGameUIStub: overrides InGameUI's 2 pure virtuals only (the real
// subclass would be W3DInGameUI, GeneralsMD/Code/GameEngineDevice/.../
// W3DDevice/GameClient/W3DInGameUI.cpp - not compiled on POSIX). ----
class InGameUIStub : public InGameUI
{
public:
	InGameUIStub() {}
	virtual ~InGameUIStub() {}

	virtual void draw() override {}
	virtual View *createView(bool /*dummy*/ = false) override { return nullptr; }
};

// ---- DisplayStub: overrides Display's pure virtuals only (the real
// subclass would be W3DDisplay, Core/GameEngineDevice/.../W3DDevice/
// GameClient/W3DDisplay.cpp - deliberately NOT compiled on POSIX this port
// cycle, per Milestone 8/9's own standing decision). ----
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
// POSIX). Resolves the GameClient destructor hazard the draft documents
// (~GameClient() unconditionally does "TheFontLibrary->reset(); delete
// TheFontLibrary;") by providing a real, minimal, non-null instance. ----
class FontLibraryStub : public FontLibrary
{
public:
	FontLibraryStub() {}
	virtual ~FontLibraryStub() {}

protected:
	virtual Bool loadFontData( GameFont * /*font*/ ) override { return FALSE; }
};

// ---- MouseStub: overrides Mouse's 4 pure virtuals only (the real subclass
// would be a Win32-backed mouse device - not compiled on POSIX). Resolves
// the GameClient destructor hazard's second half ("TheMouse->reset();
// delete TheMouse;") by providing a real, minimal, non-null instance. ----
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

// ---- GameWindowManagerStub: a real, implementation-time finding this retry
// discovered (NOT flagged by Draft 35, which explicitly listed TheWindowManager
// among the singletons deliberately left null, matching TheTerrainRenderObject/
// TheRadar - a decision that was safe for pickDrawable()'s own null-guarded
// use, but NOT for what this retry newly enables): the REAL
// InGameUI::~InGameUI() (GeneralsMD/Code/GameEngine/Source/GameClient/
// InGameUI.cpp:1281) unconditionally calls stopCameoMovie(), whose real body
// (:4341) unconditionally dereferences TheWindowManager with NO null guard
// ("TheWindowManager->winGetWindowFromId(...)") - genuinely RUNTIME-live
// (not just link-live): this harness's own teardown really does "delete
// TheGameClient", whose own real ~GameClient() unconditionally deletes
// TheInGameUI, cascading into this call. A real GDB backtrace (not
// prediction) confirmed this exact crash the first time this teardown path
// ran for real. The real subclass would be GameWindowManagerScript-mode-only
// factory output on Windows (not compiled on POSIX) - this stub overrides
// GameWindowManager's 23 pure virtuals only (all pure-drawing-function-
// pointer accessors or the window-allocation factory, none ever invoked -
// this harness never creates a GameWindow), letting winGetWindowFromId()'s
// own real, non-pure, now-linked body (GameWindowManager.cpp, part of the
// DEFER-closure) run for real: with an empty window list (the real
// constructor's own m_windowList = nullptr default), it safely returns
// nullptr instead of crashing. ----
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
