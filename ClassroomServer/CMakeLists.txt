cmake_minimum_required(VERSION 3.16)
project(ClassroomServer LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network)

qt_standard_project_setup()

qt_add_executable(ClassroomServer
    main.cpp
    serverwindow.h serverwindow.cpp
)

target_link_libraries(ClassroomServer PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)
