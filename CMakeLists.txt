project(ParallelTest)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)
find_package(PkgConfig)
find_package(Threads)
find_package(Boost 1.60 REQUIRED COMPONENTS container)
find_package(OpenCV 4.1 REQUIRED)

add_subdirectory(src)
