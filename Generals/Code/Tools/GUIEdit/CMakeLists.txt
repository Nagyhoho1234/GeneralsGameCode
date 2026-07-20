add_executable(g_guiedit WIN32)
set_target_properties(g_guiedit PROPERTIES OUTPUT_NAME "guiedit${RTS_BUILD_OUTPUT_SUFFIX}")

target_link_libraries(g_guiedit PRIVATE
    corei_guiedit
    g_gameengine
    g_gameenginedevice
    g_wwvegas
    gi_always
)

if(WIN32 OR "${CMAKE_SYSTEM}" MATCHES "Windows")
    target_link_options(g_guiedit PRIVATE /NODEFAULTLIB:libci.lib /NODEFAULTLIB:libc.lib)
endif()
