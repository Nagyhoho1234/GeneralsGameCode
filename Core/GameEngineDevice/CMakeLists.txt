# Every file below this point is W3DDevice/Win32Device/MilesAudioDevice/
# VideoDevice content - D3D8/Win32/Miles/Bink-dependent, gated behind WIN32
# for now (native port plan Phase 1: minimum-viable stub so the game target
# at least builds on non-Windows; real portable device implementations are
# Phases 4-6). The portable StdDevice file-I/O block further down is
# deliberately NOT inside this guard - it already works cross-platform.
if(WIN32)
set(GAMEENGINEDEVICE_SRC
    Include/MilesAudioDevice/MilesAudioManager.h
    Include/VideoDevice/Bink/BinkVideoPlayer.h
#    Include/W3DDevice/Common/W3DConvert.h
#    Include/W3DDevice/Common/W3DFunctionLexicon.h
#    Include/W3DDevice/Common/W3DModuleFactory.h
    Include/W3DDevice/Common/W3DRadar.h
#    Include/W3DDevice/Common/W3DThingFactory.h
    Include/W3DDevice/GameClient/BaseHeightMap.h
#    Include/W3DDevice/GameClient/camerashakesystem.h
    Include/W3DDevice/GameClient/FlatHeightMap.h
    Include/W3DDevice/GameClient/HeightMap.h
    Include/W3DDevice/GameClient/Module/W3DDebrisDraw.h
    Include/W3DDevice/GameClient/Module/W3DDefaultDraw.h
    Include/W3DDevice/GameClient/Module/W3DDependencyModelDraw.h
    Include/W3DDevice/GameClient/Module/W3DLaserDraw.h
    Include/W3DDevice/GameClient/Module/W3DModelDraw.h
    Include/W3DDevice/GameClient/Module/W3DOverlordAircraftDraw.h
    Include/W3DDevice/GameClient/Module/W3DOverlordTankDraw.h
    Include/W3DDevice/GameClient/Module/W3DOverlordTruckDraw.h
    Include/W3DDevice/GameClient/Module/W3DPoliceCarDraw.h
    Include/W3DDevice/GameClient/Module/W3DProjectileStreamDraw.h
    Include/W3DDevice/GameClient/Module/W3DPropDraw.h
    Include/W3DDevice/GameClient/Module/W3DRopeDraw.h
    Include/W3DDevice/GameClient/Module/W3DScienceModelDraw.h
    Include/W3DDevice/GameClient/Module/W3DSupplyDraw.h
    Include/W3DDevice/GameClient/Module/W3DTankDraw.h
    Include/W3DDevice/GameClient/Module/W3DTankTruckDraw.h
    Include/W3DDevice/GameClient/Module/W3DTracerDraw.h
    Include/W3DDevice/GameClient/Module/W3DTreeDraw.h
    Include/W3DDevice/GameClient/Module/W3DTruckDraw.h
    Include/W3DDevice/GameClient/TerrainTex.h
    Include/W3DDevice/GameClient/TileData.h
#    Include/W3DDevice/GameClient/W3DAssetManager.h
#    Include/W3DDevice/GameClient/W3DAssetManagerExposed.h
#    Include/W3DDevice/GameClient/W3DBibBuffer.h
#    Include/W3DDevice/GameClient/W3DBridgeBuffer.h
#    Include/W3DDevice/GameClient/W3DBufferManager.h
#    Include/W3DDevice/GameClient/W3DCustomEdging.h
    Include/W3DDevice/GameClient/W3DCustomScene.h
#    Include/W3DDevice/GameClient/W3DDebugDisplay.h
#    Include/W3DDevice/GameClient/W3DDebugIcons.h
    Include/W3DDevice/GameClient/W3DDisplay.h
#    Include/W3DDevice/GameClient/W3DDisplayString.h
#    Include/W3DDevice/GameClient/W3DDisplayStringManager.h
    Include/W3DDevice/GameClient/W3DDynamicLight.h
    Include/W3DDevice/GameClient/W3DGadget.h
#    Include/W3DDevice/GameClient/W3DGameClient.h
#    Include/W3DDevice/GameClient/W3DGameFont.h
#    Include/W3DDevice/GameClient/W3DGameWindow.h
#    Include/W3DDevice/GameClient/W3DGameWindowManager.h
#    Include/W3DDevice/GameClient/W3DGUICallbacks.h
#    Include/W3DDevice/GameClient/W3DInGameUI.h
#    Include/W3DDevice/GameClient/W3DMirror.h
    Include/W3DDevice/GameClient/W3DMouse.h
#    Include/W3DDevice/GameClient/W3DParticleSys.h
#    Include/W3DDevice/GameClient/W3DPoly.h
    Include/W3DDevice/GameClient/W3DProfilerFrameCapture.h
#    Include/W3DDevice/GameClient/W3DProjectedShadow.h
    Include/W3DDevice/GameClient/W3DPropBuffer.h
#    Include/W3DDevice/GameClient/W3DRoadBuffer.h
    Include/W3DDevice/GameClient/W3DScene.h
    Include/W3DDevice/GameClient/W3DShaderManager.h
    Include/W3DDevice/GameClient/W3DShadow.h
    Include/W3DDevice/GameClient/W3DShroud.h
    Include/W3DDevice/GameClient/W3DSmudge.h
    Include/W3DDevice/GameClient/W3DSnow.h
    Include/W3DDevice/GameClient/W3DStatusCircle.h
    Include/W3DDevice/GameClient/W3DTerrainBackground.h
    Include/W3DDevice/GameClient/W3DTerrainTracks.h
    Include/W3DDevice/GameClient/W3DTerrainVisual.h
    Include/W3DDevice/GameClient/W3DTreeBuffer.h
    Include/W3DDevice/GameClient/W3DVideoBuffer.h
    Include/W3DDevice/GameClient/W3DView.h
    Include/W3DDevice/GameClient/W3DScreenshot.h
#    Include/W3DDevice/GameClient/W3DVolumetricShadow.h
    Include/W3DDevice/GameClient/W3DWater.h
    Include/W3DDevice/GameClient/W3DWaterTracks.h
#    Include/W3DDevice/GameClient/W3DWaypointBuffer.h
#    Include/W3DDevice/GameClient/W3DWebBrowser.h
    Include/W3DDevice/GameClient/WorldHeightMap.h
    Include/W3DDevice/GameLogic/W3DGameLogic.h
    Include/W3DDevice/GameLogic/W3DGhostObject.h
    Include/W3DDevice/GameLogic/W3DTerrainLogic.h
    Include/Win32Device/Common/Win32BIGFile.h
    Include/Win32Device/Common/Win32BIGFileSystem.h
#    Include/Win32Device/Common/Win32GameEngine.h
    Include/Win32Device/Common/Win32LocalFile.h
    Include/Win32Device/Common/Win32LocalFileSystem.h
    Include/Win32Device/GameClient/Win32DIKeyboard.h
    #Include/Win32Device/GameClient/Win32DIMouse.h
    Include/Win32Device/GameClient/Win32Mouse.h
    Source/MilesAudioDevice/MilesAudioManager.cpp
    Source/VideoDevice/Bink/BinkVideoPlayer.cpp
#    Source/W3DDevice/Common/System/W3DFunctionLexicon.cpp
    Source/W3DDevice/Common/System/W3DRadar.cpp
#    Source/W3DDevice/Common/Thing/W3DModuleFactory.cpp
#    Source/W3DDevice/Common/Thing/W3DThingFactory.cpp
#    Source/W3DDevice/Common/W3DConvert.cpp
    Source/W3DDevice/GameClient/BaseHeightMap.cpp
    Source/W3DDevice/GameClient/CameraShakeSystem.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DDebrisDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DDefaultDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DDependencyModelDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DLaserDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DModelDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DOverlordAircraftDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DOverlordTankDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DOverlordTruckDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DPoliceCarDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DProjectileStreamDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DPropDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DRopeDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DScienceModelDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DSupplyDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DTankDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DTankTruckDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DTracerDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DTreeDraw.cpp
    Source/W3DDevice/GameClient/Drawable/Draw/W3DTruckDraw.cpp
    Source/W3DDevice/GameClient/FlatHeightMap.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DCheckBox.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DComboBox.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DHorizontalSlider.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DListBox.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DProgressBar.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DPushButton.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DRadioButton.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DStaticText.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DTabControl.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DTextEntry.cpp
    Source/W3DDevice/GameClient/GUI/Gadget/W3DVerticalSlider.cpp
    Source/W3DDevice/GameClient/GUI/GUICallbacks/W3DControlBar.cpp
#    Source/W3DDevice/GameClient/GUI/GUICallbacks/W3DMainMenu.cpp
#    Source/W3DDevice/GameClient/GUI/GUICallbacks/W3DMOTD.cpp
#    Source/W3DDevice/GameClient/GUI/W3DGameFont.cpp
#    Source/W3DDevice/GameClient/GUI/W3DGameWindow.cpp
#    Source/W3DDevice/GameClient/GUI/W3DGameWindowManager.cpp
    Source/W3DDevice/GameClient/HeightMap.cpp
#    Source/W3DDevice/GameClient/Shadow/W3DBufferManager.cpp
#    Source/W3DDevice/GameClient/Shadow/W3DProjectedShadow.cpp
    Source/W3DDevice/GameClient/Shadow/W3DShadow.cpp
#    Source/W3DDevice/GameClient/Shadow/W3DVolumetricShadow.cpp
    Source/W3DDevice/GameClient/TerrainTex.cpp
    Source/W3DDevice/GameClient/TileData.cpp
#    Source/W3DDevice/GameClient/W3DAssetManager.cpp
#    Source/W3DDevice/GameClient/W3DAssetManagerExposed.cpp
#    Source/W3DDevice/GameClient/W3DBibBuffer.cpp
#    Source/W3DDevice/GameClient/W3DBridgeBuffer.cpp
#    Source/W3DDevice/GameClient/W3DCustomEdging.cpp
#    Source/W3DDevice/GameClient/W3DDebugDisplay.cpp
#    Source/W3DDevice/GameClient/W3DDebugIcons.cpp
    Source/W3DDevice/GameClient/W3DDisplay.cpp
#    Source/W3DDevice/GameClient/W3DDisplayString.cpp
#    Source/W3DDevice/GameClient/W3DDisplayStringManager.cpp
    Source/W3DDevice/GameClient/W3DDynamicLight.cpp
#    Source/W3DDevice/GameClient/W3DGameClient.cpp
#    Source/W3DDevice/GameClient/W3DInGameUI.cpp
    Source/W3DDevice/GameClient/W3DMouse.cpp
#    Source/W3DDevice/GameClient/W3DParticleSys.cpp
#    Source/W3DDevice/GameClient/W3DPoly.cpp
    Source/W3DDevice/GameClient/W3DProfilerFrameCapture.cpp
    Source/W3DDevice/GameClient/W3DPropBuffer.cpp
#    Source/W3DDevice/GameClient/W3DRoadBuffer.cpp
    Source/W3DDevice/GameClient/W3DScene.cpp
    Source/W3DDevice/GameClient/W3DShaderManager.cpp
    Source/W3DDevice/GameClient/W3DShroud.cpp
    Source/W3DDevice/GameClient/W3DSmudge.cpp
    Source/W3DDevice/GameClient/W3DSnow.cpp
    Source/W3DDevice/GameClient/W3DStatusCircle.cpp
    Source/W3DDevice/GameClient/W3DTerrainBackground.cpp
    Source/W3DDevice/GameClient/W3DTerrainTracks.cpp
    Source/W3DDevice/GameClient/W3DTerrainVisual.cpp
    Source/W3DDevice/GameClient/W3DTreeBuffer.cpp
    Source/W3DDevice/GameClient/W3DVideoBuffer.cpp
    Source/W3DDevice/GameClient/W3DView.cpp
    Source/W3DDevice/GameClient/W3DScreenshot.cpp
    Source/W3DDevice/GameClient/stb_image_write_impl.cpp
#    Source/W3DDevice/GameClient/W3dWaypointBuffer.cpp
#    Source/W3DDevice/GameClient/W3DWebBrowser.cpp
    Source/W3DDevice/GameClient/Water/W3DWater.cpp
    Source/W3DDevice/GameClient/Water/W3DWaterTracks.cpp
    Source/W3DDevice/GameClient/WorldHeightMap.cpp
    Source/W3DDevice/GameLogic/W3DGameLogic.cpp
    Source/W3DDevice/GameLogic/W3DGhostObject.cpp
    Source/W3DDevice/GameLogic/W3DTerrainLogic.cpp
    Source/Win32Device/Common/Win32BIGFile.cpp
    Source/Win32Device/Common/Win32BIGFileSystem.cpp
#    Source/Win32Device/Common/Win32GameEngine.cpp
    Source/Win32Device/Common/Win32LocalFile.cpp
    Source/Win32Device/Common/Win32LocalFileSystem.cpp
#    Source/Win32Device/Common/Win32OSDisplay.cpp
    Source/Win32Device/GameClient/Win32DIKeyboard.cpp
    #Source/Win32Device/GameClient/Win32DIMouse.cpp
    Source/Win32Device/GameClient/Win32Mouse.cpp
)
endif() # WIN32

# W3DDynamicLight/W3DShroud/W3DCustomScene/Shadow::W3DShadow/W3DStatusCircle:
# five small W3DScene.cpp closure siblings unified from the per-tree
# Generals/GeneralsMD copies into the WIN32-gated block above (native port
# plan, Draft 30 finding 1, Milestone 8 Task 1 - the rung-3a warm-up before
# W3DScene.cpp itself, Task 2). All five are GeneralsMD-wins with NO
# RTS_ZEROHOUR guard needed: verified by direct diff and sorted-line
# multiset comparison that every divergence from the Generals original is
# branding only (the "Generals(tm)" vs "Generals Zero Hour(tm)" copyright
# line), except W3DDynamicLight.h (one added `friend class
# BaseHeightMapRenderObjClass;`, a harmless superset) and
# W3DStatusCircle.cpp (ZH already added `#include ".../WorldHeightMap.h"`
# and dropped its own now-redundant, provably-unused-in-this-file
# VERTEX_BUFFER_TILE_LENGTH/VERTS_IN_BLOCK_ROW macro pair in favor of the
# identical ones WorldHeightMap.h already defines - a header-hygiene
# cleanup, not a logic change).

# W3DScene.cpp/.h: the game's real scene manager (RTS3DScene/RTS2DScene/
# RTS3DInterfaceScene), unified from the per-tree Generals/GeneralsMD copies
# into the WIN32-gated block above (native port plan, Draft 30 finding 2,
# Milestone 8 Task 2). GeneralsMD-wins, with "#if RTS_ZEROHOUR ... #else ...
# #endif" two-block guards (not a straddling #if, per the Milestone 7 Task 3
# follow-up fix) for every real behavioral divergence - see the .cpp's own
# header comment for the full enumeration (the load-bearing one is a
# cross-tree Drawable API rename, getHeatVisionOpacity vs
# getSecondMaterialPassOpacity) - plus a latent-uninitialized-pointer fix to
# m_frenzyMaterialPass (Draft 30 finding 9), applied unconditionally for both
# trees since the member is provably dead code today.

# W3DDisplay.cpp/.h: the game's real W3D display, unified from the per-tree
# Generals/GeneralsMD copies into the WIN32-gated block above (native port
# plan, Draft 32, Milestone 9 Task 1). The header's divergence is confirmed
# cosmetic (branding + two comment typo fixes) - no guard needed. The .cpp has
# nine real GeneralsMD-only hunks, each guarded with "#if RTS_ZEROHOUR ...
# #endif" (where Generals has no code at that spot) or the two-block
# "#if RTS_ZEROHOUR ... #else ... #endif" (where both trees genuinely
# diverge) - never a straddling #if either way. See the .cpp's own header
# comment for the full enumeration. This is a PURE REFACTOR:
# W3DDisplay.cpp still only compiles on Windows (WIN32-gated, as before); no
# POSIX portability work was attempted or is in scope here.

# Add C++ 17 FileSystem implementation for non-VS6 builds
if(NOT IS_VS6_BUILD)
    list(APPEND GAMEENGINEDEVICE_SRC
        Include/StdDevice/Common/StdBIGFile.h
        Include/StdDevice/Common/StdBIGFileSystem.h
        Include/StdDevice/Common/StdLocalFile.h
        Include/StdDevice/Common/StdLocalFileSystem.h
        Source/StdDevice/Common/StdBIGFile.cpp
        Source/StdDevice/Common/StdBIGFileSystem.cpp
        Source/StdDevice/Common/StdLocalFile.cpp
        Source/StdDevice/Common/StdLocalFileSystem.cpp
    )
endif()

# W3DFileSystem: the WW3D<->GameEngine file-I/O seam (native port plan,
# Draft 28 finding 3, Milestone 7 Task 3). Unified from the per-tree
# Generals/GeneralsMD copies (GeneralsMD wins, RTS_ZEROHOUR-guarded for the
# one genuinely-diverged piece, the localized-directory lookup - see the
# .cpp's own header comment). Deliberately NOT inside the WIN32 gate above,
# same "already portable" reasoning as the StdDevice block just above it:
# its only real deps are TheFileSystem/TheArchiveFileSystem/TheGlobalData
# (all null-guarded or already POSIX-clean) and GetRegistryLanguage()
# (portable, Core/GameEngine/Source/Common/System/registry.cpp). This grows
# the POSIX-reachable set of the game targets themselves, not just a test
# harness - verified against the 34-error WSL2 baseline (still 34/34 after
# this addition).
list(APPEND GAMEENGINEDEVICE_SRC
    Include/W3DDevice/GameClient/W3DFileSystem.h
    Source/W3DDevice/GameClient/W3DFileSystem.cpp
)

add_library(corei_gameenginedevice_private INTERFACE)
add_library(corei_gameenginedevice_public INTERFACE)

target_sources(corei_gameenginedevice_private INTERFACE ${GAMEENGINEDEVICE_SRC})

target_include_directories(corei_gameenginedevice_public INTERFACE
    Include
)

target_link_libraries(corei_gameenginedevice_private INTERFACE
    corei_always
    corei_main
    stb
)

target_link_libraries(corei_gameenginedevice_public INTERFACE
    corei_gameengine_public
)
if(WIN32)
    target_link_libraries(corei_gameenginedevice_public INTERFACE
        binkstub
        d3d8lib
        milesstub
    )
endif()

if(RTS_BUILD_OPTION_FFMPEG)
    find_package(FFMPEG REQUIRED)

    if(FFMPEG_FOUND)
        target_sources(corei_gameenginedevice_private INTERFACE
            Include/VideoDevice/FFmpeg/FFmpegFile.h
            Include/VideoDevice/FFmpeg/FFmpegVideoPlayer.h
            Source/VideoDevice/FFmpeg/FFmpegFile.cpp
            Source/VideoDevice/FFmpeg/FFmpegVideoPlayer.cpp
        )
        target_include_directories(corei_gameenginedevice_private INTERFACE ${FFMPEG_INCLUDE_DIRS})
        target_link_directories(corei_gameenginedevice_private INTERFACE ${FFMPEG_LIBRARY_DIRS})
        target_link_libraries(corei_gameenginedevice_private INTERFACE ${FFMPEG_LIBRARIES})
        target_compile_definitions(corei_gameenginedevice_public INTERFACE RTS_HAS_FFMPEG)
    endif()
endif()
