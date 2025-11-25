cmake_minimum_required(VERSION 3.18)

find_package(Git)

set(BriefKAsten_TAG main)
set(BriefKAsten_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/BriefKAsten-src")
set(BriefKAsten_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/BriefKAsten-build")
set(BriefKAsten_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/BriefKAsten")
set(BriefKAsten_VALID OFF)
if (EXISTS "${BriefKAsten_INSTALL_DIR}/.valid")
    file(READ "${BriefKAsten_INSTALL_DIR}/.valid" CONTENTS)
    string(STRIP "${CONTENTS}" CONTENTS)
    if (CONTENTS STREQUAL "${BriefKAsten_TAG}")
        set(BriefKAsten_VALID ON)
    endif ()
endif ()

if (NOT BriefKAsten_VALID)

    # clear
    file(REMOVE_RECURSE "${BriefKAsten_SOURCE_DIR}" "${BriefKAsten_BUILD_DIR}" "${BriefKAsten_INSTALL_DIR}")
    file(MAKE_DIRECTORY "${BriefKAsten_SOURCE_DIR}")
    file(MAKE_DIRECTORY "${BriefKAsten_BUILD_DIR}")
    file(MAKE_DIRECTORY "${BriefKAsten_INSTALL_DIR}")

    # git clone
    execute_process(COMMAND ${GIT_EXECUTABLE} clone
            --depth 1
            --recurse-submodules
            --shallow-submodules
            --branch "${BriefKAsten_TAG}"
            https://github.com/niklas-uhl/BriefKAsten
            "${BriefKAsten_SOURCE_DIR}"
            RESULT_VARIABLE CLONE_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT CLONE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone BriefKAsten")
    endif ()

    # cmake configure
    execute_process(COMMAND ${CMAKE_COMMAND}
            -S "${BriefKAsten_SOURCE_DIR}"
            -B "${BriefKAsten_BUILD_DIR}"
            -G "${CMAKE_GENERATOR}"
            -DCMAKE_INSTALL_PREFIX="${BriefKAsten_INSTALL_DIR}"
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_BUILD_PARALLEL_LEVEL="${DCMAKE_BUILD_PARALLEL_LEVEL}"
            -DBRIEFKASTEN_BUILD_EXAMPLES=OFF
            -DBRIEFKASTEN_BUILD_TESTS=OFF
            -DBRIEFKASTEN_USE_CXX23=OFF
            RESULT_VARIABLE CONFIGURE_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT CONFIGURE_RESULT EQUAL 0)
        message(FATAL_ERROR "Configure failed for BriefKAsten")
    endif ()

    # cmake build
    execute_process(COMMAND "${CMAKE_COMMAND}"
            --build "${BriefKAsten_BUILD_DIR}"
            --config "${CMAKE_BUILD_TYPE}"
            RESULT_VARIABLE BUILD_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Build failed for BriefKAsten")
    endif ()

    # cmake install
    execute_process(COMMAND "${CMAKE_COMMAND}"
            --install "${BriefKAsten_BUILD_DIR}"
            --config ${CMAKE_BUILD_TYPE}
            --prefix "${BriefKAsten_INSTALL_DIR}"
            RESULT_VARIABLE INSTALL_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "Install failed for BriefKAsten")
    endif ()

    file(WRITE "${BriefKAsten_INSTALL_DIR}/.valid" "${BriefKAsten_TAG}")
    set(BriefKAsten_VALID ON)
endif ()

list(PREPEND CMAKE_PREFIX_PATH "${BriefKAsten_INSTALL_DIR}")
find_package(MPI REQUIRED)
find_package(kassert::kassert REQUIRED CONFIG PATHS "${BriefKAsten_INSTALL_DIR}")
find_package(range-v3::range-v3 REQUIRED CONFIG PATHS "${BriefKAsten_INSTALL_DIR}")

add_library(BriefKAsten INTERFACE IMPORTED GLOBAL)
set_target_properties(BriefKAsten PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BriefKAsten_SOURCE_DIR}/src")
set_property(TARGET BriefKAsten PROPERTY INTERFACE_COMPILE_FEATURES cxx_std_20)
set_property(TARGET BriefKAsten APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS BRIEFKASTEN_CXX20)
target_link_libraries(BriefKAsten INTERFACE MPI::MPI_CXX kassert::kassert range-v3::range-v3)
add_library(BriefKAsten::BriefKAsten ALIAS BriefKAsten)
