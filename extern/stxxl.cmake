cmake_minimum_required(VERSION 3.18)

block()

    cmake_policy(SET CMP0048 NEW)
    cmake_policy(SET CMP0069 OLD)
    cmake_policy(SET CMP0077 NEW)

    set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0069 OLD)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

    set(INTERPROCEDURAL_OPTIMIZATION OFF)

    set(BUILD_EXAMPLES OFF)
    set(BUILD_TESTS OFF)
    set(BUILD_EXTRAS OFF)
    set(USE_BOOST OFF)
    set(USE_VALGRIND OFF)
    set(USE_GCOV OFF)
    set(USE_TPIE OFF)
    set(BUILD_STATIC_LIBS ON)
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_SKIP_INSTALL_RULES ON)

    FetchContent_Declare(stxxl GIT_REPOSITORY https://github.com/stxxl/stxxl.git GIT_TAG 1.4.1)
    FetchContent_GetProperties(stxxl)
    if (NOT stxxl_POPULATED)
        FetchContent_Populate(stxxl)
        execute_process(COMMAND
                ${GIT_EXECUTABLE} -C "${stxxl_SOURCE_DIR}" apply --check "${TARGET_DIR}/stxxl.patch"
                RESULT_VARIABLE PATCH_ALREADY_APPLIED
                OUTPUT_QUIET ERROR_QUIET)
        if (PATCH_ALREADY_APPLIED EQUAL 0)
            execute_process(COMMAND ${GIT_EXECUTABLE} -C "${stxxl_SOURCE_DIR}" apply "${TARGET_DIR}/stxxl.patch")
        endif ()
        add_subdirectory(${stxxl_SOURCE_DIR} ${stxxl_BINARY_DIR})
    endif ()

    target_include_directories(stxxl INTERFACE
            $<BUILD_INTERFACE:${STXXL_INCLUDE_DIRS}>)

    # suppress compilation warnings for external target
    target_compile_options(stxxl PRIVATE
            $<$<CXX_COMPILER_ID:GNU>:-w>
            $<$<CXX_COMPILER_ID:Clang>:-w>
            $<$<CXX_COMPILER_ID:MSVC>:/W0>)

    add_library(stxxl::stxxl ALIAS stxxl)

endblock()
