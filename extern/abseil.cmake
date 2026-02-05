cmake_minimum_required(VERSION 3.18)

find_package(Git)

set(ABSL_TAG lts_2026_01_07)
set(ABSL_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/abseil-src")
set(ABSL_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/abseil-build")
set(ABSL_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/abseil")
set(ABSL_VALID OFF)
if (EXISTS "${ABSL_INSTALL_DIR}/.valid")
    file(READ "${ABSL_INSTALL_DIR}/.valid" CONTENTS)
    string(STRIP "${CONTENTS}" CONTENTS)
    if (CONTENTS STREQUAL "${ABSL_TAG}")
        set(ABSL_VALID ON)
    endif ()
endif ()

if (NOT ABSL_VALID)

    # clear
    file(REMOVE_RECURSE "${ABSL_SOURCE_DIR}" "${ABSL_BUILD_DIR}" "${ABSL_INSTALL_DIR}")
    file(MAKE_DIRECTORY "${ABSL_SOURCE_DIR}")
    file(MAKE_DIRECTORY "${ABSL_BUILD_DIR}")
    file(MAKE_DIRECTORY "${ABSL_INSTALL_DIR}")

    # git clone
    execute_process(COMMAND ${GIT_EXECUTABLE} clone
            --depth 1
            --recurse-submodules
            --shallow-submodules
            --branch "${ABSL_TAG}"
            https://github.com/abseil/abseil-cpp
            "${ABSL_SOURCE_DIR}"
            RESULT_VARIABLE CLONE_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT CLONE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone ABSL")
    endif ()

    # cmake configure
    execute_process(COMMAND ${CMAKE_COMMAND}
            -S "${ABSL_SOURCE_DIR}"
            -B "${ABSL_BUILD_DIR}"
            -G "${CMAKE_GENERATOR}"
            -DCMAKE_INSTALL_PREFIX="${ABSL_INSTALL_DIR}"
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL}"
            -DABSL_BUILD_TESTING=OFF
            -DABSL_USE_GOOGLETEST_HEAD=OFF
            -DCMAKE_CXX_STANDARD=20
            RESULT_VARIABLE CONFIGURE_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT CONFIGURE_RESULT EQUAL 0)
        message(FATAL_ERROR "Configure failed for ABSL")
    endif ()

    # cmake build
    execute_process(COMMAND "${CMAKE_COMMAND}"
            --build "${ABSL_BUILD_DIR}"
            --config "${CMAKE_BUILD_TYPE}"
            RESULT_VARIABLE BUILD_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Build failed for ABSL")
    endif ()

    # cmake install
    execute_process(COMMAND "${CMAKE_COMMAND}"
            --install "${ABSL_BUILD_DIR}"
            --config ${CMAKE_BUILD_TYPE}
            --prefix "${ABSL_INSTALL_DIR}"
            RESULT_VARIABLE INSTALL_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "Install failed for ABSL")
    endif ()

    file(WRITE "${ABSL_INSTALL_DIR}/.valid" "${ABSL_TAG}")
    set(ABSL_VALID ON)
endif ()

list(PREPEND CMAKE_PREFIX_PATH "${ABSL_INSTALL_DIR}")
find_package(absl CONFIG REQUIRED)
