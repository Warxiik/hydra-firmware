cmake_minimum_required(VERSION 3.22)
project(hydra-host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies via FetchContent
include(FetchContent)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)

FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(spdlog tomlplusplus json)

# Host executable
add_executable(hydra-host
    src/main.cpp
    src/core/engine.cpp
    src/core/config.cpp
    src/gcode/parser.cpp
    src/gcode/stream.cpp
    src/gcode/sync_engine.cpp
    src/motion/planner.cpp
    src/motion/trapezoid.cpp
    src/motion/corexy.cpp
    src/motion/nozzle_offset.cpp
    src/motion/step_compress.cpp
    src/motion/collision.cpp
    src/thermal/pid.cpp
    src/thermal/heater.cpp
    src/thermal/thermistor.cpp
    src/thermal/safety.cpp
    src/comms/transport.cpp
    src/comms/protocol.cpp
    src/comms/clock_sync.cpp
    src/comms/mcu_proxy.cpp
)

target_include_directories(hydra-host PRIVATE
    src
    include
    ${CMAKE_SOURCE_DIR}/../shared
)

target_link_libraries(hydra-host PRIVATE
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    nlohmann_json::nlohmann_json
    pthread
)

# Tests
if(BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()
