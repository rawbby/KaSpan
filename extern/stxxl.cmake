# set(stxxl_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/stxxl-src")
# set(stxxl_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/stxxl-src/build")
# set(stxxl_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/stxxl")
# set(stxxl_STAMP "${CMAKE_CURRENT_BINARY_DIR}/stxxl.stamp")
#
# set(stxxl_BUILD_NOW OFF)
# if (NOT EXISTS "${stxxl_STAMP}")
#     set(stxxl_BUILD_NOW ON)
#     # file(REMOVE_RECURSE "${stxxl_SOURCE_DIR}")
#     file(MAKE_DIRECTORY
#             "${CMAKE_BINARY_DIR}"
#             "${CMAKE_BINARY_DIR}/_deps"
#             "${CMAKE_BINARY_DIR}/_deps/stxxl-src"
#             RESULT MAKE_DIRECTORY_RESULT)
#     if (NOT MAKE_DIRECTORY_RESULT EQUAL 0)
#         message(FATAL_ERROR "Failed to create directory: ${stxxl_SOURCE_DIR}")
#     endif ()
#     execute_process(
#             COMMAND git clone
#             --recurse-submodules
#             --branch 1.4.1
#             --single-branch
#             --depth 1
#             https://github.com/stxxl/stxxl.git
#             "${stxxl_SOURCE_DIR}"
#             OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
# else ()
#     file(GLOB_RECURSE stxxl_SOURCES "${stxxl_SOURCE_DIR}/src/*")
#     foreach (stxxl_SOURCE IN LISTS stxxl_SOURCES)
#         if (EXISTS "${stxxl_SOURCE}" AND "${stxxl_SOURCE}" IS_NEWER_THAN "${stxxl_STAMP}")
#             set(stxxl_BUILD_NOW ON)
#             break()
#         endif ()
#     endforeach ()
# endif ()
#
# if (${stxxl_BUILD_NOW})
#     execute_process(
#             COMMAND cmake -S "${stxxl_SOURCE_DIR}" -B "${stxxl_BINARY_DIR}"
#             "-DBUILD_TESTS=OFF"
#             "-DBUILD_TESTING=OFF"
#             "-DBUILD_EXTRAS=OFF"
#             "-DBUILD_EXAMPLES=OFF"
#             "-DBUILD_SHARED_LIBS=OFF"
#             "-DUSE_VALGRIND=OFF"
#             "-DUSE_GCOV=OFF"
#             "-DUSE_TPIE=OFF"
#             "-DCMAKE_MESSAGE_LOG_LEVEL=NOTICE"
#             "-DCMAKE_SUPPRESS_DEVELOPER_WARNINGS=1"
#             "-DCMAKE_WARN_DEPRECATED=OFF"
#             "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
#             "-DCMAKE_INSTALL_PREFIX=${stxxl_INSTALL_DIR}"
#             WORKING_DIRECTORY "${stxxl_SOURCE_DIR}"
#             OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
#     execute_process(
#             COMMAND cmake --build "${stxxl_BINARY_DIR}"
#             WORKING_DIRECTORY "${stxxl_SOURCE_DIR}"
#             OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
#     execute_process(
#             COMMAND cmake --install "${stxxl_BINARY_DIR}"
#             WORKING_DIRECTORY "${stxxl_SOURCE_DIR}"
#             OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
#     file(TOUCH "${stxxl_STAMP}")
# endif ()
# list(APPEND CMAKE_PREFIX_PATH "${stxxl_INSTALL_DIR}")
#
# find_package(stxxl REQUIRED)
# add_library(stxxl::stxxl ALIAS stxxl)
#

cmake_minimum_required(VERSION 3.18)

block()
    include(FetchContent)
    FetchContent_Declare(stxxl GIT_REPOSITORY https://github.com/stxxl/stxxl.git GIT_TAG 1.4.1)
    FetchContent_MakeAvailable(stxxl)
endblock()
