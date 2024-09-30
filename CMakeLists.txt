cmake_minimum_required(VERSION 3.22)
project(ParallelComputerSystemsArchitecture)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER /usr/bin/mpic++)

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")

add_executable(${PROJECT_NAME} lab1/main.cpp
        lab1/utility.cpp
        lab1/utility.h)

target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX MPI::MPI_CXX)