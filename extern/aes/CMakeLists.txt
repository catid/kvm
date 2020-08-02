cmake_minimum_required(VERSION 3.5)
project(aes LANGUAGES CXX ASM)


################################################################################
# Enable NEON

SET(ASM_OPTIONS "-mcpu=native -mfpu=neon-vfpv3 -ftree-vectorize")
SET(CMAKE_ASM_FLAGS "${CFLAGS} ${ASM_OPTIONS}" )


################################################################################
# Source

set(INCLUDE_FILES
    include/aes128.h
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/aes128.S
)


################################################################################
# Targets

# aes library

add_library(aes ${SOURCE_FILES})
target_include_directories(aes
    PUBLIC
        include
)
target_link_libraries(aes
    PUBLIC
)
install(TARGETS aes DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# aes_test application

add_executable(aes_test test/aes_test.cpp)
target_link_libraries(aes_test
    aes
)
install(TARGETS aes_test DESTINATION bin)
