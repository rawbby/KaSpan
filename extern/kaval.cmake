cmake_minimum_required(VERSION 3.18)

block()

    set(BUILD_DIR "${CMAKE_BINARY_DIR}" CACHE STRING "" FORCE)

    FetchContent_Declare(kaval
            GIT_REPOSITORY https://github.com/niklas-uhl/kaval.git
            GIT_TAG main)
    FetchContent_MakeAvailable(kaval)

    option(KAVAL_MACHINE "the machine kaval is running on" "shared")

    python_target(kaval "${kaval_SOURCE_DIR}/run-experiments.py"
            --machine "${KAVAL_MACHINE}"
            --search-dirs "${PROJECT_SOURCE_DIR}/experiment")

endblock()
