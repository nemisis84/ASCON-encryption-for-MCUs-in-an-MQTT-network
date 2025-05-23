cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(sensor C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# User-selectable encryption mode (string)
set(SELECTED_ENCRYPTION_MODE "AES_GCM" CACHE STRING "Selected encryption mode")
set_property(CACHE SELECTED_ENCRYPTION_MODE PROPERTY STRINGS ASCON_MASKED ASCON_UNMASKED AES_GCM NONE)

# Determine paths and settings based on the selected encryption mode
if(SELECTED_ENCRYPTION_MODE STREQUAL "ASCON_MASKED")
    set(ENCRYPTION_MODE_ID 1)
    set(ASCON_PATH ${CMAKE_CURRENT_LIST_DIR}/libs/ascon-suite)
    set(CRYPTO_INCLUDE ${ASCON_PATH}/include)
    set(CRYPTO_SOURCES "")
    add_subdirectory(${ASCON_PATH} EXCLUDE_FROM_ALL)
    list(APPEND ENCRYPTION_SOURCES masked_ascon_encryption.c)

elseif(SELECTED_ENCRYPTION_MODE STREQUAL "ASCON_UNMASKED")
    set(ENCRYPTION_MODE_ID 2)
    set(ASCON_PATH ${CMAKE_CURRENT_LIST_DIR}/libs/ascon/armv6m)
    file(GLOB CRYPTO_SOURCES ${ASCON_PATH}/*.c)
    set(CRYPTO_INCLUDE ${ASCON_PATH})

elseif(SELECTED_ENCRYPTION_MODE STREQUAL "AES_GCM")
    set(ENCRYPTION_MODE_ID 3)
    set(AES_PATH ${CMAKE_CURRENT_LIST_DIR}/libs/mbedtls-fewer)
    file(GLOB CRYPTO_SOURCES ${AES_PATH}/*.c)
    set(CRYPTO_INCLUDE ${AES_PATH})

elseif(SELECTED_ENCRYPTION_MODE STREQUAL "NONE")
    set(ENCRYPTION_MODE_ID 4)
    set(CRYPTO_SOURCES "")
    set(CRYPTO_INCLUDE "")

else()
    message(FATAL_ERROR "Invalid SELECTED_ENCRYPTION_MODE: ${SELECTED_ENCRYPTION_MODE}")
endif()



add_executable(sensor
    server.c server_common.c
    encryption.c
    ${CRYPTO_SOURCES}
    ${ENCRYPTION_SOURCES}
)

target_compile_definitions(sensor PRIVATE SELECTED_ENCRYPTION_MODE=${ENCRYPTION_MODE_ID})


# Compiler flag to diable optimisations
target_compile_options(sensor PRIVATE "-O0")

pico_enable_stdio_usb(sensor 1)

target_include_directories(sensor PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CRYPTO_INCLUDE}
)

target_link_libraries(sensor
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    pico_rand
)

if(SELECTED_ENCRYPTION_MODE STREQUAL "ASCON_MASKED")
    target_link_libraries(sensor ascon)
endif()


pico_btstack_make_gatt_header(sensor PRIVATE "${CMAKE_CURRENT_LIST_DIR}/temp_sensor.gatt")

pico_add_extra_outputs(sensor)


