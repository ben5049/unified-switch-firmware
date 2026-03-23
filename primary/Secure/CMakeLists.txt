###############################################################
#                        Project Setup                        #
###############################################################

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
set(STM32_MCU_FLAGS  "-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mcmse " )

# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

###############################################################
#                          Platform                           #
###############################################################

# Turn the PLATFORM environment variable into a cmake variable
if(DEFINED ENV{PLATFORM})
    set(PLATFORM "$ENV{PLATFORM}" CACHE STRING "Platform" FORCE)
endif()
if(NOT PLATFORM)
    message(FATAL_ERROR "No PLATFORM found in environment")
endif()

# Set the project name
if(PLATFORM STREQUAL "switch")
    set(CMAKE_PROJECT_NAME primary_S)
elseif(PLATFORM STREQUAL "mag-lock")
    set(CMAKE_PROJECT_NAME mag-lock_S)
else()
    message(FATAL_ERROR "Invalid PLATFORM: ${PLATFORM}")
endif()

# Create variable that will be used as a C macro
string(TOUPPER "${PLATFORM}" PLATFORM_UPPER)
string(REPLACE "-" "_" PLATFORM_UPPER "${PLATFORM_UPPER}")

###############################################################
#                      Hardware Revision                      #
###############################################################

# Turn the environment variable into a cmake variable
if(DEFINED ENV{HW_VERSION})
    set(HW_VERSION "$ENV{HW_VERSION}" CACHE STRING "Hardware Revision" FORCE)
endif()
if(NOT HW_VERSION)
    message(FATAL_ERROR "No HW_VERSION found in environment")
endif()
message(STATUS "Configuring for hardware: ${PLATFORM} v${HW_VERSION}")

math(EXPR HW_VERSION_INT "${HW_VERSION}")

###############################################################
#                  Re-Regenerate CubeMX Files                 #
###############################################################

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Verify the path to the script actually exists before running
set(SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/scripts/move_generated_files.py")
if(NOT EXISTS "${SCRIPT_PATH}")
    message(FATAL_ERROR "Script not found at: ${SCRIPT_PATH}")
endif()

execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${SCRIPT_PATH}" "${PLATFORM}_v${HW_VERSION}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE script_res
    OUTPUT_VARIABLE script_out
    ERROR_VARIABLE  script_err
)

# Check the result
if(NOT "${script_res}" STREQUAL "0")
    message(FATAL_ERROR "Python script failed!\nResult: ${script_res}\nSTDOUT: ${script_out}\nSTDERR: ${script_err}")
endif()

###############################################################
#                           Linker                            #
###############################################################

#Linker options
set(STM32_LINKER_SCRIPT "platform/${PLATFORM}_v${HW_VERSION}/STM32H573XX_FLASH_S.ld")
set(STM32_LINKER_OPTION  -Wl,--cmse-implib\ -Wl,--out-implib="${CMAKE_CURRENT_SOURCE_DIR}/../Secure_nsclib/secure_nsclib.o")

###############################################################
#                     Create the project                      #
###############################################################

project(${CMAKE_PROJECT_NAME})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Include mx-generated.cmake
include("platform/${PLATFORM}_v${HW_VERSION}/mx-generated.cmake")

###############################################################
#                      Hardware Revision                      #
###############################################################

# Turn the HW_VERSION environment variable into a cmake variable
if(DEFINED ENV{HW_VERSION})
    set(HW_VERSION "$ENV{HW_VERSION}" CACHE STRING "Hardware Revision" FORCE)
endif()
if(NOT HW_VERSION)
    message(FATAL_ERROR "No HW_VERSION found in environment")
endif()
message(STATUS "PLATFORM = ${PLATFORM}")
message(STATUS "HW_VERSION = ${HW_VERSION}")

# Apply version specific settings
if(HW_VERSION STREQUAL "4")
elseif(HW_VERSION STREQUAL "5")
else()
    message(FATAL_ERROR "Unknown Hardware Version: ${HW_VERSION}")
endif()

# Convert the hardware version to an integer and pass it to the program
math(EXPR HW_VERSION_INT "${HW_VERSION}")
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HW_VERSION=${HW_VERSION_INT})

###############################################################
#                   Library Subdirectories                    #
###############################################################

# Add library subdirectories
add_subdirectory(bootloader)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

###############################################################
#                           Sources                           #
###############################################################

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    common/src/bootloader_callbacks.c
    common/src/memory_tools.c
)

###############################################################
#                           Includes                          #
###############################################################

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    common/inc
    bootloader # This is here because the bootloader secure_nsc.h needs to be exposed
)

###############################################################
#                            Macros                           #
###############################################################

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols

    PLATFORM="${PLATFORM}"
    PLATFORM_${PLATFORM_UPPER}
    HW_VERSION=${HW_VERSION_INT}
    DEVICE_NAME="${PLATFORM}-v${HW_VERSION}"

    TX_THREAD_SECURE_STACK_MAXIMUM=4096
)

###############################################################
#                       Library Linking                       #
###############################################################

# stm32cubemx depends on bootloader
target_link_libraries(stm32cubemx PUBLIC bootloader)

# Secure firmware depends on stm32cubemx
target_link_libraries(${CMAKE_PROJECT_NAME} stm32cubemx)

###############################################################
#                      Finishing Touches                      #
###############################################################

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
