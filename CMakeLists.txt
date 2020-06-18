cmake_minimum_required(VERSION 3.1.0)
project(QSerial) # Your project name

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_DEPRECATED_WARNINGS -fsanitize=address")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -fsanitize=address")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to run uic automatically when needed.
set(CMAKE_AUTOUIC ON)

# This will find the Qt5 files. You will need a QT5_DIR env variable
find_package(Qt5 COMPONENTS Core Gui Widgets SerialPort WebEngineWidgets REQUIRED)

set(SOURCES main.cpp mainwindow.cpp mutualtest.cpp) 
file(GLOB_RECURSE SOURCES drivers/*.cpp)
set(HEADERS mainwindow.h mutualtest.h)
file(GLOB_RECURSE HEADERS drivers/*.h)
set(UI mainwindow.ui mutualtest.ui)
set(RESOURCES resources.qrc)

find_package(PkgConfig)
pkg_search_module(LIBUSB_1 REQUIRED libusb-1.0)
# MESSAGE(STATUS ${LIBUSB_1_LIBRARIES})
# MESSAGE(STATUS ${LIBUSB_1_INCLUDE_DIRS})

include_directories(SYSTEM /usr/local/include)
link_directories(/usr/local/lib)

# This will create you executable
add_executable(QSerial ${SOURCES} ${HEADERS} ${UI})
# target_include_directories(QSerial SYSTEM PRIVATE ${LIBUSB_1_INCLUDE_DIRS})
# This will link necessary Qt5 libraries to your project
target_link_libraries(QSerial Qt5::Widgets Qt5::SerialPort Qt5::WebEngineWidgets ${LIBUSB_1_LIBRARIES})