cmake_minimum_required(VERSION 3.18)

block()

    set(BRIEFKASTEN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BRIEFKASTEN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BRIEFKASTEN_USE_CXX23 ON CACHE BOOL "" FORCE)

    FetchContent_Declare(BriefKAsten GIT_REPOSITORY https://github.com/niklas-uhl/BriefKAsten GIT_TAG main)
    FetchContent_MakeAvailable(BriefKAsten)

endblock()
