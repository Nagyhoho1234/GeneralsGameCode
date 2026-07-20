# Vendors glfw/glfw#2080 (never merged upstream): GLFW's NSGL backend
# unconditionally requests NSOpenGLPFAAccelerated with no way to opt out,
# which makes NSOpenGLPixelFormat return nil on GPU-less CI VMs (GitHub's
# hosted macOS runners lack a working accelerated GPU device - tracked at
# actions/runner-images#7085) even though Apple's software GL renderer is
# present there and does serve real GL 3.2+/4.1 core contexts (independently
# confirmed at libsdl-org/SDL#1180). Swapping the hard accelerated
# requirement for an explicit request for the generic/software renderer ID
# lets pixel format negotiation succeed on these VMs while still exercising
# Apple's real GL stack - exactly what docs/native-port-plan.md's Phase 3
# spike needs to validate macOS's core-profile strictness.
#
# Invoked as this project's GLFW FetchContent PATCH_COMMAND (see
# CMakeLists.txt): cmake -DGLFW_SRC_DIR=<path> -P patch_glfw_nsgl.cmake
# Harmless no-op on non-Apple checkouts - nsgl_context.m is only compiled
# when APPLE in GLFW's own CMakeLists.txt, this script just also happens to
# run there since FetchContent checks out the whole GLFW source tree
# regardless of target platform.

set(NSGL_FILE "${GLFW_SRC_DIR}/src/nsgl_context.m")

if(EXISTS "${NSGL_FILE}")
    file(READ "${NSGL_FILE}" CONTENTS)
    string(FIND "${CONTENTS}" "NSOpenGLPFARendererID" ALREADY_PATCHED)
    if(ALREADY_PATCHED EQUAL -1)
        string(REPLACE
            "    ADD_ATTRIB(NSOpenGLPFAAccelerated);"
            "    ADD_ATTRIB(NSOpenGLPFARendererID);\n    ADD_ATTRIB(kCGLRendererGenericFloatID);"
            CONTENTS "${CONTENTS}")
        file(WRITE "${NSGL_FILE}" "${CONTENTS}")
        message(STATUS "patch_glfw_nsgl: patched ${NSGL_FILE} per glfw/glfw#2080 (software-renderer fallback for GPU-less CI VMs)")
    else()
        message(STATUS "patch_glfw_nsgl: ${NSGL_FILE} already patched, skipping")
    endif()
else()
    message(STATUS "patch_glfw_nsgl: nsgl_context.m not found under ${GLFW_SRC_DIR} - nothing to patch (expected on non-Apple checkouts of some GLFW versions)")
endif()
