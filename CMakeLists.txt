cmake_minimum_required(VERSION 3.5)
project(kvm LANGUAGES CXX)


################################################################################
# Build Options

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

# Add debug and release modes
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

# Default to Release mode
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# In debug mode, add -DDEBUG
add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

# Extra warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fstack-protector")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


################################################################################
# Dependencies

# Install the userland drivers:
# git clone https://github.com/raspberrypi/userland.git
# cd userland
# ./buildme

find_path(MMAL_PATH REQUIRED
    NAMES
        mmal.h
    HINTS
        /opt/vc/include/interface/mmal/
)
message("MMAL_PATH: ${MMAL_PATH}")

set(USERLAND_INCLUDE_PATH "${MMAL_PATH}/../..")
message("USERLAND_INCLUDE_PATH: ${USERLAND_INCLUDE_PATH}")

set(VCSM_INCLUDE_PATH "${USERLAND_INCLUDE_PATH}/interface/vcsm")
message("VCSM_INCLUDE_PATH: ${VCSM_INCLUDE_PATH}")

find_library(MMAL_LIB REQUIRED
    NAMES
        mmal
    HINTS
        /opt/vc/lib/
)
message("MMAL_LIB: ${MMAL_LIB}")

find_library(VCSM_LIB REQUIRED
    NAMES
        vcsm
    HINTS
        /opt/vc/lib/
)
message("VCSM_LIB: ${VCSM_LIB}")

find_library(MMAL_UTIL_LIB REQUIRED
    NAMES
        mmal_util
    HINTS
        /opt/vc/lib/
)
message("MMAL_UTIL_LIB: ${MMAL_UTIL_LIB}")

find_library(MMAL_CORE_LIB REQUIRED
    NAMES
        mmal_core
    HINTS
        /opt/vc/lib/
)
message("MMAL_CORE_LIB: ${MMAL_CORE_LIB}")

find_library(VCOS_LIB REQUIRED
    NAMES
        vcos
    HINTS
        /opt/vc/lib/
)
message("VCOS_LIB: ${VCOS_LIB}")

find_library(BCM_HOST_LIB REQUIRED
    NAMES
        bcm_host
    HINTS
        /opt/vc/lib/
)
message("BCM_HOST_LIB: ${BCM_HOST_LIB}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


################################################################################
# Targets

add_subdirectory(kvm_core)
add_subdirectory(kvm_capture)
add_subdirectory(extern)
add_subdirectory(kvm_turbojpeg)
add_subdirectory(kvm_encode)
add_subdirectory(kvm_janus)
