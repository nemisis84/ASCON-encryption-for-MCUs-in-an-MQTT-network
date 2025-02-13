cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

project(sensor-MCU C CXX ASM)

# Set Pico board
set(PICO_BOARD pico_w CACHE STRING "Board type")

#Disable compiler optimizations
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -D PICO_DEBUG=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -D PICO_DEBUG=1")


# Enable Pico SDK
pico_sdk_init()

# Add ASCON library path
# set(ASCON_PATH ${CMAKE_CURRENT_LIST_DIR}/libs/ascon/armv6m)
# set(ASCON_PATH ${CMAKE_CURRENT_LIST_DIR}/libs/ascon/armv6m_lowsize)
# set(ASCON_PATH ${CMAKE_CURRENT_LIST_DIR}/libs/ascon/bi32_armv6m)
set(ASCON_PATH ${CMAKE_CURRENT_LIST_DIR}/libs/ascon/protected_bi32_armv6_leveled)

# Add executable
file(GLOB ASCON_SOURCES ${ASCON_PATH}/*.c)

add_executable(sensor-MCU 
    sensor-MCU.c 
    ${ASCON_SOURCES}
)



# Include ASCON headers
target_include_directories(sensor-MCU PRIVATE ${ASCON_PATH})

# Enable stdio (USB for debugging)
pico_enable_stdio_usb(sensor-MCU 1)
pico_enable_stdio_uart(sensor-MCU 0)

# Link required libraries
target_link_libraries(sensor-MCU
    pico_stdlib
    hardware_dma
    hardware_clocks
)

# Generate output files (UF2, bin, etc.)
pico_add_extra_outputs(sensor-MCU)
