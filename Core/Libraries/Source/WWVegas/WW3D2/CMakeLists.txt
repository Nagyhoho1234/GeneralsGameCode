# Entirely D3D8-dependent (100+ D3D8 call sites across this library - see
# docs/native-port-plan.md). Gated behind WIN32 wholesale (native port plan
# Phase 1); the actual portable rendering backend is Phase 5.
if(WIN32)
set(WW3D2_SRC
    #aabtree.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #aabtree.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #aabtreebuilder.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #aabtreebuilder.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #agg_def.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #agg_def.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    animatedsoundmgr.cpp
    animatedsoundmgr.h
    #animobj.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #animobj.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #assetmgr.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #assetmgr.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #assetstatus.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #assetstatus.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #bitmaphandler.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 22 Step 5)
    #bitmaphandler.h
    bmp2d.cpp
    bmp2d.h
    #boxrobj.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #boxrobj.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #bwrender.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 7)
    #bwrender.h
    #camera.cpp
    #camera.h
    #classid.h # unused
    #collect.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #collect.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    colorspace.h
    #coltest.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #coltest.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    coltype.h
    #composite.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 7)
    #composite.h
    #dazzle.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #dazzle.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #ddsfile.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 22 Step 5)
    #ddsfile.h
    #decalmsh.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #decalmsh.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #decalsys.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #decalsys.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #distlod.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #distlod.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    dllist.h
    #dx8caps.cpp
    #dx8caps.h
    #dx8fvf.cpp
    #dx8fvf.h
    #dx8indexbuffer.cpp
    #dx8indexbuffer.h
    dx8list.h
    #dx8polygonrenderer.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #dx8polygonrenderer.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #dx8renderer.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #dx8renderer.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #dx8rendererdebugger.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #dx8rendererdebugger.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #dx8texman.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 22 Step 5)
    #dx8texman.h
    #dx8vertexbuffer.cpp
    #dx8vertexbuffer.h
    dx8webbrowser.cpp
    dx8webbrowser.h
    #dx8wrapper_d3d8.cpp
    #dx8wrapper_common.cpp
    #dx8wrapper.h
    #dynamesh.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 26 Step 3)
    #dynamesh.h # portable, see WW3D2_SRC_PORTABLE (Draft 26 Step 3)
    #font3d.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #font3d.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #formconv.cpp
    #formconv.h
    FramGrab.cpp
    framgrab.h
    #hanim.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hanim.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hanimmgr.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hanimmgr.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hcanim.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hcanim.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hlod.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hlod.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hmdldef.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hmdldef.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hmorphanim.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hmorphanim.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hrawanim.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #hrawanim.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #htree.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #htree.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #htreemgr.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #htreemgr.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    intersec.cpp
    intersec.h
    intersec.inl
    inttest.h
    layer.cpp
    layer.h
    light.cpp
    light.h
    #lightenvironment.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #lightenvironment.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    line3d.cpp
    line3d.h
    #linegrp.cpp
    #linegrp.h
    #mapper.cpp
    #mapper.h
    #matinfo.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #matinfo.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #matpass.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #matpass.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #matrixmapper.cpp
    #matrixmapper.h
    #mesh.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #mesh.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    meshbuild.cpp
    meshbuild.h
    meshdam.cpp
    meshdam.h
    #meshgeometry.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #meshgeometry.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #meshmatdesc.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #meshmatdesc.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #meshmdl.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #meshmdl.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #meshmdlio.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #metalmap.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #metalmap.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #missingtexture.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 22 Step 5)
    #missingtexture.h
    #motchan.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #motchan.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #nullrobj.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #nullrobj.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #part_buf.cpp
    #part_buf.h
    #part_emt.cpp
    #part_emt.h
    #part_ldr.cpp
    #part_ldr.h
    #pivot.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #pivot.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #pointgr.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 26 Step 3)
    #pointgr.h # portable, see WW3D2_SRC_PORTABLE (Draft 26 Step 3)
    polyinfo.cpp
    polyinfo.h
    #predlod.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #predlod.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    prim_anim.cpp
    prim_anim.h
    #projector.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 7)
    #projector.h
    #proto.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #proto.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    proxy.h
    rddesc.h
    #render2d.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #render2d.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #render2dsentence.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #render2dsentence.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    renderobjectrecycler.cpp
    renderobjectrecycler.h
    #rendobj.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #rendobj.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #rinfo.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #rinfo.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #ringobj.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #ringobj.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    robjlist.h
    #scene.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #scene.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    segline.cpp
    segline.h
    seglinerenderer.cpp
    seglinerenderer.h
    #shader.cpp
    #shader.h
    #shattersystem.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 26 Step 3)
    #shattersystem.h # portable, see WW3D2_SRC_PORTABLE (Draft 26 Step 3)
    shdlib.h
    #snappts.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #snapPts.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #sortingrenderer.cpp
    #sortingrenderer.h
    soundlibrarybridge.h
    soundrobj.cpp
    soundrobj.h
    #sphereobj.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #sphereobj.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #static_sort_list.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 2)
    #static_sort_list.h
    #statistics.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #statistics.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    streak.cpp
    streak.h
    streakRender.cpp
    streakRender.h
    #stripoptimizer.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #stripoptimizer.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #surfaceclass.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 22 Step 5)
    #surfaceclass.h
    texproject.cpp
    texproject.h
    #textdraw.cpp # unused
    textdraw.h
    #texture.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 22 Step 5)
    #texture.h
    #texturefilter.cpp
    #texturefilter.h
    #textureloader.cpp
    #textureloader.h
    #texturethumbnail.cpp
    #texturethumbnail.h
    #vertmaterial.cpp
    #vertmaterial.h
    #visrasterizer.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    #visrasterizer.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 5)
    w3d_dep.cpp
    w3d_dep.h
    w3d_file.h
    w3d_obsolete.h
    #w3d_util.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 7)
    #w3d_util.h
    w3derr.h
    #w3dexclusionlist.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #w3dexclusionlist.h # portable, see WW3D2_SRC_PORTABLE (Draft 24 Step 6)
    #ww3d.cpp # portable, see WW3D2_SRC_PORTABLE (Draft 26 Step 6)
    #ww3dformat.cpp
    #ww3dformat.h
    ww3dids.h
    ww3dtrig.h
)
endif() # WIN32

# Portable subset: built on every platform (native port plan Phase 5(a)
# Milestone 1 - device init + clear-to-color). dx8wrapper.h's WWINLINE
# methods delegate through PortableD3D8/ (non-Windows) or the real D3D8 SDK
# (Windows) without changing, so it stays out of the WIN32-gated list above.
# dx8wrapper_d3d8.cpp and dx8wrapper_gl.cpp are mutually exclusive
# platform-specific backends for the same declared DX8Wrapper methods;
# dx8wrapper_common.cpp is genuinely shared bookkeeping compiled into both.
set(WW3D2_SRC_PORTABLE
    assetmgr_common.cpp
    camera.cpp
    camera.h
    dx8caps.cpp
    dx8caps.h
    dx8fvf.cpp
    dx8fvf.h
    dx8indexbuffer.cpp
    dx8indexbuffer.h
    dx8vertexbuffer.cpp
    dx8vertexbuffer.h
    dx8wrapper.h
    mapper.cpp
    mapper.h
    shader.cpp
    shader.h
    sortingrenderer.cpp
    sortingrenderer.h
    vertmaterial.cpp
    vertmaterial.h
    dx8wrapper_common.cpp
    dx8wrapper_draw.cpp
    formconv.cpp
    formconv.h
    texture_common.cpp
    # ww3d.cpp joins the portable set (native port plan Phase 5(a)
    # Milestone 6, Draft 26 Step 6, findings 1+4): the ~18 DX8Wrapper
    # device-enumeration/registry/windowing methods it calls (Set_Any_
    # Render_Device, Registry_Save/Load_Render_Device, Set_Device_
    # Resolution, Set_Gamma, Toggle_Windowed, Set_Swap_Interval, ...) that
    # Milestone 1's Trap 1 left the GL backend without now all have real
    # or honestly-stubbed bodies in dx8wrapper_gl.cpp (Task 4 of this
    # milestone) - see that file for each method's disposition.
    ww3d.cpp
    ww3d.h
    ww3d_common.cpp
    ww3dformat.cpp
    ww3dformat.h

    # Texture pipeline (native port plan Phase 5(a) Milestone 4, Draft 22
    # Step 5) - portability sweep of all nine TUs the plan's scope
    # statement calls out. See docs/native-port-plan.md's Draft 22 section
    # for the per-file compile gates each of these needed (D3DX includes
    # removed/Windows-gated, ZeroMemory, timeGetTime->TIMEGETTIME,
    # D3DVOLUME_DESC/D3DLOCKED_BOX/D3DCUBEMAP_FACES added to PortableD3D8).
    bitmaphandler.cpp
    bitmaphandler.h
    ddsfile.cpp
    ddsfile.h
    dx8texman.cpp
    dx8texman.h
    missingtexture.cpp
    missingtexture.h
    surfaceclass.cpp
    surfaceclass.h
    texture.cpp
    texture.h
    texturefilter.cpp
    texturefilter.h
    textureloader.cpp
    textureloader.h
    texturethumbnail.cpp
    texturethumbnail.h

    # Milestone 5 (native port plan Phase 5(a), Draft 24 Step 2, finding
    # 5): pure sort-list code over RenderObjClass/RenderInfoClass, zero
    # Windows deps - moved here because WW3D::Add_To_Static_Sort_List/
    # Render_And_Clear_Static_Sort_Lists (ww3d_common.cpp) now reference
    # it, and mesh.cpp (soon-to-be-portable, Draft 24 Step 5) is this
    # milestone's real caller of both.
    static_sort_list.cpp
    static_sort_list.h

    # Portability wave A (native port plan Phase 5(a) Milestone 5, Draft 24
    # Step 5) - the mesh render path: dx8renderer.cpp's FVF/category
    # containers, the mesh/HLod/animation object hierarchy, and their
    # supporting math (aabtree, coltest, predlod, visrasterizer,
    # stripoptimizer). Grep-verified zero Windows/D3DX/GDI dependencies
    # across the whole closure before moving.
    aabtree.cpp
    aabtree.h
    aabtreebuilder.cpp
    aabtreebuilder.h
    animobj.cpp
    animobj.h
    boxrobj.cpp
    boxrobj.h
    coltest.cpp
    coltest.h
    decalmsh.cpp
    decalmsh.h
    decalsys.cpp
    decalsys.h
    dx8polygonrenderer.cpp
    dx8polygonrenderer.h
    dx8renderer.cpp
    dx8renderer.h
    dx8rendererdebugger.cpp
    dx8rendererdebugger.h
    hanim.cpp
    hanim.h
    hanimmgr.cpp
    hanimmgr.h
    hcanim.cpp
    hcanim.h
    hlod.cpp
    hlod.h
    hmdldef.cpp
    hmdldef.h
    hmorphanim.cpp
    hmorphanim.h
    hrawanim.cpp
    hrawanim.h
    htree.cpp
    htree.h
    htreemgr.cpp
    htreemgr.h
    lightenvironment.cpp
    lightenvironment.h
    matinfo.cpp
    matinfo.h
    matpass.cpp
    matpass.h
    mesh.cpp
    mesh.h
    meshgeometry.cpp
    meshgeometry.h
    meshmatdesc.cpp
    meshmatdesc.h
    meshmdl.cpp
    meshmdl.h
    motchan.cpp
    motchan.h
    nullrobj.cpp
    nullrobj.h
    pivot.cpp
    pivot.h
    predlod.cpp
    predlod.h
    proto.cpp
    proto.h
    rendobj.cpp
    rendobj.h
    rinfo.cpp
    rinfo.h
    statistics.cpp
    statistics.h
    stripoptimizer.cpp
    stripoptimizer.h
    visrasterizer.cpp
    visrasterizer.h
    meshmdlio.cpp

    # Portability wave B (native port plan Phase 5(a) Milestone 5, Draft 24
    # Step 6) - the asset-manager closure: WW3DAssetManager's constructor
    # directly references every one of these loader TUs (Box/Ring/Sphere/
    # Dazzle/Collection/DistLOD/Aggregate/Null all link, Mesh/HLod are the
    # ones this milestone actually exercises), plus MetalMapManagerClass and
    # FontCharsClass (its three genuinely-GDI member functions - Create_GDI_
    # Font/Free_GDI_Font/Store_GDI_Char - are individually #ifdef _WIN32
    # gated inside render2dsentence.cpp; the rest of that TU, and all of
    # render2d.cpp/scene.cpp/font3d.cpp/assetstatus.cpp/collect.cpp/
    # w3dexclusionlist.cpp/snappts.cpp, is plain SurfaceClass/RenderObjClass
    # code with zero Windows deps).
    agg_def.cpp
    agg_def.h
    assetmgr.cpp
    assetmgr.h
    assetstatus.cpp
    assetstatus.h
    collect.cpp
    collect.h
    dazzle.cpp
    dazzle.h
    distlod.cpp
    distlod.h
    font3d.cpp
    font3d.h
    metalmap.cpp
    metalmap.h
    render2d.cpp
    render2d.h
    render2dsentence.cpp
    render2dsentence.h
    ringobj.cpp
    ringobj.h
    scene.cpp
    scene.h
    snappts.cpp
    snapPts.h
    sphereobj.cpp
    sphereobj.h
    w3dexclusionlist.cpp
    w3dexclusionlist.h

    # Milestone 5 (native port plan Phase 5(a), Draft 24 Step 7): pure
    # W3D-struct<->engine-type conversion helpers, zero Windows deps -
    # moved here because sphereobj.cpp (portable since Step 6) calls
    # W3dUtilityClass::Convert_Vector/Convert_Shader, a link-closure gap
    # only surfaced by Tests/RenderW3DMesh actually linking an executable
    # (g_gameenginedevice/z_gameenginedevice never link this far, being
    # static libraries).
    w3d_util.cpp
    w3d_util.h

    # Milestone 5 (native port plan Phase 5(a), Draft 24 Step 7):
    # CompositeRenderObjClass, zero Windows deps - DistLODClass (portable
    # since Step 6) inherits from it, another link-closure gap only
    # surfaced by Tests/RenderW3DMesh actually linking an executable.
    composite.cpp
    composite.h
    bwrender.cpp
    bwrender.h
    projector.cpp
    projector.h

    # Milestone 6 (native port plan Phase 5(a), Draft 26 Step 3, Task 1):
    # pointgr.cpp/shattersystem.cpp/dynamesh.cpp - verified zero Windows/D3DX/
    # GDI deps except pointgr.cpp's single D3DXMatrixRotationZ call site,
    # replaced with a bit-identical portable matrix assignment under
    # #ifdef _WIN32/#else (see pointgr.cpp for the derivation). Moved first
    # (ahead of other Draft 26 steps) because Task 2's Do_Onetime move
    # references PointGroupClass::_Init/_Shutdown and ShatterSystem::
    # Init/Shutdown and would fail to link otherwise.
    pointgr.cpp
    pointgr.h
    shattersystem.cpp
    shattersystem.h
    dynamesh.cpp
    dynamesh.h
)

if(WIN32)
    list(APPEND WW3D2_SRC_PORTABLE dx8wrapper_d3d8.cpp)
else()
    list(APPEND WW3D2_SRC_PORTABLE
        dx8wrapper_gl.cpp
        PortableD3D8/d3d8.h
        PortableD3D8/d3d8types.h
        PortableD3D8/d3d8caps.h
        PortableD3D8/gl_core33.h
        PortableD3D8/gl_core33.cpp
        PortableD3D8/gl_fixed_function.h
        # AnimatedSoundMgrClass's engine-level stub (native port plan
        # Phase 5(a) Milestone 6, Draft 26 Step 6, finding 4) - mutually
        # exclusive with the real, WIN32-only animatedsoundmgr.cpp above
        # (that one needs WWAudio.h/mss.h, a catalogued Phase 6
        # deferral), the same backend-split pattern as dx8wrapper_gl.cpp/
        # dx8wrapper_d3d8.cpp just above.
        animatedsoundmgr_null.cpp
    )
endif()

list(APPEND WW3D2_SRC ${WW3D2_SRC_PORTABLE})

add_library(corei_ww3d2 INTERFACE)

target_sources(corei_ww3d2 INTERFACE ${WW3D2_SRC})

if (MSVC AND NOT IS_VS6_BUILD)
    target_link_libraries(corei_ww3d2 INTERFACE
        comsuppw
    )
endif()

target_link_libraries(corei_ww3d2 INTERFACE
    core_browserengine
    core_wwlib
    core_wwmath
)

if(NOT WIN32)
    target_link_libraries(corei_ww3d2 INTERFACE
        portabled3d8_gl
    )
endif()
