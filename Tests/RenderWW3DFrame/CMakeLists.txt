# Phase 5(a) Milestone 6 (native port plan, Draft 26) - the milestone's exit
# criterion: the first harness in this port that drives the engine through
# its own WW3D:: entry points (WW3D::Init/Set_Render_Device/Begin_Render/
# Render/End_Render/Set_Device_Resolution/Shutdown), not DX8Wrapper directly.
# See main.cpp for the full six-check list.
#
# NOT WIN32: same reasoning as the other five harnesses - Windows already has
# the real D3D8 backend and its own CI; this only exercises the GL backend.
#
# Sibling to Tests/RenderW3DMesh - links corei_ww3d2 the same way (Milestone
# 5 made WW3D2_SRC_PORTABLE the complete portable closure; this harness needs
# ww3d.cpp itself too, portable since this same milestone's Task 4).
add_executable(RenderWW3DFrameTest
    main.cpp
    # See Tests/RenderW3DMesh/CMakeLists.txt's matching comment: matrixmapper.cpp
    # is per-tree (Generals/GeneralsMD diverge), decalsys.cpp (portable) references
    # MatrixMapperClass::Set_Texture_Transform - the symbol just needs to link,
    # decal generation is not exercised by this harness either.
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/matrixmapper.cpp
)

target_link_libraries(RenderWW3DFrameTest PRIVATE
    core_wwvegas
    corei_ww3d2
    core_wwstub
    portabled3d8_gl
    corei_always
)

target_compile_features(RenderWW3DFrameTest PRIVATE cxx_std_17)

# See RenderDeviceInit/CMakeLists.txt's matching comment - GeneralsMD's
# WW3D2 dir is added for header resolution only, no GeneralsMD .cpp is
# compiled into this target.
target_include_directories(RenderWW3DFrameTest PRIVATE
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

add_test(NAME RenderWW3DFrameTest COMMAND RenderWW3DFrameTest)
