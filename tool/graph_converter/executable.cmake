target_link_libraries(${TARGET_NAME} PRIVATE
        essential graph scc
        kamping::kamping
        OpenMP::OpenMP_CXX)
