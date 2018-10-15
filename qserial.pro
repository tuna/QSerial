QT += core gui widgets serialport
TEMPLATE = app
TARGET = QSerial
INCLUDEPATH += .
DEFINES += QT_DEPRECATED_WARNINGS
SOURCES += main.cpp mainwindow.cpp serialport.cpp serialportqt.cpp serialportcp210x.cpp
HEADERS += mainwindow.h serialport.h serialportqt.h serialportdummy.h serialportcp210x.h
RESOURCES +=
FORMS += mainwindow.ui
INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib -lusb-1.0
QMAKE_CXX_FLAGS += -fsanitize=address
QMAKE_LFLAGS += -fsanitize=address
TRANSLATIONS +=
