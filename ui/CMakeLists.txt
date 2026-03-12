cmake_minimum_required(VERSION 3.22)
project(hydra-ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2)

qt_add_executable(hydra-ui
    main.cpp
    src/backend/bridge.cpp
    src/backend/printer_model.cpp
    src/backend/file_model.cpp
    src/backend/temp_history.cpp
)

qt_add_qml_module(hydra-ui
    URI Hydra
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/components/StatusBar.qml
        qml/components/ProgressArc.qml
        qml/components/TempGauge.qml
        qml/components/NozzleIndicator.qml
        qml/pages/HomePage.qml
        qml/pages/PrintPage.qml
        qml/pages/PreparePage.qml
        qml/pages/TunePage.qml
        qml/pages/TemperaturePage.qml
        qml/pages/MovePage.qml
        qml/pages/FilesPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/NetworkPage.qml
    RESOURCES
        resources/resources.qrc
)

target_include_directories(hydra-ui PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/../shared
)

target_link_libraries(hydra-ui PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
)

# EGLFS target for embedded deployment
if(CMAKE_CROSSCOMPILING)
    target_compile_definitions(hydra-ui PRIVATE HYDRA_EMBEDDED=1)
endif()
