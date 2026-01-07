cmake_minimum_required(VERSION 3.19)
project(ClassroomSignSystem LANGUAGES CXX)

# 【修改点 1】：在组件列表中添加 Sql 和 Network
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Network)

qt_standard_project_setup()

qt_add_executable(ClassroomSignSystem
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    DatabaseManager.h
    networkworker.h
    networkworker.cpp
)

target_link_libraries(ClassroomSignSystem
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql          # 【修改点 3】：链接数据库库
        Qt::Network      # 【修改点 3】：链接网络库
)

include(GNUInstallDirs)

install(TARGETS ClassroomSignSystem
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ClassroomSignSystem
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
