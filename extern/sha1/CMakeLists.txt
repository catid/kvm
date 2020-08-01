cmake_minimum_required(VERSION 3.5)
project(sha1 LANGUAGES CXX ASM)


################################################################################
# Enable NEON

SET(ASM_OPTIONS "-mcpu=native -mfpu=neon-vfpv3 -ftree-vectorize")
SET(CMAKE_ASM_FLAGS "${CFLAGS} ${ASM_OPTIONS}")


################################################################################
# Source

set(INCLUDE_FILES
    include/sha1.h
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/sha1.S
    src/sha1_glue.cpp
)


################################################################################
# Targets

# sha1 library

add_library(sha1 ${SOURCE_FILES})
target_include_directories(sha1
    PUBLIC
        include
)
target_link_libraries(sha1
    PUBLIC
)
install(TARGETS sha1 DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# sha1_test application

add_executable(sha1_test test/sha1_test.cpp)
target_link_libraries(sha1_test
    sha1
)
install(TARGETS sha1_test DESTINATION bin)
