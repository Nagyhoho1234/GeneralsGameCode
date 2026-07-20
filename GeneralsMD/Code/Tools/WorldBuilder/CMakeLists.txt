set(WORLDBUILDER_SRC
    "include/CUndoable.h"
    "include/CameraOptions.h"
    "include/DrawObject.h"
    "include/EditAction.h"
    "include/EditCondition.h"
    "include/EditParameter.h"
    "include/ExportScriptsOptions.h"
    "include/LayersList.h"
    "include/MainFrm.h"
    "include/RulerOptions.h"
    "include/RulerTool.h"
    "include/ScriptDialog.h"
    "include/TeamObjectProperties.h"
    "include/WHeightMapEdit.h"
    "include/WorldBuilder.h"
    "include/mapobjectprops.h"
    "include/wbview.h"
    "include/wbview3d.h"
    "src/CUndoable.cpp"
    "src/CameraOptions.cpp"
    "src/DrawObject.cpp"
    "src/EditAction.cpp"
    "src/EditCondition.cpp"
    "src/EditParameter.cpp"
    "src/ExportScriptsOptions.cpp"
    "src/LayersList.cpp"
    "src/MainFrm.cpp"
    "src/RulerOptions.cpp"
    "src/RulerTool.cpp"
    "src/ScriptDialog.cpp"
    "src/TeamObjectProperties.cpp"
    "src/TileTool.cpp"
    "src/WHeightMapEdit.cpp"
    "src/WorldBuilder.cpp"
    "src/WorldBuilderDoc.cpp"
    "src/mapobjectprops.cpp"
    "src/playerlistdlg.cpp"
    "src/teamsdialog.cpp"
    "src/wbview.cpp"
    "src/wbview3d.cpp"
)

add_executable(z_worldbuilder WIN32)

target_sources(z_worldbuilder PRIVATE ${WORLDBUILDER_SRC})

target_include_directories(z_worldbuilder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    include
    res
)

target_precompile_headers(z_worldbuilder PRIVATE
    [["Utility/CppMacros.h"]] # Must be first, to be removed when abandoning VC6
    [["always.h"]]
    [["Common/STLTypedefs.h"]]
    [["StdAfx.h"]]
    [["WWCommon.h"]]
)

target_link_libraries(z_worldbuilder PRIVATE
    corei_worldbuilder
    core_debug
    core_profile_legacy
    z_gameengine
    z_gameenginedevice
    zi_always
)

if(WIN32 OR "${CMAKE_SYSTEM}" MATCHES "Windows")
    target_link_options(z_worldbuilder PRIVATE /NODEFAULTLIB:libci.lib /NODEFAULTLIB:libc.lib)
    target_compile_definitions(z_worldbuilder PRIVATE _AFXDLL)
    target_sources(z_worldbuilder PRIVATE res/WorldBuilder.rc)
    set_target_properties(z_worldbuilder PROPERTIES OUTPUT_NAME "WorldBuilderZH${RTS_BUILD_OUTPUT_SUFFIX}")
else()
    set_target_properties(z_worldbuilder PROPERTIES OUTPUT_NAME "worldbuilderzh${RTS_BUILD_OUTPUT_SUFFIX}")
endif()
