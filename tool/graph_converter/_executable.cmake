target_link_libraries(${TARGET_NAME} PRIVATE
        essential partition graph scc
        kamping::kamping
        OpenMP::OpenMP_CXX)
