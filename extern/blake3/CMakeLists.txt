cmake_minimum_required(VERSION 3.5)
project(blake3 LANGUAGES C)


################################################################################
# Enable NEON

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wextra -std=c11 -mcpu=native -mfpu=neon-vfpv3 -ftree-vectorize")


################################################################################
# Source

set(INCLUDE_FILES
    include/blake3.h
    include/blake3_impl.h
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/blake3.c
    src/blake3_dispatch.c
    src/blake3_neon.c
    src/blake3_portable.c
)


################################################################################
# Targets

# blake3 library

add_library(blake3 ${SOURCE_FILES})
target_include_directories(blake3
    PUBLIC
        include
)
target_link_libraries(blake3
    PUBLIC
)
install(TARGETS blake3 DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)
