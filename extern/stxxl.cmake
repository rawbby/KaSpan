cmake_minimum_required(VERSION 3.18)

find_package(Git)

set(stxxl_TAG 1.4.1)
set(stxxl_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/stxxl-src")
set(stxxl_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/stxxl-build")
set(stxxl_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/stxxl")
set(stxxl_VALID OFF)
if (EXISTS "${stxxl_INSTALL_DIR}/.valid")
    file(READ "${stxxl_INSTALL_DIR}/.valid" CONTENTS)
    string(STRIP "${CONTENTS}" CONTENTS)
    if (CONTENTS STREQUAL "${stxxl_TAG}")
        set(stxxl_VALID ON)
    endif ()
endif ()

if (NOT stxxl_VALID)

    # clear
    file(REMOVE_RECURSE "${stxxl_SOURCE_DIR}" "${stxxl_BUILD_DIR}" "${stxxl_INSTALL_DIR}")
    file(MAKE_DIRECTORY "${stxxl_SOURCE_DIR}")
    file(MAKE_DIRECTORY "${stxxl_BUILD_DIR}")
    file(MAKE_DIRECTORY "${stxxl_INSTALL_DIR}")

    # git clone
    execute_process(COMMAND ${GIT_EXECUTABLE} clone
            --depth 1
            --recurse-submodules
            --shallow-submodules
            --branch "${stxxl_TAG}"
            https://github.com/stxxl/stxxl.git
            "${stxxl_SOURCE_DIR}"
            RESULT_VARIABLE CLONE_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT CLONE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone stxxl (${stxxl_SOURCE_DIR}) (${CLONE_RESULT})")
    endif ()

    # cmake configure
    execute_process(COMMAND ${CMAKE_COMMAND}
            -S "${stxxl_SOURCE_DIR}"
            -B "${stxxl_BUILD_DIR}"
            -G "${CMAKE_GENERATOR}"
            -DCMAKE_INSTALL_PREFIX="${stxxl_INSTALL_DIR}"
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_TESTING=OFF
            -DCMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL}"
            RESULT_VARIABLE CONFIGURE_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT CONFIGURE_RESULT EQUAL 0)
        message(FATAL_ERROR "Configure failed for stxxl")
    endif ()

    # cmake build
    execute_process(COMMAND "${CMAKE_COMMAND}"
            --build "${stxxl_BUILD_DIR}"
            --config "${CMAKE_BUILD_TYPE}"
            RESULT_VARIABLE BUILD_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Build failed for stxxl")
    endif ()

    # cmake install
    execute_process(COMMAND "${CMAKE_COMMAND}"
            --install "${stxxl_BUILD_DIR}"
            --config ${CMAKE_BUILD_TYPE}
            --prefix "${stxxl_INSTALL_DIR}"
            RESULT_VARIABLE INSTALL_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "Install failed for stxxl")
    endif ()

    file(WRITE "${stxxl_INSTALL_DIR}/.valid" "${stxxl_TAG}")
    set(stxxl_VALID ON)
endif ()

list(PREPEND CMAKE_PREFIX_PATH "${stxxl_INSTALL_DIR}")
find_package(stxxl REQUIRED CONFIG PATHS "${stxxl_INSTALL_DIR}")
target_include_directories(stxxl SYSTEM INTERFACE "${stxxl_INSTALL_DIR}/include")
add_library(stxxl::stxxl ALIAS stxxl)
