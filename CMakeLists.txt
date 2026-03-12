cmake_minimum_required(VERSION 3.22)
project(hydra-firmware VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_HOST "Build host software" ON)
option(BUILD_UI "Build Qt6 UI" ON)
option(BUILD_MCU "Build MCU firmware (requires Pico SDK)" OFF)
option(BUILD_TESTS "Build tests" ON)

if(BUILD_HOST)
    add_subdirectory(host)
endif()

if(BUILD_UI)
    add_subdirectory(ui)
endif()

if(BUILD_MCU)
    add_subdirectory(mcu)
endif()
