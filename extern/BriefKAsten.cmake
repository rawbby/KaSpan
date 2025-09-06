cmake_minimum_required(VERSION 3.18)

block()
    include(FetchContent)
    FetchContent_Declare(BriefKAsten GIT_REPOSITORY https://github.com/niklas-uhl/BriefKAsten GIT_TAG main)
    FetchContent_MakeAvailable(BriefKAsten)
endblock()
