cmake_minimum_required(VERSION 3.5)
project(aes LANGUAGES CXX)


################################################################################
# Enable NEON

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wextra -std=c11 -mfpu=neon -ftree-vectorize")


################################################################################
# Source

set(INCLUDE_FILES
    include/aes.h
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/aes.S
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
