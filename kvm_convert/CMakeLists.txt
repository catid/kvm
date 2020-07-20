cmake_minimum_required(VERSION 3.5)
project(kvm_convert LANGUAGES CXX)


################################################################################
# Source

set(INCLUDE_FILES
    include/kvm_convert.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/kvm_convert.cpp
)


################################################################################
# Targets

# kvm_convert library

add_library(kvm_convert ${SOURCE_FILES})
target_include_directories(kvm_convert PUBLIC include)
target_link_libraries(kvm_convert
    PUBLIC
        kvm_core
)
install(TARGETS kvm_convert DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# kvm_convert_test application

add_executable(kvm_convert_test test/kvm_convert_test.cpp)
target_link_libraries(kvm_convert_test
    kvm_turbojpeg
    kvm_capture
    kvm_convert
)
install(TARGETS kvm_convert_test DESTINATION bin)
