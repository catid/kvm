cmake_minimum_required(VERSION 3.5)
project(brcmjpeg LANGUAGES C)


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

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


################################################################################
# Source

set(INCLUDE_FILES
    include/brcmjpeg.h
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/brcmjpeg.c
)


################################################################################
# Targets

# brcmjpeg library

add_library(brcmjpeg ${SOURCE_FILES})
target_include_directories(brcmjpeg
    PUBLIC
        include
        ${MMAL_PATH}
        ${USERLAND_INCLUDE_PATH}
        ${VCSM_INCLUDE_PATH}
)
target_link_libraries(brcmjpeg
    PUBLIC
        ${MMAL_LIB}
        ${VCSM_LIB}
        ${MMAL_UTIL_LIB}
        ${MMAL_CORE_LIB}
        ${VCOS_LIB}
        Threads::Threads
)
install(TARGETS brcmjpeg DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# brcmjpeg_test application

add_executable(brcmjpeg_test test/brcmjpeg_test.c)
target_link_libraries(brcmjpeg_test
    brcmjpeg
)
install(TARGETS brcmjpeg_test DESTINATION bin)
