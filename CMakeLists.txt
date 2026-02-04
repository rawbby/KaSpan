cmake_minimum_required(VERSION 3.14)

project(KaSpan VERSION 1.0
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/bootstrap.cmake")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/rawbby/cmake_utilities/refs/tags/v2.4.2/bootstrap.cmake"
            "${CMAKE_BINARY_DIR}/bootstrap.cmake")
endif ()

include("${CMAKE_BINARY_DIR}/bootstrap.cmake")
include(.cmake_utilities/modules/all.cmake)

find_package(MPI REQUIRED)

structured_directories()
