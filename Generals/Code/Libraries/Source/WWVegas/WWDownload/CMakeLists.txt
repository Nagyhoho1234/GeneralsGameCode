# corei_wwdownload contributes zero sources on non-Windows (native port plan
# Phase 1 - the whole WWDownload subsystem is gated WIN32-only) - a STATIC
# library needs at least one source file even when everything it links is
# empty, so use INTERFACE there instead.
if(WIN32)
    add_library(g_wwdownload STATIC)
    set_target_properties(g_wwdownload PROPERTIES OUTPUT_NAME wwdownload)
    target_link_libraries(g_wwdownload PRIVATE
        corei_wwdownload
        gi_always
        gi_gameengine_include
    )
else()
    add_library(g_wwdownload INTERFACE)
    target_link_libraries(g_wwdownload INTERFACE
        corei_wwdownload
        gi_always
        gi_gameengine_include
    )
endif()
