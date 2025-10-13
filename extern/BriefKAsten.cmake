cmake_minimum_required(VERSION 3.18)

block()

    set(BRIEFKASTEN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BRIEFKASTEN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BRIEFKASTEN_USE_CXX23 OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(BriefKAsten
            GIT_REPOSITORY https://github.com/niklas-uhl/BriefKAsten
            GIT_TAG main)

    FetchContent_MakeAvailable(BriefKAsten)

    # Print all (set) properties of a target
    function(print_target_properties tgt)
        if(NOT TARGET "${tgt}")
            message(FATAL_ERROR "Target '${tgt}' not found.")
        endif()

        execute_process(
                COMMAND ${CMAKE_COMMAND} --help-property-list
                OUTPUT_VARIABLE _all_props_raw
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REPLACE "\n" ";" _all_props "${_all_props_raw}")

        # Configs for <CONFIG>-suffixed properties
        set(_cfgs)
        if(CMAKE_CONFIGURATION_TYPES)
            set(_cfgs ${CMAKE_CONFIGURATION_TYPES})
        elseif(CMAKE_BUILD_TYPE)
            set(_cfgs ${CMAKE_BUILD_TYPE})
        else()
            set(_cfgs Debug Release RelWithDebInfo MinSizeRel)
        endif()

        foreach(_p IN LISTS _all_props)
            if(_p MATCHES "<CONFIG>")
                foreach(_c IN LISTS _cfgs)
                    string(REPLACE "<CONFIG>" "${_c}" _pc "${_p}")
                    get_target_property(_v "${tgt}" "${_pc}")
                    if(NOT _v STREQUAL "NOTFOUND" AND NOT _v STREQUAL "_v-NOTFOUND")
                        message("${tgt}:${_pc} = ${_v}")
                    endif()
                endforeach()
            else()
                get_target_property(_v "${tgt}" "${_p}")
                if(NOT _v STREQUAL "NOTFOUND" AND NOT _v STREQUAL "_v-NOTFOUND")
                    message("${tgt}:${_p} = ${_v}")
                endif()
            endif()
        endforeach()
    endfunction()

    # Usage:
    print_target_properties(BriefKAsten::BriefKAsten)

endblock()
