cmake_minimum_required(VERSION 3.5)
project(kvm_jpeg LANGUAGES CXX)


################################################################################
# Dependencies

# sudo apt install libturbojpeg0-dev

find_library(TJ_LIB REQUIRED
    NAMES
        turbojpeg
)
message("TJ_LIB: ${TJ_LIB}")


################################################################################
# Source

set(INCLUDE_FILES
    include/kvm_jpeg.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/kvm_jpeg.cpp
)


################################################################################
# Targets

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message("OpenMP found")
else()
    message("OpenMP NOT found")
endif()

# kvm_jpeg library

add_library(kvm_jpeg ${SOURCE_FILES})
target_include_directories(kvm_jpeg PUBLIC include)
target_link_libraries(kvm_jpeg
    PUBLIC
        kvm_core
        ${TJ_LIB}
        OpenMP::OpenMP_CXX
)
install(TARGETS kvm_jpeg DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# kvm_jpeg_test application

add_executable(kvm_jpeg_test test/kvm_jpeg_test.cpp)
target_link_libraries(kvm_jpeg_test
    kvm_jpeg
    kvm_capture
)
install(TARGETS kvm_jpeg_test DESTINATION bin)
