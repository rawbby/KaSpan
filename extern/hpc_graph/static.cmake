target_include_directories(${TARGET_NAME} PUBLIC include "${PROJECT_SOURCE_DIR}/scc/include")
target_link_libraries(${TARGET_NAME} PUBLIC
        MPI::MPI_CXX
        OpenMP::OpenMP_CXX)
target_compile_definitions(${TARGET_NAME} PUBLIC KASPAN_STATISTIC)
