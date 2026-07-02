cmake_minimum_required(VERSION 3.21)

# Default to Release before project() so toolchain picks correct flags.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

project(DanteCLI LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.6 REQUIRED
    COMPONENTS Core Gui Qml Quick QuickControls2 Network Sql Multimedia)

qt_standard_project_setup()

include(cmake/Warnings.cmake)

# Defines BUILD_TESTING (default ON) so module tests register with ctest.
include(CTest)

# One-way dependency order: core -> platform/persistence -> ui -> app.
add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(persistence)
add_subdirectory(ui)
add_subdirectory(app)
add_subdirectory(tools/dante) # dante.exe (CLI Named Pipe da orquestração)
