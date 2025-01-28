cmake_minimum_required(VERSION 3.12)

# Project Name
project(RadarApiProject LANGUAGES C CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cross-compilation setup
if(DEFINED ENV{CROSS_COMPILE})
    set(CMAKE_C_COMPILER "$ENV{CROSS_COMPILE}gcc")
    set(CMAKE_CXX_COMPILER "$ENV{CROSS_COMPILE}g++")
else()
    message(FATAL_ERROR "CROSS_COMPILE environment variable is not set!")
endif()

# Flags for all targets
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

# Include directories shared across all folders
include_directories(
    ${CMAKE_SOURCE_DIR}/include/
    /usr/include/libxml2/
)

# Add subdirectories for each component
add_subdirectory(NoveldaDriver)
add_subdirectory(ImxWiring)
add_subdirectory(RadarApi)

