# RadarApi target
add_library(RadarApi SHARED RadarApi.cpp)

# Include directories specific to this target
target_include_directories(RadarApi PRIVATE
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/XDriver/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/Radar/
    ${CMAKE_SOURCE_DIR}/ImxWiring/
)

# Link libraries for this executable
target_link_libraries(RadarApi
    NoveldaDriverLibrary
    ImxWiringLibrary
    m
    pthread
    stdc++
    rt
    #m
    #pthread
    #stdc++
    #rt
    #json-c
    #xml2
)

# Include libgpiod only in case IMX8-based system
if (IMX8)
    message(STATUS "GPIOD interface included in RadarApi lib!")
    include_directories(${LIBGPIOD_INCLUDE_DIR})
    target_link_libraries(RadarApi libgpiod)
    add_dependencies(RadarApi libgpiod-external)
else()
    message(STATUS "GPIOD interface not included!")
endif()

# Output directory for the executable
set_target_properties(RadarApi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

