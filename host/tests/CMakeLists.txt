include(GoogleTest)

add_executable(hydra-tests
    test_gcode_parser.cpp
    test_sync_engine.cpp
    ${CMAKE_SOURCE_DIR}/src/gcode/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/gcode/stream.cpp
    ${CMAKE_SOURCE_DIR}/src/gcode/sync_engine.cpp
)

target_include_directories(hydra-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/../shared
)

target_link_libraries(hydra-tests PRIVATE
    GTest::gtest_main
    spdlog::spdlog
)

gtest_discover_tests(hydra-tests)
