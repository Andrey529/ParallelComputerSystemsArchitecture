cmake_minimum_required(VERSION 3.22)
project(ParallelComputerSystemsArchitecture_lab1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER /usr/bin/mpic++)

find_package(MPI REQUIRED)
message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")

add_executable(${PROJECT_NAME} main.cpp
        utility.cpp
        utility.h)

target_link_libraries(${PROJECT_NAME} PUBLIC MPI::MPI_CXX)