cmake_minimum_required(VERSION 3.16)
project(QSerial)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_DEPRECATED_WARNINGS -fsanitize=address")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -fsanitize=address")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Try Qt5 & Qt6
find_package(Qt5 COMPONENTS Core Gui Widgets SerialPort WebEngineWidgets)
find_package(Qt6 COMPONENTS Core Gui Widgets SerialPort Core5Compat WebEngineWidgets)

set(MAIN_SOURCES main.cpp mainwindow.cpp mutualtest.cpp) 
file(GLOB_RECURSE DRIVER_SOURCES drivers/*.cpp)
set(UI mainwindow.ui mutualtest.ui)
set(RESOURCES resources.qrc)

find_package(PkgConfig)
pkg_search_module(LIBUSB_1 REQUIRED libusb-1.0)

# If serialportdummy.h is not added to the list, it won't compile on macOS. Strange...
add_executable(QSerial ${MAIN_SOURCES} ${DRIVER_SOURCES} ${UI} ${RESOURCES} drivers/serialportdummy.h)
target_include_directories(QSerial PRIVATE ${LIBUSB_1_INCLUDE_DIRS})
target_link_directories(QSerial PRIVATE ${LIBUSB_1_LIBRARY_DIRS})

if (Qt5_FOUND)
target_link_libraries(QSerial Qt5::Widgets Qt5::SerialPort Qt5::WebEngineWidgets ${LIBUSB_1_LIBRARIES})
endif ()

if (Qt6_FOUND)
target_link_libraries(QSerial Qt6::Widgets Qt6::SerialPort Qt6::Core5Compat Qt6::WebEngineWidgets ${LIBUSB_1_LIBRARIES})
endif ()

install(TARGETS QSerial)
