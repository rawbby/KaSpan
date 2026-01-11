target_link_libraries(${TARGET_NAME} INTERFACE
        BriefKAsten::BriefKAsten
        stxxl::stxxl
        KaGen::KaGen
        MPI::MPI_CXX)

option(KASPAN_CALLGRIND "Enable Valgrind Callgrind integration" OFF)
option(KASPAN_MEMCHECK "Enable Valgrind Memcheck integration" OFF)

if (KASPAN_CALLGRIND OR KASPAN_MEMCHECK)
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(VALGRIND QUIET valgrind)
    endif ()
    if (VALGRIND_FOUND)
        target_include_directories(${TARGET_NAME} INTERFACE ${VALGRIND_INCLUDE_DIRS})
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
            if (VALGRIND_LIBRARY)
                target_link_libraries(${TARGET_NAME} INTERFACE ${VALGRIND_LIBRARY})
            endif ()
        else ()
            message(FATAL_ERROR "Valgrind headers not found but KASPAN_CALLGRIND or KASPAN_MEMCHECK is ON")
        endif ()
    endif ()
endif ()

if (KASPAN_CALLGRIND)
    target_compile_definitions(${TARGET_NAME} INTERFACE KASPAN_CALLGRIND)
endif ()
if (KASPAN_MEMCHECK)
    target_compile_definitions(${TARGET_NAME} INTERFACE KASPAN_MEMCHECK)
endif ()

target_compile_definitions(${TARGET_NAME} INTERFACE $<$<CONFIG:Debug>:KASPAN_DEBUG>)
