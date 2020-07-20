cmake_minimum_required(VERSION 3.5)
project(turbojpeg LANGUAGES CXX)


################################################################################
# Source

set(INCLUDE_FILES
    include/kvm_turbojpeg.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/kvm_turbojpeg.cpp
)


################################################################################
# Targets

# kvm_turbojpeg library

add_library(kvm_turbojpeg ${SOURCE_FILES})
target_include_directories(kvm_turbojpeg PUBLIC include)
target_link_libraries(kvm_turbojpeg
    PUBLIC
        kvm_core
)
install(TARGETS kvm_turbojpeg DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# kvm_turbojpeg_test application

add_executable(kvm_turbojpeg_test test/kvm_turbojpeg_test.cpp)
target_link_libraries(kvm_turbojpeg_test
    kvm_capture
)
install(TARGETS kvm_turbojpeg_test DESTINATION bin)
