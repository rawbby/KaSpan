target_link_libraries(${TARGET_NAME} INTERFACE
        # mpi wrapper + async
        kamping::kamping
        BriefKAsten::BriefKAsten

        # for distributed graph generation
        KaGen::KaGen

        # for single threaded graph conversion
        stxxl::stxxl

        # fundamental requirements
        MPI::MPI_CXX
        OpenMP::OpenMP_CXX)
