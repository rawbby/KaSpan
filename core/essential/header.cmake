target_link_libraries(${TARGET_NAME} INTERFACE
        kamping::kamping
        MPI::MPI_CXX)
target_compile_definitions(${TARGET_NAME} INTERFACE $<$<CONFIG:Debug>:KASPAN_DEBUG>)
