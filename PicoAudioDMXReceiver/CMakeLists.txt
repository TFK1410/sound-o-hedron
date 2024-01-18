# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C   standards
project(fft-dmx-pico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

add_library(kiss_fftr kiss_fftr.c)
add_library(kiss_fft kiss_fft.c)
target_link_libraries(kiss_fftr kiss_fft)

add_library(fft_utils fft_utils.c)
target_link_libraries(fft_utils pico_stdlib pico_malloc pico_mem_ops)

# Include pico-dmx library
include(./Pico-DMX/interfaceLibForPicoSDK.cmake)

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME} 
    main.cpp
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
    hardware_adc
    hardware_dma
    hardware_gpio
    hardware_pio
    hardware_i2c
    pico_stdlib
    pico_multicore
    pico_i2c_slave
    pico_time
    picodmx
    kiss_fftr
    fft_utils
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)