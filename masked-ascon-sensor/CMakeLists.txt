# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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

project(masked-ascon-sensor C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


set(ASCON_PATH ${CMAKE_CURRENT_LIST_DIR}/libs/ascon-suite)

add_subdirectory(${ASCON_PATH} EXCLUDE_FROM_ALL)
# Add executable. Default name is the project name, version 0.1

# Standalone example that reads from the on board temperature sensor and sends notifications via BLE
# Flashes slowly each second to show it's running
add_executable(masked-ascon-sensor
    server.c server_common.c
    encryption.c
    )

pico_set_program_name(masked-ascon-sensor "masked-ascon-sensor")
pico_set_program_version(masked-ascon-sensor "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(masked-ascon-sensor 0)
pico_enable_stdio_usb(masked-ascon-sensor 1)


target_link_libraries(masked-ascon-sensor
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    ascon
    )
target_include_directories(masked-ascon-sensor PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    ${ASCON_PATH}/include # For ASCON headers
    )
pico_btstack_make_gatt_header(masked-ascon-sensor PRIVATE "${CMAKE_CURRENT_LIST_DIR}/temp_sensor.gatt")

pico_add_extra_outputs(masked-ascon-sensor)


