cmake_minimum_required(VERSION 3.18)

find_package(Git)

set(KaGen_TAG main)
set(KaGen_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/kagen-src")
set(KaGen_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/kagen-build")
set(KaGen_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/kagen")
set(KaGen_VALID OFF)
if (EXISTS "${KaGen_INSTALL_DIR}/.valid")
    file(READ "${KaGen_INSTALL_DIR}/.valid" CONTENTS)
    string(STRIP "${CONTENTS}" CONTENTS)
    if (CONTENTS STREQUAL "${KaGen_TAG}")
        set(KaGen_VALID ON)
    endif ()
endif ()

if (NOT KaGen_VALID)

    # clear
    file(REMOVE_RECURSE "${KaGen_SOURCE_DIR}" "${KaGen_BUILD_DIR}" "${KaGen_INSTALL_DIR}")
    file(MAKE_DIRECTORY "${KaGen_SOURCE_DIR}")
    file(MAKE_DIRECTORY "${KaGen_BUILD_DIR}")
    file(MAKE_DIRECTORY "${KaGen_INSTALL_DIR}")

    # git clone
    execute_process(COMMAND ${GIT_EXECUTABLE} clone
            --depth 1
            --recurse-submodules
            --shallow-submodules
            --branch "${KaGen_TAG}"
            https://github.com/sebalamm/KaGen.git
            "${KaGen_SOURCE_DIR}"
            RESULT_VARIABLE CLONE_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT CLONE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone KaGen")
    endif ()

    # cmake configure
    execute_process(COMMAND ${CMAKE_COMMAND}
            -S "${KaGen_SOURCE_DIR}"
            -B "${KaGen_BUILD_DIR}"
            -G "${CMAKE_GENERATOR}"
            -DCMAKE_INSTALL_PREFIX="${KaGen_INSTALL_DIR}"
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL}"
            -DKAGEN_BUILD_TESTS=OFF
            -DKAGEN_BUILD_APPS=OFF
            -DKAGEN_BUILD_TOOLS=OFF
            -DKAGEN_BUILD_EXAMPLES=OFF
            -DKAGEN_USE_CGAL=OFF
            -DKAGEN_USE_SPARSEHASH=OFF
            -DKAGEN_USE_XXHASH=ON
            RESULT_VARIABLE CONFIGURE_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT CONFIGURE_RESULT EQUAL 0)
        message(FATAL_ERROR "Configure failed for KaGen")
    endif ()

    # cmake build
    execute_process(COMMAND "${CMAKE_COMMAND}"
            --build "${KaGen_BUILD_DIR}"
            --config "${CMAKE_BUILD_TYPE}"
            RESULT_VARIABLE BUILD_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Build failed for KaGen")
    endif ()

    # cmake install
    execute_process(COMMAND "${CMAKE_COMMAND}"
            --install "${KaGen_BUILD_DIR}"
            --config ${CMAKE_BUILD_TYPE}
            --prefix "${KaGen_INSTALL_DIR}"
            RESULT_VARIABLE INSTALL_RESULT
            OUTPUT_QUIET ERROR_QUIET)

    if (NOT INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "Install failed for KaGen")
    endif ()

    file(WRITE "${KaGen_INSTALL_DIR}/.valid" "${KaGen_TAG}")
    set(KaGen_VALID ON)
endif ()

list(PREPEND CMAKE_PREFIX_PATH "${KaGen_INSTALL_DIR}")
find_package(KaGen CONFIG QUIET)
