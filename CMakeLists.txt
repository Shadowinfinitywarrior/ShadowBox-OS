cmake_minimum_required(VERSION 3.10)

# Project definition
project(OS C)

# Use the existing Makefile for actual compilation.
# This CMake file provides a thin wrapper so that developers can run
# `cmake .` and then `make` (or `make run`) without modifying the original build system.

# Define a custom target that invokes the top‑level Makefile.
add_custom_target(build_all
    COMMAND ${CMAKE_MAKE_PROGRAM} all
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building OS using existing Makefile"
)

# Provide a target to run the OS (QEMU) as defined in the Makefile.
add_custom_target(run_all
    COMMAND ${CMAKE_MAKE_PROGRAM} run
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running OS via make run"
)

# Make the default build invoke the build_all target.
add_custom_target(default ALL DEPENDS build_all)
