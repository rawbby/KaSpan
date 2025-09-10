#pragma once

#include <util/Arithmetic.hpp>
#include <comm/MpiBasic.hpp>

#include <mpi.h>

using scc_id_t = u64;

constexpr auto scc_id_undecided = std::numeric_limits<scc_id_t>::max();
constexpr auto scc_id_singular  = scc_id_undecided - 1;

constexpr auto mpi_scc_id_t         = mpi_basic_type<scc_id_t>;
constexpr auto mpi_scc_id_reduction = MPI_MIN;
