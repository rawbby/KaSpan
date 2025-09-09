cmake_minimum_required(VERSION 3.18)

block()

    cmake_policy(SET CMP0048 NEW)
    cmake_policy(SET CMP0069 OLD)
    cmake_policy(SET CMP0077 NEW)

    set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0069 OLD)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

    set(INTERPROCEDURAL_OPTIMIZATION OFF)

    set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_INCLUDE_INSTALL_DIRECTORY}")
    set(TLX_INSTALL_INCLUDE_DIR "${CMAKE_INCLUDE_INSTALL_DIRECTORY}")

    FetchContent_Declare(pasta_bitvector GIT_REPOSITORY https://github.com/pasta-toolbox/bit_vector.git GIT_TAG v1.0.1)
    FetchContent_MakeAvailable(pasta_bitvector)

    add_library(pasta::bitvector ALIAS pasta_bit_vector)

endblock()
