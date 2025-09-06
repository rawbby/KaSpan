cmake_minimum_required(VERSION 3.18)

block()
    include(FetchContent)
    FetchContent_Declare(kamping GIT_REPOSITORY https://github.com/kamping-site/kamping.git GIT_TAG main)
    FetchContent_MakeAvailable(kamping)
endblock()
