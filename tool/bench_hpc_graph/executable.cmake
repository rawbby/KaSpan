target_link_libraries(${TARGET_NAME} PRIVATE
    hpc_graph
    scc
    util
    debug
    MPI::MPI_CXX
    OpenMP::OpenMP_CXX)
target_compile_definitions(${TARGET_NAME} PRIVATE KASPAN_STATISTIC)
