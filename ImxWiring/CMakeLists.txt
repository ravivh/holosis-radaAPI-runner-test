# ImxWiring library
add_library(ImxWiringLibrary
    wiringx.c
    i2c-dev.c
    soc/soc.c
    soc/nxp/imx6dqrm.c
    soc/nxp/imx6sdlrm.c
    platform/platform.c
    platform/solidrun/hummingboard_base_pro_dq.c
    platform/solidrun/hummingboard_base_pro_sdl.c
    platform/solidrun/hummingboard_gate_edge_dq.c
    platform/solidrun/hummingboard_gate_edge_sdl.c
)

# Include directories specific to this library
target_include_directories(ImxWiringLibrary PRIVATE
    ${CMAKE_SOURCE_DIR}/ImxWiring/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/Radar/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/hal/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/XDriver/
    ${CMAKE_SOURCE_DIR}/include/
)

# Output directory for the object files
set_target_properties(ImxWiringLibrary PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

