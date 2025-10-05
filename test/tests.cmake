target_link_libraries(${TARGET_NAME} PRIVATE
        essential partition graph scc
        kamping::kamping
        BriefKAsten::BriefKAsten
        KaGen::KaGen
        stxxl::stxxl)
target_compile_definitions(${TARGET_NAME} PRIVATE KASPAN_NORMALIZE)
