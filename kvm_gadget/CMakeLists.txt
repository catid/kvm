cmake_minimum_required(VERSION 3.5)
project(kvm_gadget LANGUAGES CXX)


################################################################################
# Source Files

set(INCLUDE_FILES
    include/kvm_keyboard.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/kvm_keyboard.cpp
    src/Counter.h
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

# kvm_keyboard_test application

add_executable(kvm_keyboard_test test/kvm_keyboard_test.cpp)
target_link_libraries(kvm_keyboard_test
    kvm_gadget
)
install(TARGETS kvm_keyboard_test DESTINATION bin)
