# Entirely D3D8-dependent (100+ D3D8 call sites across this library - see
# docs/native-port-plan.md). Gated behind WIN32 wholesale (native port plan
# Phase 1); the actual portable rendering backend is Phase 5.
if(WIN32)
set(WW3D2_SRC
    aabtree.cpp
    aabtree.h
    #aabtreebuilder.cpp
    #aabtreebuilder.h
    agg_def.cpp
    agg_def.h
    animatedsoundmgr.cpp
    animatedsoundmgr.h
    animobj.cpp
    animobj.h
    #assetmgr.cpp
    assetmgr.h
    assetstatus.cpp
    assetstatus.h
    bitmaphandler.cpp
    bitmaphandler.h
    bmp2d.cpp
    bmp2d.h
    #boxrobj.cpp
    boxrobj.h
    bwrender.cpp
    bwrender.h
    #camera.cpp
    #camera.h
    #classid.h # unused
    collect.cpp
    collect.h
    colorspace.h
    coltest.cpp
    coltest.h
    coltype.h
    composite.cpp
    composite.h
    dazzle.cpp
    dazzle.h
    ddsfile.cpp
    ddsfile.h
    #decalmsh.cpp
    decalmsh.h
    decalsys.cpp
    decalsys.h
    distlod.cpp
    distlod.h
    dllist.h
    #dx8caps.cpp
    #dx8caps.h
    #dx8fvf.cpp
    #dx8fvf.h
    dx8indexbuffer.cpp
    dx8indexbuffer.h
    dx8list.h
    dx8polygonrenderer.cpp
    dx8polygonrenderer.h
    dx8renderer.cpp
    dx8renderer.h
    dx8rendererdebugger.cpp
    dx8rendererdebugger.h
    dx8texman.cpp
    dx8texman.h
    dx8vertexbuffer.cpp
    dx8vertexbuffer.h
    dx8webbrowser.cpp
    dx8webbrowser.h
    #dx8wrapper_d3d8.cpp
    #dx8wrapper_common.cpp
    #dx8wrapper.h
    dynamesh.cpp
    dynamesh.h
    font3d.cpp
    font3d.h
    #formconv.cpp
    #formconv.h
    FramGrab.cpp
    framgrab.h
    hanim.cpp
    hanim.h
    #hanimmgr.cpp
    hanimmgr.h
    hcanim.cpp
    hcanim.h
    #hlod.cpp
    hlod.h
    hmdldef.cpp
    hmdldef.h
    #hmorphanim.cpp
    hmorphanim.h
    #hrawanim.cpp
    hrawanim.h
    htree.cpp
    htree.h
    #htreemgr.cpp
    #htreemgr.h
    intersec.cpp
    intersec.h
    intersec.inl
    inttest.h
    layer.cpp
    layer.h
    light.cpp
    light.h
    #lightenvironment.cpp
    #lightenvironment.h
    line3d.cpp
    line3d.h
    #linegrp.cpp
    #linegrp.h
    mapper.cpp
    mapper.h
    matinfo.cpp
    matinfo.h
    matpass.cpp
    matpass.h
    #matrixmapper.cpp
    #matrixmapper.h
    #mesh.cpp
    #mesh.h
    meshbuild.cpp
    #meshbuild.h
    meshdam.cpp
    meshdam.h
    #meshgeometry.cpp
    #meshgeometry.h
    #meshmatdesc.cpp
    meshmatdesc.h
    #meshmdl.cpp
    #meshmdl.h
    #meshmdlio.cpp
    metalmap.cpp
    metalmap.h
    missingtexture.cpp
    missingtexture.h
    motchan.cpp
    motchan.h
    #nullrobj.cpp
    nullrobj.h
    #part_buf.cpp
    #part_buf.h
    #part_emt.cpp
    #part_emt.h
    #part_ldr.cpp
    #part_ldr.h
    pivot.cpp
    pivot.h
    pointgr.cpp
    pointgr.h
    polyinfo.cpp
    polyinfo.h
    predlod.cpp
    predlod.h
    prim_anim.cpp
    prim_anim.h
    projector.cpp
    projector.h
    proto.cpp
    proto.h
    proxy.h
    rddesc.h
    render2d.cpp
    render2d.h
    render2dsentence.cpp
    render2dsentence.h
    renderobjectrecycler.cpp
    renderobjectrecycler.h
    rendobj.cpp
    rendobj.h
    #rinfo.cpp
    rinfo.h
    ringobj.cpp
    ringobj.h
    robjlist.h
    scene.cpp
    scene.h
    segline.cpp
    segline.h
    seglinerenderer.cpp
    seglinerenderer.h
    shader.cpp
    shader.h
    shattersystem.cpp
    shattersystem.h
    shdlib.h
    snappts.cpp
    snapPts.h
    sortingrenderer.cpp
    sortingrenderer.h
    soundlibrarybridge.h
    soundrobj.cpp
    soundrobj.h
    sphereobj.cpp
    sphereobj.h
    static_sort_list.cpp
    static_sort_list.h
    statistics.cpp
    statistics.h
    streak.cpp
    streak.h
    streakRender.cpp
    streakRender.h
    stripoptimizer.cpp
    stripoptimizer.h
    surfaceclass.cpp
    surfaceclass.h
    texproject.cpp
    texproject.h
    #textdraw.cpp # unused
    textdraw.h
    texture.cpp
    texture.h
    texturefilter.cpp
    texturefilter.h
    textureloader.cpp
    textureloader.h
    texturethumbnail.cpp
    texturethumbnail.h
    vertmaterial.cpp
    vertmaterial.h
    visrasterizer.cpp
    visrasterizer.h
    w3d_dep.cpp
    w3d_dep.h
    #w3d_file.h
    w3d_obsolete.h
    w3d_util.cpp
    w3d_util.h
    w3derr.h
    w3dexclusionlist.cpp
    w3dexclusionlist.h
    #ww3d.cpp
    #ww3d.h
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
    camera.cpp
    camera.h
    dx8caps.cpp
    dx8caps.h
    dx8fvf.cpp
    dx8fvf.h
    dx8wrapper.h
    dx8wrapper_common.cpp
    formconv.cpp
    formconv.h
    ww3d.cpp
    ww3d.h
    ww3d_common.cpp
    ww3dformat.cpp
    ww3dformat.h
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
