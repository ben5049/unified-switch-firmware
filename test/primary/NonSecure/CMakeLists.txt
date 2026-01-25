cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Compiler options
set(STM32_MCU_FLAGS  "-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard " )

#Linker options
set(STM32_LINKER_SCRIPT STM32H573XX_FLASH_NS.ld)
set(STM32_LINKER_OPTION  )

# Set the project name
set(CMAKE_PROJECT_NAME primary_NS)
project(${CMAKE_PROJECT_NAME})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Include mx-generated
include("mx-generated.cmake")

# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")


# Turn the environment variable to a cmake variable
if(DEFINED ENV{HW_VERSION})
    set(HW_VERSION "$ENV{HW_VERSION}" CACHE STRING "Hardware Revision" FORCE)
endif()

# Default to v4 if not specified
if(NOT HW_VERSION)
    set(HW_VERSION "4" CACHE STRING "Hardware Revision: 4 or 5")
endif()

message(STATUS "Configuring for switch hardware version: v${HW_VERSION}")

if(HW_VERSION STREQUAL "4")
    # Set the macro for code: #ifdef HW_V4
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HW_V4)

    # # Add v4 specific files if needed
    # target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    #     src/hardware_v4_init.c
    # )

elseif(HW_VERSION STREQUAL "5")
    # Set the macro for code: #ifdef HW_V5
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HW_V5)

    # # Add v5 specific files if needed
    # target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    #     src/hardware_v5_init.c
    # )
else()
    message(FATAL_ERROR "Unknown Hardware Version: ${HW_VERSION}")
endif()



# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined libraries
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
