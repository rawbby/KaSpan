cmake_minimum_required(VERSION 3.18)

block()
    include(FetchContent)
    FetchContent_Declare(KaGen GIT_REPOSITORY https://github.com/sebalamm/KaGen.git GIT_TAG master)
    FetchContent_MakeAvailable(KaGen)
endblock()
