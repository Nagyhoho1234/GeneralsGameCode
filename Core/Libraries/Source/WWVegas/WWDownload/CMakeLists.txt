# This whole subsystem is a legacy Windows online-download/patch mechanism -
# Download.h and urlBuilder.cpp both depend on ftp.h's FTPClass directly, so
# there's no clean way to keep a portable subset without untangling the
# whole thing, which is Phase 7 (Networking) work, not Phase 1's. Gated
# behind WIN32 wholesale for now (native port plan Phase 1).
add_library(corei_wwdownload INTERFACE)

if(WIN32)
set(WWDOWNLOAD_SRC
    Download.cpp
    Download.h
    DownloadDebug.h
    downloaddefs.h
    FTP.cpp
    ftp.h
    ftpdefs.h
    registry.cpp
    Registry.h
    urlBuilder.cpp
    urlBuilder.h
)

target_sources(corei_wwdownload INTERFACE ${WWDOWNLOAD_SRC})
endif()

target_link_libraries(corei_wwdownload INTERFACE
    core_wwcommon
)
