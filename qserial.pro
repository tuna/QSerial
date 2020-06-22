QT += core gui widgets serialport webenginewidgets
CONFIG += console
TEMPLATE = app
TARGET = QSerial
INCLUDEPATH += .
DEFINES += QT_DEPRECATED_WARNINGS
SOURCES += main.cpp mainwindow.cpp mutualtest.cpp drivers/serialport.cpp drivers/serialportqt.cpp drivers/serialportcp210x.cpp drivers/serialportch34x.cpp drivers/serialportpl2303.cpp
HEADERS += mainwindow.h mutualtest.h drivers/serialport.h drivers/serialportqt.h drivers/serialportdummy.h drivers/serialportcp210x.h drivers/serialportch34x.h drivers/serialportpl2303.h
RESOURCES += resources.qrc
FORMS += mainwindow.ui mutualtest.ui
INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib -lusb-1.0
QMAKE_CXX_FLAGS += -fsanitize=address
QMAKE_LFLAGS += -fsanitize=address
TRANSLATIONS +=
