cmake_minimum_required(VERSION 3.5)
project(kvm_crypto LANGUAGES CXX)


################################################################################
# Source

set(INCLUDE_FILES
    include/kvm_crypto.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/kvm_crypto.cpp
)


################################################################################
# Targets

# kvm_crypto library

add_library(kvm_crypto ${SOURCE_FILES})
target_include_directories(kvm_crypto PUBLIC include)
target_link_libraries(kvm_crypto
    PUBLIC
        kvm_core
        blake3
        aes
)
install(TARGETS kvm_crypto DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)

# kvm_crypto_test application

add_executable(kvm_crypto_test test/kvm_crypto_test.cpp)
target_link_libraries(kvm_crypto_test
    kvm_crypto
)
install(TARGETS kvm_crypto_test DESTINATION bin)
