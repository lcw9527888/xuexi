cmake_minimum_required(VERSION 3.28.3)

project(virtualspeed CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(virtualspeed speed.cpp)

target_link_libraries(virtualspeed ${OpenCV_LIBS})
