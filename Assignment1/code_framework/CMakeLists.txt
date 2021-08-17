cmake_minimum_required(VERSION 3.10)
project(Rasterizer)

set(OpenCV_DIR E:/support/OpenCV/opencv-4.5.3-build/x64/mingw/lib)

find_package(OpenCV 4.5.3 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER E:/support/mingw64/bin/gcc.exe)
set(CMAKE_CXX_COMPILER E:/support/mingw64/bin/g++.exe)
set(CMAKE_INSTALL_PREFIX E:/study/0-Computer_Graphics/homework/Assignment1/code_framework/install)

include_directories(E:/support/Eigen/eigen-3.4-rc1-build/include E:/support/OpenCV/opencv-4.5.3-build/include)

add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp Triangle.hpp Triangle.cpp)
target_link_libraries(Rasterizer ${OpenCV_LIBRARIES})
