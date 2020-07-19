cmake_minimum_required(VERSION 3.5)
project(v4l2 LANGUAGES CXX)


################################################################################
# Source

set(INCLUDE_FILES
    include/kvm_capture.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    include/kvm_capture.cpp
)


################################################################################
# Targets

# kvm_capture library

add_library(kvm_capture ${SOURCE_FILES})
target_include_directories(kvm_capture PUBLIC include)
target_link_libraries(kvm_capture
    PUBLIC
        core
)
install(TARGETS kvm_capture DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# kvm_capture_test application

add_executable(kvm_capture_test ${SOURCE_FILES})
target_link_libraries(kvm_capture_test
    kvm_core
)
install(TARGETS kvm_capture_test DESTINATION bin)
