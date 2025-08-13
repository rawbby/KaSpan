target_include_directories(${TARGET_NAME} PRIVATE include)
target_link_libraries(${TARGET_NAME} PRIVATE MPI::MPI_CXX)
target_link_libraries(${TARGET_NAME} PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(${TARGET_NAME} PRIVATE util)
