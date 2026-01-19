#pragma once

#include <kaspan/debug/debug.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/pp.hpp>

namespace kaspan {

namespace internal {
inline auto
allreduce_pivot(
  degree_t local_max) -> vertex_t
{
  auto const [p, u] = mpi_basic::allreduce_single(local_max, mpi_degree_t, mpi_degree_max_op);
  DEBUG_ASSERT_NE(p, std::numeric_limits<vertex_t>::min());
  DEBUG_ASSERT_NE(u, std::numeric_limits<vertex_t>::min());
  return u;
}
}

/// This pivot selection assumes that at least one vertex is undecided globally.
/// If no vertex is decided locally this should not be a problem.
/// This pivot selection finds the undecided global vertex with the heightest initial degree product.
/// This pivot selection ignores degree reduction due to decided neighbours.
template<part_concept Part>
auto
select_pivot_from_head(
  bidi_graph_part_view<Part> graph,
  vertex_t const*            scc_id) -> vertex_t
{
  auto const& part = *graph.part;
  degree_t    local_max{ .degree_product = std::numeric_limits<index_t>::min(), .u = std::numeric_limits<vertex_t>::min() };

  for (vertex_t k = 0; k < part.local_n(); ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const degree_product = integral_cast<index_t>(graph.outdegree(k)) * graph.indegree(k);
      if (degree_product > local_max.degree_product) [[unlikely]]
        local_max = degree_t{ degree_product, part.to_global(k) };
    }
  }

  auto const pivot = internal::allreduce_pivot(local_max);
  DEBUG_ASSERT(!part.has_local(pivot) || scc_id[part.to_local(pivot)] == scc_id_undecided);
  return pivot;
}

/// This pivot selection assumes that at least one vertex is undecided globally.
/// If no vertex is decided locally this should not be a problem.
/// This is an optimized pivot selection that profits from and up-to-date
/// degree arrays. It is not only faster but also more accurate.
template<part_concept Part>
auto
select_pivot_from_degree(
  Part const&     part,
  vertex_t const* scc_id,
  vertex_t const* outdegree,
  vertex_t const* indegree) -> vertex_t
{
  degree_t local_max{ .degree_product = std::numeric_limits<index_t>::min(), .u = std::numeric_limits<vertex_t>::min() };

  for (vertex_t k = 0; k < part.local_n(); ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const degree_product = integral_cast<index_t>(outdegree[k]) * indegree[k];
      if (degree_product > local_max.degree_product) [[unlikely]]
        local_max = degree_t{ degree_product, part.to_global(k) };
    }
  }

  auto const pivot = internal::allreduce_pivot(local_max);
  DEBUG_ASSERT(!part.has_local(pivot) || scc_id[part.to_local(pivot)] == scc_id_undecided);
  return pivot;
}

} // namespace kaspan
