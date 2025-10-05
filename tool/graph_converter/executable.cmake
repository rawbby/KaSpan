target_link_libraries(${TARGET_NAME} PRIVATE
        essential partition graph
        kamping::kamping
        scc stxxl::stxxl
        OpenMP::OpenMP_CXX)
