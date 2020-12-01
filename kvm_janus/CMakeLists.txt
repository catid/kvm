cmake_minimum_required(VERSION 3.5)
project(kvm_janus LANGUAGES CXX)


################################################################################
# Dependencies

find_path(JANUS_PATH REQUIRED
    NAMES
        janus/plugins/plugin.h
)
message("JANUS_PATH: ${JANUS_PATH}")
set(JANUS_SUB_PATH "${JANUS_PATH}/janus")

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLIB glib-2.0)


################################################################################
# Source Files

set(SOURCE_FILES
    plugin/janus_plugin.cpp
)


################################################################################
# Targets

add_library(kvm_janus SHARED ${SOURCE_FILES})
target_include_directories(kvm_janus
    PUBLIC
        ${JANUS_PATH}
        ${JANUS_SUB_PATH}
        ${GLIB_INCLUDE_DIRS}
        include
)
target_link_libraries(kvm_janus
    PUBLIC
        ${Glib_LIBRARY}
        kvm_pipeline
)

install(TARGETS kvm_janus DESTINATION lib)
