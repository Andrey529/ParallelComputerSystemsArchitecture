cmake_minimum_required(VERSION 3.22)
project(ParallelComputerSystemsArchitecture_lab2)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenMP REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)