cmake_minimum_required(VERSION 3.5)
project(brcmjpeg LANGUAGES C)


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
