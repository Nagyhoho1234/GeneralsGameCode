# Phase 5(a) Milestone 1 verification harness (native port plan). Standalone
# executable (not a doctest case, following native-port-spike/'s pattern) -
# drives the real DX8Wrapper::Init/Set_Render_Device/Create_Device/
# Begin_Scene/Clear/End_Scene chain against the PortableD3D8 GL backend and
# asserts the offscreen FBO was actually cleared to the requested color.
#
# NOT WIN32: Windows already has the real D3D8 backend and its own CI
# (win32/win-x64 presets, macos-native.yml's sibling). This only exercises
# the GL backend introduced by this milestone.
#
# Milestone 6 (native port plan Phase 5(a), Draft 26 Steps 2+4 combined):
# switched from a hand-picked file list to linking corei_ww3d2 directly.
# Reason: GL Create_Device/Release_Device (dx8wrapper_gl.cpp) now
# unconditionally call DX8Wrapper::Do_Onetime_Device_Dependent_Inits/
# _Shutdowns (moved to the always-compiled dx8wrapper_draw.cpp as part of
# ending Trap 1 completely - no POSIX-gated subset, per the plan's Draft 24
# no-gating principle) - this pulls in the mesh-render/asset-manager
# closure regardless of whether main.cpp ever exercises it, the same class
# of direct-object-list link requirement the plan's Draft 26 correction note
# describes. Hand-listing that whole closure here would just re-derive
# corei_ww3d2's own list; linking it directly is simpler and is what
# RenderW3DMeshTest already established. matrixmapper.cpp/
# anim_sound_link_stub.cpp are added for the same two link-closure reasons
# documented in RenderW3DMeshTest/CMakeLists.txt (decalsys.cpp's
# MatrixMapperClass::Set_Texture_Transform reference, animobj.cpp's two
# AnimatedSoundMgrClass statics) - corei_ww3d2 pulls in decalsys.cpp/
# animobj.cpp here exactly as it does there.
add_executable(RenderDeviceInitTest
    main.cpp
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp
    ${CMAKE_SOURCE_DIR}/Tests/RenderW3DMesh/anim_sound_link_stub.cpp
)

target_link_libraries(RenderDeviceInitTest PRIVATE
    core_wwvegas
    corei_ww3d2
    core_wwstub
    portabled3d8_gl
    corei_always
)

target_compile_features(RenderDeviceInitTest PRIVATE cxx_std_17)

# dx8wrapper.h transitively includes a handful of headers
# (lightenvironment.h, ...) that aren't unified into Core/ yet - still
# per-tree only (native port plan, mid-level rendering layer, out of scope
# for this milestone). GeneralsMD's own WW3D2 dir is added here for header
# resolution ONLY (no GeneralsMD .cpp is compiled into this target) - the
# same incidental include-path overlap that already lets
# z_gameenginedevice/g_gameenginedevice build Core/'s dx8wrapper.h today.
target_include_directories(RenderDeviceInitTest PRIVATE
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

add_test(NAME RenderDeviceInitTest COMMAND RenderDeviceInitTest)
