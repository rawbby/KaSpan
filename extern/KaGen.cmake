cmake_minimum_required(VERSION 3.18)

block()

    set(KAGEN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(KAGEN_BUILD_APPS OFF CACHE BOOL "" FORCE)
    set(KAGEN_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(KAGEN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(KAGEN_USE_XXHASH OFF CACHE BOOL "" FORCE)
    set(KAGEN_WARNINGS_ARE_ERRORS OFF CACHE BOOL "" FORCE)
    set(INSTALL_KAGEN OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(KaGen GIT_REPOSITORY https://github.com/sebalamm/KaGen.git GIT_TAG master)
    FetchContent_MakeAvailable(KaGen)

endblock()
