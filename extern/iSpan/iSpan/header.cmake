target_link_libraries(${TARGET_NAME} INTERFACE
        essential partition graph scc
        MPI::MPI_CXX)
