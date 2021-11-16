cmake_minimum_required(VERSION 3.5)
project(kvm_pipeline LANGUAGES CXX)

# Fix issue with mmal linkage: https://github.com/raspberrypi/userland/issues/178
SET( CMAKE_LINK_WHAT_YOU_USE TRUE )


################################################################################
# Source

set(INCLUDE_FILES
    include/kvm_encode.hpp
    include/kvm_pipeline.hpp
    include/kvm_video.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/kvm_encode.cpp
    src/kvm_pipeline.cpp
    src/kvm_video.cpp
)


################################################################################
# Targets

# kvm_pipeline library

add_library(kvm_pipeline SHARED ${SOURCE_FILES})
target_include_directories(kvm_pipeline
    PUBLIC
        include
        ${USERLAND_INCLUDE_PATH}
        ${VCSM_INCLUDE_PATH}
)
target_link_libraries(kvm_pipeline
    PUBLIC
        kvm_core
        kvm_jpeg
        kvm_capture
        ${MMAL_LIB}
        ${MMAL_CORE_LIB}
        ${MMAL_UTIL_LIB}
        ${MMAL_CLIENT_LIB}
        ${VCOS_LIB}
        ${BCM_HOST_LIB}
)
install(TARGETS kvm_pipeline DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# kvm_encode_test application

add_executable(kvm_encode_test test/kvm_encode_test.cpp)
target_link_libraries(kvm_encode_test
    kvm_pipeline
)
install(TARGETS kvm_encode_test DESTINATION bin)

# kvm_pipeline_test application

add_executable(kvm_pipeline_test test/kvm_pipeline_test.cpp)
target_link_libraries(kvm_pipeline_test
    kvm_pipeline
)
install(TARGETS kvm_pipeline_test DESTINATION bin)
