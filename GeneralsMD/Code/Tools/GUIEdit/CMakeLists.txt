add_executable(z_guiedit WIN32)
set_target_properties(z_guiedit PROPERTIES OUTPUT_NAME "guiedit${RTS_BUILD_OUTPUT_SUFFIX}")

target_link_libraries(z_guiedit PRIVATE
    corei_guiedit
    core_debug
    core_profile_legacy
    z_gameengine
    z_gameenginedevice
    z_wwvegas
    zi_always
)

if(WIN32 OR "${CMAKE_SYSTEM}" MATCHES "Windows")
    target_link_options(z_guiedit PRIVATE /NODEFAULTLIB:libci.lib /NODEFAULTLIB:libc.lib)
endif()
