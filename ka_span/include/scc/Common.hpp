#pragma once

#include <mpi.h>

#define SCC_ID_T MPI_UINT64_T
#define SCC_ID_REDUCE_OP MPI_MIN

#define SCC_ID_UNDECIDED UINT64_MAX
#define SCC_ID_SINGLE (UINT64_MAX - 1)
// #define SCC_ID_DOUBLE (UINT64_MAX - 2)
// #define SCC_ID_TRIPLE (UINT64_MAX - 3)
// #define SCC_ID_QUADRUPLE (UINT64_MAX - 4)
#define SCC_ID_MAX (UINT64_MAX - 2)
