cmake_minimum_required(VERSION 3.14)

project(KaSpan VERSION 1.0
        LANGUAGES C CXX)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/bootstrap.cmake")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/rawbby/cmake_utilities/refs/tags/v2.3.14/bootstrap.cmake"
            "${CMAKE_BINARY_DIR}/bootstrap.cmake")
endif ()

include("${CMAKE_BINARY_DIR}/bootstrap.cmake")
include(.cmake_utilities/modules/all.cmake)

python(-m pip install --upgrade -r "${PROJECT_SOURCE_DIR}/requirements.txt")

find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)

structured_directories()
