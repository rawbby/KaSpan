target_link_libraries(${TARGET_NAME} INTERFACE
        essential partition
        stxxl::stxxl
        KaGen::KaGen)
