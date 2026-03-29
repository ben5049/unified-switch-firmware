cmake_minimum_required(VERSION 3.22)
include(ExternalProject)

# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

# Set the project name
set(CMAKE_PROJECT_NAME primary)

# Core project settings
project(${CMAKE_PROJECT_NAME})

# Turn the environment variable into a cmake variable
if(DEFINED ENV{HW_VERSION})
    set(HW_VERSION "$ENV{HW_VERSION}" CACHE STRING "Hardware Revision" FORCE)
endif()
if(NOT HW_VERSION)
    message(FATAL_ERROR "No HW_VERSION found in environment")
endif()
message(STATUS "HW_VERSION = ${HW_VERSION}")

# Include external project file
include("${CMAKE_PROJECT_NAME}-v${HW_VERSION}.cmake")
