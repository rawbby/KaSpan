#pragma once

#include <util/Arithmetic.hpp>
#include <comm/MpiBasic.hpp>

#include <mpi.h>
#include <limits>

using scc_id_t = u64;

constexpr auto scc_id_undecided = std::numeric_limits<scc_id_t>::max();
constexpr auto scc_id_singular  = scc_id_undecided - 1;

constexpr auto mpi_scc_id_t         = mpi_basic_type<scc_id_t>;
constexpr auto mpi_scc_id_reduction = MPI_MIN;

struct Degree
{
  u64 degree;
  u64 u;

  static auto max(Degree const& lhs, Degree const& rhs) -> Degree const& {
    return lhs.degree > rhs.degree or (lhs.degree == rhs.degree and lhs.u > rhs.u) ? lhs : rhs;
  }
};
