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

# Set the project name
set(CMAKE_PROJECT_NAME primary)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

###############################################################
#                         STM32CubeMX                         #
###############################################################

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

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
    Middlewares/ST/netxduo/addons/ptp/nxd_ptp_client.c
)

# Add STM32CubeMX source files
target_sources(stm32cubemx PRIVATE

    # This used to be in NetXDuo but has a dependency on the rest of the application
    application/src/nx_app/nx_stm32_eth_driver.c
)

# Add STM32CubeMX includes
target_include_directories(stm32cubemx INTERFACE
    Middlewares/ST/netxduo/addons/ptp
    application/inc/hal
)

###############################################################
#                   Library Subdirectories                    #
###############################################################

# Add library subdirectories
add_subdirectory(cmake/nanopb)
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

# Collect bootloader sources
file(GLOB_RECURSE BOOTLOADER_SOURCES
    bootloader/src/*.c
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

    # Bootloader
    ${BOOTLOADER_SOURCES}

    # Ports
    ${ZENOH_SOURCES}

    # Libraries (TODO: Covert to standalone)
    libraries/fram/src/fram.c
)

###############################################################
#                           Includes                          #
###############################################################

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths

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

    # Bootloader
    bootloader/inc

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

    ZENOH_GENERIC
    Z_FEATURE_UNSTABLE_API
)

###############################################################
#                           Cleanup                           #
###############################################################

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

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
