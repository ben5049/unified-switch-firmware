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
set(STM32_MCU_FLAGS  "-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mcmse ")

# Set the project name
set(CMAKE_PROJECT_NAME primary_S)

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
message(STATUS "PLATFORM = ${PLATFORM}")

# Create variable that will be used as a C macro
string(TOUPPER "${PLATFORM}" PLATFORM_UPPER)
string(REPLACE "-" "_" PLATFORM_UPPER "${PLATFORM_UPPER}")

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
message(STATUS "HW_VERSION = ${HW_VERSION}")

# Apply version specific settings
if(HW_VERSION STREQUAL "4")
elseif(HW_VERSION STREQUAL "5")
else()
    message(FATAL_ERROR "Unknown Hardware Version: ${HW_VERSION}")
endif()

# Convert the hardware version to an integer
math(EXPR HW_VERSION_INT "${HW_VERSION}")

###############################################################
#                  Re-Regenerate CubeMX Files                 #
###############################################################

include("${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/cmake/cubemx-tools.cmake")

cubemx_move_generated_to_platform("${CMAKE_CURRENT_SOURCE_DIR}" "${PLATFORM}_v${HW_VERSION}")

###############################################################
#                           Linker                            #
###############################################################

include("${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/cmake/linker-tools.cmake")

#Linker options
set(STM32_LINKER_SCRIPT "platform/${PLATFORM}_v${HW_VERSION}/STM32H573XX_FLASH_S.ld")
set(STM32_LINKER_OPTION  -Wl,--cmse-implib\ -Wl,--out-implib="${CMAKE_CURRENT_SOURCE_DIR}/../Secure_nsclib/secure_nsclib.o")

# Add symbols for the start, end and size of all sections
linker_add_symbols("${CMAKE_CURRENT_SOURCE_DIR}/${STM32_LINKER_SCRIPT}")

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
#                   Library Subdirectories                    #
###############################################################

# Add library subdirectories
add_subdirectory(bootloader)

###############################################################
#                           Sources                           #
###############################################################

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    application/src/app.c
    application/src/backup.c
    application/src/secure_nsc.c
    application/src/bootloader_hooks/crypto.c
    application/src/bootloader_hooks/io.c
    application/src/bootloader_hooks/memory.c
    application/src/bootloader_hooks/sys.c
    application/src/bootloader_hooks/time.c
    application/src/bootloader_hooks/user.c
)

###############################################################
#                           Includes                          #
###############################################################

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    application/inc
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

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    bootloader
)

###############################################################
#                      Finishing Touches                      #
###############################################################

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
