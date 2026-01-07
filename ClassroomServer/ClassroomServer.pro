QT       += core gui widgets sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = ClassroomServer
TEMPLATE = app

SOURCES += \
    main.cpp \
    serverwindow.cpp

HEADERS += \
    serverwindow.h

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
