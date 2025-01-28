# NoveldaDriver library
add_library(NoveldaDriverLibrary
    Utilities.cpp
    RemoteConnection.cpp
    NoveldaRadar.cpp
    hal/radar_hal.c
    XDriver/x4driver.c
    Radar/taskRadar.cpp
)

# Include directories specific to this library
target_include_directories(NoveldaDriverLibrary PRIVATE
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/Radar/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/XDriver/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/hal/
    ${CMAKE_SOURCE_DIR}/include/
)

# Output directory for the object files
set_target_properties(NoveldaDriverLibrary PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

