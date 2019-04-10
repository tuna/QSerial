QT += core gui widgets serialport webenginewidgets
TEMPLATE = app
TARGET = QSerial
INCLUDEPATH += .
DEFINES += QT_DEPRECATED_WARNINGS
SOURCES += main.cpp mainwindow.cpp mutualtest.cpp serialport.cpp serialportqt.cpp serialportcp210x.cpp serialportch34x.cpp
HEADERS += mainwindow.h mutualtest.h serialport.h serialportqt.h serialportdummy.h serialportcp210x.h serialportch34x.h
RESOURCES += resources.qrc
FORMS += mainwindow.ui mutualtest.ui
INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib -lusb-1.0
QMAKE_CXX_FLAGS += -fsanitize=address
QMAKE_LFLAGS += -fsanitize=address
TRANSLATIONS +=
