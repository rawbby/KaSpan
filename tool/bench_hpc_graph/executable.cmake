target_link_libraries(${TARGET_NAME} PRIVATE
        hpc_graph)
target_compile_definitions(${TARGET_NAME} PRIVATE KASPAN_STATISTIC KASPAN_INDEX64)
target_include_directories(${TARGET_NAME} PRIVATE "${TARGET_DIR}/src")
