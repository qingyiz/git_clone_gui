cmake_minimum_required(VERSION 3.21)

project(
    GitCloneGui
    VERSION 1.0.0
    DESCRIPTION "Clone a parent Git repository and a nested child repository"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

include(CTest)
include(cmake/CompilerWarnings.cmake)

message(STATUS
    "正在查找 Qt 6 或 Qt 5（Core/Widgets）。如果查找失败，请安装 Qt 开发包，"
    "或通过 -DCMAKE_PREFIX_PATH=/path/to/Qt 指定 Qt。"
)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

if(BUILD_TESTING)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
endif()

set(QT_PACKAGE "Qt${QT_VERSION_MAJOR}")

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
