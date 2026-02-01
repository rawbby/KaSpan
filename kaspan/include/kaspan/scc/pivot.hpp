#pragma once

#include <kaspan/debug/debug.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
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
  return u;
}
}

/// This pivot selection assumes that at least one vertex is undecided globally.
/// If no vertex is decided locally this should not be a problem.
/// This pivot selection finds the undecided global vertex with the heightest initial degree product.
/// This pivot selection ignores degree reduction due to decided neighbours.
template<part_view_concept Part>
auto
select_pivot_from_head(
  bidi_graph_part_view<Part> graph,
  u64*                       is_undecided_storage) -> vertex_t
{
  auto     is_undecided = view_bits(is_undecided_storage, graph.part.local_n());
  degree_t local_max(std::numeric_limits<index_t>::min(), std::numeric_limits<vertex_t>::min());

  is_undecided.for_each(graph.part.local_n(), [&](auto k) {
    auto const degree_product = integral_cast<index_t>(graph.outdegree(k)) * graph.indegree(k);
    if (degree_product > local_max.degree_product) [[unlikely]] {
      local_max = degree_t(degree_product, graph.part.to_global(k));
    }
  });

  auto const pivot = internal::allreduce_pivot(local_max);
  DEBUG_ASSERT(!graph.part.has_local(pivot) || is_undecided.get(graph.part.to_local(pivot)));
  return pivot;
}

/// This pivot selection assumes that at least one vertex is undecided globally.
/// If no vertex is decided locally this should not be a problem.
/// This is an optimized pivot selection that profits from and up-to-date
/// degree arrays. It is not only faster but also more accurate.
template<part_view_concept Part = single_part_view>
auto
select_pivot_from_degree(
  Part            part,
  u64*            is_undecided_storage,
  vertex_t const* outdegree,
  vertex_t const* indegree) -> vertex_t
{
  auto     is_undecided = view_bits(is_undecided_storage, part.local_n());
  degree_t local_max{ std::numeric_limits<index_t>::min(), std::numeric_limits<vertex_t>::min() };

  is_undecided.for_each(part.local_n(), [&](auto k) {
    auto const degree_product = integral_cast<index_t>(outdegree[k]) * indegree[k];
    if (degree_product > local_max.degree_product) [[unlikely]] {
      local_max = degree_t(degree_product, part.to_global(k));
    }
  });

  auto const pivot = internal::allreduce_pivot(local_max);
  DEBUG_ASSERT(!part.has_local(pivot) || is_undecided.get(part.to_local(pivot)));
  return pivot;
}

} // namespace kaspan
