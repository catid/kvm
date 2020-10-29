cmake_minimum_required(VERSION 3.5)
project(kvm_core LANGUAGES CXX)


################################################################################
# Dependencies

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


################################################################################
# Source

set(INCLUDE_FILES
    include/kvm_core.hpp
    include/kvm_frame.hpp
    include/kvm_logger.hpp
    include/kvm_serializer.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/kvm_core.cpp
    src/kvm_frame.cpp
    src/kvm_logger.cpp
    src/kvm_serializer.cpp
)


################################################################################
# Targets

# kvm_core library

add_library(kvm_core ${SOURCE_FILES})
target_include_directories(kvm_core
    PUBLIC
        include
)
target_link_libraries(kvm_core
    PUBLIC
        Threads::Threads
)
install(TARGETS kvm_core DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# kvm_core_test application

add_executable(kvm_core_test test/kvm_core_test.cpp)
target_link_libraries(kvm_core_test
    kvm_core
)
install(TARGETS kvm_core_test DESTINATION bin)
