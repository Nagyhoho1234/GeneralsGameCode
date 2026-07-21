# Interface libraries to set common defines and includes, avoid duplication later
add_library(core_wwcommon INTERFACE)

target_compile_definitions(core_wwcommon INTERFACE
    #NOMINMAX
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(core_wwcommon INTERFACE
    d3d8lib
    milesstub
    stlport
)

target_include_directories(core_wwcommon INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    WW3D2
    WWAudio
    WWDebug
    # WWDownload # is not added because headers are included as "WWDownload/file.h"
    WWLib
    WWMath
    WWSaveLoad
    # Stand-in <d3d8.h>/<d3d8types.h>/<d3d8caps.h> for non-Windows (native
    # port plan Phase 5(a)) - real D3D8 SDK headers take precedence on
    # Windows via d3d8lib, so this is additive there, not a conflict.
    $<$<NOT:$<BOOL:${WIN32}>>:${CMAKE_CURRENT_SOURCE_DIR}/WW3D2/PortableD3D8>
)

add_subdirectory(WW3D2)
add_subdirectory(WWAudio)
add_subdirectory(WWDebug)
add_subdirectory(WWDownload)
add_subdirectory(WWLib)
add_subdirectory(WWMath)
add_subdirectory(WWSaveLoad)
add_subdirectory(WWStub)

# Helpful interface to bundle the ww modules together.
add_library(core_wwvegas INTERFACE)

target_include_directories(core_wwvegas INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    WW3D2
    WWAudio
    WWDebug
    # WWDownload # is not added because headers are included as "WWDownload/file.h"
    WWLib
    WWMath
    WWSaveLoad
    $<$<NOT:$<BOOL:${WIN32}>>:${CMAKE_CURRENT_SOURCE_DIR}/WW3D2/PortableD3D8>
)

target_link_libraries(core_wwvegas INTERFACE
    # core_ww3d2
    # core_wwaudio
    core_wwdebug
    # core_wwdownload
    core_wwlib
    core_wwmath
    core_wwsaveload
)
