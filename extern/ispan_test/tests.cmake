target_link_libraries(${TARGET_NAME} PRIVATE ispan)
target_compile_definitions(${TARGET_NAME} PRIVATE KASPAN_NORMALIZE)

if (KASPAN_MEMCHECK)
    set(TARGET_BASE_COMMAND valgrind
            --leak-check=full
            --show-leak-kinds=definite,possible
            --track-origins=yes
            --error-exitcode=1
            --quiet
            --suppressions=${PROJECT_SOURCE_DIR}/valgrind.supp
            ${TARGET_COMMAND})

    set(TARGET_COMMAND
            mpirun -np 3 ${TARGET_BASE_COMMAND} --mpi-sub-process &&
            mpirun -np 1 ${TARGET_BASE_COMMAND} --mpi-sub-process &&
            mpirun -np 7 ${TARGET_BASE_COMMAND} --mpi-sub-process)
endif ()
