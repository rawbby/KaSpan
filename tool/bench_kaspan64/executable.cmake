target_link_libraries(${TARGET_NAME} PRIVATE kaspan)
target_compile_definitions(${TARGET_NAME} PRIVATE KASPAN_STATISTIC KASPAN_INDEX64)
