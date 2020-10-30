cmake_minimum_required(VERSION 3.5)
project(kvm_encode LANGUAGES CXX)


################################################################################
# Source

set(INCLUDE_FILES
    include/kvm_encode.hpp
    include/kvm_pipeline.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/kvm_encode.cpp
    src/kvm_pipeline.cpp
)


################################################################################
# Targets

# kvm_encode library

add_library(kvm_encode ${SOURCE_FILES})
target_include_directories(kvm_encode
    PUBLIC
        include
        ${USERLAND_INCLUDE_PATH}
        ${VCSM_INCLUDE_PATH}
)
target_link_libraries(kvm_encode
    PUBLIC
        kvm_core
        kvm_turbojpeg
        kvm_capture
        ${MMAL_LIB}
        ${MMAL_CORE_LIB}
        ${MMAL_UTIL_LIB}
        ${VCOS_LIB}
        ${BCM_HOST_LIB}
)
install(TARGETS kvm_encode DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# kvm_encode_test application

add_executable(kvm_encode_test test/kvm_encode_test.cpp)
target_link_libraries(kvm_encode_test
    kvm_encode
)
install(TARGETS kvm_encode_test DESTINATION bin)

# kvm_pipeline_test application

add_executable(kvm_pipeline_test test/kvm_pipeline_test.cpp)
target_link_libraries(kvm_pipeline_test
    kvm_encode
)
install(TARGETS kvm_pipeline_test DESTINATION bin)
