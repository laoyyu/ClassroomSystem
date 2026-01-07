cmake_minimum_required(VERSION 3.16)
project(ClassroomServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network)

qt_standard_project_setup()

qt_add_executable(ClassroomServer
    main.cpp
    serverwindow.h
    serverwindow.cpp
)

target_link_libraries(ClassroomServer PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)
