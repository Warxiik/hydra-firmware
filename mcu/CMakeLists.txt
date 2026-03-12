cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(hydra-mcu C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hydra-mcu
    src/main.c
    src/stepper.c
    src/transport.c
    src/protocol.c
    src/adc.c
    src/gpio.c
    src/watchdog.c
    src/tmc_uart.c
)

target_include_directories(hydra-mcu PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/../shared
)

# PIO programs
pico_generate_pio_header(hydra-mcu ${CMAKE_CURRENT_LIST_DIR}/pio/step_dir_sr.pio)
pico_generate_pio_header(hydra-mcu ${CMAKE_CURRENT_LIST_DIR}/pio/tmc_uart.pio)

target_link_libraries(hydra-mcu
    pico_stdlib
    pico_multicore
    hardware_spi
    hardware_pio
    hardware_adc
    hardware_pwm
    hardware_watchdog
    hardware_dma
)

# Use SPI for host communication (not USB stdio)
pico_enable_stdio_usb(hydra-mcu 0)
pico_enable_stdio_uart(hydra-mcu 0)

pico_add_extra_outputs(hydra-mcu)
