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
#                         STM32CubeMX                         #
###############################################################

# Include mx-generated
include("mx-generated.cmake")

# Remove sources added in stm32cubemx
get_target_property(APP_SRCS ${CMAKE_PROJECT_NAME} SOURCES)
list(FILTER APP_SRCS EXCLUDE REGEX "nx_stm32_phy_custom_driver\\.c$")
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SOURCES "${APP_SRCS}")

# Remove NetXDuo source files
get_target_property(NETX_SRCS NetXDuo SOURCES)
list(FILTER NETX_SRCS EXCLUDE REGEX "nx_stm32_eth_driver\\.c$")
set_target_properties(NetXDuo PROPERTIES SOURCES "${NETX_SRCS}")

# Add NetXDuo source files
target_sources(NetXDuo PRIVATE
    ../Middlewares/ST/netxduo/addons/ptp/nxd_ptp_client.c
)

# Add STM32CubeMX source files
target_sources(stm32cubemx PRIVATE

    # This used to be in NetXDuo but has a dependency on the rest of the application
    application/src/nx_app/nx_stm32_eth_driver.c
)

# Add STM32CubeMX includes
target_include_directories(stm32cubemx INTERFACE
    ../Middlewares/ST/netxduo/addons/ptp
    application/inc/hal
)

# Add STM32CubeMX symbols (macros)
target_compile_definitions(stm32cubemx INTERFACE
    HAL_ETH_USE_PTP
)

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

# Collect application sources
file(GLOB_RECURSE APPLICATION_SOURCES
    application/src/*.c
)

# Collect Zenoh Pico sources
file(GLOB_RECURSE ZENOH_VENDOR_SOURCES
    libraries/zenoh-pico/src/*.c
)
set(ZENOH_FILTERED)
foreach(src_file IN LISTS ZENOH_VENDOR_SOURCES)
    # Keep if NOT under src/system or is under src/system/common
    string(FIND "${src_file}" "/system/" pos_system)
    string(FIND "${src_file}" "/system/common/" pos_common)
    if(pos_system EQUAL -1 OR pos_common GREATER -1)
        list(APPEND ZENOH_FILTERED "${src_file}")
    endif()
endforeach()
set(ZENOH_SOURCES
    ${ZENOH_FILTERED}
    ports/zenoh-pico/src/network.c
    ports/zenoh-pico/src/system.c
    ports/zenoh-pico/src/tcp.c
    ports/zenoh-pico/src/udp.c
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here

    # Application
    ${APPLICATION_SOURCES}

    # Ports
    ${ZENOH_SOURCES}
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
