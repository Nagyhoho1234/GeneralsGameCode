# corei_wwdownload contributes zero sources on non-Windows (native port plan
# Phase 1 - the whole WWDownload subsystem is gated WIN32-only) - a STATIC
# library needs at least one source file even when everything it links is
# empty, so use INTERFACE there instead.
if(WIN32)
    add_library(z_wwdownload STATIC)
    set_target_properties(z_wwdownload PROPERTIES OUTPUT_NAME wwdownload)
    target_link_libraries(z_wwdownload PRIVATE
        corei_wwdownload
        zi_always
        zi_gameengine_include
    )
else()
    add_library(z_wwdownload INTERFACE)
    target_link_libraries(z_wwdownload INTERFACE
        corei_wwdownload
        zi_always
        zi_gameengine_include
    )
endif()
