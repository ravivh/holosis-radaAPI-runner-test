cmake_minimum_required(VERSION 3.12)

# Project Name
project(RadarApiProject LANGUAGES C CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the ExternalProject module
include(ExternalProject)

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
	${CMAKE_SOURCE_DIR}/NoveldaDriver/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/Radar/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/XDriver/
    ${CMAKE_SOURCE_DIR}/NoveldaDriver/hal/
    ${CMAKE_SOURCE_DIR}/ImxWiring/
)

# Check for IMX8/IMX6 flags
if (NOT IMX6 AND NOT IMX8)
    message(STATUS "Neither IMX6 nor IMX8 is set! Exiting...")
    return()
else ()
	if (IMX6 AND IMX8)
    	message(STATUS "Both IMX6 and IMX8 are set! Exiting...")
		return()
	else ()
    	if (IMX8)
			message(STATUS "IMX8 selected")
		endif ()
		if (IMX6)
			message(STATUS "IMX6 selected")
		endif ()
	endif ()
endif ()

# Differentiate SoCs
if (IMX6)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMX6")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIMX6")
elseif (IMX8)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMX8")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIMX8")
	if (GPIO_SYSFS)
		message(STATUS "GPIO_SYSFS flag selected!")
	elseif (GPIOD)
		message(STATUS "GPIOD flag selected!")
	endif()
endif()

# Include libgpiod only in case IMX8-based system
if (IMX8)
	message(STATUS "GPIOD interface included in top-level CMakeLists.txt!")

	# Configure external project - libgpiod
	set(LIBGPIO_VERSION 1.6.4)
	set(LIBGPIO_TARBALL_URL "https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/libgpiod-${LIBGPIO_VERSION}.tar.gz")
	set(LIBGPIO_BASE_DIR "${CMAKE_BINARY_DIR}/libgpiod-external")
	set(LIBGPIO_INSTALL_DIR "${LIBGPIO_BASE_DIR}/libgpiod-install")
	set(LIBGPIO_LIB_DIR "${LIBGPIO_INSTALL_DIR}/lib")
	set(LIBGPIO_INCLUDE_DIR "${LIBGPIO_INSTALL_DIR}/include")
	set(LIBGPIO_NAME "libgpiod.so.2.2.2")

	# Download, configure, build and install libgpiod
	ExternalProject_Add( libgpiod-external
		PREFIX ${CMAKE_BINARY_DIR}/libgpiod
		URL ${LIBGPIO_TARBALL_URL}
		CONFIGURE_COMMAND ${CMAKE_BINARY_DIR}/libgpiod/src/libgpiod-external/configure
			--prefix=${LIBGPIO_INSTALL_DIR}
			--enable-tools
			--host=aarch64-linux-gnu
			CC=aarch64-linux-gnu-gcc
			CXX=aarch64-linux-gnu-g++
		BUILD_COMMAND make
		INSTALL_COMMAND make install
	)

	# Prepare dependency for RadarApi lib
	add_library(libgpiod SHARED IMPORTED)
	set_target_properties(libgpiod PROPERTIES IMPORTED_LOCATION ${LIBGPIO_LIB_DIR}/${LIBGPIO_NAME})

	# Add include and library directories after the library is installed
	include_directories(${LIBGPIO_INCLUDE_DIR})
	link_directories(${LIBGPIO_LIB_DIR})
else()
	message(STATUS "GPIOD interface not included!")
endif()

# Add subdirectories for each component
add_subdirectory(NoveldaDriver)
add_subdirectory(ImxWiring)
add_subdirectory(RadarApi)
