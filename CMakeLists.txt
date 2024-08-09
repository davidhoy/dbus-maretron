# CMakeLists.txt file for dbus-maretron.  This is a driver that listens to the NMEA 2000
# bus for battery status messages from Maretron DCM100's and puts that data onto the Victron
# dbus so that the DCM100 can appear as a Victron-compatible battery monitor.

cmake_minimum_required(VERSION 3.12)
project(dbus-maretron)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define the include directories
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/NMEA2000/src)
include_directories(${PROJECT_SOURCE_DIR}/NMEA2000_socketCAN)
#include_directories(${DBUS_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

# Build the NMEA2000 submodule (which has it's own CMakeLists.txt file)
add_subdirectory("NMEA2000")
add_subdirectory("dbus-asio")

# Define the source files for the dbus-maretron process
add_executable(${PROJECT_NAME} 
	src/main.cpp 
	src/dbus.cpp
	NMEA2000_socketCAN/NMEA2000_SocketCAN.cpp)

# Define the libraries to be linked with the dbus-maretron process
set(COMMON_LINK_LIBRARIES dbus-asio ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} nmea2000 ${COMMON_LINK_LIBRARIES}) 




