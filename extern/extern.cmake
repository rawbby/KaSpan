include(FetchContent)

block()

    set(KASSERT_WARNINGS_ARE_ERRORS OFF CACHE BOOL "" FORCE)
    set(KASSERT_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    set(CMAKE_MESSAGE_LOG_LEVEL WARNING)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1)
    set(CMAKE_WARN_DEPRECATED OFF)
    set(CMAKE_CXX_EXTENSIONS ON)

    set(BUILD_DOC OFF)
    set(BUILD_SANDBOX OFF)
    set(BUILD_TESTS OFF)
    set(BUILD_STATIC_LIBS ON)
    set(BUILD_GMOCK OFF)
    set(BUILD_EXTRAS OFF)
    set(BUILD_TESTING OFF)
    set(BUILD_EXAMPLES OFF)
    set(BUILD_SHARED_LIBS OFF)

    set(SKIP_PORTABILITY_TEST OFF)
    set(SKIP_PERFORMANCE_COMPARISON OFF)
    set(JUST_INSTALL_CEREAL OFF)
    set(THREAD_SAFE OFF)
    set(WITH_WERROR OFF)
    set(CEREAL_INSTALL OFF)
    set(USE_BOOST OFF)
    set(INSTALL_GTEST OFF)

    include("${TARGET_DIR}/kamping.cmake")
    include("${TARGET_DIR}/stxxl.cmake")
    include("${TARGET_DIR}/KaGen.cmake")
    include("${TARGET_DIR}/BriefKAsten.cmake")

endblock()
