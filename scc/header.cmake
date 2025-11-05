target_link_libraries(${TARGET_NAME} INTERFACE
        BriefKAsten::BriefKAsten
        stxxl::stxxl
        KaGen::KaGen
        MPI::MPI_CXX)
target_compile_definitions(${TARGET_NAME} INTERFACE
        $<$<CONFIG:Debug>:KASPAN_DEBUG>)
