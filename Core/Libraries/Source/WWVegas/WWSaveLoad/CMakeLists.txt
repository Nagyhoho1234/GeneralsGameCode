set(WWSAVELOAD_SRC
    definition.cpp
    definition.h
    definitionclassids.h
    definitionfactory.cpp
    definitionfactory.h
    definitionfactorymgr.cpp
    definitionfactorymgr.h
    definitionmgr.cpp
    definitionmgr.h
    editable.h
    parameter.cpp
    parameter.h
    parameterlist.h
    parametertypes.h
    persist.h
    persistfactory.cpp
    persistfactory.h
    pointerremap.cpp
    pointerremap.h
    postloadable.h
    saveload.cpp
    saveload.h
    saveloadids.h
    saveloadstatus.cpp
    saveloadstatus.h
    saveloadstatus.h
    saveloadsubsystem.cpp
    saveloadsubsystem.h
    simpledefinitionfactory.h
    simpleparameter.h
    twiddler.cpp
    twiddler.h
    wwsaveload.cpp
    wwsaveload.h
)

add_library(core_wwsaveload STATIC)
set_target_properties(core_wwsaveload PROPERTIES OUTPUT_NAME wwsaveload)

target_sources(core_wwsaveload PRIVATE ${WWSAVELOAD_SRC})

target_link_libraries(core_wwsaveload PRIVATE
    core_wwcommon
    corei_always
)

# definition.cpp uses WWLib's ChunkSaveClass/ChunkLoadClass (chunkio.cpp) but
# never declared the dependency - harmless as long as every consumer also
# happened to link core_wwlib in an order the linker's single left-to-right
# archive pass tolerated, until the Phase 5(a) Milestone 1 RenderDeviceInit
# test harness (Tests/RenderDeviceInit/) linked a small enough target that
# it didn't. PUBLIC so it propagates to whoever links core_wwsaveload.
target_link_libraries(core_wwsaveload PUBLIC
    core_wwlib
)
