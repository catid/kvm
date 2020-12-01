cmake_minimum_required(VERSION 3.5)
project(kvm_gadget LANGUAGES CXX)


################################################################################
# Source Files

set(INCLUDE_FILES
    include/kvm_gadget.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/kvm_gadget.cpp
)


################################################################################
# Targets

# kvm_gadget library

add_library(kvm_gadget SHARED ${SOURCE_FILES})
target_include_directories(kvm_gadget
    PUBLIC
        include
)
target_link_libraries(kvm_gadget
    PUBLIC
        kvm_core
)

install(TARGETS kvm_gadget DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# kvm_gadget_test application

add_executable(kvm_gadget_test test/kvm_gadget_test.cpp)
target_link_libraries(kvm_gadget_test
    kvm_gadget
)
install(TARGETS kvm_gadget_test DESTINATION bin)
