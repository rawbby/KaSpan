target_link_libraries(${TARGET_NAME} INTERFACE
        BriefKAsten::BriefKAsten
        stxxl::stxxl
        KaGen::KaGen
        MPI::MPI_CXX)

option(KASPAN_VALGRIND "Enable Valgrind integration" OFF)
if (KASPAN_VALGRIND)
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(VALGRIND QUIET valgrind)
    endif ()
    if (VALGRIND_FOUND)
        target_include_directories(${TARGET_NAME} INTERFACE ${VALGRIND_INCLUDE_DIRS})
        target_compile_definitions(${TARGET_NAME} INTERFACE KASPAN_VALGRIND=1)
        # Note: We are cautious about linking against ${VALGRIND_LIBRARIES} from pkg-config
        # because they often include internal Valgrind tool libraries (like libcoregrind)
        # which cause linker errors in normal applications.
        foreach (lib ${VALGRIND_LIBRARIES})
            if (NOT lib MATCHES "coregrind" AND NOT lib MATCHES "vex")
                target_link_libraries(${TARGET_NAME} INTERFACE ${lib})
            endif ()
        endforeach ()
    else ()
        find_path(VALGRIND_INCLUDE_DIR NAMES valgrind/valgrind.h)
        find_library(VALGRIND_LIBRARY NAMES valgrind)
        if (VALGRIND_INCLUDE_DIR)
            target_include_directories(${TARGET_NAME} INTERFACE ${VALGRIND_INCLUDE_DIR})
            target_compile_definitions(${TARGET_NAME} INTERFACE KASPAN_VALGRIND=1)
            if (VALGRIND_LIBRARY)
                target_link_libraries(${TARGET_NAME} INTERFACE ${VALGRIND_LIBRARY})
            endif ()
        else ()
            message(FATAL_ERROR "Valgrind headers not found but KASPAN_VALGRIND is ON")
        endif ()
    endif ()
endif ()

target_compile_definitions(${TARGET_NAME} INTERFACE $<$<CONFIG:Debug>:KASPAN_DEBUG>)
