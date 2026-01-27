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
set(STM32_MCU_FLAGS  "-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard " )

#Linker options
set(STM32_LINKER_SCRIPT STM32H573XX_FLASH_NS.ld)
set(STM32_LINKER_OPTION  )

# Set the project name
set(CMAKE_PROJECT_NAME primary_NS)
project(${CMAKE_PROJECT_NAME})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

# Include mx-generated
include("mx-generated.cmake")

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

# Include mx-generated.cmake
if(NOT PLATFORM STREQUAL "switch")
    message(FATAL_ERROR "Invalid PLATFORM: ${PLATFORM}")
endif()

# Pass it to the program
string(TOUPPER "${PLATFORM}" PLATFORM_UPPER)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    PLATFORM="${PLATFORM}"
    PLATFORM_${PLATFORM_UPPER}
)

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

# Convert the hardware version to an integer and pass it to the program
math(EXPR HW_VERSION_INT "${HW_VERSION}")
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HW_VERSION=${HW_VERSION_INT})

# Apply version specific settings
if(HW_VERSION STREQUAL "4")

    # # Add v4 specific files if needed
    # target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    #     src/hardware_v4_init.c
    # )

elseif(HW_VERSION STREQUAL "5")

    # # Add v5 specific files if needed
    # target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    #     src/hardware_v5_init.c
    # )

else()
    message(FATAL_ERROR "Unknown Hardware Version: ${HW_VERSION}")
endif()

###############################################################
#                   Library Subdirectories                    #
###############################################################

# Add library subdirectories
add_subdirectory(ports/nanopb)
add_subdirectory(libraries/phy-drivers)
add_subdirectory(libraries/sja1105)
add_subdirectory(libraries/logging)

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
)

###############################################################
#                           Includes                          #
###############################################################

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths

    # Bootloader (needed for secure_nsc.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../Secure

    # Application
    application/inc
    application/inc/comms
    application/inc/hal
    application/inc/nx_app
    application/inc/phy
    application/inc/protobuf
    application/inc/protobuf/generated
    application/inc/ptp
    application/inc/stp
    application/inc/switch

    # Ports
    ports/zenoh-pico/inc
    libraries/zenoh-pico/include

    # Libraries (TODO: Covert to standalone)
    libraries/fram/inc
)

###############################################################
#                            Macros                           #
###############################################################

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols

    HAL_ETH_USE_PTP
    TX_THREAD_SECURE_STACK_MAXIMUM=4096

    ZENOH_GENERIC
    Z_FEATURE_UNSTABLE_API
)

###############################################################
#                       Library Linking                       #
###############################################################

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    nanopb
    sja1105
    phy_drivers
    logging
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
