target_link_libraries(${TARGET_NAME} PRIVATE kaspan absl::flat_hash_map)
target_compile_definitions(${TARGET_NAME} PRIVATE KASPAN_STATISTIC)
