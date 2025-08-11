target_link_libraries(${TARGET_NAME} INTERFACE OpenMP::OpenMP_CXX)
target_link_libraries(${TARGET_NAME} INTERFACE stxxl)
target_include_directories(${TARGET_NAME} INTERFACE ${STXXL_INCLUDE_DIRS})
