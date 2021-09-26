cmake_minimum_required(VERSION 3.10)
project(BezierCurve)

set(OpenCV_DIR E:/support/OpenCV/opencv-4.5.3-build/x64/mingw/lib)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER E:/support/mingw64/bin/gcc.exe)
set(CMAKE_CXX_COMPILER E:/support/mingw64/bin/g++.exe)

find_package(OpenCV 4.5.3 REQUIRED)

include_directories(E:/support/OpenCV/opencv-4.5.3-build/include)
add_executable(BezierCurve main.cpp)

target_link_libraries(BezierCurve ${OpenCV_LIBRARIES})
