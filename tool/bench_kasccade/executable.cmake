target_link_libraries(${TARGET_NAME} PRIVATE
        essential graph scc
        kamping::kamping
        KaGen::KaGen
        stxxl::stxxl
        OpenMP::OpenMP_CXX)
target_compile_definitions(${TARGET_NAME} PRIVATE KASPAN_STATISTIC)
